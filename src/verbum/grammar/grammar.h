#ifndef VERBUM_GRAMMAR_H
#define VERBUM_GRAMMAR_H

#include "../../lib/c-vector/cvector.h"

#include "../token_set/token_set.h"
//#include "../rule/rule.h"
#include "../rule_set/rule_set.h"


/* Grammar
 * 
 * Stores all necessary aspects of a formal language
 *
 * MEMBERS:
 * 	dynamic_tokens: this is for memory keeping and will
 * 			contain all dynamically constructed identifiers
 * 	rules: instructions on how to substitute non-terminals/variables
 * 	terminals: any token that cannot be substitute
 * 	variables: any token that can be substituted
 * 	start: the start symbol/entry point for a language
 *
 * NOTES:
 * 	repetition, optional, and group expressions are encoded into the
 * 	name of the rule with an @ followed by r, o, or g
 */
typedef struct Grammar {
	TokenSet dynamic_tokens;

	RuleSet rules;
	TokenSet terminals;
	TokenSet variables;
	const RuleSetEntry *start;
} Grammar;

Grammar grammar_new();

void grammar_delete(Grammar *g);


#endif
