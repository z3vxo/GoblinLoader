#!/usr/bin/env bash
#
# setup.sh — install the toolchain required to build loaderProject.
#
# Pipeline / what each tool is for:
#   make                            drives every makefile in the repo
#   x86_64-w64-mingw32-gcc          compiles the PIC Windows payloads (rLdr, ldr,
#                                   modules, exemap, test harness)          -> mingw-w64
#   x86_64-w64-mingw32-objcopy      extracts the .text section to *.bin     -> mingw-w64
#   nasm                            assembles the x64 entry/syscall stubs
#   python3                         merge.py, packer.py, hasher.py, seed.py, gen_secrets.py
#   go (1.25.x)                     builds src/server/backend (pure-Go sqlite, no cgo)
#   node + npm                      builds src/server/frontend (Vite 8 / React 19)
#   sqlite3                         optional: inspect/seed ~/.local/share/ldr/db/app.db
#
set -euo pipefail

# --- pretty output ---------------------------------------------------------
info()  { printf '\033[1;34m[*]\033[0m %s\n' "$*"; }
ok()    { printf '\033[1;32m[+]\033[0m %s\n' "$*"; }
warn()  { printf '\033[1;33m[!]\033[0m %s\n' "$*"; }
die()   { printf '\033[1;31m[✗]\033[0m %s\n' "$*" >&2; exit 1; }

# --- privilege helper ------------------------------------------------------
if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    command -v sudo >/dev/null 2>&1 || die "not root and sudo is missing — re-run as root"
    SUDO="sudo"
fi

# --- distro detection (apt-based assumed) ----------------------------------
if command -v apt-get >/dev/null 2>&1; then
    PKG_MGR="apt"
elif command -v dnf >/dev/null 2>&1; then
    PKG_MGR="dnf"
elif command -v pacman >/dev/null 2>&1; then
    PKG_MGR="pacman"
else
    die "unsupported distro: no apt-get / dnf / pacman found"
fi

info "package manager: $PKG_MGR"

# distro-specific package names
case "$PKG_MGR" in
    apt)
        APT_PKGS=(
            make
            nasm
            mingw-w64          # x86_64 + i686 gcc/objcopy
            python3
            golang-go
            nodejs
            npm
            sqlite3            # optional, for the DB
            git
            curl
            ca-certificates
        )
        info "updating apt indexes..."
        $SUDO apt-get update -y
        info "installing: ${APT_PKGS[*]}"
        $SUDO apt-get install -y --no-install-recommends "${APT_PKGS[@]}"
        ;;
    dnf)
        DNF_PKGS=(
            make gcc nasm mingw64-gcc mingw64-binutils python3 golang nodejs npm
            sqlite git curl ca-certificates
        )
        info "installing: ${DNF_PKGS[*]}"
        $SUDO dnf install -y "${DNF_PKGS[@]}"
        ;;
    pacman)
        PAC_PKGS=(
            make nasm mingw-w64-gcc python go nodejs npm sqlite git curl ca-certificates
        )
        info "installing: ${PAC_PKGS[*]}"
        $SUDO pacman -Sy --needed --noconfirm "${PAC_PKGS[@]}"
        ;;
esac

# --- verification ----------------------------------------------------------
info "verifying toolchain..."

missing=0
check() {
    local cmd="$1" note="${2:-}"
    if command -v "$cmd" >/dev/null 2>&1; then
        printf '    %-32s %s\n' "$cmd" "$(command -v "$cmd")"
    else
        printf '\033[1;31m    %-32s MISSING\033[0m %s\n' "$cmd" "$note"
        missing=1
    fi
}

check make
check nasm
check x86_64-w64-mingw32-gcc
check x86_64-w64-mingw32-objcopy
check python3
check go
check node
check npm
check sqlite3 "optional — only needed to inspect the DB"

[ "$missing" -eq 0 ] || die "some required tools are still missing"

# --- version sanity (best effort) ------------------------------------------
info "versions:"
printf '    %-32s %s\n' "nasm"    "$(nasm -v 2>/dev/null | head -1)"
printf '    %-32s %s\n' "mingw"   "$(x86_64-w64-mingw32-gcc --version 2>/dev/null | head -1)"
printf '    %-32s %s\n' "python3" "$(python3 --version 2>/dev/null)"
printf '    %-32s %s\n' "go"      "$(go version 2>/dev/null)"
printf '    %-32s %s\n' "node"    "$(node --version 2>/dev/null)"
printf '    %-32s %s\n' "npm"     "$(npm --version 2>/dev/null)"

# Go: go.mod pins 1.25.0; a newer local Go will auto-fetch it (toolchain auto).
if command -v go >/dev/null 2>&1; then
    go_ver="$(go env GOVERSION 2>/dev/null || true)"
    case "$go_ver" in
        go1.2[0-9]*|go1.[3-9][0-9]*) : ;;
        *) warn "go $go_ver found; go.mod wants 1.25.0 — ensure GOTOOLCHAIN=auto or install Go 1.25+" ;;
    esac
fi

# Node: Vite 8 needs Node ^20.19 || >=22.12.
if command -v node >/dev/null 2>&1; then
    node_major="$(node -p 'process.versions.node.split(".")[0]' 2>/dev/null || echo 0)"
    if [ "$node_major" -lt 20 ]; then
        warn "node $(node --version) is too old for Vite 8 (need ^20.19 || >=22.12)."
        warn "install a newer Node, e.g. via NodeSource: https://github.com/nodesource/distributions"
    fi
fi

echo
ok "done. Build with:"
printf '    cd src && make x64                 # reflective loader shellcode blob\n'
printf '    cd src/server && make all          # C2 server + embedded frontend\n'
printf '    cd src/modules && make ls INFO="..." ARGS="..."   # pack a module\n'
