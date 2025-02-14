# Regular Expression Compiler

This project implements a regular expression compiler in flex and bison as a part of project assignment of CS541.

## 📝 How It Works
- **Flex (`lexer/lexer.l`)** tokenizes the input characters.
- **Bison (`parser/parser.y`)** parses expressions according to the grammar.

## 📂 File Structure
- `lexer/lexer.l` - Lexical analyzer (token definitions)
- `parser/parser.y` - Bison Parser (grammar rules)
- `parse` - Executable file
- `tests/` - Include two files valid.txt and invalid.txt for testing
- `Makefile` - Compilation automation
- `test.txt` - Immediate test cases to use with command 7

## ⚙️ Compilation

**Commands**

1. *make*

    Builds "parse" to run the parser.

2. *make clean*

    Clean the build files.

3. *make check*

    Check for parse conflicts and generate output file. (Use only during development to see where the conflicts occur)
    
4. *make test*

    Test for the parser based on valid tests in "tests/valid.txt".
    
5. *make debug*

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


6. *./parse*

    Runs the parser and takes input from the user

7. *./parse filepath*

    Runs the parser on the input file mentioned in the argument

    Eg: 
        
        ./parse test.txt

        ./parse tests/invalid.txt