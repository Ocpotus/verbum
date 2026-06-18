#ifndef VERBUM_LR0_H
#define VERBUM_LR0_H

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"


typedef struct Item {
	Rule rule;
	size_t dot_pos;
	cvector(Token) lookaheads;
} Item;

typedef struct State {
	cvector(Item) items;
} State;

typedef struct Transition {
	size_t id;
	size_t source_index;
	size_t destination_index;
	cvector(Token) direct_reads;
	Token symbol;
} Transition;

typedef struct LR0 {
	cvector(Transition) transitions;
	cvector(State) states;
} LR0;

LR0 lr0_new(Grammar g);

void lr0_delete(LR0 lr0);


#endif
