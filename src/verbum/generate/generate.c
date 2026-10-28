#include <stdio.h>
#include <stddef.h>

#include "../table/table.h"
#include "../grammar/grammar.h"

#include "generate.h"


void verbum_generate(Grammar grammar, ParseTable table) {
	static const char *const output_file = "table_output.c";
	FILE *fp = fopen(output_file, "w+");
	(void) grammar;

	fprintf(fp, "#include <stdint.h>\n");
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
	fprintf(fp, "static const uint32_t ParseTableActionError = ParseTableActionType_Error;\n");
	fprintf(fp, "/* GotoError\n");
	fprintf(fp, " *\n");
	fprintf(fp, " * Marks a goto table entry with no transition\n");
	fprintf(fp, " */\n");
	fprintf(fp, "static const uint32_t ParseTableGotoError = UINT32_MAX;\n");
	fprintf(fp, "typedef uint32_t ParseTableGoto;\n");
	fprintf(fp, "static const ParseTableAction action_table[%zu][%zu] = {\n", (size_t )table.num_states, (size_t) table.num_terminals + 1);

	// Width of the longest element so every element can be padded to it
	int action_width = 0;
	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		for(size_t terminal_index = 0; terminal_index <= table.num_terminals; terminal_index += 1) {
			ParseTableAction action = parse_table_action_at(&table, state_index, terminal_index);
			int width;

			if(action == ParseTableActionType_Error) {
				width = snprintf(NULL, 0, "ParseTableActionError");
			} else {
				width = snprintf(NULL, 0, "%u", action);
			}

			if(width > action_width) {
				action_width = width;
			}
		}
	}

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		// One past the last terminal is the end of input
		for(size_t terminal_index = 0; terminal_index <= table.num_terminals; terminal_index += 1) {
			ParseTableAction action = parse_table_action_at(&table, state_index, terminal_index);

			if(action == ParseTableActionType_Error) {
				fprintf(fp, "%*s, ", action_width, "ParseTableActionError");
			} else {
				fprintf(fp, "%*u, ", action_width, action);
			}
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");
	fprintf(fp, "static const ParseTableGoto goto_table[%zu][%zu] = {\n", (size_t )table.num_states, (size_t) table.num_nonterminals);

	// Width of the longest element so every element can be padded to it
	int goto_width = 0;
	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
			ParseTableGoto g0t0 = parse_table_goto_at(&table, state_index, nonterminal_index);
			int width;

			if(g0t0 == ParseTableGotoError) {
				width = snprintf(NULL, 0, "ParseTableGotoError");
			} else {
				width = snprintf(NULL, 0, "%u", g0t0);
			}

			if(width > goto_width) {
				goto_width = width;
			}
		}
	}

	for(size_t state_index = 0; state_index < table.num_states; state_index += 1) {
		// One past the last terminal is the end of input
		for(size_t nonterminal_index = 0; nonterminal_index < table.num_nonterminals; nonterminal_index += 1) {
			ParseTableGoto g0t0 = parse_table_goto_at(&table, state_index, nonterminal_index);

			if(g0t0 == ParseTableGotoError) {
				fprintf(fp, "%*s, ", goto_width, "ParseTableGotoError");

			} else {
				fprintf(fp, "%*u, ", goto_width, g0t0);
			}

		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n");

	
	fclose(fp);
}
