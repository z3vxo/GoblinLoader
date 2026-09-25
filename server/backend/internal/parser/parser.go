package parser

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
)

const (
	CHECK_IN = 0xac
)

const maxFieldLen = 1 << 20

/*
  [CODE] 4 bytes
  [AGENT_ID_LEN] 4 bytes
  [AGENT_ID_STR] N bytes

  // if code == CODE_OUTPUT
  [OUTPUT_TYPE] 4 bytes
  ...
  rest of output
*/

type Reader struct {
	r   *bytes.Reader
	err error
}

func NewReader(r *bytes.Reader) *Reader {
	return &Reader{r: r}
}

func (r *Reader) Err() error {
	return r.err
}

func (r *Reader) Read4() uint32 {
	if r.err != nil {
		return 0
	}
	var val uint32
	r.err = binary.Read(r.r, binary.LittleEndian, &val)
	return val
}

func (r *Reader) ReadBytes() []byte {
	if r.err != nil {
		return nil
	}
	length := r.Read4()
	if r.err != nil {
		return nil
	}
	if length > maxFieldLen {
		r.err = fmt.Errorf("field too large: %d", length)
		return nil
	}
	buf := make([]byte, length)
	_, r.err = io.ReadFull(r.r, buf)
	if r.err != nil {
		return nil
	}
	return buf
}

func (r *Reader) ReadString() string {
	return string(r.ReadBytes())
}

func (r *Reader) GetCodeAndAgentID() (uint32, string) {
	code := r.Read4()
	agentID := r.ReadString()
	return code, agentID
}
