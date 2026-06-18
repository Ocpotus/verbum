#ifndef VERBUM_GRAMMAR_INFO_H
#define VERBUM_GRAMMAR_INFO_H

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"


typedef struct Rule {
	Token identifier;
	cvector(Token) definition;
} Rule;

typedef struct GrammarInfo {
	cvector(Rule) rules;
	Token start;

} GrammarInfo;


#endif
