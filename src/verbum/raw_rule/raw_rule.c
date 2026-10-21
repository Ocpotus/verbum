#include "../../lib/c-vector/cvector.h"

#include "raw_rule.h"


void raw_rule_delete(RawRule *rr) {
	cvector_free(rr->definition);
}
