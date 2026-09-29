CCX64   = x86_64-w64-mingw32-gcc
NASM    = nasm
OBJCOPY = x86_64-w64-mingw32-objcopy

CFLAGS  = -Os -fno-asynchronous-unwind-tables -nostdlib
CFLAGS += -fno-stack-protector -fno-ident
CFLAGS += -fno-exceptions -mno-red-zone
CFLAGS += -fno-builtin -w -masm=intel
CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-labels=1

LDFLAGS = -Wl,-Tlinker.ld,--no-seh,-e,module_start,--gc-sections,-s

MODULES = $(patsubst %.c,%,$(wildcard *.c))
OUTDIR  = output
INFO ?=
ARGS ?=

all: ls.bin

common.o: common.asm
	@ echo "[*] Assembling common stub..."
	@ $(NASM) -f win64 common.asm -o common.o

%.bin: %.c common.o linker.ld common.h
	@ echo "[*] Compiling $* module..."
	@ $(CCX64) $< common.o -o $*.exe $(CFLAGS) $(LDFLAGS)
	@ echo "[*] Extracting .text -> $*.bin"
	@ $(OBJCOPY) -O binary -j .text $*.exe $@

# make <module> INFO="..." ARGS="..."  ->  compile + pack into output/<module>.mod
$(MODULES): %: $(OUTDIR)/%.mod

$(OUTDIR)/%.mod: %.bin ../scripts/packer.py
	@ echo "[*] Packing $* -> $@"
	@ python3 ../scripts/packer.py $< "$(INFO)" "$(ARGS)" $@
	@ echo "[*] Cleaning intermediates..."
	@ rm -f $*.exe $*.bin common.o

clean:
	@ rm -rf $(OUTDIR)
	@ rm -f *.o *.exe *.bin

.PHONY: all clean $(MODULES)
