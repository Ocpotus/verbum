#include <stddef.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"

#include "definition.h"


Definition definition_new() {
	return NULL;
}

void definition_delete(Definition d) {
	cvector_free(d);
}

void definition_push_back(Definition *d, Symbol s) {
	cvector_push_back(*d, s);
}

bool definition_equals(Definition a, Definition b) {
	if(cvector_size(a) != cvector_size(b)) {
		return false;
	}

	for(size_t i = 0; i < cvector_size(a); i += 1) {
		if(!symbol_equals(a[i], b[i])) {
			return false;
		}
	}

	return true;
}
