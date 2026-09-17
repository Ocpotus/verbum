/* #include "../../lib/hashmap.c/hashmap.h"
#include "../../lib/c-vector/cvector.h"

#include "../lr0/lr0.h"
#include "../grammar/grammar.h"
#include "../token/token.h"

#include "lr1.h"


static void lr1_closure(Grammar g, LR1State *s);
static LR1State lr1_goto(Grammar g, LR1State *s, Token t);
static TokenSet calculate_first_set_for_suffix(Grammar g, TokenSet def, size_t dot_pos, Token lookahead);
static bool lr1_state_contains(LR1State state, LR1Item item);
static void item_delete(LR1Item *i);
static void state_delete(LR1State *s);
static bool state_contains(LR1State s, LR1Item i);
static bool state_equals(LR1State a, LR1State b);

LR1Parser lr1_new(Grammar g) {
	LR1Parser result = {
		.states = NULL,
		.transitions = NULL,
	};
	LR1State s0 = {
		.items = NULL,
	};
	LR1Item i0 = {
		.lr0 = {
			.dot_pos = 0,
			.rule = *g.start,
		},
		.lookahead = (Token) { .lexeme = { .data = NULL, .length = 0, }, .tag = TokenType_Invalid },
	};
	cvector(Token) tokens = NULL;
	size_t transition_id = 0;

	cvector_push_back(s0.items, i0);

	for(Token *it = cvector_begin(g.terminals); it != cvector_end(g.terminals); it += 1) {
		token_set_insert(&tokens, *it);
	}

	for(Token *it = cvector_begin(g.variables); it != cvector_end(g.variables); it += 1) {
		token_set_insert(&tokens, *it);
	}

	lr1_closure(g, &s0);
	cvector_push_back(result.states, s0);

	// Compute all the states and transitions
	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		for(Token *it1 = cvector_begin(tokens); it1 != cvector_end(tokens); it1 += 1) {
			LR1State *source = &result.states[i];
			LR1State ns = lr1_goto(g, source, *it1);

			if(cvector_size(ns.items) > 0) {
				bool found = false;
				LR1Transition t = { 0 };
				size_t destination_index = 0;

				for(size_t j = 0; j < cvector_size(result.states); j += 1) {
					if(state_equals(ns, result.states[j])) {
						found = true;
						break;
					}
				}

				if(!found) {
					destination_index = cvector_size(result.states);
					cvector_push_back(result.states, ns);
				} else {
					cvector_free(ns.items);
				}

				t = (LR1Transition) {
					.id = transition_id,
					.source_index = i,
					.destination_index = destination_index,
					.symbol = *it1,
				};
				transition_id += 1;
				cvector_push_back(result.transitions, t);
			}
		}
	}

	cvector_free(tokens);

	return result;
}

void lr1_delete(LR1Parser lr1) {
	for(LR1State *it1 = cvector_begin(lr1.states); it1 != cvector_end(lr1.states); it1 += 1) {
		state_delete(it1);
	}

	cvector_free(lr1.states);
	cvector_free(lr1.transitions);
}

static void lr1_closure(Grammar g, LR1State *s) {
	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR1Item item = s->items[i];

		// Already at end of definition skip
		if(item.lr0.dot_pos >= cvector_size(item.lr0.rule.definition)) {
			continue;
		}

		Token token = item.lr0.rule.definition[item.lr0.dot_pos];

		// If the next symbol is a non-terminal (Identifier)
		if(token.tag == TokenType_Identifier) {
			// 1. Compute FIRST(beta, a) where beta is the sequence after 'token'
			// and 'a' is the current item's lookahead.
			TokenSet first_set = calculate_first_set_for_suffix(g, item.lr0.rule.definition, item.lr0.dot_pos + 1, item.lookahead);

			// 2. For every rule matching this non-terminal
			for(size_t j = 0; j < cvector_size(g.rules); j += 1) {
				if(token_equals(token, g.rules[j].identifier)) {
					// 3. Add an item for each token in the calculated FIRST set
					for(size_t k = 0; k < cvector_size(first_set); k += 1) {
						LR1Item ni = {
							.lr0 = {
								.rule = g.rules[j],
								.dot_pos = 0,
							},
							.lookahead = first_set[k],
						};

						// Check uniqueness (same rule, dot position, AND lookahead)
						if(!lr1_state_contains(*s, ni)) {
							cvector_push_back(s->items, ni);
						}
					}
				}
			}

			cvector_free(first_set);
		}
	}
}

static LR1State lr1_goto(Grammar g, LR1State *s, Token t) {
	LR1State result = {
		.items = NULL
	};

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR1Item current = s->items[i];

		if(current.lr0.dot_pos < cvector_size(current.lr0.rule.definition)) {
			Token nt = current.lr0.rule.definition[current.lr0.dot_pos];

			if (token_equals(t, nt)) {
				LR1Item ni = {
					.lr0 = {
						.dot_pos = current.lr0.dot_pos + 1,
						.rule = current.lr0.rule,
					},
					.lookahead = current.lookahead, // Lookahead is carried over
				};

				// In LR(1), identical items with different lookaheads can merge, 
				// or you can check if an exact match exists.
				if(!lr1_state_contains(result, ni)) {
					cvector_push_back(result.items, ni);
				}
			}
		}
	}

	if(cvector_size(result.items) > 0) {
		lr1_closure(g, &result);
	}

	return result;
}
// Forward declaration
static bool is_rule_nullable(Grammar g, Token symbol);

// Checks if a specific production rule can derive epsilon (empty)
static bool is_production_nullable(Grammar g, Rule rule) {
	if(cvector_size(rule.definition) == 0) {
		return true;
	}

	// Every symbol in this rule's definition must be nullable for the production to be nullable
	for(size_t i = 0; i < cvector_size(rule.definition); i++) {
		if(!is_rule_nullable(g, rule.definition[i])) {
		    return false;
		}
	}

	return true;
}

// Checks if an identifier (non-terminal) is nullable 
// (True if ANY of its flattened rules are nullable, or if it's an @r / @o auxiliary rule)
static bool is_rule_nullable(Grammar g, Token t) {
	if(t.tag != TokenType_Identifier) {
		return false;
	}

	if(strncmp(t.lexeme.data, "@r", 2) == 0 || strncmp(t.lexeme.data, "@o", 2) == 0) {
		return true;
	}

	// Since rules are flattened (a = b | c becomes separate rules), 
	// a non-terminal is nullable if *any* rule matching this identifier is nullable.
	for(size_t i = 0; i < cvector_size(g.rules); i++) {
		if(token_equals(t, g.rules[i].identifier)) {
			if(is_production_nullable(g, g.rules[i])) {
				return true;
			}
		}
	}

	return false;
}

// Calculates FIRST(def[dot_pos ... end] + lookahead)
static TokenSet calculate_first_set_for_suffix(Grammar g, TokenSet def, size_t dot_pos, Token lookahead) {
    TokenSet result = token_set_new();

    // Base case: end of definition, lookahead takes over
    if (dot_pos >= cvector_size(def)) {
	    token_set_insert(&result, lookahead);
        return result;
    }

    // Scan through the suffix sequence
    for (size_t i = dot_pos; i < cvector_size(def); i++) {
        Token symbol = def[i];

        if (symbol.tag == TokenType_Identifier) {
            bool symbol_can_be_empty = is_rule_nullable(g, symbol);

            // Add FIRST of every rule that matches this identifier
            for (size_t j = 0; j < cvector_size(g.rules); j++) {
                if (token_equals(symbol, g.rules[j].identifier)) {
                    TokenSet sub_first = calculate_first_set_for_suffix(g, g.rules[j].definition, 0, lookahead);
                    
                    for (size_t k = 0; k < cvector_size(sub_first); k++) {
                        if (!token_set_contains(result, sub_first[k])) {
                            token_set_insert(&result, sub_first[k]);
                        }
                    }
                    cvector_free(sub_first);
                }
            }

            // If the non-terminal is NOT nullable, it blocks further symbols in the suffix
            if (!symbol_can_be_empty) {
                break;
            }
        } else {
            // It's a terminal symbol: add it and stop
            if (!token_set_contains(result, symbol)) {
                token_set_insert(&result, symbol);
            }
            break;
        }
    }

    return result;
}

// Checks if an LR1Item already exists in the state vector
static bool lr1_state_contains(LR1State state, LR1Item item) {
	for(size_t i = 0; i < cvector_size(state.items); i++) {
		LR1Item existing = state.items[i];

		if(existing.lr0.dot_pos == item.lr0.dot_pos &&
			token_equals(existing.lr0.rule.identifier, item.lr0.rule.identifier) &&
			token_equals(existing.lookahead, item.lookahead)) {
			// Also check rule definition equality if rules can share identical identifiers

			bool def_match = (cvector_size(existing.lr0.rule.definition) == cvector_size(item.lr0.rule.definition));

			if(def_match) {
				for(size_t d = 0; d < cvector_size(existing.lr0.rule.definition); d++) {
					if(!token_equals(existing.lr0.rule.definition[d], item.lr0.rule.definition[d])) {
						def_match = false;
						break;
					}
				}
			}

			if(def_match) {
				return true;
			}
		}
	}

	return false;
}

// Helper functions
static bool item_equals(LR1Item a, LR1Item b) {
	if(a.lr0.dot_pos != b.lr0.dot_pos) {
		return false;
	}

	if(!token_equals(a.lr0.rule.identifier, b.lr0.rule.identifier)) {
		return false;
	}

	if(!token_equals(a.lookahead, b.lookahead)) {
		return false;
	}

	if(cvector_size(a.lr0.rule.definition) != cvector_size(b.lr0.rule.definition)) {
		return false;
	}

	for(size_t i = 0; i < cvector_size(a.lr0.rule.definition); i += 1) {
		if(!token_equals(a.lr0.rule.definition[i], b.lr0.rule.definition[i])) {
			return false;
		}
	}

	return true;
}

static void item_delete(LR1Item *i) {
	token_delete(i->lr0.rule.identifier);

	for(Token *it1 = cvector_begin(i->lr0.rule.definition); it1 != cvector_end(i->lr0.rule.definition); it1 += 1) {
		token_delete(*it1);
	}
}

static void state_delete(LR1State *s) {
	cvector_free(s->items);
}

static bool state_contains(LR1State s, LR1Item i) {
	for(LR1Item *it = cvector_begin(s.items); it != cvector_end(s.items); it += 1) {
		if(item_equals(*it, i)) {
			return true;
		}
	}
	
	return false;
}

static bool state_equals(LR1State a, LR1State b) {
	if(cvector_size(a.items) != cvector_size(b.items)) {
		return false;
	}

	// Iterate by index because need to operate on both vectors
	for(size_t i = 0; i < cvector_size(a.items); i += 1) {
		if(!state_contains(b, a.items[i])) {
			return false;
		}
	}

	return true;
} */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../lib/hashmap.c/hashmap.h"
#include "../../lib/c-vector/cvector.h"

#include "../lr0/lr0.h"
#include "../grammar/grammar.h"
#include "../token/token.h"

#include "lr1.h"


/* ------------------------------------------------------------------ *
 * Build-time caches / hash tables
 *
 * These structures do not change any observable behaviour of the LR(1)
 * construction; they only avoid repeating work that was previously
 * being redone from scratch:
 *
 *  - NullableCache / FirstCacheTable memoize is_rule_nullable() and
 *    calculate_first_set_for_suffix(), which were previously
 *    recomputed (recursively, from scratch) for every item in every
 *    state's closure.
 *  - ItemHashSet replaces the O(n) linear scan lr1_state_contains()
 *    used while building a state's item set.
 *  - StateHashMap replaces the O(states) linear scan + O(items^2)
 *    state_equals() comparison used to deduplicate newly generated
 *    states.
 * ------------------------------------------------------------------ */

typedef struct NullableCacheEntry {
	Token token;
	bool nullable;
} NullableCacheEntry;

typedef struct FirstCacheEntry {
	const void *def_ptr;
	size_t dot_pos;
	Token lookahead;
	TokenSet result;
	bool occupied;
} FirstCacheEntry;

typedef struct FirstCacheTable {
	FirstCacheEntry *entries;
	size_t capacity;
	size_t count;
} FirstCacheTable;

typedef struct LR1BuildCache {
	cvector(NullableCacheEntry) nullable_cache;
	FirstCacheTable first_cache;
} LR1BuildCache;

typedef struct ItemHashSet {
	LR1Item *keys;
	bool *occupied;
	size_t capacity;
	size_t count;
} ItemHashSet;

typedef struct StateHashEntry {
	uint64_t hash;
	size_t index;
} StateHashEntry;

typedef struct StateHashBucket {
	cvector(StateHashEntry) entries;
} StateHashBucket;

typedef struct StateHashMap {
	StateHashBucket *buckets;
	size_t capacity;
	size_t count;
} StateHashMap;


static void lr1_closure(Grammar g, LR1State *s, LR1BuildCache *cache);
static LR1State lr1_goto(Grammar g, LR1State *s, Token t, LR1BuildCache *cache);
static TokenSet calculate_first_set_for_suffix(Grammar g, TokenSet def, size_t dot_pos, Token lookahead, LR1BuildCache *cache);
static void item_delete(LR1Item *i);
static void state_delete(LR1State *s);
static bool state_contains(LR1State s, LR1Item i);
static bool state_equals(LR1State a, LR1State b);
static bool item_equals(LR1Item a, LR1Item b);
static bool is_rule_nullable(Grammar g, Token symbol, LR1BuildCache *cache);


/* ------------------------------------------------------------------ *
 * Hashing helpers
 * ------------------------------------------------------------------ */

static uint64_t djb2_hash(const char *data, size_t length) {
	uint64_t hash = 5381;

	for(size_t i = 0; i < length; i += 1) {
		hash = ((hash << 5) + hash) + (unsigned char) data[i];
	}

	return hash;
}

static uint64_t token_hash(Token t) {
	uint64_t h = 1469598103934665603ULL;

	if(t.lexeme.data != NULL && t.lexeme.length > 0) {
		h ^= djb2_hash(t.lexeme.data, t.lexeme.length);
	}

	h = h * 1099511628211ULL ^ (uint64_t) (int) t.tag;

	return h;
}

static uint64_t rule_hash(Rule r) {
	uint64_t h = token_hash(r.identifier);

	for(size_t i = 0; i < cvector_size(r.definition); i += 1) {
		h = h * 1099511628211ULL ^ token_hash(r.definition[i]);
	}

	return h;
}

static uint64_t lr1_item_hash(LR1Item item) {
	uint64_t h = rule_hash(item.lr0.rule);

	h = h * 1099511628211ULL ^ (uint64_t) item.lr0.dot_pos;
	h = h * 1099511628211ULL ^ token_hash(item.lookahead);

	return h;
}

static uint64_t lr1_state_hash(LR1State s) {
	// Sum is used (rather than e.g. xor-shift chaining) so that the
	// hash does not depend on the order items appear in, matching the
	// set semantics of state_equals().
	uint64_t h = 0;

	for(size_t i = 0; i < cvector_size(s.items); i += 1) {
		h += lr1_item_hash(s.items[i]);
	}

	return h;
}

static TokenSet token_set_clone(TokenSet s) {
	TokenSet r = NULL;

	for(size_t i = 0; i < cvector_size(s); i += 1) {
		cvector_push_back(r, s[i]);
	}

	return r;
}


/* ------------------------------------------------------------------ *
 * Nullable cache
 * ------------------------------------------------------------------ */

static bool nullable_cache_lookup(LR1BuildCache *cache, Token t, bool *out) {
	for(size_t i = 0; i < cvector_size(cache->nullable_cache); i += 1) {
		if(token_equals(cache->nullable_cache[i].token, t)) {
			*out = cache->nullable_cache[i].nullable;
			return true;
		}
	}

	return false;
}

static void nullable_cache_store(LR1BuildCache *cache, Token t, bool value) {
	NullableCacheEntry e = { .token = t, .nullable = value };

	cvector_push_back(cache->nullable_cache, e);
}


/* ------------------------------------------------------------------ *
 * FIRST-set memoization table (keyed by rule-definition pointer,
 * dot position, and lookahead token)
 * ------------------------------------------------------------------ */

static uint64_t first_cache_hash(const void *def_ptr, size_t dot_pos, Token lookahead) {
	uint64_t h = (uint64_t) (uintptr_t) def_ptr;

	h = h * 1099511628211ULL ^ (uint64_t) dot_pos;
	h = h * 1099511628211ULL ^ token_hash(lookahead);

	return h;
}

static void first_cache_init(FirstCacheTable *t, size_t hint) {
	size_t cap = 16;

	while(cap < hint) {
		cap <<= 1;
	}

	t->capacity = cap;
	t->count = 0;
	t->entries = calloc(cap, sizeof(FirstCacheEntry));
}

static void first_cache_free(FirstCacheTable *t) {
	for(size_t i = 0; i < t->capacity; i += 1) {
		if(t->entries[i].occupied) {
			cvector_free(t->entries[i].result);
		}
	}

	free(t->entries);
	t->entries = NULL;
	t->capacity = 0;
	t->count = 0;
}

static bool first_cache_lookup(FirstCacheTable *t, const void *def_ptr, size_t dot_pos, Token lookahead, TokenSet *out_copy) {
	if(t->capacity == 0) {
		return false;
	}

	uint64_t h = first_cache_hash(def_ptr, dot_pos, lookahead);
	size_t idx = h % t->capacity;

	for(size_t probe = 0; probe < t->capacity; probe += 1) {
		size_t i = (idx + probe) % t->capacity;

		if(!t->entries[i].occupied) {
			return false;
		}

		FirstCacheEntry *e = &t->entries[i];

		if(e->def_ptr == def_ptr && e->dot_pos == dot_pos && token_equals(e->lookahead, lookahead)) {
			*out_copy = token_set_clone(e->result);
			return true;
		}
	}

	return false;
}

static void first_cache_maybe_grow(FirstCacheTable *t) {
	if(t->count * 2 < t->capacity) {
		return;
	}

	size_t new_cap = t->capacity == 0 ? 16 : t->capacity * 2;
	FirstCacheEntry *new_entries = calloc(new_cap, sizeof(FirstCacheEntry));

	for(size_t i = 0; i < t->capacity; i += 1) {
		if(!t->entries[i].occupied) {
			continue;
		}

		FirstCacheEntry e = t->entries[i];
		uint64_t h = first_cache_hash(e.def_ptr, e.dot_pos, e.lookahead);
		size_t idx = h % new_cap;

		for(size_t probe = 0; probe < new_cap; probe += 1) {
			size_t j = (idx + probe) % new_cap;

			if(!new_entries[j].occupied) {
				new_entries[j] = e;
				break;
			}
		}
	}

	free(t->entries);
	t->entries = new_entries;
	t->capacity = new_cap;
}

static void first_cache_store(FirstCacheTable *t, const void *def_ptr, size_t dot_pos, Token lookahead, TokenSet result) {
	first_cache_maybe_grow(t);

	uint64_t h = first_cache_hash(def_ptr, dot_pos, lookahead);
	size_t idx = h % t->capacity;

	for(size_t probe = 0; probe < t->capacity; probe += 1) {
		size_t i = (idx + probe) % t->capacity;

		if(!t->entries[i].occupied) {
			t->entries[i].occupied = true;
			t->entries[i].def_ptr = def_ptr;
			t->entries[i].dot_pos = dot_pos;
			t->entries[i].lookahead = lookahead;
			t->entries[i].result = token_set_clone(result);
			t->count += 1;
			return;
		}

		FirstCacheEntry *e = &t->entries[i];

		if(e->def_ptr == def_ptr && e->dot_pos == dot_pos && token_equals(e->lookahead, lookahead)) {
			// Already cached (can happen through recursive re-entry); avoid a duplicate.
			return;
		}
	}
}


/* ------------------------------------------------------------------ *
 * Item hash set, used in place of the old O(n) lr1_state_contains()
 * scan while a state's item list is being built (closure/goto).
 * ------------------------------------------------------------------ */

static void ihs_init(ItemHashSet *set, size_t hint) {
	size_t cap = 16;

	while(cap < hint * 2) {
		cap <<= 1;
	}

	set->capacity = cap;
	set->count = 0;
	set->keys = calloc(cap, sizeof(LR1Item));
	set->occupied = calloc(cap, sizeof(bool));
}

static void ihs_free(ItemHashSet *set) {
	free(set->keys);
	free(set->occupied);
	set->keys = NULL;
	set->occupied = NULL;
	set->capacity = 0;
	set->count = 0;
}

static void ihs_insert_raw(ItemHashSet *set, LR1Item item) {
	uint64_t h = lr1_item_hash(item);
	size_t idx = h % set->capacity;

	for(size_t probe = 0; probe < set->capacity; probe += 1) {
		size_t i = (idx + probe) % set->capacity;

		if(!set->occupied[i]) {
			set->occupied[i] = true;
			set->keys[i] = item;
			set->count += 1;
			return;
		}

		if(item_equals(set->keys[i], item)) {
			return;
		}
	}
}

static void ihs_grow(ItemHashSet *set) {
	size_t old_cap = set->capacity;
	LR1Item *old_keys = set->keys;
	bool *old_occupied = set->occupied;

	set->capacity = old_cap * 2;
	set->keys = calloc(set->capacity, sizeof(LR1Item));
	set->occupied = calloc(set->capacity, sizeof(bool));
	set->count = 0;

	for(size_t i = 0; i < old_cap; i += 1) {
		if(old_occupied[i]) {
			ihs_insert_raw(set, old_keys[i]);
		}
	}

	free(old_keys);
	free(old_occupied);
}

static bool ihs_contains(ItemHashSet *set, LR1Item item) {
	if(set->capacity == 0) {
		return false;
	}

	uint64_t h = lr1_item_hash(item);
	size_t idx = h % set->capacity;

	for(size_t probe = 0; probe < set->capacity; probe += 1) {
		size_t i = (idx + probe) % set->capacity;

		if(!set->occupied[i]) {
			return false;
		}

		if(item_equals(set->keys[i], item)) {
			return true;
		}
	}

	return false;
}

static void ihs_insert(ItemHashSet *set, LR1Item item) {
	if((set->count + 1) * 2 > set->capacity) {
		ihs_grow(set);
	}

	ihs_insert_raw(set, item);
}


/* ------------------------------------------------------------------ *
 * State hash map, used in place of the old O(states) linear scan +
 * O(items^2) state_equals() comparison used to deduplicate newly
 * generated states in lr1_new().
 * ------------------------------------------------------------------ */

static void shm_init(StateHashMap *m, size_t hint) {
	size_t cap = 16;

	while(cap < hint) {
		cap <<= 1;
	}

	m->capacity = cap;
	m->count = 0;
	m->buckets = calloc(cap, sizeof(StateHashBucket));
}

static void shm_free(StateHashMap *m) {
	for(size_t i = 0; i < m->capacity; i += 1) {
		cvector_free(m->buckets[i].entries);
	}

	free(m->buckets);
	m->buckets = NULL;
	m->capacity = 0;
	m->count = 0;
}

static void shm_insert(StateHashMap *m, uint64_t hash, size_t index) {
	size_t b = hash % m->capacity;
	StateHashEntry e = { .hash = hash, .index = index };

	cvector_push_back(m->buckets[b].entries, e);
	m->count += 1;
}

static void shm_maybe_grow(StateHashMap *m) {
	if(m->count < m->capacity * 2) {
		return;
	}

	size_t new_cap = m->capacity * 2;
	StateHashBucket *new_buckets = calloc(new_cap, sizeof(StateHashBucket));

	for(size_t i = 0; i < m->capacity; i += 1) {
		for(size_t j = 0; j < cvector_size(m->buckets[i].entries); j += 1) {
			StateHashEntry e = m->buckets[i].entries[j];
			size_t nb = e.hash % new_cap;

			cvector_push_back(new_buckets[nb].entries, e);
		}

		cvector_free(m->buckets[i].entries);
	}

	free(m->buckets);
	m->buckets = new_buckets;
	m->capacity = new_cap;
}


/* ------------------------------------------------------------------ *
 * LR(1) construction
 * ------------------------------------------------------------------ */

LR1Parser lr1_new(Grammar g) {
	LR1Parser result = {
		.states = NULL,
		.transitions = NULL,
	};
	LR1State s0 = {
		.items = NULL,
	};
	LR1Item i0 = {
		.lr0 = {
			.dot_pos = 0,
			.rule = *g.start,
		},
		.lookahead = (Token) { .lexeme = { .data = NULL, .length = 0, }, .tag = TokenType_Invalid },
	};
	cvector(Token) tokens = NULL;
	size_t transition_id = 0;
	LR1BuildCache cache = {
		.nullable_cache = NULL,
	};
	StateHashMap state_map;

	first_cache_init(&cache.first_cache, 64);
	shm_init(&state_map, 64);

	cvector_push_back(s0.items, i0);

	for(Token *it = cvector_begin(g.terminals); it != cvector_end(g.terminals); it += 1) {
		token_set_insert(&tokens, *it);
	}

	for(Token *it = cvector_begin(g.variables); it != cvector_end(g.variables); it += 1) {
		token_set_insert(&tokens, *it);
	}

	lr1_closure(g, &s0, &cache);
	cvector_push_back(result.states, s0);
	shm_insert(&state_map, lr1_state_hash(result.states[0]), 0);

	// Compute all the states and transitions
	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		for(Token *it1 = cvector_begin(tokens); it1 != cvector_end(tokens); it1 += 1) {
			LR1State *source = &result.states[i];
			LR1State ns = lr1_goto(g, source, *it1, &cache);

			if(cvector_size(ns.items) > 0) {
				bool found = false;
				LR1Transition t = { 0 };
				size_t destination_index = 0;
				uint64_t ns_hash = lr1_state_hash(ns);
				size_t bucket = ns_hash % state_map.capacity;

				for(size_t k = 0; k < cvector_size(state_map.buckets[bucket].entries); k += 1) {
					StateHashEntry e = state_map.buckets[bucket].entries[k];

					if(e.hash == ns_hash && state_equals(ns, result.states[e.index])) {
						found = true;
						destination_index = e.index;
						break;
					}
				}

				if(!found) {
					destination_index = cvector_size(result.states);
					cvector_push_back(result.states, ns);
					shm_insert(&state_map, ns_hash, destination_index);
					shm_maybe_grow(&state_map);
				} else {
					cvector_free(ns.items);
				}

				t = (LR1Transition) {
					.id = transition_id,
					.source_index = i,
					.destination_index = destination_index,
					.symbol = *it1,
				};
				transition_id += 1;
				cvector_push_back(result.transitions, t);
			}
		}
	}

	cvector_free(tokens);
	shm_free(&state_map);
	cvector_free(cache.nullable_cache);
	first_cache_free(&cache.first_cache);

	return result;
}

void lr1_delete(LR1Parser lr1) {
	for(LR1State *it1 = cvector_begin(lr1.states); it1 != cvector_end(lr1.states); it1 += 1) {
		state_delete(it1);
	}

	cvector_free(lr1.states);
	cvector_free(lr1.transitions);
}

static void lr1_closure(Grammar g, LR1State *s, LR1BuildCache *cache) {
	ItemHashSet seen;

	ihs_init(&seen, cvector_size(s->items) + 4);

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		ihs_insert(&seen, s->items[i]);
	}

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR1Item item = s->items[i];

		// Already at end of definition skip
		if(item.lr0.dot_pos >= cvector_size(item.lr0.rule.definition)) {
			continue;
		}

		Token token = item.lr0.rule.definition[item.lr0.dot_pos];

		// If the next symbol is a non-terminal (Identifier)
		if(token.tag == TokenType_Identifier) {
			// 1. Compute FIRST(beta, a) where beta is the sequence after 'token'
			// and 'a' is the current item's lookahead.
			TokenSet first_set = calculate_first_set_for_suffix(g, item.lr0.rule.definition, item.lr0.dot_pos + 1, item.lookahead, cache);

			// 2. For every rule matching this non-terminal
			for(size_t j = 0; j < cvector_size(g.rules); j += 1) {
				if(token_equals(token, g.rules[j].identifier)) {
					// 3. Add an item for each token in the calculated FIRST set
					for(size_t k = 0; k < cvector_size(first_set); k += 1) {
						LR1Item ni = {
							.lr0 = {
								.rule = g.rules[j],
								.dot_pos = 0,
							},
							.lookahead = first_set[k],
						};

						// Check uniqueness (same rule, dot position, AND lookahead)
						if(!ihs_contains(&seen, ni)) {
							ihs_insert(&seen, ni);
							cvector_push_back(s->items, ni);
						}
					}
				}
			}

			cvector_free(first_set);
		}
	}

	ihs_free(&seen);
}

static LR1State lr1_goto(Grammar g, LR1State *s, Token t, LR1BuildCache *cache) {
	LR1State result = {
		.items = NULL
	};
	ItemHashSet seen;

	ihs_init(&seen, cvector_size(s->items) + 4);

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR1Item current = s->items[i];

		if(current.lr0.dot_pos < cvector_size(current.lr0.rule.definition)) {
			Token nt = current.lr0.rule.definition[current.lr0.dot_pos];

			if (token_equals(t, nt)) {
				LR1Item ni = {
					.lr0 = {
						.dot_pos = current.lr0.dot_pos + 1,
						.rule = current.lr0.rule,
					},
					.lookahead = current.lookahead, // Lookahead is carried over
				};

				// In LR(1), identical items with different lookaheads can merge, 
				// or you can check if an exact match exists.
				if(!ihs_contains(&seen, ni)) {
					ihs_insert(&seen, ni);
					cvector_push_back(result.items, ni);
				}
			}
		}
	}

	ihs_free(&seen);

	if(cvector_size(result.items) > 0) {
		lr1_closure(g, &result, cache);
	}

	return result;
}
// Forward declaration
static bool is_rule_nullable(Grammar g, Token symbol, LR1BuildCache *cache);

// Checks if a specific production rule can derive epsilon (empty)
static bool is_production_nullable(Grammar g, Rule rule, LR1BuildCache *cache) {
	if(cvector_size(rule.definition) == 0) {
		return true;
	}

	// Every symbol in this rule's definition must be nullable for the production to be nullable
	for(size_t i = 0; i < cvector_size(rule.definition); i++) {
		if(!is_rule_nullable(g, rule.definition[i], cache)) {
		    return false;
		}
	}

	return true;
}

// Checks if an identifier (non-terminal) is nullable 
// (True if ANY of its flattened rules are nullable, or if it's an @r / @o auxiliary rule)
static bool is_rule_nullable(Grammar g, Token t, LR1BuildCache *cache) {
	if(t.tag != TokenType_Identifier) {
		return false;
	}

	bool cached_value;

	if(nullable_cache_lookup(cache, t, &cached_value)) {
		return cached_value;
	}

	bool computed = false;

	if(strncmp(t.lexeme.data, "@r", 2) == 0 || strncmp(t.lexeme.data, "@o", 2) == 0) {
		computed = true;
	} else {
		// Since rules are flattened (a = b | c becomes separate rules), 
		// a non-terminal is nullable if *any* rule matching this identifier is nullable.
		for(size_t i = 0; i < cvector_size(g.rules); i++) {
			if(token_equals(t, g.rules[i].identifier)) {
				if(is_production_nullable(g, g.rules[i], cache)) {
					computed = true;
					break;
				}
			}
		}
	}

	nullable_cache_store(cache, t, computed);

	return computed;
}

// Calculates FIRST(def[dot_pos ... end] + lookahead)
static TokenSet calculate_first_set_for_suffix(Grammar g, TokenSet def, size_t dot_pos, Token lookahead, LR1BuildCache *cache) {
    TokenSet cached_copy;

    if (first_cache_lookup(&cache->first_cache, def, dot_pos, lookahead, &cached_copy)) {
        return cached_copy;
    }

    TokenSet result = token_set_new();

    // Base case: end of definition, lookahead takes over
    if (dot_pos >= cvector_size(def)) {
	    token_set_insert(&result, lookahead);
        first_cache_store(&cache->first_cache, def, dot_pos, lookahead, result);
        return result;
    }

    // Scan through the suffix sequence
    for (size_t i = dot_pos; i < cvector_size(def); i++) {
        Token symbol = def[i];

        if (symbol.tag == TokenType_Identifier) {
            bool symbol_can_be_empty = is_rule_nullable(g, symbol, cache);

            // Add FIRST of every rule that matches this identifier
            for (size_t j = 0; j < cvector_size(g.rules); j++) {
                if (token_equals(symbol, g.rules[j].identifier)) {
                    TokenSet sub_first = calculate_first_set_for_suffix(g, g.rules[j].definition, 0, lookahead, cache);
                    
                    for (size_t k = 0; k < cvector_size(sub_first); k++) {
                        if (!token_set_contains(result, sub_first[k])) {
                            token_set_insert(&result, sub_first[k]);
                        }
                    }
                    cvector_free(sub_first);
                }
            }

            // If the non-terminal is NOT nullable, it blocks further symbols in the suffix
            if (!symbol_can_be_empty) {
                break;
            }
        } else {
            // It's a terminal symbol: add it and stop
            if (!token_set_contains(result, symbol)) {
                token_set_insert(&result, symbol);
            }
            break;
        }
    }

    first_cache_store(&cache->first_cache, def, dot_pos, lookahead, result);

    return result;
}

/* Helper functions */
static bool item_equals(LR1Item a, LR1Item b) {
	if(a.lr0.dot_pos != b.lr0.dot_pos) {
		return false;
	}

	if(!token_equals(a.lr0.rule.identifier, b.lr0.rule.identifier)) {
		return false;
	}

	if(!token_equals(a.lookahead, b.lookahead)) {
		return false;
	}

	if(cvector_size(a.lr0.rule.definition) != cvector_size(b.lr0.rule.definition)) {
		return false;
	}

	for(size_t i = 0; i < cvector_size(a.lr0.rule.definition); i += 1) {
		if(!token_equals(a.lr0.rule.definition[i], b.lr0.rule.definition[i])) {
			return false;
		}
	}

	return true;
}

static void item_delete(LR1Item *i) {
	token_delete(i->lr0.rule.identifier);

	for(Token *it1 = cvector_begin(i->lr0.rule.definition); it1 != cvector_end(i->lr0.rule.definition); it1 += 1) {
		token_delete(*it1);
	}
}

static void state_delete(LR1State *s) {
	cvector_free(s->items);
}

static bool state_contains(LR1State s, LR1Item i) {
	for(LR1Item *it = cvector_begin(s.items); it != cvector_end(s.items); it += 1) {
		if(item_equals(*it, i)) {
			return true;
		}
	}
	
	return false;
}

static bool state_equals(LR1State a, LR1State b) {
	if(cvector_size(a.items) != cvector_size(b.items)) {
		return false;
	}

	// Iterate by index because need to operate on both vectors
	for(size_t i = 0; i < cvector_size(a.items); i += 1) {
		if(!state_contains(b, a.items[i])) {
			return false;
		}
	}

	return true;
}
