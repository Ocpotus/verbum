#define _GNU_SOURCE
#include <string.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"
#include "../../lib/hashmap.c/hashmap.h"

#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../memory/memory.h"

#include "rule_set.h"


static uint64_t rule_set_hash(const void *item, uint64_t seed0, uint64_t seed1);

static int rule_set_compare(const void *a, const void *b, void *udata);

RuleSet rule_set_new() {
	return hashmap_new_with_allocator(memory_new, memory_resize, memory_delete,
					  sizeof(Rule), 0, 0, 0,
					  rule_set_hash, rule_set_compare, NULL, NULL);
}

void rule_set_delete(RuleSet set) {
	rule_set_iterate(set, r, 
		for(Definition *it1 = cvector_begin(r->definitions); it1 != cvector_end(r->definitions); it1 += 1) {
			definition_delete(*it1);
		}

		cvector_free(r->definitions);
	);

	hashmap_clear(set, false);
	hashmap_free(set);
}

bool rule_set_contains(RuleSet set, Symbol identifier, Definition definition) {
	const Rule *r = hashmap_get(set, &(Rule) { .identifier = identifier });

	if(r == NULL) {
		return false;
	}

	for(Definition *it1 = cvector_begin(r->definitions); it1 != cvector_end(r->definitions); it1 += 1) {
		if(definition_equals(*it1, definition)) {
			return true;
		}
	}

	return false;
}

const Rule *rule_set_insert(RuleSet *set, Symbol identifier, Definition definition) {
	Rule *r = (Rule *)hashmap_get(*set, &(Rule) { .identifier = identifier });

	if(r == NULL) {
		Rule entry = {
			.identifier = identifier,
			.definitions = NULL,
		};

		cvector_push_back(entry.definitions, definition);
		hashmap_set(*set, &entry);

		// hashmap_set returns the replaced item, so fetch the stored one
		return hashmap_get(*set, &entry);
	}

	cvector_push_back(r->definitions, definition);
	r->identifier.mask |= identifier.mask;

	return r;
}

Rule rule_set_get(RuleSet set, Symbol identifier) {
	const Rule *result = hashmap_get(set, &(Rule) { .identifier = identifier, });

	if(result == NULL) {
		return (Rule) { 0 };
	}

	return *result;
}

bool rule_set_remove(RuleSet *set, Symbol identifier) {
	hashmap_delete(*set, &(Rule) { .identifier = identifier});

	return true;
}

void rule_set_update(RuleSet *set, Symbol identifier, cvector(Definition) definitions) {
	hashmap_set(*set, &(Rule) { .identifier = identifier, .definitions = definitions });
}

static uint64_t rule_set_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const Rule *r = item;

	return hashmap_sip(r->identifier.lexeme.data, strnlen(r->identifier.lexeme.data, r->identifier.lexeme.length), seed0, seed1);
}

static int rule_set_compare(const void *a, const void *b, void *udata) {
	const Rule *ua = a;
	const Rule *ub = b;

	(void) udata;

	return !symbol_equals(ua->identifier, ub->identifier);
}
