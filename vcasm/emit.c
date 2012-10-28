#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "emit.h"
#include "insns.h"

extern struct list_head opslist;

typedef void (*op_emit)(struct operation* op, unsigned char* output_buffer);

void emit_gen(struct operation* op, unsigned char* output_buffer);
void emit_ldst(struct operation* op, unsigned char* output_buffer);
void emit_pushpop(struct operation* op, unsigned char* output_buffer);
void emit_misc(struct operation* op, unsigned char* output_buffer);
void emit_space(struct operation* op, unsigned char* output_buffer);
void emit_dat(struct operation* op, unsigned char* output_buffer);

static op_emit emitters[6] = 
{
	emit_gen,
	emit_ldst,
	emit_pushpop,
	emit_misc,
	emit_space,
	emit_dat
};

typedef int (*op_getlen)(struct operation* op);

int getlen_gen(struct operation* op);
int getlen_ldst(struct operation* op);
int getlen_pushpop(struct operation* op);
int getlen_misc(struct operation* op);
int getlen_space(struct operation* op);
int getlen_dat(struct operation* op);

static op_getlen lengthgetters[6] = 
{
	getlen_gen,
	getlen_ldst,
	getlen_pushpop,
	getlen_misc,
	getlen_space,
	getlen_dat
};

void emit8(unsigned char* dest, uint8_t value);
void emit16(unsigned char* dest, uint16_t value);
void emit32(unsigned char* dest, uint32_t value);
void emit32_split(unsigned char* dest, uint16_t value1, uint16_t value2);
void emit48(unsigned char* dest, uint16_t value1, uint32_t value2);

void emit_data(unsigned char* output_buffer)
{
	//sprintf(output_buffer, "TESTING...");

	struct operation* op;
	
	list_for_each_entry(op, &opslist, list) 
	{
		//printf("OP %s\n", opcodenames[op->opcode]);
		(*(emitters[op->optype]))(op, output_buffer);
	}
}


static unsigned char get_bit_width_flags(int memsize)
{
	switch (memsize)
	{
	case SIZE_H:
		return 0x01;
	case SIZE_B:
		return 0x02;
	case SIZE_SH:
		return 0x03;
	case SIZE_W:
	default:
		return 0x00;
	}
}

static unsigned char get_gen_opcode_fullsize(int opcode)
{
	switch (opcode)
	{
	case OP_CMN:
		return CODE_CMN;
	case OP_ADD:
		return CODE_ADD;
	case OP_BIC:
		return CODE_BIC;
	case OP_MUL:
		return CODE_MUL;
	case OP_EOR:
		return CODE_EOR;
	case OP_SUB:
		return CODE_SUB;
	case OP_AND:
		return CODE_AND;
	case OP_MVN:
		return CODE_MVN;
	case OP_ROR:
		return CODE_ROR;
	case OP_CMP:
		return CODE_CMP;
	case OP_RSB:
		return CODE_RSB;
	case OP_BTST:
		return CODE_BTST;
	case OP_OR:
		return CODE_OR;
	case OP_EXTU:
		return CODE_EXTU;
	case OP_MAX:
		return CODE_MAX;
	case OP_BSET:
		return CODE_BSET;
	case OP_MIN:
		return CODE_MIN;
	case OP_BCLR:
		return CODE_BCLR;
	case OP_BCHG:
		return CODE_BCHG;
	case OP_EXTS:
		return CODE_EXTS;
	case OP_NEG:
		return CODE_NEG;
	case OP_LSR:
		return CODE_LSR;
	case OP_CLZ:
		return CODE_CLZ;
	case OP_LSL:
		return CODE_LSL;
	case OP_BREV:
		return CODE_BREV;
	case OP_ASR:
		return CODE_ASR;
	case OP_ABS:
		return CODE_ABS;
	case OP_MOV:
	default:
		return CODE_MOV;
	}
}

static unsigned char get_condcode_fullsize(int condcode)
{
	switch (condcode)
	{
	case COND_EQ:
		return CCODE_EQ;
	case COND_NE:
		return CCODE_NE;
	case COND_CS:
	case COND_LO:
		return CCODE_CS;
	case COND_CC:
	case COND_HS:
		return CCODE_CC;
	case COND_MI:
		return CCODE_MI;
	case COND_PL:
		return CCODE_PL;
	case COND_VS:
		return CCODE_VS;
	case COND_VC:
		return CCODE_VC;
	case COND_HI:
		return CCODE_HI;
	case COND_LS:
		return CCODE_LS;
	case COND_GE:
		return CCODE_GE;
	case COND_LT:
		return CCODE_LT;
	case COND_GT:
		return CCODE_GT;
	case COND_LE:
		return CCODE_LE;
	case COND_F:
		return CCODE_F;
	case COND_ALWAYS:
	default:
		return CCODE_AL;
	}
}

static int32_t get_pc_offset(struct operand* target, struct operation* op)
{
	int target_addr;
	switch (target->type)
	{
	case OPD_SYMREF:
		target_addr = target->sym->value;
		break;
	case OPD_CONST:
	default:
		target_addr = target->constval;
	}
	
	int32_t retval = (target_addr - op->at_pc);
	return retval;
}

static int32_t get_pc_offset_div2(struct operand* target, struct operation* op)
{
	int target_addr;
	switch (target->type)
	{
	case OPD_SYMREF:
		target_addr = target->sym->value;
		break;
	case OPD_CONST:
	default:
		target_addr = target->constval;
	}
	
	int32_t retval = (target_addr - op->at_pc) / 2;
	return retval;
}


void emit_gen(struct operation* op, unsigned char* output_buffer)
{
	if (op->condcode != COND_ALWAYS)
		return; // NOT SUPPORTED YET
	
	unsigned char* dest = output_buffer + op->at_pc;
	
	struct operand* opD = op->operands[0];
	struct operand* opA = op->operands[1];
	
	if (op->n_operands == 2)
	{
		if (opA->type == OPD_CPUID)
		{
			if (op->opcode != OP_MOV || opD->type != OPD_GPREG)
				return;
			// 0000 0000 111d dddd  "mov r%i{d}, cpuid"
			uint16_t val = 0x00E0;
			val |= (opD->gpreg & 0x1F);
			
			emit16(dest, val);
			return;
		}
		if (op->condcode == COND_ALWAYS && opA->type == OPD_GPREG)
		{
			// 010p pppp ssss dddd  "%s{p} r%i{d}, r%i{s}"
			uint16_t val = 0x1 << 14;
			val |= get_gen_opcode_fullsize(op->opcode) << 8;
			val |= (opA->gpreg & 0x0F) << 4;
			val |= (opD->gpreg & 0x0F);
			
			emit16(dest, val);
			return;
		}
		if (opA->type == OPD_CONST || opA->type == OPD_SYMREF)
		{
			int constval;
			if (opA->type == OPD_SYMREF)
				constval = opA->sym->value;
			else
				constval = opA->constval;
			
			// see if constant can fit in 16-bit space
			if (-32767 < constval && constval < 32768)
			{
				// 1011 00pp pppd dddd iiii iiii iiii iiii  "%s{p} r%i{d}, #0x%04x{i}"
				uint32_t val = 0xB0000000;
				val |= get_gen_opcode_fullsize(op->opcode) << 21;
				val |= (opD->gpreg & 0x1F) << 16;
				val |= (constval & 0x0000FFFF);
				
				emit32(dest, val);
				return;
			}
			
			// 48-bit op (32-bit const)
			// 1110 10pp pppd dddd uuuu uuuu uuuu uuuu uuuu uuuu uuuu uuuu  "%s{p} r%i{d}, #0x%08x{u}"
			uint16_t val1 = 0xE800;
			val1 |= ((uint16_t)get_gen_opcode_fullsize(op->opcode)) << 5;
			val1 |= (uint16_t)(opD->gpreg & 0x1F);
			
			emit48(dest, val1, (uint32_t)constval);
			return;
		}
	}
	
	/* triadic instructions are always 32-bit */
	// NOT SUPPORTED YET
}

void emit_ldst(struct operation* op, unsigned char* output_buffer)
{
	unsigned char* dest = output_buffer + op->at_pc;
	
	struct operand* opd_reg = op->operands[0];
	struct operand* opd_mem = op->operands[1];
	
	if (op->condcode == COND_ALWAYS && opd_mem->type == OPD_ADDR_GPREG)
	{
		//0000 1ww0 ssss dddd "ld%s{w} r%i{d}, (r%i{s})"
		//0000 1ww1 ssss dddd "st%s{w} r%i{d}, (r%i{s})"

		uint16_t val = 0x1 << 11;
		val |= get_bit_width_flags(op->memsize) << 9;
		val |= (opd_mem->gpreg & 0x0F) << 4;
		val |= (opd_reg->gpreg & 0x0F);
		if (op->opcode == OP_ST)
			val |= 0x1 << 8;
		
		emit16(dest, val);
		return;
	}
	
	/* OTHER FORMATS NOT SUPPORTED YET */
}

void emit_pushpop(struct operation* op, unsigned char* output_buffer)
{
	unsigned char* dest = output_buffer + op->at_pc;
	
	// NOT SUPPORTED YET
	if (op->n_operands != 2)
		return;
	struct operand* opA = op->operands[0];
	struct operand* opB = op->operands[1];
	
	// NOT SUPPORTED YET
	if (opB->type != OPD_LRPC)
		return;
	if (opA->type != OPD_GPREG || opA->gpreg != 6)
		return;
	
	// 0000 0011 1010 0000  "push r6, lr"
	// 0000 0011 0010 0000  "pop  r6, pc"
	uint16_t val = 0x0320;
	if (op->opcode == OP_PUSH)
		val |= 0x0080;
	
	emit16(dest, val);
}

void emit_misc(struct operation* op, unsigned char* output_buffer)
{
	unsigned char* dest = output_buffer + op->at_pc;
	struct operand* opA = NULL;
	
	switch(op->opcode)
	{
	case OP_BL:
		if (op->n_operands != 1)
			return; /* NOT SUPPORTED YET */
		opA = op->operands[0];
		if (opA->type == OPD_GPREG)
		{
			// 0000 0000 011d dddd  "bl r%i{d}"
			uint16_t val = 0x0060;
			val |= (opA->gpreg & 0x1F);
			
			emit16(dest, val);
			return;
		}
		
		{
			// 1001 xxxx 1ooo oooo oooo oooo oooo oooo  "bl {$+o*2}"
			uint32_t val = 0x90800000;
			val |= (get_pc_offset_div2(opA, op) & 0x007FFFFF);
			
			emit32(dest, val);
			return;
		}
		
	case OP_B:
		if (op->n_operands != 1)
			return; /* NOT SUPPORTED YET */
		opA = op->operands[0];
		if (op->operands[0]->type == OPD_GPREG)
		{
			// 0000 0000 010d dddd  "b r%i{d}"
			uint16_t val = 0x0040;
			val |= (opA->gpreg & 0x1F);
			
			emit16(dest, val);
			return;
		}
		
		{
			/* for now, always use 16-bit, even though there is a 32-bit form */
			/* assuming we don't have huge jump values... */
			// 1001 cccc 0ooo oooo oooo oooo oooo oooo  "b%s{c} {$+o*2}"
			/*uint32_t val = 0x90000000;
			val |= get_condcode_fullsize(op->condcode) << 24;
			val |= (get_pc_offset_div2(opA, op) & 0x007FFFFF);
			
			emit32(dest, val);*/
			
			// 0001 1ccc cooo oooo  "b%s{c} @"
			uint16_t val = 0x1800;
			val |= get_condcode_fullsize(op->condcode) << 7;
			val |= (get_pc_offset_div2(opA, op) & 0x0000007F);
			
			emit16(dest, val);
			return;
		}
		
	case OP_NOP:
		emit16(dest, 0x0001);
		return;
		
	case OP_LEA:
		if (op->n_operands != 2)
			return; /* NOT SUPPORTED */
		opA = op->operands[0];
		if (opA->type != OPD_GPREG)
			return; /* NOT SUPPORTED */
		struct operand* opB = op->operands[1];
		
		// 1110 0101 000d dddd oooo oooo oooo oooo oooo oooo oooo oooo  "lea r%i{d}, 0x%08x{$+o}"
		uint16_t val1 = 0xE500;
		val1 |= (uint16_t)(opA->gpreg & 0x1F);
		
		emit48(dest, val1, get_pc_offset(opB, op));
		return;
	}
}

void emit_space(struct operation* op, unsigned char* output_buffer)
{
	memset(output_buffer + op->at_pc, 0, op->operands[0]->constval);
}

void emit_dat(struct operation* op, unsigned char* output_buffer)
{
	unsigned char* dest = output_buffer + op->at_pc;
	struct operand* opd = op->operands[0];
	
	if (op->opcode == DAT_STRING)
	{
		memcpy(dest, opd->strval, opd->constval);
		return;
	}
	
	while (opd)
	{
		switch (op->opcode)
		{
			// NOTE: Assuming we're running on little-endian!!!
		case DAT_WORD:
			*((uint32_t*)dest) = opd->constval;
			dest += 4;
			break;
		case DAT_HALF:
			*((uint16_t*)dest) = (uint16_t)(opd->constval & 0x0000FFFF);
			dest += 2;
			break;
		case DAT_BYTE:
			*((uint8_t*)dest) = (uint8_t)(opd->constval & 0x000000FF);
			dest += 1;
			break;
		}
	
		opd = opd->list_next;
	}
}


void emit8(unsigned char* dest, uint8_t value)
{
	dest[0] = value;
}

void emit16(unsigned char* dest, uint16_t value)
{
	dest[0] = value & 0x00FF;
	dest[1] = (value >> 8) & 0x00FF;
}

void emit32(unsigned char* dest, uint32_t value)
{
	emit32_split(dest, (uint16_t)((value >> 16) & 0x0000FFFF), (uint16_t)(value & 0x0000FFFF)); 
}

void emit32_split(unsigned char* dest, uint16_t value1, uint16_t value2)
{
	emit16(dest, value1);
	emit16(dest+2, value2);
}

void emit48(unsigned char* dest, uint16_t value1, uint32_t value2)
{
	emit16(dest, value1);
	emit32_split(dest+2, (uint16_t)(value2 & 0x0000FFFF), (uint16_t)((value2 >> 16) & 0x0000FFFF)); 
}





// returns the length (in bytes) required for the op.
int get_op_length(struct operation* op)
{
	return (*(lengthgetters[op->optype]))(op);
}


int getlen_gen(struct operation* op)
{
	struct operand* opD = op->operands[0];
	struct operand* opA = op->operands[1];
	
	if (op->n_operands == 2)
	{
		if (opA->type == OPD_CPUID)
			return 2;
		if (op->condcode == COND_ALWAYS && opA->type == OPD_GPREG)
			return 2;
		if (opA->type == OPD_CONST || opA->type == OPD_SYMREF)
		{
			int constval;
			if (opA->type == OPD_SYMREF)
				constval = opA->sym->value;
			else
				constval = opA->constval;
			
			// see if constant can fit in 16-bit space
			if (-32767 < constval && constval < 32768)
				return 4;
			return 6;
		}
	}
	
	/* triadic instructions are always 32-bit */
	return 4;
}

int getlen_ldst(struct operation* op)
{
	struct operand* opd_mem = op->operands[1];
	
	if (op->condcode == COND_ALWAYS && opd_mem->type == OPD_ADDR_GPREG)
		return 2;
	
	/* NOT SUPPORTED YET */
	return 0;
}

int getlen_pushpop(struct operation* op)
{
	return 2;
}

int getlen_misc(struct operation* op)
{
	switch(op->opcode)
	{
	case OP_BL:
		if (op->operands[0]->type == OPD_GPREG)
			return 2;
		return 4;
	case OP_B:
		if (op->operands[0]->type == OPD_GPREG)
			return 2;
		return 2;
	case OP_NOP:
		return 2;
	case OP_LEA:
		return 6;
	}
}

int getlen_space(struct operation* op)
{
	return op->operands[0]->constval;
}

int getlen_dat(struct operation* op)
{
	struct operand* opd = op->operands[0];
	int len_per_field = 0;
	
	switch (op->opcode)
	{
	case DAT_STRING:
		return opd->constval;
	case DAT_WORD:
		len_per_field = 4;
		break;
	case DAT_HALF:
		len_per_field = 2;
		break;
	case DAT_BYTE:
		len_per_field = 1;
		break;
	}
	
	int len = 0;
	while (opd)
	{
		len += len_per_field;
		opd = opd->list_next;
	}
	
	return len;
}

