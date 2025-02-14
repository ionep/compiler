/*
    This is the parser file which defines the grammar for our custom regular expression.
*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex();
extern char *yytext;
extern FILE *yyin;

//custom error message
struct errorCode{
    int code;
    char *msg;
};
int lineCount=1; // to store the line that we are processing and display in error message
struct errorCode code[] = {
    {0,"No error"},
    {1,"Error using const format or missing SLASH"},
    {2, "Unexpected ^, &, *, +, ? or |"},
    {3, "Check for unmatched \", (, [, { OR unexpected ^, % or unicode"}
}; // couldn't fix this to show different codes due to R/R conflict. So, use last for default as of now
int errID=0; // id to index the code[] array

// Error handling
void yyerror(const char *);

int debugging=0; // make debug variable so that we can print when we need to

%}


%union{
    char *str;
}
// list of all available tokens from lexer and make them string to print while debugging
%token <str> SLASH CONST EQUAL AMP NOT LPAR RPAR PLUS PIPE ASTRK ESC PERCENT
%token <str> QUES UNICODE QUOTE LBIG RBIG CAP WILD LCUR RCUR MINUS OTHERCHAR 
%token <str> ID;

/* list of all non terminals used in the parser. Some might differ from the assignment as they have
 been added to hold additional grammar logic */
%type <str> system definition rootregex seq regex term multiregterm regterm anychar multiliteral literal
   alt repeat range wild substitute;

// precedence and associativity of the operators (tokens)
%left NOT AMP
%left PIPE
%left ASTRK PLUS QUES 

// define line as the start non terminal
%start line
/* %left MINUS
%left LCUR RCUR
%left LBIG RBIG
%left LPAR RPAR */

%%

/*To support multiple tests in file
 a single line or multiple line */
line: system
    | line system
    | error { yyerror("Syntax error"); yyerrok; return 1;};

// System     := Definition* '/' RootRegex '/'
system: SLASH rootregex SLASH { // the case of no definition and regex in form / RootRegex /
        lineCount++; //increase linecount everytime we read a new line
        if(debugging){ // print for debugging
            printf("%d: /%s/\n",lineCount,$2); 
        }
    } 
    | definition system{ // for one or more definition i.e. const ID = / regex / / RootRegex /
        lineCount++; 
        if(debugging){ // print for debugging
            printf("%d: %s%s",lineCount,$1,$2); 
        }
    }; 

definition: CONST ID EQUAL SLASH regex SLASH{ // definition in the form of "const ID = /regex/"
        // allocate memory for $$ and return it with the required string to output at system
        // will not be used when debugging is 0 since it wont be printed
        if (debugging){
            $$ = malloc(strlen($2)+10); 
            sprintf($$,"const %s = ",$2); 
        }
    };

rootregex: rootregex AMP rootregex { // For RootRegex = RootRegex & RootRegex
        if (debugging){
            $$ = malloc(strlen($1)+strlen($3)+4); 
            sprintf($$,"%s & %s",$1,$3); 
        }
    }
    | NOT alt { // For RootRegex = ! Regex (used alt to match precedence)
        if (debugging){
            $$ = malloc(strlen($2) + 2); 
            sprintf($$,"!%s",$2);
        }
    }
    | alt { // For RootRegex = Regex (used alt to match precedence)
        if (debugging){
            $$ = malloc(strlen($1)+ 2); 
            sprintf($$,"%s",$1);
        }
    };

alt: seq { // For Regex = seq, kept here to match precedence of seq over alt
        if (debugging){
            $$=malloc(strlen($1)+5); 
            sprintf($$," ^%s^ ",$1);
        }
    }
    | alt PIPE seq { /* For alt = Regex | Regex, where we group the first(alt) and second(seq) before |
        Here, alt PIPE is done for multiple PIPE in sequence and seq represents one or more regex
         since seq has higher precedence than alt */
        if (debugging){
            $$=malloc(strlen($1)+strlen($3)+11); 
            sprintf($$," @%s | ^%s^ @ ",$1,$3);
        }
    };

//For Regex = seq
seq: regex { // For only one Regex
        if (debugging){
            $$ = malloc(strlen($1)+ 5); 
            sprintf($$,"%s",$1);  
        }
    }
    | seq regex { // For more than one regex
        if (debugging){
            $$ = malloc(strlen($1)+strlen($2)+ 1); 
            sprintf($$,"%s%s",$1,$2); 
        }
    };

regex: term { // For Regex = term
        if (debugging){
            $$ = malloc(strlen($1)+ 1); 
            sprintf($$,"%s",$1);  
        }
    } 
    | LPAR alt RPAR { // For Regex = ( Regex ), used alt because alt is the highest level making ( ) higher precedence
        if (debugging){
            $$ = malloc(strlen($2)+ 3); 
            sprintf($$,"(%s)",$2);  
        }
    }
    | repeat { // Regex = repeat (always has higher precedence than seq)
        if (debugging){
            $$ = malloc(strlen($1)+5); 
            sprintf($$," #%s# ",$1); 
        }
    }; 

// Three cases of repeat with *, + and ?
repeat: regex ASTRK { 
        if (debugging){
            $$ = malloc(strlen($1)+ 2); 
            sprintf($$,"%s*",$1); 
        }
    }
    | regex PLUS { 
        if (debugging){
            $$ = malloc(strlen($1)+ 2); 
            sprintf($$,"%s+",$1); 
        }
    }
    | regex QUES { 
        if (debugging){
            $$ = malloc(strlen($1)+ 2); 
            sprintf($$,"%s?",$1);  
        }
    };

// term = literal | range | wild | substitute
term: QUOTE multiliteral QUOTE { // For term = literal (used multiliteral to handle multiple characters in quotes)
        if (debugging){
            $$ = malloc(strlen($2)+ 5); 
            sprintf($$,"\"%s\"",$2);
        }
    }
    | range { // For term = range i.e. inside []
        if (debugging){
            $$ = malloc(strlen($1)+ 1); 
            sprintf($$,"%s",$1);  
        }
    }
    | wild { //For term ='.'
        if (debugging){
            $$ = malloc(strlen($1)+ 1); 
            sprintf($$,"%s",$1);  
        }
    }
    | substitute { // For term = ${ }
        if (debugging){
            $$ = malloc(strlen($1)+ 4); 
            sprintf($$,"${%s}",$1); 
        }
    }
    | error { 
        errID=3; yyerror("Error"); yyerrok; return 1;
    };

range: LBIG multiregterm RBIG { // Range = [ ] with no ^
        if (debugging){
            $$=malloc(strlen($2)+3); 
            sprintf($$,"[%s]",$2); 
        }
    }
    | LBIG CAP multiregterm RBIG { // Range = [^ ]
        if (debugging){
            $$=malloc(strlen($3)+4); 
            sprintf($$,"[^%s]",$3);
        }
    };

wild: WILD { // i.e. '.' 
        if (debugging){
            $$=malloc(2); 
            sprintf($$,".");
        }
    };

substitute: LCUR ID RCUR { // case of ${ }
        if (debugging){
            $$ = malloc(strlen($2)+4);
            sprintf($$,"%s",$2);
        }
    };

// for one or more characters in range i.e. [ ]
multiregterm: regterm { // only one character inside range
        if (debugging){
            $$=malloc(strlen($1)+1); 
            sprintf($$,"%s",$1);  
        }
    };
    | multiregterm regterm { //more than one characters
        if (debugging){
            $$=malloc(strlen($1)+strlen($2)+1); 
            sprintf($$,"%s%s",$1,$2);
        }
    };

// any single character inside range [ ]
regterm: anychar { // for characters which are not part of tokens eg: #, @,`, etc which are still usable
        if (debugging){
            $$=malloc(strlen($1)+1); 
            sprintf($$,"%s",$1); 
        }
    }
    | ESC RBIG { // used \] to use ] or can use unicode but question mentions only for literals
        if (debugging){
            $$=malloc(2); 
            sprintf($$,"]"); 
        }
    } 
    | QUOTE {  // [ " ] use of quote inside [ ]
        if (debugging){
            $$=malloc(3); 
            sprintf($$,"\""); 
        }
    }
    | PERCENT { // % needs to be escaped in literals but is not compulsory for range. So, use the % character
        if (debugging){
            $$=malloc(2); 
            sprintf($$,"%%");
        }
    }; 

// multiple characters inside double quotes
multiliteral: literal { // for only one character inside " "
        if (debugging){
            $$=malloc(strlen($1)+1); 
            sprintf($$,"%s",$1); 
        }
    }
    | multiliteral literal { // for multiple characters inside " "
        if (debugging){
            $$=malloc(strlen($1)+strlen($2)+1); 
            sprintf($$,"%s%s",$1,$2); 
        }
    };

literal: anychar { // represents all characters that are possible inside " " except ], " and %
        if (debugging){
            $$=malloc(strlen($1)+1); 
            sprintf($$,"%s",$1); 
        }
    }
    /* | ESC QUOTE { $$=malloc(strlen($2)+2); sprintf($$,"\\\"",$2);} */ //this works too \" but used unicode
    | RBIG { // ] since it is not part of anychar
        if (debugging){
            $$=malloc(2); 
            sprintf($$,"]");
        }
    };

// includes all the tokens defined which can exist inside literals or range too
anychar: PLUS { if (debugging){$$=malloc(2); sprintf($$,"+");} } // '+'
    | MINUS { if (debugging){$$=malloc(2); sprintf($$,"-");} } // '-'
    | CONST { if (debugging){$$=malloc(5); sprintf($$,"const");} } // 'const'
    | EQUAL { if (debugging){$$=malloc(2); sprintf($$,"=");} } // '='
    | AMP { if (debugging){$$=malloc(2); sprintf($$,"&");} } // '&'
    | NOT { if (debugging){$$=malloc(2); sprintf($$,"!");} } // '!'
    | LPAR { if (debugging){$$=malloc(2); sprintf($$,"(");}} // '('
    | RPAR { if (debugging){$$=malloc(2); sprintf($$,")");}} // ')'
    | PIPE { if (debugging){$$=malloc(2); sprintf($$,"|");}} // '|'
    | QUES { if (debugging){$$=malloc(2); sprintf($$,"?");}} // '?'
    | LBIG { if (debugging){$$=malloc(2); sprintf($$,"[");}} // '['
    | ESC ESC { if (debugging){$$=malloc(3); sprintf($$,"\\");}} // '\\'
    | ASTRK { if (debugging){$$=malloc(2); sprintf($$,"*");}} // '*'
    | WILD { if (debugging){$$=malloc(2); sprintf($$,".");} } // '.'
    | LCUR { if (debugging){$$=malloc(3); sprintf($$,"${");}} // '${'
    | RCUR { if (debugging){$$=malloc(2); sprintf($$,"}");} } // '}'
    | ID { if (debugging){$$=malloc(strlen($1)+1); sprintf($$,"%s",$1);}} // alphanumeric tokens
    | OTHERCHAR { if (debugging){$$=malloc(strlen($1)+1); sprintf($$,"%s",$1);}} // includes all other characters except tokens
    | UNICODE { if (debugging){$$=malloc(strlen($1)+1); sprintf($$,"%s",$1);} }; // includes the unicode formatted

%%
void yyerror(const char *s) {
    if(errID){
        fprintf(stderr, "Line %d: Error: %s\n", lineCount,code[errID].msg);
    }
}

int main(int argc, char *argv[]) {
    if(argc == 3){ // check for third argument as debug
        int var = atoi(argv[2]);
        if(var==0 || var == 1){ //check if it is 1 or 0, else throw error
            debugging = var;
        }
        else{
            printf("Invalid debugging argument. Only 1 or 0 possible");
        }
    }
    if (argc >= 2) {
        //open file if specified
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            printf("Error opening file");
            return 1;
        }
    }
    else{
        printf("Please provide an input:\n");
    }
    if(yyparse()==0){
        printf("accepts");
        exit(0);
    }
    else{
        printf("Exiting due to error.");
        exit(1);
    }
    return 0;
}