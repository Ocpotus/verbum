#ifndef VERBUM_TOKEN_SET_H
#define VERBUM_TOKEN_SET_H

#include <stdbool.h>

#include "../../lib/hashmap.c/hashmap.h"

#include "../token/token.h"


typedef struct hashmap *TokenSet;

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


/* token_set_iterate
 *
 * Macro for clean iteration. It exists purely to avoid using hashmap_iter because it would polute
 * the code with iters, and datas.
 *
 *
 * PARAMETERS:
 * 	set: TokenSet
 * 	name: name of the iterator
 */
#define token_set_iterate(set, name, ...) \
do {\
	size_t token_set_iterate_iter = 0; \
	void *token_set_iterate_data; \
	while(hashmap_iter(set, &token_set_iterate_iter, &token_set_iterate_data)) {\
		const Token *name = token_set_iterate_data;\
		__VA_ARGS__ \
	}\
\
} while(0)


#endif
