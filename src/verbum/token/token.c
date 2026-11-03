#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/hashmap.c/hashmap.h"

#include "../memory/memory.h"
#include "../lexeme/lexeme.h"

#include "token.h"


void token_delete(Token t) {
	if(t.tag == TokenType_Double_Literal ||
	   t.tag == TokenType_Single_Literal ||
	   t.tag == TokenType_Identifier) {
		lexeme_delete(t.lexeme);
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

	// What else could it be?
	if(a.lexeme.data == b.lexeme.data) {
		return true;
	}

	return strncmp(a.lexeme.data, b.lexeme.data, a.lexeme.length) == 0;
}

static uint64_t token_set_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int token_set_compare(const void *a, const void *b, void *udata);

TokenSet token_set_new() {
	return hashmap_new_with_allocator(memory_new, memory_resize, memory_delete,
					    sizeof(Token), 0, 0, 0,
					    token_set_hash, token_set_compare, NULL, NULL);
}

void token_set_delete(TokenSet set) {
	hashmap_free(set);
}

bool token_set_contains(TokenSet set, Token t) {
	return hashmap_get(set, &t) != NULL;
}

bool token_set_insert(TokenSet *set, Token t) {
	return hashmap_set(*set, &t) != NULL;
}

static uint64_t token_set_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const Token *token = item;

	return hashmap_sip(token->lexeme.data, strnlen(token->lexeme.data, token->lexeme.length), seed0, seed1);
}

static int token_set_compare(const void *a, const void *b, void *udata) {
	const Token *ua = a;
	const Token *ub = b;

	(void) udata;

	return !token_equals(*ua, *ub);
}
