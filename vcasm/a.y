%{
#include <stdio.h>
#include "insns.h"
int yylex(void);
%}

%locations
%error-verbose

%union {
	char* string;
	int integer;
	struct expr *expr;
	struct operand* operand;
	struct symbol* symbol;
}

%token <string> LABEL SYMBOL STRING
%token <integer> LSHIFT RSHIFT
%token <integer> GPREG LRPC CPUID
%token <integer> CONSTANT
%token <integer> GENOPCODE LDSTOPCODE PPOPCODE BLOPCODE BCHOPCODE RTSOPCODE NOPOPCODE LEAOPCODE
%token <integer> DIR_EQU DIR_SPACE DIR_STRING DIR_INTDAT

%type <expr> expr exprnosym
%type <operand> operand
%type <operand> ldstoperand gpregoperand lrpcoperand ppoperand
%type <operand> intdatlistsingleoperand intdatlist strdat
%type <symbol> symbol
%type <symbol> label

%left '|'
%left '^'
%left '&'
%left LSHIFT RSHIFT
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS '~'

%%

program:
	program line '\n'
	| program '\n'
	| program error '\n'	{ yyerrok; }
	| 
	;
	
line:
	statement
	| label					{ label_define($1); }
	;
	
label:
	LABEL					{ $$ = (struct symbol*)symbol_parse($1); }
	;
	
statement:
	instr
	| DIR_EQU symbol ',' expr	{ symbol_define($2, $4); }
	| DIR_SPACE expr		{ geninsnspace(expr_value($2)); }
	| DIR_INTDAT intdatlist	{ geninsnintdat($1, $2); }
	| DIR_STRING strdat		{ geninsnstrdat($2); }
	;
	
instr:
	GENOPCODE operand ',' operand ',' operand		{ geninsn3($1, $2, $4, $6); }
	| GENOPCODE operand ',' operand					{ geninsn2($1, $2, $4); }
	| GENOPCODE operand		{ geninsn1($1, $2); }
	| GENOPCODE				{ geninsn0($1); }
	| BLOPCODE operand		{ geninsnmisc($1, $2); }
	| BCHOPCODE operand		{ geninsnmisc($1, $2); }
	| RTSOPCODE				{ geninsnmisc(OP_B | (COND_ALWAYS << 16), create_operand_gpreg(26)); /* b rl (rl==r26) */ }
	| NOPOPCODE				{ geninsnnop(); }
	| LEAOPCODE gpregoperand ',' exprnosym			{ geninsnlea($2, (struct operand*)create_operand_const(expr_value($4))); }
	| LEAOPCODE gpregoperand ',' symbol				{ geninsnlea($2, (struct operand*)create_operand_symref($4)); }
	| LDSTOPCODE gpregoperand ',' ldstoperand		{ geninsnldst($1, $2, $4); }
	| PPOPCODE ppoperand ',' lrpcoperand			{ geninsnpp2($1, $2, $4); }
	| PPOPCODE ppoperand							{ geninsnpp1($1, $2); }
	;
	
gpregoperand:
	GPREG					{ $$ = (struct operand*)create_operand_gpreg($1); }
	;
	
lrpcoperand:
	LRPC					{ $$ = (struct operand*)create_ppoperand_lrpc(); }
	;

operand:
	gpregoperand
	| symbol				{ $$ = (struct operand*)create_operand_symref($1); }
	| exprnosym				{ $$ = (struct operand*)create_operand_const(expr_value($1)); }
	| CPUID					{ $$ = (struct operand*)create_operand_cpuid(); }
	;
	
ldstoperand:
	'(' GPREG ')'				{ $$ = (struct operand*)create_ldstoperand_gpreg($2); }
	| expr '(' GPREG ')'	{ $$ = (struct operand*)create_ldstoperand_gpregconstoffset($3, expr_value($1)); }
	| '(' GPREG ',' GPREG ')'	{ $$ = (struct operand*)create_ldstoperand_gpreggpregoffset($2, $4); }
	;
	
ppoperand:
	GPREG '-' GPREG			{ $$ = (struct operand*)create_ppoperand_gpregrange($1, $3); 
                              if ($$ == NULL) { 
            yyerror("Invalid push/pop register range. Must start with r0, r6, r16 or r24"); YYERROR; }
                            }
	| GPREG                 { $$ = (struct operand*)create_ppoperand_gpregrange($1, $1); 
                               if ($$ == NULL) { 
            yyerror("Invalid push/pop register. Must be r0, r6, r16 or r24"); YYERROR; }
                            }

	;
	
intdatlist:
	intdatlistsingleoperand
	| intdatlistsingleoperand ',' intdatlist	{ $$ = (struct operand*)create_intdatlistoperand_next($1, $3); }
	;
	
intdatlistsingleoperand:
	expr					{ $$ = (struct operand*)create_intdatlistoperand(expr_value($1)); }
	;
	
strdat:
	STRING					{ $$ = (struct operand*)create_strdatoperand($1); }
	;
	
expr:
	CONSTANT				{ $$ = (struct expr*)gen_const_expr($1); }
	| symbol				{ $$ = (struct expr*)gen_symbol_expr($1); }
	| '-' expr %prec UMINUS	{ $$ = (struct expr*)gen_op_expr(UMINUS, NULL, $2); }
	| '~' expr %prec '~'	{ $$ = (struct expr*)gen_op_expr('~', NULL, $2); }
	| expr '+' expr			{ $$ = (struct expr*)gen_op_expr('+', $1, $3); }
	| expr '-' expr			{ $$ = (struct expr*)gen_op_expr('-', $1, $3); }
	| expr '*' expr			{ $$ = (struct expr*)gen_op_expr('*', $1, $3); }
	| expr '/' expr			{ $$ = (struct expr*)gen_op_expr('/', $1, $3); }
	| expr '^' expr			{ $$ = (struct expr*)gen_op_expr('^', $1, $3); }
	| expr '&' expr			{ $$ = (struct expr*)gen_op_expr('&', $1, $3); }
	| expr '|' expr			{ $$ = (struct expr*)gen_op_expr('|', $1, $3); }
	| expr LSHIFT expr		{ $$ = (struct expr*)gen_op_expr(LSHIFT, $1, $3); }
	| expr RSHIFT expr		{ $$ = (struct expr*)gen_op_expr(RSHIFT, $1, $3); }
	| '(' expr ')'			{ $$ = (struct expr*)gen_op_expr('(', NULL, $2); }
	;
	
exprnosym:
	CONSTANT				{ $$ = (struct expr*)gen_const_expr($1); }
	| '-' expr %prec UMINUS	{ $$ = (struct expr*)gen_op_expr(UMINUS, NULL, $2); }
	| '~' expr %prec '~'	{ $$ = (struct expr*)gen_op_expr('~', NULL, $2); }
	| expr '+' expr			{ $$ = (struct expr*)gen_op_expr('+', $1, $3); }
	| expr '-' expr			{ $$ = (struct expr*)gen_op_expr('-', $1, $3); }
	| expr '*' expr			{ $$ = (struct expr*)gen_op_expr('*', $1, $3); }
	| expr '/' expr			{ $$ = (struct expr*)gen_op_expr('/', $1, $3); }
	| expr '^' expr			{ $$ = (struct expr*)gen_op_expr('^', $1, $3); }
	| expr '&' expr			{ $$ = (struct expr*)gen_op_expr('&', $1, $3); }
	| expr '|' expr			{ $$ = (struct expr*)gen_op_expr('|', $1, $3); }
	| expr LSHIFT expr		{ $$ = (struct expr*)gen_op_expr(LSHIFT, $1, $3); }
	| expr RSHIFT expr		{ $$ = (struct expr*)gen_op_expr(RSHIFT, $1, $3); }
	| '(' expr ')'			{ $$ = (struct expr*)gen_op_expr('(', NULL, $2); }
	;

symbol:
	SYMBOL					{ $$ = (struct symbol*)symbol_parse($1); }
	;
%%


