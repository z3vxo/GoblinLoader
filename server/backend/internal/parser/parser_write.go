package parser

import (
	"bytes"
	"encoding/binary"
	"os"
	"path/filepath"
)

const (
	fileTypeExe = 0xac
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
		data, err := readTaskFile(t.FileUUID)
		if err != nil {
			return nil, err
		}

		w.Write4(uint32(t.Code))
		w.Write4(uint32(t.Id))
		w.Write4(uint32(t.FileType))
		if t.FileType == fileTypeExe {
			w.Write4(uint32(t.HasReloc))
		}
		if t.Code == taskModule {
			w.Write4(uint32(t.HasArgs))
		}
		w.Write4(uint32(len(data)))
		w.buf.Write(data)
		if t.HasArgs == 1 {
			w.WriteString(t.Args)
		}
	}

	w.Write4(taskNoTask)
	return w.Bytes(), nil
}
