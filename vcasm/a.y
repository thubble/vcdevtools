%{
#include <stdio.h>
#include "insns.h"
#include "parse.h"
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
%token <integer> LSHIFT RSHIFT PLEQUAL
%token <integer> GPREG LRPC CPUID VECOP
%token <integer> CONSTANT
%token <integer> GENOPCODE LDSTOPCODE PPOPCODE BLOPCODE BCHOPCODE ABCOPCODE RTSOPCODE NOPOPCODE LEAOPCODE VECLDSTOPCODE
%token <integer> DIR_EQU DIR_SPACE DIR_STRING DIR_INTDAT

%type <expr> expr exprnosym
%type <operand> operand
%type <operand> ldstoperand gpregoperand lrpcoperand ppoperand
%type <operand> vecldstoperand vecrfoperand
%type <operand> intdatlistsingleoperand intdatlist strdat
%type <symbol> symbol
%type <symbol> label

%left '|'
%left '^'
%left '&'
%left LSHIFT RSHIFT PLEQUAL
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
	LABEL					{ $$ = symbol_parse($1); }
	;
	
statement:
	instr
	| DIR_EQU symbol ',' expr	{ symbol_define($2, $4); }
	| DIR_SPACE expr		{ geninsnspace(expr_value($2)); }
	| DIR_INTDAT intdatlist	{ geninsnintdat($1, $2); }
	| DIR_STRING strdat		{ geninsnstrdat($2); }
	;
	
instr:
	GENOPCODE gpregoperand ',' gpregoperand ',' operand		{ geninsn3($1, $2, $4, $6); }
	| GENOPCODE gpregoperand ',' operand					{ geninsn2($1, $2, $4); }
	| GENOPCODE operand		{ geninsn1($1, $2); }
	| GENOPCODE				{ geninsn0($1); }
	| BLOPCODE operand		{ geninsnmisc($1, $2); }
	| BCHOPCODE operand		{ geninsnmisc($1, $2); }
    | ABCOPCODE gpregoperand ',' operand ',' operand ',' operand	{ genmiscinsn4($1, $2, $4, $6, $8); }
	| RTSOPCODE				{ geninsnmisc(OP_B | (COND_ALWAYS << 16), create_operand_gpreg(26)); /* b rl (rl==r26) */ }
	| NOPOPCODE				{ geninsnnop(); }
	| LEAOPCODE gpregoperand ',' exprnosym			{ geninsnlea($2, create_operand_const(expr_value($4))); }
	| LEAOPCODE gpregoperand ',' symbol				{ geninsnlea($2, create_operand_symref($4)); }
	| LDSTOPCODE gpregoperand ',' ldstoperand		{ geninsnldst($1, $2, $4); }
	| PPOPCODE ppoperand ',' lrpcoperand			{ geninsnpp2($1, $2, $4); }
	| PPOPCODE ppoperand							{ geninsnpp1($1, $2); }
	| VECLDSTOPCODE vecrfoperand ',' vecldstoperand		{ geninsnvecldst($1, $2, $4); }
	;
	
gpregoperand:
	GPREG					{ $$ = create_operand_gpreg($1); }
	;
	
lrpcoperand:
	LRPC					{ $$ = create_ppoperand_lrpc(); }
	;

operand:
	gpregoperand
	| symbol				{ $$ = create_operand_symref($1); }
	| exprnosym				{ $$ = create_operand_const(expr_value($1)); }
	| CPUID					{ $$ = create_operand_cpuid(); }
	;

ldstoperand:
	'(' GPREG ')'				{ $$ = create_ldstoperand_gpreg($2); }
    | '(' GPREG ')'	'+' '+'		{ $$ = create_ldstoperand_gpreg_inc($2); }
    | '-' '-' '(' GPREG ')'		{ $$ = create_ldstoperand_gpreg_dec($4); }
	| expr '(' GPREG ')'		{ $$ = create_ldstoperand_gpregconstoffset($3, expr_value($1)); }
	| '(' GPREG ',' GPREG ')'	{ $$ = create_ldstoperand_gpreggpregoffset($2, $4); }
	;
	
vecldstoperand:
	'(' GPREG ')'										{ $$ = create_vecldstoperand($2, 0, -1); }
	| '(' GPREG '+' CONSTANT ')'						{ $$ = create_vecldstoperand($2, $4, -1); }
	| '(' GPREG PLEQUAL GPREG ')'						{ $$ = create_vecldstoperand($2, 0, $4); }
	| '(' GPREG '+' '(' CONSTANT PLEQUAL GPREG ')' ')'	{ $$ = create_vecldstoperand($2, $5, $7); }
	;
	
vecrfoperand:
	VECOP '(' CONSTANT ',' CONSTANT ')'					{ $$ = create_vecoperand($1, $3, 0, $5, 0); }
	;
	
ppoperand:
	GPREG '-' GPREG			{ $$ = create_ppoperand_gpregrange($1, $3); 
                              if ($$ == NULL) { 
            yyerror("Invalid push/pop register range. Must start with r0, r6, r16 or r24"); YYERROR; }
                            }
	| GPREG                 { $$ = create_ppoperand_gpregrange($1, $1); 
                               if ($$ == NULL) { 
            yyerror("Invalid push/pop register. Must be r0, r6, r16 or r24"); YYERROR; }
                            }

	;
	
intdatlist:
	intdatlistsingleoperand
	| intdatlistsingleoperand ',' intdatlist	{ $$ = create_intdatlistoperand_next($1, $3); }
	;
	
intdatlistsingleoperand:
	expr					{ $$ = create_intdatlistoperand(expr_value($1)); }
	;
	
strdat:
	STRING					{ $$ = create_strdatoperand($1); }
	;
	
expr:
	CONSTANT				{ $$ = gen_const_expr($1); }
	| symbol				{ $$ = gen_symbol_expr($1); }
	| '-' expr %prec UMINUS	{ $$ = gen_op_expr(UMINUS, NULL, $2); }
	| '~' expr %prec '~'	{ $$ = gen_op_expr('~', NULL, $2); }
	| expr '+' expr			{ $$ = gen_op_expr('+', $1, $3); }
	| expr '-' expr			{ $$ = gen_op_expr('-', $1, $3); }
	| expr '*' expr			{ $$ = gen_op_expr('*', $1, $3); }
	| expr '/' expr			{ $$ = gen_op_expr('/', $1, $3); }
	| expr '^' expr			{ $$ = gen_op_expr('^', $1, $3); }
	| expr '&' expr			{ $$ = gen_op_expr('&', $1, $3); }
	| expr '|' expr			{ $$ = gen_op_expr('|', $1, $3); }
	| expr LSHIFT expr		{ $$ = gen_op_expr(LSHIFT, $1, $3); }
	| expr RSHIFT expr		{ $$ = gen_op_expr(RSHIFT, $1, $3); }
	| '(' expr ')'			{ $$ = gen_op_expr('(', NULL, $2); }
	;
	
exprnosym:
	CONSTANT				{ $$ = gen_const_expr($1); }
	| '-' expr %prec UMINUS	{ $$ = gen_op_expr(UMINUS, NULL, $2); }
	| '~' expr %prec '~'	{ $$ = gen_op_expr('~', NULL, $2); }
	| expr '+' expr			{ $$ = gen_op_expr('+', $1, $3); }
	| expr '-' expr			{ $$ = gen_op_expr('-', $1, $3); }
	| expr '*' expr			{ $$ = gen_op_expr('*', $1, $3); }
	| expr '/' expr			{ $$ = gen_op_expr('/', $1, $3); }
	| expr '^' expr			{ $$ = gen_op_expr('^', $1, $3); }
	| expr '&' expr			{ $$ = gen_op_expr('&', $1, $3); }
	| expr '|' expr			{ $$ = gen_op_expr('|', $1, $3); }
	| expr LSHIFT expr		{ $$ = gen_op_expr(LSHIFT, $1, $3); }
	| expr RSHIFT expr		{ $$ = gen_op_expr(RSHIFT, $1, $3); }
	| '(' expr ')'			{ $$ = gen_op_expr('(', NULL, $2); }
	;

symbol:
	SYMBOL					{ $$ = symbol_parse($1); }
	;
%%

