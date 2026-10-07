#define CERROR_FILE "IroEval.c"
#include "compiler/common.h"
#include "compiler/IroEval.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CDecl.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroFlowgraph.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroTransform.h"
#include "compiler/IroUtil.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"

static UInt8 data_005536f0[75] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int fn_00454bb0(void)
{
    IRONode *obj;
    IRONode *first;
    IRONode *last;
    IRONode *next;
    int found;

    found = 0;
    for (obj = iro_flowgraph_head; obj != NULL; obj = obj->nextnode) {
        if (obj->last->type == IROLinearIf || obj->last->type == IROLinearIfNot) {
            first = last = obj;
            while ((next = obj->nextnode) != NULL &&
                   (next->last->type == IROLinearIf || next->last->type == IROLinearIfNot)) {
                last = (obj = next);
            }
            if (first != last && fn_00454c30(first, last) != 0)
                found = 1;
        }
    }
    if (found != 0) {
        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
    IroVars_CheckTimedLongjmp();
    return found;
}

unsigned int fn_00454c30(IRONode *first, IRONode *limit)
{
    IRONode *entry;
    IRONode *last;
    int changed;
    changed = 0;
    while (first != limit) {
        if ((first->last->type == IROLinearIf) && (fn_00455350(first, first) != 0))
            break;
        first = first->nextnode;
    }
    entry = first;
    if (first != limit) {
        last = first;
        entry = last->nextnode;
        while ((entry != limit) && (entry->last->type == IROLinearIf)) {
            if (fn_00455350(first, entry) != 0) {
                last = entry;
                entry = entry->nextnode;
            } else {
                entry = last;
                break;
            }
        }
        if (((entry == limit) || (entry->last->type == IROLinearIfNot)) && (fn_00455350(first, entry) == 0))
            entry = last;
        if ((first != entry) && (group_adjacent_compare_cases(first, entry) != 0))
            changed = 1;
        if (entry != limit)
            entry = entry->nextnode;
    }
    if ((entry != limit) && (fn_00454c30(entry, limit) != 0))
        changed = 1;
    return changed;
}

static inline Object *get_key(IROLinear *cond)
{
    SInt32 hasKey = 0, t = 0;
    if (cond != NULL) {
        if (cond->nodetype == EEQU)
            t++;
    }
    if (t != 0) {
        if (IroDump_IsType1NodeType50(cond->u.diadic.right) != 0)
            hasKey = 1;
    }
    return hasKey ? IroDump_GetObjRef(cond->u.diadic.left) : NULL;
}

SInt32 group_adjacent_compare_cases(IRONode *first, IRONode *last)
{
    IRONode *node;
    SInt32 count;
    CompareCase *recs;
    SInt32 i, j, k;
    CompareCase *entry;
    ENode *valueRecord;
    CInt64 value;
    IROLinear *object;
    SInt32 result = 0;
    if (first == last)
        return 0;
    node = first;
    count = 0;
    for (; node != last; node = node->nextnode)
        count++;
    recs = (CompareCase *)CompilerTools_AllocatePoolMemory(++count * 0x1c);
    node = first;
    for (i = 0; i < count; i++) {
        recs[i].state = 0;
        recs[i].flag = 0;
        recs[i].node = node;
        recs[i].link = NULL;
        object = node->last->u.branch.cond;
        recs[i].key = get_key(object);
        if (recs[i].key != NULL) {
            valueRecord = node->last->u.branch.cond->u.diadic.right->u.node;
            recs[i].val = valueRecord->data.intval;
            recs[i].flag = 1;
        }
        node = node->nextnode;
    }
    for (j = 0; j < count; j++) {
        if (recs[j].flag == 1 && recs[j].key != NULL) {
            recs[j].flag = -1;
            entry = &recs[j];
            for (k = j + 1; k < count; k++) {
                if (recs[j].key == recs[k].key) {
                    entry->link = &recs[k];
                    entry = entry->link;
                    recs[k].flag = 0;
                }
            }
        }
    }
    for (k = 0; k < count; k++) {
        if (recs[k].flag == -1) {
            for (entry = &recs[k]; entry != NULL; entry = entry->link) {
                if (entry->state == 0) {
                    entry->state = 2;
                    value = entry->val;
                    if (mark_adjacent_compare_cases(&recs[k], entry->link, &value) != 0) {
                        result = 1;
                        fn_00454f80(&recs[k], value);
                    } else {
                        entry->state = -1;
                    }
                }
            }
        }
    }
    return result;
}

int fn_00454f80(CompareCase *list, CInt64 value)
{
    CompareCase *last;
    CompareCase *record;
    IROLinear *expression;
    IROLinear *leftWrapper;
    IROLinear *rightWrapper;
    IROLinear *operation;
    IROLinear *constant;
    int count;
    CompareCase *entry;

    count = 0;
    for (entry = list; entry != NULL; entry = entry->link) {
        if (entry->state == 2) {
            count = count + 1;
            last = entry;
        }
    }
    if (count == 0)
        return 0;

    for (record = list; record != last; record = record->link) {
        if (record->state == 2) {
            record->state = -1;
            IroUtil_ClearZeroOperands(record->node->last);
            IroUtil_ClearZeroOperands(record->node->last->u.branch.cond);
        }
    }

    last->state = -1;
    expression = last->node->last;
    expression->u.branch.cond->nodetype = ELESSEQU;
    expression->u.branch.cond->u.diadic.right->u.node->data.intval.hi = 0;
    expression->u.branch.cond->u.diadic.right->u.node->data.intval.lo = count - 1;

    leftWrapper = IrOptimizer_NewLinear(IROLinearOp1Arg);
    leftWrapper->nodetype = ETYPCON;
    leftWrapper->rtype = get_unsigned_type(expression->u.branch.cond->u.diadic.left->rtype);
    leftWrapper->index = (linear_index_counter = linear_index_counter + 1);

    rightWrapper = IrOptimizer_NewLinear(IROLinearOp1Arg);
    *rightWrapper = *leftWrapper;
    rightWrapper->index = (linear_index_counter = linear_index_counter + 1);

    operation = IrOptimizer_NewLinear(IROLinearOp2Arg);
    operation->nodetype = EADD;
    operation->rtype = expression->u.branch.cond->u.diadic.left->rtype;
    operation->index = (linear_index_counter = linear_index_counter + 1);

    constant = IrOptimizer_NewLinear(IROLinearOperand);
    constant->nodetype = EINTCONST;
    constant->rtype = expression->u.branch.cond->u.diadic.left->rtype;
    constant->index = (linear_index_counter = linear_index_counter + 1);

    constant->u.node = IrOptimizer_NewENode(0x32);
    constant->u.node->data.intval = CInt64_Neg(value);
    constant->u.node->rtype = constant->rtype;

    leftWrapper->next = expression->u.branch.cond->u.diadic.left->next;
    expression->u.branch.cond->u.diadic.left->next = constant;
    constant->next = operation;
    operation->next = leftWrapper;
    rightWrapper->next = expression->u.branch.cond->u.diadic.right->next;
    expression->u.branch.cond->u.diadic.right->next = rightWrapper;
    leftWrapper->u.monadic = operation;
    operation->u.diadic.left = expression->u.branch.cond->u.diadic.left;
    operation->u.diadic.right = constant;
    expression->u.branch.cond->u.diadic.left = leftWrapper;
    rightWrapper->u.monadic = expression->u.branch.cond->u.diadic.right;
    expression->u.branch.cond->u.diadic.right = rightWrapper;

    return count;
}

static int IsAdjacent(CInt64 d)
{
    if (CInt64_Equal(d, cint64_one) || CInt64_Equal(d, CInt64_Neg(cint64_one)))
        return 1;
    return 0;
}

SInt32 mark_adjacent_compare_cases(CompareCase *cases, CompareCase *candidate, CInt64 *minimum)
{
    CompareCase *current;

    if (candidate == NULL)
        return 0;
    if (candidate->state != 0)
        return mark_adjacent_compare_cases(cases, candidate->link, minimum);

    current = cases;
    while (current != NULL) {
        if (current->state == 2) {
            CInt64 difference;

            if (CInt64_Equal(candidate->val, current->val)) {
                IroUtil_ClearZeroOperands(current->node->last);
                IroUtil_ClearZeroOperands(current->node->last->u.branch.cond);
                current->state = -1;
                return mark_adjacent_compare_cases(cases, candidate->link, minimum);
            }
            difference = CInt64_Sub(current->val, candidate->val);
            if (IsAdjacent(difference)) {
                candidate->state = 2;
                if (CInt64_Greater(*minimum, current->val))
                    *minimum = current->val;
                if (CInt64_Greater(*minimum, candidate->val))
                    *minimum = candidate->val;
                mark_adjacent_compare_cases(current->link, candidate, minimum);
                mark_adjacent_compare_cases(cases, cases->link, minimum);
                return 1;
            }
        }
        current = current->link;
    }
    return mark_adjacent_compare_cases(cases, candidate->link, minimum);
}

int fn_00455350(IRONode *left, IRONode *right)
{
    Object *result;

    if (left == right) {
        IROLinear *node = left->last->u.branch.cond;
        unsigned int matches = 0;
        int is_kind_17 = (node != NULL && node->nodetype == EEQU);
        if (is_kind_17) {
            if (IroDump_IsType1NodeType50(node->u.diadic.right))
                matches = 1;
        }
        if (matches)
            result = IroDump_GetObjRef(node->u.diadic.left);
        else
            result = NULL;
        if (fn_0044be00(node))
            result = NULL;
        return (int)result;
    } else {
        int matches;
        IROLinear *node = left->last;
        CLabel *label = (CLabel *)node->u.branch.label;
        Object *object = IroDump_GetObjRef(node->u.branch.cond->u.diadic.left);
        int result = 0;
        matches = 0;

        if (starts_with_branch_cond(right)) {
            if (has_label_successor(label, right))
                matches = 1;
        }
        if (matches) {
            if (get_matching_cond_objref(object, right->last->u.branch.cond))
                result = 1;
        }
        return result;
    }
}

int get_matching_cond_objref(Object *expectedResult, IROLinear *cond)
{
    Object *result = NULL;
    int kindMatches = 0;
    int payloadMatches = 0;

    if (cond != NULL && cond->nodetype == EEQU)
        kindMatches = 1;
    if (kindMatches) {
        if (IroDump_IsType1NodeType50(cond->u.diadic.right) != 0)
            payloadMatches = 1;
    }
    if (payloadMatches)
        result = IroDump_GetObjRef(cond->u.diadic.left);
    else
        result = NULL;
    if (expectedResult == NULL || expectedResult == result) {
        if (fn_0044be00(cond) == NULL)
            return (int)result;
    }
    return 0;
}

SInt32 has_label_successor(void *id, IRONode *node)
{
    IROLinear *t = node->last;

    switch (t->type) {
        case IROLinearIf:
            if (id == t->u.label)
                return 1;
            break;
        case IROLinearIfNot: {
            SInt32 i = node->numsucc;
            while (i) {
                if (iroNodesByIndex[node->succ[--i]]->first->u.label == id)
                    return 1;
            }
            break;
        }
    }
    return 0;
}

int starts_with_branch_cond(IRONode *node)
{
    IROLinear *n;

    if (node->numpred <= 1) {
        if (node->last->type == IROLinearIf || node->last->type == IROLinearIfNot) {
            n = node->first;
            while (n != node->last && (n->type == IROLinearNop || n->type == IROLinearLabel))
                n = n->next;
            if (n == find_leftmost_leaf(node->last->u.branch.cond))
                return 1;
        }
    }
    return 0;
}

IROLinear *find_leftmost_leaf(IROLinear *p)
{
    switch (p->type) {
        case IROLinearOp1Arg:
            return find_leftmost_leaf(p->u.monadic);
        case IROLinearOp2Arg:
            return find_leftmost_leaf(p->u.diadic.left);
        case IROLinearOperand:
            return p;
        default:
            return NULL;
    }
}

Type *get_unsigned_type(Type *type)
{
    TypeIntegral *it = (TypeIntegral *)type;
    if (type->type == TYPEENUM || it->type == TYPEPOINTER) {
        if (it->size == stunsignedchar.size)
            return (Type *)&stunsignedchar;
        if (it->size == stunsignedint.size)
            return (Type *)&stunsignedint;
        if (it->size == stunsignedshort.size)
            return (Type *)&stunsignedshort;
        if (it->size == stunsignedlong.size)
            return (Type *)&stunsignedlong;
        return (Type *)&stunsignedlonglong;
    }
    if (it->type != TYPEINT) {
        CError_FATAL(877);
        return NULL;
    }
    if (it == &stbool || it == &stwchar)
        return type;
    if (it == &stchar || it == &stsignedchar || it == &stunsignedchar)
        return (Type *)&stunsignedchar;
    if (it == &stsignedshort || it == &stunsignedshort)
        return (Type *)&stunsignedshort;
    if (it == &stsignedint || it == &stunsignedint)
        return (Type *)&stunsignedint;
    if (it == &stsignedlong || it == &stunsignedlong)
        return (Type *)&stunsignedlong;
    return (Type *)&stunsignedlonglong;
}

int IRO_EvaluateConditionals(void)
{
    IRONode *node;
    IROLinear *s;
    IROLinear *type;
    SInt32 changed;
    SwitchInfo *g;
    SwitchCase *p;
    char found;
    CInt64 v;

    changed = 0;
    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        s = node->last;
        switch (s->type) {
            case IROLinearIf:
            case IROLinearIfNot:
                type = s->u.branch.cond;
                if (IroDump_IsType1NodeType50(type) != 0) {
                    Boolean flag = CInt64_IsZero(&s->u.branch.cond->u.node->data.intval);
                    IroUtil_ClearZeroOperands(s->u.branch.cond);
                    if ((!flag != 0) == (s->type == IROLinearIf))
                        s->type = IROLinearGoto;
                    else
                        s->type = IROLinearNop;
                    changed = 1;
                }
                break;
            case IROLinearSwitch:
                type = s->u.swtch.cond;
                if (IroDump_IsType1NodeType50(type) != 0) {
                    v = s->u.swtch.cond->u.node->data.intval;
                    g = s->u.swtch.info;
                    p = g->cases;
                    IroUtil_ClearZeroOperands(s->u.swtch.cond);
                    s->type = IROLinearGoto;
                    found = 0;
                    while (p) {
                        if (CInt64_Equal(p->min, v)) {
                            found = 1;
                            s->u.label = p->label;
                            break;
                        }
                        p = p->next;
                    }
                    if (!found)
                        s->u.label = g->defaultlabel;
                    changed = 1;
                }
                break;
        }
    }

    if (changed) {
        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
    IroVars_CheckTimedLongjmp();
    return changed;
}

int IRO_ConstantFolding(void)
{
    IROLinear *node;
    int folded;
    int integerResult;
    union {
        Val v;
        long long ll;
        CInt64 i;
    } value;
    Val floatValue;

    for (node = linear_head; node != NULL; node = node->next) {
        switch (node->type) {
            case IROLinearOp1Arg:
                if (IroDump_IsType1NodeType50(node->u.monadic)) {
                    ENode *constant;
                    constant = NULL;
                    folded = 0;
                    value.i = node->u.monadic->u.node->data.intval;
                    if (node->nodetype == ETYPCON && node->rtype->type == TYPEFLOAT) {
                        constant = IrOptimizer_NewENode(EFLOATCONST);
                        constant->data.floatval.data.value = CInt64_ConvertToLongDouble(&value.i);
                        constant->rtype = node->rtype;
                    } else {
                        switch (node->nodetype) {
                            case ETYPCON:
                                folded = 1;
                                break;
                            case ELOGNOT:
                                value.i = CInt64_Not(value.i);
                                folded = 1;
                                break;
                            case EBINNOT:
                                value.i = CInt64_Inv(value.i);
                                folded = 1;
                                break;
                            case EMONMIN:
                                value.i = CInt64_Neg(value.i);
                                folded = 1;
                                break;
                        }
                        if (folded) {
                            IroJump_ConvertCInt64ToType(&value.i, node->rtype);
                            constant = IrOptimizer_NewENode(EINTCONST);
                            constant->rtype = node->rtype;
                            constant->data.intval = value.i;
                        }
                    }
                    if (constant) {
                        node->u.monadic->type = IROLinearNop;
                        node->type = IROLinearOperand;
                        node->u.node = constant;
                    }
                }
                break;
            case IROLinearOp2Arg:
                if (IroDump_IsType1NodeType50(node->u.diadic.left)) {
                    if (!IroDump_IsType1NodeType50(node->u.diadic.right) && data_005536f0[node->nodetype]) {
                        IROLinear *operand = node->u.diadic.right;
                        node->u.diadic.right = node->u.diadic.left;
                        node->u.diadic.left = operand;
                    }
                }
                if (IroDump_IsType1NodeType50(node->u.diadic.right)) {
                    if (node->nodetype == ESUB) {
                        node->nodetype = EADD;
                        if (IroDump_IsType1NodeType50(node->u.diadic.right)) {
                            CInt64 negated;
                            negated = CInt64_Neg(node->u.diadic.right->u.node->data.intval);
                            node->u.diadic.right->u.node->data.intval = negated;
                        } else {
                            union {
                                Float f;
                                long long ll;
                            } negated;
                            negated.f = CMach_CalcFloatMonadic(node->u.diadic.right->rtype, '-',
                                                               node->u.diadic.right->u.node->data.floatval.data.value);
                            node->u.diadic.right->u.node->data.floatval = negated.f;
                        }
                    }
                }
                if (IroDump_IsType1NodeType50(node->u.diadic.right) && data_005536f0[node->nodetype] &&
                    node->u.diadic.left->type == IROLinearOp2Arg && node->u.diadic.left->nodetype == node->nodetype &&
                    node->u.diadic.left->rtype == node->rtype &&
                    IroDump_IsType1NodeType50(node->u.diadic.left->u.diadic.right) &&
                    node->u.diadic.left->u.diadic.right->rtype == node->u.diadic.right->rtype) {
                    IROLinear *nested = node->u.diadic.left;
                    node->u.diadic.left = nested->u.diadic.left;
                    nested->u.diadic.left = nested->u.diadic.right;
                    nested->u.diadic.right = node->u.diadic.right;
                    nested->rtype = nested->u.diadic.left->rtype;
                    node->u.diadic.right = nested;
                    node = nested;
                }
                if (IroDump_IsType1NodeType50(node->u.diadic.left) && IroDump_IsType1NodeType50(node->u.diadic.right)) {
                    union {
                        long long ll;
                        CInt64 i;
                    } leftValue, rightValue;
                    leftValue.i = node->u.diadic.left->u.node->data.intval;
                    rightValue.i = node->u.diadic.right->u.node->data.intval;
                    folded = 0;
                    switch (node->nodetype) {
                        case EADD:
                            value.i = CInt64_Add(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case ESUB:
                            value.i = CInt64_Sub(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case EMUL:
                            if (Type_IsUnsigned(node->rtype))
                                value.i = CInt64_MulU(leftValue.i, rightValue.i);
                            else
                                value.i = CInt64_Mul(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case EDIV:
                            if (Type_IsUnsigned(node->rtype))
                                value.i = CInt64_DivU(leftValue.i, rightValue.i);
                            else
                                value.i = CInt64_Div(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case EMODULO:
                            if (Type_IsUnsigned(node->rtype))
                                value.i = CInt64_ModU(leftValue.i, rightValue.i);
                            else
                                value.i = CInt64_Mod(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case ESHL:
                            value.i = CInt64_Shl(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case ESHR:
                            if (Type_IsUnsigned(node->rtype))
                                value.i = CInt64_ShrU(leftValue.i, rightValue.i);
                            else
                                value.i = CInt64_Shr(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case EAND:
                            value.i = CInt64_And(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case EOR:
                            value.i = CInt64_Or(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case EXOR:
                            value.i = CInt64_Xor(leftValue.i, rightValue.i);
                            folded = 1;
                            break;
                        case ELESS:
                            if (Type_IsUnsigned(node->u.diadic.left->rtype))
                                CInt64_SetULong(&value.i, CInt64_LessU(leftValue.i, rightValue.i));
                            else
                                CInt64_SetULong(&value.i, CInt64_Less(leftValue.i, rightValue.i));
                            folded = 1;
                            break;
                        case EGREATER:
                            if (Type_IsUnsigned(node->u.diadic.left->rtype))
                                CInt64_SetULong(&value.i, CInt64_GreaterU(leftValue.i, rightValue.i));
                            else
                                CInt64_SetULong(&value.i, CInt64_Greater(leftValue.i, rightValue.i));
                            folded = 1;
                            break;
                        case ELESSEQU:
                            if (Type_IsUnsigned(node->u.diadic.left->rtype))
                                CInt64_SetULong(&value.i, CInt64_LessEqualU(leftValue.i, rightValue.i));
                            else
                                CInt64_SetULong(&value.i, CInt64_LessEqual(leftValue.i, rightValue.i));
                            folded = 1;
                            break;
                        case EGREATEREQU:
                            if (Type_IsUnsigned(node->u.diadic.left->rtype))
                                CInt64_SetULong(&value.i, CInt64_GreaterEqualU(leftValue.i, rightValue.i));
                            else
                                CInt64_SetULong(&value.i, CInt64_GreaterEqual(leftValue.i, rightValue.i));
                            folded = 1;
                            break;
                        case EEQU:
                            CInt64_SetULong(&value.i, CInt64_Equal(leftValue.i, rightValue.i));
                            folded = 1;
                            break;
                        case ENOTEQU:
                            CInt64_SetULong(&value.i, CInt64_NotEqual(leftValue.i, rightValue.i));
                            folded = 1;
                            break;
                    }
                    if (folded) {
                        ENode *constant;
                        IroJump_ConvertCInt64ToType(&value.i, node->rtype);
                        constant = IrOptimizer_NewENode(EINTCONST);
                        constant->rtype = node->rtype;
                        constant->data.intval = value.i;
                        node->u.diadic.left->type = IROLinearNop;
                        node->u.diadic.right->type = IROLinearNop;
                        node->type = IROLinearOperand;
                        node->u.node = constant;
                    }
                }
                if (IroUtil_IsTypeOneNodeTypeFiftyOne(node->u.diadic.left) &&
                    IroUtil_IsTypeOneNodeTypeFiftyOne(node->u.diadic.right)) {
                    Val leftValue, rightValue;
                    leftValue.f = node->u.diadic.left->u.node->data.floatval;
                    rightValue.f = node->u.diadic.right->u.node->data.floatval;
                    folded = 0;
                    integerResult = 0;
                    switch (node->nodetype) {
                        case EADD:
                            floatValue.ll = ((long long (*)(Type *, long long, int, long long))CMach_CalcFloatDiadic)(
                                node->rtype, leftValue.ll, '+', rightValue.ll);
                            folded = 1;
                            break;
                        case ESUB:
                            floatValue.ll = ((long long (*)(Type *, long long, int, long long))CMach_CalcFloatDiadic)(
                                node->rtype, leftValue.ll, '-', rightValue.ll);
                            folded = 1;
                            break;
                        case EMUL:
                            floatValue.ll = ((long long (*)(Type *, long long, int, long long))CMach_CalcFloatDiadic)(
                                node->rtype, leftValue.ll, '*', rightValue.ll);
                            folded = 1;
                            break;
                        case EDIV:
                            floatValue.ll = ((long long (*)(Type *, long long, int, long long))CMach_CalcFloatDiadic)(
                                node->rtype, leftValue.ll, '/', rightValue.ll);
                            folded = 1;
                            break;
                        case ELESS:
                            CInt64_SetULong(&value.i,
                                            CMach_CalcFloatDiadicBool(node->rtype, leftValue.d, '<', rightValue.d));
                            folded = 1;
                            integerResult = 1;
                            break;
                        case EGREATER:
                            CInt64_SetULong(&value.i,
                                            CMach_CalcFloatDiadicBool(node->rtype, leftValue.d, '>', rightValue.d));
                            folded = 1;
                            integerResult = 1;
                            break;
                        case ELESSEQU:
                            CInt64_SetULong(&value.i,
                                            CMach_CalcFloatDiadicBool(node->rtype, leftValue.d, 0x16a, rightValue.d));
                            folded = 1;
                            integerResult = 1;
                            break;
                        case EGREATEREQU:
                            CInt64_SetULong(&value.i,
                                            CMach_CalcFloatDiadicBool(node->rtype, leftValue.d, 0x16b, rightValue.d));
                            folded = 1;
                            integerResult = 1;
                            break;
                        case EEQU:
                            CInt64_SetULong(&value.i,
                                            CMach_CalcFloatDiadicBool(node->rtype, leftValue.d, 0x168, rightValue.d));
                            folded = 1;
                            integerResult = 1;
                            break;
                        case ENOTEQU:
                            CInt64_SetULong(&value.i,
                                            CMach_CalcFloatDiadicBool(node->rtype, leftValue.d, 0x169, rightValue.d));
                            folded = 1;
                            integerResult = 1;
                            break;
                    }
                    if (folded) {
                        ENode *constant;
                        if (integerResult) {
                            IroJump_ConvertCInt64ToType(&value.i, node->rtype);
                            constant = IrOptimizer_NewENode(EINTCONST);
                            constant->rtype = node->rtype;
                            constant->data.intval = value.i;
                        } else {
                            constant = IrOptimizer_NewENode(EFLOATCONST);
                            constant->rtype = node->rtype;
                            constant->data.bits = floatValue.ll;
                        }
                        node->u.diadic.left->type = IROLinearNop;
                        node->u.diadic.right->type = IROLinearNop;
                        node->type = IROLinearOperand;
                        node->u.node = constant;
                    }
                }
                break;
        }
    }
    return ((int (*)(void))IroVars_CheckTimedLongjmp)();
}

void convert_cint64_to_bitfield(CInt64 *val, Type *type, TypeBitfield *type2)
{
    UInt32 i;
    UInt32 j;
    UInt32 limit;
    CInt64 work;
    CInt64 work2;

    work = cint64_zero;
    limit = type2->bitlength;
    for (i = 0; i < limit; i++) {
        if (i < 32)
            work.lo = work.lo | (1 << i);
    }
    val->lo &= work.lo;
    val->hi = 0;

    if (!Type_IsUnsigned(type)) {
        work2 = cint64_zero;
        for (j = 0; j <= i - 1; j++) {
            if (j == i - 1)
                work2.lo = work2.lo | (1 << j);
        }
        if (work2.lo & val->lo) {
            for (j = i - 1; j < 32; j++)
                val->lo |= 1 << j;
            val->hi = -1;
        }
    }

    if (Type_IsUnsigned(type)) {
        switch (type->size) {
            case 1:
                CInt64_ConvertUInt8(val);
                break;
            case 2:
                CInt64_ConvertUInt16(val);
                break;
            case 4:
                CInt64_ConvertUInt32(val);
                break;
        }
    } else {
        switch (type->size) {
            case 1:
                CInt64_ConvertInt8(val);
                break;
            case 2:
                CInt64_ConvertInt16(val);
                break;
            case 4:
                CInt64_ConvertInt32(val);
                break;
        }
    }
}
