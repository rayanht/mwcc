#ifndef COMPILER_MACHINESIMULATION7400_H
#define COMPILER_MACHINESIMULATION7400_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CountedSlot {
    PCodeInstruction
        *instruction; /* 0x00: queue_instruction stores obj; advance_pipeline retrieves completedInstruction */
    SInt32
        status; /* 0x04: queue_instruction sets pipeline cost or queue completion flag; advance_pipeline decrements pipeline countdown and marks queue completion */
};
extern int fn_0052f370(PCodeInstruction *instruction);
extern int lookup_instruction_opcode_entry(PCodeInstruction *instruction);
extern void advance_pipeline(void);
extern void queue_instruction(PCodeInstruction *obj);
extern void reset_pipeline_state(void);
extern int can_issue_instruction_in_pipeline_slots(PCodeInstruction *node);
struct OpcodeScheduleInfo {
    UInt8 kind; /* 0x00: can_issue_instruction_in_pipeline_slots selects the execution slot by opcode kind */
    UInt8
        baseLatency; /* 0x01: get_adjusted_opcode_table_value reads DAT_00577661 at opcode * 7 and adds flag and multiple-register adjustments */
    SInt8 cost;      /* 0x02: queue_instruction initializes the execution slot countdown */
    UInt8 stage2Latency;    /* 0x03: advance_pipeline / Advance initializes the second pipeline stage countdown */
    UInt8 stage3Latency;    /* 0x04: advance_pipeline / Advance initializes the third pipeline stage countdown */
    UInt8 stage4Latency;    /* 0x05: advance_pipeline / Advance initializes the fourth pipeline stage countdown */
    SInt8 opcodeEntryValue; /* 0x06: lookup_instruction_opcode_entry returns this opcode's signed table entry */
};
extern int get_adjusted_opcode_table_value(PCodeInstruction *record);

#ifdef __cplusplus
}
#endif

#endif
