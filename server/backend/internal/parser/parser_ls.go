package parser

import (
	"bytes"
	"fmt"
	"strings"
)

// lsEndSig mirrors END_SIG in src/modules/ls.c.
const lsEndSig = 0xFFFFFFFF

type LsEntry struct {
	Name string
	Size uint64
	Type int // 1 = dir, 2 = file, 3 = link
}

// ParseLS decodes the payload written by the ls module:
//
//	[file len:4][file str:N][entry type:4][size:8] ... [END_SIG:4]
func ParseLS(data []byte) ([]LsEntry, error) {
	r := NewReader(bytes.NewReader(data))
	entries := []LsEntry{}

	for {
		n := r.Read4()
		if r.Err() != nil {
			return nil, r.Err()
		}
		if n == lsEndSig {
			break
		}

		name := r.ReadN(int(n))
		typ := r.Read4()
		size := r.Read8()
		if r.Err() != nil {
			return nil, r.Err()
		}

		entries = append(entries, LsEntry{
			Name: string(name),
			Size: size,
			Type: int(typ),
		})
	}

	return entries, nil
}

func lsTypeName(t int) string {
	switch t {
	case 1:
		return "dir"
	case 3:
		return "link"
	default:
		return "file"
	}
}

func humanSize(n uint64) string {
	const unit = 1024
	if n < unit {
		return fmt.Sprintf("%d B", n)
	}
	div, exp := uint64(unit), 0
	for n/div >= unit && exp < 4 {
		div *= unit
		exp++
	}
	return fmt.Sprintf("%.1f %cB", float64(n)/float64(div), "KMGT"[exp])
}

// FormatLS renders entries as aligned "NAME | SIZE | TYPE" rows.
func FormatLS(entries []LsEntry) string {
	nameW, sizeW := len("NAME"), len("SIZE")
	for _, e := range entries {
		if l := len(e.Name); l > nameW {
			nameW = l
		}
		if l := len(humanSize(e.Size)); l > sizeW {
			sizeW = l
		}
	}

	var b strings.Builder
	header := fmt.Sprintf("%-*s | %-*s | %s", nameW, "NAME", sizeW, "SIZE", "TYPE")
	b.WriteString(header)
	b.WriteString("\n")
	b.WriteString(strings.Repeat("-", len(header)))

	for _, e := range entries {
		fmt.Fprintf(&b, "\n%-*s | %-*s | %s", nameW, e.Name, sizeW, humanSize(e.Size), lsTypeName(e.Type))
	}

	return b.String()
}
