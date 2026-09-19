#include <windows.h>

typedef struct _Module {
	DWORD size;
	DWORD Version;

	void (WINAPI *ModuleWrite4)(PVOID ctx, DWORD val);
	void (WINAPI *ModuleWriteStr)(PVOID ctx, PCHAR str, DWORD len);

	HMODULE (WINAPI *ModuleGetModule)(DWORD hash);
	FARPROC (WINAPI *ModuleGetProc)(HMODULE mod, DWORD hash);
	HMODULE (WINAPI *ModuleLoadLibraryA)(PCHAR name);
	BOOL    (WINAPI *ModuleFreeLibrary)(HMODULE mod);
} Module, *pModule;

#define HASHED_user32  0x5a6bd3f3
#define HASHED_MessageBoxA 0x384f14b4

typedef int (WINAPI *pMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

BOOL WINAPI ModuleEntry(pModule api, PVOID ctx, PBYTE args, DWORD ArgLen) {
	HMODULE u32 = api->ModuleLoadLibraryA("user32.dll");
	if (!u32)
		return FALSE;

	pMessageBoxA MsgBox = (pMessageBoxA)api->ModuleGetProc(u32, HASHED_MessageBoxA);
	if (!MsgBox)
		return FALSE;

	MsgBox(NULL, "Hello from module", "Module", MB_OK);
	api->ModuleFreeLibrary(u32);
	return TRUE;
}
