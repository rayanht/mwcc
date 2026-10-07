#define CERROR_FILE "IroLoop.c"
#include "compiler/common.h"
#include "compiler/IroLoop.h"
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
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroFlowgraph.h"
#include "compiler/IroJump.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroTransform.h"
#include "compiler/IroUtil.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>
#include "compiler/BitVector.h"

#include "compiler/ENode.h"

static char lbl_00580640[4];
static struct IROLoopInd *induction_variables;
static struct IROExpr *iro_loop_roots;
static struct BitVector *data_0058064c;
static char lbl_00580650[26];
static unsigned char data_0058066a;
static char lbl_0058066B[9];
static signed int data_00580674;
static struct IRONode *loop_header;
static struct IROLinear *loop_candidate_last;
static char lbl_00580680[32];

#define GMARKED(id)                                                                                                    \
    (((id) >> 5) < IRO_LoopScratchVector_005880dc->size &&                                                             \
     (IRO_LoopScratchVector_005880dc->bits[(id) >> 5] & (1 << (id))) != 0)
#define SMARKED(v, id) (((id) >> 5) < (v)->size && ((v)->bits[(id) >> 5] & (1 << (id))) != 0)

/* LoopNode */
/* InLoop */
/* IRO_NodeTable */
/* FirstInd */
/* IRO_FirstNode */
/* IRO_NumLinear */
/* IRO_NumNodes */

typedef enum NodeKind { NK0 } NodeKind;

static inline IROLinear *MakeSequenceNode(IROLinear *left, IROLinear *right, IROList *context, int storeRight)
{
    IROLinear *sequence = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter++;
    sequence->index = linear_index_counter;
    sequence->nodetype = EADD;
    sequence->u.diadic.left = left;
    if (storeRight)
        sequence->u.diadic.right = right;
    sequence->u.diadic.right = IroUtil_CopyLinearToList(right, context);
    IroUtil_AppendLinear(sequence, context);
    return sequence;
}

/* Copies the add tree TREE to CONTEXT with its terms reordered: the address-flagged ones (0x100) first, then
   the other non-constant ones, then the constants. */
IROLinear *reorder_sequence_by_flags_and_type50(IROLinear *tree, IROList *context)
{
    IROLinear *node;
    IROElmList *previousEntry;
    IROLinear *result;
    IROElmList *entry;
    IROLinear *tailNode;
    IROLinear *combinedNode;

    IroUtil_InitList(context);
    iro_elm_list_head = elm_list_tail = NULL;
    flatten_linear_to_elm_list(tree->u.diadic.left);
    flatten_linear_to_elm_list(tree->u.diadic.right);

    result = NULL;
    entry = iro_elm_list_head;
    previousEntry = NULL;
    if (entry != NULL) {
        do {
            node = entry->element;
            if (IroDump_IsType1NodeType50(node) == 0 && (node->flags & IROLF_LoopInvariant) != 0) {
                if (result != NULL) {
                    combinedNode = MakeSequenceNode(result, node, context, 0);
                    combinedNode->flags |= IROLF_LoopInvariant;
                    combinedNode->flags |= IROLF_Reffed;
                    combinedNode->rtype = result->rtype;
                    result = combinedNode;
                } else {
                    result = IroUtil_CopyLinearToList(node, context);
                }
                if (previousEntry != NULL)
                    previousEntry->next = entry->next;
                else
                    iro_elm_list_head = entry->next;
            } else {
                previousEntry = entry;
            }
        } while ((entry = entry->next) != NULL);
    }

    entry = iro_elm_list_head;
    previousEntry = NULL;
    if (entry != NULL) {
        do {
            node = entry->element;
            if (IroDump_IsType1NodeType50(node) == 0) {
                if (result != NULL) {
                    IROLinear *sequenceNode;
                    sequenceNode = MakeSequenceNode(result, node, context, 0);
                    sequenceNode->flags |= IROLF_Reffed;
                    sequenceNode->rtype = result->rtype;
                    result = sequenceNode;
                } else {
                    result = IroUtil_CopyLinearToList(node, context);
                }
                if (previousEntry != NULL)
                    previousEntry->next = entry->next;
                else
                    iro_elm_list_head = entry->next;
            } else {
                previousEntry = entry;
            }
        } while ((entry = entry->next) != NULL);
    }

    for (entry = iro_elm_list_head; entry != NULL; entry = entry->next) {
        node = entry->element;
        if (result != NULL) {
            tailNode = MakeSequenceNode(result, node, context, 1);
            tailNode->flags |= IROLF_Reffed;
            tailNode->rtype = result->rtype;
            result = tailNode;
        } else {
            result = IroUtil_CopyLinearToList(node, context);
        }
    }
    return result;
}

void flatten_linear_to_elm_list(IROLinear *n)
{
    if (n->type == IROLinearOp2Arg && n->nodetype == EADD) {
        flatten_linear_to_elm_list(n->u.diadic.left);
        flatten_linear_to_elm_list(n->u.diadic.right);
    } else {
        IROElmList *qn = CompilerTools_AllocatePoolMemory(8);
        qn->element = n;
        qn->next = NULL;
        if (iro_elm_list_head != NULL)
            elm_list_tail->next = qn;
        else
            iro_elm_list_head = qn;
        elm_list_tail = qn;
    }
}

void rewrite_selected_monadic_references(void)
{
    IROLinear *stmt;
    IRONode *loop;
    IROList loc;
    struct IROLinear *size;

    expr_list = expr_tail = NULL;
    expression_count = 0;

    for (loop = iro_flowgraph_head; loop != NULL; loop = loop->nextnode) {
        if ((UInt32)(loop->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
            (IRO_LoopScratchVector_005880dc->bits[loop->index >> 5] & (1 << loop->index)) != 0) {
            stmt = loop->first;
            while (stmt != NULL) {
                if (stmt->type == IROLinearOp1Arg && stmt->nodetype == EINDIRECT &&
                    stmt->u.monadic->type == IROLinearOp2Arg && stmt->u.monadic->nodetype == EADD) {
                    reorder_sequence_by_flags_and_type50(stmt->u.monadic, &loc);
                    size = IroUtil_GetLinearRangeStart(stmt->u.monadic);
                    IroUtil_ReplaceFirstReference(stmt->u.monadic, loc.tail);
                    IroUtil_InsertLinearBefore(loc.head, loc.tail, size);
                }
                if (stmt == loop->last)
                    break;
                stmt = stmt->next;
            }
        }
    }
}

/* Appends to ARG the update of the loop's induction variable by COUNT steps. */
void append_scaled_induction_update(IROLoop *owner, SInt32 count, IROList *arg)
{
    SInt32 value;
    IROLinear *operand;
    IROLinear *expr;
    IROLinear *offset;
    IROLinear *assignment;
    ENode *constant;
    Type *datatype;

    expr = owner->induction->nd;
    datatype = expr->rtype;

    offset = IrOptimizer_NewLinear(IROLinearOperand);
    offset->index = ++linear_index_counter;
    offset->rtype = datatype;

    constant = IrOptimizer_NewENode(EINTCONST);
    constant->rtype = datatype;

    value = count * owner->induction->addConst;
    constant->data.intval.lo = value;
    constant->data.intval.hi = value < 0 ? -1 : 0;

    offset->u.node = constant;
    IroUtil_AppendLinear(offset, arg);

    if (expr->type == IROLinearOp1Arg && (expr->nodetype == EPREINC || expr->nodetype == EPOSTINC)) {
        operand = IroUtil_CopyLinearToList(expr->u.monadic, arg);
        assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
        assignment->index = ++linear_index_counter;
        assignment->nodetype = EADDASS;
        assignment->u.diadic.left = operand;
        assignment->u.diadic.right = offset;
        assignment->rtype = datatype;
        IroUtil_AppendLinear(assignment, arg);
    } else if (expr->type == IROLinearOp1Arg && (expr->nodetype == EPREDEC || expr->nodetype == EPOSTDEC)) {
        operand = IroUtil_CopyLinearToList(expr->u.monadic, arg);
        assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
        assignment->index = ++linear_index_counter;
        assignment->nodetype = ESUBASS;
        assignment->u.diadic.left = operand;
        assignment->u.diadic.right = offset;
        assignment->rtype = datatype;
        IroUtil_AppendLinear(assignment, arg);
    } else if (expr->type == IROLinearOp2Arg && expr->nodetype == EADDASS) {
        operand = IroUtil_CopyLinearToList(expr->u.monadic, arg);
        assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
        assignment->index = ++linear_index_counter;
        assignment->nodetype = EADDASS;
        assignment->u.diadic.left = operand;
        assignment->u.diadic.right = offset;
        assignment->rtype = datatype;
        IroUtil_AppendLinear(assignment, arg);
    }
}

static inline void unsigned_kind(UInt32 p, int *r)
{
    UInt8 k;
    TypeIntegral *v;
    TypeIntegral *t = (TypeIntegral *)p;
    v = (TypeIntegral *)((UInt32)t);
    if (v->integral == IT_UCHAR || t->integral == IT_USHORT || t->integral == IT_UINT || t->integral == IT_ULONG ||
        t->integral == IT_ULONGLONG)
        *r = 1;
}

/* Rewrites the uses of the loop's induction variable between FIRST and LAST as the variable plus ADDCONST. */
void add_const_to_induction_var_references(IROLinear *first, IROLinear *last, SInt32 addConst, IROLoop *loop)
{
    IROLinear *scan;
    IROLinear *next;
    IROLinear *insertionPoint;
    Type *type;
    int isUnsigned;
    Object *object;
    IROLinear *offsetOperand;
    IROLinear *adjustedReference;
    ENode *constantNode;
    IROLinear *scaledUse;
    Type *conversionType;
    Boolean canScale;
    int adjustedAfterScale;
    IROLinear *addressUse;
    CInt64 scale;
    CInt64 offset;
    IROList list;
    CInt64 scaledOffset;

    type = loop->induction->nd->rtype;
    unsigned_kind((UInt32)type, &isUnsigned);

    for (scan = first; scan != NULL; scan = next) {
        next = scan->next;
        if ((object = IroDump_GetObjRef(scan)) != NULL && loop->induction->var->object == object) {
            offsetOperand = IrOptimizer_NewLinear(IROLinearOperand);
            offsetOperand->index = ++linear_index_counter;
            offsetOperand->rtype = type;
            constantNode = IrOptimizer_NewENode(EINTCONST);
            constantNode->rtype = type;
            constantNode->data.intval.lo = addConst;
            constantNode->data.intval.hi = (addConst < 0) ? -1 : 0;
            offsetOperand->u.node = constantNode;

            adjustedReference = IrOptimizer_NewLinear(IROLinearOp2Arg);
            adjustedReference->index = ++linear_index_counter;
            adjustedReference->nodetype = EADD;
            adjustedReference->rtype = type;

            scaledUse = IroUtil_FindNextUse(scan);
            canScale = 1;
            if (scaledUse != NULL && scaledUse->type == IROLinearOp1Arg && scaledUse->nodetype == ETYPCON) {
                conversionType = scaledUse->rtype;
                scaledUse = IroUtil_FindNextUse(scaledUse);
                if (conversionType->type != scan->rtype->type || conversionType->size < scan->rtype->size)
                    canScale = 0;
            }

            adjustedAfterScale = 0;
            if (canScale && scaledUse != NULL && scaledUse->type == IROLinearOp2Arg &&
                (scaledUse->nodetype == ESHL || scaledUse->nodetype == EMUL) &&
                IroDump_IsType1NodeType50(scaledUse->u.diadic.right)) {
                addressUse = IroUtil_FindNextUse(scaledUse);
                if (addressUse != NULL && addressUse->type == IROLinearOp2Arg && addressUse->nodetype == EADD &&
                    addressUse->u.diadic.right == scaledUse) {
                    if ((insertionPoint = IroUtil_FindNextUse(addressUse)) != NULL) {
                        IroUtil_InitList(&list);
                        scale = scaledUse->u.diadic.right->u.node->data.intval;
                        if (scaledUse->nodetype == ESHL)
                            scale = CInt64_Shl(cint64_one, scale);
                        offset = offsetOperand->u.node->data.intval;
                        if (isUnsigned)
                            scaledOffset = CInt64_MulU(scale, offset);
                        else
                            scaledOffset = CInt64_Mul(scale, offset);
                        offsetOperand->u.node->data.intval = scaledOffset;
                        IroUtil_AppendLinear(offsetOperand, &list);
                        IroUtil_AppendLinear(adjustedReference, &list);
                        adjustedReference->u.diadic.right = offsetOperand;
                        IroUtil_InsertLinearBefore(list.head, list.tail, insertionPoint);
                        IroUtil_ReplaceNextReference(addressUse, adjustedReference);
                        adjustedReference->u.diadic.left = addressUse;
                        adjustedReference->rtype = addressUse->rtype;
                        adjustedAfterScale = 1;
                    }
                }
            }

            if (!adjustedAfterScale) {
                adjustedReference->u.diadic.right = offsetOperand;
                adjustedReference->u.diadic.right->flags |= IROLF_Reffed;
                offsetOperand->next = adjustedReference;
                adjustedReference->u.diadic.left = scan;
                IroUtil_ReplaceNextReference(scan, adjustedReference);
                adjustedReference->flags |= IROLF_Reffed;
                scan->next = offsetOperand;
                adjustedReference->next = next;
            }
        }
        if (scan == last)
            break;
    }
}

/* Appends to LIST a branch to DESTINATION taken unless SOURCE < VALUE. */
void create_source_constant_branch(IROList *list, IROLinear *source, SInt32 value, struct CLabel *destination)
{
    IROLinear *constant;
    ENode *expression;
    IROLinear *sourceNode;
    IROLinear *operation;
    IROLinear *assignment;
    Type *type;

    type = source->rtype;

    constant = IrOptimizer_NewLinear(IROLinearOperand);
    constant->index = ++linear_index_counter;
    constant->rtype = type;

    expression = IrOptimizer_NewENode(EINTCONST);
    expression->rtype = type;
    expression->data.intval.lo = value;
    expression->data.intval.hi = value < 0 ? -1 : 0;
    constant->u.node = expression;

    IroUtil_AppendLinear(constant, list);

    sourceNode = IroUtil_CopyLinearToList(source, list);

    operation = IrOptimizer_NewLinear(IROLinearOp2Arg);
    operation->index = ++linear_index_counter;
    operation->nodetype = EGREATER;
    operation->u.diadic.left = sourceNode;
    operation->u.diadic.right = constant;
    operation->u.diadic.right->flags |= IROLF_Reffed;
    operation->rtype = type;
    IroUtil_AppendLinear(operation, list);

    assignment = IrOptimizer_NewLinear(IROLinearOp1Arg);
    assignment->index = ++linear_index_counter;
    assignment->type = IROLinearIfNot;
    assignment->u.branch.cond = operation;
    assignment->u.branch.cond->flags |= IROLF_Reffed;
    assignment->rtype = type;
    assignment->u.branch.label = destination;
    IroUtil_AppendLinear(assignment, list);
}

/* Appends to INSTRUCTIONS a temporary = OBJECT * step - COUNT * step + the induction variable's update operand,
   and returns the temporary's reference. */
IROLinear *create_induction_offset_temporary(IROLinear *object, SInt32 count, IROList *instructions, IROLoop *loop)
{
    IROLinear *difference;
    Type *type;
    IROLinear *strideNode;
    IROLinear *offsetNode;
    IROLinear *product;
    IROLinear *sum;
    IROLinear *assignment;
    ENode *constant;
    Object *temporary;
    SInt32 stride;
    SInt32 offset;

    type = object->rtype;

    offsetNode = IrOptimizer_NewLinear(IROLinearOperand);
    offsetNode->index = ++linear_index_counter;
    offsetNode->rtype = type;

    constant = IrOptimizer_NewENode(EINTCONST);
    constant->rtype = type;
    offset = loop->induction->addConst * count;
    constant->data.intval.lo = offset;
    constant->data.intval.hi = (offset < 0) ? -1 : 0;
    offsetNode->u.node = constant;

    IroUtil_AppendLinear(offsetNode, instructions);
    offsetNode->flags |= IROLF_Reffed;

    strideNode = IrOptimizer_NewLinear(IROLinearOperand);
    strideNode->index = ++linear_index_counter;
    strideNode->rtype = type;

    constant = IrOptimizer_NewENode(EINTCONST);
    constant->rtype = type;
    stride = loop->induction->addConst;
    constant->data.intval.lo = stride;
    constant->data.intval.hi = (stride < 0) ? -1 : 0;
    strideNode->u.node = constant;

    IroUtil_AppendLinear(strideNode, instructions);
    strideNode->flags |= IROLF_Reffed;

    product = IrOptimizer_NewLinear(IROLinearOp2Arg);
    product->index = ++linear_index_counter;
    product->nodetype = EMUL;
    product->u.diadic.left = IroUtil_CopyLinearToList(object, instructions);
    product->u.diadic.right = strideNode;
    product->rtype = type;
    IroUtil_AppendLinear(product, instructions);
    product->flags |= IROLF_Reffed;
    product->u.diadic.left->flags &= ~IROLF_Assigned;
    product->u.diadic.left->u.monadic->flags &= ~IROLF_Assigned;

    difference = IrOptimizer_NewLinear(IROLinearOp2Arg);
    difference->index = ++linear_index_counter;
    difference->nodetype = ESUB;
    difference->u.diadic.left = product;
    difference->u.diadic.right = offsetNode;
    difference->rtype = type;
    IroUtil_AppendLinear(difference, instructions);
    difference->flags |= IROLF_Reffed;

    /* the updated variable: an increment's operand or an assignment's target */
    if (loop->induction->nd->type == IROLinearOp1Arg) {
        IroUtil_CopyLinearToList(loop->induction->nd->u.monadic, instructions);
    } else {
        IroUtil_CopyLinearToList(loop->induction->nd->u.diadic.left, instructions);
    }
    instructions->tail->flags &= ~IROLF_Assigned;
    instructions->tail->u.monadic->flags &= ~IROLF_Assigned;

    sum = IrOptimizer_NewLinear(IROLinearOp2Arg);
    sum->index = ++linear_index_counter;
    sum->nodetype = EADD;
    sum->u.diadic.left = difference;
    sum->u.diadic.right = instructions->tail;
    sum->rtype = type;
    IroUtil_AppendLinear(sum, instructions);
    sum->flags |= IROLF_Reffed;

    temporary = create_temp_object(type);
    fn_0044ba70(temporary, 1, 1);

    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    assignment->index = ++linear_index_counter;
    assignment->nodetype = EASS;
    assignment->u.diadic.left = IroUtil_AppendObjectRefAndUse(temporary, instructions);
    assignment->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.left->u.monadic->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.right = sum;
    assignment->u.diadic.right->flags |= IROLF_Reffed;
    assignment->rtype = type;
    IroUtil_AppendLinear(assignment, instructions);
    return assignment->u.diadic.left;
}

/* Appends to LIST a temporary = OBJECT * step + the induction variable, and returns the temporary's reference. */
IROLinear *create_induction_difference_temporary(IROLinear *object, SInt32 unused, IROList *list, IROLoop *loop)
{
    IROLinear *difference;
    IROLinear *constant;
    IROLinear *assignment;
    Type *type;
    IROLinear *result;
    ENode *literal;
    Object *temporary;
    SInt32 value;

    type = object->rtype;
    constant = IrOptimizer_NewLinear(IROLinearOperand);
    constant->index = ++linear_index_counter;
    constant->rtype = type;

    literal = IrOptimizer_NewENode(0x32);
    literal->rtype = type;
    value = loop->induction->addConst;
    literal->data.intval.lo = (UInt32)value;
    literal->data.intval.hi = value < 0 ? -1 : 0;
    constant->u.node = literal;

    IroUtil_AppendLinear(constant, list);
    constant->flags |= IROLF_Reffed;

    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    assignment->index = ++linear_index_counter;
    assignment->nodetype = EMUL;
    assignment->u.diadic.left = IroUtil_CopyLinearToList(object, list);
    assignment->u.diadic.right = constant;
    assignment->rtype = type;
    IroUtil_AppendLinear(assignment, list);
    assignment->flags |= IROLF_Reffed;
    assignment->u.diadic.left->flags &= ~4u;
    assignment->u.diadic.left->u.monadic->flags &= ~4u;

    /* the updated variable: an increment's operand or an assignment's target */
    if (loop->induction->nd->type == IROLinearOp1Arg)
        IroUtil_CopyLinearToList(loop->induction->nd->u.monadic, list);
    else
        IroUtil_CopyLinearToList(loop->induction->nd->u.diadic.left, list);
    list->tail->flags &= ~4u;
    list->tail->u.monadic->flags &= ~4u;

    difference = IrOptimizer_NewLinear(IROLinearOp2Arg);
    difference->index = ++linear_index_counter;
    difference->nodetype = EADD;
    difference->u.diadic.left = assignment;
    difference->u.diadic.right = list->tail;
    difference->rtype = type;
    IroUtil_AppendLinear(difference, list);
    difference->flags |= IROLF_Reffed;

    temporary = create_temp_object(type);
    fn_0044ba70(temporary, 1, 1);

    result = IrOptimizer_NewLinear(IROLinearOp2Arg);
    result->index = ++linear_index_counter;
    result->nodetype = EASS;
    result->u.diadic.left = IroUtil_AppendObjectRefAndUse(temporary, list);
    result->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    result->u.diadic.left->u.monadic->flags |= (IROLF_Assigned | IROLF_Ind);
    result->u.diadic.right = difference;
    result->u.diadic.right->flags |= IROLF_Reffed;
    result->rtype = type;
    IroUtil_AppendLinear(result, list);

    return result->u.diadic.left;
}

static void visit(IROLinear *p, IROList *b)
{
    if (p->type == IROLinearOp1Arg) {
        IroUtil_CopyLinearToList(p->u.monadic, b);
        return;
    }
    IroUtil_CopyLinearToList(p->u.diadic.left, b);
}

static ENode *newconstant(Type *ty, long n)
{
    ENode *p;
    long hi;
    p = IrOptimizer_NewENode(50);
    p->rtype = ty;
    p->data.intval.lo = n;
    hi = n < 0 ? -1 : 0;
    p->data.intval.hi = hi;
    return p;
}

static IROLinear *constantref(Type *ty, long n)
{
    IROLinear *p;
    ENode *c;
    p = IrOptimizer_NewLinear(IROLinearOperand);
    linear_index_counter += 1;
    p->index = linear_index_counter;
    p->rtype = ty;
    c = newconstant(ty, n);
    p->u.node = c;
    return p;
}

/* Appends to CHAIN a temporary = (the induction variable / VALUE + 1) * VALUE, and returns the temporary's
   reference. */
IROLinear *create_loop_ind_temporary(IROLoopInd *loop, long value, IROList *chain)
{
    IROLinear *expression;
    IROLinear *literal;
    IROLinear *assignment;
    Type *type;
    IROLinear *scaledValue;
    IROLinear *maskedValue;
    ENode *literalValue;
    IROLinear *adjustedValue;
    IROLinear *constant;
    Object *temporary;

    expression = loop->nd;
    type = expression->rtype;
    constant = constantref(type, value);
    IroUtil_AppendLinear(constant, chain);
    visit(expression, chain);
    chain->tail->flags &= ~IROLF_Assigned;
    chain->tail->u.monadic->flags &= ~IROLF_Assigned;

    adjustedValue = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter += 1;
    adjustedValue->index = linear_index_counter;
    adjustedValue->nodetype = EDIV;
    adjustedValue->u.diadic.left = chain->tail;
    adjustedValue->u.diadic.right = constant;
    adjustedValue->rtype = type;
    IroUtil_AppendLinear(adjustedValue, chain);
    adjustedValue->flags |= IROLF_Reffed;

    literal = IrOptimizer_NewLinear(IROLinearOperand);
    linear_index_counter += 1;
    literal->index = linear_index_counter;
    literalValue = IrOptimizer_NewENode(50);
    literalValue->rtype = type;
    literalValue->data.intval = cint64_one;
    literal->u.node = literalValue;
    literal->rtype = type;
    IroUtil_AppendLinear(literal, chain);
    literal->flags |= IROLF_Reffed;

    maskedValue = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter += 1;
    maskedValue->index = linear_index_counter;
    maskedValue->nodetype = EADD;
    maskedValue->u.diadic.left = adjustedValue;
    maskedValue->u.diadic.right = literal;
    maskedValue->rtype = type;
    IroUtil_AppendLinear(maskedValue, chain);
    maskedValue->flags |= IROLF_Reffed;

    IroUtil_CopyLinearToList(constant, chain);
    scaledValue = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter += 1;
    scaledValue->index = linear_index_counter;
    scaledValue->nodetype = EMUL;
    scaledValue->u.diadic.left = maskedValue;
    scaledValue->u.diadic.right = chain->tail;
    scaledValue->rtype = type;
    IroUtil_AppendLinear(scaledValue, chain);
    scaledValue->flags |= IROLF_Reffed;

    temporary = create_temp_object(type);
    fn_0044ba70(temporary, 1, 1);
    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter += 1;
    assignment->index = linear_index_counter;
    assignment->nodetype = EASS;
    assignment->u.diadic.left = IroUtil_AppendObjectRefAndUse(temporary, chain);
    assignment->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.left->u.monadic->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.right = scaledValue;
    assignment->u.diadic.right->flags |= IROLF_Reffed;
    assignment->rtype = type;
    IroUtil_AppendLinear(assignment, chain);
    return assignment->u.diadic.left;
}

/* Appends to CONTEXT a temporary = the test's bound - MULTIPLIER * step, and returns the temporary's reference. */
IROLinear *create_bound_offset_temporary(IROLinear *object, int multiplier, IROList *context, IROLoop *info)
{
    SInt32 value;
    IROLinear *bound;
    ENode *constant;
    IROLinear *result;
    IROLinear *offset;
    IROLinear *combined;
    Type *type;
    Object *temporary;

    type = object->rtype;
    offset = IrOptimizer_NewLinear(IROLinearOperand);
    offset->index = ++linear_index_counter;
    offset->rtype = type;

    constant = IrOptimizer_NewENode(0x32);
    constant->rtype = type;
    value = info->induction->addConst * multiplier;
    constant->data.intval.lo = value;
    constant->data.intval.hi = value < 0 ? -1 : 0;
    offset->u.node = constant;
    IroUtil_AppendLinear(offset, context);

    if (info->flags & 1)
        bound = IroUtil_CopyLinearToList(info->cond->u.diadic.right, context);
    else
        bound = IroUtil_CopyLinearToList(info->cond->u.diadic.left, context);

    combined = IrOptimizer_NewLinear(IROLinearOp2Arg);
    combined->index = ++linear_index_counter;
    combined->nodetype = ESUB;
    combined->u.diadic.left = bound;
    combined->u.diadic.right = offset;
    combined->rtype = type;
    IroUtil_AppendLinear(combined, context);

    temporary = create_temp_object(type);
    fn_0044ba70(temporary, 1, 1);

    result = IrOptimizer_NewLinear(IROLinearOp2Arg);
    result->index = ++linear_index_counter;
    result->nodetype = EASS;
    result->u.diadic.left = IroUtil_AppendObjectRefAndUse(temporary, context);
    result->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    result->u.diadic.left->u.monadic->flags |= (IROLF_Assigned | IROLF_Ind);
    result->u.diadic.right = combined;
    result->u.diadic.right->flags |= IROLF_Reffed;
    result->rtype = type;
    IroUtil_AppendLinear(result, context);
    return result->u.diadic.left;
}

/* Appends to LIST a temporary = the test's left operand + 1, and returns the temporary's reference. */
IROLinear *create_cond_left_add_assignment(IROList *list, IROLoop *holder)
{
    IROLinear *operation;
    IROLinear *assignment;
    Object *object;
    ENode *constant;
    Type *type;
    IROLinear *constantNode;

    type = holder->cond->u.diadic.left->rtype;

    constantNode = IrOptimizer_NewLinear(IROLinearOperand);
    linear_index_counter++;
    constantNode->index = linear_index_counter;

    constant = IrOptimizer_NewENode(EINTCONST);
    constant->rtype = type;
    constant->data.intval = cint64_one;
    constantNode->u.node = constant;
    constantNode->rtype = type;
    IroUtil_AppendLinear(constantNode, list);
    constantNode->flags |= IROLF_Reffed;

    operation = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter++;
    operation->index = linear_index_counter;
    operation->nodetype = EADD;
    operation->rtype = type;
    operation->u.diadic.left = IroUtil_CopyLinearToList(holder->cond->u.diadic.left, list);
    operation->u.diadic.left->flags |= IROLF_Reffed;
    operation->u.diadic.left->flags &= ~IROLF_Assigned;
    operation->u.diadic.left->u.monadic->flags &= ~IROLF_Assigned;
    operation->u.diadic.right = constantNode;
    IroUtil_AppendLinear(operation, list);

    object = create_temp_object(type);
    fn_0044ba70(object, 1, 1);

    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter++;
    assignment->index = linear_index_counter;
    assignment->nodetype = EASS;
    assignment->u.diadic.left = IroUtil_AppendObjectRefAndUse(object, list);
    assignment->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.left->u.monadic->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.right = operation;
    assignment->rtype = type;
    IroUtil_AppendLinear(assignment, list);
    return assignment->u.diadic.left;
}

#define Bv_IsBitSet(bit, bv) (((bit) >> 5) < (bv)->size && ((bv)->bits[(bit) >> 5] & (1 << (bit))) != 0)

static IROLinear *NewCondLinear(UInt8 type, IROList *list)
{
    IROLinear *nd = IrOptimizer_NewLinear(IROLinearOp1Arg);
    nd->index = ++linear_index_counter;
    if (type == 5)
        nd->type = IROLinearIfNot;
    else
        nd->type = IROLinearIf;
    IroUtil_AppendLinear(nd, list);
    return nd;
}

static IROLinear *NewTypedLinear(UInt8 type, IROList *list)
{
    IROLinear *nd = IrOptimizer_NewLinear(IROLinearOp1Arg);
    nd->index = ++linear_index_counter;
    nd->type = type;
    IroUtil_AppendLinear(nd, list);
    return nd;
}

static IROLinear *NewLabelLinear(IROList *list)
{
    IROLinear *lab = IrOptimizer_NewLinear(IROLinearLabel);
    lab->index = linear_index_counter++;
    lab->u.label = IroUtil_NewLabel();
    lab->flags |= 1;
    IroUtil_AppendLinear(lab, list);
    return lab;
}

static void NopOutBlock(IRONode *node)
{
    IROLinear *nd = node->first;
    IROLinear *last = node->last;
    for (; nd; nd = nd->next) {
        nd->type = IROLinearNop;
        if (nd == last)
            break;
    }
}

/* 0x555420: zero */
/* 0x555428: one */

struct IROLinear *create_loop_iteration_count(struct IROList *context, struct IROLoop *statement)
{
    Type *type;
    IROLinear *assignment;
    Object *resultObject;
    IROLinear *quotient;
    IROLinear *savedSum;
    IROLinear *minusOne;
    ENode *constant;
    SInt32 step;
    IROLinear *difference;
    IROLinear *bound;
    IROLinear *value;
    IROLinear *stepValue;
    Boolean unitBound;
    IROLinear *sum;
    IROLinear *stepNode;
    SInt32 shift;

    unitBound = 0;
    bound = statement->init->u.diadic.right;
    if (IroDump_IsType1NodeType50(bound) && CInt64_Equal(bound->u.node->data.intval, cint64_zero))
        unitBound = 1;
    if (!unitBound)
        bound = IroUtil_CopyLinearToList((bound), (context));

    if (statement->flags & 1) {
        value = IroUtil_CopyLinearToList((statement->cond->u.diadic.right), (context));
        type = statement->cond->u.diadic.right->rtype;
    } else {
        value = IroUtil_CopyLinearToList((statement->cond->u.diadic.left), (context));
        type = statement->cond->u.diadic.left->rtype;
    }

    CError_ASSERT(10087, statement->induction != 0);
    CError_ASSERT(10092, statement->induction->addConst != 0);

    stepNode = IrOptimizer_NewLinear(IROLinearOperand);
    linear_index_counter++;
    stepNode->index = linear_index_counter;
    stepNode->rtype = type;

    constant = IrOptimizer_NewENode(EINTCONST);
    constant->rtype = type;
    step = statement->induction->addConst;
    constant->data.intval.lo = step;
    constant->data.intval.hi = (step < 0) ? 0xffffffff : 0;
    stepNode->u.node = constant;

    if (unitBound)
        difference = value;
    else {
        difference = IrOptimizer_NewLinear(IROLinearOp2Arg);
        linear_index_counter++;
        difference->index = linear_index_counter;
        difference->nodetype = ESUB;
        difference->u.diadic.left = value;
        difference->u.diadic.right = bound;
        difference->rtype = type;
        IroUtil_AppendLinear(difference, context);
    }

    stepValue = IroUtil_CopyLinearToList((stepNode), (context));

    sum = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter++;
    sum->index = (UInt16)linear_index_counter;
    sum->nodetype = EADD;
    sum->u.diadic.left = difference;
    sum->u.diadic.right = stepValue;
    sum->rtype = type;
    IroUtil_AppendLinear(sum, context);

    if (statement->cond->type == IROLinearOp2Arg && statement->cond->nodetype == ELESS) {
        savedSum = sum;
        minusOne = IrOptimizer_NewLinear(IROLinearOperand);
        linear_index_counter++;
        minusOne->index = linear_index_counter;
        minusOne->rtype = type;

        constant = IrOptimizer_NewENode(EINTCONST);
        constant->rtype = type;
        constant->data.intval.lo = 0xffffffff;
        constant->data.intval.hi = 0xffffffff;
        minusOne->u.node = constant;
        IroUtil_AppendLinear(minusOne, context);

        sum = IrOptimizer_NewLinear(IROLinearOp2Arg);
        linear_index_counter++;
        sum->index = (UInt16)linear_index_counter;
        sum->nodetype = EADD;
        sum->u.diadic.left = savedSum;
        sum->u.diadic.right = minusOne;
        sum->rtype = type;
        IroUtil_AppendLinear(sum, context);
    }

    if (CInt64_Equal(stepNode->u.node->data.intval, cint64_one))
        quotient = sum;
    else {
        if (stepNode->rtype->size <= 4 && stepNode->rtype->type == TYPEINT && IroDump_IsPowerOfTwo(stepNode, &shift)) {
            quotient = IrOptimizer_NewLinear(IROLinearOp2Arg);
            linear_index_counter++;
            quotient->index = linear_index_counter;
            quotient->nodetype = ESHL;
            quotient->u.diadic.left = sum;
            quotient->u.diadic.right = stepNode;
            stepNode->u.node->data.intval.lo = shift;
        } else {
            quotient = IrOptimizer_NewLinear(IROLinearOp2Arg);
            linear_index_counter++;
            quotient->index = linear_index_counter;
            quotient->nodetype = EDIV;
            quotient->u.diadic.left = sum;
            quotient->u.diadic.right = stepNode;
        }
        quotient->rtype = type;
        IroUtil_AppendLinear(stepNode, context);
        IroUtil_AppendLinear(quotient, context);
    }

    resultObject = create_temp_object(type);
    fn_0044ba70((resultObject), 1, 1);

    assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter++;
    assignment->index = linear_index_counter;
    assignment->nodetype = EASS;
    assignment->u.diadic.left = IroUtil_AppendObjectRefAndUse(resultObject, context);
    assignment->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.left->u.diadic.left->flags |= (IROLF_Assigned | IROLF_Ind);
    assignment->u.diadic.right = quotient;
    assignment->u.diadic.right->flags |= IROLF_Reffed;
    assignment->rtype = type;
    IroUtil_AppendLinear(assignment, context);
    return assignment->u.diadic.left;
}

void IroLoop_0045c520(IROLoop *loop, CInt64 *iterationCount, int *unrollFactor, int *remainderLoop, int *unrollLoop,
                      int *exactMultiple)
{
    TypeIntegral *conditionType;
    SInt32 isUnsigned;
    UInt8 kind;
    Type *operandType;
    CInt64 factor;
    CInt64 remainder;

    isUnsigned = 0;
    operandType = (loop->flags & 1) ? loop->cond->u.diadic.right->rtype : loop->cond->u.diadic.left->rtype;
    conditionType = (TypeIntegral *)operandType;
    if (conditionType->integral == IT_UCHAR || (kind = conditionType->integral) == IT_USHORT || kind == IT_UINT ||
        kind == IT_ULONG || kind == IT_ULONGLONG)
        isUnsigned = 1;
    CInt64_SetLong(&factor, *unrollFactor);
    if (isUnsigned) {
        if (CInt64_LessEqualU(*iterationCount, factor)) {
            *remainderLoop = 0;
            *unrollLoop = 0;
            *unrollFactor = iterationCount->lo;
            return;
        }
    } else {
        if (CInt64_LessEqual(*iterationCount, factor)) {
            *remainderLoop = 0;
            *unrollLoop = 0;
            *unrollFactor = iterationCount->lo;
            return;
        }
    }
    if (iterationCount->hi == 0 && iterationCount->lo <= 8) {
        *remainderLoop = 0;
        *unrollLoop = 0;
        *unrollFactor = iterationCount->lo;
        return;
    }
    remainder = CInt64_ModU(*iterationCount, factor);
    if (CInt64_Equal(remainder, cint64_zero)) {
        *remainderLoop = 0;
        *exactMultiple = 1;
    }
}

/* Whether two address sums differ only by a constant (returned in DIFFERENCE). */
int compute_positive_addr_record_difference(IROAddrRecord *first, IROAddrRecord *second, int context,
                                            CInt64 *difference)
{
    CInt64 secondHash;
    CInt64 firstHash;
    CInt64 result;
    UInt32 index;
    IROElmList *node;
    IROLinear *elem;

    if (second->numObjRefs == first->numObjRefs && second->numObjRefs != 0)
        return 0;
    if (second->numObjRefs != first->numObjRefs)
        return 0;

    if (second->numMisc == first->numMisc && second->numMisc != 0) {
        for (index = 0; index < (UInt32)(UInt16)second->numMisc; index++) {
            if (IroUtil_LinearConstantTreesSame(first->misc->element, second->misc->element) == 0)
                return 0;
        }
    } else if (second->numMisc != first->numMisc) {
        return 0;
    }

    secondHash = cint64_zero;
    for (node = second->ints; node != NULL; node = node->next) {
        elem = node->element;
        secondHash = CMach_CalcIntDiadic(elem->rtype, secondHash, 0x2b, elem->u.node->data.intval);
    }

    firstHash = cint64_zero;
    for (node = first->ints; node != NULL; node = node->next) {
        elem = node->element;
        firstHash = CMach_CalcIntDiadic(elem->rtype, firstHash, 0x2b, elem->u.node->data.intval);
    }

    if (CInt64_Equal(secondHash, firstHash))
        return 0;
    if (CInt64_Greater(secondHash, firstHash)) {
        result = CInt64_Sub(secondHash, firstHash);
        *difference = result;
        return 1;
    }
    return 0;
}

/* The loop's iteration count, when its bounds differ by a constant. */
int compute_loop_count(IROLoop *loop, CInt64 *count)
{
    IROLinear *start;
    IROLinear *limit;
    int isUnsigned;
    IROAddrRecord *startTerms;
    IROAddrRecord *limitTerms;
    Type *limitType;
    TypeIntegral *integralType;
    CInt64 startValue;
    CInt64 limitValue;
    CInt64 step;
    CInt64 minusOne;

    isUnsigned = 0;
    start = loop->init->u.diadic.right;
    if (loop->flags & 1) {
        limit = loop->cond->u.diadic.right;
        limitType = limit->rtype;
    } else {
        limit = loop->cond->u.diadic.left;
        limitType = limit->rtype;
    }
    integralType = (TypeIntegral *)limitType;
    if (integralType->integral == IT_UCHAR || integralType->integral == IT_USHORT ||
        integralType->integral == IT_UINT || integralType->integral == IT_ULONG ||
        integralType->integral == IT_ULONGLONG)
        isUnsigned = 1;
    if (IroDump_IsType1NodeType50(start) != 0 && IroDump_IsType1NodeType50(limit) != 0) {
        startValue = start->u.node->data.intval;
        limitValue = limit->u.node->data.intval;
        if (isUnsigned != 0) {
            if (CInt64_LessEqualU(limitValue, startValue))
                return 0;
        } else {
            if (CInt64_LessEqual(limitValue, startValue))
                return 0;
        }
        step.lo = loop->induction->addConst;
        step.hi = (0 > loop->induction->addConst) ? -1 : 0;
        minusOne.lo = -1;
        minusOne.hi = -1;
        *count = CInt64_Sub(limitValue, startValue);
        *count = CInt64_Add(*count, step);
        if (loop->cond->type == IROLinearOp2Arg && loop->cond->nodetype == ELESS)
            *count = CInt64_Add(*count, minusOne);
        if (isUnsigned != 0)
            *count = CInt64_DivU(*count, step);
        else
            *count = CInt64_Div(*count, step);
        if (CInt64_Equal(*count, cint64_zero))
            return 0;
        if (isUnsigned != 0)
            CError_ASSERT(9775, !CInt64_LessEqualU(*count, cint64_zero));
        else
            CError_ASSERT(9784, !CInt64_LessEqual(*count, cint64_zero));
        return 1;
    }
    startTerms = IroVars_CreateAddrRecord(start);
    limitTerms = IroVars_CreateAddrRecord(limit);
    if (start->type == IROLinearOp2Arg && start->nodetype == EADD) {
        IroVars_CollectAddrRecordElements(start, startTerms);
    } else if (IroDump_IsType1NodeType50(start) != 0) {
        startTerms->numInts += 1;
        IroVars_PrependElmList(start, &startTerms->ints);
        startTerms->numObjRefs = 0;
        startTerms->numMisc = 0;
    } else {
        startTerms->numMisc += 1;
        IroVars_PrependElmList(start, &startTerms->misc);
        startTerms->numObjRefs = 0;
        startTerms->numInts = 0;
    }
    if (limit->type == IROLinearOp2Arg && limit->nodetype == EADD) {
        IroVars_CollectAddrRecordElements(limit, limitTerms);
    } else if (IroDump_IsType1NodeType50(limit) != 0) {
        limitTerms->numInts += 1;
        IroVars_PrependElmList(limit, &limitTerms->ints);
        limitTerms->numObjRefs = 0;
        limitTerms->numMisc = 0;
    } else {
        limitTerms->numMisc += 1;
        IroVars_PrependElmList(limit, &limitTerms->misc);
        limitTerms->numObjRefs = 0;
        limitTerms->numInts = 0;
    }
    if (compute_positive_addr_record_difference(startTerms, limitTerms, isUnsigned, count)) {
        if (loop->cond->type == IROLinearOp2Arg && loop->cond->nodetype == ELESSEQU)
            *count = CInt64_Add(*count, cint64_one);
        return 1;
    }
    return 0;
}

/* Whether the loop can be unrolled (the reasons it cannot go to the dump). */
int is_loop_unrollable(IROLoop *loop)
{
    IROLinear *operand;
    TypeIntegral *initializationType;
    CInt64 value;

    if (loop->flags & 0x8000) {
        IroDump_Print("IsLoopUnrollable:No due to LP_LOOP_HAS_ASM \n");
        return 0;
    }
    if (loop->flags & 0x0020) {
        IroDump_Print("IsLoopUnrollable:No due to LP_IFEXPR_NON_CANONICAL \n");
        return 0;
    }
    if (loop->flags & 0x0002) {
        IroDump_Print("IsLoopUnrollable:No due to LP_LOOP_HAS_CALLS \n");
        return 0;
    }
    if (loop->flags & 0x0004) {
        IroDump_Print("IsLoopUnrollable:No due to LP_LOOP_HAS_CNTRLFLOW \n");
        return 0;
    }
    if (loop->flags & 0x0010) {
        IroDump_Print("IsLoopUnrollable:No due to LP_INDUCTION_NOT_FOUND \n");
        return 0;
    }
    if (loop->flags & 0x0080) {
        IroDump_Print("IsLoopUnrollable:No due to LP_LOOP_HDR_HAS_SIDEEFFECTS \n");
        return 0;
    }
    if (!(loop->flags & 0x0200)) {
        IroDump_Print("IsLoopUnrollable:No because header does not follow induction update \n");
        return 0;
    }

    if (!(loop->flags & 0x10000)) {
        operand = loop->cond->u.diadic.right;
        if (IroDump_IsType1NodeType50(loop->cond->u.diadic.right) == 0 && (operand->flags & IROLF_LoopInvariant) == 0) {
            IroDump_Print("IsLoopUnrollable:No because Loop Upper Bound is Variant in the loop\n");
            return 0;
        }
        if (loop->init == NULL) {
            IroDump_Print("IsLoopUnrollable:No because there is no initialization of loop index in PreHeader\n");
            return 0;
        }
        if (IroDump_GetObjRef(loop->init->u.diadic.left) == NULL) {
            IroDump_Print("IsLoopUnrollable:No because initial value of induction stored thru pointer\n");
            return 0;
        }
        initializationType = (TypeIntegral *)loop->init->rtype;
        if (initializationType->integral == IT_CHAR || initializationType->integral == IT_SHORT ||
            initializationType->integral == IT_INT || initializationType->integral == IT_LONG ||
            initializationType->integral == IT_LONGLONG) {
            if (IroDump_IsType1NodeType50(loop->init->u.diadic.right) != 0) {
                ENode *literal = loop->init->u.diadic.right->u.node;
                if (!CInt64_GreaterEqual(literal->data.intval, cint64_zero)) {
                    IroDump_Print("IsLoopUnrollable:No because initial value of induction is signed but > 0\n");
                    return 0;
                }
            } else {
                if (compute_loop_count(loop, &value)) {
                    IroDump_Print("IsLoopUnrollable:Yes, the limits substract out to be constants\n");
                } else {
                    IroDump_Print(
                        "IsLoopUnrollable:No because initial value of induction is signed and not constant\n");
                    return 0;
                }
            }
        }
        if (!(loop->flags & 0x0100)) {
            IroDump_Print(
                "IsLoopUnrollable:No because LP_LOOP_STEP_ISADD  is not set i.e induciton is not updated by 1\n");
            return 0;
        }
    } else {
        initializationType = (TypeIntegral *)loop->cond->u.diadic.left->rtype;
        if (initializationType->integral == IT_CHAR || initializationType->integral == IT_SHORT ||
            initializationType->integral == IT_INT || initializationType->integral == IT_LONG ||
            initializationType->integral == IT_LONGLONG) {
            IroDump_Print("IsLoopUnrollable:No because the while loop induction is signed\n");
            return 0;
        }
        if (!(loop->flags & 0x2000)) {
            IroDump_Print("IsLoopUnrollable:No because the while loop operator is not of decrement form\n");
            return 0;
        }
    }

    if (loop->count > copts.fd1) {
        IroDump_Print("IsLoopUnrollable:No because loop size greater than threshold\n");
        return 0;
    }
    return 1;
}

void unroll_loop(int factor, struct IRONode *header)
{
    IROLinear *checkLinear;
    IRONode *adjacentNode;
    IRONode *prevpred;
    IRONode *pred;
    IRONode *fnode;
    IRONode *nextnode;
    IRONode *node1;
    IROLinear *finalLast;
    IRONode *node3;
    IRONode *node4;
    IRONode *node5;
    IRONode *node6;
    IRONode *newnode;
    IROLinear *elast;
    IROLinear *elin;
    IROLinear *econd;
    IROLinear *ecur;
    IROLinear *eend;
    IRONode *labelnode;
    VarRecord *var;
    IRONode *node2;
    IROLinear *initialCountLinear;
    IROLoop *loop;
    IROLinear *indirectLinear;
    IROLinear *addressLinear;
    IROLinear *valueLinear;
    IROLinear *convertedValue;
    IROLinear *base;
    IROLinear *addNd;
    IROLinear *mulNd;
    IROLinear *cmpNd;
    IROLinear *cmpLin;
    IROLinear *right2;
    IROLinear *left2;
    IROLinear *limit;
    IROLinear *right1;
    IROLinear *cond;
    IRONode *fnode2;
    IROLinear *last;
    IROLinear *first;
    IROLinear *lab2;
    IROLinear *lastCond;
    IROLinear *zeroOpnd;
    IROLinear *zeroCmp;
    IROLinear *oldlast;
    IROLinear *oldCondition;
    IROLinear *copy;
    IROLinear *lab;
    IROLinear *ctrlNd;
    IROLinear *incNd;
    IROLinear *decNd;
    IROLinear *dupLast;
    IROLinear *leftCopy;
    IROLinear *opnd;
    Type *type;
    ENode *enode;
    Object *object;
    Object *otherObject;
    UInt16 i;
    UInt16 j;
    IROLoopInd *ind;
    UInt16 k;
    UInt16 copies;
    UInt32 predcount;
    int foundpred;
    UInt8 count2;
    UInt8 count1;
    SInt32 offset;
    SInt32 n;
    IROLinear *nd;
    IRONode *prevsucc;
    int result;
    IROLinear *secondBoundLinear;
    CLabel *label;
    CLabel *gotoLabel;
    CLabel *label3;
    IROLinear *gotoNd;
    IROLinear *boundLinear;
    IROLinear *testLast;
    IROLinear *bodyLast;
    IROLinear *thirdBoundLinear;
    IROLinear *remLast;
    IROLinear *eremLast;
    IROLinear *initialLinear;
    IROLinear *preFirst;
    IROLinear *preLast;
    IROLinear *exitFirst;
    IROLinear *exitLast;
    IROLinear *jumpNd;
    IROLinear *plast;

    IROList list;
    CInt64 niters;
    int flag1;
    int flag2;
    int flag3;
    CInt64 step;
    CInt64 initval;
    CInt64 mask;
    CInt64 stride;
    CInt64 final;
    CInt64 trips;

    result = 0;
    nd = NULL;
    first = NULL;
    last = NULL;
    loop = NULL;
    ind = NULL;
    loop_header = header;
    compute_mustreach();
    for (var = var_records; var; var = var->next)
        var->inductionState = 1;
    fn_00460f70();
    mark_nonintersecting_linears();
    find_induction_variables();

    prevpred = NULL;
    loop_header = header;
    data_0058851c = 0;
    foundpred = 0;
    for (i = 0; i < loop_header->numpred; i++) {
        adjacentNode = iroNodesByIndex[loop_header->pred[i]];
        if (!Bv_IsBitSet(adjacentNode->index, IRO_LoopScratchVector_005880dc)) {
            foundpred = 1;
            if (adjacentNode->nextnode == header) {
                if (prevpred && adjacentNode != prevpred)
                    CError_FATAL(7599);
                prevpred = adjacentNode;
            }
        }
    }

    if (!foundpred) {
        IroDump_Print("No predecessor outside the loop\n");
        return;
    }

    if (loop_header->last->type == IROLinearIf || loop_header->last->type == IROLinearIfNot) {
        IRONode *next = loop_header->nextnode;
        if (next && !Bv_IsBitSet(next->index, IRO_LoopScratchVector_005880dc)) {
            prevsucc = NULL;
            for (j = 0; j < loop_header->numsucc; j++) {
                adjacentNode = iroNodesByIndex[loop_header->succ[j]];
                if (Bv_IsBitSet(adjacentNode->index, IRO_LoopScratchVector_005880dc)) {
                    if (prevsucc)
                        CError_FATAL(7652);
                    prevsucc = adjacentNode;
                }
            }

            pred = NULL;
            predcount = 0;
            for (k = 0; k < loop_header->numpred; k++) {
                adjacentNode = iroNodesByIndex[loop_header->pred[k]];
                if (!Bv_IsBitSet(adjacentNode->index, IRO_LoopScratchVector_005880dc)) {
                    predcount++;
                    pred = adjacentNode;
                }
            }

            if (predcount == 1 && pred->last->type == IROLinearGoto && pred->nextnode == prevsucc &&
                prevsucc != loop_header) {
                offset = 0;
                flag1 = 0;
                flag2 = 0;
                flag3 = 0;
                data_0058851c = 1;
                loop = fn_0045faa0(header);
                loop->preheader = pred;
                loop->body = prevsucc;
                find_induction_init(loop, pred);
                if (!is_loop_unrollable(loop))
                    return;

                if (loop->flags & 0x10000) {
                    IroDump_Print("while(n--) loop \n");
                    if (loop->flags & 0x800) {
                        IroDump_Print("loop not unrolled because induction used in loop \n");
                        return;
                    }
                    if (loop->flags & 0x1000) {
                        IroDump_Print("loop not unrolled because loop has multiple exits \n");
                        return;
                    }
                    if (!(loop->flags & 0x40))
                        return;

                    for (ind = induction_variables; ind; ind = ind->next) {
                        if ((ind->flags & 1) && (ind->flags & 2))
                            break;
                    }
                    if (!ind) {
                        IroDump_Print("Could not find loop with and induction with MOD and DIV operation\n");
                        return;
                    }

                    {
                        Type *t = ind->nd->rtype;
                        if (TYPE_INTEGRAL(t)->integral == IT_CHAR || TYPE_INTEGRAL(t)->integral == IT_SHORT ||
                            TYPE_INTEGRAL(t)->integral == IT_INT || TYPE_INTEGRAL(t)->integral == IT_LONG ||
                            TYPE_INTEGRAL(t)->integral == IT_LONGLONG)
                            return;
                    }

                    if (ind->nd->type == IROLinearOp2Arg) {
                        if (ind->nd->nodetype == EADDASS && fn_0044d520(ind->nd->u.diadic.right)) {
                            if (ind->addConst != 1)
                                return;
                        } else if (ind->nd->nodetype == EASS) {
                            if (!(ind->nd->u.diadic.right->type == IROLinearOp2Arg &&
                                  ind->nd->u.diadic.right->nodetype == EADD &&
                                  fn_0044d520(ind->nd->u.diadic.right->u.diadic.right)))
                                return;
                            if (ind->addConst != 1)
                                return;
                        } else {
                            return;
                        }
                    } else if (ind->nd->type == IROLinearOp1Arg) {
                        if (ind->nd->nodetype != EPREINC && ind->nd->nodetype != EPOSTINC)
                            return;
                    }

                    loop->induction = ind;
                    loop->hi = ind->nd->index;
                    loop->lo = IroUtil_GetLinearRangeStart(ind->nd)->index;

                    for (fnode = iro_flowgraph_head; fnode; fnode = fnode->nextnode) {
                        if (Bv_IsBitSet(fnode->index, IRO_LoopScratchVector_005880dc) && fnode != header) {
                            count2 = 0;
                            count1 = 0;
                            checkLinear = fnode->first;
                            if (checkLinear) {
                                for (;;) {
                                    if ((checkLinear->index < loop->lo || checkLinear->index > loop->hi) &&
                                        !(checkLinear->flags & IROLF_Reffed) && checkLinear->type != IROLinearNop &&
                                        checkLinear->type != IROLinearLabel) {
                                        if (checkLinear->type == IROLinearOp2Arg &&
                                            (checkLinear->nodetype == EORASS || checkLinear->nodetype == EANDASS ||
                                             checkLinear->nodetype == EXORASS)) {
                                            count1++;
                                            indirectLinear = checkLinear->u.diadic.left;
                                            if (indirectLinear->type == IROLinearOp1Arg &&
                                                indirectLinear->nodetype == EINDIRECT) {
                                                addressLinear = indirectLinear->u.monadic;
                                                if (addressLinear->type == IROLinearOp2Arg &&
                                                    addressLinear->nodetype == EADD) {
                                                    base = addressLinear->u.diadic.left;
                                                    type = addressLinear->rtype;
                                                    if (IroDump_GetObjRef(base)) {
                                                        mulNd = addressLinear->u.diadic.right;
                                                        if (mulNd->type == IROLinearOp2Arg && mulNd->nodetype == ESHL &&
                                                            fn_0044d520(mulNd->u.diadic.right)) {
                                                            stride = mulNd->u.diadic.right->u.node->data.intval;
                                                            addNd = mulNd->u.diadic.left;
                                                        } else {
                                                            return;
                                                        }
                                                    } else {
                                                        return;
                                                    }
                                                } else {
                                                    return;
                                                }
                                            } else {
                                                return;
                                            }

                                            valueLinear = checkLinear->u.diadic.right;
                                            if (valueLinear->type == IROLinearOp1Arg &&
                                                valueLinear->nodetype == ETYPCON) {
                                                if (checkLinear->type == IROLinearOp2Arg &&
                                                    checkLinear->nodetype == EANDASS) {
                                                    if (valueLinear->u.monadic->type == IROLinearOp1Arg &&
                                                        valueLinear->u.monadic->nodetype == EBINNOT)
                                                        cmpNd = valueLinear->u.monadic->u.monadic;
                                                    else
                                                        return;
                                                } else {
                                                    cmpNd = valueLinear->u.monadic;
                                                }
                                                if (cmpNd->type == IROLinearOp2Arg && cmpNd->nodetype == ESHL &&
                                                    fn_0044d520(cmpNd->u.diadic.left)) {
                                                    initval = cmpNd->u.diadic.left->u.node->data.intval;
                                                    limit = cmpNd->u.diadic.right;
                                                } else {
                                                    return;
                                                }
                                            } else if (valueLinear->type == IROLinearOp2Arg &&
                                                       valueLinear->nodetype == ESHL &&
                                                       checkLinear->type == IROLinearOp2Arg &&
                                                       (checkLinear->nodetype == EORASS ||
                                                        checkLinear->nodetype == EXORASS)) {
                                                cmpNd = valueLinear;
                                                if (fn_0044d520(valueLinear->u.diadic.left)) {
                                                    initval = valueLinear->u.diadic.left->u.node->data.intval;
                                                    limit = valueLinear->u.diadic.right;
                                                } else {
                                                    return;
                                                }
                                            } else if (valueLinear->type == IROLinearOp1Arg &&
                                                       valueLinear->nodetype == EBINNOT &&
                                                       checkLinear->type == IROLinearOp2Arg &&
                                                       checkLinear->nodetype == EANDASS) {
                                                cmpNd = valueLinear->u.monadic;
                                                if (cmpNd->type == IROLinearOp2Arg && cmpNd->nodetype == ESHL &&
                                                    fn_0044d520(cmpNd->u.diadic.left)) {
                                                    initval = cmpNd->u.diadic.left->u.node->data.intval;
                                                    limit = cmpNd->u.diadic.right;
                                                } else {
                                                    return;
                                                }
                                            } else {
                                                return;
                                            }

                                            if (limit->type == IROLinearOp2Arg && limit->nodetype == EAND &&
                                                addNd->type == IROLinearOp2Arg && addNd->nodetype == ESHR) {
                                                left2 = limit->u.diadic.left;
                                                object = IroDump_GetObjRef(addNd->u.diadic.left);
                                                otherObject = IroDump_GetObjRef(left2);
                                                if (object == otherObject && object == ind->var->object) {
                                                    right1 = addNd->u.diadic.right;
                                                    right2 = limit->u.diadic.right;
                                                    if (fn_0044d520(right1) && fn_0044d520(right2)) {
                                                        step = CInt64_Shl(cint64_one, right1->u.node->data.intval);
                                                        trips = CInt64_Sub(step, cint64_one);
                                                        if (CInt64_Equal(trips, right2->u.node->data.intval)) {
                                                            if (trips.hi == 0) {
                                                                factor = trips.lo + 1;
                                                                if (combine_nonoverlapping_shifts(
                                                                        CInt64_Add(trips, cint64_one), initval,
                                                                        &mask)) {
                                                                    count2++;
                                                                    if (checkLinear->type == IROLinearOp2Arg &&
                                                                        checkLinear->nodetype == EANDASS)
                                                                        mask = CFunc_LogicalNotCInt64(mask);
                                                                }
                                                            } else {
                                                                return;
                                                            }
                                                        } else {
                                                            return;
                                                        }
                                                    } else {
                                                        return;
                                                    }
                                                } else {
                                                    return;
                                                }
                                            } else {
                                                return;
                                            }
                                        } else {
                                            return;
                                        }
                                    }
                                    if (checkLinear == fnode->last)
                                        break;
                                    checkLinear = checkLinear->next;
                                }
                            }
                        }
                    }

                    if (count2 > 1 || count1 > 1)
                        return;

                    plast = pred->last;
                    gotoLabel = plast->u.label;
                    jumpNd = IroUtil_FindLabel(gotoLabel, plast);
                    IroUtil_InitList(&list);
                    IroUtil_CopyLinearRangeToList(jumpNd->next, loop_header->last->u.diadic.right->u.monadic, &list);
                    IroUtil_CopyLinearToList(loop_header->last->u.diadic.right, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    oldCondition = list.tail;

                    IroUtil_InitList(&list);
                    cond = NewCondLinear(loop_header->last->type, &list);
                    label = IroUtil_NewLabel();
                    cond->u.label = label;
                    cond->u.diadic.right = oldCondition;
                    cond->u.diadic.right->flags |= IROLF_Reffed;
                    cond->rtype = loop_header->last->rtype;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    initialLinear = create_cond_left_add_assignment(&list, loop);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    preFirst = list.head;

                    IroUtil_InitList(&list);
                    boundLinear = create_loop_ind_temporary(ind, factor, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    secondBoundLinear = create_induction_offset_temporary(initialLinear, factor, &list, loop);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    thirdBoundLinear = create_induction_difference_temporary(initialLinear, factor, &list, loop);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    create_source_constant_branch(&list, initialLinear, factor, gotoLabel);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    preLast = list.tail;

                    IroUtil_InitList(&list);
                    lab = NewLabelLinear(&list);
                    limit = (IROLinear *)lab->u.label;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    testLast = list.head;

                    first = NULL;
                    for (fnode2 = prevsucc; fnode2 && fnode2 != header; fnode2 = fnode2->nextnode) {
                        IroUtil_InitList(&list);
                        last = fnode2->last;
                        nd = fnode2->first;
                        for (;;) {
                            if (nd->stmt)
                                nd->stmt->flags |= 0x10;
                            if (nd->type != IROLinearLabel && !(nd->flags & IROLF_Reffed)) {
                                IroUtil_CopyLinearToList(nd, &list);
                                if (!first)
                                    first = list.head;
                            }
                            if (nd == last)
                                break;
                            nd = nd->next;
                        }
                        if (list.head && list.tail)
                            IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    }

                    IroUtil_InitList(&list);
                    if (ind->nd->type == IROLinearOp1Arg)
                        IroUtil_CopyLinearToList(ind->nd->u.monadic, &list);
                    else
                        IroUtil_CopyLinearToList(ind->nd->u.diadic.left, &list);
                    list.tail->flags &= ~IROLF_Assigned;
                    leftCopy = list.tail;
                    IroUtil_CopyLinearToList(boundLinear, &list);
                    list.tail->flags &= ~IROLF_Assigned;
                    cmpLin = IrOptimizer_NewLinear(IROLinearOp2Arg);
                    cmpLin->nodetype = ELESS;
                    cmpLin->rtype = (Type *)&stbool;
                    cmpLin->index = ++linear_index_counter;
                    cmpLin->next = NULL;
                    cmpLin->u.diadic.left = leftCopy;
                    cmpLin->u.diadic.right = list.tail;
                    IroUtil_AppendLinear(cmpLin, &list);
                    cmpLin->flags |= IROLF_Reffed;
                    ctrlNd = NewTypedLinear(loop_header->last->type, &list);
                    ctrlNd->u.label = (CLabel *)limit;
                    ctrlNd->u.diadic.right = cmpLin;
                    ctrlNd->u.diadic.right->flags |= IROLF_Reffed;
                    ctrlNd->rtype = loop_header->last->rtype;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    bodyLast = list.tail;

                    IroUtil_InitList(&list);
                    label3 = IroUtil_NewLabel();
                    gotoNd = IrOptimizer_NewLinear(IROLinearOp1Arg);
                    gotoNd->index = ++linear_index_counter;
                    gotoNd->type = IROLinearGoto;
                    gotoNd->u.label = label3;
                    IroUtil_AppendLinear(gotoNd, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    lab = NewLabelLinear(&list);
                    object = (Object *)lab->u.label;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    exitFirst = list.head;

                    for (i = 0; i < 8; i++) {
                        first = NULL;
                        for (fnode = prevsucc; fnode && fnode != header; fnode = fnode->nextnode) {
                            IroUtil_InitList(&list);
                            last = fnode->last;
                            nd = fnode->first;
                            n = i;
                            for (;;) {
                                if (nd->stmt)
                                    nd->stmt->flags |= 0x10;
                                if ((nd->index < loop->lo || nd->index > loop->hi) && nd->type != IROLinearLabel &&
                                    nd->type != IROLinearNop && !(nd->flags & IROLF_Reffed)) {
                                    if (!(nd->nodetype == EORASS || nd->nodetype == EANDASS || nd->nodetype == EXORASS))
                                        CError_FATAL(8537);
                                    IroUtil_CopyLinearToList(mulNd, &list);
                                    dupLast = list.tail;
                                    (void)(i != 0);
                                    final = CInt64_Shl(cint64_one, stride);
                                    opnd = IrOptimizer_NewLinear(IROLinearOperand);
                                    opnd->index = ++linear_index_counter;
                                    opnd->rtype = mulNd->rtype;
                                    enode = IrOptimizer_NewENode(EINTCONST);
                                    enode->rtype = mulNd->rtype;
                                    CInt64_SetLong(&enode->data.intval, n * final.lo);
                                    opnd->u.node = enode;
                                    IroUtil_AppendLinear(opnd, &list);
                                    IroUtil_CopyLinearToList(base, &list);
                                    incNd = IrOptimizer_NewLinear(IROLinearOp2Arg);
                                    incNd->index = ++linear_index_counter;
                                    incNd->nodetype = EADD;
                                    incNd->rtype = type;
                                    incNd->u.diadic.left = list.tail;
                                    incNd->u.diadic.right = opnd;
                                    IroUtil_AppendLinear(incNd, &list);
                                    decNd = IrOptimizer_NewLinear(IROLinearOp2Arg);
                                    decNd->index = ++linear_index_counter;
                                    decNd->nodetype = EADD;
                                    decNd->rtype = type;
                                    decNd->u.diadic.left = incNd;
                                    decNd->u.diadic.right = dupLast;
                                    IroUtil_AppendLinear(decNd, &list);
                                    indirectLinear = IrOptimizer_NewLinear(IROLinearOp1Arg);
                                    indirectLinear->index = ++linear_index_counter;
                                    indirectLinear->nodetype = EINDIRECT;
                                    indirectLinear->rtype = nd->rtype;
                                    indirectLinear->u.monadic = decNd;
                                    IroUtil_AppendLinear(indirectLinear, &list);
                                    copy = IrOptimizer_NewLinear(IROLinearOp2Arg);
                                    *copy = *nd;
                                    copy->index = ++linear_index_counter;
                                    copy->u.diadic.left = list.tail;
                                    copy->next = NULL;
                                    opnd = IrOptimizer_NewLinear(IROLinearOperand);
                                    opnd->index = ++linear_index_counter;
                                    opnd->rtype = cmpNd->rtype;
                                    enode = IrOptimizer_NewENode(EINTCONST);
                                    enode->rtype = cmpNd->rtype;
                                    opnd->u.node = enode;
                                    opnd->next = NULL;
                                    enode->data.intval = mask;
                                    (void)(factor == 32);
                                    if (nd->type == IROLinearOp2Arg && nd->nodetype == EANDASS &&
                                        CInt64_Equal(mask, cint64_zero)) {
                                        copy->nodetype = EASS;
                                    } else if (nd->type == IROLinearOp2Arg && nd->nodetype == EORASS && mask.hi == 0) {
                                        if (nd->rtype->size == 1 && mask.lo == 0xff)
                                            copy->nodetype = EASS;
                                        else if (nd->rtype->size == 2 && mask.lo == 0xffff)
                                            copy->nodetype = EASS;
                                        else if (nd->rtype->size == 4 && mask.lo == 0xffffffff)
                                            copy->nodetype = EASS;
                                    }
                                    IroUtil_AppendLinear(opnd, &list);
                                    if (valueLinear->type == IROLinearOp1Arg && valueLinear->nodetype == ETYPCON) {
                                        convertedValue = IrOptimizer_NewLinear(IROLinearOp1Arg);
                                        *convertedValue = *valueLinear;
                                        convertedValue->index = ++linear_index_counter;
                                        convertedValue->u.monadic = opnd;
                                        convertedValue->next = NULL;
                                        IroUtil_AppendLinear(convertedValue, &list);
                                    } else {
                                        convertedValue = opnd;
                                    }
                                    copy->u.diadic.right = convertedValue;
                                    IroUtil_AppendLinear(copy, &list);
                                    if (!first)
                                        first = list.head;
                                }
                                if (nd == last)
                                    break;
                                nd = nd->next;
                            }
                            if (list.head && list.tail)
                                IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                        }
                    }

                    IroUtil_InitList(&list);
                    append_scaled_induction_update(loop, factor * 8, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    exitLast = list.tail;

                    IroUtil_InitList(&list);
                    lab2 = IrOptimizer_NewLinear(IROLinearLabel);
                    lab2->index = linear_index_counter++;
                    lab2->u.label = label3;
                    lab2->flags |= 1;
                    IroUtil_AppendLinear(lab2, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    if (ind->nd->type == IROLinearOp1Arg)
                        IroUtil_CopyLinearToList(ind->nd->u.monadic, &list);
                    else
                        IroUtil_CopyLinearToList(ind->nd->u.diadic.left, &list);
                    list.tail->flags &= ~IROLF_Assigned;
                    leftCopy = list.tail;
                    IroUtil_CopyLinearToList(secondBoundLinear, &list);
                    list.tail->flags &= ~IROLF_Assigned;
                    cmpLin = IrOptimizer_NewLinear(IROLinearOp2Arg);
                    cmpLin->nodetype = ELESS;
                    cmpLin->rtype = (Type *)&stbool;
                    cmpLin->index = ++linear_index_counter;
                    cmpLin->next = NULL;
                    cmpLin->u.diadic.left = leftCopy;
                    cmpLin->u.diadic.right = list.tail;
                    IroUtil_AppendLinear(cmpLin, &list);
                    cmpLin->flags |= IROLF_Reffed;
                    ctrlNd = NewTypedLinear(loop_header->last->type, &list);
                    ctrlNd->u.label = (CLabel *)object;
                    ctrlNd->u.diadic.right = cmpLin;
                    ctrlNd->u.diadic.right->flags |= IROLF_Reffed;
                    ctrlNd->rtype = loop_header->last->rtype;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    remLast = list.tail;

                    IroUtil_InitList(&list);
                    if (ind->nd->type == IROLinearOp1Arg)
                        IroUtil_CopyLinearToList(ind->nd->u.monadic, &list);
                    else
                        IroUtil_CopyLinearToList(ind->nd->u.diadic.left, &list);
                    list.tail->flags &= ~IROLF_Assigned;
                    leftCopy = list.tail;
                    IroUtil_CopyLinearToList(thirdBoundLinear, &list);
                    list.tail->flags &= ~IROLF_Assigned;
                    cmpLin = IrOptimizer_NewLinear(IROLinearOp2Arg);
                    cmpLin->nodetype = ELESS;
                    cmpLin->rtype = (Type *)&stbool;
                    cmpLin->index = ++linear_index_counter;
                    cmpLin->next = NULL;
                    cmpLin->u.diadic.left = leftCopy;
                    cmpLin->u.diadic.right = list.tail;
                    IroUtil_AppendLinear(cmpLin, &list);
                    cmpLin->flags |= IROLF_Reffed;
                    lastCond = loop_header->last->u.diadic.right;
                    IroUtil_InsertLinearBefore(list.head, list.tail, loop_header->last);
                    loop_header->last->u.diadic.right = list.tail;

                    IroUtil_InitList(&list);
                    zeroOpnd = IrOptimizer_NewLinear(IROLinearOperand);
                    zeroOpnd->index = ++linear_index_counter;
                    enode = IrOptimizer_NewENode(EINTCONST);
                    enode->rtype = lastCond->u.monadic->rtype;
                    enode->data.intval = cint64_zero;
                    zeroOpnd->u.node = enode;
                    zeroOpnd->rtype = enode->rtype;
                    IroUtil_AppendLinear(zeroOpnd, &list);
                    zeroOpnd->flags |= IROLF_Reffed;
                    IroUtil_CopyLinearToList(lastCond->u.monadic, &list);
                    zeroCmp = IrOptimizer_NewLinear(IROLinearOp2Arg);
                    zeroCmp->nodetype = EASS;
                    zeroCmp->rtype = list.tail->rtype;
                    zeroCmp->index = ++linear_index_counter;
                    zeroCmp->next = NULL;
                    zeroCmp->u.diadic.left = list.tail;
                    zeroCmp->u.diadic.right = zeroOpnd;
                    IroUtil_AppendLinear(zeroCmp, &list);
                    zeroCmp->flags |= IROLF_Assigned;
                    IroUtil_ClearZeroOperands(lastCond);

                    nextnode = pred->nextnode;
                    oldlast = pred->last;
                    pred->last = cond;
                    node1 = CError_NewIRONode();
                    node1->first = preFirst;
                    node1->last = preLast;
                    pred->nextnode = node1;
                    node2 = CError_NewIRONode();
                    node2->first = testLast;
                    node2->last = bodyLast;
                    ((CLabel *)testLast->u.label)->target.node = node2;
                    node1->nextnode = node2;
                    node3 = CError_NewIRONode();
                    node3->first = gotoNd;
                    node3->last = gotoNd;
                    node2->nextnode = node3;
                    node4 = CError_NewIRONode();
                    node4->first = exitFirst;
                    node4->last = exitLast;
                    ((CLabel *)exitFirst->u.label)->target.node = node4;
                    node3->nextnode = node4;
                    node5 = CError_NewIRONode();
                    node5->first = lab2;
                    node5->last = remLast;
                    ((CLabel *)lab2->u.label)->target.node = node5;
                    node4->nextnode = node5;
                    node6 = CError_NewIRONode();
                    node6->first = oldlast;
                    node6->last = oldlast;
                    node5->nextnode = node6;
                    node6->nextnode = nextnode;

                    newnode = (IRONode *)CompilerTools_AllocatePoolMemory(sizeof(*newnode));
                    memset(newnode, 0, sizeof(*newnode));
                    newnode->index = iro_node_count++;
                    newnode->first = list.head;
                    newnode->last = list.tail;
                    list.tail->next = loop_header->last->next;
                    loop_header->last->next = list.head;
                    newnode->nextnode = loop_header->nextnode;
                    loop_header->nextnode = newnode;

                    labelnode = (IRONode *)CompilerTools_AllocatePoolMemory(sizeof(*labelnode));
                    memset(labelnode, 0, sizeof(*labelnode));
                    labelnode->index = iro_node_count++;
                    lab = IrOptimizer_NewLinear(IROLinearLabel);
                    lab->index = linear_index_counter++;
                    lab->next = NULL;
                    lab->u.label = label;
                    lab->flags |= 1;
                    label->target.node = labelnode;
                    labelnode->first = lab;
                    labelnode->last = lab;
                    lab->next = newnode->last->next;
                    newnode->last->next = lab;
                    labelnode->nextnode = newnode->nextnode;
                    newnode->nextnode = labelnode;
                } else {
                    gotoLabel = pred->last->u.label;
                    elin = IroUtil_FindLabel(gotoLabel, pred->last);
                    IroUtil_InitList(&list);
                    IroUtil_CopyLinearRangeToList(elin->next, loop_header->last->u.diadic.right, &list);
                    IroUtil_CopyLinearToList(loop_header->last->u.diadic.right, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    econd = list.tail;

                    IroUtil_InitList(&list);
                    cond = NewCondLinear(loop_header->last->type, &list);
                    label = IroUtil_NewLabel();
                    cond->u.label = label;
                    cond->u.diadic.right = econd;
                    cond->u.diadic.right->flags |= IROLF_Reffed;
                    cond->rtype = loop_header->last->rtype;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    n = compute_loop_count(loop, &niters);
                    flag1 = 1;
                    flag2 = 1;
                    flag3 = 0;
                    if (n)
                        IroLoop_0045c520(loop, &niters, &factor, &flag1, &flag2, &flag3);

                    IroUtil_InitList(&list);
                    initialCountLinear = create_loop_iteration_count(&list, loop);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    preFirst = list.head;

                    IroUtil_InitList(&list);
                    boundLinear = create_bound_offset_temporary(initialCountLinear, factor, &list, loop);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    create_source_constant_branch(&list, initialCountLinear, factor, gotoLabel);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    preLast = list.tail;

                    label3 = IroUtil_NewLabel();
                    IroUtil_InitList(&list);
                    gotoNd = IrOptimizer_NewLinear(IROLinearOp1Arg);
                    gotoNd->index = ++linear_index_counter;
                    gotoNd->type = IROLinearGoto;
                    gotoNd->u.label = label3;
                    IroUtil_AppendLinear(gotoNd, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    lab = NewLabelLinear(&list);
                    right1 = (IROLinear *)lab->u.label;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    exitFirst = list.head;

                    for (copies = 0; copies < factor; copies++) {
                        first = NULL;
                        elast = NULL;
                        for (fnode = prevsucc; fnode && fnode != header; fnode = fnode->nextnode) {
                            IroUtil_InitList(&list);
                            eend = fnode->last;
                            ecur = fnode->first;
                            for (;;) {
                                if (ecur->stmt)
                                    ecur->stmt->flags |= 0x10;
                                if ((ecur->index < loop->lo || ecur->index > loop->hi) &&
                                    ecur->type != IROLinearLabel && !(ecur->flags & IROLF_Reffed)) {
                                    IroUtil_CopyLinearToList(ecur, &list);
                                    if (!first)
                                        first = list.head;
                                    elast = list.tail;
                                }
                                if (ecur == eend)
                                    break;
                                ecur = ecur->next;
                            }
                            if (list.head && list.tail)
                                IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                        }
                        if (copies) {
                            offset = n = offset += loop->induction->addConst;
                            add_const_to_induction_var_references(first, elast, offset, loop);
                        }
                    }

                    IroUtil_InitList(&list);
                    append_scaled_induction_update(loop, factor, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    exitLast = list.tail;

                    IroUtil_InitList(&list);
                    lab2 = IrOptimizer_NewLinear(IROLinearLabel);
                    lab2->index = linear_index_counter++;
                    lab2->u.label = label3;
                    lab2->flags |= 1;
                    IroUtil_AppendLinear(lab2, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);

                    IroUtil_InitList(&list);
                    IroUtil_CopyLinearToList(loop_header->last->u.diadic.right->u.monadic, &list);
                    leftCopy = list.tail;
                    if (flag3)
                        IroUtil_CopyLinearToList(loop->cond->u.diadic.right, &list);
                    else
                        IroUtil_CopyLinearToList(boundLinear, &list);
                    copy = IrOptimizer_NewLinear(loop_header->last->u.diadic.right->type);
                    *copy = *loop_header->last->u.diadic.right;
                    copy->index = ++linear_index_counter;
                    copy->next = NULL;
                    copy->expr = NULL;
                    copy->u.diadic.left = leftCopy;
                    copy->u.diadic.right = list.tail;
                    IroUtil_AppendLinear(copy, &list);
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    finalLast = list.tail;

                    IroUtil_InitList(&list);
                    ctrlNd = NewTypedLinear(loop_header->last->type, &list);
                    ctrlNd->u.label = (CLabel *)right1;
                    ctrlNd->u.diadic.right = finalLast;
                    ctrlNd->u.diadic.right->flags |= IROLF_Reffed;
                    ctrlNd->rtype = loop_header->last->rtype;
                    IroUtil_InsertLinearBefore(list.head, list.tail, pred->last);
                    eremLast = list.tail;

                    nextnode = pred->nextnode;
                    oldlast = pred->last;
                    pred->last = cond;
                    node1 = CError_NewIRONode();
                    node1->first = preFirst;
                    node1->last = preLast;
                    pred->nextnode = node1;
                    node2 = CError_NewIRONode();
                    node2->first = gotoNd;
                    node2->last = gotoNd;
                    node1->nextnode = node2;
                    node3 = CError_NewIRONode();
                    node3->first = exitFirst;
                    node3->last = exitLast;
                    ((CLabel *)exitFirst->u.label)->target.node = node3;
                    if (node2)
                        node2->nextnode = node3;
                    else
                        node1->nextnode = node3;
                    node4 = CError_NewIRONode();
                    node4->first = lab2;
                    node4->last = eremLast;
                    ((CLabel *)lab2->u.label)->target.node = node4;
                    node3->nextnode = node4;
                    node5 = CError_NewIRONode();
                    node5->first = oldlast;
                    node5->last = oldlast;
                    node4->nextnode = node5;
                    node5->nextnode = nextnode;

                    labelnode = (IRONode *)CompilerTools_AllocatePoolMemory(sizeof(*labelnode));
                    memset(labelnode, 0, sizeof(*labelnode));
                    labelnode->index = iro_node_count++;
                    lab = IrOptimizer_NewLinear(IROLinearLabel);
                    lab->index = linear_index_counter++;
                    lab->next = NULL;
                    lab->u.label = label;
                    lab->flags |= 1;
                    label->target.node = labelnode;
                    labelnode->first = lab;
                    labelnode->last = lab;
                    lab->next = loop_header->last->next;
                    loop_header->last->next = lab;
                    labelnode->nextnode = loop_header->nextnode;
                    loop_header->nextnode = labelnode;

                    if (!flag1) {
                        NopOutBlock(node5);
                        NopOutBlock(header);
                        NopOutBlock(prevsucc);
                        NopOutBlock(loop->induction->node);
                        IroUtil_ClearZeroOperands(node1->last->u.diadic.right);
                        node1->last->type = IROLinearNop;
                    }
                    if (!flag2) {
                        IroUtil_ClearZeroOperands(cond->u.diadic.right);
                        cond->type = IROLinearNop;
                        IroUtil_ClearZeroOperands(node4->last->u.diadic.right);
                        node4->last->type = IROLinearNop;
                        if (node2)
                            node2->last->type = IROLinearNop;
                        for (nd = node1->first; nd; nd = nd->next) {
                            if (!(nd->flags & IROLF_Reffed))
                                IroUtil_ClearZeroOperands(nd);
                            if (nd == node1->last)
                                break;
                        }
                    }
                }
                result = 1;
            }
        }
    }

    if (result) {
        iroNodesByIndex = (IRONode **)CompilerTools_AllocatePoolMemory(iro_node_count * sizeof(*iroNodesByIndex));
        for (node6 = iro_flowgraph_head; node6; node6 = node6->nextnode)
            iroNodesByIndex[node6->index] = node6;
        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
}

/* 0x555420: 64-bit zero */
/* 0x555428: 64-bit one */

int combine_nonoverlapping_shifts(CInt64 a, CInt64 b, CInt64 *out)
{
    CInt64 term;
    CInt64 result = cint64_zero;
    CInt64 sum;
    CInt64 index = cint64_zero;

    if (CInt64_Less(cint64_zero, a)) {
        do {
            term = CInt64_Shl(b, index);
            sum = CInt64_And(term, result);
            if (CInt64_NotEqual(sum, cint64_zero))
                return 0;
            result = CExpr2_BitwiseOrCInt64(term, result);
            index = CInt64_Add(index, cint64_one);
        } while (CInt64_Less(index, a));
    }
    *out = result;
    return 1;
}

static void BitVectorInsert(BitVector *bv, unsigned bit)
{
    if ((bit >> 5) < bv->size)
        bv->bits[bit >> 5] |= 1u << bit;
    else
        CError_Internal("BitVector.h", 47);
}

unsigned int IRO_FindLoops_Unroll(void)
{
    IRONode *header;
    unsigned short predecessorIndex;
    unsigned short foundLoop;
    IRONode *predecessor;
    LoopCandidate *loop;
    LoopCandidate *outer;
    LoopCandidate *inner;

    header = iro_flowgraph_head;
    loop_candidates = NULL;
    if (iro_flowgraph_head != NULL) {
        do {
            foundLoop = 0;
            for (predecessorIndex = 0; predecessorIndex < header->numpred; predecessorIndex++) {
                predecessor = iroNodesByIndex[header->pred[predecessorIndex]];
                if ((header->index >> 5) < predecessor->dom->size &&
                    ((1 << header->index) & predecessor->dom->bits[header->index >> 5])) {
                    if (!foundLoop) {
                        IroBitVect_AllocateBitVector(&IRO_LoopScratchVector_005880dc, iro_node_count + 1);
                        IroBitVect_ClearBitVector(IRO_LoopScratchVector_005880dc);
                        BitVectorInsert(IRO_LoopScratchVector_005880dc, header->index);
                    }
                    foundLoop = 1;
                    BitVectorInsert(IRO_LoopScratchVector_005880dc, predecessor->index);
                    if (predecessor != header)
                        IRO_CollectLoopBlocks_004614f0(predecessor);
                }
            }
            if (foundLoop) {
                if (loop_candidates == NULL) {
                    loop = (LoopCandidate *)CompilerTools_AllocatePoolMemory(0x12);
                    loop->next = NULL;
                } else {
                    loop = (LoopCandidate *)CompilerTools_AllocatePoolMemory(0x12);
                    loop->next = loop_candidates;
                }
                loop_candidates = loop;
                IroBitVect_AllocateBitVector(&loop->blocks, iro_node_count + 1);
                loop->flags |= 1;
                IroBitVect_CopyBitVector(IRO_LoopScratchVector_005880dc, loop->blocks);
                loop->header = header;
                loop->value0e = 0;
            }
            header = header->nextnode;
        } while (header != NULL);
    }
    outer = loop_candidates;
    IroBitVect_AllocateBitVector(&data_005876bc, iro_node_count + 1);
    for (; outer != NULL; outer = outer->next) {
        for (inner = loop_candidates; inner != NULL; inner = inner->next) {
            if (inner == outer)
                continue;
            IroDump_Print(" header = %d \n", inner->header->index);
            IroDump_Print(" l1 bit vector=\n");
            IroDump_PrintBitSet("", inner->blocks);
            IroDump_Print(" l bit vector=\n");
            IroDump_PrintBitSet("", outer->blocks);
            if (IroBitVect_ContainsSubset(outer->blocks, inner->blocks)) {
                inner->flags &= ~1;
            }
        }
    }
    for (loop = loop_candidates; loop != NULL; loop = loop->next) {
        if (loop->flags & 1) {
            IroBitVect_CopyBitVector(loop->blocks, IRO_LoopScratchVector_005880dc);
            predecessor = loop->header;
            IroDump_Print("IRO_FindLoops_Unroll:Found loop with header %d\n", predecessor->index);
            IroDump_PrintBitSet("Loop includes: ", IRO_LoopScratchVector_005880dc);
            unroll_loop(copts.unrollOption, predecessor);
            IRO_ExpressionPropagation();
        }
    }
}

void IRO_LoopUnroller(void)

{
    data_0058800c = 1;
    IRO_FindLoops_Unroll();
    IroVars_CheckTimedLongjmp();
    return;
}

/* Analyses the loop whose test ends LOOP. */
static char lbl_00554480[] = "\nVector StatementNum = %d Int Num= %d Partition = %d";
static char lbl_005544b8[] = "<ST_OPS_PASS>";
static char lbl_005544c8[] = "<ST_OPS_FAIL>";
static char lbl_005544d8[] = "<ST_SELF_DEP>";
static char lbl_005544e8[] = "<ST_IS_DEP>";
static char lbl_005544f4[] = "<ST_VEC_PART>";
static char lbl_00554504[] = "\n";
static char lbl_00554508[] = "Dependency List:";
static char lbl_0055451c[] = "%d:";
static char lbl_00554520[] = "Weak Dependency List:";
static char lbl_00554538[] = "Store at %d and Load at %d weak dependence\n";
static char lbl_00554564[] = "Store at %d and Load at %d Cannot Vectorize\n";
static char lbl_00554594[] = "Store at %d and Load at %d Can Vectorize\n";
static char lbl_005545c0[] = "Store at %d and Store at %d Cannot Vectorize\n";
static char lbl_005545f0[] = "Store at %d and Store at %d Can Vectorize\n";

IROLoop *fn_0045faa0(IRONode *loop)
{
    IROLinear *condition;
    IROLoopInd *induction;
    UInt32 successorIndex;
    IROLoop *info;
    IROLoopInd *found;
    IRONode *block;
    IROLinear *update;
    IROLinear *constant;
    Object *testResult;
    VarRecord *lookupResult;
    UInt32 inductionCount;
    UInt8 hasFunctionCall;
    UInt8 gotoCount;
    UInt8 hasControlFlow;

    hasFunctionCall = 0;
    hasControlFlow = 0;
    gotoCount = 0;
    loop_header = loop;
    info = (IROLoop *)CompilerTools_AllocatePoolMemory(sizeof(*info));
    info->fnode = loop;
    condition = loop_header->last->u.diadic.right;
    info->cond = condition;
    info->flags = 0;
    info->x8 = 0;
    info->init = NULL;
    info->induction = NULL;
    info->lo = -1;
    info->hi = -1;
    info->count = 0;

    if (condition->type == IROLinearOp2Arg && condition->rtype->type == TYPEINT) {
        IROLinear *left;
        IROLinear *right;
        Object *testResult;
        IROLinear *operand;
        IROLoopInd *rightInduction;
        found = NULL;
        left = condition->u.diadic.left;
        right = condition->u.diadic.right;
        testResult = IroDump_GetObjRef(left);
        if (testResult != NULL) {
            lookupResult = fn_0044ba70(left->u.monadic->u.node->data.objref, 0, 1);
            if (lookupResult != NULL) {
                induction = induction_variables;
                while (induction != NULL && induction->var != lookupResult) {
                    induction = induction->next;
                }
                if (induction != NULL) {
                    found = induction;
                    info->flags |= 1;
                    info->induction = induction;
                }
            }
        }
        testResult = IroDump_GetObjRef(right);
        if (testResult != NULL) {
            lookupResult = fn_0044ba70(right->u.monadic->u.node->data.objref, 0, 1);
            if (lookupResult != NULL) {
                rightInduction = induction_variables;
                while (rightInduction != NULL && rightInduction->var != lookupResult) {
                    rightInduction = rightInduction->next;
                }
                if (rightInduction != NULL) {
                    found = rightInduction;
                    info->flags &= ~1;
                    info->induction = rightInduction;
                }
            }
        }
        if (found != NULL && 0 < found->addConst) {
            if ((info->flags & 1) != 0) {
                if (info->cond->type != IROLinearOp2Arg ||
                    (info->cond->nodetype != ELESS &&
                     (UInt8)(info->cond->nodetype - EGREATER) > EGREATEREQU - EGREATER)) {
                    info->flags |= 0x20;
                    return info;
                }
            } else {
                if (info->cond->type == IROLinearOp2Arg &&
                    (info->cond->nodetype == EGREATER || info->cond->nodetype == EGREATEREQU)) {
                    operand = info->cond->u.diadic.left;
                    info->cond->u.diadic.left = info->cond->u.diadic.right;
                    info->cond->u.diadic.right = operand;
                    if (info->cond->nodetype == EGREATER) {
                        info->cond->nodetype = ELESS;
                    } else if (info->cond->nodetype == EGREATEREQU) {
                        info->cond->nodetype = ELESSEQU;
                    }
                    info->flags |= 1;
                } else if (info->cond->type == IROLinearOp2Arg &&
                           (info->cond->nodetype == ELESS || info->cond->nodetype == ELESSEQU)) {
                    operand = info->cond->u.diadic.left;
                    info->cond->u.diadic.left = info->cond->u.diadic.right;
                    info->cond->u.diadic.right = operand;
                    if (info->cond->nodetype == ELESS) {
                        info->cond->nodetype = EGREATER;
                    } else if (info->cond->nodetype == ELESSEQU) {
                        info->cond->nodetype = EGREATEREQU;
                    }
                    info->flags |= 1;
                } else {
                    info->flags |= 0x20;
                    return info;
                }
            }
        } else {
            info->flags |= 0x10;
            return info;
        }
    } else {
        if (condition->type == IROLinearOp1Arg && condition->rtype->type == TYPEINT) {
            if (condition->nodetype == EPREINC || condition->nodetype == EPREDEC || condition->nodetype <= EPOSTDEC) {
                testResult = IroDump_GetObjRef(condition->u.monadic);
                if (testResult != NULL) {
                    lookupResult = fn_0044ba70(condition->u.monadic->u.monadic->u.node->data.objref, 0, 1);
                    if (lookupResult != NULL) {
                        induction = induction_variables;
                        while (induction != NULL && induction->var != lookupResult) {
                            induction = induction->next;
                        }
                        if (induction != NULL) {
                            found = induction;
                            info->flags |= 0x10000;
                            info->induction = induction;
                        } else {
                            info->flags |= 0x10;
                            return info;
                        }
                    } else {
                        info->flags |= 0x10;
                        return info;
                    }
                } else {
                    info->flags |= 0x10;
                    return info;
                }
            } else {
                info->flags |= 0x10;
                return info;
            }
        } else {
            info->flags |= 0x20;
            return info;
        }
    }

    inductionCount = 0;
    induction = induction_variables;
    if (induction != NULL) {
        do {
            induction = induction->next;
            inductionCount++;
        } while (induction != NULL);
    }
    if (1 < inductionCount) {
        info->flags |= 0x40;
    }
    if (info->induction != NULL) {
        IROLoopInd *selectedInduction;
        selectedInduction = info->induction;
        update = selectedInduction->nd;
        if (update->type == IROLinearOp2Arg) {
            if (info->cond->type == IROLinearOp2Arg &&
                (info->cond->nodetype == ELESS || info->cond->nodetype == ELESSEQU)) {
                if (update->nodetype == EADDASS) {
                    if (selectedInduction->addConst == 1) {
                        info->flags |= 0x100;
                    }
                    if (0 < selectedInduction->addConst) {
                        info->flags |= 0x400;
                    }
                } else if (update->nodetype == EASS && update->u.diadic.right->type == IROLinearOp2Arg &&
                           update->u.diadic.right->nodetype == EADD) {
                    if (fn_0044d520(constant = update->u.diadic.right->u.diadic.right) &&
                        constant->u.node->data.intval.hi == 0) {
                        if (constant->u.node->data.intval.lo == 1) {
                            info->flags |= 0x100;
                        }
                        if (constant->u.node->data.intval.lo != 0) {
                            info->flags |= 0x400;
                        }
                    }
                }
            }
        } else if (update->type == IROLinearOp1Arg) {
            if (update->nodetype == EPREINC || update->nodetype == EPOSTINC) {
                if (selectedInduction->addConst == 1) {
                    info->flags |= 0x100;
                }
                if (0 < selectedInduction->addConst) {
                    info->flags |= 0x400;
                }
            }
            if (update->nodetype == EPREDEC || update->nodetype == EPOSTDEC) {
                if (selectedInduction->addConst == 1) {
                    info->flags |= 0x2000;
                }
                if (0 < selectedInduction->addConst) {
                    info->flags |= 0x4000;
                }
            }
        }
        info->hi = update->index;
        info->lo = IroUtil_GetLinearRangeStart(update)->index;
    }

    if (found != NULL) {
        IROLinear *end;
        IROLinear *prefix;
        IROLinear *suffix;
        IROLinear *scan;
        end = IroUtil_GetLinearRangeStart(loop->last->u.branch.cond);
        info->flags |= 0x200;
        if ((info->flags & 0x10000) != 0) {
            for (prefix = info->fnode->first; prefix != NULL && prefix != end; prefix = prefix->next) {
                if (prefix->type != IROLinearLabel && prefix->type != IROLinearNop) {
                    info->flags &= ~0x200;
                }
            }
        } else {
            for (suffix = found->nd->next; suffix != NULL && suffix != end; suffix = suffix->next) {
                if (suffix->type != IROLinearLabel && suffix->type != IROLinearNop) {
                    info->flags &= ~0x200;
                }
            }
            for (scan = info->fnode->first; scan != NULL && scan != end; scan = scan->next) {
                if ((scan->index < info->lo || scan->index > info->hi) &&
                    (scan->type != IROLinearLabel && scan->type != IROLinearNop)) {
                    info->flags &= ~0x200;
                }
            }
        }
    }

    block = iro_flowgraph_head;
    {
        Object *assignmentObject;
        IROLoopInd *assignmentInduction;
        IROLinear *linear;
        while (block != NULL) {
            if ((block->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
                (IRO_LoopScratchVector_005880dc->bits[block->index >> 5] & (1 << block->index)) != 0 && block != loop) {
                data_00587c68 = block;
                linear = block->first;
                if (linear != NULL) {
                    for (;;) {
                        if (linear->type == IROLinearFunccall) {
                            hasFunctionCall = 1;
                        }
                        if (linear->type == IROLinearGoto) {
                            gotoCount++;
                        }
                        if (1 <= gotoCount) {
                            hasControlFlow = 1;
                        }
                        if (linear->type == IROLinearSwitch || (UInt8)(linear->type - 5) <= 2) {
                            hasControlFlow = 1;
                        }
                        if (linear->type == IROLinearAsm) {
                            info->flags |= 0x8000;
                        }
                        if ((linear->flags & IROLF_Reffed) == 0 && linear->type != IROLinearNop &&
                            linear->type != IROLinearLabel && data_00551d6c[linear->nodetype] == 0) {
                            info->flags |= 8;
                        }
                        if (linear->index < info->lo || linear->index > info->hi) {
                            {
                                Object *referencedObject;
                                UInt32 referenceFlags;
                                if (linear->type == IROLinearOperand &&
                                    (update = linear->u.diadic.left)->type == 0x38) {
                                    referencedObject = (Object *)update->stmt;
                                    if (((referenceFlags = linear->flags) & 0x20) != 0 && referencedObject != NULL &&
                                        referencedObject == info->induction->var->object &&
                                        ((referenceFlags & 4) == 0 || (referenceFlags & 0x10) != 0)) {
                                        IroDump_Print("Induction Used in loop\n");
                                        info->flags |= 0x800;
                                    }
                                }
                            }
                            if (linear->type == IROLinearOp2Arg && linear->nodetype == ESHR &&
                                (assignmentObject = IroDump_GetObjRef(linear->u.diadic.left)) != NULL &&
                                IroDump_IsType1NodeType50(linear->u.diadic.right) != 0) {
                                for (assignmentInduction = induction_variables; assignmentInduction != NULL;
                                     assignmentInduction = assignmentInduction->next) {
                                    if (assignmentInduction->var->object == assignmentObject) {
                                        IroDump_Print("Induction has DIV: %s\n", assignmentObject->name->name);
                                        assignmentInduction->flags |= 2;
                                    }
                                }
                            }
                            {
                                Object *comparisonObject;
                                IROLoopInd *comparisonInduction;
                                if (linear->type == IROLinearOp2Arg && linear->nodetype == EAND &&
                                    (comparisonObject = IroDump_GetObjRef(linear->u.diadic.left)) != NULL &&
                                    IroDump_IsType1NodeType50(linear->u.diadic.right) != 0) {
                                    comparisonInduction = induction_variables;
                                    while (comparisonInduction != NULL && comparisonObject != NULL) {
                                        if (comparisonInduction->var->object == comparisonObject) {
                                            IroDump_Print("Induction has MOD: %s\n", comparisonObject->name->name);
                                            comparisonInduction->flags |= 1;
                                        }
                                        comparisonInduction = comparisonInduction->next;
                                    }
                                }
                            }
                        }
                        info->count++;
                        if (linear == block->last) {
                            break;
                        }
                        linear = linear->next;
                    }
                }
            }
            if (hasFunctionCall) {
                info->flags |= 2;
            }
            if (hasControlFlow) {
                info->flags |= 4;
            }
            successorIndex = 0;
            while (successorIndex < block->numsucc && block != loop) {
                if ((block->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
                    (IRO_LoopScratchVector_005880dc->bits[block->index >> 5] & (1 << block->index)) != 0) {
                    if (!((block->succ[successorIndex] >> 5) < IRO_LoopScratchVector_005880dc->size &&
                          (IRO_LoopScratchVector_005880dc->bits[block->succ[successorIndex] >> 5] &
                           (1 << block->succ[successorIndex])) != 0)) {
                        IroDump_Print("Node %d has an out of loop successor %d\n", block->index,
                                      block->succ[successorIndex]);
                        IroDump_PrintBitSet("Loop includes: ", IRO_LoopScratchVector_005880dc);
                        IroDump_Print("loop has multiple exits\n");
                        info->flags |= 0x1000;
                    }
                }
                successorIndex++;
            }
            block = block->nextnode;
        }
    }
    return info;
}

void find_induction_init(struct IROLoop *state, struct IRONode *list)
{
    IROLinear *item;
    UInt32 bit;
    UInt32 idx;
    UInt32 mask;

    if (state->induction == NULL) {
        state->init = NULL;
        return;
    }

    bit = state->induction->var->index;
    item = list->first;
    if (item != NULL) {
        idx = bit >> 5;
        mask = 1u << (bit & 0x1f);
        do {
            IroBitVect_ClearBitVector(data_00588018);
            IroVars_0044b2d0(item);
            if (idx < data_00588018->size && (data_00588018->bits[idx] & mask) != 0)
                state->init = item;
            if (item == list->last)
                break;
            item = item->next;
        } while (item != NULL);
    }

    if (state->init != NULL && state->init->type != IROLinearOp2Arg)
        state->init = NULL;
}

#define IN_SET(v)                                                                                                      \
    (((v) >> 5) < IRO_LoopScratchVector_005880dc->size &&                                                              \
     (IRO_LoopScratchVector_005880dc->bits[(v) >> 5] & (1 << ((v) & 31))) != 0)

void reduce_strength_and_move_loop_invariants(IRONode *func)
{
    IROExpr *entry;
    IROExpr *nextEntry;
    IROExpr *available;
    IROLinear *expression;
    VarRecord *key;
    IROLinear *bases;
    UInt8 matched;
    IRONode *functionNode;
    IRONode *candidate;
    SInt32 found;
    UInt16 i;
    IRONode *node;
    IROExpr *previousExpr;
    IROExpr *nextUse;
    IROExpr *memberUse;
    IROLoopInd *info;
    IROExpr *matchingEntry;

    expr_list = NULL;
    loop_header = func;
    compute_mustreach();
    for (key = var_records; key != NULL; key = key->next)
        key->inductionState = 1;
    fn_00460f70();
    mark_nonintersecting_linears();
    find_induction_variables();
    functionNode = NULL;
    loop_header = func;
    data_0058851c = 0;
    found = 0;
    candidate = NULL;
    for (i = 0; i < loop_header->numpred; i++) {
        node = iroNodesByIndex[loop_header->pred[i]];
        if (!IN_SET(node->index)) {
            if (found == 0)
                candidate = node;
            else
                candidate = NULL;
            found = 1;
            if (node->nextnode == func) {
                if (functionNode != NULL && node != functionNode)
                    CError_FATAL(2516);
                functionNode = node;
            }
        }
    }
    if (found == 0) {
        IroDump_Print("No predecessor outside the loop\n");
        return;
    }
    if (candidate == NULL || candidate->last->type != IROLinearGoto)
        candidate = insert_loop_preheader(func, functionNode);
    loop_candidate_last = candidate->last;
    if (loop_candidate_last->type == IROLinearGoto) {
        if (loop_candidate_last->u.label != loop_header->first->u.label) {
            if (data_0058066a == 0 || data_00580674 == 0) {
                for (functionNode = iro_flowgraph_head; functionNode != NULL; functionNode = functionNode->nextnode)
                    functionNode->mustreach = 0;
            } else {
                loop_candidate_last->u.label = loop_header->first->u.label;
                IroFlowgraph_RebuildSuccPred();
            }
        }
    }
    if (copts.fc5 != 0 || copts.fc8 != 0) {
        rewrite_selected_monadic_references();
        IroCSE_BuildLoopExprList();
        IroDump_DumpExpressions();
    }
    if (copts.fc5 != 0) {
        for (previousExpr = expr_list; previousExpr != NULL; previousExpr = previousExpr->next) {
            if (IN_SET(previousExpr->node->index) && previousExpr->hasSideEffects == 0 &&
                (previousExpr->mayTrap == 0 || previousExpr->node->mustreach != 0) &&
                (previousExpr->linear->flags & IROLF_LoopInvariant) != 0) {
                IroUtil_VisitLinearTree(previousExpr->linear, (void (*)(IROLinear *, int))fn_0044e340);
                previousExpr->linear->flags = previousExpr->linear->flags | 0x100;
            }
        }
    }
    if (copts.uniformSpillBlockWeight == 0 && copts.fc8 != 0) {
        for (previousExpr = expr_list; previousExpr != NULL; previousExpr = previousExpr->next) {
            if (IN_SET(previousExpr->node->index) && previousExpr->hasSideEffects == 0 &&
                (previousExpr->mayTrap == 0 || previousExpr->node->mustreach != 0)) {
                if (match_induction_expression(previousExpr->linear, &expression, &bases, &key) != 0) {
                    IroUtil_VisitLinearTree(previousExpr->linear, (void (*)(IROLinear *, int))fn_00461dc0);
                    previousExpr->linear->flags = previousExpr->linear->flags | 0x800;
                    previousExpr->factor = expression;
                    previousExpr->var = key;
                    previousExpr->expr = bases;
                }
            }
        }
    }
    if (copts.uniformSpillBlockWeight == 0 && copts.fc8 != 0) {
        iro_loop_roots = NULL;
        memberUse = expr_list;
        while (memberUse != NULL) {
            nextUse = memberUse->next;
            if ((memberUse->linear->flags & IROLF_Ris) != 0) {
                expression = memberUse->factor;
                key = memberUse->var;
                bases = memberUse->expr;
                for (info = induction_variables; info != NULL && info->var != key; info = info->next)
                    ;
                if (info == NULL)
                    CError_FATAL(3410);
                fn_0044f350(expression);
                if (IroBitVect_Intersects(data_00552b88, data_0058064c) == 0) {
                    IroDump_Print("Found reduction in strength: %d\n", memberUse->linear->index);
                    if (memberUse->linear->type == IROLinearOp2Arg && memberUse->linear->nodetype == ESHL) {
                        memberUse->linear->nodetype = EMUL;
                        expression->u.node->data.intval.lo = 1 << expression->u.node->data.intval.lo;
                        expression->u.node->rtype = memberUse->linear->rtype;
                    }
                    matchingEntry = iro_loop_roots;
                    while (1) {
                        if (matchingEntry == NULL)
                            break;
                        if (IroUtil_LinearConstantTreesSame(memberUse->linear, matchingEntry->linear) != 0)
                            break;
                        matchingEntry = matchingEntry->next;
                    }
                    if (matchingEntry != NULL) {
                        IroDump_Print("Using existing RIS: %d\n", matchingEntry->linear->index);
                        IroUtil_VisitLinearTree(memberUse->linear, forward_expr_if_check_object);
                    } else {
                        IROExpr *result = IroLoop_00461860(memberUse, bases, expression, info, 0);
                        matchingEntry = result;
                    }
                    IroCSE_ReplaceReference(memberUse->linear, matchingEntry->temp, memberUse->linear);
                    IroUtil_RemoveLinearRange(memberUse);
                }
            }
            memberUse = nextUse;
        }
    }
    iro_loop_roots = NULL;
    available = NULL;
    entry = expr_list;
    if (copts.fc5 != 0) {
        for (; entry != NULL; entry = nextEntry) {
            nextEntry = entry->next;
            matched = 0;
            if (IN_SET(entry->node->index) && entry->hasSideEffects == 0 &&
                (entry->mayTrap == 0 || entry->node->mustreach != 0) &&
                (entry->linear->flags & IROLF_LoopInvariant) != 0) {
                IroDump_Print("Found loop invariant: %d\n", entry->linear->index);
                for (previousExpr = available; previousExpr != NULL; previousExpr = previousExpr->next) {
                    if (IroUtil_LinearConstantTreesSame(previousExpr->linear, entry->linear) != 0) {
                        IroCSE_ReplaceReference(entry->linear, previousExpr->temp, entry->linear);
                        IroUtil_ClearZeroOperands(entry->linear);
                        IroDump_Print("Using already removed expr: %d\n", previousExpr->linear->index);
                        IroCSE_RemoveExpr(entry);
                        matched = 1;
                    }
                }
                if (matched == 0 && entry->temp == NULL) {
                    IroCSE_CreateTemp(entry);
                    IroCSE_ReplaceReference(entry->linear, entry->temp, entry->linear);
                    IroUtil_MoveExprBefore(entry, loop_candidate_last);
                    IroCSE_CreateTempAssignment(entry);
                    IroCSE_RemoveExpr(entry);
                    entry->next = available;
                    available = entry;
                }
            }
        }
    }
}

static char lbl_00554724[] = "IRO_LoopVectorizer:Found loop with header %d\n";

void find_induction_variables(void)
{
    IROLinear *rhs;
    SInt32 isUnsigned;
    Boolean isInduction;
    UInt8 state;
    IROLinear *stmt;
    VarRecord *variable;
    VarRecord *var;
    IRONode *candidateLoop;
    IRONode *loop;
    TypeIntegral *type;
    Object *object;
    Object *target;

    IroBitVect_AllocateBitVector(&data_0058064c, iroVarCount + 1);
    IroBitVect_AllocateBitVector(&data_00588018, iroVarCount + 1);

    for (loop = iro_flowgraph_head; loop != NULL; loop = loop->nextnode) {
        if ((loop->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
            (IRO_LoopScratchVector_005880dc->bits[loop->index >> 5] & (1 << loop->index)) != 0) {
            stmt = loop->first;
            if (stmt != NULL) {
                for (;;) {
                    IroBitVect_ClearBitVector(data_00588018);
                    IroVars_0044b2d0(stmt);
                    IroBitVect_Or(data_00588018, data_0058064c);
                    isInduction = 0;
                    if ((stmt->type == IROLinearOp2Arg && stmt->rtype->size <= 4 &&
                         (stmt->nodetype == EADDASS || stmt->nodetype == ESUBASS) &&
                         (target = IroDump_GetObjRef(stmt->u.diadic.left)) != NULL &&
                         target->type->size == stmt->rtype->size && target->type->type == TYPEINT &&
                         ((fn_0044d520(stmt->u.diadic.right) != 0 &&
                           stmt->u.diadic.right->u.node->data.intval.lo != 0) ||
                          (stmt->u.diadic.right->flags & IROLF_LoopInvariant))) ||
                        (stmt->type == IROLinearOp1Arg && stmt->rtype->size <= 4 &&
                         (stmt->nodetype == EPOSTINC || stmt->nodetype == EPOSTDEC || stmt->nodetype == EPREINC ||
                          stmt->nodetype == EPREDEC) &&
                         (target = IroDump_GetObjRef(stmt->u.diadic.left)) != NULL &&
                         target->type->size == stmt->rtype->size && target->type->type == TYPEINT)) {
                        isInduction = 1;
                    } else if (stmt->type == IROLinearOp2Arg && stmt->rtype->size <= 4 && stmt->nodetype == EASS &&
                               (target = IroDump_GetObjRef(stmt->u.diadic.left)) != NULL &&
                               target->type->size == stmt->rtype->size && target->type->type == TYPEINT &&
                               stmt->u.diadic.right->type == IROLinearOp2Arg &&
                               (stmt->u.diadic.right->nodetype == EADD || stmt->u.diadic.right->nodetype == ESUB)) {
                        Object *left;
                        Object *right;
                        if (stmt->u.diadic.right->nodetype == EADD) {
                            left = IroDump_GetObjRef(stmt->u.diadic.right->u.diadic.left);
                            right = IroDump_GetObjRef(stmt->u.diadic.right->u.diadic.right);
                            if (left == target && right != target &&
                                (stmt->u.diadic.right->u.diadic.right->flags & IROLF_LoopInvariant))
                                isInduction = 1;
                            if (right == target && left != target &&
                                (stmt->u.diadic.right->u.diadic.left->flags & IROLF_LoopInvariant)) {
                                IROLinear *temp;
                                isInduction = 1;
                                temp = stmt->u.diadic.right->u.diadic.left;
                                stmt->u.diadic.right->u.diadic.left = stmt->u.diadic.right->u.diadic.right;
                                stmt->u.diadic.right->u.diadic.right = temp;
                            }
                        } else {
                            left = IroDump_GetObjRef(stmt->u.diadic.right->u.diadic.left);
                            right = IroDump_GetObjRef(stmt->u.diadic.right->u.diadic.right);
                            if (left == target && right != target &&
                                (stmt->u.diadic.right->u.diadic.right->flags & IROLF_LoopInvariant))
                                isInduction = 1;
                        }
                    }
                    if (isInduction) {
                        variable = IroVars_GetOperandVarRecord(stmt);
                        if (variable != NULL) {
                            if ((state = variable->inductionState) == 2)
                                variable->inductionState = 0;
                            else if (state == 1)
                                variable->inductionState = 2;
                        }
                    } else {
                        for (var = var_records; var != NULL; var = var->next) {
                            if ((var->index >> 5) < data_00588018->size &&
                                (data_00588018->bits[var->index >> 5] & (1u << (var->index & 31))) != 0)
                                var->inductionState = 0;
                        }
                    }
                    if (stmt == loop->last)
                        break;
                    stmt = stmt->next;
                }
            }
        }
    }

    IroDump_PrintBitSet("Killed in loop: ", data_0058064c);

    candidateLoop = iro_flowgraph_head;
    induction_variables = NULL;
    if (candidateLoop != NULL) {
        do {
            if ((candidateLoop->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
                (IRO_LoopScratchVector_005880dc->bits[candidateLoop->index >> 5] & (1 << candidateLoop->index)) != 0) {
                stmt = candidateLoop->first;
                if (stmt != NULL) {
                    for (;;) {
                        if ((stmt->type == IROLinearOp2Arg &&
                             (stmt->nodetype == EADDASS || stmt->nodetype == ESUBASS || stmt->nodetype == EASS)) ||
                            (stmt->type == IROLinearOp1Arg &&
                             (stmt->nodetype == EPOSTINC || stmt->nodetype == EPOSTDEC || stmt->nodetype == EPREINC ||
                              stmt->nodetype == EPREDEC))) {
                            variable = IroVars_GetOperandVarRecord(stmt);
                            if (variable != NULL && variable->inductionState == 2 &&
                                (object = variable->object)->type->size <= 4) {
                                IROLoopInd *induction;
                                induction = CompilerTools_AllocatePoolMemory(sizeof(IROLoopInd));
                                induction->node = candidateLoop;
                                induction->var = variable;
                                induction->nd = stmt;
                                induction->next = induction_variables;
                                induction->step = NULL;
                                induction->addConst = 0;
                                induction->flags = 0;
                                if (stmt->type == IROLinearOp2Arg) {
                                    isUnsigned = 0;
                                    type = (TypeIntegral *)stmt->rtype;
                                    if (type->integral == IT_UCHAR || type->integral == IT_USHORT ||
                                        type->integral == IT_UINT || type->integral == IT_ULONG ||
                                        type->integral == IT_ULONGLONG)
                                        isUnsigned = 1;
                                    if (fn_0044d520(stmt->u.diadic.right) != 0) {
                                        CInt64 value;
                                        value = stmt->u.diadic.right->u.node->data.intval;
                                        if (stmt->type == IROLinearOp2Arg && stmt->nodetype == EADDASS &&
                                            CInt64_Less(value, cint64_zero)) {
                                            stmt->nodetype = ESUBASS;
                                            stmt->u.diadic.right->u.node->data.intval = CInt64_Inv(value);
                                        }
                                        if (isUnsigned) {
                                            rhs = stmt->u.diadic.right;
                                            CExpr2_ClearCInt64Hi(&rhs->u.node->data.intval);
                                        } else
                                            CExpr2_SignExtendCInt64(&(rhs = stmt->u.diadic.right)->u.node->data.intval);
                                        induction->addConst = stmt->u.diadic.right->u.node->data.intval.lo;
                                        induction->step = NULL;
                                    } else {
                                        if (stmt->nodetype == EADDASS || stmt->nodetype == ESUBASS)
                                            induction->step = stmt->u.diadic.right;
                                        else if (stmt->nodetype == EASS)
                                            induction->step = stmt->u.diadic.right->u.diadic.right;
                                    }
                                } else {
                                    if (stmt->rtype->type == TYPEPOINTER)
                                        induction->addConst = ((TypePointer *)stmt->rtype)->target->size;
                                    else
                                        induction->addConst = 1;
                                    induction->step = NULL;
                                }
                                induction_variables = induction;
                                if (stmt->type == IROLinearOp2Arg &&
                                    (stmt->nodetype == EADDASS || stmt->nodetype == ESUBASS)) {
                                    if (stmt->u.diadic.right->flags & IROLF_LoopInvariant)
                                        IroDump_Print("Found induction variable the new way: %s\n",
                                                      variable->object->name->name);
                                    else
                                        IroDump_Print("Found induction variable the old way: %s\n",
                                                      variable->object->name->name);
                                } else if (stmt->type == IROLinearOp2Arg && (stmt->u.diadic.right->nodetype == EADD ||
                                                                             stmt->u.diadic.right->nodetype == ESUB)) {
                                    IroDump_Print("Found induction variable the new way: %s\n",
                                                  variable->object->name->name);
                                } else {
                                    IroDump_Print("Found induction variable the old way: %s\n",
                                                  variable->object->name->name);
                                }
                            }
                        }
                        if (stmt == candidateLoop->last)
                            break;
                        stmt = stmt->next;
                    }
                }
            }
        } while ((candidateLoop = candidateLoop->nextnode) != NULL);
    }
}

void mark_nonintersecting_linears(void)
{
    IRONode *cls;
    IROLinear *func;

    data_00552b88 = data_00588018;
    for (cls = iro_flowgraph_head; cls != NULL; cls = cls->nextnode) {
        if ((cls->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
            (IRO_LoopScratchVector_005880dc->bits[cls->index >> 5] & (1 << cls->index)) != 0) {
            func = cls->first;
            if (func != NULL) {
                func->flags = func->flags & ~IROLF_LoopInvariant;
                do {
                    if ((func->flags & IROLF_Reffed) != 0 && func->type != IROLinearNop) {
                        IroCSE_CollectExpressionVarRefsAndFlags(func);
                        if (DAT_005880a4 == 0 && IroBitVect_Intersects(data_00552b88, data_0058064c) == 0) {
                            func->flags = func->flags | 0x100;
                        }
                    }
                    if (func == cls->last)
                        break;
                    func = func->next;
                } while (1);
            }
        }
    }
}

#define BVTEST(v, idx) ((UInt32)((idx) >> 5) < (v)->size && ((v)->bits[(idx) >> 5] & (UInt32)(1 << ((idx) & 31))) != 0)

static void BVSET(BitVector *bv, UInt32 index)
{
    if ((index >> 5) < bv->size)
        bv->bits[index >> 5] |= (UInt32)(1 << (index & 31));
    else
        CError_Internal("BitVector.h", 0x2f);
}

#define BV_TEST(v, bit)                                                                                                \
    (((UInt32)((bit) >> 5) < (v)->size) && (((v)->bits[(UInt32)((bit) >> 5)] & (1u << ((bit) & 31))) != 0))

static inline void BV_SET(BitVector *v, unsigned int bit)
{
    if (((UInt32)(bit)) >> 5 < (v)->size)
        (v)->bits[((UInt32)(bit)) >> 5] |= 1u << ((bit) & 31);
    else
        CError_Internal("BitVector.h", 47);
}

void fn_00460f70(void)
{
    IRONode *node;
    IROLinear *item;

    IroBitVect_AllocateBitVector(&data_0058064c, iroVarCount + 1);
    IroBitVect_AllocateBitVector(&data_00588018, iroVarCount + 1);
    for (node = iro_flowgraph_head; node != NULL; node = node->nextnode) {
        if ((node->index >> 5) >= IRO_LoopScratchVector_005880dc->size)
            continue;
        if (!((1 << node->index) & IRO_LoopScratchVector_005880dc->bits[node->index >> 5]))
            continue;
        item = node->first;
        if (item == NULL)
            continue;
        IroBitVect_AllocateBitVector(&node->in, iroVarCount + 1);
        IroBitVect_AllocateBitVector(&node->gen, iroVarCount + 1);
        for (;;) {
            IroBitVect_ClearBitVector(data_00588018);
            IroVars_0044b2d0(item);
            IroBitVect_Or(data_00588018, data_0058064c);
            if (item == node->last)
                break;
            item = item->next;
        }
    }
}

static char lbl_005547c0[] = "Subable Expression is %d\n";

void IRO_FindLoops(void)
{
    IRONode *loop;
    IRONode *block;
    IRONode *p;
    UInt16 i;
    UInt16 found;
    IROExpr *prev;
    IROExpr *node;

    loop = iro_flowgraph_head;
    while (loop != NULL) {
        found = 0;
        for (i = 0; i < loop->numpred; i++) {
            block = iroNodesByIndex[*(loop->pred + i)];
            if (BVTEST(block->dom, loop->index)) {
                if (!found) {
                    IroBitVect_AllocateBitVector(&IRO_LoopScratchVector_005880dc, iro_node_count + 1);
                    IroBitVect_ClearBitVector(IRO_LoopScratchVector_005880dc);
                    BVSET(IRO_LoopScratchVector_005880dc, loop->index);
                }
                found = 1;
                BVSET(IRO_LoopScratchVector_005880dc, block->index);
                if (block != loop)
                    IRO_CollectLoopBlocks_004614f0(block);
            }
        }
        if (found) {
            for (p = iro_flowgraph_head; p != NULL; p = p->nextnode) {
                if (BVTEST(IRO_LoopScratchVector_005880dc, p->index))
                    p->loopdepth++;
            }
            IroDump_Print("IRO_FindLoops:Found loop with header %d\n", loop->index);
            IroDump_PrintBitSet("Loop includes: ", IRO_LoopScratchVector_005880dc);
            reduce_strength_and_move_loop_invariants(loop);
            node = expr_list;
            prev = NULL;
            while (node != NULL) {
                /* candidates whose expression was removed */
                if (node->linear->type == IROLinearNop) {
                    if (prev != NULL)
                        prev->next = node->next;
                    else
                        expr_list = node->next;
                } else {
                    prev = node;
                }
                node = node->next;
            }
            IRO_ExpressionPropagation();
            IroVars_BuildNoregisterBitVector();
        }
        loop = loop->nextnode;
    }
    if (data_00588526 == 0 && data_00587fac != iroNodeTail)
        split_last_linear_into_new_node();
}

void split_last_linear_into_new_node(void)
{
    CLabel *label;
    IRONode *node;
    IROLinear *labelLinear;
    IROLinear *lastLinear;
    IRONode *scan;

    if (data_00587fac != NULL && data_00587fac->last != NULL && data_00587fac->last->type == IROLinearEnd) {
        label = IroUtil_NewLabel();
        node = CError_NewIRONode();

        iroNodeTail->nextnode = node;
        label->target.node = node;

        iroNodesByIndex = (IRONode **)CompilerTools_AllocatePoolMemory(iro_node_count * sizeof(*iroNodesByIndex));
        for (scan = iro_flowgraph_head; scan != NULL; scan = scan->nextnode)
            iroNodesByIndex[scan->index] = scan;

        labelLinear = IrOptimizer_NewLinear(IROLinearLabel);
        labelLinear->index = (UInt16)++linear_index_counter;
        labelLinear->u.label = label;
        labelLinear->flags |= 1;
        node->first = labelLinear;
        last_linear->next = labelLinear;

        lastLinear = IrOptimizer_NewLinear(IROLinearEnd);
        memcpy(lastLinear, data_00587fac->last, sizeof(*lastLinear));
        lastLinear->index = (UInt16)++linear_index_counter;
        lastLinear->next = NULL;
        node->last = lastLinear;
        labelLinear->next = lastLinear;
        last_linear = lastLinear;

        data_00587fac->last->type = IROLinearGoto;
        data_00587fac->last->u.label = label;
        data_00587fac = iroNodeTail = node;

        IroFlowgraph_RebuildSuccPred();
        IroFlowgraph_ComputeDom();
    }
}

void IroLoop_ComputeLoopDepth(void)
{
    UInt16 i;
    SInt32 flag;
    IRONode *bb;
    IRONode *q;
    IRONode *p;

    for (bb = iro_flowgraph_head; bb != NULL; bb = bb->nextnode)
        bb->loopdepth = 0;

    for (bb = iro_flowgraph_head; bb != NULL; bb = bb->nextnode) {
        flag = 0;
        for (i = 0; i < bb->numpred; i++) {
            q = iroNodesByIndex[bb->pred[i]];
            if (BV_TEST(q->dom, bb->index)) {
                if (!(SInt16)flag) {
                    IroBitVect_AllocateBitVector(&IRO_LoopScratchVector_005880dc, iro_node_count + 1);
                    IroBitVect_ClearBitVector(IRO_LoopScratchVector_005880dc);
                    BV_SET(IRO_LoopScratchVector_005880dc, bb->index);
                }
                flag = 1;
                BV_SET(IRO_LoopScratchVector_005880dc, q->index);
                if (q != bb)
                    IRO_CollectLoopBlocks_004614f0(q);
            }
        }
        if ((SInt16)flag) {
            for (p = iro_flowgraph_head; p != NULL; p = p->nextnode) {
                if (((p->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
                     (IRO_LoopScratchVector_005880dc->bits[p->index >> 5] & (1u << (p->index & 31))) != 0))
                    p->loopdepth++;
            }
        }
    }

    IroVars_CheckTimedLongjmp();
}

#define BitVector_Test(v, id) (((id) >> 5) < (v)->size && ((v)->bits[(id) >> 5] & (1u << ((id) & 31))))

void IRO_CollectLoopBlocks_004614f0(IRONode *lp)
{
    SInt32 i;
    IRONode *b;

    for (i = 0; i < lp->numpred; i++) {
        b = iroNodesByIndex[lp->pred[i]];
        if (!BitVector_Test(IRO_LoopScratchVector_005880dc, b->index)) {
            IROUseDef_SetBit(b->index, IRO_LoopScratchVector_005880dc);
            IRO_CollectLoopBlocks_004614f0(b);
        }
    }
}

IRONode *insert_loop_preheader(IRONode *loopHead, IRONode *predecessor)
{
    IRONode *preheader;
    IROLinear *label;
    CLabel *preheaderLabel;
    IRONode *block;
    SwitchInfo *switchInfo;
    SwitchCase *switchCase;

    preheader = (IRONode *)CompilerTools_AllocatePoolMemory(sizeof(IRONode));
    memset(preheader, 0, sizeof(IRONode));
    preheader->index = iro_node_count++;
    label = IrOptimizer_NewLinear(IROLinearLabel);
    label->index = linear_index_counter++;
    label->next = NULL;
    label->u.label = IroUtil_NewLabel();
    label->flags |= 1;
    preheaderLabel = label->u.label;
    /* The flow graph uses the label's basic-block arm. */
    preheaderLabel->target.node = preheader;
    preheader->first = label;
    preheader->last = label;
    if (predecessor != NULL) {
        predecessor->last->next = label;
        label->next = IrOptimizer_NewLinear(IROLinearNop);
        label->next->next = loopHead->first;
        predecessor->nextnode = preheader;
        preheader->nextnode = loopHead;
    } else {
        CError_ASSERT(1362, loopHead->first->type == IROLinearLabel);
        label->next = IrOptimizer_NewLinear(IROLinearGoto);
        label->next->u.label = loopHead->first->u.label;
        iroNodeTail->last->next = label;
        iroNodeTail->nextnode = preheader;
        iroNodeTail = preheader;
        last_linear = label->next;
    }
    preheader->last = label->next;
    iroNodesByIndex = (IRONode **)CompilerTools_AllocatePoolMemory(iro_node_count * sizeof(*iroNodesByIndex));
    for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
        iroNodesByIndex[block->index] = block;
    }
    if (loopHead->first->type == IROLinearLabel) {
        CLabel *oldlabel = loopHead->first->u.label;
        CLabel *newlabel = preheader->first->u.label;
        for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
            if (!((block->index >> 5) < IRO_LoopScratchVector_005880dc->size &&
                  ((IRO_LoopScratchVector_005880dc->bits[block->index >> 5] & (1u << block->index)) != 0)) &&
                block != preheader) {
                switch (block->last->type) {
                    case IROLinearGoto:
                        if (block->last->u.label == oldlabel)
                            block->last->u.label = newlabel;
                        break;
                    case IROLinearIf:
                    case IROLinearIfNot:
                        if (block->last->u.label == oldlabel)
                            block->last->u.label = newlabel;
                        break;
                    case IROLinearSwitch:
                        switchInfo = block->last->u.swtch.info;
                        for (switchCase = switchInfo->cases; switchCase != NULL; switchCase = switchCase->next) {
                            if (switchCase->label == oldlabel)
                                switchCase->label = newlabel;
                        }
                        if (switchInfo->defaultlabel == oldlabel)
                            switchInfo->defaultlabel = newlabel;
                        break;
                }
            }
        }
    }
    IroFlowgraph_RebuildSuccPred();
    IroFlowgraph_ComputeDom();
    return preheader;
}

enum { LOOP_FLAGS_10 = 0x10, LOOP_FLAGS_24 = 0x24, LOOP_FLAGS_34 = 0x34 };

/* Strength-reduces ROOT, an address of CONTEXT's induction variable times a factor: a temporary set to its value
   before the loop (from INITIAL, STEP and MODE) and stepped with the variable. */
IROExpr *IroLoop_00461860(IROExpr *root, IROLinear *initial, IROLinear *step, IROLoopInd *context, SInt32 mode)
{
    Object *temporary;
    Object *stepTemporary;
    Boolean hasStepTemporary;
    Type *type;
    IROList initList;
    IROList updateList;
    IROLinear *value;
    IROLinear *operation;
    SInt32 constant;

    hasStepTemporary = 0;
    type = root->linear->rtype;
    temporary = create_temp_object(type);
    fn_0044ba70((temporary), 1, 1);
    IroUtil_InitList(&initList);
    if (root->linear->type == IROLinearOp2Arg && root->linear->nodetype == EADD) {
        value = IroUtil_CopyLinearToList((root->linear), (&initList));
    } else if (root->linear->type == IROLinearOp2Arg &&
               (root->linear->nodetype == EDIV || root->linear->nodetype == ESHR)) {
        value = IroUtil_CopyLinearToList((root->linear), (&initList));
    } else {
        value = IrOptimizer_NewLinear(IROLinearOp2Arg);
        linear_index_counter++;
        value->index = linear_index_counter;
        value->rtype = type;
        value->nodetype = EMUL;
        value->u.diadic.left = IroUtil_CopyLinearToList((initial), (&initList));
        if (mode != 0) {
            value->u.diadic.right = IroUtil_CopyLinearToList((initial), (&initList));
        } else {
            value->u.diadic.right = IroUtil_CopyLinearToList((step), (&initList));
        }
        IroUtil_AppendLinear(value, &initList);
    }
    operation = IrOptimizer_NewLinear(IROLinearOp2Arg);
    linear_index_counter++;
    operation->index = linear_index_counter;
    operation->rtype = type;
    operation->nodetype = EASS;
    operation->u.diadic.left = IroUtil_AppendObjectRefAndUse(temporary, &initList);
    operation->u.diadic.left->flags |= LOOP_FLAGS_24;
    operation->u.diadic.left->u.monadic->flags |= LOOP_FLAGS_24;
    operation->u.diadic.right = value;
    IroUtil_AppendLinear(operation, &initList);
    IroUtil_InsertLinearBefore(initList.head, initList.tail, loop_candidate_last);

    if ((root->linear->type != IROLinearOp2Arg || (root->linear->nodetype != EDIV && root->linear->nodetype != ESHR)) &&
        context->addConst != 1 && (context->step != NULL || fn_0044d520(step) == 0)) {
        hasStepTemporary = 1;
        IroUtil_InitList(&initList);
        value = IrOptimizer_NewLinear(IROLinearOp2Arg);
        linear_index_counter++;
        value->index = linear_index_counter;
        value->rtype = step->rtype;
        value->nodetype = EMUL;
        if (context->step == NULL) {
            value->u.diadic.left = IroUtil_CopyLinearToList((step), (&initList));
            value->u.diadic.right = IrOptimizer_NewLinear(IROLinearOperand);
            linear_index_counter++;
            value->u.diadic.right->index = linear_index_counter;
            {
                ENode *integer = IrOptimizer_NewENode(EINTCONST);
                integer->rtype = step->rtype;
                value->u.diadic.right->rtype = step->rtype;
                constant = context->addConst;
                integer->data.intval.lo = constant;
                integer->data.intval.hi = (constant < 0) ? -1 : 0;
                value->u.diadic.right->u.node = integer;
            }
            IroUtil_AppendLinear(value->u.diadic.right, &initList);
        } else {
            value->u.diadic.left = IroUtil_CopyLinearToList((step), (&initList));
            value->u.diadic.right = IroUtil_CopyLinearToList((context->step), (&initList));
            if (step->rtype != context->step->rtype) {
                operation = IrOptimizer_NewLinear(IROLinearOp1Arg);
                operation->nodetype = ETYPCON;
                linear_index_counter++;
                operation->index = linear_index_counter;
                operation->rtype = step->rtype;
                operation->u.monadic = value->u.diadic.right;
                IroUtil_AppendLinear(operation, &initList);
                value->u.diadic.right = operation;
            }
        }
        IroUtil_AppendLinear(value, &initList);
        stepTemporary = create_temp_object(step->rtype);
        operation = IrOptimizer_NewLinear(IROLinearOp2Arg);
        linear_index_counter++;
        operation->index = linear_index_counter;
        operation->rtype = step->rtype;
        operation->nodetype = EASS;
        operation->u.diadic.left = IroUtil_AppendObjectRefAndUse(stepTemporary, &initList);
        operation->u.diadic.left->flags |= LOOP_FLAGS_24;
        operation->u.diadic.left->u.monadic->flags |= LOOP_FLAGS_24;
        operation->u.diadic.right = value;
        IroUtil_AppendLinear(operation, &initList);
        IroUtil_InsertLinearBefore(initList.head, initList.tail, loop_candidate_last);
    }

    IroUtil_InitList(&updateList);
    operation = IrOptimizer_NewLinear(IROLinearOp2Arg);
    operation->index = (linear_index_counter += 1);
    operation->rtype = type;
    if (context->nd->type == IROLinearOp2Arg) {
        if (context->nd->nodetype == EASS && context->nd->u.diadic.right->type == IROLinearOp2Arg &&
            context->nd->u.diadic.right->nodetype == EADD) {
            operation->nodetype = EADDASS;
        } else if (context->nd->nodetype == EASS && context->nd->u.diadic.right->type == IROLinearOp2Arg &&
                   context->nd->u.diadic.right->nodetype == ESUB) {
            operation->nodetype = ESUBASS;
        } else {
            operation->nodetype = ((volatile IROLinear *)context->nd)->nodetype;
        }
    } else if (context->nd->nodetype == EPREINC || context->nd->nodetype == EPOSTINC) {
        operation->nodetype = EADDASS;
    } else {
        operation->nodetype = ESUBASS;
    }
    operation->u.diadic.left = IroUtil_AppendObjectRefAndUse(temporary, &updateList);
    operation->u.diadic.left->flags |= LOOP_FLAGS_34;
    operation->u.diadic.left->u.monadic->flags |= LOOP_FLAGS_34;
    if (mode == 0) {
        if (!hasStepTemporary) {
            if (context->addConst == 1 || (root->linear->type == IROLinearOp2Arg &&
                                           (root->linear->nodetype == EDIV || root->linear->nodetype == ESHR))) {
                operation->u.diadic.right = IroUtil_CopyLinearToList((step), (&updateList));
            } else {
                operation->u.diadic.right = IrOptimizer_NewLinear(IROLinearOperand);
                linear_index_counter++;
                operation->u.diadic.right->index = linear_index_counter;
                {
                    ENode *integer = IrOptimizer_NewENode(EINTCONST);
                    integer->rtype = step->rtype;
                    operation->u.diadic.right->rtype = step->rtype;
                    constant = step->u.node->data.intval.lo * context->addConst;
                    integer->data.intval.lo = constant;
                    integer->data.intval.hi = (constant < 0) ? -1 : 0;
                    operation->u.diadic.right->u.node = integer;
                }
                IroUtil_AppendLinear(operation->u.diadic.right, &updateList);
            }
        } else {
            operation->u.diadic.right = IroUtil_AppendObjectRefAndUse(stepTemporary, &updateList);
            operation->u.diadic.right->flags |= LOOP_FLAGS_10;
        }
    }
    linear_index_counter++;
    operation->index = linear_index_counter;
    IroUtil_AppendLinear(operation, &updateList);
    {
        IROLinear *predecessor;
        predecessor = IroUtil_GetLinearRangeStart(context->nd);
        IroUtil_InsertLinearBefore(updateList.head, updateList.tail, predecessor);
    }
    IroUtil_VisitLinearTree(root->linear, forward_expr_if_check_object);
    root->temp = temporary;
    root->next = iro_loop_roots;
    iro_loop_roots = root;
    return root;
}

/* (IroUtil_VisitLinearTree visitors) */
void fn_00461dc0(IROLinear *type, unsigned int enabled)
{
    if (enabled != 0U) {
        if (type->expr != 0U) {
            type->flags &= ~2048U;
        }
    }
}

void forward_expr_if_check_object(IROLinear *entry, int checkObject)

{
    if ((checkObject != 0) && (entry->expr != NULL)) {
        IroCSE_RemoveExpr(entry->expr);
    }
    return;
}

/* Whether NODE is an address of an induction variable VARIABLE (in EXPRESSION) times FACTOR. */
int match_induction_expression(IROLinear *node, IROLinear **factor, IROLinear **expression, VarRecord **variable)
{
    NodeKind nodeKind;
    SInt32 count;
    ENode *constant;
    Object *value;
    IROLinear *left;
    IROLinear *right;
    IROLinear *variableNode;
    Boolean leftFlag100;
    Boolean rightFlag100;
    Boolean leftConverted;
    Boolean rightConverted;
    CInt64 divisor;

    leftFlag100 = 0;
    rightFlag100 = 0;
    leftConverted = 0;
    rightConverted = 0;
    nodeKind = node->type;
    do {
        if (nodeKind == IROLinearOp2Arg && node->nodetype == EADD &&
            (node->rtype->type == TYPEINT || node->rtype->type == TYPEPOINTER)) {
            left = node->u.diadic.left;
            right = node->u.diadic.right;
            if (left->type == IROLinearOp1Arg && left->nodetype == ETYPCON) {
                leftConverted = 1;
                leftFlag100 = (left->flags & IROLF_LoopInvariant) != 0;
                left = left->u.monadic;
            }
            if (right->type == IROLinearOp1Arg && right->nodetype == ETYPCON) {
                rightConverted = 1;
                rightFlag100 = (right->flags & IROLF_LoopInvariant) != 0;
                right = right->u.monadic;
            }
            if (((left->flags & IROLF_LoopInvariant) != 0 || leftFlag100 != 0) && IroDump_GetObjRef(right) != NULL) {
                if (leftFlag100 != 0 ||
                    (((value = IroDump_GetObjRef(left)) == NULL || IroPropagate_IsRegisterEligible(value) == 0) &&
                     fn_0044d520(left) == 0)) {
                    if (left->type == IROLinearOp2Arg && left->nodetype == EADD) {
                        value = IroDump_GetObjRef(left->u.diadic.left);
                        if (value != NULL) {
                            if (IroPropagate_IsRegisterEligible(value) != 0) {
                                if (fn_0044d520(left->u.diadic.right) != 0)
                                    return 0;
                            }
                        }
                    }
                    if (rightConverted != 0) {
                        if (is_value_preserving_integral_conversion(node->u.diadic.right) == 0)
                            return 0;
                        *expression = node->u.diadic.right;
                    } else {
                        *expression = right;
                    }
                    variableNode = right;
                    *factor = IrOptimizer_NewLinear(IROLinearOperand);
                    constant = IrOptimizer_NewENode(EINTCONST);
                    constant->data.intval = cint64_one;
                    constant->rtype = node->u.diadic.right->rtype;
                    (*factor)->rtype = node->u.diadic.right->rtype;
                    (*factor)->u.node = constant;
                    break;
                }
                return 0;
            }

            if (((left->flags & IROLF_LoopInvariant) != 0 || leftFlag100 != 0) && right->type == IROLinearOp2Arg &&
                rightConverted == 0 && (right->nodetype == EMUL || right->nodetype == ESHL)) {
                if (fn_0044d520(right->u.diadic.right) != 0) {
                    if (right->nodetype == ESHL) {
                        right->nodetype = EMUL;
                        right->u.diadic.right->u.node->data.intval =
                            CInt64_Shl(cint64_one, right->u.diadic.right->u.node->data.intval);
                    }
                    if (right->u.diadic.left->type == IROLinearOp1Arg) {
                        if (IroDump_GetObjRef(right->u.diadic.left) != NULL) {
                            *expression = right->u.diadic.left;
                            variableNode = right->u.diadic.left;
                            *factor = right->u.diadic.right;
                            break;
                        }
                        if (right->u.diadic.left->nodetype == ETYPCON &&
                            IroDump_GetObjRef(right->u.diadic.left->u.monadic) != NULL) {
                            if (is_value_preserving_integral_conversion(right->u.diadic.left) == 0)
                                return 0;
                            *expression = right->u.diadic.left;
                            variableNode = right->u.diadic.left->u.monadic;
                            *factor = right->u.diadic.right;
                            break;
                        }
                        return 0;
                    }
                    return 0;
                }
                return 0;
            }

            if (((right->flags & IROLF_LoopInvariant) != 0 || rightFlag100 != 0) && left->type == IROLinearOp2Arg &&
                leftConverted == 0 && (left->nodetype == EMUL || left->nodetype == ESHL)) {
                if (fn_0044d520(left->u.diadic.right) != 0) {
                    if (left->nodetype == ESHL) {
                        left->nodetype = EMUL;
                        left->u.diadic.right->u.node->data.intval =
                            CInt64_Shl(cint64_one, left->u.diadic.right->u.node->data.intval);
                    }
                    if (left->u.diadic.left->type == IROLinearOp1Arg) {
                        if (IroDump_GetObjRef(left->u.diadic.left) != NULL) {
                            *expression = left->u.diadic.left;
                            variableNode = left->u.diadic.left;
                            *factor = left->u.diadic.right;
                            break;
                        }
                        if (left->u.diadic.left->nodetype == ETYPCON &&
                            IroDump_GetObjRef(left->u.diadic.left->u.monadic) != NULL) {
                            if (is_value_preserving_integral_conversion(left->u.diadic.left) == 0)
                                return 0;
                            *expression = left->u.diadic.left;
                            variableNode = left->u.diadic.left->u.monadic;
                            *factor = left->u.diadic.right;
                            break;
                        }
                        return 0;
                    }
                    return 0;
                }
                return 0;
            }

            if (((right->flags & IROLF_LoopInvariant) != 0 || rightFlag100 != 0) && IroDump_GetObjRef(left) != NULL) {
                if (rightFlag100 != 0 ||
                    (((value = IroDump_GetObjRef(right)) == NULL || IroPropagate_IsRegisterEligible(value) == 0) &&
                     fn_0044d520(right) == 0)) {
                    if (right->type == IROLinearOp2Arg && right->nodetype == EADD) {
                        value = IroDump_GetObjRef(right->u.diadic.left);
                        if (value != NULL) {
                            if (IroPropagate_IsRegisterEligible(value) != 0) {
                                if (fn_0044d520(right->u.diadic.right) != 0)
                                    return 0;
                            }
                        }
                    }
                    if (leftConverted != 0) {
                        if (is_value_preserving_integral_conversion(node->u.diadic.left) == 0)
                            return 0;
                        *expression = node->u.diadic.left;
                    } else {
                        *expression = left;
                    }
                    variableNode = left;
                    *factor = IrOptimizer_NewLinear(IROLinearOperand);
                    constant = IrOptimizer_NewENode(EINTCONST);
                    constant->data.intval = cint64_one;
                    constant->rtype = node->u.diadic.left->rtype;
                    (*factor)->rtype = node->u.diadic.left->rtype;
                    (*factor)->u.node = constant;
                    break;
                }
                return 0;
            }
            return 0;
        }

        if (nodeKind == IROLinearOp2Arg && (node->nodetype == EMUL || node->nodetype == ESHL) &&
            node->rtype->size <= 4 && (node->rtype->type == TYPEINT || node->rtype->type == TYPEPOINTER)) {
            right = node->u.diadic.right;
            left = node->u.diadic.left;
            if (fn_0044d520(right) != 0 && IroDump_GetObjRef(left) != NULL) {
                *expression = left;
                variableNode = left;
                *factor = right;
                break;
            }
            if (fn_0044d520(node->u.diadic.right) != 0 && left->type == IROLinearOp1Arg && left->nodetype == ETYPCON &&
                IroDump_GetObjRef(left->u.monadic) != NULL) {
                if (is_value_preserving_integral_conversion(left) == 0)
                    return 0;
                *expression = left;
                variableNode = left->u.monadic;
                *factor = right;
                break;
            }
            if (node->type == IROLinearOp2Arg && node->nodetype == ESHL)
                return 0;
            if ((node->u.diadic.right->flags & IROLF_LoopInvariant) != 0) {
                if (IroDump_GetObjRef(left) != NULL) {
                    *expression = left;
                    variableNode = left;
                    *factor = right;
                    break;
                }
                if (left->type == IROLinearOp1Arg && left->nodetype == ETYPCON &&
                    IroDump_GetObjRef(left->u.monadic) != NULL) {
                    if (is_value_preserving_integral_conversion(left) == 0)
                        return 0;
                    *expression = left;
                    variableNode = left->u.monadic;
                    *factor = right;
                    break;
                }
                return 0;
            }
            if ((node->u.diadic.left->flags & IROLF_LoopInvariant) != 0) {
                if (IroDump_GetObjRef(right) != NULL) {
                    *expression = right;
                    variableNode = right;
                    *factor = left;
                    break;
                }
                if (right->type == IROLinearOp1Arg && right->nodetype == ETYPCON &&
                    IroDump_GetObjRef(right->u.monadic) != NULL && node->type == IROLinearOp2Arg &&
                    node->nodetype == EMUL) {
                    if (is_value_preserving_integral_conversion(right) == 0)
                        return 0;
                    *expression = right;
                    variableNode = right->u.monadic;
                    *factor = left;
                    break;
                }
                return 0;
            }
            return 0;
        }

        if (nodeKind == IROLinearOp2Arg && (node->nodetype == EDIV || node->nodetype == ESHR) &&
            node->rtype->size <= 4 && node->rtype->type == TYPEINT) {
            if (IroDump_GetObjRef(node->u.diadic.left) != NULL && fn_0044d520(node->u.diadic.right) != 0) {
                constant = node->u.diadic.right->u.node;
                divisor = constant->data.intval;
                if (node->type == IROLinearOp2Arg && node->nodetype == ESHR) {
                    if (divisor.lo < 0 || divisor.lo > 0x20 || divisor.hi != 0)
                        return 0;
                    divisor = CInt64_Shl(cint64_one, divisor);
                }
                *expression = node->u.diadic.left;
                variableNode = node->u.diadic.left;
                break;
            }
            return 0;
        } else {
            return 0;
        }
    } while (0);
    if (node->type == IROLinearOp2Arg && node->nodetype == ESHL) {
        if (fn_0044d520(*factor) == 0 || (count = (*factor)->u.node->data.intval.lo) < 0 || count > 0x20 ||
            (*factor)->u.node->data.intval.hi != 0)
            return 0;
    }
    if (variableNode->u.monadic->u.node == NULL)
        CError_FATAL(903);
    constant = variableNode->u.monadic->u.node;
    *variable = fn_0044ba70(constant->data.objref, 0, 1);
    if (*variable == NULL || (*variable)->inductionState != 2)
        return 0;
    if (copts.rejectZeroLengthArrayMembers != 0 || copts.fc9 != 0) {
        TypeIntegral *variableType = (TypeIntegral *)(*variable)->object->type;
        if ((variableType->integral == IT_UCHAR || variableType->integral == IT_USHORT ||
             variableType->integral == IT_UINT || variableType->integral == IT_ULONG ||
             variableType->integral == IT_ULONGLONG) &&
            variableType->size < stunsignedlong.size)
            return 0;
    }
    if (node->type == IROLinearOp2Arg && (node->nodetype == ESHR || node->nodetype == EDIV)) {
        IROLoopInd *induction;
        for (induction = induction_variables; induction != NULL && induction->var != *variable;
             induction = induction->next)
            ;
        if (induction == NULL)
            CError_FATAL(944);
        if (induction->step == NULL) {
            SInt32 iterations = induction->addConst;
            SInt32 step = divisor.lo;
            if (iterations < step)
                return 0;
            count = iterations / step;
            if (count <= 0)
                return 0;
            *factor = IrOptimizer_NewLinear(IROLinearOperand);
            constant = IrOptimizer_NewENode(EINTCONST);
            constant->data.intval.lo = count;
            constant->data.intval.hi = 0;
            constant->rtype = node->u.diadic.left->rtype;
            (*factor)->rtype = node->u.diadic.left->rtype;
            (*factor)->u.node = constant;
        } else {
            return 0;
        }
    }
    return 1;
}

int is_value_preserving_integral_conversion(IROLinear *op)
{
    SInt32 sourceSize;
    SInt32 destinationSize;
    Type *sourceType;
    Type *destinationType;
    TypeIntegral *sourceIntegral;
    TypeIntegral *destinationIntegral;
    Boolean sourceUnsigned;
    Boolean destinationUnsigned;

    sourceUnsigned = 0;
    destinationUnsigned = 0;
    sourceType = op->u.monadic->rtype;
    destinationType = op->rtype;

    if (sourceType->type != TYPEINT || destinationType->type != TYPEINT)
        return 0;

    sourceIntegral = (TypeIntegral *)sourceType;
    destinationIntegral = (TypeIntegral *)destinationType;

    sourceSize = sourceIntegral->size;
    destinationSize = destinationIntegral->size;

    if (sourceIntegral->integral == IT_UCHAR || sourceIntegral->integral == IT_USHORT ||
        sourceIntegral->integral == IT_UINT || sourceIntegral->integral == IT_ULONG ||
        sourceIntegral->integral == IT_ULONGLONG)
        sourceUnsigned = 1;

    if (destinationIntegral->integral == IT_UCHAR || destinationIntegral->integral == IT_USHORT ||
        destinationIntegral->integral == IT_UINT || destinationIntegral->integral == IT_ULONG ||
        destinationIntegral->integral == IT_ULONGLONG)
        destinationUnsigned = 1;

    if (sourceUnsigned == destinationUnsigned && destinationSize >= sourceSize)
        return 1;
    if (sourceUnsigned == 1 && destinationUnsigned == 0 && destinationSize > sourceSize)
        return 1;
    return 0;
}

void compute_mustreach(void)
{
    IRONode *p;
    IRONode *q;
    IRONode *x;
    SInt32 i;

    for (p = iro_flowgraph_head; p != NULL; p = p->nextnode) {
        p->mustreach = 0;
        if (GMARKED(p->index)) {
            p->mustreach = 1;
            for (i = 0; i < loop_header->numpred; i++) {
                x = iroNodesByIndex[loop_header->pred[i]];
                if (GMARKED(x->index) && !SMARKED(x->dom, p->index)) {
                    p->mustreach = 0;
                    break;
                }
            }
            for (q = iro_flowgraph_head; q != NULL; q = q->nextnode) {
                if (GMARKED(q->index)) {
                    for (i = 0; i < q->numsucc; i++) {
                        if (!GMARKED(q->succ[i]) && !SMARKED(q->dom, p->index)) {
                            p->mustreach = 0;
                            break;
                        }
                    }
                    if (!p->mustreach)
                        break;
                }
            }
        }
    }
}
