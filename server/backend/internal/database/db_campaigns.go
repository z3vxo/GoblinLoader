package database

import (
	"database/sql"
	"github.com/google/uuid"
)

func (db *DB) DeleteCampaign(uuid string) error {
	res, err := db.conn.Exec(`DELETE FROM campaigns WHERE uuid = ?`, uuid)
	if err != nil {
		return err
	}
	n, _ := res.RowsAffected()
	if n == 0 {
		return sql.ErrNoRows
	}
	return nil
}

func (db *DB) InsertCampaign(name string) (*Campaign, error) {
	id := uuid.New().String()
	_, err := db.conn.Exec(
		`INSERT INTO campaigns (uuid, name) VALUES (?, ?)`,
		id, name,
	)
	if err != nil {
		return nil, err
	}

	var c Campaign
	err = db.conn.QueryRow(
		`SELECT uuid, name, created_at FROM campaigns WHERE uuid = ?`, id,
	).Scan(&c.UUID, &c.Name, &c.CreatedAt)
	if err != nil {
		return nil, err
	}
	return &c, nil
}

func (db *DB) GetCampaigns() (*CampaignsResp, error) {
	rows, err := db.conn.Query(`
		SELECT
			c.uuid,
			c.name,
			c.created_at,
			COUNT(DISTINCT a.id) AS total_agents,
			COUNT(DISTINCT f.id) AS total_files
		FROM campaigns c
		LEFT JOIN agents a ON a.campaign_uuid = c.uuid
		LEFT JOIN files  f ON f.campaign_uuid = c.uuid
		GROUP BY c.uuid
		ORDER BY c.created_at DESC
	`)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var campaigns []Campaign
	for rows.Next() {
		var c Campaign
		if err := rows.Scan(&c.UUID, &c.Name, &c.CreatedAt, &c.TotalAgents, &c.TotalFiles); err != nil {
			return nil, err
		}
		campaigns = append(campaigns, c)
	}

	if campaigns == nil {
		campaigns = []Campaign{}
	}

	return &CampaignsResp{
		Total:     len(campaigns),
		Campaigns: campaigns,
	}, nil
}
