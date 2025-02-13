# Regex Checker

This project implements a basic calculator using Bison and Flex.

## 📝 How It Works
- **Flex (`lexer.l`)** tokenizes the input.
- **Bison (`parser.y`)** parses expressions and evaluates them.

## 📂 File Structure
- `lexer.l` - Lexical analyzer (token definitions)
- `parser.y` - Syntax analyzer (grammar rules)
- `Makefile` - Compilation automation

## ⚙️ Compilation
Run the following commands:

```sh
bison -d parser.y
flex lexer.l
gcc parser.tab.c lex.yy.c -o calculator -lm

**Commands**

1. *make*

    Builds "parse.exe" to run the parser.

2. *make clean*

    Clean the build files.

3. *make check*

    Check for parse conflicts and generate output file.
    
4. *make test*

    Test for the parser based on valid tests.

5. *./parse.exe*

    Takes input from the user

6. *./parse.exe filepath*

    Input a file to the parser

    Eg: 
        
        ./parse.exe tests/valid.txt

        ./parse.exe tests/invalid.txt