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
	return fmt.Sprintf("%s/.local/share/ldr/db/app.db", home), nil
}


func SetupDB(path string) (*sql.DB, error) {
	db, err := sql.Open("sqlite", path)
	if err != nil {
		return nil, err
	}

	if err := db.Ping(); err != nil {
		return nil, errors.New("[!] Failed Connecting to DB")
	}

	// Enable foreign key enforcement (off by default in SQLite)
	if _, err := db.Exec(`PRAGMA foreign_keys = ON`); err != nil {
		return nil, err
	}

	tables := []string{
		`CREATE TABLE IF NOT EXISTS users (
			id       INTEGER PRIMARY KEY AUTOINCREMENT,
			username TEXT    NOT NULL,
			password TEXT    NOT NULL
		)`,

		`CREATE TABLE IF NOT EXISTS campaigns (
			id         INTEGER  PRIMARY KEY AUTOINCREMENT,
			uuid       TEXT     NOT NULL UNIQUE,
			name       TEXT     NOT NULL,
			created_at DATETIME DEFAULT CURRENT_TIMESTAMP
		)`,

		`CREATE TABLE IF NOT EXISTS agents (
			id              INTEGER PRIMARY KEY AUTOINCREMENT,
			uuid            TEXT    NOT NULL UNIQUE,
			campaign_uuid   TEXT    NOT NULL REFERENCES campaigns(uuid) ON DELETE CASCADE,
			username        TEXT    NOT NULL,
			hostname        TEXT    NOT NULL,
			domain          TEXT    NOT NULL,
			process			TEXT    NOT NULL,
			architecture    TEXT    NOT NULL,
			isElev          INTEGER NOT NULL,
			country         TEXT    NOT NULL,
			last_seen       DATETIME DEFAULT CURRENT_TIMESTAMP
		)`,

		`CREATE TABLE IF NOT EXISTS files (
			id            INTEGER PRIMARY KEY AUTOINCREMENT,
			uuid          TEXT    NOT NULL UNIQUE,
			campaign_uuid TEXT    NOT NULL REFERENCES campaigns(uuid) ON DELETE CASCADE,
			name          TEXT    NOT NULL,
			size          INTEGER NOT NULL,
			kind          TEXT    NOT NULL DEFAULT '',
			arch          TEXT    NOT NULL DEFAULT '',
			has_reloc     INTEGER NOT NULL DEFAULT 0,
			sha256        TEXT    NOT NULL DEFAULT '',
			created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
		)`,

		`CREATE TABLE IF NOT EXISTS tasks (
			id            INTEGER PRIMARY KEY AUTOINCREMENT,
			agent_uuid    TEXT    NOT NULL REFERENCES agents(uuid) ON DELETE CASCADE,
			code          INTEGER NOT NULL,
			file_type     INTEGER NOT NULL,
			has_reloc     INTEGER NOT NULL DEFAULT 0,
			has_args      INTEGER NOT NULL DEFAULT 0,
			file_uuid     TEXT    REFERENCES files(uuid) ON DELETE CASCADE,
			args          TEXT,
			status        INTEGER NOT NULL DEFAULT 0,
			created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
		)`,

		`CREATE INDEX IF NOT EXISTS idx_tasks_agent_status ON tasks(agent_uuid, status)`,
	}

	for _, stmt := range tables {
		if _, err := db.Exec(stmt); err != nil {
			return nil, err
		}
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