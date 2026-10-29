#ifndef VERBUM_AST_H
#define VERBUM_AST_H

#include "../../lib/c-vector/cvector.h"

#include "../token/token.h"
#include "../raw_rule/raw_rule.h"


typedef struct AstGrammar AstGrammar;
typedef struct AstRule AstRule;
typedef struct AstExpression AstExpression;
typedef struct AstList AstList;
typedef struct AstFactor AstFactor;

typedef enum AstFactorType {
	AstFactorType_Invalid = -1,
	AstFactorType_Identifier,
	AstFactorType_Single_Literal,
	AstFactorType_Double_Literal,
	AstFactorType_Optional,
	AstFactorType_Repetition,
	AstFactorType_Grouping,
	AstFactorType_Lambda,
} AstFactorType;

typedef struct AstFactor {
	union {
		Token identifier;
		Token single_literal;
		Token double_literal;
		AstExpression *optional;
		AstExpression *repetition;
		AstExpression *grouping;
	};
	AstFactorType tag;
} AstFactor;

typedef struct AstList {
	cvector(AstFactor) factors;
} AstList;

typedef struct AstExpression {
	cvector(AstList) lists;
} AstExpression;

typedef struct AstRule {
	Token identifier;
	AstExpression *definition;
} AstRule;

typedef struct AstGrammar {
	cvector(AstRule) rules;
} AstGrammar;


AstFactor ast_new_factor_1(Token t);
AstFactor ast_new_factor_2(Token t);
AstFactor ast_new_factor_3(Token t);
AstFactor ast_new_factor_4(AstExpression e);
AstFactor ast_new_factor_5(AstExpression e);
AstFactor ast_new_factor_6(AstExpression e);
AstFactor ast_new_factor_7(Token t);
AstList ast_new_list(cvector(AstFactor) factors);
AstExpression ast_new_expression(cvector(AstList) lists);
AstRule ast_new_rule(Token identifier, AstExpression definition);
AstGrammar ast_new_grammar(cvector(AstRule) rules);

typedef AstGrammar AST;

AST *ast_new(AstGrammar g1);

/* ast_new_from_raw_rules
 *
 * Builds an AST from rules as written in the source
 *
 * NOTES:
 * 	lexemes are copied, the AST owns them while raw rules borrow from the lexer
 */
AST *ast_new_from_raw_rules(cvector(RawRule) raw_rules);
void ast_delete(AST *ast);
void ast_print(AST *ast);


#endif
