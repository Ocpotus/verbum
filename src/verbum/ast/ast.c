#include <stdio.h>

#include "../../lib/try.h/try.h"
#include "../../lib/c-vector/cvector.h"
#include "../../lib/c-vector/cvector_utils.h"

#include "../memory/memory.h"

#include "ast.h"


static void ast_delete_grammar(AstGrammar g1);
static void ast_delete_rule(AstRule r1);
static void ast_delete_expression(AstExpression e1);
static void ast_delete_list(AstList l1);
static void ast_delete_factor(AstFactor f1);
static Token ast_own_token(Token t);
static AstExpression ast_parse_expression(cvector(Token) tokens, size_t *position);
static AstFactor ast_parse_factor(cvector(Token) tokens, size_t *position);


AstFactor ast_new_factor_1(Token t) {
	return (AstFactor) {
		.identifier = t,
		.tag = AstFactorType_Identifier,
	};
}

AstFactor ast_new_factor_2(Token t) {
	return (AstFactor) {
		.single_literal = t,
		.tag = AstFactorType_Single_Literal,
	};
}

AstFactor ast_new_factor_3(Token t) {
	return (AstFactor) {
		.double_literal = t,
		.tag = AstFactorType_Double_Literal,
	};
}

AstFactor ast_new_factor_4(AstExpression e) {
	return (AstFactor) {
		.optional = memory_copy(&e, sizeof(e)),
		.tag = AstFactorType_Optional,
	};
}

AstFactor ast_new_factor_5(AstExpression e) {
	return (AstFactor) {
		.repetition = memory_copy(&e, sizeof(e)),
		.tag = AstFactorType_Repetition,
	};
}

AstFactor ast_new_factor_6(AstExpression e) {
	return (AstFactor) {
		.grouping = memory_copy(&e, sizeof(e)),
		.tag = AstFactorType_Grouping,
	};
}

AstFactor ast_new_factor_7(Token t) {
	return (AstFactor) {
		.identifier = t,
		.tag = AstFactorType_Lambda,
	};
}


AstList ast_new_list(cvector(AstFactor) factors) {
	return (AstList) {
		.factors = factors,
	};
}

AstExpression ast_new_expression(cvector(AstList) lists) {
	return (AstExpression) {
		.lists = lists,
	};
}

AstRule ast_new_rule(Token identifier, AstExpression definition) {
	return (AstRule) {
		.identifier = identifier,
		.definition = memory_copy(&definition, sizeof(definition)),
	};
}

AstGrammar ast_new_grammar(cvector(AstRule) rules) {
	return (AstGrammar) {
		.rules = rules,
	};
}

AST *ast_new(AstGrammar g1) {
	AST *result = try(memory_copy(&g1, sizeof(*result)), result == NULL, { return NULL; });

	return result;
}

AST *ast_new_from_raw_rules(cvector(RawRule) raw_rules) {
	cvector(AstRule) rules = NULL;

	for(RawRule *it1 = cvector_begin(raw_rules); it1 != cvector_end(raw_rules); it1 += 1) {
		size_t position = 0;
		AstExpression definition = ast_parse_expression(it1->definition, &position);

		cvector_push_back(rules, ast_new_rule(ast_own_token(it1->identifier), definition));
	}

	return ast_new(ast_new_grammar(rules));
}

static Token ast_own_token(Token t) {
	Token result = token_copy(t);

	result.lexeme.data = t.lexeme.length == 0 ? NULL : memory_copy((void *) t.lexeme.data, t.lexeme.length);

	return result;
}

/* ast_parse_expression
 *
 * PARAMETERS:
 * 	tokens: a raw rule's definition, commas are already excluded so adjacent factors are concatenated
 * 	position: current position in tokens, left after the closing token
 *
 * RETURNS:
 * 	a list for each alternative separated by '|'
 */
static AstExpression ast_parse_expression(cvector(Token) tokens, size_t *position) {
	cvector(AstList) lists = NULL;
	cvector(AstFactor) factors = NULL;

	while(*position < cvector_size(tokens)) {
		TokenType tag = tokens[*position].tag;

		if(tag == TokenType_RightParenthesis || tag == TokenType_RightBracket || tag == TokenType_RightBrace) {
			*position += 1;
			break;
		}

		if(tag == TokenType_Pipe) {
			cvector_push_back(lists, ast_new_list(factors));
			factors = NULL;
			*position += 1;
			continue;
		}

		if(tag != TokenType_Identifier && tag != TokenType_Single_Literal && tag != TokenType_Double_Literal &&
		   tag != TokenType_LeftParenthesis && tag != TokenType_LeftBracket && tag != TokenType_LeftBrace) {
			*position += 1;
			continue;
		}

		cvector_push_back(factors, ast_parse_factor(tokens, position));
	}

	cvector_push_back(lists, ast_new_list(factors));

	return ast_new_expression(lists);
}

static AstFactor ast_parse_factor(cvector(Token) tokens, size_t *position) {
	Token t = tokens[*position];

	*position += 1;

	switch(t.tag) {
	case TokenType_Identifier:
		return ast_new_factor_1(ast_own_token(t));
	case TokenType_Single_Literal:
		return ast_new_factor_2(ast_own_token(t));
	case TokenType_Double_Literal:
		return ast_new_factor_3(ast_own_token(t));
	case TokenType_LeftBracket:
		return ast_new_factor_4(ast_parse_expression(tokens, position));
	case TokenType_LeftBrace:
		return ast_new_factor_5(ast_parse_expression(tokens, position));
	case TokenType_LeftParenthesis:
		return ast_new_factor_6(ast_parse_expression(tokens, position));
	default:
		return (AstFactor) { .tag = AstFactorType_Invalid };
	}
}

void ast_delete(AST *ast) {
	if(ast == NULL) {
		return;
	}

	ast_delete_grammar(*ast);
	memory_delete(ast);
}

static void ast_delete_grammar(AstGrammar g1) {
	cvector_for_each(g1.rules, ast_delete_rule);
	cvector_free(g1.rules);
}

static void ast_delete_rule(AstRule r1) {
	token_delete(r1.identifier);

	if(r1.definition != NULL) {
		ast_delete_expression(*(r1.definition));
	}

	memory_delete(r1.definition);
}

static void ast_delete_expression(AstExpression e1) {
	cvector_for_each(e1.lists, ast_delete_list);
	cvector_free(e1.lists);
}

static void ast_delete_list(AstList l1) {
	cvector_for_each(l1.factors, ast_delete_factor);
	cvector_free(l1.factors);
}

static void ast_delete_factor(AstFactor f1) {
	switch(f1.tag) {
	case AstFactorType_Lambda:
		token_delete(f1.identifier);
		break;
	case AstFactorType_Identifier:
		token_delete(f1.identifier);
		break;
	case AstFactorType_Single_Literal:
		token_delete(f1.single_literal);
		break;
	case AstFactorType_Double_Literal:
		token_delete(f1.double_literal);
		break;
	case AstFactorType_Optional:
		if(f1.optional != NULL) {
			ast_delete_expression(*(f1.optional));
		}

		memory_delete(f1.optional);
		break;
	case AstFactorType_Repetition:
		if(f1.repetition != NULL) {
			ast_delete_expression(*(f1.repetition));
		}

		memory_delete(f1.repetition);
		break;
	case AstFactorType_Grouping:
		if(f1.grouping != NULL) {
			ast_delete_expression(*(f1.grouping));
		}

		memory_delete(f1.grouping);
		break;
	default:
		break;
	}
}

static void ast_print_grammar(AstGrammar g1);
static void ast_print_rule(AstRule r1);
static void ast_print_expression(AstExpression e1);
static void ast_print_list(AstList l1);
static void ast_print_factor(AstFactor f1);

void ast_print(AST *ast) {
	ast_print_grammar(*ast);
}

static void ast_print_grammar(AstGrammar g1) {
	cvector_for_each(g1.rules, ast_print_rule);
}

static void ast_print_rule(AstRule r1) {
	printf("%.*s", (int) r1.identifier.lexeme.length, r1.identifier.lexeme.data);
	printf("\t=");
	ast_print_expression(*(r1.definition));
	printf(";\n");
}

static void ast_print_expression(AstExpression e1) {
	for(AstList *it = cvector_begin(e1.lists); it != cvector_end(e1.lists); it += 1) {
		if(it != cvector_begin(e1.lists)) {
			printf("|");
		}

		ast_print_list(*it);
	}
}

static void ast_print_list(AstList l1) {
	for(AstFactor *it = cvector_begin(l1.factors); it != cvector_end(l1.factors); it += 1) {
		if(it != cvector_begin(l1.factors)) {
			printf(",");
		}

		ast_print_factor(*it);
	}
}

static void ast_print_factor(AstFactor f1) {
	switch(f1.tag) {
	case AstFactorType_Identifier:
		printf(" %.*s ", (int) f1.identifier.lexeme.length, f1.identifier.lexeme.data);
		break;
	case AstFactorType_Single_Literal:
		printf(" '%.*s' ", (int) f1.single_literal.lexeme.length, f1.single_literal.lexeme.data);
		break;
	case AstFactorType_Double_Literal:
		printf(" \"%.*s\" ", (int) f1.double_literal.lexeme.length, f1.double_literal.lexeme.data);
		break;
	case AstFactorType_Optional:
		printf(" [ ");
		ast_print_expression(*(f1.optional));
		printf(" ] ");
		break;
	case AstFactorType_Repetition:
		printf(" { ");
		ast_print_expression(*(f1.repetition));
		printf(" } ");
		break;
	case AstFactorType_Grouping:
		printf(" ( ");
		ast_print_expression(*(f1.grouping));
		printf(" ) ");
		break;
	case AstFactorType_Lambda:
		printf(" /\\ ");
		break;
	default:
		break;
	}
}
