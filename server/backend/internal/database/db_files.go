package database

import "database/sql"

func (db *DB) InsertFile(f File) (*File, error) {
	hasReloc := 0
	if f.HasReloc {
		hasReloc = 1
	}

	_, err := db.conn.Exec(
		`INSERT INTO files (uuid, campaign_uuid, name, size, kind, arch, has_reloc, sha256)
		 VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
		f.UUID, f.CampaignUUID, f.Name, f.Size, f.Kind, f.Arch, hasReloc, f.SHA256,
	)
	if err != nil {
		return nil, err
	}

	if err := db.conn.QueryRow(
		`SELECT created_at FROM files WHERE uuid = ?`, f.UUID,
	).Scan(&f.CreatedAt); err != nil {
		return nil, err
	}

	return &f, nil
}


func (db *DB) ListFileMetadata(id string) (*FileListResp, error) {
	rows, err := db.conn.Query(
		`SELECT uuid, name FROM files WHERE campaign_uuid = ? ORDER BY created_at DESC`, id)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var files []FileResp
	for rows.Next() {
		var f FileResp
		if err := rows.Scan(&f.UUID, &f.Name); err != nil {
			return nil, err
		}
		files = append(files, f)
	}

	if files == nil {
		files = []FileResp{}
	}

	return &FileListResp{Total: len(files), Files: files}, nil
}

func (db *DB) GetFiles(campaignUUID string) (*Files, error) {
	rows, err := db.conn.Query(
		`SELECT uuid, campaign_uuid, name, size, kind, arch, has_reloc, sha256, created_at
		 FROM files WHERE campaign_uuid = ? ORDER BY created_at DESC`, campaignUUID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var files []File
	for rows.Next() {
		var f File
		var hasReloc int
		if err := rows.Scan(
			&f.UUID, &f.CampaignUUID, &f.Name, &f.Size, &f.Kind,
			&f.Arch, &hasReloc, &f.SHA256, &f.CreatedAt,
		); err != nil {
			return nil, err
		}
		f.HasReloc = hasReloc != 0
		files = append(files, f)
	}

	if files == nil {
		files = []File{}
	}

	return &Files{Total: len(files), Files: files}, nil
}

func (db *DB) DeleteFile(uuid string) error {
	res, err := db.conn.Exec(`DELETE FROM files WHERE uuid = ?`, uuid)
	if err != nil {
		return err
	}
	if n, _ := res.RowsAffected(); n == 0 {
		return sql.ErrNoRows
	}
	return nil
}
