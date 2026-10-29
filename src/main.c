#include <stdio.h>
#include <stdlib.h>

#include "lib/log.c/log.h"
#include "lib/c-vector/cvector.h"

#include "verbum/ast/ast.h"
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
#include "verbum/table/table.h"
#include "verbum/generate/generate.h"


/* table_column_name
 *
 * Formats a symbol as a table header, sub expressions share a lexeme (r, o, g) so their Nth index is appended
 *
 * RETURNS:
 * 	the display width of the name, UTF-8 continuation bytes take no space
 */
static int table_column_name(Symbol s, char *out, size_t size) {
	int length = snprintf(out, size, "%.*s", (int) s.lexeme.length, s.lexeme.data);
	int width = 0;

	if(s.mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
		snprintf(out + length, size - length, "%u", (unsigned) (s.mask >> 4));
	}

	for(char *c = out; *c != '\0'; c += 1) {
		width += ((unsigned char) *c & 0xC0) != 0x80;
	}

	return width;
}

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

	/* for(LR1State *it1 = cvector_begin(lane_table.states); it1 != cvector_end(lane_table.states); it1 += 1) {
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

		for(LR1Transition *it2 = cvector_begin(lane_table.transitions); it2 != cvector_end(lane_table.transitions); it2 += 1) {
			if(it2->source == *it1) {
				printf("s%zu--", it2->source);
				printf("%.*s", (int) it2->symbol.lexeme.length, it2->symbol.lexeme.data);

				if(it2->symbol.mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
					printf("%d", it2->symbol.mask >> 4);
				}

				printf("-->s%zu\n", it2->destination);
			}
		}
	} */

	ParseTable table = parse_table_new(grammar, lane_table);

	// Columns are symbol_set_index ordered, the set iterates in hash order
	cvector(Symbol) nonterminals = NULL;

	cvector_resize(nonterminals, symbol_set_size(grammar.nonterminals), ((Symbol) { 0 }));

	symbol_set_iterate(grammar.nonterminals, it1, {
		bool found = true;
		size_t index = symbol_set_index(grammar.nonterminals, *it1, &found);

		nonterminals[index] = *it1;
	});

	// Every column is as wide as the longest name so long identifiers don't push the rest out of line
	int width = 10;
	char name[64];

	for(size_t terminal_index = 0; terminal_index < table.num_terminals; terminal_index += 1) {
		int name_width = table_column_name(lane_table.terminals[terminal_index], name, sizeof(name));

		width = (name_width > width) ? name_width : width;
	}

	for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
		int name_width = table_column_name(nonterminals[nonterminal_index], name, sizeof(name));

		width = (name_width > width) ? name_width : width;
	}

	printf("%*s", width, "");

	for(size_t terminal_index = 0; terminal_index < table.num_terminals; terminal_index += 1) {
		int name_width = table_column_name(lane_table.terminals[terminal_index], name, sizeof(name));

		// printf pads by bytes, pad by display width so multibyte names (π) line up
		printf("|%*s%s", width - name_width, "", name);
	}

	printf("|%*s", width, "$");

	for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
		int name_width = table_column_name(nonterminals[nonterminal_index], name, sizeof(name));

		printf("|%*s%s", width - name_width, "", name);
	}

	printf("|\n");

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		printf("s%-*zu", width - 1, state_index);

		// One past the last terminal is the end of input
		for(size_t terminal_index = 0; terminal_index <= table.num_terminals; terminal_index += 1) {
			ParseTableAction action = parse_table_action_at(&table, state_index, terminal_index);
			char cell[16];

			switch(parse_table_action_type(action)) {
				case ParseTableActionType_Accept:
					snprintf(cell, sizeof(cell), "accept");
					break;
				case ParseTableActionType_Shift:
					snprintf(cell, sizeof(cell), "s%u", parse_table_action_value(action));
					break;
				case ParseTableActionType_Reduce:
					snprintf(cell, sizeof(cell), "r%u", parse_table_action_value(action));
					break;
				default:
					snprintf(cell, sizeof(cell), "error");
					break;
			}

			printf("|%*s", width, cell);
		}

		for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
			ParseTableGoto g0t0 = parse_table_goto_at(&table, state_index, nonterminal_index);
			char cell[16];

			if(g0t0 == ParseTableGotoError) {
				snprintf(cell, sizeof(cell), "error");
			} else {
				snprintf(cell, sizeof(cell), "s%u", g0t0);
			}

			printf("|%*s", width, cell);
		}

		printf("|\n");
	}

	printf("%u * %u * %zu = %zu bytes", table.num_nonterminals + table.num_states, table.num_states, sizeof(*table.action_table),
			((size_t) (table.num_nonterminals + table.num_states)) * ((size_t) table.num_states) * ((size_t) sizeof(*table.action_table)));

	verbum_generate(rules, grammar, table);

	cvector_free(nonterminals);
	parse_table_delete(&table);
	cvector_free(conflicts);
	lr1_delete(lane_table);
	lr0_delete(lr0);
	grammar_delete(&grammar);


	AST *ast = ast_new_from_raw_rules(rules);

	ast_print(ast);

	/* for(RawRule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		printf("%.*s = ", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
		for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
			printf("%.*s, ", (int) it2->lexeme.length, it2->lexeme.data);
		}
		printf(";\n");

		raw_rule_delete(it1);
	} */

	cvector_free(rules);
	lexer_delete(lexer);

	return 0;
}
