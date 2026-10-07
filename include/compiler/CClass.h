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
struct CFriend {
    struct CFriend *next; /* 0x00: CDecl_AddFriend links class friends */
    union {
        struct Type *type;     /* 0x04: CDecl_AddFriend sets when is_class != 0; serialize_entry_list writes type */
        struct Object *object; /* 0x04: CDecl_AddFriend sets when is_class == 0; serialize_entry_list writes object */
    } target;
    UInt8 is_class; /* 0x08: CDecl_AddFriend selects type or object; check_base_path_access tests it */
};
#pragma options align = reset
#pragma pack(push, 1)
struct ClassLayout {
    TypeClass *theclass;
    struct ClassLayoutMember *members;
    struct ClassLayoutBase *children;
    SInt32 offset;
    SInt32 voffset;
    Boolean done;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ClassLayoutBase {
    struct ClassLayoutBase *
        next; /* 0x00: create_class_layout links layout->children; find_class_layout_by_class_and_offset traverses it */
    struct ClassLayout
        *layout; /* 0x04: create_class_layout builds the base layout; select_member_override searches it */
    Boolean
        is_virtual; /* 0x08: create_class_layout sets from base->is_virtual; contains_base_layout selects the virtual base */
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ClassLayoutMember {
    struct ClassLayoutMember *next;     /* 0x00: create_class_layout links layout->members */
    struct Object *object;              /* 0x04: create_class_layout stores the DVFUNC object */
    struct ClassLayout *selectedClass;  /* 0x08: select_member_override selects the overriding class */
    struct ClassLayoutMember *selected; /* 0x0c: select_member_override selects the overriding function */
    struct ClassLayoutMember *conflict; /* 0x10: select_member_override records an ambiguous override */
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
extern void CClass_GenerateVTable(TypeClass *cls);
extern void fn_004e9ca0(void);
extern void CClass_CheckOverrides(TypeClass *cls);
extern void check_hidden_inherited_virtual_functions(ClassLayout *layout, ClassLayout *base);
extern Object *CClass_004ea020(ClassLayout *record, char report);
extern void build_virtual_function_entries(ClassLayout *ctx);
extern void CClass_DefineCovariantFuncs(Object *func, CInlineInfo *arg2);
extern ObjectList *prepend_base_method_copies(ObjectList *objects, Object *method, TypeClass *theclass);
extern CClassNode *collect_override_return_class_types(CClassNode *types, TypeClass *tclass, Object *method,
                                                       Boolean skipClass);
extern void select_member_override(ClassLayout *classRecord, ClassLayout *context, struct ClassLayoutMember *search);
extern Boolean contains_base_layout(ClassLayout *identity, ClassLayout *sub);
extern ClassLayout *create_class_layout(ClassLayout *root, TypeClass *cls, SInt32 offset, SInt32 voffset);
extern ClassLayout *find_class_layout_by_class_and_offset(ClassLayout *layout, TypeClass *cls, SInt32 offset);
extern unsigned char CClass_OverridesBaseMember(TypeClass *theclass, HashNameNode *name, Object *target);
extern Boolean CClass_ClassDominates(TypeClass *cls, TypeClass *base);
extern int CClass_GetPathOffset(BClassList *cl);
extern ENode *CClass_DirectBasePointerCast(ENode *expr, TypeClass *theclass, TypeClass *base);
extern ENode *CClass_ConvertClassPointer(ENode *expr, TypeClass *sourceClass, TypeClass *targetClass,
                                         Boolean convertIndirect, Boolean errorflag);
extern ENode *CClass_AdjustBasePointer(ENode *expr, SInt16 count, Boolean reverse);
extern TypeClass *CClass_GetQualifiedClass(void);
extern BClassList *CClass_GetBasePath(TypeClass *type, TypeClass *target, SInt16 *flags, Boolean *status);
extern BClassList *find_target_base_path(TypeClass *cls, TypeClass *target, SInt32 offset, SInt16 level);
extern SInt16 CClass_GetBasePathLevel(void);
extern void CClass_Init(void);
extern void CClass_CheckBaseAccess(BClassList *bases, char access);
extern Object *create_root_class_layout(TypeClass *type);
extern void build_class_layout(TypeClass *type);
extern Object *CClass_CheckPures(TypeClass *a0);
extern UInt8 CClass_FindBasePath(TypeClass *sourceClass, TypeClass *targetClass, char option1, char option2);
extern void CClass_CheckEnumAccess(BClassList *bases, ObjBase *object);
extern void CClass_CheckObjectAccess(BClassList *bases, Object *reference);
extern ENode *CClass_CreateThisSelfExpr(void);
extern Object *CClass_ThisSelfObject(void);
extern unsigned int CClass_VirtualBaseVTableOffset(TypeClass *type, TypeClass *base);
extern SInt32 CClass_FindVBaseOffset(TypeClass *cls, TypeClass *base);
extern unsigned char CClass_IsMoreAccessiblePath(BClassList *a0, BClassList *a1);
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
extern void select_layout_member_overrides(ClassLayout *node);
extern Object *CClass_CopyConstructor(TypeClass *cls);
extern Object *CClass_AssignmentOperator(TypeClass *theclass);
extern Object *CClass_DefaultConstructor(TypeClass *cls);
extern Boolean CClass_IsEmpty(register TypeClass *type);
extern UInt8 CClass_GetOverrideKind(TypeFunc *a, TypeFunc *b, Boolean errorflag);
extern void mark_vbases_has_override(ClassLayout *root, ClassLayout *node, unsigned char mark);
extern struct ClassList *base_path[];
extern SInt16 base_path_depth;
extern SInt16 base_path_level;
extern Object *get_or_create_thunk_object(Object *source, SInt32 firstArgument, SInt32 secondArgument,
                                          SInt32 thirdArgument);
extern void CClass_GenThunks(void);
extern void CClass_ResetPendingThunks(void);
struct BClassList;

#ifdef __cplusplus
}
#endif

#endif
