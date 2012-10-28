%{
#include <stdio.h>
#include <stdarg.h>

#include "a.tab.h"
#include "parse.h"
#include "insns.h"

void yyerror(char *s, ...);
%}

%option yylineno

%option noinput
%option nounput

letter			[A-Za-z]
digit			[0-9]
hexdigit		[0-9a-fA-F]
gpreg			r{digit}+
cond			eq|ne|cs|lo|cc|hs|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|f
genop			mov|cmn|add|bic|mul|eor|sub|and|mvn|ror|cmp|rsb|btst|or|extu|max|bset|min|bclr|bchg|exts|neg|lsr|clz|lsl|brev|asr|abs
ldstop			ld|st
ppop			push|pop
size			b|h|sh
lrpc			lr|pc
symfirstchar	{letter}|[_]
symbolname		{symfirstchar}({symfirstchar}|{digit})*


%%

\.set|\.equ		return DIR_EQU;
\.space			return DIR_SPACE;
\.string		return DIR_STRING;
\.byte			{ yylval.integer = DAT_BYTE; return DIR_INTDAT; }
\.half			{ yylval.integer = DAT_HALF; return DIR_INTDAT; }
\.word			{ yylval.integer = DAT_WORD; return DIR_INTDAT; }
{gpreg}			{ yylval.integer = atoi(yytext+1); return GPREG; }
{lrpc}			return LRPC;
{genop}{cond}	{ yylval.integer = str2opcodecond(yytext); return GENOPCODE; }
{genop}			{ yylval.integer = str2opcode(yytext) | (COND_ALWAYS << 16); return GENOPCODE; }
{ppop}			{ yylval.integer = str2opcode(yytext) | (COND_ALWAYS << 16); return PPOPCODE; }
{ldstop}{size}{cond}	{ 
					yylval.integer = str2opcodesizecond(yytext);
					return LDSTOPCODE;
				}
{ldstop}{cond}	{ 
					yylval.integer = str2opcodecond(yytext) | (SIZE_W << 24);
					return LDSTOPCODE;
				}
{ldstop}{size}	{ 
					yylval.integer = str2opcodesize(yytext) | (COND_ALWAYS << 16);
					return LDSTOPCODE;
				}
{ldstop}		{ yylval.integer = str2opcode(yytext) | (COND_ALWAYS << 16) | (SIZE_W << 24); return LDSTOPCODE; }
bl				{ 
					yylval.integer = OP_BL | (COND_ALWAYS << 16);
					return BLOPCODE;
				}
b{cond}			{ 
					yylval.integer = str2opcodecond(yytext);
					return BCHOPCODE;
				}
b				{ 
					yylval.integer = str2opcode(yytext) | (COND_ALWAYS << 16);
					return BCHOPCODE;
				}
rts				{ 
					yylval.integer = OP_B | (COND_ALWAYS << 16); /* b rl (rl==r26) */
					return RTSOPCODE;
				}
nop				{ 
					yylval.integer = OP_NOP;
					return NOPOPCODE;
				}
lea				{ 
					yylval.integer = OP_LEA;
					return LEAOPCODE;
				}
cpuid			{ return CPUID; }
{symbolname}:	{
					yylval.string = strdup(yytext);
					yylval.string[strlen(yytext) - 1] = 0;
					return LABEL;
				}
{symbolname}	{
					yylval.string = strdup(yytext);
					return SYMBOL;
				}
0x{hexdigit}+	{ yylval.integer = (int)strtoul(yytext, NULL, 16); return CONSTANT; }
{digit}+		{ yylval.integer = atoi(yytext); return CONSTANT; }
\"(\\.|[^\\"])*\"	{ yylval.string = yytext; return STRING; }
\<\<			return LSHIFT;
\>\>			return RSHIFT;
[-,+*/|&^()~\[\]\n]			{ return *yytext; }
[ \t\r]+		;
;.*				;		/* comment */
<<EOF>>			{ static int once = 0; return once++ ? 0 : '\n'; }
.				yyerror("invalid character '%c'", *yytext);

%%

int yywrap(void)
{
	return 1;
}

void yyerror(char *s, ...)
{
	va_list ap;
	va_start(ap, s);

	fprintf(stderr, "line %d: Error: ", yylineno);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
}
