#ifndef VERBUM_TOKEN_SET_H
#define VERBUM_TOKEN_SET_H

#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"


typedef cvector(Token) TokenSet;


/* token_set_new
 *
 * Creates a new TokenSet
 *
 */
TokenSet token_set_new();

/* token_set_delete
 *
 * Deletes a given token_set
 *
 */
void token_set_delete(TokenSet set);

/* token_set_contains
 *
 * Checks if a token is in a given TokenSet
 *
 * PARAMETERS:
 * 	set: set to check
 * 	t: token to check for
 *
 * RETURNS:
 * 	true if found, false if not
 *
 */
bool token_set_contains(TokenSet set, Token t);

/* token_set_insert
 *
 * Inserts a token into the set when possible
 *
 * PARAMETERS:
 * 	set: set to insert into
 * 	t: token to insert
 *
 * RETURNS:
 * 	true if inserted, false if not
 */
bool token_set_insert(TokenSet *set, Token t);


#endif
