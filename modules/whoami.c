#include <windows.h>
#include "common.h"


typedef NTSTATUS(NTAPI* pNtOpenProcessToken)(HANDLE hProc, ACCESS_MASK mask, PHANDLE TokenHandle);
typedef NTSTATUS(NTAPI* pNtQueryInformationToken)(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength);
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef BOOL(WINAPI *pConvertSidToStringSidA)(PSID Sid, LPSTR *StringSid);
typedef BOOL(WINAPI *pLookypAccountSidA)(LPCSTR lpSysName, PSID Sid, LPSTR Name, LPDWORD cchName, LPSTR ReferenceDomain, LPDWORD cchRefDomain, PSID_NAME_USE peUse);
typedef BOOL(WINAPI *pLookupPrivilegeNameA)(LPCSTR lpSysName, PLUID lpLuid, LPSTR lpName, LPDWORD cchName);

#define HASHED_LookupPrivilegeNameA 0xe6176fe8
#define HASHED_NtOpenProcessToken 0x7bd07459
#define HASHED_NtQueryInformationToken 0x2ce5a244
#define HASHED_LookupAccountSidA 0xbc518d2d
#define HASHED_ConvertSidToStringSidA 0x99a22dc1


#define OUTPUT_WHOAMI 0x05
#define END_SIG 0xFFFFFFFF

__attribute__((section(".text$B")))
BOOL WINAPI ModuleEntry(pModule api, PVOID ctx, PBYTE args, DWORD ArgLen) {
	HMODULE ntdll = api->ModuleGetModule(HASHED_ntdll);
	HMODULE vapi  = api->ModuleGetModule(HASHED_advapi32);
	pNtOpenProcessToken _NtOpenProcessToken = (pNtOpenProcessToken)api->ModuleGetProc(ntdll, HASHED_NtOpenProcessToken);
	pNtQueryInformationToken _NtQueryInformationToken = (pNtQueryInformationToken)api->ModuleGetProc(ntdll, HASHED_NtQueryInformationToken);

	pLookypAccountSidA _LookupAccountSidA = (pLookypAccountSidA)api->ModuleGetProc(vapi, HASHED_LookupAccountSidA);
	pConvertSidToStringSidA _ConvertSidToStringSidA = (pConvertSidToStringSidA)api->ModuleGetProc(vapi, HASHED_ConvertSidToStringSidA);
	pLookupPrivilegeNameA _LookupPrivlegeNameA = (pLookupPrivilegeNameA)api->ModuleGetProc(vapi, HASHED_LookupPrivilegeNameA);

	// A misspelled export hash resolves to NULL; bail instead of calling it.
	if(!ntdll || !vapi || !_NtOpenProcessToken || !_NtQueryInformationToken ||
	   !_LookupAccountSidA || !_ConvertSidToStringSidA || !_LookupPrivlegeNameA) {
		return FALSE;
	}




	PTOKEN_PRIVILEGES TokenPrivs = NULL;
	PCHAR buf 					 = NULL;
	HANDLE hToken 				 = NULL;
	PTOKEN_USER TokenUsr 		 = NULL;
	LPSTR SidStr 				 = NULL;

	CHAR nameBuf[256]   = {0};
	CHAR DomainBuf[256] = {0};
	DWORD NameLen 		= sizeof(nameBuf);
	DWORD DomainLen 	= sizeof(DomainBuf);
	DWORD TokenUsrSize  = 0;
	DWORD TokenPrivSize = 0;
	DWORD SidLen        = 0;
	SID_NAME_USE sidType;
	NTSTATUS Stat;


	Stat = _NtOpenProcessToken(CurrentProcess(), TOKEN_QUERY,&hToken);
	if(!NT_SUCCESS(Stat)) {
		return FALSE;
	}

	_NtQueryInformationToken(hToken, TokenUser, NULL, 0, &TokenUsrSize);
	TokenUsr = (PTOKEN_USER)api->ModuleAllocate(TokenUsrSize);

	Stat = _NtQueryInformationToken(hToken, TokenUser, TokenUsr, TokenUsrSize, &TokenUsrSize);

	if(!NT_SUCCESS(Stat)) {
		api->ModuleFree(TokenUsr);
		return FALSE;
	}

	if(!_ConvertSidToStringSidA(TokenUsr->User.Sid, &SidStr)) {
		api->ModuleFree(TokenUsr);
		return FALSE;
	}

	if(!_LookupAccountSidA(NULL, TokenUsr->User.Sid, nameBuf, &NameLen, DomainBuf, &DomainLen, &sidType)) {
		api->ModuleFree(TokenUsr);
		return FALSE;
	}

	_NtQueryInformationToken(hToken, TokenPrivileges, NULL, 0, &TokenPrivSize);
	TokenPrivs = (PTOKEN_PRIVILEGES)api->ModuleAllocate(TokenPrivSize);

	Stat = _NtQueryInformationToken(hToken, TokenPrivileges, TokenPrivs, TokenPrivSize, &TokenPrivSize);
	if(!NT_SUCCESS(Stat)) {
		api->ModuleFree(TokenUsr);
		api->ModuleFree(TokenPrivs);
		return FALSE;
	}

	api->ModuleWrite4(ctx, OUTPUT_WHOAMI);

	api->ModuleWrite4(ctx, NameLen);
	api->ModuleWriteStr(ctx, nameBuf, NameLen);
	api->ModuleWrite4(ctx, DomainLen);
	api->ModuleWriteStr(ctx, DomainBuf, DomainLen);
	SidLen = (DWORD)mStrLen(SidStr);
	api->ModuleWrite4(ctx, SidLen);
	api->ModuleWriteStr(ctx, SidStr, SidLen);

	for(DWORD i = 0; i < TokenPrivs->PrivilegeCount; i++) {
		LUID_AND_ATTRIBUTES Privs = TokenPrivs->Privileges[i];

		CHAR PrivName[256] = {0};
		DWORD PrivNameLen = sizeof(PrivName);
		DWORD PrivStatus = 0;

		if(!_LookupPrivlegeNameA(NULL, &Privs.Luid, PrivName, &PrivNameLen)) {
			continue;
		}

		if(Privs.Attributes & SE_PRIVILEGE_REMOVED) PrivStatus = PrivRemoved;
		else if (Privs.Attributes & SE_PRIVILEGE_ENABLED) PrivStatus = PrivEnabled;
		else if (Privs.Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) PrivStatus = PrivEnabledByDefault;
		else PrivStatus = PrivDisabled;


		api->ModuleWrite4(ctx, PrivNameLen);
		api->ModuleWriteStr(ctx, PrivName, PrivNameLen);
		api->ModuleWrite4(ctx, PrivStatus);


	}
	api->ModuleWrite4(ctx,END_SIG);

	if(TokenPrivs) {api->ModuleFree(TokenPrivs);}
	if(TokenUsr)   {api->ModuleFree(TokenUsr);}
	if(SidStr)     {api->ModuleFree(SidStr);}

	return TRUE;




}