#ifndef COMPILER_CTEMPLATEFUNC_H
#define COMPILER_CTEMPLATEFUNC_H

#include "compiler/common.h"
#include "compiler/templates.h"
#include "compiler/CTemplateTools.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Boolean CTemplTool_IsTemplateArgumentDependentType(Type *ty);
extern Boolean CTemplateFunc_MatchType(Type *pattern, UInt32 patternQual, Type *argument, UInt32 argumentQual,
                                       TemplArg *state, Boolean flag);
extern Boolean match_args(TypeFunc *a, TypeFunc *b, TemplArg *c, Boolean d);
extern Boolean match_state_elem_arguments(TemplArg *a, TemplArg *b, TemplArg *r, char flag);
extern int CTemplateFunc_GetArgumentParameterIndex(TemplArg *argument);
extern Object *select_unique_undominated_match(Object *func, struct MatchLink *funcs, int arg3);
extern unsigned char match_candidate_to_template_args(Object *candidate, Object *templ);
extern struct TemplFuncInstance *instantiate_accessible_template_for_type(Object *func, Type *ftype, void *args,
                                                                          Object *flags, int arg5);
extern struct TemplFuncInstance *CTemplateFunc_FindOrCreateMatchedSpecialization(Object *func, Type *name, void *arg3,
                                                                                 Object *arg4);
extern Boolean match_class_args_or_bases(struct TemplClass *p1, TemplArg *p2, TypeClass *p3, TemplArg *p4, Boolean p5);
extern struct TemplFuncInstance *find_or_create_template_specialization(Object *func, TemplArg *args, Object *premade);
extern void fn_00514380(ObjectList *list, void *ptype, ENodeList *args, struct ArgMatch *ctx, ENode *flag);
extern Boolean match_template_function_args(Object *obj, DeduceInfo *state, FuncArg *arg, ENodeList *exprs,
                                            ArgMatch *ctx);
extern Object *CTemplateFunc_FindSpecializationObject(DeclInfo *search, ObjectList *candidates);

#ifdef __cplusplus
}
#endif

#endif
