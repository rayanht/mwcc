#ifndef COMPILER_MACHINESIMULATION750_H
#define COMPILER_MACHINESIMULATION750_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OptPair {
    struct PCodeInstruction
        *instruction; /* 0x00: record_opt_arg stores p; advance_simulation_pipeline matches completed instructions */
    SInt32
        completed; /* 0x04: record_opt_arg clears; advance_simulation_pipeline sets on completion and tests before retirement */
};
struct PoolEntry {
    struct PCodeInstruction *obj;
    SInt32 cnt;
};
extern int get_opcode_table_first_entry(PCodeInstruction *instruction);
extern void advance_simulation_pipeline(void);
extern void record_instruction_kind(PCodeInstruction *p);
extern void reset_simulation_pipeline(void);
extern int fn_0052f330(PCodeInstruction *instruction);
extern int fn_0052f120(struct PCodeInstruction *instruction);
extern struct OpcodeOperandInfo {
    char count;
    UInt8 reserved[4];
    UInt8 kind;
} DAT_00576f29[];
extern char opcode_table_entries[];
extern struct OpcodeScheduleInfo {
    UInt8 kind;
    UInt8 reserved[2];
    SInt8 latency3;
    SInt8 latency4;
    UInt8 reserved5;
} data_00576f28[];

#ifdef __cplusplus
}
#endif

#endif
