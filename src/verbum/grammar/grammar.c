#include <stdio.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../token/token.h"
#include "../token_set/token_set.h"
#include "../rule/rule.h"
#include "../rule_set/rule_set.h"

#include "grammar.h"


Grammar grammar_new() {
	return (Grammar) {
		.dynamic_tokens = token_set_new(),
		.rules = NULL,
		.start = NULL,
		.terminals = token_set_new(),
		.nonterminals = token_set_new(),
	};
}

void grammar_delete(Grammar *g) {
	token_set_iterate(g->dynamic_tokens, it1,  {
		token_delete(*it1);
	});

	token_set_delete(g->dynamic_tokens);
	token_set_delete(g->terminals);
	token_set_delete(g->nonterminals);
	rule_set_delete(g->rules);
}
