#ifndef VERBUM_TABLE_H
#define VERBUM_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../grammar/grammar.h"


/* ParseTable API
 */

typedef enum ParseTableActionMask {
	ParseTableActionMask_None = 0x0 << 0,
	ParseTableActionMask_Error = 0x1 << 0,
	ParseTableActionMask_Accept = 0x1 << 1,
	ParseTableActionMask_Shift = 0x1 << 2,
	ParseTableActionMask_Reduce = 0x1 << 3,
} ParseTableActionMask;


/* Simple alias
 */
typedef uint32_t ParseTableGoto;

/* ParseTableActionType
 *
 * 	ParseTableActionType_Error: Parser has entered an invalid state
 * 	ParseTableActionType_Accept: Parser has reached a valid state
 * 	ParseTableActionType_Shift: Parser has entered state where no reduction can happen, shift input on stack
 * 	ParseTableActionType_Reduce: Parser has entered state where a rule can be produce, pop and place Symbol on stack
 */
typedef enum ParseTableActionType {
	ParseTableActionType_Error = -1,
	ParseTableActionType_Accept,
	ParseTableActionType_Shift,
	ParseTableActionType_Reduce,
} ParseTableActionType;

/* ParseTableAction
 *
 * Bitmask representing the possible states an action can be
 *
 * If action > 0 it is a shift else a reduce.
 * If action is INT32_MIN it is an error state.
 * If action is INT32_MAX it is an accept state.
 * 
 */
typedef int32_t ParseTableAction;
static const int32_t ActionError = INT32_MIN;
static const int32_t ActionAccept = INT32_MAX;

/* Returns the action type
 *
 */
/* inline ParseTableActionType parse_table_action_type(ParseTableAction action) {
	if(action < 0) {
		if(action == ActionError) {
			return ParseTableActionType_Error;
		}

		return ParseTableActionType_Reduce;
	} else {
		if(action == ActionAccept) {
			return ParseTableActionType_Accept;
		}

		return ParseTableActionType_Shift;
	}
} */

/* ParseTable
 * 	The ParseTable contains all the combinations that a parser can be in
 *
 * MEMBERS:
 * 	action_table: is implemented as a 1d array but will be interacted with as a 2d array
 * 	goto_table: is implemented as a 1d array but will be interacted with as a 2d array
 *
 * 	num_*: number of * in the table
 *
 * NOTE:
 * 	the *_table members were implemented as 1d arrays to increase performance by reducing cache misses
 *
 *
 */
/* typedef struct ParseTable {
	ParseTableAction *action_table; // 2D array: [num_states][num_terminals]
	ParseTableGoto *goto_table;     // 2D array: [num_states][num_non_terminals]

	uint32_t num_states;
	uint32_t num_terminals;
	uint32_t num_non_terminals;
} ParseTable; */

/* table_new
 *
 * DESCRIPTION:
 * 	Creates a new table and allocates memory for the backing arrays
 *
 * RETURNS:
 * 	newly constructed ParseTable
 *
 */
/* ParseTable parse_table_new(uint32_t num_states, uint32_t num_terminals, uint32_t num_non_terminals);

bool parse_table_populate(ParseTable *table, Grammar *g);

void parse_table_delete(ParseTable *t); */


/* parse_table_action_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
//inline ParseTableAction parse_table_action_at(ParseTable *t, uint32_t row, uint32_t column);

/* parse_table_goto_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
//inline ParseTableGoto parse_table_goto_at(ParseTable *t, uint32_t row, uint32_t column);

/* parse_table_action_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
/* inline ParseTableAction parse_table_action_at(ParseTable *t, uint32_t row, uint32_t column) {
	return t->action_table[(row * (t->num_terminals + 1)) + column];
} */

/* parse_table_goto_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
/* inline ParseTableGoto parse_table_goto_at(ParseTable *t, uint32_t row, uint32_t column) {
	return t->goto_table[(row * t->num_non_terminals) + column];
} */


#endif
