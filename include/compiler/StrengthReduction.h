#ifndef COMPILER_STRENGTHREDUCTION_H
#define COMPILER_STRENGTHREDUCTION_H

#include "compiler/common.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CMBody;
struct CMNode {
    struct CMNode *next;
    struct CMBody *body;
};
#pragma options align = mac68k
struct CMRegisterNode {
    struct CMRegisterNode *next;
    struct Loop *loop;
    struct CodeMotionCandidate *candidates;
    struct CodeMotionRef *refs;
    struct PCodeInstruction *reachingDefinition;
    SInt32 increment;
    SInt16 reg;
};
#pragma options align = reset
#pragma options align = mac68k
struct ClassLookupResult {
    SInt32 value;
    SInt16 index;
    SInt16 kind60Value;
    SInt16 kind63Value;
    SInt32 extraValue;
};
#pragma options align = reset
#pragma options align = mac68k
struct CodeMotionCandidate {
    struct CodeMotionCandidate *next;
    struct CMRegisterNode *owner;
    struct PCodeInstruction *instruction;
    struct PCodeInstruction *replacement_instruction;
    struct Loop *lastLoop;
    unsigned int scale;
    SInt16 base_operand;
    SInt16 source_operand;
    SInt16 source_register;
    SInt16 destination_register;
};
#pragma options align = reset
#pragma options align = mac68k
struct CodeMotionRef {
    struct CodeMotionRef *next;
    struct PCodeInstruction *def;
};
#pragma options align = reset

struct RegisterRewriteEntry {
    int unknown_00;
    struct RegisterRewriteEntry *link;
    unsigned char unknown_08[8];
    union {
        struct PCodeInstruction *instruction;
        struct PCodeBlock *block;
    } value;
};
struct ScaleTarget {
    struct ScaleTarget *next;
    unsigned int *words;
};
struct ScaleTargetGroup {
    unsigned char unknown_00[12];
    struct ScaleTarget *targets;
    unsigned char unknown_10[4];
    unsigned int factor;
};

extern void rewrite_and_relocate_addi_definition(Loop *obj, struct CMRegisterNode *firstId,
                                                 struct CMRegisterNode *secondId, SInt32 offset);
extern void StrengthReduction_RunLoopPasses(void);
extern void rewrite_code_motion_candidates(struct CMRegisterNode *func);
extern void hoist_child_code_motion_instructions(SInt16 operandIndex, int mode, void *destination,
                                                 struct Loop *context);
extern void insert_scaled_increment(CodeMotionCandidate *request);
extern void add_code_motion_candidate(struct CMRegisterNode *list, PCodeInstruction *attributes,
                                      unsigned int argumentValue, short firstValue, short secondValue,
                                      Loop *thirdValue);
extern int visit_code_motion_searches(Loop *p);
extern void fn_00527290(Loop *holder);
extern int matches_redundancy_without_prior_reg_use(CodeMotionCandidate *entry, SInt32 stride, unsigned int reg,
                                                    unsigned int displacement, Loop *outerLoop, Loop *innerLoop);
extern void initialize_candidate_register(CodeMotionCandidate *state);
extern void collect_code_motion_candidates(Loop *context);
extern int visit_loops_children_first(register Loop *node);
extern void fn_005275a0(struct CMRegisterNode *input, ClassLookupResult *result);
extern void fn_00527e80(Loop *node);
extern void find_addi_code_motion_candidates(Loop *cm);
extern void add_code_motion_search(Loop *block, SInt16 reg, SInt32 param3);
extern PCodeInstruction *fn_005288e0(Loop *search, SInt16 reg);
extern SInt32 check_strength_reduction_use(struct CMRegisterNode *info, SInt32 useIndex, SInt32 *value,
                                           SInt16 *operandIndex, SInt16 *otherOperandIndex, Loop **lastBlock);

#ifdef __cplusplus
}
#endif

#endif
