#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "../../lib/utf8.h/utf8.h"
#include "../../lib/utf8file/utf8file.h"

#include "../memory/memory.h"

#include "lexer.h"


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

/* Advances a Lexer's position
 *
 * PARAMETERS:
 * 	l: pointer to a Lexer to advance
 * 
 * RETURNS:
 * 	The retrieved character from the given Lexer's stream
 */
utf8_int32_t lexer_advance(Lexer *l);

/* Retrieves the current character of a Lexer
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer to process
 *
 * RETURNS:
 * 	The current character in the Lexer's stream
 */
utf8_int32_t lexer_current_character(Lexer *l);

/* Peeks a character in a Lexer's stream
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer to process
 *
 * RETURNS:
 * 	The next character in the Lexer's stream
 */
utf8_int32_t lexer_peek_character(Lexer *l);

/* Undos the previous action by a Lexer
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer to process
 */
void lexer_undo(Lexer *l);

/* Checks if a Lexer is at the end of its stream
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer to check
 *
 * RETURNS:
 *	true if the given Lexer is at the end, false otherwise
 */
bool lexer_at_end(Lexer *l);

/* Reports an error encountered by the Lexer
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer to propogate the error from
 * 	c: the character that caused the error
 */
void lexer_report_error(Lexer *l, utf8_int32_t c);

/* Retrieves a token
 *
 * PARAMETERS:
 * 	l: a pointer to a Lexer
 *
 * RETURNS:
 * 	The next Token in the stream
 */
Token lexer_lex(Lexer *l);

Lexer *lexer_new(const char *path) {
	Lexer *result = memory_new(1 * sizeof(*result));

	if(result != NULL) {
		result->f = utf8file_open(path);
		result->pos.row = 1;
		result->pos.col = 1;

		if(result->f == NULL) {
			memory_delete(result);
		}
	}

	return result;
}

void lexer_delete(Lexer *l) {
	utf8file_close(l->f);
	memory_delete(l);
}

Token lexer_get_token(Lexer *l) {
	return lexer_lex(l);
}

const char *lexer_get_source(Lexer *l) {
	return utf8file_path(l->f);
}

static Token lexer_lex_identifier(Lexer *l);
static Token lexer_lex_operator(Lexer *l);
static Token lexer_lex_single_literal(Lexer *l);
static Token lexer_lex_double_literal(Lexer *l);

utf8_int32_t lexer_advance(Lexer *l) {
	utf8_int32_t result = '\0';

	if(!lexer_at_end(l)) {
		result = utf8file_get_codepoint(l->f);

		if(result == '\n') {
			l->pos.row += 1;
			l->pos.col = 1;
		} else {
			l->pos.col += 1;
		}

		l->cc = result;
	}

	return result;
}

utf8_int32_t lexer_current_character(Lexer *l) {
	return l->cc;
}

utf8_int32_t lexer_peek_character(Lexer *l) {
	utf8_int32_t result = utf8file_get_codepoint(l->f);

	utf8file_unget_codepoint(l->f);

	return result;
}

void lexer_undo(Lexer *l) {
	utf8file_unget_codepoint(l->f);
}

bool lexer_at_end(Lexer *l) {
	return utf8file_at_end(l->f);
}

void lexer_report_error(Lexer *l, utf8_int32_t c) {
	fprintf(stderr, "%s: (%d:%d) unrecognized token '%lc'\n", utf8file_path(l->f), l->pos.row, l->pos.col, c);
}

Token lexer_lex(Lexer *l) {
	Token result = {
		.lexeme = {
			.data = "EOF",
			.length = 3,
		},
		.tag = TokenType_EOF,
		.pos = {
			.row = l->pos.row,
			.col = l->pos.col,
		},
	};
	utf8_int32_t c = lexer_advance(l);

	while(isascii(c) && isspace(c)) {
		c = lexer_advance(l);
	}

	if(lexer_at_end(l)) {
		return (Token) {
			.lexeme = {
				.data = "EOF",
				.length = 3,
			},
			.tag = TokenType_EOF,
			.pos = {
				.row = l->pos.row,
				.col = l->pos.col,
			},
		};
	}

	if(utf8chr(OPERATOR_CHARS, c)) {
		return lexer_lex_operator(l);
	}

	if(utf8chr(LETTER_CHARS "_", c)) {
		return lexer_lex_identifier(l);
	}

	if(utf8chr("\"", c)) {
		return lexer_lex_double_literal(l);
	}

	if(utf8chr("\'", c)) {
		return lexer_lex_single_literal(l);
	}

	if(utf8chr(INVALID_CHARS, c)) {
		result.lexeme.data = "invalid";
		result.lexeme.length = 7;
		result.tag = TokenType_Invalid;
	}

	lexer_advance(l);

	return result;
}

Token lexer_lex_identifier(Lexer *l) {
	Token result = {
		.lexeme = {
			.data = NULL,
			.length = 0,

		},
		.tag = TokenType_Identifier,
		.pos = {
			.row = l->pos.row,
			.col = l->pos.col,
		},
	};
	size_t len = 1;

	do {
		len += utf8codepointsize(lexer_current_character(l));
	} while(utf8chr(LETTER_CHARS DIGIT_CHARS "_", lexer_advance(l)) != NULL);

	result.lexeme = (Lexeme) {
		.data = utf8file_at(l->f, utf8file_tell(l->f) - len),
		.length = len - 1,
	};

	lexer_undo(l);

	return result;
}

static Token lexer_lex_operator(Lexer *l) {
	Token result = {
		.lexeme = (Lexeme) {
			.data = utf8file_at(l->f, utf8file_tell(l->f) - 1),
			.length = 1,
		},
		.tag = TokenType_Operator,
		.pos = {
			.row = l->pos.row,
			.col = l->pos.col,
		},
	};

        switch(lexer_current_character(l)) {
        case '=':
		result.tag = TokenType_Equal;
                break;
	case '|':
		result.tag = TokenType_Pipe;
		break;
	case '[':
		result.tag = TokenType_LeftBracket;
		break;
	case ']':
		result.tag = TokenType_RightBracket;
		break;
	case '{':
		result.tag = TokenType_LeftBrace;
		break;
	case '}':
		result.tag = TokenType_RightBrace;
		break;
	case '(':
		result.tag = TokenType_LeftParenthesis;
		break;
	case ')':
		result.tag = TokenType_RightParenthesis;
		break;
	case ',':
		result.tag = TokenType_Comma;
		break;
	case ';':
		result.tag = TokenType_Semicolon;
		break;
	case '-':
		result.tag = TokenType_Hyphen;
		break;
	default:
		result.tag = TokenType_Invalid;
		break;
        }

        return result;
}

static Token lexer_lex_single_literal(Lexer *l) {
	Token result = {
		.lexeme = { 0 },
		.tag = TokenType_Single_Literal,
		.pos = {
			.row = l->pos.row,
			.col = l->pos.col,
		},
	};
	size_t len = 0;

	do {
		len += utf8codepointsize(lexer_advance(l));
	} while(!utf8chr("\'", lexer_current_character(l)));

	result.lexeme = (Lexeme) {
		.data = utf8file_at(l->f, utf8file_tell(l->f) - len - 1),
		.length = len + 2 - 1,
	};

	return result;
}

static Token lexer_lex_double_literal(Lexer *l) {
	Token result = {
		.lexeme = { 0 },
		.tag = TokenType_Double_Literal,
		.pos = {
			.row = l->pos.row,
			.col = l->pos.col,
		},
	};
	size_t len = 0;

	do {
		len += utf8codepointsize(lexer_advance(l));
	} while(!utf8chr("\"", lexer_current_character(l)));

	result.lexeme = (Lexeme) {
		.data = utf8file_at(l->f, utf8file_tell(l->f) - len - 1),
		.length = len + 2 - 1,
	};

	return result;
}
