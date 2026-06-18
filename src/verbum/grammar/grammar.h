#ifndef VERBUM_GRAMMAR_H
#define VERBUM_GRAMMAR_H

#include "../../lib/c-vector/cvector.h"

//#include "../parser"
#include "../token/token.h"


typedef struct Rule {
	Token identifier;
	cvector(Token) definition;
} Rule;

typedef struct Grammar {
	struct {
		cvector(Rule) rules;
		cvector(Rule) terminal_rules;
		cvector(Rule) non_terminal_rules;
	} original;
	cvector(Rule) rules;
	cvector(Token) terminals;
	cvector(Token) variables;
	cvector(Token) dynamic_tokens;
	Rule *start;
} Grammar;

void grammar_delete(Grammar *g);


#endif
