#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"
#include "../../lib/hashmap.c/hashmap.h"

#include "../token/token.h"
#include "../definition/definition.h"
#include "../memory/memory.h"

#include "rule_set.h"


static uint64_t rule_set_hash(const void *item, uint64_t seed0, uint64_t seed1);

static int rule_set_compare(const void *a, const void *b, void *udata);

static void rule_set_memory_delete(void *memory);

RuleSet rule_set_new() {
	return hashmap_new_with_allocator(memory_new, memory_resize, rule_set_memory_delete,
					  sizeof(RuleSetEntry), 0, 0, 0,
					  rule_set_hash, rule_set_compare, NULL, NULL);
}

void rule_set_delete(RuleSet set) {
	rule_set_iterate(set, rse, 
		for(Definition *it1 = cvector_begin(rse->definitions); it1 != cvector_end(rse->definitions); it1 += 1) {
			definition_delete(*it1);
		}

		cvector_free(rse->definitions);
	);

	hashmap_clear(set, false);
	hashmap_free(set);
}

bool rule_set_contains(RuleSet set, Token identifier, Definition definition) {
	const RuleSetEntry *rse = hashmap_get(set, &(RuleSetEntry) { .identifier = identifier });

	if(rse == NULL) {
		return false;
	}

	for(Definition *it1 = cvector_begin(rse->definitions); it1 != cvector_end(rse->definitions); it1 += 1) {
		if(!definition_equals(*it1, definition)) {
			return false;
		}
	}

	return true;
}

const void *rule_set_insert(RuleSet *set, Token identifier, Definition definition, uint8_t mask) {
	RuleSetEntry *rse = (RuleSetEntry *)hashmap_get(*set, &(RuleSetEntry) { .identifier = identifier });

	if(rse == NULL) {
		rse = &(RuleSetEntry) {
			.identifier = identifier,
			.mask = mask,
			.definitions = NULL,
		};
		rse->mask |= mask;
		
		cvector_push_back(rse->definitions, definition);

		return hashmap_set(*set, rse);
	}

	cvector_push_back(rse->definitions, definition);
	rse->mask |= mask;

	return rse;
}

RuleSetEntry rule_set_get(RuleSet set, Token identifier) {
	const RuleSetEntry *result = hashmap_get(set, &(RuleSetEntry) { .identifier = identifier, });

	if(result == NULL) {
		return (RuleSetEntry) { 0 };
	}

	return *result;
}

bool rule_set_remove(RuleSet *set, Token identifier) {
	hashmap_delete(*set, &(RuleSetEntry) { .identifier = identifier});

	return true;
}

void rule_set_update(RuleSet *set, Token identifier, cvector(Definition) definitions, uint8_t mask) {
	hashmap_set(*set, &(RuleSetEntry) { .identifier = identifier, .definitions = definitions, .mask = mask });
}


void rule_set_entry_delete(RuleSetEntry rse) {
	if(rse.mask & RuleSetEntryMask_Dynamic) {
		token_delete(rse.identifier);
	}
}

void rule_set_entry_push_back(RuleSetEntry *rse, Definition d) {
	cvector_push_back(rse->definitions, d);
}

static uint64_t rule_set_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const RuleSetEntry *rse = item;

	return hashmap_sip(rse->identifier.lexeme.data, strnlen(rse->identifier.lexeme.data, rse->identifier.lexeme.length), seed0, seed1);
}

static int rule_set_compare(const void *a, const void *b, void *udata) {
	const RuleSetEntry *ua = a;
	const RuleSetEntry *ub = b;

	if(ua->mask != ub->mask) {
		return false;
	}

	return !token_equals(ua->identifier, ub->identifier);
}

static void rule_set_memory_delete(void *memory) {
	rule_set_entry_delete(*(RuleSetEntry *) memory);
	memory_delete(memory);
}
