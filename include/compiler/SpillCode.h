#ifndef COMPILER_SPILLCODE_H
#define COMPILER_SPILLCODE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void SpillCode_RewriteSpilledRegisterMove(void *unused, PCodeInstruction *pc);
extern void SpillCode_EmitOperandSpills(PCodeBlock *unused, PCodeInstruction *op);
extern void SpillCode_ReplaceInstructionWithFPRSpillCode(PCodeBlock *block, PCodeInstruction *instruction);
extern void SpillCode_RewriteSpilledVR(PCodeBlock *block, PCodeInstruction *instruction);
extern void SpillCode_RewriteSpilledFPRs(PCodeBlock *unused, PCodeInstruction *instruction);
extern void SpillCode_InsertGPRSpillCode(PCodeBlock *block, PCodeInstruction *instruction);
extern short spill_address_register;
extern void SpillCode_ComputeSpillCosts(int reg_class);
extern void SpillCode_SpillRegisters(int reg_class, int register_count);

#ifdef __cplusplus
}
#endif

#endif
