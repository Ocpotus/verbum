#include <stddef.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"

#include "definition.h"


Definition definition_new() {
	return NULL;
}

void definition_delete(Definition d) {
	cvector_free(d);
}

void definition_push_back(Definition *d, Token t) {
	cvector_push_back(*d, t);
}

bool definition_equals(Definition a, Definition b) {
	if(cvector_size(a) != cvector_size(b)) {
		return false;
	}

	for(size_t i = 0; i < cvector_size(a); a += 1) {
		if(!token_equals(a[i], b[i])) {
			return false;
		}
	}

	return true;
}
