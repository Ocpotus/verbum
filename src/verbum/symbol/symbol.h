#ifndef VERBUM_SYMBOL_H
#define VERBUM_SYMBOL_H

#include <stdint.h>
#include <stdbool.h>

#include "../lexeme/lexeme.h"


/*
 * C99 standard guarantees sizeof enum is >= sizeof int
 * First 4 bits will store metadata
 * Last 28 bits will be used store to get nth group, repetition, optional
 * Allowing for 2^28 # of groups, repetitions, or optionals. Which is more than sufficient
 *
 * Diagram:
 * 	Group #33: 1 0 0 1 | 1 0 0 0 0 1 ...
 * 		   |	 | \_______________/
 * Group is dynamic/     |         \Nth group processed
 * 	    Group bit set/
 */
typedef enum SymbolMask {
	SymbolMask_None = 0x0 << 0,
	SymbolMask_Dynamic = 0x1 << 0,
	SymbolMask_Repetition = 0x1 << 1,
	SymbolMask_Optional = 0x1 << 2,
	SymbolMask_Group = 0x1 << 3,
} SymbolMask;

typedef struct Symbol {
	Lexeme lexeme;
	uint32_t mask;
} Symbol;

Symbol symbol_new(Lexeme l, uint32_t mask);

void symbol_delete(Symbol *s);

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
 * Checks if a token is in a given SymbolSet
 *
 * PARAMETERS:
 * 	set: set to check
 * 	t: token to check for
 *
 * RETURNS:
 * 	true if found, false if not
 *
 */
bool symbol_set_contains(SymbolSet set, Symbol t);

/* symbol_set_insert
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
bool symbol_set_insert(SymbolSet *set, Symbol t);


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
