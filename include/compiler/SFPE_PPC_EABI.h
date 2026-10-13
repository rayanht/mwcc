#ifndef COMPILER_SFPE_PPC_EABI_H
#define COMPILER_SFPE_PPC_EABI_H

#include "compiler/common.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void SFPE_PPC_EABI_GenerateConversion(ENode *expression, short requested_register, short requested_pair_register,
                                             Operand *result);
extern int SFPE_PPC_EABI_GenerateComparison(ENode *node, Operand *result, int branch);
extern void SFPE_PPC_EABI_GenerateMonadicOperand(ENode *input, short firstRegister, short secondRegister,
                                                 Operand *result);
extern void SFPE_PPC_EABI_GenerateAssignment(ENode *g, SInt16 requestedRegister, SInt16 requestedRegisterPair,
                                             Operand *res);
extern void SFPE_PPC_EABI_ToggleHighBit(ENode *expr, SInt16 regA, SInt16 regB, Operand *res);
#if VERSION == VERSION_GC_1_0
extern void SFPE_PPC_EABI_MakeIntvalOperand(ENode *node, SInt16 reg1, SInt16 reg2, Operand *op);
#endif
extern void SFPE_PPC_EABI_LoadFloatConstant(ENode *node, SInt16 regA, SInt16 regB, Operand *out);
extern void SFPE_PPC_EABI_GenerateFloatPostIncDec(ENode *expr, SInt16 outputReg, SInt16 outputReg2, Operand *result);
extern void SFPE_PPC_EABI_GenerateDiadicArithmetic(ENode *node, SInt16 arg2, SInt16 arg3, Operand *result);
extern struct Object *data_00587584;
extern struct Object *data_005875ac;
extern struct Object *data_005875bc;
extern struct Object *data_005875f4;
#if VERSION > VERSION_GC_1_0
extern short return_gpr_first;
extern short returnRegHi;
extern SInt16 sfpe_right_operand_reg;
extern SInt16 sfpe_right_operand_reg_hi;
#else
/* (1.0 has no such globals: a runtime routine takes and returns a register pair big-endian) */
enum { returnRegHi = 3, return_gpr_first = 4, sfpe_right_operand_reg_hi = 5, sfpe_right_operand_reg = 6 };
#endif

#ifdef __cplusplus
}
#endif

#endif
