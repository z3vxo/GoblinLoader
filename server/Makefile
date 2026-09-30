BINARY  := server
BACKEND := backend
OUT     := $(BINARY)

.PHONY: build run secrets clean frontend all

## Default: npm install + build frontend + embed + build backend
all: frontend

## Build the backend binary (embeds static/ as-is)
build:
	cd $(BACKEND) && go build -ldflags="-s -w" -trimpath -o ../$(BINARY) ./cmd/main.go 
	@echo "[*] Built -> $(OUT)"

## Re-generate build-time secrets then build
secrets:
	cd $(BACKEND) && go generate ./cmd/
	cd $(BACKEND) && go build -ldflags="-s -w" -trimpath -o ../$(BINARY) ./cmd/main.go
	@echo "[*] Built with fresh secrets -> $(OUT)"

## Build the React frontend then embed it into the backend binary
frontend:
	cd frontend && npm install && npm run build
	rm -rf $(BACKEND)/internal/assets/static
	cp -r frontend/dist $(BACKEND)/internal/assets/static
	cd $(BACKEND) && go build -ldflags="-s -w" -trimpath -o ../$(BINARY) ./cmd/main.go 
	@echo "[*] Built -> $(OUT)"

## Run the server (pass ARGS for flags, e.g. make run ARGS="--addr 0.0.0.0 --port 8080")
run: build
	./$(OUT) run $(ARGS)

## Setup (requires root — collects machine fingerprint)
setup: build
	sudo ./$(OUT) setup

## Register operator account
## Usage: make register USER=admin PASS=secret DOMAIN=http://127.0.0.1:8081
register: build
	./$(OUT) register $(USER) $(PASS) $(DOMAIN)

clean:
	rm -f $(OUT)
	rm -rf $(BACKEND)/internal/assets/static
