#include <stdio.h>
#include <stdlib.h>

#include "lib/log.c/log.h"
#include "lib/c-vector/cvector.h"

#include "verbum/lexer/lexer.h"
#include "verbum/symbol/symbol.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/lr1/lr1.h"
#include "verbum/lane_table/lane_table.h"
#include "verbum/parse/parse.h"
#include "verbum/rule_set/rule_set.h"
#include "verbum/definition/definition.h"
#include "verbum/raw_rule/raw_rule.h"
#include "verbum/bnf/bnf.h"


int main(int argc, char **argv) {
#ifdef FORGE_RELEASE
	log_set_quiet(true);
#endif
	Lexer *lexer = lexer_new(argc > 1 ? argv[1] : "test/ebnf.ebnf");
	cvector(RawRule) rules = verbum_parse(lexer);
	Grammar grammar = analyzer_analyze(rules);
	//bnf_print(grammar);

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
	); */

	LR0Automaton lr0 = lr0_new(grammar);
	cvector(size_t) conflicts = NULL;
	LR1Automaton lane_table = lane_table_new(grammar, lr0, &conflicts);

	printf("@start: %.*s\n", (int) grammar.start->identifier.lexeme.length, grammar.start->identifier.lexeme.data);

	for(LR1State *it1 = cvector_begin(lane_table.states); it1 != cvector_end(lane_table.states); it1 += 1) {
		printf("s%zu:\n", it1 - cvector_begin(lane_table.states));

		for(LR1Item *lane_item = cvector_begin(it1->items); lane_item != cvector_end(it1->items); lane_item += 1) {
			LR0Item *it2 = &lane_item->lr0;
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

			if(lane_item->lookahead == NULL) {
				printf(" _");
			} else {
				for(size_t i = 0; i <= cvector_size(lane_table.terminals); i += 1) {
					if(!lr1_lookahead_contains(lane_item->lookahead, i)) {
						continue;
					}

					if(i == cvector_size(lane_table.terminals)) {
						printf(" $");
					} else {
						printf(" %.*s", (int) lane_table.terminals[i].lexeme.length, lane_table.terminals[i].lexeme.data);
					}
				}
			}

			printf(" }");
			printf(" %zu", it2->dot_pos);
			printf("\n");
		}
	}

	for(LR1Transition *it1 = cvector_begin(lane_table.transitions); it1 != cvector_end(lane_table.transitions); it1 += 1) {
		printf("s%zu--", it1->source);
		printf("%.*s", (int) it1->symbol.lexeme.length, it1->symbol.lexeme.data);

		if(it1->symbol.mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
			printf("%d", it1->symbol.mask >> 4);
		}

		printf("-->s%zu\n", it1->destination);
	}

	for(size_t *it1 = cvector_begin(conflicts); it1 != cvector_end(conflicts); it1 += 1) {
		printf("conflict: s%zu\n", *it1);
	}

	cvector_free(conflicts);
	lr1_delete(lane_table);
	lr0_delete(lr0);
	grammar_delete(&grammar);


	for(RawRule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		raw_rule_delete(it1);
	}

	cvector_free(rules);
	lexer_delete(lexer);

	return 0;
}
