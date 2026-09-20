#pragma once
#include "nt.h"

#define NTDLL_HASH                   0x22d3b5ed
#define KERNEL32_HASH                0x7040ee75

#define NTALLOCATEVIRTUALMEMORY_HASH 0x6793c34c
#define NTPROTECTVIRTUALMEMORY_HASH  0x082962c8
#define NTFLUSHINSTRUCTIONCACHE_HASH 0x80183adf
#define HASHED_NtUnmapViewOfSection  0x595014ad
#define LDRLOADDLL_HASH              0x0307db23
#define LOADLIBRARYA_HASH   		 0x5fbff0fb
#define GETPROCADDRESS_HASH 		 0xcf31bb1f
#define TLSALLOC_HASH                0x8bf55163
#define TLSSETVALUE_HASH             0xc324eba1


#define DOS_HEADER(base)       ((PIMAGE_DOS_HEADER)(base))
#define NT_HEADERS(base)       ((PIMAGE_NT_HEADERS)((PBYTE)(base) + DOS_HEADER(base)->e_lfanew))
#define OPT_HEADER(base)       (&NT_HEADERS(base)->OptionalHeader)
#define FILE_HEADER(base)      (&NT_HEADERS(base)->FileHeader)
#define SECTION_HEADER(base)   ((PIMAGE_SECTION_HEADER)((PBYTE)&NT_HEADERS(base)->OptionalHeader + FILE_HEADER(base)->SizeOfOptionalHeader))
#define RVA2VA(type, base, rva) ((type)((PBYTE)(base) + (rva)))

#define CurrentProcess() ((HANDLE)-1)

typedef HMODULE (WINAPI *fnLoadLibraryA)(LPCSTR);
typedef FARPROC (WINAPI *fnGetProcAddress)(HMODULE, LPCSTR);
typedef DWORD (WINAPI *fnTlsAlloc)(void);
typedef BOOL  (WINAPI *fnTlsSetValue)(DWORD dwTlsIndex, LPVOID lpTlsValue);


typedef struct _Loader {
	struct {
		fnNtProtectVirtualMemory  NtProtectVirtualMemory;
		fnNtAllocateVirtualMemory NtAllocateVirtualMemory;
		pNtUnmapViewOfSection     NtUnmapViewOfSection;
		fnNtFlushInstructionCache NtFlushInstructionCache;
		fnGetProcAddress          GetProcAddress;
		fnLoadLibraryA     	      LoadLibraryA;
		fnTlsAlloc                TlsAlloc;
		fnTlsSetValue             TlsSetValue;
		
		
	} Funcs;

	struct {
		HMODULE ntdll;
		HMODULE kernel32;
	} Modules;
} Loader;


PVOID GetModule(DWORD ModuleHash);
PVOID GetProc(HMODULE dll, DWORD FuncHash);
PPEB GetPeb();

