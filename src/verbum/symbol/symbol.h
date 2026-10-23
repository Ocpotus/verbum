#ifndef VERBUM_SYMBOL_H
#define VERBUM_SYMBOL_H

#include <stdint.h>
#include <stdbool.h>

#include "../lexeme/lexeme.h"
#include "../token/token.h"


/*
 * C99 standard guarantees sizeof enum is >= sizeof int
 * First 4 bits will store metadata
 * Last 28 bits will be used store to get nth group, repetition, optional
 * Allowing for 2^28 # of groups, repetitions, or optionals. Which is more than sufficient...
 *
 * The whole mask takes part in equality, sub expressions share a lexeme (e.g. "(") and are only
 * told apart by their flag and Nth index
 *
 * Diagram:
 * 	Group #33: 0 0 0 1 | 1 0 0 0 0 1 ...
 * 		   	 | \_______________/
 *	                 |  \Nth group processed
 * 	    Group bit set/
 */
typedef enum SymbolMask {
	SymbolMask_None = 0x0 << 0,
	SymbolMask_Repetition = 0x1 << 0,
	SymbolMask_Optional = 0x1 << 1,
	SymbolMask_Group = 0x1 << 2,
	SymbolMask_Terminal = 0x1 << 3,
} SymbolMask;

typedef struct Symbol {
	Lexeme lexeme;
	uint32_t mask;
} Symbol;

Symbol symbol_new(Lexeme l, uint32_t mask);

/* symbol_from_token
 *
 * Converts a Token into a Symbol, the lexeme is borrowed not copied
 *
 * PARAMETERS:
 * 	t: token to convert
 *
 * RETURNS:
 * 	a Symbol marked as a terminal if t is a literal
 */
Symbol symbol_from_token(Token t);

bool symbol_equals(Symbol a, Symbol b);

typedef struct hashmap *SymbolSet;

/* symbol_set_new
 *
 * Creates a new SymbolSet
 *
 */
SymbolSet symbol_set_new();

/* symbol_set_delete
 *
 * Deletes a given symbol_set
 *
 */
void symbol_set_delete(SymbolSet set);

/* symbol_set_contains
 *
 * Checks if a symbol is in a given SymbolSet
 *
 * PARAMETERS:
 * 	set: set to check
 * 	t: symbol to check for
 *
 * RETURNS:
 * 	true if found, false if not
 *
 */
bool symbol_set_contains(SymbolSet set, Symbol t);

/* symbol_set_insert
 *
 * Inserts a symbol into the set when possible
 *
 * PARAMETERS:
 * 	set: set to insert into
 * 	t: symbol to insert
 *
 * RETURNS:
 * 	true if inserted, false if not
 */
bool symbol_set_insert(SymbolSet *set, Symbol t);

size_t symbol_set_size(SymbolSet set);

size_t symbol_set_index(SymbolSet set, Symbol symbol, bool *found);

static inline bool symbol_is_nullable(Symbol s) {
	return s.mask & (SymbolMask_Optional | SymbolMask_Repetition);
}

/* symbol_set_iterate
 *
 * Macro for clean iteration. It exists purely to avoid using hashmap_iter because it would polute
 * the code with iters, and datas.
 *
 *
 * PARAMETERS:
 * 	set: SymbolSet
 * 	name: name of the iterator
 */
#define symbol_set_iterate(set, name, ...) \
do {\
	size_t symbol_set_iterate_iter = 0; \
	void *symbol_set_iterate_data; \
	while(hashmap_iter(set, &symbol_set_iterate_iter, &symbol_set_iterate_data)) {\
		const Symbol *name = symbol_set_iterate_data;\
		__VA_ARGS__ \
	}\
\
} while(0)


#endif
