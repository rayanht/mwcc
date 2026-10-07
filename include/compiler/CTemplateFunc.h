#ifndef COMPILER_CTEMPLATEFUNC_H
#define COMPILER_CTEMPLATEFUNC_H

#include "compiler/common.h"
#include "compiler/CTemplateTools.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct TemplateMatchState {
    CTStateElem *slots;
    CTStateElem inline_slots[16];
    SInt32 nslots;
    SInt32 suppliedArgumentCount;
    UInt8 depth;
    UInt8
        alignmentPadding; /* 0x12d: CTemplTool_InitDeduceInfo clears sizeof(*info); unused trailing byte after depth */
};
#pragma options align = reset
#pragma options align = mac68k
struct TemplateSpecializationData {
    struct TemplateSpecializationData *next;
    struct Object *object;
    struct CTStateElem *templateArguments;
    UInt8 active;
    UInt8 internalStorage;
    UInt8 suppressImplicitInstantiation;
};
#pragma options align = reset
extern Boolean CTemplTool_IsTemplateArgumentDependentType(Type *ty);
extern Boolean CTemplateFunc_MatchType(Type *pattern, UInt32 patternQual, Type *argument, UInt32 argumentQual,
                                       CTStateElem *state, Boolean flag);
extern Boolean match_args(TypeFunc *a, TypeFunc *b, CTStateElem *c, Boolean d);
extern Boolean match_state_elem_arguments(CTStateElem *a, CTStateElem *b, CTStateElem *r, char flag);
extern int CTemplateFunc_GetArgumentParameterIndex(CTStateElem *argument);
extern Object *select_unique_undominated_match(Object *func, struct MatchLink *funcs, int arg3);
extern unsigned char match_candidate_to_template_args(Object *candidate, Object *templ);
extern struct TemplateSpecializationData *instantiate_accessible_template_for_type(Object *func, Type *ftype,
                                                                                   void *args, Object *flags, int arg5);
extern struct TemplateSpecializationData *CTemplateFunc_FindOrCreateMatchedSpecialization(Object *func, Type *name,
                                                                                          void *arg3, Object *arg4);
extern Boolean match_class_args_or_bases(struct TypeClassTemplate *p1, CTStateElem *p2, TypeClass *p3, CTStateElem *p4,
                                         Boolean p5);
extern struct TemplateSpecializationData *find_or_create_template_specialization(Object *func, CTStateElem *args,
                                                                                 Object *premade);
extern void fn_00514220(void);
extern void fn_00514380(ObjectList *list, void *ptype, ENodeList *args, struct ArgMatch *ctx, ENode *flag);
extern Boolean match_template_function_args(Object *obj, TemplateMatchState *state, FuncArg *arg, ENodeList *exprs,
                                            ArgMatch *ctx);
extern Object *CTemplateFunc_FindSpecializationObject(DeclInfo *search, ObjectList *candidates);

#ifdef __cplusplus
}
#endif

#endif
