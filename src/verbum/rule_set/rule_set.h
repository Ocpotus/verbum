#ifndef VERBUM_RULE_SET_H
#define VERBUM_RULE_SET_H

#include <stdbool.h>
#include <stdint.h>

#include "../../lib/c-vector/cvector.h"
#include "../../lib/hashmap.c/hashmap.h"

#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../rule/rule.h"


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
void rule_set_update(RuleSet *set, Symbol identifier, cvector(Definition) definitions);


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

void rule_push_back(Rule *r, Definition d);


#endif
