#ifndef COMPILER_EXCEPTIONS_H
#define COMPILER_EXCEPTIONS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ExceptionAlignmentFill {
    int zero;
};

#pragma pack(push, 1)
struct ExceptionByteRecord {
    UInt8 kindFlags, offsetFlags;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ExceptionOffsetReference {
    UInt8 kindFlags, offsetFlags;
    UInt16 offset;
    SInt32 reference;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ExceptionOffsetValueReference {
    UInt8 kindFlags, offsetFlags;
    UInt16 offset;
    SInt32 value, reference;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ExceptionOffsetValuesReference {
    UInt8 kindFlags, offsetFlags;
    UInt16 offset;
    SInt32 firstValue, secondValue, thirdValue, reference;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ExceptionReferenceTableRecord {
    UInt8 kindFlags, offsetFlags;
    UInt16 count;
    SInt32 typeValue, offset;
    UInt16 value;
};
#pragma pack(pop)
/* Exceptions_AppendScopeEntry allocates 0x18 bytes with lalloc(0x18). */
struct ExceptionScopeEntry {
    struct ExceptionScopeEntry *next;
    struct ExceptionScopeEntry *previous;
    struct PCodeInstruction *start;
    struct PCodeInstruction *end;
    struct ExceptionAction *elements;
    struct ObjectGroup *info;
};
#pragma pack(push, 1)
struct ExceptionShortRecord {
    UInt8 kindFlags, offsetFlags;
    UInt16 value;
};
#pragma pack(pop)
struct ExceptionTableEntry {
    unsigned int offset;
    short length;
    short value;
};
struct ExceptionTableHeader {
    short flags;
    short value;
};
#pragma pack(push, 1)
struct ExceptionTwoOffsetsReference {
    UInt8 kindFlags, offsetFlags;
    UInt16 firstOffset, secondOffset, value;
    SInt32 reference;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ExceptionTwoOffsetsValueReference {
    UInt8 kindFlags, offsetFlags;
    UInt16 firstOffset, secondOffset, value;
    SInt32 argument, reference;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ExceptionTypeReferenceRecord {
    UInt8 kindFlags, offsetFlags;
    UInt16 value;
    SInt32 reference, typeValue, offset;
};
#pragma pack(pop)
struct ObjGenRelocationRequest {
    struct ObjGenRelocationRequest *next;
    struct Object *object;
    SInt32 offset;
    UInt32 reserved0c[3];
};
struct ObjectGroup {
    struct ObjectGroup *next;
    struct ObjectGroup *parent;
    struct ExceptionAction *object;
    unsigned short child_count;
    unsigned short recordOffset;
};
extern void Exceptions_EmitExceptionTable(Object *object, int offset);
extern void Exceptions_AppendScopeEntry(PCodeInstruction *context, ExceptionAction *elements);
extern void Exceptions_Reset(void);
extern int compact_exception_scope_entries(void);
extern int Exceptions_CountBoundObjectFields(ExceptionAction *action);
extern void emit_exception_records(ObjectGroup *node);
extern void Exceptions_CollectRegisterOperands(ExceptionAction *node, PCodeOperand *out);
extern struct ObjectGroup *find_or_create_object_group(ExceptionAction *object);
extern void *object_groups[];

#ifdef __cplusplus
}
#endif

#endif
