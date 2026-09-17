#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"
#include "../token_set/token_set.h"
#include "../rule/rule.h"

#include "grammar.h"


void grammar_delete(Grammar *g) {
	for(Rule *it = cvector_begin(g->rules); it != cvector_end(g->rules); it += 1) {
		rule_delete(*it);
	}

	cvector_free(g->rules);
	token_set_delete(g->terminals);
	token_set_delete(g->variables);

	for(Token *it1 = cvector_begin(g->dynamic_tokens); it1 != cvector_end(g->dynamic_tokens); it1 += 1) {
		token_delete(*it1);
	}

	token_set_delete(g->dynamic_tokens);
}
