#ifndef VERBUM_RULE_SET_H
#define VERBUM_RULE_SET_H

#include <stdbool.h>
#include <stdint.h>

#include "../../lib/c-vector/cvector.h"
#include "../../lib/hashmap.c/hashmap.h"

#include "../token/token.h"
#include "../definition/definition.h"



typedef enum RuleSetEntryMask {
	RuleSetEntryMask_None = 0x0,
	RuleSetEntryMask_Dynamic = 0x1 << 0,
	RuleSetEntryMask_Repetition = 0x1 << 1,
	RuleSetEntryMask_Optional = 0x1 << 2,
} RuleSetEntryMask;

// Will become rule eventually
typedef struct RuleSetEntry {
	cvector(Definition) definitions;
	Token identifier;
	uint8_t mask;
} RuleSetEntry;

typedef struct hashmap *RuleSet;

RuleSet rule_set_new();

void rule_set_delete(RuleSet set);

/* rule_set_contains
 *
 * Checks if a token is in a given RuleSet
 *
 * PARAMETERS:
 * 	set: set to check
 * 	t: token to check for
 *
 * RETURNS:
 * 	true if found, false if not
 *
 */
bool rule_set_contains(RuleSet set, Token identifier, Definition definition);

/* rule_set_insert
 *
 * Inserts a token into the set when possible
 *
 * PARAMETERS:
 * 	set: set to insert into
 * 	t: token to insert
 *
 * RETURNS:
 * 	true if inserted, false if not
 */
const void *rule_set_insert(RuleSet *set, Token identifier, Definition definition, uint8_t mask);

RuleSetEntry rule_set_get(RuleSet set, Token identifier);

bool rule_set_remove(RuleSet *set, Token identifier);
void rule_set_update(RuleSet *set, Token identifier, cvector(Definition) definitions, uint8_t mask);


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
		const RuleSetEntry *name = rule_set_iterate_data;\
		__VA_ARGS__ \
	}\
\
} while(0)

void rule_set_entry_delete(RuleSetEntry rse);

void rule_set_entry_push_back(RuleSetEntry *rse, Definition d);


#endif
