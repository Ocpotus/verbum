#include <stdio.h>
#include <stdlib.h>

#include "lib/log.c/log.h"
#include "lib/c-vector/cvector.h"

#include "verbum/lexer/lexer.h"
#include "verbum/symbol/symbol.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/parse/parse.h"
#include "verbum/rule_set/rule_set.h"
#include "verbum/definition/definition.h"
#include "verbum/raw_rule/raw_rule.h"
#include "verbum/bnf/bnf.h"


int main() {
#ifdef FORGE_RELEASE
	log_set_quiet(true);
#endif
	Lexer *lexer = lexer_new("test/ebnf.ebnf");
	cvector(RawRule) rules = verbum_parse(lexer);
	Grammar grammar = analyzer_analyze(rules);
	bnf_print(grammar);

	/* rule_set_iterate(grammar.rules, rule, 
		for(Definition *it1 = cvector_begin(rule->definitions); it1 != cvector_end(rule->definitions); it1 += 1) {
			Definition definition = *it1;
			uint32_t mask = rule->identifier.mask;

			printf("%.*s", (int) rule->identifier.lexeme.length, rule->identifier.lexeme.data);

			if(mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
				printf("%d", rule->identifier.mask >> 4);
			}

			printf(" -> ");

			if(!cvector_empty(definition)) {
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

	LR0Automaton lr0 = lr0_new(grammar);

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

			if(!cvector_empty(it2->rule.definitions[it2->rule_index])) {
				for(size_t i = 0; i < cvector_size(it2->rule.definitions[it2->rule_index]); i += 1) {
					Symbol s = it2->rule.definitions[it2->rule_index][i];

					if(i == it2->dot_pos) {
						printf("* ");
					}

					uint32_t mask = s.mask;

					printf("%.*s", (int) s.lexeme.length, s.lexeme.data);

					if(mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
						printf("%d", s.mask >> 4);
					}

					printf(" ");
				}
			} else {
				printf("λ ");
			}

			if(it2->dot_pos == cvector_size(it2->rule.definitions[it2->rule_index])) {
				printf("* ");
			}

			printf("{");
			printf(" }");
			printf(" %zu", it2->dot_pos);
			printf("\n");
		}
	} */

	/* lr0_delete(lr0);
	grammar_delete(&grammar);
	*/

	for(RawRule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		raw_rule_delete(it1);
	}

	cvector_free(rules);
	lexer_delete(lexer);

	return 0;
}
