#define CERROR_FILE "IroVars.c"
#include "compiler/common.h"
#include "compiler/IroVars.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BitVector.h"
#include "compiler/CClass.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CParser.h"
#include "compiler/CompilerTools.h"
#include "compiler/IrOptimizer.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroUtil.h"
#include "compiler/Registers.h"
#include "driver/COSToolsCLT.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/cc-eabi-ppc.h"

struct VarRecord *var_records;
SInt32 data_00587ef4;
unsigned int iroVarCount;
struct VarRecord *var_records_tail;
struct IROVarPart *var_part_use_tail;
unsigned int data_00588234;
struct IROVarPart *class_data_parts;
struct BitVector *noregister_bitvector;
UInt8 data_00588510;

static struct IROLinear *saved_node;

/* Flags of each ENode type. */
SInt8 monadic_addr_flags_by_nodetype[75] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
UInt8 data_00551d6c[75] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#define IRO_BV_TEST(bv, idx)                                                                                           \
    ((UInt32)((idx) >> 5) < (UInt32)(bv)->size && ((bv)->bits[(idx) >> 5] & ((UInt32)1 << ((idx) & 31))) != 0)

static int IRO_ListHasObject(Object *key);

static inline int fn_0044a6f0_inline1(VarRecord *a1)
{
    int v11;
    ObjectList *v12;
    v11 = (int)a1->object;
    v12 = arguments;
    while ((int)v12 != 0) {
        if ((int)v12->object == v11) {
            return 1;
        }
        v12 = v12->next;
    }
    return 0;
}

IROAddrRecord *IroVars_CreateAddrRecord(struct IROLinear *linear)
{
    IROAddrRecord *record;
    record = (IROAddrRecord *)oalloc(28U);
    record->x4 = 0U;
    record->numObjRefs = 0U;
    record->objRefs = 0U;
    record->numMisc = 0U;
    record->misc = 0U;
    record->numInts = 0U;
    record->ints = 0U;
    record->x18 = 0U;
    record->linear = linear;
    return record;
}

IROLinear *fn_0044be00(IROLinear *node)
{
    IROLinear *r;

    if (node == NULL)
        return node;
    if (node->rtype != NULL && CParser_IsVolatile(node->rtype, node->nodeflags & 3))
        return node;
    switch (node->type) {
        case IROLinearAsm:
            return node;
        case IROLinearOperand:
            if (node->u.node->type == EOBJREF && is_volatile_object(node->u.node->data.objref))
                return node;
            return NULL;
        case IROLinearOp1Arg:
            if (data_00551d6c[node->nodetype] != 0)
                return node;
            return fn_0044be00(node->u.diadic.left);
        case IROLinearOp2Arg:
            if (data_00551d6c[node->nodetype] != 0)
                return node;
            r = fn_0044be00(node->u.diadic.left);
            if (r == NULL)
                r = fn_0044be00(node->u.diadic.right);
            return r;
        case IROLinearFunccall:
            return node;
        default:
            return node;
    }
}

IROLinear *IroVars_NopOutWithSideEffectsChecking(IROLinear *node)
{
    IROLinear *result;

    if (node == NULL)
        return node;
    if (node->rtype != NULL && CParser_IsVolatile(node->rtype, node->nodeflags & 3)) {
        node->flags &= ~IROLF_Reffed;
        return node;
    }
    result = NULL;
    switch (node->type) {
        case IROLinearAsm:
            node->flags &= ~IROLF_Reffed;
            return node;
        case IROLinearOperand:
            if (node->u.node->type == EOBJREF && is_volatile_object(node->u.node->data.objref)) {
                node->flags &= ~IROLF_Reffed;
                return node;
            }
            break;
        case IROLinearOp1Arg:
            if (data_00551d6c[node->nodetype]) {
                node->flags &= ~IROLF_Reffed;
                return node;
            }
            result = IroVars_NopOutWithSideEffectsChecking(node->u.monadic);
            if (result != NULL && node->nodetype == EINDIRECT && node->u.monadic->type == IROLinearOperand) {
                node->flags &= ~IROLF_Reffed;
                return node;
            }
            break;
        case IROLinearOp2Arg:
            if (data_00551d6c[node->nodetype]) {
                node->flags &= ~IROLF_Reffed;
                return node;
            }
            result = IroVars_NopOutWithSideEffectsChecking(node->u.diadic.left);
            {
                IROLinear *right = IroVars_NopOutWithSideEffectsChecking(node->u.diadic.right);
                if (result == NULL)
                    result = right;
            }
            break;
        case IROLinearFunccall:
            node->flags &= ~IROLF_Reffed;
            return node;
        default:
            return node;
    }
    node->type = IROLinearNop;
    IroDump_Print("Nop out with side-effects checking at: %d\n", node->index);
    return result;
}

void IroVars_VisitExceptionOperands(ExceptionAction *node, void (*visitOperand)(Object *))
{
    for (; node != NULL; node = node->next) {
        switch (node->kind) {
            case EAT_DESTROYLOCAL:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DESTROYLOCALCOND:
                visitOperand(node->data.operands[0].object);
                visitOperand(node->data.operands[1].object);
                break;
            case EAT_DESTROYLOCALOFFSET:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DESTROYLOCALPOINTER:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DESTROYLOCALARRAY:
                visitOperand(node->data.operands[0].object);
                break;
            case 6:
                visitOperand(node->data.operands[0].object);
                visitOperand(node->data.operands[1].object);
                visitOperand(node->data.operands[3].object);
                break;
            case EAT_DESTROYBASE:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DESTROYMEMBER:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DESTROYMEMBERCOND:
                visitOperand(node->data.operands[0].object);
                visitOperand(node->data.operands[1].object);
                break;
            case EAT_DESTROYMEMBERARRAY:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DELETEPOINTER:
                visitOperand(node->data.operands[0].object);
                break;
            case EAT_DELETEPOINTERCOND:
                visitOperand(node->data.operands[0].object);
                visitOperand(node->data.operands[2].object);
                break;
        }
    }
}

/* An integer-constant operand of TYPE. */
IROLinear *IroVars_CreateIntConstant(CInt64 value, Type *type)
{
    ENode *record;
    IROLinear *linear;
    record = IrOptimizer_NewENode(50);
    record->data.intval = value;
    record->rtype = type;
    linear = IrOptimizer_NewLinear(IROLinearOperand);
    linear->rtype = type;
    linear->u.node = record;
    return linear;
}

#pragma auto_inline off
void IroVars_CheckTimedLongjmp(void)
{
    unsigned int current;
    unsigned int previous;
    current = COS_GetTicks();
    previous = data_00588234;
    previous += 8U;
    if (previous < current) {
        if (fn_0041b910(compiler_plugin_cu.context) != 0U)
            CError_Longjmp();
        data_00588234 = COS_GetTicks();
    }
}
#pragma auto_inline reset

VarRecord *fn_0044ba70(Object *object, unsigned int create, unsigned int mode)
{
    VarRecord *entry = object->aliasOrVarRecord.varRecord;
    VarInfo *info;

    if (entry == NULL && create == 1) {
        info = NULL;
        if (object->datatype == DLOCAL)
            info = object->u.var.info;
        if (info != NULL)
            info->usage = 0;

        entry = (VarRecord *)oalloc(sizeof(VarRecord));
        entry->object = object;
        iroVarCount = iroVarCount + 1;
        entry->index = iroVarCount;
        entry->x6 = 0;
        entry->inductionState = 0;
        entry->next = NULL;
        entry->defs = NULL;
        entry->uses = NULL;
        entry->x1a = NULL;
        entry->bitFieldReplacement = NULL;
        entry->noregister = 0;
        if (var_records != NULL) {
            var_records_tail->next = entry;
        } else {
            var_records = entry;
        }
        var_records_tail = entry;
        object->aliasOrVarRecord.varRecord = entry;
    }
    if (entry != NULL && mode == 0) {
        entry->noregister = 1;
        if (object->datatype == DLOCAL) {
            info = object->u.var.info;
            if (info != NULL)
                info->noregister = 1;
        }
    }
    return entry;
}

/* The object's VarRecord, created (when CREATE is 1) at the end of the variable list; MODE 0 marks it as one that
   cannot live in a register. */
void visit_dobjstack_objects(IROLinear *linear)
{
    ExceptionAction *node;
    node = linear->stmt->dobjstack;
    while (node != NULL) {
        switch (node->kind) {
            case EAT_DESTROYLOCAL:
                fn_0044ba70(node->data.local.object, 1, 0);
                break;
            case EAT_DESTROYLOCALCOND:
                fn_0044ba70(node->data.local_cond.object, 1, 0);
                break;
            case EAT_DESTROYLOCALOFFSET:
                fn_0044ba70(node->data.local.object, 1, 0);
                break;
            case EAT_DESTROYLOCALARRAY:
                fn_0044ba70(node->data.call.context, 1, 0);
                break;
            case EAT_DESTROYMEMBER:
            case EAT_DESTROYBASE:
                fn_0044ba70(node->data.member.objectptr, 1, 0);
                break;
            case EAT_DESTROYMEMBERCOND:
                fn_0044ba70(node->data.member_cond.objectptr, 1, 0);
                break;
            case EAT_DESTROYMEMBERARRAY:
                fn_0044ba70(node->data.member_array.objectptr, 1, 0);
                break;
            case EAT_CATCHBLOCK:
                if (node->data.catch_block.object != NULL)
                    fn_0044ba70(node->data.catch_block.object, 1, 0);
                fn_0044ba70(node->data.catch_block.info, 1, 0);
                break;
            case EAT_ACTIVECATCHBLOCK:
                fn_0044ba70(node->data.active_catch.info, 1, 0);
                break;
        }
        node = node->next;
    }
}

void IroVars_BuildVarRecords(void)
{
    IROLinear *node;
    VarRecord *item;
    AsmOut references;
    SInt32 index;
    Object *object;
    Object *context;

    node = linear_head;
    iroVarCount = 0;
    var_records = var_records_tail = NULL;
    if (linear_head != NULL) {
        do {
            if (node->type == IROLinearOperand && node->u.node->type == EOBJREF) {
                object = node->u.node->data.objref;
                if (object->datatype == DDATA || object->datatype == DLOCAL) {
                    context = object;
                    fn_0044ba70(context, 1, node->flags & Q_REFERENCE);
                } else
                    object->aliasOrVarRecord.varRecord = NULL;
            } else if (node->type == IROLinearFunccall) {
                visit_dobjstack_objects(node);
            } else if (node->type == IROLinearAsm) {
                fn_00462d70(node->u.asm_stmt, &references);
                for (index = 0; index < references.numoperands; index++)
                    fn_0044ba70(references.operands[index].object, 1, references.operands[index].type != 3);
            }
            node = node->next;
        } while (node != NULL);
    }
    item = var_records;
    IroBitVect_AllocateBitVector(&noregister_bitvector, iroVarCount + 1);
    IroBitVect_SetBit(0, noregister_bitvector);
    while (item != NULL) {
        object = item->object;
        if (object->datatype == DDATA || item->noregister != 0)
            IroBitVect_SetBit(item->index, noregister_bitvector);
        item = item->next;
    }
    IroVars_CheckTimedLongjmp();
}

void IroVars_ClearObjectVarRecords(void)
{
    VarRecord *p;
    Object *context;

    p = var_records;
    if (p != NULL) {
        do {
            context = p->object;
            context->aliasOrVarRecord.varRecord = NULL;
            p = p->next;
        } while (p != NULL);
    }
    return;
}

void IroVars_BuildNoregisterBitVector(void)
{
    VarRecord *p;
    IROLinear *q;
    Object *obj;
    SInt32 flag;

    for (p = var_records; p != NULL; p = p->next)
        p->noregister = 0;

    for (q = linear_head; q != NULL; q = q->next) {
        if (q->type == IROLinearOperand && q->u.node->type == EOBJREF) {
            obj = q->u.node->data.objref;
            if (obj->datatype == DDATA || obj->datatype == DLOCAL) {
                flag = (q->flags & IROLF_Ind) || !(q->flags & IROLF_Reffed);
                fn_0044ba70(obj, 2, flag);
            }
        }
        if (q->type == IROLinearFunccall)
            visit_dobjstack_objects(q);
    }

    p = var_records;
    IroBitVect_AllocateBitVector(&noregister_bitvector, iroVarCount + 1);
    IroBitVect_SetBit(0, noregister_bitvector);

    while (p != NULL) {
        if (p->object->datatype == DDATA || p->noregister != 0)
            IroBitVect_SetBit(p->index, noregister_bitvector);
        p = p->next;
    }
}

/* Pushes NODE on the term list *HEAD. */
void IroVars_PrependElmList(IROLinear *node, IROElmList **head)
{
    IROElmList *entry;

    entry = (IROElmList *)oalloc(sizeof(IROElmList));
    entry->element = node;
    entry->next = NULL;
    if (*head != NULL) {
        entry->next = *head;
    }
    *head = entry;
}

void IroVars_CollectAddrRecordElements(IROLinear *tree, IROAddrRecord *collection)
{
    IROElmList *leftInt;
    IROElmList *leftObjRef;
    IROElmList *leftMisc;
    IROElmList *rightInt;
    IROElmList *rightObjRef;
    IROElmList *rightMisc;
    IROLinear *leftIntNode;
    IROLinear *leftObjRefNode;
    IROLinear *leftMiscNode;
    IROLinear *rightIntNode;
    IROLinear *rightObjRefNode;
    IROLinear *rightMiscNode;

    if (tree->u.diadic.left->type == IROLinearOp2Arg && tree->u.diadic.left->nodetype == EADD) {
        IroVars_CollectAddrRecordElements(tree->u.diadic.left, collection);
    } else if (tree->u.diadic.left->type == EPOSTDEC && tree->u.diadic.left->u.node->type == EINTCONST) {
        collection->numInts += 1;
        leftIntNode = tree->u.diadic.left;
        leftInt = (IROElmList *)oalloc(sizeof(IROElmList));
        leftInt->element = leftIntNode;
        leftInt->next = NULL;
        if (collection->ints == NULL) {
            collection->ints = leftInt;
        } else {
            leftInt->next = collection->ints;
            collection->ints = leftInt;
        }
    } else if (tree->u.diadic.left->type == EPOSTDEC && tree->u.diadic.left->u.node->type == EOBJREF) {
        collection->numObjRefs += 1;
        leftObjRefNode = tree->u.diadic.left;
        leftObjRef = (IROElmList *)oalloc(sizeof(IROElmList));
        leftObjRef->element = leftObjRefNode;
        leftObjRef->next = NULL;
        if (collection->objRefs == NULL) {
            collection->objRefs = leftObjRef;
        } else {
            leftObjRef->next = collection->objRefs;
            collection->objRefs = leftObjRef;
        }
    } else {
        collection->numMisc += 1;
        leftMiscNode = tree->u.diadic.left;
        leftMisc = (IROElmList *)oalloc(sizeof(IROElmList));
        leftMisc->element = leftMiscNode;
        leftMisc->next = NULL;
        if (collection->misc != NULL) {
            leftMisc->next = collection->misc;
        }
        collection->misc = leftMisc;
    }
    if (tree->u.diadic.right->type == IROLinearOp2Arg && tree->u.diadic.right->nodetype == EADD) {
        IroVars_CollectAddrRecordElements(tree->u.diadic.right, collection);
    } else if (tree->u.diadic.right->type == EPOSTDEC && tree->u.diadic.right->u.node->type == EINTCONST) {
        collection->numInts += 1;
        rightIntNode = tree->u.diadic.right;
        rightInt = (IROElmList *)oalloc(sizeof(IROElmList));
        rightInt->element = rightIntNode;
        rightInt->next = NULL;
        if (collection->ints == NULL) {
            collection->ints = rightInt;
        } else {
            rightInt->next = collection->ints;
            collection->ints = rightInt;
        }
    } else if (tree->u.diadic.right->type == EPOSTDEC && tree->u.diadic.right->u.node->type == EOBJREF) {
        collection->numObjRefs += 1;
        rightObjRefNode = tree->u.diadic.right;
        rightObjRef = (IROElmList *)oalloc(sizeof(IROElmList));
        rightObjRef->element = rightObjRefNode;
        rightObjRef->next = NULL;
        if (collection->objRefs == NULL) {
            collection->objRefs = rightObjRef;
        } else {
            rightObjRef->next = collection->objRefs;
            collection->objRefs = rightObjRef;
        }
    } else {
        collection->numMisc += 1;
        rightMiscNode = tree->u.diadic.right;
        rightMisc = (IROElmList *)oalloc(sizeof(IROElmList));
        rightMisc->element = rightMiscNode;
        rightMisc->next = NULL;
        if (collection->misc != NULL) {
            rightMisc->next = collection->misc;
        }
        collection->misc = rightMisc;
    }
}

/* IRO node record as used by IroVars.c: byte kind at 0x00, byte op at 0x01,
 * a long at 0x0a, and two child pointers at 0x1a / 0x1e. */

void fn_0044b4e0(IROLinear *node)
{
    if (node->u.diadic.left->type == IROLinearOp2Arg && node->u.diadic.left->nodetype == EADD) {
        fn_0044b4e0(node->u.diadic.left);
    } else if ((node->u.diadic.left->type != EPOSTDEC || node->u.diadic.left->u.node->type != EINTCONST) &&
               (node->u.diadic.left->type == EPOSTDEC && node->u.diadic.left->u.node->type == EOBJREF)) {
        data_00587ef4++;
        saved_node = node->u.diadic.left;
    }

    if (node->u.diadic.right->type == IROLinearOp2Arg && node->u.diadic.right->nodetype == EADD) {
        fn_0044b4e0(node->u.diadic.right);
    } else if ((node->u.diadic.right->type != EPOSTDEC || node->u.diadic.right->u.node->type != EINTCONST) &&
               (node->u.diadic.right->type == EPOSTDEC && node->u.diadic.right->u.node->type == EOBJREF)) {
        data_00587ef4++;
        saved_node = node->u.diadic.right;
    }
}

/* The variable an assignment (3) or increment (2) writes, through an indirection of its address. */
VarRecord *IroVars_GetOperandVarRecord(IROLinear *node)
{
    VarRecord *r;

    data_00588510 = 0;

    if (node->type == IROLinearOp2Arg)
        node = node->u.diadic.left;
    else if (node->type == IROLinearOp1Arg)
        node = node->u.monadic;
    else
        CError_FATAL(646);

    if (node->type == IROLinearOp1Arg && node->nodetype == EINDIRECT)
        node = node->u.monadic;

    if (node->type == IROLinearOp2Arg && node->nodetype == EADD) {
        if (node->u.diadic.left->type == EPOSTDEC && node->u.diadic.left->u.node->type == EOBJREF &&
            node->u.diadic.right->type == EPOSTDEC && node->u.diadic.right->u.node->type == EINTCONST) {
            node = node->u.diadic.left;
        } else {
            data_00587ef4 = 0;
            fn_0044b4e0(node);
            if (data_00587ef4 == 1)
                node = saved_node;
        }
    }

    if (node->type != IROLinearOperand || node->u.node->type != EOBJREF)
        return NULL;
    r = fn_0044ba70(node->u.node->data.objref, 0, 1);
    if (r != NULL)
        return r;
    return NULL;
}

void fn_0044b2d0(IROLinear *p)
{
    SInt32 i;
    UInt32 n;
    VarRecord *v;
    AsmOut vars;

    switch (p->type) {
        case IROLinearOp1Arg:
        case IROLinearOp2Arg:
            if (data_00551d6c[p->nodetype] == 0)
                break;
            v = IroVars_GetOperandVarRecord(p);
            n = 0;
            if (v != NULL)
                n = v->index;
            IroBitVect_SetBit(n, data_00588018);
            if (n != 0)
                break;
            IroBitVect_Or(noregister_bitvector, data_00588018);
            break;

        case IROLinearFunccall:
            IroBitVect_Or(noregister_bitvector, data_00588018);
            break;

        case IROLinearAsm:
            fn_00462d70(p->u.asm_stmt, &vars);
            for (i = 0; i < vars.numoperands; i++) {
                switch (vars.operands[i].type) {
                    case 1:
                    case 2:
                        v = fn_0044ba70(vars.operands[i].object, 0, 1);
                        if (v != NULL)
                            IroBitVect_SetBit(v->index, data_00588018);
                        break;
                }
            }
            if (vars.writesMemory != 0 || vars.branchWithLink != 0)
                IroBitVect_Or(noregister_bitvector, data_00588018);
            break;
    }
}

void IroVars_CheckVariablesInitializedBeforeUse(void)
{
    SInt32 changed;
    ObjectList *local;
    BitVector *temporary;
    IROLinear *statement;
    SInt32 predecessor;
    IRONode *block;
    SInt32 i;
    AsmOut buffer;

    IroJump_MarkReachable(iro_flowgraph_head);
    block = iro_flowgraph_head;
    IroBitVect_AllocateBitVector(&data_00588018, iroVarCount + 1);
    for (; block != NULL; block = block->nextnode) {
        IroBitVect_AllocateBitVector(&block->in, iroVarCount + 1);
        IroBitVect_AllocateBitVector(&block->gen, iroVarCount + 1);
        for (statement = block->first; statement != NULL; statement = statement->next) {
            if (statement->type == IROLinearOperand && statement->u.node->type == EOBJREF) {
                if (statement->flags & IROLF_Ind) {
                    if ((statement->flags & IROLF_Assigned) == 0 || (statement->flags & IROLF_Used) != 0) {
                        VarRecord *variable = fn_0044ba70(statement->u.node->data.objref, 0, 1);
                        if (variable != NULL) {
                            if (!IRO_BV_TEST(block->gen, variable->index))
                                IroBitVect_SetBit(variable->index, block->in);
                        }
                    }
                } else {
                    VarRecord *variable = fn_0044ba70(statement->u.node->data.objref, 0, 0);
                    if (variable != NULL)
                        IroBitVect_SetBit(variable->index, block->gen);
                }
            } else if (statement->type == IROLinearOp2Arg && statement->nodetype == EASS) {
                VarRecord *variable = IroVars_GetOperandVarRecord(statement);
                if (variable != NULL)
                    IroBitVect_SetBit(variable->index, block->gen);
            } else if (statement->type == IROLinearAsm) {
                fn_00462d70(statement->u.asm_stmt, &buffer);
                for (i = 0; i < buffer.numoperands; i++) {
                    VarRecord *variable = fn_0044ba70(buffer.operands[i].object, 0, 1);
                    switch (buffer.operands[i].type) {
                        case 1:
                        case 2:
                            IroBitVect_SetBit(variable->index, block->gen);
                            break;
                    }
                }
            }
            if (statement == block->last)
                break;
        }
    }
    IroBitVect_AllocateBitVector(&temporary, iroVarCount + 1);
    do {
        changed = 0;
        for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
            IroBitVect_CopyBitVector(block->gen, temporary);
            for (predecessor = 0; predecessor < block->numpred; predecessor++)
                IroBitVect_Or(iroNodesByIndex[block->pred[predecessor]]->gen, temporary);
            if (IroBitVect_AreEqual(temporary, block->gen) == 0) {
                IroBitVect_CopyBitVector(temporary, block->gen);
                changed = 1;
            }
        }
    } while (changed);
    IroBitVect_ClearBitVector(data_00588018);
    for (block = iro_flowgraph_head; block != NULL; block = block->nextnode) {
        if (block->reachable != 0) {
            IroBitVect_CopyBitVector(block->in, temporary);
            for (i = 0; i < block->numpred; i++)
                IroBitVect_Subtract(iroNodesByIndex[block->pred[i]]->gen, temporary);
            IroBitVect_Or(temporary, data_00588018);
        }
    }
    for (local = locals; local != NULL; local = local->next) {
        VarRecord *variable = fn_0044ba70(local->object, 0, 1);
        if (variable != NULL) {
            if (IRO_BV_TEST(data_00588018, variable->index)) {
                VarInfo *info = local->object->u.var.info;
                if (!CParser_IsNullOrAtOrDollarPrefixedName(local->object->name) &&
                    !is_volatile_object(local->object) &&
                    (local->object->type->type != TYPECLASS || !CClass_IsEmpty((TypeClass *)local->object->type))) {
                    CError_SetBufferedToken(&info->deftoken);
                    CError_Warning(ERR_VARIABLE_NOT_INITIALIZED_BEFORE_BEING_USED, local->object->name->name);
                }
            }
        }
    }
    IroVars_CheckTimedLongjmp();
}

void RewriteBitFieldTemps(void)
{
    IROLinear *temp = linear_head;

    while (temp != NULL) {
        Object *type = IroDump_GetObjRef(temp);
        if (type != NULL && (temp->flags & 0x80000) != 0) {
            VarRecord *p = fn_0044ba70(type, 0, 1);
            IROLinear *q;
            IROList pair;

            CError_ASSERT(1108, p != 0);
            q = p->bitFieldReplacement;
            CError_ASSERT(1114, q != 0);

            IroUtil_InitList(&pair);
            IroUtil_CopyLinearToList(q, &pair);
            IroUtil_ClearZeroOperands(temp->u.monadic);
            temp->u.monadic = pair.tail;
            IroUtil_InsertLinearBefore(pair.head, pair.tail, temp);
        }
        temp = temp->next;
    }
}

void IRO_ScalarizeClassDataMembers(void)
{
    Object *variable;
    Object *object;
    IROLinear *insertionPoint;
    IROList range;
    AsmOut references;
    IROVarPart *classData;
    IROElmList *member;
    IROLinear *node;
    SInt32 i;
    VarRecord *reference;
    IROLinear *expression;
    IROLinear *replacement;
    IROLinear *objectNode;
    IROLinear *destination;
    IROLinear *source;
    IROLinear *assignment;

    node = linear_head;
    class_data_parts = NULL;
    var_part_use_tail = NULL;
    while (node != NULL) {
        if (node->type == IROLinearOp1Arg && node->nodetype == EINDIRECT)
            record_monadic_var_part_use(node);
        if (node->type == IROLinearAsm) {
            fn_00462d70(node->u.asm_stmt, &references);
            for (i = 0; i < references.numoperands; i++) {
                reference = fn_0044ba70(references.operands[i].object, 0, 1);
                if (reference != NULL)
                    record_var_part_use(node, reference, -1, &stvoid);
                else
                    CError_FATAL(1406);
            }
        }
        node = node->next;
    }

    for (classData = class_data_parts; classData != NULL; classData = classData->next) {
        if ((classData->flags & 1) == 0) {
            variable = create_temp_object(classData->type);
            fn_0044ba70(variable, 1, 1);
            for (member = classData->uses; member != NULL; member = member->next) {
                IroUtil_InitList(&range);
                IroUtil_CopyLinearToList(member->element, &range);
                if (member->element->type == IROLinearOperand && member->element->u.node->type == EOBJREF) {
                    member->element->u.node = create_objectrefnode(variable);
                } else {
                    expression = IroUtil_FindNextUse(member->element);
                    IroUtil_ClearZeroOperands(expression->u.monadic);
                    expression->u.monadic = IrOptimizer_NewLinear(IROLinearOperand);
                    expression->u.monadic->u.node = create_objectrefnode(variable);
                    expression->u.monadic->rtype = member->element->rtype;
                    linear_index_counter++;
                    expression->u.monadic->index = linear_index_counter;
                    expression->u.monadic->flags |= (IROLF_Reffed | IROLF_Ind);
                    IroUtil_ClearZeroOperands(member->element);
                    replacement = expression->u.monadic;
                    IroUtil_InsertLinearRangeAfter(replacement, replacement, member->element);
                }
            }
            object = classData->var->object;
            if (IRO_ListHasObject(object)) {
                objectNode = IrOptimizer_NewLinear(IROLinearOperand);
                objectNode->u.node = create_objectrefnode(variable);
                objectNode->rtype = objectNode->u.node->data.objref->type;
                linear_index_counter++;
                objectNode->index = linear_index_counter;
                objectNode->flags |= (IROLF_Assigned | IROLF_Ind);
                insertionPoint = range.tail;
                destination = IrOptimizer_NewLinear(IROLinearOp1Arg);
                destination->nodetype = EINDIRECT;
                destination->rtype = classData->type;
                destination->u.monadic = objectNode;
                linear_index_counter++;
                destination->index = linear_index_counter;
                destination->flags |= IROLF_Assigned;
                source = IrOptimizer_NewLinear(IROLinearOp1Arg);
                source->nodetype = EINDIRECT;
                source->rtype = classData->type;
                source->u.monadic = insertionPoint;
                linear_index_counter++;
                source->index = linear_index_counter;
                source->flags |= IROLF_Reffed;
                assignment = IrOptimizer_NewLinear(IROLinearOp2Arg);
                assignment->nodetype = EASS;
                assignment->u.diadic.left = destination;
                assignment->u.diadic.right = source;
                assignment->rtype = classData->type;
                linear_index_counter++;
                assignment->index = linear_index_counter;
                insertionPoint->next = source;
                source->next = objectNode;
                objectNode->next = destination;
                destination->next = assignment;
                IroUtil_InsertLinearRangeAfter(range.head, assignment, linear_head);
            }
        }
    }
    IroVars_CheckTimedLongjmp();
}

static int IRO_ListHasObject(Object *key)
{
    ObjectList *a;

    for (a = arguments; a != NULL; a = a->next) {
        if (a->object == key) {
            return 1;
        }
    }
    return 0;
}

SInt32 record_monadic_var_part_use(IROLinear *obj)
{
    IROLinear *n = obj->u.monadic;
    Object *q;
    VarRecord *res;

    if (n->type == IROLinearOperand && n->u.node->type == EOBJREF) {
        q = n->u.node->data.objref;
        if ((q->type->type == TYPECLASS || q->type->type == TYPESTRUCT || q->type->type == TYPEARRAY) &&
            q->datatype == DLOCAL) {
            res = fn_0044ba70(q, 0, 1);
            if (res == NULL)
                CError_FATAL(1575);
            record_var_part_use(obj, res, 0, obj->rtype);
        }
    } else if (n->type == IROLinearOp2Arg && n->nodetype == EADD && n->u.diadic.left->type == IROLinearOperand &&
               n->u.diadic.left->u.node->type == EOBJREF) {
        if (n->u.diadic.right->type == EPOSTDEC && n->u.diadic.right->u.node->type == EINTCONST &&
            n->u.diadic.right->u.node->data.intval.hi == 0) {
            q = n->u.diadic.left->u.node->data.objref;
            if ((q->type->type == TYPECLASS || q->type->type == TYPESTRUCT || q->type->type == TYPEARRAY) &&
                q->datatype == DLOCAL) {
                res = fn_0044ba70(q, 0, 1);
                if (res == NULL)
                    CError_FATAL(1602);
                record_var_part_use(obj, res, n->u.diadic.right->u.node->data.intval.lo, obj->rtype);
            }
        } else {
            q = n->u.diadic.left->u.node->data.objref;
            if ((q->type->type == TYPECLASS || q->type->type == TYPESTRUCT || q->type->type == TYPEARRAY) &&
                q->datatype == DLOCAL) {
                res = fn_0044ba70(q, 0, 1);
                record_var_part_use(obj, res, -1, obj->rtype);
            }
        }
    }
    return 0;
}

int record_var_part_use(IROLinear *source, VarRecord *operand, int offset, Type *type)
{
    IROVarPart *range;
    IROElmList *use;
    IROVarPart *scan;
    IROVarPart *newRange;
    int overlap;
    IROLinear *value;
    overlap = 0;
    value = source->u.monadic;

    if (operand->noregister != 0)
        return 0;
    if (source->rtype != NULL && CParser_IsVolatile(source->rtype, source->nodeflags & 3) != 0)
        offset = -1;
    if (is_volatile_object(operand->object) != 0)
        offset = -1;
    range = class_data_parts;
    while (range != NULL) {
        if (range->var->index == operand->index) {
            if ((range->flags & 1) != 0)
                return 0;
            if (offset == -1) {
                overlap = 1;
                break;
            }
            if (range->offset == offset && IroUtil_AreTypesEqual(range->type, type) != 0) {
                use = (IROElmList *)oalloc(sizeof(*use));
                use->element = value;
                use->next = NULL;
                if (range->uses != NULL) {
                    use->next = range->uses;
                    range->uses = use;
                } else {
                    range->uses = use;
                }
                return 1;
            }
            if (range->offset == offset && IroUtil_AreTypesEqual(range->type, type) == 0) {
                overlap = 1;
                break;
            }
            if (range->offset < offset) {
                UInt32 rangeEnd = range->offset + range->size;
                if (rangeEnd > offset) {
                    overlap = 1;
                    break;
                }
            }
            if (range->offset == offset) {
                UInt32 rangeEnd = range->offset + range->size;
                if (rangeEnd < offset + type->size) {
                    overlap = 1;
                    break;
                }
            }
            if (range->offset == offset) {
                UInt32 rangeEnd = range->offset + range->size;
                if (rangeEnd > offset + type->size) {
                    overlap = 1;
                    break;
                }
            }
            if (range->offset > offset && range->offset < offset + type->size) {
                overlap = 1;
                break;
            }
        }
        range = range->next;
    }
    if (overlap != 0) {
        scan = class_data_parts;
        while (scan != NULL) {
            if (range->var->index == scan->var->index)
                scan->flags |= 1;
            scan = scan->next;
        }
        if (offset >= 0)
            return 0;
    }
    newRange = (IROVarPart *)oalloc(sizeof(*newRange));
    newRange->source = source;
    newRange->flags = 0;
    newRange->var = operand;
    newRange->offset = offset;
    newRange->size = type->size;
    newRange->type = type;
    newRange->next = NULL;
    newRange->uses = (IROElmList *)oalloc(sizeof(*newRange->uses));
    newRange->uses->element = value;
    newRange->uses->next = NULL;
    if ((type->type == TYPEFLOAT || type->type == TYPEINT || type->type == TYPEPOINTER) && offset != -1) {
        if ((value->flags & 16384) != 0) {
            if (fn_0044a6f0_inline1(operand) != 0)
                newRange->flags |= 1;
        }
    } else {
        newRange->flags |= 1;
    }
    if (class_data_parts != NULL)
        var_part_use_tail->next = newRange;
    else
        class_data_parts = newRange;
    var_part_use_tail = newRange;
    return 1;
}
