package parser

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"path/filepath"

	"ldrserver/internal/modules"
)

const (
	fileTypeExe = 0xac
	taskLoad    = 0x1
	taskModule  = 0x3
	taskNoTask  = 0xff
)

type Task struct {
	Id       int
	Code     int
	FileType int
	HasReloc int
	HasArgs  int
	FileUUID string
	Module   string
	Args     string
}

type Tasks struct {
	Total int
	Task  []Task
}

type Writer struct {
	buf bytes.Buffer
}

func NewWriter() *Writer {
	return &Writer{}
}

func (w *Writer) Write4(v uint32) {
	binary.Write(&w.buf, binary.LittleEndian, v)
}

func (w *Writer) WriteString(s string) {
	w.Write4(uint32(len(s)))
	w.buf.WriteString(s)
}

func (w *Writer) Bytes() []byte {
	return w.buf.Bytes()
}

func readTaskFile(fileUUID string) ([]byte, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return nil, err
	}
	return os.ReadFile(filepath.Join(home, ".local", "share", "ldr", "files", fileUUID))
}

func (w *Writer) WriteTasks(tasks *Tasks) ([]byte, error) {
	for _, t := range tasks.Task {
		if err := w.writeTask(t); err != nil {
			return nil, err
		}
	}

	w.Write4(taskNoTask)
	return w.Bytes(), nil
}

func (w *Writer) writeTask(t Task) error {
	var data []byte
	var err error

	switch t.Code {
	case taskModule:
		data, err = modules.GetCode(t.Module)
	case taskLoad:
		data, err = readTaskFile(t.FileUUID)
	default:
		return fmt.Errorf("unknown task code %d", t.Code)
	}
	if err != nil {
		return err
	}

	w.Write4(uint32(t.Code))
	w.Write4(uint32(t.Id))
	w.Write4(uint32(t.FileType))

	if t.FileType == fileTypeExe {
		w.Write4(uint32(t.HasReloc))
	}

	hasArgs := 0
	if t.Code == taskModule && len(t.Args) > 0 {
		hasArgs = 1
	}
	if t.Code == taskModule {
		w.Write4(uint32(hasArgs))
	}

	w.Write4(uint32(len(data)))
	w.buf.Write(data)

	if hasArgs == 1 {
		w.WriteString(t.Args)
	}
	return nil
}
