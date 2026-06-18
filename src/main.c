#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "verbum/lexer/lexer.h"
#include "verbum/token//token.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/lalr/lalr.h"
#include "verbum/bnf/bnf.h"


int main() {
	srand(time(NULL));

	Lexer *l = lexer_new("test/ebnf.ebnf");
	Analyzer *a = analyzer_new(l);
	Grammar g = analyzer_analyze(a);

	for(Rule *it1 = cvector_begin(g.original.non_terminal_rules); it1 != cvector_end(g.original.non_terminal_rules); it1 += 1) {
		printf("NONTERMINAL: %.*s\n", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);

	}
	for(Rule *it1 = cvector_begin(g.original.terminal_rules); it1 != cvector_end(g.original.terminal_rules); it1 += 1) {
		printf("TERMINAL: %.*s\n", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);

	}

	//LR0 lr0 = lr0_new(g);

	// bnf_print(g);
	//lalr_construct(g, lr0);
	//lr0_delete(lr0);
	analyzer_delete(a);
	grammar_delete(&g);

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
