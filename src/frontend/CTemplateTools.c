#define CERROR_FILE "CTemplateTools.c"
#include "compiler/common.h"
#include "compiler/CTemplateTools.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>

#include "compiler/Types.h"

typedef struct TemplateComparisonEntry TemplateComparisonEntry;

Boolean CTemplateTools_MatchTypeAndCheckBoundSlots(Object *obj, Type *type, void *templateArgs)
{
    DeduceInfo matchState;
    SInt32 slotIndex;

    if (CTemplTool_InitDeduceInfo(&matchState, CTemplTool_GetFuncTempl(obj)->params, (TemplArg *)templateArgs, 0)) {
        if (CTemplateFunc_MatchType(obj->type, 0, type, 0, matchState.args, 1)) {
            for (slotIndex = 0; slotIndex < matchState.maxCount; slotIndex++) {
                if (matchState.args[slotIndex].is_deduced == 0)
                    return 0;
            }
            return 1;
        }
    }
    return 0;
}

Boolean CTemplateTools_IsTemplDepClassBase(TypeClass *type, TypeTemplDep *templateType)
{
    TemplClass *foundClass;

    foundClass = CTemplTool_IsTemplate(templateType);
    return &foundClass->theclass == type;
}

Type *CTemplTool_ResolveMemberSelfRefs(TemplClass *tclass, Type *type, UInt32 *qualifiers)
{
    TypeDeduce state;
    memclrw(&state, sizeof(state));
    state.tmclass = tclass;
    state.processingClassTypes = 1;
    if (type->type == TYPEFUNC) {
        TypeFunc *function = (TypeFunc *)type;
        function->functype = CTemplateTools_ResolveType(&state, function->functype, &function->qual);
        function->args = CTemplateTools_005160b0(&state, function->args);
        fn_00504240(function);
        type = (Type *)function;
    } else {
        type = CTemplateTools_ResolveType(&state, type, qualifiers);
    }
    return type;
}

Type *CTemplateTools_ResolveType(TypeDeduce *ctx, Type *type, UInt32 *qual)
{
    UInt32 functionQual;
    switch ((SInt8)type->type) {
        case TYPETEMPLATE: {
            UInt32 templateQual = 0;
            TypeTemplDep *dependentType = (TypeTemplDep *)type;
            Type *result = resolve_templ_dep_type(ctx, dependentType, &templateQual);
            if ((*qual & (Q_CONST | Q_VOLATILE)) != 0 && result->type == TYPEPOINTER) {
                TypePointer *qualifiedType = (TypePointer *)galloc(sizeof(TypePointer));
                *qualifiedType = *(TypePointer *)result;
                qualifiedType->qual |= *qual & (Q_CONST | Q_VOLATILE);
                *qual &= ~(Q_CONST | Q_VOLATILE);
                result = (Type *)qualifiedType;
            }
            *qual |= templateQual;
            return result;
        }
        case TYPEVOID:
        case TYPEINT:
        case TYPEFLOAT:
        case TYPESTRUCT:
            return type;
        case TYPEENUM: {
            TemplClassInst *theclass;
            if (((TypeEnum *)type)->nspace->theclass != NULL &&
                (((TypeEnum *)type)->nspace->theclass->flags & Q_VIRTUAL) != 0 && ctx->processingClassTypes == 0) {
                CError_ASSERT(2103, ((TypeEnum *)type)->enumname);
                theclass = find_corresponding_instance_class(ctx, ((TypeEnum *)type)->nspace->theclass);
                CError_ASSERT(2105, theclass != NULL && theclass->theclass.type == TYPECLASS);
                type = CScope_GetTagType(theclass->theclass.nspace, ((TypeEnum *)type)->enumname);
                CError_ASSERT(2108, type != NULL);
                return type;
            }
            return type;
        }
        case TYPECLASS: {
            if (ctx->processingClassTypes == 0) {
                if ((((TypeClass *)type)->flags & Q_VIRTUAL) != 0) {
                    TemplClassInst *resolvedClass = find_corresponding_instance_class(ctx, (TypeClass *)type);
                    return (Type *)resolvedClass;
                }
                if (((TypeClass *)type)->nspace->theclass != NULL &&
                    (((TypeClass *)type)->nspace->theclass->flags & Q_VIRTUAL) != 0) {
                    Type *result;
                    CError_ASSERT(2123, ctx->inst);
                    CError_ASSERT(2124, ((TypeClass *)type)->classname);
                    result = CScope_GetTagType(TYPE_CLASS(ctx->inst)->nspace, ((TypeClass *)type)->classname);
                    CError_ASSERT(2127, result != NULL);
                    return result;
                }
            }
            return type;
        }
        case TYPEARRAY: {
            TypePointer *array = (TypePointer *)galloc(sizeof(TypePointer));
            *array = *(TypePointer *)type;
            array->target = CTemplateTools_ResolveType(ctx, ((TypePointer *)type)->target, qual);
            do {
                type = ((TypePointer *)type)->target;
            } while (type->type == TYPEARRAY);
            if (type->type == TYPETEMPLATE) {
                CDecl_CompleteType(array->target);
                array->size = array->target->size * array->size;
            }
            return (Type *)array;
        }
        case TYPEPOINTER: {
            TypePointer *newPointer = (TypePointer *)galloc(sizeof(TypePointer));
            *newPointer = *(TypePointer *)type;
            newPointer->target = CTemplateTools_ResolveType(ctx, ((TypePointer *)type)->target, qual);
            return (Type *)newPointer;
        }
        case TYPEBITFIELD: {
            TypePointer *bitfield = (TypePointer *)galloc(sizeof(TypePointer));
            *bitfield = *(TypePointer *)type;
            bitfield->target = CTemplateTools_ResolveType(ctx, ((TypePointer *)type)->target, qual);
            return (Type *)bitfield;
        }
        case TYPEMEMBERPOINTER: {
            TypeMemberPointer *memberPointer = (TypeMemberPointer *)galloc(sizeof(TypeMemberPointer));
            TypeMemberPointer *original = (TypeMemberPointer *)type;
            *memberPointer = *original;
            memberPointer->memberType = CTemplateTools_ResolveType(ctx, original->memberType, qual);
            memberPointer->owner.type = CTemplateTools_ResolveType(ctx, original->owner.type, qual);
            if (memberPointer->owner.type->type != TYPECLASS && ctx->processingClassTypes == 0 &&
                ctx->processingArgument == 0) {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
                return memberPointer->memberType;
            }
            return (Type *)memberPointer;
        }
        case TYPEFUNC: {
            TypeFunc *function;
            if (((TypeFunc *)type)->flags & FUNC_METHOD) {
                TypeMemberFunc *method;
                Type *methodClass;
                Type *resolvedMethodClass;
                functionQual = 0;
                method = (TypeMemberFunc *)galloc(sizeof(TypeMemberFunc));
                *method = *(TypeMemberFunc *)type;
                method->funcid = 0;
                methodClass = (Type *)((TypeMemberFunc *)type)->theclass;
                resolvedMethodClass = resolve_templ_dep_pointer_target(ctx, methodClass, &functionQual);
                method->theclass = (TypeClass *)resolvedMethodClass;
                CError_ASSERT(2179, method->theclass->type == TYPECLASS);
                function = (TypeFunc *)method;
            } else {
                function = (TypeFunc *)galloc(sizeof(TypeFunc));
                *function = *(TypeFunc *)type;
            }
            function->flags &= ~0x400;
            functionQual = ((TypeFunc *)type)->qual;
            function->functype = resolve_templ_dep_pointer_target(ctx, ((TypeFunc *)type)->functype, &functionQual);
            function->qual = functionQual;
            function->args = CTemplateTools_005160b0(ctx, ((TypeFunc *)type)->args);
            if (((TypeFunc *)type)->exspecs != NULL) {
                function->exspecs = copy_resolved_except_spec_list(ctx, ((TypeFunc *)type)->exspecs);
            }
            fn_00504240(function);
            return (Type *)function;
        }
        default:
            CError_FATAL(2199);
            return NULL;
    }
}

ExceptSpecList *copy_resolved_except_spec_list(void *ctx, ExceptSpecList *n)
{
    ExceptSpecList *c = (ExceptSpecList *)galloc(0xc);

    *c = *n;
    if (c->type != NULL) {
        if (CTemplateTools_IsDependentType(c->type)) {
            c->type = CTemplateTools_ResolveType(ctx, c->type, &c->qual);
        }
    }
    if (c->next != NULL) {
        c->next = copy_resolved_except_spec_list(ctx, c->next);
    }
    return c;
}

static Boolean CTT_IsNeg(CInt64 *v)
{
    return (v->hi & 0x80000000) != 0;
}

FuncArg *CTemplateTools_005160b0(TypeDeduce *ctx, FuncArg *args)
{
    FuncArg *newlist;
    Boolean isDependentExpression;
    ENode *expression;
    FuncArg *tail;

    if (args == &data_00584748 || args == &data_00583098)
        return args;
    newlist = NULL;
    while (args != NULL) {
        if (args == &data_00583098) {
            tail->next = args;
            break;
        }
        if (newlist != NULL) {
            tail->next = (FuncArg *)galloc(sizeof(FuncArg));
            tail = tail->next;
        } else {
            tail = (FuncArg *)galloc(sizeof(FuncArg));
            newlist = tail;
        }
        *tail = *args;
        tail->type = resolve_templ_dep_pointer_target(ctx, tail->type, &tail->qual);
        if (ctx->processingClassTypes == 0 && (copts.f96 == 0 || ctx->processingArgument == 0)) {
            expression = tail->dexpr;
            if (expression == NULL)
                isDependentExpression = 0;
            else
                isDependentExpression = (expression->rtype->type == TYPETEMPLDEPEXPR);
            if (isDependentExpression) {
                tail->dexpr =
                    CExpr_AssignmentPromotion(CTemplTool_DeduceExpr(ctx, tail->dexpr), tail->type, tail->qual, 0);
                tail->dexpr = fn_00513040(tail->dexpr, 1);
            }
        }
        args = args->next;
    }
    return newlist;
}

static Boolean IsTemplDep(ENode *e)
{
    if (e == NULL)
        return 0;
    return e->rtype->type == TYPETEMPLDEPEXPR;
}

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

static inline TemplArg *find(TemplArg *e, TemplParamID pid)
{
    for (; e; e = e->next) {
        if (e->pid.index == pid.index && e->pid.nindex == pid.nindex) {
            if (pid.type != e->pid.type)
                CError_FATAL(1654);
            return e;
        }
    }
    return NULL;
}

#define NP(nd) (nd)

static ENode *CloneNode(ENode *src)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *n = *src;
    return n;
}

static ENode *CloneRecNode(TemplArg *r)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *n = *r->data.paramdecl.expr;
    return n;
}

static FuncArg *CTemplateTools_FirstArg(TypeMemberFunc *f)
{
    FuncArg *a = f->args;

    if ((f->flags & FUNC_METHOD) && !f->is_static) {
        CError_ASSERT(374, a != NULL);
        a = a->next;
        if ((f->flags & FUNC_IS_DTOR) && (f->theclass->flags & CLASS_HAS_VBASES)) {
            CError_ASSERT(379, a != NULL);
            a = a->next;
        }
    }
    return a;
}

static void CTemplateTools_SetFirstArgName(TypeMemberFunc *f)
{
    FuncArg *a;

    if ((f->flags & FUNC_METHOD) && !f->is_static) {
        CError_ASSERT(410, (a = f->args) != NULL);
        if (a->name == NULL)
            a->name = this_arg_name;
    }
}

Type *resolve_templ_dep_pointer_target(TypeDeduce *context, Type *typeArg, UInt32 *qualifiers)
{
    Type *type = typeArg;
    UInt32 originalQualifiers;
    Type *resolvedType;
    TypePointer *pointerType;
    UInt32 targetQualifiers;

    originalQualifiers = *qualifiers;
    if ((type->type == TYPEPOINTER) && (((TypePointer *)type)->target->type == TYPETEMPLATE)) {
        TypeTemplDep *dependentType;
        targetQualifiers = 0;
        dependentType = (TypeTemplDep *)((TypePointer *)type)->target;
        resolvedType = resolve_templ_dep_type(context, dependentType, &targetQualifiers);
        pointerType = (TypePointer *)galloc(sizeof(*pointerType));
        *pointerType = *(TypePointer *)type;
        if (resolvedType->type == TYPEPOINTER) {
            pointerType->target = (Type *)galloc(sizeof(*pointerType));
            *(TypePointer *)pointerType->target = *(TypePointer *)resolvedType;
            *qualifiers = targetQualifiers & 3;
            ((TypePointer *)pointerType->target)->qual |= originalQualifiers & 3;
        } else {
            pointerType->target = resolvedType;
            *qualifiers = (originalQualifiers | targetQualifiers) & 3;
        }
        return (Type *)pointerType;
    }
    resolvedType = CTemplateTools_ResolveType(context, type, qualifiers);
    return resolvedType;
}

Type *resolve_templ_dep_type(TypeDeduce *ctx, TypeTemplDep *arg, UInt32 *out)
{
    UInt32 resolutionData = 0;
    Type *type;
    Type *result;
    TypeTemplDep *copy;
    TemplArg *item;
    TemplClass *resolvedClass;

    if (ctx->processingClassTypes != 0) {
        resolvedClass = CTemplTool_IsTemplate(arg);
        if (resolvedClass != NULL && resolvedClass == ctx->tmclass)
            return (Type *)resolvedClass;
        switch (arg->kind) {
            case 0:
                return (Type *)arg;
            case 1:
                type = CTemplateTools_ResolveType(ctx, (Type *)arg->u.qual.type, &resolutionData);
                if (type == (Type *)arg->u.qual.type)
                    return (Type *)arg;
                if (type->type != TYPECLASS) {
                    CError_ASSERT(1826, type->type == TYPETEMPLATE);
                    copy = galloc(sizeof(*copy));
                    *copy = *arg;
                    copy->u.qual.type = (TypeTemplDep *)type;
                    return (Type *)copy;
                }
                CError_ASSERT(1832, type == (Type *)ctx->tmclass);
                result = CScope_GetType(TYPE_CLASS(type)->nspace, arg->u.qual.name, out);
                if (result != NULL)
                    return result;
                CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, arg->u.qual.name->name);
                return (Type *)arg;
            case 2:
                for (item = arg->u.templ.args; item != NULL; item = item->next) {
                    if (item->pid.type > 0)
                        item->data.typeparam.type = CTemplateTools_ResolveType(ctx, item->data.typeparam.type,
                                                                               (UInt32 *)&item->data.typeparam.qual);
                }
                return (Type *)arg;
            case 3:
                arg->u.array.type = CTemplateTools_ResolveType(ctx, arg->u.array.type, &resolutionData);
                return (Type *)arg;
            case 4:
                arg->u.qualtempl.type =
                    (TypeTemplDep *)resolve_templ_dep_type(ctx, arg->u.qualtempl.type, &resolutionData);
                for (item = arg->u.qualtempl.args; item != NULL; item = item->next) {
                    if (item->pid.type > 0)
                        item->data.typeparam.type = CTemplateTools_ResolveType(ctx, item->data.typeparam.type,
                                                                               (UInt32 *)&item->data.typeparam.qual);
                }
                return (Type *)arg;
            case 5:
                arg->u.bitfield.type = CTemplateTools_ResolveType(ctx, arg->u.bitfield.type, &resolutionData);
                return (Type *)arg;
        }
    } else {
        switch (arg->kind) {
            case 0:
                if (ctx->processingArgument != 0 && arg->u.pid.nindex == ctx->nindex)
                    return (Type *)arg;
                item = find_template_argument(ctx, arg->u.pid);
                CError_ASSERT(1887, item->pid.type != 0);
                *out = item->data.typeparam.qual;
                return item->data.typeparam.type;
            case 1:
                type = CTemplateTools_ResolveType(ctx, (Type *)arg->u.qual.type, &resolutionData);
                if (type->type == TYPECLASS) {
                    CDecl_CompleteType(type);
                    result = CScope_GetType(TYPE_CLASS(type)->nspace, arg->u.qual.name, out);
                    if (result != NULL)
                        return result;
                    CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, arg->u.qual.name->name);
                } else {
                    if ((ctx->processingArgument != 0 || ctx->inst == NULL) && type->type == TYPETEMPLATE) {
                        copy = galloc(sizeof(*copy));
                        *copy = *arg;
                        copy->u.qual.type = (TypeTemplDep *)type;
                        return (Type *)copy;
                    }
                    CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE, arg->u.qual.name->name);
                }
                return (Type *)&stsignedint;
            case 2:
                CTemplateTools_00516930(ctx, arg->u.templ.templ, arg->u.templ.args);
                return;
            case 3: {
                ENode *sizeExpression = arg->u.array.index;
                Type *base = arg->u.array.type;
                ENode *resolvedSize;

                if (CTemplateTools_IsDependentType(base)) {
                    UInt32 baseResolutionData = 0;
                    base = CTemplateTools_ResolveType(ctx, base, &baseResolutionData);
                }
                resolvedSize = CTemplTool_DeduceExpr(ctx, sizeExpression);
                if (resolvedSize->type == EINTCONST) {
                    Boolean negativeSize = (resolvedSize->data.intval.hi & 0x80000000) != 0;
                    if (negativeSize) {
                        CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                        resolvedSize->data.intval = cint64_one;
                    }
                    if (!CDecl_CheckArrayIntegr(base))
                        base = (Type *)&stsignedchar;
                    base = CDecl_NewArrayType(base, base->size * (SInt32)resolvedSize->data.intval.lo);
                } else {
                    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                }
                return base;
            }
            case 4:
                resolvedClass =
                    (TemplClass *)CTemplateTools_ResolveType(ctx, (Type *)arg->u.qualtempl.type, &resolutionData);
                if (resolvedClass->theclass.type != TYPECLASS ||
                    (resolvedClass->theclass.flags & CLASS_IS_TEMPL) == 0) {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    return (Type *)&stsignedint;
                }
                CTemplateTools_00516930(ctx, resolvedClass, arg->u.qualtempl.args);
                return;
            case 5:
                return make_bitfield_type(ctx, arg->u.bitfield.type, arg->u.bitfield.size, out);
        }
    }
    CError_FATAL(1936);
    return NULL;
}

Type *make_bitfield_type(TypeDeduce *ctx, Type *ty, ENode *node, UInt32 *out)
{
    UInt32 n;
    SInt16 bits;
    SInt32 width;
    TypeBitfield *tb;

    if (CTemplateTools_IsDependentType(ty)) {
        n = 0;
        ty = CTemplateTools_ResolveType(ctx, ty, &n);
    }
    if (ty->type != TYPEINT && ty->type != TYPEENUM) {
        CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
        ty = (Type *)&stunsignedint;
    }
    switch (ty->size) {
        case 1:
            bits = 8;
            break;
        case 2:
            bits = 16;
            break;
        case 4:
            bits = 32;
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
            return ty;
    }
    if (node->type != EINTCONST) {
        node = CTemplTool_DeduceExpr(ctx, node);
        if (node->type != EINTCONST) {
            CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
            return ty;
        }
    }
    width = node->data.intval.lo;
    if ((SInt16)width > bits || CTT_IsNeg(&node->data.intval)) {
        CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
        width = 1;
    }
    tb = (TypeBitfield *)galloc(sizeof(TypeBitfield));
    memclrw(tb, sizeof(TypeBitfield));
    tb->type = TYPEBITFIELD;
    tb->size = ty->size;
    tb->bitfieldtype = ty;
    tb->bitlength = (char)width;
    return (Type *)tb;
}

TemplArg *find_template_argument(struct TypeDeduce *context, struct TemplParamID pid)
{
    TemplArg *entry;
    TemplClass *currentClass;
    TemplClass *originalClass;

    if ((entry = find(context->args, pid)))
        return entry;
    CE_ASSERT((originalClass = context->tmclass) == 0, CError_FATAL(1678));
    if (!(currentClass = (TemplClass *)context->inst)) {
        CE_ASSERT(originalClass->templ_parent == 0 || originalClass->inst_parent == 0, CError_FATAL(1681));
        originalClass = context->tmclass;
        currentClass = (TemplClass *)originalClass->inst_parent;
    }
    for (;;) {
        if ((entry = find((TemplArg *)currentClass->templ__params, pid)))
            return entry;
        if (!(currentClass = currentClass->templ_parent))
            CError_FATAL(1692);
    }
}

void CTemplateTools_00516930(void *context, TemplClass *function, TemplArg *arguments)
{
    TemplArg *source;
    TemplArg *head;
    TemplArg *tail;
    TemplParam *parameter;

    if (function->templ_parent != NULL)
        function = fn_00516b50(context, function);
    source = arguments;
    head = NULL;
    parameter = function->templ__params;

    if (source != NULL) {
        do {
            if (head != NULL) {
                tail->next = (TemplArg *)galloc(sizeof(TemplArg));
                tail = tail->next;
            } else {
                tail = (TemplArg *)galloc(sizeof(TemplArg));
                head = tail;
            }
            *tail = *source;
            CError_ASSERT(1553, parameter != 0 && parameter->pid.type == tail->pid.type);
            tail->pid = parameter->pid;
            parameter = parameter->next;
            if (tail->pid.type != 0) {
                tail->data.typeparam.type = CTemplateTools_ResolveType((TypeDeduce *)context, tail->data.typeparam.type,
                                                                       (UInt32 *)&tail->data.typeparam.qual);
            } else if (tail->data.paramdecl.expr == NULL) {
                CError_FATAL(1566);
            } else if (IsTemplDep(tail->data.paramdecl.expr)) {
                tail->data.paramdecl.expr =
                    CExpr_GeneratePointerAndRewriteConst(CTemplTool_DeduceExpr(context, tail->data.paramdecl.expr));
                tail->data.paramdecl.expr = fn_00513040(tail->data.paramdecl.expr, 1);
            }
            source = source->next;
        } while (source != NULL);
    }

    {
        TemplArg *argument = head;
        while (argument != NULL) {
            if (argument->pid.type != 0) {
                if (CTemplateTools_IsDependentType(argument->data.typeparam.type))
                    break;
            } else {
                if (IsTemplDep(argument->data.paramdecl.expr))
                    break;
                switch (argument->data.paramdecl.expr->type) {
                    case EOBJREF:
                        if (CParser_HasInternalLinkage(argument->data.paramdecl.expr->data.objref))
                            CError_ReportError(ERR_TEMPLATE_NON_TYPE_ARGUMENT_OBJECTS_SHALL);
                        break;
                    case EINTCONST:
                    case ENEWEXCEPTION:
                        break;
                    default:
                        CError_ReportError(ERR_ILLEGAL_NON_TYPE_TEMPLATE_ARGUMENT);
                        argument->data.paramdecl.expr = nullnode();
                }
            }
            argument = argument->next;
        }
        if (argument != NULL) {
            TypeTemplDep *result = CDecl_NewTemplDepType(2);
            result->u.templ.templ = function;
            result->u.templ.args = head;
            return;
        }
    }

    if (CTemplTool_IsDependentTemplate(function, head) != NULL)
        return;
    CTemplateClass_GetInstance(function, head, NULL);
}

TemplClass *fn_00516b50(TemplateLookupContext *context, TemplClass *record)
{
    TemplClass *result;
    TemplClass *owner;
    TemplClass *scope;

    if (record->inst_parent != NULL) {
        return record;
    }
    if (((owner = context->owner) == NULL) || ((scope = context->currentClass) == NULL)) {
        CError_FATAL(1472);
    }
    for (;;) {
        if (record->templ_parent == owner) {
            result = (TemplClass *)CScope_GetTagType(scope->theclass.nspace, record->theclass.classname);
            if ((((result != NULL) && (result->theclass.type == TYPECLASS)) &&
                 ((result->theclass.flags & CLASS_IS_TEMPL) != 0)) &&
                (result->templ_parent == record->templ_parent)) {
                return result;
            }
        }
        owner = owner->templ_parent;
        if (owner == NULL) {
            break;
        }
        scope = scope->templ_parent;
        if (scope == NULL) {
            CError_FATAL(1486);
        }
    }
    return record;
}

/* Result storage used by template member lookup and expression conversion. */
/* Scope lookup entry, linking a name lookup to its object. */

ENode *CTemplTool_DeduceExpr(TypeDeduce *ctx, ENode *node)
{
    ObjectList *entry;
    TemplArg *outerRecord;
    ENodeList *argument;
    ENode *result;
    Boolean dependent;
    ENodeList *arguments;
    ENodeList *sourceArgument;
    ENode *function;
    TemplArg *record;
    TemplClassInst *group;
    Type *type;
    TypeClass *classType;
    TemplClassInst *resolvedObject;
    CScopeParseResult expression;
    int savedScope;
    UInt32 qualifiers;
    ENodeList *callArguments;
    ENodeList *sourceCallArgument;
    ENodeList *callArgument;

    if (node == NULL) {
        dependent = 0;
    } else {
        dependent = node->rtype->type == TYPETEMPLDEPEXPR;
    }
    if (!dependent) {
        return CloneNode(node);
    }

    switch (node->type) {
        case EOBJLIST:
            switch (node->data.templatecomparison.tag) {
                case 0:
                    if (ctx->processingArgument != 0 &&
                        node->data.templatecomparison.u.wb.templateLevel == ctx->nindex) {
                        return CloneNode(node);
                    }
                    for (record = ctx->args; record != NULL; record = record->next) {
                        if (record->pid.index == node->data.templatecomparison.u.wb.parameterIndex &&
                            record->pid.nindex == node->data.templatecomparison.u.wb.templateLevel) {
                            CError_ASSERT(1310, record->pid.type == 0 && record->data.typeparam.type != NULL);
                            return CloneRecNode(record);
                        }
                    }
                    for (group = ctx->inst; group != NULL; group = group->parent) {
                        for (outerRecord = group->inst_args; outerRecord != NULL; outerRecord = outerRecord->next) {
                            if (outerRecord->pid.index == node->data.templatecomparison.u.wb.parameterIndex &&
                                outerRecord->pid.nindex == node->data.templatecomparison.u.wb.templateLevel) {
                                CError_ASSERT(1323,
                                              outerRecord->pid.type == 0 && outerRecord->data.typeparam.type != NULL);
                                return CloneRecNode(outerRecord);
                            }
                        }
                    }
                    CError_FATAL(1330);
                case 1:
                    qualifiers = 0;
                    type = CTemplateTools_ResolveType(ctx, (Type *)NP(node)->data.templatecomparison.u.p0, &qualifiers);
                    CDecl_CompleteType(type);
                    return intconstnode(CABI_GetSizeTType(), type->size);
                case 3:
                    qualifiers = node->data.templatecomparison.qualifiers;
                    type = CTemplateTools_ResolveType(ctx, (Type *)NP(node)->data.templatecomparison.p4, &qualifiers);
                    for (sourceArgument = (ENodeList *)NP(node)->data.templatecomparison.u.p0, arguments = NULL;
                         sourceArgument != NULL; sourceArgument = sourceArgument->next) {
                        if (arguments != NULL) {
                            argument->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                            argument = argument->next;
                        } else {
                            argument = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                            arguments = argument;
                        }
                        *argument = *sourceArgument;
                        argument->node = CTemplTool_DeduceExpr(ctx, argument->node);
                    }
                    return CExpr_DoExplicitConversion(type, qualifiers, arguments);
                case 4:
                    qualifiers = 0;
                    type = CTemplateTools_ResolveType(ctx, (Type *)NP(node)->data.templatecomparison.u.p0, &qualifiers);
                    if (type->type == TYPECLASS) {
                        CDecl_CompleteType(type);
                        classType = (TypeClass *)type;
                        if (CScope_FindQualifiedClassMember(&expression, classType,
                                                            (HashNameNode *)NP(node)->data.templatecomparison.p4)) {
                            return CExpr_GeneratePointerAndRewriteConst(CExpr_MakeNameLookupResultExpr(&expression));
                        }
                        CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER,
                                           ((HashNameNode *)NP(node)->data.templatecomparison.p4)->name);
                    } else {
                        if (type->type == TYPETEMPLATE && ctx->inst == NULL) {
                            result = CloneNode(node);
                            NP(result)->data.templatecomparison.u.p0 = type;
                            return result;
                        }
                        CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE,
                                           ((HashNameNode *)NP(node)->data.templatecomparison.p4)->name);
                    }
                    return nullnode();
                case 5:
                    resolvedObject = find_corresponding_instance_class(
                        ctx, ((TypeClass *)NP(node)->data.templatecomparison.u.p0)->nspace->theclass);
                    if (resolvedObject == NULL || resolvedObject->theclass.type != TYPECLASS) {
                        CError_FATAL(1379);
                    }
                    entry = CScope_FindObjectListInNameSpace(
                        resolvedObject->theclass.nspace,
                        ((TypeClass *)NP(node)->data.templatecomparison.u.p0)->classname);
                    if (entry == NULL) {
                        CError_FATAL(1382);
                    }
                    memclrw(&expression, sizeof(expression));
                    expression.object = (ObjBase *)entry->object.value;
                    return CExpr_GeneratePointerAndRewriteConst(CExpr_MakeNameLookupResultExpr(&expression));
                case 6:
                    CError_SaveAndSetWrittenEntry((TStreamElement *)NP(node)->data.templatecomparison.p4, &savedScope);
                    result = CTemplTool_DeduceExpr(ctx, (ENode *)NP(node)->data.templatecomparison.u.p0);
                    CError_SetWrittenEntry(&savedScope);
                    return result;
                default:
                    CError_FATAL(1394);
            }
        case EFUNCCALL:
            function = CExpr_PointerGeneration(CTemplTool_DeduceExpr(ctx, node->data.funccall.funcref));
            for (sourceCallArgument = node->data.funccall.args, callArguments = NULL; sourceCallArgument != NULL;
                 sourceCallArgument = sourceCallArgument->next) {
                if (callArguments != NULL) {
                    callArgument->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                    callArgument = callArgument->next;
                } else {
                    callArgument = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                    callArguments = callArgument;
                }
                *callArgument = *sourceCallArgument;
                callArgument->node = CTemplTool_DeduceExpr(ctx, callArgument->node);
            }
            return CExpr_MakeFunctionCall(function, callArguments);
        case ELOGNOT:
            return CExpr_New_ELOGNOT_Node(CTemplTool_DeduceExpr(ctx, node->data.monadic));
        case EMONMIN:
            return CExpr_New_EMONMIN_Node(CTemplTool_DeduceExpr(ctx, node->data.monadic));
        case EBINNOT:
            return CExpr_New_EBINNOT_Node(CTemplTool_DeduceExpr(ctx, node->data.monadic));
        case EMUL:
        case EDIV:
        case EMODULO:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
        case ELAND:
        case ELOR:
            return CExpr_NewDyadicNode(CTemplTool_DeduceExpr(ctx, node->data.diadic.left), node->type,
                                       CTemplTool_DeduceExpr(ctx, node->data.diadic.right));
        case ECOND:
            return CExpr_New_ECOND_Node(CTemplTool_DeduceExpr(ctx, node->data.cond.cond),
                                        CTemplTool_DeduceExpr(ctx, node->data.cond.expr1),
                                        CTemplTool_DeduceExpr(ctx, node->data.cond.expr2));
        default:
            CError_FATAL(1448);
        case EINTCONST:
            return CloneNode(node);
    }
}

TemplClassInst *find_corresponding_instance_class(TypeDeduce *list, TypeClass *targetClass)
{
    TemplClassInst *cur;
    TemplClassInst *curq;
    TemplClassInst *p;
    TemplClassInst *q;

    if ((p = ((TemplClassInst *)list->tmclass)) == NULL)
        CError_FATAL(1170);
    if ((q = (list->inst)) == NULL) {
        if (!(p->parent != NULL && p->templ != NULL))
            CError_FATAL(1173);
        p = p->parent;
        q = (TemplClassInst *)((TemplClassInst *)list->tmclass)->templ;
    }
    cur = p;
    curq = q;
    while (1) {
        if (&cur->theclass == targetClass)
            return curq;
        cur = cur->parent;
        if (cur == NULL)
            break;
        curq = curq->parent;
        if (!(curq != NULL))
            CError_FATAL(1186);
    }
    cur = (TemplClassInst *)targetClass;
    if (cur->inst_args == NULL) {
        for (;;) {
            if (p->inst_args != NULL)
                break;
            p = p->parent;
            if (!(p != NULL))
                CError_FATAL(1195);
            q = q->parent;
            if (!(q != NULL))
                CError_FATAL(1196);
        }
        cur = (TemplClassInst *)targetClass;
        cur = cur->parent;
        while (cur != NULL) {
            if (cur == p)
                return fn_00517270(targetClass, p, q);
            if (cur->inst_args != NULL)
                break;
            cur = cur->parent;
        }
    }
    CError_FATAL(1225);
    return NULL;
}

TemplClassInst *fn_00517270(TypeClass *current, TemplClassInst *limit, TemplClassInst *target)
{
    int depth;
    TemplClassInst *match;
    TypeClass *ancestors[32];
    TemplClassInst *record;

    depth = 0;
    ancestors[0] = current;
    while (1) {
        if (32 <= depth) {
            CError_FATAL(1134);
        }
        record = (TemplClassInst *)current;
        current = (TypeClass *)record->parent;
        if (current == NULL) {
            CError_FATAL(1135);
        }
        if (current == &limit->theclass)
            break;
        record = (TemplClassInst *)current;
        if (record->inst_args != NULL) {
            CError_FATAL(1137);
        }
        depth = depth + 1;
        ancestors[depth] = current;
    }
    do {
        record = (TemplClassInst *)ancestors[depth];
        match = ((TemplClass *)record)->instances;
        depth = depth - 1;
        while (1) {
            if (match == NULL) {
                CError_FATAL(1146);
            }
            if (match->parent == target)
                break;
            match = match->next;
        }
        target = match;
        if ((match->theclass.flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800) {
            CTemplateClass_InstantiateClass(&match->theclass);
        }
    } while (0 <= depth);
    return match;
}

Type *CTemplateTools_GetArgumentType(TemplArg *record, TypeTemplDep *key, unsigned int qualifiers,
                                     unsigned int *resultQualifiers)
{
    UInt16 index;
    *resultQualifiers = qualifiers;
    if (key->type == TYPETEMPLATE && key->kind == 0) {
        do {
            if (record == NULL) {
                CError_FATAL(1103);
            }
            index = record->pid.index;
        } while (index != key->u.pid.index || record->pid.nindex != key->u.pid.nindex);
        if (record->pid.type == 0) {
            CError_FATAL(1107);
        }
        *resultQualifiers |= record->data.typeparam.qual;
        return record->data.typeparam.type;
    }
    CError_ReportError(190U);
    return (Type *)&stsignedint;
}

Boolean CTemplTool_TemplDepTypeCompare(TypeTemplDep *a, TypeTemplDep *b)
{
    if (a == b)
        return 1;
    if (a->kind != b->kind)
        return 0;
    switch (a->kind) {
        case 0:
            return a->u.pid.nindex == b->u.pid.nindex && a->u.pid.index == b->u.pid.index;
        case 1:
            return CTemplTool_TemplDepTypeCompare(a->u.qual.type, b->u.qual.type) && a->u.qual.name == b->u.qual.name;
        case 2:
            return a->u.templ.templ == b->u.templ.templ && CTemplTool_EqualArgs(a->u.templ.args, b->u.templ.args);
        case 3:
            return iscpp_typeequal(a->u.array.type, b->u.array.type) &&
                   CTemplateTools_00517a40(a->u.array.index, b->u.array.index);
        case 4:
            return CTemplTool_TemplDepTypeCompare(a->u.qualtempl.type, b->u.qualtempl.type) &&
                   CTemplTool_EqualArgs(a->u.qualtempl.args, b->u.qualtempl.args);
        case 5:
            return iscpp_typeequal(a->u.bitfield.type, b->u.bitfield.type) &&
                   CTemplateTools_00517a40(a->u.bitfield.size, b->u.bitfield.size);
        default:
            CError_FATAL(1079);
            return 0;
    }
}

TemplArg *CTemplateTools_CopyCTStateElemList(TemplArg *p)
{
    TemplArg *res = NULL;
    TemplArg *cur;

    while (p != NULL) {
        if (res != NULL) {
            cur->next = (TemplArg *)galloc(sizeof(TemplArg));
            cur = cur->next;
        } else {
            res = cur = (TemplArg *)galloc(sizeof(TemplArg));
        }
        *cur = *p;
        if (cur->pid.type == 0 && cur->data.paramdecl.expr != NULL)
            cur->data.paramdecl.expr = fn_00513040(cur->data.paramdecl.expr, 1);
        p = p->next;
    }
    return res;
}

UInt8 CTemplTool_EqualArgs(TemplArg *left, TemplArg *right)
{
    SInt16 namesEqual;
    UInt8 argumentsEqual;
    if (left != NULL) {
        do {
            if (right == NULL) {
                return '\0';
            }
            if (left->pid.type != '\0') {
                if ((right->pid.type == '\0') ||
                    (namesEqual = iscpp_typeequal(left->data.typeparam.type, right->data.typeparam.type),
                     namesEqual == 0) ||
                    (left->data.typeparam.qual != right->data.typeparam.qual)) {
                    return '\0';
                }
            } else {
                if ((right->pid.type != '\0') ||
                    (argumentsEqual = CTemplateTools_00517a40(left->data.paramdecl.expr, right->data.paramdecl.expr),
                     argumentsEqual == '\0')) {
                    return '\0';
                }
            }
            left = left->next;
            right = right->next;
        } while (left != NULL);
    }
    if (right != NULL) {
        return '\0';
    }
    return '\x01';
}

void CTemplTool_CheckTemplArgType(Type *type)
{
    TypeClass *classType = (TypeClass *)type;
    while (classType->type == TYPEPOINTER) {
        Type *baseType = (Type *)classType;
        classType = (TypeClass *)baseType->array[0].element;
    }
    if (classType->type == TYPECLASS) {
        if (CParser_IsNullOrAtOrDollarPrefixedName(classType->classname) ||
            CScope_IsInLocalNameSpace(classType->nspace)) {
            CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
            return;
        }
    }
    return;
}

ENode *CTempl_MakeTemplDepExpr(ENode *left, UInt8 kind, ENode *right)
{
    if (right->rtype->type != TYPETEMPLDEPEXPR) {
        right = CExpr_GeneratePointerAndRewriteConst(right);
        if (right->type != EINTCONST) {
            CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENT_DEPENDENT_EXPRESSION);
            right = nullnode();
        }
    }
    if (left != NULL) {
        if (left->rtype->type != TYPETEMPLDEPEXPR) {
            left = CExpr_GeneratePointerAndRewriteConst(left);
            if (left->type != EINTCONST) {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENT_DEPENDENT_EXPRESSION);
                left = nullnode();
            }
        }
        left = makediadicnode(left, right, kind);
    } else {
        left = makemonadicnode(right, kind);
    }
    left->rtype = &data_0055d5c0;
    return left;
}

#define NV(n) ((ENode *)(n))

Boolean CTemplateTools_00517a40(ENode *left, ENode *right)
{
    if (left == NULL || right == NULL)
        return 0;
    if (left->type != right->type)
        return 0;

    switch (left->type) {
        case EINTCONST:
            return CInt64_Equal(left->data.intval, right->data.intval);

        case EOBJREF: {
            Object *leftObject, *rightObject;
            leftObject = left->data.objref;
            while (leftObject->datatype == DALIAS)
                leftObject = leftObject->u.alias.object;
            rightObject = right->data.objref;
            while (rightObject->datatype == DALIAS)
                rightObject = rightObject->u.alias.object;
            return leftObject == rightObject;
        }

        case EOBJLIST:
            if (left->data.templatecomparison.tag != right->data.templatecomparison.tag)
                return 0;
            switch (left->data.templatecomparison.tag) {
                case 0:
                    return left->data.templatecomparison.u.wb.templateLevel ==
                               right->data.templatecomparison.u.wb.templateLevel &&
                           left->data.templatecomparison.u.wb.parameterIndex ==
                               right->data.templatecomparison.u.wb.parameterIndex;
                case 1:
                    return iscpp_typeequal(left->data.templatecomparison.u.p0, right->data.templatecomparison.u.p0);
                case 3:
                    return iscpp_typeequal(left->data.templatecomparison.p4, right->data.templatecomparison.p4) != 0 &&
                           left->data.templatecomparison.qualifiers == right->data.templatecomparison.qualifiers;
                case 4:
                    return iscpp_typeequal(left->data.templatecomparison.u.p0, right->data.templatecomparison.u.p0) !=
                               0 &&
                           left->data.templatecomparison.p4 == right->data.templatecomparison.p4;
                case 5:
                    return left->data.templatecomparison.u.d0 == right->data.templatecomparison.u.d0;
                default:
                    CError_FATAL(890);
            }

        case EMUL:
        case EDIV:
        case EMODULO:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
            return CTemplateTools_00517a40(left->data.diadic.left, right->data.diadic.left) &&
                   CTemplateTools_00517a40(left->data.diadic.right, right->data.diadic.right);

        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
            return CTemplateTools_00517a40(left->data.monadic, right->data.monadic);

        case ECOND:
            return CTemplateTools_00517a40(left->data.cond.cond, right->data.cond.cond) &&
                   CTemplateTools_00517a40(left->data.cond.expr1, right->data.cond.expr1) &&
                   CTemplateTools_00517a40(left->data.cond.expr2, right->data.cond.expr2);
    }

    CError_FATAL(922);
    return 0;
}

Type *CTemplTool_IsDependentTemplate(TemplClass *templateClass, TemplArg *arguments)
{
    TypeTemplDep *result;
    TemplParam *parameter;
    TemplArg *argument;
    unsigned char dependent;
    if (!templateClass->templ_parent || templateClass->inst_parent) {
        argument = arguments;
        parameter = templateClass->templ__params;
        for (;;) {
            if (!argument) {
                if (parameter)
                    CError_FATAL(806);
                return NULL;
            }
            if (!parameter || (unsigned char)argument->pid.type != (unsigned char)parameter->pid.type)
                CError_FATAL(809);
            if (argument->pid.type) {
                dependent = CTemplateTools_IsDependentType(argument->data.typeparam.type);
            } else {
                TypePointer *type = (TypePointer *)argument->data.typeparam.type;
                if (!type)
                    dependent = 0;
                else
                    dependent = type->target->type == TYPETEMPLDEPEXPR;
            }
            if (dependent)
                break;
            argument = argument->next;
            parameter = parameter->next;
        }
    }
    if (currentNameSpace->theclass == (TypeClass *)templateClass &&
        CTemplTool_IsSameTemplate(templateClass->templ__params, arguments))
        return (Type *)templateClass;
    result = CDecl_NewTemplDepType(2);
    result->u.templ.templ = templateClass;
    result->u.templ.args = arguments;
    return (Type *)result;
}

TypeClass *CTemplateTools_GetTemplClass(TypeTemplDep *record)
{
    if (record->kind == 1 && record->u.qual.type->kind == 2) {
        record = record->u.qual.type;
    } else if (record->kind != 2) {
        return NULL;
    }
    if (CTemplTool_IsSameTemplate((record->u.templ.templ)->templ__params, record->u.templ.args))
        return (TypeClass *)record->u.templ.templ;
    return NULL;
}

UInt8 CTemplTool_IsSameTemplate(TemplParam *parameter, TemplArg *argument)
{
    for (;;) {
        if (argument == NULL) {
            if (parameter != NULL)
                CError_FATAL(681);
            return 1;
        }
        if (parameter == NULL || argument->pid.type != parameter->pid.type)
            CError_FATAL(684);

        if (argument->pid.type) {
            if (argument->data.typeparam.type->type != TYPETEMPLATE ||
                ((TypeTemplDep *)argument->data.typeparam.type)->kind != 0 ||
                ((TypeTemplDep *)argument->data.typeparam.type)->u.pid.nindex != parameter->pid.nindex ||
                ((TypeTemplDep *)argument->data.typeparam.type)->u.pid.index != parameter->pid.index ||
                argument->data.typeparam.qual != 0)
                return 0;
        } else {
            if (argument->data.paramdecl.expr->type != EOBJLIST ||
                argument->data.paramdecl.expr->data.templatecomparison.tag != 0 ||
                argument->data.paramdecl.expr->data.templatecomparison.u.wb.templateLevel != parameter->pid.nindex ||
                argument->data.paramdecl.expr->data.templatecomparison.u.wb.parameterIndex != parameter->pid.index)
                return 0;
        }
        argument = argument->next;
        parameter = parameter->next;
    }
}

Boolean CTemplTool_IsTypeDepExpr(ENode *node)
{
    if (node == NULL) {
        return 0;
    }
    return node->rtype->type == TYPETEMPLDEPEXPR;
}

unsigned char CTemplateTools_IsDependentType(Type *type)
{
    for (;;) {
        switch ((SInt8)type->type) {
            case TYPETEMPLATE:
                return 1;
            case TYPEVOID:
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPESTRUCT:
                return 0;
            case TYPECLASS:
                return (((TypeClass *)type)->flags & CLASS_IS_TEMPL) != 0;
            case TYPEMEMBERPOINTER:
                if (CTemplateTools_IsDependentType(((TypeMemberPointer *)type)->memberType))
                    return 1;
                type = ((TypeMemberPointer *)type)->owner.type;
                break;
            case TYPEPOINTER:
            case TYPEARRAY:
                type = ((TypePointer *)type)->target;
                break;
            case TYPEFUNC: {
                FuncArg *e = ((TypeFunc *)type)->args;
                while (e != NULL && e != &data_00583098 && e != &data_00584748) {
                    if (CTemplateTools_IsDependentType(e->type))
                        return 1;
                    e = e->next;
                }
                type = ((TypeFunc *)type)->functype;
                break;
            }
            case TYPEBITFIELD:
                type = ((TypePointer *)type)->target;
                break;
            default:
                CError_FATAL(653);
                break;
        }
    }
}

void CTemplTool_RemoveTemplateArgumentNameSpace(NameSpace *args, TemplClassInst *func, CScopeSave *scope)
{
    NameSpace *arg;
    arg = func->theclass.nspace;
    while (arg->parent) {
        if (arg->theclass != NULL && (arg->theclass->flags & CLASS_IS_TEMPL_INST) != 0 && arg->parent->is_templ != 0)
            arg->parent = arg->parent->parent;
        arg = arg->parent;
    }
    CScope_RestoreScope(scope);
}

void CTemplTool_RemoveOuterTemplateArgumentNameSpace(NameSpace *ns)
{
    TypeClass *theclass;
    NameSpace *next;

    while ((next = ns->parent) != NULL) {
        if ((theclass = ns->theclass) != NULL && (theclass->flags & CLASS_IS_TEMPL_INST) != 0 && next->is_templ != 0) {
            ns->parent = next->parent;
        }
        ns = ns->parent;
    }
}

NameSpace *CTemplateTools_InsertTemplateArgs(TemplParam *context, TemplClassInst *function, CScopeSave *scope)
{
    NameSpace *args;
    NameSpace *arg;
    TemplClassInst *type;
    args = CTemplTool_SetupTemplateArgumentNameSpace(context, function->inst_args, 0);
    args->parent = function->theclass.nspace->parent;
    function->theclass.nspace->parent = args;
    for (arg = args; arg != NULL; arg = arg->parent) {
        if ((type = (TemplClassInst *)arg->theclass) != NULL && (type->theclass.flags & 0x800U) != 0U) {
            NameSpace *expanded =
                CTemplTool_SetupTemplateArgumentNameSpace((type->templ)->templ__params, type->inst_args, 0);
            expanded->parent = arg->parent;
            arg->parent = expanded;
        }
    }
    BE_elf_SaveAndSetScope(function->theclass.nspace, scope);
    return args;
}

void CTemplTool_SetupOuterTemplateArgumentNameSpace(NameSpace *nameSpace)
{
    NameSpace *newNameSpace;
    TemplClassInst *classInfo;

    while (nameSpace != NULL) {
        if (nameSpace->theclass != NULL && (nameSpace->theclass->flags & CLASS_IS_TEMPL_INST) != 0) {
            classInfo = (TemplClassInst *)nameSpace->theclass;
            newNameSpace =
                CTemplTool_SetupTemplateArgumentNameSpace((classInfo->templ)->templ__params, classInfo->inst_args, 0);
            newNameSpace->parent = nameSpace->parent;
            nameSpace->parent = newNameSpace;
        }
        nameSpace = nameSpace->parent;
    }
}

NameSpace *CTemplTool_SetupTemplateArgumentNameSpace(TemplParam *arglist, TemplArg *targlist, Boolean flag)
{
    Boolean copy;
    NameSpace *ns;
    Object *obj;
    copy = 0;
    if (!flag && data_00588240 != NULL) {
        flag = copy = 1;
    }
    ns = CScope_NewListNameSpace(NULL, flag);
    ns->is_templ = 1;
    if (copy)
        ns->is_global = 0;
    if (arglist == NULL)
        return ns;
    while (arglist != NULL) {
        if (targlist == NULL)
            CError_FATAL(468);
        if (arglist->name != NULL) {
            if (targlist->pid.type == 0) {
                if (flag) {
                    obj = (Object *)galloc(sizeof(Object));
                    memclrw(obj, sizeof(Object));
                } else {
                    obj = (Object *)CompilerTools_AllocatePool(sizeof(Object));
                    memclrw(obj, sizeof(Object));
                }
                obj->otype = OT_OBJECT;
                obj->access = ACCESSPUBLIC;
                obj->nspace = ns;
                obj->name = arglist->name;
                obj->type = targlist->data.paramdecl.expr->rtype;
                obj->qual = targlist->data.paramdecl.expr->flags & ENODE_FLAG_QUALS;
                obj->datatype = DEXPR;
                obj->u.expr = targlist->data.paramdecl.expr;
                if (flag)
                    obj->u.expr = fn_00513040(obj->u.expr, 1);
            } else {
                ObjType *typeObject;
                if (flag) {
                    typeObject = (ObjType *)galloc(sizeof(ObjType));
                    memclrw(typeObject, sizeof(ObjType));
                } else {
                    typeObject = (ObjType *)CompilerTools_AllocatePool(sizeof(ObjType));
                    memclrw(typeObject, sizeof(ObjType));
                }
                typeObject->otype = OT_TYPE;
                typeObject->access = ACCESSPUBLIC;
                typeObject->type = targlist->data.typeparam.type;
                typeObject->qual = targlist->data.typeparam.qual;
                obj = (Object *)typeObject;
            }
            CScope_AddObject(ns, arglist->name, (ObjBase *)obj);
        }
        arglist = arglist->next;
        targlist = targlist->next;
    }
    if (targlist != NULL)
        CError_FATAL(519);
    return ns;
}

UInt8 CTemplTool_EqualParams(TemplParam *left, TemplParam *right, char copyValue)
{
    SInt16 stringsMatch;

    while (1) {
        if (left == NULL) {
            return right == NULL;
        }
        if (right == NULL) {
            return '\0';
        }
        if (left->pid.type != right->pid.type) {
            return '\0';
        }
        if (copyValue != '\0') {
            left->name = right->name;
        }
        if (left->pid.type == '\0') {
            stringsMatch = iscpp_typeequal(left->data.paramdecl.type, right->data.paramdecl.type);
            if ((stringsMatch == 0) || (left->data.paramdecl.qual != right->data.paramdecl.qual)) {
                return '\0';
            }
        }
        left = left->next;
        right = right->next;
    }
}

void CTemplTool_MergeArgNames(Type *sourceFunc, Type *destinationFunc)
{
    FuncArg *sourceArg;
    FuncArg *destinationArg;

    if (destinationFunc->type != TYPEFUNC || sourceFunc->type != TYPEFUNC)
        CError_FATAL(396);

    sourceArg = CTemplateTools_FirstArg((TypeMemberFunc *)sourceFunc);
    destinationArg = CTemplateTools_FirstArg((TypeMemberFunc *)destinationFunc);

    for (;;) {
        if (sourceArg == NULL || destinationArg == NULL || sourceArg == &data_00583098 ||
            destinationArg == &data_00583098) {
            CError_ASSERT(403, sourceArg == destinationArg);
            break;
        }
        destinationArg->name = sourceArg->name;
        sourceArg = sourceArg->next;
        destinationArg = destinationArg->next;
    }

    CTemplateTools_SetFirstArgName((TypeMemberFunc *)destinationFunc);
}

void CTemplTool_MergeDefaultArgs(TemplParam *destination, TemplParam *source)
{
    do {
        if (destination == NULL) {
            if (source != NULL) {
                CError_FATAL(339);
            }
            return;
        }
        if (source == NULL) {
            CError_FATAL(342);
        }
        if (destination->pid.type != source->pid.type) {
            CError_FATAL(343);
        }
        if (destination->pid.type != 0) {
            if (destination->data.typeparam.type == NULL && source->data.typeparam.type != NULL) {
                memcpy(&destination->data, &source->data, 12);
            }
        } else {
            if (destination->data.paramdecl.defaultarg == NULL && source->data.paramdecl.defaultarg != NULL) {
                memcpy(&destination->data, &source->data, 12);
            }
        }
        destination = destination->next;
        source = source->next;
    } while (1);
}

struct TemplateFunction *CTemplTool_GetFuncTempl(Object *obj)
{
    Object *p = obj;
    while (p->datatype == DALIAS) {
        p = p->u.alias.object;
    }
    if (p->type->type != TYPEFUNC || (((TypeFunc *)p->type)->flags & 0x400U) == 0) {
        CError_FATAL(311);
    }
    return p->u.templateFunction;
}

TemplClass *CTemplTool_IsTemplate(TypeTemplDep *reference)
{
    TemplClass *target;
    TemplClass *parent;
    TemplClass *instance;
    TemplClass *nestedParent;
    TypeClass *nestedInstance;
    TemplClass *nestedClass;
    TemplClass *resolved;
    TemplArg *arguments;
    CE_ASSERT(reference->type != TYPETEMPLATE, CError_FATAL(242));
    if (reference->kind == 2) {
        if (CTemplTool_IsIdenticalTemplArgList(reference->u.templ.args, (reference->u.templ.templ)->templ__params) !=
            0) {
            return reference->u.templ.templ;
        }
        if (((target = reference->u.templ.templ))->pspecs != NULL) {
            resolved = target;
            if (CTemplateClass_SelectSpecialization(reference->u.templ.args, &resolved, &arguments) != 0 &&
                CTemplTool_IsIdenticalTemplArgList(arguments, resolved->templ__params) != 0) {
                return resolved;
            }
        }
        return NULL;
    }
    if (reference->kind == 1) {
        parent = CTemplTool_IsTemplate(reference->u.qual.type);
        if (parent != NULL) {
            instance = (TemplClass *)CScope_GetTagType(parent->theclass.nspace, reference->u.qual.name);
            if (instance != NULL && instance->theclass.type == TYPECLASS &&
                (instance->theclass.flags & CLASS_IS_TEMPL) != 0 && instance->templ__params == NULL) {
                return instance;
            }
        }
        return NULL;
    }
    if (reference->kind == 4) {
        CE_ASSERT(reference->u.qualtempl.type->kind != 1, CError_FATAL(284));
        nestedParent = CTemplTool_IsTemplate(reference->u.qualtempl.type->u.qual.type);
        if (nestedParent != NULL) {
            nestedInstance =
                (TypeClass *)CScope_GetTagType(nestedParent->theclass.nspace, reference->u.qualtempl.type->u.qual.name);
            if (nestedInstance != NULL && nestedInstance->type == TYPECLASS &&
                (nestedInstance->flags & CLASS_IS_TEMPL) != 0) {
                nestedClass = (TemplClass *)nestedInstance;
                if (CTemplTool_IsIdenticalTemplArgList(reference->u.qualtempl.args, nestedClass->templ__params) != 0) {
                    return nestedClass;
                }
            }
        }
    }
    return NULL;
}

/* Records used by the template argument comparison. */
/* The two layouts referenced by a pattern's value. */

UInt8 CTemplTool_IsIdenticalTemplArgList(TemplArg *pattern, TemplParam *argument)
{
    TypeTemplDep *type;
    while (pattern != NULL) {
        if (argument == NULL)
            return 0;
        if (argument->pid.type != pattern->pid.type)
            CError_FATAL(207);
        if (pattern->pid.type) {
            if (pattern->data.typeparam.type->type != TYPETEMPLATE ||
                (type = (TypeTemplDep *)pattern->data.typeparam.type)->kind != 0 ||
                type->u.pid.index != argument->pid.index || type->u.pid.nindex != argument->pid.nindex)
                return 0;
        } else {
            if (pattern->data.paramdecl.expr->type != EOBJLIST ||
                pattern->data.paramdecl.expr->data.templatecomparison.tag != 0 ||
                pattern->data.paramdecl.expr->data.templatecomparison.u.wb.parameterIndex != argument->pid.index ||
                pattern->data.paramdecl.expr->data.templatecomparison.u.wb.templateLevel != argument->pid.nindex)
                return 0;
        }
        pattern = pattern->next;
        argument = argument->next;
    }
    return argument == NULL;
}

TemplArg *CTemplateTools_CopySlotsToList(struct DeduceInfo *src)
{
    SInt32 i = 0;
    TemplArg *head;
    TemplArg *ep;

    for (i = 0; i < src->maxCount; i++) {
        if (i != 0) {
            ep = ep->next = (TemplArg *)galloc(0x12);
        } else {
            ep = (TemplArg *)galloc(0x12);
            head = ep;
        }
        *ep = src->args[i];
    }
    ep->next = NULL;
    return head;
}

void CTemplTool_InsertTemplateParameter(NameSpace *scope, TemplParam *source)
{
    TypeTemplDep *nspace;
    ObjType *object;
    nspace = CDecl_NewTemplDepType(0);
    nspace->u.pid = source->pid;
    object = (ObjType *)galloc(10);
    memclrw(object, 10);
    object->otype = OT_TYPE;
    object->access = ACCESSPUBLIC;
    object->type = (Type *)nspace;
    CScope_AddObject(scope, source->name, (ObjBase *)object);
}

#ifndef TRUE
#endif
#ifndef FALSE
#endif

Boolean CTemplTool_InitDeduceInfo(DeduceInfo *info, TemplParam *params, TemplArg *args, Boolean allowUnnamed)
{
    SInt32 argumentIndex;
    SInt32 parameterIndex;
    TemplArg *slots;
    TemplArg *nextSlot;
    TemplParam *parameter;
    SInt32 count;

    if (params == NULL) {
        info->args = info->argBuffer;
        info->maxCount = 0;
        info->depth = 0xff;
        return TRUE;
    }

    memclrw(info, sizeof(*info));

    count = 0;
    parameter = params;
    while (parameter != NULL) {
        parameter = parameter->next;
        count++;
    }

    if (count > 16) {
        slots = (TemplArg *)CompilerTools_AllocatePool(count * sizeof(*slots));
        memclrw(slots, count * sizeof(*slots));
    } else {
        slots = info->argBuffer;
    }

    info->args = slots;
    info->maxCount = count;
    info->depth = params->pid.nindex;

    count = 0;
    parameter = params;
    while (parameter != NULL) {
        slots[count].pid = parameter->pid;
        count++;
        parameter = parameter->next;
    }

    argumentIndex = 0;
    parameter = params;
    if (args != NULL) {
        nextSlot = slots;
        do {
            TemplArg *argument;
            if (parameter == NULL || parameter->pid.type != args->pid.type) {
                return FALSE;
            }
            argument = args;
            (slots)[argumentIndex].data = argument->data;
            if (argumentIndex > 0) {
                slots[argumentIndex - 1].next = nextSlot;
            }
            slots[argumentIndex].next = NULL;
            slots[argumentIndex].is_deduced = TRUE;
            info->count++;
            if (parameter->pid.type == 0) {
                if (assign_check(slots[argumentIndex].data.paramdecl.expr, parameter->data.paramdecl.type,
                                 parameter->data.paramdecl.qual, 0, 0, 0)) {
                    slots[argumentIndex].data.paramdecl.expr =
                        oldassignmentpromotion(slots[argumentIndex].data.paramdecl.expr, parameter->data.paramdecl.type,
                                               parameter->data.paramdecl.qual, 0);
                } else {
                    return FALSE;
                }
            }
            nextSlot++;
            argumentIndex++;
            args = args->next;
            parameter = parameter->next;
        } while (args != NULL);
    }

    if (allowUnnamed) {
        parameter = params;
        parameterIndex = 0;
        while (parameter != NULL) {
            if (slots[parameterIndex].is_deduced == 0 && parameter->name == NULL) {
                if (args->pid.type != 0) {
                    slots[parameterIndex].data.typeparam.type = &stvoid;
                } else {
                    slots[parameterIndex].data.paramdecl.expr = nullnode();
                }
                slots[parameterIndex].is_deduced = TRUE;
            }
            parameter = parameter->next;
            parameterIndex++;
        }
    }

    return TRUE;
}

struct TemplStack *CTemplateTools_PopObjectReferenceEntry(struct TemplStack *entry)
{
    struct TemplStack *next;
    if (object_reference_stack != entry)
        CError_FATAL(53);
    next = entry->next;
    object_reference_stack = next;
    objectReferenceEntryCount -= 1U;
    if (objectReferenceEntryCount < 0)
        objectReferenceEntryCount = 0U;
    return next;
}
