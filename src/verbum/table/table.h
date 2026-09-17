#ifndef VERBUM_TABLE_H
#define VERBUM_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../grammar/grammar.h"


/* ParseTable API
 */

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
 * MEMBERS:
 * 	shift:
 * 		destination: where to go
 * 	reduce:
 * 		destination: where to go
 *
 * 	tag: type of ParseTableAction
 */
typedef struct ParseTableAction {
	union {
		struct {
			size_t destination;
		} shift;
		struct {
			size_t destination;
		} reduce;
	};
	ParseTableActionType tag;
} ParseTableAction;

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
typedef struct ParseTable {
	ParseTableAction *action_table; // 2D array: [num_states][num_terminals]
	ParseTableGoto *goto_table;     // 2D array: [num_states][num_non_terminals]

	uint32_t num_states;
	uint32_t num_terminals;
	uint32_t num_non_terminals;
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
ParseTable parse_table_new(uint32_t num_states, uint32_t num_terminals, uint32_t num_non_terminals);

bool parse_table_populate(ParseTable *table, Grammar *g);

void parse_table_delete(ParseTable *t);


/* parse_table_action_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
inline ParseTableAction parse_table_action_at(ParseTable *t, uint32_t row, uint32_t column);

/* parse_table_goto_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
inline ParseTableGoto parse_table_goto_at(ParseTable *t, uint32_t row, uint32_t column);

/* parse_table_action_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
inline ParseTableAction parse_table_action_at(ParseTable *t, uint32_t row, uint32_t column) {
	return t->action_table[(row * (t->num_terminals + 1)) + column];
}

/* parse_table_goto_at
 *
 * DESCRIPTION:
 * 	Gets the element at the specified coordinates
 *
 */
inline ParseTableGoto parse_table_goto_at(ParseTable *t, uint32_t row, uint32_t column) {
	return t->goto_table[(row * t->num_non_terminals) + column];
}


#endif
