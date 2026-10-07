#ifndef COMPILER_CTEMPLATETOOLS_H
#define COMPILER_CTEMPLATETOOLS_H

#include "compiler/common.h"
#include "compiler/templates.h"
#include "compiler/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct TemplateLookupContext {
    struct TemplClass *owner;
    struct TemplClass *currentClass;
    void *unk8;
    struct TemplateParameterIDEntry *entries;
};
#pragma pack(pop)
struct TemplateParameterIDEntry {
    struct TemplateParameterIDEntry *next;
    struct {
        short index;
        char nindex;
        char type;
    } pid;
};
extern Type *make_bitfield_type(TypeDeduce *ctx, Type *ty, ENode *node, UInt32 *out);
extern Boolean CTemplTool_TemplDepTypeCompare(TypeTemplDep *a, TypeTemplDep *b);
extern Boolean CTemplTool_EqualExprTypes(ENode *n1, ENode *n2);
extern void CTemplTool_RemoveOuterTemplateArgumentNameSpace(NameSpace *ns);
extern void CTemplTool_MergeArgNames(Type *sourceFunc, Type *destinationFunc);
extern void CTemplTool_InsertTemplateParameter(NameSpace *scope, TemplParam *source);
extern Type *resolve_templ_dep_pointer_target(TypeDeduce *context, Type *typeArg, UInt32 *qualifiers);
extern TemplClassInst *fn_00517270(TypeClass *current, TemplClassInst *limit, TemplClassInst *target);
extern void CTemplTool_CheckTemplArgType(Type *type);
extern ENode *CTempl_MakeTemplDepExpr(ENode *left, UInt8 kind, ENode *right);
extern Boolean CTemplTool_IsTemplateArgumentDependentExpression(ENode *node);
extern void CTemplTool_RemoveTemplateArgumentNameSpace(NameSpace *args, TemplClassInst *func, CScopeSave *scope);
extern void CTemplTool_SetupOuterTemplateArgumentNameSpace(NameSpace *nameSpace);
extern struct TemplateFunction *CTemplTool_GetFuncTempl(Object *obj);
extern Type *resolve_templ_dep_type(TypeDeduce *ctx, TypeTemplDep *arg, UInt32 *out);
extern struct TemplArg *find_template_argument(struct TypeDeduce *context, struct TemplParamID pid);
extern void fn_00516930(void *context, struct TemplClass *function, TemplArg *arguments);
extern ExceptSpecList *copy_resolved_except_spec_list(void *ctx, ExceptSpecList *n);
extern unsigned char CTemplTool_IsTemplateArgumentDependentType(Type *type);
extern TemplClassInst *find_corresponding_instance_class(TypeDeduce *list, TypeClass *targetClass);
extern UInt8 CTemplTool_IsIdenticalTemplArgList(TemplArg *pattern, TemplParam *argument);
extern UInt8 CTemplTool_IsSameTemplate(TemplParam *parameter, TemplArg *argument);
extern TemplArg *CTemplTool_MakeTemplArgList(struct DeduceInfo *src);
extern NameSpace *CTemplTool_SetupTemplateArgumentNameSpace(TemplParam *arglist, TemplArg *targlist, Boolean flag);
extern TypeClass *CTemplTool_IsTemplate(TypeTemplDep *record);
extern NameSpace *CTemplTool_InsertTemplateArgumentNameSpace(TemplParam *context, TemplClassInst *function,
                                                             CScopeSave *scope);
extern FuncArg *CTemplTool_DeduceArgCopy(TypeDeduce *ctx, FuncArg *args);
extern Type *CTemplTool_DeduceArgDepType(TemplArg *record, TypeTemplDep *key, unsigned int qualifiers,
                                         unsigned int *resultQualifiers);
extern void CTemplTool_MergeDefaultArgs(TemplParam *destination, TemplParam *source);
extern ENode *CTemplTool_DeduceExpr(TypeDeduce *ctx, ENode *node);
extern Type *CTemplTool_IsDependentTemplate(struct TemplClass *templateClass, TemplArg *arguments);
extern struct TemplClass *CTemplTool_GetSelfRefTemplate(TypeTemplDep *reference);
extern TemplArg *CTemplTool_MakeGlobalTemplArgCopy(TemplArg *p);
extern void CTemplTool_PushInstance(TemplStack *stack, TypeClass *tmclass, Object *func);
extern void CTemplTool_PopInstance(TemplStack *stack);
extern UInt8 CTemplTool_EqualParams(TemplParam *left, TemplParam *right, char copyValue);
extern UInt8 CTemplTool_EqualArgs(TemplArg *left, TemplArg *right);
extern Boolean CTemplTool_InitDeduceInfo(DeduceInfo *info, TemplParam *params, TemplArg *args, Boolean flag);
extern Type *CTemplTool_DeduceTypeCopy(TypeDeduce *ctx, Type *type, UInt32 *qual);
extern struct TemplClass *fn_00516b50(TemplateLookupContext *context, struct TemplClass *record);
extern Boolean CTemplateTools_MatchTypeAndCheckBoundSlots(Object *obj, Type *name, void *arg3);
extern Boolean CTemplTool_IsSameTemplateType(TypeClass *type, TypeTemplDep *templateType);
extern Type *CTemplTool_ResolveMemberSelfRefs(struct TemplClass *tclass, Type *type, UInt32 *qualifiers);
extern SInt16 ctempl_instdepth;
struct TemplArg;

#ifdef __cplusplus
}
#endif

#endif
