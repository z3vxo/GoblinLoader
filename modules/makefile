CCX64 = x86_64-w64-mingw32-gcc

CFLAGS  = -Os -fno-asynchronous-unwind-tables -nostdlib
CFLAGS += -fno-stack-protector -fno-ident
CFLAGS += -fno-exceptions -mno-red-zone
CFLAGS += -fno-builtin -w -masm=intel
CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-labels=1

LDFLAGS = -shared -Wl,--no-seh,--gc-sections,-s,--dynamicbase,-e,ModuleEntry

all: ls

ls: ls.dll

%.dll: %.c common.h
	@ echo "[*] Compiling $* module..."
	@ $(CCX64) $< -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	@ rm -f *.dll

.PHONY: all ls clean
