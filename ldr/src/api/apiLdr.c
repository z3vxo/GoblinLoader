#include "../../includes/core/core.h"
#include "../../includes/api/apis.h"
#include "../../includes/core/nt.h"

#define HASH_SEED 5381

DWORD HashStringA(const char *str) {
    UINT32 hash = HASH_SEED;
    while (*str)
        hash = ((hash << 5) + hash) + (unsigned char)*str++;
    return hash;
}

DWORD HashStringW(const wchar_t *str) {
    UINT32 hash = HASH_SEED;
    while (*str) {
        unsigned char c = (unsigned char)*str++;
        if (c >= 'A' && c <= 'Z')
            c += 0x20;
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}


HMODULE GetModule(DWORD Hash) {
    PPEB peb = GetPeb();
    PEB_LDR_DATA* ldr = peb->Ldr;
    LIST_ENTRY* modules = NULL;
    modules = &ldr->InMemoryOrderModuleList;
    LIST_ENTRY* start = modules->Flink;

    for (LIST_ENTRY* List = start; List != modules; List = List->Flink) {
        LDR_DATA_TABLE_ENTRY* entry = (LDR_DATA_TABLE_ENTRY*)((BYTE*)List - sizeof(LIST_ENTRY));
        if (HashStringW(entry->BaseDllName.Buffer) == Hash) {
            return(HMODULE)entry->DllBase;
        }
    }
    return NULL;


}
FARPROC GetProc(HANDLE dll, DWORD Hash)
{
    static int recursionDepth = 0;
    if (recursionDepth > 6)
        return NULL;

    if (dll == NULL)
        return 0;

    recursionDepth++;

    uintptr_t dllAddress = (uintptr_t)dll;

    PIMAGE_NT_HEADERS       ntHeaders = (PIMAGE_NT_HEADERS)(dllAddress + ((PIMAGE_DOS_HEADER)dllAddress)->e_lfanew);
    PIMAGE_DATA_DIRECTORY   dataDirectory = (PIMAGE_DATA_DIRECTORY)&ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)(dllAddress + dataDirectory->VirtualAddress);

    uintptr_t exportedAddressTable = dllAddress + exportDirectory->AddressOfFunctions;
    uintptr_t namePointerTable = dllAddress + exportDirectory->AddressOfNames;
    uintptr_t ordinalTable = dllAddress + exportDirectory->AddressOfNameOrdinals;
    uintptr_t symbolAddress = 0;
    DWORD     procAddress = 0;

    DWORD dwCounter = exportDirectory->NumberOfNames;
    while (dwCounter--) {
        PUCHAR cpExportedFunctionName = (PUCHAR)(dllAddress + *(DWORD*)namePointerTable);
        if (HashStringA((const char*)cpExportedFunctionName) == Hash) {
            exportedAddressTable += (*(WORD*)ordinalTable * sizeof(DWORD));
            procAddress = *(DWORD*)exportedAddressTable;
            symbolAddress = dllAddress + procAddress;

            if (dataDirectory->VirtualAddress < procAddress && procAddress < dataDirectory->VirtualAddress + dataDirectory->Size) {
                char* symbol = (char*)(symbolAddress);
                char  moduleName[64] = { 0 };
                char  funcName[64] = { 0 };

                char* dot = LdrStrchr(symbol, '.');
                if (dot) {
                    int index = (int)(dot - symbol) + 1;
                    LdrMemcpy(moduleName, symbol, index);
                    moduleName[index] = 'd';
                    moduleName[index + 1] = 'l';
                    moduleName[index + 2] = 'l';
                    moduleName[index + 3] = 0;

                    LdrMemcpy(funcName, symbol + index, LdrStrlen(symbol) - index + 1);

                    BOOL isApiSetDll = FALSE;
                    if (LdrStrlen(moduleName) > 11 && LdrStrncmp(moduleName, "api-ms-win-", 11) == 0)
                        isApiSetDll = TRUE;
                    else if (LdrStrlen(moduleName) > 7 && LdrStrncmp(moduleName, "ext-ms-", 7) == 0)
                        isApiSetDll = TRUE;

                    HMODULE hForwardModule = ldr->win32->LoadLibraryA(moduleName);

                    if (hForwardModule) {
                        LPVOID result = NULL;

                        if (isApiSetDll) {
                            result = (LPVOID)ldr->win32->GetProcAddress(hForwardModule, funcName);
                        }
                        else {
                            ULONG hashFunc = HashStringA(funcName);
                            result = (LPVOID)GetProc(hForwardModule, hashFunc);
                        }

                        LdrMemset(moduleName, 0, LdrStrlen(moduleName));
                        LdrMemset(funcName, 0, LdrStrlen(funcName));

                        recursionDepth--;
                        return (FARPROC)result;
                    }

                    LdrMemset(moduleName, 0, LdrStrlen(moduleName));
                    LdrMemset(funcName, 0, LdrStrlen(funcName));
                }
                break;
            }
            else {
                recursionDepth--;
                return (FARPROC)symbolAddress;
            }
        }
        namePointerTable += sizeof(DWORD);
        ordinalTable += sizeof(WORD);
    }

    recursionDepth--;
    return NULL;
}