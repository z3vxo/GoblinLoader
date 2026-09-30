package setup

import (
	"fmt"
	"os"
	"path/filepath"
)

func checkWritable(dir string) error {
	f, err := os.CreateTemp(dir, ".writetest")
	if err != nil {
		return fmt.Errorf("cannot write to %s — if a previous run was as root, remove it first:\n  sudo rm -rf %s", dir, filepath.Dir(dir))
	}
	f.Close()
	os.Remove(f.Name())
	return nil
}

func SetupFolder() (*os.File, string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return nil, "", err
	}

	dirPath := filepath.Join(home, ".local", "share", "ldr")

	dbDir := filepath.Join(dirPath, "db")
	if err := os.MkdirAll(dbDir, 0755); err != nil {
		return nil, "", err
	}
	if err := checkWritable(dbDir); err != nil {
		return nil, "", err
	}

	dbPath := filepath.Join(dbDir, "app.db")

	configDir := filepath.Join(dirPath, "config")
	if err := os.MkdirAll(configDir, 0755); err != nil {
		return nil, "", err
	}
	if err := checkWritable(configDir); err != nil {
		return nil, "", err
	}

	modulesDir := filepath.Join(dirPath, "modules")
	if err := os.MkdirAll(modulesDir, 0755); err != nil {
		return nil, "", err
	}

	utilsDir := filepath.Join(dirPath, "utils")
	if err := os.MkdirAll(utilsDir, 0755); err != nil {
		return nil, "", err
	}

	configFile, err := os.Create(filepath.Join(configDir, "config.json"))
	if err != nil {
		return nil, "", err
	}

	return configFile, dbPath, nil
}
