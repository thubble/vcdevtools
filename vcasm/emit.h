#ifndef __EMIT_H
#define __EMIT_H

#include "insns.h"

void emit_data(unsigned char* output_buffer);
int get_op_length(struct operation* op);

#endif // __EMIT_H