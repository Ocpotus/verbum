#ifndef VERBUM_TABLE_H
#define VERBUM_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../grammar/grammar.h"
#include "../lr1/lr1.h"


/* ParseTable API
 */

/*
 * C99 standard guarantees sizeof enum is >= sizeof int
 * First 4 bits will store metadata
 * Last 28 bits will be used store to specify the state or production
 * Allowing for 2^28 # of states to shift to or productions to reduce by. Which is more than sufficient...
 *
 * An all zero action is an error so a zeroed table starts out as all errors
 *
 * Diagram:
 * 	Reduce #33: 0 1 0 0 | 0 0 ... 1 0 0 0 0 1
 * 		    	    | \_________________/
 *	                    |  \What production to reduce by
 * 	      Reduce bit set/
 */
typedef enum ParseTableActionType {
	ParseTableActionType_Error = 0x0 << 0,
	ParseTableActionType_Accept = 0x1 << 0,
	ParseTableActionType_Shift = 0x1 << 1,
	ParseTableActionType_Reduce = 0x1 << 2,
} ParseTableActionType;

typedef uint32_t ParseTableAction;

static const uint32_t ParseTableActionTypeShift = 28;
static const uint32_t ParseTableActionValueMask = (UINT32_C(1) << 28) - 1;

/* GotoError
 *
 * Marks a goto table entry with no transition
 */
static const uint32_t ParseTableGotoError = UINT32_MAX;

typedef uint32_t ParseTableGoto;

/* ParseTableProduction
 *
 * MEMBERS:
 * 	lhs: index of the produced nonterminal, the goto table column
 * 	length: number of symbols on the right hand side, states to pop on a reduce
 */
typedef struct ParseTableProduction {
	size_t lhs;
	size_t length;
} ParseTableProduction;

/* parse_table_action_new
 *
 * PARAMETERS:
 * 	type: kind of action
 * 	value: state to shift to or production to reduce by, truncated to 28 bits
 *
 * RETURNS:
 * 	the packed action
 */
ParseTableAction parse_table_action_new(ParseTableActionType type, uint32_t value);

/* Returns the action type
 *
 */
ParseTableActionType parse_table_action_type(ParseTableAction action);

/* Returns the state to shift to or the production to reduce by
 *
 */
uint32_t parse_table_action_value(ParseTableAction action);

/* ParseTable
 * 	The ParseTable contains all the combinations that a parser can be in
 *
 * MEMBERS:
 * 	action_table: is implemented as a 1d array but will be interacted with as a 2d array
 * 	goto_table: is implemented as a 1d array but will be interacted with as a 2d array
 *
 * 	productions: every production, the value of a reduce action indexes into it
 *
 * 	num_*: number of * in the table
 *
 * NOTE:
 * 	the *_table members were implemented as 1d arrays to increase performance by reducing cache misses
 *
 * 	action table columns are symbol_set_index of the terminal, column num_terminals is the end of input
 *
 */
typedef struct ParseTable {
	ParseTableAction *action_table; // 2D array: [num_states][num_terminals + 1]
	ParseTableGoto *goto_table;     // 2D array: [num_states][num_non_terminals]
	ParseTableProduction *productions;

	uint32_t num_states;
	uint32_t num_terminals;
	uint32_t num_nonterminals;
	uint32_t num_productions;
} ParseTable;

/* table_new
 *
 * DESCRIPTION:
 * 	Creates a new table and allocates memory for the backing arrays
 *
 * RETURNS:
 * 	newly constructed ParseTable
 *
 */
ParseTable parse_table_new(Grammar grammar, LR1Automaton lr1);

/* parse_table_populate
 *
 * DESCRIPTION:
 * 	Fills the action, goto, and production tables from an LR(1) automaton.
 * 	Every cell is reset first so the table can be repopulated.
 *
 * PARAMETERS:
 * 	table: table sized for lr1 and g
 * 	g: the grammar lr1 was built from
 * 	lr1: automaton with its lookaheads computed, NULL lookaheads reduce on every free terminal
 *
 * RETURNS:
 * 	false on a size mismatch, an unknown symbol, or a conflict, the conflicting cell keeps its first action
 */
//bool parse_table_populate(ParseTable *table, Grammar *g, LR1Automaton lr1);

void parse_table_delete(ParseTable *t);


/* parse_table_action_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
ParseTableAction parse_table_action_at(ParseTable *t, uint32_t row, uint32_t column);

/* parse_table_goto_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
ParseTableGoto parse_table_goto_at(ParseTable *t, uint32_t row, uint32_t column);


#endif
