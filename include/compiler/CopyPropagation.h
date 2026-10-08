#ifndef COMPILER_COPYPROPAGATION_H
#define COMPILER_COPYPROPAGATION_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct CodeMotionListNode {
    struct CodeMotionListNode *next;
    SInt32 index;
};
#pragma options align = reset
#pragma options align = mac68k
struct CodeMotionRec {
    struct PCodeInstruction *node;
    struct CodeMotionListNode *list;
};
#pragma options align = reset
#pragma options align = mac68k
struct CopyPropagationBitSets {
    UInt32 *gen;
    UInt32 *kill;
    UInt32 *out;
    UInt32 *in;
};
#pragma options align = reset
extern short gInitialObjectGPRLast;
extern int can_propagate_copy_to_use(int def, int use);
extern void CopyPropagation_ReplaceRegisterUses(int index);
extern void CopyPropagation_ComputeInOutSets(void);
extern void CopyPropagation_BuildCodeMotionRecords(void);
extern void CopyPropagation_CountBlockCopies(void);
extern void CopyPropagation_ComputeGenKill(void);
extern UInt32 *block_copy_counts;
extern SInt32 copyCount;
extern SInt32 *blockCopyStartIndices;
extern struct CodeMotionRec *code_motion_records;
extern struct CopyPropagationBitSets *copyPropagationBitSets;

extern void COpt_CopyPropagation(SInt32 mode);

#ifdef __cplusplus
}
#endif

#endif
