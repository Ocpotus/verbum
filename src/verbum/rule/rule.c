#include "../../lib/c-vector/cvector.h"
//#include "../token/token.h"

#include "rule.h"


void rule_delete(Rule r) {
	cvector_free(r.definition);
}
