#define CERROR_FILE "Exceptions.c"
#include "compiler/common.h"
#include "compiler/Exceptions.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CodeGen.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"

static struct ExceptionScopeEntry *exception_scope_entries;
static struct ExceptionScopeEntry *last_exception_scope_entry;
static GList exception_records;
static struct ObjGenRelocationRequest *exception_table_relocation_requests;
static struct ObjGenRelocationRequest *relocation_request_tail;
#pragma opt_lifetimes off

static inline unsigned short flags(void)
{
    return CTool_EndianConvertWord16(gGPRSaveSpan << 11 | (gFPRSaveSpan & 31) << 6 | (data_005883ee != 0) << 5 |
                                     (data_0058852d & 1) << 4 | 8);
}

void Exceptions_EmitExceptionTable(Object *object, int offset)
{
    unsigned long size;
    PCodeBlock *block;
    PCodeInstruction *end;
    ExceptionScopeEntry *range;
    ExceptionScopeEntry *entryRange;
    int endOffset;
    ExceptionTableHeader *header;
    int tableCursor;
    PCodeInstruction *location;
    unsigned int startOffset;
    ExceptionAlignmentFill fill;

    fill.zero = 0;
    endOffset = compact_exception_scope_entries();
    InitGList(&exception_records, 256);
    AppendGListNoData(&exception_records, (endOffset << 3) + 4);
    AppendGListLong(&exception_records, 0);
    for (range = exception_scope_entries; range != NULL; range = range->next) {
        if (range->info->child_count == 0 && range->info->recordOffset == 0) {
            if (((size = exception_records.size) & 3) != 0)
                AppendGListData(&exception_records, &fill, tableCursor = ((size + 3) & -4) - size);
            emit_exception_records(range->info);
        }
    }
    header = (ExceptionTableHeader *)(tableCursor = (int)*exception_records.data);
    if (copts.altivec_model != 0 && gVRSaveSpan != 0) {
        header->flags = flags();
        header->flags = (short)(header->flags | 4);
        if (copts.altivec_vrsave != 0)
            header->value = CTool_EndianConvertWord16(gVRSaveSpan << 11 | 1024);
        else
            header->value = CTool_EndianConvertWord16(gVRSaveSpan << 11);
    } else {
        header->flags = flags();
        header->value = 0;
    }
    tableCursor += sizeof(*header);
    entryRange = exception_scope_entries;
    while (entryRange != NULL) {
        endOffset = (int)entryRange->start;
        block = ((PCodeInstruction *)endOffset)->block;
        (void)block;
        size = (unsigned long)((PCodeInstruction *)endOffset)->previous;
        (void)size;
        startOffset = block->code_offset;
        while (size != 0) {
            size = (unsigned long)((PCodeInstruction *)size)->previous;
            startOffset += 4;
        }
        block = (end = entryRange->end)->block;
        location = end->previous;
        (void)location;
        endOffset = block->code_offset;
        while (location != NULL) {
            location = location->previous;
            endOffset += 4;
        }
        CError_ASSERT(953, ((unsigned int)(endOffset - startOffset) >> 2 & -65536) == 0);
        ((ExceptionTableEntry *)tableCursor)->offset = CTool_EndianConvertWord32(startOffset + 4);
        ((ExceptionTableEntry *)tableCursor)->length =
            CTool_EndianConvertWord16((unsigned int)(endOffset - startOffset) >> 2);
        ((ExceptionTableEntry *)tableCursor)->value = CTool_EndianConvertWord16(entryRange->info->recordOffset);
        tableCursor += sizeof(ExceptionTableEntry);
        entryRange = entryRange->next;
    }
    LockGList(&exception_records);
    ObjGen_PPC_EABI_EmitDescriptorWithRelocations(object, offset, *exception_records.data, exception_records.size,
                                                  exception_table_relocation_requests);
    FreeGList(&exception_records);
}

#pragma opt_lifetimes reset

static inline int Exceptions_GetBoundUID(void *object)
{
    if (!Registers_GetInfo(object))
        return 0;
    return Registers_GetInfo(object)->reg;
}

static void RemoveEntry(ExceptionScopeEntry *e)
{
    if (e->previous)
        e->previous->next = e->next;
    else
        exception_scope_entries = e->next;
    if (e->next)
        e->next->previous = e->previous;
}

int compact_exception_scope_entries(void)
{
    ExceptionScopeEntry *e;
    ExceptionScopeEntry *prev;
    int count;

    if (exception_scope_entries == NULL)
        return 0;

    for (e = exception_scope_entries; e != NULL; e = e->next) {
        if (((PCodeInstruction *)e->start)->block->flags & 0x10)
            RemoveEntry(e);
    }

    if (exception_scope_entries == NULL)
        return 0;

    for (e = exception_scope_entries; e != NULL; e = e->next) {
        e->info = e->elements ? find_or_create_object_group(e->elements) : NULL;
    }

    prev = exception_scope_entries;
    for (e = exception_scope_entries->next; e != NULL; e = e->next) {
        if (e->info == prev->info) {
            prev->end = e->end;
            RemoveEntry(e);
        } else {
            prev = e;
        }
    }

    count = 0;
    for (e = exception_scope_entries; e != NULL; e = e->next) {
        if (e->elements == NULL)
            RemoveEntry(e);
        else
            count++;
    }
    return count;
}

void Exceptions_AppendScopeEntry(PCodeInstruction *context, ExceptionAction *elements)
{
    ExceptionScopeEntry *entry;

    if (elements == NULL) {
        if (last_exception_scope_entry == NULL || last_exception_scope_entry->elements == NULL)
            return;
    }
    entry = (ExceptionScopeEntry *)lalloc(0x18);
    entry->next = NULL;
    entry->end = context;
    entry->start = entry->end;
    entry->elements = elements;
    entry->previous = last_exception_scope_entry;
    if (last_exception_scope_entry != NULL)
        last_exception_scope_entry->next = entry;
    else
        exception_scope_entries = entry;
    last_exception_scope_entry = entry;
    while (elements != NULL) {
        if (elements->kind == 13 && elements->data.catch_block.label->pclabel)
            PCode_AddSuccessor(context->block, elements->data.catch_block.label->pclabel);
        else if (elements->kind == 15 && elements->data.specification.label->pclabel)
            PCode_AddSuccessor(context->block, elements->data.specification.label->pclabel);
        elements = elements->next;
    }
}

void Exceptions_CollectRegisterOperands(ExceptionAction *node, PCodeOperand *out)
{
    int uid;

    while (node != NULL) {
        switch (node->kind) {
            case 2:
                if ((uid = Registers_GetInfo(node->data.pair.second) ? Registers_GetInfo(node->data.pair.second)->reg
                                                                     : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 4:
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 7:
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 17:
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 8:
                if ((uid = Registers_GetInfo(node->data.pair.second) ? Registers_GetInfo(node->data.pair.second)->reg
                                                                     : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 9:
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 10:
            case 11:
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
            case 12:
                if ((uid = Registers_GetInfo(node->data.delete_pointer_cond.cond)
                               ? Registers_GetInfo(node->data.delete_pointer_cond.cond)->reg
                               : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                if ((uid = Registers_GetInfo(node->data.pair.first) ? Registers_GetInfo(node->data.pair.first)->reg
                                                                    : 0) != 0) {
                    out->kind = PCOp_GPR;
                    out->value.reg = uid;
                    out->flags = 1;
                    ++out;
                }
                break;
        }
        node = node->next;
    }
}

static inline int Exceptions_BoundObjectCount(Object **objectSlot)
{
    if (Registers_GetInfo(*objectSlot))
        return Registers_GetInfo(*objectSlot)->reg;
    return 0;
}

int Exceptions_CountBoundObjectFields(ExceptionAction *action)
{
    int count = 0;

    while (action != NULL) {
        switch (action->kind) {
            case 2:
                if (Exceptions_BoundObjectCount(&action->data.local_cond.cond))
                    count++;
                break;
            case 4:
                if (Exceptions_BoundObjectCount(&action->data.pair.first))
                    count++;
                break;
            case 7:
                if (Exceptions_BoundObjectCount(&action->data.member.objectptr))
                    count++;
                break;
            case 17:
                if (Exceptions_BoundObjectCount(&action->data.member.objectptr))
                    count++;
                break;
            case 8:
                if (Exceptions_BoundObjectCount(&action->data.member_cond.cond))
                    count++;
                if (Exceptions_BoundObjectCount(&action->data.member_cond.objectptr))
                    count++;
                break;
            case 9:
                if (Exceptions_BoundObjectCount(&action->data.member_array.objectptr))
                    count++;
                break;
            case 10:
            case 11:
                if (Exceptions_BoundObjectCount(&action->data.pair.first))
                    count++;
                break;
            case 12:
                if (Exceptions_BoundObjectCount(&action->data.delete_pointer_cond.cond))
                    count++;
                if (Exceptions_BoundObjectCount(&action->data.pair.first))
                    count++;
                break;
        }
        action = action->next;
    }
    return count;
}

void Exceptions_Reset(void)

{
    int i;

    for (i = 0; i < 0x12; i = i + 1) {
        object_groups[i] = NULL;
    }
    exception_scope_entries = last_exception_scope_entry = NULL;
    exception_table_relocation_requests = relocation_request_tail = NULL;
    return;
}

static inline void append_reference(void *obj, SInt32 offset)
{
    ObjGenRelocationRequest *it;
    it = (ObjGenRelocationRequest *)lalloc(16);
    it->next = NULL;
    it->object = obj;
    it->offset = offset;
    it->reserved0c[0] = 0;
    if (exception_table_relocation_requests != NULL)
        relocation_request_tail->next = it;
    else
        exception_table_relocation_requests = it;
    relocation_request_tail = it;
}

static inline SInt32 ExceptionTypeValue(CLabel *typeInfo)
{
    return typeInfo->pclabel->target.block->code_offset;
}

static inline int ExceptionHasTypeValue(CLabel *typeInfo)
{
    return typeInfo->pclabel != NULL;
}

void emit_exception_records(ObjectGroup *node)
{
    ExceptionOffsetReference offsetReference;
    ExceptionTwoOffsetsReference twoOffsetsReference;
    ExceptionOffsetReference adjustedOffsetReference;
    ExceptionOffsetReference optionalOffsetReference;
    ExceptionTwoOffsetsReference offsetAndValuesReference;
    ExceptionOffsetValueReference offsetValueReference;
    ExceptionOffsetValueReference alternateOffsetValueReference;
    ExceptionTwoOffsetsValueReference twoOffsetsValueReference;
    ExceptionOffsetValuesReference offsetValuesReference;
    ExceptionOffsetReference kind10Reference;
    ExceptionTwoOffsetsReference kind12Reference;
    ExceptionTypeReferenceRecord typeReference;
    ExceptionShortRecord shortRecord;
    ExceptionReferenceTableRecord referenceTable;
    ExceptionByteRecord byteRecord;
    ExceptionShortRecord linkRecord;
    ExceptionAction *record;
    SInt32 recordOffset;
    unsigned int baseOffset;
    SInt32 offset;
    SInt32 firstOffset;
    SInt32 thirdOffset;
    SInt32 secondOffset;
    SInt32 referenceIndex;
    SInt32 referenceOffset;
    SInt32 flags;

    while (node != NULL && (node->recordOffset == 0 || node->parent == NULL)) {
        recordOffset = exception_records.size;
        if (node->recordOffset == 0)
            node->recordOffset = recordOffset;
        if (node->parent != NULL)
            flags = 0;
        else
            flags = 0x80;
        switch ((record = node->object)->kind) {
            case 0:
                CError_FATAL(149);
                break;
            case 1:
                offsetReference.kindFlags = flags | 2;
                offsetReference.offsetFlags = 0;
                if (Registers_GetInfo(record->data.local.object)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                offsetReference.offset = CTool_EndianConvertWord16(record->data.local.object->u.var.uid + baseOffset);
                offsetReference.reference = 0;
                AppendGListData(&exception_records, &offsetReference, sizeof(offsetReference));
                append_reference(record->data.local.dtor, recordOffset + 4);
                break;
            case 2:
                if (Registers_GetInfo(record->data.local_cond.cond) != NULL)
                    offset = Registers_GetInfo(record->data.local_cond.cond)->reg;
                else
                    offset = 0;
                twoOffsetsReference.kindFlags = flags | 3;
                twoOffsetsReference.offsetFlags = (offset ? 1 : 0) << 7;
                if (offset == 0) {
                    if (Registers_GetInfo(record->data.local_cond.cond)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    offset = record->data.local_cond.cond->u.var.uid + baseOffset;
                }
                twoOffsetsReference.firstOffset = CTool_EndianConvertWord16(offset);
                if (Registers_GetInfo(record->data.local_cond.object)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                twoOffsetsReference.secondOffset =
                    CTool_EndianConvertWord16(record->data.local_cond.object->u.var.uid + baseOffset);
                twoOffsetsReference.reference = 0;
                AppendGListData(&exception_records, &twoOffsetsReference, sizeof(twoOffsetsReference));
                append_reference(record->data.local_cond.dtor, recordOffset + 8);
                break;
            case 3:
                adjustedOffsetReference.offsetFlags = 0;
                adjustedOffsetReference.kindFlags = flags | 2;
                if (Registers_GetInfo(record->data.local.object)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                adjustedOffsetReference.offset = CTool_EndianConvertWord16(record->data.local.object->u.var.uid +
                                                                           baseOffset + record->data.local.offset);
                adjustedOffsetReference.reference = 0;
                AppendGListData(&exception_records, &adjustedOffsetReference, sizeof(adjustedOffsetReference));
                append_reference(record->data.local.dtor, recordOffset + 4);
                break;
            case 4:
                if (Registers_GetInfo(record->data.local_pointer.pointer) != NULL)
                    offset = Registers_GetInfo(record->data.local_pointer.pointer)->reg;
                else
                    offset = 0;
                optionalOffsetReference.kindFlags = flags | 4;
                optionalOffsetReference.offsetFlags = (offset ? 1 : 0) << 7;
                if (offset == 0) {
                    if (Registers_GetInfo(record->data.local_pointer.pointer)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    offset = record->data.local_pointer.pointer->u.var.uid + baseOffset;
                }
                optionalOffsetReference.offset = CTool_EndianConvertWord16(offset);
                optionalOffsetReference.reference = 0;
                AppendGListData(&exception_records, &optionalOffsetReference, sizeof(optionalOffsetReference));
                append_reference(record->data.local_pointer.dtor, recordOffset + 4);
                break;
            case 5:
                offsetAndValuesReference.offsetFlags = 0;
                offsetAndValuesReference.kindFlags = flags | 5;
                if (Registers_GetInfo(record->data.member_array.objectptr)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                offsetAndValuesReference.firstOffset =
                    CTool_EndianConvertWord16(record->data.member_array.objectptr->u.var.uid + baseOffset);
                offsetAndValuesReference.secondOffset =
                    CTool_EndianConvertWord16((UInt32)record->data.member_array.offset);
                offsetAndValuesReference.value = CTool_EndianConvertWord16((UInt32)record->data.member_array.count);
                offsetAndValuesReference.reference = 0;
                AppendGListData(&exception_records, &offsetAndValuesReference, sizeof(offsetAndValuesReference));
                append_reference(record->data.member_array.dtor, recordOffset + 8);
                break;
            case 7:
                if (Registers_GetInfo(record->data.member.objectptr) != NULL)
                    offset = Registers_GetInfo(record->data.member.objectptr)->reg;
                else
                    offset = 0;
                offsetValueReference.kindFlags = flags | 7;
                offsetValueReference.offsetFlags = (offset ? 1 : 0) << 7;
                if (offset == 0) {
                    if (Registers_GetInfo(record->data.member.objectptr)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    offset = record->data.member.objectptr->u.var.uid + baseOffset;
                }
                offsetValueReference.offset = CTool_EndianConvertWord16(offset);
                offsetValueReference.value = CTool_EndianConvertWord32(record->data.member.offset);
                offsetValueReference.reference = 0;
                AppendGListData(&exception_records, &offsetValueReference, sizeof(offsetValueReference));
                append_reference(record->data.member.dtor, recordOffset + 8);
                break;
            case 17:
                if (Registers_GetInfo(record->data.member.objectptr) != NULL)
                    offset = Registers_GetInfo(record->data.member.objectptr)->reg;
                else
                    offset = 0;
                alternateOffsetValueReference.kindFlags = flags | 6;
                alternateOffsetValueReference.offsetFlags = (offset ? 1 : 0) << 7;
                if (offset == 0) {
                    if (Registers_GetInfo(record->data.member.objectptr)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    offset = record->data.member.objectptr->u.var.uid + baseOffset;
                }
                alternateOffsetValueReference.offset = CTool_EndianConvertWord16(offset);
                alternateOffsetValueReference.value = CTool_EndianConvertWord32(record->data.member.offset);
                alternateOffsetValueReference.reference = 0;
                AppendGListData(&exception_records, &alternateOffsetValueReference,
                                sizeof(alternateOffsetValueReference));
                append_reference(record->data.member.dtor, recordOffset + 8);
                break;
            case 8:
                if (Registers_GetInfo(record->data.member_cond.cond) != NULL)
                    secondOffset = Registers_GetInfo(record->data.member_cond.cond)->reg;
                else
                    secondOffset = 0;
                if (Registers_GetInfo(record->data.member_cond.objectptr) != NULL)
                    firstOffset = Registers_GetInfo(record->data.member_cond.objectptr)->reg;
                else
                    firstOffset = 0;
                twoOffsetsValueReference.kindFlags = flags | 8;
                twoOffsetsValueReference.offsetFlags =
                    ((secondOffset ? 1 : 0) << 7) | (((firstOffset ? 1 : 0) & 1) << 6);
                if (secondOffset == 0) {
                    if (Registers_GetInfo(record->data.member_cond.cond)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    secondOffset = record->data.member_cond.cond->u.var.uid + baseOffset;
                }
                twoOffsetsValueReference.firstOffset = CTool_EndianConvertWord16(secondOffset);
                if (firstOffset == 0) {
                    if (Registers_GetInfo(record->data.member_cond.objectptr)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    firstOffset = record->data.member_cond.objectptr->u.var.uid + baseOffset;
                }
                twoOffsetsValueReference.secondOffset = CTool_EndianConvertWord16(firstOffset);
                twoOffsetsValueReference.argument = CTool_EndianConvertWord32(record->data.member_cond.offset);
                twoOffsetsValueReference.reference = 0;
                AppendGListData(&exception_records, &twoOffsetsValueReference, sizeof(twoOffsetsValueReference));
                append_reference(record->data.member_cond.dtor, recordOffset + 0xc);
                break;
            case 9:
                if (Registers_GetInfo(record->data.member_array.objectptr) != NULL)
                    offset = Registers_GetInfo(record->data.member_array.objectptr)->reg;
                else
                    offset = 0;
                offsetValuesReference.kindFlags = flags | 9;
                offsetValuesReference.offsetFlags = (offset ? 1 : 0) << 7;
                if (offset == 0) {
                    if (Registers_GetInfo(record->data.member_array.objectptr)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    offset = record->data.member_array.objectptr->u.var.uid + baseOffset;
                }
                offsetValuesReference.offset = CTool_EndianConvertWord16(offset);
                offsetValuesReference.firstValue = CTool_EndianConvertWord32(record->data.member_array.offset);
                offsetValuesReference.secondValue = CTool_EndianConvertWord32(record->data.member_array.count);
                offsetValuesReference.thirdValue = CTool_EndianConvertWord32(record->data.member_array.size);
                offsetValuesReference.reference = 0;
                AppendGListData(&exception_records, &offsetValuesReference, sizeof(offsetValuesReference));
                append_reference(record->data.member_array.dtor, recordOffset + 0x10);
                break;
            case 10:
            case 11:
                if (Registers_GetInfo(record->data.pair.first) != NULL)
                    offset = Registers_GetInfo(record->data.pair.first)->reg;
                else
                    offset = 0;
                kind10Reference.kindFlags = flags | 0xa;
                kind10Reference.offsetFlags = (offset ? 1 : 0) << 7;
                if (offset == 0) {
                    if (Registers_GetInfo(record->data.pair.first)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    offset = record->data.pair.first->u.var.uid + baseOffset;
                }
                kind10Reference.offset = CTool_EndianConvertWord16(offset);
                kind10Reference.reference = 0;
                AppendGListData(&exception_records, &kind10Reference, sizeof(kind10Reference));
                append_reference(record->data.pair.second, recordOffset + 4);
                break;
            case 12:
                if (Registers_GetInfo(record->data.delete_pointer_cond.cond) != NULL)
                    thirdOffset = Registers_GetInfo(record->data.delete_pointer_cond.cond)->reg;
                else
                    thirdOffset = 0;
                if (Registers_GetInfo(record->data.delete_pointer_cond.pointer) != NULL)
                    firstOffset = Registers_GetInfo(record->data.delete_pointer_cond.pointer)->reg;
                else
                    firstOffset = 0;
                kind12Reference.kindFlags = flags | 0xb;
                kind12Reference.offsetFlags = ((thirdOffset ? 1 : 0) << 7) | (((firstOffset ? 1 : 0) & 1) << 6);
                if (thirdOffset == 0) {
                    if (Registers_GetInfo(record->data.delete_pointer_cond.cond)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    thirdOffset = record->data.delete_pointer_cond.cond->u.var.uid + baseOffset;
                }
                kind12Reference.firstOffset = CTool_EndianConvertWord16(thirdOffset);
                if (firstOffset == 0) {
                    if (Registers_GetInfo(record->data.delete_pointer_cond.pointer)->in_param_area)
                        baseOffset = fn_004a9f70(0);
                    else
                        baseOffset = fn_004a9f90();
                    firstOffset = record->data.delete_pointer_cond.pointer->u.var.uid + baseOffset;
                }
                kind12Reference.secondOffset = CTool_EndianConvertWord16(firstOffset);
                kind12Reference.reference = 0;
                AppendGListData(&exception_records, &kind12Reference, sizeof(kind12Reference));
                append_reference(record->data.delete_pointer_cond.deletefunc, recordOffset + 8);
                break;
            case 13:
                typeReference.offsetFlags = 0;
                typeReference.kindFlags = flags | 0x10;
                typeReference.reference = 0;
                if (ExceptionHasTypeValue(record->data.catch_block.label))
                    typeReference.typeValue =
                        CTool_EndianConvertWord32(ExceptionTypeValue(record->data.catch_block.label));
                else
                    typeReference.typeValue = 0;
                if (Registers_GetInfo(record->data.catch_block.info)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                typeReference.offset = CTool_EndianConvertWord32(record->data.catch_block.info->u.var.uid + baseOffset);
                AppendGListData(&exception_records, &typeReference, sizeof(typeReference));
                if (record->data.catch_block.typeInfo != NULL)
                    append_reference(record->data.catch_block.typeInfo, recordOffset + 4);
                break;
            case 14:
                shortRecord.offsetFlags = 0;
                shortRecord.kindFlags = flags | 0xd;
                if (Registers_GetInfo(record->data.active_catch.info)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                shortRecord.value = CTool_EndianConvertWord16(record->data.active_catch.info->u.var.uid + baseOffset);
                AppendGListData(&exception_records, &shortRecord, sizeof(shortRecord));
                break;
            case 15:
                referenceTable.offsetFlags = 0;
                referenceTable.kindFlags = flags | 0xf;
                referenceTable.count = CTool_EndianConvertWord16(record->data.specification.count);
                if (ExceptionHasTypeValue(record->data.specification.label))
                    referenceTable.typeValue =
                        CTool_EndianConvertWord32(ExceptionTypeValue(record->data.specification.label));
                if (Registers_GetInfo(record->data.specification.info)->in_param_area)
                    baseOffset = fn_004a9f70(0);
                else
                    baseOffset = fn_004a9f90();
                referenceTable.offset =
                    CTool_EndianConvertWord32(record->data.specification.info->u.var.uid + baseOffset);
                AppendGListData(&exception_records, &referenceTable, 12);
                referenceIndex = 0;
                referenceOffset = 12;
                for (; referenceIndex < record->data.specification.count; referenceIndex++) {
                    append_reference(record->data.specification.ids[referenceIndex], recordOffset + referenceOffset);
                    AppendGListLong(&exception_records, 0);
                    referenceOffset += 4;
                }
                break;
            case 16:
                byteRecord.offsetFlags = 0;
                byteRecord.kindFlags = flags | 0xe;
                AppendGListData(&exception_records, &byteRecord, sizeof(byteRecord));
                break;
            default:
                CError_FATAL(467);
                break;
        }
        node = node->parent;
    }
    if (node != NULL) {
        linkRecord.kindFlags = 1;
        linkRecord.offsetFlags = 0;
        linkRecord.value = CTool_EndianConvertWord16(node->recordOffset);
        AppendGListData(&exception_records, &linkRecord, sizeof(linkRecord));
    }
}

struct ObjectGroup *find_or_create_object_group(ExceptionAction *object)
{
    struct ObjectGroup *node;
    struct ObjectGroup *found;
    struct ObjectGroup *parent;

    node = object_groups[object->kind];
    if (node != NULL) {
        do {
            if (node->object == object)
                return node;
            node = node->next;
        } while (node != NULL);
    }

    if (object->next != NULL)
        parent = find_or_create_object_group(object->next);
    else
        parent = NULL;

    found = object_groups[object->kind];
    if (found != NULL) {
        do {
            if (found->parent == parent) {
                if (CExcept_ActionCompare(found->object, object))
                    return found;
            }
            found = found->next;
        } while (found != NULL);
    }

    node = (struct ObjectGroup *)lalloc(sizeof(struct ObjectGroup));
    node->parent = parent;
    node->object = object;
    node->child_count = 0;
    node->recordOffset = 0;
    node->next = object_groups[object->kind];
    object_groups[object->kind] = node;
    if (parent != NULL)
        parent->child_count++;
    return node;
}
