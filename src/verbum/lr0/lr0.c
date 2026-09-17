#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

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
	cvector(Token) tokens = NULL;
	size_t transition_id = 0;

	cvector_push_back(s0.items, i0);

	for(Token *it = cvector_begin(g.terminals); it != cvector_end(g.terminals); it += 1) {
		token_set_insert(&tokens, *it);
	}

	for(Token *it = cvector_begin(g.variables); it != cvector_end(g.variables); it += 1) {
		token_set_insert(&tokens, *it);
	}

	lr0_closure(g, &s0);
	cvector_push_back(result.states, s0);

	// Compute all the states and transitions
	for(size_t i = 0; i < cvector_size(result.states); i += 1) {
		for(Token *it1 = cvector_begin(tokens); it1 < cvector_end(tokens); it1 += 1) {
			LR0State *source = &result.states[i];
			LR0State ns = lr0_goto(g, source, *it1);

			if(cvector_size(ns.items) > 0) {
				bool found = false;
				LR0Transition t = { 0 };
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

				t = (LR0Transition) {
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

void lr0_delete(LR0Parser lr0) {
	for(LR0State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		state_delete(it1);
	}

	cvector_free(lr0.states);
	cvector_free(lr0.transitions);
}

static void lr0_closure(Grammar g, LR0State *s) {
	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR0Item item = s->items[i];
		
		// Already at end of definition skip
		if(item.dot_pos >= cvector_size(item.rule.definition)) {
			continue;
		}

		Token token = item.rule.definition[item.dot_pos];

		// Nonterminal/variable is next add all items with dot_pos == 0
		if(token.tag == TokenType_Identifier) {
			for(size_t j = 0; j < cvector_size(g.rules); j += 1) {
				if(token_equals(token, g.rules[j].identifier)) {
					LR0Item ni = {
						.rule = g.rules[j],
						.dot_pos = 0, 
					};

					if(!state_contains(*s, ni)) {
						cvector_push_back(s->items, ni);
					}
				}
			}
		}
	}
}

static LR0State lr0_goto(Grammar g, LR0State *s, Token t) {
	LR0State result = {
		.items = NULL
	};

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		LR0Item current = s->items[i];

		if(current.dot_pos < cvector_size(current.rule.definition)) {
			Token nt = current.rule.definition[current.dot_pos];

			if(token_equals(t, nt)) {
				LR0Item ni = {
					.dot_pos = current.dot_pos + 1,
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

/* Helper functions */
static bool item_equals(LR0Item a, LR0Item b) {
	if(a.dot_pos != b.dot_pos) {
		return false;
	}

	if(!token_equals(a.rule.identifier, b.rule.identifier)) {
		return false;
	}

	if(cvector_size(a.rule.definition) != cvector_size(b.rule.definition)) {
		return false;
	}

	for(size_t i = 0; i < cvector_size(a.rule.definition); i += 1) {
		if(!token_equals(a.rule.definition[i], b.rule.definition[i])) {
			return false;
		}
	}

	return true;
}

static void item_delete(LR0Item *i) {
	token_delete(i->rule.identifier);

	for(Token *it1 = cvector_begin(i->rule.definition); it1 != cvector_end(i->rule.definition); it1 += 1) {
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
