#ifndef VERBUM_GENERATE_H
#define VERBUM_GENERATE_H

#include "../../lib/c-vector/cvector.h"

#include "../table/table.h"
#include "../grammar/grammar.h"
#include "../raw_rule/raw_rule.h"


void verbum_generate(cvector(RawRule) raw_rules, Grammar grammar, ParseTable table);


#endif
