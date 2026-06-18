#ifndef VERBUM_TOKEN_H
#define VERBUM_TOKEN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Operator charset */
#define OPERATOR_CHARS "=|[]{}(),;-"
/* Invalid charset */
#define INVALID_CHARS "?&^!+/%~.<>"
/* Whitespace charset */
#define WHITE_SPACE " \t\n\v\f\r"
/* Invalid charset for an identifier or keyword */
#define INVALID_IDENTIFIER_CHARS OPERATOR_CHARS INVALID_CHARS WHITE_SPACE

/* Identifier and keyword charset */
#define LETTER_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGIT_CHARS "1234567890"
#define SYMBOL_CHARS "`~!@#$%^&*()-_+={}[]|\\:;\"\'<>?,./"

#define IDENTIFIER_CHARS LETTER_CHARS DIGIT_CHARS "_"


/* Simple alias for const char * */
//typedef const char *Lexeme;

/* Possible Token types of the C programming language */
typedef enum TokenType {
        TokenType_Invalid = -2,
        TokenType_EOF = -1, // EOF

	TokenType_Whitespace,

	TokenType_Identifier, // _ab1, abc
	TokenType_Single_Literal, // 'hello'
	TokenType_Double_Literal, // "hello"

	TokenType_Operator,
	TokenType_Equal, // =
        TokenType_Pipe, // |
	TokenType_Hyphen, // -
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

typedef struct Lexeme {
	const char *data;
	size_t length;
} Lexeme;
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

Token token_copy(Token t);

bool token_equals(Token a, Token b);


#endif
