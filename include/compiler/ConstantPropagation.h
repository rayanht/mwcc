#ifndef COMPILER_CONSTANTPROPAGATION_H
#define COMPILER_CONSTANTPROPAGATION_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum PCodeInstructionFlags {
    PCodeInstruction_SkipCodeMotion = 0x0004,
    PCodeInstruction_ImplicitUse = 0x0008,
    PCodeInstruction_ImplicitDefinition = 0x0010,
    PCodeInstruction_NullObjectMemory = 0x0040,
    PCodeInstruction_CloneExtraOperandExcluded = 0x0080,
    PCodeInstruction_CloneExtraOperand = 0x0200,
    PCodeInstruction_CoalesceDisabled = 0x0400,
    PCodeInstruction_CopySourceExclusion = 0x0800,
    PCodeInstruction_GPRFixedRange = 0x0020,
    PCodeInstruction_GPRPairInterference = 0x8000,
    PCodeInstruction_ObjectFlag1 = 0x10000,
    PCodeInstruction_ObjectFlag2 = 0x20000
};
#pragma options align = mac68k
struct CBlockData {
    UInt32 *generatedLoads;
    UInt32 *killedLoads;
    UInt32 unusedStorage[2];
};
#pragma options align = reset
extern void ConstantPropagation_PropagateConstantsInBlock(struct PCodeBlock *block);
extern struct PCodeInstruction *find_dlocal_addi(PCodeOperand *operand, SInt16 *size_out, SInt16 displacement);
extern void ConstantPropagation_FindUniqueDefinitions(struct PCodeBlock *state);
extern int gConstantPropagationChanged;
extern void COpt_LoadDeletion(void);
extern int gLoadDeletionChanged;
extern struct CBlockData *data_00587c98;
extern void COpt_ConstantPropagation(void);

#ifdef __cplusplus
}
#endif

#endif
