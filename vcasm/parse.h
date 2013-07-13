#ifndef __PARSE_H
#define __PARSE_H

#include "expression.h"

int cur_pc;

int str2opcode(char* str);
int str2opcodecond(char* str);
int str2opcodesize(char* str);
int str2opcodesizecond(char* str);
int str2sizecode(char* str);
int str2condcode(char* str);
int str2vecop(char* str);
int str2vecldstopcode(char* str, int is_st);

struct operand* create_operand_gpreg(int gpreg);
struct operand* create_operand_symref(struct symbol* sym);
struct operand* create_operand_const(int constval);
struct operand* create_operand_cpuid();

struct operand* create_ldstoperand_gpreg(int gpreg);
struct operand* create_ldstoperand_gpreg_inc(int gpreg);
struct operand* create_ldstoperand_gpreg_dec(int gpreg);
struct operand* create_ldstoperand_gpregconstoffset(int gpreg, int constoffset);
struct operand* create_ldstoperand_gpreggpregoffset(int gpreg, int gpreg2);
struct operand* create_vecldstoperand(int gpreg, int constval, int gpreg2);

struct operand* create_ppoperand_gpregrange(int gpreg, int gpreg2);
struct operand* create_ppoperand_lrpc();

struct operand* create_vecoperand(int vecop, int y, int yinc, int x, int xinc);

struct operand* create_intdatlistoperand(int val);
struct operand* create_intdatlistoperand_next(struct operand* cur, struct operand* list);

struct operand* create_strdatoperand(char* str);


void geninsn0(int opc);
void geninsn1(int opc, struct operand* opA);
void geninsn2(int opc, struct operand* opA, struct operand* opB);
void geninsn3(int opc, struct operand* opA, struct operand* opB, struct operand* opC);

void geninsnmisc(int opc, struct operand* opA);
void geninsnnop();
void geninsnlea(struct operand* reg, struct operand* offset);

void geninsnldst(int opc, struct operand* reg, struct operand* mem);

void geninsnvecldst(int vecop, struct operand* vecrf, struct operand* mem);

void geninsnpp1(int opc, struct operand* ppop);
void geninsnpp2(int opc, struct operand* ppop, struct operand* lrpc);

void geninsnspace(int size);

void geninsnintdat(int datsize, struct operand* opdlist);

void geninsnstrdat(struct operand* strop);

#endif // __PARSE_H
