#include <stdbool.h>

#include "../../lib/hashmap.c/hashmap.h"
#include "../../lib/log.c/log.h"
#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../lr0/lr0.h"
#include "../grammar/grammar.h"
#include "../token/token.h"
#include "../rule/rule.h"
#include "../token_set/token_set.h"

#include "lr1.h"


/* lr1_closure
 *
 * Computes implicit items in a State.
 * In short if an Item has a dot preceding a nonterminal it gathers all
 * Rules with that nonterminal as the identifier.
 *
 * PARAMETERS:
 * 	g: grammar to act on
 * 	s: current state
 *
 *
 */
//static void lr1_closure(Grammar g, LR1State *s, struct hashmap *store);
/* lr1_goto
 *
 * Computes the next state if a token is consumed.
 *
 * PARAMETERS:
 * 	g: the grammar to act on
 * 	s: the current state
 * 	t: the token used to determine the next state
 *
 * RETURNS:
 * 	a newly constructed state
 */
/* static LR1State lr1_goto(Grammar g, LR1State *s, Token t, struct hashmap *store);

static void item_delete(LR1Item *i);
static void state_delete(LR1State *s);
static bool state_contains(LR1State s, LR1Item i);
static bool state_equals(LR1State a, LR1State b);

static TokenSet first(Grammar g, Token t, struct hashmap *store);
int first_compare(const void *a, const void *b, void *udata); */

/* bool first_iter(const void *item, void *udata) {
	const struct user *user = item;
	printf("%s (age=%d)\n", user->name, user->age);
	return true;
} */
/*
uint64_t first_hash(const void *item, uint64_t seed0, uint64_t seed1);

typedef struct TokenSetFirstStore {
	Token symbol;
	TokenSet set;
} TokenSetFirstStore;


LR1Parser lr1_new(Grammar g, LR0Parser lr0) {
	log_debug("ENTERING %s", __FUNCTION__);
	LR1Parser result = {
		.states = NULL,
		.transitions = NULL,
	};
	LR1State s0 = {
		.items = NULL,
	};
	LR1Item i0 = {
		.lr0 = lr0.states[0].items[0],
		.lookahead = (Token) {
			.lexeme = (Lexeme) {
				.data = NULL,
				.length = 0,
			},
			.tag = TokenType_EOF,
		},
	};
	cvector(Token) tokens = NULL;
	size_t transition_id = 0;

	cvector_push_back(s0.items, i0);

	for(Token *it = cvector_begin(g.terminals); it != cvector_end(g.terminals); it += 1) {
		token_set_insert(&tokens, *it);
	}

	for(Token *it = cvector_begin(g.nonterminals); it != cvector_end(g.nonterminals); it += 1) {
		token_set_insert(&tokens, *it);
	}

	struct hashmap *store = hashmap_new_with_allocator(memory_new, memory_resize, memory_delete, sizeof(TokenSetFirstStore),
			0, 0, 0, first_hash, first_compare, NULL, NULL);
	lr1_closure(g, &s0, store);
	cvector_push_back(result.states, s0);

	// Compute all the states and transitions
	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		for(Token *it1 = cvector_begin(tokens); it1 < cvector_end(tokens); it1 += 1) {
			LR1State *source = &result.states[i];
			LR1State ns = lr1_goto(g, source, *it1, store);

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
				log_warn("LENGTH OF TRANSITIONS: %zu", cvector_size(result.transitions));
			}
		}
	}

	cvector_free(tokens);
	log_debug("EXITING %s", __FUNCTION__);

	return result;
}

void lr1_delete(LR1Parser lr1) {
	for(LR1State *it1 = cvector_begin(lr1.states); it1 != cvector_end(lr1.states); it1 += 1) {
		state_delete(it1);
	}

	cvector_free(lr1.states);
	cvector_free(lr1.transitions);
}

static void lr1_closure(Grammar g, LR1State *s, struct hashmap *store) {
	log_debug("COMPUTING %s for %p", __FUNCTION__, s);

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR1Item item = s->items[i];

		if(item.lr0.dot_pos >= cvector_size(item.lr0.rule.definition)) {
			continue;
		}

		Token token = item.lr0.rule.definition[item.lr0.dot_pos];

		if(token.tag == TokenType_Identifier) {
			// Since the check earlier we are guaranteed to not go out of bounds
			// Get first set of symbol after current position
			printf("FIRST FOR ITERATION %zu %.*s\n", i, (int) token.lexeme.length, token.lexeme.data);
			TokenSet first_set = first(g,
			item.lr0.rule.definition[item.lr0.dot_pos + 1 > cvector_size(item.lr0.rule.definition) ? item.lr0.dot_pos + 1 : item.lr0.dot_pos], store);
			// Default lookahead (inherit) token_set_insert(&first_set, item.lookahead);

			for(Rule *it2 = cvector_begin(g.rules); it2 != cvector_end(g.rules); it2 += 1) {
				if(token_equals(it2->identifier, token)) {
					for(Token *it3 = cvector_begin(first_set); it3 != cvector_end(first_set); it3 += 1) {
						LR1Item candidate = {
							.lr0 = (LR0Item) {
								.dot_pos = 0,
								.rule = *it2,
							},
							.lookahead = *it3,
						};

						if(!state_contains(*s, candidate)) {
							cvector_push_back(s->items, candidate);
						}
					}
				}
			}

			token_set_delete(first_set);
		}
	}
}

static LR1State lr1_goto(Grammar g, LR1State *s, Token t, struct hashmap *store) {
	log_debug("COMPUTING %s for %p", __FUNCTION__, s);

	LR1State result = { 
		.items = NULL,
	};

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR1Item current = s->items[i];

		if(current.lr0.dot_pos < cvector_size(current.lr0.rule.definition)) {
			Token nt = current.lr0.rule.definition[current.lr0.dot_pos];

			if(token_equals(t, nt)) {
				LR1Item ni = {
					.lr0 = {
						.dot_pos = current.lr0.dot_pos + 1,
						.rule = current.lr0.rule,
					},
					.lookahead = current.lookahead,
				};

				cvector_push_back(result.items, ni);
			}
		}
	}

	if(cvector_size(result.items) > 0) {
		lr1_closure(g, &result, store);
	}

	return result;
}

// Helper functions
static bool item_equals(LR1Item a, LR1Item b) {
	if(a.lr0.dot_pos != b.lr0.dot_pos) {
		return false;
	}

	if(cvector_size(a.lr0.rule.definition) != cvector_size(b.lr0.rule.definition)) {
		return false;
	}

	if(!token_equals(a.lr0.rule.identifier, b.lr0.rule.identifier)) {
		return false;
	}

	if(!token_equals(a.lookahead, b.lookahead)) {
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

static TokenSet first(Grammar g, Token t, struct hashmap *store) {
	//log_debug("COMPUTING %s for %.*s", __FUNCTION__, (int) t.lexeme.length, t.lexeme.data);
	TokenSet result = NULL;
	const TokenSetFirstStore *set = hashmap_get(store,  &(TokenSetFirstStore) { .symbol = t });

	// If already calculated exit
	if(set != NULL) {
		return set->set;
	}

	result = token_set_new();
	bool can_be_empty = true;

	if(t.tag == TokenType_Identifier) {
		for(Rule *it1 = cvector_begin(g.rules); it1 != cvector_end(g.rules); it1 += 1) {
			if(token_equals(it1->identifier, t)) {
				for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
					if(it2->tag != TokenType_Identifier) {
						token_set_insert(&result, *it2);
						can_be_empty = false;
						break;
					}
					// Have to compute firsts for the non-terminal
					TokenSet sub = first(g, *it2, store);
					//printf("FINISHED CALCUALTE SUB FOR %.*s\n", (int) it2->lexeme.length, it2->lexeme.data);
					bool sub_has_epsilon = false;
					for(Token *it3 = cvector_begin(sub); it3 != cvector_end(sub); it3 += 1) {
						if(it3->tag == TokenType_Invalid) { // Assuming TokenType_Invalid or a dedicated epsilon token
							sub_has_epsilon = true;
						} else {
							token_set_insert(&result, *it3);
						}
					}

					// If this non-terminal cannot derive epsilon, stop here
					if(!sub_has_epsilon) {
						can_be_empty = false;
						break;
					}
				}

				// If all symbols in the definition could derive epsilon, add epsilon to result
				if(can_be_empty) {
				    Token lambda = { .lexeme = { .data = NULL, .length = 0 }, .tag = TokenType_Invalid };
				    token_set_insert(&result, lambda);
				}

			}
		}
	} else {
		token_set_insert(&result, t);
	}

	hashmap_set(store, &(TokenSetFirstStore) { .symbol = t, .set = result });

	return result;
}

int first_compare(const void *a, const void *b, void *udata) {
	const struct TokenSetFirstStore *ua = a;
	const struct TokenSetFirstStore *ub = b;
	return token_equals(ua->symbol, ub->symbol);
}

uint64_t first_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const struct TokenSetFirstStore *first = item;
	return hashmap_sip(first->symbol.lexeme.data, first->symbol.lexeme.length, seed0, seed1);
} */
