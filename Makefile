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
	rm -f $(LEXER_DIR)/lex.yy.c $(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.tab.h $(PARSER_DIR)/*.o $(PARSER_DIR)/*.output $(LEXER_DIR)/*.o parse

test:
	./parse tests/valid.txt

debug: 
	./parse tests/valid.txt 1

check:
	make clean && cd $(PARSER_DIR) && bison -v parser.y && cd ..