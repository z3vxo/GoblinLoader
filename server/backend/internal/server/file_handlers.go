package server

import (
	"bytes"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"github.com/go-chi/chi/v5"
	"github.com/google/uuid"

	"ldrserver/internal/database"
	"ldrserver/internal/utils"
)

const maxUploadSize = 100 << 20

type peInfo struct {
	kind     string
	arch     string
	hasReloc bool
	isPE     bool
}

func filesDir() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	dir := filepath.Join(home, ".local", "share", "ldr", "files")
	if err := os.MkdirAll(dir, 0755); err != nil {
		return "", err
	}
	return dir, nil
}

// parsePE inspects a file on disk and classifies it as exe/dll/shellcode,
// detecting architecture and whether it carries base relocations.
func parsePE(path string) peInfo {
	info := peInfo{kind: "shellcode"}

	f, err := os.Open(path)
	if err != nil {
		return info
	}
	defer f.Close()

	var dos [64]byte
	if _, err := io.ReadFull(f, dos[:]); err != nil {
		return info
	}
	if dos[0] != 'M' || dos[1] != 'Z' {
		return info
	}

	lfanew := int64(binary.LittleEndian.Uint32(dos[0x3C:0x40]))
	if _, err := f.Seek(lfanew, io.SeekStart); err != nil {
		return info
	}

	var sig [4]byte
	if _, err := io.ReadFull(f, sig[:]); err != nil {
		return info
	}
	if sig != [4]byte{'P', 'E', 0, 0} {
		return info
	}
	info.isPE = true

	var coff [20]byte
	if _, err := io.ReadFull(f, coff[:]); err != nil {
		return info
	}
	machine := binary.LittleEndian.Uint16(coff[0:2])
	numSections := binary.LittleEndian.Uint16(coff[2:4])
	optSize := binary.LittleEndian.Uint16(coff[16:18])
	characteristics := binary.LittleEndian.Uint16(coff[18:20])

	switch machine {
	case 0x8664:
		info.arch = "x64"
	case 0x14c:
		info.arch = "x86"
	case 0xaa64:
		info.arch = "arm64"
	}

	if characteristics&0x2000 != 0 {
		info.kind = "dll"
	} else {
		info.kind = "exe"
	}

	secOff := lfanew + 4 + 20 + int64(optSize)
	if _, err := f.Seek(secOff, io.SeekStart); err != nil {
		return info
	}
	for i := 0; i < int(numSections); i++ {
		var sh [40]byte
		if _, err := io.ReadFull(f, sh[:]); err != nil {
			break
		}
		name := string(bytes.TrimRight(sh[0:8], "\x00"))
		sizeRaw := binary.LittleEndian.Uint32(sh[16:20])
		if name == ".reloc" && sizeRaw > 0 {
			info.hasReloc = true
		}
	}

	return info
}

func (s *Server) UploadFile(w http.ResponseWriter, r *http.Request) {
	campaignUUID := chi.URLParam(r, "campaignID")
	if campaignUUID == "" {
		utils.Return400(w, "missing campaign id")
		return
	}

	r.Body = http.MaxBytesReader(w, r.Body, maxUploadSize)
	defer r.Body.Close()

	if err := r.ParseMultipartForm(8 << 20); err != nil {
		utils.Return400(w, "invalid multipart form")
		return
	}
	if r.MultipartForm != nil {
		defer r.MultipartForm.RemoveAll()
	}

	src, header, err := r.FormFile("file")
	if err != nil {
		utils.Return400(w, "missing 'file' field")
		return
	}
	defer src.Close()

	dir, err := filesDir()
	if err != nil {
		utils.Return500(w, "failed preparing storage")
		return
	}

	id := uuid.New().String()
	dstPath := filepath.Join(dir, id)

	dst, err := os.Create(dstPath)
	if err != nil {
		utils.Return500(w, "failed creating file")
		return
	}

	hash := sha256.New()
	size, err := io.Copy(io.MultiWriter(dst, hash), src)
	dst.Close()
	if err != nil {
		os.Remove(dstPath)
		utils.Return500(w, "failed writing file")
		return
	}

	info := parsePE(dstPath)

	kind := strings.ToLower(strings.TrimSpace(r.FormValue("kind")))
	switch kind {
	case "", "auto":
		kind = info.kind
	case "shellcode":
	case "exe", "dll":
		if !info.isPE {
			os.Remove(dstPath)
			utils.Return400(w, "file is not a valid PE")
			return
		}
		if info.kind != kind {
			os.Remove(dstPath)
			utils.Return400(w, fmt.Sprintf("file is a %s, not a %s", info.kind, kind))
			return
		}
	default:
		os.Remove(dstPath)
		utils.Return400(w, "invalid file type")
		return
	}

	arch := info.arch
	if kind == "shellcode" {
		arch = strings.ToLower(strings.TrimSpace(r.FormValue("arch")))
		if arch == "" {
			arch = "x64"
		}
	}
	if arch == "" {
		arch = "unknown"
	}
	if arch != "x64" && arch != "x86" {
		os.Remove(dstPath)
		utils.Return400(w, arch+" unsupported")
		return
	}

	file, err := s.DB.InsertFile(database.File{
		UUID:         id,
		CampaignUUID: campaignUUID,
		Name:         filepath.Base(header.Filename),
		Size:         size,
		Kind:         kind,
		Arch:         arch,
		HasReloc:     info.hasReloc,
		SHA256:       hex.EncodeToString(hash.Sum(nil)),
	})
	if err != nil {
		os.Remove(dstPath)
		utils.Return500(w, "failed saving file metadata")
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(file)
}

func (s *Server) ListFiles(w http.ResponseWriter, r *http.Request) {
	campaignUUID := chi.URLParam(r, "campaignID")
	files, err := s.DB.GetFiles(campaignUUID)
	if err != nil {
		utils.Return500(w, "failed getting files from db")
		return
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(files)
}

func (s *Server) DeleteFile(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if err := s.DB.DeleteFile(id); err != nil {
		utils.Return500(w, "failed deleting file")
		return
	}

	if dir, err := filesDir(); err == nil {
		os.Remove(filepath.Join(dir, id))
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"msg": "Succesfully deleted file"})
}
