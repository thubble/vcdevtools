#ifndef __INSNS_H
#define __INSNS_H

#include "list.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif // #ifndef ARRAY_SIZE

/*
(define-table p ["mov", "cmn", "add", "bic", "mul", "eor", "sub", "and", "mvn", "ror", "cmp", "rsb", "btst", 
"or", "extu", "max", "bset", "min", "bclr", "adds2", "bchg", "adds4", "adds8", "adds16", "exts", "neg", 
"lsr", "clz", "lsl", "brev", "asr", "abs"])

*/

enum genopcode_values
{
	CODE_MOV,
	CODE_CMN,
	CODE_ADD,
	CODE_BIC,
	CODE_MUL,
	CODE_EOR,
	CODE_SUB,
	CODE_AND,
	CODE_MVN,
	CODE_ROR,
	CODE_CMP,
	CODE_RSB,
	CODE_BTST,
	CODE_OR,
	CODE_EXTU,
	CODE_MAX,
	CODE_BSET,
	CODE_MIN,
	CODE_BCLR,
	CODE_ADDS2,
	CODE_BCHG,
	CODE_ADDS4,
	CODE_ADDS8,
	CODE_ADDS16,
	CODE_EXTS,
	CODE_NEG,
	CODE_LSR,
	CODE_CLZ,
	CODE_LSL,
	CODE_BREV,
	CODE_ASR,
	CODE_ABS,
};

//(define-table c ["eq", "ne", "cs/lo", "cc/hs", "mi", "pl", "vs", "vc", "hi", 
//"ls", "ge", "lt", "gt", "le", "", "f"])
enum condcode_values
{
	CCODE_EQ,
	CCODE_NE,
	CCODE_CS,
	CCODE_CC,
	CCODE_MI,
	CCODE_PL,
	CCODE_VS,
	CCODE_VC,
	CCODE_HI,
	CCODE_LS,
	CCODE_GE,
	CCODE_LT,
	CCODE_GT,
	CCODE_LE,
	CCODE_AL,
	CCODE_F,
};


#define	GENOPS \
	OP(0, MOV), \
	OP(1, CMN), \
	OP(2, ADD), \
	OP(3, BIC), \
	OP(4, MUL), \
	OP(5, EOR), \
	OP(6, SUB), \
	OP(7, AND), \
	OP(8, MVN), \
	OP(9, ROR), \
	OP(10, CMP), \
	OP(11, RSB), \
	OP(12, BTST), \
	OP(13, OR), \
	OP(14, EXTU), \
	OP(15, MAX), \
	OP(16, BSET), \
	OP(17, MIN), \
	OP(18, BCLR), \
	OP(19, BCHG), \
	OP(20, EXTS), \
	OP(21, NEG), \
	OP(22, LSR), \
	OP(23, CLZ), \
	OP(24, LSL), \
	OP(25, BREV), \
	OP(26, ASR), \
	OP(27, ABS),

#define	NUMGENOPS	28

#define PPOPS \
	PPOP(0, PUSH), \
	PPOP(1, POP),

#define	NUMPPOPS	2

#define LSOPS \
	LSOP(0, LD), \
	LSOP(1, ST),

#define	NUMLSOPS	2

#define MISCOPS \
	MISCOP(0, BL), \
	MISCOP(1, B), \
	MISCOP(2, ADDCMPB), \
	MISCOP(3, NOP), \
	MISCOP(4, LEA),

#define	NUMMISCOPS	5

#define	OP(ind, name)		#name
#define	PPOP(ind, name)		#name
#define	LSOP(ind, name)		#name
#define	MISCOP(ind, name)	#name
static char* opcodenames[NUMGENOPS+NUMPPOPS+NUMLSOPS+NUMMISCOPS] = { GENOPS PPOPS LSOPS MISCOPS };
#undef OP
#undef PPOP
#undef LSOP
#undef MISCOP

#define	OP(ind, name)		OP_##name = ind
#define	PPOP(ind, name)		OP_##name = (ind+NUMGENOPS)
#define	LSOP(ind, name)		OP_##name = (ind+NUMGENOPS+NUMPPOPS)
#define	MISCOP(ind, name)	OP_##name = (ind+NUMGENOPS+NUMPPOPS+NUMLSOPS)
enum opcodes { GENOPS PPOPS LSOPS MISCOPS };
#undef OP
#undef PPOP
#undef LSOP
#undef MISCOP


#define	CONDCODES \
	COND(0, EQ), \
	COND(1, NE), \
	COND(2, CS), \
	COND(3, LO), \
	COND(4, CC), \
	COND(5, HS), \
	COND(6, MI), \
	COND(7, PL), \
	COND(8, VS), \
	COND(9, VC), \
	COND(10, HI), \
	COND(11, LS), \
	COND(12, GE), \
	COND(13, LT), \
	COND(14, GT), \
	COND(15, LE), \
	COND(16, F)

#define	NUMCONDCODES	17
#define	COND_ALWAYS		NUMCONDCODES

#define	COND(ind, name)	#name
static char* condcodenames[NUMCONDCODES] = { CONDCODES };
#undef COND

#define	COND(ind, name)	COND_##name = ind
enum condcodes { CONDCODES };
#undef COND


#define	SIZECODES \
	SIZE(0, H), \
	SIZE(1, B), \
	SIZE(2, SH)

#define	NUMSIZECODES	3
#define	SIZE_W			NUMSIZECODES

#define	SIZE(ind, name)	#name
static char* sizecodenames[NUMSIZECODES] = { SIZECODES };
#undef SIZE

#define	SIZE(ind, name)	SIZE_##name = ind
enum sizecodes { SIZECODES };
#undef SIZE


enum dat_type
{
	DAT_STRING,
	DAT_BYTE,
	DAT_HALF,
	DAT_WORD
};

enum op_type
{
	OPTYPE_GEN,
	OPTYPE_LDST,
	OPTYPE_PUSHPOP,
	OPTYPE_MISC,
	OPTYPE_VECLDST,
	OPTYPE_VEC,
	OPTYPE_SPACE,
	OPTYPE_DAT
};

enum operand_type
{
	OPD_GPREG,
	OPD_SYMREF,
	OPD_CONST,
	OPD_CPUID,
	OPD_ADDR_GPREG,
    OPD_ADDR_GPREG_INC,
    OPD_ADDR_GPREG_DEC,
	OPD_ADDR_GPREG_CONSTOFFSET,
	OPD_ADDR_GPREG_GPREGOFFSET,
	OPD_GPREG_RANGE,
	OPD_LRPC,
	OPD_INTDAT,
	OPD_STRDAT,
	OPD_VECLDST,
	OPD_VECRF
};

struct symbol {
	char* name;
	int type;
	int value;
	struct list_head list;
};

struct operand
{
	int type;
	int gpreg;
	int gpreg2;
	struct symbol* sym;
	int constval;
	char* strval;
	
	/* VECTOR SPECIFIC */
	int vrf_is_scalar;
	int vrf_is_vertical;
	int vrf_is_discard;
	int vrf_bit_size; /* 8, 16, 32 */
	int vrf_x;
	int vrf_y;
	int vrf_xinc;
	int vrf_yinc;
	
	struct operand* list_next;
};


struct operation
{
	int opcode;
	int optype;
	int condcode;
	int memsize;
	int at_pc;
	int n_operands;
	struct operand** operands;
	
	/* VECTOR SPECIFIC */
	int vec_rep_count;
	int vec_bit_size;
	int vec_is_store;
	
	struct list_head list;
};

#endif // __INSNS_H
