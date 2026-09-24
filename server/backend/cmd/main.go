// Code generated secrets live in internal/{auth,utils,setup}/*_generated.go
//go:generate python3 ../gen_secrets.py

package main

import (
	"fmt"
	"log"
	"os"

	"ldrserver/internal/server"
	"ldrserver/internal/setup"
)

func main() {
	if len(os.Args) < 2 {
		usage()
		os.Exit(1)
	}

	switch os.Args[1] {
	case "setup":
		if err := setup.RunSetup(); err != nil {
			log.Fatal(err)
		}
		fmt.Println("[*] Fingerprint written to /etc/ldr/fingerprint.json")
		fmt.Println("[*] Run './server register <user> <pass> <domain>' next")

	case "register":
		if len(os.Args) < 5 {
			log.Fatalf("usage: %s register <user> <pass> <domain>", os.Args[0])
		}
		if err := setup.RunRegister(os.Args[2], os.Args[3], os.Args[4]); err != nil {
			log.Fatal(err)
		}
		fmt.Println("[*] Registration complete — run './server run' to start")

	case "run":
		s, err := server.New()
		if err != nil {
			log.Fatal(err)
		}

		if err := s.Parse(os.Args); err != nil {
			log.Fatal(err)
		}
		if err := s.Start(); err != nil {
			log.Fatal(err)
		}

	default:
		usage()
		os.Exit(1)
	}
}

func usage() {
	fmt.Fprintf(os.Stderr, "usage:\n")
	fmt.Fprintf(os.Stderr, "  sudo %s setup\n", os.Args[0])
	fmt.Fprintf(os.Stderr, "  %s register <user> <pass> <domain>\n", os.Args[0])
	fmt.Fprintf(os.Stderr, "  %s run\n", os.Args[0])
}
