#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../lib/log.c/log.h"
#include "../../lib/c-vector/cvector.h"
#include "../../lib/utf8.h/utf8.h"
#include "../../lib/hashmap.c/hashmap.h"

#include "../grammar/grammar.h"
#include "../rule/rule.h"
#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../memory/memory.h"

#include "dfa.h"


/* NFAEpsilon
 *
 * Codepoint of an edge taken without consuming input, no codepoint is negative
 */
static const utf8_int32_t NFAEpsilon = -1;

/* NFAEdge
 *
 * Represents an edge in an NFA.
 *
 * (source)-[codepoint]->(destination)
 *
 * NOTE:
 * 	source is implicit, as it is only used in the context of an NFAState
 *
 * MEMBERS:
 * 	destination: index of the state the edge leads to
 * 	codepoint: codepoint consumed to take the edge, NFAEpsilon if none
 */
typedef struct NFAEdge {
	uint32_t destination;
	utf8_int32_t codepoint;
} NFAEdge;

/* NFAState
 *
 * Represents a state in an NFA.
 *
 *		 /--[codepoint]->(destination)
 *		/
 * (source)-----[codepoint]->(destination)
 *
 * MEMBERS:
 * 	edges: outgoing edges
 * 	accept: TokenType ended by this state (the terminal's symbol_set_index), NonAccepting if none
 * 	priority: breaks ties between accepting states, lower wins, literals are 0 and named rules 1
 */
typedef struct NFAState {
	cvector(NFAEdge) edges;
	uint32_t accept;
	uint32_t priority;
} NFAState;

/* NFA
 *
 * Thompson NFA recognising every terminal of a grammar
 *
 * MEMBERS:
 * 	states: every state, edges refer to them by index
 * 	start: state with an epsilon edge into each terminal
 */
typedef struct NFA {
	cvector(NFAState) states;
	uint32_t start;
} NFA;

/* NFAFragment
 *
 * A piece of an NFA under construction, entered at source and left at destination
 */
typedef struct NFAFragment {
	uint32_t source;
	uint32_t destination;
} NFAFragment;

/* DFASetEntry
 *
 * Maps the sorted set of NFA states behind a DFA state to its id, the set is borrowed from
 * DFABuilder.sets
 */
typedef struct DFASetEntry {
	const uint32_t *set;
	size_t length;
	uint32_t id;
} DFASetEntry;

/* DFABuilder
 *
 * MEMBERS:
 * 	nfa: NFA being converted
 * 	dfa: DFA being built
 * 	visited: scratch for nfa_closure, one per NFA state, all false between calls
 * 	sets: NFA states behind each DFA state, indexed by DFA id, owns the sets
 * 	ids: set to DFA id (see DFASetEntry)
 * 	terminals: terminal symbols by index, for warnings
 * 	warned: num_terminals * num_terminals, true once an ambiguity between two terminals is logged
 */
typedef struct DFABuilder {
	NFA nfa;
	DFA *dfa;
	bool *visited;
	cvector(cvector(uint32_t)) sets;
	struct hashmap *ids;
	Symbol *terminals;
	size_t num_terminals;
	bool *warned;
} DFABuilder;

/* NFAMove
 *
 * A non epsilon NFA edge leaving a DFA state's set, gathered to compute that state's edges
 */
typedef struct NFAMove {
	utf8_int32_t codepoint;
	uint32_t destination;
} NFAMove;

static bool nfa_build(Grammar g, NFA *out);
static void nfa_delete(NFA *nfa);
static void nfa_print(FILE *fp, NFA nfa) __attribute__((unused));
static void nfa_closure(DFABuilder *b, cvector(uint32_t) *set);

static uint32_t dfa_state(DFABuilder *b, cvector(uint32_t) set);
static uint32_t dfa_resolve_accept(DFABuilder *b, const uint32_t *set, size_t length);
static void dfa_minimise(DFA *dfa);
static uint32_t dfa_partition(cvector(uint32_t) *signatures, size_t count, uint32_t *out);
static uint32_t dfa_destination(DFAState state, utf8_int32_t codepoint);

static int codepoint_compare(const void *a, const void *b);

static uint64_t dfa_set_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int dfa_set_compare(const void *a, const void *b, void *udata);
static int nfa_move_compare(const void *a, const void *b);
static int uint32_compare(const void *a, const void *b);
static void print_codepoint(FILE *fp, utf8_int32_t codepoint);
static uint32_t nfa_state(NFA *nfa);
static void nfa_edge(NFA *nfa, uint32_t source, uint32_t destination, utf8_int32_t codepoint);

static NFAFragment nfa_fragment_epsilon(NFA *nfa);
static NFAFragment nfa_fragment_literal(NFA *nfa, Lexeme l);
static NFAFragment nfa_fragment_concatenation(NFA *nfa, NFAFragment a, NFAFragment b);
static NFAFragment nfa_fragment_star(NFA *nfa, NFAFragment a);
static NFAFragment build_symbol(NFA *nfa, Grammar g, Symbol symbol, cvector(Symbol) *stack, bool *ok);
static NFAFragment build_definition(NFA *nfa, Grammar g, Symbol *symbols, size_t count, cvector(Symbol) *stack, bool *ok);
static NFAFragment build_rule(NFA *nfa, Grammar g, Symbol identifier, cvector(Symbol) *stack, bool *ok);

static bool symbol_is_literal(Symbol s);

bool dfa_build(Grammar g, DFA *out) {
	DFABuilder b = { .dfa = out };
	cvector(uint32_t) start = NULL;
	cvector(NFAMove) moves = NULL;

	*out = (DFA) { 0 };

	if(!nfa_build(g, &b.nfa)) {
		nfa_delete(&b.nfa);

		return false;
	}

	b.num_terminals = symbol_set_size(g.terminals);
	b.visited = memory_initialize(cvector_size(b.nfa.states) * sizeof(bool), 0);
	b.terminals = memory_initialize((b.num_terminals + 1) * sizeof(Symbol), 0);
	b.warned = memory_initialize((b.num_terminals * b.num_terminals + 1) * sizeof(bool), 0);
	b.ids = hashmap_new_with_allocator(memory_new, memory_resize, memory_delete,
					   sizeof(DFASetEntry), 0, 0, 0,
					   dfa_set_hash, dfa_set_compare, NULL, NULL);

	symbol_set_iterate(g.terminals, it1, {
		bool unused;
		b.terminals[symbol_set_index(g.terminals, *it1, &unused)] = *it1;
	});

	// Dead state, it has no set of NFA states
	cvector_push_back(out->states, ((DFAState) { .edges = NULL, .accept = NonAccepting }));
	cvector_push_back(b.sets, NULL);

	cvector_push_back(start, b.nfa.start);
	nfa_closure(&b, &start);
	dfa_state(&b, start);

	// Ids are handed out in creation order, so states past the one being processed are the worklist
	for(size_t id = DFAStart; id < cvector_size(out->states); id += 1) {
		cvector_clear(moves);

		for(uint32_t *it1 = cvector_begin(b.sets[id]); it1 != cvector_end(b.sets[id]); it1 += 1) {
			NFAState state = b.nfa.states[*it1];

			for(NFAEdge *it2 = cvector_begin(state.edges); it2 != cvector_end(state.edges); it2 += 1) {
				if(it2->codepoint != NFAEpsilon) {
					cvector_push_back(moves, ((NFAMove) { it2->codepoint, it2->destination }));
				}
			}
		}

		if(cvector_size(moves) > 0) {
			qsort(moves, cvector_size(moves), sizeof(NFAMove), nfa_move_compare);
		}

		// Each run of equal codepoints is one edge, runs come in codepoint order so edges stay sorted
		for(size_t i = 0; i < cvector_size(moves);) {
			utf8_int32_t codepoint = moves[i].codepoint;
			cvector(uint32_t) target = NULL;

			for(; i < cvector_size(moves) && moves[i].codepoint == codepoint; i += 1) {
				// Sorted, so duplicates are adjacent
				if(cvector_size(target) == 0 || target[cvector_size(target) - 1] != moves[i].destination) {
					cvector_push_back(target, moves[i].destination);
				}
			}

			nfa_closure(&b, &target);

			// dfa_state may reallocate out->states, index it after the call
			uint32_t destination = dfa_state(&b, target);
			cvector_push_back(out->states[id].edges, ((DFAEdge) { codepoint, destination }));
		}
	}

	for(size_t i = 0; i < cvector_size(b.sets); i += 1) {
		cvector_free(b.sets[i]);
	}

	cvector_free(b.sets);
	cvector_free(moves);
	hashmap_free(b.ids);
	memory_delete(b.visited);
	memory_delete(b.terminals);
	memory_delete(b.warned);
	nfa_delete(&b.nfa);

	size_t unminimised = cvector_size(out->states);

	dfa_minimise(out);
	log_debug("DFA minimised from %zu to %zu states", unminimised, cvector_size(out->states));

	return true;
}

void dfa_delete(DFA *dfa) {
	for(DFAState *it1 = cvector_begin(dfa->states); it1 != cvector_end(dfa->states); it1 += 1) {
		cvector_free(it1->edges);
	}

	cvector_free(dfa->states);
	*dfa = (DFA) { 0 };
}

void dfa_table_build(DFA dfa, DFATable *out) {
	cvector(utf8_int32_t) codepoints = NULL;
	cvector(cvector(uint32_t)) columns = NULL;
	size_t num_states = cvector_size(dfa.states);

	*out = (DFATable) {
		.num_states = (uint32_t) num_states,
		.num_classes = 1,
	};

	// Every codepoint some edge consumes, sorted and unique
	for(DFAState *it1 = cvector_begin(dfa.states); it1 != cvector_end(dfa.states); it1 += 1) {
		for(DFAEdge *it2 = cvector_begin(it1->edges); it2 != cvector_end(it1->edges); it2 += 1) {
			cvector_push_back(codepoints, it2->codepoint);
		}
	}

	if(cvector_size(codepoints) > 0) {
		qsort(codepoints, cvector_size(codepoints), sizeof(utf8_int32_t), codepoint_compare);
	}

	size_t num_codepoints = 0;

	for(size_t i = 0; i < cvector_size(codepoints); i += 1) {
		if(num_codepoints == 0 || codepoints[num_codepoints - 1] != codepoints[i]) {
			codepoints[num_codepoints] = codepoints[i];
			num_codepoints += 1;
		}
	}

	// A codepoint's column is where it leads from every state, equal columns share a class
	uint32_t *classes = memory_new((num_codepoints + 1) * sizeof(uint32_t));

	cvector_resize(columns, num_codepoints, NULL);

	for(size_t i = 0; i < num_codepoints; i += 1) {
		for(size_t state = 0; state < num_states; state += 1) {
			cvector_push_back(columns[i], dfa_destination(dfa.states[state], codepoints[i]));
		}
	}

	// Class 0 is every codepoint no edge consumes
	out->num_classes = 1 + dfa_partition(columns, num_codepoints, classes);
	out->next = memory_initialize((num_states * out->num_classes + 1) * sizeof(uint32_t), 0);
	out->accept = memory_new((num_states + 1) * sizeof(uint32_t));

	for(size_t i = 0; i < num_codepoints; i += 1) {
		uint32_t class = classes[i] + 1;
		size_t last = cvector_size(out->ranges);

		for(size_t state = 0; state < num_states; state += 1) {
			out->next[state * out->num_classes + class] = columns[i][state];
		}

		if(last > 0 && out->ranges[last - 1].class == class && out->ranges[last - 1].last + 1 == codepoints[i]) {
			out->ranges[last - 1].last = codepoints[i];
		} else {
			cvector_push_back(out->ranges, ((DFAClassRange) { codepoints[i], codepoints[i], class }));
		}
	}

	for(size_t state = 0; state < num_states; state += 1) {
		out->accept[state] = dfa.states[state].accept;
	}

	for(size_t i = 0; i < num_codepoints; i += 1) {
		cvector_free(columns[i]);
	}

	cvector_free(columns);
	cvector_free(codepoints);
	memory_delete(classes);
}

void dfa_table_delete(DFATable *table) {
	memory_delete(table->next);
	memory_delete(table->accept);
	cvector_free(table->ranges);
	*table = (DFATable) { 0 };
}

/* dfa_destination
 *
 * Follows a state's edge for a codepoint, edges are sorted so this is a binary search
 *
 * RETURNS:
 * 	destination state, DFADead if the state has no edge for the codepoint
 */
static uint32_t dfa_destination(DFAState state, utf8_int32_t codepoint) {
	size_t low = 0;
	size_t high = cvector_size(state.edges);

	while(low < high) {
		size_t middle = low + (high - low) / 2;

		if(state.edges[middle].codepoint < codepoint) {
			low = middle + 1;
		} else if(state.edges[middle].codepoint > codepoint) {
			high = middle;
		} else {
			return state.edges[middle].destination;
		}
	}

	return DFADead;
}

static int codepoint_compare(const void *a, const void *b) {
	utf8_int32_t x = *(const utf8_int32_t *) a;
	utf8_int32_t y = *(const utf8_int32_t *) b;

	return (x > y) - (x < y);
}

void dfa_print(FILE *fp, DFA dfa) {
	for(size_t i = 0; i < cvector_size(dfa.states); i += 1) {
		DFAState state = dfa.states[i];

		fprintf(fp, "%zu", i);

		if(state.accept != NonAccepting) {
			fprintf(fp, " [accept %u]", state.accept);
		}

		fprintf(fp, ":");

		for(size_t j = 0; j < cvector_size(state.edges);) {
			size_t last = j;

			while(last + 1 < cvector_size(state.edges) &&
			      state.edges[last + 1].codepoint == state.edges[last].codepoint + 1 &&
			      state.edges[last + 1].destination == state.edges[j].destination) {
				last += 1;
			}

			fprintf(fp, " ");
			print_codepoint(fp, state.edges[j].codepoint);

			if(last != j) {
				fprintf(fp, "-");
				print_codepoint(fp, state.edges[last].codepoint);
			}

			fprintf(fp, "->%u", state.edges[j].destination);
			j = last + 1;
		}

		fprintf(fp, "\n");
	}
}

/* nfa_closure
 *
 * Expands a set of NFA states with every state reachable through epsilon edges, then sorts it so
 * equal closures compare equal. The visited check also stops at the epsilon loops of a star.
 *
 * PARAMETERS:
 * 	b: builder holding the NFA and the visited scratch
 * 	set: duplicate free states to expand, replaced with the sorted closure
 */
static void nfa_closure(DFABuilder *b, cvector(uint32_t) *set) {
	cvector(uint32_t) stack = NULL;

	for(uint32_t *it1 = cvector_begin(*set); it1 != cvector_end(*set); it1 += 1) {
		b->visited[*it1] = true;
		cvector_push_back(stack, *it1);
	}

	while(cvector_size(stack) > 0) {
		uint32_t state = stack[cvector_size(stack) - 1];

		cvector_pop_back(stack);

		for(NFAEdge *it1 = cvector_begin(b->nfa.states[state].edges); it1 != cvector_end(b->nfa.states[state].edges); it1 += 1) {
			if(it1->codepoint == NFAEpsilon && !b->visited[it1->destination]) {
				b->visited[it1->destination] = true;
				cvector_push_back(*set, it1->destination);
				cvector_push_back(stack, it1->destination);
			}
		}
	}

	// Reset only what was touched, keeping each call proportional to the closure
	for(uint32_t *it1 = cvector_begin(*set); it1 != cvector_end(*set); it1 += 1) {
		b->visited[*it1] = false;
	}

	if(cvector_size(*set) > 0) {
		qsort(*set, cvector_size(*set), sizeof(uint32_t), uint32_compare);
	}

	cvector_free(stack);
}

/* dfa_state
 *
 * Finds the DFA state for a closure, creating it if needed
 *
 * PARAMETERS:
 * 	b: builder
 * 	set: sorted closure, ownership moves to the builder, it is freed if the state already exists
 *
 * RETURNS:
 * 	id of the DFA state
 */
static uint32_t dfa_state(DFABuilder *b, cvector(uint32_t) set) {
	DFASetEntry key = {
		.set = set,
		.length = cvector_size(set),
	};
	const DFASetEntry *found = hashmap_get(b->ids, &key);

	if(found != NULL) {
		cvector_free(set);

		return found->id;
	}

	key.id = (uint32_t) cvector_size(b->dfa->states);

	cvector_push_back(b->sets, set);
	cvector_push_back(b->dfa->states, ((DFAState) {
		.edges = NULL,
		.accept = dfa_resolve_accept(b, set, cvector_size(set)),
	}));
	hashmap_set(b->ids, &key);

	return key.id;
}

/* dfa_resolve_accept
 *
 * Picks the token a DFA state accepts, the lowest (priority, accept) among its NFA states.
 * A loser with the same priority as the winner is an ambiguity in the grammar and is logged once
 * per pair of terminals.
 *
 * RETURNS:
 * 	winning TokenType, NonAccepting if no NFA state accepts
 */
static uint32_t dfa_resolve_accept(DFABuilder *b, const uint32_t *set, size_t length) {
	uint32_t accept = NonAccepting;
	uint32_t priority = UINT32_MAX;

	for(size_t i = 0; i < length; i += 1) {
		NFAState state = b->nfa.states[set[i]];

		if(state.accept == NonAccepting) {
			continue;
		}

		if(state.priority < priority || (state.priority == priority && state.accept < accept)) {
			accept = state.accept;
			priority = state.priority;
		}
	}

	for(size_t i = 0; i < length && accept != NonAccepting; i += 1) {
		NFAState state = b->nfa.states[set[i]];

		if(state.accept == NonAccepting || state.accept == accept || state.priority != priority) {
			continue;
		}

		if(!b->warned[accept * b->num_terminals + state.accept]) {
			Symbol winner = b->terminals[accept];
			Symbol loser = b->terminals[state.accept];

			b->warned[accept * b->num_terminals + state.accept] = true;
			log_warn("tokens %.*s and %.*s match the same text, %.*s wins",
				 (int) winner.lexeme.length, winner.lexeme.data,
				 (int) loser.lexeme.length, loser.lexeme.data,
				 (int) winner.lexeme.length, winner.lexeme.data);
		}
	}

	return accept;
}

/* dfa_minimise
 *
 * Merges states no input can tell apart, using Moore's partition refinement. States start grouped
 * by what they accept, then a group is split whenever its states lead to different groups on the
 * same codepoint, until nothing splits. Each final group becomes one state.
 *
 * Blocks are numbered in order of first appearance by state index, so DFADead and DFAStart keep
 * their ids. Dead is seeded into a block of its own, nothing else can merge into it.
 *
 * Comparing edge lists treats a missing edge as leading to dead. That is exact because every
 * other state can reach an accepting state, every path through the NFA ends in a terminal.
 *
 * PARAMETERS:
 * 	dfa: DFA to minimise in place
 */
static void dfa_minimise(DFA *dfa) {
	size_t num_states = cvector_size(dfa->states);
	cvector(cvector(uint32_t)) signatures = NULL;
	cvector(DFAState) minimised = NULL;
	uint32_t *group = memory_new((num_states + 1) * sizeof(uint32_t));
	uint32_t *next = memory_new((num_states + 1) * sizeof(uint32_t));
	uint32_t num_groups;

	cvector_resize(signatures, num_states, NULL);

	// Initial partition, by token accepted
	for(size_t i = 0; i < num_states; i += 1) {
		cvector_push_back(signatures[i], dfa->states[i].accept);
		cvector_push_back(signatures[i], (uint32_t) (i == DFADead));
	}

	num_groups = dfa_partition(signatures, num_states, group);

	while(true) {
		for(size_t i = 0; i < num_states; i += 1) {
			DFAState state = dfa->states[i];

			cvector_clear(signatures[i]);
			cvector_push_back(signatures[i], group[i]);

			for(DFAEdge *it1 = cvector_begin(state.edges); it1 != cvector_end(state.edges); it1 += 1) {
				cvector_push_back(signatures[i], (uint32_t) it1->codepoint);
				cvector_push_back(signatures[i], group[it1->destination]);
			}
		}

		uint32_t num_next = dfa_partition(signatures, num_states, next);
		uint32_t *swap = group;

		group = next;
		next = swap;

		// Refinement only splits, an unchanged count means nothing split
		if(num_next == num_groups) {
			break;
		}

		num_groups = num_next;
	}

	// Blocks are numbered by first appearance, so a block's first member is seen as the count reaches it
	for(size_t i = 0; i < num_states; i += 1) {
		if(group[i] != cvector_size(minimised)) {
			continue;
		}

		DFAState representative = dfa->states[i];
		DFAState state = {
			.edges = NULL,
			.accept = representative.accept,
		};

		for(DFAEdge *it1 = cvector_begin(representative.edges); it1 != cvector_end(representative.edges); it1 += 1) {
			cvector_push_back(state.edges, ((DFAEdge) { it1->codepoint, group[it1->destination] }));
		}

		cvector_push_back(minimised, state);
	}

	for(size_t i = 0; i < num_states; i += 1) {
		cvector_free(signatures[i]);
	}

	cvector_free(signatures);
	memory_delete(group);
	memory_delete(next);
	dfa_delete(dfa);
	dfa->states = minimised;
}

/* dfa_partition
 *
 * Groups items with equal signatures, numbering groups in order of first appearance
 *
 * PARAMETERS:
 * 	signatures: one per item, borrowed
 * 	count: number of items
 * 	out: receives each item's group
 *
 * RETURNS:
 * 	number of groups
 */
static uint32_t dfa_partition(cvector(uint32_t) *signatures, size_t count, uint32_t *out) {
	struct hashmap *groups = hashmap_new_with_allocator(memory_new, memory_resize, memory_delete,
							    sizeof(DFASetEntry), 0, 0, 0,
							    dfa_set_hash, dfa_set_compare, NULL, NULL);
	uint32_t num_groups = 0;

	for(size_t i = 0; i < count; i += 1) {
		DFASetEntry key = {
			.set = signatures[i],
			.length = cvector_size(signatures[i]),
			.id = num_groups,
		};
		const DFASetEntry *found = hashmap_get(groups, &key);

		if(found != NULL) {
			out[i] = found->id;
		} else {
			hashmap_set(groups, &key);
			out[i] = num_groups;
			num_groups += 1;
		}
	}

	hashmap_free(groups);

	return num_groups;
}

static uint64_t dfa_set_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const DFASetEntry *entry = item;

	return hashmap_sip(entry->set, entry->length * sizeof(uint32_t), seed0, seed1);
}

static int dfa_set_compare(const void *a, const void *b, void *udata) {
	(void) udata;
	const DFASetEntry *x = a;
	const DFASetEntry *y = b;

	if(x->length != y->length) {
		return x->length < y->length ? -1 : 1;
	}

	return memcmp(x->set, y->set, x->length * sizeof(uint32_t));
}

static int nfa_move_compare(const void *a, const void *b) {
	const NFAMove *x = a;
	const NFAMove *y = b;

	if(x->codepoint != y->codepoint) {
		return x->codepoint < y->codepoint ? -1 : 1;
	}

	return (x->destination > y->destination) - (x->destination < y->destination);
}

static int uint32_compare(const void *a, const void *b) {
	uint32_t x = *(const uint32_t *) a;
	uint32_t y = *(const uint32_t *) b;

	return (x > y) - (x < y);
}

static void print_codepoint(FILE *fp, utf8_int32_t codepoint) {
	if(codepoint > ' ' && codepoint < 0x7F) {
		fprintf(fp, "'%c'", (char) codepoint);
	} else {
		fprintf(fp, "U+%04X", (unsigned) codepoint);
	}
}

static bool nfa_build(Grammar g, NFA *out) {
	bool ok = true;
	cvector(Symbol) stack = NULL;

	*out = (NFA) { 0 };
	out->start = nfa_state(out);

	symbol_set_iterate(g.terminals, it1, {
		bool found;
		uint32_t index = (uint32_t) symbol_set_index(g.terminals, *it1, &found);
		NFAFragment fragment = build_symbol(out, g, *it1, &stack, &ok);

		nfa_edge(out, out->start, fragment.source, NFAEpsilon);
		out->states[fragment.destination].accept = index;
		out->states[fragment.destination].priority = symbol_is_literal(*it1) ? 0 : 1;
	});

	cvector_free(stack);

	return ok;
}

static void nfa_delete(NFA *nfa) {
	for(NFAState *it1 = cvector_begin(nfa->states); it1 != cvector_end(nfa->states); it1 += 1) {
		cvector_free(it1->edges);
	}

	cvector_free(nfa->states);
	*nfa = (NFA) { 0 };
}

static void nfa_print(FILE *fp, NFA nfa) {
	fprintf(fp, "start: %u\n", nfa.start);

	for(size_t i = 0; i < cvector_size(nfa.states); i += 1) {
		NFAState state = nfa.states[i];

		fprintf(fp, "%zu", i);

		if(state.accept != NonAccepting) {
			fprintf(fp, " [accept %u, priority %u]", state.accept, state.priority);
		}

		fprintf(fp, ":");

		for(NFAEdge *it1 = cvector_begin(state.edges); it1 != cvector_end(state.edges); it1 += 1) {
			if(it1->codepoint == NFAEpsilon) {
				fprintf(fp, " ε->%u", it1->destination);
			} else if(it1->codepoint > ' ' && it1->codepoint < 0x7F) {
				fprintf(fp, " '%c'->%u", (char) it1->codepoint, it1->destination);
			} else {
				fprintf(fp, " U+%04X->%u", (unsigned) it1->codepoint, it1->destination);
			}
		}

		fprintf(fp, "\n");
	}
}

/* build_symbol
 *
 * Builds a fragment for a literal, or for the rule a symbol names
 */
static NFAFragment build_symbol(NFA *nfa, Grammar g, Symbol symbol, cvector(Symbol) *stack, bool *ok) {
	if(symbol_is_literal(symbol)) {
		return nfa_fragment_literal(nfa, symbol.lexeme);
	}

	return build_rule(nfa, g, symbol, stack, ok);
}

/* build_definition
 *
 * Concatenates the fragments of the first count symbols, an empty definition is epsilon
 */
static NFAFragment build_definition(NFA *nfa, Grammar g, Symbol *symbols, size_t count, cvector(Symbol) *stack, bool *ok) {
	NFAFragment result = nfa_fragment_epsilon(nfa);

	for(size_t i = 0; i < count; i += 1) {
		result = nfa_fragment_concatenation(nfa, result, build_symbol(nfa, g, symbols[i], stack, ok));
	}

	return result;
}

/* build_rule
 *
 * Builds a fresh fragment for a rule each time it is referenced, sharing one would let paths
 * entering from one reference leave through another.
 *
 * Optional and group rules need nothing special, the ε alternative of an optional is an empty
 * definition. A repetition is hoisted as R → X R | ε, so its ε alternative and trailing
 * self reference are dropped and the rest is starred.
 *
 * PARAMETERS:
 * 	stack: rules being expanded, a reference to one of them is recursion
 * 	ok: set to false on error
 */
static NFAFragment build_rule(NFA *nfa, Grammar g, Symbol identifier, cvector(Symbol) *stack, bool *ok) {
	// Terminals carry the terminal bit, the stored rule does not
	identifier.mask &= ~SymbolMask_Terminal;

	// Recursion is not allowed in in a lexical rule
	for(Symbol *it1 = cvector_begin(*stack); it1 != cvector_end(*stack); it1 += 1) {
		if(symbol_equals(*it1, identifier)) {
			log_error("lexical rule %.*s is recursive", (int) identifier.lexeme.length, identifier.lexeme.data);
			*ok = false;

			return nfa_fragment_epsilon(nfa);
		}
	}

	Rule rule = rule_set_get(g.rules, identifier);

	if(rule.identifier.lexeme.data == NULL) {
		log_error("undefined lexical rule %.*s", (int) identifier.lexeme.length, identifier.lexeme.data);
		*ok = false;

		return nfa_fragment_epsilon(nfa);
	}

	bool repetition = identifier.mask & SymbolMask_Repetition;
	uint32_t source = nfa_state(nfa);
	uint32_t destination = nfa_state(nfa);

	cvector_push_back(*stack, identifier);

	for(Definition *it1 = cvector_begin(rule.definitions); it1 != cvector_end(rule.definitions); it1 += 1) {
		size_t count = cvector_size(*it1);

		if(repetition) {
			// ε alternative, the star supplies it
			if(count == 0) {
				continue;
			}

			// Trailing self reference
			count -= 1;
		}

		NFAFragment fragment = build_definition(nfa, g, *it1, count, stack, ok);

		nfa_edge(nfa, source, fragment.source, NFAEpsilon);
		nfa_edge(nfa, fragment.destination, destination, NFAEpsilon);
	}

	cvector_pop_back(*stack);

	NFAFragment result = { source, destination };

	return repetition ? nfa_fragment_star(nfa, result) : result;
}

static bool symbol_is_literal(Symbol s) {
	return s.lexeme.length > 0 && (s.lexeme.data[0] == '"' || s.lexeme.data[0] == '\'');
}

/* nfa_state
 *
 * Appends a non accepting state, may reallocate states so never hold a pointer into it across calls
 *
 * RETURNS:
 * 	index of the new state
 */
static uint32_t nfa_state(NFA *nfa) {
	NFAState new = {
		.accept = NonAccepting,
		.edges = NULL,
		.priority = 0,
	};

	cvector_push_back(nfa->states, new);

	return (uint32_t) cvector_size(nfa->states) - 1;
}

static void nfa_edge(NFA *nfa, uint32_t source, uint32_t destination, utf8_int32_t codepoint) {
	NFAEdge edge = {
		.destination = destination,
		.codepoint = codepoint,
	};

	cvector_push_back(nfa->states[source].edges, edge);
}

static NFAFragment nfa_fragment_epsilon(NFA *nfa) {
	uint32_t s = nfa_state(nfa);

	return (NFAFragment) { s, s };
}

/* nfa_fragment_literal
 *
 * Chains one edge per codepoint, the lexeme keeps its quotes and has no escapes
 */
static NFAFragment nfa_fragment_literal(NFA *nfa, Lexeme l) {
	uint32_t source = nfa_state(nfa);
	uint32_t current = source;
	const char *it = l.data + 1;
	const char *end = l.data + l.length - 1;

	while(it < end) {
		utf8_int32_t codepoint;
		uint32_t next = nfa_state(nfa);

		it = utf8codepoint(it, &codepoint);
		nfa_edge(nfa, current, next, codepoint);
		current = next;
	}

	return (NFAFragment) { source, current };
}

static NFAFragment nfa_fragment_concatenation(NFA *nfa, NFAFragment a, NFAFragment b) {
	nfa_edge(nfa, a.destination, b.source, NFAEpsilon);

	return (NFAFragment) { a.source, b.destination };
}

static NFAFragment nfa_fragment_star(NFA *nfa, NFAFragment a) {
	uint32_t source = nfa_state(nfa);
	uint32_t destination = nfa_state(nfa);

	nfa_edge(nfa, source, a.source, NFAEpsilon);			// once
	nfa_edge(nfa, source, destination, NFAEpsilon);			// zero times
	nfa_edge(nfa, a.destination, a.source, NFAEpsilon);		// again
	nfa_edge(nfa, a.destination, destination, NFAEpsilon);		// done

	return (NFAFragment) { source, destination };
}
