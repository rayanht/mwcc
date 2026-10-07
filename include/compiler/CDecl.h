#ifndef COMPILER_CDECL_H
#define COMPILER_CDECL_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/types.h"
#include "compiler/CInline.h"
#include "compiler/CPrep.h"
#include "compiler/CParser.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ClassLayout {
    ObjBase **objlist;
    ObjMemberVar *vtable_ivar;
    UInt16 lex_order_count;
    UInt16 firstVirtualSlot;
    Boolean has_vtable;
};
/* A declaration's storage class (DeclInfo.storage): the keyword's token. An enumeration: CFunc compares the storage with these
 * enumerators, which compiles differently from a comparison with the numbers. */
enum { STORAGE_AUTO = TK_AUTO, STORAGE_REGISTER = TK_REGISTER, STORAGE_STATIC = TK_STATIC, STORAGE_EXTERN = TK_EXTERN };

#pragma options align = mac68k
struct DeclInfo {
    Type *thetype;
    UInt32 qual;
    struct NameSpace *nspace;
    HashNameNode *name;
    struct ObjBase *resolvedObject;
    struct NameSpaceObjectList *resolvedObjects;
    struct FuncArg *parameterNames;
    struct NameSpace *parameterScope;
    Type *templateType;
    ENode *arrayBound;
    struct TypeClass *pendingClass;
    struct TemplArg *expltargs;
    struct TemplParam *templateParameters;
    struct TemplateScopeState *templateScope;
    SInt16 operator_token;
    SInt16 storageclass;
    SInt16 section;
    UInt8 exportflags;
    UInt8 hasParameterNames;
    UInt8 oldStyleParameters;
    UInt8 isNewExpression;
    UInt8 hasArrayDimension;
    char missingTypeSpecifier;
    UInt8 isType;
    Boolean parserOption;
    Boolean isConstructor;
    Boolean allowForeignNamespace;
    Boolean in_friend_decl;
    UInt8 requireMangledName;
    UInt8 isNewTypeId;
    UInt8 requireTemplateClassMember;
    UInt8 isStructMemberDeclarator;
    Boolean allowTemplateArguments;
    Boolean has_expltargs;
    UInt8 hasTypename;
    struct CPrepFileInfo *file;
    struct CPrepFileInfo *file2;
    SInt32 sourceoffset;
};
#pragma options align = reset
struct DefArgCtorInfo {
    struct Object *default_func;
    struct ENode *default_arg;
};
union FunctionTypeBuffer {
    TypeMemberFunc member_function;
    TypeFunc function;
};
#pragma pack(push, 1)
struct BigDeclInfo {
    DeclInfo declinfo;
    DeclInfo declinfo2;
    UInt8 unused;
    Boolean valid;
};
#pragma pack(pop)
extern TypeIntegral stunsignedint;
struct ClassLayout;
extern UInt8 CDecl_ParseDeclarationAttributeFlags(void);
extern TypeClass *CDecl_DefineClass(struct NameSpace *nspace, struct HashNameNode *name, struct TypeClass *type,
                                    short mode, char flag4, char flag5);
extern void CDecl_CompleteClass(ClassLayout *ctx, TypeClass *cls);
extern void fill_class_layout_entries(ClassLayout *table, TypeClass *type, ObjBase **entries);
extern void declare_auto_generated_destructor(ClassLayout *type, TypeClass *cls);
extern void generate_copy_constructor(ClassLayout *type, TypeClass *cls);
extern void declare_default_copy_constructor(ClassLayout *decl, TypeClass *type);
extern TypeMemberFunc *CDecl_MakeDefaultDtorType(TypeClass *theclass, char is_const);
extern void make_auto_generated_dtor(ClassLayout *context, TypeClass *cls);
extern void make_defarg_function(TypeClass *cls);
extern void parse_class_bases(struct TemplClass *classType, short mode, char allowDependent);
extern Boolean CDecl_CheckNewBase(TypeClass *cls, TypeClass *base, Boolean flag);
extern void CDecl_SetVBaseOffsets(TypeClass *cls);
extern VClassList *append_unique_vbase(TypeClass *cls, TypeClass *base);
extern ObjMemberVar *add_member_var(ClassLayout *declaration, TypeClass *cls, Type *type, UInt32 qual,
                                    HashNameNode *name, AccessType access);
extern void parse_friend_declaration(struct TemplClass *cls);
extern void CDecl_AddFriend(TypeClass *typeClass, Object *object, TypeClass *type);
extern void CDecl_UnpackDeclInfo(DeclInfo *dst, PackedDeclInfo *src);
extern unsigned char CDecl_PackDeclInfo(PackedDeclInfo *destination, DeclInfo *source);
extern Boolean check_qualified_identifier_or_operator(TypeClass *tclass, AccessType access);
extern TypeMemberFunc *CDecl_NewTypeMemberFunc(TypeFunc *type, TypeClass *theclass, Boolean is_static, Boolean arg);
extern void scan_inline_definition(Object *object, TypeClass *classType);
extern int parse_struct_members(TypeStruct *obj, Boolean block);
extern void scanenum(DeclInfo *result);
extern void *parse_enum_body(TypeEnum *enumList, HashNameNode *nameId);
extern Boolean CDecl_FunctionDeclarator(DeclInfo *decl, NameSpace *mode, Boolean allow_definition, int arg4);
extern void compute_struct_layout(Type *str);
extern TypeEnum *parse_enum_definition(TypeEnum *decl, HashNameNode *name);
extern void scanstruct(DeclInfo *state, SInt16 spec);
extern void declare_member_function(ClassLayout *gen, TypeClass *cls, struct DeclInfo *info, UInt8 access,
                                    UInt8 allowPure, UInt8 specialMember, UInt8 parseBody, UInt8 declarationOnly);
extern void declare_object(DeclInfo *d, UInt8 b, Boolean c);
extern void CDecl_ComputeUnderlyingEnumType(TypeEnum *res);
extern Object *CDecl_GetFunctionObject(DeclInfo *decl, NameSpace *nspace, Boolean *is_new, char mode);
extern void CDecl_TypedefDeclarator(DeclInfo *decl);
extern void CDecl_ScanDeclarator(DeclInfo *p);
extern void parse_resolved_member_function_decl(DeclInfo *di, Boolean define);
extern void CDecl_ScanStructDeclarator(BigDeclInfo *p);
extern struct HashNameNode *unnamed_name;
extern struct NameSpace *cscope_current;
extern struct FileOffsetInfo member_foi;
extern TypeIntegral stunsignedshort;
extern TypeIntegral stunsignedchar;
extern void CDecl_ParseClass(DeclInfo *ctx, SInt16 kind, Boolean advanceToken, UInt8 extraFlags);
extern void CheckDefaultArgs(FuncArg *args);
extern void MergeDefaultArgs(FuncArg *args, FuncArg *otherArgs);
extern Object *find_or_create_function_object(ObjectList *list, DeclInfo *ref, Boolean *found, UInt8 mode,
                                              Boolean conv);
extern void parse_class_members(ClassLayout *decle, TypeClass *tclass, SInt16 mode);
extern struct HashNameNode *destructor_name;
extern UInt8 member_access;
extern void conversion_type_name(DeclInfo *result);
extern void CDecl_ParseDeclarator(DeclInfo *p);
extern void CDecl_MakeMemberPointerType(Type **result, TypeClass *owner, unsigned int value);
extern void parse_direct_declarator(DeclInfo *state, NameSpace *function);
extern Boolean check_operator_declaration(DeclInfo *declaration, Boolean isMember);
extern unsigned int parse_parenthesized_declarator(DeclInfo *args);
extern void replace_nested_type(Type *oldtype, Type *ty, Type *newtype);
extern void CDecl_ParseDirectFuncDecl(DeclInfo *d);
extern void fn_00504240(TypeFunc *type);
extern Boolean check_function_return_type(Type *type);
extern Boolean CDecl_CheckArrayIntegr(Type *type);
extern void CDecl_PrependFuncArg(TypeFunc *type, TypeIntegral *argtype);
extern void CDecl_MakePTMFuncType(TypeFunc *func);
extern void prepend_class_pointer_argument(TypeFunc *owner, TypeClass *classType, char parseModifiers);
extern void CDecl_WrapTypePointer(Type **type, unsigned int flags);
extern Boolean check_object_creation_type(Type *type);
extern unsigned char CDecl_CheckObjectType(Type *type);
extern Boolean CanCreateObject(Type *tc);
extern Boolean CanAllocObject(Type *theclass);
extern int CDecl_CompleteType(Type *type);
extern void CDecl_NewConvFuncType(DeclInfo *state);
extern void CDecl_ScanPointer(DeclInfo *declarator, NameSpace *nspace, char finish);
extern void replace_type_placeholder(Type *type, Type *ctype);
extern Type type_placeholder;
extern struct HashNameNode *this_arg_name;
extern void scandeclarator(DeclInfo *decl);
extern TypeTemplDep *CDecl_NewTemplDepType(UInt8 templateKind);
extern Type *CDecl_NewPointerType(Type *targetType);
extern Type *CDecl_NewArrayType(Type *elementType, SInt32 size);
extern Type *CDecl_NewStructType(SInt32 size, SInt16 align);
extern ENode *CDecl_ParseSelectorExpression(void);
extern ENode *CDecl_ParseProtocolExpression(void);
extern TypeIntegral stsignedlonglong;
extern TypeIntegral stsignedshort;
extern TypeIntegral stsignedchar;
extern struct HashNameNode *constructor_name;

#ifdef __cplusplus
}
#endif

#endif
