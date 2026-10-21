#include "../../lib/c-vector/cvector.h"

#include "../lexer/lexer.h"
#include "../token/token.h"
#include "../raw_rule/raw_rule.h"

#include "parse.h"


cvector(RawRule) verbum_parse(Lexer *l) {
	cvector(RawRule) result = NULL;

	while(true) {
		Token current = lexer_get_token(l);

		if(current.tag == TokenType_EOF) {
			break;
		}

		if(current.tag == TokenType_Identifier) {
			RawRule rule = {
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
					cvector_push_back(rule.definition, current);
				}
			} while(current.tag != TokenType_EOF);

			cvector_push_back(result, rule);
		} else {
			// Error
		}
	}

	return result;
}

