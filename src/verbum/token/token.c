#include <stdbool.h>
#include <string.h>

#include "../memory/memory.h"

#include "token.h"


void token_delete(Token t) {
	if(t.tag == TokenType_Double_Literal ||
	   t.tag == TokenType_Single_Literal ||
	   t.tag == TokenType_Identifier) {
		memory_delete((void *)t.lexeme.data);
	}
}

Token token_copy(Token t) {
	return (Token) {
		.lexeme = t.lexeme,
		.pos = t.pos,
		.tag = t.tag,
	};
}

bool token_equals(Token a, Token b) {
	if((a.tag != b.tag) || (a.lexeme.length != b.lexeme.length)) {
		return false;
	}

	return strncmp(a.lexeme.data, b.lexeme.data, a.lexeme.length) == 0;
}
