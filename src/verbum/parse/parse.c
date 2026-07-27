#include "../../lib/c-vector/cvector.h"

#include "../lexer/lexer.h"
#include "../rule/rule.h"

#include "parse.h"


cvector(Rule) verbum_parse(Lexer *l) {
	cvector(Rule) result = NULL;

	// Split by ';' and remove ','
	// TOOD: Make this actually check for valid , placement. It just filters them out
	while(true) {
		Token current = lexer_get_token(l);

		if(current.tag == TokenType_EOF) {
			break;
		}

		if(current.tag == TokenType_Identifier) {
			Rule new = { 
				.identifier = current,
				.definition = NULL,
			};

			if(lexer_get_token(l).tag != TokenType_Equal) {
				// Error
			}

			do {
				current = lexer_get_token(l);

				if(current.tag == TokenType_Semicolon) {
					break;
				} else if(current.tag != TokenType_Comma) {
					cvector_push_back(new.definition, current);
				}

			} while(current.tag != TokenType_EOF);

			//token_set_insert(&result.variables, new.identifier);
			cvector_push_back(result, new);
		} else {
			// Error
		}
	}

	return result;
}
