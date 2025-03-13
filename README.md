# Regular Expression Compiler

This project implements a regular expression compiler in flex and bison as a part of project assignment of CS541. The grammar for the compiler is
given at the bottom. At the current version, it generates an abstract syntax tree and a symbol table. Also, it makes sure that there is no memory leaks by
manually deallocating all memory.


## 📝 How It Works
- **Flex (`lexer/lexer.l`)** tokenizes the input characters.
- **Bison (`parser/parser.y`)** parses expressions according to the grammar.

## 📂 File Structure
- `lexer/lexer.l` - Lexical analyzer (token definitions)
- `parser/parser.y` - Bison Parser (grammar rules)
- `lib/AST.h` - Custom Library for AST defining data structure and essential functions
- `lib/Symbol.h` - Custom Library for Symbol Table defining data structure and essential functions
- `parse` - Executable file
- `tests/` - Include two files valid.txt and invalid.txt for testing
- `Makefile` - Compilation automation
- `test.txt` - Immediate test cases to use with run command 2

## Requirements

- Flex

        sudo apt install flex

- Bison

        sudo apt install bison

- Address Sanitizer (clang) - optional

        sudo apt install clang

- Valgrind - optional

        sudo apt install valgrind

## ⚙️ Compilation

**Commands**

1. *make*

    Builds "parse" to run the parser.

2. *make mem*

    Builds "parse" with Address Sanitizer (see requirements). Usage is same as normal.

2. *make clean*

    Clean the build files.

3. *make check*

    Check for parse conflicts and generate output file. (Use only during development to see where the conflicts occur)
    
4. *make test*

    Test for the parser based on valid tests in "tests/valid.txt".
    
5. *make debug*

    Test for the parser based on valid tests (tests/valid.txt) and set debugging to 1 to print back the Abstract Syntax Tree and Symbol table.

    
    ```markdown
    Old Version for validating the parse [Checkout](https://github.com/ionep/compiler/commit/364f7f9cf1b2ac050de0462a0f3233b00d3210f9)
    
    Test for the parser based on valid tests (tests/valid.txt) and set debugging to 1 to print back the parsed contents.

    Warning: Debug mode can run into Segmentation fault as it uses malloc to see how parser is reading the input and is continuously allocating memory. So, make sure you are not running large files here (use make test instead)

    Usage: I have used a few additional conventions here to separate alternation, sequence and repeat. It helps
    to see their precedence in action.

        Alternation => @ @
        
        Sequence => ^ ^

        Repeat => # #
    
    Note that this is just to print and keep track during debugging and doesn't affect the actual parsing of the regular expression.
    
    Eg:

        /"repeat"*/ will be printed as / #"repeat"*# /

        /"alt1" | "alt2"/ will be printed as / @"alt1"|"alt2"@ /

        /"reg1" "reg2"/ will be printed as / ^"reg1""reg2"^ /
    ```


## ⚙️ Run

**Commands**


1. *./parse*

    Runs the parser and takes input from the user

2. *./parse filepath*

    Runs the parser on the input file mentioned in the argument

    Eg: 
        
        ./parse test.txt

        ./parse tests/invalid.txt

3. *valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./parse filepath* - optional

    Check the memory leaks using valgrind.

    Eg:

        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./parse test.txt

        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./parse tests/valid.txt


## Grammar

        System     := Definition* '/' RootRegex '/'
        Definition :=  'const' ID '=' '/' Regex '/'
        RootRegex  :=  RootRegex '&' RootRegex | '!' Regex | Regex
        Regex      :=  Seq | Alt | Repeat | Term | '(' Regex ')'
        Seq        :=  Regex+
        Alt        :=  Regex '|' Regex
        Repeat     :=  Regex'*' | Regex'+'  | Regex'?'
        Term       :=  Literal | Range | Wild | Substitute
        Literal    :=  '"' escaped unicode '"' 
        Range      :=  '[' '^'? unicode char ranges ']' // range is C1-C2 & may be escaped
        Wild       :=  '.'
        Substitute :=  '${' ID '}'
        ID         :=  [a-zA-Z0-9_]+