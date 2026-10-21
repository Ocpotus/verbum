#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/hashmap.c/hashmap.h"

#include "../memory/memory.h"
#include "../token/token.h"

#include "token_set.h"


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

	return !token_equals(*ua, *ub);
}
