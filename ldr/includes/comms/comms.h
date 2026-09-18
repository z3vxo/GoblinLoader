#pragma once
#include <windows.h>

#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#define WINHTTP_NO_REFERER                  NULL
#define WINHTTP_DEFAULT_ACCEPT_TYPES        NULL
#define WINHTTP_NO_ADDITIONAL_HEADERS       NULL
#define WINHTTP_NO_REQUEST_DATA             NULL



BOOL NwLoadApis();
PVOID NwGetPayload(DWORD *PayloadSize, PBYTE PostBody, DWORD PostBodySize);