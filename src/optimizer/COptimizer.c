#define CERROR_FILE "COptimizer.c"
#include "compiler/common.h"
#include "compiler/COptimizer.h"
#include "compiler/Switch.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/DumpIR.h"
#include "compiler/IROUseDef.h"
#include "compiler/IrOptimizer.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>
#include "compiler/ENode.h"
#include "compiler/Types.h"

#include <setjmp.h>
/* Declarations gathered from the merged files. */

static const SInt16 bit_masks[16] = {
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
};

#pragma options align = mac68k
static Boolean optimizer_changed;
static struct COptBlock *current_opt_block;
static struct ENode *current_cse_expr;
static SInt16 opt_block_bits_size;
static struct COptCSE *cse_entries[75];
static struct OptimizerOccurrence *occurrence_list;
static short data_005812fc;
static char data_005812fe;
static char data_005812ff;
static UInt8 data_00581300;
static short data_00581302;
static struct ENode *last_node;
static int data_00581308;
#pragma options align = reset

static inline void set_statement_location(Statement *statement)
{
    current_statement_number = (UInt16)statement->value; /* set_statement_location: unsigned source-location value */
}

static inline void optimize_expression(Statement *statement)
{
    set_statement_location(statement);
    COptimizer_CountExpressionObjectUses(statement->expr);
}

Statement *DumpIR_OptimizeStatements(Object *object, Statement *statements)
{
    Statement *statement;

    data_00588513 = 1;
    if (copts.globaloptimizer)
        statements = IRO_Optimizer(object, statements);
    data_00581300 = 0;
    COptimizer_OptimizeStatementList(object, statements);
    if (object && !(object->qual & Q_INLINE))
        COptimizer_CheckStmtsForNonVoidFunction(object, statements);
    for (statement = statements->next; statement; statement = statement->next) {
        if (statement->type >= ST_EXPRESSION && statement->type <= 15 && statement->expr) {
            optimize_expression(statement);
        } else if (statement->type == ST_ASM) {
            set_statement_location(statement);
            InlineAsm_RecordObjectUses(statement);
        }
    }
    return statements;
}

void DumpIR_OptimizeStatementList(Object *object, Statement *statements)
{
    Statement *statement;

    statement = statements;
    if (statements != NULL) {
        do {
            if ((ST_EXPRESSION <= statement->type) && (statement->type <= 0xf)) {
                if (statement->expr != NULL) {
                    CExpr_SearchExprTree(statement->expr, set_label_stmt_flag, 1, 0x3f);
                }
            }
            statement = statement->next;
        } while (statement != NULL);
    }
    data_00581300 = 1;
    data_0058802c = NULL;
    COptimizer_OptimizeStatementList(object, statements);
    COptimizer_CheckStmtsForNonVoidFunction(object, statements);
}

void set_label_stmt_flag(ENode *a)
{
    Statement *statement;
    CLabel *label;
    label = (CLabel *)a->data.longval;
    if ((statement = label->stmt) == NULL || statement->type != ST_LABEL)
        CError_FATAL(2133);
    label = (CLabel *)a->data.longval;
    statement = label->stmt;
    statement->flags |= 1;
}

static void CheckStmts(Statement *p)
{
    if (p) {
        do {
            if (p->next == NULL && p->type != ST_GOTO && p->type != ST_RETURN) {
                CError_Warning(ERR_RETURN_VALUE_EXPECTED);
                break;
            }
            if (p->type == ST_RETURN && p->expr == NULL && !(p->flags & 8)) {
                CError_Warning(ERR_RETURN_VALUE_EXPECTED);
                break;
            }
            p = p->next;
        } while (p != NULL);
    }
}

static CLabel *COpt_Follow(Statement *self, CLabel *node)
{
    Statement *q;

    for (q = node->stmt; q != NULL; q = q->next) {
        if (q->type > 2) {
            if (q != self && q->type == ST_GOTO) {
                if (q->label != node)
                    optimizer_changed = 1;
                return q->label;
            }
            return node;
        }
    }
    return node;
}

static inline void COpt_Pass(Statement *stmt)
{
    Statement *p;
    Statement *u;

    remove_unreferenced_labels(stmt);
    for (p = stmt; p != NULL; p = p->next) {
        switch (p->type) {
            case ST_GOTO:
                for (u = p->next; u != NULL; u = u->next) {
                    if (p->label->stmt == u) {
                        p->type = ST_NOP;
                        optimizer_changed = 1;
                        goto next;
                    }
                    if (u->type > 2)
                        break;
                }
                p->label = COpt_Follow(p, p->label);
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                fold_and_invert_conditional_branch(p);
                break;
            case ST_SWITCH:
                follow_switch_labels_and_fold_constant(p);
                break;
        }
    next:;
    }
}

void COptimizer_OptimizeStatementList(Object *unused, Statement *list)
{
    Statement *p;
    Statement **pp;

    do {
        optimizer_changed = 0;
        COpt_Pass(list->next);
        eliminate_unreachable_statements(list->next);
    } while (optimizer_changed);

    pp = &list->next;
    while ((p = *pp) != NULL) {
        if (p->type == ST_NOP)
            *pp = p->next;
        else
            pp = &p->next;
    }
}

void COptimizer_CheckStmtsForNonVoidFunction(Object *func, Statement *stmt)
{
    if ((copts.extended_errorcheck || copts.cplusplus) && func != NULL && TYPE_FUNC(func->type)->functype != &stvoid)
        CheckStmts(stmt);
}

static inline void COptimizer_SimplifyBranch(Statement *stmt)
{
    Statement *node;
    for (node = stmt->next; node != NULL; node = node->next) {
        if (stmt->label->stmt == node) {
            stmt->type = ST_NOP;
            optimizer_changed = 1;
            return;
        }
        if (node->type > 2)
            break;
    }
    stmt->label = COpt_Follow(stmt, stmt->label);
}

static inline CLabel *COFindOwner(Statement *s)
{
    Statement *q;
    UInt8 kind;
    SInt32 saved = (SInt32)s->label;
    CLabel *owner = (CLabel *)saved;
    for (q = ((CLabel *)saved)->stmt; q != NULL; q = q->next) {
        if ((kind = q->type) <= 2)
            continue;
        if (q != s && kind == 3) {
            if (q->label != owner)
                optimizer_changed = 1;
            return q->label;
        } else {
            return owner;
        }
    }
    return owner;
}

static inline CLabel *COFindOwnerPlain(Statement *s)
{
    SInt32 saved = (SInt32)s->label;
    CLabel *owner = (CLabel *)saved;
    Statement *q;
    for (q = ((CLabel *)saved)->stmt; q != NULL; q = q->next) {
        if (q->type <= 2)
            continue;
        if (q != s && q->type == ST_GOTO) {
            if (q->label != owner)
                optimizer_changed = 1;
            return q->label;
        } else {
            return owner;
        }
    }
    return owner;
}

static inline void clear_words(COptBlock *p, SInt16 i)
{
    SInt16 *lo, *hi;
    lo = p->referenceBits;
    lo[i] = 0;
    (hi = p->referenceBarrierBits)[i] = 0;
}

static inline COptBlock *new_block(void)
{
    COptBlock *p;
    SInt16 i, n;

    p = (COptBlock *)lalloc(opt_block_bits_size * 2 + 0x20);
    p->flag = 0;
    p->next = NULL;
    p->pred = NULL;
    p->succ = NULL;
    p->unused = 0;
    p->referenceBits = (SInt16 *)(p + 1);
    p->referenceBarrierBits = (SInt16 *)((UInt8 *)p + opt_block_bits_size + 0x20);
    i = 0;
    n = opt_block_bits_size / 2;
    for (; i < n; i++) {
        clear_words(p, i);
    }
    return p;
}

static inline void add_succ(COptBlock *b, CLabel *owner)
{
    COptBlockLink *e;

    e = (COptBlockLink *)lalloc(8);
    e->next = b->succ;
    b->succ = e;
    e->target.statement = owner->stmt;
}

static inline void clear_words2(COptBlock *p, SInt16 i)
{
    SInt16 *lo, *hi;
    lo = p->referenceBits;
    lo[i] = 0;
    hi = p->referenceBarrierBits;
    hi[i] = 0;
}

static inline COptBlock *new_block2(void)
{
    COptBlock *p;
    SInt16 i, n;
    p = (COptBlock *)lalloc(opt_block_bits_size * 2 + 0x20);
    p->flag = 0;
    p->next = NULL;
    p->pred = NULL;
    p->succ = NULL;
    p->unused = 0;
    p->referenceBits = (SInt16 *)(p + 1);
    p->referenceBarrierBits = (SInt16 *)((UInt8 *)p + opt_block_bits_size + 0x20);
    i = 0;
    n = opt_block_bits_size / 2;
    for (; i < n; i++) {
        clear_words2(p, i);
    }
    return p;
}

static SInt16 TestBit_4bfa30(SInt16 *vec, SInt16 bit)
{
    return vec[bit >> 4] & bit_masks[bit & 0xf];
}

static void SetBit_4bfa30(SInt16 *vec, SInt16 bit)
{
    vec[bit >> 4] |= bit_masks[bit & 0xf];
}

static inline short tbit(short *p, int w, int b)
{
    return bit_masks[b] & p[w];
}

static inline void sbit(short *p, int w, int b)
{
    p[w] |= bit_masks[b];
}

static SInt16 TestBit(SInt16 *vec, SInt16 bit)
{
    return vec[bit >> 4] & bit_masks[bit & 0xf];
}

static void SetBit(SInt16 *vec, SInt16 bit)
{
    vec[bit >> 4] |= bit_masks[bit & 0xf];
}

static COptCSE *COpt_NewCSE(ENode *expr)
{
    COptCSE *cse;

    (cse = (COptCSE *)oalloc(30))->expr = expr;
    cse->replacement = NULL;
    cse->block = current_opt_block;
    cse->last = current_cse_expr;
    cse->left = NULL;
    cse->right = NULL;
    cse->uses = 1;
    return cse;
}

/* Records an occurrence of a common subexpression: the expression and the CSE it computes. */
static COptCSE *COpt_AddOccurrence(ENode *expr, COptCSE *cse)
{
    OptimizerOccurrence *occ = (OptimizerOccurrence *)oalloc(12);

    occ->next = occurrence_list;
    occurrence_list = occ;
    occ->group = cse;
    occ->expression = expr;
    return cse;
}

static COptCSE *COpt_IntConst(ENode *expr)
{
    COptCSE *cse;
    ENode *node;

    for (cse = cse_entries[EINTCONST]; cse; cse = cse->next) {
        if (expr->rtype == (node = cse->expr)->rtype && CInt64_Equal(node->data.intval, expr->data.intval))
            return cse;
    }
    cse = COpt_NewCSE(expr);
    cse->next = cse_entries[EINTCONST];
    cse_entries[EINTCONST] = cse;
    return cse;
}

static COptCSE *COpt_FloatConst(ENode *expr)
{
    COptCSE *cse;
    ENode *node;
    Float value;

    for (cse = cse_entries[EFLOATCONST], value = expr->data.floatval; cse; cse = cse->next) {
        node = cse->expr;
        if (CMach_CalcFloatDiadicBool(node->rtype, node->data.floatval.data.value, 360, value.data.value) &&
            expr->rtype == cse->expr->rtype)
            return cse;
    }
    cse = COpt_NewCSE(expr);
    cse->next = cse_entries[EFLOATCONST];
    cse_entries[EFLOATCONST] = cse;
    return cse;
}

static inline COptCSE *COpt_VectorConst(ENode *expr)
{
    COptCSE *cse;
    MWVector128 value;

    for (cse = cse_entries[EVECTOR128CONST], value = expr->data.vector128val; cse; cse = cse->next) {
        if (CMach_CalcVectorDiadicBool((unsigned int)cse->expr->rtype, &cse->expr->data.vector128val, 360, &value) &&
            expr->rtype == cse->expr->rtype)
            return cse;
    }
    cse = (COptCSE *)oalloc(30);
    cse->expr = expr;
    cse->replacement = NULL;
    cse->block = current_opt_block;
    cse->last = current_cse_expr;
    cse->left = NULL;
    cse->right = NULL;
    cse->uses = 1;
    cse->next = cse_entries[EVECTOR128CONST];
    cse_entries[EVECTOR128CONST] = cse;
    return cse;
}

static COptCSE *COpt_ObjectRef(ENode *expr, COptCSE *cse)
{
    Object *obj = expr->data.objref;

    for (; cse; cse = cse->next) {
        if (cse->expr->data.objref == obj)
            return cse;
    }
    cse = COpt_NewCSE(expr);
    cse->next = cse_entries[EOBJREF];
    cse_entries[EOBJREF] = cse;
    return cse;
}

static void COpt_IncDecTarget(ENode *expr)
{
    ENode *target = expr->data.monadic;

    collect_expr_cse(target->data.monadic);
    invalidate_expr_cse(expr->data.monadic);
}

static inline void clearEntries(int count)
{
    int index;
    for (index = 0; (short)index < count; index++)
        cse_entries[(short)index] = NULL;
}

void eliminate_unreachable_statements(Statement *items)
{
    Statement *item;
    for (item = items; item; item = item->next)
        item->marked = 0;

    mark_reachable_statements(items);

    for (item = items; item; item = item->next) {
        if (!item->marked && (item->flags & 1))
            mark_reachable_statements(item);
    }

    for (item = items; item; item = item->next) {
        if (!item->marked && item->type != ST_NOP) {
            item->type = ST_NOP;
            optimizer_changed = 1;
        }
    }
}

void mark_reachable_statements(Statement *input)
{
    Statement *node = input;
    SwitchCase *element;
    CLabel *sub;

    while (node != NULL && node->marked == 0) {
        node->marked = 1;
        switch (node->type) {
            case ST_GOTOEXPR:
                return;
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_OVF:
                mark_reachable_statements(node->label->stmt);
                break;
            case ST_GOTO:
            case ST_EXIT:
                node = node->label->stmt;
                continue;
            case ST_RETURN:
                return;
            case ST_SWITCH:
                for (element = ((SwitchInfo *)node->label)->cases; element != NULL; element = element->next)
                    mark_reachable_statements(element->label->stmt);
                node = ((SwitchInfo *)node->label)->defaultlabel->stmt;
                continue;
            case ST_ASM:
                sub = InlineAsm_FindOperandLabel(node);
                if (sub != NULL)
                    mark_reachable_statements(sub->stmt);
                sub = InlineAsm_GetOperandLabel(node);
                if (sub != NULL)
                    mark_reachable_statements(sub->stmt);
                break;
            case ST_NOP:
            case ST_LABEL:
            case ST_EXPRESSION:
            case ST_ENTRY:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
                break;
            default:
                CError_FATAL(2037);
        }
        node = node->next;
    }
}

/* COptimizer_CountExpressionObjectUses inlines this; COptimizer_RecordObjectUse, defined after it, has the same body. */
static inline void RecordObjectUse(Object *object, unsigned char direct_reference)
{
    VarInfo *info;

    if (object->datatype == DALIAS) {
        CError_FATAL(1850);
    }
    if (object->datatype != DLOCAL) {
        return;
    }

    info = object->u.var.info;
    info->used = 1;
    if (copts.optimizesize) {
        info->usage++;
    } else {
        info->usage += current_statement_number;
    }
    if (direct_reference) {
        info->noregister = 1;
    }
}

void COptimizer_CountExpressionObjectUses(ENode *expression)
{
    ENodeList *list;

    for (;;) {
        switch (expression->type) {
            case EOBJREF:
                RecordObjectUse(expression->data.objref, 1);
                return;

            case EINDIRECT:
                if (expression->data.monadic->type == EOBJREF) {
                    RecordObjectUse(expression->data.monadic->data.objref, 0);
                    return;
                }
                expression = expression->data.monadic;
                break;

            case EFUNCCALL:
            case EFUNCCALLP:
                data_00588513 = 0;
                COptimizer_CountExpressionObjectUses(expression->data.monadic);
                if ((list = expression->data.funccall.args) != NULL) {
                    do {
                        COptimizer_CountExpressionObjectUses(list->node);
                    } while ((list = list->next) != NULL);
                }
                return;

            case ECOND:
                COptimizer_CountExpressionObjectUses(expression->data.monadic);
                COptimizer_CountExpressionObjectUses(expression->data.diadic.right);
                COptimizer_CountExpressionObjectUses(expression->data.cond.expr2);
                return;

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
                COptimizer_CountExpressionObjectUses(expression->data.monadic);
                expression = expression->data.diadic.right;
                break;

            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case EFORCELOAD:
            case ETYPCON:
            case EBITFIELD:
                expression = expression->data.monadic;
                break;

            case ENULLCHECK:
                COptimizer_CountExpressionObjectUses(expression->data.monadic);
                expression = expression->data.diadic.right;
                break;

            case EINTCONST:
            case EFLOATCONST:
            case EPRECOMP:
            case ELABEL:
            case EVECTOR128CONST:
                return;

            case ESTRINGCONST:
                return;

            default:
                CError_FATAL(1949);
                break;
        }
    }
}

/* Entries attached to optimizer statements. */
/* Statement records traversed by the optimizer. */

void COptimizer_RecordObjectUse(Object *object, unsigned char direct_reference)
{
    VarInfo *info;

    if (object->datatype == DALIAS) {
        CError_FATAL(1850);
    }
    if (object->datatype != DLOCAL) {
        return;
    }

    info = object->u.var.info;
    info->used = 1;
    if (copts.optimizesize) {
        info->usage++;
    } else {
        info->usage += current_statement_number;
    }
    if (direct_reference) {
        info->noregister = 1;
    }
}

void simplify_statement_branches(Statement *stmt)
{
    Statement *p;

    remove_unreferenced_labels(stmt);
    for (p = stmt; p != NULL; p = p->next) {
        switch (p->type) {
            case ST_GOTO:
                COptimizer_SimplifyBranch(p);
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                fold_and_invert_conditional_branch(p);
                break;
            case ST_SWITCH:
                follow_switch_labels_and_fold_constant(p);
                break;
        }
    }
}

void remove_unreferenced_labels(Statement *statements)
{
    Statement *statement;
    SwitchCase *group;
    CLabel *result;

    for (statement = statements; statement != NULL; statement = statement->next)
        statement->marked = 0;
    for (statement = statements; statement != NULL; statement = statement->next) {
        switch (statement->type) {
            case ST_GOTO:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_OVF:
                if (statement->label->stmt != NULL)
                    statement->label->stmt->marked = 1;
                break;
            case ST_SWITCH: {
                SwitchInfo *head = (SwitchInfo *)statement->label;
                head->defaultlabel->stmt->marked = 1;
                for (group = ((SwitchInfo *)statement->label)->cases; group != NULL; group = group->next)
                    group->label->stmt->marked = 1;
                break;
            }
            case ST_ASM: {
                CLabel *operandLabel;
                result = InlineAsm_FindOperandLabel(statement);
                if (result != NULL)
                    result->stmt->marked = 1;
                operandLabel = (CLabel *)InlineAsm_GetOperandLabel(statement);
                if (operandLabel != NULL)
                    operandLabel->stmt->marked = 1;
                break;
            }
            default: {
                ExceptionAction *exception;
                for (exception = statement->dobjstack; exception != NULL; exception = exception->next) {
                    if (exception->kind == 0xd) {
                        exception->data.catch_block.label->stmt->marked = 1;
                        exception->data.catch_block.label->stmt->flags |= 1;
                    } else if (exception->kind == 0xf) {
                        exception->data.specification.label->stmt->marked = 1;
                        exception->data.specification.label->stmt->flags |= 1;
                    }
                }
                break;
            }
        }
    }
    for (statement = statements; statement != NULL; statement = statement->next) {
        if (statement->type == ST_LABEL && statement->marked == 0 && (statement->flags & 1) == 0) {
            statement->type = ST_NOP;
            optimizer_changed = 1;
        }
    }
}

void follow_switch_labels_and_fold_constant(Statement *self)
{
    SwitchInfo *b;
    SwitchCase *p;

    CError_ASSERT(1731, (b = (SwitchInfo *)self->label) != NULL && b->cases != NULL && b->defaultlabel != NULL);

    b->defaultlabel = COpt_Follow(self, b->defaultlabel);
    for (p = b->cases; p != NULL; p = p->next)
        p->label = COpt_Follow(self, p->label);

    if (self->expr->type == EINTCONST) {
        for (p = b->cases; p != NULL; p = p->next) {
            if (CInt64_Equal(p->min, self->expr->data.intval))
                break;
        }
        self->type = ST_GOTO;
        self->label = (p != NULL) ? p->label : b->defaultlabel;
    }
}

void fold_and_invert_conditional_branch(Statement *s)
{
    Boolean seen;
    Statement *prev;
    Statement *q;
    Statement *allocated;
    UInt8 kind;
    Statement *head;

    if (iszero(s->expr)) {
        if (s->type == ST_IFNGOTO) {
            q = lalloc(sizeof(Statement));
            *q = *s;
            q->type = ST_GOTO;
            s->next = q;
        }
        s->type = ST_EXPRESSION;
        optimizer_changed = 1;
        return;
    }
    if (isnotzero(s->expr)) {
        do {
            if (s->type != ST_IFGOTO) {
                if (s->type != ST_IFNGOTO)
                    break;
            } else {
                allocated = lalloc(sizeof(Statement));
                *allocated = *s;
                allocated->type = ST_GOTO;
                s->next = allocated;
            }
        } while (0);
        s->type = ST_EXPRESSION;
        optimizer_changed = 1;
        return;
    }
    seen = 0;
    q = head = s->next;
    if (head)
        do {
            if ((kind = q->type) > 2) {
                if (kind == 3) {
                    if (q->label == s->label) {
                        s->type = ST_EXPRESSION;
                        optimizer_changed = 1;
                        return;
                    }
                    if (seen)
                        break;
                    prev = q;
                    for (q = q->next; q != NULL; q = q->next) {
                        if (q->type > 2)
                            break;
                        if (s->label->stmt == q) {
                            s->label = prev->label;
                            prev->type = ST_NOP;
                            if (s->type == ST_IFGOTO)
                                s->type = ST_IFNGOTO;
                            else
                                s->type = ST_IFGOTO;
                            optimizer_changed = 1;
                            s->label = COFindOwner(s);
                            return;
                        }
                    }
                    break;
                } else if (kind == 8 && q->expr == NULL && data_00581300 == 0 && !seen) {
                    prev = q;
                    for (q = q->next; q != NULL; q = q->next) {
                        if (q->type > 2)
                            break;
                        if (s->label->stmt == q) {
                            s->label = data_0058802c;
                            data_0058851f = 1;
                            prev->type = ST_NOP;
                            if (s->type == ST_IFGOTO)
                                s->type = ST_IFNGOTO;
                            else
                                s->type = ST_IFGOTO;
                            optimizer_changed = 1;
                            return;
                        }
                    }
                    break;
                } else
                    break;
            } else {
                if (kind == 2)
                    seen = 1;
                if (s->label->stmt == q) {
                    s->type = ST_EXPRESSION;
                    optimizer_changed = 1;
                    return;
                }
            }
        } while ((q = q->next) != NULL);
    s->label = COFindOwnerPlain(s);
}

void build_opt_blocks(Statement *first)
{
    COptBlock *block;
    COptBlock *target;
    COptBlockLink *successor;
    COptBlockLink *predecessor;
    SwitchCase *caseEntry;
    ObjectList *objectEntry;
    SInt16 reg;
    int nodeCount;

    opt_block_bits_size = (SInt16)((next_varnumber - 1) / 16 * 2 + 2);
    if (copts.globaloptimizer != 0) {
        opt_block_bits_size += 0x20;
        data_005812fc = 0;
    }
    target = new_block();
    opt_blocks = block = target;
    target->items = NULL;
    target->count = 0;
    if (first != NULL) {
        successor = lalloc(sizeof(*successor));
        successor->next = NULL;
        target->succ = successor;
        successor->target.statement = first;
    }
    for (objectEntry = arguments; objectEntry != NULL; objectEntry = objectEntry->next) {
        reg = objectEntry->object->u.var.info->varnumber;
        target->referenceBarrierBits[reg >> 4] |= bit_masks[reg & 0xf];
    }
    if (first != NULL) {
        for (;;) {
            block = block->next = new_block2();
            block->items = first;
            nodeCount = 1;
            for (;;) {
                switch (first->type) {
                    case ST_NOP:
                    case ST_LABEL:
                    case ST_ENTRY:
                        if (first->next != NULL && first->next->type != ST_LABEL) {
                            nodeCount++;
                            first = first->next;
                            continue;
                        }
                        goto end_block;
                    case ST_EXPRESSION:
                    case ST_BEGINCATCH:
                    case ST_ENDCATCH:
                    case ST_ENDCATCHDTOR:
                    case ST_ASM:
                        if (first->next != NULL && first->next->type == ST_GOTO) {
                            nodeCount++;
                            first = first->next;
                            continue;
                        }
                    end_block:
                    default:
                    case ST_EXIT:
                        if (first->next != NULL) {
                            first->next->type != ST_LABEL;
                        }
                    case ST_GOTO:
                    case ST_SWITCH:
                    case ST_IFGOTO:
                    case ST_IFNGOTO:
                    case ST_RETURN:
                    case ST_OVF:
                    case ST_GOTOEXPR:
                        switch (first->type) {
                            case ST_SWITCH:
                                if (((SwitchInfo *)first->label)->defaultlabel != data_0058802c) {
                                    add_succ(block, ((SwitchInfo *)first->label)->defaultlabel);
                                }
                                for (caseEntry = ((SwitchInfo *)first->label)->cases; caseEntry != NULL;
                                     caseEntry = caseEntry->next) {
                                    if (caseEntry->label != data_0058802c) {
                                        add_succ(block, caseEntry->label);
                                    }
                                }
                                break;
                            case ST_GOTO:
                            case ST_IFGOTO:
                            case ST_IFNGOTO:
                            case ST_OVF:
                                if (first->label != data_0058802c) {
                                    add_succ(block, first->label);
                                }
                                if (first->type == ST_GOTO) {
                                    break;
                                }
                            case ST_EXPRESSION:
                            case ST_ENTRY:
                            case ST_BEGINCATCH:
                            case ST_ENDCATCH:
                            case ST_ENDCATCHDTOR:
                            default:
                                if (first->next != NULL) {
                                    COptBlockLink *edge = lalloc(sizeof(*edge));
                                    edge->next = block->succ;
                                    block->succ = edge;
                                    edge->target.statement = first->next;
                                }
                                break;
                            case ST_RETURN:
                            case ST_EXIT:
                            case ST_GOTOEXPR:
                                break;
                        }
                        break;
                }
                break;
            }
            first = first->next;
            block->count = (SInt16)nodeCount;
            if (first == NULL) {
                break;
            }
        }
    }
    for (block = opt_blocks; block != NULL; block = block->next) {
        for (successor = block->succ; successor != NULL; successor = successor->next) {
            first = successor->target.statement;
            for (target = opt_blocks->next; target != NULL; target = target->next) {
                if (target->items == first) {
                    successor->target.block = target;
                    predecessor = lalloc(sizeof(*predecessor));
                    predecessor->next = target->pred;
                    target->pred = predecessor;
                    predecessor->target.block = block;
                    break;
                }
            }
            if (target == NULL) {
                CError_FATAL(1587);
            }
        }
    }
    COptimizer_004bf980();
    mark_and_propagate_dlocal_reference_bits();
}

void COptimizer_004bf980(void)
{
    COptBlock *func;
    func = opt_blocks->next;
    while (func != NULL) {
        Boolean advance = 1;
        Statement *op = func->items;
        SInt16 remaining = func->count;
        if (remaining > 0) {
            do {
                ENode *expr;
                if (op->type >= 4 && op->type <= 0xf && (expr = op->expr) != NULL && expr->type == ECOMMA) {
                    Statement *next = (Statement *)lalloc(0x1a);
                    *next = *op;
                    op->next = next;
                    op->type = ST_EXPRESSION;
                    op->expr = expr->data.diadic.left;
                    next->expr = expr->data.diadic.right;
                    func->count++;
                    advance = 0;
                    break;
                }
                op = op->next;
            } while (--remaining > 0);
        }
        if (advance)
            func = func->next;
    }
}

void mark_and_propagate_dlocal_reference_bits(void)
{
    SInt16 i;
    Statement *item;
    COptBlock *node;

    current_opt_block = opt_blocks;
    while (current_opt_block != NULL) {
        item = current_opt_block->items;
        if (current_opt_block->count > 0 && item->type == ST_LABEL && (item->flags & 1) != 0) {
            for (i = 0; i < next_varnumber; i++)
                SetBit_4bfa30(current_opt_block->referenceBarrierBits, i);
        }
        for (i = current_opt_block->count; i > 0; i--) {
            if (item->type >= 4 && item->type <= 0xf && item->expr != NULL)
                mark_dlocal_reference_bits(item->expr);
            item = item->next;
        }
        current_opt_block = current_opt_block->next;
    }

    for (i = 0; i < next_varnumber; i++) {
        for (node = opt_blocks; node != NULL; node = node->next) {
            if (node->flag == 0 && TestBit_4bfa30(node->referenceBits, i))
                propagate_bit_to_preds(node, i);
        }
        for (node = opt_blocks; node != NULL; node = node->next)
            node->flag = 0;
    }
}

void propagate_bit_to_preds(COptBlock *node, short bit)
{
    int word;
    COptBlockLink *edge;
    COptBlock *child;
    COptBlockLink *childEdge;
    int mask;
    word = (mask = bit) >> 4;
    mask &= 15;
    for (;;) {
        node->flag = 1;
        edge = node->pred;
        if (edge == NULL)
            break;
    next_edge:;
        if (edge->target.block->flag == 0) {
            if (tbit(edge->target.block->referenceBarrierBits, word, mask) == 0) {
                sbit(edge->target.block->referenceBits, word, mask);
                if (edge->next == NULL) {
                    node = edge->target.block;
                    continue;
                }
                child = edge->target.block;
                for (;;) {
                    child->flag = 1;
                    childEdge = child->pred;
                    if (childEdge == NULL)
                        break;
                next_child_edge:;
                    if (childEdge->target.block->flag == 0) {
                        if (test_bit(childEdge->target.block->referenceBarrierBits, bit) == 0) {
                            set_bit(childEdge->target.block->referenceBits, bit);
                            if (childEdge->next == NULL) {
                                child = childEdge->target.block;
                                continue;
                            }
                            propagate_bit_to_preds(childEdge->target.block, bit);
                        } else {
                            childEdge->target.block->flag = 1;
                        }
                    }
                    if ((childEdge = childEdge->next) != NULL)
                        goto next_child_edge;
                    break;
                }

            } else {
                edge->target.block->flag = 1;
            }
        }
        if ((edge = edge->next) != NULL)
            goto next_edge;
        break;
    }
    return;
}

void mark_dlocal_reference_bits(ENode *node)
{
    for (;;) {
        switch (node->type) {
            case EINDIRECT:
                if (node->data.monadic->type == EOBJREF) {
                    Object *obj = node->data.monadic->data.objref;
                    if (obj->datatype == DLOCAL)
                        SetBit(current_opt_block->referenceBits, obj->u.var.info->varnumber);
                    return;
                }
                node = node->data.monadic;
                break;

            case EOBJREF: {
                Object *obj = node->data.objref;
                if (obj->datatype == DLOCAL) {
                    VarInfo *info = obj->u.var.info;
                    if (!TestBit(current_opt_block->referenceBits, info->varnumber))
                        SetBit(current_opt_block->referenceBarrierBits, info->varnumber);
                }
                return;
            }

            case EFUNCCALL:
            case EFUNCCALLP: {
                ENodeList *arg;
                mark_dlocal_reference_bits(node->data.funccall.funcref);
                if ((arg = node->data.funccall.args) != NULL) {
                    do {
                        mark_dlocal_reference_bits(arg->node);
                    } while ((arg = arg->next) != NULL);
                }
                return;
            }

            case ECOND:
                mark_dlocal_reference_bits(node->data.cond.cond);
                mark_dlocal_reference_bits(node->data.cond.expr1);
                mark_dlocal_reference_bits(node->data.cond.expr2);
                return;

            case EASS:
                if (node->data.diadic.left->type == EINDIRECT &&
                    node->data.diadic.left->data.monadic->type == EOBJREF) {
                    VarInfo *info;
                    Object *obj;
                    mark_dlocal_reference_bits(node->data.diadic.right);
                    obj = node->data.diadic.left->data.monadic->data.objref;
                    if (obj->datatype == DLOCAL) {
                        info = obj->u.var.info;
                        if (!TestBit(current_opt_block->referenceBits, info->varnumber))
                            SetBit(current_opt_block->referenceBarrierBits, info->varnumber);
                    }
                    return;
                }
                /* fallthrough */
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
                mark_dlocal_reference_bits(node->data.diadic.left);
                node = node->data.diadic.right;
                break;

            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case EFORCELOAD:
            case ETYPCON:
            case EBITFIELD:
                node = node->data.monadic;
                break;

            case ENULLCHECK:
                mark_dlocal_reference_bits(node->data.diadic.left);
                node = node->data.diadic.right;
                break;

            case EINTCONST:
            case EFLOATCONST:
            case ESTRINGCONST:
            case EPRECOMP:
            case ELABEL:
            case EVECTOR128CONST:
                return;

            default:
                CError_FATAL(1331);
                break;
        }
    }
}

void set_bit(SInt16 *p, SInt16 n)
{
    p[n >> 4] |= bit_masks[n & 0xf];
}

UInt16 test_bit(const SInt16 *words, short bit)
{
    return bit_masks[bit & 15] & words[bit >> 4];
}

/* The common subexpression an expression computes, found or entered in the table by its kind and operands, every
   occurrence recorded; assignments and calls end the subexpressions they may change. */
COptCSE *collect_expr_cse(ENode *expr)
{
    COptCSE *list;
    OptimizerOccurrence *occ;
    ENodeList *arg;
    COptCSE *cse;
    COptCSE *left;
    ENode *saved;
    Object *obj;
    UInt8 type;

    switch (type = expr->type) {
        case EFUNCCALL:
            COptimizer_004c0470(expr);
            return NULL;
        case EFUNCCALLP:
            for (arg = expr->data.funccall.args; arg; arg = arg->next)
                collect_expr_cse(arg->node);
            if (expr->type == EFUNCCALLP)
                collect_expr_cse(expr->data.funccall.funcref);
            eliminate_common_subexpressions();
            for (cse = list = cse_entries[4]; list; cse = cse->next, list = cse) {
                if (cse->expr->data.monadic->type == EOBJREF) {
                    obj = cse->expr->data.monadic->data.objref;
                    CError_ASSERT(672, obj->datatype != DALIAS);
                    if (obj->datatype == DLOCAL && !obj->u.var.info->noregister)
                        continue;
                }
                Registers_InvalidateCSE(cse);
            }
            return NULL;
        case EPOSTINC:
        case EPOSTDEC:
        case EPREINC:
        case EPREDEC:
            saved = current_cse_expr;
            current_cse_expr = expr;
            if (expr->data.monadic->type != EINDIRECT)
                CError_FATAL(816);
            COpt_IncDecTarget(expr);
            current_cse_expr = saved;
            return NULL;
        case EINDIRECT:
        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
        case EFORCELOAD:
            cse = find_or_create_left_cse(expr, collect_expr_cse(expr->data.monadic));
            return COpt_AddOccurrence(expr, cse);
        case ETYPCON:
            left = find_or_create_unary_cse(expr, collect_expr_cse(expr->data.monadic));
            return COpt_AddOccurrence(expr, left);
        case EMUL:
        case EMULV:
        case EADDV:
        case EADD:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
            if ((left = collect_expr_cse(expr->data.diadic.left)), expr->type == type) {
                cse = find_or_create_commutative_cse(expr, left, collect_expr_cse(expr->data.diadic.right));
                return COpt_AddOccurrence(expr, cse);
            }
            return NULL;
        case EDIV:
        case EMODULO:
        case ESUBV:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBTST:
            left = collect_expr_cse(expr->data.diadic.left);
            if (expr->type == type) {
                left = find_or_create_cse(expr, left, collect_expr_cse(expr->data.diadic.right));
                return COpt_AddOccurrence(expr, left);
            }
            return NULL;
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
            if (expr->data.diadic.left->type != EINDIRECT)
                CError_FATAL(887);
            saved = current_cse_expr;
            current_cse_expr = expr;
            collect_expr_cse(expr->data.diadic.right);
            if (expr->type == type) {
                collect_expr_cse(expr->data.diadic.left->data.monadic);
                invalidate_expr_cse(expr->data.diadic.left);
                current_cse_expr = saved;
                return NULL;
            } else {
                invalidate_expr_cse(NULL);
                return NULL;
            }
        case ECOMMA:
            collect_expr_cse(expr->data.diadic.left);
            saved = current_cse_expr;
            current_cse_expr = expr->data.diadic.right;
            cse = collect_expr_cse(expr->data.diadic.right);
            current_cse_expr = saved;
            return cse;
        case EINTCONST:
            return COpt_IntConst(expr);
        case EFLOATCONST:
            return COpt_FloatConst(expr);
        case EVECTOR128CONST:
            return COpt_VectorConst(expr);
        case EOBJREF:
            return COpt_ObjectRef(expr, cse_entries[EOBJREF]);
        case EBITFIELD:
            invalidate_expr_cse(NULL);
            return NULL;
        case ECOND:
            collect_expr_cse(expr->data.cond.cond);
            invalidate_expr_cse(NULL);
            return NULL;
        case ENULLCHECK:
            collect_expr_cse(expr->data.monadic);
            invalidate_expr_cse(NULL);
            return NULL;
        case ELAND:
        case ELOR:
            collect_expr_cse(expr->data.diadic.left);
            invalidate_expr_cse(NULL);
            return NULL;
        case ELABEL:
            return NULL;
        case EPRECOMP:
            CError_FATAL(947);
        default:
            CError_FATAL(950);
            return NULL;
    }
}

void COptimizer_004c0470(ENode *node)
{
    Object *obj;
    COptCSE *entry;
    COptCSE *entries;

    if (traverse_node_list_reverse(node->data.funccall.args, node->data.funccall.functype->args)) {
        if (node->type == EFUNCCALL)
            collect_expr_cse(node->data.funccall.funcref);
        eliminate_common_subexpressions();
        entry = entries = cse_entries[4];
        if (entries) {
            do {
                if (entry->expr->data.monadic->type == EOBJREF) {
                    obj = entry->expr->data.monadic->data.addr.objref;
                    CError_ASSERT(672, obj->datatype != DALIAS);
                    if (obj->datatype != DLOCAL || obj->u.var.info->noregister != 0)
                        Registers_InvalidateCSE(entry);
                } else {
                    Registers_InvalidateCSE(entry);
                }
                entry = entry->next;
            } while (entry);
        }
    } else {
        invalidate_expr_cse(NULL);
    }
}

Boolean traverse_node_list_reverse(ENodeList *node, FuncArg *value)
{
    Boolean result;
    if (node == NULL)
        return 1;
    if (value != NULL && (const void *)value != &elipsis && (const void *)value != &oldstyle)
        value = value->next;
    if (node->next != NULL)
        result = traverse_node_list_reverse(node->next, value);
    collect_expr_cse(node->node);
    return result;
}

void invalidate_expr_cse(ENode *expression)
{
    ENode *result;
    Object *identity;
    Object *object;
    ENode *candidate;
    COptCSE *entry;
    COptCSE *remaining;
    Object *remainingObject;
    COptCSE *allEntries;
    COptCSE *head;
    if (expression == NULL) {
        eliminate_common_subexpressions();
        clearEntries(75);
        occurrence_list = NULL;
        freeoheap();
        return;
    }
    data_005812ff = 1;
    if (expression->type == EINDIRECT) {
        result = fn_004c07c0(expression->data.monadic);
        if (result != NULL) {
            do {
                identity = result->data.objref;
                eliminate_common_subexpressions();
                head = cse_entries[4];
                entry = head;
                if (head != NULL) {
                    do {
                        candidate = fn_004c07c0(entry->expr->data.monadic);
                        if (candidate != NULL && identity == candidate->data.objref)
                            Registers_InvalidateCSE(entry);
                        entry = entry->next;
                    } while (entry != NULL);
                }
                result = fn_004c07c0(expression->data.monadic);
                if (result == NULL) {
                    head = cse_entries[4];
                    remaining = head;
                    if (head != NULL) {
                        do {
                            if (remaining->expr->data.monadic->type == EOBJREF) {
                                object = remaining->expr->data.monadic->data.objref;
                                if (object->datatype == DALIAS)
                                    CError_FATAL(672);
                                if (object->datatype == DLOCAL && object->u.var.info->noregister == 0)
                                    continue;
                            }
                            Registers_InvalidateCSE(remaining);
                        } while ((remaining = remaining->next) != NULL);
                    }
                    return;
                }
            } while (identity != result->data.objref);
            return;
        }
    }
    eliminate_common_subexpressions();
    head = cse_entries[4];
    allEntries = head;
    if (head != NULL) {
        do {
            if (allEntries->expr->data.monadic->type == EOBJREF) {
                remainingObject = allEntries->expr->data.monadic->data.objref;
                if (remainingObject->datatype == DALIAS)
                    CError_FATAL(672);
                if (remainingObject->datatype == DLOCAL && remainingObject->u.var.info->noregister == 0)
                    continue;
            }
            Registers_InvalidateCSE(allEntries);
        } while ((allEntries = allEntries->next) != NULL);
    }
}

ENode *fn_004c07c0(ENode *expr)
{
    last_node = NULL;
    data_00581308 = 0;
    COptimizer_004c0800(expr);
    if (data_00581308 == 1) {
        return last_node;
    }
    return NULL;
}

void COptimizer_004c0800(ENode *n)
{
    for (;;) {
        switch (n->type) {
            case ETYPCON:
                n = n->data.diadic.left;
                break;
            case EOBJREF:
                last_node = n;
                data_00581308++;
                return;
            case EADD:
            case ESUB:
                COptimizer_004c0800(n->data.diadic.left);
                n = n->data.diadic.right;
                break;
            default:
                return;
        }
    }
}

COptCSE *find_or_create_commutative_cse(ENode *expr, COptCSE *left, COptCSE *right)
{
    COptCSE *entry;

    if (left == NULL || right == NULL)
        return NULL;

    for (entry = cse_entries[expr->type]; entry != NULL; entry = entry->next) {
        if ((entry->left == left && entry->right == right) || (entry->left == right && entry->right == left)) {
            if (entry->expr == expr)
                CError_FATAL(612);
            entry->uses++;
            return entry;
        }
    }

    entry = (COptCSE *)oalloc(30);
    entry->expr = expr;
    entry->replacement = NULL;
    entry->block = current_opt_block;
    entry->last = current_cse_expr;
    entry->left = NULL;
    entry->right = NULL;
    entry->uses = 1;
    entry->next = cse_entries[expr->type];
    cse_entries[expr->type] = entry;
    entry->left = left;
    entry->right = right;
    return entry;
}

COptCSE *find_or_create_cse(ENode *expr, COptCSE *left, COptCSE *right)
{
    COptCSE *entry;
    if (left == NULL || right == NULL)
        return NULL;
    entry = cse_entries[expr->type];
    while (entry != NULL) {
        if (entry->left == left && entry->right == right) {
            if (entry->expr == expr) {
                CError_FATAL(581);
            }
            entry->uses++;
            return entry;
        }
        entry = entry->next;
    }
    entry = (COptCSE *)oalloc(30);
    entry->expr = expr;
    entry->replacement = NULL;
    entry->block = current_opt_block;
    entry->last = current_cse_expr;
    entry->left = NULL;
    entry->right = NULL;
    entry->uses = 1;
    entry->next = cse_entries[expr->type];
    cse_entries[expr->type] = entry;
    entry->left = left;
    entry->right = right;
    return entry;
}

COptCSE *find_or_create_unary_cse(ENode *expr, COptCSE *left)
{
    COptCSE *entry;
    if (left == NULL)
        return NULL;
    entry = cse_entries[expr->type];
    while (entry != NULL) {
        if (entry->left == left && entry->expr->rtype == expr->rtype) {
            if (entry->expr == expr)
                CError_FATAL(552);
            entry->uses++;
            return entry;
        }
        entry = entry->next;
    }
    entry = (COptCSE *)oalloc(30);
    entry->expr = expr;
    entry->replacement = NULL;
    entry->block = current_opt_block;
    entry->last = current_cse_expr;
    entry->left = NULL;
    entry->right = NULL;
    entry->uses = 1;
    entry->next = cse_entries[expr->type];
    cse_entries[expr->type] = entry;
    entry->left = left;
    return entry;
}

COptCSE *find_or_create_left_cse(ENode *expr, COptCSE *left)
{
    COptCSE *entry;
    COptCSE *new_entry;
    if (left == NULL)
        return NULL;
    entry = cse_entries[expr->type];
    while (entry != NULL) {
        if (entry->left == left && entry->expr->rtype == expr->rtype) {
            if (entry->expr == expr)
                CError_FATAL(524);
            entry->uses++;
            return entry;
        }
        entry = entry->next;
    }
    new_entry = (COptCSE *)oalloc(0x1e);
    new_entry->expr = expr;
    new_entry->replacement = NULL;
    new_entry->block = current_opt_block;
    new_entry->last = current_cse_expr;
    new_entry->left = NULL;
    new_entry->right = NULL;
    new_entry->uses = 1;
    new_entry->next = cse_entries[expr->type];
    cse_entries[expr->type] = new_entry;
    new_entry->left = left;
    return new_entry;
}

void eliminate_common_subexpressions(void)
{
    COptCSE *group;
    int kind;
    ENode *reference;
    short cost;
    OptimizerOccurrence *occurrence;
    ENode *replacement;
    ENode *saved;
    OptimizerOccurrence *scan;
    int bestCost;
    int index;
    OptimizerOccurrence *update;
    COptCSE *bucket;
    ENode *objectExpression;
    ObjectList *objectLink;
    VarInfo *registerInfo;
    Object *object;
    char changed;
    COptCSE *best;
    ENode *assignment;
    ENode *oldLast;

    for (;;) {
        index = 0;
        best = NULL;
        bestCost = 0;
        changed = 0;
        for (; (kind = (short)index) < 75; index++) {
            switch (kind) {
                case 50:
                case 51:
                case 56:
                case 74:
                    continue;
            }
            group = bucket = cse_entries[(short)index];
            if (bucket == NULL)
                continue;
            do {
                if (group->uses > 1) {
                    cost = Registers_GetCSEWeight(group);
                    if (cost > 4) {
                        if (group->replacement != NULL) {
                            data_00581302 = 0;
                            occurrence = occurrence_list;
                            replacement = group->replacement;
                            while (occurrence != NULL) {
                                if (occurrence->group == group) {
                                    *occurrence->expression = *replacement;
                                    data_00581302 += 1;
                                    occurrence->group = NULL;
                                    occurrence->expression = NULL;
                                }
                                occurrence = occurrence->next;
                            }
                            if (data_00581302 < 1)
                                CError_FATAL(348);
                            data_005812fe = 1;
                            changed = 1;
                            Registers_DivideUses(group, group->uses);
                        } else if (group->uses * cost > bestCost) {
                            best = group;
                            bestCost = group->uses * cost;
                        }
                    }
                }
                group = group->next;
            } while (group != NULL);
        }
        if (changed != 0)
            continue;
        if (best == NULL || data_005812fc >= 256)
            return;

        object = (Object *)lalloc(sizeof(Object));
        memclrw(object, sizeof(Object));
        object->name = CParser_GetUniqueName();
        object->type = best->expr->rtype;
        object->datatype = DLOCAL;
        registerInfo = CPrep_AllocateVarInfo();
        object->u.var.info = registerInfo;
        objectLink = (ObjectList *)lalloc(sizeof(ObjectList));
        objectLink->object = object;
        objectLink->next = locals;
        locals = objectLink;
        registerInfo->used = 1;
        registerInfo->usage = best->uses + 1;
        objectExpression = (ENode *)lalloc(sizeof(ENode));
        objectExpression->type = EOBJREF;
        objectExpression->cost = 0;
        objectExpression->flags = 0;
        objectExpression->data.objref = object;
        objectExpression->rtype = CDecl_NewPointerType(object->type);
        reference = (ENode *)lalloc(sizeof(ENode));
        reference->type = EINDIRECT;
        reference->cost = 1;
        reference->flags = 0;
        reference->data.diadic.left = objectExpression;
        reference->rtype = object->type;
        assignment = (ENode *)lalloc(sizeof(ENode));
        assignment->type = EASS;
        assignment->cost = 255;
        assignment->flags = 0;
        assignment->rtype = object->type;
        assignment->data.diadic.left = reference;
        assignment->data.diadic.right = (ENode *)lalloc(sizeof(ENode));
        *assignment->data.diadic.right = *best->expr;
        best->expr = assignment->data.diadic.right;
        data_00581302 = 0;
        scan = occurrence_list;
        while (scan != NULL) {
            if (scan->group == best) {
                *scan->expression = *reference;
                data_00581302 += 1;
                scan->group = NULL;
                scan->expression = NULL;
            }
            scan = scan->next;
        }
        best->replacement = reference;
        if (data_00581302 < 2)
            CError_FATAL(390);
        else
            data_005812fe = 1;
        saved = (ENode *)lalloc(sizeof(ENode));
        *saved = *best->last;
        best->last->type = ECOMMA;
        best->last->data.diadic.left = assignment;
        best->last->data.diadic.right = saved;
        Registers_DivideUses(best, best->uses);
        data_005812fc += 1;
        oldLast = best->last;
        if (current_cse_expr == best->last)
            current_cse_expr = saved;
        update = occurrence_list;
        while (update != NULL) {
            if (update->group != NULL && Registers_ContainsCOptCSE(update->group, best) == 0) {
                if (update->group->last == oldLast)
                    update->group->last = saved;
                if (update->group->expr == oldLast)
                    update->group->expr = saved;
                if (update->expression == oldLast)
                    update->expression = saved;
            }
            update = update->next;
        }
    }
}

/* The cost of evaluating CSE's expression tree: its nodes, the divisions and multiplications twice, and the loads
   of variables that cannot live in a register. */
int Registers_GetCSEWeight(COptCSE *tree)
{
    Object *object;
    int result;
    int weight;
    if (tree != NULL) {
        while (tree->expr->type == ETYPCON && tree->expr->rtype->type == tree->expr->data.monadic->rtype->type &&
               tree->expr->rtype->size == tree->expr->data.monadic->rtype->size) {
            tree = tree->left;
        }
        if (tree->expr->type == EINDIRECT && tree->expr->data.monadic->type == EOBJREF) {
            object = tree->expr->data.monadic->data.objref;
            if (object->datatype == DLOCAL && object->u.var.info->noregister == 0) {
                result = 0;
            } else {
                result = 1;
            }
            return result;
        }
        weight = 1;
        if (copts.optimizesize == 0 &&
            (tree->expr->type == EMUL || (tree->expr->type == EDIV || tree->expr->type == EMODULO))) {
            weight = 2;
        }
        return Registers_GetCSEWeight(tree->left) + Registers_GetCSEWeight(tree->right) + weight;
    }
    return 0;
}

Boolean Registers_ContainsCOptCSE(COptCSE *target, COptCSE *node)
{
    if (target == node)
        return 1;
    if (node->left != NULL && Registers_ContainsCOptCSE(target, node->left))
        return 1;
    if (node->right != NULL && Registers_ContainsCOptCSE(target, node->right))
        return 1;
    return 0;
}

void Registers_DivideUses(COptCSE *node, SInt16 divisor)
{
    node->uses /= divisor;
    if (node->left)
        Registers_DivideUses(node->left, divisor);
    if (node->right)
        Registers_DivideUses(node->right, divisor);
}

void Registers_InvalidateCSE(COptCSE *node)
{
    COptCSE *dependent;
    OptimizerOccurrence *entry;
    short listIndex;

    if (node != NULL) {
        for (entry = occurrence_list; entry != NULL; entry = entry->next) {
            if (entry->group == node) {
                entry->group = NULL;
                entry->expression = NULL;
            }
        }
        node->uses = 0xffff;
        node->left = NULL;
        node->right = NULL;
        listIndex = 0;
        while ((long)listIndex < 0x4b) {
            for (dependent = cse_entries[listIndex]; dependent != NULL; dependent = dependent->next) {
                if (dependent->left == node || dependent->right == node) {
                    Registers_InvalidateCSE(dependent);
                }
            }
            listIndex++;
        }
    }
}
