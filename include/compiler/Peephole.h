#ifndef COMPILER_PEEPHOLE_H
#define COMPILER_PEEPHOLE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k

#pragma options align = reset
#pragma options align = mac68k
struct CmpCtx {
    struct PCodeInstruction *first;
    struct PCodeInstruction *second;
    UInt8 unusedBytes[0x0c];
    UInt16 errorCode;
};
#pragma options align = reset
#pragma options align = mac68k
struct PeepHandler {
    struct PeepHandler *next;
    union {
        int (*func)(struct PCodeInstruction *, UInt32, UInt32, UInt32, UInt32);
        int (*instruction_func)(PCodeInstruction *);
    };
};
#pragma options align = reset
#pragma options align = mac68k
struct RegisterBlockLiveness {
    UInt32 use;
    UInt32 def;
    UInt32 live_in;
    UInt32 live_out;
};
#pragma options align = reset
extern struct PCodeBlock *gReturnBlock;
extern void Peephole_MergeAdjacentBlocks(Object *object, Boolean mergePrologue);
extern void Peephole_OptimizeBlocks(Object *object);
extern void peephole_optimize_block(PCodeBlock *scope);
extern SInt32 fold_rlwimi_store_to_stwbrx(PCodeInstruction *node, UInt32 mask);
extern unsigned int fold_to_rlwnm(unsigned int instruction, unsigned int liveRegisters);
extern int has_register_conflict(UInt32 mask, PCodeInstruction *group, PCodeInstruction *operand,
                                 PCodeInstruction *other, PCodeInstruction *target);
extern unsigned int combine_srawi(PCodeInstruction *pc, UInt32 mask);
extern int fold_rlwimi_rlwinm_to_sthbrx(PCodeInstruction *instruction, int mask);
extern unsigned int combine_addi(PCodeInstruction *pc, UInt32 mask);
extern int fold_rlwinm_or_mr_reaching_def(PCodeInstruction *pc, UInt32 mask);
extern int eliminate_redundant_store(PCodeInstruction *in);
extern unsigned int rewrite_reaching_def_reg(PCodeInstruction *p1, SInt32 p2, UInt32 p3);
extern unsigned int retarget_reaching_def_register(PCodeInstruction *p1, UInt32 p3, SInt32 p2);
extern unsigned int fold_not_into_andc(PCodeInstruction *pc, UInt32 mask);
extern int remove_redundant_extsb(PCodeInstruction *self, UInt32 mask);
extern SInt32 bypass_cmpli_zero(PCodeInstruction *p);
extern SInt32 fold_constant_compare_branch(PCodeInstruction *node);
extern int eliminate_matching_addi(PCodeInstruction *instruction, UInt32 registerMask);
extern int fold_reaching_lha_or_extsb(PCodeInstruction *p, UInt32 mask);
extern int fold_lbz_lbzx_mask(PCodeInstruction *instruction, int register_mask);
extern int make_record_form_and_unlink_instruction(PCodeInstruction *instruction);
extern int fn_004cba60(CmpCtx *ctx);
extern int unlink_instruction_matching_vmr_source(PCodeInstruction *p);
extern int unlink_instruction_for_unmatched_fmr(PCodeInstruction *p);
extern int unlink_instruction_with_matching_mr_def(PCodeInstruction *p);
extern int unlink_same_reg_move(PCodeInstruction *instruction);
extern int unlink_same_reg_instruction(PCodeInstruction *object);
extern unsigned int unlink_equal_reg_instruction(PCodeInstruction *record);
extern int fn_004cbe40(PCodeBlock *node, PCodeInstruction *item, int instructionCount, int branchCount, int targetCount,
                       int memoryCount, int specialCount);
extern int has_reg_flag_one_before_flag_two(PCodeInstruction *list, int hash);
extern int fn_004cc040(PCodeInstruction *p, UInt32 m0, UInt32 m1, UInt32 m2, UInt32 m3, Boolean flag);
extern void build_reaching_def_table(PCodeBlock *info);
extern void build_register_block_liveness(Object *object);
extern void compute_register_block_liveness(RegisterBlockLiveness *data, UInt32 mask);
extern int combine_mulli(struct PCodeInstruction *state, int register_mask);
extern int merge_reaching_def_instruction(PCodeInstruction *instruction, int operand1, int operand2, int operand3,
                                          int registerMask);
extern int replace_with_reaching_li(PCodeInstruction *instruction, int register_mask);
extern int fold_reaching_addi(PCodeInstruction *current);
extern int fold_li_operand(PCodeInstruction *instruction, int register_mask);
extern int fold_lhz_lhzx_mask(PCodeInstruction *instruction, int register_mask);
extern int bypass_extsh_for_rotated_mask(struct PCodeInstruction *pattern, int registerMask);
extern int bypass_extsb_for_low_byte_rotated_mask(PCodeInstruction *instruction, int register_mask);
extern void initialize_register_block_liveness(void);
extern int fold_addi_or_mr_reaching_def(PCodeInstruction *pc, UInt32 mask);
extern int rewrite_as_addi(PCodeInstruction *record);
extern void register_peephole_rules(void);
extern void Peephole_VisitBlocksWithMultipleInstructions(void *arg1);
extern void optimize_rlwinm_and_addi(PCodeBlock *block);
extern unsigned int make_contiguous_mask(unsigned int value);
extern SInt32 compute_register_mask(PCodeInstruction *node, SInt16 registerNumber);

#ifdef __cplusplus
}
#endif

#endif
