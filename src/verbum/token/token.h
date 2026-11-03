#ifndef VERBUM_TOKEN_H
#define VERBUM_TOKEN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../../lib/hashmap.c/hashmap.h"

#include "../lexeme/lexeme.h"


/* Operator charset */
#define OPERATOR_CHARS "=:|[]{}(),;"
/* Invalid charset */
#define INVALID_CHARS "?&^!+/%~.<>-"
/* Whitespace charset */
#define WHITE_SPACE " \t\n\v\f\r"
/* Invalid charset for an identifier or keyword */
#define INVALID_IDENTIFIER_CHARS OPERATOR_CHARS INVALID_CHARS WHITE_SPACE

/* Identifier and keyword charset */
#define LETTER_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGIT_CHARS "1234567890"
#define SYMBOL_CHARS "`~!@#$%^&*()-_+={}[]|\\:;\"\'<>?,./"

#define IDENTIFIER_CHARS LETTER_CHARS DIGIT_CHARS "_"


typedef enum TokenType {
        TokenType_Invalid = -2,
        TokenType_EOF = -1, // EOF

	TokenType_Whitespace,

	TokenType_Identifier, // _ab1, abc
	TokenType_Single_Literal, // 'hello'
	TokenType_Double_Literal, // "hello"

	TokenType_Operator,
	TokenType_Equal, // =
	TokenType_ColonEqual, // :=
        TokenType_Pipe, // |
	TokenType_Comma, // ,
	TokenType_LeftBracket, // [
	TokenType_RightBracket, // ]
	TokenType_LeftBrace, // {
	TokenType_RightBrace, // }
	TokenType_LeftParenthesis, // (
	TokenType_RightParenthesis, // )
	TokenType_LeftComment, // (*
	TokenType_RightComment, // *)
	TokenType_Semicolon, // ;
} TokenType;

/* Token
 *
 * MEMBERS:
 *	lexeme: lexeme processed
 *	type: the type of token processed
 *	pos:
 *		row: row in the file processed
 *		col: column in the file processed
 */
typedef struct Token {
	Lexeme lexeme;
	TokenType tag;
	struct {
		uint32_t row;
		uint32_t col;
	} pos;
} Token;

/* Deletes a Token
 *
 * PARAMETERS:
 * 	t: Token to delete
 */
void token_delete(Token t);

/* token_copy
 *
 * Shallow copies the contents of a token.
 *
 * PARAMETERS:
 * 	t: token to copy
 *
 * RETURNS:
 * 	A new token with the same contents as t.
 */
Token token_copy(Token t);

/* token_equals
 *
 * Checks the equivalency of two tokens.
 *
 * PARAMETERS:
 * 	a, b: tokens to compare
 *
 * RETURNS:
 * 	true if tokens equal each other, false otherwise.
 * 	The equality is determined by mask and data.
 *
 */
bool token_equals(Token a, Token b);

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
