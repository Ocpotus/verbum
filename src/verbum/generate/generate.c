#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../lib/log.c/log.h"
#include "../../lib/c-vector/cvector.h"

#include "../table/table.h"
#include "../grammar/grammar.h"
//#include "../ast/ast.h"
#include "../rule/rule.h"
#include "../symbol/symbol.h"
#include "../definition/definition.h"

#include "generate.h"


typedef struct Context {
	struct {
		char *buffer;
		size_t len;
		FILE *fp;
	} table_out;
	struct {
		char *buffer;
		size_t len;
		FILE *fp;
	} symbol_out;
	struct {
		char *buffer;
		size_t len;
		FILE *fp;
	} ast_out;
	struct {
		char *buffer;
		size_t len;
		FILE *fp;
	} parser_out;
	FILE *out_file;
} Context;

static void generate_table(Context ctx, Grammar grammar, ParseTable table);
static void print_symbol(FILE *fp, Symbol s) __attribute__((unused));
static void print_symbols(Context ctx, Grammar g);
static void generate_ast(Context context, Grammar grammar);
//static void generate_parser(Context ctx, Grammar g, ParseTable table);

void verbum_generate(cvector(RawRule) raw_rules, Grammar grammar, ParseTable table) {
	(void) grammar;
	(void) raw_rules;
	Context context = { 0 };

	context.table_out.fp = open_memstream(&context.table_out.buffer, &context.table_out.len);
	context.symbol_out.fp = open_memstream(&context.symbol_out.buffer, &context.symbol_out.len);
	context.parser_out.fp = open_memstream(&context.parser_out.buffer, &context.parser_out.len);
	context.ast_out.fp = open_memstream(&context.ast_out.buffer, &context.ast_out.len);
	context.out_file = fopen("verbum.c", "w+");

	if(context.table_out.fp == NULL || context.symbol_out.fp == NULL || context.parser_out.fp == NULL) {
		// Error;
		return;
	}

	generate_table(context, grammar, table);
	print_symbols(context, grammar);
	generate_ast(context, grammar);
	//generate_parser(context.parser_out.fp, grammar, table);

	fclose(context.table_out.fp);
	fclose(context.symbol_out.fp);
	fclose(context.parser_out.fp);
	fclose(context.ast_out.fp);

	if((context.table_out.len > fwrite(context.table_out.buffer, 1, context.table_out.len, context.out_file) ||
	   (context.symbol_out.len > fwrite(context.symbol_out.buffer, 1, context.symbol_out.len, context.out_file)) ||
	   (context.ast_out.len > fwrite(context.ast_out.buffer, 1, context.ast_out.len, context.out_file)) ||
	   (context.parser_out.len > fwrite(context.parser_out.buffer, 1, context.parser_out.len, context.out_file)))) {
		// Error
	}

	fclose(context.out_file);
}

static void generate_table(Context ctx, Grammar grammar, ParseTable table) {
	(void) grammar;
	FILE *fp = ctx.table_out.fp;

	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "/*\n");
	fprintf(fp, " * C99 standard guarantees sizeof enum is >= sizeof int\n");
	fprintf(fp, " * First 4 bits will store metadata\n");
	fprintf(fp, " * Last 28 bits will be used store to specify the state or production\n");
	fprintf(fp, " * Allowing for 2^28 # of states to shift to or productions to reduce by. Which is more than sufficient...\n");
	fprintf(fp, " *\n");
	fprintf(fp, " * An all zero action is an error so a zeroed table starts out as all errors\n");
	fprintf(fp, " *\n");
	fprintf(fp, " * Diagram:\n");
	fprintf(fp, " * 	Reduce #33: 0 1 0 0 | 0 0 ... 1 0 0 0 0 1\n");
	fprintf(fp, " * 		    	    | \\_________________/\n");
	fprintf(fp, " *	                    |  \\What production to reduce by\n");
	fprintf(fp, " * 	      Reduce bit set/\n");
	fprintf(fp, " */\n");
	fprintf(fp, "typedef enum ParseTableActionType {\n");
	fprintf(fp, "	ParseTableActionType_Error = 0x0 << 0,\n");
	fprintf(fp, "	ParseTableActionType_Accept = 0x1 << 0,\n");
	fprintf(fp, "	ParseTableActionType_Shift = 0x1 << 1,\n");
	fprintf(fp, "	ParseTableActionType_Reduce = 0x1 << 2,\n");
	fprintf(fp, "} ParseTableActionType;\n");
	fprintf(fp, "typedef uint32_t ParseTableAction;\n");
	fprintf(fp, "static const uint32_t ParseTableActionTypeShift = 28;\n");
	fprintf(fp, "static const uint32_t ParseTableActionValueMask = (UINT32_C(1) << 28) - 1;\n");
	fprintf(fp, "static const uint32_t ActionError = ParseTableActionType_Error;\n");
	fprintf(fp, "/* GotoError\n");
	fprintf(fp, " *\n");
	fprintf(fp, " * Marks a goto table entry with no transition\n");
	fprintf(fp, " */\n");
	fprintf(fp, "static const uint32_t GotoError = UINT32_MAX;\n");
	fprintf(fp, "typedef uint32_t ParseTableGoto;\n");

	fprintf(fp, "static const ParseTableAction action_table[%zu * %zu] = {\n", (size_t )table.num_states, (size_t) table.num_terminals + 1);

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		// One past the last terminal is the end of input
		for(size_t terminal_index = 0; terminal_index <= table.num_terminals; terminal_index += 1) {
			ParseTableAction action = parse_table_action_at(&table, state_index, terminal_index);

			fprintf(fp, "0x%08X, ", action);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");
	fprintf(fp, "static const ParseTableGoto goto_table[%zu * %zu] = {\n", (size_t )table.num_states, (size_t) table.num_nonterminals);

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
			ParseTableGoto g0t0 = parse_table_goto_at(&table, state_index, nonterminal_index);

			fprintf(fp, "0x%08X, ", g0t0);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");
}

static void generate_ast(Context context, Grammar grammar) {
	fprintf(context.ast_out.fp, "typedef enum TokenType {\n");

	rule_set_iterate(grammar.rules, it1, {
		if(it1->mask & RuleMask_Token) {
			fprintf(context.ast_out.fp, "\tTokenType_");
			print_symbol(context.ast_out.fp, it1->identifier);
			fprintf(context.ast_out.fp, ",\n");
		}
	});

	fprintf(context.ast_out.fp, "} TokenType;\n");

	fprintf(context.ast_out.fp, "typedef struct Lexeme {\n");
	fprintf(context.ast_out.fp, "\tconst char *data;\n");
	fprintf(context.ast_out.fp, "\tsize_t length;\n");
	fprintf(context.ast_out.fp, "} Lexeme;\n");
	fprintf(context.ast_out.fp, "typedef struct Token {\n");
	fprintf(context.ast_out.fp, "\tLexeme lexeme;\n");
	fprintf(context.ast_out.fp, "\tTokenType tag;\n");
	fprintf(context.ast_out.fp, "\tstruct {\n");
	fprintf(context.ast_out.fp, "\t\tuint32_t row;\n");
	fprintf(context.ast_out.fp, "\t\tuint32_t col;\n");
	fprintf(context.ast_out.fp, "\t} pos;\n");
	fprintf(context.ast_out.fp, "} Token;\n");

	fprintf(context.ast_out.fp, "typedef enum ASTType {\n");

	rule_set_iterate(grammar.rules, it1, {
		if(!symbol_is_hoisted(it1->identifier) && !(it1->mask & RuleMask_Lexical) && !symbol_equals(it1->identifier, grammar.start->identifier)) {
			fprintf(context.ast_out.fp, "\tASTType_");
			print_symbol(context.ast_out.fp, it1->identifier);
			fprintf(context.ast_out.fp, ",\n");
		}
	});

	fprintf(context.ast_out.fp, "\tASTType_Token,\n");
	fprintf(context.ast_out.fp, "} ASTType;\n");

	fprintf(context.ast_out.fp, "typedef uint32_t TokenIndex;\n");
	fprintf(context.ast_out.fp, "typedef uint32_t ASTIndex;\n");

	fprintf(context.ast_out.fp, "typedef struct ASTNode {\n");
	fprintf(context.ast_out.fp, "\tunion {\n");

	rule_set_iterate(grammar.rules, it1, {
		bool alternates = false;
		size_t alternates_count = 0;

		if(cvector_size(it1->definitions) > 1) {
			alternates = true;
		}
		
		if(!(it1->mask & RuleMask_Lexical) && !symbol_equals(it1->identifier, grammar.start->identifier)) {
			fprintf(context.ast_out.fp, "\t\t%s {\n", alternates ? "union" : "struct");

			for(Definition *it2 = cvector_begin(it1->definitions); it2 != cvector_end(it1->definitions); it2 += 1) {
				size_t expression = 0;

				if(alternates) {
					fprintf(context.ast_out.fp, "\t\t\tstruct {\n");
				}

				for(Symbol *it3 = cvector_begin(*it2); it3 != cvector_end(*it2); it3 += 1) {
					if(alternates) {
						fprintf(context.ast_out.fp, "\t");
					}

					fprintf(context.ast_out.fp, "\t\t\t");

					if(it3->mask & SymbolMask_Terminal) {
						fprintf(context.ast_out.fp, "TokenIndex _%zu;", expression);
					} else {
						fprintf(context.ast_out.fp, "ASTIndex _%zu;", expression);
					}

					fprintf(context.ast_out.fp, " /* ");
					print_symbol(context.ast_out.fp, *it3);
					fprintf(context.ast_out.fp, " */\n");
					expression += 1;
				}

				fprintf(context.ast_out.fp, "\t\t");

				if(alternates) {
					fprintf(context.ast_out.fp, "\t");
					fprintf(context.ast_out.fp, "} _c%zu;\n", alternates_count);
					alternates_count += 1;
				}
			}

			if(alternates) {
				fprintf(context.ast_out.fp, "\t\t");
			}

			fprintf(context.ast_out.fp, "} ");
			print_symbol(context.ast_out.fp, it1->identifier);
			fprintf(context.ast_out.fp, ";\n");
		}
	});

	fprintf(context.ast_out.fp, "\t\tToken token;\n");
	fprintf(context.ast_out.fp, "\t};\n");
	fprintf(context.ast_out.fp, "\tASTType tag;\n");
	fprintf(context.ast_out.fp, "} ASTNode;\n");

	fprintf(context.ast_out.fp, "typedef struct AST {\n");
	fprintf(context.ast_out.fp, "\tchar *scratch;\n");
	fprintf(context.ast_out.fp, "\tToken* tokens;\n");
	fprintf(context.ast_out.fp, "\tASTNode* nodes;\n");
	fprintf(context.ast_out.fp, "} AST;\n");
}

static void print_symbol(FILE *fp, Symbol s) {
	if(s.mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
		fprintf(fp, "_%.*s%u", (int) s.lexeme.length, s.lexeme.data, s.mask >> SYMBOL_MASK_INDEX_OFFSET);
	} else {
		fprintf(fp, "%.*s", (int) s.lexeme.length, s.lexeme.data);
	}
}

/* print_escaped
 *
 * Prints bytes so they are valid inside a C string literal
 */
static void print_escaped(FILE *fp, const char *data, size_t length) {
	for(size_t i = 0; i < length; i += 1) {
		char c = data[i];

		if(c == '"' || c == '\\') {
			fprintf(fp, "\\%c", c);
		} else if(c == '\n') {
			fprintf(fp, "\\n");
		} else if(c == '\t') {
			fprintf(fp, "\\t");
		} else {
			fputc(c, fp);
		}
	}
}

/* print_symbol_string
 *
 * Prints a symbol as a C string literal
 */
static void print_symbol_string(FILE *fp, Symbol s) {
	fputc('"', fp);

	if(symbol_is_hoisted(s)) {
		fputc('_', fp);
		print_escaped(fp, s.lexeme.data, s.lexeme.length);
		fprintf(fp, "%u", s.mask >> SYMBOL_MASK_INDEX_OFFSET);
	} else {
		print_escaped(fp, s.lexeme.data, s.lexeme.length);
	}

	fputc('"', fp);
}

/* print_symbols
 *
 * Emits a table of symbol names laid out as:
 * 	[0, T)          terminals, same order as the action table columns
 * 	T               end of input
 * 	[T + 1, T + 1 + N) nonterminals, same order as the goto table columns
 */
static void print_symbols(Context ctx, Grammar g) {
	FILE *fp = ctx.symbol_out.fp;
	size_t num_terminals = symbol_set_size(g.terminals);
	size_t num_nonterminals = symbol_set_size(g.nonterminals);
	size_t num_symbols = num_terminals + 1 + num_nonterminals;
	cvector(Symbol) store = NULL;

	cvector_resize(store, num_symbols, (Symbol) { 0 });

	symbol_set_iterate(g.terminals, it1, {
		bool unused;
		store[symbol_set_index(g.terminals, *it1, &unused)] = *it1;
	});

	symbol_set_iterate(g.nonterminals, it1, {
		bool unused;
		store[num_terminals + 1 + symbol_set_index(g.nonterminals, *it1, &unused)] = *it1;
	});

	fprintf(fp, "static const char *const symbols[%zu] = {\n", num_symbols);

	for(size_t i = 0; i < num_symbols; i += 1) {
		fprintf(fp, "\t");

		if(i == num_terminals) {
			fprintf(fp, "\"$end\"");
		} else {
			print_symbol_string(fp, store[i]);
		}

		fprintf(fp, ",\n");
	}

	fprintf(fp, "};\n");

	cvector_free(store);
}
