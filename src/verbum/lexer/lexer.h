#ifndef VERBUM_LEXER_H
#define VERBUM_LEXER_H

#include "../token/token.h"


/* Opaque declaration */
typedef struct Lexer Lexer;

/* Constructs a new Lexer
 *
 * PARAMETERS:
 * 	path: path of file to open
 *
 * RETURNS:
 * 	A pointer to a Lexer or NULL on failure
 */
Lexer *lexer_new(const char *path);

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
