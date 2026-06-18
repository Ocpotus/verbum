#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "../../../lib/try.h/try.h"

#include "../ast/ast.h"
#include "../../lexer/lexer.h"
#include "../../memory/memory.h"

#include "parser.h"


/* Parser struct
 *
 * MEMBERS:
 * 	l: file Lexer
 * 	previous: previously processed Token from l
 * 	current: the current token in l
 * 	peek: the peeked token in l
 */
typedef struct Parser {
	Lexer *l;
	Token previous;
	Token current;
	Token peek;
	const char *error;
} Parser;


/* Advances the parser
 *
 * PARAMETERS:
 * 	p: parser to advance
 *
 * RETURNS:
 * 	the next token
 */
static Token parser_advance(Parser *p);

/* Retrieves the previous token
 *
 * PARAMETERS:
 * 	p: the parser to get the previous token
 *
 * RETURNS:
 * 	the previous token
 */
static Token parser_previous(Parser *p);

/* Retrieves the current token
 *
 * PARAMETERS:
 * 	p: the parser to get the current token
 *
 * RETURNS:
 * 	the current token
 */
static Token parser_current(Parser *p);

/* Checks if a Parser's previously processed Token is of a certain TokenType
 *
 * PARAMETERS:
 * 	p: Parser to check
 * 	tt: TokenType to check for
 *
 * RETURNS:
 * 	If the previously parsed token is of the given type
 */
static bool parser_previous_is(Parser *p, TokenType tt);

/* Peeks the next token without advancing
 *
 * PARAMETERS:
 * 	p: the parser to get the next token
 *
 * RETURNS:
 *	the peeked token
 */
static Token parser_peek(Parser *p);

/* Checks if the next token is of the desired type
 *
 * PARAMETERS:
 * 	p: the parser to check
 * 	tt: the desired token type
 *
 * RETURNS:
 * 	if the next token is of the desired type
 */
static bool parser_check(Parser *p, TokenType tt);

/* Checks and advances if the next token is of the desired type
 *
 * PARAMETERS:
 * 	p: parser to match to
 * 	tt: desired type
 *
 * RETURNS:
 * 	true: if the match was successful
 * 	false: if the match was unsuccessful
 */
static bool parser_match(Parser *p, TokenType tt);

/* Checks and advances if the next token is any of the desired types
 *
 * PARAMETERS:
 * 	p: parser to match to
 * 	tts: an array of expected TokenTypes
 * 	n: length of tts
 *
 * RETURNS:
 * 	true: if the match was successful
 * 	false: if the match was unsuccessful
 */
static bool parser_match_any(Parser *p, TokenType *tts, size_t n);

/* Checks if the parser is at the end of the token stream
 *
 * PARAMETERS:
 * 	p: parser to check
 *
 * RETURNS:
 * 	true: if the parser is at the end
 * 	false: if the parser is not at the end
 */
static bool parser_at_end(Parser *p);

static const char *parser_set_error(Parser *p, const char *error);

static void parser_reset_error(Parser *p);

static bool parser_errored(Parser *p);

static Grammar parser_parse_internal(Parser *p);


Parser *parser_new(const char *path) {
	Parser *result = memory_new(1 * sizeof(*result));

	if(result != NULL) {
		result->l = lexer_new(path);
		result->previous = (Token) {
			.tag = TokenType_Whitespace,
			.lexeme = NULL,
		};
		result->current = lexer_get_token(result->l);
		result->peek = lexer_get_token(result->l);
	}

	return result;
}

void parser_delete(Parser *p) {
	lexer_delete(p->l);
	memory_delete(p);
}

AST *parser_parse(Parser *p) {
	Grammar g1 = parser_parse_internal(p);
	AST *result = try(memory_copy(&g1, sizeof(*result)), result == NULL, { return NULL; });

	return result;
}

static Grammar parser_parse_grammar(Parser *p);
static Rule parser_parse_rule(Parser *p);
static Expression parser_parse_expression(Parser *p);
static List parser_parse_list(Parser *p);
static Factor parser_parse_factor(Parser *p);

static Token parser_advance(Parser *p) {
	if(!parser_at_end(p)) {
		Token t;

		do {
			t = lexer_get_token(p->l);
		} while(t.tag == TokenType_Whitespace);

		p->previous = p->current;
		p->current = p->peek;
		p->peek = t;
		p->error = NULL;
	}

	return parser_current(p);
}

static bool parser_at_end(Parser *p) {
	return parser_current(p).tag == TokenType_EOF;
}

static Token parser_previous(Parser *p) {
	return p->previous;
}

static Token parser_current(Parser *p) {
	return p->current;
}

static Token parser_peek(Parser *p) {
	return p->peek;
}

static bool parser_check(Parser *p, TokenType tt) {
	return parser_current(p).tag == tt;
}

static bool parser_previous_is(Parser *p, TokenType tt) {
	return parser_previous(p).tag == tt;
}

static bool parser_match(Parser *p, TokenType tt) {
	if(parser_check(p, tt)) {
		parser_advance(p);

		return true;
	}

	return false;
}

static bool parser_match_any(Parser *p, TokenType *tts, size_t n) {
	for(size_t i = 0; i < n; i++) {
		if(parser_match(p, tts[i])) {
			return true;
		}
	}

	return false;
}

static const char *parser_set_error(Parser *p, const char *error) {
	p->error = error;

	return error;
}

static void parser_reset_error(Parser *p) {
	p->error = NULL;
}

static bool parser_errored(Parser *p) {
	return p->error != NULL;
}

static void parser_panic(Parser *p, const char *format, ...) {
        va_list list;
	struct Token t;

        va_start(list, format);
	fprintf(stderr, "Error! (%s) ", lexer_get_source(p->l));
	vfprintf(stderr, format, list);
        va_end(list);

	if(parser_peek(p).tag == TokenType_RightBrace) {
		return;
	}

        t = parser_advance(p);

        while(!parser_at_end(p)) {
                if(parser_previous(p).tag == TokenType_Semicolon || parser_previous(p).tag == TokenType_Comma) {
                        return;
                }

		token_delete(t);

		t = parser_advance(p);
        }
}

static Grammar parser_parse_internal(Parser *p) {
	return parser_parse_grammar(p);
}

static Grammar parser_parse_grammar(Parser *p) {
	cvector(Rule) rl1 = NULL;

	while(!parser_match(p, TokenType_EOF)) {
		Rule r1 = parser_parse_rule(p);

		if(p->error != NULL) {
			return (Grammar) { 0 };
		}

		cvector_push_back(rl1, r1);
	}

	return (Grammar) {
		.rule1 = rl1,
	};
}

static Rule parser_parse_rule(Parser *p) {
	static TokenType tts[] = { TokenType_Identifier };

	if(parser_match_any(p, tts, sizeof(tts) / sizeof(*tts))) {
		Token t1 = parser_previous(p);
		Expression e1 = { 0 };
		Rule r = { 0 };

		if(!parser_match(p, TokenType_Equal)) {
			parser_set_error(p, "invalid token");
			parser_panic(p, "expected '=' encountered '%s'", parser_current(p));
			return (Rule) { 0 };
		}

		e1 = parser_parse_expression(p);

		if(p->error != NULL) {
			return (Rule) { 0 };
		}

		if(!parser_match(p, TokenType_Semicolon)) {
			parser_set_error(p, "invalid token");
			parser_panic(p, "expected ';' encountered '%s'", parser_current(p));
			return (Rule) { 0 };
		}

		r = (Rule) {
			.token1 = t1,
			.expression1 = memory_copy(&e1, sizeof(e1))
		};

		if(r.expression1 == NULL) {
			return (Rule) { 0 };
		}

		return r;
	}
	
	return (Rule) { 0 };
}

static Expression parser_parse_expression(Parser *p) {
	List l1 = parser_parse_list(p);
	cvector(List) ll1 = NULL;

	if(p->error != NULL) {
		return (Expression) { 0 };
	}

	while(parser_match(p, TokenType_Pipe)) {
		List t2 = parser_parse_list(p);

		if(p->error != NULL) {
			return (Expression) { 0 };
		}

		cvector_push_back(ll1, t2);
	}

	return (Expression) {
		.list1 = l1,
		.list2 = ll1,
	};
}

static List parser_parse_list(Parser *p) {
	Factor f1 = parser_parse_factor(p);
	cvector(Factor) fl1 = NULL;

	if(p->error != NULL) {
		return (List) { 0 };
	}

	while(parser_match(p, TokenType_Comma)) {
		Factor f2 = parser_parse_factor(p);

		if(p->error != NULL) {
			return (List) { 0 };
		}

		cvector_push_back(fl1, f2);
	}

	return (List) {
		.factor1 = f1,
		.factor2 = fl1,
	};
}

static Factor parser_parse_factor(Parser *p) {
	if(parser_match(p, TokenType_Identifier)) {
		return ast_new_factor_identifier(parser_previous(p));
	}

	if(parser_match(p, TokenType_Single_Literal)) {
		return ast_new_factor_single_literal(parser_previous(p));
	}

	if(parser_match(p, TokenType_Double_Literal)) {
		return ast_new_factor_double_literal(parser_previous(p));
	}

	if(parser_match(p, TokenType_LeftBracket)) {
		Expression e = parser_parse_expression(p);

		if(parser_match(p, TokenType_RightBracket)) {
			return ast_new_factor_optional(e);
		} else {
			parser_set_error(p, "invalid token");
			parser_panic(p, "expected ']', got %s", parser_current(p));
			return (Factor) { 0 };
		}
	}

	if(parser_match(p, TokenType_LeftBrace)) {
		Expression e = parser_parse_expression(p);

		if(parser_match(p, TokenType_RightBrace)) {
			return ast_new_factor_repetition(e);
		} else {
			parser_set_error(p, "invalid token");
			parser_panic(p, "expected '{', got %s", parser_current(p));
			return (Factor) { 0 };
		}
	}

	if(parser_match(p, TokenType_LeftParenthesis)) {
		Expression e = parser_parse_expression(p);

		if(parser_match(p, TokenType_RightParenthesis)) {
			return ast_new_factor_grouping(e);

		} else {
			parser_set_error(p, "invalid token");
			parser_panic(p, "expected ')', got %s", parser_current(p));
			return (Factor) { 0 };
		}
	}

	return (Factor) { 0 };
}
