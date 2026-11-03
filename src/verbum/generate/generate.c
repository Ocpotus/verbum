#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../lib/c-vector/cvector.h"

#include "../table/table.h"
#include "../grammar/grammar.h"
//#include "../ast/ast.h"
#include "../rule/rule.h"
#include "../symbol/symbol.h"

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
	} ast_out;
	struct {
		char *buffer;
		size_t len;
		FILE *fp;
	} parser_out;
	FILE *out_file;
} Context;

static void generate_table(Context ctx, Grammar grammar, ParseTable table);
static void generate_ast(Context ctx, Grammar grammar);
static void print_symbol(FILE *fp, Symbol s);
static void print_node(FILE *fp, Rule r);


void verbum_generate(cvector(RawRule) raw_rules, Grammar grammar, ParseTable table) {
	(void) grammar;
	(void) raw_rules;
	Context context = { 0 };

	context.table_out.fp = open_memstream(&context.table_out.buffer, &context.table_out.len);
	context.ast_out.fp = open_memstream(&context.ast_out.buffer, &context.ast_out.len);
	context.parser_out.fp = open_memstream(&context.parser_out.buffer, &context.parser_out.len);
	context.out_file = fopen("verbum.c", "w+");

	if(context.table_out.fp == NULL || context.ast_out.fp == NULL || context.parser_out.fp == NULL) {
		// Error;
		return;
	}

	generate_table(context, grammar, table);
	//generate_parser(context.parser_out.fp, grammar, table);
	generate_ast(context, grammar);

	fclose(context.table_out.fp);
	fclose(context.ast_out.fp);
	fclose(context.parser_out.fp);

	if((context.table_out.len > fwrite(context.table_out.buffer, 1, context.table_out.len, context.out_file) ||
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

	fprintf(fp, "static const ParseTableAction action_table[%zu][%zu] = {\n", (size_t )table.num_states, (size_t) table.num_terminals + 1);

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		// One past the last terminal is the end of input
		for(size_t terminal_index = 0; terminal_index <= table.num_terminals; terminal_index += 1) {
			ParseTableAction action = parse_table_action_at(&table, state_index, terminal_index);

			fprintf(fp, "0x%08X, ", action);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");
	fprintf(fp, "static const ParseTableGoto goto_table[%zu][%zu] = {\n", (size_t )table.num_states, (size_t) table.num_nonterminals);

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
			ParseTableGoto g0t0 = parse_table_goto_at(&table, state_index, nonterminal_index);

			fprintf(fp, "0x%08X, ", g0t0);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");
}

static void generate_ast(Context ctx, Grammar grammar) {
	FILE *fp = ctx.ast_out.fp;

	fprintf(fp, "typedef enum TokenType {\n");

	symbol_set_iterate(grammar.terminals, it1, {
		bool found; // Not used

		fprintf(fp, "\tTokenType_%zu, // ", symbol_set_index(grammar.terminals, *it1, &found));
		print_symbol(fp, *it1);
		fprintf(fp, "\n");
	});

	fprintf(fp, "} TokenType;\n");

	// Lexeme definition
	fprintf(fp, "typedef struct Lexeme {\n");
	fprintf(fp, "\tconst char *data;\n");
	fprintf(fp, "\tsize_t length;\n");
	fprintf(fp, "} Lexeme;\n");
	fprintf(fp, "typedef struct Token {\n");

	// Token definition
	fprintf(fp, "\tLexeme lexeme;\n");
	fprintf(fp, "\tTokenType tag;\n");
	fprintf(fp, "\tstruct {\n");
	fprintf(fp, "\t\tsize_t row;\n");
	fprintf(fp, "\t\tsize_t column;\n");
	fprintf(fp, "\t} position;\n");
	fprintf(fp, "} Token;\n");

	symbol_set_iterate(grammar.nonterminals, it1, {
		if(!(it1->mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group))) {
			print_node(fp, rule_set_get(grammar.rules, *it1));
		}
	});
}

static void print_symbol(FILE *fp, Symbol s) {
	if(s.mask & (SymbolMask_Repetition | SymbolMask_Optional | SymbolMask_Group)) {
		fprintf(fp, "_%.*s%u", (int) s.lexeme.length, s.lexeme.data, s.mask >> SYMBOL_MASK_INDEX_OFFSET);
	} else {
		fprintf(fp, "%.*s", (int) s.lexeme.length, s.lexeme.data);
	}
}

static void print_node(FILE *fp, Rule r) {
	bool alternates = cvector_size(r.definitions) > 1;
	size_t num_tokens = 0;

	fprintf(fp, "typedef struct ");
	print_symbol(fp, r.identifier);
	fprintf(fp, " {\n");

	if(alternates) {
		fprintf(fp, "\tsize_t tag;\n");
		fprintf(fp, "\tunion {\n");
	}

	for(size_t i = 0; i < cvector_size(r.definitions); i += 1) {
		if(alternates) {
			fprintf(fp, "struct {\n");
		}

		for(Symbol *it1 = cvector_begin(r.definitions[i]); it1 != cvector_end(r.definitions[i]); it1 += 1) {
			if(it1->mask & SymbolMask_Repetition) {

			} else if(it1->mask & SymbolMask_Optional) {
				
			} else if(it1->mask & SymbolMask_Group) {
			} else if(it1->mask & SymbolMask_Terminal) {
				fprintf(fp, "Token _t%zu; // ", num_tokens);
				num_tokens += 1;
				print_symbol(fp, *it1);
				fprintf(fp, "\n");
			} else {
				fprintf(fp, "struct ");
				print_symbol(fp, *it1);
				fprintf(fp, "* ");
				print_symbol(fp, *it1);
				fprintf(fp, ";\n");
			}
		}

		if(alternates) {
			fprintf(fp, "} _c%zu;\n", i);
		}
	}

	if(alternates) {
		fprintf(fp, "\t};\n");
	}

	fprintf(fp, "} ");
	print_symbol(fp, r.identifier);
	fprintf(fp, ";\n");

}
