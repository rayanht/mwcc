#ifndef COMPILER_TOC_H
#define COMPILER_TOC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MemberPointerConstant {
    struct MemberPointerConstant *next;
    struct Object *object;
    int offset;
    MWVector128 *value;
};
#pragma options align = mac68k

#pragma options align = reset
struct TOCEntry {
    SInt32 a;
    SInt32 b;
    SInt32 c;
    SInt32 d;
};
#pragma pack(push, 1)
struct TOCNameEntry {
    struct TOCNameEntry *next;
    struct Object *object;
    struct CLabel *label;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct TOCReferenceEntry {
    struct TOCReferenceEntry *next;
    struct Object *object;
    struct Object *lookupObject;
    struct ENode *expression;
    char makeIndirect;
};
#pragma pack(pop)
extern void TOC_0049d710(ENode *node, Type *targetType, int ignored);
extern unsigned char is_small_splat_or_table_vector(long value, Type *type);
extern void TOC_EmitMemberPointerConstants(void);
extern Object *TOC_GetFloatObject(Type *type, Float *value);
extern Object *fn_0049f230(Object *object, SInt32 a, SInt32 b);
extern void fn_0049ebb0(ENode *expr);
extern void add_initial_object(void *object);
extern Object *get_or_create_label_object(CLabel *node);
extern void rewrite_indirect_toc_references(void);
extern void expandpreincdec(ENode *node);
extern Type *select_common_arithmetic_type(Type *t1, Type *t2);
extern ENode *create_diadic_node_with_constant(ENode *e);
extern void TOC_EnumerateObjectCodeOffsets(void *arg);
extern void rewrite_compound_assignment(ENode *expr, unsigned char opcode);
extern void replace_vector_constant_with_objectref(ENode *node);
extern Object *TOC_CreateSinitObject(void);
extern void fn_0049d420(Statement *statements);
extern void add_exception_initial_objects(ExceptionAction *node);
extern UInt8 TOC_HasObjectReferenceWithoutExpression(Object *key);
extern void add_toc_reference(Object *id, Object *a, ENode *b, char c);
extern void fn_0049f4b0(Object *object);
extern char vector128_patterns[256];
extern MWVector128 alternate_vector_patterns[16];
extern struct ObjectList *float_object_list;
extern struct MemberPointerConstant *member_pointer_constants;
extern struct TOCNameEntry *toc_name_entries;
extern Boolean data_00588500;
extern struct ObjectList *gInitialObjectList_005882ac;
extern void make_objectref_offset(Object *object, Object *lookupObject, ENode *expression, Boolean makeIndirect);

#ifdef __cplusplus
}
#endif

#endif
