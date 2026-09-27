#pragma once
#include "nt.h"

#define NTDLL_HASH                   0x22d3b5ed
#define KERNEL32_HASH                0x7040ee75
#define NTALLOCATEVIRTUALMEMORY_HASH 0x6793c34c
#define NTPROTECTVIRTUALMEMORY_HASH  0x082962c8
#define LDRLOADDLL_HASH              0x0307db23
#define LOADLIBRARYA_HASH   		 0x5fbff0fb
#define GETPROCADDRESS_HASH 		 0xcf31bb1f
#define NTFLUSHINSTRUCTIONCACHE_HASH 0x80183adf
#define NTCONTINUE_HASH 			 0x780a612c
#define RTLCAPTURECONTEXT_HASH 		 0x7733eed0
#define RTLADDFUNCTIONTABLE_HASH 	 0xbdb9f1ae
#define NTCREATESECTION_HASH         0xd02e20d0
#define NTMAPVIEWOFSECTION_HASH      0x231f196a
#define CREATEFILEA_HASH              0xeb96c5fa
#define KERNELBASE_HASH               0xa721952b
#define SETPROCESSVALIDCALLTARGETS_HASH 0xbb6970d6

#define DOS_HEADER(base)       ((PIMAGE_DOS_HEADER)(base))
#define NT_HEADERS(base)       ((PIMAGE_NT_HEADERS)((PBYTE)(base) + DOS_HEADER(base)->e_lfanew))
#define OPT_HEADER(base)       (&NT_HEADERS(base)->OptionalHeader)
#define FILE_HEADER(base)      (&NT_HEADERS(base)->FileHeader)
#define SECTION_HEADER(base)   ((PIMAGE_SECTION_HEADER)((PBYTE)&NT_HEADERS(base)->OptionalHeader + FILE_HEADER(base)->SizeOfOptionalHeader))
#define RVA2VA(type, base, rva) ((type)((PBYTE)(base) + (rva)))

#define CurrentProcess() ((HANDLE)-1)

typedef HMODULE (WINAPI *fnLoadLibraryA)(LPCSTR);
typedef FARPROC (WINAPI *fnGetProcAddress)(HMODULE, LPCSTR);
typedef BOOL (WINAPI *fnDllMain)(HINSTANCE, DWORD, LPVOID);

typedef struct _Loader {
	struct {
		fnNtProtectVirtualMemory  NtProtectVirtualMemory;
		fnNtFlushInstructionCache NtFlushInstructionCache;
		fnGetProcAddress          GetProcAddress;
		fnLoadLibraryA     	      LoadLibraryA;
		fnNtCreateSection 		  NtCreateSection;
		fnNtMapViewOfSection 	  NtMapViewOfSection;
		fnNtContinue         	  NtContinue;
		fnRtlCaptureContext       RtlCaptureContext;
		fnRtlAddFunctionTable     RtlAddFunctionTable;
		fnCreateFileA             CreateFileA;
	} Funcs;

	struct {
		HMODULE ntdll;
		HMODULE kernel32;
	} Modules;
} Loader;


typedef struct _SYSCALL_INFO {
	DWORD ssn;
	ULONG_PTR gadget;
} SYSCALL_INFO;

#define UP -32
#define DOWN 32

PVOID GetModule(DWORD ModuleHash);
PVOID GetProc(HMODULE dll, DWORD FuncHash);
DWORD find_ssn(PVOID address);
ULONG_PTR find_gadget(PVOID address);
SYSCALL_INFO prepare_syscall(PVOID address);

NTSTATUS do_syscall(ULONG_PTR a1, ULONG_PTR a2, ULONG_PTR a3, ULONG_PTR a4,
                    ULONG_PTR a5, ULONG_PTR a6, ULONG_PTR a7, ULONG_PTR a8,
                    ULONG_PTR a9, ULONG_PTR a10, DWORD ssn, ULONG_PTR gadget);

#define SC_GLUE(a, b) a##b
#define SC_GLUE2(a, b) SC_GLUE(a, b)

#define SC_COUNT(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define SC_NARGS(...) SC_COUNT(__VA_ARGS__,10,9,8,7,6,5,4,3,2,1)

#define SC_PAD_1(a1) (ULONG_PTR)(a1),0,0,0,0,0,0,0,0,0
#define SC_PAD_2(a1,a2) (ULONG_PTR)(a1),(ULONG_PTR)(a2),0,0,0,0,0,0,0,0
#define SC_PAD_3(a1,a2,a3) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),0,0,0,0,0,0,0
#define SC_PAD_4(a1,a2,a3,a4) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),0,0,0,0,0,0
#define SC_PAD_5(a1,a2,a3,a4,a5) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),(ULONG_PTR)(a5),0,0,0,0,0
#define SC_PAD_6(a1,a2,a3,a4,a5,a6) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),(ULONG_PTR)(a5),(ULONG_PTR)(a6),0,0,0,0
#define SC_PAD_7(a1,a2,a3,a4,a5,a6,a7) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),(ULONG_PTR)(a5),(ULONG_PTR)(a6),(ULONG_PTR)(a7),0,0,0
#define SC_PAD_8(a1,a2,a3,a4,a5,a6,a7,a8) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),(ULONG_PTR)(a5),(ULONG_PTR)(a6),(ULONG_PTR)(a7),(ULONG_PTR)(a8),0,0
#define SC_PAD_9(a1,a2,a3,a4,a5,a6,a7,a8,a9) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),(ULONG_PTR)(a5),(ULONG_PTR)(a6),(ULONG_PTR)(a7),(ULONG_PTR)(a8),(ULONG_PTR)(a9),0
#define SC_PAD_10(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) (ULONG_PTR)(a1),(ULONG_PTR)(a2),(ULONG_PTR)(a3),(ULONG_PTR)(a4),(ULONG_PTR)(a5),(ULONG_PTR)(a6),(ULONG_PTR)(a7),(ULONG_PTR)(a8),(ULONG_PTR)(a9),(ULONG_PTR)(a10)

#define SC_PAD(...) SC_GLUE2(SC_PAD_, SC_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define DO_SYSCALL(address, ...) do { \
	SYSCALL_INFO sysInfo = prepare_syscall((PVOID)(address)); \
	do_syscall(SC_PAD(__VA_ARGS__), sysInfo.ssn, sysInfo.gadget); \
} while(0)
