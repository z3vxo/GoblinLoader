package database


import (
	"database/sql"
	_ "modernc.org/sqlite"
	"os"
)

type DB struct {
	conn *sql.DB
}


func GetDbPath() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	return fmt.Sprintf("%s/.local/share/ldr/database/app.db")
}


func SetupDB() {

}

func NewDB() (*DB, error) {
	path, err := GetDbPath()
	if err != nil {
		return nil, err
	}

	if _, err := os.Stat(path); os.IsNotExist(err) {
		SetupDB()
	}




}