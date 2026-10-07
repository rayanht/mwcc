#ifndef COMPILER_IROUTIL_H
#define COMPILER_IROUTIL_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern IROLinear *IroUtil_GetFirstLinear(IROLinear *p);
extern IROLinear *IroUtil_AppendObjectRefAndUse(Object *obj, IROList *list);
extern void IroUtil_InitList(IROList *state);
extern void IroUtil_InsertLinearRangeAfter(IROLinear *first, IROLinear *replacement, IROLinear *object);
extern void visit_linear_range_trees(IROLinear *node, IROLinear *last, void *ctx);
extern void IroUtil_ClearZeroOperands(struct IROLinear *object);
extern void clear_operand_if_zero(Operand *operand, int value);
extern void IroUtil_AppendLinear(IROLinear *object, IROList *chain);
extern void IroUtil_CopyLinearRangeToList(IROLinear *first, IROLinear *last, IROList *argument);
extern IROLinear *IroUtil_FindLabel(CLabel *key, IROLinear *head);
extern IROLinear *IroUtil_FindNextUse(IROLinear *self);
extern void set_expr_node_if_update_type(IROLinear *linear, unsigned int updateType);
extern void remove_linear_range(IROLinear *first, IROLinear *last);
extern void *IroUtil_MoveLinearRangeBeforeObject(IROLinear *object, IROLinear *first, IROLinear *last);
extern void IroUtil_RemoveLinearRange(IROExpr *record);
extern void fn_0044c6b0(IROLinear *object, unsigned int enabled);
extern short IroUtil_IsOne(IROLinear *node);
extern void IroUtil_MoveExprBefore(IROExpr *object, IROLinear *target);
extern IROLinear *IroUtil_ReplaceFirstReference(IROLinear *node, IROLinear *newref);
extern void IroUtil_InsertLinearBefore(IROLinear *newnode, IROLinear *owner, IROLinear *oldnode);
extern short IroUtil_IsZeroConstant(IROLinear *node);
extern void IroUtil_VisitLinearTree(IROLinear *node, void (*visit)(IROLinear *, int));
extern IROLinear *IroUtil_CopyLinearToList(IROLinear *node, IROList *list);
extern IROLinear *IroUtil_ReplaceNextReference(IROLinear *obj, IROLinear *newobj);
extern short IroUtil_0044cad0(IROLinear *p);
extern void visit_linear_postorder(IROLinear *node, void (*visit)(IROLinear *, int));
extern CLabel *IroUtil_NewLabel(void);
extern int IroUtil_AreTypesEqual(Type *a, Type *b);
extern int equal_enode_values(ENode *left, ENode *right);
extern int fn_0044d460(IROLinear *node);
extern int IroUtil_IsTypeOneNodeTypeFiftyOne(IROLinear *record);
extern struct IROLinear *IroUtil_GetLinearRangeStart(struct IROLinear *node);
extern int IroUtil_LinearsSame(IROLinear *a, IROLinear *b);
extern int IroUtil_LinearConstantTreesSame(IROLinear *a, IROLinear *b);
extern SInt16 IroUtil_IsTypeSame(Type *left, Type *right);

#ifdef __cplusplus
}
#endif

#endif
