#define CERROR_FILE "IrOptimizer.c"
#include "compiler/common.h"
#include "compiler/IrOptimizer.h"
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
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroEval.h"
#include "compiler/IroFlowgraph.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroTransform.h"
#include "compiler/IroUtil.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "compiler/Objects.h"

/* The weight of a use at each loop depth. */
static UInt16 ir_size_table[4] = {1, 4, 16, 64};
static char lbl_0054ea30[12] = {0};

static Statement *current_statement;
static UInt8 data_0057f6b4;
static UInt8 data_0057f6b5;
static char lbl_0057f6b6[10];

void fn_0042c920(void)
{
    return;
}

void IrOptimizer_SetDeleteDeadInstructionsFlags(void)
{
    copts.deadcode = copts.deleteDeadInstructions >= 1;
    copts.commonsubs = copts.propagation = copts.deleteDeadInstructions >= 2;
    copts.loopinvariants = copts.strengthreduction = copts.lifetimes = copts.deadstore = copts.unrolling =
        copts.vectorizeloops = copts.deleteDeadInstructions >= 3;
    copts.irSecondOptimizationPass = copts.deleteDeadInstructions >= 4;
}

static inline void IRO_MarkExpressionOperands(IROLinear *expression)
{
    expression->u.diadic.left->flags |= IROLF_Reffed;
    expression->u.diadic.right->flags |= IROLF_Reffed;
}

void IRO_ExpressionPropagation(void)
{
    IROLinear *e;
    IROLinear *n;
    int i;
    IRONode *node;

    for (e = linear_head; e != NULL; e = e->next)
        e->flags &= ~IROLF_Reffed;

    for (e = linear_head; e != NULL; e = e->next) {
        switch (e->type) {
            case IROLinearOp1Arg:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
                e->u.monadic->flags |= IROLF_Reffed;
                break;
            case IROLinearOp2Arg:
                IRO_MarkExpressionOperands(e);
                break;
            case IROLinearIf:
            case IROLinearIfNot:
                e->u.branch.cond->flags |= IROLF_Reffed;
                break;
            case IROLinearReturn:
                if (e->u.monadic != NULL)
                    e->u.monadic->flags |= IROLF_Reffed;
                break;
            case IROLinearSwitch:
                e->u.swtch.cond->flags |= IROLF_Reffed;
                break;
            case IROLinearFunccall:
                e->u.funccall.callee->flags |= IROLF_Reffed;
                for (i = 0; i < e->u.funccall.argCount; i++)
                    e->u.funccall.args[i]->flags |= IROLF_Reffed;
                break;
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearEntry:
            case IROLinearExit:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            default:
                CError_FATAL(395);
        }
    }

    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        for (n = node->first; n != NULL; n = n->next) {
            if ((n->flags & IROLF_Reffed) == 0) {
                switch (n->type) {
                    case IROLinearOperand:
                    case IROLinearOp1Arg:
                    case IROLinearOp2Arg:
                    case IROLinearFunccall:
                        trav_expr_to_update_flags(n, 0);
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot:
                        trav_expr_to_update_flags(n->u.branch.cond, 1);
                        break;
                    case IROLinearReturn:
                        if (n->u.monadic != NULL)
                            trav_expr_to_update_flags(n->u.monadic, 1);
                        break;
                    case IROLinearSwitch:
                        trav_expr_to_update_flags(n->u.swtch.cond, 1);
                        break;
                    case IROLinearBeginCatch:
                        trav_expr_to_update_flags(n->u.monadic, 1);
                        break;
                    case IROLinearEndCatch:
                        trav_expr_to_update_flags(n->u.monadic, 1);
                        break;
                    case IROLinearEndCatchDtor:
                        trav_expr_to_update_flags(n->u.monadic, 1);
                        break;
                    case IROLinearNop:
                    case IROLinearGoto:
                    case IROLinearLabel:
                    case IROLinearEntry:
                    case IROLinearExit:
                    case IROLinearAsm:
                    case IROLinearEnd:
                        break;
                    default:
                        CError_FATAL(5154);
                }
            }
            if (n == node->last)
                break;
        }
    }
}

void trav_expr_to_update_flags(IROLinear *expression, unsigned int flag)
{
    IROLinear *operand;
    IROAddrRecord *auxiliary;
    SInt32 index;

    expression->flags &= 0xffffc683;
    switch (expression->type) {
        case IROLinearNop:
        case IROLinearOperand:
            break;
        case IROLinearOp1Arg:
            trav_expr_to_update_flags(expression->u.diadic.left, 1);
            if (data_00551d6c[expression->nodetype] != 0)
                set_monadic_addr_flags(expression->u.diadic.left, 1);
            if (expression->nodetype == EINDIRECT) {
                operand = expression->u.diadic.left;
                if (operand->type == IROLinearOp2Arg && operand->nodetype == EADD) {
                    if (operand->u.diadic.left->type == EPOSTDEC && operand->u.diadic.left->u.node->type == EOBJREF &&
                        operand->u.diadic.right->type == EPOSTDEC && operand->u.diadic.right->u.node->type == EINTCONST)
                        operand->u.diadic.left->flags |= IROLF_Ind;
                    auxiliary = IroVars_CreateAddrRecord(operand);
                    IroVars_CollectAddrRecordElements(operand, auxiliary);
                    if (auxiliary->numObjRefs == 1 && operand->u.diadic.left->type == IROLinearOperand &&
                        operand->u.diadic.left->u.node->type == EOBJREF)
                        auxiliary->objRefs->element->flags |= IROLF_Ind;
                }
                expression->u.diadic.left->flags |= (IROLF_Ind | IROLF_Immind);
                if (expression->u.diadic.left->type == IROLinearOp2Arg && expression->u.diadic.left->nodetype == EADD)
                    fn_004315b0(expression->u.diadic.left);
            }
            break;
        case IROLinearOp2Arg:
            trav_expr_to_update_flags(expression->u.diadic.left, 1);
            trav_expr_to_update_flags(expression->u.diadic.right, 1);
            if (data_00551d6c[expression->nodetype] != 0)
                set_monadic_addr_flags(expression->u.diadic.left, monadic_addr_flags_by_nodetype[expression->nodetype]);
            break;
        case IROLinearFunccall:
            trav_expr_to_update_flags(expression->u.funccall.callee, 1);
            for (index = expression->u.funccall.argCount - 1; 0 <= index; index--)
                trav_expr_to_update_flags(expression->u.funccall.args[index], 1);
            break;
        default:
            IroDump_Print("Oh, oh, bad expression type in TravExprToUpdateFlags at: %d\n", expression->index);
            CError_FATAL(5073);
            break;
    }
}

static inline UInt8 IRO_CopyPropagationSetting(void)
{
    return copts.propagation;
}

void *IRO_Optimizer(Object *function, void *incomingBody)
{
    SInt32 changed;
    UInt8 pass;
    UInt8 passes;
    IROLinear *node;
    UInt8 eliminateUnused;
    Statement *body = incomingBody;

    data_0057f6b4 = 0;
    data_005875b8 = function;
    data_00588244 = 1;
    data_005876e4 = 1;
    data_005871b0 = 0;
    data_00552b88 = NULL;
    data_0057f6b5 = 0;
    data_00588518 = 1;
    data_00588234 = 0;
    data_00588526 = 0;

    IroDump_Print("Starting function %s\n", function ? function->name->name : "Init-code");
    IroDump_Print("--------------------------------------------------------------------------------\n");

    if (data_005876e4 != 0)
        visit_statement_expressions(body);
    if (copts.deleteDeadInstructions > 0)
        IroCSE_RewriteStatementExpressions(body);

    data_00587e50 = 0;
    data_0058800c = 0;
    build_linear_from_statements(body);
    current_statement = NULL;
    iro_flowgraph_head = NULL;

    if (copts.deleteDeadInstructions > 0)
        IroTransform_SimplifyLinear();

    IRO_BuildflowGraph(linear_head);
    IroDump_DumpFunction("IRO_BuildflowGraph", 0);
    IroVars_BuildVarRecords();
    IroVars_CheckVariablesInitializedBeforeUse();

    if (copts.deleteDeadInstructions > 0) {
        changed = IRO_EvaluateConditionals();
        if (data_0057f6b4 == 0) {
            changed |= IRO_RemoveUnreachable();
            IroDump_DumpFunction("IRO_RemoveUnreachable", 0);
        }
        changed |= IRO_RemoveRedundantJumps();
        IroDump_DumpFunction("IRO_RemoveRedundantJumps", 0);
        if (data_0057f6b4 == 0) {
            changed |= IRO_RemoveLabels();
            IroDump_DumpFunction("IRO_RemoveLabels()", 0);
        }
        if (changed != 0) {
            IRO_BuildflowGraph(linear_head);
            IroDump_DumpFunction("IRO_BuildflowGraph--1", 0);
        }
    }

    if (data_0057f6b4 == 0 && copts.deleteDeadInstructions > 0) {
        if (copts.irSecondOptimizationPass != 0)
            passes = 2;
        else
            passes = 1;
        data_00587148 = 1;
        for (pass = 0; pass < passes; pass++) {
            IroDump_Print("*****************\n");
            IroDump_Print("Dumps for pass=%d\n", pass);
            IroDump_Print("*****************\n");
            if (data_00588244 != 0)
                IRO_ScalarizeClassDataMembers();
            IroDump_DumpFunction("IRO_ScalarizeClassDataMembers", 0);
            if (IRO_CopyPropagationSetting() != 0) {
                IRO_CopyAndConstantPropagation();
                data_00587148 = 0;
                IroPropagate_PropagateExpressions();
                IroDump_DumpFunction("Copy and constant propagation", 0);
                IRO_RangePropagateInFNode();
                IroDump_DumpFunction("IRO_RangePropagateInFNode", 0);
                IRO_ExpressionPropagation();
            }
            IroDump_DumpFunction("IRO_ExpressionPropagation", 0);
            if ((eliminateUnused = copts.deadstore) != 0 || IRO_CopyPropagationSetting() != 0)
                IRO_UseDef(eliminateUnused, IRO_CopyPropagationSetting());
            IroDump_DumpFunction("after IRO_UseDef", 0);
            IroVars_BuildNoregisterBitVector();
            IRO_ConstantFolding();
            IroDump_DumpFunction("IRO_ConstantFolding", 0);
            IRO_EvaluateConditionals();
            IRO_RemoveUnreachable();
            fn_00454bb0();
            if (copts.unrolling != 0 && copts.optimizesize == 0 && pass == 0) {
                IroDump_DumpFunction("Before IRO_LoopUnroller", 0);
                IRO_LoopUnroller();
                IroDump_DumpFunction("After IRO_LoopUnroller", 0);
                node = linear_head;
                linear_index_counter = 0;
                if (linear_head != NULL) {
                    do {
                        node->index = linear_index_counter;
                        linear_index_counter++;
                    } while ((node = node->next) != NULL);
                }
            }
            data_0058800c = 0;
            if (pass == 0 && (copts.loopinvariants != 0 || copts.strengthreduction != 0)) {
                IroDump_DumpFunction("Before IRO_FindLoops", 0);
                IRO_FindLoops();
                data_0057f6b5 = 1;
                IroLoop_ComputeLoopDepth();
            }
            IroDump_DumpFunction("After IRO_FindLoops", 0);
            if (IRO_CopyPropagationSetting() != 0) {
                IRO_CopyAndConstantPropagation();
                IRO_ConstantFolding();
                IRO_EvaluateConditionals();
            }
            IroDump_DumpFunction(
                "Second pass:IRO_CopyAndConstantPropagation,IRO_ConstantFolding,IRO_EvaluateConditionals", 0);
            if (copts.commonsubs != 0)
                IroCSE_ClearExpr();
            if (copts.commonsubs != 0) {
                IroCSE_ComputeAvailableExpressions();
                IRO_CommonSubs();
            }
            IroDump_DumpFunction("IRO_CommonSubs", 0);
            IRO_ExpressionPropagation();
            IroVars_BuildNoregisterBitVector();
            IroTransform_SimplifyLinear();
            IRO_ConstantFolding();
            do {
                IRO_ExpressionPropagation();
                changed = 0;
                if (copts.deadcode != 0)
                    IRO_RemoveUnreachable();
                IroDump_DumpFunction("IRO_RemoveUnreachable", 0);
                changed |= IRO_RemoveRedundantJumps();
                IroDump_DumpFunction("IRO_RemoveRedundantJumps", 0);
                changed |= IRO_RemoveLabels();
                IroDump_DumpFunction("IRO_RemoveLabels", 0);
                changed |= IRO_DoJumpChaining();
                IroDump_DumpFunction("IRO_DoJumpChaining", 0);
                if (IRO_CopyPropagationSetting() != 0) {
                    node = linear_head;
                    linear_index_counter = 0;
                    if (linear_head != NULL) {
                        do {
                            node->index = linear_index_counter;
                            linear_index_counter++;
                        } while ((node = node->next) != NULL);
                    }
                    IroDump_DumpFunction("Before IRO_CopyAndConstantPropagation", 0);
                    IRO_CopyAndConstantPropagation();
                    IroDump_DumpFunction("After IRO_CopyAndConstantPropagation", 0);
                    IRO_ConstantFolding();
                    changed |= data_005880c8;
                }
                if ((eliminateUnused = copts.deadstore) != 0 || IRO_CopyPropagationSetting() != 0)
                    changed |= IRO_UseDef(eliminateUnused, IRO_CopyPropagationSetting());
                IroDump_DumpFunction("IRO_UseDef", 0);
                changed |= IRO_EvaluateConditionals();
                IroDump_DumpFunction("IRO_EvaluateConditionals", 0);
            } while (changed != 0);
        }

        if (copts.lifetimes != 0) {
            IRO_UseDef(0, 0);
            fn_00459420();
        }
        IroTransform_SimplifyLinear();
        IroDump_DumpFunction("Before RebuildCondExpressions", 0);
    }

    node = linear_head;
    linear_index_counter = 0;
    if (linear_head != NULL) {
        do {
            node->index = linear_index_counter;
            linear_index_counter++;
        } while ((node = node->next) != NULL);
    }
    IroDump_DumpFunction("before RewriteBitFieldTemps", 0);
    RewriteBitFieldTemps();
    IroDump_DumpFunction("After RewriteBitFieldTemps", 0);
    record_object_usage();
    IroDump_DumpFunction("After IRO_Optimizer", 0);
    incomingBody = convert_linear_to_statements();
    IroVars_ClearObjectVarRecords();
    CompilerTools_ResetPool();
    return incomingBody;
}

static char lbl_0054eedc[] = "BitVector.h";

Boolean IrOptimizer_0042d2c0(IROLinear *e)
{
    if (e == NULL)
        return 0;
    switch (e->type) {
        case IROLinearOperand:
        case IROLinearOp1Arg:
        case IROLinearOp2Arg:
        case IROLinearFunccall:
        case IROLinearAsm:
            return fn_0044be00(e) != NULL;
        case IROLinearEndCatch:
        case IROLinearEndCatchDtor:
            return IrOptimizer_0042d2c0(e->u.monadic);
        case IROLinearIf:
        case IROLinearIfNot:
            return IrOptimizer_0042d2c0(e->u.args3.b) || IrOptimizer_0042d2c0(e->u.args3.c);
        case IROLinearReturn:
            return IrOptimizer_0042d2c0(e->u.monadic);
        case IROLinearSwitch:
            return IrOptimizer_0042d2c0(e->u.args3.b);
        case IROLinearBeginCatch:
            return IrOptimizer_0042d2c0(e->u.monadic) || IrOptimizer_0042d2c0(e->u.args3.b) ||
                   IrOptimizer_0042d2c0(e->u.args3.c);
        default:
            return 0;
    }
}

Boolean contains_linear_index(IROLinear *a, IROLinear *b)
{
    int result;
    int flag;
    SInt16 i;

    if (a == NULL)
        return 0;
    if (a->index == b->index)
        return 1;
    switch (a->type) {
        case IROLinearOp1Arg:
        case IROLinearEndCatch:
        case IROLinearEndCatchDtor:
            return contains_linear_index(a->u.monadic, b);
        case IROLinearOp2Arg:
            result = 1;
            if (!contains_linear_index(a->u.monadic, b) && !contains_linear_index(a->u.args3.b, b))
                result = 0;
            return result;
        case IROLinearIf:
        case IROLinearIfNot:
            result = 1;
            if (!contains_linear_index(a->u.args3.b, b) && !contains_linear_index(a->u.args3.c, b))
                result = 0;
            return result;
        case IROLinearReturn:
            return contains_linear_index(a->u.monadic, b);
        case IROLinearSwitch:
            return contains_linear_index(a->u.args3.b, b);
        case IROLinearFunccall:
            if (contains_linear_index(a->u.args3.c, b))
                return 1;
            for (i = 0; i < a->u.funccall.argCount; i++) {
                if (contains_linear_index(a->u.funccall.args[i], b))
                    return 1;
            }
            return 0;
        case IROLinearBeginCatch:
            result = 1;
            flag = 1;
            if (!contains_linear_index(a->u.monadic, b) && !contains_linear_index(a->u.args3.b, b))
                flag = 0;
            if (!flag && !contains_linear_index(a->u.args3.c, b))
                result = 0;
            return result;
        default:
            return 0;
    }
}

static __inline int IR_Replace(IROLinear **fieldp, IROUse *ctx, ENode *obj, IROLinear *newval, Boolean doReplace)
{
    ENode *r;
    IROLinear *t;
    IROLinear *f8;
    IROLinear *a;
    f8 = ctx->linear;
    if ((a = *fieldp) != NULL && a->type == IROLinearOp1Arg && a->nodetype == EINDIRECT && (t = a->u.monadic) != NULL &&
        (f8 == NULL || f8 == t) && t->type == EPOSTDEC && (r = t->u.node)->type == EOBJREF &&
        r->data.objref == obj->data.objref) {
        if (doReplace) {
            IroUtil_ClearZeroOperands(a);
            *fieldp = newval;
        }
        return 1;
    }
    return 0;
}

static __inline int IR_ReplaceArray(IROLinear **fieldp, IROUse *ctx, ENode *obj, IROLinear *newval, Boolean doReplace)
{
    ENode *r;
    IROLinear *t;
    IROLinear *f8;
    f8 = ctx->linear;
    if (*fieldp != NULL && (*fieldp)->type == IROLinearOp1Arg && (*fieldp)->nodetype == EINDIRECT &&
        (t = (*fieldp)->u.monadic) != NULL && (f8 == NULL || f8 == t) && t->type == EPOSTDEC &&
        (r = t->u.node)->type == EOBJREF && r->data.objref == obj->data.objref) {
        if (doReplace) {
            IroUtil_ClearZeroOperands(*fieldp);
            *fieldp = newval;
        }
        return 1;
    }
    return 0;
}

static __inline int IR_ReplaceCall(IROLinear **fieldp, IROUse *ctx, ENode *obj, IROLinear *newval, Boolean doReplace)
{
    ENode *r;
    IROLinear *t;
    IROLinear *f8;
    IROLinear *a;
    a = *fieldp;
    f8 = ctx->linear;
    (void)a;
    if (a != NULL && a->type == IROLinearOp1Arg && a->nodetype == EINDIRECT && (t = a->u.monadic) != NULL &&
        (f8 == NULL || f8 == t) && t->type == EPOSTDEC && (r = t->u.node)->type == EOBJREF &&
        r->data.objref == obj->data.objref) {
        if (doReplace) {
            IroUtil_ClearZeroOperands(a);
            *fieldp = newval;
        }
        return 1;
    }
    return 0;
}

int replace_pending_reference(IROUse *ctx, ENode *obj, IROLinear *newval, Boolean doReplace)
{
    int count;
    IRONode *p;
    IROLinear *q;
    SInt16 i;
    ENode *n;
    if (ctx != NULL && ctx->reachingDefCount != 0 && ctx->linear != NULL && ctx->linear->type == IROLinearOperand &&
        (n = ctx->linear->u.node)->type == EOBJREF && n->data.objref == obj->data.objref) {
        count = 0;
        for (p = ctx->node; p != NULL && count == 0; p = p->nextnode) {
            for (q = p->first; q != NULL && count == 0 && p->last != NULL && q != p->last->next; q = q->next) {
                switch (q->type) {
                    case IROLinearOp1Arg:
                        if (monadic_addr_flags_by_nodetype[q->nodetype] == 0)
                            count += IR_Replace(&q->u.monadic, ctx, obj, newval, doReplace);
                        break;
                    case IROLinearEndCatch:
                    case IROLinearEndCatchDtor:
                        count += IR_Replace(&q->u.monadic, ctx, obj, newval, doReplace);
                        break;
                    case IROLinearOp2Arg:
                        if (monadic_addr_flags_by_nodetype[q->nodetype] == 0)
                            count += IR_Replace(&q->u.monadic, ctx, obj, newval, doReplace);
                        count += IR_Replace(&q->u.diadic.right, ctx, obj, newval, doReplace);
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot:
                        count += IR_Replace(&q->u.diadic.right, ctx, obj, newval, doReplace);
                        count += IR_Replace(&q->u.args3.c, ctx, obj, newval, doReplace);
                        break;
                    case IROLinearReturn:
                        count += IR_Replace(&q->u.monadic, ctx, obj, newval, doReplace);
                        break;
                    case IROLinearSwitch:
                        count += IR_Replace(&q->u.diadic.right, ctx, obj, newval, doReplace);
                        break;
                    case IROLinearFunccall:
                        count += IR_ReplaceCall(&q->u.funccall.callee, ctx, obj, newval, doReplace);
                        for (i = 0; i < q->u.funccall.argCount; i++)
                            count += IR_ReplaceArray(&q->u.funccall.args[i], ctx, obj, newval, doReplace);
                        break;
                    case IROLinearBeginCatch:
                        count += IR_Replace(&q->u.monadic, ctx, obj, newval, doReplace);
                        count += IR_Replace(&q->u.diadic.right, ctx, obj, newval, doReplace);
                        count += IR_Replace(&q->u.args3.c, ctx, obj, newval, doReplace);
                        break;
                }
            }
        }
        if (count != 0) {
            if (doReplace)
                ctx->reachingDefCount = 0;
            return count;
        }
    }
    return 0;
}

static SInt32 IrSize(SInt32 cls)
{
    SInt32 n = cls;
    if (cls > 3)
        n = 3;
    return ir_size_table[n];
}

static inline void RecordObject(Object *object, SInt32 depth, IROLinear *node)
{
    if (object->datatype == DALIAS)
        CError_FATAL(2640);
    if (object->datatype == DLOCAL && object->u.var.info != NULL) {
        object->u.var.info->usage += IrSize(depth);
        object->u.var.info->used = 1;
        if ((node->flags & IROLF_Used) != 0 && (node->flags & IROLF_Assigned) != 0) {
            object->u.var.info->usage += IrSize(depth);
            object->u.var.info->used = 1;
        }
        if ((node->flags & IROLF_Immind) == 0)
            object->u.var.info->noregister = 1;
    }
}

void record_object_usage(void)
{
    IRONode *obj;
    IROLinear *node;
    Object *o;
    IROLinear *obj2;
    SInt32 i;
    AsmOut list;
    Object *o_s;

    obj = iro_flowgraph_head;
    if (obj != NULL) {
        for (; obj != NULL; obj = obj->nextnode) {
            for (node = obj->first; node != NULL; node = node->next) {
                if (node->type == IROLinearOperand && node->u.node->type == EOBJREF) {
                    o = node->u.node->data.objref;
                    RecordObject(o, obj->loopdepth, node);
                } else if (node->type == IROLinearOp1Arg && node->nodetype == EINDIRECT) {
                    IrOptimizer_0042e200(node->u.monadic, obj->loopdepth);
                }
                if (node->type == IROLinearAsm) {
                    InlineAsmPPC_00462d70(node->u.asm_stmt, &list);
                    for (i = 0; i < list.numoperands; i++) {
                        o_s = list.operands[i].object;
                        if (o_s->datatype == DLOCAL && o_s->u.var.info != NULL) {
                            o_s->u.var.info->usage += IrSize(obj->loopdepth);
                            o_s->u.var.info->used = 1;
                            if (list.operands[i].type == 3)
                                o_s->u.var.info->noregister = 1;
                        }
                    }
                }
                if (node == obj->last)
                    break;
            }
        }
    } else {
        for (obj2 = linear_head; obj2 != NULL; obj2 = obj2->next) {
            if (obj2->type == IROLinearOperand && obj2->u.node->type == EOBJREF) {
                o = obj2->u.node->data.objref;
                RecordObject(o, 0, obj2);
            } else if (obj2->type == IROLinearOp1Arg && obj2->nodetype == EINDIRECT) {
                IrOptimizer_0042e200(obj2->u.monadic, 0);
            }
        }
    }
    IroVars_CheckTimedLongjmp();
}

static inline void IrOptimizer_RecordUsage(Object *obj, int level)
{
    int index = level;
    if (index > 3)
        index = 3;
    obj->u.var.info->usage += ir_size_table[index];
    obj->u.var.info->used = 1;
}

void IrOptimizer_0042e200(IROLinear *node, int level)
{
    int index;
    Object *obj;
    IROLinear *operand;
    IROLinear *child;

    if (IroDump_GetObjRef(node)) {
        operand = node->u.monadic;
        obj = operand->u.node->data.objref;
        CError_ASSERT(2640, obj->datatype != DALIAS);
        if (obj->datatype == DLOCAL && obj->u.var.info != NULL) {
            IrOptimizer_RecordUsage(obj, level);
            if ((operand->flags & IROLF_Used) && (operand->flags & IROLF_Assigned)) {
                IrOptimizer_RecordUsage(obj, level);
            }
            if (!(operand->flags & IROLF_Immind))
                obj->u.var.info->noregister = 1;
        }
    } else if (node->type == IROLinearOp2Arg && node->nodetype == EADD) {
        child = node->u.diadic.left;
        if (IroDump_GetObjRef(child)) {
            add_local_usage_and_set_noregister(child->u.monadic, level);
        } else if (child->type == IROLinearOp2Arg) {
            if (child->nodetype == EADD) {
                IrOptimizer_0042e200(child->u.diadic.left, level);
                IrOptimizer_0042e200(child->u.diadic.right, level);
            } else if (return_zero_for_irolinear(child) && IroDump_GetObjRef(child->u.diadic.left)) {
                add_local_usage_and_set_noregister(child->u.diadic.left->u.monadic, level);
            }
        }

        child = node->u.diadic.right;
        if (IroDump_GetObjRef(child)) {
            add_local_usage_and_set_noregister(child->u.monadic, level);
        } else if (child->type == IROLinearOp2Arg) {
            if (child->nodetype == EADD) {
                IrOptimizer_0042e200(child->u.diadic.left, level);
                IrOptimizer_0042e200(child->u.diadic.right, level);
            } else if (return_zero_for_irolinear(child) && IroDump_GetObjRef(child->u.diadic.left)) {
                add_local_usage_and_set_noregister(child->u.diadic.left->u.monadic, level);
            }
        }
    }
}

static Statement *NewIrNode(UInt8 type, IROLinear *src)
{
    Statement *n;

    n = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
    memset(n, 0, sizeof(Statement));
    n->type = type;
    n->value = 1;
    if (src->stmt != NULL) {
        n->dobjstack = src->stmt->dobjstack;
        n->sourceoffset = src->stmt->sourceoffset;
        n->value = src->stmt->value;
        n->flags = src->stmt->flags;
    } else {
        n->sourceoffset = -1;
    }
    return n;
}

static inline void ClearReferences(void)
{
    IROLinear *item;
    for (item = linear_head; item != NULL; item = item->next) {
        item->flags &= ~IROLF_Reffed;
    }
    for (item = linear_head; item != NULL; item = item->next) {
        switch (item->type) {
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearEntry:
            case IROLinearExit:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            case IROLinearOp1Arg:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
                item->u.monadic->flags |= IROLF_Reffed;
                break;
            case IROLinearOp2Arg:
                item->u.diadic.left->flags |= IROLF_Reffed;
                item->u.diadic.right->flags |= IROLF_Reffed;
                break;
            case IROLinearIf:
            case IROLinearIfNot:
                item->u.branch.cond->flags |= IROLF_Reffed;
                break;
            case IROLinearReturn:
                if (item->u.monadic != NULL) {
                    item->u.monadic->flags |= IROLF_Reffed;
                }
                break;
            case IROLinearSwitch:
                item->u.swtch.cond->flags |= IROLF_Reffed;
                break;
            case IROLinearFunccall: {
                SInt32 j;
                item->u.funccall.callee->flags |= IROLF_Reffed;
                for (j = 0; j < item->u.funccall.argCount; j++) {
                    item->u.funccall.args[j]->flags |= IROLF_Reffed;
                }
                break;
            }
            default:
                CError_FATAL(395);
        }
    }
}

static ENode *NewNode(UInt8 type)
{
    ENode *p;
    p = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memset(p, 0, sizeof(ENode));
    p->type = type;
    return p;
}

static IROLinear *NewInsn(UInt8 type)
{
    IROLinear *p = (IROLinear *)CompilerTools_AllocatePoolMemory(sizeof(IROLinear));
    memset(p, 0, sizeof(IROLinear));
    p->stmt = current_statement;
    p->nodetype = EPOSTINC;
    p->next = NULL;
    p->type = type;
    p->rtype = NULL;
    p->flags = 0;
    p->nodeflags = 0;
    p->expr = NULL;
    p->range = NULL;
    return p;
}

static void MarkUsedInputs(IROLinear *insn)
{
    SInt32 i;
    for (i = 0; i < insn->u.funccall.argCount; i++)
        insn->u.funccall.args[i]->flags |= IROLF_Reffed;
}

static inline void AddLocalUsage(Object *object, int weightIndex)
{
    int index = weightIndex;
    if (index > 3)
        index = 3;
    object->u.var.info->usage += ir_size_table[index];
    object->u.var.info->used = 1;
}

void add_local_usage_and_set_noregister(IROLinear *node, int weightIndex)
{
    Object *object;

    object = node->u.node->data.objref;
    if (object->datatype == DALIAS)
        CError_FATAL(2640);
    if (object->datatype == DLOCAL && object->u.var.info) {
        AddLocalUsage(object, weightIndex);
        if ((node->flags & IROLF_Used) && (node->flags & IROLF_Assigned)) {
            AddLocalUsage(object, weightIndex);
        }
        if (!(node->flags & IROLF_Immind))
            object->u.var.info->noregister = 1;
    }
}

Statement *convert_linear_to_statements(void)
{
    IRONode *block;
    IrNodeList nodes;
    IROLinear *item;
    Statement *node;
    SInt32 depth;
    nodes.first = nodes.last = NULL;

    ClearReferences();

    for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
        for (item = block->first; item != NULL; item = item->next) {
            node = NULL;
            if ((item->flags & IROLF_Reffed) == 0) {
                switch (item->type) {
                    case IROLinearNop:
                        if (block == iro_flowgraph_head) {
                            node = NewIrNode(1, item);
                        } else {
                            node = NULL;
                        }
                        break;
                    case IROLinearOperand:
                    case IROLinearOp1Arg:
                    case IROLinearOp2Arg:
                    case IROLinearFunccall:
                        node = NewIrNode(4, item);
                        node->expr = IrOptimizer_0042eb40(item);
                        break;
                    case IROLinearGoto:
                        node = NewIrNode(3, item);
                        node->label = item->u.label;
                        break;
                    case IROLinearExit:
                        node = NewIrNode(0x0a, item);
                        node->label = item->u.label;
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot: {
                        UInt8 nodeType;
                        if (item->type == IROLinearIf) {
                            nodeType = 6;
                        } else {
                            nodeType = 7;
                        }
                        node = NewIrNode(nodeType, item);
                        node->label = item->u.branch.label;
                        node->expr = IrOptimizer_0042eb40(item->u.branch.cond);
                        break;
                    }
                    case IROLinearReturn:
                        node = NewIrNode(8, item);
                        if (item->u.monadic != NULL) {
                            node->expr = IrOptimizer_0042eb40(item->u.monadic);
                        }
                        break;
                    case IROLinearLabel:
                        node = NewIrNode(2, item);
                        node->label = item->u.label;
                        node->label->stmt = node;
                        break;
                    case IROLinearEntry:
                        node = NewIrNode(0x0b, item);
                        node->label = item->u.label;
                        node->label->stmt = node;
                        break;
                    case IROLinearSwitch:
                        node = NewIrNode(5, item);
                        node->expr = IrOptimizer_0042eb40(item->u.swtch.cond);
                        /* a switch statement's label slot holds its SwitchInfo */
                        node->label = (CLabel *)item->u.swtch.info;
                        break;
                    case IROLinearBeginCatch:
                        node = NewIrNode(0x0c, item);
                        node->expr = IrOptimizer_0042eb40(item->u.monadic);
                        break;
                    case IROLinearEndCatch:
                        node = NewIrNode(0x0d, item);
                        node->expr = IrOptimizer_0042eb40(item->u.monadic);
                        break;
                    case IROLinearEndCatchDtor:
                        node = NewIrNode(0x0e, item);
                        node->expr = IrOptimizer_0042eb40(item->u.monadic);
                        break;
                    case IROLinearAsm:
                        node = item->u.asm_stmt;
                        break;
                    case IROLinearEnd:
                        node = NULL;
                        break;
                    default:
                        CError_FATAL(2560);
                }
                if (node != NULL) {
                    if (data_0057f6b5 != 0) {
                        SInt32 weight = 1;
                        for (depth = 0; depth < block->loopdepth; depth++) {
                            if (weight < 0x1000) {
                                weight = weight << 3;
                            } else {
                                weight++;
                            }
                        }
                        node->value = weight;
                    }
                    if (nodes.first != NULL) {
                        nodes.last->next = node;
                    } else {
                        nodes.first = node;
                    }
                    nodes.last = node;
                }
            }
            if (item == block->last) {
                break;
            }
        }
    }
    return nodes.first;
}

ENode *IrOptimizer_0042eb40(IROLinear *e)
{
    ENode *p;
    ENodeList *l;
    SInt32 i;

    switch (e->type) {
        case IROLinearOperand:
            p = NewNode(e->u.node->type);
            p->flags = e->nodeflags;
            *p = *e->u.node;
            break;
        case IROLinearOp1Arg:
            p = NewNode(e->nodetype);
            p->flags = e->nodeflags;
            p->data.monadic = IrOptimizer_0042eb40(e->u.monadic);
            p->rtype = e->rtype;
            p->cost = p->data.monadic->cost;
            if (p->cost == 0)
                p->cost = 1;
            break;
        case IROLinearOp2Arg:
            p = NewNode(e->nodetype);
            p->flags = e->nodeflags;
            p->data.diadic.left = IrOptimizer_0042eb40(e->u.diadic.left);
            p->data.diadic.right = IrOptimizer_0042eb40(e->u.diadic.right);
            p->cost = p->data.diadic.left->cost;
            if (p->data.diadic.right->cost > p->cost)
                p->cost = p->data.diadic.right->cost;
            else if (p->data.diadic.right->cost == p->cost)
                p->cost += 1;
            if (p->type == ESHL || p->type == ESHR || p->type == EDIV || p->type == EMODULO)
                p->cost += 2;
            if (p->cost > 200)
                p->cost = 200;
            p->rtype = e->rtype;
            break;
        case IROLinearFunccall: {
            UInt8 ty;
            ENodeList *l;
            SInt32 i;
            if (e->u.funccall.ispure)
                ty = 0x37;
            else
                ty = 0x36;
            p = NewNode(ty);
            p->flags = e->nodeflags;
            p->data.funccall.funcref = IrOptimizer_0042eb40(e->u.funccall.callee);
            p->data.funccall.functype = e->u.funccall.functype;
            p->data.funccall.args = NULL;
            p->cost = 200;
            {
                SInt32 n = e->u.funccall.argCount - 1;
                i = n;
                if (n >= 0) {
                    do {
                        l = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                        l->node = IrOptimizer_0042eb40(e->u.funccall.args[i]);
                        l->next = p->data.funccall.args;
                        p->data.funccall.args = l;
                    } while (--i >= 0);
                }
            }
        }
            p->rtype = e->rtype;
            break;
        default:
            IroDump_Print("Oh, oh, bad expression type in BuildExpr at: %d\n", e->index);
            CError_FATAL(2274);
            break;
    }
    return p;
}

ENode *IrOptimizer_NewENode(UInt8 type)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memset(node, 0, sizeof(ENode));
    node->type = (UInt8)type;
    return node;
}

void build_linear_from_statements(Statement *node)
{
    IROLinear *insn;
    IROLinear *input;
    AsmOut usageInfo;

    linear_head = last_linear = NULL;
    linear_index_counter = 0;
    while (node != NULL) {
        current_statement = node;
        data_00587f08 = 0;
        insn = NULL;
        switch (node->type) {
            case ST_NOP:
                insn = NewInsn(IROLinearNop);
                break;
            case ST_LABEL:
                insn = NewInsn(IROLinearLabel);
                insn->u.label = node->label;
                insn->flags |= 1;
                break;
            case ST_GOTO:
                insn = NewInsn(IROLinearGoto);
                insn->u.label = node->label;
                break;
            case ST_EXPRESSION:
                linearize_expression(node->expr);
                break;
            case ST_SWITCH:
                insn = NewInsn(IROLinearSwitch);
                insn->u.swtch.cond = linearize_expression(node->expr);
                insn->u.swtch.info = (SwitchInfo *)node->label;
                break;
            case ST_IFGOTO:
                insn = NewInsn(IROLinearIf);
                insn->u.branch.cond = linearize_expression(node->expr);
                insn->u.branch.label = node->label;
                break;
            case ST_IFNGOTO:
                insn = NewInsn(IROLinearIfNot);
                insn->u.branch.cond = linearize_expression(node->expr);
                insn->u.branch.label = node->label;
                break;
            case ST_RETURN:
                data_00588526 = 1;
                insn = NewInsn(IROLinearReturn);
                if (node->expr != NULL)
                    insn->u.monadic = linearize_expression(node->expr);
                else
                    insn->u.monadic = NULL;
                break;
            case ST_OVF:
                CError_FATAL(2049);
                break;
            case ST_EXIT:
                insn = NewInsn(IROLinearExit);
                insn->u.label = node->label;
                break;
            case ST_ENTRY:
                insn = NewInsn(IROLinearEntry);
                insn->u.label = node->label;
                insn->flags |= 1;
                break;
            case ST_BEGINCATCH:
                insn = NewInsn(IROLinearBeginCatch);
                insn->u.args3.a = linearize_expression(node->expr);
                insn->u.args3.b = NULL;
                insn->u.args3.c = NULL;
                break;
            case ST_ENDCATCH:
                insn = NewInsn(IROLinearEndCatch);
                insn->u.monadic = linearize_expression(node->expr);
                break;
            case ST_ENDCATCHDTOR:
                insn = NewInsn(IROLinearEndCatchDtor);
                insn->u.monadic = linearize_expression(node->expr);
                break;
            case ST_ASM:
                insn = NewInsn(IROLinearAsm);
                insn->u.asm_stmt = node;
                if (copts.fc3 > 0) {
                    InlineAsmPPC_00462d70(node, &usageInfo);
                    if (usageInfo.optimizationBarrier != 0 || usageInfo.unmodeledControlFlow != 0)
                        data_0057f6b4 = 1;
                } else {
                    data_0057f6b4 = 1;
                }
                break;
            default:
                CError_FATAL(2100);
                break;
        }
        if (insn != NULL) {
            insn->index = linear_index_counter++;
            if (linear_head != NULL)
                last_linear->next = insn;
            else
                linear_head = insn;
            last_linear = insn;
        }
        node = node->next;
    }
    insn = NewInsn(IROLinearEnd);
    insn->flags |= 1;
    insn->index = linear_index_counter++;
    if (linear_head != NULL)
        last_linear->next = insn;
    else
        linear_head = insn;
    last_linear = insn;

    insn = linear_head;
    while (insn != NULL) {
        insn->flags &= ~IROLF_Reffed;
        insn = insn->next;
    }
    insn = linear_head;
    while (insn != NULL) {
        switch (insn->type) {
            case IROLinearOp1Arg:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
                input = insn->u.monadic;
                input->flags |= IROLF_Reffed;
                break;
            case IROLinearOp2Arg:
                insn->u.diadic.left->flags |= IROLF_Reffed, insn->u.diadic.right->flags |= IROLF_Reffed;
                break;
            case IROLinearIf:
            case IROLinearIfNot:
                input = insn->u.branch.cond;
                input->flags |= IROLF_Reffed;
                break;
            case IROLinearReturn:
                if (insn->u.monadic != NULL) {
                    input = insn->u.monadic;
                    input->flags |= IROLF_Reffed;
                }
                break;
            case IROLinearSwitch:
                input = insn->u.swtch.cond;
                input->flags |= IROLF_Reffed;
                break;
            case IROLinearFunccall:
                input = insn->u.funccall.callee;
                input->flags |= IROLF_Reffed;
                MarkUsedInputs(insn);
                break;
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearEntry:
            case IROLinearExit:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            default:
                CError_FATAL(395);
                break;
        }
        insn = insn->next;
    }
    IroVars_CheckTimedLongjmp();
}

void mark_referenced_linear_nodes(void)
{
    IROLinear *p;
    int i;

    for (p = linear_head; p != NULL; p = p->next) {
        p->flags &= ~2u;
    }

    for (p = linear_head; p != NULL; p = p->next) {
        switch (p->type) {
            case IROLinearNop:
            case IROLinearOperand:
            case IROLinearGoto:
            case IROLinearLabel:
            case IROLinearEntry:
            case IROLinearExit:
            case IROLinearAsm:
            case IROLinearEnd:
                break;
            case IROLinearOp1Arg:
            case IROLinearBeginCatch:
            case IROLinearEndCatch:
            case IROLinearEndCatchDtor:
                p->u.monadic->flags |= 2u;
                break;
            case IROLinearOp2Arg:
                p->u.diadic.left->flags |= 2u, p->u.diadic.right->flags |= 2u;
                break;
            case IROLinearIf:
            case IROLinearIfNot:
                p->u.branch.cond->flags |= 2u;
                break;
            case IROLinearReturn:
                if (p->u.monadic)
                    p->u.monadic->flags |= 2u;
                break;
            case IROLinearSwitch:
                p->u.swtch.cond->flags |= 2u;
                break;
            case IROLinearFunccall:
                p->u.funccall.callee->flags |= 2u;
                for (i = 0; i < p->u.funccall.argCount; i++)
                    p->u.funccall.args[i]->flags |= 2u;
                break;
            default:
                CError_FATAL(395);
                break;
        }
    }
}

void visit_statement_expressions(struct Statement *stmt)
{
    linear_head = last_linear = NULL;
    linear_index_counter = 0;
    current_optimizer_statement = statement_insertion_point = NULL;
    while (stmt != NULL) {
        current_statement = (struct Statement *)stmt;
        current_optimizer_statement = stmt;
        data_00587f08 = 0;
        temporary_list = NULL;
        switch (stmt->type) {
            case ST_NOP:
            case ST_LABEL:
            case ST_GOTO:
            case ST_EXIT:
            case ST_ENTRY:
            case ST_ASM:
                break;
            case ST_OVF:
                CError_FATAL(1895);
            case ST_EXPRESSION:
                lower_expression_to_statements(stmt->expr, 0, 0);
                break;
            case ST_SWITCH:
                lower_expression_to_statements(stmt->expr, 1, 0);
                break;
            case ST_IFGOTO:
                lower_expression_to_statements(stmt->expr, 1, 0);
                break;
            case ST_IFNGOTO:
                lower_expression_to_statements(stmt->expr, 1, 0);
                break;
            case ST_RETURN:
                if (stmt->expr != NULL) {
                    lower_expression_to_statements(stmt->expr, 1, 0);
                }
                break;
            case ST_BEGINCATCH:
                lower_expression_to_statements(stmt->expr, 1, 0);
                break;
            case ST_ENDCATCH:
                lower_expression_to_statements(stmt->expr, 1, 0);
                break;
            case ST_ENDCATCHDTOR:
                lower_expression_to_statements(stmt->expr, 1, 0);
                break;
            default:
                CError_FATAL(1944);
        }
        statement_insertion_point = stmt;
        stmt = stmt->next;
    }
    IroVars_CheckTimedLongjmp();
}

static inline IROLinear *new_linear(int type)
{
    IROLinear *node = (IROLinear *)CompilerTools_AllocatePoolMemory(sizeof(IROLinear));
    memset(node, 0, sizeof(IROLinear));
    node->stmt = current_statement;
    node->nodetype = EPOSTINC;
    node->next = NULL;
    node->type = type;
    node->rtype = NULL;
    node->flags = 0;
    node->nodeflags = 0;
    node->expr = NULL;
    node->range = NULL;
    return node;
}

static inline void append_linear(IROLinear *node)
{
    node->index = linear_index_counter;
    linear_index_counter++;
    if (linear_head != NULL)
        last_linear->next = node;
    else
        linear_head = node;
    last_linear = node;
}

static inline void mark_address(IROLinear *operand)
{
    if (operand->type == IROLinearOp2Arg && operand->nodetype == EADD) {
        IROAddrRecord *operands;
        IROLinear *left;
        if ((left = operand->u.diadic.left)->type == EPOSTDEC && left->u.node->type == EOBJREF &&
            operand->u.diadic.right->type == EPOSTDEC && operand->u.diadic.right->u.node->type == EINTCONST)
            left->flags |= IROLF_Ind;
        operands = IroVars_CreateAddrRecord(operand);
        IroVars_CollectAddrRecordElements(operand, operands);
        if (operands->numObjRefs == 1 && (left = operand->u.diadic.left)->type == EPOSTDEC &&
            left->u.node->type == EOBJREF)
            operands->objRefs->element->flags |= IROLF_Ind;
    }
}

struct IROLinear *linearize_expression(ENode *expression)
{
    IROLinear **arguments;
    SInt16 *argumentOrder;
    SInt32 argumentIndex;
    IROLinear *result = NULL;

    switch (expression->type) {
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
        case EBITFIELD: {
            IROLinear *node;

            node = new_linear(2);
            result = node;
            node->u.diadic.left = linearize_expression(expression->data.monadic);
            node->nodetype = expression->type;
            node->rtype = expression->rtype;
            node->nodeflags = expression->flags;
            if (data_00551d6c[node->nodetype] != 0)
                set_monadic_addr_flags(node->u.diadic.left, 1);
            if (node->nodetype == EINDIRECT) {
                mark_address(node->u.diadic.left);
                node->u.diadic.left->flags |= (IROLF_Ind | IROLF_Immind);
                if (node->u.diadic.left->type == IROLinearOp2Arg && node->u.diadic.left->nodetype == EADD)
                    fn_004315b0(node->u.diadic.left);
            }
            break;
        }

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
        case ECOMMA:
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBTST: {
            IROLinear *node;

            node = new_linear(3);
            result = node;
            node->nodeflags = expression->flags;
            if (expression->type != ECOMMA &&
                expression->data.diadic.right->cost >= expression->data.diadic.left->cost) {
                node->u.diadic.right = linearize_expression(expression->data.diadic.right);
                node->u.diadic.left = linearize_expression(expression->data.diadic.left);
                node->flags |= 0x8000;
            } else {
                node->u.diadic.left = linearize_expression(expression->data.diadic.left);
                node->u.diadic.right = linearize_expression(expression->data.diadic.right);
            }
            node->nodetype = expression->type;
            node->rtype = expression->rtype;
            if (data_00551d6c[node->nodetype] != 0)
                set_monadic_addr_flags(node->u.diadic.left, monadic_addr_flags_by_nodetype[node->nodetype]);
            break;
        }

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
        case EBCLR:
        case EBSET: {
            IROLinear *node;

            node = new_linear(3);
            result = node;
            node->nodeflags = expression->flags;
            node->u.diadic.right = linearize_expression(expression->data.diadic.right);
            node->u.diadic.left = linearize_expression(expression->data.diadic.left);
            node->flags |= 0x8000;
            node->nodetype = expression->type;
            node->rtype = expression->rtype;
            set_monadic_addr_flags(node->u.diadic.left, monadic_addr_flags_by_nodetype[node->nodetype]);
            break;
        }

        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case EVECTOR128CONST: {
            IROLinear *operand;
            operand = new_linear(1);
            result = operand;
            operand->nodeflags = expression->flags;
            operand->u.node = expression;
            operand->rtype = expression->rtype;
            break;
        }

        case EFUNCCALL:
        case EFUNCCALLP: {
            IROLinear *node;
            ENodeList *cursor;
            SInt32 argumentCount;

            node = new_linear(10);
            result = node;
            node->nodeflags = expression->flags;
            node->u.funccall.ispure = (expression->type == EFUNCCALLP);
            argumentCount = 0;
            for (cursor = expression->data.funccall.args; cursor != NULL; cursor = cursor->next)
                argumentCount++;
            arguments = NULL;
            if (argumentCount != 0) {
                arguments = (IROLinear **)CompilerTools_AllocatePoolMemory(argumentCount * sizeof(IROLinear *));
                cursor = expression->data.funccall.args;
                for (argumentCount = 0; cursor != NULL; cursor = cursor->next)
                    arguments[argumentCount++] = (IROLinear *)cursor->node;
                argumentOrder = (SInt16 *)CompilerTools_AllocatePoolMemory(argumentCount * sizeof(SInt16));
                for (argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
                    argumentOrder[argumentIndex] = argumentCount - argumentIndex - 1;
                for (argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++) {
                    arguments[argumentOrder[argumentIndex]] =
                        linearize_expression((ENode *)arguments[argumentOrder[argumentIndex]]);
                    mark_node_and_operand_flags(arguments[argumentOrder[argumentIndex]], 1);
                }
            }
            node->u.funccall.argCount = argumentCount;
            node->u.funccall.args = arguments;
            node->u.funccall.callee = linearize_expression(expression->data.funccall.funcref);
            node->u.funccall.functype = expression->data.funccall.functype;
            node->rtype = expression->rtype;
            break;
        }

        default:
            CError_FATAL(1850);
            break;
    }

    if (result != NULL) {
        append_linear(result);
    }
    return result;
}

static inline Statement *NewStmt(UInt8 type)
{
    Statement *s = (Statement *)CompilerTools_AllocatePool(0x1a);
    memset(s, 0, 0x1a);
    s->type = type;
    return s;
}

static inline void AppendStmt(Statement *s)
{
    s->dobjstack = current_optimizer_statement->dobjstack;
    s->sourceoffset = current_optimizer_statement->sourceoffset;
    s->value = current_optimizer_statement->value;
    s->flags = current_optimizer_statement->flags;
    s->next = statement_insertion_point->next;
    statement_insertion_point->next = s;
    statement_insertion_point = s;
}

static inline Object *LookupTemporary(ENode *node)
{
    List12 *p = temporary_list;
    while (p != NULL) {
        if (p->nullCheckExpression == node->data.monadic)
            break;
        p = p->next;
    }
    CError_ASSERT(809, p != NULL);
    return p->temporary;
}

void lower_expression_to_statements(ENode *node, int valueNeeded, int force)
{
    UInt8 type = node->type;
    Object *forceLoadTemp;
    CLabel *andEndLabel;
    Object *andResult;
    CLabel *orEndLabel;
    Object *orResult;
    Object *conditionalResult;
    CLabel *nullCheckEndLabel;
    Object *nullCheckTemp;

    switch (type) {
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
            lower_expression_to_statements(node->data.monadic, 1, 0);
            if (node->type == EFORCELOAD) {
                lower_monadic_expression_to_statement(node, &forceLoadTemp);
                node->type = EINDIRECT;
                node->data.monadic = create_objectrefnode(forceLoadTemp);
                CError_ASSERT(1356, node->rtype->type != TYPEVOID);
            }
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
        case ECOMMA:
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBTST: {
            Statement *statement;
            if (node->type == ECOMMA && data_00587f08 == 0) {
                lower_expression_to_statements(node->data.diadic.left, 0, 1);
                statement = NewStmt(4);
                statement->expr = node->data.diadic.left, AppendStmt(statement);
                lower_expression_to_statements(node->data.diadic.right, valueNeeded, 0);
                extract_right_operand_to_statement(node, force);
            } else {
                if (node->data.diadic.right->cost >= node->data.diadic.left->cost) {
                    lower_expression_to_statements(node->data.diadic.right, 1, 0);
                    lower_expression_to_statements(node->data.diadic.left, 1, 0);
                } else {
                    lower_expression_to_statements(node->data.diadic.left, 1, 0);
                    lower_expression_to_statements(node->data.diadic.right, 1, 0);
                }
            }
            break;
        }

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
        case EBCLR:
        case EBSET:
            lower_expression_to_statements(node->data.diadic.right, 1, 0);
            lower_expression_to_statements(node->data.diadic.left, 1, 0);
            break;

        case ELAND:
        case ELOR:
            if (node->type == ELAND) {
                Statement *statement;
                create_zero_initialized_temp_object(node, &andResult);
                lower_expression_to_statements(node->data.diadic.left, 1, 0);
                andEndLabel = IroUtil_NewLabel();
                statement = NewStmt(7);
                statement->expr = node->data.diadic.left;
                statement->label = andEndLabel, AppendStmt(statement);
                lower_expression_to_statements(node->data.diadic.right, 1, 0);
                IrOptimizer_00430820(node, &andResult, &andEndLabel);
            } else if (node->type == ELOR) {
                Statement *statement;
                insert_intconst_assignment(node, &orResult);
                lower_expression_to_statements(node->data.diadic.left, 1, 0);
                orEndLabel = IroUtil_NewLabel();
                statement = NewStmt(6);
                statement->expr = node->data.diadic.left;
                statement->label = orEndLabel, AppendStmt(statement);
                lower_expression_to_statements(node->data.diadic.right, 1, 0);
                fn_004305e0(node, &orResult, &orEndLabel);
            }
            break;

        case ECOND: {
            Statement *statement;
            CLabel *elseLabel;
            CLabel *endLabel;
            lower_expression_to_statements(node->data.cond.cond, 1, 0);
            elseLabel = IroUtil_NewLabel();
            statement = NewStmt(7);
            statement->expr = node->data.cond.cond;
            statement->label = elseLabel, AppendStmt(statement);
            lower_expression_to_statements(node->data.cond.expr1, 1, 0);
            IrOptimizer_00430e60(node, &conditionalResult);
            endLabel = IroUtil_NewLabel();
            statement = NewStmt(3);
            statement->label = endLabel, AppendStmt(statement);
            statement = NewStmt(2);
            statement->label = elseLabel;
            statement->label->stmt = statement, AppendStmt(statement);
            lower_expression_to_statements(node->data.cond.expr2, 1, 0);
            append_cond_expr2_statement(node, &conditionalResult);
            statement = NewStmt(2);
            statement->label = endLabel;
            statement->label->stmt = statement, AppendStmt(statement);
            if (node->rtype->type != TYPEVOID) {
                node->type = EINDIRECT;
                node->data.cond.cond = create_objectrefnode(conditionalResult);
                CError_ASSERT(1495, node->rtype->type != TYPEVOID);
            } else {
                node->type = EINTCONST;
                node->data.intval = cint64_zero;
            }
            break;
        }

        case EPRECOMP: {
            Object *temporary = LookupTemporary(node);
            node->type = EINDIRECT;
            node->data.monadic = create_objectrefnode(temporary);
            CError_ASSERT(1513, node->rtype->type != TYPEVOID);
            break;
        }

        case ENULLCHECK: {
            Statement *statement;
            lower_expression_to_statements(node->data.diadic.left, 1, 0);
            create_temp_object_assignment(node, &nullCheckTemp);
            insert_indirect_statement_with_label(node, &nullCheckTemp, &nullCheckEndLabel);
            lower_expression_to_statements(node->data.diadic.right, 1, 0);
            IrOptimizer_00430a60(node, &nullCheckTemp);
            statement = NewStmt(2);
            statement->label = nullCheckEndLabel;
            statement->label->stmt = statement, AppendStmt(statement);
            if (node->rtype->type != TYPEVOID) {
                node->type = EINDIRECT;
                node->data.diadic.left = create_objectrefnode(nullCheckTemp);
                CError_ASSERT(1538, node->rtype->type != TYPEVOID);
            } else {
                node->type = EINTCONST;
                node->data.intval = cint64_zero;
            }
            break;
        }

        case EFUNCCALL:
        case EFUNCCALLP: {
            ENodeList *argument;
            ENode **arguments;
            SInt16 *argumentOrder;
            int argumentCount;
            int filledCount, index, orderIndex;
            data_00588518 = 0;
            argument = node->data.funccall.args;
            argumentCount = 0;
            while (argument != NULL) {
                argument = argument->next;
                argumentCount++;
            }
            if (argumentCount != 0) {
                arguments = (ENode **)CompilerTools_AllocatePoolMemory(argumentCount * sizeof(*arguments));
                argument = node->data.funccall.args;
                filledCount = 0;
                while (argument != NULL) {
                    arguments[filledCount++] = argument->node;
                    argument = argument->next;
                }
                argumentOrder = (SInt16 *)CompilerTools_AllocatePoolMemory(filledCount * sizeof(*argumentOrder));
                for (index = 0; index < filledCount; index++)
                    argumentOrder[index] = filledCount - index - 1;
                for (orderIndex = 0; orderIndex < filledCount; orderIndex++)
                    lower_expression_to_statements(arguments[argumentOrder[orderIndex]], 1, 0);
            }
            lower_expression_to_statements(node->data.funccall.funcref, 1, 0);
            break;
        }

        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ESETCONST:
        case EVECTOR128CONST:
            break;

        default:
            CError_FATAL(1621);
    }
}

static ENode *NewENode(UInt8 type)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    memset(n, 0, 0x1a);
    n->type = type;
    return n;
}

static Statement *NewIRStat(UInt8 type)
{
    Statement *n = (Statement *)CompilerTools_AllocatePool(0x1a);
    memset(n, 0, 0x1a);
    n->type = type;
    return n;
}

void fn_004305e0(ENode *e, Object **pp, CLabel **lab)
{
    Statement *s;
    ENode *ref;
    ENode *c;
    Statement *a;
    Statement *g;

    s = NewIRStat(6);
    s->expr = e->data.diadic.right;
    s->label = *lab;
    s->dobjstack = current_optimizer_statement->dobjstack;
    s->sourceoffset = current_optimizer_statement->sourceoffset;
    s->value = current_optimizer_statement->value;
    s->flags = current_optimizer_statement->flags;
    s->next = statement_insertion_point->next;
    statement_insertion_point->next = s;
    statement_insertion_point = s;

    ref = NewENode(EINDIRECT);
    ref->data.monadic = create_objectrefnode(*pp);
    if (e->rtype->type != TYPEVOID)
        ref->rtype = e->rtype;
    else
        CError_FATAL(1286);

    c = NewENode(EINTCONST);
    c->data.intval = cint64_zero;
    c->rtype = e->rtype;

    a = NewIRStat(4);
    a->expr = NewENode(EASS);
    a->expr->data.diadic.left = ref;
    a->expr->data.diadic.right = c;
    a->expr->rtype = e->rtype;
    a->dobjstack = current_optimizer_statement->dobjstack;
    a->sourceoffset = current_optimizer_statement->sourceoffset;
    a->value = current_optimizer_statement->value;
    a->flags = current_optimizer_statement->flags;
    a->next = statement_insertion_point->next;
    statement_insertion_point->next = a;
    statement_insertion_point = a;

    g = NewIRStat(2);
    g->label = *lab;
    g->label->stmt = g;
    g->dobjstack = current_optimizer_statement->dobjstack;
    g->sourceoffset = current_optimizer_statement->sourceoffset;
    g->value = current_optimizer_statement->value;
    g->flags = current_optimizer_statement->flags;
    g->next = statement_insertion_point->next;
    statement_insertion_point->next = g;
    statement_insertion_point = g;

    e->type = EINDIRECT;
    e->data.monadic = create_objectrefnode(*pp);
    CError_ASSERT(1328, e->rtype->type != TYPEVOID);
}

void IrOptimizer_00430820(ENode *e, Object **pp, CLabel **lab)
{
    Statement *s;
    ENode *ref;
    ENode *c;
    Statement *a;
    Statement *g;

    s = NewIRStat(7);
    s->expr = e->data.diadic.right;
    s->label = *lab;
    s->dobjstack = current_optimizer_statement->dobjstack;
    s->sourceoffset = current_optimizer_statement->sourceoffset;
    s->value = current_optimizer_statement->value;
    s->flags = current_optimizer_statement->flags;
    s->next = statement_insertion_point->next;
    statement_insertion_point->next = s;
    statement_insertion_point = s;

    ref = NewENode(EINDIRECT);
    ref->data.monadic = create_objectrefnode(*pp);
    if (e->rtype->type != TYPEVOID)
        ref->rtype = e->rtype;
    else
        CError_FATAL(1170);

    c = NewENode(EINTCONST);
    c->data.intval = cint64_one;
    c->rtype = e->rtype;

    a = NewIRStat(4);
    a->expr = NewENode(EASS);
    a->expr->data.diadic.left = ref;
    a->expr->data.diadic.right = c;
    a->expr->rtype = e->rtype;
    a->dobjstack = current_optimizer_statement->dobjstack;
    a->sourceoffset = current_optimizer_statement->sourceoffset;
    a->value = current_optimizer_statement->value;
    a->flags = current_optimizer_statement->flags;
    a->next = statement_insertion_point->next;
    statement_insertion_point->next = a;
    statement_insertion_point = a;

    g = NewIRStat(2);
    g->label = *lab;
    g->label->stmt = g;
    g->dobjstack = current_optimizer_statement->dobjstack;
    g->sourceoffset = current_optimizer_statement->sourceoffset;
    g->value = current_optimizer_statement->value;
    g->flags = current_optimizer_statement->flags;
    g->next = statement_insertion_point->next;
    statement_insertion_point->next = g;
    statement_insertion_point = g;

    e->type = EINDIRECT;
    e->data.monadic = create_objectrefnode(*pp);
    CError_ASSERT(1212, e->rtype->type != TYPEVOID);
}

void IrOptimizer_00430a60(ENode *p, Object **objp)
{
    ENode *c;
    Statement *b;
    ENode *a;

    a = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memset(a, 0, sizeof(ENode));
    a->type = EINDIRECT;
    a->data.diadic.left = create_objectrefnode(*objp);
    if (p->rtype->type)
        a->rtype = p->rtype;
    else
        a->rtype = p->data.diadic.left->rtype;

    b = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
    memset(b, 0, sizeof(Statement));
    b->type = ST_EXPRESSION;

    c = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memset(c, 0, sizeof(ENode));
    c->type = EASS;
    b->expr = c;
    b->expr->data.diadic.left = a;
    b->expr->data.diadic.right = p->data.diadic.right;
    if (p->rtype->type)
        b->expr->rtype = p->rtype;
    else
        b->expr->rtype = p->data.diadic.left->rtype;

    b->dobjstack = current_optimizer_statement->dobjstack;
    b->sourceoffset = current_optimizer_statement->sourceoffset;
    b->value = current_optimizer_statement->value;
    b->flags = current_optimizer_statement->flags;

    b->next = statement_insertion_point->next;
    statement_insertion_point->next = b;
    statement_insertion_point = b;
}

void insert_indirect_statement_with_label(ENode *node, Object **object, struct CLabel **label)
{
    Statement *statement;
    ENode *indirect;

    indirect = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memset(indirect, 0, sizeof(ENode));
    indirect->type = EINDIRECT;
    indirect->data.monadic = create_objectrefnode(*object);
    if (node->rtype->type == TYPEVOID)
        indirect->rtype = node->data.monadic->rtype;
    else
        indirect->rtype = node->rtype;

    *label = IroUtil_NewLabel();

    statement = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
    memset(statement, 0, sizeof(Statement));
    statement->type = ST_IFNGOTO;
    statement->expr = indirect;
    statement->label = *label;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

void lower_monadic_expression_to_statement(ENode *e, Object **pp)
{
    ENode *ref;
    Statement *n;

    if (e->rtype->type != TYPEVOID) {
        *pp = create_temp_object(e->rtype);
        ref = NewENode(EINDIRECT);
        ref->data.monadic = create_objectrefnode(*pp);
        ref->rtype = e->rtype;
        CError_ASSERT(970, e->rtype->type != TYPEVOID);
    }

    n = NewIRStat(4);
    if (e->rtype->type != TYPEVOID) {
        n->expr = NewENode(EASS);
        n->expr->data.diadic.left = ref;
        n->expr->data.diadic.right = e->data.monadic;
        n->expr->rtype = e->rtype;
    } else {
        n->expr = e->data.monadic;
    }
    n->dobjstack = current_optimizer_statement->dobjstack;
    n->sourceoffset = current_optimizer_statement->sourceoffset;
    n->value = current_optimizer_statement->value;
    n->flags = current_optimizer_statement->flags;
    n->next = statement_insertion_point->next;
    statement_insertion_point->next = n;
    statement_insertion_point = n;
}

void append_cond_expr2_statement(ENode *e, Object **pp)
{
    ENode *ref;
    Statement *n;

    if (e->rtype->type != TYPEVOID) {
        ref = NewENode(EINDIRECT);
        ref->data.monadic = create_objectrefnode(*pp);
        ref->rtype = e->rtype;
        CError_ASSERT(923, e->rtype->type != TYPEVOID);
    }

    n = NewIRStat(4);
    if (e->rtype->type != TYPEVOID) {
        n->expr = NewENode(EASS);
        n->expr->data.diadic.left = ref;
        n->expr->data.diadic.right = e->data.cond.expr2;
        n->expr->rtype = e->rtype;
    } else {
        n->expr = e->data.cond.expr2;
    }
    n->dobjstack = current_optimizer_statement->dobjstack;
    n->sourceoffset = current_optimizer_statement->sourceoffset;
    n->value = current_optimizer_statement->value;
    n->flags = current_optimizer_statement->flags;
    n->next = statement_insertion_point->next;
    statement_insertion_point->next = n;
    statement_insertion_point = n;
}

void IrOptimizer_00430e60(ENode *e, Object **pp)
{
    ENode *ref;
    Statement *n;

    if (e->rtype->type != TYPEVOID) {
        *pp = create_temp_object(e->rtype);
        ref = NewENode(EINDIRECT);
        ref->data.monadic = create_objectrefnode(*pp);
        ref->rtype = e->rtype;
        CError_ASSERT(877, e->rtype->type != TYPEVOID);
    }

    n = NewIRStat(4);
    if (e->rtype->type != TYPEVOID) {
        n->expr = NewENode(EASS);
        n->expr->data.diadic.left = ref;
        n->expr->data.diadic.right = e->data.diadic.right;
        n->expr->rtype = e->rtype;
    } else {
        n->expr = e->data.diadic.right;
    }
    n->dobjstack = current_optimizer_statement->dobjstack;
    n->sourceoffset = current_optimizer_statement->sourceoffset;
    n->value = current_optimizer_statement->value;
    n->flags = current_optimizer_statement->flags;
    n->next = statement_insertion_point->next;
    statement_insertion_point->next = n;
    statement_insertion_point = n;
}

void create_temp_object_assignment(ENode *expression, Object **tempObject)
{
    Object *object;
    List12 *entry;
    ENode *reference;
    Statement *statement;

    if (expression->rtype->type != TYPEVOID)
        *tempObject = create_temp_object(expression->rtype);
    else
        *tempObject = create_temp_object(expression->data.monadic->rtype);

    object = *tempObject;
    entry = (List12 *)CompilerTools_AllocatePool(sizeof(List12));
    entry->nullCheckExpression = (ENode *)expression->data.funccall.functype;
    entry->temporary = object;
    entry->next = NULL;
    if (temporary_list) {
        entry->next = temporary_list;
        temporary_list = entry;
    } else {
        temporary_list = entry;
    }

    reference = NewENode(EINDIRECT);
    reference->data.monadic = create_objectrefnode(*tempObject);
    if (expression->rtype->type != TYPEVOID)
        reference->rtype = expression->rtype;
    else
        reference->rtype = expression->data.monadic->rtype;

    statement = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
    memset(statement, 0, sizeof(Statement));
    statement->type = ST_EXPRESSION;
    statement->expr = NewENode(EASS);
    statement->expr->data.diadic.left = reference;
    statement->expr->data.diadic.right = expression->data.monadic;
    if (expression->rtype->type != TYPEVOID)
        statement->expr->rtype = expression->rtype;
    else
        statement->expr->rtype = expression->data.monadic->rtype;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

static void *IRO_NewNode(UInt8 type)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    memset(n, 0, 0x1a);
    n->type = type;
    return n;
}

void insert_intconst_assignment(ENode *expr, Object **out)
{
    Statement *statement;
    ENode *indirect;
    ENode *one;

    *out = create_temp_object(expr->rtype);

    indirect = IRO_NewNode(EINDIRECT);
    indirect->data.monadic = create_objectrefnode(*out);
    if (expr->rtype->type != TYPEVOID) {
        indirect->rtype = expr->rtype;
    } else {
        CError_FATAL(683);
    }

    one = IRO_NewNode(EINTCONST);
    one->data.intval = cint64_one;
    one->rtype = expr->rtype;

    statement = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
    memset(statement, 0, sizeof(Statement));
    statement->type = ST_EXPRESSION;
    statement->expr = IRO_NewNode(EASS);
    statement->expr->data.diadic.left = indirect;
    statement->expr->data.diadic.right = one;
    statement->expr->rtype = expr->rtype;

    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;

    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

void create_zero_initialized_temp_object(ENode *expr, Object **out)
{
    Statement *statement;
    ENode *indirect;
    ENode *zero;

    *out = create_temp_object(expr->rtype);

    indirect = IRO_NewNode(EINDIRECT);
    indirect->data.monadic = create_objectrefnode(*out);
    if (expr->rtype->type != TYPEVOID) {
        indirect->rtype = expr->rtype;
    } else {
        CError_FATAL(645);
    }

    zero = IRO_NewNode(EINTCONST);
    zero->data.intval = cint64_zero;
    zero->rtype = expr->rtype;

    statement = (Statement *)CompilerTools_AllocatePool(sizeof(*statement));
    memset(statement, 0, sizeof(*statement));
    statement->type = ST_EXPRESSION;
    statement->expr = IRO_NewNode(EASS);
    statement->expr->data.diadic.left = indirect;
    statement->expr->data.diadic.right = zero;
    statement->expr->rtype = expr->rtype;

    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;

    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

static void *new_enode(UInt8 type)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    memset(n, 0, 0x1a);
    n->type = type;
    return n;
}

void extract_right_operand_to_statement(ENode *node, int force)
{
    Object *obj;
    Statement *stmt;
    ENode *ind;

    if (node->rtype->type != TYPEVOID && force == 0) {
        obj = create_temp_object(node->rtype);
        ind = new_enode(EINDIRECT);
        ind->data.monadic = create_objectrefnode(obj);
        if (node->rtype->type != TYPEVOID) {
            ind->rtype = node->rtype;
        } else {
            CError_FATAL(574);
        }
        stmt = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
        memset(stmt, 0, sizeof(Statement));
        stmt->type = ST_EXPRESSION;
        stmt->expr = new_enode(EASS);
        stmt->expr->data.diadic.left = ind;
        stmt->expr->data.diadic.right = node->data.diadic.right;
        stmt->expr->rtype = node->rtype;
        stmt->dobjstack = current_optimizer_statement->dobjstack;
        stmt->sourceoffset = current_optimizer_statement->sourceoffset;
        stmt->value = current_optimizer_statement->value;
        stmt->flags = current_optimizer_statement->flags;
        stmt->next = statement_insertion_point->next;
        statement_insertion_point->next = stmt;
        statement_insertion_point = stmt;
        node->type = EINDIRECT;
        node->data.monadic = create_objectrefnode(obj);
        CError_ASSERT(604, node->rtype->type != TYPEVOID);
    } else {
        stmt = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
        memset(stmt, 0, sizeof(Statement));
        stmt->type = ST_EXPRESSION;
        stmt->expr = node->data.diadic.right;
        stmt->dobjstack = current_optimizer_statement->dobjstack;
        stmt->sourceoffset = current_optimizer_statement->sourceoffset;
        stmt->value = current_optimizer_statement->value;
        stmt->flags = current_optimizer_statement->flags;
        stmt->next = statement_insertion_point->next;
        statement_insertion_point->next = stmt;
        statement_insertion_point = stmt;
        node->type = EINTCONST;
        node->data.intval = cint64_zero;
    }
}

void mark_node_and_operand_flags(IROLinear *node, int flag)
{
    int i;
    node->flags |= flag << 14;
    switch (node->type) {
        case IROLinearOp1Arg:
        case IROLinearBeginCatch:
        case IROLinearEndCatch:
        case IROLinearEndCatchDtor:
            mark_node_and_operand_flags(node->u.diadic.left, 1);
            break;
        case IROLinearOp2Arg:
            mark_node_and_operand_flags(node->u.diadic.left, 1);
            mark_node_and_operand_flags(node->u.diadic.right, 1);
            break;
        case IROLinearIf:
        case IROLinearIfNot:
            mark_node_and_operand_flags(node->u.diadic.right, 1);
            break;
        case IROLinearReturn:
            if (node->u.diadic.left)
                mark_node_and_operand_flags(node->u.diadic.left, 1);
            break;
        case IROLinearSwitch:
            mark_node_and_operand_flags(node->u.diadic.right, 1);
            break;
        case IROLinearFunccall:
            mark_node_and_operand_flags(node->u.funccall.callee, 1);
            for (i = 0; i < node->u.funccall.argCount; i++)
                mark_node_and_operand_flags(node->u.funccall.args[i], 1);
            break;
        case IROLinearNop:
        case IROLinearOperand:
        case IROLinearGoto:
        case IROLinearLabel:
        case IROLinearEntry:
        case IROLinearExit:
        case IROLinearAsm:
        case 17:
        case 18:
        case IROLinearEnd:
            break;
    }
}

void fn_004315b0(IROLinear *node)
{
    node->flags |= IROLF_Subs;
    if (node->type == IROLinearOp2Arg && node->nodetype == EADD) {
        if ((&node->u.diadic.left, node->u.diadic.left)->type == IROLinearOp1Arg &&
            node->u.diadic.left->nodetype == EINDIRECT) {
            node->u.diadic.left->flags |= IROLF_Subs;
        }
        if ((&node->u.diadic.right, node->u.diadic.right)->type == IROLinearOp1Arg &&
            node->u.diadic.right->nodetype == EINDIRECT) {
            node->u.diadic.right->flags |= IROLF_Subs;
        }
        if (node->u.diadic.left->type == IROLinearOp2Arg && node->u.diadic.left->nodetype == EADD) {
            fn_004315b0(node->u.diadic.left);
        }
        if (node->u.diadic.right->type == IROLinearOp2Arg && node->u.diadic.right->nodetype == EADD) {
            fn_004315b0(node->u.diadic.right);
        }
    }
    return;
}

unsigned int return_zero_for_irolinear(struct IROLinear *linear)
{
    return 0U;
}

void set_monadic_addr_flags(IROLinear *p, int flag)
{
    if (p->type == IROLinearOp1Arg && p->nodetype == EINDIRECT) {
        p->flags |= IROLF_Assigned;
        if (p->u.monadic->type == IROLinearOperand) {
            p->u.monadic->flags |= IROLF_Assigned;
            if (flag) {
                p->flags |= IROLF_Used;
                p->u.monadic->flags |= IROLF_Used;
            }
        }
    }
    if (p->type == IROLinearOp1Arg && p->nodetype == EINDIRECT) {
        IROLinear *r;
        p->flags |= IROLF_Assigned;
        r = p->u.monadic;
        if (r->type == IROLinearOp2Arg && r->nodetype == EADD) {
            if (r->u.diadic.left->type == EPOSTDEC && r->u.diadic.left->u.node->type == EOBJREF) {
                if (r->u.diadic.right->type == EPOSTDEC && r->u.diadic.right->u.node->type == EINTCONST) {
                    r->u.diadic.left->flags |= IROLF_Assigned;
                }
                if (flag) {
                    p->flags |= IROLF_Used;
                    r->u.diadic.left->flags |= IROLF_Used;
                }
            }
            {
                IROAddrRecord *n = IroVars_CreateAddrRecord(r);
                IroVars_CollectAddrRecordElements(r, n);
                if (n->numObjRefs == 1 && r->u.diadic.left->type == EPOSTDEC &&
                    r->u.diadic.left->u.node->type == EOBJREF) {
                    n->objRefs->element->flags |= IROLF_Assigned;
                    if (flag) {
                        p->flags |= IROLF_Used;
                        n->objRefs->element->flags |= IROLF_Used;
                    }
                }
            }
        }
    }
}

IROLinear *IrOptimizer_NewLinear(unsigned char kind)
{
    IROLinear *linear;
    linear = CompilerTools_AllocatePoolMemory(sizeof(IROLinear));
    memset(linear, 0, sizeof(IROLinear));
    linear->stmt = current_statement;
    linear->nodetype = EPOSTINC;
    linear->next = NULL;
    linear->type = kind;
    linear->rtype = NULL;
    linear->flags = 0;
    linear->nodeflags = 0;
    linear->expr = NULL;
    linear->range = NULL;
    return linear;
}
