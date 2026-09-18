CCX64 = x86_64-w64-mingw32-gcc

CFLAGS  = -Os -fno-asynchronous-unwind-tables -nostdlib
CFLAGS += -fno-stack-protector -fno-ident
CFLAGS += -fno-exceptions -mno-red-zone
CFLAGS += -fno-builtin -w -masm=intel
CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-labels=1


OUTPUT_X64 = shellcode.bin

SRCS := $(wildcard src/**/*.c)
all: x64

x64: clean
	@ echo "[*] Compiling loader..."
	@ $(CCX64) $(SRCS) -o $(OUTPUT_X64) $(CFLAGS) -Iincludes

clean:
	@ rm -f loader.ll $(OUTPUT_X64)
