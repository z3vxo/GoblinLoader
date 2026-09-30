#pragma once


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
    PVOID   (WINAPI *ModuleAllocate)(DWORD Size);
    void    (WINAPI *ModuleFree)(HLOCAL mem);
} Module, *pModule;

#define HASHED_kernel32            0x7040ee75
#define HASHED_ntdll               0x22d3b5ed
#define HASHED_advapi32            0x67208a49

#define CurrentProcess() ((HANDLE)-1)

/* Privilege state, emitted as one DWORD per privilege. Must match the server. */
#define PrivDisabled         0x00
#define PrivEnabled          0x01
#define PrivEnabledByDefault 0x02
#define PrivRemoved          0x03

static inline int pStrcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
    	s1++;
    	s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static inline SIZE_T mStrLen(const char *str) {
    const volatile char *s = (const volatile char *)str;
    SIZE_T n = 0;
    while (*s++) n++;
    return n;
}