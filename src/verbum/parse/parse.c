#include "../../lib/c-vector/cvector.h"

#include "../lexer/lexer.h"
//#include "../rule/rule.h"

#include "parse.h"


RuleSet verbum_parse(Lexer *l) {
	RuleSet result = rule_set_new();

	while(true) {
		Token current = lexer_get_token(l);

		if(current.tag == TokenType_EOF) {
			break;
		}

		if(current.tag == TokenType_Identifier) {
			Token identifier = current;
			Definition definition = definition_new();

			if(lexer_get_token(l).tag != TokenType_Equal) {
				// Error
			}

			do {
				current = lexer_get_token(l);

				if(current.tag == TokenType_Semicolon) {
					break;
				} else if(current.tag != TokenType_Comma) {
					cvector_push_back(definition, current);
				}
			} while(current.tag != TokenType_EOF);

			rule_set_insert(&result, identifier, definition, false);
		} else {
			// Error
		}
	}

	return result;
}
