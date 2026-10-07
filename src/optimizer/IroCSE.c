#define CERROR_FILE "IroCSE.c"
#include "compiler/common.h"
#include "compiler/IroCSE.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
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
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroSubable.h"
#include "compiler/IroTransform.h"
#include "compiler/IroUtil.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

#include "compiler/ENode.h"

struct BitVector *data_00552b88 = NULL;

static void IRO_BitVectorSet(UInt32 bit, BitVector *bv)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

static IROLinear *MakeRef(Object *obj, IROLinear *arg3)
{
    IROLinear *n1;
    IROLinear *n2;

    n1 = IrOptimizer_NewLinear(IROLinearOperand);
    n1->u.node = create_objectrefnode(obj);
    n1->rtype = n1->u.node->data.objref->type;
    n1->index = (UInt16)++linear_index_counter;
    n1->flags |= (IROLF_Reffed | IROLF_Ind);
    n2 = IrOptimizer_NewLinear(IROLinearOp1Arg);
    n2->nodetype = EINDIRECT;
    n2->rtype = obj->type;
    n2->u.monadic = n1;
    n2->index = (UInt16)++linear_index_counter;
    n2->flags |= IROLF_Reffed;
    n1->next = n2;
    IroUtil_InsertLinearRangeAfter(n1, n2, arg3);
    return n2;
}

static void IRO_BitVectorSet_0044ecc0(UInt32 bit, BitVector *bv)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

static void IRO_BitVectorSet_0044ef10(UInt32 bit, BitVector *bv)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

static void IRO_BitVectorSet_0044f3d0(UInt32 bit, BitVector *bv)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

#undef BVSET

static void set_bit_vector_bit(UInt32 bit, BitVector *bv)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

void traverse_expr_postorder(ENode *expr)
{
    switch (expr->type) {
        case EPOSTINC:
        case EPOSTDEC:
        case EPREINC:
        case EPREDEC:
        case EINDIRECT:
        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
        case EFORCELOAD:
        case ETYPCON:
        case EBITFIELD:
            traverse_expr_postorder(expr->data.monadic);
            break;
        case EMUL:
        case EMULV:
        case EDIV:
        case EMODULO:
        case EADDV:
        case ESUBV:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
        case ELAND:
        case ELOR:
        case EASS:
        case EMULASS:
        case EDIVASS:
        case EMODASS:
        case EADDASS:
        case ESUBASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
        case ECOMMA:
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBCLR:
        case EBTST:
        case EBSET:
            traverse_expr_postorder(expr->data.diadic.left);
            traverse_expr_postorder(expr->data.diadic.right);
            break;
        case EFUNCCALL:
        case EFUNCCALLP: {
            ENodeList *arg;
            traverse_expr_postorder(expr->data.funccall.funcref);
            arg = expr->data.funccall.args;
            while (arg) {
                traverse_expr_postorder(arg->node);
                arg = arg->next;
            }
            break;
        }
        case ECOND:
            traverse_expr_postorder(expr->data.cond.cond);
            traverse_expr_postorder(expr->data.cond.expr1);
            traverse_expr_postorder(expr->data.cond.expr2);
            break;
        case ENULLCHECK:
            traverse_expr_postorder(expr->data.diadic.left);
            traverse_expr_postorder(expr->data.diadic.right);
            break;
    }
    fold_nested_diadic_intval(expr);
}

int fn_0044fad0(ENode *node)
{
    ENode *expression = node;
    if (expression->type == nested_bitwise_type) {
        unsigned char left;
        unsigned char right;
        left = fn_0044fad0(expression->data.diadic.left);
        right = fn_0044fad0(expression->data.diadic.right);
        return left & right;
    } else if (expression->type == EINDIRECT) {
        if (expression->data.monadic->type == EOBJREF) {
            ENode *operand = expression->data.monadic;
            if (data_005871b8 == 0) {
                data_005871b8 = (int)operand->data.objref;
                data_005881ec = expression;
                return 1;
            }
            if (operand->data.objref != (Object *)data_005871b8) {
                return 0;
            }
            return 1;
        }
        return 0;
    } else if (expression->type == EINTCONST) {
        if (nested_bitwise_expressions_rewritten != 0) {
            data_005834f8 = expression->data.intval;
            nested_bitwise_expressions_rewritten = 0;
        } else if (nested_bitwise_type == EAND) {
            data_005834f8 = CInt64_And(expression->data.intval, data_005834f8);
        } else if (nested_bitwise_type == EOR) {
            data_005834f8 = CInt64_Or(expression->data.intval, data_005834f8);
        } else if (nested_bitwise_type == EXOR) {
            data_005834f8 = CInt64_Xor(expression->data.intval, data_005834f8);
        }
        return 1;
    }
    return 0;
}

void rewrite_nested_bitwise_expressions(ENode *en)
{
    ENodeList *arg;

    switch ((SInt32)en->type) {
        case EPOSTINC:
        case EPOSTDEC:
        case EPREINC:
        case EPREDEC:
        case EINDIRECT:
        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
        case EFORCELOAD:
        case ETYPCON:
        case EBITFIELD:
            rewrite_nested_bitwise_expressions(en->data.diadic.left);
            break;
        case EMUL:
        case EMULV:
        case EDIV:
        case EMODULO:
        case EADDV:
        case ESUBV:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
        case ELAND:
        case ELOR:
        case EASS:
        case EMULASS:
        case EDIVASS:
        case EMODASS:
        case EADDASS:
        case ESUBASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
        case ECOMMA:
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBCLR:
        case EBTST:
        case EBSET:
            rewrite_nested_bitwise_expressions(en->data.diadic.left);
            rewrite_nested_bitwise_expressions(en->data.diadic.right);
            break;
        case EFUNCCALL:
        case EFUNCCALLP:
            rewrite_nested_bitwise_expressions(en->data.funccall.funcref);
            for (arg = en->data.funccall.args; arg != NULL; arg = arg->next)
                rewrite_nested_bitwise_expressions(arg->node);
            break;
        case ECOND:
            rewrite_nested_bitwise_expressions(en->data.cond.cond);
            rewrite_nested_bitwise_expressions(en->data.cond.expr1);
            rewrite_nested_bitwise_expressions(en->data.cond.expr2);
            break;
        case ENULLCHECK:
            rewrite_nested_bitwise_expressions(en->data.diadic.left);
            rewrite_nested_bitwise_expressions(en->data.diadic.right);
            break;
    }

    if ((en->type == EAND || en->type == EXOR || en->type == EOR) &&
        (en->type == en->data.diadic.left->type || en->type == en->data.diadic.right->type)) {
        data_005871b8 = 0;
        nested_bitwise_type = en->type;
        nested_bitwise_expressions_rewritten = 1;
        data_005881ec = NULL;
        if (fn_0044fad0(en)) {
            en->data.diadic.left = data_005881ec;
            en->data.diadic.right->type = EINTCONST;
            en->data.diadic.right->data.intval = data_005834f8;
        }
    }
}

void IroCSE_RewriteStatementExpressions(Statement *stmt)
{
    Statement *s = stmt;
    Statement *next;

    while (s != NULL) {
        next = s->next;
        switch (s->type) {
            case ST_EXPRESSION:
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_RETURN:
                if (s->expr != NULL) {
                    s->expr = walk_expr_postorder(s->expr);
                    rewrite_nested_bitwise_expressions(s->expr);
                    traverse_expr_postorder(s->expr);
                }
                break;
        }
        s = next;
    }
    IroVars_CheckTimedLongjmp();
}

#define BVSET(bit) set_bit_vector_bit((bit), data_00552b88)

/* As collect_expression_var_refs_and_flags, for a candidate common subexpression (stopping at the first side effect). */
void IroCSE_0044f6a0(IROLinear *e, SInt32 flag)
{
    if (e->rtype != NULL && CParser_IsVolatile(e->rtype, e->nodeflags & 3)) {
        data_005880a4 = 1;
        data_00587630 = 1;
    }
    if (data_00587630 != 0)
        return;

    switch (e->type) {
        case IROLinearOperand: {
            VarRecord *q;
            UInt32 v;
            if (flag == 0)
                break;
            if (e->u.node->type != EOBJREF)
                break;
            q = fn_0044ba70(e->u.node->data.objref, 0, 1);
            if (q) {
                if (is_volatile_object(q->object)) {
                    data_005880a4 = 1;
                    data_00587630 = 1;
                }
                v = q->index;
                BVSET(v);
            } else {
                data_00587630 = 1;
            }
            break;
        }

        case IROLinearOp1Arg: {
            IROLinear *p;
            IROAddrRecord *t;
            if (data_00551d6c[e->nodetype] != 0) {
                data_00587630 = 1;
                return;
            }
            if (e->nodetype == EINDIRECT) {
                p = e->u.monadic;
                if (p->type != IROLinearOperand || p->u.node->type != EOBJREF) {
                    if (p->type == IROLinearOp2Arg && p->nodetype == EADD) {
                        t = IroVars_CreateAddrRecord(p);
                        IroVars_CollectAddrRecordElements(p, t);
                        if (t->numObjRefs != 1) {
                            data_00587e58 = 1;
                            BVSET(0);
                            IroBitVect_Or(noregister_bitvector, data_00552b88);
                        }
                    } else {
                        data_00587e58 = 1;
                        BVSET(0);
                        IroBitVect_Or(noregister_bitvector, data_00552b88);
                    }
                }
            }
            IroCSE_0044f6a0(e->u.monadic, e->nodetype == EINDIRECT);
            break;
        }

        case IROLinearOp2Arg:
            if (data_00551d6c[e->nodetype] != 0) {
                data_00587630 = 1;
                return;
            }
            if ((UInt8)(e->nodetype - 0x0B) <= 1) {
                if (IroDump_IsType1NodeType50(e->u.diadic.right) == 0 ||
                    CInt64_Equal(e->u.diadic.right->u.node->data.intval, cint64_zero) != 0) {
                    data_00587e58 = 1;
                }
            }
            IroCSE_0044f6a0(e->u.diadic.left, flag);
            IroCSE_0044f6a0(e->u.diadic.right, flag);
            break;

        case IROLinearFunccall:
            data_00587630 = 1;
            break;

        default:
            CError_FATAL(136);
            break;
    }
}

#define BVSET(bit) IRO_BitVectorSet_0044f3d0((bit), data_00552b88)

/* Notes the variables E reads (data_00552b88) and whether it has side effects (data_00587630) or reads memory
   through an unknown address (data_00587e58). */
void collect_expression_var_refs_and_flags(IROLinear *e, SInt32 flag)
{
    int i;

    if (e->rtype != NULL && CParser_IsVolatile(e->rtype, e->nodeflags & 3)) {
        data_005880a4 = 1;
        data_00587630 = 1;
    }

    switch (e->type) {
        case IROLinearOperand: {
            VarRecord *q;
            UInt32 v;
            if (flag == 0)
                break;
            if (e->u.node->type != EOBJREF)
                break;
            q = fn_0044ba70(e->u.node->data.objref, 0, 1);
            if (q) {
                if (is_volatile_object(q->object)) {
                    data_005880a4 = 1;
                    data_00587630 = 1;
                }
                v = q->index;
                BVSET(v);
            } else {
                data_00587630 = 1;
            }
            break;
        }

        case IROLinearOp1Arg: {
            IROLinear *p;
            if (data_00551d6c[e->nodetype] != 0)
                data_00587630 = 1;
            if (e->nodetype == EINDIRECT) {
                p = e->u.monadic;
                if (!(p->type == IROLinearOperand && p->u.node->type == EOBJREF)) {
                    if (p->type == IROLinearOp2Arg && p->nodetype == EADD) {
                        data_00587ef4 = 0;
                        fn_0044b4e0(p);
                        if (data_00587ef4 != 1) {
                            data_00587e58 = 1;
                            BVSET(0);
                            IroBitVect_Or(noregister_bitvector, data_00552b88);
                        }
                    } else {
                        data_00587e58 = 1;
                        BVSET(0);
                        IroBitVect_Or(noregister_bitvector, data_00552b88);
                    }
                }
            }
            collect_expression_var_refs_and_flags(e->u.monadic, e->nodetype == EINDIRECT);
            break;
        }

        case IROLinearOp2Arg:
            if (data_00551d6c[e->nodetype] != 0)
                data_00587630 = 1;
            if (e->nodetype == EDIV || e->nodetype == EMODULO) {
                if (IroDump_IsType1NodeType50(e->u.diadic.right) == 0 ||
                    CInt64_Equal(e->u.diadic.right->u.node->data.intval, cint64_zero) != 0) {
                    data_00587e58 = 1;
                }
            }
            collect_expression_var_refs_and_flags(e->u.diadic.left, flag);
            collect_expression_var_refs_and_flags(e->u.diadic.right, flag);
            break;

        case IROLinearFunccall: {
            data_00587630 = 1;
            collect_expression_var_refs_and_flags(e->u.funccall.callee, 0);
            IroBitVect_Or(noregister_bitvector, data_00552b88);
            for (i = e->u.funccall.argCount - 1; i >= 0; i--)
                collect_expression_var_refs_and_flags(e->u.funccall.args[i], 0);
            break;
        }

        default:
            CError_FATAL(279);
            break;
    }
}

void IroCSE_CollectExpressionVarRefsAndFlags(struct IROLinear *input)
{
    IroBitVect_ClearBitVector(data_00552b88);
    data_00587e58 = 0;
    data_00587630 = 0;
    data_005880a4 = 0;
    collect_expression_var_refs_and_flags(input, 0);
}

void fn_0044f350(IROLinear *expression)
{
    IroBitVect_AllocateBitVector(&data_00552b88, iroVarCount + 1U);
    data_00587e58 = 0;
    data_00587630 = 0;
    IroCSE_0044f6a0(expression, 0);
}

void set_global_if_zero_value_and_flag(IROLinear *type, int value)
{
    if (value == 0U && (type->flags & 0x10000U) != 0U)
        data_005870f8 = 1U;
}

/* Records EXPRESSION (in block VALUE) as a candidate common subexpression when it is one. */
void fn_0044f230(IROLinear *expression, IRONode *value)
{
    IROExpr *entry;
    int eligible;

    if (expression->flags & IROLF_Reffed) {
        if (fn_004f0040(expression)) {
            data_005870f8 = 0;
            IroUtil_VisitLinearTree(expression, set_global_if_zero_value_and_flag);
            eligible = 0;
            if (data_005870f8 == 0)
                eligible = 1;
            if (eligible) {
                entry = CompilerTools_AllocatePoolMemory(sizeof(*entry));
                expression_count++;
                entry->index = expression_count;
                entry->linear = expression;
                entry->temp = NULL;
                entry->node = value;
                entry->state = 0;
                IroBitVect_AllocateBitVector(&data_00552b88, iroVarCount + 1);
                data_00587e58 = 0;
                data_00587630 = 0;
                IroCSE_0044f6a0(expression, 0);
                entry->depends = data_00552b88;
                entry->hasSideEffects = data_00587630;
                entry->mayTrap = data_00587e58;
                entry->next = NULL;
                entry->use = NULL;
                if (expr_list != NULL)
                    expr_tail->next = entry;
                else
                    expr_list = entry;
                expr_tail = entry;
                expression->expr = entry;
            }
        }
    }
}

void IroCSE_ClearExpr(void)
{
    IROLinear *node;
    IRONode *block;

    expr_list = expr_tail = NULL;
    expression_count = 0U;
    for (block = iro_flowgraph_head; block; block = block->nextnode) {
        for (node = block->first; node;) {
            node->expr = 0U;
            fn_0044f230(node, block);
            if (node == block->last)
                break;
            node = node->next;
        }
    }
    IroVars_CheckTimedLongjmp();
}

void IroCSE_BuildLoopExprList(void)
{
    IRONode *rec;
    IROLinear *node;

    expr_list = expr_tail = NULL;
    expression_count = 0;
    for (rec = iro_flowgraph_head; rec != NULL; rec = rec->nextnode) {
        if ((rec->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
            (1 << rec->index) & IRO_LoopScratchVector_005880dc->bits[rec->index >> 5]) {
            for (node = rec->first; node != NULL; node = node->next) {
                node->expr = NULL;
                fn_0044f230(node, rec);
                if (node == rec->last)
                    break;
            }
        } else {
            for (node = rec->first; node != NULL; node = node->next) {
                node->expr = NULL;
                if (node == rec->last)
                    break;
            }
        }
    }
}

/* Removes ENTRY from the candidates. */
void IroCSE_RemoveExpr(IROExpr *entry)
{
    IROExpr *previous;
    IROExpr *current;

    current = expr_list;
    previous = NULL;
    if (expr_list != entry) {
        do {
            previous = current;
            current = current->next;
            if (current == NULL) {
                CError_FATAL(470);
            }
        } while (current != entry);
    }
    entry->linear->expr = NULL;
    if (previous != NULL) {
        previous->next = entry->next;
    } else {
        expr_list = entry->next;
    }
}

#define SETBIT(n) IRO_BitVectorSet_0044ef10((n), killed_exprs)

/* The expressions NODE kills: their bits in killed_exprs. */
void mark_dependent_exprs(IROLinear *node)
{
    IROExpr *object;
    VarRecord *referencedObject;
    SInt32 uid;
    UInt32 bit;

    IroBitVect_ClearBitVector(killed_exprs);
    do {
        switch (node->type) {
            case IROLinearOp1Arg:
            case IROLinearOp2Arg:
                if (data_00551d6c[node->nodetype] == 0)
                    continue;
                referencedObject = IroVars_GetOperandVarRecord(node);
                uid = 0;
                if (referencedObject != NULL)
                    uid = referencedObject->index;
                if (uid != 0) {
                    for (object = expr_list; object != NULL; object = object->next) {
                        if ((UInt32)(uid >> 5) < (UInt32)object->depends->size &&
                            (object->depends->bits[uid >> 5] & (1 << uid)) != 0) {
                            bit = object->index;
                            SETBIT(bit);
                        }
                    }
                    continue;
                }
                break;
            case IROLinearAsm:
                IroVars_0044b2d0(node);
                for (object = expr_list; object != NULL; object = object->next)
                    if (IroBitVect_Intersects(object->depends, data_00588018)) {
                        bit = object->index;
                        SETBIT(bit);
                    }
                continue;
            case IROLinearFunccall:
                break;
            default:
                continue;
        }
        for (object = expr_list; object != NULL; object = object->next)
            if (IroBitVect_Intersects(object->depends, noregister_bitvector)) {
                SETBIT(object->index);
            }
    } while (0);
}

void IroCSE_ComputeAvailableExpressions(void)
{
    IRONode *blk;
    IROLinear *node;
    BitVector *marks;
    Boolean changed;
    UInt16 i;
    SInt32 n = 0;

    blk = iro_flowgraph_head;
    IroBitVect_AllocateBitVector(&data_00588018, iroVarCount + 1);
    IroBitVect_AllocateBitVector(&killed_exprs, expression_count + 1);
    for (; blk != NULL; blk = blk->nextnode) {
        IroBitVect_AllocateBitVector(&blk->in, expression_count);
        if (blk->numpred != 0)
            IroBitVect_SetAllBits(blk->in);
        IroBitVect_AllocateBitVector(&blk->kill, expression_count);
        IroBitVect_AllocateBitVector(&blk->gen, expression_count);
        IroBitVect_AllocateBitVector(&blk->out, expression_count);
        for (node = blk->first; node != NULL; node = node->next) {
            if (node->expr != NULL)
                IRO_BitVectorSet_0044ecc0(node->expr->index, (BitVector *)blk->gen);
            mark_dependent_exprs(node);
            IroBitVect_Or(killed_exprs, blk->kill);
            IroBitVect_Subtract(killed_exprs, blk->gen);
            if (node == blk->last)
                break;
            if (n > 0xfa) {
                IroVars_CheckTimedLongjmp();
                n = 0;
            } else
                n++;
        }
        IroBitVect_CopyBitVector(blk->in, blk->out);
        IroBitVect_Subtract(blk->kill, blk->out);
        IroBitVect_Or(blk->gen, blk->out);
    }
    IroVars_CheckTimedLongjmp();
    IroBitVect_AllocateBitVector(&marks, expression_count);
    do {
        IRONode *b;
        changed = 0;
        for (b = iro_flowgraph_head; b != NULL; b = b->nextnode) {
            if (b->numpred == 0) {
                IroBitVect_ClearBitVector(marks);
            } else {
                IroBitVect_SetAllBits(marks);
                for (i = 0; i < b->numpred; i++) {
                    IroBitVect_Intersect(iroNodesByIndex[b->pred[i]]->out, marks);
                }
            }
            if (IroBitVect_AreEqual(marks, b->in) == 0) {
                changed = 1;
                IroBitVect_CopyBitVector(marks, b->in);
            }
            IroBitVect_CopyBitVector(b->in, b->out);
            IroBitVect_Subtract(b->kill, b->out);
            IroBitVect_Or(b->gen, b->out);
        }
        IroVars_CheckTimedLongjmp();
    } while (changed);
}

/* Replaces the operand TARGET with a load of OBJECT (inserted before REFERENCE). */
void IroCSE_ReplaceReference(IROLinear *target, Object *object, IROLinear *reference)
{
    IROLinear *node;
    int index;
    int pass;

    node = target->next;
    for (pass = 0; pass < 2; pass++) {
        while (node != NULL) {
            switch (node->type) {
                case IROLinearIf:
                case IROLinearIfNot:
                    if (node->u.branch.cond == target) {
                        node->u.branch.cond = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.branch.cond->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    break;
                case IROLinearReturn:
                    if (node->u.monadic == target) {
                        node->u.monadic = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.monadic->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    break;
                case IROLinearOp1Arg:
                    if (node->u.monadic == target) {
                        node->u.monadic = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.monadic->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    break;
                case IROLinearSwitch:
                    if (node->u.swtch.cond == target) {
                        node->u.swtch.cond = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.swtch.cond->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    break;
                case IROLinearOp2Arg:
                    if (node->u.diadic.left == target) {
                        node->u.diadic.left = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.diadic.left->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    if (node->u.diadic.right == target) {
                        node->u.diadic.right = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.diadic.right->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    break;
                case IROLinearFunccall:
                    if (node->u.funccall.callee == target) {
                        node->u.funccall.callee = MakeRef(object, reference);
                        if (target->flags & IROLF_LoopInvariant)
                            node->u.funccall.callee->flags |= IROLF_LoopInvariant;
                        return;
                    }
                    for (index = 0; index < node->u.funccall.argCount; index++) {
                        if (node->u.funccall.args[index] == target) {
                            node->u.funccall.args[index] = MakeRef(object, reference);
                            if (target->flags & IROLF_LoopInvariant)
                                node->u.funccall.args[index]->flags |= IROLF_LoopInvariant;
                            return;
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
                case IROLinearEnd:
                    break;
                default:
                    CError_FATAL(844);
            }
            node = node->next;
        }
        node = linear_head;
    }
    IroDump_Print("Oh, oh, did not find reference to replace\n");
}

/* Makes the node using FROM as an operand use TO instead. */
void IroCSE_0044e560(IROLinear *from, IROLinear *to)
{
    int pass;
    IROLinear *ep;

    ep = from->next;
    for (pass = 0; pass < 2; pass++) {
        while (ep != NULL) {
            switch (ep->type) {
                case IROLinearIf:
                case IROLinearIfNot:
                    if (ep->u.branch.cond == from) {
                        ep->u.branch.cond = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    break;
                case IROLinearReturn:
                    if (ep->u.monadic == from) {
                        ep->u.monadic = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    break;
                case IROLinearOp1Arg:
                    if (ep->u.monadic == from) {
                        ep->u.monadic = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    break;
                case IROLinearSwitch:
                    if (ep->u.swtch.cond == from) {
                        ep->u.swtch.cond = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    break;
                case IROLinearOp2Arg:
                    if (ep->u.diadic.left == from) {
                        ep->u.diadic.left = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    if (ep->u.diadic.right == from) {
                        ep->u.diadic.right = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    break;
                case IROLinearFunccall:
                    if (ep->u.funccall.callee == from) {
                        ep->u.funccall.callee = to;
                        to->flags |= IROLF_Reffed;
                        return;
                    }
                    {
                        int argIndex;
                        for (argIndex = 0; argIndex < ep->u.funccall.argCount; argIndex++) {
                            if (ep->u.funccall.args[argIndex] == from) {
                                ep->u.funccall.args[argIndex] = to;
                                to->flags |= IROLF_Reffed;
                                return;
                            }
                        }
                    }
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
                case IROLinearEnd:
                    break;
                default:
                    CError_FATAL(959);
                    break;
            }
            ep = ep->next;
        }
        ep = linear_head;
    }
    IroDump_Print("Oh, oh, did not find reference to replace\n");
}

/* Makes EXPRESSION's temporary. */
void IroCSE_CreateTemp(IROExpr *mapping)
{
    IROLinear *original;
    Type *type;
    Object *replacement;
    Object *mappedObject;
    original = mapping->linear;
    type = original->rtype;
    replacement = create_temp_object(type);
    mapping->temp = replacement;
    mappedObject = mapping->temp;
    fn_0044ba70(mappedObject, 1U, 1U);
}

/* The assignment of PAIR's temporary to PAIR's expression, inserted before the expression. */
IROLinear *IroCSE_CreateTempAssignment(IROExpr *pair)
{
    IROLinear *value;
    IROLinear *expression;
    IROLinear *assignment;

    value = IrOptimizer_NewLinear(IROLinearOperand);
    value->u.node = create_objectrefnode(pair->temp);
    value->rtype = value->u.node->data.objref->type;
    ++linear_index_counter;
    value->index = linear_index_counter;
    value->flags |= 0x26U;

    expression = IrOptimizer_NewLinear(IROLinearOp1Arg);
    expression->nodetype = 4U;
    expression->rtype = pair->linear->rtype;
    expression->u.monadic = value;
    ++linear_index_counter;
    expression->index = linear_index_counter;
    expression->flags |= 6U;

    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    assignment->nodetype = 0x1eU;
    assignment->u.diadic.left = expression;
    assignment->u.diadic.right = pair->linear;
    assignment->rtype = pair->linear->rtype;
    ++linear_index_counter;
    assignment->index = linear_index_counter;
    value->next = expression;
    expression->next = assignment;
    IroUtil_InsertLinearRangeAfter(value, assignment, pair->linear);
    return assignment;
}

/* Evaluates REPLACEMENT's expression into a new temporary where it stands. */
void create_replacement_temp_assignment(IROExpr *replacement)
{
    IROLinear *reference;
    IROLinear *conversion;
    IROLinear *assignment;

    replacement->temp = create_temp_object(replacement->linear->rtype);
    fn_0044ba70(replacement->temp, 1, 1);
    reference = IrOptimizer_NewLinear(IROLinearOperand);
    reference->u.node = create_objectrefnode(replacement->temp);
    reference->rtype = reference->u.node->data.objref->type;
    reference->index = ++linear_index_counter;
    reference->flags |= (IROLF_Reffed | IROLF_Assigned | IROLF_Ind);
    conversion = IrOptimizer_NewLinear(IROLinearOp1Arg);
    conversion->nodetype = EINDIRECT;
    conversion->rtype = replacement->linear->rtype;
    conversion->u.monadic = reference;
    conversion->index = ++linear_index_counter;
    conversion->flags |= (IROLF_Reffed | IROLF_Assigned);
    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    assignment->nodetype = EASS;
    assignment->u.diadic.left = conversion;
    assignment->u.diadic.right = replacement->linear;
    assignment->rtype = replacement->linear->rtype;
    assignment->index = ++linear_index_counter;
    reference->next = conversion;
    conversion->next = assignment;
    IroCSE_0044e560(replacement->linear, assignment);
    IroUtil_InsertLinearRangeAfter(reference, assignment, replacement->linear);
}

IROLinear *fn_0044e360(IROLinear *type, unsigned int clearBit)
{
    if (clearBit != 0U) {
        type->flags &= ~8U;
    }
    return type;
}

/* (IroUtil_VisitLinearTree visitors) */
IROLinear *fn_0044e340(IROLinear *node, unsigned int clearBit)
{
    if (clearBit != 0U) {
        node->flags &= ~IROLF_LoopInvariant;
    }
    return node;
}

/* Makes GROUP's temporary: assigned where the expressions repeating it are evaluated in common (an enclosing
   expression they all share), or at the expression itself (create_replacement_temp_assignment). */
void move_common_sub(IROExpr *group)
{
    IROExpr *child;
    int nodeCount;
    int childNodeCount;
    int unmatchedNodes;
    int unmatchedChildNodes;
    IROLinear *node;
    int splitIndex;
    struct IROLinear *splitNode;
    IROLinear *nodes[64];
    IROLinear *childNodes[64];
    nodeCount = 0;
    node = group->linear;
    do {
        node = IroUtil_FindNextUse(node);
        if (node != NULL) {
            if (nodeCount == 64)
                return;
            nodes[nodeCount] = node;
            nodeCount++;
        }
    } while (node != NULL);
    splitIndex = -1;
    child = expr_list;
    while (child != NULL) {
        if (child->use == group) {
            childNodeCount = 0;
            node = child->linear;
            do {
                node = IroUtil_FindNextUse(node);
                if (node != NULL) {
                    if (childNodeCount == 64)
                        return;
                    childNodes[childNodeCount] = node;
                    childNodeCount++;
                }
            } while (node != NULL);
            unmatchedNodes = nodeCount;
            unmatchedChildNodes = childNodeCount;
            while (unmatchedNodes != 0 && unmatchedChildNodes != 0 &&
                   nodes[unmatchedNodes - 1] == childNodes[unmatchedChildNodes - 1]) {
                unmatchedNodes--;
                unmatchedChildNodes--;
            }
            if (unmatchedNodes != nodeCount && unmatchedNodes > splitIndex)
                splitIndex = unmatchedNodes;
        }
        child = child->next;
    }
    if (splitIndex < 0) {
        create_replacement_temp_assignment(group);
    } else {
        IroDump_Print("Moving common sub from node %d to %d\n", group->linear->index, nodes[splitIndex]->index);
        splitNode = IroUtil_GetLinearRangeStart(nodes[splitIndex]);
        group->temp = create_temp_object(group->linear->rtype);
        fn_0044ba70(group->temp, 1, 1);
        IroCSE_ReplaceReference(group->linear, group->temp, group->linear);
        IroUtil_MoveExprBefore(group, splitNode);
        node = IrOptimizer_NewLinear(IROLinearOp2Arg);
        node->nodetype = ECOMMA;
        node->rtype = nodes[splitIndex]->rtype;
        node->u.diadic.left = IroCSE_CreateTempAssignment(group);
        node->u.diadic.right = nodes[splitIndex];
        node->stmt = nodes[splitIndex]->stmt;
        IroCSE_0044e560(nodes[splitIndex], node);
        IroUtil_InsertLinearRangeAfter(node, node, nodes[splitIndex]);
    }
}

void IRO_CommonSubs(void)
{
    IROExpr *def;
    IRONode *proc;
    IROExpr *prev;
    SInt32 count;
    IROLinear *stmt;
    IROExpr *cse;

    count = 0;
    for (proc = iro_flowgraph_head; proc != NULL; proc = proc->nextnode) {
        availableExpressions = proc->in;
        stmt = proc->first;
        while (1) {
            if (stmt == NULL)
                break;
            if (stmt->expr != NULL && stmt->expr->hasSideEffects == 0) {
                for (cse = expr_list; cse != NULL; cse = cse->next) {
                    if (cse->linear != stmt && cse->use == NULL && (cse->index >> 5) < availableExpressions->size &&
                        (availableExpressions->bits[cse->index >> 5] & (1 << cse->index)) != 0 &&
                        IroUtil_LinearConstantTreesSame(stmt, cse->linear) != 0) {
                        IroUtil_VisitLinearTree(stmt, (void (*)(IROLinear *, int))fn_0044e360);
                        stmt->flags |= 8;
                        stmt->expr->use = cse;
                        break;
                    }
                }
            }
            if (stmt->expr != NULL)
                IRO_BitVectorSet(stmt->expr->index, availableExpressions);
            mark_dependent_exprs(stmt);
            IroBitVect_Subtract(killed_exprs, availableExpressions);
            if (stmt == proc->last)
                break;
            if (count > 0xfa) {
                IroVars_CheckTimedLongjmp();
                count = 0;
            } else {
                count++;
            }
            stmt = stmt->next;
        }
    }

    for (proc = iro_flowgraph_head; proc != NULL; proc = proc->nextnode) {
        for (stmt = proc->first; stmt != NULL; stmt = stmt->next) {
            if (stmt->expr != NULL && (stmt->flags & 8) != 0) {
                cse = stmt->expr->use;
                if (cse->temp != NULL || (move_common_sub(cse), cse->temp != NULL)) {
                    IroDump_Print("Replacing common sub at %d with %d\n", stmt->index, cse->linear->index);
                    IroCSE_ReplaceReference(stmt, cse->temp, stmt);
                    def = stmt->expr;
                    prev = NULL;
                    cse = expr_list;
                    while (cse != def) {
                        prev = cse;
                        cse = cse->next;
                        if (cse == NULL)
                            CError_FATAL(470);
                    }
                    def->linear->expr = NULL;
                    if (prev != NULL)
                        prev->next = def->next;
                    else
                        expr_list = def->next;
                    IroUtil_ClearZeroOperands(stmt);
                }
            }
            if (stmt == proc->last)
                break;
            if (count > 0xfa) {
                IroVars_CheckTimedLongjmp();
                count = 0;
            } else {
                count++;
            }
        }
    }
    IroVars_CheckTimedLongjmp();
}
