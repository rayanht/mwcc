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

SInt32 data_00587148;
SInt32 data_005871b0;
struct IROLinear *last_linear;
SInt32 data_005876e4;
struct Statement *current_optimizer_statement;
SInt32 data_00587e50;
struct List12 *temporary_list;
SInt32 data_00587f08;
struct IROLinear *linear_head;
SInt32 data_0058800c;
SInt32 data_005880c8;
struct Statement *statement_insertion_point;
SInt32 data_00588244;
UInt8 data_00588518;
UInt8 data_00588526;

static Statement *current_statement;
static UInt8 data_0057f6b4;
static UInt8 data_0057f6b5;
static char lbl_0057f6b6[10];

static inline Statement *NewStmt(UInt8 type);

static inline UInt8 IRO_CopyPropagationSetting(void)
{
    return copts.propagation;
}

#if VERSION > VERSION_GC_1_0
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

#define IR_REPLACE(replace, field) replace(field, ctx, obj, newval, doReplace)
#endif

static inline Statement *NewStmt(UInt8 type)
{
    Statement *s = (Statement *)lalloc(0x1a);
    memset(s, 0, 0x1a);
    s->type = type;
    return s;
}

static void append_linear(IROLinear *node)
{
    node->index = linear_index_counter;
    linear_index_counter++;
    if (linear_head != NULL)
        last_linear->next = node;
    else
        linear_head = node;
    last_linear = node;
}

IROLinear *IrOptimizer_NewLinear(unsigned char kind)
{
    IROLinear *linear;
    linear = oalloc(sizeof(IROLinear));
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

unsigned int return_zero_for_irolinear(struct IROLinear *linear)
{
    return 0U;
}

void fn_004315b0(IROLinear *node)
{
    node->flags |= IROLF_Subs;
    if (node->type == IROLinearOp2Arg && node->nodetype == EADD) {
        if (return_zero_for_irolinear(node->u.diadic.left) ||
            (node->u.diadic.left->type == IROLinearOp1Arg && node->u.diadic.left->nodetype == EINDIRECT))
            node->u.diadic.left->flags |= IROLF_Subs;
        if (return_zero_for_irolinear(node->u.diadic.right) ||
            (node->u.diadic.right->type == IROLinearOp1Arg && node->u.diadic.right->nodetype == EINDIRECT))
            node->u.diadic.right->flags |= IROLF_Subs;
        if (node->u.diadic.left->type == IROLinearOp2Arg && node->u.diadic.left->nodetype == EADD) {
            fn_004315b0(node->u.diadic.left);
        }
        if (node->u.diadic.right->type == IROLinearOp2Arg && node->u.diadic.right->nodetype == EADD) {
            fn_004315b0(node->u.diadic.right);
        }
    }
    return;
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

static void mark_address(IROLinear *operand)
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
static void append_comma_left_statement(ENode *node)
{
    Statement *statement;

    statement = NewStmt(ST_EXPRESSION);
    statement->expr = node->data.diadic.left;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

void extract_right_operand_to_statement(ENode *node, int force)
{
    Object *obj;
    Statement *stmt;
    ENode *ind;

    if (node->rtype->type != TYPEVOID && force == 0) {
        obj = create_temp_object(node->rtype);
        ind = IrOptimizer_NewENode(EINDIRECT);
        ind->data.monadic = create_objectrefnode(obj);
        if (node->rtype->type != TYPEVOID) {
            ind->rtype = node->rtype;
        } else {
            CError_FATAL(574);
        }
        stmt = (Statement *)lalloc(sizeof(Statement));
        memset(stmt, 0, sizeof(Statement));
        stmt->type = ST_EXPRESSION;
        stmt->expr = IrOptimizer_NewENode(EASS);
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
        stmt = (Statement *)lalloc(sizeof(Statement));
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

void create_zero_initialized_temp_object(ENode *expr, Object **out)
{
    Statement *statement;
    ENode *indirect;
    ENode *zero;

    *out = create_temp_object(expr->rtype);

    indirect = IrOptimizer_NewENode(EINDIRECT);
    indirect->data.monadic = create_objectrefnode(*out);
    if (expr->rtype->type != TYPEVOID) {
        indirect->rtype = expr->rtype;
    } else {
        CError_FATAL(645);
    }

    zero = IrOptimizer_NewENode(EINTCONST);
    zero->data.intval = cint64_zero;
    zero->rtype = expr->rtype;

    statement = (Statement *)lalloc(sizeof(*statement));
    memset(statement, 0, sizeof(*statement));
    statement->type = ST_EXPRESSION;
    statement->expr = IrOptimizer_NewENode(EASS);
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

void insert_intconst_assignment(ENode *expr, Object **out)
{
    Statement *statement;
    ENode *indirect;
    ENode *one;

    *out = create_temp_object(expr->rtype);

    indirect = IrOptimizer_NewENode(EINDIRECT);
    indirect->data.monadic = create_objectrefnode(*out);
    if (expr->rtype->type != TYPEVOID) {
        indirect->rtype = expr->rtype;
    } else {
        CError_FATAL(683);
    }

    one = IrOptimizer_NewENode(EINTCONST);
    one->data.intval = cint64_one;
    one->rtype = expr->rtype;

    statement = (Statement *)lalloc(sizeof(Statement));
    memset(statement, 0, sizeof(Statement));
    statement->type = ST_EXPRESSION;
    statement->expr = IrOptimizer_NewENode(EASS);
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

static void append_cond_test_statement(ENode *node, CLabel **label)
{
    Statement *statement;

    *label = IroUtil_NewLabel();
    statement = NewStmt(ST_IFNGOTO);
    statement->expr = node->data.cond.cond;
    statement->label = *label;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

static void append_goto_statement(ENode *node, CLabel **label)
{
    Statement *statement;

    *label = IroUtil_NewLabel();
    statement = NewStmt(ST_GOTO);
    statement->label = *label;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

static void record_temporary(ENode *node, Object *temporary)
{
    List12 *entry;

    entry = (List12 *)lalloc(sizeof(List12));
    entry->nullCheckExpression = (ENode *)node->data.funccall.functype;
    entry->temporary = temporary;
    entry->next = NULL;
    if (temporary_list) {
        entry->next = temporary_list;
        temporary_list = entry;
    } else {
        temporary_list = entry;
    }
}

static Object *LookupTemporary(ENode *node)
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

void create_temp_object_assignment(ENode *expression, Object **tempObject)
{
    Statement *statement;
    ENode *reference;

    if (expression->rtype->type != TYPEVOID)
        *tempObject = create_temp_object(expression->rtype);
    else
        *tempObject = create_temp_object(expression->data.monadic->rtype);
    record_temporary(expression, *tempObject);
    reference = IrOptimizer_NewENode(EINDIRECT);
    reference->data.monadic = create_objectrefnode(*tempObject);
    if (expression->rtype->type != TYPEVOID)
        reference->rtype = expression->rtype;
    else
        reference->rtype = expression->data.monadic->rtype;
#if VERSION == VERSION_GC_1_0
    statement = NewStmt(ST_EXPRESSION);
#else
    statement = (Statement *)lalloc(sizeof(Statement));
    memset(statement, 0, sizeof(Statement));
    statement->type = ST_EXPRESSION;
#endif
    statement->expr = IrOptimizer_NewENode(EASS);
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
void fn_00430e60(ENode *e, Object **pp)
{
    ENode *ref;
    Statement *n;

    if (e->rtype->type != TYPEVOID) {
        *pp = create_temp_object(e->rtype);
        ref = IrOptimizer_NewENode(EINDIRECT);
        ref->data.monadic = create_objectrefnode(*pp);
        ref->rtype = e->rtype;
        CError_ASSERT(877, e->rtype->type != TYPEVOID);
    }

    n = NewStmt(4);
    if (e->rtype->type != TYPEVOID) {
        n->expr = IrOptimizer_NewENode(EASS);
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

void append_cond_expr2_statement(ENode *e, Object **pp)
{
    ENode *ref;
    Statement *n;

    if (e->rtype->type != TYPEVOID) {
        ref = IrOptimizer_NewENode(EINDIRECT);
        ref->data.monadic = create_objectrefnode(*pp);
        ref->rtype = e->rtype;
        CError_ASSERT(923, e->rtype->type != TYPEVOID);
    }

    n = NewStmt(4);
    if (e->rtype->type != TYPEVOID) {
        n->expr = IrOptimizer_NewENode(EASS);
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

void lower_monadic_expression_to_statement(ENode *e, Object **pp)
{
    ENode *ref;
    Statement *n;

    if (e->rtype->type != TYPEVOID) {
        *pp = create_temp_object(e->rtype);
        ref = IrOptimizer_NewENode(EINDIRECT);
        ref->data.monadic = create_objectrefnode(*pp);
        ref->rtype = e->rtype;
        CError_ASSERT(970, e->rtype->type != TYPEVOID);
    }

    n = NewStmt(4);
    if (e->rtype->type != TYPEVOID) {
        n->expr = IrOptimizer_NewENode(EASS);
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

static void append_label_statement(CLabel **label)
{
    Statement *statement;

    statement = NewStmt(ST_LABEL);
    statement->label = *label;
    statement->label->stmt = statement;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

void insert_indirect_statement_with_label(ENode *node, Object **object, struct CLabel **label)
{
    Statement *statement;
    ENode *indirect;

    indirect = IrOptimizer_NewENode(EINDIRECT);
    indirect->data.monadic = create_objectrefnode(*object);
    if (node->rtype->type == TYPEVOID)
        indirect->rtype = node->data.monadic->rtype;
    else
        indirect->rtype = node->rtype;

    *label = IroUtil_NewLabel();

    statement = (Statement *)lalloc(sizeof(Statement));
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

void fn_00430a60(ENode *p, Object **objp)
{
    Statement *b;
    ENode *a;

    a = IrOptimizer_NewENode(EINDIRECT);
    a->data.diadic.left = create_objectrefnode(*objp);
    if (p->rtype->type)
        a->rtype = p->rtype;
    else
        a->rtype = p->data.diadic.left->rtype;

    b = (Statement *)lalloc(sizeof(Statement));
    memset(b, 0, sizeof(Statement));
    b->type = ST_EXPRESSION;

    b->expr = IrOptimizer_NewENode(EASS);
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

static void append_land_test_statement(ENode *node, Object **temporary, CLabel **label)
{
    Statement *statement;

    *label = IroUtil_NewLabel();
    statement = NewStmt(ST_IFNGOTO);
    statement->expr = node->data.diadic.left;
    statement->label = *label;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

void fn_00430820(ENode *e, Object **pp, CLabel **lab)
{
    Statement *s;
    ENode *ref;
    ENode *c;
    Statement *a;
    Statement *g;

    s = NewStmt(7);
    s->expr = e->data.diadic.right;
    s->label = *lab;
    s->dobjstack = current_optimizer_statement->dobjstack;
    s->sourceoffset = current_optimizer_statement->sourceoffset;
    s->value = current_optimizer_statement->value;
    s->flags = current_optimizer_statement->flags;
    s->next = statement_insertion_point->next;
    statement_insertion_point->next = s;
    statement_insertion_point = s;

    ref = IrOptimizer_NewENode(EINDIRECT);
    ref->data.monadic = create_objectrefnode(*pp);
    if (e->rtype->type != TYPEVOID)
        ref->rtype = e->rtype;
    else
        CError_FATAL(1170);

    c = IrOptimizer_NewENode(EINTCONST);
    c->data.intval = cint64_one;
    c->rtype = e->rtype;

    a = NewStmt(4);
    a->expr = IrOptimizer_NewENode(EASS);
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

    g = NewStmt(2);
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

static void append_lor_test_statement(ENode *node, Object **temporary, CLabel **label)
{
    Statement *statement;

    *label = IroUtil_NewLabel();
    statement = NewStmt(ST_IFGOTO);
    statement->expr = node->data.diadic.left;
    statement->label = *label;
    statement->dobjstack = current_optimizer_statement->dobjstack;
    statement->sourceoffset = current_optimizer_statement->sourceoffset;
    statement->value = current_optimizer_statement->value;
    statement->flags = current_optimizer_statement->flags;
    statement->next = statement_insertion_point->next;
    statement_insertion_point->next = statement;
    statement_insertion_point = statement;
}

void fn_004305e0(ENode *e, Object **pp, CLabel **lab)
{
    Statement *s;
    ENode *ref;
    ENode *c;
    Statement *a;
    Statement *g;

    s = NewStmt(6);
    s->expr = e->data.diadic.right;
    s->label = *lab;
    s->dobjstack = current_optimizer_statement->dobjstack;
    s->sourceoffset = current_optimizer_statement->sourceoffset;
    s->value = current_optimizer_statement->value;
    s->flags = current_optimizer_statement->flags;
    s->next = statement_insertion_point->next;
    statement_insertion_point->next = s;
    statement_insertion_point = s;

    ref = IrOptimizer_NewENode(EINDIRECT);
    ref->data.monadic = create_objectrefnode(*pp);
    if (e->rtype->type != TYPEVOID)
        ref->rtype = e->rtype;
    else
        CError_FATAL(1286);

    c = IrOptimizer_NewENode(EINTCONST);
    c->data.intval = cint64_zero;
    c->rtype = e->rtype;

    a = NewStmt(4);
    a->expr = IrOptimizer_NewENode(EASS);
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

    g = NewStmt(2);
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

void lower_expression_to_statements(ENode *node, int valueNeeded, int force)
{
    UInt8 type = node->type;
    Object *forceLoadTemp;
    CLabel *andEndLabel;
    Object *andResult;
    CLabel *orEndLabel;
    Object *orResult;
    CLabel *elseLabel;
    CLabel *endLabel;
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
        case EBTST:
            if (node->type == ECOMMA && data_00587f08 == 0) {
                lower_expression_to_statements(node->data.diadic.left, 0, 1);
                append_comma_left_statement(node);
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
                create_zero_initialized_temp_object(node, &andResult);
                lower_expression_to_statements(node->data.diadic.left, 1, 0);
                append_land_test_statement(node, &andResult, &andEndLabel);
                lower_expression_to_statements(node->data.diadic.right, 1, 0);
                fn_00430820(node, &andResult, &andEndLabel);
            } else if (node->type == ELOR) {
                insert_intconst_assignment(node, &orResult);
                lower_expression_to_statements(node->data.diadic.left, 1, 0);
                append_lor_test_statement(node, &orResult, &orEndLabel);
                lower_expression_to_statements(node->data.diadic.right, 1, 0);
                fn_004305e0(node, &orResult, &orEndLabel);
            }
            break;

        case ECOND:
            lower_expression_to_statements(node->data.cond.cond, 1, 0);
            append_cond_test_statement(node, &elseLabel);
            lower_expression_to_statements(node->data.cond.expr1, 1, 0);
            fn_00430e60(node, &conditionalResult);
            append_goto_statement(node, &endLabel);
            append_label_statement(&elseLabel);
            lower_expression_to_statements(node->data.cond.expr2, 1, 0);
            append_cond_expr2_statement(node, &conditionalResult);
            append_label_statement(&endLabel);
            if (node->rtype->type != TYPEVOID) {
                node->type = EINDIRECT;
                node->data.cond.cond = create_objectrefnode(conditionalResult);
                CError_ASSERT(1495, node->rtype->type != TYPEVOID);
            } else {
                node->type = EINTCONST;
                node->data.intval = cint64_zero;
            }
            break;

        case EPRECOMP: {
            Object *temporary = LookupTemporary(node);
            node->type = EINDIRECT;
            node->data.monadic = create_objectrefnode(temporary);
            CError_ASSERT(1513, node->rtype->type != TYPEVOID);
            break;
        }

        case ENULLCHECK:
            lower_expression_to_statements(node->data.diadic.left, 1, 0);
            create_temp_object_assignment(node, &nullCheckTemp);
            insert_indirect_statement_with_label(node, &nullCheckTemp, &nullCheckEndLabel);
            lower_expression_to_statements(node->data.diadic.right, 1, 0);
            fn_00430a60(node, &nullCheckTemp);
            append_label_statement(&nullCheckEndLabel);
            if (node->rtype->type != TYPEVOID) {
                node->type = EINDIRECT;
                node->data.diadic.left = create_objectrefnode(nullCheckTemp);
                CError_ASSERT(1538, node->rtype->type != TYPEVOID);
            } else {
                node->type = EINTCONST;
                node->data.intval = cint64_zero;
            }
            break;

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
                arguments = (ENode **)oalloc(argumentCount * sizeof(*arguments));
                argument = node->data.funccall.args;
                filledCount = 0;
                while (argument != NULL) {
                    arguments[filledCount++] = argument->node;
                    argument = argument->next;
                }
                argumentOrder = (SInt16 *)oalloc(filledCount * sizeof(*argumentOrder));
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

struct IROLinear *linearize_expression(ENode *expression)
{
    ENodeList *cursor;
    SInt32 argumentCount;
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
        case EBITFIELD:
            result = IrOptimizer_NewLinear(2);
            result->u.diadic.left = linearize_expression(expression->data.monadic);
            result->nodetype = expression->type;
            result->rtype = expression->rtype;
            result->nodeflags = expression->flags;
            if (data_00551d6c[result->nodetype] != 0)
                set_monadic_addr_flags(result->u.diadic.left, 1);
            if (result->nodetype == EINDIRECT) {
                mark_address(result->u.diadic.left);
                result->u.diadic.left->flags |= (IROLF_Ind | IROLF_Immind);
                if (result->u.diadic.left->type == IROLinearOp2Arg && result->u.diadic.left->nodetype == EADD)
                    fn_004315b0(result->u.diadic.left);
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
        case EBTST:
            result = IrOptimizer_NewLinear(3);
            result->nodeflags = expression->flags;
            if (expression->type != ECOMMA &&
                expression->data.diadic.right->cost >= expression->data.diadic.left->cost) {
                result->u.diadic.right = linearize_expression(expression->data.diadic.right);
                result->u.diadic.left = linearize_expression(expression->data.diadic.left);
                result->flags |= 0x8000;
            } else {
                result->u.diadic.left = linearize_expression(expression->data.diadic.left);
                result->u.diadic.right = linearize_expression(expression->data.diadic.right);
            }
            result->nodetype = expression->type;
            result->rtype = expression->rtype;
            if (data_00551d6c[result->nodetype] != 0)
                set_monadic_addr_flags(result->u.diadic.left, monadic_addr_flags_by_nodetype[result->nodetype]);
            break;

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
            result = IrOptimizer_NewLinear(3);
            result->nodeflags = expression->flags;
            result->u.diadic.right = linearize_expression(expression->data.diadic.right);
            result->u.diadic.left = linearize_expression(expression->data.diadic.left);
            result->flags |= 0x8000;
            result->nodetype = expression->type;
            result->rtype = expression->rtype;
            set_monadic_addr_flags(result->u.diadic.left, monadic_addr_flags_by_nodetype[result->nodetype]);
            break;

        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case EVECTOR128CONST:
            result = IrOptimizer_NewLinear(1);
            result->nodeflags = expression->flags;
            result->u.node = expression;
            result->rtype = expression->rtype;
            break;

        case EFUNCCALL:
        case EFUNCCALLP:
            result = IrOptimizer_NewLinear(10);
            result->nodeflags = expression->flags;
            result->u.funccall.ispure = (expression->type == EFUNCCALLP);
            argumentCount = 0;
            for (cursor = expression->data.funccall.args; cursor != NULL; cursor = cursor->next)
                argumentCount++;
            arguments = NULL;
            if (argumentCount != 0) {
                arguments = (IROLinear **)oalloc(argumentCount * sizeof(IROLinear *));
                cursor = expression->data.funccall.args;
                for (argumentCount = 0; cursor != NULL; cursor = cursor->next)
                    arguments[argumentCount++] = (IROLinear *)cursor->node;
                argumentOrder = (SInt16 *)oalloc(argumentCount * sizeof(SInt16));
                for (argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
                    argumentOrder[argumentIndex] = argumentCount - argumentIndex - 1;
                for (argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++) {
                    arguments[argumentOrder[argumentIndex]] =
                        linearize_expression((ENode *)arguments[argumentOrder[argumentIndex]]);
                    mark_node_and_operand_flags(arguments[argumentOrder[argumentIndex]], 1);
                }
            }
            result->u.funccall.argCount = argumentCount;
            result->u.funccall.args = arguments;
            result->u.funccall.callee = linearize_expression(expression->data.funccall.funcref);
            result->u.funccall.functype = expression->data.funccall.functype;
            result->rtype = expression->rtype;
            break;

        default:
            CError_FATAL(1850);
            break;
    }

    if (result != NULL) {
        append_linear(result);
    }
    return result;
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

static inline void mark_linear_inputs(IROLinear *p)
{
    int i;

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

void mark_referenced_linear_nodes(void)
{
    IROLinear *p;

    for (p = linear_head; p != NULL; p = p->next) {
        p->flags &= ~2u;
    }

    for (p = linear_head; p != NULL; p = p->next)
        mark_linear_inputs(p);
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
                insn = IrOptimizer_NewLinear(IROLinearNop);
                break;
            case ST_LABEL:
                insn = IrOptimizer_NewLinear(IROLinearLabel);
                insn->u.label = node->label;
                insn->flags |= 1;
                break;
            case ST_GOTO:
                insn = IrOptimizer_NewLinear(IROLinearGoto);
                insn->u.label = node->label;
                break;
            case ST_EXPRESSION:
                linearize_expression(node->expr);
                break;
            case ST_SWITCH:
                insn = IrOptimizer_NewLinear(IROLinearSwitch);
                insn->u.swtch.cond = linearize_expression(node->expr);
                insn->u.swtch.info = (SwitchInfo *)node->label;
                break;
            case ST_IFGOTO:
                insn = IrOptimizer_NewLinear(IROLinearIf);
                insn->u.branch.cond = linearize_expression(node->expr);
                insn->u.branch.label = node->label;
                break;
            case ST_IFNGOTO:
                insn = IrOptimizer_NewLinear(IROLinearIfNot);
                insn->u.branch.cond = linearize_expression(node->expr);
                insn->u.branch.label = node->label;
                break;
            case ST_RETURN:
                data_00588526 = 1;
                insn = IrOptimizer_NewLinear(IROLinearReturn);
                if (node->expr != NULL)
                    insn->u.monadic = linearize_expression(node->expr);
                else
                    insn->u.monadic = NULL;
                break;
            case ST_OVF:
                CError_FATAL(2049);
                break;
            case ST_EXIT:
                insn = IrOptimizer_NewLinear(IROLinearExit);
                insn->u.label = node->label;
                break;
            case ST_ENTRY:
                insn = IrOptimizer_NewLinear(IROLinearEntry);
                insn->u.label = node->label;
                insn->flags |= 1;
                break;
            case ST_BEGINCATCH:
                insn = IrOptimizer_NewLinear(IROLinearBeginCatch);
                insn->u.args3.a = linearize_expression(node->expr);
                insn->u.args3.b = NULL;
                insn->u.args3.c = NULL;
                break;
            case ST_ENDCATCH:
                insn = IrOptimizer_NewLinear(IROLinearEndCatch);
                insn->u.monadic = linearize_expression(node->expr);
                break;
            case ST_ENDCATCHDTOR:
                insn = IrOptimizer_NewLinear(IROLinearEndCatchDtor);
                insn->u.monadic = linearize_expression(node->expr);
                break;
            case ST_ASM:
                insn = IrOptimizer_NewLinear(IROLinearAsm);
                insn->u.asm_stmt = node;
                if (copts.fc3 > 0) {
                    fn_00462d70(node, &usageInfo);
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
        if (insn != NULL)
            append_linear(insn);
        node = node->next;
    }
    insn = IrOptimizer_NewLinear(IROLinearEnd);
    insn->flags |= 1;
    append_linear(insn);

    mark_referenced_linear_nodes();
    IroVars_CheckTimedLongjmp();
}

static Statement *NewIrNode(IROLinear *src, UInt8 type)
{
    Statement *n;

    n = (Statement *)lalloc(sizeof(Statement));
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
ENode *IrOptimizer_NewENode(UInt8 type)
{
    ENode *node;
    node = (ENode *)lalloc(sizeof(ENode));
    memset(node, 0, sizeof(ENode));
    node->type = (UInt8)type;
    return node;
}

ENode *fn_0042eb40(IROLinear *e)
{
    ENode *p;
    ENodeList *l;
    SInt32 i;

    switch (e->type) {
        case IROLinearOperand:
            p = IrOptimizer_NewENode(e->u.node->type);
            p->flags = e->nodeflags;
            *p = *e->u.node;
            break;
        case IROLinearOp1Arg:
            p = IrOptimizer_NewENode(e->nodetype);
            p->flags = e->nodeflags;
            p->data.monadic = fn_0042eb40(e->u.monadic);
            p->rtype = e->rtype;
            p->cost = p->data.monadic->cost;
            if (p->cost == 0)
                p->cost = 1;
            break;
        case IROLinearOp2Arg:
            p = IrOptimizer_NewENode(e->nodetype);
            p->flags = e->nodeflags;
            p->data.diadic.left = fn_0042eb40(e->u.diadic.left);
            p->data.diadic.right = fn_0042eb40(e->u.diadic.right);
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
            p = IrOptimizer_NewENode(ty);
            p->flags = e->nodeflags;
            p->data.funccall.funcref = fn_0042eb40(e->u.funccall.callee);
            p->data.funccall.functype = e->u.funccall.functype;
            p->data.funccall.args = NULL;
            p->cost = 200;
            {
                SInt32 n = e->u.funccall.argCount - 1;
                i = n;
                if (n >= 0) {
                    do {
                        l = (ENodeList *)lalloc(sizeof(ENodeList));
                        l->node = fn_0042eb40(e->u.funccall.args[i]);
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

Statement *convert_linear_to_statements(void)
{
    IROLinear *item;
    Statement *node;
    SInt32 depth;
    Statement *first;
    Statement *last;
    IRONode *block;
    first = last = NULL;

    mark_referenced_linear_nodes();

    for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
        for (item = block->first; item != NULL; item = item->next) {
            node = NULL;
            if ((item->flags & IROLF_Reffed) == 0) {
                switch (item->type) {
                    case IROLinearNop:
                        if (block == iro_flowgraph_head) {
                            node = NewIrNode(item, 1);
                        } else {
                            node = NULL;
                        }
                        break;
                    case IROLinearOperand:
                    case IROLinearOp1Arg:
                    case IROLinearOp2Arg:
                    case IROLinearFunccall:
                        node = NewIrNode(item, 4);
                        node->expr = fn_0042eb40(item);
                        break;
                    case IROLinearGoto:
                        node = NewIrNode(item, 3);
                        node->label = item->u.label;
                        break;
                    case IROLinearExit:
                        node = NewIrNode(item, 0x0a);
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
                        node = NewIrNode(item, nodeType);
                        node->label = item->u.branch.label;
                        node->expr = fn_0042eb40(item->u.branch.cond);
                        break;
                    }
                    case IROLinearReturn:
                        node = NewIrNode(item, 8);
                        if (item->u.monadic != NULL) {
                            node->expr = fn_0042eb40(item->u.monadic);
                        }
                        break;
                    case IROLinearLabel:
                        node = NewIrNode(item, 2);
                        node->label = item->u.label;
                        node->label->stmt = node;
                        break;
                    case IROLinearEntry:
                        node = NewIrNode(item, 0x0b);
                        node->label = item->u.label;
                        node->label->stmt = node;
                        break;
                    case IROLinearSwitch:
                        node = NewIrNode(item, 5);
                        node->expr = fn_0042eb40(item->u.swtch.cond);
                        /* a switch statement's label slot holds its SwitchInfo */
                        node->label = (CLabel *)item->u.swtch.info;
                        break;
                    case IROLinearBeginCatch:
                        node = NewIrNode(item, 0x0c);
                        node->expr = fn_0042eb40(item->u.monadic);
                        break;
                    case IROLinearEndCatch:
                        node = NewIrNode(item, 0x0d);
                        node->expr = fn_0042eb40(item->u.monadic);
                        break;
                    case IROLinearEndCatchDtor:
                        node = NewIrNode(item, 0x0e);
                        node->expr = fn_0042eb40(item->u.monadic);
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
                    if (first != NULL) {
                        last->next = node;
                    } else {
                        first = node;
                    }
                    last = node;
                }
            }
            if (item == block->last) {
                break;
            }
        }
    }
    return first;
}

/* The weight of a use at each loop depth. */
static UInt16 ir_size_table[4] = {1, 4, 16, 64};

static void AddLocalUsage(Object *object, int weightIndex)
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

void fn_0042e200(IROLinear *node, int level)
{
    if (IroUtil_GetObjRef(node)) {
        add_local_usage_and_set_noregister(node->u.monadic, level);
    } else if (node->type == IROLinearOp2Arg) {
        if (node->nodetype == EADD) {
            fn_0042e200(node->u.diadic.left, level);
            fn_0042e200(node->u.diadic.right, level);
        } else if (return_zero_for_irolinear(node) && IroUtil_GetObjRef(node->u.diadic.left)) {
            add_local_usage_and_set_noregister(node->u.diadic.left->u.monadic, level);
        }
    }
}

void record_object_usage(void)
{
    IRONode *obj;
    IROLinear *node;
    IROLinear *obj2;
    SInt32 i;
    AsmOut list;
    Object *o_s;

    obj = iro_flowgraph_head;
    if (obj != NULL) {
        for (; obj != NULL; obj = obj->nextnode) {
            for (node = obj->first; node != NULL; node = node->next) {
                if (node->type == IROLinearOperand && node->u.node->type == EOBJREF) {
                    add_local_usage_and_set_noregister(node, obj->loopdepth);
                } else if (node->type == IROLinearOp1Arg && node->nodetype == EINDIRECT) {
                    fn_0042e200(node->u.monadic, obj->loopdepth);
                }
                if (node->type == IROLinearAsm) {
                    fn_00462d70(node->u.asm_stmt, &list);
                    for (i = 0; i < list.numoperands; i++) {
                        o_s = list.operands[i].object;
                        if (o_s->datatype == DLOCAL && o_s->u.var.info != NULL) {
                            AddLocalUsage(o_s, obj->loopdepth);
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
                add_local_usage_and_set_noregister(obj2, 0);
            } else if (obj2->type == IROLinearOp1Arg && obj2->nodetype == EINDIRECT) {
                fn_0042e200(obj2->u.monadic, 0);
            }
        }
    }
    IroVars_CheckTimedLongjmp();
}

static void renumber_linear_nodes(void)
{
    IROLinear *node;

    node = linear_head;
    linear_index_counter = 0;
    if (linear_head != NULL) {
        do {
            node->index = linear_index_counter;
            linear_index_counter++;
        } while ((node = node->next) != NULL);
    }
}
#if VERSION == VERSION_GC_1_0
static int IR_Replace(IROLinear **fieldp, ENode *obj, IROLinear *newval, Boolean doReplace, IROLinear *f8)
{
    IROLinear *a;
    ENode *r;
    IROLinear *t;
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
#define IR_REPLACE(replace, field) IR_Replace(field, obj, newval, doReplace, ctx->linear)
#endif

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
                            count += IR_REPLACE(IR_Replace, &q->u.monadic);
                        break;
                    case IROLinearEndCatch:
                    case IROLinearEndCatchDtor:
                        count += IR_REPLACE(IR_Replace, &q->u.monadic);
                        break;
                    case IROLinearOp2Arg:
                        if (monadic_addr_flags_by_nodetype[q->nodetype] == 0)
                            count += IR_REPLACE(IR_Replace, &q->u.monadic);
                        count += IR_REPLACE(IR_Replace, &q->u.diadic.right);
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot:
                        count += IR_REPLACE(IR_Replace, &q->u.diadic.right);
                        count += IR_REPLACE(IR_Replace, &q->u.args3.c);
                        break;
                    case IROLinearReturn:
                        count += IR_REPLACE(IR_Replace, &q->u.monadic);
                        break;
                    case IROLinearSwitch:
                        count += IR_REPLACE(IR_Replace, &q->u.diadic.right);
                        break;
                    case IROLinearFunccall:
                        count += IR_REPLACE(IR_ReplaceCall, &q->u.funccall.callee);
                        for (i = 0; i < q->u.funccall.argCount; i++)
                            count += IR_REPLACE(IR_ReplaceArray, &q->u.funccall.args[i]);
                        break;
                    case IROLinearBeginCatch:
                        count += IR_REPLACE(IR_Replace, &q->u.monadic);
                        count += IR_REPLACE(IR_Replace, &q->u.diadic.right);
                        count += IR_REPLACE(IR_Replace, &q->u.args3.c);
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

Boolean fn_0042d2c0(IROLinear *e)
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
            return fn_0042d2c0(e->u.monadic);
        case IROLinearIf:
        case IROLinearIfNot:
            return fn_0042d2c0(e->u.args3.b) || fn_0042d2c0(e->u.args3.c);
        case IROLinearReturn:
            return fn_0042d2c0(e->u.monadic);
        case IROLinearSwitch:
            return fn_0042d2c0(e->u.args3.b);
        case IROLinearBeginCatch:
            return fn_0042d2c0(e->u.monadic) || fn_0042d2c0(e->u.args3.b) || fn_0042d2c0(e->u.args3.c);
        default:
            return 0;
    }
}

/* The linker stripped the function that used this literal; it stays in the unit's .data. */
static void IrOptimizer_StrippedLiterals(const char **literals)
{
    literals[0] = "BitVector.h";
}

static char lbl_0054ea30[12] = {0};

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
        IroTransform_RewriteStatementExpressions(body);

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
                renumber_linear_nodes();
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
                    renumber_linear_nodes();
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

    renumber_linear_nodes();
    IroDump_DumpFunction("before RewriteBitFieldTemps", 0);
    RewriteBitFieldTemps();
    IroDump_DumpFunction("After RewriteBitFieldTemps", 0);
    record_object_usage();
    IroDump_DumpFunction("After IRO_Optimizer", 0);
    incomingBody = convert_linear_to_statements();
    IroVars_ClearObjectVarRecords();
    freeoheap();
    return incomingBody;
}

void trav_expr_to_update_flags(IROLinear *expression, unsigned int flag)
{
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
                mark_address(expression->u.diadic.left);
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

void IRO_ExpressionPropagation(void)
{
    IROLinear *e;
    IROLinear *n;
    int i;
    IRONode *node;

    mark_referenced_linear_nodes();

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

void IrOptimizer_SetDeleteDeadInstructionsFlags(void)
{
    copts.deadcode = copts.deleteDeadInstructions >= 1;
    copts.commonsubs = copts.propagation = copts.deleteDeadInstructions >= 2;
    copts.loopinvariants = copts.strengthreduction = copts.lifetimes = copts.deadstore = copts.unrolling =
        copts.vectorizeloops = copts.deleteDeadInstructions >= 3;
    copts.irSecondOptimizationPass = copts.deleteDeadInstructions >= 4;
}
