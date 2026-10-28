#ifndef VERBUM_LR1_H
#define VERBUM_LR1_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"
#include "../lr0/lr0.h"
#include "../grammar/grammar.h"


/* LR1 API
 *
 * Definitions shared by every LR(1) construction (canonical, lane table, ...).
 * lr1_new only lifts an LR(0) automaton into an LR(0-1) one, computing the actual
 * lookaheads is left to the construction algorithm.
 */

/* LookaheadSet
 *
 * Bitset of terminals, indexed the same as LR1Automaton.terminals.
 * The bit at index cvector_size(terminals) is the end of input.
 *
 * NULL is the wildcard, it matches every terminal and is used by LR(0-1) items whose
 * lookahead was never needed.
 */
typedef uint64_t *LookaheadSet;

typedef struct LR1Item {
	LR0Item lr0;
	LookaheadSet lookahead;
} LR1Item;

typedef struct LR1State {
	cvector(LR1Item) items;
} LR1State;

typedef struct LR1Transition {
	size_t id;
	size_t source;
	size_t destination;
	Symbol symbol;
} LR1Transition;

/* LR1Automaton
 *
 * MEMBERS:
 * 	states: LR(1) states, state 0 is the start state
 * 	transitions: edges between states
 * 	terminals: maps a lookahead index to its terminal, ordered by symbol_set_index, one past the end is the end of input
 * 	word_count: number of words backing a LookaheadSet
 */
typedef struct LR1Automaton {
	cvector(LR1State) states;
	cvector(LR1Transition) transitions;
	cvector(Symbol) terminals;
	size_t word_count;
} LR1Automaton;


/* lr1_new
 *
 * Builds an LR(1) automaton out of an LR(0) automaton, every item gets the wildcard lookahead
 *
 * PARAMETERS:
 * 	g: the grammar lr0 was built from
 * 	lr0: the LR(0) automaton, it is only read from
 *
 * RETURNS:
 * 	a newly constructed automaton
 */
LR1Automaton lr1_new(Grammar g, LR0Automaton lr0);

void lr1_delete(LR1Automaton lr1);

/* lr1_end
 *
 * RETURNS:
 * 	the lookahead index of the end of input
 */
static inline size_t lr1_end(LR1Automaton *a) {
	return cvector_size(a->terminals);
}

/* lr1_terminal_index
 *
 * PARAMETERS:
 * 	a: automaton to search
 * 	s: terminal to find
 *
 * RETURNS:
 * 	the lookahead index of s, SIZE_MAX if s is not a terminal
 */
size_t lr1_terminal_index(LR1Automaton *a, Symbol s);

/* lr1_item_new
 *
 * PARAMETERS:
 * 	lr0: kernel of the item
 * 	lookahead: set to take ownership of, NULL for the wildcard
 */
LR1Item lr1_item_new(LR0Item lr0, LookaheadSet lookahead);

LR1Item lr1_item_copy(LR1Automaton *a, LR1Item item);

void lr1_item_delete(LR1Item *item);

/* lr1_item_kernel_equals
 *
 * RETURNS:
 * 	true if both items have the same LR(0) kernel, lookaheads are ignored
 */
bool lr1_item_kernel_equals(LR0Item a, LR0Item b);

bool lr1_item_equals(LR1Automaton *a, LR1Item x, LR1Item y);

LR1State lr1_state_new();

/* lr1_state_copy
 *
 * Deep copies a state, lookaheads included
 */
LR1State lr1_state_copy(LR1Automaton *a, LR1State state);

void lr1_state_delete(LR1State *state);

/* lr1_state_find
 *
 * PARAMETERS:
 * 	state: state to search
 * 	kernel: LR(0) item to look for
 *
 * RETURNS:
 * 	index of the item with the given kernel, SIZE_MAX if not found
 */
size_t lr1_state_find(LR1State state, LR0Item kernel);

bool lr1_state_equals(LR1Automaton *a, LR1State x, LR1State y);

/* lr1_lookahead_new
 *
 * RETURNS:
 * 	an empty LookaheadSet sized for the automaton
 */
LookaheadSet lr1_lookahead_new(LR1Automaton *a);

/* lr1_lookahead_copy
 *
 * RETURNS:
 * 	a copy of set, NULL if set is the wildcard
 */
LookaheadSet lr1_lookahead_copy(LR1Automaton *a, LookaheadSet set);

/* lr1_lookahead_delete
 *
 * Deletes a set and resets it to the wildcard
 */
void lr1_lookahead_delete(LookaheadSet *set);

/* lr1_lookahead_contains
 *
 * PARAMETERS:
 * 	set: set to check
 * 	terminal: index into LR1Automaton.terminals, one past the end being the end of input
 *
 * RETURNS:
 * 	true if found, false if not
 */
bool lr1_lookahead_contains(LookaheadSet set, size_t terminal);

/* lr1_lookahead_insert
 *
 * RETURNS:
 * 	true if the set grew, inserting into the wildcard never grows it
 */
bool lr1_lookahead_insert(LookaheadSet *set, size_t terminal);

/* lr1_lookahead_union
 *
 * Unions src into dst, a wildcard src turns dst into the wildcard
 *
 * RETURNS:
 * 	true if dst grew
 */
bool lr1_lookahead_union(LR1Automaton *a, LookaheadSet *dst, LookaheadSet src);

bool lr1_lookahead_intersects(LR1Automaton *a, LookaheadSet x, LookaheadSet y);

bool lr1_lookahead_empty(LR1Automaton *a, LookaheadSet set);

bool lr1_lookahead_equals(LR1Automaton *a, LookaheadSet x, LookaheadSet y);


#endif
