#ifndef COMPILER_CSOM_H
#define COMPILER_CSOM_H

#include "compiler/common.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/CTemplateClass.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SOMInfo {
    struct TypeClass *baseClass;
    struct Object *classDataObject;
    struct SOMInfoEntry *methodNameList;
    SInt32 descriptorValue0;
    SInt32 descriptorValue1;
    UInt8 omitEnvironmentParameter;
    char alignmentPadding[3];
    struct Object *specialfunc;
    char unusedTail[40];
    UInt8 has_assign;
    UInt8 has_other;
};
struct CSOMRefNode {
    struct CSOMRefNode *next;
    struct Object *object;
    struct TypeClass *theclass;
    SInt32 id;
    UInt8 kind;
};
#pragma pack(push, 1)
struct SOMClassBuildState {
    struct SOMEntry *members;
    struct SOMVTable *bases;
    struct Object *object;
    struct Object *ancestorObject;
    struct Object *overrideMethodsObject;
    struct Object *registrationFunction;
    struct Object *specialFunctionsObject;
    UInt32 descriptorValues[2];
    UInt32 descriptorFlags;
    SInt16 alignmentKind;
    UInt16 memberCount;
    UInt16 directBaseCount;
    UInt16 implicitBaseCount;
    UInt16 overrideBaseCount;
    UInt16 inheritedMemberCount;
    SInt16 descriptorAttribute5;
    char pad36[0x0a];
    SInt32 overrideMethodCount;
    UInt8 hasNewOperator;
    UInt8 hasDeleteOperator;
};
#pragma pack(pop)
/* SOM class descriptor image; pointer slots are supplied by relocations. */
struct SOMClassDescriptor {
    UInt32 value0;
    struct SOMClassData *classData;
    void *overrideMethods;
    struct SOMClassData **ancestors;
    void (*registrationFunction)(void);
    void *specialFunctions;
    UInt32 values24[7];
    struct SOMDescriptorOutput *descriptorText;
    char *className;
    UInt32 classSize;
    SInt32 *baseValues;
    UInt8 *memberKinds;
    UInt8 *memberOverrides;
    char *memberNames;
    UInt16 *overrideMethodIndices;
    UInt16 *words;
    UInt32 value88;
    UInt32 values92[5];
};
struct SOMDescriptorOutput {
    UInt32 descriptorValues[2];
    UInt32 descriptorFlags;
    SInt16 alignmentKind;
    SInt16 memberCount;
    SInt16 directBaseCount;
    SInt16 implicitBaseCount;
    SInt16 overrideBaseCount;
    SInt16 inheritedMemberCount;
    SInt16 descriptorAttribute5;
    UInt8 reserved[0x0a];
};
struct SOMEntry {
    SOMEntry *next;
    HashNameNode *key;
    union {
        Object *object;
        struct {
            UInt16 offset;
            UInt16 index;
        } vt;
    } u;
    UInt8 kind;
    UInt8 flag;
};
#pragma pack(push, 2)
struct SOMInfoEntry {
    SOMInfoEntry *next;
    HashNameNode *name;
    UInt8 kind;
    UInt8 serializedPadding;
    UInt8 alignmentPadding[2];
};
#pragma pack(pop)
#pragma pack(push, 1)
struct SOMPragmaNames {
    SOMInfoEntry *head;
};
#pragma pack(pop)
struct SOMSlot {
    struct SOMSlot *next;
    Object *overrideMethod;
    Object *baseMethod;
};
#pragma options align = mac68k
struct SOMVTable {
    struct SOMVTable *next;
    struct TypeClass *base;
    struct SOMSlot *slots;
    UInt8 isImplicitBase;
    UInt8 isDirectBase;
};
#pragma options align = reset
extern ENode *create_glue_objectrefnode(TypeClass *cls, SInt32 id, Object *obj);
extern Boolean fn_004e3cd0(Type *ftype);
extern ENode *CSOM_AppendPointerArgCall(ENode *node, ENodeList *spec);
extern void CSOM_GenerateSomselfAssignment(TypeClass *tclass, Statement *stmt);
extern ENode *CSOM_GetOrCreateLocalObjectNode(TypeClass *value);
extern void find_method_vtbl_class_and_offset(TypeClass *cls, Object *method, TypeClass **outcls, SInt32 *outofs);
extern void fn_004e4390(Object *obj);
extern ENode *CSOM_BuildNewObjectInstance(TypeClass *cls);
extern Object *fn_004e45b0(char *name, char *signature);
extern void set_owner_target_flag(void);
extern void CSOM_ParseBaseClass(void);
extern void CSOM_ParseMethodNameList(void);
extern void CSOM_BuildClass(TypeClass *func);
extern void create_special_functions_object(SOMClassBuildState *info, TypeClass *cls);
extern void build_class_vtables_and_members(SOMClassBuildState *layout, TypeClass *cls);
extern void CSOM_CompleteClass(TypeClass *tclass);
extern Object **build_vtbl_index_table(TypeClass *theclass, SInt32 *count);
extern void CSOM_EncodeMemberFunctionTypes(TypeMemberFunc *function);
extern ENode *CSOM_CallReleaseObjectReference(TypeClass *unused, ENode *argument);
extern void CSOM_PrependTheClassArg(TypeFunc *function);
extern void CSOM_InitSOMInfo(TypeClass *type);
extern void CSOM_ParseDescriptorValues(void);
extern void encode_member_function_types(TypeMemberFunc *t, Boolean flag);
extern UInt8 encode_som_type(UInt8 *p, Type *ty, Boolean flag);
extern void CSOM_GenerateRefNodeCode(void);
extern void CSOM_Init(char flag);
extern void make_class_descriptor(SOMClassBuildState *record, TypeClass *classType);
extern void create_ancestor_object(SOMClassBuildState *info, TypeClass *cls);
extern void create_override_methods_object(SOMClassBuildState *cls, TypeClass *func);
extern void build_descriptor_output(SOMClassBuildState *desc, TypeClass *cls, struct SOMDescriptorOutput *out);
extern void initialize_class_data_object(SOMClassBuildState *methods, TypeClass *tclass);
extern struct SOMVTable *find_or_add_base(SOMClassBuildState *list, TypeClass *unused, TypeClass *id, UInt16 *index);
extern Object *build_base_method_vtbl_index_object(SOMClassBuildState *groups);
extern ENode *CSOM_MakeMethodReference(BClassList *node, Object *obj, Boolean flag);
extern ENode *CSOM_CreateMemberAccessExpr(BClassList *classList, ObjMemberVar *request, ENode *expr);
extern void emit_som_kind_nibbles(struct SOMClassBuildState *info);
extern struct CSOMRefNode *somReferences;
extern void fn_004e67a0(void);
extern void CSOM_NoOp(void);

#ifdef __cplusplus
}
#endif

#endif
