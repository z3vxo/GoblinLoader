#pragma once
#include <windows.h>


#define HASHED_LocalAlloc     0x73cebc5b
#define HASHED_LocalFree      0xa66df372
#define HASHED_LocalReAlloc   0xabad9db2
#define HASHED_GetProcAddress 0xcf31bb1f
#define HASHED_LoadLibraryA   0x5fbff0fb

#define HASHED_ntdll         0x22d3b5ed
#define HASHED_kernel32      0x7040ee75

typedef HLOCAL(WINAPI *pLocalAlloc)(UINT uFlags, SIZE_T uBytes);
typedef HLOCAL(WINAPI* pLocalReAlloc)(HLOCAL hMem, SIZE_T uBytes, UINT uFlags);
typedef HLOCAL(WINAPI *pLocalFree)(HLOCAL);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
typedef HMODULE(WINAPI* pLoadLibraryA)(LPCSTR dllName);


typedef struct _Win32 {
    // win32
    pLocalAlloc      LocalAlloc;
    pLocalFree       LocalFree;
    pLocalReAlloc    LocalReAlloc;
    pGetProcAddress  GetProcAddress;
    pLoadLibraryA    LoadLibraryA;
} Win32, *PWin32;

typedef struct _Modules {
    HMODULE ntdll;
    HMODULE kernel32;
    HMODULE winhttp;
} Modules;
