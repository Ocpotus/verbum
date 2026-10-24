#define _GNU_SOURCE
#include <string.h>
#include <stdbool.h>

#include "../../lib/hashmap.c/hashmap.h"

#include "../memory/memory.h"
#include "../lexeme/lexeme.h"
#include "../token/token.h"

#include "symbol.h"

/* SymbolSetEntry
 *
 * MEMBERS:
 * 	symbol: actual symbol
 * 	index: index of the symbol (insertion order)
 */
typedef struct SymbolSetEntry {
	Symbol symbol;
	size_t index;
} SymbolSetEntry;

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
					    sizeof(SymbolSetEntry), 0, 0, 0,
					    symbol_set_hash, symbol_set_compare, NULL, NULL);
}

void symbol_set_delete(SymbolSet set) {
	hashmap_free(set);
}

bool symbol_set_contains(SymbolSet set, Symbol t) {
	return hashmap_get(set, &(SymbolSetEntry) { .symbol = t, .index = 0 }) != NULL;
}

bool symbol_set_insert(SymbolSet *set, Symbol t) {
	return hashmap_set(*set, &(SymbolSetEntry) { .symbol = t, .index = symbol_set_size(*set) + 1 }) != NULL;
}

size_t symbol_set_size(SymbolSet set) {
	return hashmap_count(set);
}

size_t symbol_set_index(SymbolSet set, Symbol symbol, bool *found) {
	const SymbolSetEntry *entry = hashmap_get(set, &(SymbolSetEntry) { .symbol = symbol, .index = 0 });

	if(entry == NULL) {
		*found = false;

		return 0;
	}

	*found = true;

	return entry->index;
}

static uint64_t symbol_set_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const Symbol *symbol = item;

	return hashmap_sip(symbol->lexeme.data, strnlen(symbol->lexeme.data, symbol->lexeme.length), seed0, seed1);
}

static int symbol_set_compare(const void *a, const void *b, void *udata) {
	const Symbol *ua = a;
	const Symbol *ub = b;

	(void) udata;

	return !symbol_equals(*ua, *ub);
}
