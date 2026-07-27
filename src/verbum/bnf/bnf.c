#include <stdio.h>

#include "../rule/rule.h"
#include "../grammar/grammar.h"
#include "../token/token.h"

#include "bnf.h"


void bnf_print(Grammar g) {
	for(Rule *it = cvector_begin(g.rules); it != cvector_end(g.rules); it += 1) {
		if(it->identifier.lexeme.data != NULL) {
			printf("<%.*s> ::=", (int) it->identifier.lexeme.length, it->identifier.lexeme.data);
		}

		if(it->definition == NULL) {
			printf(" λ");
		} else {
			for(Token *it2 = cvector_begin(it->definition); it2 != cvector_end(it->definition); it2 += 1) {
				if(it2->lexeme.data != NULL) {
					printf(" ");

					if(it2->tag == TokenType_Identifier) {
						printf("<");
					}

					printf("%.*s", (int) it2->lexeme.length, it2->lexeme.data);

					if(it2->tag == TokenType_Identifier) {
						printf(">");
					}
				}
			}
		}

		printf("\n");
	}
}
