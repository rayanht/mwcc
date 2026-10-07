#ifndef COMPILER_CSCOPE_H
#define COMPILER_CSCOPE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The current scope, saved while another is entered (CScope_SetFunctionScope, BE_elf_SaveAndSetClassScope, BE_elf_SaveScopeAndEnterClass, BE_elf_SaveAndSetScope) and put
   back by CScope_RestoreScope. */
#pragma options align = mac68k
struct CScopeSave {
    struct NameSpace *nspace;   /* 0x00: BE_elf_SaveScope saves currentNameSpace. */
    struct TypeClass *theclass; /* 0x04: BE_elf_SaveScope saves data_00588040. */
    struct Object *function;    /* 0x08: BE_elf_SaveScope saves data_00588238. */
    UInt8 member_context;       /* 0x0c: BE_elf_SaveScope saves data_005884f8. */
    UInt8 trailingPadding
        [3]; /* 0x0d: no CScopeSave user reads or writes these bytes; BE_elf_SaveScope saves only the four preceding members. */
};
#pragma options align = reset
#pragma options align = mac68k
struct CScopeParseResult {
    struct NameSpace *nspace;
    struct HashNameNode *name;
    struct {
        struct Type *
            base; /* 0x08: set_parse_result_from_objects sets the resolved type; CScope.c tests type == 9 before using TypeTemplDep */
    } type;
    UInt32 qualifiers;
    struct ObjBase *object;
    struct NameSpaceObjectList *objects;
    struct BClassList *basePath;
    Boolean is_destructor;
    Boolean is_qualified;
    Boolean isambig;
    Boolean x1F;
    Boolean is_type;
    Boolean
        alignmentPadding; /* 0x21: CScope_AddClassUsingDeclaration clears sizeof(result); unused trailing byte rounds the result to two-byte alignment. */
};
#pragma options align = reset
struct LookupCtx {
    struct NameSpace *namespaceCursor;
    struct ScopeRec *scopeChain;
    struct CScopeParseResult *lookupState;
};
struct ScopeRec {
    struct ScopeRec *outer;
    struct NameSpace *ns;
    struct NameSpaceList *list;
};
extern void CScope_ParseUsingDeclaration(NameSpace *nspace, AccessType flag, Boolean unused);
extern void CScope_AddClassUsingDeclaration(TypeClass *def, TypeClass *tp, HashNameNode *name, Boolean flag);
extern void add_using_declaration(BClassList *bases, NameSpace *scope, ObjBase *def, HashNameNode *name, char access);
extern BClassList *CScope_GetClassAccessPath(BClassList *classes, TypeClass *base);
extern ObjectList *CScope_FindObjectListInNameSpace(NameSpace *nspace, HashNameNode *name);
extern ObjectList *remove_dalias_objects(NameSpaceObjectList *list);
extern Boolean CScope_FindTypeName(NameSpace *arg0, HashNameNode *arg1, CScopeParseResult *arg2);
extern Type *CScope_GetTagType(NameSpace *nspace, HashNameNode *name);
extern void CScope_DefineTypeTag(NameSpace *ns, HashNameNode *arg2, Type *arg3);
extern NameSpaceObjectList *CScope_NextNameSpaceObjectList(ScopeSearch *state);
extern Object *CScope_NextObject(ScopeSearch *s);
extern int CScope_InitScopeSearch(ScopeSearch *save, NameSpace *obj);
extern Boolean CScope_PossibleTypeName(HashNameNode *arg);
extern NameSpaceObjectList *CScope_FindObjectList(CScopeParseResult *result, HashNameNode *arg);
extern Boolean CScope_ParseElaborateName(CScopeParseResult *s);
extern Boolean CScope_ParseQualifiedScope(CScopeParseResult *result, SInt32 flag);
extern Boolean CScope_ParseDeclName(CScopeParseResult *lookup);
extern Boolean CScope_ParseExprName(CScopeParseResult *scope);
extern Boolean parse_name_in_namespace(CScopeParseResult *scope, NameSpace *ns);
extern Boolean parse_qualified_templdep_type(CScopeParseResult *context, Type *qualifier, Boolean allowToken328);
extern Type *CScope_FindTagType(NameSpace *nspace, HashNameNode *name);
extern Type *CScope_GetType(NameSpace *nspace, HashNameNode *name, UInt32 *qual);
extern Boolean find_type_name_in_scope(CScopeParseResult *out, NameSpace *scope, HashNameNode *name);
extern NameSpaceObjectList *find_namespace_object(CScopeParseResult *state, NameSpace *nspace, HashNameNode *name,
                                                  NameSpace **foundSpace);
extern NameSpace *find_name_nspace(CScopeParseResult *result, NameSpace *nspace, HashNameNode *name);
extern Boolean CScope_FindClassMemberObject(TypeClass *tclass, CScopeParseResult *result, HashNameNode *name);
extern Boolean set_parse_result_from_objects(CScopeParseResult *result, NameSpaceObjectList *objects,
                                             HashNameNode *name);
extern NameSpaceObjectList *find_scope_object_list(LookupCtx *ctx, HashNameNode *key);
extern NameSpaceObjectList *CScope_0049a000(ScopeRec *ctx, HashNameNode *name, NameSpace **outscope);
extern Boolean find_and_append_class_member_path(CScopeParseResult *scope, NameSpace *target, HashNameNode *mode,
                                                 Boolean flag);
extern NameSpace *get_object_list_nspace(ObjectList *objects, Boolean *flag);
extern BClassList *find_class_member_path(CScopeParseResult *result, TypeClass *tclass, SInt32 offset);
extern ScopeRec *build_usings_scope_list(NameSpace *ns);
extern void CScope_AddGlobalObject(Object *object);
extern NameSpace *CScope_NewListNameSpace(HashNameNode *name, Boolean is_global);
extern NameSpaceList *collect_type_namespaces(NameSpaceList *acc, Type *type);
extern NameSpaceObjectList *CScope_InsertNameSpaceName(NameSpace *nspace, HashNameNode *name);
extern UInt8 CScope_IsEmptyNameSpace(NameSpace *nameSpace);
extern Boolean CScope_IsStdNameSpace(NameSpace *nspace);
extern NameSpace *CScope_FindGlobalNS(NameSpace *scope);
extern NameSpace *CScope_FindNonClassNonTemplNameSpace(NameSpace *nspace);
extern UInt8 CScope_IsInLocalNameSpace(NameSpace *scope);
extern UInt8 CScope_FindQualifiedClassMember(CScopeParseResult *holder, TypeClass *type, HashNameNode *name);
extern BClassList *find_base_class_path(TypeClass *theclass, TypeClass *target, unsigned int offset);
extern void CScope_MergeNameSpace(NameSpace *dest, NameSpace *source);
extern NameSpace *CScope_NewHashNameSpace(HashNameNode *name);
extern NameSpaceObjectList *CScope_InsertName(NameSpace *scope, HashNameNode *name);
extern NameSpaceName *CScope_FindNameSpaceName(NameSpace *nameSpace, HashNameNode *name);
extern NameSpaceObjectList *CScope_FindName(NameSpace *space, HashNameNode *name);
extern Boolean CScope_IsEmptySymTable(void);
extern void CScope_AddObject(NameSpace *a0, HashNameNode *a1, ObjBase *a2);
extern Boolean CScope_FindObject(NameSpace *nspace, CScopeParseResult *result, HashNameNode *name);
extern Boolean CScope_ParseMemberName(TypeClass *ctx, CScopeParseResult *node, Boolean flag);
extern NameSpaceObjectList *CScope_ArgumentDependentNameLookup(NameSpaceObjectList *results, HashNameNode *name,
                                                               ENodeList *objects, char excludeMethods);
extern struct ScopeRec *build_namespace_scope_rec(NameSpace *nspace);
extern NameSpaceList *fn_0049b300(NameSpaceList *list, NameSpace *nspace);
extern unsigned int CScope_ParseUsingDirective(NameSpace *container);
extern NameSpace *parse_namespace_name(NameSpace *nameSpace);
extern void CScope_ParseNameSpaceAlias(HashNameNode *name);
extern void CScope_RestoreScope(CScopeSave *save);
extern void CScope_SetMethodScope(Object *cls, TypeClass *ns, unsigned char flag, CScopeSave *save);
extern void CScope_SetFunctionScope(Object *function, CScopeSave *saved);
extern UInt8 data_005884f8;
struct HashNameNode;
struct HashNameNode;
struct NameSpace;

#ifdef __cplusplus
}
#endif

#endif
