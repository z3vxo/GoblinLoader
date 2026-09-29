CCX64   = x86_64-w64-mingw32-gcc
NASM    = nasm
OBJCOPY = x86_64-w64-mingw32-objcopy

RLDR_DIR = src/rLdr
LDR_DIR  = src/ldr

# ── shared PIC flags ──
CFLAGS  = -Os -fno-asynchronous-unwind-tables -nostdlib
CFLAGS += -fno-stack-protector -fno-ident
CFLAGS += -fno-exceptions -mno-red-zone
CFLAGS += -fno-builtin -w -masm=intel
CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-labels=1

# ── rLdr (reflective loader → shellcode blob) ──
RLDR_LDFLAGS = -Wl,-T$(RLDR_DIR)/scripts/linker.ld,--no-seh,-e,start,--gc-sections,-s
RLDR_EXE     = $(RLDR_DIR)/shellcode.x64.exe
RLDR_BIN     = $(RLDR_DIR)/loader.bin

# ── ldr (payload DLL) ──
LDR_SRCS = $(shell find $(LDR_DIR)/src -name '*.c')
LDR_DLL  = $(LDR_DIR)/file.dll
LDR_LDFLAGS = -shared -Wl,--no-seh,--gc-sections,-s,-e,DllMain
LDR_LIBS =
LDR_DEBUG_LIBS = -lkernel32 -luser32

# ── output ──
OUTPUT = shellcode.bin

all: x64

# ── release target ──
x64: clean rldr _ldr merge

# ── debug target ──
x64-debug: clean rldr _ldr-debug merge

rldr:
	@ if [ -f $(RLDR_BIN) ]; then \
		echo "[*] loader.bin exists, skipping rLdr build"; \
	else \
		echo "[*] Assembling rLdr stubs..."; \
		$(NASM) -f win64 $(RLDR_DIR)/core/assembly/asmx64.asm -o $(RLDR_DIR)/asm.x64.o; \
		$(NASM) -f win64 $(RLDR_DIR)/core/assembly/syscalls.x64.asm -o $(RLDR_DIR)/syscalls.x64.o; \
		echo "[*] Compiling rLdr..."; \
		$(CCX64) $(RLDR_DIR)/core/*.c $(RLDR_DIR)/asm.x64.o $(RLDR_DIR)/syscalls.x64.o \
			-o $(RLDR_EXE) $(CFLAGS) $(RLDR_LDFLAGS) -I$(RLDR_DIR)/includes; \
		echo "[*] Extracting .text section..."; \
		$(OBJCOPY) -O binary -j .text $(RLDR_EXE) $(RLDR_BIN); \
	fi

# ── release ldr ──
_ldr:
	@ echo "[*] Compiling payload DLL..."
	@ $(CCX64) $(LDR_SRCS) -o $(LDR_DLL) $(CFLAGS) $(LDR_LDFLAGS) \
		-I$(LDR_DIR)/includes $(LDR_LIBS)

# ── debug ldr ──
_ldr-debug:
	@ echo "[*] Compiling payload DLL (DEBUG)..."
	@ $(CCX64) $(LDR_SRCS) -o $(LDR_DLL) $(CFLAGS) $(LDR_LDFLAGS) \
		-DDEBUG -I$(LDR_DIR)/includes $(LDR_DEBUG_LIBS)

merge:
	@ echo "[*] Merging loader + DLL..."
	@ python3 scripts/merge.py $(RLDR_BIN) $(LDR_DLL) $(OUTPUT)
	@ echo "[*] Done. $(OUTPUT) ready."

clean:
	@ rm -f $(RLDR_DIR)/asm.x64.o $(RLDR_DIR)/syscalls.x64.o
	@ rm -f $(RLDR_EXE) $(LDR_DLL) $(OUTPUT)

.PHONY: all x64 x64-debug \
	rldr _ldr _ldr-debug \
	merge clean
