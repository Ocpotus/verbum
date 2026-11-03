#ifndef VERBUM_RULE_H
#define VERBUM_RULE_H

#include <stdbool.h>
#include <stdint.h>

#include "../../lib/c-vector/cvector.h"
#include "../../lib/hashmap.c/hashmap.h"

#include "../symbol/symbol.h"
#include "../token/token.h"
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

typedef struct hashmap *RuleSet;

RuleSet rule_set_new();

void rule_set_delete(RuleSet set);

/* rule_set_contains
 *
 * Checks if a rule with a given definition is in a given RuleSet
 *
 * PARAMETERS:
 * 	set: set to check
 * 	identifier: rule to check for
 * 	definition: definition to check for
 *
 * RETURNS:
 * 	true if found, false if not
 *
 */
bool rule_set_contains(RuleSet set, Symbol identifier, Definition definition);

/* rule_set_insert
 *
 * Appends a definition to a rule, creating the rule if needed.
 * The identifier's mask is merged into the stored identifier's mask.
 *
 * PARAMETERS:
 * 	set: set to insert into
 * 	identifier: rule to insert into
 * 	definition: definition to append
 *
 * RETURNS:
 * 	the stored Rule, NULL on failure
 */
const Rule *rule_set_insert(RuleSet *set, Symbol identifier, Definition definition);

Rule rule_set_get(RuleSet set, Symbol identifier);

bool rule_set_remove(RuleSet *set, Symbol identifier);

/* rule_set_update
 *
 * Replaces a stored rule in place, creating it if needed. The rule is looked up by its identifier.
 * Updating an existing rule never resizes the set, so it is safe during rule_set_iterate.
 *
 * PARAMETERS:
 * 	set: set to update
 * 	rule: rule to store, ownership of its definitions moves to the set
 */
void rule_set_update(RuleSet *set, Rule rule);

/* rule_set_mark
 *
 * Adds properties to a stored rule in place. Does nothing if the rule is not stored.
 * Never resizes the set, so it is safe during rule_set_iterate.
 *
 * PARAMETERS:
 * 	set: set holding the rule
 * 	identifier: rule to mark
 * 	mask: RuleMask bits to add
 */
void rule_set_mark(RuleSet *set, Symbol identifier, uint32_t mask);


/* rule_set_iterate
 *
 * Macro for clean iteration. It exists purely to avoid using hashmap_iter because it would polute
 * the code with iters, and datas.
 *
 *
 * PARAMETERS:
 * 	set: RuleSet
 * 	name: name of the iterator
 */
#define rule_set_iterate(set, name, ...) \
do {\
	size_t rule_set_iterate_iter = 0; \
	void *rule_set_iterate_data; \
	while(hashmap_iter(set, &rule_set_iterate_iter, &rule_set_iterate_data)) {\
		const Rule *name = rule_set_iterate_data;\
		__VA_ARGS__ \
	}\
\
} while(0)

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

/* raw_rule_delete
 *
 * Deletes RawRule
 *
 * PARAMETERS:
 * 	rr: RawRule to delete
 */
void raw_rule_delete(RawRule *rr);


#endif
