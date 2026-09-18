#include "../token/token.h"
#include "parser.h"

Parser parser_new(ParseTable t) {
    Parser p;
    p.input = NULL;
    p.table = t;
    p.stack.top = 0;
    // Assuming cvector initialization macro exists in your vector library (e.g., cvector_init)
    p.stack.data = NULL; 
    return p;
}

void parser_parse(Parser *p, char *input) {
    p->input = input;
    p->stack.top = 0;

    // Push initial state 0 onto the stack (stored as a Token/state representation)
    Token initial_token;
    memset(&initial_token, 0, sizeof(Token));
    initial_token.state = 0; // Assuming Token has a state field or equivalent
    cvector_push_back(p->stack.data, initial_token);
    p->stack.top++;

    while (true) {
        // Retrieve current state from the top of the stack
        size_t current_state = p->stack.data[p->stack.top - 1].state;
        
        // Get the current terminal/token index from the input stream
        uint32_t current_terminal = get_next_token_index(p->input); 

        ParseTableAction action = parse_table_action_at(&p->table, current_state, current_terminal);

        switch (action.tag) {
            case ParseTableActionType_Shift: {
                // Shift: push next state and token onto stack
                Token next_token = advance_and_get_token(&p->input);
                next_token.state = action.shift.destination;
                cvector_push_back(p->stack.data, next_token);
                p->stack.top++;
                break;
            }
            case ParseTableActionType_Reduce: {
                // Reduce: Pop rule RHS elements off the stack, push LHS non-terminal using GOTO table
                Rule *rule = get_rule_by_index(&p->table, action.reduce.destination);
                size_t rhs_length = get_rule_rhs_length(rule);
                
                p->stack.top -= rhs_length; // Pop RHS items
                
                size_t previous_state = p->stack.data[p->stack.top - 1].state;
                uint32_t lhs_non_terminal = get_non_terminal_index(rule->lhs);
                
                ParseTableGoto next_state = parse_table_goto_at(&p->table, previous_state, lhs_non_terminal);
                
                Token non_terminal_token;
                memset(&non_terminal_token, 0, sizeof(Token));
                non_terminal_token.state = next_state;
                
                cvector_push_back(p->stack.data, non_terminal_token);
                p->stack.top++;
                break;
            }
            case ParseTableActionType_Accept:
                printf("Parsing completed successfully!\n");
                return;
            case ParseTableActionType_Error:
            default:
                fprintf(stderr, "Syntax Error: Invalid parser state or unexpected token.\n");
                return;
        }
    }
}
