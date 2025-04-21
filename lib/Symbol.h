// Symbol Table Entry
typedef struct Symbol {
    char *name; //symbol
    struct Symbol *next; //next symbol
    ASTNode *node; // pointer to ASTNode
} Symbol;

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