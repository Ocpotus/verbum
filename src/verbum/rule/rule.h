#ifndef VERBUM_RULE_H
#define VERBUM_RULE_H

#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"


/* Rule
 *
 * Contains information for a particular substitution
 *
 * MEMBERS:
 * 	identifier: rule name
 * 	defintion: what does the rule turn into
 */
typedef struct Rule {
	Token identifier;
	cvector(Token) definition;
	bool nullable;
} Rule;


void rule_delete(Rule);


#endif
