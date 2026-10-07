#ifndef COMPILER_MACHINESIMULATION603_H
#define COMPILER_MACHINESIMULATION603_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int get_opcode_table_entry(PCodeInstruction *instruction);
extern void retire_and_advance_pending_instructions(void);
extern void assign_entry_to_execution_unit(PCodeInstruction *instr);
extern int fn_0052dfa0(PCodeInstruction *instruction);
extern void fn_0052e000(void);
extern SInt32 get_adjusted_latency(struct PCodeInstruction *p);

#ifdef __cplusplus
}
#endif

#endif
