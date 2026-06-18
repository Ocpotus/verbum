#include <stdio.h>
#include <stdbool.h>

#include "../../lib/c-vector/cvector.h"


#include "../grammar/grammar.h"
#include "../lr0/lr0.h"
#include "../token/token.h"

#include "lalr.h"


typedef struct VariableInfo {
	Token variable;
	bool nullable;
	cvector(Token) firsts;
} VariableInfo;

typedef struct DirectedGraphNode {
	int index;
	bool on_stack;
} DirectedGraphNode;

typedef struct LookbackLink {
	size_t reduction_item_index;
	int state_index;
	int transition_index;
} LookbackLink;

static bool token_set_contains(cvector(Token) set, Token t);
static void token_set_insert(cvector(Token) *set, Token t);
static bool variable_info_set_contains(cvector(VariableInfo) set, VariableInfo vi);
static void variable_info_set_insert(cvector(VariableInfo) *set, VariableInfo vi);
static VariableInfo variable_info_set_get(cvector(VariableInfo) set, VariableInfo vi);
static void variable_info_set_update(cvector(VariableInfo) set, VariableInfo vi);
static void digraph_traverse_reads(cvector(int) digraph_stack, int x, int *l_val, cvector(DirectedGraphNode) nodes, cvector(Transition) transitions, cvector(cvector(int)) graph);
static int trace_transition_path(cvector(Transition) transitions, int start_state, cvector(Token) path);



void lalr_construct(Grammar g, LR0 lr0) {
	cvector(VariableInfo) variable_info = NULL;
	bool changed = true;

	// Propogate variable_info
	for(Rule *it1 = cvector_begin(g.rules); it1 != cvector_end(g.rules); it1 += 1) {
		VariableInfo vi = {
			.variable = it1->identifier,
			.firsts = NULL,
			.nullable = false,
		};

		if(!variable_info_set_contains(variable_info, vi)) {
			variable_info_set_insert(&variable_info, vi);
		}
	}

	// Scan nullability
	while(changed) {
		changed = false;

		for(Rule *it1 = cvector_begin(g.rules); it1 != cvector_end(g.rules); it1 += 1) {
			VariableInfo vi = variable_info_set_get(variable_info, (VariableInfo){ .variable = it1->identifier });
			bool rhs_all_nullable = true;

			if(vi.variable.tag == TokenType_Invalid || vi.nullable) {
				continue;
			}


			for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
				if(it2->tag == TokenType_Double_Literal || it2->tag == TokenType_Single_Literal) {
					rhs_all_nullable = false;
					break;
				} else if(it2->tag == TokenType_Identifier) {
					VariableInfo sub = variable_info_set_get(variable_info, (VariableInfo){ .variable = *it2 });

					if(sub.variable.tag == TokenType_Invalid || !sub.nullable) {
						rhs_all_nullable = false;
						break;
					}
				}
			}

			if(rhs_all_nullable) {
				vi.nullable = true;
				variable_info_set_update(variable_info, vi);
				changed = true;
			}
		}
	}

	changed = true;

	// Retrieve FIRST sets
	while(changed) {
		changed = false;

		for(Rule *it1 = cvector_begin(g.rules); it1 != cvector_end(g.rules); it1 += 1) {
			VariableInfo vi = variable_info_set_get(variable_info, (VariableInfo){ .variable = it1->identifier });
			size_t initial_size = cvector_size(vi.firsts);

			for(Token *it2 = cvector_begin(it1->definition); it2 != cvector_end(it1->definition); it2 += 1) {
				if(it2->tag == TokenType_Double_Literal || it2->tag == TokenType_Single_Literal) {
					token_set_insert(&vi.firsts, *it2);
					break; // Terminal, no need to check further
				} else if(it2->tag == TokenType_Identifier) {
					VariableInfo sub = variable_info_set_get(variable_info, (VariableInfo){ .variable = *it2 });

					if(sub.variable.tag == TokenType_Invalid) {
						break;
					}

					for(Token *it3 = cvector_begin(sub.firsts); it3 != cvector_end(sub.firsts); it3 += 1) {
						token_set_insert(&vi.firsts, *it3);
					}

					// Lookahead can only bypass this token if it is marked nullable
					if(!sub.nullable) {
						break;
					}
				}
			}

			if(cvector_size(vi.firsts) != initial_size) {
				variable_info_set_update(variable_info, vi);
				changed = true;
			}
		}
	}

	// Compute Direct Read sets for each transition
	// A direct read set is a set of terminals that can be read right after a transition
	// I.E.
	// δ(p, A) = q
	// δ(q, "x") = q'
	// δ(q, "y") = q''
	// direct_read(p, A) = { "x" , "y" }
	for(Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		for(Transition *it2 = cvector_begin(lr0.transitions); it2 != cvector_end(lr0.transitions); it2 += 1) {
			if(it1->destination_index == it2->source_index) {
				if(it2->symbol.tag == TokenType_Single_Literal || it2->symbol.tag == TokenType_Double_Literal) {
					token_set_insert(&it1->direct_reads, it2->symbol);
					changed = true;
				}
			}
		}
	}

	// Compute read sets
	cvector(cvector(int)) read_graph = NULL;

	for(Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		cvector_push_back(read_graph, NULL);

		for(Transition *it2 = cvector_begin(lr0.transitions); it2 != cvector_end(lr0.transitions); it2 += 1) {
			if(it1->destination_index == it2->source_index) {
				if(it2->symbol.tag == TokenType_Identifier) {
					VariableInfo vi = variable_info_set_get(variable_info, (VariableInfo) { .variable = it2->symbol });

					if(vi.nullable && vi.variable.tag != TokenType_Invalid) {
						cvector_push_back(read_graph[cvector_size(read_graph) - 1], it2 - cvector_begin(lr0.transitions));
					}

				}
			}
		}
	}

	cvector(DirectedGraphNode) nodes = NULL;
	cvector(int) digraph_stack = NULL;
	//cvector_reserve(nodes, cvector_size(lr0.transitions));
	cvector_resize(nodes, cvector_size(lr0.transitions), (DirectedGraphNode) { 0 });

	for(size_t i = 0; i < cvector_size(lr0.transitions); i += 1) {
		nodes[i].index = -1;
		nodes[i].on_stack = false;
	}

	int lookahead_depth = 0;
	for(size_t i = 0; i < cvector_size(lr0.transitions); i += 1) {
		if(nodes[i].index == -1) {
			digraph_traverse_reads(digraph_stack, (int) i, &lookahead_depth, nodes, lr0.transitions, read_graph);
		}
	}

	cvector_free(digraph_stack);
	cvector_free(nodes);
	cvector_free(read_graph);

	// Includes & lookback
	cvector(cvector(int)) includes_graph = NULL;
	//cvector_reserve(includes_graph, cvector_size(lr0.transitions));
	cvector_resize(includes_graph, cvector_size(lr0.transitions), 0);
	cvector(LookbackLink) lookbacks = NULL;
	
	for(size_t i = 0; i < cvector_size(lr0.transitions); i += 1) {
		includes_graph[i] = NULL;
	}

	for(Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		if(it1->symbol.tag != TokenType_Identifier) {
			continue;
		}

		int source_index = it1->source_index;
		Token t = it1->symbol;

		for(Rule *it2 = cvector_begin(g.rules); it2 != cvector_end(g.rules); it2 += 1) {
			int x = trace_transition_path(lr0.transitions, source_index, it2->definition);

			if(x == -1) {
				continue;
			}

			for(Item *it3 = cvector_begin(lr0.states[x].items); it3 != cvector_end(lr0.states[x].items); it3 += 1) {
				if(it3->dot_pos == cvector_size(it3->rule.definition) && token_equals(it3->rule.identifier, it2->identifier)) {
					LookbackLink lb = {
						.reduction_item_index = it3 - cvector_begin(lr0.states[x].items),
						.state_index = x,
						.transition_index = it1->id,
					};

					cvector_push_back(lookbacks, lb);
				}
			}

				for (int s_len = (int)cvector_size(it2->definition); s_len >= 0; s_len -= 1) {
				    cvector(Token) p_path = NULL;
				    for (int k = 0; k < s_len; k += 1) {
					cvector_push_back(p_path, it2->definition[k]);
				    }

				    int q = trace_transition_path(lr0.transitions, source_index, p_path);
				    cvector_free(p_path);

				    if (q == -1) {
					break;
				    }

				    // Connect the Includes relation
				    for (Transition *it3 = cvector_begin(lr0.transitions); it3 != cvector_end(lr0.transitions); it3 += 1) {
					if (it3->source_index == q && token_equals(it3->symbol, it2->identifier)) {
					    bool edge_exists = false;
					    for (size_t e = 0; e < cvector_size(includes_graph[it3->id]); e += 1) {
						if (includes_graph[it3->id][e] == it1->id) {
						    edge_exists = true;
						    break;
						}
					    }
					    if (!edge_exists) {
						cvector_push_back(includes_graph[it3->id], it1->id);
					    }
					}
				    }

				    // Nullability check: can lookaheads skip over the token at index [s_len - 1]?
				    if (s_len > 0) {
					Token nt = it2->definition[s_len - 1];
					if (nt.tag != TokenType_Identifier) {
					    break; // Terminals stop backward propagation
					}

					VariableInfo vi = variable_info_set_get(variable_info, (VariableInfo) { .variable = nt });
					if (!vi.nullable) {
					    // This token is not nullable, so lookaheads cannot pass through it.
					    // However, we still need to process the s_len = 0 path to link the parent context!
					    s_len = 1; // Forces the next iteration to be s_len = 0
					}
				    }
				}
		}
	}

	// Final pass
	cvector(DirectedGraphNode) include_nodes = NULL;
	cvector_resize(include_nodes, cvector_size(lr0.transitions), (DirectedGraphNode) { 0 });
	//cvector_reserve(include_nodes, cvector_size(lr0.transitions));

	for(size_t i = 0; i < cvector_size(include_nodes); i += 1) {
		include_nodes[i].index = -1;
		include_nodes[i].on_stack = false;
	}

	int include_depth = 0;

	for(size_t i = 0; i < cvector_size(lr0.transitions); i += 1) {
		if(include_nodes[i].index == -1) {
			digraph_traverse_reads(digraph_stack, (int) i, &include_depth, include_nodes, lr0.transitions, includes_graph);
		}
	}

	for(LookbackLink *it1 = cvector_begin(lookbacks); it1 != cvector_end(lookbacks); it1 += 1) {
		Transition source = lr0.transitions[it1->transition_index];

		for(Token *it2 = cvector_begin(source.direct_reads); it2 != cvector_end(source.direct_reads); it2 += 1) {
			token_set_insert(&lr0.states[it1->state_index].items[it1->reduction_item_index].lookaheads, *it2);
		}
	}

	cvector_free(include_nodes);
	cvector_free(lookbacks);

	// Printing, debugging will delete eventually
	printf("@start: %.*s\n", (int) g.start->identifier.lexeme.length, g.start->identifier.lexeme.data);

	for(State *it1 = cvector_begin(lr0.states); it1 != cvector_end(lr0.states); it1 += 1) {
		printf("s%zu:\n", it1 - cvector_begin(lr0.states));

		for(Item *it2 = cvector_begin(it1->items); it2 != cvector_end(it1->items); it2 += 1) {
			printf("\t%.*s -> ", (int) it2->rule.identifier.lexeme.length, it2->rule.identifier.lexeme.data);

			for(Token *it3 = cvector_begin(it2->rule.definition); it3 != cvector_end(it2->rule.definition); it3 += 1) {
				if((size_t) (it3 - cvector_begin(it2->rule.definition)) == it2->dot_pos) {
					printf("* ");
				}

				printf("%.*s ", (int) it3->lexeme.length, it3->lexeme.data);
			}

			if(it2->dot_pos == cvector_size(it2->rule.definition)) {
				printf("*");
			}

			printf(" {");

			for(Token *it3 = cvector_begin(it2->lookaheads); it3 != cvector_end(it2->lookaheads); it3 += 1) {
				printf(" %.*s", (int) it3->lexeme.length, it3->lexeme.data);
			}
			printf(" }");

			printf("\n");
		}
	}

	for(Transition *it1 = cvector_begin(lr0.transitions); it1 != cvector_end(lr0.transitions); it1 += 1) {
		printf("δ(s%zu, %.*s) = s%zu\n", it1->source_index, (int) it1->symbol.lexeme.length,
		       				 it1->symbol.lexeme.data, it1->destination_index);
		printf("Direct Reads:");

		for(Token *it2 = cvector_begin(it1->direct_reads); it2 != cvector_end(it1->direct_reads); it2 += 1) {
			printf(" %.*s", (int) it2->lexeme.length, it2->lexeme.data);
		}

		printf("\n");
	}

	for(VariableInfo *it1 = cvector_begin(variable_info); it1 != cvector_end(variable_info); it1 += 1) {
		printf("NULLABLE(%.*s) = %s\n", (int) it1->variable.lexeme.length, it1->variable.lexeme.data, it1->nullable ? "true" : "false");
	}

	for(VariableInfo *it1 = cvector_begin(variable_info); it1 != cvector_end(variable_info); it1 += 1) {
		printf("FIRST(%.*s):", (int) it1->variable.lexeme.length, it1->variable.lexeme.data);

		for(Token *it2 = cvector_begin(it1->firsts); it2 != cvector_end(it1->firsts); it2 += 1) {
			printf(" %.*s", (int) it2->lexeme.length, it2->lexeme.data);
		}

		printf("\n");
	}

	for(VariableInfo *it1 = cvector_begin(variable_info); it1 != cvector_end(variable_info); it1 += 1) {
		cvector_free(it1->firsts);
	}

	cvector_free(variable_info);
}

static bool token_set_contains(cvector(Token) set, Token t) {
	for(Token *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(*it1, t)) {
			return true;
		}
	}

	return false;
}

static void token_set_insert(cvector(Token) *set, Token t) {
	for(Token *it = cvector_begin(*set); it != cvector_end(*set); it += 1) {
		if(token_equals(*it, t)) {
			return;
		}
	}

	cvector_push_back(*set, t);
}

static bool variable_info_set_contains(cvector(VariableInfo) set, VariableInfo vi) {
	for(VariableInfo *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(it1->variable, vi.variable)) {
			return true;
		}
	}

	return false;
}

static void variable_info_set_insert(cvector(VariableInfo) *set, VariableInfo vi) {
	for(VariableInfo *it1 = cvector_begin(*set); it1 != cvector_end(*set); it1 += 1) {
		if(token_equals(it1->variable, vi.variable)) {
			return;
		}
	}

	cvector_push_back(*set, vi);
}

static VariableInfo variable_info_set_get(cvector(VariableInfo) set, VariableInfo vi) {
	for(VariableInfo *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(it1->variable, vi.variable)) {
			return *it1;
		}
	}

	return (VariableInfo) {
		.variable = {
			.tag = TokenType_Invalid,
			.lexeme = {
				.data = NULL,
				.length = 0,
			},
		},
		.nullable = false,
		.firsts = NULL,
	};
}

static void variable_info_set_update(cvector(VariableInfo) set, VariableInfo vi) {
	for(VariableInfo *it1 = cvector_begin(set); it1 != cvector_end(set); it1 += 1) {
		if(token_equals(it1->variable, vi.variable)) {
			it1->firsts = vi.firsts;
			it1->nullable = vi.nullable;
		}
	}
}

static void digraph_traverse_reads(cvector(int) digraph_stack, int x, int *l_val, cvector(DirectedGraphNode) nodes, cvector(Transition) transitions, cvector(cvector(int)) graph) {
	cvector_push_back(digraph_stack, x);
	nodes[x].on_stack = true;
	*l_val += 1;
	nodes[x].index = *l_val;

	int min_depth = nodes[x].index;

	for(size_t i = 0; i < cvector_size(graph[x]); i += 1) {
		int y = graph[x][i];

		if(nodes[y].index == -1) {
			digraph_traverse_reads(digraph_stack, y, l_val, nodes, transitions, graph);
		}

		if(nodes[y].on_stack) {
			if(nodes[y].index < min_depth) {
				min_depth = nodes[y].index;
			}
		}

		for(Token *it1 = cvector_begin(transitions[y].direct_reads); it1 != cvector_end(transitions[y].direct_reads); it1 += 1) {
			token_set_insert(&transitions[x].direct_reads, *it1);
		}
	}

	if(nodes[x].index == min_depth) {
		while(true) {
			int top_idx = *cvector_back(digraph_stack);
			cvector_pop_back(digraph_stack);

			nodes[top_idx].on_stack = false;

			for(Token *it1 = cvector_begin(transitions[x].direct_reads); it1 != cvector_end(transitions[x].direct_reads); it1 += 1) {
				token_set_insert(&transitions[top_idx].direct_reads, *it1);
			}

			if(top_idx == x) {
				break;
			}
		}
	}
}

static int trace_transition_path(cvector(Transition) transitions, int start_state, cvector(Token) path) {
	int current_state = start_state;

	for(Token *it1 = cvector_begin(path); it1 != cvector_end(path); it1 += 1) {
		bool step_found = false;

		for(Transition *it2 = cvector_begin(transitions); it2 != cvector_end(transitions); it2 += 1) {
			if(it2->source_index == current_state && token_equals(it2->symbol, *it1)) {
				current_state = it2->destination_index;
				step_found = true;
				break;
			}
		}

		if(!step_found) {
			return -1;
		}
	}

	return current_state;
}
