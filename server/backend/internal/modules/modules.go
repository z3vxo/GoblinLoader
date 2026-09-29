package modules

import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
)

const (
	magic      = "LMOD"
	version    = 1
	headerSize = 64
	maxInfoLen = 1 << 20
)

// Info is the metadata section of a .mod container.
type Info struct {
	Name string `json:"name"`
	Info string `json:"info"`
	Args string `json:"args"`
}

type ListResp struct {
	Total   int    `json:"total"`
	Modules []Info `json:"modules"`
}

type header struct {
	infoOff uint32
	infoLen uint32
	codeOff uint32
	codeLen uint32
	size    uint64
}

// Dir returns the global (not campaign-scoped) modules directory.
func Dir() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	return filepath.Join(home, ".local", "share", "ldr", "modules"), nil
}

// List enumerates every valid .mod container in the modules directory. The
// filename is irrelevant — identity comes from the JSON info section — so each
// file is parsed and gated on the LMOD magic. Invalid files are skipped.
func List() (*ListResp, error) {
	dir, err := Dir()
	if err != nil {
		return nil, err
	}

	entries, err := os.ReadDir(dir)
	if errors.Is(err, os.ErrNotExist) {
		return &ListResp{Modules: []Info{}}, nil
	}
	if err != nil {
		return nil, err
	}

	mods := []Info{}
	for _, e := range entries {
		if e.IsDir() {
			continue
		}
		info, err := parse(filepath.Join(dir, e.Name()))
		if err != nil {
			continue
		}
		mods = append(mods, info)
	}

	return &ListResp{Total: len(mods), Modules: mods}, nil
}

// GetCode verifies the named module exists and returns its raw code section.
func GetCode(name string) ([]byte, error) {
	dir, err := Dir()
	if err != nil {
		return nil, err
	}

	path := filepath.Join(dir, name+".mod")
	if _, err := os.Stat(path); err != nil {
		if path, err = findByInfoName(dir, name); err != nil {
			return nil, err
		}
	}
	return readCode(path)
}

// findByInfoName falls back to matching the JSON info name when the file is not
// named <name>.mod.
func findByInfoName(dir, name string) (string, error) {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return "", err
	}
	for _, e := range entries {
		if e.IsDir() {
			continue
		}
		p := filepath.Join(dir, e.Name())
		info, err := parse(p)
		if err != nil {
			continue
		}
		if info.Name == name {
			return p, nil
		}
	}
	return "", fmt.Errorf("module not found: %s", name)
}

func readHeader(f *os.File) (header, error) {
	var h header
	var raw [headerSize]byte
	if _, err := io.ReadFull(f, raw[:]); err != nil {
		return h, errors.New("truncated header")
	}
	if string(raw[0:4]) != magic {
		return h, errors.New("bad magic")
	}
	if binary.LittleEndian.Uint16(raw[4:6]) != version {
		return h, errors.New("unsupported version")
	}

	h.infoOff = binary.LittleEndian.Uint32(raw[8:12])
	h.infoLen = binary.LittleEndian.Uint32(raw[12:16])
	h.codeOff = binary.LittleEndian.Uint32(raw[16:20])
	h.codeLen = binary.LittleEndian.Uint32(raw[20:24])

	st, err := f.Stat()
	if err != nil {
		return h, err
	}
	h.size = uint64(st.Size())
	return h, nil
}

// parse reads only the header + info JSON; the code section is left untouched.
func parse(path string) (Info, error) {
	var info Info

	f, err := os.Open(path)
	if err != nil {
		return info, err
	}
	defer f.Close()

	h, err := readHeader(f)
	if err != nil {
		return info, err
	}
	if h.infoLen == 0 || h.infoLen > maxInfoLen {
		return info, errors.New("bad info length")
	}
	if uint64(h.infoOff)+uint64(h.infoLen) > h.size {
		return info, errors.New("info section out of bounds")
	}

	buf := make([]byte, h.infoLen)
	if _, err := f.ReadAt(buf, int64(h.infoOff)); err != nil {
		return info, err
	}
	if err := json.Unmarshal(buf, &info); err != nil {
		return info, err
	}

	return info, nil
}

func readCode(path string) ([]byte, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	h, err := readHeader(f)
	if err != nil {
		return nil, err
	}
	if h.codeLen == 0 {
		return nil, errors.New("empty code section")
	}
	if uint64(h.codeOff)+uint64(h.codeLen) > h.size {
		return nil, errors.New("code section out of bounds")
	}

	buf := make([]byte, h.codeLen)
	if _, err := f.ReadAt(buf, int64(h.codeOff)); err != nil {
		return nil, err
	}
	return buf, nil
}
