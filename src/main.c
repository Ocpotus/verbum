#include <stdio.h>
#include <stdlib.h>

#include "lib/c-vector/cvector.h"

#include "verbum/lexer/lexer.h"
#include "verbum/symbol/symbol.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/parse/parse.h"
#include "verbum/rule_set/rule_set.h"
#include "verbum/definition/definition.h"
#include "verbum/raw_rule/raw_rule.h"


int main() {
	Lexer *lexer = lexer_new("test/ebnf.ebnf");
	cvector(RawRule) rules = verbum_parse(lexer);
	Grammar grammar = analyzer_analyze(rules);

	rule_set_iterate(grammar.rules, rule, 
		for(Definition *it1 = cvector_begin(rule->definitions); it1 != cvector_end(rule->definitions); it1 += 1) {
			Definition definition = *it1;
			uint32_t mask = rule->identifier.mask;

			printf("%.*s", (int) rule->identifier.lexeme.length, rule->identifier.lexeme.data);

			if(mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
				printf("%d", rule->identifier.mask >> 4);
			}

			printf(" -> ");

			if(cvector_size(definition) != 0) {
				for(Symbol *it2 = cvector_begin(definition); it2 != cvector_end(definition); it2 += 1) {
					uint32_t mask = it2->mask;

					printf("%.*s", (int) it2->lexeme.length, it2->lexeme.data);

					if(mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
						printf("%d", rule->identifier.mask >> 4);
					}

					printf(" ");
				}
			} else {
				printf(" λ");
			}

			printf("\n");
		}
	);

	LR0Parser lr0 = lr0_new(grammar);

	printf("@start: %.*s\n", (int) grammar.start->identifier.lexeme.length, grammar.start->identifier.lexeme.data);

	for(LR0State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		printf("s%zu:\n", it1 - cvector_begin(lr0.states));

		for(LR0Item *it2 = cvector_begin(it1->items); it2 != cvector_end(it1->items); it2 += 1) {
			uint32_t mask = it2->rule.identifier.mask;

			printf("\t%.*s", (int) it2->rule.identifier.lexeme.length, it2->rule.identifier.lexeme.data);

			if(mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
				printf("%d", it2->rule.identifier.mask >> 4);
			}

			printf(" -> ");

			for(Symbol *it3 = cvector_begin(it2->rule.definitions[it2->rule_index]); it3 != cvector_end(it2->rule.definitions[it2->rule_index]); it3 += 1) {
				if((size_t) (it3 - cvector_begin(it2->rule.definitions[it2->rule_index])) == it2->dot_pos) {
					printf("* ");
				}

				uint32_t mask = it3->mask;

				printf("%.*s", (int) it3->lexeme.length, it3->lexeme.data);

				if(mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
					printf("%d", it3->mask >> 4);
				}

				printf(" ");
			}

			if(it2->dot_pos == cvector_size(it2->rule.definitions[it2->rule_index])) {
				printf("*");
			}

			printf(" {");
			printf(" }");
			printf("\n");
		}
	}

	lr0_delete(lr0);
	grammar_delete(&grammar);

	for(RawRule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		raw_rule_delete(it1);
	}

	cvector_free(rules);
	lexer_delete(lexer);

	return 0;
}
