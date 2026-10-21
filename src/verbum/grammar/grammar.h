#ifndef VERBUM_GRAMMAR_H
#define VERBUM_GRAMMAR_H

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"
#include "../rule_set/rule_set.h"


/* Grammar
 * 
 * Stores all necessary aspects of a formal language
 *
 * MEMBERS:
 * 	dynamic_symbols: this is for memory keeping and will
 * 			 contain all dynamically constructed identifiers
 * 	rules: instructions on how to substitute nonterminals
 * 	terminals: any symbol that cannot be substitute
 * 	nonterminals: any symbol that can be substituted
 * 	start: the start symbol/entry point for a language
 *
 * NOTES:
 * 	repetition, optional, and group expressions are encoded into the
 * 	name of the rule with an @ followed by r, o, or g
 *
 * 	only dynamic_symbols own their lexemes, every other symbol borrows
 * 	from the Lexer's source
 */
typedef struct Grammar {
	SymbolSet dynamic_symbols;

	RuleSet rules;
	SymbolSet terminals;
	SymbolSet nonterminals;
	const RuleSetEntry *start;
} Grammar;

Grammar grammar_new();

void grammar_delete(Grammar *g);


#endif
