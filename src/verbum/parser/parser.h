#ifndef VERBUM_PARSER_H
#define VERBUM_PARSER_H

#include "../../lib/c-vector/cvector.h"

#include "../table/table.h"
#include "../memory/memory.h"
#include "../token/token.h"
/*

typedef struct Parser {
	char *input;
	ParseTable table;
	struct {
		size_t top;
		cvector(Token) data;
	} stack;
} Parser;


Parser parser_new(ParseTable t);
void parser_parse(Parser *p, char *input);

*/
#endif
