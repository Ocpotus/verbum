#ifndef VERBUM_PARSER_H
#define VERBUM_PARSER_H

#include "../ast/ast.h"


/* Opaque declaration */
typedef struct Parser Parser;

/* Constructs a new Parser
 *
 * PARAMETERS:
 * 	path: path to a file to parse
 *
 * RETURNS:
 * 	Success: a pointer to a Parser
 * 	Failure: NULL
 */
Parser *parser_new(const char *path);

/* Deletes a Parser
 *
 * PARAMETERS:
 * 	p: Parser to delete
 */
void parser_delete(Parser *p);

/* Executes the parsing process for a Parser
 *
 * PARAMETERS:
 * 	p: Parser to execute
 *
 * RETURNS:
 * 	A pointer to a Unit representing the parsed
 * 	file passed in with the path in parser_new
 */
AST *parser_parse(Parser *p);


#endif
