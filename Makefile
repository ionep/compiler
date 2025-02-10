CC = gcc
CFLAGS = -Wall -lm
LEXER_DIR = lexer
PARSER_DIR = parser
all: parse

parse: $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o
	$(CC) -o parse $(LEXER_DIR)/lex.yy.o $(PARSER_DIR)/parser.tab.o -lm

$(LEXER_DIR)/lex.yy.o: $(LEXER_DIR)/lex.yy.c $(PARSER_DIR)/parser.tab.h
	cd $(LEXER_DIR) && $(CC) -c lex.yy.c && cd ..

$(PARSER_DIR)/parser.tab.o: $(PARSER_DIR)/parser.tab.c
	cd $(PARSER_DIR) && $(CC) -c parser.tab.c && cd ..

$(LEXER_DIR)/lex.yy.c: $(LEXER_DIR)/lexer.l
	cd $(LEXER_DIR) && flex lexer.l && cd ..

$(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.tab.h: $(PARSER_DIR)/parser.y
	cd $(PARSER_DIR) && bison -d parser.y && cd ..

clean:
	rm -f $(LEXER_DIR)/lex.yy.c $(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.tab.h $(PARSER_DIR)/*.o $(LEXER_DIR)/*.o parse

test:
	./parse.exe tests/valid.txt
check:
	rm -f $(PARSER_DIR)/parser.output && make clean && cd $(PARSER_DIR) && bison -v parser.y && cd ..