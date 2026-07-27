#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "verbum/lexer/lexer.h"
#include "verbum/token/token.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/generate/generate.h"
#include "verbum/lalr/lalr.h"
#include "verbum/bnf/bnf.h"
#include "verbum/parse/parse.h"


int main() {
	srand(time(NULL));

	Lexer *l = lexer_new("test/ebnf.ebnf");
	cvector(Rule) rules = verbum_parse(l);

	//Analyzer *a = analyzer_new(l);
	Grammar g = analyzer_analyze(rules);
/*
	for(Rule *it1 = cvector_begin(g.original.non_terminal_rules); it1 != cvector_end(g.original.non_terminal_rules); it1 += 1) {
		printf("NONTERMINAL: %.*s\n", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);

	}
	for(Rule *it1 = cvector_begin(g.original.terminal_rules); it1 != cvector_end(g.original.terminal_rules); it1 += 1) {
		printf("TERMINAL: %.*s\n", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}
*/
	//analyzer_analyze(rules);
	//generate(g);

	LR0 lr0 = lr0_new(g);

	printf("@start: %.*s\n", (int) g.start->identifier.lexeme.length, g.start->identifier.lexeme.data);

	for(State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		printf("s%zu:\n", it1 - cvector_begin(lr0.states));

		for(Item *it2 = cvector_begin(it1->items); it2 != cvector_end(it1->items); it2 += 1) {
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

			for(Token *it3 = cvector_begin(it2->lookaheads); it3 != cvector_end(it2->lookaheads); it3 += 1) {
				printf(" %.*s", (int) it3->lexeme.length, it3->lexeme.data);
			}
			printf(" }");

			printf("\n");
		}
	}

	/* for(Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		printf("δ(s%zu, %.*s) = s%zu\n", it1->source_index, (int) it1->symbol.lexeme.length,
		       				 it1->symbol.lexeme.data, it1->destination_index);
		printf("Direct Reads:");

		for(Token *it2 = cvector_begin(it1->direct_reads); it2 != cvector_end(it1->direct_reads); it2 += 1) {
			printf(" %.*s", (int) it2->lexeme.length, it2->lexeme.data);
		}

		printf("\n");
	} */
	//lalr_construct(g, lr0);

	lexer_delete(l);
	grammar_delete(&g);
	lr0_delete(lr0);
	//LR0 lr0 = lr0_new(g);

	// bnf_print(g);
	//lalr_construct(g, lr0);
	//lr0_delete(lr0);
	//analyzer_delete(a);

	/* Lexer *l = lexer_new("test/ebnf.ebnf");
	Token t = { 0 };

	while(true) {
		t = lexer_get_token(l);

		if(t.tag == TokenType_EOF) {
			break;
		}

		printf("LENGTH: %zu ; DATA: %.*s\n", t.lexeme.length, (int)t.lexeme.length, t.lexeme.data);
	}

	lexer_delete(l); */

	return 0;
}
