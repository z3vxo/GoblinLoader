package parser

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
)


type AgentRegister struct {
	AgentID     string
	CampaignID  string
	Username    string
	Hostname    string
	Domain      string
	ProcessName string
	Country     string
	Arch			  string
	IsElev      int

}


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



/*

	[CAMPAIGN ID LEN] 4 bytes
	[CAMPAIGN ID ]    N BYTES
	[USERNAME LEN] 		4 BYTES
	[USERNAME STR] 		N BYTES
	[HOSTNAME LEN] 		4 BYTES
	[HOSTNAME STR] 		N BYTES
	[DOMAIN LEN]   		4 BYTES
	[DOMAIN STR]   		N BYTES
	[PROCESS LEN]     4 BYTES
	[PROCESS STR]     N BYTES
	[COUTRY LEN]      4 BYTES
	[COUTRY STR]      N BYTES
	[ARCH]       		  4 BYTES
	[IsAdmin]         4 BYTES
*/

func (r *Reader) ParseRegister(id string) (AgentRegister, error) {
	var agent AgentRegister
	agent.AgentID = id
	agent.CampaignID    = r.ReadString()
	agent.Username      = r.ReadString()
	agent.Hostname      = r.ReadString()
	agent.Domain        = r.ReadString()
	agent.ProcessName   = r.ReadString()
	agent.Country       = r.ReadString()
	arch := r.Read4()
	if(arch == 1) {
		agent.Arch = "x64"
	} else {
		agent.Arch = "x86"
	}
	agent.IsElev        = int(r.Read4())

	return agent, r.err

}

func (r *Reader) GetCodeAndAgentID() (uint32, string) {
	code := r.Read4()
	agentID := r.ReadString()
	return code, agentID
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
