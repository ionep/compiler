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