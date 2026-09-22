#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "verbum/lexer/lexer.h"
#include "verbum/token/token.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/parse/parse.h"
#include "verbum/table/table.h"
#include "verbum/parser/parser.h"


int main() {
	srand(time(NULL));
	Lexer *lexer = lexer_new("test/c2.ebnf");
	cvector(Rule) rules = verbum_parse(lexer);
	Grammar grammar = analyzer_analyze(rules);
	LR0Parser lr0 = lr0_new(grammar);

	printf("@start: %.*s\n", (int) grammar.start->identifier.lexeme.length, grammar.start->identifier.lexeme.data);

	for(LR0State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		printf("s%zu:\n", it1 - cvector_begin(lr0.states));

		for(LR0Item *it2 = cvector_begin(it1->items); it2 != cvector_end(it1->items); it2 += 1) {
			printf("\t%.*s -> ", (int) it2->rule.identifier.lexeme.length, it2->rule.identifier.lexeme.data);

			for(Token *it3 = cvector_begin(it2->rule.definition); it3 != cvector_end(it2->rule.definition); it3 += 1) {
				if((size_t) (it3 - cvector_begin(it2->rule.definition)) == it2->dot_pos) {
					printf("* ");
				}

				printf("%.*s ", (int) it3->lexeme.length, it3->lexeme.data);
			}

			if(it2->dot_pos == cvector_size(it2->rule.definition)) {
				printf("*");
			}

			printf(" {");
			printf(" }");
			printf("\n");
		}
	}

	char *input = "hello world";
	ParseTable table = parse_table_new(cvector_size(lr0.states), cvector_size(grammar.terminals) + 1, cvector_size(grammar.variables));
	Parser p = parser_new(table);

	parser_parse(&p, input);

	lexer_delete(lexer);
	grammar_delete(&grammar);
	parse_table_delete(&table);

	//analyzer_delete(a);

	return 0;
}
