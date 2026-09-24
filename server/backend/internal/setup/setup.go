package setup

import (
	"bytes"
	"crypto/sha256"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"os"
	"time"
	"bufio"
	"strings"
	"ldrserver/internal/database"
	"ldrserver/internal/utils"
)

const fingerprintPath = "/etc/ldr/fingerprint.json"

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
	MachineID   string `json:"machine_id"`
	ProductID   string `json:"product_uuid"`
	BoardSerial string `json:"board_serial"`
	CPU         string `json:"cpu"`
	InstallID   string `json:"install_id"`
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
	fp.MachineID = hashField(machineid(), utils.FingerprintSalt)
	fp.ProductID = hashField(readTrim("/sys/class/dmi/id/product_uuid"), utils.FingerprintSalt)
	fp.BoardSerial = hashField(readTrim("/sys/class/dmi/id/board_serial"), utils.FingerprintSalt)
	fp.CPU = hashField(cpuFingerprint(), utils.FingerprintSalt)
	fp.InstallID = hashField(installID, utils.FingerprintSalt)
	return fp
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

// RunSetup runs as root: collects machine fingerprint and writes it to /etc/ldr/fingerprint.json.
func RunSetup() error {
	if os.Getuid() != 0 {
		return errors.New("[!] 'setup' must be run as root (sudo ./server setup)")
	}

	if err := os.MkdirAll("/etc/ldr", 0755); err != nil {
		return fmt.Errorf("failed creating /etc/ldr: %w", err)
	}

	fp := CollectMachineInfo()
	data, err := json.MarshalIndent(fp, "", "  ")
	if err != nil {
		return err
	}

	if err := os.WriteFile(fingerprintPath, data, 0644); err != nil {
		return fmt.Errorf("failed writing fingerprint: %w", err)
	}

	return nil
}

// RunRegister runs as a normal user: reads the fingerprint written by setup,
// creates local dirs, registers with the server, and writes the config + DB.
func RunRegister(user, pass, domain string) error {
	data, err := os.ReadFile(fingerprintPath)
	if err != nil {
		return fmt.Errorf("fingerprint not found — run 'sudo ./server setup' first: %w", err)
	}
	var fp MachineInfo
	if err := json.Unmarshal(data, &fp); err != nil {
		return fmt.Errorf("failed parsing fingerprint: %w", err)
	}

	configFile, dbPath, err := SetupFolder()
	if err != nil {
		return err
	}

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
		return errors.New("[!] Invalid credentials")
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

	passHash := hashField(pass, utils.FingerprintSalt)
	if err := database.InsertUser(dbPath, user, passHash); err != nil {
		return err
	}

	return nil
}
