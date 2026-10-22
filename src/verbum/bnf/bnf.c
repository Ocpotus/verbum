#include <stdio.h>

#include "../../lib/c-vector/cvector.h"

#include "../symbol/symbol.h"
#include "../definition/definition.h"
#include "../rule/rule.h"
#include "../rule_set/rule_set.h"
#include "../grammar/grammar.h"

#include "bnf.h"


void bnf_print(Grammar g) {
	rule_set_iterate(g.rules, rule, {
		if(rule->identifier.lexeme.data != NULL) {
			printf("<%.*s", (int) rule->identifier.lexeme.length, rule->identifier.lexeme.data);

			if(rule->identifier.mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
				printf("%d", rule->identifier.mask >> 4);
			}

			printf("> ::=");
		}

		if(cvector_size(rule->definitions) == 0) {
			printf(" λ");
		}

		for(Definition *it1 = cvector_begin(rule->definitions); it1 != cvector_end(rule->definitions); it1 += 1) {
			if(it1 != cvector_begin(rule->definitions)) {
				printf(" |");
			}

			if(cvector_size(*it1) == 0) {
				printf(" λ");
				continue;
			}

			for(Symbol *it2 = cvector_begin(*it1); it2 != cvector_end(*it1); it2 += 1) {
				bool terminal = (it2->mask & SymbolMask_Terminal) != 0;

				if(it2->lexeme.data == NULL) {
					return;
				}

				printf(" ");

				if(!terminal) {
					printf("<");
				}

				printf("%.*s", (int) it2->lexeme.length, it2->lexeme.data);

				if(it2->mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
					printf("%d", it2->mask >> 4);
				}

				if(!terminal) {
					printf(">");
				}
			}
		}

		printf("\n");
	});
}
