package setup


import (
	//"errors"
	"os"
	"path/filepath"
)

func SetupFolder() (*os.File, string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return nil, "", err
	}

	dirPath := filepath.Join(home, ".local", "share", "ldr")

	dbir := filepath.Join(dirPath, "db")
	if err := os.MkdirAll(dbir, 0755); err != nil {
		return nil, "", err
	}


	dbPath := filepath.Join(dbir, "app.db")
	dbFile, err := os.Create(dbPath)
	if err != nil {
		return nil, "", err
	}

	dbFile.Close()


	configDir := filepath.Join(dirPath, "config")
	if err := os.MkdirAll(configDir, 0755); err != nil {
		return nil, "", err
	}

	configFile, err := os.Create(filepath.Join(configDir, "config.json"))
	if err != nil {
		return nil, "", err
	}

	return configFile, dbPath, nil


}