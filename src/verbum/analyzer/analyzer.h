#ifndef VERBUM_ANALYZER_H
#define VERBUM_ANALYZER_H

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"
#include "../rule_set/rule_set.h"


/* Constructs a grammar from a vector of rules
 */
Grammar analyzer_analyze(RuleSet rules);


#endif
