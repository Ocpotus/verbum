#ifndef VERBUM_RAW_RULE_H
#define VERBUM_RAW_RULE_H

#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"


/* RawRule
 *
 * A rule as written in the source, before analysis
 *
 * MEMBERS:
 * 	identifier: rule name
 * 	definition: every token between '=' (or ':=') and ';' excluding ','
 * 	lexical: true if defined with ':=', the rule is constructed from text
 */
typedef struct RawRule {
	Token identifier;
	cvector(Token) definition;
	bool lexical;
} RawRule;

void raw_rule_delete(RawRule *rr);

#endif
