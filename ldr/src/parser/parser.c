#include "../../includes/core/core.h"
#include "../../includes/parser/parser.h"
#include "../../includes/core/utils.h"

#define PARSER_WRITE_INITIAL 256

struct ParserRead {
	PBYTE buf;
	SIZE_T len;
	SIZE_T index;
};

struct ParserWrite {
	PBYTE buf;
	SIZE_T len;
	SIZE_T index;
};


ParserRead* ParserInitRead(PBYTE buf, SIZE_T length) {
	ParserRead *p = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(ParserRead));
	if(!p) return NULL;
	p->buf = buf;
	p->len = length;
	p->index = 0;
	return p;
}


UINT32 ParserRead4(ParserRead *p) {
	if(p->index + 4 > p->len) return 0;
	UINT32 v;
	LdrMemcpy(&v, p->buf + p->index, 4);
	p->index += 4;
	return v;
}


char* ParserReadString(ParserRead *p) {
	UINT32 n = ParserRead4(p);
	if(p->index + n > p->len) return NULL;
	char *s = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, n + 1);
	if(!s) return NULL;
	LdrMemcpy(s, p->buf + p->index, n);
	p->index += n;
	return s;
}

BOOL ParserReadBytes(ParserRead *p, PVOID dst, DWORD size) {
	if (p->index + size > p->len) return FALSE;
	LdrMemcpy(dst, p->buf + p->index, size);
	p->index += size;
	return TRUE;
}

INT ParserReadStringInto(ParserRead *p, char *dst, SIZE_T bufSize) {
	if(bufSize == 0) return -1;
	UINT32 n = ParserRead4(p);
	if(p->index + n > p->len) return -1;
	if(n > bufSize) return -1;
	LdrMemcpy(dst, p->buf + p->index, n);
	p->index += n;
	return (INT)n;
}


ParserWrite* ParserInitWrite() {
	ParserWrite *p = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(ParserWrite));
	if(!p) return NULL;
	p->buf = ldr->win32->LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, PARSER_WRITE_INITIAL);
	if(!p->buf) {
		ldr->win32->LocalFree(p);
		return NULL;
	}
	p->len = PARSER_WRITE_INITIAL;
	p->index = 0;
	return p;
}

static BOOL ParserWriteGrow(ParserWrite *p, SIZE_T needed) {
	if(p->index + needed <= p->len)
		return TRUE;
	SIZE_T newLen = p->len;
	while(newLen < p->index + needed)
		newLen *= 2;
	PBYTE newBuf = ldr->win32->LocalReAlloc(p->buf, newLen, LMEM_MOVEABLE);
	if(!newBuf) return FALSE;
	p->buf = newBuf;
	p->len = newLen;
	return TRUE;
}

BOOL ParserWrite4(ParserWrite *p, DWORD Data) {
	if(!ParserWriteGrow(p, 4)) return FALSE;
	LdrMemcpy(p->buf + p->index, &Data, 4);
	p->index += 4;
	return TRUE;
}

INT ParserWriteBytes(ParserWrite *p, PBYTE Data, SIZE_T len) {
	if(!ParserWriteGrow(p, 4 + len)) return -1;
	ParserWrite4(p, (DWORD)len);
	LdrMemcpy(p->buf + p->index, Data, len);
	p->index += len;
	return (INT)len;
}


PBYTE ParserWriteReturnPointer(ParserWrite *p) { return p->buf; }
SIZE_T ParserWriteReturnSize(ParserWrite *p)  { return p->index; }

void ParserFreeString(char *s)         { ldr->win32->LocalFree(s); }
void ParserClearRead(ParserRead *p)    { ldr->win32->LocalFree(p); }
void ParserClearWrite(ParserWrite *p) {
	if(p->buf) ldr->win32->LocalFree(p->buf);
	ldr->win32->LocalFree(p);
}
