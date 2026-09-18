#include "../../includes/core/core.h"




void LdrExitThread(NTSTATUS code) {
	pRtlExitUserThread fnExit = ldr->win32->RtlExitUserThread;
	pLocalFree fnFree = ldr->win32->LocalFree;
	fnFree(ldr->config);
	fnFree(ldr->modules);
	fnFree(ldr->win32);
	fnFree(ldr);
	fnExit(code);
}