package database

import (
	"crypto/sha256"
	"database/sql"
	"fmt"
	"ldrserver/internal/utils"
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
	h := sha256.Sum256([]byte(utils.FingerprintSalt + pass))
	return fmt.Sprintf("%x", h) == storedHash, nil
}



func (db *DB) GetAgents(campaignUUID string) (*Agents, error) {
	rows, err := db.conn.Query(
		`SELECT uuid, username, hostname, domain, architecture, country
		 FROM agents WHERE campaign_uuid = ?`, campaignUUID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var agents []Agent
	for rows.Next() {
		var a Agent
		if err := rows.Scan(&a.UUID, &a.Username, &a.Hostname, &a.Domain, &a.Arch, &a.Country); err != nil {
			return nil, err
		}
		agents = append(agents, a)
	}

	if agents == nil {
		agents = []Agent{}
	}

	return &Agents{
		Total:  len(agents),
		Agents: agents,
	}, nil
}


func (db *DB) DeleteAgent(id string) error {
    q := `DELETE FROM agents WHERE uuid = ?`
    _, err := db.conn.Exec(q, id)
    if err != nil {
        return err
    }
    return nil
}
