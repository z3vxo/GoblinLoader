#pragma once

#include "../core/core.h"



typedef struct _LdrMemContext {
	PVOID BaseAddress;
} LdrMemContext;

#define FILE_EXE       0xac
#define FILE_DLL 	   0xab
#define FILE_SHELLCODE 0xad

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)


#define DOS_HEADER(base)       ((PIMAGE_DOS_HEADER)(base))
#define NT_HEADERS(base)       ((PIMAGE_NT_HEADERS)((PBYTE)(base) + DOS_HEADER(base)->e_lfanew))
#define OPT_HEADER(base)       (&NT_HEADERS(base)->OptionalHeader)
#define FILE_HEADER(base)      (&NT_HEADERS(base)->FileHeader)
#define SECTION_HEADER(base)   ((PIMAGE_SECTION_HEADER)((PBYTE)&NT_HEADERS(base)->OptionalHeader + FILE_HEADER(base)->SizeOfOptionalHeader))
#define RVA2VA(type, base, rva) ((type)((PBYTE)(base) + (rva)))

#define CurrentProcess() ((HANDLE)-1)

LdrInfo LdrPullFile();
BOOL LdrLoadAndRun(LdrInfo info, BOOL CleanUpAfter);
BOOL LdrRunExe(LdrInfo info);

BOOL   HasReloc(PBYTE pe);
DWORD  SectionCharsToProt(DWORD chars);
void   LdrCopySections(PVOID Base, PBYTE Raw, PIMAGE_SECTION_HEADER sec, WORD numSections);
BOOL   LdrProcessRelocs(PVOID Base, PBYTE Raw);
void   LdrProcessIAT(PVOID Base, PBYTE Raw);
void   LdrSetSectionPerms(PVOID Base, PIMAGE_SECTION_HEADER sec, WORD numSections);
