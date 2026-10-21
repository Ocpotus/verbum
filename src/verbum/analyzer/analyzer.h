#ifndef VERBUM_ANALYZER_H
#define VERBUM_ANALYZER_H

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"
#include "../raw_rule/raw_rule.h"


/* Constructs a grammar from a vector of raw rules
 *
 * NOTES:
 * 	the grammar borrows lexemes from the Lexer that produced the rules,
 * 	delete the Grammar before the Lexer
 */
Grammar analyzer_analyze(cvector(RawRule) rules);


#endif
