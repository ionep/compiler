#include <stdio.h>
#include <stdlib.h>
#include<string.h>

struct ASTNode;
// Symbol Table Entry
typedef struct Symbol {
    char *name; //symbol
    struct Symbol *next; //next symbol
    struct ASTNode *node; // pointer to ASTNode
} Symbol;

// AST Node Structure
typedef struct ASTNode {
    char *type; //name for the node
    char *value; // value of the node
    struct ASTNode *left; //if sub-branches, then pointer to left sub node
    struct ASTNode *right; //if sub-branches, then pointer to right sub node
} ASTNode;


// Function to insert into symbol table. Use pointer to pointer to update on global
void insertSymbol(char *name, ASTNode *val, Symbol **symbolTable) {
    Symbol *newSymbol = (Symbol *)malloc(sizeof(Symbol)); // allocate size for Symbol
    if (!newSymbol) {
        printf("Memory allocation failed!\n");
        return;
    }
    if(val != NULL){ // add nodes to the symbol table if not NULL
        newSymbol->node = val; // save value of the node
    }
    newSymbol->name=strdup(name); // save name
    newSymbol->next = *symbolTable; // next symbol
    *symbolTable = newSymbol; //save to the original pointer
    //printf("Symbol inserted: %s \n",(*symbolTable)->name);
}

// Function to check if symbol exists
int checkSymbol(char *name, Symbol *symbolTable) {
    Symbol *current = symbolTable; // copy reference start of symbol table to current
    while (current) { // do until current is null
        if (strcmp(current->name, name) == 0){ //verify if the name of current and the check string is same
            return 1;
        }
        current = current->next; // if not, move to next symbol in the table
    }
    return 0; // return 0 when no symbol in the table matches check string
}

// Function to check if symbol exists
ASTNode* getSymbol(char *name, Symbol *symbolTable) {
    Symbol *current = symbolTable; // copy reference start of symbol table to current
    while (current) { // do until current is null
        if (strcmp(current->name, name) == 0){ //verify if the name of current and the check string is same
            return current->node;
        }
        current = current->next; // if not, move to next symbol in the table
    }
    return NULL; // return 0 when no symbol in the table matches check string
}
// Function to print the symbol table
void printSymbolTable(Symbol *table) {
    if(table == NULL){
        printf("Symbol Table is empty\n");    
        return;
    }
    Symbol *test = table;
    printf("\nSymbol Table:\n");
    printf("+----------------+\n");
    printf("| Identifier     |");
    printf(" Value          |\n");
    printf("+----------------+\n");

    while (test != NULL) {
        printf("| %-14s |", test->name);
        if(test->node->type){
            printf(" %-14s |\n", test->node->type);
        }
        test = test->next;
    }

    printf("+----------------+\n");
}

// Function to free the symbol table
void freeSymbolTable(Symbol *table) {
    if(table == NULL){
        return;
    }

    freeSymbolTable(table->next); // recursively free each symbol in the table

    if(table->name != NULL){ // free name if not NULL
        free(table->name);
        table->name=NULL;
    }
    
    free(table); // free the table
    table=NULL; // make sure to keep the table NULL to avoid dangling pointers
}


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


enum TYPE{ // define the types of transitions
    TYPE_DEFAULT,
    TYPE_WILDCARD,
    TYPE_UNICODE
};
struct Transition {
    char* match; // NULL = epsilon
    int type; // 0 = default, 1 = wildcard, 2 = unicode
    State* to;
    Transition* next; // linked list of transitions
};

struct State {
    int id;
    int is_accept;
    Transition* transitions;
    ASTNode* node; 
    State* pair; // end and start state pair
    State* next; 
};

//concat multiple NFA and NOTREGEX
#define MAX_SUBNFAS 100 // maximum number of sub NFAs
State* startStates[MAX_SUBNFAS];
int invertFlags[MAX_SUBNFAS]; //not accepting states
int startCount = 0; 


// Global state tracking
// State* all_states_tail = NULL;
State* all_states = NULL; // pointer to the first state
int state_id = 0;

State *existing_states[MAX_SUBNFAS*1024];
int noOfLiveStates = 0;

//for range states and transitions
int unicode = 0;
int minusEncountered = 0;



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


void freeStates(State *e[]) {
    for(int i = 0; i < noOfLiveStates; i++) {
        if(e[i] != NULL) { // free state if not NULL
            freeTransitions(e[i]->transitions); // free transitions of the state
            free(e[i]); // free the state
            e[i]=NULL; // make sure to keep the state NULL to avoid dangling pointers
        }
    }
}

State* createState(int is_accept) {
    State* s = (State *)malloc(sizeof(State));
    s->id = state_id++;
    s->is_accept = is_accept;
    s->transitions = NULL;
    noOfLiveStates++;
    s->next = all_states; 
    all_states = s; // set the current state to the new state
    return s;
}

void addTransition(State* from, char *match, State* to) {
    Transition* t = (Transition *)malloc(sizeof(Transition));
    t->match = (match!=NULL) ? strdup(match) : NULL; // copy the match string
    t->to = to;
    t->type= TYPE_DEFAULT;
    t->next = from->transitions;
    from->transitions = t;
}

void addTransitionReverse(State* from, char *match, State* to) {
    Transition* t = (Transition *)malloc(sizeof(Transition));
    t->match = (match!=NULL) ? strdup(match) : NULL; // copy the match string
    t->to = to;
    t->type= TYPE_DEFAULT;
    from->transitions->next = t;
}

void addTransitionWithType(State* from, char *match, int type, State* to) {
    Transition* t = (Transition *)malloc(sizeof(Transition));
    t->match = (match!=NULL) ? strdup(match) : NULL; // copy the match string
    t->to = to;
    t->type=type; 
    t->next = from->transitions;
    from->transitions = t;
}

char addRangeTransitions(ASTNode* node, State* start, State* end) {
    if (!node) return '\0'; 

    if (strcmp(node->type, "RANGE_VAL") == 0){
        if(strcmp(node->left->type,"RANGE_VAL") == 0) { // if there is more range values
        
            char low = addRangeTransitions(node->left, start, end); // recursively add transitions for the left side of the range

            
            if(low != '\0'){
                if(low == '\n'){ // if unicode range is consumed
                    minusEncountered = 0;
                    if(strcmp(node->right->type,"UNICODE")==0){
                        long hi;
                        sscanf(node->right->value, "%%x%lx;", &hi);
                        return (char)(int)hi;
                    }
                    else{
                        for (int i=0; i < strlen(node->right->value)-1; ++i) { // add all transitions but the last
                            char c = node->right->value[i];
                            char buf[2] = { c, '\0' };
                            addTransition(start, buf, end); 
                        }
                        return node->right->value[strlen(node->right->value) - 1]; // last character of right value
                    }
                }
                if(!minusEncountered){
                    if(strcmp(node->right->type,"MINUS") == 0){
                        minusEncountered=1;
                        return low;
                    }
                    if(unicode){
                        unicode=0;
                        char buf[12];
                        sprintf(buf, "%d", (int)low); // convert low to string
                        addTransitionWithType(start, buf, TYPE_UNICODE, end);
                        if(strcmp(node->right->type,"UNICODE")==0){
                            long hi;
                            unicode = 1;
                            sscanf(node->right->value, "%%x%lx;", &hi);
                            return (char)(int)hi;
                        }
                        else{
                            for (int i=0; i < strlen(node->right->value)-1; ++i) { // add all transitions but the last
                                char c = node->right->value[i];
                                char buf[2] = { c, '\0' };
                                addTransition(start, buf, end);
                            }
                            return node->right->value[strlen(node->right->value) - 1]; // last character of right value
                        }   
                    }
                    else{
                        char buf[2]={low, '\0'};
                        addTransition(start, buf, end);
                        if(strcmp(node->right->type,"UNICODE")==0){
                            long hi;
                            unicode = 1;
                            sscanf(node->right->value, "%%x%lx;", &hi);
                            return (char)(int)hi;
                        }
                        else{
                            for (int i=0; i < strlen(node->right->value)-1; ++i) { // add all transitions but the last
                                char c = node->right->value[i];
                                char buf[2] = { c, '\0' };
                                addTransition(start, buf, end);
                            }
                            return node->right->value[strlen(node->right->value) - 1]; // last character of right value
                        }
                    }
                }
                minusEncountered=0;
                if(!unicode && strcmp(node->right->type,"UNICODE")!=0){
                    char hi = node->right->value[0];
                    for (char c = low; c <= hi && low!='\n'; ++c) { //define range of transitions
                        char buf[2] = { c, '\0' };
                        addTransition(start, buf, end);
                    }
                    for (int i=1; i < strlen(node->right->value)-1; ++i) { // add all transitions but the last
                        char c = node->right->value[i];
                        char buf[2] = { c, '\0' };
                        addTransition(start, buf, end);
                    }
                    return node->right->value[strlen(node->right->value) - 1]; // last character of right value
                }
                else{
                    long hi;
                    int l = (int) low;
                    unicode=0;
                    minusEncountered = 0;
                    if(strcmp(node->right->type,"UNICODE")==0){
                        sscanf(node->right->value, "%%x%lx;", &hi);
                        for (int i = l; i <= hi; ++i) { //define range of transitions
                            char buf[12];
                            sprintf(buf, "%d", (int)i); // convert i to string
                            addTransitionWithType(start, buf, TYPE_UNICODE, end); // add transition to the start state
                        }
                        return '\n';
                    }
                    else{
                        hi = (int)node->right->value[0];
                        for (int i = l; i <= hi; ++i) { //define range of transitions
                            char buf[12];
                            sprintf(buf, "%d", (int)i); // convert i to string
                            addTransitionWithType(start, buf, TYPE_UNICODE, end); // add transition to the start state
                        }
                        for (int i=1; i < strlen(node->right->value)-1; ++i) { // add all transitions but the last (done as unicode as can be any characters)
                            char c = node->right->value[i];
                            char buf[12];
                            sprintf(buf, "%d", (int)i); // convert i to string
                            addTransitionWithType(start, buf, TYPE_UNICODE, end);
                        }
                        if(strlen(node->right->value)>1)
                            return node->right->value[strlen(node->right->value) - 1]; // last character of right value
                        else
                            return '\n';
                    }
                }
            }
            else{
                if(strcmp(node->right->type,"UNICODE")==0){
                    long i;
                    sscanf(node->right->value, "%%x%lx;", &i);
                    char buf[12];
                    sprintf(buf, "%d", (int)i); // convert i to string
                    addTransitionWithType(start, buf, TYPE_UNICODE, end);
                }
                else{
                    for (int i=0; i < strlen(node->right->value); ++i) { // add all transitions but the last#FIXME
                        char c = node->right->value[i];
                        char buf[2] = { c, '\0' };
                        addTransition(start, buf, end);
                    }
                }
                return '\0';
            }
        }
        else if(strcmp(node->left->type,"UNICODE")==0){
            unicode = 1;
            if(strcmp(node->right->type,"MINUS") == 0){
                long left;
                sscanf(node->left->value, "%%x%lx;", &left);
                minusEncountered = 1;
                return (char)(int)left; 
            }
            else{
                long left;
                sscanf(node->left->value, "%%x%lx;", &left);
                char buf[12];
                sprintf(buf, "%d", (int)left); // convert left to string
                addTransitionWithType(start, buf, TYPE_UNICODE, end);
                if(strcmp(node->right->type,"UNICODE")==0){
                    long i;
                    sscanf(node->right->value, "%%x%lx;", &i);
                    char buf[12];
                    sprintf(buf, "%d", (int)i); // convert i to string
                    addTransitionWithType(start, buf, TYPE_UNICODE, end);
                }
                
                else{
                    for (int i=0; i < strlen(node->right->value); ++i) { 
                        char c = node->right->value[i];
                        char buf[2] = { c, '\0' };
                        addTransition(start, buf, end);
                    }
                }
                return '\0';
            }
        }
        else{ // when left is a final value, we add transition to end unless it has minus in right
            for (int i=0; i < strlen(node->left->value)-1; ++i) {
                char c = node->left->value[i];
                char buf[2] = { c, '\0' };
                addTransition(start, buf, end);
            }
            if(strcmp(node->right->type,"MINUS") == 0){
                minusEncountered = 1;
                return node->left->value[strlen(node->left->value) - 1]; // last character of left value
            }
            else{
                char c = node->left->value[strlen(node->left->value) - 1];
                char buf[2] = { c, '\0' };
                addTransition(start, buf, end);
                return '\0';
            }
        }
    }
    else{
        if(strcmp(node->type,"UNICODE")==0){
            long i;
            sscanf(node->value, "%%x%lx;", &i);
            char buf[12];
            sprintf(buf, "%d", (int)i); // convert i to string
            unicode = 1;
            return (char)(int)i; // return the unicode value
            // addTransitionWithType(start, buf, TYPE_UNICODE, end);
        }
        else{
            unicode = 0;
            for (int i=0; i < strlen(node->value)-1; ++i) { 
                char c = node->value[i];
                char buf[2] = { c, '\0' };
                addTransition(start, buf, end);
            }
            return node->value[strlen(node->value) - 1]; // last character of left value
        }
        return '\0';
    }
}

Transition* repeatFound= NULL;
char repeatFoundChar = '\0'; // to store the character of the repeat found
State *topSeq = NULL;

void addSequenceTransitions(ASTNode* node, State *start, State *end, State *L, State *R) {
    if(!node) return;

    if(strcmp(node->type,"SEQ")==0){
        if(strcmp(node->left->type,"REPEAT")==0){
            if(strcmp(node->left->left->type,"WILD")==0){
                char buf[20]={'\0'};
                State *temp = R;
                for(Transition *t = temp->transitions; t; t = t->next) {
                    if(t->match){
                        // printf("Matched: %s\n",t->match);
                        strcpy(buf,t->match);
                        break;
                    }
                }
                // printf("buf:%s\n",buf);
                
                if(node->left->value[0] == '*'){
                    addTransition(L->transitions->next->to->pair, buf, R->pair); // add transition from L->pair to end
                    addTransitionReverse(L->transitions->next->to, buf, R->pair); // add transition from L->pair to end
                }
                else if(node->left->value[0] == '+'){
                    addTransition(L->transitions->to->pair, buf, R->pair); // add transition from L->pair to end
                }
            }
        }
        else{ // when repeat is in right or deeply rooted inside in right
            if(strcmp(node->right->type,"REPEAT")==0){
                if(strcmp(node->right->left->type,"WILD")==0){
                    repeatFound = R->transitions; // return the pointer to wild so that we set it in outer SEQ
                    repeatFoundChar = node->right->value[0]; // store the repeat character
                }
            }
        }
    }
}

State* generateStates(ASTNode* node, Symbol *symbolTable) {
    if (node == NULL) return NULL;

    State* start = createState(0);
    State* end = createState(0);

    start->pair = end; // pair the start and end states
    end->pair = start; // pair the end and start states

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
        //special case with wildcard repeat issue
        if(!topSeq) {
            topSeq = start; // set the top sequence state
        }
        State* L = generateStates(node->left,symbolTable);
        State* R = generateStates(node->right,symbolTable);
        addTransition(start,    NULL, L);
        addTransition(L->pair,  NULL, R);
        addTransition(R->pair,  NULL, end);
        start->node = node;

        addSequenceTransitions(node, start, end,L,R); 
        if(repeatFound != NULL && topSeq == start){ // i.e. if in some inner SEQ, repeat>wild was found
            topSeq = NULL; // reset the top sequence state
            if(repeatFoundChar == '*'){
                addTransition(repeatFound->next->to->pair, node->right->value, R->pair); 
                addTransitionReverse(repeatFound->next->to, node->right->value, R->pair); 
            }
            else if(repeatFoundChar == '+'){
                addTransition(repeatFound->to->pair, node->right->value, R->pair); 
            }
            repeatFound = NULL; // reset the repeat found pointer
            repeatFoundChar = '\0'; // reset the repeat found character
        }
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
        start->node = node;
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
        char c =addRangeTransitions(node->left, start, end); // add range transitions
        if (c != '\0' && c != '\n') {
            char buf[2] = { c, '\0' };
            addTransition(start, buf, end); // add transition for the last character
        }
        if(minusEncountered){
            addTransition(start, "-", end); // add transition for the last character
            minusEncountered=0;
        }
        unicode=0;
        return start;
    }
    else if (strcmp(node->type, "NEGRANGE") == 0) {
        State *sink = createState(0);
        addRangeTransitions(node->left, start, sink);
        addTransitionWithType(start, ".", TYPE_WILDCARD, end);
        // addRangeTransitions(node->left, start, end,1);
        return start;
    }
    // 7) Substitute: SUBSTITUTE ← ${ ID }
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
    // 8) Wildcard: WILD ← “.”
    else if (strcmp(node->type, "WILD") == 0) {
        addTransitionWithType(start, ".",TYPE_WILDCARD, end);
        start->node = node;
        return start;
    }
    else if(strcmp(node->type, "LITERAL") == 0) {
        State* left_state = generateStates(node->left,symbolTable);
        addTransition(start, NULL, left_state); // Transition for literal
        
        State* right_state = generateStates(node->right,symbolTable);
        addTransition(left_state->pair, NULL, right_state); // Transition from left to right because left is always defined when right is defined
        addTransition(right_state->pair, NULL, end); // Transition to end state
        return start;
    }
    else if(strcmp(node->type, "SYSTEM") == 0) {
        State *R = generateStates(node->right,symbolTable); // get the right node
        addTransition(start, NULL, R);
        addTransition(R->pair, NULL, end); // Transition to end state
        return start;
    }
    else if(strcmp(node->type, "CONCAT") == 0) {
        State *L = generateStates(node->left,  symbolTable);
        State *R = generateStates(node->right, symbolTable);
        if (startCount + 2 <= MAX_SUBNFAS) {
            if(L){
                startStates[startCount]    = L;
                startStates[startCount]->pair->is_accept=1;
                invertFlags[startCount++]  = 0;
            }
            if(R){
                startStates[startCount]    = R;
                startStates[startCount]->pair->is_accept=1;
                invertFlags[startCount++]  = 0;
            }
        }
        return NULL;
    }
    else if(strcmp(node->type, "NOTREGEX") == 0){
        State *inner = generateStates(node->left,symbolTable); // get the left node
        if(startCount+1 <=MAX_SUBNFAS){
            startStates[startCount] = inner; // add the inner state to the list of start states
            startStates[startCount]->pair->is_accept=1;
            invertFlags[startCount++] = 1; // set the invert flag for the inner state
        }
        return NULL;
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

void reorderWildcards() {
    for (State *s = all_states; s; s = s->next) {
        Transition *wildHead = NULL, *wildTail = NULL;
        Transition *otherHead = NULL, *otherTail = NULL;

        // Split into two lists, preserving the original relative order
        for (Transition *t = s->transitions; t; t = t->next) {
            if (t->type == TYPE_WILDCARD) {
                if (!wildHead) wildHead = wildTail = t;
                else {
                    wildTail->next = t;
                    wildTail = t;
                }
            } else {
                if (!otherHead) otherHead = otherTail = t;
                else {
                    otherTail->next = t;
                    otherTail = t;
                }
            }
        }

        // Terminate both lists
        if (wildTail)   wildTail->next   = NULL;
        if (otherTail) otherTail->next = NULL;

        // Rebuild s->transitions as: [wildcards] ++ [others]
        if (wildHead) {
            s->transitions = wildHead;
            wildTail->next = otherHead;
        } else {
            s->transitions = otherHead;
        }
    }
}

void headerCode(FILE *file); // forward declaration

void generateParseCode(ASTNode *node, FILE *file, Symbol *symbolTable) {

    State *start = generateStates(node,symbolTable);
    if(startCount == 0 && start){
        startStates[startCount++] = start; // add the start state to the list of start states
        start->pair->is_accept = 1; // set the end state as accept state
    }
    // reorderWildcards(); // reorder the wildcards in the state machine
    headerCode(file); 
}

void headerCode(FILE *file) {
    // 1) Include + struct definitions
    fprintf(file,
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n\n"
        "typedef struct State State;\n"
        "typedef struct Transition Transition;\n\n"
        "struct Transition {\n"
        "    char *match;       \n"
        "    State *to;\n"
        "    int type;\n"
        "    Transition *next;\n"
        "};\n\n"
        "struct State {\n"
        "    int id;\n"
        "    int is_accept;\n"
        "    Transition *transitions;\n"
        "};\n\n"
    );


    // 2) Declare State variables
    for (State *s = all_states; s; s = s->next) {
        fprintf(file, "State s%d;\n", s->id);
    }
    fprintf(file, "\n");

    // 3) Declare Transition variables
    for (State *s = all_states; s; s = s->next) {
        for (Transition *t = s->transitions; t; t = t->next) {
            if (t->match)
                fprintf(file,
                    "Transition t_%d_%p = { \"%s\", &s%d, %d, NULL };\n",
                    s->id, (void*)t, t->match, t->to->id, t->type);
            else
                fprintf(file,
                    "Transition t_%d_%p = { NULL, &s%d, %d, NULL };\n",
                    s->id, (void*)t, t->to->id, t->type);
        }
    }
    fprintf(file, "\n");


    fprintf(file,
        "int startCount = %d;\n",
        startCount
    );

    fprintf(file,
        "State *startStates[%d] = {", startCount);
    for (int i = 0; i < startCount; i++) {
        fprintf(file, "&s%d%s",
            startStates[i]->id,
            (i + 1 < startCount ? ", " : "" )
        );
    }
    fprintf(file, "};\n");

    fprintf(file,
        "int invertFlags[%d] = {", startCount);
    for (int i = 0; i < startCount; i++) {
        fprintf(file, "%d%s",
            invertFlags[i],
            (i + 1 < startCount ? ", " : "" )
        );
    }
    fprintf(file, "};\n\n");

    // 4) setup(): initialize states and link transitions
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

    // 5) NFA runner: single‐pass step() + match()
    fprintf(file,
        "// active states frontier\n"
        "State *state_list[1024];\n"
        "int state_count;\n\n"

        "// add a state to a list if not already present\n"
        "void add_state_to(State **list, int *count, State *s) {\n"
        "    for (int i = 0; i < *count; ++i)\n"
        "        if (list[i] == s) return;\n"
        "    list[(*count)++] = s;\n"
        "}\n\n"

        "// epsilon‐closure into an arbitrary list\n"
        "void add_epsilon_closure_to(State *s, State **list, int *count) {\n"
        "    add_state_to(list, count, s);\n"
        "    for (Transition *t = s->transitions; t; t = t->next) {\n"
        "        if (t->match == NULL)\n"
        "            add_epsilon_closure_to(t->to, list, count);\n"
        "    }\n"
        "}\n\n"

        "// consume exactly one chunk from input[*i] and build next_states\n"
        "int step(const char *input, int *i, int len) {\n"
        "    State *next_states[1024];\n"
        "    int next_count = 0;\n"
        "    int consumed = 0;\n\n"
        "    // For each currently active state\n"
        "    for (int si = 0; si < state_count && !consumed; ++si) {\n"
        "        State *s = state_list[si];\n"
        "        for (Transition *t = s->transitions; t && !consumed; t = t->next) {\n"
        "            if (!t->match) continue;\n"
        "            if (t->type == 1) {\n"
        "                // wildcard: any single char\n"
        "                if (*i < len) {\n"
        "                    consumed = 1;\n"
        "                    add_epsilon_closure_to(t->to, next_states, &next_count);\n"
        "                }\n"
        "            } else if (t->type == 2) {\n"
        "                // validate unicode of given char\n"
        "                if (input[*i] == (char)atoi(t->match)) {\n"
        "                    consumed = 1;\n"
        "                    add_epsilon_closure_to(t->to, next_states, &next_count);\n"
        "                }\n"
        "            } else {\n"
        "                int m = strlen(t->match);\n"
        "                if (*i + m <= len && strncmp(input + *i, t->match, m) == 0) {\n"
        "                    consumed = m;\n"
        "                    add_epsilon_closure_to(t->to, next_states, &next_count);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n\n"
        "    if (!consumed) return 0;\n\n"
        "    // Commit next_states → state_list\n"
        "    state_count = next_count;\n"
        "    memcpy(state_list, next_states, next_count * sizeof(State *));\n"
        "    *i += consumed;\n"
        "    return consumed;\n"
        "}\n\n"

        "// Run the matcher in exactly one pass over the input\n"
        "int match(const char *input, State *start) {\n"
        "    int len = strlen(input);\n"
        "    state_count = 0;\n"
        "    add_epsilon_closure_to(start, state_list, &state_count);\n"
        "    int i = 0;\n"
        "    while (i < len) {\n"
        "        if (!step(input, &i, len)) return 0;\n"
        "    }\n"
        "    // Accept if any remaining state is accepting\n"
        "    for (int si = 0; si < state_count; ++si){\n"
        "        if (state_list[si]->is_accept == 1) return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}\n\n"
    );

    fprintf(file,
        "int main(int argc, char **argv) {\n"
        "    if (argc < 2) { fprintf(stderr, \"Usage: %%s <file>\\n\", argv[0]); return 1; }\n"
        "    setup();\n"
        "    FILE *f = fopen(argv[1], \"r\"); if (!f) { perror(\"fopen\"); return 1; }\n"
        "    fseek(f, 0, SEEK_END); long len = ftell(f);\n"
        "    fseek(f, 0, SEEK_SET);\n"
        "    char *buf = malloc(len + 1);\n"
        "    fread(buf, 1, len, f);\n"
        "    buf[len] = '\\0'; fclose(f);\n"
        "    int result = 1;\n"
        "    for (int i = 0; i < %d; i++) {\n"
        "        int m = match(buf, startStates[i]);\n"
        "        if (invertFlags[i]) m = !m;\n"
        "        if (!m) { result = 0; break; }\n"
        "    }\n"
        "    if (result) printf(\"ACCEPTS\\n\"); else printf(\"REJECTS\\n\");\n"
        "    free(buf);\n"
        "    return 0;\n"
        "}\n",
        startCount);
}
