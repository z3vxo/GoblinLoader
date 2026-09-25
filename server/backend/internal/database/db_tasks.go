package database

import (
	"database/sql"
)

type task struct {
	Id       int
	Code     int
	FileType int
	HasReloc int
	HasArgs  int
	FileUUID string
	Args     string
}

type Tasks struct {
	Total int
	Task  []task
}

func (db *DB) GetTasks(id string) (*Tasks, error) {
	rows, err := db.conn.Query(
		`SELECT id, code, file_type, has_reloc, has_args, file_uuid, args
		 FROM tasks WHERE agent_uuid = ? AND status = 0`, id)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var tasks []task
	for rows.Next() {
		var t task
		var fileUUID sql.NullString
		var args sql.NullString
		if err := rows.Scan(&t.Id, &t.Code, &t.FileType, &t.HasReloc, &t.HasArgs, &fileUUID, &args); err != nil {
			return nil, err
		}
		t.FileUUID = fileUUID.String
		t.Args = args.String
		tasks = append(tasks, t)
	}

	if tasks == nil {
		tasks = []task{}
	}

	return &Tasks{Total: len(tasks), Task: tasks}, nil
}
