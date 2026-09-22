package database

import (
	"ldrserver/internal/utils"
	"database/sql"
	_ "modernc.org/sqlite"

)


func (db *DB) VerifyCreds(user, pass string) (bool, error) {
    var storedHash string
    err := db.conn.QueryRow("SELECT password FROM users WHERE username = ?", user).Scan(&storedHash)
    if err == sql.ErrNoRows {
        return false, nil
    }
    if err != nil {
        return false, err
    }
     h := sha256.Sum256([]byte(utils.fingerprintsalt + pass))
     return fmt.Sprintf("%x", h) == storedHash, nil
}