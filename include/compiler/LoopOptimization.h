#ifndef COMPILER_LOOPOPTIMIZATION_H
#define COMPILER_LOOPOPTIMIZATION_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LoopVar {
    struct LoopVar *next;
    struct Object *object;
    UInt8 isvalid : 1;
    UInt8 isfloat : 1;
    UInt8 isgpr : 1;
    UInt8 spare : 5;
    SInt32 elemSize;
    SInt32 arraySize;
    SInt32 count;
    SInt32 extra;
    SInt32 values[1];
};
extern void COpt_ArrayToRegister(void);
extern LoopVar *build_array_loopvars(void);
extern void dispatch_counting_loop_transforms(Loop *loop);
extern void convert_to_count_register_loop(Loop *loop);
extern void unroll_ctr_loop(Loop *loop);
extern void unroll_loop_by_factor(Loop *loop);
extern void LoopOptimization_AddMissingSuccessorPredecessors(PCodeBlock *block);
extern void fn_0052b1a0(Loop *loop);
extern PCodeBlock *insert_block_after(PCodeBlock *list, int execution_weight);
extern void unlink_loop_body_edge_and_instructions(Loop *p);
extern void convert_loop_to_count_register(struct Loop *loop);
extern void unroll_counting_loop(Loop *loop);
extern void fn_005289b0(void);
extern void remove_unused_self_addi(Loop *loop);
extern void mark_registers_used_outside_loop(Loop *state);
extern void walk_loop_children_postorder(register Loop *loop);
extern int gArrayToRegisterChanged;
extern int gArrayToRegisterEnabled;
extern int gLoopTransformChanged;
extern void COpt_ConstantPropagation(void);
struct PCodeBlock;

#ifdef __cplusplus
}
#endif

#endif
