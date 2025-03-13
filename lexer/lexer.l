%{
    /*
        Lexer file to read all valid characters and change them into tokens for later use
    */
    //file location to the parser header generated
    #include "../parser/parser.tab.h" 
%}

%%

"//".* {    // Get double slash and do nothing for comments

}
"/" {       // Take slash for the start and end of regex
    return SLASH;
}
"const" {   // const as a keyword for the "definition"
    return CONST_TOK;
}
"=" {       // Paired with const keyword *    Note: * => but also can come inside literal or range
    return EQUAL;
}
"&" {       // For RootRegex & RootRegex in RootRegex *
    return AMP;
}
"!" {       // To use as !Regex in RootRegex *
    return NOT;
}
"(" {       // To use as ( Regex ) in Regex *
    return LPAR; 
}
")" {       // Pair to LPAR but the closing braces *
    return RPAR; 
}
"+" {       // To use as Regex + in Repeat *
    return PLUS; 
}
"*" {       // To use as Regex * in Repeat *
    return ASTRK; 
}
"?" {       // To use as Regex ? in Repeat *
    return QUES;
}
"|" {       // To use as Regex | Regex in Alt *
    return PIPE;
}
"\\" {      /* This is used to escape any characters. For the sake of the assignment, it is only used
             in Range for ] inside [] as [ /] ] where the first ] is escaped. This can be used for
             double quotes and % too but the question asks for unicode in that case.  *       */
    return ESC;
}
"\"" {      /* For literals as start and end i.e. " ". Used \" to escape in lexer *
                To use in literal, equivalent unicode of " is only supported */
    return QUOTE;
}

"[" {       // To use as [  ] in Range *
    return LBIG;
}
"]" {       // To use as [  ] in Range *
    return RBIG;
}
"^" {       // To use as [^  ] in Range *
    return CAP;
}
"." {       // Wild character as a term *
    return WILD;
}
"${" {      // Start of Substitute but paired "${" as same since they always occur in pair *
    return LCUR;
}
"}" {       // End of substitute as a pair to "${" *
    return RCUR;
}
"-" {       // This is used for Range eg: [ 0-9 ] *
    return MINUS;
}
[a-zA-Z0-9_]+ {     // ID for definition and subsitute. * it matches alphanumerics and underscore
    yylval.str = strdup(yytext);
    return ID;
}
[ \t\r\n]+ {        // all whitespace are ignored
    // Ignore whitespace
}
"%"x[+-]?[0-9]*";" {     // unicode is escaped in the format %x[0-9]+;
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
"%" {               /* this is just to make sure we dont have % in literals as they need to be escaped
                     So we separate the % and dont match it in literal */
    return PERCENT;
}
. {                 //select all other characters too which can exist inside "" or []
    return OTHERCHAR;
}
<<EOF>> {       // return 0 only when the file ends so that we handle multiple regex
    return 0;
}
%%


int yywrap() {
    return 1; // 1 for single input and 0 for multiple
}
