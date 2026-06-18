#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"
#include "../token/token.h"

#include "lr0.h"


static void item_delete(Item *i);
static void state_delete(State *s);
static bool state_contains(State s, Item i);
static bool state_equals(State a, State b);
static void lr0_closure(Grammar g, State *s);
static State lr0_goto(Grammar g, State *s, Token t);
static bool token_set_contains(cvector(Token) set, Token t);
static void token_set_insert(cvector(Token) *set, Token t);



LR0 lr0_new(Grammar g) {
	LR0 result = {
		.states = NULL,
		.transitions = NULL,
	};
	State s0 = {
		.items = NULL,
	};
	Item i0 = {
		.dot_pos = 0,
		.rule = *g.start,
		.lookaheads = NULL,
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
			State *source = &result.states[i];
			State ns = lr0_goto(g, source, *it1);

			if(cvector_size(ns.items) > 0) {
				bool found = false;
				Transition t = { 0 };
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

				t = (Transition) {
					.id = transition_id,
					.source_index = i,
					.destination_index = destination_index,
					.direct_reads = NULL,
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

void lr0_delete(LR0 lr0) {
	for(State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		for(Item *it2 = cvector_begin(it1->items); it2 != cvector_end(it1->items); it2 += 1) {
			cvector_free(it2->lookaheads);
		}

		cvector_free(it1->items);
	}

	for(Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		cvector_free(it1->direct_reads);
	}

	cvector_free(lr0.states);
	cvector_free(lr0.transitions);
}

static void lr0_closure(Grammar g, State *s) {
	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		Item item = s->items[i];
		
		if(item.dot_pos >= cvector_size(item.rule.definition)) {
			continue;
		}

		Token token = item.rule.definition[item.dot_pos];

		if(token.tag == TokenType_Identifier) {
			for(size_t j = 0; j < cvector_size(g.rules); j += 1) {
				if(token_equals(token, g.rules[j].identifier)) {
					Item ni = {
						.rule = g.rules[j],
						.dot_pos = 0, 
						.lookaheads = NULL,
					};

					if(!state_contains(*s, ni)) {
						cvector_push_back(s->items, ni);
					}
				}
			}
		}
	}
}

static State lr0_goto(Grammar g, State *s, Token t) {
	State result = {
		.items = NULL
	};

	for(size_t i = 0; i < cvector_size(s->items); i += 1) {
		Item current = s->items[i];

		if(current.dot_pos < cvector_size(current.rule.definition)) {
			Token nt = current.rule.definition[current.dot_pos];

			if(token_equals(t, nt)) {
				Item ni = {
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
static bool item_equals(Item a, Item b) {
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

static void item_delete(Item *i) {
	token_delete(i->rule.identifier);

	for(Token *it1 = cvector_begin(i->rule.definition); it1 != cvector_end(i->rule.definition); it1 += 1) {
		token_delete(*it1);
	}
}

static void state_delete(State *s) {
	for(Item *it1 = cvector_begin(s->items); it1 != cvector_end(s->items); it1 += 1) {
		item_delete(it1);
	}
}

static bool state_contains(State s, Item i) {
	for(Item *it = cvector_begin(s.items); it != cvector_end(s.items); it += 1) {
		if(item_equals(*it, i)) {
			return true;
		}
	}
	
	return false;
}

static bool state_equals(State a, State b) {
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

static bool token_set_contains(cvector(Token) set, Token t) {
	for(Token *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(*it1, t)) {
			return true;
		}
	}

	return false;
}

static void token_set_insert(cvector(Token) *set, Token t) {
	for(Token *it = cvector_begin(*set); it != cvector_end(*set); it += 1) {
		if(token_equals(*it, t)) {
			return;
		}
	}

	cvector_push_back(*set, t);
}
