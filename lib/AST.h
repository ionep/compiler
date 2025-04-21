#include <stdio.h>
#include <stdlib.h>
#include<string.h>

// AST Node Structure
typedef struct ASTNode {
    char *type; //name for the node
    char *value; // value of the node
    struct ASTNode *left; //if sub-branches, then pointer to left sub node
    struct ASTNode *right; //if sub-branches, then pointer to right sub node
} ASTNode;

// Function to create an AST node
ASTNode* createNode(char *type, char *value, ASTNode *left, ASTNode *right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode)); // allocate size of ASTNode
    node->type = strdup(type); // get the type
    node->value = value ? strdup(value): NULL; // check if value is NULL, if not save the value
    node->left = left; // left sub node
    node->right = right; // right sub node
    return node; // return the new node
}

// Recursive function to print AST in a tree format
void printAST(ASTNode *node, int depth) {
    if (node == NULL)
        return;
    
    // Indentation for hierarchy visualization
    for (int i = 0; i < depth; i++)
        printf("  ");

    printf("|-%s", node->type);
    if (node->value)
        printf(" -%s", node->value);
    printf("\n");

    // Recursively print left and right children
    printAST(node->left, depth + 1);
    printAST(node->right, depth + 1);
}

// Recursive function to free each subnode of AST
void freeAST(ASTNode *node) {
    if (node == NULL)
        return;

    // printf("Freeing AST:%s %s\n",node->type,node->value);

    freeAST(node->left); // recursively free left subnode
    freeAST(node->right); // recursively free right subnode

    if(node->type != NULL){ // free type if not NULL
        free(node->type);
        node->type=NULL;
    }
    if(node->value != NULL){ // free value if not NULL
        free(node->value);
        node->value=NULL;
    }
    free(node); // free the node
    node=NULL; // make sure to keep the node NULL to avoid dangling pointers
}


typedef struct State State; // Forward declaration of State structure
typedef struct Transition Transition; // Forward declaration of Transition structure

struct Transition {
    char* match; // NULL = epsilon
    State* to;
    Transition* next; // linked list of transitions
};

struct State {
    int id;
    int is_accept;
    Transition* transitions;
    State* pair; // end and start state pair
    State* next; 
};

// Global state tracking
State *all_states_head = NULL; 
// State* all_states_tail = NULL;
State* all_states = NULL; // pointer to the first state
int state_id = 0;


void freeTransitions(Transition *t) {
    if (t == NULL)
        return;

    if(t->match != NULL) { // free match if not NULL
        free(t->match);
        t->match=NULL;
    }

    freeTransitions(t->next); // recursively free right subnode
    free(t); // free the node
    t=NULL; // make sure to keep the node NULL to avoid dangling pointers
}


void freeStates(State *head) {
    if (head == NULL)
        return;

    // printf("Freeing AST:%s %s\n",node->type,node->value);
    
    freeStates(head->next); // recursively free left subnode
    freeTransitions(head->transitions); // recursively free right subnode
    free(head); // free the node
    head=NULL; // make sure to keep the node NULL to avoid dangling pointers
}

State* createState(int is_accept) {
    State* s = malloc(sizeof(State));
    s->id = state_id++;
    s->is_accept = is_accept;
    s->transitions = NULL;
    if(!all_states_head) {
        all_states_head = s; // set the head of the linked list
    }
    s->next = all_states; 
    all_states = s; // set the current state to the new state
    return s;
}

void addTransition(State* from, char *match, State* to) {
    Transition* t = malloc(sizeof(Transition));
    t->match = (match!=NULL) ? strdup(match) : NULL; // copy the match string
    t->to = to;
    t->next = from->transitions;
    from->transitions = t;
}

void addRangeTransitions(ASTNode* node, State* start, State* end) {
    if (!node) return;

    if (strcmp(node->type, "RANGE_VAL") == 0) {
        // A binary tree: left and right both parts of the range-list
        addRangeTransitions(node->left,  start, end);
        addRangeTransitions(node->right, start, end);
    }
    else {
        // Leaf: one character or unicode literal
        // node->value is something like "a" or "%x1234;"
        addTransition(start, node->value, end);
    }
}

State* generateStates(ASTNode* node, Symbol *symbolTable) {
    if (node == NULL) return NULL;

    State* start = createState(0);
    State* end = createState(0);

    start->pair = end; // pair the start and end states
    end->pair = start; // pair the end and start states
    printf("\nGenerating state for: %s", node->type);

    // 1) Alternation:  ALT ← left | right
    if (strcmp(node->type, "ALT") == 0) {
        State* L = generateStates(node->left,symbolTable);
        State* R = generateStates(node->right,symbolTable);
        addTransition(start, NULL, L);
        addTransition(start, NULL, R);
        addTransition(L ->pair, NULL, end);
        addTransition(R ->pair, NULL, end);
        return start;
    }
    // 2) Sequence: SEQ ← left · right
    else if (strcmp(node->type, "SEQ") == 0) {
        State* L = generateStates(node->left,symbolTable);
        State* R = generateStates(node->right,symbolTable);
        addTransition(start,    NULL, L);
        addTransition(L->pair,  NULL, R);
        addTransition(R->pair,  NULL, end);
        return start;
    }
    // 3) Repetition: REPEAT ← child  with operator in node->value (“*”, “+”, or “?”)
    else if (strcmp(node->type, "REPEAT") == 0) {
        char op = node->value[0];
        State* F = generateStates(node->left,symbolTable);
        if (op == '*') {
            addTransition(start,    NULL, F);
            addTransition(start,    NULL, end);
            addTransition(F->pair,  NULL, F);
            addTransition(F->pair,  NULL, end);
        }
        else if (op == '+') {
            addTransition(start,    NULL, F);
            addTransition(F->pair,  NULL, F);
            addTransition(F->pair,  NULL, end);
        }
        else if (op == '?') {
            addTransition(start,    NULL, F);
            addTransition(start,    NULL, end);
            addTransition(F->pair,  NULL, end);
        }
        return start;
    }
    // 4) Parentheses: PAREN ← ( child )
    else if (strcmp(node->type, "PAREN") == 0) {
        State* C = generateStates(node->left,symbolTable);
        addTransition(start,   NULL, C);
        addTransition(C->pair, NULL, end);
        return start;
    }
     // 5) Character class: RANGE ← [ ... ]
     else if (strcmp(node->type, "RANGE") == 0) {
        // node->left is the multiregterm subtree
        addRangeTransitions(node->left, start, end);
        return start;
    }

    // 6) Substitute: SUBSTITUTE ← ${ ID }
    //    (you’ll want to replace this by expanding the ID’s definition AST)
    else if (strcmp(node->type, "SUBSTITUTE") == 0) {
        // for now treat as literal match of the name
        // node->left is the ASTNode("ID", name)
        ASTNode *symNode = getSymbol(node->left->value, symbolTable); // get the symbol from the symbol table
        if(symNode == NULL) {
            fprintf(stderr, "Error: Symbol %s not found in symbol table\n", node->left->value);
            exit(1);
        }
        State *fragment = generateStates(symNode, symbolTable); // generate states for the symbol node
        addTransition(start, NULL, fragment); // add transition from start to fragment
        addTransition(fragment->pair, NULL, end); // add transition from fragment to end
        return start;
    }
    // 7) Wildcard: WILD ← “.”
    else if (strcmp(node->type, "WILD") == 0) {
        addTransition(start, "*.*", end);
        return start;
    }
    if(strcmp(node->type, "LITERAL") == 0) {
        State* left_state = generateStates(node->left,symbolTable);
        addTransition(start, NULL, left_state); // Transition for literal
        
        State* right_state = generateStates(node->right,symbolTable);
        addTransition(left_state->pair, NULL, right_state); // Transition from left to right because left is always defined when right is defined
        addTransition(right_state->pair, NULL, end); // Transition to end state
        return start;
    }
    // else if(strcmp(node->type, "ID") == 0 || strcmp(node->type, "PLUS") == 0 || strcmp(node->type, "MINUS") == 0 || strcmp(node->type, "RBIG") == 0 
    //         || strcmp(node->type, "CONST") == 0 || strcmp(node->type, "EQUAL") == 0 || strcmp(node->type, "AMP") == 0 || strcmp(node->type, "NOT") == 0
    //         || strcmp(node->type, "LPAR") == 0 || strcmp(node->type, "RPAR") == 0 || strcmp(node->type, "PIPE") == 0 || strcmp(node->type, "QUES") == 0
    //         || strcmp(node->type, "LBIG") == 0 || strcmp(node->type, "ESC") == 0 || strcmp(node->type, "ASTRK") == 0 || strcmp(node->type, "DOT") == 0
    //         || strcmp(node->type, "LCUR") == 0 || strcmp(node->type, "RCUR") == 0 || strcmp(node->type, "OTHERS") == 0 || strcmp(node->type, "UNICODE") == 0
    //         ) 
    else
        {
        addTransition(start, node->value, end); // Start to end connected through the string value
        return start;
    }
    return start;
}
void headerCode(FILE *file); // forward declaration

void generateParseCode(ASTNode *node, FILE *file, Symbol *symbolTable) {
    State *start = generateStates(node,symbolTable);
    start->pair->is_accept = 1; // set the end state as accept state
    headerCode(file); 
}

void headerCode(FILE *file) {
    // 1) Includes & typedefs
    fprintf(file,
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n\n"

        "typedef struct State State;\n"
        "typedef struct Transition Transition;\n\n"

        "struct Transition {\n"
        "    char *match;         // NULL = epsilon, \"*.*\" = wildcard, else literal\n"
        "    State *to;\n"
        "    Transition *next;\n"
        "};\n\n"

        "struct State {\n"
        "    int id;\n"
        "    int is_accept;\n"
        "    Transition *transitions;\n"
        "};\n\n"
    );

    // 2) Declare one State variable per state
    for (State *s = all_states; s; s = s->next) {
        fprintf(file, "State s%d;\n", s->id);
    }
    fprintf(file, "\n");

    // 3) Declare one Transition variable per transition
    for (State *s = all_states; s; s = s->next) {
        for (Transition *t = s->transitions; t; t = t->next) {
            if (t->match)
                fprintf(file, "Transition t_%d_%p = { \"%s\", &s%d, NULL };\n",
                        s->id, (void*)t, t->match, t->to->id);
            else
                fprintf(file, "Transition t_%d_%p = { NULL, &s%d, NULL };\n",
                        s->id, (void*)t, t->to->id);
        }
    }
    fprintf(file, "\n");

    // 4) setup(): initialize ids, is_accept, and link transitions
    fprintf(file, "void setup() {\n");
    for (State *s = all_states; s; s = s->next) {
        fprintf(file,
            "    s%d.id = %d;\n"
            "    s%d.is_accept = %d;\n"
            "    s%d.transitions = NULL;\n",
            s->id, s->id,
            s->id, s->is_accept,
            s->id
        );
        for (Transition *t = s->transitions; t; t = t->next) {
            fprintf(file,
                "    t_%d_%p.next = s%d.transitions;\n"
                "    s%d.transitions = &t_%d_%p;\n",
                s->id, (void*)t, s->id,
                s->id,
                s->id, (void*)t
            );
        }
    }
    fprintf(file, "}\n\n");

    // 5) NFA runner: state_list, add_state, epsilon closure, step, is_accepting, match
    fprintf(file,
        "State* state_list[1024];\n"
        "int state_count;\n\n"

        "void add_state(State* s) {\n"
        "    for (int i = 0; i < state_count; ++i)\n"
        "        if (state_list[i] == s) return;\n"
        "    state_list[state_count++] = s;\n"
        "}\n\n"

        "void add_epsilon_closure(State* s) {\n"
        "    add_state(s);\n"
        "    for (Transition* t = s->transitions; t; t = t->next)\n"
        "        if (t->match == NULL) add_epsilon_closure(t->to);\n"
        "}\n\n"

        "int step(const char* input, int* i, int len) {\n"
        "    for (int s = 0; s < state_count; ++s) {\n"
        "        for (Transition* t = state_list[s]->transitions; t; t = t->next) {\n"
        "            if (t->match) {\n"
        "                if (strcmp(t->match, \"*.*\") == 0) {\n"
        "                    if (*i < len) {\n"
        "                        add_epsilon_closure(t->to);\n"
        "                        (*i)++;\n"
        "                        return 1;\n"
        "                    }\n"
        "                } else {\n"
        "                    int m = strlen(t->match);\n"
        "                    if (*i + m <= len && strncmp(input + *i, t->match, m) == 0) {\n"
        "                        *i += m;\n"
        "                        add_epsilon_closure(t->to);\n"
        "                        return 1;\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    return 0;\n"
        "}\n\n"

        "int is_accepting() {\n"
        "    for (int i = 0; i < state_count; ++i)\n"
        "        if (state_list[i]->is_accept) return 1;\n"
        "    return 0;\n"
        "}\n\n"

        "int match(const char* input, State* start) {\n"
        "    int len = strlen(input);\n"
        "    state_count = 0;\n"
        "    add_epsilon_closure(start);\n"
        "    int i = 0;\n"
        "    while (i < len) {\n"
        "        if (!step(input, &i, len)) return 0;\n"
        "    }\n"
        "    return is_accepting();\n"
        "}\n\n"
    );

    // 6) main(): read file, run match, print result
    fprintf(file,
        "int main(int argc, char** argv) {\n"
        "    if (argc < 2) { fprintf(stderr, \"Usage: %%s <file>\\n\", argv[0]); return 1; }\n"
        "    setup();\n"
        "    FILE *f = fopen(argv[1], \"r\");\n"
        "    if (!f) { perror(\"fopen\"); return 1; }\n"
        "    fseek(f, 0, SEEK_END); long len = ftell(f);\n"
        "    fseek(f, 0, SEEK_SET);\n"
        "    char *buf = malloc(len + 1);\n"
        "    fread(buf, 1, len, f);\n"
        "    buf[len] = '\\0';\n"
        "    fclose(f);\n"
        "    if (match(buf, &s%d)) printf(\"ACCEPTS\\n\"); else printf(\"REJECTS\\n\");\n"
        "    free(buf);\n"
        "    return 0;\n"
        "}\n",
        all_states_head->id
    );
}


/*
void headerCode(FILE *file){
    fprintf(file, "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");
    fprintf(file,
        "typedef struct State State;\n"
        "typedef struct Transition Transition;\n"
        "struct Transition {\n"
        "    char* match;\n"
        "    State* to;\n"
        "    Transition* next;\n"
        "};\n"
        "struct State {\n"
        "    int id;\n"
        "    int is_accept;\n"
        "    Transition* transitions;\n"
        "};\n\n");

        for (State* s = all_states; s; s = s->next) {
            fprintf(file, "State s%d;\n", s->id);
        }
        fprintf(file, "\n");
        for (State* s = all_states; s; s = s->next) {
            for (Transition* t = s->transitions; t; t = t->next) {
                if(t->match){
                    fprintf(file, "Transition t_%d_%p = { \"%s\", &s%d, NULL };\n",
                    s->id, (void*)t, t->match, t->to->id);
                }
                else{
                    fprintf(file, "Transition t_%d_%p = { NULL, &s%d, NULL };\n",
                        s->id, (void*)t, t->to->id);
                }
            }
        }
        fprintf(file, "\nvoid setup() {\n");
        for (State* s = all_states; s; s = s->next) {
            fprintf(file, "    s%d.id = %d;\n", s->id, s->id);
            fprintf(file, "    s%d.is_accept = %d;\n", s->id, s->is_accept);
            fprintf(file, "    s%d.transitions = NULL;\n", s->id);
            for (Transition* t = s->transitions; t; t = t->next) {
                fprintf(file, "    t_%d_%p.next = s%d.transitions;\n", s->id, (void*)t, s->id);
                fprintf(file, "    s%d.transitions = &t_%d_%p;\n", s->id, s->id, (void*)t);
            }
        }
        fprintf(file, "}\n\n");

        fprintf(file,
            "State* state_list[1024];\n"
            "int state_count;\n\n"
    
            "void add_state(State* s) {\n"
            "    for (int i = 0; i < state_count; ++i)\n"
            "        if (state_list[i] == s) return;\n"
            "    state_list[state_count++] = s;\n"
            "}\n\n"
    
            "void add_epsilon_closure(State* s) {\n"
            "    add_state(s);\n"
            "    for (Transition* t = s->transitions; t; t = t->next)\n"
            "        if (t->match == NULL) add_epsilon_closure(t->to);\n"
            "}\n\n"
    
            "int step(const char* input, int* i, int len) {\n"
            "    State* new_states[1024];\n"
            "    int new_count = 0;\n"
            "    for (int s = 0; s < state_count; ++s) {\n"
            "        for (Transition* t = state_list[s]->transitions; t; t = t->next) {\n"
            "            if (t->match) {\n"
            "                    if (strcmp(t->match, \"*.*\") == 0) {\n"
            "                        // WILD: match any single character\n"
            "                        if (*i < len) {\n"
            "                            add_epsilon_closure(t->to);\n"
            "                            return 1;\n"
            "                        }\n"
            "                    } else {\n"
            "                    int match_len = strlen(t->match);\n"
            "                    if (*i + match_len <= len && strncmp(&input[*i], t->match, match_len) == 0) {\n"
            "                        // Move input index forward by match_len\n"
            "                        *i += match_len - 1;  // -1 because the main loop will increment it\n"
            "                        add_epsilon_closure(t->to);\n"
            "                        return 1;\n"
            "                    }\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "    return 0;\n"
            "}\n\n"
    
            "int is_accepting() {\n"
            "    for (int i = 0; i < state_count; ++i)\n"
            "        if (state_list[i]->is_accept) return 1;\n"
            "    return 0;\n"
            "}\n\n"
    
            "int match(const char* input, State* start) {\n"
            "    int len = strlen(input);\n"
            "    state_count = 0;\n"
            "    add_epsilon_closure(start);\n"
            "    int i;\n"
            "    for (i = 0; i<len; ++i) {\n"
            "        if(!step(input,&i,len)){\n"
            "            return 0;\n"
            "        }\n"
            "    }\n"
            "    return is_accepting(); \n"
            "}\n\n"
    
            "int main(int argc, char** argv) {\n"
            "    if (argc < 2) { printf(\"Usage: %%s <file>\\n\", argv[0]); return 1; }\n"
            "    setup();\n"
            "    FILE *f = fopen(argv[1], \"r\");\n"
            "    if (!f) { perror(\"fopen\"); return 1; }\n"
            "    fseek(f, 0, SEEK_END);\n"
            "    long len = ftell(f);\n"
            "    fseek(f, 0, SEEK_SET);\n"
            "    char *buf = malloc(len + 1);\n"
            "    fread(buf, 1, len, f);\n"
            "    buf[len] = '\\0';\n"
            "    fclose(f);\n"
            "    if (match(buf, &s%d)) printf(\"ACCEPTS\\n\");\n"
            "    else printf(\"REJECTS\\n\");\n"
            "    return 0;\n"
            "}\n", all_states_head->id);
}*/