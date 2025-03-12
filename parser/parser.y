/*
    This is the parser file which defines the grammar for our custom regular expression.
*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "astlib.h" // library to define special structs and functions


int yylex();

extern FILE *yyin;

//custom error message
struct errorCode{
    int code;
    char *msg;
};


int lineCount=1; // to store the line that we are processing and display in error message
struct errorCode code[] = {
    {0,"No error"},
    {1,"Unknown Error"},
    {2, "Unexpected ^, &, *, +, ? or |"},
    {3, "Check for unmatched \", (, [, { OR unexpected ^ or %"},
    {4, "Undefined identifier"},
    {5, "Invalid Unicode escape"},
    {6, "Error using const format or missing SLASH"}
}; // couldn't fix this to show different codes due to R/R conflict. So, use last for default as of now

// Error handling
void yyerror(const char *);
void clearYylval(); // function to clear yylval after every token to avoid memory leaks

int debugging=0; // make debug variable so that we can print when we need to

Symbol *symbolTable = NULL; // declare symbol table to be empty

%}


%union{
    struct ASTNode *node; // nodes to define each non terminal for AST
    char *str;
}
// list of all available tokens from lexer and make them string to print while debugging
%token <str> SLASH CONST_TOK EQUAL AMP NOT LPAR RPAR PLUS PIPE ASTRK ESC PERCENT
%token <str> QUES UNICODE QUOTE LBIG RBIG CAP WILD LCUR RCUR MINUS OTHERCHAR 
%token <str> ID;

/* list of all non terminals used in the parser. Some might differ from the assignment as they have
 been added to hold additional grammar logic */
%type <node> system definition rootregex seq regex term multiregterm regterm anychar multiliteral literal
   alt repeat range substitute wild;

// precedence and associativity of the operators (tokens)
%left NOT AMP
%left PIPE
%left ASTRK PLUS QUES 

// define line as the start non terminal
%start line

%%

/*To support multiple tests in file
 a single line or multiple line */
line: system {
        if(debugging){ // print the Abstract Syntax Tree for debugging
            printf("%d:",lineCount); 
            printAST($1,0); // print the AST
        }
        freeAST($1); // free the AST
    }
    | line system {
        lineCount++; //increase linecount everytime we read a new line
        if(debugging){ // print the Abstract Syntax Tree for debugging
            printf("%d:",lineCount); 
            printAST($2,0);
        }
        freeAST($2); // free the AST
    }
    | error { 
        yyerror(code[1].msg); 
        yyerrok; 
        return 1; // returns 1 to report error to main
    };

// System     := Definition* '/' RootRegex '/'
system: SLASH rootregex SLASH { // the case of no definition and regex in form / RootRegex /
        $$ = createNode("SYSTEM",NULL,$2,NULL); // create a regex start
    } 
    | definition system{ // for one or more definition i.e. const ID = / regex / / RootRegex /
        $$ = createNode("SYSTEM",NULL,$1,$2); // create a regex start
    }; 

definition: CONST_TOK ID EQUAL SLASH regex SLASH{ // definition in the form of "const ID = /regex/"
        insertSymbol($2,&symbolTable); // pass by reference to update global
        ASTNode *id= createNode("ID",$2,NULL,NULL); // create a node for ID
        $$ = createNode("DEFINITION",NULL,id,$5); // create DEFINITION node with id as value
        free($2); // free the ID as it is already stored in symbol table
    };

rootregex: rootregex AMP rootregex { // For RootRegex = RootRegex & RootRegex
        $$ = createNode("ROOTREGEX", "&", $1, $3); // amp node
    }
    | NOT alt { // For RootRegex = ! Regex (used alt to match precedence)
        $$ = createNode("NOT REGEX", NULL, $2, NULL);
    }
    | alt { // For RootRegex = Regex (used alt to match precedence)
        $$ = createNode("REGEX", NULL, $1, NULL);
    };

alt: seq { // For Regex = seq, kept here to match precedence of seq over alt
        $$= createNode("SEQ",NULL,$1,NULL);
    }
    | alt PIPE seq { /* For alt = Regex | Regex, where we group the first(alt) and second(seq) before |
        Here, alt PIPE is done for multiple PIPE in sequence and seq represents one or more regex
         since seq has higher precedence than alt */
        $$ = createNode("ALT", $2, $1, $3);
    };

//For Regex = seq
seq: regex { // For only one Regex
        $$ = createNode("REGEX",NULL,$1,NULL);
    }
    | seq regex { // For more than one regex
        $$ = createNode("SEQ", NULL, $1, $2);
    };

regex: term { // For Regex = term
        $$ = createNode("TERM",NULL,$1,NULL);
        clearYylval();
    } 
    | LPAR alt RPAR { // For Regex = ( Regex ), used alt because alt is the highest level making ( ) higher precedence
        $$ = createNode("(REGEX)"," () ",$2,NULL);
    }
    | repeat { // Regex = repeat (always has higher precedence than seq)
        $$ = createNode("REPEAT",NULL,$1,NULL);
    }; 

// Three cases of repeat with *, + and ?
repeat: regex ASTRK { 
        $$ = createNode("*", $2, $1, NULL);
    }
    | regex PLUS { 
        $$ = createNode("+", $2, $1, NULL);
    }
    | regex QUES { 
        $$ = createNode("?", $2, $1, NULL);
    };

// term = literal | range | wild | substitute
term: QUOTE multiliteral QUOTE { // For term = literal (used multiliteral to handle multiple characters in quotes)
        $$ = createNode("LITERAL", NULL, $2, NULL);
    }
    | range { // For term = range i.e. inside []
        $$ = createNode("RANGE",NULL,$1,NULL);
    }
    | wild { //For term ='.'
        $$ = createNode("WILD",NULL,$1,NULL);
    }
    | substitute { // For term = ${ }
        if (!checkSymbol($1->value,symbolTable)) { // check if the ID is defined in symbol table
            yyerror(code[4].msg); // print error message
            freeAST($1);
            return 1;
        }
        $$ = createNode("SUBSTITUTE", "${ }",$1,NULL);
    }
    | error { 
        yyerror(code[4].msg); yyerrok; return 1;
    };

range: LBIG multiregterm RBIG { // Range = [ ] with no ^
        $$ = createNode("[]", NULL, $2, NULL);
    }
    | LBIG CAP multiregterm RBIG { // Range = [^ ]
        $$ = createNode("[^]", NULL, $3, NULL);
    };

wild: WILD { // i.e. '.' 
        $$ = createNode("WILD",$1,NULL,NULL);
    };

substitute: LCUR ID RCUR { // case of ${ }
        $$ = createNode("ID", $2, NULL, NULL); 
    };

// for one or more characters in range i.e. [ ]
multiregterm: regterm { // only one character inside range
        $$ = createNode("REGTERM",NULL,$1,NULL);
    };
    | multiregterm regterm { //more than one characters
        $$ = createNode("REGTERM", NULL, $1, $2);
    };

// any single character inside range [ ]
regterm: anychar { // for characters which are not part of tokens eg: #, @,`, etc which are still usable
        $$ = createNode("ANYCHAR",NULL,$1,NULL);
    }
    | ESC RBIG { // used \] to use ] or can use unicode but question mentions only for literals
        $$ = createNode("RBIG",$1,NULL,NULL);
    } 
    | QUOTE {  // [ " ] use of quote inside [ ]
        $$ = createNode("\"",$1,NULL,NULL);
    }
    | PERCENT { // % needs to be escaped in literals but is not compulsory for range. So, use the % character
        $$ = createNode("%",$1,NULL,NULL);
    }; 

// multiple characters inside double quotes
multiliteral: literal { // for only one character inside " "
        $$ = createNode("LITERAL",NULL,$1,NULL);
    }
    | multiliteral literal { // for multiple characters inside " "
        $$ = createNode("LITERAL", NULL, $1, $2);
    };

literal: anychar { // represents all characters that are possible inside " " except ], " and %
        $$ = createNode("ANYCHAR",NULL,$1,NULL);
    }
    /* | ESC QUOTE { $$=malloc(strlen($2)+2); sprintf($$,"\\\"",$2);} //this works too \" but used unicode */
    | RBIG { // ] since it is not part of anychar
        $$= createNode("]",$1,NULL,NULL); 
    };

// includes all the tokens defined which can exist inside literals or range too
anychar: PLUS { $$= createNode("PLUS",$1,NULL,NULL);  } // '+'
    | MINUS { $$= createNode("MINUS",$1,NULL,NULL);  } // '-'
    | CONST_TOK { $$= createNode("CONST",$1,NULL,NULL); } // 'const'
    | EQUAL { $$= createNode("EQUAL",$1,NULL,NULL); } // '='
    | AMP { $$= createNode("AMP",$1,NULL,NULL); } // '&'
    | NOT { $$= createNode("NOT",$1,NULL,NULL); } // '!'
    | LPAR { $$= createNode("LPAR",$1,NULL,NULL); } // '('
    | RPAR { $$= createNode("RPAR",$1,NULL,NULL); } // ')'
    | PIPE { $$= createNode("PIPE",$1,NULL,NULL); } // '|'
    | QUES { $$= createNode("QUES",$1,NULL,NULL); } // '?'
    | LBIG { $$= createNode("LBIG",$1,NULL,NULL); } // '['
    | ESC ESC { $$= createNode("ESC",$1,NULL,NULL); } // '\\'
    | ASTRK { $$= createNode("ASTRK",$1,NULL,NULL); } // '*'
    | WILD {  $$ = createNode("WILD",$1,NULL,NULL); }; // '.'
    | LCUR { $$= createNode("LCUR",$1,NULL,NULL); } // '${'
    | RCUR { $$= createNode("RCUR",$1,NULL,NULL); } // '}'
    | ID { $$= createNode("ID",$1,NULL,NULL); clearYylval();} // alphanumeric tokens
    | OTHERCHAR { $$= createNode("OTHERS",$1,NULL,NULL); } // includes all other characters except tokens
    | UNICODE { 
        // Validate Unicode escape
        long x=0;
        // extracting the number from the unicode. 2 represents the starting number and -1 represents the character before ";"
        for(int i=2; i<strlen($1)-1; i++){
            x = x*10 + (int)($1[i]- '0');
        }
        if (x<0 || x > 1114111 ) { // max unicode codepoint is 0x10FFFF which is 1114111 in decimal
            yyerror(code[5].msg); 
            return 1;
        }
        $$ = createNode("UNICODE", $1, NULL, NULL);
        clearYylval();
    }; // includes the unicode formatted

%%

void yyerror(const char *s){ // function to print error message{
    fprintf(stderr, "Line %d: Error: %s\n", lineCount,s);
}

void clearYylval(){ // function to clear yylval which is done after every token to avoid memory leaks
    if (yylval.str != NULL) {
        free(yylval.str);
        yylval.str = NULL;
    }
}

void cleanUp(){ // clean up the symbol table, file pointer and yylval at the end
    freeSymbolTable(symbolTable); // free the symbol table
    if(yyin){ // close file if opened
        fclose(yyin);
    }
    clearYylval(); // clear yylval
}

int main(int argc, char *argv[]) {
    if(argc == 3){ // check for third argument as debug
        int var = atoi(argv[2]); // the argument is considered as string so convert to int
        if(var==0 || var == 1){ //check if it is 1 or 0, else throw error
            debugging = var;
        }
        else{
            printf("Invalid debugging argument (1 or 0). Setting to 0 instead\n");
        }
    }
    if (argc >= 2) { // second argument is filepath 
        //open file if specified
        yyin = fopen(argv[1], "r");
        if (!yyin) { // exit if file doesn't exist or cannot open
            printf("Error opening file\n");
            exit(1);
        }
    }
    else{ // if no file is provided, take input manually
        printf("Please provide an input:\n");
    }
    if(yyparse()==0){ // if regular expression is correct, parser will return 0, else 1
        printf("accepts\n");
        if(debugging){
            printSymbolTable(symbolTable);
        }
        cleanUp(); // clean up at the end
        exit(0);
    }
    else{
        printf("Exiting due to error.\n");
        cleanUp(); // clean up at the end
        exit(1);
    }
    return 0;
}