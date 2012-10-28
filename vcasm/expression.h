#ifndef __EXPRESSION_H
#define __EXPRESSION_H
/*
 * das things to do with (integer) expressions
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */

struct expr;
struct symbol;
struct label;

/* Parse */
struct expr* gen_const_expr(int val);
struct expr* gen_symbol_expr(struct symbol *sym);
struct expr* gen_op_expr(int op, struct expr* left, struct expr* right);

/* Analyse */
int expr_value(struct expr *expr);

/* Cleanup */
void free_expr(struct expr *e);


struct symbol* symbol_parse(char* name);
void symbol_define(struct symbol* sym, struct expr* value);
void label_define(struct symbol* sym);

#endif // __EXPRESSION_H
