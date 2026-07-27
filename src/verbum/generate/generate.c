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

/*
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
typedef const char *Literal;

typedef struct Node {
	NodeType tag;
	union {
		struct {
			// ENTRY
			NodeReference rule; // ELEMENT
			NodeReference grammar_repetition; // ELEMENT
		} grammar; // MEMBER
		struct {
			// ENTRY
			Identifier identifier; // ELEMENT
			NodeReference expression; // ELEMENT
		} rule; // MEMBER
		struct {
			// ENTRY
			NodeReference list; // ELEMENT
			NodeReference expression_repetition; // ELEMENT
		} expression; // MEMBER
		struct {
			// ENTRY
			NodeReference factor; // ELEMENT
			NodeReference list_repetition; // ELEMENT
		} list; // MEMBER
		struct {
			union {
				// ENTRY
				struct {
					Identifier identifier; // ELEMENT
				} o1;
				// ENTRY
				struct {
					SingleLiteral single_literal; // ELEMENT
				} o2;
				// ENTRY
				struct {
					DoubleLiteral double_literal; // ELEMENT
				} o3;
				// ENTRY
				struct {
					Literal lit1; // ELEMENT
					NodeReference expression; // ELEMENT
					Literal lit2; // ELEMENT
				} o4;
				// ENTRY
				struct {
					Literal lit1; // ELEMENT
					NodeReference expression; // ELEMENT
					Literal lit2; // ELEMENT
				} o5;
				// ENTRY
				struct {
					Literal lit1; // ELEMENT
					NodeReference expression; // ELEMENT
					Literal lit2; // ELEMENT
				} o6;
			};
			uint8_t o;
		} factor; // MEMBER
	};
} Node; // NODE

typedef cvector(Node) AST;

Node *ast_get(AST ast, NodeReference nr) {
	return &ast[nr];
}

Node *ast_add(AST ast, Node n) {
	cvector_push_back(ast, n);

	return cvector_back(ast);
}
*/

typedef enum ElementType {
	ElementType_Terminal,
	ElementType_NonTerminal,
	ElementType_Literal,
} ElementType;

typedef struct Element {
	ElementType tag;
	Token identifier;
} Element;

typedef struct Entry {
	cvector(Element) elements;
} Entry;

typedef struct Member {
	Token identifier;
	cvector(Entry) entries;
} Member;

typedef struct Node {
	cvector(Member) members;
} Node;

static bool token_set_contains(cvector(Token) set, Token t);
static Node construct_node(Grammar g);

void generate(Grammar g) {
	Node node = construct_node(g);
	char terminal_name[32] = { 0 };

	printf("typedef enum NodeType {");

	for(Rule *it1 = cvector_begin(g.original.non_terminal_rules); it1 != cvector_end(g.original.non_terminal_rules); it1 += 1) {
		printf(" NodeType_%.*s,", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}

	printf(" } NodeType;\n");


	snprintf(terminal_name, sizeof(terminal_name) / sizeof(*terminal_name),
		 "%s%u", "Terminal", (unsigned int) rand());
	printf("typedef struct %s { const char *data; size_t length; } %s;\n", terminal_name, terminal_name);
	printf("typedef size_t ASTNodeReference;\n");

	for(Rule *it1 = cvector_begin(g.original.terminal_rules); it1 != cvector_end(g.original.terminal_rules); it1 += 1) {
		printf("typedef %s %.*s;\n", terminal_name, (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}

	printf("typedef struct ASTNode {\n");
	printf("\tNodeType tag;\n");
	printf("\tunion {\n");

	for(Member *it1 = cvector_begin(node.members); it1 != cvector_end(node.members); it1 += 1) {
		printf("\t\tstruct {\n");

		if(cvector_size(it1->entries) > 1) {
			printf("\t\t\tunion {\n");

			for(Entry *it2 = cvector_begin(it1->entries); it2 != cvector_end(it1->entries); it2 += 1) {
				printf("\t\t\t\tstruct {\n");

				for(Element *it3 = cvector_begin(it2->elements); it3 != cvector_end(it2->elements); it3 += 1) {
					printf("\t\t\t\t\t");

					if(it3->tag == ElementType_NonTerminal) {
						printf("NodeReference e%zu;\n", it3 - cvector_begin(it2->elements));
					} else if(it3->tag == ElementType_Terminal) {
						printf("%.*s e%zu;\n", (int) it3->identifier.lexeme.length, it3->identifier.lexeme.data, it3 - cvector_begin(it2->elements));
					} else {
						printf("%s e%zu;\n", terminal_name, it3 - cvector_begin(it2->elements));
					}
				}

				printf("\t\t\t\t} o%zu;\n", it2 - cvector_begin(it1->entries));
			}

			printf("\t\t\t};\n");
			printf("\t\t\tuint8_t o;\n");
		} else {
			for(Element *it2 = cvector_begin(it1->entries[0].elements); it2 != cvector_end(it1->entries[0].elements); it2 += 1) {
				printf("\t\t\t\t");

				if(it2->tag == ElementType_NonTerminal) {
					printf("NodeReference e%zu;\n", it2 - cvector_begin(it1->entries[0].elements));
				} else if(it2->tag == ElementType_Terminal) {
					printf("%.*s e%zu;\n", (int) it2->identifier.lexeme.length, it2->identifier.lexeme.data, it2 - cvector_begin(it1->entries[0].elements));
				} else {
					printf("%s e%zu;\n", terminal_name, it2 - cvector_begin(it1->entries[0].elements));
				}
			}

		}

		printf("\t\t} %.*s;\n", (int) it1->identifier.lexeme.length, it1->identifier.lexeme.data);
	}

	printf("\t};\n");
	printf("};\n");
}
/*
static bool token_set_contains(cvector(Token) set, Token t) {
	for(Token *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(*it1, t)) {
			return true;
		}
	}

	return false;
}
*/
static Node construct_node(Grammar g) {
	Node result = {
		.members = NULL,
	};

	// WOW!!!! Algorithm!!!!
	for(Rule *it1 = cvector_begin(g.original.non_terminal_rules); it1 != cvector_end(g.original.non_terminal_rules); it1 += 1) {
		Member new_member = { 0 };
		new_member = (Member) {
			.identifier = it1->identifier,
			.entries = NULL,
		};

		for(Rule *it2 = cvector_begin(g.rules); it2 != cvector_end(g.rules); it2 += 1) {
			if(token_equals(it2->identifier, new_member.identifier)) {
				Entry new_entry = {
					.elements = NULL,
				};

				for(Token *it3 = cvector_begin(it2->definition); it3 != cvector_end(it2->definition); it3 += 1) {
					Element new_element = {
						.identifier = *it3,
						.tag = ElementType_NonTerminal,
					};

					if(it3->tag == TokenType_Identifier) {
						for(Rule *it4 = cvector_begin(g.original.terminal_rules); it4 != cvector_end(g.original.non_terminal_rules); it4 += 1) {
							if(token_equals(it4->identifier, *it3)) {
								new_element.tag = ElementType_Terminal;
								break;
							}
						}
					} else if(it3->tag == TokenType_Single_Literal || it3->tag == TokenType_Double_Literal) {
						new_element.tag = ElementType_Literal;
					}

					cvector_push_back(new_entry.elements, new_element);
				}

				cvector_push_back(new_member.entries, new_entry);
			}
		}

		cvector_push_back(result.members, new_member);
	}

	return result;
}
