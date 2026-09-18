CCX64 = x86_64-w64-mingw32-gcc

CFLAGS  = -Os -fno-asynchronous-unwind-tables -nostdlib
CFLAGS += -fno-stack-protector -fno-ident
CFLAGS += -fno-exceptions -mno-red-zone
CFLAGS += -fno-builtin -w -masm=intel
CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-labels=1

LDFLAGS = -Wl,-Tscripts/linker.ld,--no-seh,-e,start,--gc-sections,-s

OUTPUT_X64 = shellcode.x64.exe

all: x64

x64: clean
	@ echo "[*] Compiling test DLL..."
	@ $(CCX64) -shared -nostdlib -e DllMain file.c -o file.dll -luser32 -lkernel32
	@ echo "[*] Running nasm..."
	@ nasm -f win64 core/assembly/asmx64.asm -o asm.x64.o
	@ nasm -f win64 core/assembly/syscalls.x64.asm -o syscalls.x64.o
	@ echo "[*] Compiling loader..."
	@ $(CCX64) core/*.c asm.x64.o syscalls.x64.o -o $(OUTPUT_X64) $(CFLAGS) $(LDFLAGS) -Iincludes
	@ echo "[*] Extracting .text section..."
	@ x86_64-w64-mingw32-objcopy -O binary -j .text $(OUTPUT_X64) loader.bin
	@ echo "[*] Prepending loader to DLL..."
	@ cat loader.bin file.dll > shellcode.bin
	@ echo "[*] Done. shellcode.bin ready (loader + DLL)."

clean:
	@ rm -f asm.x64.o syscalls.x64.o $(OUTPUT_X64) loader.bin shellcode.bin file.dll
