#define _GNU_SOURCE
#include <string.h>
#include <stdbool.h>

#include "../../lib/hashmap.c/hashmap.h"

#include "../memory/memory.h"
#include "../lexeme/lexeme.h"
#include "../token/token.h"

#include "symbol.h"


static uint64_t symbol_set_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int symbol_set_compare(const void *a, const void *b, void *udata);

Symbol symbol_new(Lexeme l, uint32_t mask) {
	return (Symbol) {
		.lexeme = l,
		.mask = mask,
	};
}

Symbol symbol_from_token(Token t) {
	uint32_t mask = SymbolMask_None;

	if(t.tag == TokenType_Single_Literal || t.tag == TokenType_Double_Literal) {
		mask |= SymbolMask_Terminal;
	}

	return symbol_new(t.lexeme, mask);
}

bool symbol_equals(Symbol a, Symbol b) {
	if(a.mask != b.mask || a.lexeme.length != b.lexeme.length) {
		return false;
	}

	if(a.lexeme.data == b.lexeme.data) {
		return true;
	}

	return strncmp(a.lexeme.data, b.lexeme.data, a.lexeme.length) == 0;
}

SymbolSet symbol_set_new() {
	return hashmap_new_with_allocator(memory_new, memory_resize, memory_delete,
					    sizeof(Symbol), 0, 0, 0,
					    symbol_set_hash, symbol_set_compare, NULL, NULL);
}

void symbol_set_delete(SymbolSet set) {
	hashmap_free(set);
}

bool symbol_set_contains(SymbolSet set, Symbol t) {
	return hashmap_get(set, &t) != NULL;
}

bool symbol_set_insert(SymbolSet *set, Symbol t) {
	return hashmap_set(*set, &t) != NULL;
}

static uint64_t symbol_set_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const Symbol *symbol = item;

	return hashmap_sip(symbol->lexeme.data, strnlen(symbol->lexeme.data, symbol->lexeme.length), seed0, seed1);
}

static int symbol_set_compare(const void *a, const void *b, void *udata) {
	const Symbol *ua = a;
	const Symbol *ub = b;

	return !symbol_equals(*ua, *ub);
}
