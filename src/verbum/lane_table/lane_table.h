#ifndef VERBUM_LANE_TABLE_H
#define VERBUM_LANE_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"
#include "../grammar/grammar.h"
#include "../lr0/lr0.h"
#include "../lr1/lr1.h"


/* Lane Table API
 *
 * LR(1) construction based on Pager and Chen, "The Lane Table Method Of Constructing LR(1) Parsers".
 * See https://github.com/lalrpop/lalrpop/blob/master/lalrpop/src/lr1/lane_table/README.md
 *
 * In short:
 * 	1. every LR(0) state is converted into an LR(0-1) state, every item has a wildcard lookahead
 * 	2. every inconsistent state is resolved by tracing the lookahead of each conflicting action
 * 	   backwards through the automaton, the result being a lane table
 * 	3. if the columns of the lane table are disjoint the lookaheads are simply updated (LALR)
 * 	4. otherwise the states along the lanes are cloned until every lane has a consistent context
 */

/* lane_table_new
 *
 * Builds an LR(1) automaton out of an LR(0) automaton, splitting states only when needed
 *
 * PARAMETERS:
 * 	g: the grammar lr0 was built from
 * 	lr0: the LR(0) automaton, it is only read from
 * 	conflicts: appended with the states that are still inconsistent (the grammar is not LR(1)), may be NULL
 *
 * RETURNS:
 * 	a newly constructed automaton, delete it with lr1_delete
 */
LR1Automaton lane_table_new(Grammar g, LR0Automaton lr0, cvector(size_t) *conflicts);


#endif
