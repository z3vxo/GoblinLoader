#include "../includes/nt.h"
#include "../includes/core.h"


__attribute__((section(".text$B")))
PPEB GetPeb() {
	#if defined(_WIN64) || defined(__x86_64__)
    PPEB pPeb = NULL;
    pPeb = (PPEB)__readgsqword(0x60);
#else
    PPEB pPeb = NULL;
    __asm__ ("mov %0, fs:[0x30]" : "=r" (pPeb));
#endif
    return pPeb;
}

__attribute__((section(".text$B")))
PVOID GetModule(DWORD ModuleHash) {
	PPEB peb = GetPeb();
	PEB_LDR_DATA* ldr = peb->Ldr;
	LIST_ENTRY* modules = NULL;
	modules = &ldr->InMemoryOrderModuleList;
	LIST_ENTRY* start = modules->Flink;

	for(LIST_ENTRY* List = start; List != modules; List = List->Flink) {
		LDR_DATA_TABLE_ENTRY* entry = (LDR_DATA_TABLE_ENTRY*)((BYTE*)List - sizeof(LIST_ENTRY));
		if(HashStringW(entry->BaseDllName.Buffer) == ModuleHash) {
			return (HMODULE)entry->DllBase;
		}
	}
	return NULL;
}

__attribute__((section(".text$B")))
PVOID GetProc(HMODULE dll, DWORD ModuleHash) {
	PBYTE base = (PBYTE)dll;
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)base;

	if(pDos->e_magic != IMAGE_DOS_SIGNATURE) {
		return NULL;
	}

	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(base + pDos->e_lfanew);
	if(pNt->Signature != IMAGE_NT_SIGNATURE) {
		return NULL;
	}

	IMAGE_OPTIONAL_HEADER pOpt = pNt->OptionalHeader;

	PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(base + pOpt.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	PDWORD NameArray = (PDWORD)(base + pExportDir->AddressOfNames);
	PDWORD FuncArray = (PDWORD)(base + pExportDir->AddressOfFunctions);
	PWORD  OrdArray  = (PWORD) (base + pExportDir->AddressOfNameOrdinals);

	for(DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
		CHAR* name = (CHAR*)(base + NameArray[i]);
		if(HashStringA(name) == ModuleHash) {
			FARPROC Address = (FARPROC)(base + FuncArray[OrdArray[i]]);
			return Address;
		}
		
	}
	return NULL;
}
