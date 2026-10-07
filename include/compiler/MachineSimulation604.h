#ifndef COMPILER_MACHINESIMULATION604_H
#define COMPILER_MACHINESIMULATION604_H

#include "compiler/common.h"
#include "compiler/MachineSimulation603.h"

#ifdef __cplusplus
extern "C" {
#endif

struct InstructionValueEntry {
    struct PCodeInstruction *instruction;
    int value;
};
extern int get_opcode_table_value(PCodeInstruction *instruction);
extern void advance_instruction_stages_and_retire(void);
extern void assign_instruction_to_execution_unit(struct PCodeInstruction *instruction);
extern void fn_0052eb60(void);
extern SInt32 get_size_rec_latency(struct PCodeInstruction *p);
extern int can_issue_instruction(struct PCodeInstruction *instruction);

#ifdef __cplusplus
}
#endif

#endif
