#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "insns.h"
#include "parse.h"
#include "emit.h"
#include "list.h"

LIST_HEAD(opslist);

void PRINTOPERAND(struct operand* opd);

int str2opcode(char* str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(opcodenames); i++) {
		if (0 == strcasecmp(str, opcodenames[i]))
			return i;
	}
	
	return -1;
}

int str2opcodecond(char* str)
{
	int len = strlen(str);
	int cond;
	if (str[len-1] == 'f')
	{
		cond = str2condcode(str+(len-1));
		str[len-1] = 0;
	}
	else
	{
		cond = str2condcode(str+(len-2));
		str[len-2] = 0;
	}
	return str2opcode(str) | (cond << 16);
}

int str2opcodesize(char* str)
{
	int len = strlen(str);
	int size;
	if (str[len-2] == 's')
	{
		size = str2sizecode(str+(len-2));
		str[len-2] = 0;
	}
	else
	{
		size = str2sizecode(str+(len-1));
		str[len-1] = 0;
	}
	
	return str2opcode(str) | (size << 24);
}

int str2opcodesizecond(char* str)
{
	int len = strlen(str);
	int cond;
	int size;
	if (str[len-1] == 'f')
	{
		cond = str2condcode(str+(len-1));
		str[len-1] = 0;
	}
	else
	{
		cond = str2condcode(str+(len-2));
		str[len-2] = 0;
	}
	
	len = strlen(str);
	if (str[len-2] == 's')
	{
		size = str2sizecode(str+(len-2));
		str[len-2] = 0;
	}
	else
	{
		size = str2sizecode(str+(len-1));
		str[len-1] = 0;
	}
	
	return str2opcode(str) | (cond << 16) | (size << 24);
}

int str2condcode(char* str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(condcodenames); i++) {
		if (0 == strcasecmp(str, condcodenames[i]))
			return i;
	}
	
	return COND_ALWAYS;
}

int str2sizecode(char* str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sizecodenames); i++) {
		if (0 == strcasecmp(str, sizecodenames[i]))
			return i;
	}
	
	return SIZE_W;
}

int str2vecop(char* str)
{
	int vecop = 0x00000000;
	if (str[0] == 'V')
		vecop |= 0x00010000; /* bit 16: vertical */
	
	if (strlen(str) == 1)
		vecop |= 0x08; /* 8-bit */
	else if (str[1] == 'X')
		vecop |= 0x10; /* 16-bit */
	else if (str[1] == 'Y')
		vecop |= 0x20; /* 32-bit */
	
	return vecop;
}

int str2vecldstopcode(char* str, int is_st)
{
	int vecldstop = 0x00000000;
	
	if (is_st)
		vecldstop |= 0x00010000; /* bit 16: store */
	
	if (strlen(str) < 4)
		vecldstop |= 0x20; /* 32-bit */
	else if (str[3] == 'w')
		vecldstop |= 0x20; /* 32-bit */
	else if (str[3] == 'h')
		vecldstop |= 0x10; /* 16-bit */
	else if (str[3] == 'b')
		vecldstop |= 0x08; /* 8-bit */
	
	return vecldstop;
}


struct operand* create_operand_gpreg(int gpreg)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_GPREG;
	opd->gpreg = gpreg;
	
	return opd;
}

struct operand* create_operand_symref(struct symbol* sym)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_SYMREF;
	opd->sym = sym;
	
	return opd;
}

struct operand* create_operand_const(int constval)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_CONST;
	opd->constval = constval;
	
	return opd;
}

struct operand* create_operand_cpuid()
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_CPUID;
	
	return opd;
}

struct operand* create_ldstoperand_gpreg(int gpreg)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_ADDR_GPREG;
	opd->gpreg = gpreg;
    opd->constval = 0; /* Code might later treat OPD_ADDR_GPREG the same as 
                          OPD_ADDR_GPREG_CONSTOFFSET with a zero offset. */
	return opd;
}

struct operand* create_ldstoperand_gpreg_inc(int gpreg) {
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_ADDR_GPREG_INC;
	opd->gpreg = gpreg;
	return opd;
}

struct operand* create_ldstoperand_gpreg_dec(int gpreg) {
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_ADDR_GPREG_DEC;
	opd->gpreg = gpreg;
	return opd;
}

struct operand* create_ldstoperand_gpregconstoffset(int gpreg, int constoffset)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_ADDR_GPREG_CONSTOFFSET;
	opd->gpreg = gpreg;
	opd->constval = constoffset;
	
	return opd;
}

struct operand* create_ldstoperand_gpreggpregoffset(int gpreg, int gpreg2)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_ADDR_GPREG_GPREGOFFSET;
	opd->gpreg = gpreg;
	opd->gpreg2 = gpreg2;
	
	return opd;
}

struct operand* create_vecldstoperand(int gpreg, int constval, int gpreg2)
{
	if (gpreg < 0 || gpreg > 6)
		return NULL;
	if (gpreg2 != -1 && (gpreg2 < 0 || gpreg2 > 6))
		return NULL;
	
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_VECLDST;
	opd->gpreg = gpreg;
	opd->gpreg2 = gpreg2;
	opd->constval = constval;
	
	return opd;
}

struct operand* create_ppoperand_gpregrange(int gpreg, int gpreg2)
{
    if(gpreg != 0 && gpreg != 6 && gpreg != 16 && gpreg != 24) {
        return NULL;
    }

	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_GPREG_RANGE;
	opd->gpreg = gpreg;
	opd->gpreg2 = gpreg2;
	
	return opd;
}

struct operand* create_ppoperand_lrpc()
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_LRPC;
	
	return opd;
}

struct operand* create_vecoperand(int vecop, int y, int yinc, int x, int xinc)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_VECRF;
	
	opd->vrf_y = y;
	opd->vrf_yinc = yinc;
	opd->vrf_x = x;
	opd->vrf_xinc = xinc;
	
	opd->vrf_bit_size = (vecop & 0x0000FFFF);
	opd->vrf_is_vertical = ((vecop & 0xFFFF0000) != 0);
	
	return opd;
}

struct operand* create_intdatlistoperand(int val)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_INTDAT;
	opd->constval = val;
	
	return opd;
}

struct operand* create_intdatlistoperand_next(struct operand* cur, struct operand* list)
{
	cur->list_next = list;
	return cur;
}

struct operand* create_strdatoperand(char* str)
{
	struct operand* opd = calloc(1, sizeof(struct operand));
	
	opd->type = OPD_STRDAT;
	opd->constval = strlen(str) + 1; // Don't forget the null byte the on end.
	opd->strval = str;
	
	return opd;
}


void geninsn0(int opc)
{
	int cond = (opc >> 16) & 0x0000FFFF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_GEN;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 0;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsn1(int opc, struct operand* opA)
{
	int cond = (opc >> 16) & 0x0000FFFF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_GEN;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 1;
	op->operands = calloc(1, sizeof(struct operand*));
	op->operands[0] = opA;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsn2(int opc, struct operand* opA, struct operand* opB)
{
	int cond = (opc >> 16) & 0x0000FFFF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_GEN;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 2;
	op->operands = calloc(2, sizeof(struct operand*));
	op->operands[0] = opA;
	op->operands[1] = opB;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsn3(int opc, struct operand* opA, struct operand* opB, struct operand* opC)
{
	int cond = (opc >> 16) & 0x0000FFFF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_GEN;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 3;
	op->operands = calloc(3, sizeof(struct operand*));
	op->operands[0] = opA;
	op->operands[1] = opB;
	op->operands[2] = opC;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void genmiscinsn4(int opc, struct operand* opA, struct operand* opB, struct operand* opC, struct operand* opD)
{
	int cond = (opc >> 16) & 0x0000FFFF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_MISC;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 4;
	op->operands = calloc(4, sizeof(struct operand*));
	op->operands[0] = opA;
	op->operands[1] = opB;
	op->operands[2] = opC;
    op->operands[3] = opD;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}


void geninsnmisc(int opc, struct operand* opA)
{
	int cond = (opc >> 16) & 0x0000FFFF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_MISC;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 1;
	op->operands = calloc(1, sizeof(struct operand*));
	op->operands[0] = opA;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsnnop()
{
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_MISC;
	op->opcode = OP_NOP;
	op->condcode = COND_ALWAYS;
	op->at_pc = cur_pc;
	
	op->n_operands = 0;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}


void geninsnldst(int opc, struct operand* reg, struct operand* mem)
{
	int cond = (opc >> 16) & 0x000000FF;
	int memsize = (opc >> 24) & 0x000000FF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_LDST;
	op->opcode = opc;
	op->condcode = cond;
	op->memsize = memsize;
	op->at_pc = cur_pc;
	
	op->n_operands = 2;
	op->operands = calloc(2, sizeof(struct operand*));
	op->operands[0] = reg;
	op->operands[1] = mem;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsnvecldst(int vecop, struct operand* vecrf, struct operand* mem)
{
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_VECLDST;
	op->vec_is_store = ((vecop & 0xFFFF0000) != 0);
	op->vec_bit_size = (vecop & 0x0000FFFF);
	op->at_pc = cur_pc;
	
	op->n_operands = 2;
	op->operands = calloc(2, sizeof(struct operand*));
	op->operands[0] = vecrf;
	op->operands[1] = mem;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsnlea(struct operand* reg, struct operand* offset)
{
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_MISC;
	op->opcode = OP_LEA;
	op->condcode = COND_ALWAYS;
	op->at_pc = cur_pc;
	
	op->n_operands = 2;
	op->operands = calloc(2, sizeof(struct operand*));
	op->operands[0] = reg;
	op->operands[1] = offset;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}


void geninsnpp1(int opc, struct operand* ppop)
{
	int cond = (opc >> 16) & 0x000000FF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_PUSHPOP;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 1;
	op->operands = calloc(1, sizeof(struct operand*));
	op->operands[0] = ppop;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}

void geninsnpp2(int opc, struct operand* ppop, struct operand* lrpc)
{
	int cond = (opc >> 16) & 0x000000FF;
	opc &= 0x0000FFFF;
	
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_PUSHPOP;
	op->opcode = opc;
	op->condcode = cond;
	op->at_pc = cur_pc;
	
	op->n_operands = 2;
	op->operands = calloc(2, sizeof(struct operand*));
	op->operands[0] = ppop;
	op->operands[1] = lrpc;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}


void geninsnspace(int size)
{
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_SPACE;
	op->at_pc = cur_pc;
	cur_pc += size;
	
	op->n_operands = 1;
	op->operands = calloc(1, sizeof(struct operand*));
	op->operands[0] = create_operand_const(size);
	
	list_add_tail(&op->list, &opslist);
}


void geninsnintdat(int datsize, struct operand* opdlist)
{
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_DAT;
	op->opcode = datsize;
	op->at_pc = cur_pc;
	
	op->n_operands = 1;
	op->operands = calloc(1, sizeof(struct operand*));
	op->operands[0] = opdlist;
	
	cur_pc += insnlen(op);
	list_add_tail(&op->list, &opslist);
}


void geninsnstrdat(struct operand* strop)
{
	struct operation* op = calloc(1, sizeof(*op));
	
	op->optype = OPTYPE_DAT;
	op->opcode = DAT_STRING;
	op->at_pc = cur_pc;
	cur_pc += strop->constval;
	
	op->n_operands = 1;
	op->operands = calloc(1, sizeof(struct operand*));
	op->operands[0] = strop;
	
	list_add_tail(&op->list, &opslist);
}


int insnlen(struct operation* op)
{
	return get_op_length(op);
}

