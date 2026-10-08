#ifndef COMPILER_STRUCTMOVES_H
#define COMPILER_STRUCTMOVES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void StructMoves_EmitCopy(Operand *sourceOperand, Operand *targetOperand, unsigned int size,
                                 unsigned int alignment);
extern void emit_load_store_copy(Operand *destination, Operand *source, int size);
extern void emit_pair_copy_loop(Operand *destination, Operand *source, int size);
extern void emit_unrolled_copy(Operand *dest, Operand *src, SInt32 size, SInt32 align);
extern void fn_0051aee0(Operand *node, SInt32 offset);
extern void StructMoves_PrepareOperandForOffset(Operand *operand, unsigned int offset, int n);

#ifdef __cplusplus
}
#endif

#endif
