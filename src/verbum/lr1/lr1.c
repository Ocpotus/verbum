#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../symbol/symbol.h"
#include "../grammar/grammar.h"
#include "../lr0/lr0.h"

#include "lr1.h"


static const size_t NotFound = SIZE_MAX;


LR1Automaton lr1_new(Grammar g, LR0Automaton lr0) {
	LR1Automaton result = {
		.states = NULL,
		.transitions = NULL,
		.terminals = NULL,
		.word_count = 0,
	};

	symbol_set_iterate(g.terminals, it1, {
		cvector_push_back(result.terminals, *it1);
	});

	// One extra bit for the end of input
	result.word_count = (lr1_end(&result) + 1 + 63) / 64;

	for(LR0State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		LR1State s = lr1_state_new();

		for(LR0Item *it2 = cvector_begin(it1->items); it2 != cvector_end(it1->items); it2 += 1) {
			cvector_push_back(s.items, lr1_item_new(*it2, NULL));
		}

		cvector_push_back(result.states, s);
	}

	for(LR0Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		LR1Transition t = {
			.id = it1->id,
			.source = it1->source,
			.destination = it1->destination,
			.symbol = it1->symbol,
		};

		cvector_push_back(result.transitions, t);
	}

	return result;
}

void lr1_delete(LR1Automaton lr1) {
	for(LR1State *it1 = cvector_begin(lr1.states); it1 != cvector_end(lr1.states); it1 += 1) {
		lr1_state_delete(it1);
	}

	cvector_free(lr1.states);
	cvector_free(lr1.transitions);
	cvector_free(lr1.terminals);
}

size_t lr1_terminal_index(LR1Automaton *a, Symbol s) {
	for(size_t i = 0; i < cvector_size(a->terminals); i += 1) {
		if(symbol_equals(a->terminals[i], s)) {
			return i;
		}
	}

	return NotFound;
}

// Item
LR1Item lr1_item_new(LR0Item lr0, LookaheadSet lookahead) {
	return (LR1Item) {
		.lr0 = lr0,
		.lookahead = lookahead,
	};
}

LR1Item lr1_item_copy(LR1Automaton *a, LR1Item item) {
	return lr1_item_new(item.lr0, lr1_lookahead_copy(a, item.lookahead));
}

void lr1_item_delete(LR1Item *item) {
	// The rule is borrowed from the grammar, only the lookahead is owned
	lr1_lookahead_delete(&item->lookahead);
}

bool lr1_item_core_equals(LR0Item a, LR0Item b) {
	// Rules are unique per identifier so their definitions identify them
	return a.rule.definitions == b.rule.definitions && a.rule_index == b.rule_index && a.dot_pos == b.dot_pos;
}

bool lr1_item_equals(LR1Automaton *a, LR1Item x, LR1Item y) {
	return lr1_item_core_equals(x.lr0, y.lr0) && lr1_lookahead_equals(a, x.lookahead, y.lookahead);
}

// State
LR1State lr1_state_new() {
	return (LR1State) {
		.items = NULL,
	};
}

LR1State lr1_state_copy(LR1Automaton *a, LR1State state) {
	LR1State result = lr1_state_new();

	for(LR1Item *it1 = cvector_begin(state.items); it1 != cvector_end(state.items); it1 += 1) {
		cvector_push_back(result.items, lr1_item_copy(a, *it1));
	}

	return result;
}

void lr1_state_delete(LR1State *state) {
	for(LR1Item *it1 = cvector_begin(state->items); it1 != cvector_end(state->items); it1 += 1) {
		lr1_item_delete(it1);
	}

	cvector_free(state->items);
	state->items = NULL;
}

size_t lr1_state_find(LR1State state, LR0Item core) {
	for(size_t i = 0; i < cvector_size(state.items); i += 1) {
		if(lr1_item_core_equals(state.items[i].lr0, core)) {
			return i;
		}
	}

	return NotFound;
}

bool lr1_state_equals(LR1Automaton *a, LR1State x, LR1State y) {
	if(cvector_size(x.items) != cvector_size(y.items)) {
		return false;
	}

	// Cores are unique within a state so matching by core is enough
	for(LR1Item *it1 = cvector_begin(x.items); it1 != cvector_end(x.items); it1 += 1) {
		size_t i = lr1_state_find(y, it1->lr0);

		if(i == NotFound || !lr1_lookahead_equals(a, it1->lookahead, y.items[i].lookahead)) {
			return false;
		}
	}

	return true;
}

// LookaheadSet
LookaheadSet lr1_lookahead_new(LR1Automaton *a) {
	LookaheadSet result = memory_new(a->word_count * sizeof(uint64_t));

	memset(result, 0, a->word_count * sizeof(uint64_t));

	return result;
}

LookaheadSet lr1_lookahead_copy(LR1Automaton *a, LookaheadSet set) {
	if(set == NULL) {
		return set;
	}

	return memory_copy(set, a->word_count * sizeof(uint64_t));
}

void lr1_lookahead_delete(LookaheadSet *set) {
	memory_delete(*set);
	*set = NULL;
}

bool lr1_lookahead_contains(LookaheadSet set, size_t terminal) {
	if(set == NULL) {
		return true;
	}

	return (set[terminal / 64] >> (terminal % 64)) & 1;
}

bool lr1_lookahead_insert(LookaheadSet *set, size_t terminal) {
	if(*set == NULL || terminal == NotFound) {
		return false;
	}

	uint64_t bit = (uint64_t) 1 << (terminal % 64);
	bool result = !((*set)[terminal / 64] & bit);

	(*set)[terminal / 64] |= bit;

	return result;
}

bool lr1_lookahead_union(LR1Automaton *a, LookaheadSet *dst, LookaheadSet src) {
	if(*dst == NULL) {
		return false;
	}

	// Deleting leaves dst NULL, the wildcard
	if(src == NULL) {
		lr1_lookahead_delete(dst);

		return true;
	}

	bool result = false;

	for(size_t i = 0; i < a->word_count; i += 1) {
		uint64_t merged = (*dst)[i] | src[i];

		result |= merged != (*dst)[i];
		(*dst)[i] = merged;
	}

	return result;
}

bool lr1_lookahead_intersects(LR1Automaton *a, LookaheadSet x, LookaheadSet y) {
	if(x == NULL) {
		return y == NULL || !lr1_lookahead_empty(a, y);
	}

	if(y == NULL) {
		return !lr1_lookahead_empty(a, x);
	}

	for(size_t i = 0; i < a->word_count; i += 1) {
		if(x[i] & y[i]) {
			return true;
		}
	}

	return false;
}

bool lr1_lookahead_empty(LR1Automaton *a, LookaheadSet set) {
	if(set == NULL) {
		return false;
	}

	for(size_t i = 0; i < a->word_count; i += 1) {
		if(set[i] != 0) {
			return false;
		}
	}

	return true;
}

bool lr1_lookahead_equals(LR1Automaton *a, LookaheadSet x, LookaheadSet y) {
	if(x == NULL || y == NULL) {
		return x == y;
	}

	return memcmp(x, y, a->word_count * sizeof(uint64_t)) == 0;
}
