#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"

#include "../grammar/grammar.h"

#include "table.h"

//
// (row * width) + col
ParseTable parse_table_new(uint32_t num_states, uint32_t num_terminals, uint32_t num_non_terminals) {
	ParseTable result = {
		.num_states = num_states,
		.num_terminals = num_terminals,
		.num_non_terminals = num_non_terminals,
	};

	result.goto_table = memory_new((num_states * num_non_terminals) * sizeof(*result.goto_table));

	if(result.goto_table == NULL) {
		// Error
	}

	result.action_table = memory_new((num_states * num_terminals + 1) * sizeof(*result.action_table));

	if(result.action_table == NULL) {
		// Error
	}

	return result;
}

void parse_table_delete(ParseTable *t) {
	memory_delete(t->goto_table);
	memory_delete(t->action_table);
}

bool parse_table_populate(ParseTable *table, Grammar *g) {
	bool result = true;

	if((cvector_size(g->terminals) != (size_t) table->num_terminals) || (cvector_size(g->variables) != (size_t) table->num_non_terminals)) {
		// Error: mismatch grammar and table size
		return false;
	}


	return result;
}
