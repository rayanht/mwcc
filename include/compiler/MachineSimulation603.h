#ifndef COMPILER_MACHINESIMULATION603_H
#define COMPILER_MACHINESIMULATION603_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One opcode's entry in a machine model's table: its class there, and its length. */
struct MachineOpcodeInfo {
    UInt8 executionUnit;
    SInt8 latency;
    UInt8 initialStageCycles;
    UInt8 secondStageCycles;
    UInt8 thirdStageCycles;
    UInt8
        fourthStageCycles; /* 0x05: MachineSimulation601.c advance_instruction_pipeline loads sclass_pipeline_table[opcode * 6 + 3] for the fourth stage countdown. */
};
extern int get_opcode_table_entry(PCodeInstruction *instruction);
extern void retire_and_advance_pending_instructions(void);
extern void assign_entry_to_execution_unit(short *entry);
extern int fn_0052dfa0(PCodeInstruction *instruction);
extern void fn_0052e000(void);
extern char opcode_table[];
struct PipelineStage {
    PCodeInstruction *instruction;
    int value;
};
extern SInt8 opcode_simulation_table[];
extern SInt32 get_adjusted_latency(struct PCodeInstruction *p);
extern MachineOpcodeInfo machine_opcode_info[];

#ifdef __cplusplus
}
#endif

#endif
