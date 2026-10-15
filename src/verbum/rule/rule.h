#ifndef VERBUM_RULE_H
#define VERBUM_RULE_H

#include <stdbool.h>

#include "../token/token.h"

#include "../definition/definition.h"


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
	Definition definition;
	bool nullable;
} Rule;


void rule_delete(Rule);


#endif
