#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"
#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../grammar/grammar.h"
#include "../rule/rule.h"

#include "../../lib/log.c/log.h"

#include "analyzer.h"


static const char * const sub_expression_prefixes = "org";
static const Symbol augmented_start = {
	.lexeme = { .data = "π", .length = 2 },
	.mask = SymbolMask_None,
};

/* Context
 *
 * MEMBERS:
 * 	num_*: sub expressions hoisted so far, shared by every rule so hoisted names never collide
 * 	lexical_rules: names of every lexical (':=') rule
 * 	lexical: true while hoisting a lexical rule
 */
typedef struct Context {
	uint32_t num_repetition;
	uint32_t num_optional;
	uint32_t num_group;
	SymbolSet lexical_rules;
	bool lexical;
} Context;

static bool rule_referenced(RuleSet rules, Rule target);
static cvector(Definition) hoist(Grammar *g, cvector(Token) body, size_t *position, TokenType sub_end_tag, Context *context);
static Symbol create_sub_expression_identifier(TokenType tag, Context *context);
static bool rule_derives_cycle(RuleSet rules, Symbol identifier, cvector(Symbol) *path, SymbolSet *done);
static void check_raw_rules(cvector(RawRule) raw_rules, SymbolSet *lexical_rules);
static void mark_tokens(Grammar *g);

Grammar analyzer_analyze(cvector(RawRule) raw_rules) {
	Grammar result = grammar_new();
	result.rules = rule_set_new();
	Context context = {
		.num_group = 0,
		.num_optional = 0,
		.num_repetition = 0,
		.lexical_rules = symbol_set_new(),
		.lexical = false,
	};

	check_raw_rules(raw_rules, &context.lexical_rules);

	for(RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		size_t start_position = 0;
		Symbol identifier = symbol_from_token(it1->identifier);

		context.lexical = it1->lexical;

		cvector(Definition) alternatives = hoist(&result, it1->definition, &start_position, TokenType_Invalid, &context);

		for(Definition *it2 = cvector_begin(alternatives); it2 != cvector_end(alternatives); it2 += 1) {
			if(rule_set_insert(&result.rules, identifier, *it2) == NULL) {
				// Error
			}
		}

		if(it1->lexical) {
			rule_set_mark(&result.rules, identifier, RuleMask_Lexical);
		} else {
			symbol_set_insert(&result.nonterminals, identifier);
		}

		cvector_free(alternatives);
	}

	mark_tokens(&result);

	rule_set_iterate(result.rules, rule, {
		// Tokens are only referenced as terminals, so lexical rules look unreferenced
		if(!(rule->mask & RuleMask_Lexical) && !rule_referenced(result.rules, *rule)) {
			result.start = rule;
			break;
		}
	});

	if(result.start != NULL) {
		// Copy before inserting, the insert can move the start rule
		Symbol start = result.start->identifier;
		Definition definition = definition_new();

		definition_push_back(&definition, start);
		result.start = rule_set_insert(&result.rules, augmented_start, definition);
		symbol_set_insert(&result.nonterminals, augmented_start);
	}

	rule_set_iterate(result.rules, it1, {
		cvector(Symbol) path = NULL;
		SymbolSet done = symbol_set_new();

		if(rule_derives_cycle(result.rules, it1->identifier, &path, &done)) {
			rule_set_mark(&result.rules, it1->identifier, RuleMask_Cycle);
		}

		cvector_free(path);
		symbol_set_delete(done);
	});

	// The lexer can only be built from regular rules
	for(const RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		if(it1->lexical && (rule_set_get(result.rules, symbol_from_token(it1->identifier)).mask & RuleMask_Cycle)) {
			log_error("%u:%u: lexical rule '%.*s' is not regular, it derives a cycle",
					it1->identifier.pos.row, it1->identifier.pos.col,
					(int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
		}
	}

	symbol_set_delete(context.lexical_rules);

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
static cvector(Definition) hoist(Grammar *g, cvector(Token) definition, size_t *position, TokenType sub_end_tag, Context *context) {
	cvector(Definition) result = NULL;
	Definition current = definition_new();

	while(*position < cvector_size(definition)) {
		Token current_token = definition[*position];

		if(current_token.tag == TokenType_LeftBrace ||
		   current_token.tag == TokenType_LeftBracket ||
		   current_token.tag == TokenType_LeftParenthesis) {
			// Get sub expression offset by 1
			*position += 1;
			cvector(Definition) sub_alternatives = hoist(g, definition, position, current_token.tag + 1, context);
			Symbol sub_identifier = create_sub_expression_identifier(current_token.tag, context);

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

			if(context->lexical) {
				rule_set_mark(&g->rules, sub_identifier, RuleMask_Lexical);
			} else {
				symbol_set_insert(&g->nonterminals, sub_identifier);
			}
		} else if(current_token.tag == TokenType_Single_Literal || current_token.tag == TokenType_Double_Literal) {
			Symbol terminal = symbol_from_token(current_token);

			definition_push_back(&current, terminal);

			// Literals inside lexical rules are text for the lexer, not parser terminals
			if(!context->lexical) {
				symbol_set_insert(&g->terminals, terminal);
			}

			*position += 1;
		} else if(current_token.tag == TokenType_Identifier) {
			Symbol reference = symbol_from_token(current_token);

			if(context->lexical) {
				// Kept as a plain reference, the lexer inlines it
			} else if(symbol_set_contains(context->lexical_rules, reference)) {
				// A syntax rule referencing a lexical rule sees it as a token
				reference.mask |= SymbolMask_Terminal;
				symbol_set_insert(&g->terminals, reference);
			} else {
				symbol_set_insert(&g->nonterminals, reference);
			}

			definition_push_back(&current, reference);
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

static Symbol create_sub_expression_identifier(TokenType tag, Context *context) {
	Symbol result = {
		.lexeme = { 0 },
		.mask = SymbolMask_None,
	};
	static const uint8_t offset = SYMBOL_MASK_INDEX_OFFSET;

	switch(tag) {
	case TokenType_LeftParenthesis:
		context->num_group += 1;
		result.lexeme = (Lexeme) {
			.data = &sub_expression_prefixes[2],
			.length = 1,
		};
		result.mask |= SymbolMask_Group;
		result.mask |= context->num_group << offset;
		break;
	case TokenType_LeftBrace:
		context->num_repetition += 1;
		result.lexeme = (Lexeme) {
			.data = &sub_expression_prefixes[1],
			.length = 1,
		};
		result.mask |= SymbolMask_Repetition;
		result.mask |= context->num_repetition << offset; break;
	case TokenType_LeftBracket:
		context->num_optional += 1;
		result.lexeme = (Lexeme) {
			.data = &sub_expression_prefixes[0],
			.length = 1,
		};
		result.mask |= SymbolMask_Optional;
		result.mask |= context->num_optional << offset;
		break;
	default:
		// Error
		break;
	}

	return result;
}

static bool rule_referenced(RuleSet rules, Rule target) {
	rule_set_iterate(rules, it1, {
		// Only syntax rules can reference the start rule
		if(it1->mask & RuleMask_Lexical) {
			continue;
		}

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

// AI generated
/* rule_derives_cycle
 *
 * Depth first search for a cycle reachable from a rule, the rule itself need not be on the cycle.
 * A repetition's reference to itself is ignored, it only encodes the repetition.
 *
 * PARAMETERS:
 * 	rules: rules to search
 * 	identifier: rule to search from
 * 	path: rules on the current search path, a reference back into it is a cycle
 * 	done: rules already fully searched without finding a cycle
 *
 * RETURNS:
 * 	true if a cycle can be derived from identifier, false if not
 */
static bool rule_derives_cycle(RuleSet rules, Symbol identifier, cvector(Symbol) *path, SymbolSet *done) {
	cvector(Definition) definitions = rule_set_get(rules, identifier).definitions;

	cvector_push_back(*path, identifier);

	for(Definition *it1 = cvector_begin(definitions); it1 != cvector_end(definitions); it1 += 1) {
		for(Symbol *it2 = cvector_begin(*it1); it2 != cvector_end(*it1); it2 += 1) {
			if(it2->mask & SymbolMask_Terminal) {
				continue;
			}

			// Hoisting appends a repetition to its own alternatives (_r = X _r | ε), that loop is
			// how a repetition is encoded and not a cycle in the user's grammar
			if((it2->mask & SymbolMask_Repetition) && symbol_equals(*it2, identifier)) {
				continue;
			}

			for(Symbol *it3 = cvector_begin(*path); it3 != cvector_end(*path); it3 += 1) {
				if(symbol_equals(*it2, *it3)) {
					return true;
				}
			}

			if(symbol_set_contains(*done, *it2)) {
				continue;
			}

			if(rule_derives_cycle(rules, *it2, path, done)) {
				return true;
			}
		}
	}

	cvector_pop_back(*path);
	symbol_set_insert(done, identifier);

	return false;
}

/* check_raw_rules
 *
 * Checks how lexical (':=') rules are used, working on raw rules so reports keep their position.
 * Text cannot be constructed from syntax, so a lexical rule referencing a syntax rule is an error.
 *
 * PARAMETERS:
 * 	raw_rules: every rule as written
 * 	lexical_rules: filled with the name of every lexical rule
 */
static void check_raw_rules(cvector(RawRule) raw_rules, SymbolSet *lexical_rules) {
	SymbolSet syntax_rules = symbol_set_new();
	SymbolSet referenced = symbol_set_new();

	for(const RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		symbol_set_insert(it1->lexical ? lexical_rules : &syntax_rules, symbol_from_token(it1->identifier));
	}

	if(symbol_set_size(syntax_rules) == 0 && cvector_size(raw_rules) > 0) {
		log_error("grammar has no syntax rules, at least one rule must be defined with '='");
	}

	for(const RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		Symbol identifier = symbol_from_token(it1->identifier);

		for(const Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
			if(it2->tag != TokenType_Identifier) {
				continue;
			}

			Symbol reference = symbol_from_token(*it2);

			// A rule referencing itself does not count as being used
			if(!symbol_equals(reference, identifier)) {
				symbol_set_insert(&referenced, reference);
			}

			if(it1->lexical && symbol_set_contains(syntax_rules, reference)) {
				log_error("%u:%u: lexical rule '%.*s' references syntax rule '%.*s'",
						it2->pos.row, it2->pos.col,
						(int) it1->identifier.lexeme.length, it1->identifier.lexeme.data,
						(int) it2->lexeme.length, it2->lexeme.data);
			}
		}
	}

	for(const RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		if(it1->lexical && !symbol_set_contains(referenced, symbol_from_token(it1->identifier))) {
			log_warn("%u:%u: lexical rule '%.*s' is never referenced",
					it1->identifier.pos.row, it1->identifier.pos.col,
					(int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
		}
	}

	symbol_set_delete(syntax_rules);
	symbol_set_delete(referenced);
}

/* mark_tokens
 *
 * Marks every lexical rule a syntax rule references as a token. Syntax rules reference them as
 * terminals named after the rule, literals keep their quotes so they never name a rule.
 */
static void mark_tokens(Grammar *g) {
	rule_set_iterate(g->rules, it1, {
		if(it1->mask & RuleMask_Lexical) {
			continue;
		}

		for(Definition *it2 = cvector_begin(it1->definitions); it2 != cvector_end(it1->definitions); it2 += 1) {
			for(Symbol *it3 = cvector_begin(*it2); it3 != cvector_end(*it2); it3 += 1) {
				if(it3->mask & SymbolMask_Terminal) {
					rule_set_mark(&g->rules, symbol_new(it3->lexeme, SymbolMask_None), RuleMask_Token);
				}
			}
		}
	});
}
