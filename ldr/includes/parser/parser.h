#pragma once
#include <windows.h>


typedef struct Parser Parser;

Parser *ParserInit(PBYTE buf, SIZE_T length);
UINT32 ParserRead4(Parser *p);
INT ParserReadStringInto(Parser *p, char *dst, SIZE_T bufSize);
char *ParserReadString(Parser *p);
void ParserFreeString(char *s);
void ParserClear(Parser *p);