#include "../../includes/core/core.h"

LdrInstance *ldr = NULL;

BOOL ParseConfig() {
	return TRUE;
}

void LdrMain() {
	if(!LdrAllocateCoreStructsAndLoadApis()) {
		//LdrExitSafely();
	}

	if(!ParseConfig())
		return;
}