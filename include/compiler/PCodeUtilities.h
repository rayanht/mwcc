#ifndef COMPILER_PCODEUTILITIES_H
#define COMPILER_PCODEUTILITIES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PCodeInstruction *PCodeUtilities_MakeInstructionWithObject(short opcode, short operand, Object *object,
                                                                  short flags, char appendToBlock);
extern void PCodeUtilities_EmitAddress(short resultReg, short baseReg, struct Object *object, short offset);
extern void PCodeUtilities_EmitObjectInstructionWithPayload(Object *operand, SInt16 emitInstruction, SInt32 value1,
                                                            SInt32 value2, SInt32 value3);
extern PCodeOperand *PCodeUtilities_004a2290(PCodeOperand *p, UInt32 mask0, UInt32 mask1, UInt32 mask2);
extern void PCodeUtilities_EmitInstructionAndCreateBlock(Object *a0);
extern unsigned int PCodeUtilities_EmitConditionalBranch(unsigned int a0, PCodeLabel *a1);
extern void PCodeUtilities_EmitBranch(PCodeLabel *a0);
extern PCodeInstruction *PCodeUtilities_CreateInstructionWithObject(short operand1, short operand2, Object *operand3,
                                                                    short operand4, unsigned char recordInstruction);
extern void PCodeUtilities_EmitConditionBranch(SInt16 operand, SInt16 condition, SInt16 branchIfTrue,
                                               PCodeLabel *targetBlock);
extern void PCodeUtilities_ResolveLabel(PCodeLabel *data);
extern PCodeInstruction *PCodeUtilities_EmitInstruction(short opcode, ...);
extern PCodeInstruction *PCodeUtilities_CreateInstruction(UInt16 op, ...);
extern PCodeInstruction *create_pcode_instruction(SInt16 opcode, char *args);
extern void PCodeUtilities_MakeRecordForm(PCodeInstruction *o);
extern void PCodeUtilities_EmitLoadImmediate(SInt16 destination, SInt32 value);
extern void PCodeUtilities_LoadImmediate(SInt16 target, SInt32 value);
extern void fn_004a1cb0(int first, int second, int third);
extern void emit_opcode_with_base_offset(short opcode, short dest_reg, short base_reg, Object *obj, SInt32 offset);
extern struct Statement *gCurrentStatement;
extern struct PCodeBlock *gCurrentBlock;

#ifdef __cplusplus
}
#endif

#endif
