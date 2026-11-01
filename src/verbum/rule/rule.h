#ifndef VERBUM_RULE_H
#define VERBUM_RULE_H

#include <stdint.h>

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"
#include "../definition/definition.h"


/* RuleMask
 *
 * Properties of a rule, a rule's mask may hold several
 *
 * MEMBERS:
 * 	Cycle: a cycle can be derived from the rule
 * 	Lexical: defined with ':=', or hoisted out of such a rule, the rule is constructed from text
 * 	Token: a lexical rule referenced by a syntax rule, it is a terminal in the parse table
 *
 * NOTES:
 * 	a lexical rule without Token is a fragment, it is only inlined into other lexical rules
 */
typedef enum RuleMask {
	RuleMask_None = 0x0 << 0,
	RuleMask_Cycle = 0x1 << 0,
	RuleMask_Lexical = 0x1 << 1,
	RuleMask_Token = 0x1 << 2,
} RuleMask;

/* Rule
 *
 * MEMBERS:
 * 	definitions: every alternative for the rule
 * 	identifier: rule name, its mask holds the rule's metadata (see SymbolMask)
 * 	mask: properties of the rule (see RuleMask)
 */
typedef struct Rule {
	cvector(Definition) definitions;
	Symbol identifier;
	uint32_t mask;
} Rule;

void rule_push_back(Rule *r, Definition d);


#endif
