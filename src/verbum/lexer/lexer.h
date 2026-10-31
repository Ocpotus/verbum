#ifndef VERBUM_LEXER_H
#define VERBUM_LEXER_H

#include <stddef.h>

#include "../../lib/utf8file/utf8file.h"
#include "../../lib/utf8.h/utf8.h"

#include "../token/token.h"


/* Lexer struct
 *
 * MEMBERS:
 * 	f: a pointer to a UTF8File to read from
 * 	pos:
 * 		row: current row in the file
 * 		column: current column in the file
 * 	cc: current character
 */
typedef struct Lexer {
	UTF8File *f;
	struct {
		uint32_t row;
		uint32_t col;
	} pos;
	utf8_int32_t cc;
} Lexer;

/* Constructs a new Lexer
 *
 * PARAMETERS:
 * 	path: path of file to open
 *
 * RETURNS:
 * 	A pointer to a Lexer or NULL on failure
 */
Lexer lexer_new(const char *path);

/* Deletes a Lexer
 *
 * PARAMETERS:
 * 	l: a pointer to a lexer
 */
void lexer_delete(Lexer *l);

/* Retrieves a token
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer
 *
 * RETURNS:
 * 	The next Token in the stream
 */
Token lexer_get_token(Lexer *l);

/* Retrieves the source file
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer
 *
 * RETURNS:
 * 	The name of the source file
 */
const char *lexer_get_source(Lexer *l);


#endif
