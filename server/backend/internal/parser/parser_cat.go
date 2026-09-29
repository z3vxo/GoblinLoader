package parser

import "bytes"

// ParseCat decodes the payload written by the cat module:
//
//	[size:4][data:size]
func ParseCat(data []byte) ([]byte, error) {
	r := NewReader(bytes.NewReader(data))
	n := r.Read4()
	if r.Err() != nil {
		return nil, r.Err()
	}
	buf := r.ReadN(int(n))
	if r.Err() != nil {
		return nil, r.Err()
	}
	return buf, nil
}
