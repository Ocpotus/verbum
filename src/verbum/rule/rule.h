#ifndef VERBUM_RULE_H
#define VERBUM_RULE_H

#include "../../lib/c-vector/cvector.h"

#include "../token//token.h"

typedef struct Rule {
	Token identifier;
	cvector(Token) definition;
} Rule;


void rule_delete(Rule);


#endif
