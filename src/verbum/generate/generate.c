#include <stdio.h>
#include <stddef.h>

#include "../../lib/c-vector/cvector.h"

#include "../grammar/grammar.h"

#include "generate.h"


/* For ast code generation it will follow this format
 *
 * Create a NodeType enum for each type of node
 *
 * Then a tagged union
 *
 * Prerequisites:
 * 	1. Must have a list of terminal rules (ebnf rules that when following the path* will not return back to it)
 * 		* the path is following the definition, this does not mean it is not in a definition
 * 		* example: a -> b -> ... -> d -> a (nonterminal) ; a -> b -> ... -> z (terminal)
 * 	2. Must have a list of nonterminal rules
 *
 *
 *
 */


typedef enum NodeType {
	NodeType_Grammar,
	NodeType_Rule,
	NodeType_Expression,
	NodeType_List,
	NodeType_Factor,
} NodeType;

typedef size_t NodeReference;
typedef const char *Identifier;
typedef const char *SingleLiteral;
typedef const char *DoubleLiteral;

typedef struct Node {
	NodeType tag;
	union {
		struct {
			NodeReference rule;
			NodeReference grammar_repetition;
		} grammar;
		struct {
			Identifier identifier;
			NodeReference expression;
		} rule;
		struct {
			NodeReference list;
			NodeReference expression_repetition;
		} expression;
		struct {
			NodeReference factor;
			NodeReference list_repetition;
		} list;
		struct {
			union {
				Identifier identifier;
				SingleLiteral single_literal;
				DoubleLiteral double_literal;
				NodeReference expression;
			};
		} factor;
	};
} Node;

typedef cvector(Node) AST;

Node *ast_get(AST ast, NodeReference nr) {
	return &ast[nr];
}

Node *ast_add(AST ast, Node n) {
	cvector_push_back(ast, n);
	return cvector_back(ast);
}

void generate(Grammar g) {
	printf("typedef enum NodeType {");

	for(Rule *it1 = cvector_begin(g.original.non_terminal_rules); it1 != cvector_end(g.original.non_terminal_rules); it1 += 1) {
		printf(" NodeType_%.*s,", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}

	printf(" } NodeType;\n");

	char terminal_name[32] = { 0 };
	snprintf(terminal_name, sizeof(terminal_name) / sizeof(*terminal_name),
		 "%s%u", "Terminal", (unsigned int) rand());
	printf("typedef struct %s { const char *data; size_t length; } %s\n", terminal_name, terminal_name);
	printf("typedef size_t ASTNodeReference;\n");

	for(Rule *it1 = cvector_begin(g.original.terminal_rules); it1 != cvector_end(g.original.terminal_rules); it1 += 1) {
		printf("typedef %s %.*s;\n", terminal_name, (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}

	printf("typedef struct ASTNode {\n");
	printf("\tNodeType tag;\n");
	printf("\tunion {\n");

	for(Rule *it1 = cvector_begin(g.original.rules); it1 != cvector_end(g.original.rules); it1 += 1) {
		printf("\t\tstruct{\n");

		for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
			printf("\t\t\t");

			if(it2->tag == TokenType_Identifier) {
				printf("ASTNodeReference ")

			}
		}

		printf("\t\t} %.*s;\n", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}

	printf("\t};\n");
}
