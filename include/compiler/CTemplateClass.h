#ifndef COMPILER_CTEMPLATECLASS_H
#define COMPILER_CTEMPLATECLASS_H

#include "compiler/common.h"
#include "compiler/templates.h"
#include "compiler/objects.h"
#include "compiler/enode.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateNew.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 2)
struct TemplateClassMatch {
    struct TemplateClassMatch *next;
    struct TemplPartialSpec *candidate;
};
#pragma pack(pop)

extern unsigned char CTemplateClass_InstantiateClass(TypeClass *theclass);
extern void instantiate_namespace_objects(TypeDeduce *map, TypeClass *unused, TypeClass *obj);
extern void fn_0051c680(TypeDeduce *ctx, Object *obj);
extern void instantiate_objtype(TypeDeduce *context, ObjType *type, HashNameNode *name);
extern void initialize_enum_constants(TypeDeduce *context, struct TemplateAction *object, TypeEnum *scope);
extern void instantiate_enum(TypeDeduce *context, struct TemplateAction *entry);
extern struct TemplateClassMatch *remove_less_specialized_matches(struct TemplateClassMatch *list);
extern unsigned char match_template_arguments(TemplPartialSpec *arguments, TemplPartialSpec *pattern);
extern TemplArg *match_specialization_arguments(TemplPartialSpec *arguments, TemplArg *actual, char instantiate);
extern void CTemplateClass_ParseClassDeclaration(TemplateScopeState *scope, TemplParam *parameters, short access,
                                                 SInt32 *state);
extern unsigned char CTemplateClass_CompleteClassLayout(struct TemplClass *state, ClassLayout *values);
extern struct TemplClass *CTemplateClass_CreateClassTemplateDeclaration(TypeClass *owner, HashNameNode *arg1,
                                                                        short arg2);
extern char CTemplateClass_SelectSpecialization(TemplArg *context, struct TemplClass **classType, TemplArg **result);
extern TemplClassInst *create_class_template_instance(struct TemplClass *definition, void *argument,
                                                      void *alternate_argument);
extern void CTemplateClass_AppendExpressionRecord(struct TemplClass *type, Object *object, ENode *expression);
extern void CTemplateClass_AppendEnumConstDeclaration(struct TemplClass *self, ObjEnumConst *a, ENode *str);
extern void CTemplateClass_AppendEnumDeclaration(struct TemplClass *type, TypeEnum *value);
extern void CTemplateClass_AppendFuncDeclaration(struct TemplClass *owner, TypeTemplDep *value, unsigned char kind);
extern struct TemplClass *CTemplateClass_ResolveRelatedClass(struct TemplClass *record);
extern unsigned int CTemplateClass_PrependTemplateRecordEntry(struct TemplClass *list, Type *value,
                                                              unsigned char value24, unsigned char value25);
extern void instantiate_friend_declaration(TypeDeduce *ctx, struct TemplateFriend *declaration);
extern void instantiate_object_type(TypeDeduce *context, TemplateAction *function, ObjBase *object);
extern void instantiate_template_object(TypeDeduce *ctx, Object *templ);
extern void fn_0051cec0(TypeDeduce *context, struct TemplClass *templateClass);
extern void CTemplateClass_ParsePartialSpecialization(TemplateScopeState *scope, struct TemplParam *parameters,
                                                      short access, SInt32 *position);
extern TemplClassInst *CTemplateClass_GetInstance(struct TemplClass *cls, TemplArg *key, TemplArg *flag);
extern struct TemplateMember *CTemplateClass_AddTemplateArgumentOverride(struct TemplClass *owner, Object *key,
                                                                         FileOffsetInfo *name,
                                                                         struct TokenStream *payload);
extern void CTemplateClass_AppendObjectDeclaration(struct TemplClass *ctx, Object *obj);
extern void instantiate_bases(TypeDeduce *arg1, TypeClass *arg2, struct TemplClass *arg3);
extern void instantiate_ivars(TypeDeduce *ctx, TypeClass *dst, struct TemplClass *src);
extern void CTemplateClass_AddDeferredFunctionDeclaration(struct TemplClass *p1, DeclInfo *p2);
extern void fn_0051b800(void);
extern void fn_0051b810(void);
extern struct TemplClass *class_template_list;
extern char *CTemplateClass_ParseDouble(char *value, double *result, char *error);
struct TemplClass;

#ifdef __cplusplus
}
#endif

#endif
