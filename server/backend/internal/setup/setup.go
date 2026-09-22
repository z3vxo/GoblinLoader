package setup


import (
	"os"
	"errors"
	"fmt"
	"encoding/json"
	"net/http"
	"time"
	"bytes"
	//"path/filepath"
	"bufio"
	"strings"
	"github.com/google/uuid"
	"ldrserver/internal/database"
	"ldrserver/internal/utils"
	"crypto/sha256"

)


func hashField(value, salt string) string {
    if value == "" {
        return ""
    }
    h := sha256.Sum256([]byte(salt + value))
    return fmt.Sprintf("%x", h)
}


type Resp struct {
	JwtToken string `json:"jwt"`
}

type TokenResp struct {
	Token string `json:"token"`
}

type User struct {
	User string `json:"username"`
	Pass string `json:"password"`
}


type MachineInfo struct {
	MachineID string `json:"machine_id"`
	ProductID string `json:"product_uuid"`
	BoardSerial string `json:"board_serial"`
	CPU string `json:"cpu"`
	InstallID string `json:"instal_id"`
}


func readTrim(path string) string {
	b, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	return strings.ToLower(strings.TrimSpace(string(b)))
}


func machineid() string {
	if id := readTrim("/etc/machine-id"); id != "" {
		return id
	}
	return readTrim("/var/lib/dbus/machine-id")
}

func CollectMachineInfo() MachineInfo {
	fp := MachineInfo{}
	fp.MachineID = hashField(machineid(), utils.fingerprintsalt) 
	fp.ProductID = hashField(readTrim("/sys/class/dmi/id/product_uuid"), utils.fingerprintsalt)
	fp.BoardSerial = hashField(readTrim("/sys/class/dmi/id/board_serial"), utils.fingerprintsalt)
	fp.CPU = hashField(cpuFingerprint(), utils.fingerprintsalt)
	fp.InstallID = hashField(genInstallID(), utils.fingerprintsalt)
	return fp;
}

func cpuFingerprint() string {
    model, cores := cpuInfo()
    if model == "" {
        return ""
    }
    return fmt.Sprintf("%s:%d", model, cores)
}


func cpuInfo() (model string, cores int) {
	f, err := os.Open("/proc/cpuinfo")
	if err != nil {
		return "", 0
	}
	defer f.Close()

	scanner := bufio.NewScanner(f)
	for scanner.Scan() {
		line := scanner.Text()
		if model == "" && strings.HasPrefix(line, "model name") {
			if parts := strings.SplitN(line, ":", 2); len(parts) == 2 {
				model = strings.TrimSpace(parts[1])
			}
		}
		if strings.HasPrefix(line, "processor") {
			cores++
		}
	}
	return model, cores
}


func genInstallID() string {
	id, err := uuid.NewRandom()
	if err != nil {
		return ""
	}
	return id.String()
}

func Run() error {
    configFile, dbPath, err := SetupFolder()
    if err != nil {
        return err
    }

    user := os.Getenv("USER")
    pass := os.Getenv("PASSWORD")
    domain := os.Getenv("DOMAIN")

    if user == "" || pass == "" || domain == "" {
        return errors.New("[!] Missing USERNAME, PASSWORD or DOMAIN env var, please check docs.txt")
    }

    // --- Login ---
    loginData, err := json.Marshal(User{User: user, Pass: pass})
    if err != nil {
        return err
    }

    client := &http.Client{Timeout: 10 * time.Second}

    loginReq, err := http.NewRequest("POST", domain+"/login", bytes.NewBuffer(loginData))
    if err != nil {
        return err
    }
    loginReq.Header.Set("Content-Type", "application/json")
    loginReq.Header.Set("Accept", "application/json")

    loginResp, err := client.Do(loginReq)
    if err != nil {
        return err
    }
    defer loginResp.Body.Close()

    if loginResp.StatusCode == http.StatusUnauthorized {
        return errors.New("[!] Invalid credentials, please ensure they match what you signed up with")
    }
    if loginResp.StatusCode != http.StatusOK {
        return fmt.Errorf("[!] Login failed with status %d", loginResp.StatusCode)
    }

    var tokenResp Resp
    if err := json.NewDecoder(loginResp.Body).Decode(&tokenResp); err != nil {
        return fmt.Errorf("[!] Failed to parse login response: %w", err)
    }
    if tokenResp.JwtToken == "" {
        return errors.New("[!] No JWT token in login response")
    }

    // --- Register machine ---
    fp := CollectMachineInfo()

    machineData, err := json.Marshal(fp)
    if err != nil {
        return err
    }

    regReq, err := http.NewRequest("POST", domain+"/register", bytes.NewBuffer(machineData))
    if err != nil {
        return err
    }
    regReq.Header.Set("Content-Type", "application/json")
    regReq.Header.Set("Accept", "application/json")
    regReq.Header.Set("Authorization", "Bearer "+tokenResp.JwtToken)

    regResp, err := client.Do(regReq)
    if err != nil {
        return err
    }
    defer regResp.Body.Close()

    if regResp.StatusCode != http.StatusOK {
        return fmt.Errorf("[!] Registration failed with status %d", regResp.StatusCode)
    }

    var token TokenResp 
	if err := json.NewDecoder(regResp.Body).Decode(&token); err != nil {
	    return fmt.Errorf("[!] Failed to parse registration response: %w", err)
	}

	if token.Token == "" {
	    return errors.New("[!] No token in registration response")
	}

	conf := struct {
	    Token string `json:"token"`
	}{Token: token.Token}

	confData, err := json.MarshalIndent(conf, "", "  ")
	if err != nil {
	    return err
	}

	if _, err := configFile.Write(confData); err != nil {
	    return err
	}

	_, err = database.SetupDB(dbPath)
	if err != nil {
		return err
	}

	passHash := hashField(pass, utils.fingerprintsalt)
	if err := database.InsertUser(dbPath, user, passHash); err != nil {
		return err
	}

    return nil
}
