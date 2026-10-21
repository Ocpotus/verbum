#include <stdbool.h>

#include "../symbol/symbol.h"
#include "../rule_set/rule_set.h"

#include "grammar.h"


Grammar grammar_new() {
	return (Grammar) {
		.rules = NULL,
		.start = NULL,
		.terminals = symbol_set_new(),
		.nonterminals = symbol_set_new(),
	};
}

void grammar_delete(Grammar *g) {
	symbol_set_delete(g->terminals);
	symbol_set_delete(g->nonterminals);
	rule_set_delete(g->rules);
}
