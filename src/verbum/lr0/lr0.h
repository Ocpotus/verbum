#ifndef VERBUM_LR0_H
#define VERBUM_LR0_H

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"
#include "../token_set/token_set.h"


typedef struct LR0Item {
	Rule rule;
	size_t dot_pos;
} LR0Item;

typedef struct LR0State {
	cvector(LR0Item) items;
} LR0State;

typedef struct LR0Transition {
	size_t id;
	size_t source_index;
	size_t destination_index;
	Token symbol;
} LR0Transition;

typedef struct LR0Parser {
	cvector(LR0Transition) transitions;
	cvector(LR0State) states;
} LR0Parser;

LR0Parser lr0_new(Grammar g);

void lr0_delete(LR0Parser lr0);


#endif
