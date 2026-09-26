package database

import (
	"crypto/sha256"
	"database/sql"
	"fmt"
	"ldrserver/internal/utils"
	"ldrserver/internal/parser"
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
		`SELECT uuid, username, hostname, domain, architecture, isElev, country, last_seen
		 FROM agents WHERE campaign_uuid = ?`, campaignUUID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var agents []Agent
	for rows.Next() {
		var a Agent
		if err := rows.Scan(&a.UUID, &a.Username, &a.Hostname, &a.Domain, &a.Arch, &a.IsElev, &a.Country, &a.LastSeen); err != nil {
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


func (db *DB) GetSingleAgentInfo(campId, agentID string) (*AgentInfo, error) {
	q := `SELECT username, hostname, domain, process, architecture, isElev, country FROM agents WHERE campaign_uuid = ? AND uuid = ?`

	info := &AgentInfo{}

	err := db.conn.QueryRow(q, campId, agentID).Scan(&info.Username, &info.Hostname, &info.Domain, &info.Process, &info.Arch, &info.IsElev, &info.Country)
	if err != nil {
		return nil, err
	}
	return info, nil

}


func (db *DB) InsertAgent(agent parser.AgentRegister) error {
	q := `INSERT INTO agents(uuid, campaign_uuid, username, hostname, domain, process, architecture, isElev, country)
	      VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)
	      ON CONFLICT(uuid) DO UPDATE SET
	          campaign_uuid = excluded.campaign_uuid,
	          username      = excluded.username,
	          hostname      = excluded.hostname,
	          domain        = excluded.domain,
	          process       = excluded.process,
	          architecture  = excluded.architecture,
	          isElev        = excluded.isElev,
	          country       = excluded.country,
	          last_seen     = CURRENT_TIMESTAMP`

	_, err := db.conn.Exec(q, agent.AgentID, agent.CampaignID, agent.Username, agent.Hostname, agent.Domain, agent.ProcessName, agent.Arch, agent.IsElev, agent.Country)
	return err
}


func (db *DB) DeleteAgent(id string) error {
    q := `DELETE FROM agents WHERE uuid = ?`
    _, err := db.conn.Exec(q, id)
    if err != nil {
        return err
    }
    return nil
}
