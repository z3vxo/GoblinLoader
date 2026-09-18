#include "../../includes/core/core.h"
#include "../../includes/parser/parser.h"

PBYTE GetConfig() {
	return (PBYTE)"\x24\x00\x00\x00\x64\x38\x64\x33\x62\x66\x63\x38\x2d\x65\x35\x36\x65\x2d\x34\x33\x64\x30\x2d\x62\x63\x34\x61\x2d\x33\x37\x62\x64\x35\x33\x34\x31\x35\x35\x31\x33\x24\x00\x00\x00\x32\x31\x31\x63\x38\x61\x34\x31\x2d\x63\x39\x30\x31\x2d\x34\x33\x32\x32\x2d\x38\x66\x36\x32\x2d\x65\x36\x33\x31\x34\x30\x31\x34\x66\x65\x38\x33";
}

UINT32 ConfigSize() {
	return 80;
}

BOOL ParseConfig() {
	ParserRead *p = ParserInitRead(GetConfig(), ConfigSize());
	INT bytesRead = 0;
	bytesRead = ParserReadStringInto(p, ldr->config->UserId, 37);
	if(bytesRead == 0) 
		return FALSE;
	// load and exit simply pulls a file, maps it and exits safley
#ifdef LOAD_AND_EXIT
	bytesRead = ParserReadStringInto(p, ldr->config->FileId, 37);
	if(bytesRead == 0) 
		return FALSE;
#endif
	return TRUE;

}


