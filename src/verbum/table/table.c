#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/log.c/log.h"
#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"

#include "../symbol/symbol.h"
#include "../grammar/grammar.h"
#include "../lr1/lr1.h"

#include "table.h"


static bool parse_table_populate(ParseTable *table, Grammar *g, LR1Automaton lr1);
static bool parse_table_set_action(ParseTable *t, size_t state, size_t column, ParseTableAction action);
static bool parse_table_productions(ParseTable *t, Grammar *g, size_t *base);
static bool parse_table_reduce(ParseTable *t, Grammar *g, LR1Automaton *lr1, size_t *base, size_t state, LR1Item *item, bool wildcard);

//
// (row * width) + col
ParseTable parse_table_new(Grammar grammar, LR1Automaton lr1) {
	//ParseTable table = parse_table_new(cvector_size(lane_table.states), cvector_size(lane_table.terminals), symbol_set_size(grammar.nonterminals));
	ParseTable result = {
		.num_states = cvector_size(lr1.states),
		.num_terminals = symbol_set_size(grammar.terminals),
		.num_nonterminals = symbol_set_size(grammar.nonterminals),
	};

	result.goto_table = memory_initialize((((size_t) result.num_states) * ((size_t )result.num_nonterminals)) * sizeof(*result.goto_table), 0);

	if(result.goto_table == NULL) {
		// Error
	}

	result.action_table = memory_initialize((((size_t) result.num_states) * ((size_t) result.num_terminals + 1)) * sizeof(*result.action_table), 0);

	if(result.action_table == NULL) {
		// Error
	}

	if(!parse_table_populate(&result, &grammar, lr1)) {
		// Error
	}

	return result;
}

void parse_table_delete(ParseTable *t) {
	memory_delete(t->goto_table);
	memory_delete(t->action_table);
	memory_delete(t->productions);
}

ParseTableAction parse_table_action_new(ParseTableActionType type, uint32_t value) {
	return ((ParseTableAction) type << ParseTableActionTypeShift) | (value & ParseTableActionValueMask);
}

ParseTableActionType parse_table_action_type(ParseTableAction action) {
	return (ParseTableActionType) (action >> ParseTableActionTypeShift);
}

uint32_t parse_table_action_value(ParseTableAction action) {
	return action & ParseTableActionValueMask;
}

static const char *parse_table_action_name(ParseTableAction action) {
	switch(parse_table_action_type(action)) {
		case ParseTableActionType_Accept: return "accept";
		case ParseTableActionType_Shift: return "shift";
		case ParseTableActionType_Reduce: return "reduce";
		default: return "error";
	}
}

ParseTableAction parse_table_action_at(ParseTable *t, uint32_t row, uint32_t column) {
	return t->action_table[(((size_t) row) * (t->num_terminals + 1)) + column];
}

ParseTableGoto parse_table_goto_at(ParseTable *t, uint32_t row, uint32_t column) {
	return t->goto_table[(((size_t) row) * t->num_nonterminals) + column];
}

/* parse_table_set_action
 *
 * Writes an action into an empty cell, an occupied cell with a different action is a conflict
 *
 * RETURNS:
 * 	false on a conflict, the cell is left untouched
 */
static bool parse_table_set_action(ParseTable *t, size_t state, size_t column, ParseTableAction action) {
	ParseTableAction *cell = &t->action_table[(state * (t->num_terminals + 1)) + column];

	if(parse_table_action_type(*cell) != ParseTableActionType_Error && *cell != action) {
		log_error("conflict in state %zu on column %zu: %s %u against %s %u", state, column,
			parse_table_action_name(*cell), parse_table_action_value(*cell),
			parse_table_action_name(action), parse_table_action_value(action));

		return false;
	}

	*cell = action;

	return true;
}

/* parse_table_productions
 *
 * Numbers every production, grouped by the index of their nonterminal so ids are stable across runs
 *
 * PARAMETERS:
 * 	base: filled with the id of the first production of each nonterminal, sized num_non_terminals
 *
 * RETURNS:
 * 	false if a rule has no nonterminal
 */
static bool parse_table_productions(ParseTable *t, Grammar *g, size_t *base) {
	bool result = true;
	size_t total = 0;

	for(size_t i = 0; i < t->num_nonterminals; i += 1) {
		base[i] = 0;
	}

	rule_set_iterate(g->rules, rule, {
		bool found = true;
		size_t lhs = symbol_set_index(g->nonterminals, rule->identifier, &found);

		if(!found) {
			log_error("rule %.*s is not a nonterminal", (int) rule->identifier.lexeme.length, rule->identifier.lexeme.data);
			result = false;
			continue;
		}

		base[lhs] = cvector_size(rule->definitions);
		total += base[lhs];
	});

	// Counts to offsets
	for(size_t i = 0, offset = 0; i < t->num_nonterminals; i += 1) {
		size_t count = base[i];

		base[i] = offset;
		offset += count;
	}

	memory_delete(t->productions);
	t->productions = memory_new(total * sizeof(*t->productions));
	t->num_productions = (uint32_t) total;

	if(t->productions == NULL && total != 0) {
		return false;
	}

	rule_set_iterate(g->rules, rule, {
		bool found = true;
		size_t lhs = symbol_set_index(g->nonterminals, rule->identifier, &found);

		if(!found) {
			continue;
		}

		for(size_t i = 0; i < cvector_size(rule->definitions); i += 1) {
			t->productions[base[lhs] + i] = (ParseTableProduction) {
				.lhs = lhs,
				.length = cvector_size(rule->definitions[i]),
			};
		}
	});

	return result;
}

/* parse_table_reduce
 *
 * Writes the reduce or accept actions of a complete item
 *
 * PARAMETERS:
 * 	wildcard: only handle wildcard items when true, only explicit lookaheads when false
 */
static bool parse_table_reduce(ParseTable *t, Grammar *g, LR1Automaton *lr1, size_t *base, size_t state, LR1Item *item, bool wildcard) {
	bool result = true;
	const LR0Item *lr0 = &item->lr0;
	bool found = true;

	if((item->lookahead == NULL) != wildcard || lr0->dot_pos != cvector_size(lr0->rule.definitions[lr0->rule_index])) {
		return true;
	}

	size_t lhs = symbol_set_index(g->nonterminals, lr0->rule.identifier, &found);

	if(!found) {
		return false;
	}

	ParseTableAction reduce = parse_table_action_new(ParseTableActionType_Reduce, (uint32_t) (base[lhs] + lr0->rule_index));
	ParseTableAction accept = parse_table_action_new(ParseTableActionType_Accept, 0);
	bool start = symbol_equals(lr0->rule.identifier, g->start->identifier);

	for(size_t column = 0; column <= lr1_end(lr1); column += 1) {
		// The augmented start rule is only ever reduced at the end of input, as an accept
		if(start && column != lr1_end(lr1)) {
			continue;
		}

		ParseTableAction action = start ? accept : reduce;

		if(wildcard) {
			// A wildcard was never needed to resolve a conflict, it only fills what is left
			if(parse_table_action_type(t->action_table[(state * (t->num_terminals + 1)) + column]) == ParseTableActionType_Error) {
				t->action_table[(state * (t->num_terminals + 1)) + column] = action;
			}
		} else if(lr1_lookahead_contains(item->lookahead, column)) {
			result &= parse_table_set_action(t, state, column, action);
		}
	}

	return result;
}

static bool parse_table_populate(ParseTable *table, Grammar *g, LR1Automaton lr1) {
	bool result = true;

	if((symbol_set_size(g->terminals) != (size_t) table->num_terminals) || (lr1_end(&lr1) != (size_t) table->num_terminals)
		|| (symbol_set_size(g->nonterminals) != (size_t) table->num_nonterminals) || (cvector_size(lr1.states) != (size_t) table->num_states)) {
		log_error("parse table does not match the grammar and automaton sizes");

		return false;
	}

	if(table->num_states > ParseTableActionValueMask) {
		log_error("%u states do not fit in an action", table->num_states);

		return false;
	}

	for(size_t i = 0; i < (size_t) table->num_states * (table->num_terminals + 1); i += 1) {
		table->action_table[i] = parse_table_action_new(ParseTableActionType_Error, 0);
	}

	for(size_t i = 0; i < (size_t) table->num_states * table->num_nonterminals; i += 1) {
		table->goto_table[i] = ParseTableGotoError;
	}

	size_t *base = memory_new((table->num_nonterminals + 1) * sizeof(*base));

	if(base == NULL) {
		return false;
	}

	result &= parse_table_productions(table, g, base);

	if(table->num_productions > ParseTableActionValueMask) {
		log_error("%u productions do not fit in an action", table->num_productions);
		memory_delete(base);

		return false;
	}

	// Shifts and gotos
	for(LR1Transition *it1 = cvector_begin(lr1.transitions); it1 != cvector_end(lr1.transitions); it1 += 1) {
		if(it1->symbol.mask & SymbolMask_Terminal) {
			bool found = true;
			size_t column = symbol_set_index(g->terminals, it1->symbol, &found);

			if(!found) {
				log_error("transition %zu shifts an unknown terminal", it1->id);
				result = false;
				continue;
			}

			result &= parse_table_set_action(table, it1->source, column, parse_table_action_new(ParseTableActionType_Shift, (uint32_t) it1->destination));
		} else {
			bool found = true;
			size_t column = symbol_set_index(g->nonterminals, it1->symbol, &found);

			if(!found) {
				log_error("transition %zu goes over an unknown nonterminal", it1->id);
				result = false;
				continue;
			}

			table->goto_table[(it1->source * table->num_nonterminals) + column] = (ParseTableGoto) it1->destination;
		}
	}

	// Reduces, explicit lookaheads claim their cells before wildcards fill the rest
	for(size_t pass = 0; pass < 2; pass += 1) {
		for(size_t i = 0; i < cvector_size(lr1.states); i += 1) {
			for(LR1Item *it1 = cvector_begin(lr1.states[i].items); it1 != cvector_end(lr1.states[i].items); it1 += 1) {
				result &= parse_table_reduce(table, g, &lr1, base, i, it1, pass == 1);
			}
		}
	}

	memory_delete(base);

	return result;
}
