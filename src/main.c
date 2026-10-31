#include <stdio.h>
#include <stdlib.h>

#include <libgen.h>

#include "lib/log.c/log.h"
#include "lib/c-vector/cvector.h"
#include "lib/optparse99/optparse99.h"

#include "verbum/ast/ast.h"
#include "verbum/lexer/lexer.h"
#include "verbum/analyzer/analyzer.h"
#include "verbum/lr0/lr0.h"
#include "verbum/lr1/lr1.h"
#include "verbum/lane_table/lane_table.h"
#include "verbum/parse/parse.h"
#include "verbum/raw_rule/raw_rule.h"
#include "verbum/table/table.h"
#include "verbum/generate/generate.h"


/* Helper macro for default `--help` option */
#define CLI_HELP_OPTION .short_name = 'h',\
			.long_name = "help",\
			.description = "displays this page",\
			.function = optparse_print_help,



void verbum(int argc, char **argv);

int main(int argc, char **argv) {
#ifdef FORGE_RELEASE
	log_set_level(2);
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	struct optparse_cmd commands = {
		.name = "verbum",
		.about = "verbum 0.1.0 - ebnf to c parser generator",
		.description = "verbum is an ebnf to c parser generator that produces good code.",
		.usage = "verbum <SUBCOMMAND> [OPTIONS]",
		.options = (struct optparse_opt []) {
			{ CLI_HELP_OPTION },
			{ END_OF_OPTIONS },
		},
		.subcommands = (struct optparse_cmd []) {
			(struct optparse_cmd) {
				.name = "generate",
				.about = "generates parser",
				.description = "generates parser",
				.usage = "generate <FILE>",
				.options = (struct optparse_opt []) {
					{ CLI_HELP_OPTION },
					{ END_OF_OPTIONS },
				},
				.function = verbum,
			},
			{ END_OF_SUBCOMMANDS },
		},


	};

#pragma GCC diagnostic pop

	optparse_parse(&commands, &argc, &argv);

	return 0;
}


void verbum(int argc, char **argv) {
	if(argc != 2) {
		fprintf(stderr, "Expected file got none!\n");
		exit(EXIT_FAILURE);
	}

	Lexer lexer = lexer_new(argv[1]);
	cvector(RawRule) rules = verbum_parse(&lexer);
	Grammar grammar = analyzer_analyze(rules);
	LR0Automaton lr0 = lr0_new(grammar);
	cvector(size_t) conflicts = NULL;
	LR1Automaton lane_table = lane_table_new(grammar, lr0, &conflicts);
	ParseTable table = parse_table_new(grammar, lane_table);
	AST *ast = ast_new_from_raw_rules(rules);

	log_info("Generated table size for %s: %zu bytes", basename(argv[1]),
			((size_t) (table.num_nonterminals + table.num_states)) * ((size_t) table.num_states) * ((size_t) sizeof(*table.action_table)));

	verbum_generate(rules, grammar, table);

	for(RawRule *it1 = cvector_begin(rules); it1 != cvector_end(rules); it1 += 1) {
		raw_rule_delete(it1);
	}

	ast_delete(ast);
	parse_table_delete(&table);
	cvector_free(conflicts);
	lr1_delete(lane_table);
	lr0_delete(lr0);
	grammar_delete(&grammar);
	cvector_free(rules);
	lexer_delete(&lexer);
}
