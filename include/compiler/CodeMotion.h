#ifndef COMPILER_CODEMOTION_H
#define COMPILER_CODEMOTION_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum PCodeInstructionFlagMasks {
    PCodeInstruction_GPRResultMask = 0x0018,
    PCodeInstruction_DeadCodeBarrierMask = 0x00020434
};
#pragma pack(push, 2)
union CodeMotionEntryValue {
    short reg;
    unsigned int raw;
    struct Object *object;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct CodeMotionEntry {
    struct PCodeInstruction *instruction;
    unsigned char kind;
    unsigned char is_implicit;
    CodeMotionEntryValue value;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct CodeMotionEntryLink {
    struct CodeMotionEntryLink *next;
    int entry_index;
};
#pragma pack(pop)
struct CodeMotionObjectNode {
    struct CodeMotionObjectNode *allocation_next;
    struct CodeMotionObjectNode *left;
    struct CodeMotionObjectNode *right;
    struct Object *object;
    CodeMotionEntryLink *use_entries;
    CodeMotionEntryLink *definition_entries;
};
#pragma options align = mac68k
struct CMDefInfo {
    UInt8 kind;        /* 0x00: move_instruction_to_preheader views CodeMotionEntry.kind */
    UInt8 is_implicit; /* 0x01: move_instruction_to_preheader views CodeMotionEntry.is_implicit */
    union {
        SInt16 reg;            /* 0x02: move_instruction_to_preheader, kind 0, 1 or 9 indexes register heads */
        struct Object *object; /* 0x02: fn_00526950, other kinds pass object to find_object_node */
    } u;                       /* 0x02: move_instruction_to_preheader selects by definition kind */
};
#pragma options align = reset
extern int COpt_005266e0(int definitionIndex, Loop *ctx);
extern void unswitch_loop(Loop *context);
extern PCodeBlock *clone_block_with_bridge(Loop *region, PCodeBlock *insertionPoint, PCodeBlock *source,
                                           PCodeBlock *destination);
extern void CodeMotion_00525e70(Loop *region, PCodeBlock *block, PCodeInstruction *instruction,
                                PCodeInstruction *replacement, PCodeOperand *argument);
extern void fn_00525f20(PCodeBlock *replacement, PCodeBlock *block, PCodeBlock *successor);
extern void replace_successor(PCodeBlock *block, PCodeBlock *oldSuccessor, PCodeBlock *newSuccessor);
extern unsigned int fn_00525fc0(PCodeInstruction *node, Loop *arg2, UInt32 *arg3);
extern SInt32 CodeMotion_00526070(PCodeInstruction *definition, Loop *context);
extern PCodeBlockLink *collect_single_successor_memberblocks(Loop *cm, PCodeBlock *cur);
extern void propagate_use_sets(void);
extern void solve_definition_sets(void);
extern void compute_block_definition_and_use_sets(void);
extern void move_instruction_to_preheader(PCodeInstruction *instruction, Loop *region);
extern struct CodeMotionDataflowState {
    UInt32 *definition_sets[4];
    UInt32 *use_sets[4];
} *data_00587fe4;
extern int has_use_outside_loop_in_preheader_set(PCodeOperand *ref, Loop *info);
extern int fn_00526950(int itemIndex, int targetIndex);
extern int is_loop_invariant(PCodeInstruction *info, Loop *ctx, UInt32 *defs, SInt32 p4, SInt32 p5);
extern void COpt_SetLoopCodeMotionMode(int mode);
extern void build_use_definition_entries(int include_implicit);
extern void assign_definition_and_use_starts(int flag);
extern int is_object_access_compatible(PCodeInstruction *instruction, Object *object);
extern void COpt_00524b20(Object *object);
extern CodeMotionObjectNode *find_object_node(Object *object);
extern void CodeMotion_VisitLoops(void);
extern void visit_loops_postorder(register Loop *node);
extern void move_instructions_to_preheader(Loop *node);
extern int is_only_definition_in_loop(PCodeInstruction *def, Loop *ctx);
extern void visit_leaf_loops(Loop *p);
extern struct CodeMotionEntryLink **register_use_entry_heads;
extern struct CodeMotionEntryLink **codeMotionUseEntryHeads;
extern struct CodeMotionEntryLink **code_motion_register_use_heads;
extern struct CodeMotionEntryLink **register_definition_heads;
extern int data_00587e38;
extern SInt32 codeMotionEntryCount;
extern struct CodeMotionEntryLink **code_motion_register_definition_heads;
extern struct CodeMotionEntryLink **data_00587f04;
extern struct CodeMotionObjectNode *gCodeMotionAllocationList_005870fc;
extern int gCodeMotionCounter_005880b8;
extern struct CodeMotionObjectNode *gCodeMotionObjectTree_005880ac;
extern struct CodeMotionEntry *code_motion_entries;
extern struct CodeMotionEntry *cm_entries;

#ifdef __cplusplus
}
#endif

#endif
