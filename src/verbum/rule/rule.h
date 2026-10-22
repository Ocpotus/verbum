#ifndef VERBUM_RULE_H
#define VERBUM_RULE_H

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"
#include "../definition/definition.h"


/* Rule
 *
 * MEMBERS:
 * 	definitions: every alternative for the rule
 * 	identifier: rule name, its mask holds the rule's metadata (see SymbolMask)
 */
typedef struct Rule {
	cvector(Definition) definitions;
	Symbol identifier;
} Rule;

void rule_push_back(Rule *r, Definition d);


#endif
