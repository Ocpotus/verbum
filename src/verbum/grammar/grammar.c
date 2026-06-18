#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"

#include "grammar.h"


void grammar_delete(Grammar *g) {
	for(Rule *it = cvector_begin(g->rules); it != cvector_end(g->rules); it += 1) {
		cvector_free(it->definition);
	}

	cvector_free(g->original.rules);
	cvector_free(g->rules);
	cvector_free(g->terminals);
	cvector_free(g->variables);

	for(Token *it1 = cvector_begin(g->dynamic_tokens); it1 != cvector_end(g->dynamic_tokens); it1 += 1) {
		token_delete(*it1);

	}

	cvector_free(g->dynamic_tokens);
}
