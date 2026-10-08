#ifndef COMPILER_OPERANDS_H
#define COMPILER_OPERANDS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void Operands_ConvertSignedIntegerToFloat(struct Operand *operand, char subtract, short resultReg);
extern void Operands_ExtendGPR(Operand *operand, Type *type, short requestedReg);
extern void Operands_MoveToNewGPR(Operand *op, short outputReg);
extern void Operands_ConvertIntegerToFloat(struct Operand *result, Boolean useOpcodeA5, SInt16 requestedRegister);
extern void Operands_ConvertFloatToInteger(Operand *operand, SInt16 reg);
extern void Operands_ExtractBitfield(Operand *operand, TypeBitfield *tbitfield, SInt16 reg, Operand *result);
extern void Operands_EmitAddress(SInt16 reg, Operand *operand);
extern void Operands_Normalize(Operand *op);
extern void Operands_EmitOpcodeWithObjectBaseOffset(short dest, Type *type, Object *obj);
extern unsigned int Operands_InsertBitField(unsigned short reg, Operand *operand, TypeBitfield *record);
extern void Operands_Add(Operand *op1, Operand *op2, SInt16 hint, Operand *dest);
extern void Operands_EmitSTVX(SInt16 reg, Operand *operand, Type *type);
extern void Operands_EmitGPRMemoryInstruction(SInt16 reg, Operand *node, Type *type);
extern void Operands_EmitTypedGPRMemoryInstruction(short reg, Operand *opnd, Type *type);
extern void Operands_StoreGPRPair(SInt16 reg, SInt16 regHi, Operand *op, Type *type);
extern void Operands_MakeIndirect(Operand *op, ENode *e);
extern void Operands_ForceFPR(Operand *operand, Type *type, short requestedReg);
extern void Operands_ForceVR(Operand *operand, Type *type, short targetReg);
extern void Operands_ForceGPR(Operand *node, Type *type, SInt16 reg);
extern void Operands_ForceGPRPair(Operand *op, Type *type, SInt16 first, SInt16 second);
extern short low_word_offset;
extern short high_word_offset;
extern Float float_one;
extern Float data_0055e9e8;
extern Float data_0055e9f0;
extern void Operands_ClearTrailingObjectInfo(void);
extern int Operands_IsLogicalExpression(ENode *enode);
extern int fn_0049f630(ENode *expr);
extern void fn_0049f560(void);
extern struct ObjectList *data_00587660;
extern SInt32 data_00588200;
extern unsigned char data_005883f0[];
extern UInt8 data_00588508;

#ifdef __cplusplus
}
#endif

#endif
