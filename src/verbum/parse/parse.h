#ifndef VERBUM_PARSE_H
#define VERBUM_PARSE_H

#include "../../lib/c-vector/cvector.h"

#include "../lexer/lexer.h"
#include "../rule/rule.h"
#include "../rule_set/rule_set.h"


RuleSet verbum_parse(Lexer *l);
//cvector(Rule) verbum_parse(Lexer *l);


#endif
