%{
#include "../parser/parser.tab.h" 
%}

%%

"//".* {
    //these are comments
}
"/" {
    return SLASH;
}
"const" {
    return CONST;
}
"=" {
    return EQUAL;
}
"&" {
    return AMP;
}
"!" {
    return NOT;
}
"(" { 
    return LPAR; 
}
")" { 
    return RPAR; 
}
"+" { 
    return PLUS; 
}
"|" {
    return PIPE;
}
"\\" {
    return ESC;
}
"*" { 
    return ASTRK; 
}
"?" {
    return QUES;
}
"\"" {
    return QUOTE;
}

"[" {
    return LBIG;
}
"]" {
    return RBIG;
}
"^" {
    return CAP;
}
"." {
    return WILD;
}
"${" {
    return LCUR;
}
"}" {
    return RCUR;
}
"-" {
    return MINUS;
}
[a-zA-Z0-9_]+ {
    yylval.str = strdup(yytext);
    return ID;
}
[ \t\r\n]+ { 
    // Ignore whitespace
}
"%"x[0-9]+";" { 
    yylval.str = strdup(yytext);
    return UNICODE; 
}
"%" {
    return PERCENT;
}
. { 
    // printf("Invalid character '%s' at line %d\n", yytext, yylineno);
    // exit(1); 
    //select all other characters too which can exist inside "" or []
    yylval.str = strdup(yytext);
    return OTHERCHAR;
}
<<EOF>> {
    // return 0 only when the file ends so that we handle multiple regex
    return 0;
}
%%


int yywrap() {
    return 1; // 1 for single input and 0 for multiple
}
