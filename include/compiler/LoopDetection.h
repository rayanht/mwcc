#ifndef COMPILER_LOOPDETECTION_H
#define COMPILER_LOOPDETECTION_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct Loop {
    struct Loop *parent;
    struct Loop *sibling;
    struct Loop *children; /* 0x08: hoist_child_code_motion_instructions visits child counting loops */
    struct PCodeBlock
        *body; /* 0x0c: hoist_child_code_motion_instructions tests child body index in backedge_dominators */
    struct PCodeBlock *preheader;
    struct PCodeBlock *footer;
    struct PCodeInstruction *
        inductionUpdate; /* 0x18: LoopDetection.c stores found2, the add-immediate instruction supplying the induction step */
    struct PCodeBlockLink *blocks; /* 0x1c: matches_redundancy_without_prior_reg_use scans loop block instructions */
    UInt32 *memberblocks;
    UInt32 *exitblocks;
    UInt32 *block_membership;
    UInt32 *backedge_dominators; /* 0x2c: hoist_child_code_motion_instructions tests child body membership */
    struct CMRegisterNode
        *codeMotionSearches; /* 0x30: fn_00527290 traverses CMRegisterNode next, head and definition */
    int execution_weight; /* 0x34: create_loops copies body execution_weight; LoopDetection_CreatePreheader assigns parent weight to newBlock */
    SInt32 bodySize;
    SInt32 iterationCount;
    SInt32 lower;
    SInt32 upper;
    SInt32 step;
    UInt8 unknownCondition;
    UInt8 has_call;
    UInt8 uses_count_register;
    UInt8 skip_leaf_pass_4f;
    UInt8 isUnknownCountingLoop;
    UInt8 isKnownCountingLoop;
    UInt8 has_block_flag_40;
    UInt8 has_indexed_load;
    UInt8 has_indexed_store;
    UInt8 lowerType;
    UInt8 upperType;
    UInt8 has_memory_barrier;
};
#pragma options align = reset
#pragma pack(push, 1)
struct SelectedNode {
    struct SelectedNode *next;
    struct PCodeBlock *node;
};
#pragma pack(pop)
extern void LoopDetection_TraverseLoopsPostorder(Loop *p);
extern void LoopDetection_ComputeLoopPropertiesRecursive(void);
extern void compute_loop_properties_recursive(register Loop *node);
extern void LoopDetection_ComputeLoopProperties(Loop *node);
extern void LoopDetection_CreatePreheader(Loop *region);
extern void detect_counting_loop(Loop *loop);
extern unsigned int fn_00522640(unsigned int selector, unsigned int mode, int value, UInt8 *result);
extern SInt32 compute_unsigned_iteration_count(SInt32 op, SInt32 mode, UInt32 a, UInt32 b, SInt32 step, SInt32 *result);
extern int compute_iteration_count(int op, int reg, SInt32 lower, SInt32 upper, SInt32 step, SInt32 *result);
extern void LoopDetection_DetectLoops(void);
extern void create_loops(void);
extern void fn_00523000(Loop *node, Loop **siblings);
extern void compute_loop_block_sets(Loop *w);
extern void LoopDetection_AddBlock(Loop *loop, PCodeBlock *block);
extern struct SelectedNode *collect_nodes_in_predecessor_bitsets(void);
extern void compute_dominators(void);
extern void traverse_loops_postorder(register Loop *node);
extern signed long data_005871a4;
extern int gPCodeBlockCount;
struct Loop;

#ifdef __cplusplus
}
#endif

#endif
