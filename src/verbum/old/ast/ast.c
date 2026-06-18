#include <stdio.h>

#include "../../../lib/try.h/try.h"
#include "../../../lib/c-vector/cvector.h"
#include "../../../lib/c-vector/cvector_utils.h"

#include "../../memory/memory.h"

#include "../ast/ast.h"


static void ast_delete_grammar(Grammar g1);
static void ast_delete_rule(Rule r1);
static void ast_delete_expression(Expression e1);
static void ast_delete_list(List l1);
static void ast_delete_factor(Factor f1);


Factor ast_new_factor_identifier(Token t) {
	return (Factor) {
		.identifier = t,
		.tag = FactorType_Identifier,
	};
}

Factor ast_new_factor_single_literal(Token t) {
	return (Factor) {
		.singleliteral = t,
		.tag = FactorType_SingleLiteral,
	};
}

Factor ast_new_factor_double_literal(Token t) {
	return (Factor) {
		.singleliteral = t,
		.tag = FactorType_DoubleLiteral,
	};
}

Factor ast_new_factor_optional(Expression e) {
	return (Factor) {
		.optional = memory_copy(&e, sizeof(e)),
		.tag = FactorType_Optional,
	};
}

Factor ast_new_factor_repetition(Expression e) {
	return (Factor) {
		.repetition = memory_copy(&e, sizeof(e)),
		.tag = FactorType_Repetition,
	};
}

Factor ast_new_factor_grouping(Expression e) {
	return (Factor) {
		.grouping = memory_copy(&e, sizeof(e)),
		.tag = FactorType_Grouping,
	};
}

List ast_new_list(Factor f1, cvector(Factor) f2) {
	return (List) {
		.factor1 = f1,
		.factor2 = f2,
	};
}

Expression ast_new_expression(List l1, cvector(List) l2) {
	return (Expression) {
		.list1 = l1,
		.list2 = l2,
	};
}

Rule ast_new_rule(Token t1, Expression e1) {
	return (Rule) {
		.token1 = t1,
		.expression1 = memory_copy(&e1, sizeof(e1)),
	};
}

Grammar ast_new_grammar(cvector(Rule) r1) {
	return (Grammar) {
		.rule1 = r1,
	};
}

void ast_delete(AST *ast) {
	ast_delete_grammar(*ast);
	memory_delete(ast);

}

AST *ast_new(Grammar g1) {
	AST *result = try(memory_copy(&g1, sizeof(*result)), result == NULL, { return NULL; });

	return result;
}

static void ast_delete_grammar(Grammar g1) {
	cvector_for_each(g1.rule1, ast_delete_rule);
	cvector_free(g1.rule1);
}

static void ast_delete_rule(Rule r1) {
	token_delete(r1.token1);

	if(r1.expression1 != NULL) {
		ast_delete_expression(*(r1.expression1));
	}

	memory_delete(r1.expression1);
}

static void ast_delete_expression(Expression e1) {
	ast_delete_list(e1.list1);

	if(e1.list2 != NULL) {
		cvector_for_each(e1.list2, ast_delete_list);
		cvector_free(e1.list2);
	}
}

static void ast_delete_list(List l1) {
	ast_delete_factor(l1.factor1);

	if(l1.factor2 != NULL) {
		cvector_for_each(l1.factor2, ast_delete_factor);
		cvector_free(l1.factor2);
	}
}

static void ast_delete_factor(Factor f1) {
	switch(f1.tag) {
	case FactorType_Identifier:
		token_delete(f1.identifier);
		break;
	case FactorType_SingleLiteral:
		token_delete(f1.singleliteral);
		break;
	case FactorType_DoubleLiteral:
		token_delete(f1.doubleliteral);
		break;
	case FactorType_Optional:
		ast_delete_expression(*(f1.optional));
		memory_delete(f1.optional);
		break;
	case FactorType_Repetition:
		ast_delete_expression(*(f1.grouping));
		memory_delete(f1.grouping);
		break;
	case FactorType_Grouping:
		ast_delete_expression(*(f1.grouping));
		memory_delete(f1.grouping);
		break;
	default:
		break;
	}
}

static void ast_print_grammar(Grammar g1);
static void ast_print_rule(Rule r1);
static void ast_print_expression(Expression e1);
static void ast_print_list(List l1);
static void ast_print_factor(Factor f1);

void ast_print(AST *ast) {
	ast_print_grammar(*ast);
}

static void ast_print_grammar(Grammar g1) {
	cvector_for_each(g1.rule1, ast_print_rule);
}

static void ast_print_rule(Rule r1) {
	printf("%s", r1.token1.lexeme);
	printf("\t=");
	ast_print_expression(*(r1.expression1));
	printf(";\n");
}

static void ast_print_expression(Expression e1) {
	ast_print_list(e1.list1);

	if(e1.list2 != NULL) {
		for(List *it = cvector_begin(e1.list2); it != cvector_end(e1.list2); it += 1) {
			if((it + 1) != cvector_end(e1.list2)) {
				printf("|");
				ast_print_list(*it);
			}
		}
	}
}

static void ast_print_list(List l1) {
	ast_print_factor(l1.factor1);

	if(l1.factor2 != NULL) {
		for(Factor *it = cvector_begin(l1.factor2); it != cvector_end(l1.factor2); it += 1) {
			printf(",");
			ast_print_factor(*it);
		}
	}
}

static void ast_print_factor(Factor f1) {
	switch(f1.tag) {
	case FactorType_Identifier:
		printf(" %s ", f1.identifier.lexeme);
		break;
	case FactorType_SingleLiteral:
		printf(" %s ", f1.singleliteral.lexeme);
		break;
	case FactorType_DoubleLiteral:
		printf(" %s ", f1.doubleliteral.lexeme);
		break;
	case FactorType_Optional:
		printf(" [ ");
		ast_print_expression(*(f1.optional));
		printf(" ] ");
		break;
	case FactorType_Repetition:
		printf(" { ");
		ast_print_expression(*(f1.grouping));
		printf(" } ");
		break;
	case FactorType_Grouping:
		printf(" ( ");
		ast_print_expression(*(f1.grouping));
		printf(" ) ");
		break;
	default:
		break;
	}
}
