all:
	flex --header-file=lex.yy.h lex.l
	bison -d yacc.y
	flex lex.l
	gcc -o differ yacc.tab.c lex.yy.c functions.c -lreadline -O3 -s
	rm yacc.tab.c
	rm yacc.tab.h
	rm lex.yy.c
	rm lex.yy.h
