#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../../lib/log.c/log.h"
#include "../../lib/c-vector/cvector.h"

#include "../definition/definition.h"
#include "../rule_set/rule_set.h"
#include "../grammar/grammar.h"
#include "../token/token.h"
#include "../token_set/token_set.h"

#include "lr0.h"


/* lr0_closure
 *
 * Computes implicit items in a State.
 * In short if an Item has a dot preceding a nonterminal/variable it gathers all
 * Rules with that nonterminal as the identifier.
 *
 * PARAMETERS:
 * 	g: grammar to act on
 * 	s: current state
 *
 *
 */
static void lr0_closure(Grammar g, LR0State *s);
/* lr0_goto
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
static LR0State lr0_goto(Grammar g, LR0State *s, Token t);

static void item_delete(LR0Item *i);
static void state_delete(LR0State *s);
static bool state_contains(LR0State s, LR0Item i);
static bool state_equals(LR0State a, LR0State b);


LR0Parser lr0_new(Grammar g) {
	log_debug("Entering %s", __FUNCTION__);
	LR0Parser result = {
		.states = NULL,
		.transitions = NULL,
	};
	LR0State s0 = {
		.items = NULL,
	};
	LR0Item i0 = {
		.dot_pos = 0,
		.rule = *g.start,
	};
	TokenSet token_sets[] = { g.terminals, g.variables };
	size_t transition_id = 0;
	size_t goto_count = 0;

	cvector_push_back(s0.items, i0);
	lr0_closure(g, &s0);
	cvector_push_back(result.states, s0);

	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		for(size_t j = 0; j < 2; j += 1) {
			TokenSet tokens = token_sets[j];

			token_set_iterate(tokens, it1, {
				LR0State *source = &result.states[i];
				LR0State ns = lr0_goto(g, source, *it1);

				if(cvector_size(ns.items) > 0) {
					bool found = false;
					LR0Transition t = { 0 };
					size_t destination_index = 0;

					for(size_t k = 0; k < cvector_size(result.states); k += 1) {
						if(state_equals(ns, result.states[k])) {
							found = true;
							destination_index = k;
							break;
						}
					}

					if(!found) {
						destination_index = cvector_size(result.states);
						cvector_push_back(result.states, ns);
					} else {
						cvector_free(ns.items);
					}

					t = (LR0Transition) {
						.id = transition_id,
						.source_index = i,
						.destination_index = destination_index,
						.symbol = *it1,
					};
					transition_id += 1;
					cvector_push_back(result.transitions, t);
				}
			});
		}
	}

	log_debug("EXITING %s", __FUNCTION__);

	return result;
}

void lr0_delete(LR0Parser lr0) {
	for(LR0State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		state_delete(it1);
	}

	cvector_free(lr0.states);
	cvector_free(lr0.transitions);
}

static void lr0_closure(Grammar g, LR0State *s) {
	log_debug("COMPUTING THE %s for %p", __FUNCTION__, s);
	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR0Item item = s->items[i];
		
		// Already at end of definition skip
		if(item.dot_pos >= cvector_size(item.rule.definitions[item.rule_index])) {
			continue;
		}

		Token token = item.rule.definitions[item.rule_index][item.dot_pos];

		// Nonterminal/variable is next add all items with dot_pos == 0

		if(token_set_contains(g.variables, token)) {
			RuleSetEntry rs = rule_set_get(g.rules, token);
			
			for(size_t j = 0; j < cvector_size(rs.definitions); j += 1) {
				LR0Item ni = {
					.rule = rs,
					.rule_index = j,
					.dot_pos = 0,
				};

				if(!state_contains(*s, ni)) {
					cvector_push_back(s->items, ni);
				}
			}
		}
	}
}

static LR0State lr0_goto(Grammar g, LR0State *s, Token t) {
	log_debug("COMPUTING %s for %p", __FUNCTION__, s);
	LR0State result = {
		.items = NULL
	};

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR0Item current = s->items[i];


		if(current.dot_pos < cvector_size(current.rule.definitions[current.rule_index])) {
			Token nt = current.rule.definitions[current.rule_index][current.dot_pos];

			if(token_equals(t, nt)) {
				LR0Item ni = {
					.dot_pos = current.dot_pos + 1,
					.rule_index = current.rule_index,
					.rule = current.rule,
				};

                    		cvector_push_back(result.items, ni);
			}
		}
	}

	if(cvector_size(result.items) > 0) {
		lr0_closure(g, &result);
	}

	return result;
}

// Helper functions
static bool item_equals(LR0Item a, LR0Item b) {
	if(a.dot_pos != b.dot_pos) {
		return false;
	}

	if(a.rule.definitions != b.rule.definitions) {
		return false;
	}

	if(a.rule_index != b.rule_index) {
		return false;
	}

	if(cvector_size(a.rule.definitions) != cvector_size(b.rule.definitions)) {
		return false;
	}

	if(cvector_size(a.rule.definitions[a.rule_index]) != cvector_size(b.rule.definitions[b.rule_index])) {
		return false;
	}

	if(!token_equals(a.rule.identifier, b.rule.identifier)) {
		return false;
	}

	for(size_t i = 0; i < cvector_size(a.rule.definitions[a.rule_index]); i += 1) {
		if(!token_equals(a.rule.definitions[a.rule_index][i], b.rule.definitions[b.rule_index][i])) {
			return false;
		}
	}

	return true;
}

static void item_delete(LR0Item *i) {
	token_delete(i->rule.identifier);

	for(Token *it1 = cvector_begin(i->rule.definitions[i->rule_index]); it1 != cvector_end(i->rule.definitions[i->rule_index]); it1 += 1) {
		token_delete(*it1);
	}
}

static void state_delete(LR0State *s) {
	cvector_free(s->items);
}

static bool state_contains(LR0State s, LR0Item i) {
	for(LR0Item *it = cvector_begin(s.items); it != cvector_end(s.items); it += 1) {
		if(item_equals(*it, i)) {
			return true;
		}
	}
	
	return false;
}

static bool state_equals(LR0State a, LR0State b) {
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
