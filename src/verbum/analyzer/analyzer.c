#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../token/token.h"
#include "../token_set/token_set.h"
//#include "../rule/rule.h"
#include "../grammar/grammar.h"
#include "../rule_set/rule_set.h"

#include "analyzer.h"


static bool rule_referenced(RuleSet rules, RuleSetEntry target);

static Definition hoist(Grammar *g, Definition d, size_t *position, Token sub_end_token);
static cvector(Definition) split(RuleSetEntry *rule);

typedef enum SubExpressionType {
	SubExpressionType_Grouping,
	SubExpressionType_Repetition,
	SubExpressionType_Optional,
} SubExpressionType;

static Token create_sub_expression_identifier(TokenType tag);

Grammar analyzer_analyze(RuleSet raw_rules) {
	Grammar result = grammar_new();
	result.rules = rule_set_new();

	rule_set_iterate(raw_rules, rule, 
		for(Definition *it1 = cvector_begin(rule->definitions); it1 != cvector_end(rule->definitions); it1 += 1) {
			size_t start_position = 0;
			Definition new_definition = hoist(&result, *it1, &start_position, (Token) { .lexeme = it1[0]->lexeme, .tag = it1[0]->tag + 1, });

			if(new_definition != NULL) {
				if(!rule_set_insert(&result.rules, rule->identifier, new_definition, RuleSetEntryMask_None)) {
					// Error
				}
			}
		}
	);

	rule_set_iterate(result.rules, rule,
		RuleSetEntry *rse = (RuleSetEntry *)rule;
		rse->definitions = split((RuleSetEntry *)rule);
	);

	rule_set_iterate(result.rules, rule, {
		if(!rule_referenced(result.rules, *rule)) {
			result.start = rule;
			token_set_insert(&result.nonterminals, rule->identifier);
			break;
		}
	});

	return result;
}

static Definition hoist(Grammar *g, Definition d, size_t *position, Token sub_end_token) {
	Definition result = definition_new();

	while(*position < cvector_size(d)) {
		Token current_token = d[*position];

		if(current_token.tag == TokenType_LeftBrace ||
		   current_token.tag == TokenType_LeftBracket ||
		   current_token.tag == TokenType_LeftParenthesis) {
			// Get sub expression offset by 1
			*position += 1;
			Definition sub_definition = hoist(g, d, position, (Token) { .tag = current_token.tag + 1 });
			Token sub_identifier = create_sub_expression_identifier(current_token.tag);

			switch(current_token.tag) {
			case TokenType_LeftBrace:
				if(!rule_set_insert(&g->rules, sub_identifier, sub_definition, RuleSetEntryMask_Repetition | RuleSetEntryMask_Dynamic)) {
					// Error
				}
				break;
			case TokenType_LeftBracket:
				if(!rule_set_insert(&g->rules, sub_identifier, sub_definition, RuleSetEntryMask_Optional | RuleSetEntryMask_Optional)) {
					// Error
				}
				break;
			case TokenType_LeftParenthesis:
				rule_set_insert(&g->rules, sub_identifier, sub_definition, RuleSetEntryMask_Dynamic);
				break;
			default:
				break;
			}

			definition_push_back(&result, sub_identifier);
			token_set_insert(&g->nonterminals, sub_identifier);
			token_set_insert(&g->dynamic_tokens, sub_identifier);
		} else if(current_token.tag == TokenType_Single_Literal || current_token.tag == TokenType_Double_Literal) {
			definition_push_back(&result, current_token);
			token_set_insert(&g->terminals, current_token);
			*position += 1;
		} else if(current_token.tag == TokenType_Identifier) {
			definition_push_back(&result, current_token);
			token_set_insert(&g->nonterminals, current_token);
			*position += 1;
		} else if(current_token.tag == TokenType_RightBrace ||
			  current_token.tag == TokenType_RightBracket ||
			  current_token.tag == TokenType_RightParenthesis) {
			if(current_token.tag != sub_end_token.tag) {
				// Error
				// mismatch
			}

			*position += 1;

			return result;
		} else if(current_token.tag == TokenType_Pipe) {
			definition_push_back(&result, current_token);
			*position += 1;
		}
	}

	return result;
}

static cvector(Definition) split(RuleSetEntry *rule) {
	cvector(Definition) result = NULL;
	//bool repetition = strncmp("@r", rule->identifier.lexeme.data, 2) == 0;
	Definition current = definition_new();

	for(Token *it1 = cvector_begin(rule->definitions[0]); it1 != cvector_end(rule->definitions[0]); it1 += 1) {
		if(it1->tag == TokenType_Pipe) {
			if(rule->identifier.lexeme.data[0] == '@' && rule->identifier.lexeme.data[1] == 'r') {
				definition_push_back(&current, rule->identifier);
			}

			cvector_push_back(result, current);
			current = definition_new();

			continue;
		}

		definition_push_back(&current, *it1);
	}

	if(rule->mask & RuleSetEntryMask_Repetition) {
		definition_push_back(&current, rule->identifier);
	}

	cvector_push_back(result, current);

	if((rule->mask & RuleSetEntryMask_Repetition) || (rule->mask & RuleSetEntryMask_Optional)) {
		current = definition_new();
		definition_push_back(&current, (Token) { .tag = TokenType_Invalid, });
		cvector_push_back(result, current);
	}

	definition_delete(rule->definitions[0]);
	cvector_free(rule->definitions);
	rule->definitions = result;

	return result;
}

static Token create_sub_expression_identifier(TokenType tag) {
	char name[17] = { 0 };
	char prefix = '\0';

	switch(tag) {
	case TokenType_LeftParenthesis:
		prefix = 'g';
		break;
	case TokenType_LeftBrace:
		prefix = 'r';
		break;
	case TokenType_LeftBracket:
		prefix = 'o';
		break;
	default:
		// Error
		break;
	}

	snprintf(name, sizeof(name) /  sizeof(*name), "@%c%u", prefix, (unsigned int) rand());

	return (Token) {
		.lexeme = (Lexeme) {
			.data = memory_copy(name, sizeof(name) / sizeof(*name)),
			.length = strlen(name),
		},
		.tag = TokenType_Identifier,
	};
}

static bool rule_referenced(RuleSet rules, RuleSetEntry target) {
	rule_set_iterate(rules, it1, {
		for(Definition *it2 = cvector_begin(it1->definitions); it2 != cvector_end(it1->definitions); it2 += 1) {
			for(Token *it3 = cvector_begin(*it2); it3 != cvector_end(*it2); it3 += 1) {
				if(token_equals(*it3, target.identifier)) {
					return true;
				}

			}
		}
	});

	return false;
}

/* static bool rule_referenced(cvector(Rule) rules, Rule target) {
	for(Rule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
			if(it2->tag == TokenType_Identifier) {
				if(token_equals(*it2, target.identifier)) { return true;
				}
			}
		}
	}

	return false;
} */
/*
	// Hoist sub expressions into their own rules
	while(changed) {
		changed = false;

		for(size_t i = 0; i < cvector_size(result.rules); i += 1) {
			Rule current_rule = result.rules[i];
			size_t depth = 0;
			TokenType end_of_sub_type = TokenType_Invalid;
			size_t sub_start = 0;

			for(size_t j = 0; j < cvector_size(current_rule.definition); j += 1) {
				Token current_token = current_rule.definition[j];

				if(current_token.tag == TokenType_LeftBrace ||
				   current_token.tag == TokenType_LeftBracket ||
				   current_token.tag == TokenType_LeftParenthesis) {
					if(depth == 0) {
						end_of_sub_type = current_token.tag + 1;
						sub_start = j;
					}

					depth += 1;
				} else if(current_token.tag == TokenType_Single_Literal || current_token.tag == TokenType_Double_Literal) {
					printf("%d\n", __LINE__);
					token_set_insert(&result.terminals, current_token);
				} else if(current_token.tag == TokenType_Identifier) {
					printf("%d\n", __LINE__);
					token_set_insert(&result.nonterminals, current_token);
				} else if(current_token.tag == TokenType_RightBrace ||
				   	  current_token.tag == TokenType_RightBracket ||
					  current_token.tag == TokenType_RightParenthesis) {
					// Depth is already zero extra ] } )
					if(depth == 0) {
						// Error
					}

					depth -= 1;

					// Not end of sub expression, keep going until reached
					if(depth != 0) {
						continue;
					}

					// End of sub expression (depth == 0), but marker does not match start
					if(current_token.tag != end_of_sub_type) {
						// Error
					}

					char sub_expression_name[17] = { 0 };
					char prefix = (current_token.tag == TokenType_RightBrace) ? 'r' :
						      ((current_token.tag == TokenType_RightBracket) ? 'o' : 'g');

					// Set new rule name
					snprintf(sub_expression_name, sizeof(sub_expression_name) / sizeof(*sub_expression_name),
						 "@%c%u", prefix, (unsigned int) rand());

					Token new_identifier = {
						.lexeme = (Lexeme) {
							.data = memory_copy(sub_expression_name, sizeof(sub_expression_name) / sizeof(*sub_expression_name)),
							.length = strlen(sub_expression_name),
						},
						.tag = TokenType_Identifier,
					};

					printf("%d\n", __LINE__);
					token_set_insert(&result.nonterminals, new_identifier);
					printf("%d\n", __LINE__);
					token_set_insert(&result.dynamic_tokens, new_identifier);

					Rule new_rule = {
						.identifier = new_identifier,
						.definition = NULL,
					};

					size_t sub_end = j;

					for(size_t k = sub_start + 1; k < sub_end; k += 1) {
						if(current_rule.definition[k].tag == TokenType_Pipe && end_of_sub_type == TokenType_RightBrace) {
							cvector_push_back(new_rule.definition, new_rule.identifier);
						}

						cvector_push_back(new_rule.definition, current_rule.definition[k]);
					}

					if(end_of_sub_type == TokenType_RightBrace) {
						cvector_push_back(new_rule.definition, new_rule.identifier);
					}

					cvector_push_back(result.rules, new_rule);
					changed = true;

					if(end_of_sub_type == TokenType_RightBrace || end_of_sub_type == TokenType_RightBracket) {
						Rule lambda_rule = {
							.identifier = new_identifier,
							.definition = NULL,
						};

						cvector_push_back(result.rules, lambda_rule);
					}

					for(size_t k = 0; k < (sub_end - sub_start) + 1; k += 1) {
						cvector_erase(current_rule.definition, sub_start);
					}

					cvector_insert(current_rule.definition, sub_start, new_rule.identifier);
				}
			}
		}
	}

	changed = true;

	// Split rules by '|'
	while(changed) {
		changed = false;

		for(size_t i = 0; i < cvector_size(result.rules); i++) {
			Rule *current_rule = &result.rules[i]; 

			for(size_t j = 0; j < cvector_size(current_rule->definition); j++) {
				Token *current_token = &current_rule->definition[j];

				if(current_token->tag == TokenType_Pipe) {
					size_t pipe_pos = j;
					Rule first_rule = { 0 };
					Rule second_rule = { 0 };

					first_rule.identifier = (Token) {
						.lexeme = current_rule->identifier.lexeme,
						.tag = current_rule->identifier.tag,
					};

					for(size_t k = 0; k < pipe_pos; k += 1) {
						cvector_push_back(first_rule.definition, current_rule->definition[k]);

						if(current_rule->definition[k].tag == TokenType_Identifier) {
					printf("%d\n", __LINE__);
							token_set_insert(&result.nonterminals, current_rule->definition[k]);
						} else if(current_rule->definition[k].tag == TokenType_Double_Literal ||
						   	  current_rule->definition[k].tag == TokenType_Single_Literal) {
					printf("%d\n", __LINE__);
							token_set_insert(&result.terminals, current_rule->definition[k]);
						}
					}

					second_rule.identifier = (Token) {
						.lexeme = current_rule->identifier.lexeme,
						.tag = current_rule->identifier.tag,
					};

					for(size_t k = pipe_pos + 1; k < cvector_size(current_rule->definition); k += 1) {
						cvector_push_back(second_rule.definition, current_rule->definition[k]);

						if(current_rule->definition[k].tag == TokenType_Identifier) {
					printf("%d\n", __LINE__);
							token_set_insert(&result.nonterminals, current_rule->definition[k]);
						} else if(current_rule->definition[k].tag == TokenType_Double_Literal ||
						   	  current_rule->definition[k].tag == TokenType_Single_Literal) {
					printf("%d\n", __LINE__);
							token_set_insert(&result.terminals, current_rule->definition[k]);
						}
					}

					cvector_push_back(result.rules, first_rule);
					cvector_push_back(result.rules, second_rule);

					Rule *r = cvector_at(result.rules, i);

					if(r != NULL) {
						cvector_free(r->definition);
					}

					cvector_erase(result.rules, i);
					changed = true;

					break;
				}
			}

			if(changed) {
				break;
			}
		}
	}
*/

	// Find start symbol
	/* for(Rule *it1 = cvector_begin(result.rules); it1 != cvector_end(result.rules); it1 += 1) {
		if(!rule_referenced(result.rules, *it1)) {
			result.start = it1;
			break;
		}
	} */
