#ifndef COMPILER_IROCSE_H
#define COMPILER_IROCSE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct IROExpr {
    UInt16 index;
    struct IROLinear *linear;
    struct Object *temp;
    struct IRONode *node;
    struct BitVector *depends;
    struct IROExpr *use;
    UInt8 state;
    UInt8 mayTrap;
    UInt8 hasSideEffects;
    UInt8 alignmentPadding;
    struct IROLinear *factor;
    struct VarRecord *var;
    struct IROLinear *expr;
    struct IROExpr *next;
};
#pragma options align = reset
extern UInt8 data_00551d6c[];
extern void IroCSE_RewriteStatementExpressions(Statement *stmt);
extern void rewrite_nested_bitwise_expressions(ENode *en);
extern void traverse_expr_postorder(ENode *expr);
extern ENode *walk_expr_postorder(ENode *expr);
extern void fold_nested_diadic_intval(ENode *node);
extern ENode *data_005881ec;
extern int fn_0044fad0(ENode *expression);
extern CInt64 data_005834f8;
extern int data_005871b8;
extern char nested_bitwise_expressions_rewritten;
extern UInt8 nested_bitwise_type;
extern void IRO_CommonSubs(void);
extern void move_common_sub(IROExpr *group);
extern void create_replacement_temp_assignment(IROExpr *replacement);
extern void IroCSE_CreateTemp(IROExpr *mapping);
extern void IroCSE_ComputeAvailableExpressions(void);
extern void mark_dependent_exprs(IROLinear *node);
extern void IroCSE_BuildLoopExprList(void);
extern void fn_0044f230(IROLinear *expression, IRONode *value);
extern void IroCSE_CollectExpressionVarRefsAndFlags(struct IROLinear *input);
extern void fn_0044f350(IROLinear *expression);
extern IROLinear *fn_0044e340(IROLinear *node, unsigned int clearBit);
extern IROLinear *fn_0044e360(IROLinear *type, unsigned int clearBit);
extern void set_global_if_zero_value_and_flag(IROLinear *type, int value);
extern void IroCSE_ClearExpr(void);
extern IROLinear *IroCSE_CreateTempAssignment(IROExpr *pair);
extern void IroCSE_RemoveExpr(IROExpr *entry);
extern void collect_expression_var_refs_and_flags(IROLinear *e, SInt32 flag);
extern void IroCSE_0044e560(IROLinear *from, IROLinear *to);
extern void IroCSE_ReplaceReference(IROLinear *target, Object *object, IROLinear *reference);
extern void IroCSE_0044f6a0(IROLinear *e, SInt32 flag);
extern unsigned int data_00587630;
extern int data_00587e58;
extern SInt32 data_005880a4;
extern struct BitVector *data_00552b88;
extern unsigned int expression_count;
extern unsigned int data_005870f8;
extern struct IROExpr *expr_tail;
extern struct IROExpr *expr_list;
extern BitVector *killed_exprs;
extern ENode *canonicalize_diadic_expression(ENode *expression);

#ifdef __cplusplus
}
#endif

#endif
