.DEFAULT_GOAL := vcasm

a.tab.c a.tab.h: a.y
	bison -d a.y

lex.yy.c: a.l a.tab.h
	flex a.l

vcasm: a.tab.c lex.yy.c parse.c expression.c emit.c main.c a.tab.h parse.h expression.h emit.h insns.h list.h
	gcc a.tab.c lex.yy.c parse.c expression.c emit.c main.c -o vcasm
	
clean:
	rm -f a.tab.c a.tab.h lex.yy.c vcasm
