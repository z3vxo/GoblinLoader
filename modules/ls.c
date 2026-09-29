#include <windows.h>
#include "common.h"

typedef struct _Module {
	DWORD size;
	DWORD Version;

	void (WINAPI *ModuleWrite4)(PVOID ctx, DWORD val);
	void (WINAPI *ModuleWrite8)(PVOID ctx, ULONGLONG val);
	void (WINAPI *ModuleWriteStr)(PVOID ctx, PCHAR str, DWORD len);

	HMODULE (WINAPI *ModuleGetModule)(DWORD hash);
	FARPROC (WINAPI *ModuleGetProc)(HMODULE mod, DWORD hash);
	HMODULE (WINAPI *ModuleLoadLibraryA)(PCHAR name);
	BOOL    (WINAPI *ModuleFreeLibrary)(HMODULE mod);
} Module, *pModule;


#define HASHED_kernel32            0x7040ee75
#define HASHED_FindFirstFileA      0xae2636cf
#define HASHED_FindNextFileA       0xf3b43c46
#define HASHED_CloseHandle         0x3870ca07
#define HASHED_pGetFileAttributes  0xcc9c6ccd
#define HASHED_GetFullPathNameA    0x3524e9e7


typedef HANDLE(WINAPI *pFindFirstFileA)(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFIleData);
typedef BOOL(WINAPI *pFindNextFileA)(HANDLE lpFindFile, LPWIN32_FIND_DATAA lpFindFIleData);
typedef DWORD(WINAPI *pGetFullPathNameA)(LPCSTR lpFileName, DWORD nBufferLenght, LPCSTR lpBuffer, LPCSTR *lpFilePart);
typedef DWORD(WINAPI *pGetFileAttributesA)(LPCSTR lpFileName);
typedef BOOL(WINAPI *pCloseHandle)(HANDLE hObject);

#define END_SIG 0xFFFFFFFF
#define OUTPUT_LS 0x02


__attribute__((section(".text$B")))
BOOL WINAPI ModuleEntry(pModule api, PVOID ctx, PBYTE args, DWORD ArgLen) {
	HMODULE k32 = api->ModuleGetModule(HASHED_kernel32);
	if (!k32)
		return FALSE;

	
	pFindNextFileA      fnFindNextFileA      = (pFindNextFileA)api->ModuleGetProc(k32, HASHED_FindNextFileA);
	pFindFirstFileA     fnFindFirstFileA     = (pFindFirstFileA)api->ModuleGetProc(k32, HASHED_FindFirstFileA);
	pCloseHandle        fnCloseHandle        = (pCloseHandle)api->ModuleGetProc(k32, HASHED_CloseHandle);
	pGetFullPathNameA   fnGetFullPathNameA   = (pGetFullPathNameA)api->ModuleGetProc(k32, HASHED_GetFullPathNameA);
	pGetFileAttributesA fnGetFileAttributesA = (pGetFileAttributesA)api->ModuleGetProc(k32, HASHED_pGetFileAttributes);

	PCHAR Dir = (PCHAR)args;
	CHAR Path[MAX_PATH];
	DWORD PathSize = fnGetFullPathNameA(Dir, MAX_PATH, Path, NULL);
	DWORD Attrs = fnGetFileAttributesA(Dir);
	BOOL file = (Attrs != INVALID_FILE_ATTRIBUTES) && !(Attrs & FILE_ATTRIBUTE_DIRECTORY);
	if(!file) {
		Path[PathSize] = '\\';
		Path[++PathSize] = '*';
		Path[++PathSize] = '\0';
	}

	WIN32_FIND_DATAA fData = {0};
	HANDLE hFind = fnFindFirstFileA(Path, &fData);
	if(!hFind || hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD EntryType = 0;
	CHAR dot1[3];
	CHAR dot2[2];

	api->ModuleWrite4(ctx, OUTPUT_LS);

	// PIC, cant use string literals as cbf dealing with .rdata
	dot1[0] = '.';
	dot1[1] = '.';
	dot1[2] = '\0';
	dot2[0] = '.';
	dot2[1] = '\0';
	do {

		if(pStrcmp(fData.cFileName, dot1) == 0 || pStrcmp(fData.cFileName, dot2) == 0)  {
			continue;
		}
		if(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			EntryType = 1;
		}
		else if(fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			EntryType = 3;
		} else {
			EntryType = 2;
		}

		UINT Len = (UINT)mStrLen(fData.cFileName);
		api->ModuleWrite4(ctx, Len);
		api->ModuleWriteStr(ctx, fData.cFileName, Len);
		api->ModuleWrite4(ctx, EntryType);
		ULONGLONG size = ((ULONGLONG)fData.nFileSizeHigh << 32) | fData.nFileSizeLow;
		api->ModuleWrite8(ctx, size);
	} while(fnFindNextFileA(hFind, &fData));

	api->ModuleWrite4(ctx, END_SIG);

	return TRUE;
}
