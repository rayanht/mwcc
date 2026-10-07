#ifndef COMPILER_COPTIMIZER_H
#define COMPILER_COPTIMIZER_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct COptBlockLink {
    struct COptBlockLink *next;
    union {
        struct COptBlock *block;
        struct Statement *statement;
    } target;
};
#pragma options align = reset
#pragma options align = mac68k
struct COptBlock {
    struct COptBlock *next;
    COptBlockLink *pred;
    COptBlockLink *succ;
    struct Statement *items;
    SInt16 *referenceBits;
    SInt16 *referenceBarrierBits;
    SInt32 unused;
    SInt16 count;
    UInt8 flag;
};
#pragma options align = reset
#pragma options align = mac68k
struct COptCSE {
    struct COptCSE *next;
    struct COptCSE *left;
    struct COptCSE *right;
    struct COptBlock *block;
    struct ENode *expr;
    struct ENode *last;
    struct ENode *replacement;
    SInt16 uses;
};
#pragma options align = reset
/* Links a common-subexpression occurrence to its candidate group. */
struct OptimizerOccurrence {
    struct OptimizerOccurrence *next;
    struct COptCSE *group;
    struct ENode *expression;
};
extern UInt8 data_0058851f;
extern unsigned char data_00588513;
extern void COptimizer_OptimizeStatementList(Object *unused, Statement *list);
extern void mark_reachable_statements(Statement *input);
extern void COptimizer_RecordObjectUse(Object *object, unsigned char direct_reference);
extern void COptimizer_CountExpressionObjectUses(ENode *expression);
extern void simplify_statement_branches(Statement *stmt);
extern void follow_switch_labels_and_fold_constant(Statement *self);
extern void COptimizer_004bf980(void);
extern void mark_dlocal_reference_bits(ENode *node);
extern void set_bit(SInt16 *p, SInt16 n);
extern UInt16 test_bit(const SInt16 *words, short bit);
extern void COptimizer_004c0470(ENode *node);
extern void invalidate_expr_cse(ENode *expression);
extern ENode *fn_004c07c0(ENode *expr);
extern void eliminate_unreachable_statements(Statement *items);
extern void COptimizer_004c0800(ENode *n);
extern COptCSE *find_or_create_commutative_cse(ENode *expr, COptCSE *left, COptCSE *right);
extern void eliminate_common_subexpressions(void);
extern COptCSE *collect_expr_cse(ENode *expr);
extern Boolean traverse_node_list_reverse(ENodeList *node, FuncArg *value);
extern COptCSE *find_or_create_left_cse(ENode *expr, COptCSE *left);
extern void fold_and_invert_conditional_branch(Statement *s);
extern void remove_unreferenced_labels(Statement *statements);
extern void COptimizer_CheckStmtsForNonVoidFunction(Object *func, Statement *stmt);
extern void set_label_stmt_flag(ENode *a);
extern void propagate_bit_to_preds(COptBlock *node, short bit);
extern void build_opt_blocks(Statement *first);
extern COptCSE *find_or_create_cse(ENode *expr, COptCSE *left, COptCSE *right);
extern COptCSE *find_or_create_unary_cse(ENode *expr, COptCSE *left);
extern void mark_and_propagate_dlocal_reference_bits(void);
extern struct CLabel *data_0058802c;
extern struct COptBlock *opt_blocks;

extern Statement *DumpIR_OptimizeStatements(Object *object, Statement *statements);
extern void DumpIR_OptimizeStatementList(Object *object, Statement *statements);
extern int Registers_GetCSEWeight(COptCSE *tree);
extern Boolean Registers_ContainsCOptCSE(COptCSE *target, COptCSE *node);
extern void Registers_DivideUses(COptCSE *node, SInt16 divisor);
extern void Registers_InvalidateCSE(COptCSE *node);

#ifdef __cplusplus
}
#endif

#endif
