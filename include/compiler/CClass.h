#ifndef COMPILER_CCLASS_H
#define COMPILER_CCLASS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct CClassNode {
    struct CClassNode *next;
    struct Type *type;
};
#pragma pack(pop)
#pragma options align = mac68k
struct ClassFriend {
    struct ClassFriend *next;
    union {
        struct TypeClass *theclass;
        struct Object *obj;
    } u;
    UInt8 isclass;
};
#pragma options align = reset
#pragma pack(push, 1)
struct OverrideClass {
    TypeClass *theclass;
    struct OverrideFunc *members;
    struct OverrideClassBase *children;
    SInt32 offset;
    SInt32 voffset;
    Boolean done;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct OverrideClassBase {
    struct OverrideClassBase *next;
    struct OverrideClass *layout;
    Boolean is_virtual;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct OverrideFunc {
    struct OverrideFunc *next;
    struct Object *object;
    struct OverrideClass *selectedClass;
    struct OverrideFunc *selected;
    struct OverrideFunc *conflict;
};
#pragma pack(pop)
struct PendingThunk {
    struct PendingThunk *next;
    Object *thunkObject;
    Object *functionObject;
    SInt32 b;
    SInt32 c;
    SInt32 d;
};
#pragma pack(push, 1)
struct VirtualFunctionEntry {
    struct VirtualFunctionEntry *next;
    Object *func;
    SInt32 slot;
    SInt32 x;
};
#pragma pack(pop)
extern ENode *CClass_AccessMember(ENode *node, Type *type, UInt32 quals, int value);
extern void narrow_bitfield_type(Type **type, int *displacement);
extern Type *copy_pointer_array_type(Type *type);
extern void CClass_CheckStaticAccess(BClassList *type, TypeClass *owner, UInt8 access);
extern BClassList *deduplicate_and_select_base_path_suffix(BClassList *p, TypeClass *base);
extern Boolean check_base_path_access(BClassList *cl, UInt8 acc);
extern void CClass_MemberDef(Object *object, TypeClass *cls);
extern void CClass_MakeStaticActionClass(TypeClass *theclass);
extern void CClass_ClassAction(TypeClass *cls);
extern void CClass_ClassDefaultFuncAction(TypeClass *tclass);
extern void CClass_CheckOverrides(TypeClass *cls);
extern void check_hidden_inherited_virtual_functions(OverrideClass *layout, OverrideClass *base);
extern Object *CClass_004ea020(OverrideClass *record, char report);
extern void build_virtual_function_entries(OverrideClass *ctx);
extern void CClass_DefineCovariantFuncs(Object *func, CInlineInfo *arg2);
extern ObjectList *prepend_base_method_copies(ObjectList *objects, Object *method, TypeClass *theclass);
extern CClassNode *collect_override_return_class_types(CClassNode *types, TypeClass *tclass, Object *method,
                                                       Boolean skipClass);
extern void select_member_override(OverrideClass *classRecord, OverrideClass *context, struct OverrideFunc *search);
extern Boolean contains_base_layout(OverrideClass *identity, OverrideClass *sub);
extern OverrideClass *create_class_layout(OverrideClass *root, TypeClass *cls, SInt32 offset, SInt32 voffset);
extern OverrideClass *find_class_layout_by_class_and_offset(OverrideClass *layout, TypeClass *cls, SInt32 offset);
extern unsigned char CClass_OverridesBaseMember(TypeClass *theclass, HashNameNode *name, Object *target);
extern Boolean CClass_ClassDominates(TypeClass *cls, TypeClass *base);
extern int CClass_GetPathOffset(BClassList *cl);
extern ENode *CClass_DirectBasePointerCast(ENode *expr, TypeClass *theclass, TypeClass *base);
extern ENode *CClass_ClassPointerCast(ENode *expr, TypeClass *sourceClass, TypeClass *targetClass,
                                      Boolean convertIndirect, Boolean errorflag);
extern ENode *CClass_AdjustBasePointer(ENode *expr, SInt16 count, Boolean reverse);
extern TypeClass *CClass_GetQualifiedClass(void);
extern BClassList *CClass_GetBasePath(TypeClass *type, TypeClass *target, SInt16 *flags, Boolean *status);
extern BClassList *find_target_base_path(TypeClass *cls, TypeClass *target, SInt32 offset, SInt16 level);
extern SInt16 CClass_GetBasePathLevel(void);
extern void fn_004eb810(void);
extern void CClass_CheckBaseAccess(BClassList *bases, char access);
extern Object *create_root_class_layout(TypeClass *type);
extern void build_class_layout(TypeClass *type);
extern Object *CClass_CheckPures(TypeClass *type);
extern UInt8 CClass_FindBasePath(TypeClass *sourceClass, TypeClass *targetClass, char option1, char option2);
extern void CClass_CheckEnumAccess(BClassList *bases, ObjBase *object);
extern void CClass_CheckObjectAccess(BClassList *bases, Object *reference);
extern ENode *CClass_CreateThisSelfExpr(void);
extern Object *CClass_ThisSelfObject(void);
extern unsigned int CClass_VirtualBaseVTableOffset(TypeClass *type, TypeClass *base);
extern SInt32 CClass_VirtualBaseOffset(TypeClass *cls, TypeClass *base);
extern unsigned char CClass_IsMoreAccessiblePath(BClassList *path, BClassList *otherPath);
extern UInt8 get_path_access(BClassList *path);
extern Boolean find_virtual_base_path(register TypeClass *cls, register TypeClass *target);
extern Boolean find_base_path(TypeClass *cls, TypeClass *base, SInt32 offset, Boolean f1, Boolean f2);
extern BClassList *CClass_AppendPath(BClassList *list, BClassList *tail);
extern BClassList *CClass_GetPathCopy(BClassList *list, Boolean global);
extern void fn_004ebae0(TypeClass *type);
extern Boolean CClass_ReferenceArgument(TypeClass *cls);
extern Boolean CClass_HasTypeFuncFlag16384(Object *object);
extern Boolean CClass_IsDestructor(Object *obj);
extern Object *CClass_Destructor(TypeClass *cls);
extern NameSpaceObjectList *CClass_Constructor(TypeClass *type);
extern NameSpaceObjectList *CClass_MemberObject(TypeClass *type, HashNameNode *name);
extern void select_layout_member_overrides(OverrideClass *node);
extern Object *CClass_CopyConstructor(TypeClass *cls);
extern Object *CClass_AssignmentOperator(TypeClass *theclass);
extern Object *CClass_DefaultConstructor(TypeClass *cls);
extern Boolean CClass_IsEmpty(register TypeClass *type);
extern UInt8 CClass_GetOverrideKind(TypeFunc *a, TypeFunc *b, Boolean errorflag);
extern void mark_vbases_has_override(OverrideClass *root, OverrideClass *node, unsigned char mark);
extern struct ClassList *base_path[];
extern SInt16 base_path_depth;
extern SInt16 base_path_level;
extern Object *get_or_create_thunk_object(Object *source, SInt32 firstArgument, SInt32 secondArgument,
                                          SInt32 thirdArgument);
extern void CClass_GenThunks(void);
extern void CClass_Init(void);
struct BClassList;

#ifdef __cplusplus
}
#endif

#endif
