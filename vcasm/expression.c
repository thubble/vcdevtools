/*
 * das things to do with (integer) expressions
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 *
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "insns.h"
#include "expression.h"
#include "parse.h"
#include "list.h"
#include "a.tab.h"

enum sym_type {
	SYM_LABEL,
	SYM_EXPR,
};

enum expr_type {
	EXPR_SYMBOL,
	EXPR_CONSTANT,
	EXPR_OPERATOR,
};

struct expr {
	int type;
	int value;
	union {
		struct symbol *symbol;
		int op;
	};
	struct expr *left;	/* wasted space if not an operator node */
	struct expr *right;
};

static LIST_HEAD(all_symbols);

/* internal unconditional (re)calculation */
static int expr_value_calc(struct expr *e)
{
	int right, left;

	if (e->type == EXPR_CONSTANT) {
		return e->value;
	}
	/* operator */
	right = expr_value(e->right);
	switch (e->op) {
	case UMINUS: return - expr_value(e->right);
	case '~': return ~ expr_value(e->right);
	case '(': return expr_value(e->right);		/* (expr) is a no-op */
	}

	left = expr_value(e->left);
	switch (e->op) {
	case '-': return left - right;
	case '+': return left + right;
	case '*': return left * right;
	case '/':
		if (right == 0) {
			return 0;
		} else {
			return left / right;
		}
		break;
	case '|': return left | right;
	case '^': return left ^ right;
	case '&': return left & right;
	case LSHIFT: return left << right;
	case RSHIFT: return left >> right;
	}
	
	return -1;
}

/*
 * Parse
 */
struct expr* gen_const_expr(int val)
{
	struct expr *e = calloc(sizeof *e, 1);
	
	e->type = EXPR_CONSTANT;
	e->value = val;
	
	return e;
}

struct expr* gen_symbol_expr(struct symbol *sym)
{
	struct expr *e = calloc(sizeof *e, 1);
	
	e->type = EXPR_SYMBOL;
	e->value = sym->value;
	e->symbol = sym;
	
	return e;
}

struct expr* gen_op_expr(int op, struct expr* left, struct expr* right)
{
	struct expr *e;

	e = calloc(sizeof *e, 1);
	
	e->type = EXPR_OPERATOR;
	e->op = op;
	e->left = left;
	e->right = right;
	
	e->value = expr_value_calc(e);
	
	return e;
}


int expr_value(struct expr *e)
{
	if (e->type == EXPR_SYMBOL)
		return e->symbol->value;
	else
		return e->value;
}


/* Cleanup */

void free_expr(struct expr *e)
{
	if (e->right)
		free_expr(e->right);
		
	if (e->left)
		free_expr(e->left);
	
	free(e);
}


struct symbol* symbol_inlist(char *name, struct list_head *head)
{
	struct symbol *l;
	list_for_each_entry(l, head, list) {
		if (0 == strcmp(l->name, name))
			return l;
	}
	return NULL;
}

struct symbol* symbol_parse(char* name)
{
	struct symbol *sym;
	sym = symbol_inlist(name, &all_symbols);
	if (!sym) {
		/* not seen a symbol with this name before */
		sym = calloc(1, sizeof *sym);
		sym->name = strdup(name);
		sym->type = SYM_LABEL; /* assume undefined symbols are labels - constants are required to be defined before use */
		list_add_tail(&sym->list, &all_symbols);
	}
	return sym;
}

void symbol_define(struct symbol* sym, struct expr* value)
{
	sym->type = SYM_EXPR;
	sym->value = expr_value(value);
}

void label_define(struct symbol* sym)
{
	sym->type = SYM_LABEL;
	sym->value = cur_pc;
}
