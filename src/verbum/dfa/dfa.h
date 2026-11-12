#ifndef VERBUM_DFA_H
#define VERBUM_DFA_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"
#include "../../lib/utf8.h/utf8.h"

#include "../grammar/grammar.h"


/* NonAccepting
 *
 * Accept value of a state that does not end a token
 */
static const uint32_t NonAccepting = UINT32_MAX;
/* DFADead
 *
 * State with no edges that never accepts, every missing edge leads here. Being 0 a zeroed
 * transition table is all errors.
 */
static const uint32_t DFADead = 0;
/* DFAStart
 *
 * State lexing begins in
 */
static const uint32_t DFAStart = 1;

/* DFAEdge
 *
 * MEMBERS:
 * 	codepoint: codepoint consumed to take the edge
 * 	destination: index of the state the edge leads to
 */
typedef struct DFAEdge {
	utf8_int32_t codepoint;
	uint32_t destination;
} DFAEdge;

/* DFAState
 *
 * MEMBERS:
 * 	edges: outgoing edges sorted by codepoint, one per codepoint, a missing codepoint leads to DFADead
 * 	accept: TokenType ended by this state (the terminal's symbol_set_index), NonAccepting if none
 */
typedef struct DFAState {
	cvector(DFAEdge) edges;
	uint32_t accept;
} DFAState;

/* DFA
 *
 * Deterministic automaton recognising every terminal of a grammar
 *
 * MEMBERS:
 * 	states: every state, DFADead and DFAStart first, edges refer to them by index
 */
typedef struct DFA {
	cvector(DFAState) states;
} DFA;

/* dfa_build
 *
 * Builds a DFA for every terminal in a grammar. Lexical rules a terminal references are inlined.
 * When two terminals match the same text a literal beats a named rule, otherwise the lower
 * terminal index wins and a warning is logged.
 *
 * PARAMETERS:
 * 	g: grammar to build from
 * 	out: receives the DFA, delete it with dfa_delete even on failure
 *
 * RETURNS:
 * 	true on success, false if a lexical rule is recursive or undefined
 */
bool dfa_build(Grammar g, DFA *out);
/* dfa_delete
 *
 * Deletes a DFA
 *
 * PARAMETERS:
 * 	dfa: DFA to delete
 */
void dfa_delete(DFA *dfa);
/* dfa_print
 *
 * Prints a DFA one state per line, for debugging. Runs of consecutive codepoints leading to the
 * same state are printed as a range.
 *
 * PARAMETERS:
 * 	fp: stream to print to
 * 	dfa: DFA to print
 */
void dfa_print(FILE *fp, DFA dfa);

/* DFAClassRange
 *
 * Consecutive codepoints sharing a class
 *
 * MEMBERS:
 * 	first: first codepoint of the range
 * 	last: last codepoint of the range, inclusive
 * 	class: column of the codepoints in DFATable.next
 */
typedef struct DFAClassRange {
	utf8_int32_t first;
	utf8_int32_t last;
	uint32_t class;
} DFAClassRange;

/* DFATable
 *
 * A DFA as dense tables. Codepoints that lead to the same state from every state share a class,
 * so the transition table has one column per class rather than per codepoint.
 *
 * MEMBERS:
 * 	num_states: rows of next, length of accept
 * 	num_classes: columns of next, class 0 is every codepoint no edge consumes
 * 	next: next[state * num_classes + class] is the destination, DFADead if none
 * 	accept: TokenType accepted by each state, NonAccepting if none
 * 	ranges: sorted, non overlapping, a codepoint outside every range is class 0
 */
typedef struct DFATable {
	uint32_t num_states;
	uint32_t num_classes;
	uint32_t *next;
	uint32_t *accept;
	cvector(DFAClassRange) ranges;
} DFATable;

/* dfa_table_build
 *
 * Converts a DFA into dense tables
 *
 * PARAMETERS:
 * 	dfa: DFA to convert
 * 	out: receives the tables, delete them with dfa_table_delete
 */
void dfa_table_build(DFA dfa, DFATable *out);
/* dfa_table_delete
 *
 * Deletes a DFATable
 *
 * PARAMETERS:
 * 	table: tables to delete
 */
void dfa_table_delete(DFATable *table);


#endif
