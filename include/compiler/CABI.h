#ifndef COMPILER_CABI_H
#define COMPILER_CABI_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* What a constructor's initializer list initializes (KIND): a base, a virtual base or a member (U), with EXPR. */
enum { INIT_BASE, INIT_VBASE, INIT_MEMBER };
struct BaseOffsetPath {
    struct BaseOffsetPath *next;
    SInt32 offset;
};
#pragma options align = mac68k
struct OffsetEntry {
    struct OffsetEntry *next;
    struct Type *type;
    SInt32 start;
    SInt32 end;
    Boolean flag;
};
#pragma options align = reset
struct VToff {
    struct VToff *next;
    SInt32 off;
};
struct VtOffEntry {
    struct VtOffEntry *next;
    SInt32 value;
};
extern ENode *CABI_MakeThisExpr(TypeClass *typeClass, int count);
extern ENode *CABI_DestroyObject(Object *dtor, ENode *objexpr, UInt8 mode, Boolean flag1, Boolean flag2);
extern Object *CABI_GetDestructorObject(Object *obj, UInt8 mode);
extern void CABI_MakeDefaultDestructor(TypeClass *tclass, Object *func);
extern void CABI_TransDestructor(Object *destructor, Object *completeDestructor, Statement *stmt, TypeClass *tclass,
                                 int mode);
extern Statement *build_base_destruction_statements(Statement *node, VClassList *bl);
extern Statement *destroy_members(Statement *expr, ObjMemberVar *member, TypeClass *cls);
extern OffsetEntry *fn_0050bf30(OffsetEntry *list, Type *type, SInt32 offset, Boolean flag);
extern Object *CABI_ConstructorCallsNew(TypeClass *tclass);
extern Statement *assign_vbase_ctor_offsets(Statement *list, TypeClass *cls);
extern SInt32 CABI_GetCtorOffsetOffset(TypeClass *tclass, TypeClass *baseclass);
extern BaseOffsetPath *find_shortest_virtual_base_offset_path(TypeClass *tclass, TypeClass *base);
extern void CABI_MakeDefaultArgConstructor(TypeClass *theclass, Object *function);
extern int CABI_LayoutClass(struct ClassLayout *members, TypeClass *type);
extern SInt32 CABI_GetVTableOffset(TypeClass *tclass);
extern void CABI_AddVTable(TypeClass *tclass);
extern void layout_class_ivars(ClassLayout *member, TypeClass *type);
extern void CABI_ReverseBitField(TypeBitfield *tbitfield);
extern void CABI_GenClassFunction(TypeClass *tclass, Object *function);
extern Statement *destroy_nonvirtual_bases(Statement *acc, ClassList *list);
extern void CABI_MakeDefaultConstructor(TypeClass *cls, Object *func);
extern int get_vtable_size_without_vbases(TypeClass *cl);
extern void CABI_GenerateClassFunction(TypeClass *cl, Object *func);
extern ENode *build_vbase_ptr_initializers(ENode *expr, TypeClass *func, TypeClass *cls, TypeClass *vbase,
                                           SInt32 offset);
extern Statement *assign_vtable_pointers(Statement *result, Object *obj, TypeClass *cls, TypeClass *base, SInt32 offset,
                                         SInt32 voffset);
extern void layout_nonvirtual_bases(void *context, TypeClass *theclass);
extern void layout_vtable(ClassLayout *layout, TypeClass *classArg);
extern Object *CABI_FindZeroVirtualBaseMember(TypeClass *scope, Object *key);
extern Statement *make_baseclass_and_ivars_copy_statements(Statement *stmt, TypeClass *tclass, TypeClass *baseclass,
                                                           SInt32 offset, Boolean flag);
extern void CABI_InsertConstructorInitialization(Object *obj, Statement *stmt, TypeClass *tclass,
                                                 Statement *(*callback)(Statement *, TypeClass *, TypeClass *, SInt32,
                                                                        Boolean),
                                                 Boolean has_try);
extern MessageArgument *CABI_SplitNameIntoMessageArguments(HashNameNode *hname, char *flag);
extern struct HashNameNode *vtable_name;
extern TypeIntegral stunsignedlong;
extern SInt16 CABI_ComputeAlignmentPadding(Type *data, SInt32 mask);
extern Type *CABI_GetPtrDiffTType(void);
extern Type *CABI_GetSizeTType(void);
extern struct Object *newh_func;

#ifdef __cplusplus
}
#endif

#endif
