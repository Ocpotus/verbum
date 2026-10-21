#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../token/token.h"
#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../grammar/grammar.h"
#include "../rule_set/rule_set.h"
#include "../raw_rule/raw_rule.h"

#include "analyzer.h"


static bool rule_referenced(RuleSet rules, RuleSetEntry target);

static cvector(Definition) hoist(Grammar *g, cvector(Token) body, size_t *position, TokenType sub_end_tag);

static Symbol create_sub_expression_identifier(TokenType tag);

Grammar analyzer_analyze(cvector(RawRule) raw_rules) {
	Grammar result = grammar_new();
	result.rules = rule_set_new();

	for(RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		size_t start_position = 0;
		Symbol identifier = symbol_from_token(it1->identifier);
		cvector(Definition) alternatives = hoist(&result, it1->definition, &start_position, TokenType_Invalid);

		for(Definition *it2 = cvector_begin(alternatives); it2 != cvector_end(alternatives); it2 += 1) {
			if(rule_set_insert(&result.rules, identifier, *it2) == NULL) {
				// Error
			}
		}

		symbol_set_insert(&result.nonterminals, identifier);
		cvector_free(alternatives);
	}

	rule_set_iterate(result.rules, rule, {
		if(!rule_referenced(result.rules, *rule)) {
			result.start = rule;
			break;
		}
	});

	return result;
}

/* hoist
 *
 * Converts tokens into symbols, hoisting sub expressions into their own rules
 *
 * PARAMETERS:
 * 	g: grammar to insert hoisted rules and symbols into
 * 	definition: tokens to convert
 * 	position: current position in definition, left after the closing token
 * 	sub_end_tag: closing token expected, TokenType_Invalid if top level
 *
 * RETURNS:
 * 	a definition for each alternative separated by '|'
 */
static cvector(Definition) hoist(Grammar *g, cvector(Token) definition, size_t *position, TokenType sub_end_tag) {
	cvector(Definition) result = NULL;
	Definition current = definition_new();

	while(*position < cvector_size(definition)) {
		Token current_token = definition[*position];

		if(current_token.tag == TokenType_LeftBrace ||
		   current_token.tag == TokenType_LeftBracket ||
		   current_token.tag == TokenType_LeftParenthesis) {
			// Get sub expression offset by 1
			*position += 1;
			cvector(Definition) sub_alternatives = hoist(g, definition, position, current_token.tag + 1);
			Symbol sub_identifier = create_sub_expression_identifier(current_token.tag);

			for(Definition *it1 = cvector_begin(sub_alternatives); it1 != cvector_end(sub_alternatives); it1 += 1) {
				if(sub_identifier.mask & SymbolMask_Repetition) {
					definition_push_back(it1, sub_identifier);
				}

				if(rule_set_insert(&g->rules, sub_identifier, *it1) == NULL) {
					// Error
				}
			}

			// λ alternative
			if(sub_identifier.mask & (SymbolMask_Repetition | SymbolMask_Optional)) {
				rule_set_insert(&g->rules, sub_identifier, definition_new());
			}

			cvector_free(sub_alternatives);

			definition_push_back(&current, sub_identifier);
			symbol_set_insert(&g->nonterminals, sub_identifier);
			symbol_set_insert(&g->dynamic_symbols, sub_identifier);
		} else if(current_token.tag == TokenType_Single_Literal || current_token.tag == TokenType_Double_Literal) {
			Symbol terminal = symbol_from_token(current_token);

			definition_push_back(&current, terminal);
			symbol_set_insert(&g->terminals, terminal);
			*position += 1;
		} else if(current_token.tag == TokenType_Identifier) {
			Symbol nonterminal = symbol_from_token(current_token);

			definition_push_back(&current, nonterminal);
			symbol_set_insert(&g->nonterminals, nonterminal);
			*position += 1;
		} else if(current_token.tag == TokenType_RightBrace ||
			  current_token.tag == TokenType_RightBracket ||
			  current_token.tag == TokenType_RightParenthesis) {
			if(current_token.tag != sub_end_tag) {
				// Error
				// mismatch
			}

			*position += 1;

			break;
		} else if(current_token.tag == TokenType_Pipe) {
			cvector_push_back(result, current);
			current = definition_new();
			*position += 1;
		} else {
			*position += 1;
		}
	}

	cvector_push_back(result, current);

	return result;
}

static Symbol create_sub_expression_identifier(TokenType tag) {
	char name[17] = { 0 };
	char prefix = '\0';
	uint32_t mask = SymbolMask_Dynamic;

	switch(tag) {
	case TokenType_LeftParenthesis:
		prefix = 'g';
		mask |= SymbolMask_Group;
		break;
	case TokenType_LeftBrace:
		prefix = 'r';
		mask |= SymbolMask_Repetition;
		break;
	case TokenType_LeftBracket:
		prefix = 'o';
		mask |= SymbolMask_Optional;
		break;
	default:
		// Error
		break;
	}

	snprintf(name, sizeof(name) /  sizeof(*name), "@%c%u", prefix, (unsigned int) rand());

	return symbol_new((Lexeme) {
		.data = memory_copy(name, sizeof(name) / sizeof(*name)),
		.length = strlen(name),
	}, mask);
}

static bool rule_referenced(RuleSet rules, RuleSetEntry target) {
	rule_set_iterate(rules, it1, {
		for(Definition *it2 = cvector_begin(it1->definitions); it2 != cvector_end(it1->definitions); it2 += 1) {
			for(Symbol *it3 = cvector_begin(*it2); it3 != cvector_end(*it2); it3 += 1) {
				if(symbol_equals(*it3, target.identifier)) {
					return true;
				}
			}
		}
	});

	return false;
}
