#ifndef VERBUM_LALR_H
#define VERBUM_LALR_H

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"
#include "../lr0/lr0.h"

/*
typedef struct LALRItem {
	Item item;
	cvector(Token) lookaheads;
} LALRItem;

typedef struct LALRState {
	cvector(LALRItem) items;
} LALRState;

typedef struct LALR {
	cvector(LALRState) states;
	cvector()
} LALR;

void lalr_construct(Grammar g, LR0 lr0);
*/

#endif
