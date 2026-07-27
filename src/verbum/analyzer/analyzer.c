#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../lib/c-vector/cvector.h"

#include "../memory/memory.h"
#include "../token/token.h"
#include "../rule/rule.h"
#include "../grammar/grammar.h"

#include "analyzer.h"


static void token_set_insert(cvector(Token) *set, Token t);
static bool rule_has_any_cycle(cvector(Rule) rules, Rule start_rule);
static bool rule_referenced(cvector(Rule) rules, Rule target);

Grammar analyzer_analyze(cvector(Rule) r) {
	Grammar result = { 
		.original = {
			.terminal_rules = NULL,
			.non_terminal_rules = NULL,
		},
		.rules = r,
		.terminals = NULL,
		.variables = NULL,
		.dynamic_tokens = NULL,
		.start = NULL,
	};
	bool changed = true;

	for(Rule *it1 = cvector_begin(result.rules); it1 != cvector_end(result.rules); it1 += 1) {
		Rule nr = {
			.identifier = it1->identifier,
			.definition = NULL,
		};

		cvector_copy(it1->definition, nr.definition);

		if(rule_has_any_cycle(result.rules, *it1)) {
			cvector_push_back(result.original.non_terminal_rules, nr);
		} else {
			cvector_push_back(result.original.terminal_rules, nr);
		}
	}

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
					token_set_insert(&result.terminals, current_token);
				} else if(current_token.tag == TokenType_Identifier) {
					token_set_insert(&result.variables, current_token);
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

					token_set_insert(&result.variables, new_identifier);
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
						Rule lamdba_rule = {
							.identifier = new_identifier,
							.definition = NULL,
						};

						cvector_push_back(result.rules, lamdba_rule);
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
							token_set_insert(&result.variables, current_rule->definition[k]);
						} else if(current_rule->definition[k].tag == TokenType_Double_Literal ||
						   	  current_rule->definition[k].tag == TokenType_Single_Literal) {
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
							token_set_insert(&result.variables, current_rule->definition[k]);
						} else if(current_rule->definition[k].tag == TokenType_Double_Literal ||
						   	  current_rule->definition[k].tag == TokenType_Single_Literal) {
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

	// Find start symbol
	for(Rule *it1 = cvector_begin(result.rules); it1 != cvector_end(result.rules); it1 += 1) {
		if(!rule_referenced(result.rules, *it1)) {
			result.start = it1;
			break;
		}
	}

	return result;
}

static void token_set_insert(cvector(Token) *set, Token t) {
	for(Token *it = cvector_begin(*set); it != cvector_end(*set); it += 1) {
		if(token_equals(*it, t)) {
			return;
		}
	}

	cvector_push_back(*set, t);
}


/* AI generated to find any cycle in a graph, pretty simple */
// Internal helper function that checks for any cycle in the current path
static bool rule_has_any_cycle_internal(cvector(Rule) rules, Rule *current_rule, cvector(Rule) *visited) {
    // 1. Check if the current rule is already in our active recursion stack
    for(Rule *it1 = cvector_begin(*visited); it1 != cvector_end(*visited); it1 += 1) {
        if(token_equals((it1)->identifier, current_rule->identifier)) {
            return true; // Cycle detected! We've looped back to a rule we are already visiting.
        }
    }

    // 2. Add current rule to the active stack
    cvector_push_back(*visited, *current_rule);

    // 3. Iterate through all tokens in the current rule's definition
    for(Token *it1 = cvector_begin(current_rule->definition); it1 != cvector_end(current_rule->definition); it1 += 1) {
        if(it1->tag == TokenType_Identifier) {
            
            // Find the next rule definition in the list matching the token identifier
            Rule *next_rule = NULL;
            for(Rule *it2 = cvector_begin(rules); it2 != cvector_end(rules); it2 += 1) {
                if(token_equals(it2->identifier, *it1)) {
                    next_rule = it2;
                    break;
                }
            }

            // Recurse into the next rule
            if(next_rule != NULL) {
                if(rule_has_any_cycle_internal(rules, next_rule, visited)) {
                    // Do not pop here; bubble the true up to clean up at the top wrapper
                    return true; 
                }
            }
        }
    }

    // 4. Pop the current rule off the stack as we backtrack (this path is clear)
    cvector_pop_back(*visited);

    return false;
}

// Public wrapper function
static bool rule_has_any_cycle(cvector(Rule) rules, Rule start_rule) {
    cvector(Rule) visited = NULL; 
    bool has_cycle = rule_has_any_cycle_internal(rules, &start_rule, &visited);
    
    cvector_free(visited);

    return has_cycle;
}

static bool rule_referenced(cvector(Rule) rules, Rule target) {
	for(Rule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
			if(it2->tag == TokenType_Identifier) {
				if(token_equals(*it2, target.identifier)) {
					return true;
				}
			}
		}
	}

	return false;
}
