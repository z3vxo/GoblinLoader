CCX64   = x86_64-w64-mingw32-gcc
NASM    = nasm
OBJCOPY = x86_64-w64-mingw32-objcopy

CFLAGS  = -Os -fno-asynchronous-unwind-tables -nostdlib
CFLAGS += -fno-stack-protector -fno-ident
CFLAGS += -fno-exceptions -mno-red-zone
CFLAGS += -fno-builtin -w -masm=intel
CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-labels=1

LDFLAGS = -Wl,-Tscripts/linker.ld,--no-seh,-e,start,--gc-sections,-s

PAYLOAD_SRC = ../test/test.c
PAYLOAD_EXE = payload.exe

OUTPUT_X64 = shellcode.x64.exe
LOADER_BIN = loader.bin
OUTPUT     = shellcode.bin

all: x64

x64: clean
	@ echo "[*] Running nasm..."
	@ $(NASM) -f win64 core/assembly/asmx64.asm -o asm.x64.o
	@ echo "[*] Compiling loader..."
	@ $(CCX64) core/loader.c core/utils.c asm.x64.o -o $(OUTPUT_X64) $(CFLAGS) $(LDFLAGS) -Iincludes
	@ echo "[*] Extracting .text section..."
	@ $(OBJCOPY) -O binary -j .text $(OUTPUT_X64) $(LOADER_BIN)
	@ echo "[*] Compiling payload EXE (fixed base, no .reloc)..."
	@ $(CCX64) $(PAYLOAD_SRC) -o $(PAYLOAD_EXE) -lkernel32 -luser32 -Wl,--disable-dynamicbase
	@ $(OBJCOPY) --remove-section=.reloc $(PAYLOAD_EXE)
	@ echo "[*] Prepending loader to EXE..."
	@ cat $(LOADER_BIN) $(PAYLOAD_EXE) > $(OUTPUT)
	@ echo "[*] Done. $(OUTPUT) ready (loader + EXE)."

clean:
	@ rm -f asm.x64.o $(OUTPUT_X64) $(LOADER_BIN) $(PAYLOAD_EXE) $(OUTPUT)

.PHONY: all x64 clean
