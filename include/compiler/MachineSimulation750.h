#ifndef COMPILER_MACHINESIMULATION750_H
#define COMPILER_MACHINESIMULATION750_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int get_opcode_table_first_entry(PCodeInstruction *instruction);
extern void advance_simulation_pipeline(void);
extern void record_instruction_kind(PCodeInstruction *p);
extern void reset_simulation_pipeline(void);
extern int fn_0052f330(PCodeInstruction *instruction);
extern int fn_0052f120(struct PCodeInstruction *instruction);

#ifdef __cplusplus
}
#endif

#endif
