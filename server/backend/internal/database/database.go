package database


import (
	"database/sql"
	_ "modernc.org/sqlite"
	"os"
	"errors"
	"fmt"
)

type DB struct {
	conn *sql.DB
}


func GetDbPath() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	return fmt.Sprintf("%s/.local/share/ldr/database/app.db", home), nil
}


func SetupDB(path string) (*sql.DB, error) {
	db, err := sql.Open("sqlite", path)
	if err != nil {
		return nil, err
	}

	if err := db.Ping(); err != nil {
		return nil, errors.New("[!] Failed Connecting to DB")
	}

	usersTable := `CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL,
    password TEXT NOT NULL
);`



	_, err = db.Exec(usersTable)
	if err != nil {
		return nil, err
	}

	campaignTable := `CREATE TABLE IF NOT EXISTS campaigns(
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		name TEXT NOT NULL,
		created_at DATETIME DEFAULT CURRENT_TIMESTAMP
	);`

	_, err = db.Exec(campaignTable)
	if err != nil {
		return nil, err
	}

	agentTable := `CREATE TABLE IF NOT EXISTS agents(
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		uuid TEXT NOT NULL,
		username TEXT NOT NULL,
		hostname TEXT NOT NULL,
		domain   TEXT NOT NULL,
		architecure TEXT NOT NULL);`

	_, err = db.Exec(agentTable)
	if err != nil {
		return nil, err
	}

	return db, nil

}


func InsertUser(path, user, pass_hash string) error {
	db, err := sql.Open("sqlite", path);
	if err != nil {
		return err
	}

	q := `INSERT INTO users(username, password) VALUES(?, ?);`
	_, err = db.Exec(q, user, pass_hash)
	if err != nil {
		return err
	}
	return nil
}

func NewDB() (*DB, error) {
	path, err := GetDbPath()
	if err != nil {
		return nil, err
	}

	db, err := SetupDB(path)
	if err != nil {
		return nil, err
	}

	d := &DB{
		conn: db,
	}

	return d, nil




}