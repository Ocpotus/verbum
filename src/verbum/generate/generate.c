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
#include "../dfa/dfa.h"

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
	struct {
		char *buffer;
		size_t len;
		FILE *fp;
	} lexer_out;
	FILE *out_file;
} Context;

static void generate_table(Context ctx, Grammar grammar, ParseTable table);
static void print_symbol(FILE *fp, Symbol s) __attribute__((unused));
static void print_symbols(Context ctx, Grammar g);
static void generate_token_types(FILE *fp, Grammar g);
static void generate_ast(Context context, Grammar grammar);
static void generate_lexer(Context context, DFATable table);
//static void generate_parser(Context ctx, Grammar g, ParseTable table);

void verbum_generate(cvector(RawRule) raw_rules, Grammar grammar, ParseTable table) {
	(void) grammar;
	(void) raw_rules;
	Context context = { 0 };

	context.table_out.fp = open_memstream(&context.table_out.buffer, &context.table_out.len);
	context.symbol_out.fp = open_memstream(&context.symbol_out.buffer, &context.symbol_out.len);
	context.parser_out.fp = open_memstream(&context.parser_out.buffer, &context.parser_out.len);
	context.lexer_out.fp = open_memstream(&context.lexer_out.buffer, &context.lexer_out.len);
	context.ast_out.fp = open_memstream(&context.ast_out.buffer, &context.ast_out.len);
	context.out_file = fopen("verbum.c", "w+");

	if(context.table_out.fp == NULL || context.symbol_out.fp == NULL || context.parser_out.fp == NULL ||
	   context.lexer_out.fp == NULL || context.ast_out.fp == NULL) {
		// Error;
		return;
	}

	generate_table(context, grammar, table);
	print_symbols(context, grammar);
	generate_ast(context, grammar);

	DFA dfa;

	if(dfa_build(grammar, &dfa)) {
		DFATable lexer_table;

		dfa_table_build(dfa, &lexer_table);
		generate_lexer(context, lexer_table);
		dfa_table_delete(&lexer_table);
	} else {
		log_error("lexer not generated, the grammar's lexical rules are invalid");
	}

	dfa_delete(&dfa);
	//generate_parser(context.parser_out.fp, grammar, table);

	fclose(context.table_out.fp);
	fclose(context.symbol_out.fp);
	fclose(context.parser_out.fp);
	fclose(context.lexer_out.fp);
	fclose(context.ast_out.fp);

	if((context.table_out.len > fwrite(context.table_out.buffer, 1, context.table_out.len, context.out_file) ||
	   (context.symbol_out.len > fwrite(context.symbol_out.buffer, 1, context.symbol_out.len, context.out_file)) ||
	   (context.ast_out.len > fwrite(context.ast_out.buffer, 1, context.ast_out.len, context.out_file)) ||
	   (context.lexer_out.len > fwrite(context.lexer_out.buffer, 1, context.lexer_out.len, context.out_file)) ||
	   (context.parser_out.len > fwrite(context.parser_out.buffer, 1, context.parser_out.len, context.out_file)))) {
		// Error
	}

	fclose(context.out_file);
	free(context.table_out.buffer);
	free(context.symbol_out.buffer);
	free(context.ast_out.buffer);
	free(context.lexer_out.buffer);
	free(context.parser_out.buffer);
}

static void generate_table(Context ctx, Grammar grammar, ParseTable table) {
	(void) grammar;
	FILE *fp = ctx.table_out.fp;

	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "#include \"utf8.h\"\n");
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

/* generate_token_types
 *
 * Emits the TokenType enum. A token's value is its terminal's column in the action table, so a
 * lexed tag indexes the table directly. The layout matches print_symbols:
 * 	[0, T)  terminals, ordered by symbol_set_index(g.terminals)
 * 	T       end of input
 * 	T + 1   invalid text, not a column, the lexer's error token
 *
 * Terminals named by a lexical rule keep that name (TokenType_identifier). Literals are not valid
 * C identifiers, so they are named by index (TokenType_Literal3) with the literal in a comment.
 */
static void generate_token_types(FILE *fp, Grammar g) {
	size_t num_terminals = symbol_set_size(g.terminals);
	cvector(Symbol) store = NULL;

	cvector_resize(store, num_terminals, (Symbol) { 0 });

	symbol_set_iterate(g.terminals, it1, {
		bool unused;
		store[symbol_set_index(g.terminals, *it1, &unused)] = *it1;
	});

	fprintf(fp, "typedef enum TokenType {\n");

	for(size_t i = 0; i < num_terminals; i += 1) {
		Symbol s = store[i];
		bool literal = s.lexeme.length > 0 && (s.lexeme.data[0] == '"' || s.lexeme.data[0] == '\'');

		if(literal) {
			fprintf(fp, "\tTokenType_Literal%zu = %zu, /* ", i, i);
			// Keep "*/" in a literal from closing the comment early
			for(size_t j = 0; j < s.lexeme.length; j += 1) {
				fputc(s.lexeme.data[j], fp);

				if(s.lexeme.data[j] == '*' && j + 1 < s.lexeme.length && s.lexeme.data[j + 1] == '/') {
					fputc(' ', fp);
				}
			}
			fprintf(fp, " */\n");
		} else {
			fprintf(fp, "\tTokenType_");
			print_symbol(fp, s);
			fprintf(fp, " = %zu,\n", i);
		}
	}

	fprintf(fp, "\tTokenType_EOF = %zu,\n", num_terminals);
	// Not an action table column, the lexer returns it for text that starts no token
	fprintf(fp, "\tTokenType_Invalid = %zu,\n", num_terminals + 1);
	fprintf(fp, "} TokenType;\n");

	cvector_free(store);
}

/* lexer_declarations
 *
 * Emitted before the lexer tables, everything the tables and the driver need declared
 */
static const char lexer_declarations[] =
	"#define LEXER_DEAD UINT32_C(0)\n"
	"#define LEXER_START UINT32_C(1)\n"
	"#define LEXER_NONE UINT32_MAX\n"
	"/* LexerRange\n"
	" *\n"
	" * Consecutive codepoints sharing a column of lexer_transitions, a codepoint outside every range is class 0\n"
	" */\n"
	"typedef struct LexerRange {\n"
	"\tutf8_int32_t first;\n"
	"\tutf8_int32_t last;\n"
	"\tuint32_t class_index;\n"
	"} LexerRange;\n"
	"/* Lexer\n"
	" *\n"
	" * MEMBERS:\n"
	" * \tsource: text being lexed, borrowed, tokens point into it\n"
	" * \tlength: bytes in source\n"
	" * \toffset: byte offset of the next unlexed character\n"
	" * \trow, col: position of offset, both start at 1\n"
	" */\n"
	"typedef struct Lexer {\n"
	"\tconst char *source;\n"
	"\tsize_t length;\n"
	"\tsize_t offset;\n"
	"\tuint32_t row;\n"
	"\tuint32_t col;\n"
	"} Lexer;\n";

/* lexer_driver
 *
 * Emitted after the lexer tables, a maximal munch driver over them
 */
static const char * const lexer_driver =
	"/* lexer_codepoint\n"
	" *\n"
	" * utf8codepoint at a byte offset, a sequence cut off by the end of the text is not read and\n"
	" * yields -1, which is class 0\n"
	" */\n"
	"static utf8_int32_t lexer_codepoint(const Lexer *l, size_t offset, size_t *size) {\n"
	"\tutf8_int32_t codepoint = -1;\n"
	"\n"
	"\t*size = utf8codepointcalcsize(l->source + offset);\n"
	"\n"
	"\tif(*size <= l->length - offset) {\n"
	"\t\tutf8codepoint(l->source + offset, &codepoint);\n"
	"\t} else {\n"
	"\t\t*size = 1;\n"
	"\t}\n"
	"\n"
	"\treturn codepoint;\n"
	"}\n"
	"/* lexer_class\n"
	" *\n"
	" * Binary search of lexer_ranges for the column of a codepoint\n"
	" */\n"
	"static uint32_t lexer_class(utf8_int32_t codepoint) {\n"
	"\tsize_t low = 0;\n"
	"\tsize_t high = LEXER_NUM_RANGES;\n"
	"\n"
	"\twhile(low < high) {\n"
	"\t\tsize_t middle = low + (high - low) / 2;\n"
	"\n"
	"\t\tif(codepoint < lexer_ranges[middle].first) {\n"
	"\t\t\thigh = middle;\n"
	"\t\t} else if(codepoint > lexer_ranges[middle].last) {\n"
	"\t\t\tlow = middle + 1;\n"
	"\t\t} else {\n"
	"\t\t\treturn lexer_ranges[middle].class_index;\n"
	"\t\t}\n"
	"\t}\n"
	"\n"
	"\treturn 0;\n"
	"}\n"
	"/* lexer_consume\n"
	" *\n"
	" * Moves a Lexer to a byte offset, keeping its row and column up to date\n"
	" */\n"
	"static void lexer_consume(Lexer *l, size_t end) {\n"
	"\tfor(; l->offset < end; l->offset += 1) {\n"
	"\t\tunsigned char c = (unsigned char) l->source[l->offset];\n"
	"\n"
	"\t\tif(c == '\\n') {\n"
	"\t\t\tl->row += 1;\n"
	"\t\t\tl->col = 1;\n"
	"\t\t} else if((c & 0xC0) != 0x80) {\n"
	"\t\t\tl->col += 1;\n"
	"\t\t}\n"
	"\t}\n"
	"}\n"
	"/* lexer_new\n"
	" *\n"
	" * Creates a Lexer over text, the text must outlive the Lexer and its tokens\n"
	" */\n"
	"Lexer lexer_new(const char *source, size_t length) {\n"
	"\treturn (Lexer) {\n"
	"\t\t.source = source,\n"
	"\t\t.length = length,\n"
	"\t\t.offset = 0,\n"
	"\t\t.row = 1,\n"
	"\t\t.col = 1,\n"
	"\t};\n"
	"}\n"
	"/* lexer_next\n"
	" *\n"
	" * Skips whitespace then returns the longest token at the Lexer's position. Text that starts no\n"
	" * token is returned one codepoint at a time as TokenType_Invalid. At the end it returns TokenType_EOF.\n"
	" */\n"
	"Token lexer_next(Lexer *l) {\n"
	"\tconst unsigned char *source = (const unsigned char *) l->source;\n"
	"\n"
	"\twhile(l->offset < l->length && (source[l->offset] == ' ' || (source[l->offset] >= '\\t' && source[l->offset] <= '\\r'))) {\n"
	"\t\tlexer_consume(l, l->offset + 1);\n"
	"\t}\n"
	"\n"
	"\tToken result = {\n"
	"\t\t.lexeme = {\n"
	"\t\t\t.data = l->source + l->offset,\n"
	"\t\t\t.length = 0,\n"
	"\t\t},\n"
	"\t\t.tag = TokenType_EOF,\n"
	"\t\t.pos = {\n"
	"\t\t\t.row = l->row,\n"
	"\t\t\t.col = l->col,\n"
	"\t\t},\n"
	"\t};\n"
	"\n"
	"\tif(l->offset >= l->length) {\n"
	"\t\treturn result;\n"
	"\t}\n"
	"\n"
	"\tuint32_t state = LEXER_START;\n"
	"\tuint32_t tag = LEXER_NONE;\n"
	"\tsize_t end = l->offset;\n"
	"\n"
	"\t// Run until the DFA dies, remembering the last accepting position\n"
	"\tfor(size_t i = l->offset; i < l->length;) {\n"
	"\t\tsize_t size;\n"
	"\t\tutf8_int32_t codepoint = lexer_codepoint(l, i, &size);\n"
	"\n"
	"\t\tstate = lexer_transitions[state * LEXER_NUM_CLASSES + lexer_class(codepoint)];\n"
	"\n"
	"\t\tif(state == LEXER_DEAD) {\n"
	"\t\t\tbreak;\n"
	"\t\t}\n"
	"\n"
	"\t\ti += size;\n"
	"\n"
	"\t\tif(lexer_accept[state] != LEXER_NONE) {\n"
	"\t\t\ttag = lexer_accept[state];\n"
	"\t\t\tend = i;\n"
	"\t\t}\n"
	"\t}\n"
	"\n"
	"\tif(tag == LEXER_NONE) {\n"
	"\t\tsize_t size;\n"
	"\n"
	"\t\tlexer_codepoint(l, l->offset, &size);\n"
	"\t\tend = l->offset + size;\n"
	"\t\ttag = TokenType_Invalid;\n"
	"\t}\n"
	"\n"
	"\tresult.lexeme.length = end - l->offset;\n"
	"\tresult.tag = (TokenType) tag;\n"
	"\tlexer_consume(l, end);\n"
	"\n"
	"\treturn result;\n"
	"}\n";

/* generate_lexer
 *
 * Emits a table driven lexer: the DFA's transition, accept and codepoint class tables, then a
 * fixed driver over them. Needs Token and TokenType, so it is emitted after the AST section.
 */
static void generate_lexer(Context context, DFATable table) {
	FILE *fp = context.lexer_out.fp;
	size_t num_ranges = cvector_size(table.ranges);

	fputs(lexer_declarations, fp);

	fprintf(fp, "#define LEXER_NUM_STATES %u\n", table.num_states);
	fprintf(fp, "#define LEXER_NUM_CLASSES %u\n", table.num_classes);
	fprintf(fp, "#define LEXER_NUM_RANGES %zu\n", num_ranges);

	fprintf(fp, "static const uint32_t lexer_transitions[%u * %u] = {\n", table.num_states, table.num_classes);

	for(uint32_t state = 0; state < table.num_states; state += 1) {
		fprintf(fp, "\t");

		for(uint32_t class = 0; class < table.num_classes; class += 1) {
			fprintf(fp, "%u, ", table.next[state * table.num_classes + class]);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");
	fprintf(fp, "static const uint32_t lexer_accept[%u] = {\n", table.num_states);

	for(uint32_t state = 0; state < table.num_states; state += 1) {
		if(table.accept[state] == NonAccepting) {
			fprintf(fp, "\tLEXER_NONE,\n");
		} else {
			fprintf(fp, "\t%u,\n", table.accept[state]);
		}
	}

	fprintf(fp, "};\n");

	// C has no empty arrays, a grammar without terminals gets one range that matches nothing
	fprintf(fp, "static const LexerRange lexer_ranges[%zu] = {\n", num_ranges > 0 ? num_ranges : 1);

	for(size_t i = 0; i < num_ranges; i += 1) {
		fprintf(fp, "\t{ 0x%X, 0x%X, %u },\n", (unsigned) table.ranges[i].first, (unsigned) table.ranges[i].last, table.ranges[i].class);
	}

	if(num_ranges == 0) {
		fprintf(fp, "\t{ 1, 0, 0 },\n");
	}

	fprintf(fp, "};\n");

	fputs(lexer_driver, fp);
}

static void generate_ast(Context context, Grammar grammar) {
	generate_token_types(context.ast_out.fp, grammar);

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
			fprintf(context.ast_out.fp, "\t\tstruct {\n");

			if(alternates) {
				fprintf(context.ast_out.fp, "\t\t\tsize_t tag;\n");
				fprintf(context.ast_out.fp, "\t\t\tunion {\n");
			}


			for(Definition *it2 = cvector_begin(it1->definitions); it2 != cvector_end(it1->definitions); it2 += 1) {
				size_t expression = 0;

				if(alternates) {
					fprintf(context.ast_out.fp, "\t\t\t\tstruct {\n");
				}

				for(Symbol *it3 = cvector_begin(*it2); it3 != cvector_end(*it2); it3 += 1) {
					if(alternates) {
						fprintf(context.ast_out.fp, "\t\t");
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
					fprintf(context.ast_out.fp, "\t\t");
					fprintf(context.ast_out.fp, "} _c%zu;\n", alternates_count);
					alternates_count += 1;
				}
			}

			if(alternates) {
				fprintf(context.ast_out.fp, "\t\t\t};\n");
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
