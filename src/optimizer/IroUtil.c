#define CERROR_FILE "IroUtil.c"
#include "compiler/common.h"
#include "compiler/IroUtil.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CompilerTools.h"
#include "compiler/InstrSelection.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"

static struct IROLinear *linear_range_start;
static IROLinear *linear_range_end;
static struct IRONode *move_expr_before_node;

Object *IroUtil_GetObjRef(IROLinear *linear)
{
    if (linear->type == IROLinearOp1Arg && linear->nodetype == EINDIRECT &&
        linear->u.monadic->type == IROLinearOperand && linear->u.monadic->u.node->type == EOBJREF)
        return linear->u.monadic->u.node->data.objref;
    return NULL;
}

int fn_0044d520(IROLinear *node)
{
    if (node->type == 1U &&
        (node->u.node->type == EINTCONST || node->u.node->type == EVECTOR128CONST || node->u.node->type == EFLOATCONST))
        return 1;
    return 0;
}

SInt32 IroUtil_IsPowerOfTwo(IROLinear *node, SInt32 *bit)
{
    SInt32 value;
    SInt32 mask;
    UInt32 index;
    ENode *expr;

    *bit = -1;
    if (node->type == IROLinearOperand) {
        if ((expr = node->u.node)->type == EINTCONST) {
            if (expr->data.intval.hi != 0)
                return 0;
            value = expr->data.intval.lo;
            mask = 1;
            index = 0;
            do {
                if (mask == value) {
                    *bit = index;
                    return 1;
                }
                index++;
                mask += mask;
            } while (index < 31);
        }
    }
    return 0;
}

unsigned int IroUtil_IsType1NodeType50(IROLinear *linear)
{
    if (linear->type == IROLinearOperand && linear->u.node->type == 50U)
        return 1U;
    return 0U;
}

#pragma auto_inline off
int IroUtil_IsTypeOneNodeTypeFiftyOne(IROLinear *record)
{
    if (record->type == 1U && record->u.node->type == 51U)
        return 1;
    return 0;
}
#pragma auto_inline reset

/* an assignment: a one- or two-operand op whose operator assigns */
int fn_0044d460(IROLinear *node)
{
    if (((node->type == IROLinearOp1Arg) || (node->type == IROLinearOp2Arg)) && (data_00551d6c[node->nodetype] != 0)) {
        return 1;
    }
    return 0;
}

int equal_enode_values(ENode *left, ENode *right)
{
    if (left->type == right->type) {
        switch (left->type) {
            case '2':
                return left->data.intval.lo == right->data.intval.lo && left->data.intval.hi == right->data.intval.hi;
            case '4':
                return 0;
            case '3':
                return left->data.floatval.data.value == right->data.floatval.data.value;
            case 'J':
                return left->data.vector128val.ul[0] == right->data.vector128val.ul[0] &&
                       left->data.vector128val.ul[1] == right->data.vector128val.ul[1] &&
                       left->data.vector128val.ul[2] == right->data.vector128val.ul[2] &&
                       left->data.vector128val.ul[3] == right->data.vector128val.ul[3];
            case '8':
                return left->data.objref == right->data.objref;
        }
    }
    return 0;
}

int IroUtil_AreTypesEqual(Type *a, Type *b)
{
    TypeBitfield *ba;
    TypeBitfield *bb;

    if (IS_TYPE_BITFIELD(a)) {
        if (IS_TYPE_BITFIELD(b)) {
            ba = TYPE_BITFIELD(a);
            bb = TYPE_BITFIELD(b);
            if (ba->bitfieldtype == bb->bitfieldtype && ba->offset == bb->offset && ba->bitlength == bb->bitlength)
                return 1;
        }
        return 0;
    }
    if (IS_TYPE_POINTER_ONLY(a) && IS_TYPE_POINTER_ONLY(b))
        return 1;
    return is_typesame(a, b);
}

SInt16 IroUtil_IsTypeSame(Type *left, Type *right)
{
    if (left->type == 11U && right->type == 11U)
        return 1U;
    return is_typesame(left, right);
}

int IroUtil_LinearConstantTreesSame(IROLinear *a, IROLinear *b)
{
    if (a->type == b->type && IroUtil_AreTypesEqual(a->rtype, b->rtype)) {
        switch (a->type) {
            case IROLinearOperand:
                return equal_enode_values(a->u.node, b->u.node);
            case IROLinearOp1Arg:
                if (a->nodetype == b->nodetype)
                    return IroUtil_LinearConstantTreesSame(a->u.monadic, b->u.monadic);
                return 0;
            case IROLinearOp2Arg:
                if (a->nodetype == b->nodetype)
                    return IroUtil_LinearConstantTreesSame(a->u.diadic.left, b->u.diadic.left) &&
                           IroUtil_LinearConstantTreesSame(a->u.diadic.right, b->u.diadic.right);
                return 0;
            case IROLinearFunccall:
                return 0;
            default:
                return 0;
        }
    }
    return 0;
}

/* A new label, on the function's label list. */
CLabel *IroUtil_NewLabel(void)
{
    CLabel *lift_call_0;
    CLabel *node;
    lift_call_0 = newlabel();
    node = lift_call_0;
    node->next = Labels;
    Labels = node;
    return lift_call_0;
}

int IroUtil_LinearsSame(IROLinear *a, IROLinear *b)
{
    if (a->type == b->type && IroUtil_AreTypesEqual(a->rtype, b->rtype)) {
        int flag = 0;
        switch (a->type) {
            case IROLinearOperand:
                return equal_enode_values(a->u.node, b->u.node);
            case IROLinearOp1Arg:
                if (a->nodetype == b->nodetype)
                    return IroUtil_LinearsSame(a->u.monadic, b->u.monadic);
                return 0;
            case IROLinearOp2Arg:
                if (a->nodetype == b->nodetype) {
                    switch (a->nodetype) {
                        case EMUL:
                        case EADD:
                        case EAND:
                        case EXOR:
                        case EOR:
                        case ELAND:
                        case ELOR:
                            if (!fn_0044be00(a))
                                flag = IroUtil_LinearsSame(a->u.diadic.left, b->u.diadic.right) &&
                                       IroUtil_LinearsSame(a->u.diadic.right, b->u.diadic.left);
                            break;
                    }
                    return flag || (IroUtil_LinearsSame(a->u.diadic.left, b->u.diadic.left) &&
                                    IroUtil_LinearsSame(a->u.diadic.right, b->u.diadic.right));
                }
                return 0;
            case IROLinearFunccall:
                return 0;
            default:
                return 0;
        }
    }
    return 0;
}

static IROLinear *find_previous_linear(IROLinear *object)
{
    IROLinear *current;
    IROLinear *previous;

    current = linear_head;
    previous = current;
    while (current != NULL && current != object) {
        previous = current;
        current = current->next;
    }
    if (current == NULL)
        previous = current;
    return previous;
}

IROLinear *IroUtil_GetFirstLinear(IROLinear *p)
{
    switch (p->type) {
        case IROLinearOperand:
            return p;
        case IROLinearOp1Arg:
            return IroUtil_GetFirstLinear(p->u.monadic);
        case IROLinearOp2Arg:
            if (p->u.diadic.right->index < p->u.diadic.left->index)
                return IroUtil_GetFirstLinear(p->u.diadic.right);
            else
                return IroUtil_GetFirstLinear(p->u.diadic.left);
        case IROLinearFunccall:
            return IroUtil_GetFirstLinear(p->u.funccall.args[(SInt16)(p->u.funccall.argCount - 1)]);
        default:
            CError_FATAL(454);
            return NULL;
    }
}

void *IroUtil_MoveLinearRangeBeforeObject(IROLinear *object, IROLinear *first, IROLinear *last)
{
    IROLinear *previous;
    IROLinear *next;

    previous = find_previous_linear(object);
    next = first->next;
    previous->next = next;
    next = last->next;
    first->next = next;
    last->next = object;
    return next;
}

short IroUtil_IsZeroConstant(IROLinear *node)
{
    return (IroUtil_IsType1NodeType50(node) && CInt64_IsZero(&node->u.node->data.intval)) ||
           (IroUtil_IsTypeOneNodeTypeFiftyOne(node) && CMach_FloatIsZero(node->u.node->data.floatval.data.value));
}

#ifndef TRUE
#endif

static inline Boolean is_one(CInt64 *val)
{
    return val->hi == 0 && val->lo == 1;
}

short fn_0044cad0(IROLinear *p)
{
    return (IroUtil_IsType1NodeType50(p) && is_one(&p->u.node->data.intval)) ||
           (IroUtil_IsTypeOneNodeTypeFiftyOne(p) && CMach_FloatIsOne(p->u.node->data.floatval.data.value));
}

short IroUtil_IsOne(IROLinear *node)
{
    CInt64 value;

    value = CInt64_Neg(node->u.node->data.intval);
    return (IroUtil_IsType1NodeType50(node) && is_one(&value)) ||
           (IroUtil_IsTypeOneNodeTypeFiftyOne(node) && CMach_FloatIsNegOne(node->u.node->data.floatval.data.value));
}

void clear_operand_if_zero(Operand *operand, int value)
{
    if (value == 0) {
        operand->kind = OpndType_GPR;
        operand->object = NULL;
    }
    return;
}

void IroUtil_ClearZeroOperands(struct IROLinear *object)
{
    IroUtil_VisitLinearTree(object, (void (*)(IROLinear *, int))clear_operand_if_zero);
}

void IroUtil_VisitLinearTree(IROLinear *node, void (*visit)(IROLinear *, int))
{
    SInt32 i;

    visit(node, 1);
    switch (node->type) {
        case IROLinearOperand:
            break;
        case IROLinearOp1Arg:
            IroUtil_VisitLinearTree(node->u.monadic, visit);
            break;
        case IROLinearOp2Arg:
            IroUtil_VisitLinearTree(node->u.diadic.left, visit);
            IroUtil_VisitLinearTree(node->u.diadic.right, visit);
            break;
        case IROLinearFunccall:
            IroUtil_VisitLinearTree(node->u.funccall.callee, visit);
            for (i = 0; i < node->u.funccall.argCount; i++)
                IroUtil_VisitLinearTree(node->u.funccall.args[i], visit);
            break;
    }
    visit(node, 0);
}

void visit_linear_postorder(IROLinear *node, void (*visit)(IROLinear *, int))
{
    int i;

    switch (node->type) {
        case IROLinearOperand:
            break;
        case IROLinearOp1Arg:
            visit_linear_postorder(node->u.monadic, visit);
            break;
        case IROLinearOp2Arg:
            visit_linear_postorder(node->u.diadic.left, visit);
            visit_linear_postorder(node->u.diadic.right, visit);
            break;
        case IROLinearFunccall:
            visit_linear_postorder(node->u.funccall.callee, visit);
            for (i = 0; i < node->u.funccall.argCount; ++i)
                visit_linear_postorder(node->u.funccall.args[i], visit);
            break;
    }
    visit(node, 0);
}

void visit_linear_range_trees(IROLinear *node, IROLinear *last, void *ctx)
{
    IROLinear *n;

    for (n = node; n != NULL; n = n->next) {
        switch (n->type) {
            case IROLinearNop:
            case IROLinearAsm:
            case 17:
            case 18:
            case IROLinearEnd:
                break;
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
                IroUtil_VisitLinearTree(n->u.monadic, (void (*)(IROLinear *, int))ctx);
                break;
            case IROLinearOperand:
            case IROLinearOp1Arg:
            case IROLinearOp2Arg:
            case IROLinearFunccall:
                IroUtil_VisitLinearTree(n, (void (*)(IROLinear *, int))ctx);
                break;
            case IROLinearIf:
            case IROLinearIfNot:
                IroUtil_VisitLinearTree(n->u.branch.cond, (void (*)(IROLinear *, int))ctx);
                break;
            case IROLinearReturn:
                if (n->u.monadic != NULL)
                    IroUtil_VisitLinearTree(n->u.monadic, (void (*)(IROLinear *, int))ctx);
                break;
            case IROLinearSwitch:
                IroUtil_VisitLinearTree(n->u.swtch.cond, (void (*)(IROLinear *, int))ctx);
                break;
        }
        if (n == last)
            return;
    }
}

void remove_linear_range(IROLinear *first, IROLinear *last)
{
    IROLinear *node;
    IRONode *range;
    IROLinear *previous;

    for (node = first;; node = node->next) {
        node->stmt = NULL;
        if (node == last)
            break;
    }

    previous = NULL;
    node = linear_head;
    if (node != first) {
        do {
            previous = node;
            node = node->next;
        } while (node != first);
    }

    range = iro_flowgraph_head;
    if (range != NULL) {
        do {
            if (range->first == first) {
                if (range->last == last) {
                    range->last = NULL;
                    range->first = range->last;
                } else {
                    range->first = last->next;
                }
                break;
            }
            if (range->last == last) {
                range->last = previous;
                break;
            }
            range = range->nextnode;
        } while (range != NULL);
    }

    if (previous != NULL)
        previous->next = last->next;
    else
        linear_head = last->next;
}

void IroUtil_InsertLinearBefore(IROLinear *newnode, IROLinear *owner, IROLinear *oldnode)
{
    IRONode *node;
    IROLinear *p;
    IROLinear *prev;

    if (oldnode->type == IROLinearLabel) {
        CError_FATAL(773);
    }

    prev = NULL;
    p = linear_head;
    while (p != oldnode) {
        prev = p;
        p = p->next;
    }

    node = iro_flowgraph_head;
    while (node != NULL) {
        if (node->first == oldnode) {
            node->first = newnode;
            break;
        }
        node = node->nextnode;
    }

    owner->next = oldnode;
    if (prev != NULL) {
        prev->next = newnode;
    } else {
        linear_head = newnode;
    }
}

void IroUtil_InsertLinearRangeAfter(IROLinear *first, IROLinear *replacement, IROLinear *object)
{
    IRONode *entry;

    /* (nothing goes after a goto, if, ifnot or switch) */
    switch (object->type) {
        case IROLinearGoto:
        case IROLinearIf:
        case IROLinearIfNot:
        case IROLinearSwitch:
            CError_FATAL(828);
    }
    entry = iro_flowgraph_head;
    while (entry != NULL) {
        if (entry->last == object) {
            entry->last = replacement;
            break;
        }
        entry = entry->nextnode;
    }
    replacement->next = object->next;
    object->next = first;
}

void fn_0044c6b0(IROLinear *object, unsigned int enabled)
{
    IROLinear *node;
    if (enabled != 0) {
        node = object;
        do {
            if (node == linear_range_start) {
                linear_range_start = object;
                return;
            }
            if (node == linear_range_end)
                return;
            node = node->next;
        } while (node != NULL);
    }
}

void IroUtil_RemoveLinearRange(IROExpr *record)
{
    linear_range_start = linear_range_end = record->linear;
    IroUtil_VisitLinearTree(record->linear, (void (*)(IROLinear *, int))fn_0044c6b0);
    remove_linear_range(linear_range_start, linear_range_end);
}

void set_expr_node_if_update_type(IROLinear *linear, unsigned int updateType)
{
    if (updateType != 0U) {
        if (linear->expr != NULL) {
            linear->expr->node = move_expr_before_node;
        }
    }
}

void IroUtil_MoveExprBefore(IROExpr *object, IROLinear *target)
{
    IRONode *node;
    IROLinear *entry;

    linear_range_start = linear_range_end = object->linear;
    IroUtil_VisitLinearTree(object->linear, (void (*)(IROLinear *, int))fn_0044c6b0);
    if (linear_range_start == target)
        return;
    remove_linear_range(linear_range_start, linear_range_end);
    IroUtil_InsertLinearBefore(linear_range_start, linear_range_end, target);
    node = iro_flowgraph_head;
    while (node != NULL) {
        entry = node->first;
        while (entry != NULL) {
            if (entry == object->linear) {
                object->node = node;
                break;
            }
            if (entry == node->last)
                break;
            entry = entry->next;
        }
        node = node->nextnode;
    }
    move_expr_before_node = object->node;
    IroUtil_VisitLinearTree(object->linear, (void (*)(IROLinear *, int))set_expr_node_if_update_type);
}

void IroUtil_InitList(IROList *state)
{
    state->tail = NULL;
    state->head = state->tail;
}

void IroUtil_AppendLinear(IROLinear *object, IROList *chain)
{
    IROLinear *next;
    if (chain->head)
        chain->tail->next = object;
    else
        chain->head = object;
    chain->tail = object;
    while ((next = chain->tail->next) != NULL)
        chain->tail = next;
}

IROLinear *IroUtil_FindLabel(CLabel *key, IROLinear *head)
{
    IROLinear *entry;
    for (entry = head; entry; entry = entry->next) {
        if (entry->type == IROLinearLabel && entry->u.label == key)
            break;
    }
    if (!entry)
        CError_FATAL(1016);
    return entry;
}

void IroUtil_CopyLinearRangeToList(IROLinear *first, IROLinear *last, IROList *argument)
{
    IROLinear *record;
    for (record = first; record; record = record->next) {
        if (record->type != IROLinearNop && !(record->flags & IROLF_Reffed))
            IroUtil_CopyLinearToList(record, argument);
        if (record == last)
            break;
    }
}

IROLinear *IroUtil_CopyLinearToList(IROLinear *node, IROList *list)
{
    IROLinear *newnode;
    ENode *tmp;
    SInt32 i;

    newnode = IrOptimizer_NewLinear(node->type);
    *newnode = *node;
    newnode->index = ++linear_index_counter;
    newnode->next = NULL;
    newnode->expr = NULL;

    switch (newnode->type) {
        case IROLinearNop:
            break;
        case IROLinearOperand:
            tmp = lalloc(sizeof(*tmp));
            *tmp = *node->u.node;
            newnode->u.node = tmp;
            break;
        case IROLinearOp1Arg:
            newnode->u.monadic = IroUtil_CopyLinearToList(newnode->u.monadic, list);
            break;
        case IROLinearOp2Arg:
            if (node->flags & 0x8000) {
                newnode->u.diadic.right = IroUtil_CopyLinearToList(newnode->u.diadic.right, list);
                newnode->u.diadic.left = IroUtil_CopyLinearToList(newnode->u.diadic.left, list);
            } else {
                newnode->u.diadic.left = IroUtil_CopyLinearToList(newnode->u.diadic.left, list);
                newnode->u.diadic.right = IroUtil_CopyLinearToList(newnode->u.diadic.right, list);
            }
            break;
        case IROLinearFunccall:
            newnode->u.funccall.callee = IroUtil_CopyLinearToList(newnode->u.funccall.callee, list);
            newnode->u.funccall.args =
                (IROLinear **)oalloc(newnode->u.funccall.argCount * sizeof(*newnode->u.funccall.args));
            for (i = 0; i < newnode->u.funccall.argCount; i++)
                newnode->u.funccall.args[i] = IroUtil_CopyLinearToList(node->u.funccall.args[i], list);
            break;
        case IROLinearAsm:
            newnode->u.asm_stmt = InlineAsmPPC_CopyStatement(node->u.asm_stmt);
            break;
    }

    IroUtil_AppendLinear(newnode, list);

    return newnode;
}

IROLinear *IroUtil_AppendObjectRefAndUse(Object *obj, IROList *list)
{
    IROLinear *expressionNode;
    IROLinear *useNode;

    expressionNode = IrOptimizer_NewLinear(IROLinearOperand);
    expressionNode->u.node = create_objectrefnode(obj);
    expressionNode->rtype = expressionNode->u.node->data.objref->type;
    linear_index_counter++;
    expressionNode->index = linear_index_counter;
    expressionNode->flags |= IROLF_Ind;

    IroUtil_AppendLinear(expressionNode, list);

    useNode = IrOptimizer_NewLinear(IROLinearOp1Arg);
    useNode->nodetype = EINDIRECT;
    useNode->rtype = obj->type;
    useNode->u.monadic = expressionNode;
    linear_index_counter++;
    useNode->index = linear_index_counter;
    useNode->next = NULL;

    IroUtil_AppendLinear(useNode, list);

    expressionNode->next = useNode;
    return useNode;
}

IROLinear *IroUtil_FindNextUse(IROLinear *self)
{
    IROLinear *n;
    int i;
    for (n = self->next; n != NULL; n = n->next) {
        switch (n->type) {
            case IROLinearIf:
            case IROLinearIfNot:
                if (n->u.branch.cond == self)
                    return n;
                break;
            case IROLinearReturn:
                if (n->u.monadic == self)
                    return n;
                break;
            case IROLinearOp1Arg:
                if (n->u.monadic == self)
                    return n;
                break;
            case IROLinearSwitch:
                if (n->u.swtch.cond == self)
                    return n;
                break;
            case IROLinearOp2Arg:
                if (n->u.diadic.left == self)
                    return n;
                if (n->u.diadic.right == self)
                    return n;
                break;
            case IROLinearFunccall:
                if (n->u.funccall.callee == self)
                    return n;
                for (i = 0; i < n->u.funccall.argCount; i++)
                    if (n->u.funccall.args[i] == self)
                        return n;
                break;
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearEntry:
            case IROLinearExit:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            default:
                CError_FATAL(1258);
                break;
        }
    }
    return NULL;
}

IROLinear *IroUtil_ReplaceFirstReference(IROLinear *node, IROLinear *newref)
{
    IROLinear *p;
    SInt32 i;

    for (p = node->next; p != NULL; p = p->next) {
        switch (p->type) {
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            case IROLinearIf:
            case IROLinearIfNot:
                if (p->u.branch.cond == node) {
                    IroUtil_ClearZeroOperands(p->u.branch.cond);
                    p->u.branch.cond = newref;
                    return p;
                }
                break;
            case IROLinearReturn:
                if (p->u.monadic == node) {
                    IroUtil_ClearZeroOperands(p->u.monadic);
                    p->u.monadic = newref;
                    return p;
                }
                break;
            case IROLinearOp1Arg:
                if (p->u.monadic == node) {
                    IroUtil_ClearZeroOperands(p->u.monadic);
                    p->u.monadic = newref;
                    return p;
                }
                break;
            case IROLinearSwitch:
                if (p->u.swtch.cond == node) {
                    IroUtil_ClearZeroOperands(p->u.swtch.cond);
                    p->u.swtch.cond = newref;
                    return p;
                }
                break;
            case IROLinearOp2Arg:
                if (p->u.diadic.left == node) {
                    IroUtil_ClearZeroOperands(p->u.diadic.left);
                    p->u.diadic.left = newref;
                    return p;
                }
                if (p->u.diadic.right == node) {
                    IroUtil_ClearZeroOperands(p->u.diadic.right);
                    p->u.diadic.right = newref;
                    return p;
                }
                break;
            case IROLinearFunccall:
                if (p->u.funccall.callee == node) {
                    IroUtil_ClearZeroOperands(p->u.funccall.callee);
                    p->u.funccall.callee = newref;
                    return p;
                }
                for (i = 0; i < p->u.funccall.argCount; i++) {
                    if (p->u.funccall.args[i] == node) {
                        IroUtil_ClearZeroOperands(p->u.funccall.args[i]);
                        p->u.funccall.args[i] = newref;
                        return p;
                    }
                }
                break;
            default:
                CError_FATAL(1391);
        }
    }
    return NULL;
}

IROLinear *IroUtil_ReplaceNextReference(IROLinear *obj, IROLinear *newobj)
{
    IROLinear *node;
    SInt32 i;

    for (node = obj->next; node != NULL; node = node->next) {
        switch (node->type) {
            case IROLinearIf:
            case IROLinearIfNot:
                if (node->u.branch.cond == obj) {
                    node->u.branch.cond = newobj;
                    return node;
                }
                break;
            case IROLinearReturn:
                if (node->u.monadic == obj) {
                    node->u.monadic = newobj;
                    return node;
                }
                break;
            case IROLinearOp1Arg:
                if (node->u.monadic == obj) {
                    node->u.monadic = newobj;
                    return node;
                }
                break;
            case IROLinearSwitch:
                if (node->u.swtch.cond == obj) {
                    node->u.swtch.cond = newobj;
                    return node;
                }
                break;
            case IROLinearOp2Arg:
                if (node->u.diadic.left == obj) {
                    node->u.diadic.left = newobj;
                    return node;
                }
                if (node->u.diadic.right == obj) {
                    node->u.diadic.right = newobj;
                    return node;
                }
                break;
            case IROLinearFunccall:
                if (node->u.funccall.callee == obj) {
                    node->u.funccall.callee = newobj;
                    return node;
                }
                for (i = 0; i < node->u.funccall.argCount; i++) {
                    if (node->u.funccall.args[i] == obj) {
                        node->u.funccall.args[i] = newobj;
                        return node;
                    }
                }
                break;
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            default:
                CError_FATAL(1522);
        }
    }
    return NULL;
}

struct IROLinear *IroUtil_GetLinearRangeStart(struct IROLinear *node)
{
    linear_range_start = linear_range_end = node;
    IroUtil_VisitLinearTree(node, (void (*)(IROLinear *, int))fn_0044c6b0);
    return linear_range_start;
}
