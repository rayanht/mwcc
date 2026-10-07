#ifndef COMPILER_MACHINESIMULATION601_H
#define COMPILER_MACHINESIMULATION601_H

#include "compiler/common.h"
#include "compiler/MachineSimulation603.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SInt32 get_latency(struct PCodeInstruction *p);
extern Boolean is_execution_unit_seven(int instruction);
extern void advance_instruction_pipeline(void);
extern void set_execution_unit_instruction(PCodeInstruction *instruction);
extern int is_execution_unit_available(PCodeInstruction *instruction);
extern void clear_instruction_and_globals(void);
struct InstructionCountdown {
    PCodeInstruction *instruction;
    unsigned int count;
};

#ifdef __cplusplus
}
#endif

#endif
