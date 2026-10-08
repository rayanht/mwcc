#ifndef COMPILER_INTERFERENCEGRAPH_H
#define COMPILER_INTERFERENCEGRAPH_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 2)
struct PCodeBlockLiveness {
    unsigned int *use;
    unsigned int *def;
    unsigned int *live_in;
    unsigned int *live_out;
};
#pragma pack(pop)
#pragma options align = mac68k
struct SpillInstruction {
    struct SpillInstruction *next;
    unsigned char unknown_04[16];
    short opcode;
    int flags;
    short operand_count;
    char first_operand;
};
#pragma options align = reset
extern void InterferenceGraph_SpillRegisters(int reg_class, int register_count);
extern void SpillCode_BuildInterference(Object *function, int reg_class, int register_count);
extern void SpillCode_MarkLastUses(SInt32 var, UInt32 count);
extern void SpillCode_MaterializeGraph(UInt32 count);
extern void SpillCode_CoalesceCopies(SInt32 regclass, UInt32 numRegs);
extern void SpillCode_ConstructInterference(SInt32 registerClass, UInt32 registerCount);
extern short gFPRCoalesceFirst;
extern short gGPRCoalesceFirst;
extern struct PCodeBlock *gPCodeBlocks;
extern short gVRCoalesceFirst;

#ifdef __cplusplus
}
#endif

#endif
