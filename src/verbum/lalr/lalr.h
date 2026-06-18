#ifndef VERBUM_LALR_H
#define VERBUM_LALR_H

#include "../grammar/grammar.h"
#include "../lr0/lr0.h"


void lalr_construct(Grammar g, LR0 lr0);


#endif
