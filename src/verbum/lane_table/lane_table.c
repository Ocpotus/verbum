#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../lib/log.c/log.h"
#include "../../lib/c-vector/cvector.h"
//#include "../../lib/hashmap.c/hashmap.h"

#include "../memory/memory.h"
#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../rule_set/rule_set.h"
#include "../grammar/grammar.h"
#include "../lr0/lr0.h"
#include "../lr1/lr1.h"

#include "lane_table.h"


static const size_t NotFound = SIZE_MAX;

/* Builder
 *
 * Everything needed while constructing the automaton
 *
 * MEMBERS:
 * 	g: grammar being acted on
 * 	automaton: automaton being constructed
 * 	first: FIRST set of every nonterminal, indexed by symbol_set_index of g.nonterminals
 * 	nullable: bitset of the nonterminals that derive λ, indexed like first
 * 	settled: whether a state no longer needs to be resolved
 */
typedef struct Builder {
	Grammar g;
	LR1Automaton *automaton;
	cvector(LookaheadSet) first;
	uint64_t *nullable;
	cvector(bool) settled;
} Builder;

/* LaneRow
 *
 * A row of the lane table, the lookahead a state contributes to each conflicting action.
 * Rows double as nodes of a union-find, each set tracking a context (a lookahead per conflict).
 *
 * MEMBERS:
 * 	state: state the row describes
 * 	origin: state this row's state was cloned from, itself if not a clone
 * 	lookaheads: lookahead contributed by this state, one per conflict
 * 	successors: states in the table this state has an edge to
 * 	parent: union-find parent row
 * 	context: union of the lookaheads of every row in the set, only valid on the root
 * 	visited: whether the walk has reached this row
 */
typedef struct LaneRow {
	size_t state;
	size_t origin;
	cvector(LookaheadSet) lookaheads;
	cvector(size_t) successors;
	size_t parent;
	cvector(LookaheadSet) context;
	bool visited;
} LaneRow;

/* LaneTable
 *
 * One row per state reached while tracing. Every row holds a lookahead per conflicting action,
 * column 0 is every shift, the rest are reductions.
 */
typedef cvector(LaneRow) LaneTable;

typedef struct TraceKey {
	size_t state;
	const void *definitions;
	size_t rule_index;
	size_t dot_pos;
} TraceKey;

static inline bool item_complete(LR0Item i);
static Symbol item_next(LR0Item i);

/* compute_first
 *
 * Computes the FIRST set and nullability of every nonterminal
 *
 */
static void compute_first(Builder *b);
/* first_of_sequence
 *
 * Unions FIRST of definition[from..] into out
 *
 * PARAMETERS:
 * 	b: builder
 * 	definition: symbols to act on
 * 	from: index of the first symbol
 * 	out: set to union into
 * 	changed: set to true if out grew, may be NULL
 *
 * RETURNS:
 * 	true if definition[from..] derives λ
 */
static bool first_of_sequence(Builder *b, Definition definition, size_t from, LookaheadSet *out, bool *changed);
static bool nullable_contains(Builder *b, size_t nonterminal);
static void nullable_insert(Builder *b, size_t nonterminal);

static bool state_inconsistent(Builder *b, size_t state);
/* resolve
 *
 * Builds the lane table of an inconsistent state, then updates its lookaheads (LALR) or
 * splits the states along its lanes
 *
 */
static void resolve(Builder *b, size_t state);
/* trace
 *
 * Walks backwards from an item looking for the lookahead of a conflicting action
 *
 * PARAMETERS:
 * 	b: builder
 * 	t: table to record into
 * 	state: state the item belongs to
 * 	conflict: column of the conflicting action
 * 	item: item whose lookahead is wanted
 * 	visited: (state, item) pairs already traced
 */
static void trace(Builder *b, LaneTable *t, size_t state, size_t conflict, LR0Item item, struct hashmap *visited);
/* split
 *
 * Walks the lane table from its beachheads, merging contexts along edges and cloning states
 * whenever a merge would be inconsistent
 *
 * RETURNS:
 * 	false if a context could not be made consistent
 */
static bool split(Builder *b, LaneTable *t);
static bool walk(Builder *b, LaneTable *t, size_t start);
static size_t clone(Builder *b, LaneTable *t, size_t row);
static void redirect(Builder *b, LaneTable *t, size_t row, size_t successor, size_t destination);
static void assign(Builder *b, size_t state, cvector(size_t) reductions, cvector(LookaheadSet) context);
static void prune(Builder *b);

static size_t row_find(LaneTable *t, size_t state);
static size_t row_get(Builder *b, LaneTable *t, size_t state);
static size_t row_insert(Builder *b, LaneTable *t, size_t state, size_t num_conflicts);
static void row_add_successor(Builder *b, LaneTable *t, size_t state, size_t successor);
static size_t row_root(LaneTable *t, size_t row);
static void row_union(Builder *b, LaneTable *t, size_t x, size_t y);
static bool context_mergeable(Builder *b, size_t n, cvector(LookaheadSet) x, cvector(LookaheadSet) y);
static bool context_disjoint(Builder *b, cvector(LookaheadSet) context);
static size_t lane_table_conflicts(LaneTable t);
static void lane_table_free(LaneTable *t);

static inline uint64_t trace_key_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int trace_key_compare(const void *a, const void *b, void *udata);

LR1Automaton lane_table_new(Grammar g, LR0Automaton lr0, cvector(size_t) *conflicts) {
	// LR(0) -> LR(0-1), every item gets the wildcard lookahead
	LR1Automaton result = lr1_new(g, lr0);
	Builder b = {
		.g = g,
		.automaton = &result,
		.first = NULL,
		.nullable = NULL,
		.settled = NULL,
	};

	compute_first(&b);

	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		cvector_push_back(b.settled, false);
	}

	// Clones are appended so this doubles as the queue
	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		if(!b.settled[i] && state_inconsistent(&b, i)) {
			resolve(&b, i);
		}
	}

	prune(&b);

	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		if(state_inconsistent(&b, i)) {
			log_warn("state %zu is inconsistent, the grammar is not LR(1)", i);

			if(conflicts != NULL) {
				cvector_push_back(*conflicts, i);
			}
		}
	}

	for(LookaheadSet *it1 = cvector_begin(b.first); it1 != cvector_end(b.first); it1 += 1) {
		lr1_lookahead_delete(it1);
	}

	cvector_free(b.first);
	memory_delete(b.nullable);
	cvector_free(b.settled);

	return result;
}

static void compute_first(Builder *b) {
	log_debug("Entering %s", __FUNCTION__);
	size_t num_nonterminals = symbol_set_size(b->g.nonterminals);
	size_t nullable_words = (num_nonterminals + 63) / 64;

	for(size_t i = 0; i < num_nonterminals; i += 1) {
		cvector_push_back(b->first, lr1_lookahead_new(b->automaton));
	}

	b->nullable = memory_new(nullable_words * sizeof(uint64_t));
	memset(b->nullable, 0, nullable_words * sizeof(uint64_t));

	bool changed = true;

	while(changed) {
		changed = false;

		rule_set_iterate(b->g.rules, rule, {
			bool found = false;
			size_t n = symbol_set_index(b->g.nonterminals, rule->identifier, &found);

			if(!found) {
				continue;
			}

			for(Definition *it1 = cvector_begin(rule->definitions); it1 != cvector_end(rule->definitions); it1 += 1) {
				if(first_of_sequence(b, *it1, 0, &b->first[n], &changed) && !nullable_contains(b, n)) {
					nullable_insert(b, n);
					changed = true;
				}
			}
		});
	}
}

static bool first_of_sequence(Builder *b, Definition definition, size_t from, LookaheadSet *out, bool *changed) {
	log_debug("Entering %s", __FUNCTION__);
	bool grew = false;
	bool result = true;

	for(size_t i = from; i < cvector_size(definition); i += 1) {
		Symbol s = definition[i];

		if(s.mask & SymbolMask_Terminal) {
			grew |= lr1_lookahead_insert(out, lr1_terminal_index(b->automaton, s));
			result = false;
			break;
		}

		bool found = false;
		size_t n = symbol_set_index(b->g.nonterminals, s, &found);

		// Undefined nonterminal, derives nothing
		if(!found) {
			result = false;
			break;
		}

		grew |= lr1_lookahead_union(b->automaton, out, b->first[n]);

		if(!nullable_contains(b, n)) {
			result = false;
			break;
		}
	}

	if(changed != NULL) {
		*changed |= grew;
	}

	return result;
}

static bool nullable_contains(Builder *b, size_t nonterminal) {
	return (b->nullable[nonterminal / 64] >> (nonterminal % 64)) & 1;
}

static void nullable_insert(Builder *b, size_t nonterminal) {
	b->nullable[nonterminal / 64] |= (uint64_t) 1 << (nonterminal % 64);
}

static bool state_inconsistent(Builder *b, size_t state) {
	log_debug("Entering %s", __FUNCTION__);
	LR1State s = b->automaton->states[state];
	LookaheadSet shifts = lr1_lookahead_new(b->automaton);
	cvector(LookaheadSet) reductions = NULL;
	bool result = false;

	for(LR1Item *it1 = cvector_begin(s.items); it1 != cvector_end(s.items); it1 += 1) {
		if(item_complete(it1->lr0)) {
			cvector_push_back(reductions, it1->lookahead);
		} else if(item_next(it1->lr0).mask & SymbolMask_Terminal) {
			lr1_lookahead_insert(&shifts, lr1_terminal_index(b->automaton, item_next(it1->lr0)));
		}
	}

	for(size_t i = 0; i < cvector_size(reductions) && !result; i += 1) {
		result = lr1_lookahead_intersects(b->automaton, reductions[i], shifts);

		for(size_t j = i + 1; j < cvector_size(reductions) && !result; j += 1) {
			result = lr1_lookahead_intersects(b->automaton, reductions[i], reductions[j]);
		}
	}

	lr1_lookahead_delete(&shifts);
	cvector_free(reductions);

	return result;
}

static void resolve(Builder *b, size_t state) {
	log_debug("Entering %s", __FUNCTION__);
	LaneTable t = NULL;
	cvector(size_t) reductions = NULL;
	cvector(LookaheadSet) columns = NULL;

	log_debug("resolving state %zu", state);

	// Column 0 is every shift, they never conflict with each other
	for(size_t i = 0; i < cvector_size(b->automaton->states[state].items); i += 1) {
		if(item_complete(b->automaton->states[state].items[i].lr0)) {
			cvector_push_back(reductions, i);
		}
	}

	size_t num_conflicts = 1 + cvector_size(reductions);
	size_t row = row_insert(b, &t, state, num_conflicts);

	for(LR1Item *it1 = cvector_begin(b->automaton->states[state].items); it1 != cvector_end(b->automaton->states[state].items); it1 += 1) {
		if(!item_complete(it1->lr0) && (item_next(it1->lr0).mask & SymbolMask_Terminal)) {
			lr1_lookahead_insert(&t[row].lookaheads[0], lr1_terminal_index(b->automaton, item_next(it1->lr0)));
		}
	}

	// Step 3a: build the lane table
	for(size_t i = 0; i < cvector_size(reductions); i += 1) {
		struct hashmap *visited = hashmap_new_with_allocator(memory_new, memory_resize, memory_delete,
								     sizeof(TraceKey), 0, 0, 0,
								     trace_key_hash, trace_key_compare, NULL, NULL);

		trace(b, &t, state, i + 1, b->automaton->states[state].items[reductions[i]].lr0, visited);
		hashmap_free(visited);
	}

	// Step 3b: LALR, union every column
	for(size_t i = 0; i < num_conflicts; i += 1) {
		cvector_push_back(columns, lr1_lookahead_new(b->automaton));

		for(LaneRow *it1 = cvector_begin(t); it1 != cvector_end(t); it1 += 1) {
			lr1_lookahead_union(b->automaton, &columns[i], it1->lookaheads[i]);
		}
	}

	if(context_disjoint(b, columns)) {
		assign(b, state, reductions, columns);
		b->settled[state] = true;
	} else {
		// Step 3c: isolate lanes
		if(!split(b, &t)) {
			log_debug("state %zu could not be split into consistent states", state);
		}

		for(LaneRow *it1 = cvector_begin(t); it1 != cvector_end(t); it1 += 1) {
			if(it1->origin == state) {
				assign(b, it1->state, reductions, t[row_root(&t, it1 - t)].context);
				b->settled[it1->state] = true;
			}
		}
	}

	for(LookaheadSet *it1 = cvector_begin(columns); it1 != cvector_end(columns); it1 += 1) {
		lr1_lookahead_delete(it1);
	}

	cvector_free(columns);
	cvector_free(reductions);
	lane_table_free(&t);
}

static void trace(Builder *b, LaneTable *t, size_t state, size_t conflict, LR0Item item, struct hashmap *visited) {
	log_debug("Entering %s", __FUNCTION__);
	TraceKey key = {
		.state = state,
		.definitions = item.rule.definitions,
		.rule_index = item.rule_index,
		.dot_pos = item.dot_pos,
	};

	if(hashmap_get(visited, &key) != NULL) {
		return;
	}

	hashmap_set(visited, &key);

	// Dot is in the middle, the previous symbol was shifted by a predecessor
	if(item.dot_pos > 0) {
		Symbol previous = item.rule.definitions[item.rule_index][item.dot_pos - 1];
		LR0Item shifted = item;

		shifted.dot_pos -= 1;

		for(size_t i = 0; i < cvector_size(b->automaton->transitions); i += 1) {
			LR1Transition tr = b->automaton->transitions[i];

			if(tr.destination == state && symbol_equals(tr.symbol, previous)) {
				row_add_successor(b, t, tr.source, state);
				trace(b, t, tr.source, conflict, shifted, visited);
			}
		}

		return;
	}

	size_t row = row_get(b, t, state);

	if(symbol_equals(item.rule.identifier, b->g.start->identifier)) {
		lr1_lookahead_insert(&(*t)[row].lookaheads[conflict], lr1_end(b->automaton));
	}

	// Dot is at the front, look for the items in this state that brought the item in by closure
	for(size_t i = 0; i < cvector_size(b->automaton->states[state].items); i += 1) {
		LR0Item predecessor = b->automaton->states[state].items[i].lr0;

		if(item_complete(predecessor) || !symbol_equals(item_next(predecessor), item.rule.identifier)) {
			continue;
		}

		Definition definition = predecessor.rule.definitions[predecessor.rule_index];
		LookaheadSet first = lr1_lookahead_new(b->automaton);
		bool nullable = first_of_sequence(b, definition, predecessor.dot_pos + 1, &first, NULL);

		row = row_get(b, t, state);
		lr1_lookahead_union(b->automaton, &(*t)[row].lookaheads[conflict], first);
		lr1_lookahead_delete(&first);

		// Whatever follows the predecessor's nonterminal follows this item too
		if(nullable) {
			trace(b, t, state, conflict, predecessor, visited);
		}
	}
}

static bool split(Builder *b, LaneTable *t) {
	log_debug("Entering %s", __FUNCTION__);
	size_t num_conflicts = lane_table_conflicts(*t);
	bool result = true;
	size_t num_rows = cvector_size(*t);

	// Initially every row is its own set, its context being its own column entries
	for(LaneRow *it1 = cvector_begin(*t); it1 != cvector_end(*t); it1 += 1) {
		for(size_t i = 0; i < num_conflicts; i += 1) {
			lr1_lookahead_union(b->automaton, &it1->context[i], it1->lookaheads[i]);
		}
	}

	// Beachheads are rows not reachable from any other row
	for(size_t i = 0; i < num_rows; i += 1) {
		bool beachhead = true;

		for(size_t j = 0; j < num_rows && beachhead; j += 1) {
			if(j == i) {
				continue;
			}

			for(size_t *it1 = cvector_begin((*t)[j].successors); it1 != cvector_end((*t)[j].successors); it1 += 1) {
				if(*it1 == (*t)[i].state) {
					beachhead = false;
					break;
				}
			}
		}

		if(beachhead) {
			result &= walk(b, t, i);
		}
	}

	// Rows only reachable through a cycle
	for(size_t i = 0; i < cvector_size(*t); i += 1) {
		if(!(*t)[i].visited) {
			result &= walk(b, t, i);
		}
	}

	return result;
}

static bool walk(Builder *b, LaneTable *t, size_t start) {
	log_debug("Entering %s", __FUNCTION__);
	size_t num_conflicts = lane_table_conflicts(*t);
	cvector(size_t) stack = NULL;
	bool result = true;

	if((*t)[start].visited) {
		return result;
	}

	(*t)[start].visited = true;
	cvector_push_back(stack, start);

	while(!cvector_empty(stack)) {
		size_t x = stack[cvector_size(stack) - 1];

		cvector_pop_back(stack);

		for(size_t k = 0; k < cvector_size((*t)[x].successors); k += 1) {
			size_t y = row_find(t, (*t)[x].successors[k]);
			size_t rx = row_root(t, x);
			size_t ry = row_root(t, y);
			size_t chosen = NotFound;

			if(rx == ry || context_mergeable(b, num_conflicts, (*t)[rx].context, (*t)[ry].context)) {
				chosen = y;
			} else {
				// Prefer a clone already sharing our context, then any clone we can merge with
				for(size_t i = 0; i < cvector_size(*t) && chosen == NotFound; i += 1) {
					if((*t)[i].origin == (*t)[y].origin && row_root(t, i) == rx) {
						chosen = i;
					}
				}

				for(size_t i = 0; i < cvector_size(*t) && chosen == NotFound; i += 1) {
					if((*t)[i].origin == (*t)[y].origin &&
					   context_mergeable(b, num_conflicts, (*t)[rx].context, (*t)[row_root(t, i)].context)) {
						chosen = i;
					}
				}

				if(chosen == NotFound) {
					if(context_mergeable(b, num_conflicts, (*t)[rx].context, (*t)[y].lookaheads)) {
						chosen = clone(b, t, y);
					} else {
						// Not even a fresh clone fits, a genuine conflict
						chosen = y;
						result = false;
					}
				}

				if(chosen != y) {
					redirect(b, t, x, k, (*t)[chosen].state);
				}
			}

			row_union(b, t, rx, row_root(t, chosen));

			if(!(*t)[chosen].visited) {
				(*t)[chosen].visited = true;
				cvector_push_back(stack, chosen);
			}
		}
	}

	cvector_free(stack);

	return result;
}

static size_t clone(Builder *b, LaneTable *t, size_t row) {
	log_debug("Entering %s", __FUNCTION__);
	size_t num_conflicts = lane_table_conflicts(*t);
	LR1Automaton *a = b->automaton;
	size_t source = (*t)[row].state;
	size_t destination = cvector_size(a->states);
	size_t num_transitions = cvector_size(a->transitions);
	LR1State s = lr1_state_copy(a, a->states[source]);
	LaneRow r = {
		.state = destination,
		.origin = (*t)[row].origin,
		.lookaheads = NULL,
		.successors = NULL,
		.parent = cvector_size(*t),
		.context = NULL,
		.visited = false,
	};

	log_debug("cloning state %zu into %zu", source, destination);

	cvector_push_back(a->states, s);
	cvector_push_back(b->settled, b->settled[source]);

	// Outgoing edges stay the same, at least to start
	for(size_t i = 0; i < num_transitions; i += 1) {
		if(a->transitions[i].source == source) {
			LR1Transition tr = a->transitions[i];

			tr.id = cvector_size(a->transitions);
			tr.source = destination;
			cvector_push_back(a->transitions, tr);
		}
	}

	for(size_t i = 0; i < num_conflicts; i += 1) {
		cvector_push_back(r.lookaheads, lr1_lookahead_copy(b->automaton, (*t)[row].lookaheads[i]));
		cvector_push_back(r.context, lr1_lookahead_copy(b->automaton, (*t)[row].lookaheads[i]));
	}

	for(size_t *it1 = cvector_begin((*t)[row].successors); it1 != cvector_end((*t)[row].successors); it1 += 1) {
		cvector_push_back(r.successors, *it1);
	}

	cvector_push_back((*t), r);

	return cvector_size(*t) - 1;
}

static void redirect(Builder *b, LaneTable *t, size_t row, size_t successor, size_t destination) {
	size_t source = (*t)[row].state;
	size_t target = (*t)[row].successors[successor];

	log_debug("redirecting s%zu->s%zu to s%zu", source, target, destination);

	for(LR1Transition *it1 = cvector_begin(b->automaton->transitions); it1 != cvector_end(b->automaton->transitions); it1 += 1) {
		if(it1->source == source && it1->destination == target) {
			it1->destination = destination;
		}
	}

	(*t)[row].successors[successor] = destination;
}

static void assign(Builder *b, size_t state, cvector(size_t) reductions, cvector(LookaheadSet) context) {
	log_debug("Entering %s", __FUNCTION__);
	for(size_t i = 0; i < cvector_size(reductions); i += 1) {
		LookaheadSet *lookahead = &b->automaton->states[state].items[reductions[i]].lookahead;

		lr1_lookahead_delete(lookahead);
		*lookahead = lr1_lookahead_copy(b->automaton, context[i + 1]);
	}
}

/* Cloning can leave states without any predecessors, drop them and renumber
 */
static void prune(Builder *b) {
	log_debug("Entering %s", __FUNCTION__);
	LR1Automaton *a = b->automaton;
	cvector(size_t) mapping = NULL;
	cvector(size_t) stack = NULL;
	cvector(LR1State) states = NULL;
	cvector(LR1Transition) transitions = NULL;

	for(size_t i = 0; i < cvector_size(a->states); i += 1) {
		cvector_push_back(mapping, NotFound);
	}

	mapping[0] = 0;
	cvector_push_back(stack, 0);

	while(!cvector_empty(stack)) {
		size_t current = stack[cvector_size(stack) - 1];

		cvector_pop_back(stack);

		for(LR1Transition *it1 = cvector_begin(a->transitions); it1 != cvector_end(a->transitions); it1 += 1) {
			if(it1->source == current && mapping[it1->destination] == NotFound) {
				mapping[it1->destination] = 0;
				cvector_push_back(stack, it1->destination);
			}
		}
	}

	// Keep the original order
	for(size_t i = 0; i < cvector_size(a->states); i += 1) {
		if(mapping[i] == NotFound) {
			lr1_state_delete(&a->states[i]);
		} else {
			mapping[i] = cvector_size(states);
			cvector_push_back(states, a->states[i]);
		}
	}

	for(LR1Transition *it1 = cvector_begin(a->transitions); it1 != cvector_end(a->transitions); it1 += 1) {
		if(mapping[it1->source] != NotFound) {
			LR1Transition tr = *it1;

			tr.id = cvector_size(transitions);
			tr.source = mapping[tr.source];
			tr.destination = mapping[tr.destination];
			cvector_push_back(transitions, tr);
		}
	}

	cvector_free(a->states);
	cvector_free(a->transitions);
	a->states = states;
	a->transitions = transitions;

	cvector_free(mapping);
	cvector_free(stack);
}

// Lane table helpers
static size_t row_find(LaneTable *t, size_t state) {
	log_debug("Entering %s", __FUNCTION__);
	for(size_t i = 0; i < cvector_size(*t); i += 1) {
		if((*t)[i].state == state) {
			return i;
		}
	}

	return NotFound;
}

static size_t row_get(Builder *b, LaneTable *t, size_t state) {
	log_debug("Entering %s", __FUNCTION__);
	size_t result = row_find(t, state);

	if(result != NotFound) {
		return result;
	}

	return row_insert(b, t, state, lane_table_conflicts(*t));
}

static size_t row_insert(Builder *b, LaneTable *t, size_t state, size_t num_conflicts) {
	LaneRow r = {
		.state = state,
		.origin = state,
		.lookaheads = NULL,
		.successors = NULL,
		.parent = cvector_size(*t),
		.context = NULL,
		.visited = false,
	};

	for(size_t i = 0; i < num_conflicts; i += 1) {
		cvector_push_back(r.lookaheads, lr1_lookahead_new(b->automaton));
		cvector_push_back(r.context, lr1_lookahead_new(b->automaton));
	}

	cvector_push_back((*t), r);

	return cvector_size(*t) - 1;
}

static void row_add_successor(Builder *b, LaneTable *t, size_t state, size_t successor) {
	log_debug("Entering %s", __FUNCTION__);
	size_t row = row_get(b, t, state);

	row_get(b, t, successor);

	for(size_t *it1 = cvector_begin((*t)[row].successors); it1 != cvector_end((*t)[row].successors); it1 += 1) {
		if(*it1 == successor) {
			return;
		}
	}

	cvector_push_back((*t)[row].successors, successor);
}

static size_t row_root(LaneTable *t, size_t row) {
	log_debug("Entering %s", __FUNCTION__);
	while((*t)[row].parent != row) {
		row = (*t)[row].parent;
	}

	return row;
}

static void row_union(Builder *b, LaneTable *t, size_t x, size_t y) {
	log_debug("Entering %s", __FUNCTION__);
	if(x == y) {
		return;
	}

	(*t)[y].parent = x;

	for(size_t i = 0; i < cvector_size((*t)[x].context); i += 1) {
		lr1_lookahead_union(b->automaton, &(*t)[x].context[i], (*t)[y].context[i]);
	}
}

static bool context_mergeable(Builder *b, size_t n, cvector(LookaheadSet) x, cvector(LookaheadSet) y) {
	log_debug("Entering %s", __FUNCTION__);
	for(size_t i = 0; i < n; i += 1) {
		for(size_t j = i + 1; j < n; j += 1) {
			for(size_t w = 0; w < b->automaton->word_count; w += 1) {
				if((x[i][w] | y[i][w]) & (x[j][w] | y[j][w])) {
					return false;
				}
			}
		}
	}

	return true;
}

static bool context_disjoint(Builder *b, cvector(LookaheadSet) context) {
	log_debug("Entering %s", __FUNCTION__);
	return context_mergeable(b, cvector_size(context), context, context);
}

static void lane_table_free(LaneTable *t) {
	log_debug("Entering %s", __FUNCTION__);
	for(LaneRow *it1 = cvector_begin(*t); it1 != cvector_end(*t); it1 += 1) {
		for(size_t i = 0; i < cvector_size(it1->lookaheads); i += 1) {
			lr1_lookahead_delete(&it1->lookaheads[i]);
			lr1_lookahead_delete(&it1->context[i]);
		}

		cvector_free(it1->lookaheads);
		cvector_free(it1->context);
		cvector_free(it1->successors);
	}

	cvector_free(*t);
}

/* The inconsistent state's row is inserted first, so the table is never empty
 */
static size_t lane_table_conflicts(LaneTable t) {
	return cvector_size(t[0].lookaheads);
}

// Helper functions
static inline bool item_complete(LR0Item i) {
	return i.dot_pos >= cvector_size(i.rule.definitions[i.rule_index]);
}

static inline Symbol item_next(LR0Item i) {
	return i.rule.definitions[i.rule_index][i.dot_pos];
}

static inline uint64_t trace_key_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	return hashmap_sip(item, sizeof(TraceKey), seed0, seed1);
}

static inline int trace_key_compare(const void *a, const void *b, void *udata) {
	(void) udata;
	return memcmp(a, b, sizeof(TraceKey));
}
