#ifndef VERBUM_ANALYZER_H
#define VERBUM_ANALYZER_H

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"
#include "../lexer/lexer.h"
#include "../grammar/grammar.h"
#include "../rule/rule.h"


Grammar analyzer_analyze(cvector(Rule) rules);


#endif
