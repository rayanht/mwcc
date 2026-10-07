#define CERROR_FILE "CTemplateFunc.c"
#include "compiler/common.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"

#pragma options align = mac68k
static Boolean data_005824c8;
static UInt8 template_argument_depth;
static SInt32 data_005824ca;
static UInt8 data_005824ce;
static char lbl_005824cf[9];
#pragma options align = reset

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

static inline char CTemplateFunc_MatchesSpecialization(Object *candidate, Type *value, TemplArg *context)
{
    int index;
    DeduceInfo match;
    if (CTemplTool_InitDeduceInfo(&match, CTemplTool_GetFuncTempl(candidate)->params, context, 0) &&
        CTemplateFunc_MatchType(candidate->type, 0, value, 0, match.args, 1)) {
        for (index = 0; index < match.maxCount; index++) {
            if (!match.args[index].is_deduced)
                return 0;
        }
        return 1;
    }
    return 0;
}

Object *CTemplateFunc_FindSpecializationObject(DeclInfo *search, ObjectList *candidates)
{
    Object *candidate;
    Type *value;
    char matched;
    struct TemplFuncInstance *specialization;
    struct TemplFuncInstance *selected;
    TemplArg *context;
    if (search->requireTemplateClassMember != 0 || search->has_expltargs != 0) {
        for (; candidates != NULL; candidates = candidates->next) {
            if (candidates->object->otype == OT_OBJECT && candidates->object->type->type == TYPEFUNC &&
                (((TypeFunc *)candidates->object->type)->flags & 1024) != 0) {
                context = search->expltargs;
                value = search->thetype;
                candidate = (Object *)(int)candidates->object;
                matched = CTemplateFunc_MatchesSpecialization(candidate, value, context);
                if (matched != 0) {
                    specialization = (selected = CTemplateFunc_FindOrCreateMatchedSpecialization(
                                          candidates->object, search->thetype, search->expltargs, NULL));
                    if (selected != NULL) {
                        if (search->requireTemplateClassMember != 0) {
                            if (specialization->is_specialized == 0 && specialization->is_instantiated != 0)
                                CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_SPECIALIZATION);
                            specialization->is_specialized = 1;
                            search->requireTemplateClassMember = 0;
                        }
                        return specialization->object;
                    }
                }
            }
        }
    }
    return NULL;
}

void fn_00514380(ObjectList *list, void *ptype, ENodeList *args, struct ArgMatch *ctx, ENode *flag)
{
    Object *found = NULL;
    Object *savedObject = ctx->object;
    MemberCallArguments match;
    DeduceInfo conversion;
    DeduceInfo bestConversion;
    ObjectList one;

    while (list != NULL) {
        Object *object;
        if ((object = list->object)->otype == OT_OBJECT && object->type->type == TYPEFUNC &&
            (TYPE_FUNC(object->type)->flags & 0x400)) {
            if (CExpr_GetFuncMatchArgs(object, args, flag, &match)) {
                if (CTemplTool_InitDeduceInfo(&conversion, CTemplTool_GetFuncTempl(object)->params, ptype, 0)) {
                    if (match_template_function_args(object, &conversion, match.parameters, match.arguments, ctx)) {
                        bestConversion = conversion;
                        if (conversion.args == conversion.argBuffer)
                            bestConversion.args = bestConversion.argBuffer;
                        found = object;
                    }
                }
            }
        }
        list = list->next;
    }
    if (found != NULL) {
        if (ctx->list != NULL) {
            ENodeList *arg = args;
            int count = 0;
            Object *result;
            while (arg != NULL) {
                arg = arg->next;
                count++;
            }
            result = select_unique_undominated_match(ctx->object, ctx->list, count);
            if (result == NULL) {
                CError_OverloadedFunctionError(ctx->object, ctx->list);
                ctx->list = NULL;
                if (savedObject != NULL)
                    ctx->object = savedObject;
                else
                    ctx->object = find_or_create_template_specialization(found, bestConversion.args, NULL)->object;
            } else {
                one.next = NULL;
                one.object = result;
                memclrw(ctx, sizeof(*ctx));
                fn_00514380(&one, ptype, args, ctx, flag);
            }
        } else {
            ctx->object = find_or_create_template_specialization(found, bestConversion.args, NULL)->object;
        }
    } else {
        ctx->object = savedObject;
    }
}
Boolean match_template_function_args(Object *obj, DeduceInfo *state, FuncArg *arg, ENodeList *exprs, ArgMatch *ctx)
{
    UInt32 qual;
    ObjectList *candidate;
    int index;
    Type *matchType;
    UInt32 targetQual;
    int argumentQual;
    Type *argumentType;
    Type *targetType;
    Type *exprType;
    ArgMatch counts;
    UInt32 exprQual;
    UInt32 argQual;
    UInt32 resolvedQual;
    TypeDeduce tempArg;
    Type *resolvedType;

    memclrw(&counts, 30);
    for (;;) {
        if (!arg || arg->type == &stvoid) {
            if (exprs)
                return 0;
            break;
        }
        if (arg == &data_00583098 || arg == &data_00584748)
            break;
        if (!exprs) {
            if (arg->dexpr)
                break;
            return 0;
        }
        template_argument_depth = state->depth;
        data_005824ca = state->count;
        data_005824ce = 1;
        if (CTemplTool_IsTemplateArgumentDependentType(arg->type)) {
            if (!data_005824ce) {
                resolvedType = arg->type;
                argQual = arg->qual & (Q_CONST | Q_VOLATILE);
                exprType = exprs->node->rtype;
                exprQual = exprs->node->flags & ENODE_FLAG_QUALS;
                if (resolvedType->type == TYPEPOINTER && (((TypePointer *)resolvedType)->qual & Q_REFERENCE)) {
                    resolvedType = ((TypePointer *)resolvedType)->target;
                    targetQual = CParser_GetCVTypeQualifiers(resolvedType, argQual);
                    qual = CParser_GetCVTypeQualifiers(exprType, exprQual);
                    if ((!(targetQual & Q_CONST) && (qual & Q_CONST)) ||
                        (!(targetQual & Q_VOLATILE) && (qual & Q_VOLATILE)))
                        return 0;
                    if (!(targetQual & Q_CONST) && !CExpr_IsLValue(exprs->node))
                        return 0;
                }
                targetType = CParser_RemoveTopMostQualifiers(resolvedType, &argQual);
                resolvedType = CParser_RemoveTopMostQualifiers(exprType, &exprQual);
                if (exprs->node->type == EOBJREF && exprs->node->data.objref->type->type == TYPEFUNC &&
                    (((TypeFunc *)exprs->node->data.objref->type)->flags & 1024))
                    return 0;
                if (exprs->node->type == ENEWEXCEPTION) {
                    for (candidate = exprs->node->data.overloadCandidates; candidate; candidate = candidate->next) {
                        if (candidate->object->otype == OT_OBJECT && candidate->object->type->type == TYPEFUNC &&
                            (((TypeFunc *)candidate->object->type)->flags & 1024))
                            break;
                    }
                    if (candidate)
                        return 0;
                    if (resolvedType->type == TYPEFUNC)
                        resolvedType = CDecl_NewPointerType(resolvedType);
                }
                data_005824c8 = 0;
                if (!CTemplateFunc_MatchType(targetType, argQual, resolvedType, exprQual, state->args, 1))
                    return 0;
                if (data_005824c8)
                    counts.score3Count += 1;
                else
                    counts.score1Count += 1;
                if (arg->type->type == TYPEPOINTER)
                    CExpr_MatchCV(resolvedType, exprQual, arg->type, arg->qual, &counts);
            } else {
                TemplArg *entries;
                Type *resultType;
                TemplateFunction *nspace;
                entries = state->args;
                argumentQual = arg->qual;
                argumentType = arg->type;
                nspace = CTemplTool_GetFuncTempl(obj);
                memclrw(&tempArg, sizeof(tempArg));
                tempArg.params = nspace->params;
                tempArg.args = entries;
                resolvedQual = argumentQual;
                resultType = matchType = CTemplateTools_ResolveType(&tempArg, argumentType, &resolvedQual);
                if (resultType && !CExpr2_UpdateArgMatchScores(matchType, resolvedQual, exprs->node, &counts))
                    return 0;
            }
        } else if (!CExpr2_UpdateArgMatchScores(arg->type, arg->qual, exprs->node, &counts)) {
            return 0;
        }
        exprs = exprs->next;
        arg = arg->next;
    }
    for (index = 0; index < state->maxCount; ++index) {
        if (!state->args[index].is_deduced)
            return 0;
        state->args[index].next = &state->args[index + 1];
    }
    state->args[state->maxCount].next = NULL;
    return CExpr_MatchCompare(obj, ctx, &counts);
}

Boolean CTemplTool_IsTemplateArgumentDependentType(Type *ty)
{
    FuncArg *a;
    Boolean r;

    for (;;) {
        switch ((SInt8)ty->type) {
            case TYPETEMPLATE:
                switch (((TypeTemplDep *)ty)->dtype) {
                    case 0:
                        if ((SInt32)((TypeTemplDep *)ty)->u.pid.index >= data_005824ca ||
                            ((TypeTemplDep *)ty)->u.pid.nindex != template_argument_depth)
                            data_005824ce = 0;
                        return 1;
                    case 1:
                        CTemplTool_IsTemplateArgumentDependentType(((TypeTemplDep *)ty)->u.array.type);
                        return 1;
                    case 2:
                        data_005824ce = 0;
                        return 1;
                    case 3:
                        CTemplTool_IsTemplateArgumentDependentType(((TypeTemplDep *)ty)->u.array.type);
                        data_005824ce = 0;
                        return 1;
                    case 4:
                        CTemplTool_IsTemplateArgumentDependentType(((TypeTemplDep *)ty)->u.array.type);
                        data_005824ce = 0;
                        return 1;
                    case 5:
                    default:
                        CError_FATAL(904);
                }
            case TYPEVOID:
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPESTRUCT:
            case TYPECLASS:
                return 0;
            case TYPEMEMBERPOINTER:
                r = 0;
                if (CTemplTool_IsTemplateArgumentDependentType(((TypeMemberPointer *)ty)->ty1))
                    r = 1;
                if (CTemplTool_IsTemplateArgumentDependentType(((TypeMemberPointer *)ty)->ty2))
                    r = 1;
                return r;
            case TYPEPOINTER:
            case TYPEARRAY:
                ty = ((TypePointer *)ty)->target;
                continue;
            case TYPEFUNC:
                r = 0;
                for (a = ((TypeFunc *)ty)->args; a != NULL && a != &data_00583098; a = a->next)
                    if (CTemplTool_IsTemplateArgumentDependentType(a->type))
                        r = 1;
                if (CTemplTool_IsTemplateArgumentDependentType(((TypeFunc *)ty)->functype))
                    r = 1;
                return r;
            default:
                CError_FATAL(936);
                continue;
        }
    }
}
static inline TypeTemplDep *CTemplateFunc_TemplateType(Type *type)
{
    return (TypeTemplDep *)type;
}

Boolean CTemplateFunc_MatchType(Type *pattern, UInt32 patternQual, Type *argument, UInt32 argumentQual, TemplArg *state,
                                Boolean flag)
{
    for (;;) {
        switch ((char)pattern->type) {
            case TYPETEMPLATE:
                switch (CTemplateFunc_TemplateType(pattern)->dtype) {
                    case 0: {
                        SInt16 index = CTemplateFunc_TemplateType(pattern)->u.pid.index;
                        if (state[index].is_deduced) {
                            if (iscpp_typeequal(argument, state[index].data.typeparam.type) == 0)
                                return 0;
                            patternQual |= state[index].data.typeparam.qual;
                            if ((argumentQual & 1) && !(patternQual & 1))
                                return 0;
                            if ((argumentQual & 2) && !(patternQual & 2))
                                return 0;
                        } else {
                            state[index].data.typeparam.type = argument;
                            state[index].data.typeparam.qual = 0;
                            if ((argumentQual & 1) && !(patternQual & 1))
                                state[index].data.typeparam.qual |= Q_CONST;
                            if ((argumentQual & 2) && !(patternQual & 2))
                                state[index].data.typeparam.qual |= Q_VOLATILE;
                            state[index].pid.type = 1;
                            state[index].is_deduced = 1;
                        }
                        return 1;
                    }
                    case 1:
                        return 1;
                    case 2:
                        if (argument->type == TYPECLASS) {
                            TypeClass *argumentClass = (TypeClass *)argument;
                            return match_class_args_or_bases(CTemplateFunc_TemplateType(pattern)->u.templ.templ,
                                                             CTemplateFunc_TemplateType(pattern)->u.templ.args,
                                                             argumentClass, state, flag);
                        }
                        if (argument->type == TYPETEMPLATE) {
                            TemplArg *argumentData = CTemplateFunc_TemplateType(argument)->u.templ.args;
                            TemplArg *patternData = CTemplateFunc_TemplateType(pattern)->u.templ.args;
                            return CTemplateFunc_TemplateType(argument)->u.templ.templ !=
                                           CTemplateFunc_TemplateType(pattern)->u.templ.templ
                                       ? (Boolean)0
                                       : match_state_elem_arguments(patternData, argumentData, state, flag);
                        }
                        return 0;
                    case 3:
                        return 0;
                    case 4:
                        return 0;
                    default:
                    case 5:
                        CError_FATAL(808);
                        return 1;
                }
            case TYPEVOID:
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPESTRUCT:
            case TYPECLASS:
                return pattern == argument;
            case TYPEMEMBERPOINTER:
                if (pattern->type != argument->type)
                    return 0;
                if (!CTemplateFunc_MatchType(((TypeMemberPointer *)pattern)->ty1, patternQual,
                                             ((TypeMemberPointer *)argument)->ty1, argumentQual, state, flag))
                    return 0;
                pattern = ((TypeMemberPointer *)pattern)->ty2;
                argument = ((TypeMemberPointer *)argument)->ty2;
                break;
            case TYPEARRAY:
                if (argument->type != TYPEARRAY)
                    return 0;
                pattern = ((TypePointer *)pattern)->target;
                argument = ((TypePointer *)argument)->target;
                break;
            case TYPEPOINTER:
                if (argument->type != TYPEPOINTER || ((TypePointer *)pattern)->qual != ((TypePointer *)argument)->qual)
                    return 0;
                pattern = ((TypePointer *)pattern)->target;
                argument = ((TypePointer *)argument)->target;
                break;
            case TYPEFUNC:
                if (pattern->type != argument->type)
                    return 0;
                if (!CTemplateFunc_MatchType(((TypeFunc *)pattern)->functype, ((TypeFunc *)pattern)->qual,
                                             ((TypeFunc *)argument)->functype, ((TypeFunc *)argument)->qual, state,
                                             flag))
                    return 0;
                {
                    TypeFunc *patternFunc = (TypeFunc *)pattern;
                    TypeFunc *argumentFunc = (TypeFunc *)argument;
                    match_args(patternFunc, argumentFunc, state, flag);
                }
                return;
            default:
                CError_FATAL(854);
                break;
        }
    }
}

Boolean match_args(TypeFunc *a, TypeFunc *b, TemplArg *c, Boolean d)
{
    FuncArg *pa = a->args;
    FuncArg *pb = b->args;

    while (1) {
        if (pa == NULL)
            return pb == NULL;
        if (pa == &data_00584748)
            return pb == &data_00584748;
        if (pa == &data_00583098)
            return pb == &data_00583098;
        if (pb == NULL || pb == &data_00584748 || pb == &data_00583098)
            return 0;
        if (!CTemplateFunc_MatchType(pa->type, pa->qual, pb->type, pb->qual, c, d))
            return 0;
        pa = pa->next;
        pb = pb->next;
    }
}
#define CB_TRUE 1
#define CB_FALSE 0

static Boolean CTemplateFunc_MatchClassArgs(TemplClassInst *cur, TypeClass *target, TemplArg *a2, TemplArg *a4,
                                            Boolean a5)
{
    while (cur != NULL) {
        if (&cur->theclass == target)
            return match_state_elem_arguments(a2, cur->oargs ? cur->oargs : cur->inst_args, a4, a5);
        cur = cur->next;
    }
    return CB_FALSE;
}

Boolean match_class_args_or_bases(TemplClass *classTemplate, TemplArg *templateArgs, TypeClass *candidateClass,
                                  TemplArg *deducedArgs, Boolean matchBases)
{
    ClassList *base;

    if (CTemplateFunc_MatchClassArgs(classTemplate->instances, candidateClass, templateArgs, deducedArgs, matchBases))
        return CB_TRUE;
    if (matchBases) {
        for (base = candidateClass->bases; base != NULL; base = base->next) {
            if (CTemplateFunc_MatchClassArgs(classTemplate->instances, base->base, templateArgs, deducedArgs,
                                             matchBases)) {
                data_005824c8 = 1;
                return CB_TRUE;
            }
        }
        for (base = candidateClass->bases; base != NULL; base = base->next) {
            if (match_class_args_or_bases(classTemplate, templateArgs, base->base, deducedArgs, matchBases)) {
                data_005824c8 = 1;
                return CB_TRUE;
            }
        }
    }
    return CB_FALSE;
}

static SInt32 CTF_GetIndex(TemplArg *a)
{
    if (a->data.paramdecl.expr->type == 'E' && a->data.paramdecl.expr->data.templdep.subtype == TDE_PARAM)
        return a->data.paramdecl.expr->data.templdep.u.pid.index;
    return -1;
}

Boolean match_state_elem_arguments(TemplArg *a, TemplArg *b, TemplArg *r, char flag)
{
    SInt32 idx;

    while (1) {
        if (a == NULL)
            return b == NULL;
        if (b == NULL)
            return 0;
        if (a->pid.type != 0) {
            if (b->pid.type == 0)
                return 0;
            if (!CTemplateFunc_MatchType(a->data.typeparam.type, a->data.typeparam.qual, b->data.typeparam.type,
                                         b->data.typeparam.qual, r, flag))
                return 0;
        } else {
            if (b->pid.type != 0)
                return 0;
            if (CTemplTool_IsTypeDepExpr(a->data.paramdecl.expr) != 0) {
                if (a->data.paramdecl.expr == NULL)
                    CError_FATAL(515);
                idx = CTF_GetIndex(a);
                if (idx < 0)
                    return 0;
                if (r[idx].is_deduced != 0) {
                    if (!CTemplateTools_00517a40(b->data.paramdecl.expr, r[idx].data.paramdecl.expr))
                        return 0;
                } else {
                    r[idx].data.paramdecl.expr = b->data.paramdecl.expr;
                    r[idx].pid.type = 0;
                    r[idx].is_deduced = 1;
                }
            } else {
                if (!CTemplateTools_00517a40(b->data.paramdecl.expr, a->data.paramdecl.expr))
                    return 0;
            }
        }
        a = a->next;
        b = b->next;
    }
}
int CTemplateFunc_GetArgumentParameterIndex(TemplArg *argument)
{
    if (argument->data.paramdecl.expr == NULL)
        CError_FATAL(515);
    if (argument->data.paramdecl.expr->type == 69U && argument->data.paramdecl.expr->data.templdep.subtype == TDE_PARAM)
        return argument->data.paramdecl.expr->data.templdep.u.pid.index;
    return -1;
}

static inline struct TemplFuncInstance *InstantiateAccessibleTemplate(Object *func, DeduceInfo *frame, Object *flags)
{
    SInt32 i = 0;
    while (i < frame->maxCount) {
        if (frame->args[i++].is_deduced == 0)
            return NULL;
    }
    return find_or_create_template_specialization(func, frame->args, flags);
}

Object *select_unique_undominated_match(Object *func, struct ObjectList *funcs, int options)
{
    int count;
    struct ObjectList *link;
    Object *other;
    int i;
    int reverseDoesNotMatch;
    int index;
    int j;
    int dominates;
    Object **candidates;
    Object *localCandidates[16];
    Object *candidate;

    count = 1;

    for (link = funcs; link != NULL; link = link->next) {
        if (link->object->type->type == TYPEFUNC && (TYPE_FUNC(link->object->type)->flags & 0x400) != 0)
            count++;
    }

    if (count > 16)
        candidates = (Object **)CompilerTools_AllocatePool(count * sizeof(Object *));
    else
        candidates = localCandidates;

    candidates[0] = func;

    for (index = 1, link = funcs; link != NULL; link = link->next) {
        if (link->object->type->type == TYPEFUNC && (TYPE_FUNC(link->object->type)->flags & 0x400) != 0)
            candidates[index++] = link->object;
    }

    for (i = 0; i < count; i++) {
        if (candidates[i] == NULL)
            continue;
        for (j = 0; j < count; j++) {
            if ((other = candidates[j]) == NULL)
                continue;
            if (i == j)
                continue;
            candidate = candidates[i];
            dominates = 0;
            if (match_candidate_to_template_args(candidate, other)) {
                reverseDoesNotMatch = 0;
                if (!match_candidate_to_template_args(other, candidate))
                    reverseDoesNotMatch = 1;
                if (reverseDoesNotMatch)
                    dominates = 1;
            }
            if ((Boolean)dominates)
                candidates[j] = NULL;
        }
    }

    func = NULL;
    for (i = 0; i < count; i++) {
        if (candidates[i] != NULL) {
            if (func != NULL)
                return NULL;
            func = candidates[i];
        }
    }
    return func;
}

/* State records passed to the template deduction helpers. */
/* Views of the metadata used during template substitution. */

static inline void templateFunctionAssertion(int line)
{
    CError_Internal("CTemplateFunc.c", line);
}

unsigned char match_candidate_to_template_args(Object *candidate, Object *templ)
{
    FuncArg *templateArg;
    ENodeList *arguments;
    Type *candidateType;
    ENode *expression;
    int remaining;
    ENode *namespaceExpr;
    ENodeList *tail;
    FuncArg *deducedArg;
    FuncArg *candidateArg;
    FuncArg *matchArg;
    Type *matchType;
    ENode *objectExpr;
    TemplateFunction *templateInfo;
    int countRemaining;
    int argumentCount;
    ArgMatch deductionState;
    MemberCallArguments result;
    DeduceInfo bindings;
    TypeDeduce substitution;
    TemplArg *bindingList;
    TypeMemberFunc *templateType;
    TypeMemberFunc *candidateFuncType;

    templateType = (TypeMemberFunc *)templ->type;
    templateArg = templateType->args;
    if ((templateType->flags & FUNC_METHOD) != 0 && !templateType->is_static)
        templateArg = templateArg->next;
    argumentCount = 0;
    for (;;) {
        if (templateArg == NULL)
            break;
        CE_ASSERT(templateArg->type == &stvoid, templateFunctionAssertion(299));
        if (templateArg == &data_00583098 || templateArg == &data_00584748)
            break;
        templateArg = templateArg->next;
        ++argumentCount;
    }
    candidateFuncType = (TypeMemberFunc *)candidate->type;
    candidateArg = candidateFuncType->args;
    if ((candidateFuncType->flags & FUNC_METHOD) != 0 && !candidateFuncType->is_static)
        candidateArg = candidateArg->next;
    arguments = NULL;
    countRemaining = argumentCount;
    while (countRemaining > 0) {
        if (candidateArg == NULL)
            break;
        CE_ASSERT(candidateArg->type == &stvoid, templateFunctionAssertion(313));
        if (candidateArg == &data_00583098 || candidateArg == &data_00584748)
            break;
        objectExpr = nullnode();
        objectExpr->rtype = (Type *)&void_ptr;
        expression = makemonadicnode(objectExpr, EINDIRECT);
        expression->rtype = candidateArg->type;
        expression->flags = candidateArg->qual & ENODE_FLAG_QUALS;
        if (expression->rtype->type == TYPEPOINTER && (((TypePointer *)expression->rtype)->qual & Q_REFERENCE) != 0)
            expression->rtype = ((TypePointer *)expression->rtype)->target;
        if (arguments != NULL) {
            tail->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
            tail = tail->next;
        } else {
            tail = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
            arguments = tail;
        }
        tail->node = expression;
        tail->next = NULL;
        candidateArg = candidateArg->next;
        --countRemaining;
    }
    if (templ->nspace->theclass != NULL) {
        namespaceExpr = nullnode();
        namespaceExpr->rtype = (Type *)templ->nspace->theclass;
    } else {
        namespaceExpr = NULL;
    }
    if (!CExpr_GetFuncMatchArgs(templ, arguments, namespaceExpr, &result))
        return 0;
    memclrw(&deductionState, sizeof(deductionState));
    if (CTemplTool_InitDeduceInfo(&bindings, CTemplTool_GetFuncTempl(templ)->params, NULL, 0) &&
        match_template_function_args(templ, &bindings, result.parameters, result.arguments, &deductionState)) {
        if (deductionState.score2Count != 0 || deductionState.score3Count != 0 || deductionState.score4Count != 0)
            return 0;
        CE_ASSERT(templ->type->type != TYPEFUNC, templateFunctionAssertion(356));
        bindingList = bindings.args;
        templateInfo = CTemplTool_GetFuncTempl(templ);
        memclrw(&substitution, sizeof(substitution));
        substitution.params = templateInfo->params;
        substitution.args = bindingList;
        deducedArg = CTemplateTools_005160b0(&substitution, ((TypeMemberFunc *)templ->type)->args);
        if (((TypeFunc *)templ->type)->flags & FUNC_METHOD) {
            TypeMemberFunc *memberType = (TypeMemberFunc *)templ->type;
            if (!memberType->is_static)
                deducedArg = deducedArg->next;
        }
        candidateFuncType = (TypeMemberFunc *)candidate->type;
        matchArg = candidateFuncType->args;
        if ((candidateFuncType->flags & FUNC_METHOD) != 0 && !candidateFuncType->is_static)
            matchArg = matchArg->next;
        remaining = argumentCount;
        while (remaining > 0) {
            if (matchArg == NULL)
                break;
            if (matchArg == &data_00583098 || matchArg == &data_00584748)
                break;
            CE_ASSERT(deducedArg == 0, templateFunctionAssertion(380));
            candidateType = matchArg->type;
            if (candidateType->type == TYPEPOINTER && (((TypePointer *)candidateType)->qual & Q_REFERENCE) != 0)
                candidateType = ((TypePointer *)candidateType)->target;
            matchType = deducedArg->type;
            if (matchType->type == TYPEPOINTER && (((TypePointer *)matchType)->qual & Q_REFERENCE) != 0)
                matchType = ((TypePointer *)matchType)->target;
            if (!iscpp_typeequal(candidateType, matchType))
                return 0;
            if (matchArg->qual != deducedArg->qual && candidateType->type == TYPEPOINTER)
                return 0;
            --remaining;
            matchArg = matchArg->next;
            deducedArg = deducedArg->next;
        }
        return 1;
    }
    return 0;
}

struct TemplFuncInstance *instantiate_accessible_template_for_type(Object *func, Type *ftype, void *templateArguments,
                                                                   Object *flags, int instantiationMode)
{
    TemplateFunction *functionTemplate;
    DeduceInfo matchState;
    struct TemplFuncInstance *result;

    functionTemplate = CTemplTool_GetFuncTempl(func);
    if (!CTemplTool_InitDeduceInfo(&matchState, functionTemplate->params, templateArguments, 1))
        result = NULL;
    else if (!CTemplateFunc_MatchType(func->type, 0, ftype, 0, matchState.args, 1))
        result = NULL;
    else
        result = InstantiateAccessibleTemplate(func, &matchState, flags);
    return result;
}
struct TemplFuncInstance *CTemplateFunc_FindOrCreateMatchedSpecialization(Object *func, Type *matchedType,
                                                                          void *templateArgs, Object *specialization)
{
    TemplateFunction *functionTemplate;
    DeduceInfo match;
    SInt32 slotIndex;

    functionTemplate = CTemplTool_GetFuncTempl(func);
    if (!CTemplTool_InitDeduceInfo(&match, functionTemplate->params, templateArgs, 1))
        return NULL;
    if (!CTemplateFunc_MatchType(func->type, 0, matchedType, 0, match.args, 1))
        return NULL;
    slotIndex = 0;
    while (slotIndex < match.maxCount) {
        if (match.args[slotIndex++].is_deduced == 0)
            return NULL;
    }
    return find_or_create_template_specialization(func, match.args, specialization);
}
struct TemplFuncInstance *find_or_create_template_specialization(Object *func, TemplArg *args, Object *premade)
{
    TemplateFunction *info;
    SInt16 argumentCount;
    TemplParam *parameter;
    TemplArg *argument;
    TemplArg *firstArgument;
    TemplArg *lastArgument;
    SInt16 argumentIndex;
    struct TemplFuncInstance *instance;
    struct TemplFuncInstance *existing;
    SInt16 linkIndex;
    TemplArg *instanceArgs;
    Object *object;
    TypeDeduce local;

    info = CTemplTool_GetFuncTempl(func);
    argumentCount = 0;
    for (parameter = info->params; parameter != NULL; parameter = parameter->next)
        argumentCount++;

    linkIndex = 1;
    argument = args;
    if (linkIndex < argumentCount) {
        do {
            argument->next = argument + 1;
            argument++;
        } while (++linkIndex < argumentCount);
    }
    argument->next = NULL;

    existing = info->instances;
    while (existing != NULL) {
        if (CTemplTool_EqualArgs(existing->args, args)) {
            if (premade != NULL)
                existing->object = premade;
            return existing;
        }
        existing = existing->next;
    }

    instance = galloc(sizeof(*instance));
    memclrw(instance, sizeof(*instance));
    instance->next = info->instances;
    info->instances = instance;

    argumentIndex = 0;
    firstArgument = NULL;
    if (argumentCount > 0) {
        do {
            if (firstArgument != NULL) {
                lastArgument->next = (TemplArg *)galloc(sizeof(*lastArgument));
                lastArgument = lastArgument->next;
            } else {
                lastArgument = (TemplArg *)galloc(sizeof(*lastArgument));
                firstArgument = lastArgument;
            }
            *lastArgument = *args;
            args++;
            lastArgument->next = NULL;
            if (lastArgument->pid.type == 0) {
                if (lastArgument->data.typeparam.type == NULL)
                    CError_FATAL(106);
                lastArgument->data.paramdecl.expr = fn_00513040(lastArgument->data.paramdecl.expr, 1);
            }
            argumentIndex++;
        } while (argumentIndex < argumentCount);
    }
    instance->args = firstArgument;

    if (premade == NULL) {
        instanceArgs = instance->args;
        memclrw(&local, sizeof(local));
        local.params = info->params;
        local.args = instanceArgs;

        if (func->nspace->theclass != NULL && (func->nspace->theclass->flags & CLASS_IS_TEMPL_INST) != 0) {
            local.tmclass = ((TemplClassInst *)func->nspace->theclass)->templ;
            local.inst = (TemplClassInst *)func->nspace->theclass;
        }

        object = CParser_NewFunctionObject(NULL);
        object->nspace = func->nspace;
        object->qual = func->qual | Q_MANGLE_NAME;
        object->name = CMangler_TemplateInstanceName(info->name, instance->args);
        object->type = CTemplateTools_ResolveType(&local, func->type, &object->qual);
        if (object->type->type == TYPEFUNC) {
            ((TypeFunc *)object->type)->flags &= ~FUNC_DEFINED;
            ((TypeFunc *)object->type)->flags |= 0x800;
        }
        object->sclass = func->sclass;
        instance->object = object;
        if ((object->qual & Q_INLINE) != 0 && info->stream.tokens != 0)
            CInline_AddSpecialization(object, info, instance);
    } else {
        instance->object = premade;
    }
    return instance;
}
