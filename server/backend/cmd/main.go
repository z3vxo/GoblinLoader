package main

import (
	//"ldrserver/internal/server"
	"ldrserver/internal/setup"
	"os"
	"log"
	"fmt"
)

func main() {
	if len(os.Args) > 1 && os.Args[1] == "setup" {
	    if err := setup.Run(); err != nil {
	        log.Fatal(err)
	    }
	    fmt.Println("[*] Server setup, you can now visit site and log in using credentials!")
	    return
	}
}