%{
    /*
        Lexer file to read all valid characters and change them into tokens for later use
    */
    //file location to the parser header generated
    #include "../parser/parser.tab.h" 
    #include <string.h>
%}
%x LITERAL RANGE
SLASH "/"
CONST "const"
EQUAL "="
AMP "&"
NOT "!"
LPAR "("
RPAR ")"
PLUS "+"
ASTRK "*"
QUES "?"
PIPE "|"
ESC "\\"
CAP "^"
WILD "."
LCUR "${"
RCUR "}"
MINUS "-"
ID [a-zA-Z0-9_]+
UNICODE "%x"[+-]?[0-9A-Fa-f]*";"
PERCENT "%"
OTHERCHAR .
%%

"//".* {    // Get double slash and do nothing for comments

}
<INITIAL,LITERAL,RANGE>{SLASH} {       // Take slash for the start and end of regex
    return SLASH;
}
<INITIAL,LITERAL,RANGE>{CONST} {   // const as a keyword for the "definition"
    return CONST_TOK;
}
<INITIAL,LITERAL,RANGE>{EQUAL} {       // Paired with const keyword *    Note: * => but also can come inside literal or range
    return EQUAL;
}
<INITIAL,LITERAL,RANGE>{AMP} {       // For RootRegex & RootRegex in RootRegex *
    return AMP;
}
<INITIAL,LITERAL,RANGE>{NOT} {       // To use as !Regex in RootRegex *
    return NOT;
}
<INITIAL,LITERAL,RANGE>{LPAR} {       // To use as ( Regex ) in Regex *
    return LPAR; 
}
<INITIAL,LITERAL,RANGE>{RPAR} {       // Pair to LPAR but the closing braces *
    return RPAR; 
}
<INITIAL,LITERAL,RANGE>{PLUS} {       // To use as Regex + in Repeat *
    return PLUS; 
}
<INITIAL,LITERAL,RANGE>{ASTRK} {       // To use as Regex * in Repeat *
    return ASTRK; 
}
<INITIAL,LITERAL,RANGE>{QUES} {       // To use as Regex ? in Repeat *
    return QUES;
}
<INITIAL,LITERAL,RANGE>{PIPE} {       // To use as Regex | Regex in Alt *
    return PIPE;
}
<INITIAL,LITERAL,RANGE>{ESC} {      /* This is used to escape any characters. For the sake of the assignment, it is only used
             in Range for ] inside [] as [ /] ] where the first ] is escaped. This can be used for
             double quotes and % too but the question asks for unicode in that case.  *       */
    return ESC;
}
\"              { BEGIN(LITERAL); return QUOTE; }
<LITERAL>\"      { BEGIN(INITIAL); return QUOTE; }


\[              { BEGIN(RANGE); return LBIG; }
<RANGE>\]        { BEGIN(INITIAL); return RBIG; }

<LITERAL>[ ]+   { yylval.str = strdup(yytext); return OTHERCHAR; }
<RANGE>[ ]+     { yylval.str = strdup(yytext); return OTHERCHAR; }

[ \t\r\n]+   { /* Ignore whitespace outside */ }

<INITIAL,LITERAL,RANGE>{CAP} {       // To use as [^  ] in Range *
    return CAP;
}
<INITIAL,LITERAL,RANGE>{WILD} {       // Wild character as a term *
    return WILD;
}
<INITIAL,LITERAL,RANGE>{LCUR} {      // Start of Substitute but paired "${" as same since they always occur in pair *
    return LCUR;
}
<INITIAL,LITERAL,RANGE>{RCUR} {       // End of substitute as a pair to "${" *
    return RCUR;
}
<INITIAL,LITERAL,RANGE>{MINUS} {       // This is used for MINUS
    return MINUS;
}
<INITIAL,LITERAL,RANGE>{ID} {     // ID for definition and subsitute. * it matches alphanumerics and underscore
    yylval.str = strdup(yytext);
    return ID;
}
<INITIAL,LITERAL,RANGE>{UNICODE} {     // unicode is escaped in the format %x[0-9]+;
    yylval.str = strdup(yytext);
    if(yylval.str[2] == '+' || yylval.str[2] == '-') { // filter signs and no number cases from lexer
        fprintf(stderr, "Error: Invalid unicode escape sequence %s\n", yylval.str);
        exit(1);
    } else {
        if(strlen(yylval.str) <= 3) {
            fprintf(stderr, "Error: Invalid unicode escape sequence %s\n", yylval.str);
            exit(1);
        }
    }
    return UNICODE; 
}
<INITIAL,LITERAL,RANGE>{PERCENT} {               /* this is just to make sure we dont have % in literals as they need to be escaped
                     So we separate the % and dont match it in literal */
    return PERCENT;
}
<INITIAL,LITERAL,RANGE>{OTHERCHAR} {                 //select all other characters too which can exist inside "" or []
    yylval.str = strdup(yytext);
    return OTHERCHAR;
}
<INITIAL,LITERAL,RANGE><<EOF>> {       // return 0 only when the file ends so that we handle multiple regex
    return 0;
}
%%


int yywrap() {
    return 1; // 1 for single input and 0 for multiple
}
