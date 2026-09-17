#include "../includes/core/core.h"
#include "../includes/parser/parser.h"
#include "../includes/utils/utils.h"

struct Parser {
	PBYTE buf;
	SIZE_T len;
	SIZE_T index;
}

Parser *ParserInit(PBYTE buf, SIZE_T length) {
	Parser *p = ldr->win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, length);
	if(!p) return NULL;
	p->buf = buf;
	p->len = length;
	p->index = 0;
	return p;
}


UINT32 ParserRead4(Parser *p) {
	if(p->index + 4 > p->len) return 0;
	UINT32 v;
	LdrMemcpy(&v, p->buf + p->index, 4);
	p->index += 4;
	return v;
}


char *ParserReadString(Parser *p) {
	UINT32 n = ParserRead4(p);
	if(p->index > p->len) return NULL;
	char *s = ldr->win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, n);
	if(!s) return NULL;
	LdrMemcpy(s, p->buf + p->index, n);
	p->index += n;
	return s;
}

INT ParserReadStringInto(Parser *p, char *dst, SIZE_T bufSize) {
	if(bufSize == 0) return -1;
	UINT32 n = ParserRead4(p);
	if(p->index + n > p->len) return -1;
	if(n > bufSize) return -1;
	LdrMemcpy(dst, p->buf + p->index, n);
	p->index += n;
	return (INT)n;
}


void ParserFreeString(char *s) { ldr->win32.LocalFree(s);  }
void ParserClear(Parser *p)    { ldr->win32.LocalAlloc(p); }