#ifndef COMPILER_MACHINESIMULATION821_H
#define COMPILER_MACHINESIMULATION821_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IndexedRecord {
    unsigned char header[0x14];
    short index;
};
struct QueueSlot {
    struct PCodeInstruction *obj;
    SInt32 flag;
};
struct StageSlot {
    struct PCodeInstruction *instruction;
    SInt32 count;
};
extern unsigned char DAT_00578e50[];
extern char DAT_00578e52[];
extern SInt8 data_00578e53[];
extern void fn_00530660(void);
extern void fn_00530830(IndexedRecord *record);
extern int fn_005308b0(struct PCodeInstruction *pcode);
extern void reset_spill_state(void);
extern int get_instruction_opcode_table_entry(PCodeInstruction *instruction);
extern char DAT_00578e55[];
extern int get_instruction_cost(PCodeInstruction *instruction);
extern char instruction_costs[];

#ifdef __cplusplus
}
#endif

#endif
