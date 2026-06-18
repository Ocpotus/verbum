#ifndef VERBUM_AST_C
#define VERBUM_AST_C

#include "../../../lib/optional.h/optional.h"
#include "../../../lib/c-vector/cvector.h"

#include "../../token/token.h"


typedef struct Grammar Grammar;
typedef struct Rule Rule;
typedef struct Expression Expression;
typedef struct List List;
typedef struct Factor Factor;

typedef enum FactorType {
	FactorType_Invalid = -1,
	FactorType_Identifier,
	FactorType_SingleLiteral,
	FactorType_DoubleLiteral,
	FactorType_Optional,
	FactorType_Repetition,
	FactorType_Grouping,
} FactorType;

typedef struct Factor {
	union {
		Token identifier;
		Token singleliteral;
		Token doubleliteral;
		Expression *optional;
		Expression *repetition;
		Expression *grouping;
	};
	FactorType tag;
} Factor;

typedef struct List {
	Factor factor1;
	cvector(Factor) factor2;
} List;

typedef struct Expression {
	List list1;
	cvector(List) list2;
} Expression;

typedef struct Rule {
	Token token1;
	Expression *expression1;
} Rule;

typedef struct Grammar {
	cvector(Rule) rule1;
} Grammar;


Factor ast_new_factor_identifier(Token t);
Factor ast_new_factor_single_literal(Token t);
Factor ast_new_factor_double_literal(Token t);
Factor ast_new_factor_optional(Expression e);
Factor ast_new_factor_repetition(Expression e);
Factor ast_new_factor_grouping(Expression e);
List ast_new_list(Factor t1, cvector(Factor) t2);
Expression ast_new_expression(List l1, cvector(List) l2);
Rule ast_new_rule(Token t1, Expression e1);
Grammar ast_new_grammar(cvector(Rule) r1);

typedef Grammar AST;

AST *ast_new(Grammar g1);
void ast_delete(AST *ast);
void ast_print(AST *ast);


#endif
