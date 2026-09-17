#include "../includes/core.h"



__attribute__((section(".text$B")))
static BOOL is_clean_stub(PVOID address) {
	return *((PBYTE)address + 0) == 0x4C
		&& *((PBYTE)address + 1) == 0x8B
		&& *((PBYTE)address + 2) == 0xD1
		&& *((PBYTE)address + 3) == 0xB8;
}

__attribute__((section(".text$B")))
static ULONG_PTR find_syscall_instr(PVOID address) {
	for (DWORD i = 0; i < 32; i++) {
		if (*((PBYTE)address + i) == 0x0F && *((PBYTE)address + i + 1) == 0x05) {
			return (ULONG_PTR)((PBYTE)address + i);
		}
	}
	return 0;
}

__attribute__((section(".text$B")))
DWORD find_ssn(PVOID address) {
	if (is_clean_stub(address)) {
		return *(DWORD *)((PBYTE)address + 4);
	}

	if (*((PBYTE)address) == 0xE9
		|| *((PBYTE)address + 3) == 0xE9
		|| *((PBYTE)address + 8) == 0xE9
		|| *((PBYTE)address + 10) == 0xE9
		|| *((PBYTE)address + 12) == 0xE9)
	{
		for (WORD idx = 1; idx <= 500; idx++) {
			PVOID down = (PVOID)((PBYTE)address + idx * DOWN);
			PVOID up   = (PVOID)((PBYTE)address + idx * UP);

			if (is_clean_stub(down)) {
				return *(DWORD *)((PBYTE)down + 4) + idx;
			}

			if (is_clean_stub(up)) {
				return *(DWORD *)((PBYTE)up + 4) - idx;
			}
		}
	}

	return 0;
}

__attribute__((section(".text$B")))
ULONG_PTR find_gadget(PVOID address) {
	if (is_clean_stub(address)) {
		return find_syscall_instr(address);
	}

	if (*((PBYTE)address) == 0xE9
		|| *((PBYTE)address + 3) == 0xE9
		|| *((PBYTE)address + 8) == 0xE9
		|| *((PBYTE)address + 10) == 0xE9
		|| *((PBYTE)address + 12) == 0xE9)
	{
		for (WORD idx = 1; idx <= 500; idx++) {
			PVOID down = (PVOID)((PBYTE)address + idx * DOWN);
			PVOID up   = (PVOID)((PBYTE)address + idx * UP);

			if (is_clean_stub(down)) {
				return find_syscall_instr(down);
			}

			if (is_clean_stub(up)) {
				return find_syscall_instr(up);
			}
		}
	}

	return 0;
}

__attribute__((section(".text$B")))
SYSCALL_INFO prepare_syscall(PVOID address) {
	SYSCALL_INFO sysInfo = {0};

	sysInfo.ssn    = find_ssn(address);
	sysInfo.gadget = find_gadget(address);

	return sysInfo;
}
