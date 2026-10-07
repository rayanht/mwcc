#ifndef COMPILER_MACHINESIMULATION7400_H
#define COMPILER_MACHINESIMULATION7400_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int fn_0052f370(PCodeInstruction *instruction);
extern int lookup_instruction_opcode_entry(PCodeInstruction *instruction);
extern void advance_pipeline(void);
extern void queue_instruction(PCodeInstruction *obj);
extern void reset_pipeline_state(void);
extern int can_issue_instruction_in_pipeline_slots(PCodeInstruction *node);
struct OpcodeScheduleInfo {
    UInt8 kind;
    UInt8 baseLatency;
    SInt8 cost;
    UInt8 stage2Latency;
    UInt8 stage3Latency;
    UInt8 stage4Latency;
    SInt8 opcodeEntryValue;
};
extern int get_adjusted_opcode_table_value(PCodeInstruction *record);

#ifdef __cplusplus
}
#endif

#endif
