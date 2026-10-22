#ifndef VERBUM_LR1_H
#define VERBUM_LR1_H

#include "../../lib/c-vector/cvector.h"

#include "../lr0/lr0.h"
#include "../grammar/grammar.h"
#include "../token_set/token_set.h"


/* typedef struct LR1Item {
	LR0Item lr0;
	Token lookahead;
} LR1Item;

typedef struct LR1State {
	cvector(LR1Item) items;
} LR1State;

typedef struct LR1Transition {
	size_t id;
	size_t source_index;
	size_t destination_index;
	Token symbol;
} LR1Transition;

typedef struct LR1Parser {
	cvector(LR1Transition) transitions;
	cvector(LR1State) states;
} LR1Parser;

LR1Parser lr1_new(Grammar g, LR0Parser lr0);

void lr1_delete(LR1Parser lr1);
*/

#endif
