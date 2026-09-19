#pragma once
#include <windows.h>


typedef struct ParserRead ParserRead;
typedef struct ParserWrite ParserWrite;

ParserRead *ParserInitRead(PBYTE buf, SIZE_T length);
UINT32 ParserRead4(ParserRead *p);
INT ParserReadStringInto(ParserRead *p, char *dst, SIZE_T bufSize);
char *ParserReadString(ParserRead *p);
void ParserFreeString(char *s);
PBYTE ParserReturnDataPointer();
void ParserClearRead(ParserRead *p);


ParserWrite *ParserInitWrite();
BOOL ParserWrite4(ParserWrite *p, DWORD Data);
BOOL ParserReadBytes(ParserRead *p, PVOID dst, DWORD size);
INT ParserWriteBytes(ParserWrite *p, PBYTE Data, SIZE_T len);
PBYTE ParserWriteReturnPointer(ParserWrite *p);
SIZE_T ParserWriteReturnSize(ParserWrite *p);
void ParserClearWrite(ParserWrite *p);