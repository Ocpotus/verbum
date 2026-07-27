#ifndef VERBUM_PARSE_H
#define VERBUM_PARSE_H

#include "../../lib/c-vector/cvector.h"

#include "../lexer/lexer.h"
#include "../rule/rule.h"


cvector(Rule) verbum_parse(Lexer *l);

#endif
