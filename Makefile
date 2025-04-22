CC = gcc 
CLANG = clang #can use gcc too
CLANGFLAGS = -fsanitize=address
CFLAGS = -Wall -lm
LEXER_DIR = lexer
PARSER_DIR = parser
LIB_DIR = lib
all: generate

# generate a C file for regex matching
generate: $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o
	$(CC) -o generate $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o $(CFLAGS)

rexec: rexec.c
	$(CC) rexec.c -o rexec

rexec.c: generate
	./generate test.txt

rexecmem:rexec.c
	$(CLANG) $(CLANGFLAGS) rexec.c -o rexec

# generate default parse target for running 
parse: $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o
	$(CC) -o parse $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o $(CFLAGS)

$(LEXER_DIR)/lex.yy.o: $(LEXER_DIR)/lex.yy.c $(PARSER_DIR)/parser.tab.h
	cd $(LEXER_DIR) && $(CC) -c lex.yy.c && cd ..

$(PARSER_DIR)/parser.tab.o: $(PARSER_DIR)/parser.tab.c
	cd $(PARSER_DIR) && $(CC) -c parser.tab.c && cd ..

# generate parse target with address sanitizer for memory debugging
mem:$(LEXER_DIR)/lex.yy.clang.o $(PARSER_DIR)/parser.tab.clang.o 
	$(CLANG) $(CLANGFLAGS) -o generate $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o 

$(LEXER_DIR)/lex.yy.clang.o: $(LEXER_DIR)/lex.yy.c $(PARSER_DIR)/parser.tab.h
	cd $(LEXER_DIR) && $(CLANG) $(CLANGFLAGS) -c lex.yy.c && cd ..

$(PARSER_DIR)/parser.tab.clang.o: $(PARSER_DIR)/parser.tab.c
	cd $(PARSER_DIR) && $(CLANG) $(CLANGFLAGS) -c parser.tab.c && cd ..

$(LEXER_DIR)/lex.yy.c: $(LEXER_DIR)/lexer.l
	cd $(LEXER_DIR) && flex lexer.l && cd ..

$(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.tab.h: $(PARSER_DIR)/parser.y $(LIB_DIR)/AST.h $(LIB_DIR)/Symbol.h $(LIB_DIR)/lib.h
	cd $(PARSER_DIR) && bison -d parser.y && cd ..

# clean up the generated files
clean:
	rm -f $(LEXER_DIR)/lex.yy.c $(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.tab.h $(PARSER_DIR)/*.o $(PARSER_DIR)/*.output $(LEXER_DIR)/*.o parse
	rm -f generate rexec.c rexec

# run the parser with the test file
# test:
# 	./parse tests/valid.txt

test:
	./generate test.txt
# run the parser with the test file and print the AST and symbol table
debug: 
	./parse tests/valid.txt 1

stringtest:
	make rexec && ./rexec ctest.txt

# build the parser with debug file for managing S/R and R/R conflicts
check:
	make clean && cd $(PARSER_DIR) && bison -v parser.y && cd ..