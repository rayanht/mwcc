#define CERROR_FILE "Scheduler.c"
#include "compiler/common.h"
#include "compiler/Scheduler.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
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
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"

int Scheduler_ReturnZero(PCodeInstruction *list, PCodeInstruction *ref, char c)
{
    SInt32 refid;
    PCodeOperand *r;
    SInt32 i;

    if (ref != NULL) {
        r = &ref->operandData.operands[0];
        if (ref->operand_count >= 1 && (SInt8)r->kind == c && (r->flags & 2) != 0) {
            refid = r->value.reg;
            for (i = 0; i < list->operand_count; i++) {
                PCodeOperand *e = &list->operandData.operands[i];

                if ((SInt8)list->operandData.operands[i].kind == c && (e->flags & 3) != 0 && e->value.reg == refid)
                    return 0;
            }
        }
    }
    return 0;
}

void Scheduler_Schedule(char force)
{
    PCodeBlock *blk;
    SInt32 tgt;

    if (copts.altivecModel || (tgt = copts.processorModel) == 7)
        data_00581b80 = data_00577640;
    else if (tgt == 2)
        data_00581b80 = checkers;
    else if (tgt == 5)
        data_00581b80 = scheduler_checkers;
    else if (tgt == 3)
        data_00581b80 = data_005763f8;
    else if (tgt == 6)
        data_00581b80 = data_005763f8;
    else if (tgt == 4)
        data_00581b80 = data_00576f08;
    else if (tgt == 1)
        data_00581b80 = scheduler_checker_array;
    else if (tgt == 9)
        data_00581b80 = data_00578e30;
    else
        data_00581b80 = checkers;

    for (blk = gPCodeBlocks; blk != NULL; blk = blk->next) {
        if (blk->instruction_count > 2 && (force || (blk->flags & 3) == 0) && (blk->flags & 8) == 0) {
            schedule_block(blk);
            blk->flags |= 8;
        }
    }
}

static inline SInt32 CountScheduled(CColoringList *list)
{
    SInt32 count = 0;
    CColoringList *entry;
    for (entry = list; entry; entry = entry->next)
        if (entry->owner->flag == 1)
            count++;
    return count;
}

void schedule_block(PCodeBlock *function)
{
    CColoringNode *cursor;
    CColoringNode *neighbor;
    PCodeInstruction *selectedObject;
    UInt16 slot;
    UInt16 iteration;
    PCodeInstruction *object;
    CColoringNode *selected;
    CColoringNode *node;
    CColoringNode *tail;

    init_register_owner_lists();
    data_00581b7c = NULL;
    tail = NULL;
    for (object = function->reverse_instructions; object != NULL; object = object->previous) {
        node = (CColoringNode *)CompilerTools_AllocatePoolMemory(sizeof(CColoringNode));
        node->prev = NULL;
        node->next = NULL;
        node->conflicts = NULL;
        node->obj = object;
        node->height = data_00581b80->getLatency(object);
        node->latency = node->height;
        node->earliestCycle = 0;
        node->latestCycle = 0;
        node->flag = 0;
        build_sched_dependencies(tail, node);
        if (object->flags & fIsBranch)
            data_00581b7c = node;
        if (tail != NULL)
            tail->next = node;
        node->prev = tail;
        tail = node;
    }
    for (cursor = tail; cursor != NULL; cursor = cursor->prev)
        cursor->latestCycle = max_height - cursor->height;
    function->reverse_instructions = NULL;
    function->instructions = function->reverse_instructions;
    function->instruction_count = 0;
    data_00581b80->beginScheduling();
    iteration = 0;
    while (tail != NULL) {
        slot = 0;
        while (slot < data_00581b80->count) {
            CColoringList *conflict;
            if (tail == NULL)
                break;
            selected = select_ready_coloring_node(tail, iteration);
            if (selected == NULL)
                break;
            selectedObject = selected->obj;
            if (selected->conflicts != NULL) {
                for (conflict = selected->conflicts; conflict != NULL; conflict = conflict->next) {
                    neighbor = conflict->owner;
                    neighbor->flag--;
                    if (neighbor->earliestCycle < conflict->value + iteration)
                        neighbor->earliestCycle = conflict->value + iteration;
                }
            }
            PCode_AppendInstruction(function, selectedObject);
            data_00581b80->issueInstruction(selectedObject);
            if (selected->next != NULL)
                selected->next->prev = selected->prev;
            else
                tail = selected->prev;
            if (selected->prev != NULL)
                selected->prev->next = selected->next;
            slot++;
        }
        data_00581b80->advanceCycle();
        iteration++;
    }
    CompilerTools_ResetPool();
}

struct CColoringNode *select_ready_coloring_node(struct CColoringNode *list, UInt16 id)
{
    CColoringNode *best;
    CColoringNode *candidate;
    SInt32 candidateCount;
    SInt32 bestCount;

    best = list;
    while (best) {
        if (best->flag == 0 && best->earliestCycle <= id && data_00581b80->check(best->obj))
            break;
        best = best->prev;
    }
    if (best == NULL)
        return NULL;
    for (candidate = best->prev; candidate; candidate = candidate->prev) {
        if (candidate->flag != 0)
            continue;
        if (candidate->earliestCycle > id)
            continue;
        if (!data_00581b80->check(candidate->obj))
            continue;
        if (best->latestCycle > id || candidate->latestCycle <= id) {
            if (!(best->latestCycle > id && candidate->latestCycle <= id)) {
                candidateCount = CountScheduled(candidate->conflicts);
                bestCount = CountScheduled(best->conflicts);
                if (bestCount > candidateCount)
                    ;
                else if (bestCount < candidateCount)
                    best = candidate;
                else if (best->height > candidate->height)
                    ;
                else if (best->height < candidate->height)
                    best = candidate;
                else if (gVirtualRegistersActive != 0) {
                    if (gPCodeOpcodeDescriptors[best->obj->opcode].rank <
                        gPCodeOpcodeDescriptors[candidate->obj->opcode].rank)
                        ;
                    else if (gPCodeOpcodeDescriptors[best->obj->opcode].rank <=
                             gPCodeOpcodeDescriptors[candidate->obj->opcode].rank)
                        ;
                    else
                        best = candidate;
                }
            } else {
                best = candidate;
            }
        }
    }
    return best;
}

void build_sched_dependencies(CColoringNode *list, CColoringNode *blk)
{
    PCodeInstruction *body;
    PCodeOperand *item;
    SchedEntry *n;
    CColoringNode *p;
    SInt32 i;
    SInt32 reg;

    body = blk->obj;
    item = body->operandData.operands;
    for (i = 0; i < body->operand_count; i++, item++) {
        switch (item->kind) {
            case PCOp_GPR:
                CError_ASSERT(600,
                              item->value.reg == stack_base_reg ||
                                  (item->value.reg >= 0 &&
                                   item->value.reg <= (gVirtualRegistersActive ? gUsedVirtualRegistersGPR : 0x1f)));
                if ((SInt32)item->value.reg != 2 && (SInt32)item->value.reg != 13 &&
                    !((SInt32)item->value.reg == 0 && (item->flags & 3) == 0)) {
                    fn_004cd7c0(0, blk, &gpr_owner_lists[item->value.reg], &data_00581b00[item->value.reg],
                                item->flags & 2);
                }
                break;
            case PCOp_FPR:
                CError_ASSERT(628, item->value.reg >= 0 &&
                                       item->value.reg <= (gVirtualRegistersActive ? gUsedVirtualRegistersFPR : 0x1f));
                fn_004cd7c0(1, blk, &fpr_owner_lists[item->value.reg], &data_00581b08[item->value.reg],
                            item->flags & 2);
                break;
            case PCOp_VR:
                CError_ASSERT(634, item->value.reg >= 0 &&
                                       item->value.reg <= (gVirtualRegistersActive ? gUsedVirtualRegistersVR : 0x1f));
                fn_004cd7c0(9, blk, &register_owner_lists[item->value.reg],
                            &virtual_register_owner_lists[item->value.reg], item->flags & 2);
                break;
            case PCOp_SPR:
                CError_ASSERT(640, item->value.reg >= 0 && item->value.reg <= 3);
                fn_004cd7c0(2, blk, &data_00581b24[item->value.reg], &data_00581b18[item->value.reg], item->flags & 2);
                break;
            case PCOp_CRFIELD:
                CError_ASSERT(645, item->value.reg >= 0 && item->value.reg <= 7);
                fn_004cd7c0(3, blk, &data_00581b50[item->value.reg], &data_00581b30[item->value.reg], item->flags & 2);
                break;
            case PCOp_MEMORY:
                if ((body->flags & (fIsRead | fIsWrite)) != 0) {
                    if ((body->flags & fIsVolatile) != 0)
                        fn_004cd650(blk, item->object, body->flags & fIsWrite);
                    else
                        check_and_add_spill_node(blk, item->object, body->flags & fIsWrite);
                }
                break;
            case PCOp_IMMEDIATE:
                if ((body->flags & (fIsRead | fIsWrite)) != 0 && item->object != NULL) {
                    if ((body->flags & fIsVolatile) != 0)
                        fn_004cd650(blk, item->object, body->flags & fIsWrite);
                    else
                        check_and_add_spill_node(blk, item->object, body->flags & fIsWrite);
                }
                break;
        }
    }

    if ((body->flags & fIsPtrOp) != 0)
        add_memory_dependencies(blk, body->flags & fIsWrite, body->flags & fIsVolatile);

    if ((body->flags & 0x420) != 0 || data_00581b80->checkLate(body) != 0) {
        for (p = list; p != NULL; p = p->prev)
            add_dependency(blk, p, 0);
        n = (struct SchedEntry *)CompilerTools_AllocatePoolMemory(sizeof(*n));
        n->blk = blk;
        n->zero = 0;
        n->next = sched_entry_list;
        sched_entry_list = n;
    }

    if (sched_entry_list != NULL) {
        for (n = sched_entry_list; n != NULL; n = n->next) {
            if (n->blk != blk)
                add_dependency(blk, n->blk, 0);
        }
    }
    if (blk->conflicts == NULL && data_00581b7c != NULL)
        add_dependency(blk, data_00581b7c, 0);
    if (blk->height > max_height)
        max_height = blk->height;
}

void add_memory_dependencies(CColoringNode *owner, int mode, int flags)
{
    DependencyEntry *node;
    if (mode != 0) {
        for (node = memory_dependency_list; node != NULL; node = node->next) {
            if (!node->object || node->object->datatype == DDATA || PCodeUtilities_Require(node->object) ||
                (node->object->datatype == DLOCAL &&
                 (node->object->u.var.info->noregister || node->object->type->type == TYPEARRAY ||
                  node->object->type->type == TYPESTRUCT || node->object->type->type == TYPECLASS ||
                  (node->object->type->type == TYPEMEMBERPOINTER && node->object->type->size == 12))))
                add_dependency(owner, node->owner, 1);
        }
        for (node = dependency_entry_list; node != NULL; node = node->next) {
            if (!node->object || node->object->datatype == DDATA || PCodeUtilities_Require(node->object) ||
                (node->object->datatype == DLOCAL &&
                 (node->object->u.var.info->noregister || node->object->type->type == TYPEARRAY ||
                  node->object->type->type == TYPESTRUCT || node->object->type->type == TYPECLASS ||
                  (node->object->type->type == TYPEMEMBERPOINTER && node->object->type->size == 12))))
                add_dependency(owner, node->owner, 1);
        }
        node = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
        node->next = NULL;
        node->owner = owner;
        node->object = NULL;
        node->object = NULL;
        node->next = dependency_entry_list;
        dependency_entry_list = node;
    } else {
        for (node = dependency_entry_list; node != NULL; node = node->next) {
            if (!node->object || node->object->datatype == DDATA || PCodeUtilities_Require(node->object) ||
                (node->object->datatype == DLOCAL &&
                 (node->object->u.var.info->noregister || node->object->type->type == TYPEARRAY ||
                  node->object->type->type == TYPESTRUCT || node->object->type->type == TYPECLASS ||
                  (node->object->type->type == TYPEMEMBERPOINTER && node->object->type->size == 12))))
                add_dependency(owner, node->owner, 1);
        }
        node = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
        node->next = NULL;
        node->owner = owner;
        node->object = NULL;
        node->object = NULL;
        node->next = memory_dependency_list;
        memory_dependency_list = node;
    }
}

static inline void CheckSpillNode(CColoringNode *a1, Object *a2, DependencyEntry *p)
{
    if (p->object == a2 || (p->object == NULL && (a2->datatype == DDATA || PCodeUtilities_Require(a2) != 0 ||
                                                  (a2->datatype == DLOCAL &&
                                                   (a2->u.var.info->noregister != 0 || a2->type->type == TYPEARRAY ||
                                                    (UInt8)(a2->type->type - TYPESTRUCT) <= (TYPECLASS - TYPESTRUCT) ||
                                                    (a2->type->type == TYPEMEMBERPOINTER && a2->type->size == 12)))))) {
        add_dependency(a1, p->owner, 1);
    }
}

static inline DependencyEntry *NewSpillNode(CColoringNode *obj)
{
    DependencyEntry *node = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
    node->next = NULL;
    node->owner = obj;
    node->object = NULL;
    return node;
}

void check_and_add_spill_node(CColoringNode *ownerObject, Object *object, SInt16 checkAllSpills)
{
    DependencyEntry *spill;
    DependencyEntry *newSpill;

    if (checkAllSpills != 0) {
        for (spill = memory_dependency_list; spill != NULL; spill = spill->next) {
            CheckSpillNode(ownerObject, object, spill);
        }
        for (spill = dependency_entry_list; spill != NULL; spill = spill->next) {
            CheckSpillNode(ownerObject, object, spill);
        }
        newSpill = NewSpillNode(ownerObject);
        newSpill->object = object;
        newSpill->next = dependency_entry_list;
        dependency_entry_list = newSpill;
    } else {
        for (spill = dependency_entry_list; spill != NULL; spill = spill->next) {
            CheckSpillNode(ownerObject, object, spill);
        }
        newSpill = NewSpillNode(ownerObject);
        newSpill->object = object;
        newSpill->next = memory_dependency_list;
        memory_dependency_list = newSpill;
    }
}

static inline void UpdateDependencyHeight(CColoringNode *owner, CColoringNode *dependency, CColoringList *node)
{
    if (dependency->height + node->value > owner->height)
        owner->height = dependency->height + node->value;
}

static inline SInt32 DependencyLatency(CColoringNode *owner)
{
    return owner->latency;
}

static inline void CountDependency(CColoringNode *dependency)
{
    dependency->flag++;
}

void fn_004cd650(void *object, Object *key, SInt16 useList74)
{
    typedef struct ReferenceRecord {
        UInt8 reserved[0xc];
        ReferenceFlags *inner;
    } ReferenceRecord;
    DependencyEntry *node;
    for (node = memory_dependency_list; node != NULL; node = node->next) {
        UInt8 type;
        ReferenceRecord *reference = (ReferenceRecord *)node->owner;
        if ((reference->inner->flags & 0x20000) ||
            (useList74 != 0 &&
             (node->object == key ||
              (node->object == NULL &&
               (key->datatype == DDATA || PCodeUtilities_Require(key) ||
                (key->datatype == DLOCAL &&
                 (key->u.var.info->noregister != 0 || (type = key->type->type) == TYPEARRAY || type == TYPESTRUCT ||
                  type == TYPECLASS || (type == TYPEMEMBERPOINTER && key->type->size == 0xc))))))))
            add_dependency(object, node->owner, 1);
    }
    for (node = dependency_entry_list; node != NULL; node = node->next) {
        UInt8 type;
        ReferenceRecord *reference = (ReferenceRecord *)node->owner;
        if ((reference->inner->flags & 0x20000) ||
            (node->object == key ||
             (node->object == NULL &&
              (key->datatype == DDATA || PCodeUtilities_Require(key) ||
               (key->datatype == DLOCAL &&
                (key->u.var.info->noregister != 0 || (type = key->type->type) == TYPEARRAY || type == TYPESTRUCT ||
                 type == TYPECLASS || (type == TYPEMEMBERPOINTER && key->type->size == 0xc)))))))
            add_dependency(object, node->owner, 1);
    }
    if (useList74 != 0) {
        node = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
        node->next = NULL;
        node->owner = object;
        node->object = NULL;
        node->object = key;
        node->next = dependency_entry_list;
        dependency_entry_list = node;
    } else {
        node = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
        node->next = NULL;
        node->owner = object;
        node->object = NULL;
        node->object = key;
        node->next = memory_dependency_list;
        memory_dependency_list = node;
    }
}

void fn_004cd7c0(int kind, CColoringNode *value, DependencyEntry **firstList, DependencyEntry **secondList,
                 int useSecondList)
{
    DependencyEntry *first;
    DependencyEntry *other;
    Boolean flag;
    DependencyEntry *second;
    DependencyEntry *entry;
    Boolean otherFlag;
    if (useSecondList != 0) {
        second = (DependencyEntry *)*secondList;
        if (second != NULL) {
            do {
                if (second->owner != value) {
                    add_dependency(value, second->owner, 1);
                }
                second = second->next;
            } while (second != NULL);
        }
        first = (DependencyEntry *)*firstList;
        if (first != NULL) {
            do {
                if (first->owner != value) {
                    flag = (kind != 0 && kind != 1 && kind != 9) || !data_00581b80->omitRegisterAntiDependencyLatency;
                    add_dependency(value, first->owner, flag);
                }
                first = first->next;
            } while (first != NULL);
        }
        entry = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
        entry->next = NULL;
        entry->owner = value;
        entry->object = NULL;
        entry->next = (DependencyEntry *)*firstList;
        *firstList = (DependencyEntry *)entry;
    } else {
        other = (DependencyEntry *)*firstList;
        if (other != NULL) {
            do {
                if (other->owner != value) {
                    otherFlag =
                        (kind != 0 && kind != 1 && kind != 9) || !data_00581b80->omitRegisterAntiDependencyLatency;
                    add_dependency(value, other->owner, otherFlag);
                }
                other = other->next;
            } while (other != NULL);
        }
        entry = (DependencyEntry *)CompilerTools_AllocatePoolMemory(sizeof(DependencyEntry));
        entry->next = NULL;
        entry->owner = value;
        entry->object = NULL;
        entry->next = (DependencyEntry *)*secondList;
        *secondList = (DependencyEntry *)entry;
    }
}

void add_dependency(CColoringNode *owner, CColoringNode *dependency, Boolean hasLatency)
{
    CColoringList *node;
    SInt32 dependencyDelay;

    if (owner == dependency)
        return;

    node = owner->conflicts;
    while (node != NULL) {
        if (node->owner == dependency) {
            if (node->value < hasLatency)
                dependencyDelay = DependencyLatency(owner);
            else
                dependencyDelay = 0;
            if (dependencyDelay != 0) {
                node->value = hasLatency ? DependencyLatency(owner) : 0;
                UpdateDependencyHeight(owner, dependency, node);
            }
            return;
        }
        node = node->next;
    }

    node = (CColoringList *)CompilerTools_AllocatePoolMemory(sizeof(*node));
    node->owner = dependency;
    node->next = owner->conflicts;
    owner->conflicts = node;
    node->value = hasLatency ? DependencyLatency(owner) : 0;
    CountDependency(dependency);
    UpdateDependencyHeight(owner, dependency, node);
}

void init_register_owner_lists(void)
{
    int gprCount, fprCount, vrCount;
    int i;

    gprCount = gVirtualRegistersActive ? gUsedVirtualRegistersGPR : 32;
    fprCount = gVirtualRegistersActive ? gUsedVirtualRegistersFPR : 32;
    vrCount = gVirtualRegistersActive ? gUsedVirtualRegistersVR : 32;

    data_00581b00 = (DependencyEntry **)CompilerTools_AllocatePoolMemory(gprCount * sizeof(*data_00581b00));
    gpr_owner_lists = (DependencyEntry **)CompilerTools_AllocatePoolMemory(gprCount * sizeof(*gpr_owner_lists));
    for (i = 0; i < gprCount; i++)
        data_00581b00[i] = gpr_owner_lists[i] = NULL;

    data_00581b08 = (DependencyEntry **)CompilerTools_AllocatePoolMemory(fprCount * sizeof(*data_00581b08));
    fpr_owner_lists = (DependencyEntry **)CompilerTools_AllocatePoolMemory(fprCount * sizeof(*fpr_owner_lists));
    for (i = 0; i < fprCount; i++)
        data_00581b08[i] = fpr_owner_lists[i] = NULL;

    virtual_register_owner_lists =
        (DependencyEntry **)CompilerTools_AllocatePoolMemory(vrCount * sizeof(*virtual_register_owner_lists));
    register_owner_lists =
        (DependencyEntry **)CompilerTools_AllocatePoolMemory(vrCount * sizeof(*register_owner_lists));
    for (i = 0; i < vrCount; i++)
        virtual_register_owner_lists[i] = register_owner_lists[i] = NULL;

    data_00581b30[0] = data_00581b50[0] = NULL;
    DAT_00581b34 = DAT_00581b54 = 0;
    DAT_00581b38 = DAT_00581b58 = 0;
    DAT_00581b3c = DAT_00581b5c = 0;
    DAT_00581b40 = DAT_00581b60 = 0;
    DAT_00581b44 = DAT_00581b64 = 0;
    DAT_00581b48 = DAT_00581b68 = 0;
    DAT_00581b4c = DAT_00581b6c = 0;
    data_00581b18[0] = data_00581b24[0] = NULL;
    DAT_00581b1c = DAT_00581b28 = 0;
    DAT_00581b20 = DAT_00581b2c = 0;
    memory_dependency_list = dependency_entry_list = NULL;
    sched_entry_list = NULL;
    max_height = 0;
}
