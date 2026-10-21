#ifndef VERBUM_PARSE_H
#define VERBUM_PARSE_H

#include "../../lib/c-vector/cvector.h"

#include "../lexer/lexer.h"
#include "../raw_rule/raw_rule.h"


cvector(RawRule) verbum_parse(Lexer *l);


#endif
