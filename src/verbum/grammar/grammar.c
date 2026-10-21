#include <stdio.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../symbol/symbol.h"
#include "../rule_set/rule_set.h"

#include "grammar.h"


Grammar grammar_new() {
	return (Grammar) {
		.dynamic_symbols = symbol_set_new(),
		.rules = NULL,
		.start = NULL,
		.terminals = symbol_set_new(),
		.nonterminals = symbol_set_new(),
	};
}

void grammar_delete(Grammar *g) {
	symbol_set_iterate(g->dynamic_symbols, it1,  {
		symbol_delete((Symbol *) it1);
	});

	symbol_set_delete(g->dynamic_symbols);
	symbol_set_delete(g->terminals);
	symbol_set_delete(g->nonterminals);
	rule_set_delete(g->rules);
}
