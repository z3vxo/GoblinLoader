#include "../includes/core/core.h"
#include "../includes/parser/parser.h"


BOOL ParseConfig() {
	Parser *p = ParserInit(GetConfig(), ConfigSize());
	ParserReadStringInto(p, ldr->config.ID, 37);
}


PBYTE GetConfig() {
	return (PBYTE)"\xab\xae";
}

UINT32 ConfigSize() {
	return 2;
}