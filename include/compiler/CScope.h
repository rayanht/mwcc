#ifndef COMPILER_CSCOPE_H
#define COMPILER_CSCOPE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The current scope, saved while another is entered (CScope_SetFunctionScope, CScope_SetClassDefScope, CScope_SetClassScope, CScope_SetNameSpaceScope) and put
   back by CScope_RestoreScope. */
#pragma options align = mac68k
struct CScopeSave {
    struct NameSpace *current;
    struct TypeClass *currentclass;
    struct Object *currentfunc;
    UInt8 is_member_func;
};
#pragma options align = reset
#pragma options align = mac68k
struct NameResult {
    struct NameSpace *nspace;
    struct HashNameNode *name;
    Type *type;
    UInt32 qual;
    struct ObjBase *object;
    struct NameSpaceObjectList *objects;
    struct BClassList *basePath;
    Boolean is_destructor;
    Boolean is_qualified;
    Boolean isambig;
    Boolean x1F;
    Boolean is_type;
};
#pragma options align = reset
struct CScopeObjectIterator {
    struct NameSpace *nspace;
    struct NameSpaceName *nextname;
    struct NameSpaceObjectList *currlist;
    SInt32 hashindex;
};
struct CScopeNSIterator {
    struct NameSpace *nspace;
    struct NameSpaceLookupList *lookup;
    struct NameResult *result;
};
struct NameSpaceLookupList {
    struct NameSpaceLookupList *next;
    struct NameSpace *nspace;
    struct NameSpaceList *namespaces;
};
extern void CScope_ParseUsingDeclaration(NameSpace *nspace, AccessType flag, Boolean unused);
extern void CScope_AddClassUsingDeclaration(TypeClass *def, TypeClass *tp, HashNameNode *name, Boolean flag);
extern void add_using_declaration(BClassList *bases, NameSpace *scope, ObjBase *def, HashNameNode *name, char access);
extern BClassList *CScope_GetClassAccessPath(BClassList *classes, TypeClass *base);
extern ObjectList *CScope_GetLocalObject(NameSpace *nspace, HashNameNode *name);
extern ObjectList *remove_dalias_objects(NameSpaceObjectList *list);
extern Boolean CScope_FindTypeName(NameSpace *arg0, HashNameNode *arg1, NameResult *arg2);
extern Type *CScope_GetLocalTagType(NameSpace *nspace, HashNameNode *name);
extern void CScope_DefineTypeTag(NameSpace *ns, HashNameNode *arg2, Type *arg3);
extern NameSpaceObjectList *CScope_NextObjectIteratorObjectList(CScopeObjectIterator *state);
extern Object *CScope_NextObjectIteratorObject(CScopeObjectIterator *s);
extern int CScope_InitObjectIterator(CScopeObjectIterator *save, NameSpace *obj);
extern Boolean CScope_PossibleTypeName(HashNameNode *arg);
extern NameSpaceObjectList *CScope_FindObjectList(NameResult *result, HashNameNode *arg);
extern Boolean CScope_ParseElaborateName(NameResult *s);
extern Boolean CScope_ParseQualifiedNameSpace(NameResult *result, SInt32 flag);
extern Boolean CScope_ParseDeclName(NameResult *lookup);
extern Boolean CScope_ParseExprName(NameResult *scope);
extern Boolean parse_name_in_namespace(NameResult *scope, NameSpace *ns);
extern Boolean parse_qualified_templdep_type(NameResult *context, Type *qualifier, Boolean allowToken328);
extern Type *CScope_GetTagType(NameSpace *nspace, HashNameNode *name);
extern Type *CScope_GetType(NameSpace *nspace, HashNameNode *name, UInt32 *qual);
extern Boolean find_type_name_in_scope(NameResult *out, NameSpace *scope, HashNameNode *name);
extern NameSpaceObjectList *find_namespace_object(NameResult *state, NameSpace *nspace, HashNameNode *name,
                                                  NameSpace **foundSpace);
extern NameSpace *find_name_nspace(NameResult *result, NameSpace *nspace, HashNameNode *name);
extern Boolean CScope_FindClassMemberObject(TypeClass *tclass, NameResult *result, HashNameNode *name);
extern Boolean set_parse_result_from_objects(NameResult *result, NameSpaceObjectList *objects, HashNameNode *name);
extern NameSpaceObjectList *find_scope_object_list(CScopeNSIterator *ctx, HashNameNode *key);
extern NameSpaceObjectList *fn_0049a000(NameSpaceLookupList *ctx, HashNameNode *name, NameSpace **outscope);
extern Boolean find_and_append_class_member_path(NameResult *scope, NameSpace *target, HashNameNode *mode,
                                                 Boolean flag);
extern NameSpace *get_object_list_nspace(ObjectList *objects, Boolean *flag);
extern BClassList *find_class_member_path(NameResult *result, TypeClass *tclass, SInt32 offset);
extern NameSpaceLookupList *CScope_BuildNameSpaceLookupList(NameSpace *ns);
extern void CScope_AddGlobalObject(Object *object);
extern NameSpace *CScope_NewListNameSpace(HashNameNode *name, Boolean is_global);
extern NameSpaceList *collect_type_namespaces(NameSpaceList *acc, Type *type);
extern NameSpaceObjectList *CScope_InsertNameSpaceName(NameSpace *nspace, HashNameNode *name);
extern UInt8 CScope_IsEmptyNameSpace(NameSpace *nameSpace);
extern Boolean CScope_IsStdNameSpace(NameSpace *nspace);
extern NameSpace *CScope_FindGlobalNS(NameSpace *scope);
extern NameSpace *CScope_FindNonClassNonFunctionNS(NameSpace *nspace);
extern UInt8 CScope_IsInLocalNameSpace(NameSpace *scope);
extern UInt8 CScope_FindQualifiedClassMember(NameResult *holder, TypeClass *type, HashNameNode *name);
extern BClassList *find_base_class_path(TypeClass *theclass, TypeClass *target, unsigned int offset);
extern void CScope_MergeNameSpace(NameSpace *dest, NameSpace *source);
extern NameSpace *CScope_NewHashNameSpace(HashNameNode *name);
extern NameSpaceObjectList *CScope_InsertName(NameSpace *scope, HashNameNode *name);
extern NameSpaceName *CScope_FindNameSpaceName(NameSpace *nameSpace, HashNameNode *name);
extern NameSpaceObjectList *CScope_FindName(NameSpace *space, HashNameNode *name);
extern Boolean CScope_IsEmptySymTable(void);
extern void CScope_AddObject(NameSpace *scope, HashNameNode *name, ObjBase *object);
extern Boolean CScope_FindObject(NameSpace *nspace, NameResult *result, HashNameNode *name);
extern Boolean CScope_ParseMemberName(TypeClass *ctx, NameResult *node, Boolean flag);
extern NameSpaceObjectList *CScope_ArgumentDependentNameLookup(NameSpaceObjectList *results, HashNameNode *name,
                                                               ENodeList *objects, char excludeMethods);
extern struct NameSpaceLookupList *build_namespace_scope_rec(NameSpace *nspace);
extern NameSpaceList *fn_0049b300(NameSpaceList *list, NameSpace *nspace);
extern unsigned int CScope_ParseUsingDirective(NameSpace *container);
extern NameSpace *parse_namespace_name(NameSpace *nameSpace);
extern void CScope_ParseNameSpaceAlias(HashNameNode *name);
extern void CScope_Setup(void);
extern void CScope_Cleanup(void);
extern void CScope_GetScope(CScopeSave *save);
extern void CScope_SetNameSpaceScope(NameSpace *nspace, CScopeSave *save);
extern void CScope_SetClassScope(TypeClass *cls, CScopeSave *save);
extern void CScope_SetClassDefScope(TypeClass *cls, CScopeSave *save);
extern void CScope_RestoreScope(CScopeSave *save);
extern void CScope_SetMethodScope(Object *cls, TypeClass *ns, unsigned char flag, CScopeSave *save);
extern void CScope_SetFunctionScope(Object *function, CScopeSave *saved);
extern UInt8 cscope_is_member_func;
struct HashNameNode;
struct NameSpace;

#ifdef __cplusplus
}
#endif

#endif
