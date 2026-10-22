#include "../../lib/c-vector/cvector.h"

#include "../definition/definition.h"

#include "rule.h"


void rule_push_back(Rule *r, Definition d) {
	cvector_push_back(r->definitions, d);
}
