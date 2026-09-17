#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"

#include "token_set.h"


TokenSet token_set_new() {
	return NULL;
}

void token_set_delete(TokenSet set) {
	cvector_free(set);
}

bool token_set_contains(TokenSet set, Token t) {
	for(Token *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(*it1, t)) {
			return true;
		}
	}

	return false;
}

bool token_set_insert(TokenSet *set, Token t) {
	for(Token *it = cvector_begin(*set); it != cvector_end(*set); it += 1) {
		if(token_equals(*it, t)) {
			return false;
		}
	}

	cvector_push_back(*set, t);

	return true;
}
