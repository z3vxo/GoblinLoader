//go:build ignore

// test client: registers an agent against the checkin endpoint.
// build/run with an explicit filename so the ignore tag is bypassed:
//
//	go run test.go
//	go build -o testclient test.go
package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"

	"github.com/google/uuid"
)

const (
	codeRegister = 0xab
	campaignID   = "a5c729ea-d990-48e8-a497-6b59a3f8408a"
)

func write4(buf *bytes.Buffer, v uint32) {
	binary.Write(buf, binary.LittleEndian, v)
}

func writeStr(buf *bytes.Buffer, s string) {
	write4(buf, uint32(len(s)))
	buf.WriteString(s)
}

func main() {
	url := flag.String("url", "http://127.0.0.1:8081/rest/checkin", "checkin endpoint")
	agentID := flag.String("agent", uuid.NewString(), "agent uuid (default: random)")
	flag.Parse()

	// [code:4][agentID len:4][agentID]
	// [campaignID][username][hostname][domain][process][country][arch:4][isElev:4]
	pkt := &bytes.Buffer{}
	write4(pkt, codeRegister)
	writeStr(pkt, *agentID)
	writeStr(pkt, campaignID)
	writeStr(pkt, "tester")
	writeStr(pkt, "testhost")
	writeStr(pkt, "TESTDOMAIN")
	writeStr(pkt, "testclient.exe")
	writeStr(pkt, "US")
	write4(pkt, 1) // arch: 1 = x64
	write4(pkt, 0) // isElev

	resp, err := http.Post(*url, "application/octet-stream", bytes.NewReader(pkt.Bytes()))
	if err != nil {
		log.Fatalf("[!] request failed: %v", err)
	}
	defer resp.Body.Close()

	fmt.Printf("[*] agent:    %s\n", *agentID)
	fmt.Printf("[*] campaign: %s\n", campaignID)
	fmt.Printf("[*] response: %s\n", resp.Status)

	switch resp.StatusCode {
	case http.StatusOK:
		fmt.Println("[+] registered")
	default:
		os.Exit(1)
	}
}
