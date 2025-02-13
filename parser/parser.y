/**
 * @file parser.y
 * @brief Bison parser for Regex checker.
 *
 * This file defines the grammar for our custom regular expression.
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
int lineCount=1;
struct errorCode code[] = {
    {0,"No error"},
    {1,"Error using const format or missing SLASH"},
    {2, "Unexpected ^, &, *, +, ? or |"},
    {3, "Unmatched quotes, braces (, [ or { OR unexpected ^, % or unicode"}
}; // couldn't fix this to show different codes due to R/R conflict. So, use last for default as of now
int errID=0; 

// Error handling
void yyerror(const char *);
%}


%union{
    char *str;
}

%token <str> SLASH CONST EQUAL AMP NOT LPAR RPAR PLUS PIPE ASTRK ESC PERCENT
%token <str> QUES UNICODE QUOTE LBIG RBIG CAP WILD LCUR RCUR MINUS OTHERCHAR 
%token <str> ID;
%type <str> system definition rootregex seq regex term multiregterm regterm anychar multiliteral literal
    alt repeat range wild substitute;

%left NOT AMP
%left PIPE
%left ASTRK PLUS QUES 
%start line
/* %left MINUS
%left LCUR RCUR
%left LBIG RBIG
%left LPAR RPAR */

%%

//to support multiple tests in file
// a single line or multiple line
line: system
    | line system
    | error { yyerror("Syntax error"); yyerrok; return 1;};

system: SLASH rootregex SLASH { lineCount++; printf("/%s/\n",$2); }
    | definition system{ lineCount++; printf("%s%s",$1,$2); };

definition: CONST ID EQUAL { $$ = malloc(strlen($2)+10); sprintf($$,"const %s = ",$2); };

rootregex: rootregex AMP rootregex { $$ = malloc(strlen($1)+strlen($3)+4); sprintf($$,"%s & %s",$1,$3); }
    | NOT alt { $$ = malloc(strlen($2) + 2); sprintf($$,"!%s",$2);}
    | alt { $$ = malloc(strlen($1)+ 2); sprintf($$,"%s",$1);  };

// one or more regex | regex
alt: alt PIPE seq { $$=malloc(strlen($1)+strlen($3)+4); sprintf($$,"$%s | %s$",$1,$3);}
    | seq { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1);  }
    | LPAR alt RPAR { $$ = malloc(strlen($2)+ 3); sprintf($$,"(%s)",$2); }
    | alt LPAR alt RPAR { $$ = malloc(strlen($1)+strlen($3)+ 3); sprintf($$,"%s(%s)",$1,$3); };

//multiple regex
seq: regex { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1);  }
    | seq regex { $$=malloc(strlen($1)+strlen($2)+1); sprintf($$,"%s%s",$1,$2);};
    /* | alt { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1); }; */

regex: term { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1); }
    | repeat { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1); }; // repeat has higher precedence than seq


repeat: regex ASTRK { $$ = malloc(strlen($1)+ 2); sprintf($$,"%s*",$1); }
    | regex PLUS { $$ = malloc(strlen($1)+ 2); sprintf($$,"%s+",$1);  }
    | regex QUES { $$ = malloc(strlen($1)+ 2); sprintf($$,"%s?",$1);  };

term: QUOTE multiliteral QUOTE { $$ = malloc(strlen($2)+ 5); sprintf($$,"\"%s\"",$2);}
    | range { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1);  }
    | wild { $$ = malloc(strlen($1)+ 1); sprintf($$,"%s",$1);  }
    | substitute { $$ = malloc(strlen($1)+ 4); sprintf($$,"%s",$1); }
    | error { errID=3; yyerror("Error"); yyerrok; return 1;};

range: LBIG multiregterm RBIG { $$=malloc(strlen($2)+3); sprintf($$,"[%s]",$2); }
    | LBIG CAP multiregterm RBIG { $$=malloc(strlen($3)+4); sprintf($$,"[^%s]",$3);};

wild: WILD { $$ = malloc(2); sprintf($$,"%s",$1);  };

substitute: LCUR ID RCUR { $$ = $2;};

multiregterm: regterm { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1);  };
    | multiregterm regterm { $$=malloc(strlen($1)+strlen($2)+1); sprintf($$,"%s%s",$1,$2); };

regterm: anychar { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1); }
    | ESC RBIG { $$=malloc(2); sprintf($$,"]"); } // used \] to use ] or can use unicode but question mentions only for literals
    | QUOTE { $$=malloc(3); sprintf($$,"\""); }
    | PERCENT { $$=malloc(2); sprintf($$,"%");}; // added which are not inside anychar

multiliteral: literal { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1); }
    | multiliteral literal { $$=malloc(strlen($1)+strlen($2)+1); sprintf($$,"%s%s",$1,$2); };

literal: anychar { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1); }
    /* | ESC QUOTE { $$=malloc(strlen($2)+2); sprintf($$,"\\\"",$2);} */ //this works too \" but used unicode
    | RBIG { $$=malloc(2); sprintf($$,"]");}
    | UNICODE { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1); }; //can put in anychar to support unicode inside range

anychar: PLUS { $$=malloc(2); sprintf($$,"+"); }
    | MINUS { $$=malloc(2); sprintf($$,"-"); }
    | CONST { $$=malloc(5); sprintf($$,"const"); }
    | EQUAL { $$=malloc(2); sprintf($$,"="); }
    | AMP { $$=malloc(2); sprintf($$,"&"); }
    | NOT { $$=malloc(2); sprintf($$,"!"); }
    | LPAR { $$=malloc(2); sprintf($$,"(");}
    | RPAR { $$=malloc(2); sprintf($$,")");}
    | PIPE { $$=malloc(2); sprintf($$,"|");}
    | QUES { $$=malloc(2); sprintf($$,"?");}
    | LBIG { $$=malloc(2); sprintf($$,"[");}
    | ESC ESC { $$=malloc(3); sprintf($$,"\\");}
    | ASTRK { $$=malloc(2); sprintf($$,"*");}
    | WILD { $$=malloc(2); sprintf($$,"."); }
    | LCUR { $$=malloc(3); sprintf($$,"${");}
    | RCUR { $$=malloc(2); sprintf($$,"}"); }
    | ID { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1);}
    | OTHERCHAR { $$=malloc(strlen($1)+1); sprintf($$,"%s",$1);};

%%
void yyerror(const char *s) {
    if(errID){
        fprintf(stderr, "Line %d: Error: %s\n", lineCount,code[errID].msg);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
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