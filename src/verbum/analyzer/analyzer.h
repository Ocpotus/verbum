#ifndef VERBUM_ANALYZER_H
#define VERBUM_ANALYZER_H

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"
#include "../lexer/lexer.h"
#include "../grammar/grammar.h"


typedef struct Analyzer Analyzer;


Analyzer *analyzer_new(Lexer *l);

void analyzer_delete(Analyzer *a);

Grammar analyzer_analyze(Analyzer *a);


#endif
