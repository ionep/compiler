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
    // printf("plus\n");
    return PLUS; 
}
"|" {
    // printf("pipe\n");
    return PIPE;
}
"\\" {
    return ESC;
}
"*" { 
    return ASTRK; 
}
"?" {
    // printf("question\n");
    return QUES;
}
"\"" {
    // printf("Quote\n");
    return QUOTE;
}

"[" {
    // printf("lbig\n");
    return LBIG;
}
"]" {
    // printf("rbig\n");
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
    // printf("minus\n");
    return MINUS;
}
[a-zA-Z0-9_]+ {
    // printf("%s\n",yytext);
    yylval.str = strdup(yytext);
    return ID;
}
[ \t\r\n]+ { 
    // Ignore whitespace
}
"%"x[0-9]+";" { 
    // printf("Unicode");
    yylval.str = strdup(yytext);
    return UNICODE; 
}
"%" {
    // printf("Percent");
    return PERCENT;
}
. { 
    // printf("Invalid character '%s' at line %d\n", yytext, yylineno);
    // exit(1); 
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
