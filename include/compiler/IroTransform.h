#ifndef COMPILER_IROTRANSFORM_H
#define COMPILER_IROTRANSFORM_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void fold_nested_diadic_intval(ENode *node);
extern ENode *walk_expr_postorder(ENode *expr);
extern ENode *canonicalize_diadic_expression(ENode *expression);
extern ENode *IroTransform_CombineEAddTerms(ENode *expression);
extern void IroTransform_SimplifyLinear(void);
extern void remove_redundant_monadic_ops(IROLinear *expression);
extern void fn_00450720(IROLinear *o);
extern void fn_004507d0(IROLinear *node);
extern void remove_common_op(struct IROLinear *expr);
extern void simplify_diadic_matching_child(IROLinear *node);
extern void simplify_matching_monadic_operands(IROLinear *node);
extern IROLinear *rotate_left_child(IROLinear *nd, int isleft);
extern void rewrite_diadic_as_rtype_matched_monadic(IROLinear *expression);
extern void rewrite_diadic_as_monadic(IROLinear *node);
extern void simplify_diadic_nodes(IROLinear *nd);
extern void collect_eadd_terms(ENode *n);
extern void simplify_same_linears(IROLinear *node);
extern void simplify_diadic_constants(IROLinear *node);
extern ENodeList *eadd_terms_tail;
extern struct ENodeList *eadd_terms;
extern void simplify_diadic_with_monadic_operand(IROLinear *expr);

#ifdef __cplusplus
}
#endif

#endif
