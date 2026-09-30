#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"
#include "../../includes/parser/parser.h"

// Built-in commands are compiled into the agent image: no payload on the wire,
// no arena/cache slot, and they keep working even if the module subsystem
// (ole32 stomp + module arena) is unavailable. Keep them small and stable.

static void CmdWriteText(ParserWrite *p, PBYTE text, SIZE_T len) {
	ParserWrite4(p, OUTPUT_TEXT);
	ParserWriteRaw(p, text, len);
}

BOOL LdrRunCmd(LdrTask info, ParserWrite *p) {
	BOOL ok = FALSE;
	HMODULE k32 = ldr->modules->kernel32;

	if (info.Data && k32) {
		switch (HashStringA((const char *)info.Data)) {

		case HASHED_BUILTIN_PWD: {
			pGetCurrentDirectoryA getFn = (pGetCurrentDirectoryA)GetProc(k32, HASHED_GetCurrentDirectoryA);
			if (getFn) {
				CHAR buf[MAX_PATH];
				DWORD n = getFn(MAX_PATH, buf);
				if (n) {
					CmdWriteText(p, (PBYTE)buf, n);
					ok = TRUE;
				}
			}
			break;
		}

		case HASHED_BUILTIN_CD: {
			pSetCurrentDirectoryA setFn = (pSetCurrentDirectoryA)GetProc(k32, HASHED_SetCurrentDirectoryA);
			pGetCurrentDirectoryA getFn = (pGetCurrentDirectoryA)GetProc(k32, HASHED_GetCurrentDirectoryA);
			if (setFn) {
				PCHAR dir = info.args ? info.args : ".";
				if (setFn(dir)) {
					CHAR buf[MAX_PATH];
					DWORD n = getFn ? getFn(MAX_PATH, buf) : 0;
					if (n)
						CmdWriteText(p, (PBYTE)buf, n);
					else
						CmdWriteText(p, (PBYTE)"OK", 2);
				} else {
					PCHAR err = "failed to change directory: ";
					CmdWriteText(p, (PBYTE)err, LdrStrlen(err));
					if (info.args)
						ParserWriteRaw(p, (PBYTE)info.args, LdrStrlen(info.args));
				}
				ok = TRUE;
			}
			break;
		}

		default:
			break;
		}
	}

	if (info.Data)
		ldr->win32->LocalFree(info.Data);
	if (info.args)
		ldr->win32->LocalFree(info.args);

	return ok;
}
