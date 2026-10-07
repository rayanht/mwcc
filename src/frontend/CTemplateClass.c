#define CERROR_FILE "CTemplateClass.c"
#include "compiler/common.h"
#include "compiler/CTemplateClass.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CBrowse.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMangler.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/FunctionCalls.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <errno.h>
#include <stdlib.h>

typedef struct TCtx TCtx;

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

struct S2;
struct S3;

void fn_0051b800(void)
{
    return;
}

void fn_0051b810(void)
{
    return;
}

unsigned char CTemplateClass_InstantiateClass(TypeClass *theclass)
{
    struct TemplateClassDeclaration *declaration;
    TypeClassTemplate *templateClass;
    Type *instantiatedType;
    struct CParseSave *savedState;
    struct TemplateInstantiationMapping *objectMapping;
    Object *object;
    ENode *initializer;
    struct TemplateInstantiationMapping *typeMapping;
    unsigned char access;
    TypeTemplDep *templateType;
    struct TemplateInstantiationMapping *baseMapping;
    char savedMode;
    TypeClassExt800 *classInstance;
    struct TemplateClassDeclaration *memberDeclaration;
    struct ObjectReferenceEntry templateSave;
    CScopeSave scopeSave;
    TypeClassTemplate *resolvedTemplate;
    ClassLayoutInput classInfo;
    TemplateContext instantiation;
    int sourceSave;
    CTStateElem *resolvedArgs;
    UInt32 typeResult;
    CE_ASSERT((theclass->flags & CLASS_IS_TEMPL_INST) == 0, CError_FATAL(1907));
    if ((theclass->flags & CLASS_COMPLETED) != 0)
        return 1;
    if ((classInstance = (TypeClassExt800 *)theclass)->suppressImplicitInstantiation != 0)
        return 0;
    templateClass = (TypeClassTemplate *)classInstance->classTemplate;
    if (templateClass->relatedClass != NULL) {
        templateClass = (TypeClassTemplate *)templateClass->base.nspace->theclass;
        CE_ASSERT((templateClass->base.flags & CLASS_IS_TEMPL) == 0, CError_FATAL(42));
    }
    resolvedTemplate = templateClass;
    if (templateClass->specializations != NULL &&
        CTemplateClass_SelectSpecialization(classInstance->targs, &resolvedTemplate, &resolvedArgs) != 0) {
        CE_ASSERT(classInstance->templateArgumentOverride != 0, CError_FATAL(1926));
        classInstance->classTemplate = (Type *)resolvedTemplate;
        classInstance->templateArgumentOverride = classInstance->targs;
        classInstance->targs = resolvedArgs;
    }
    if ((resolvedTemplate->base.flags & CLASS_COMPLETED) == 0)
        return 0;
    if (classInstance->instantiating != 0)
        return 0;
    classInstance->instantiating = 1;
    BE_elf_SaveScopeAndEnterClass(theclass, &scopeSave);
    FunctionCalls_PushObjectReferenceEntry(&templateSave, theclass, NULL);
    savedState = data_00588240;
    data_00588240 = NULL;
    memclrw(&instantiation, sizeof(instantiation));
    instantiation.templateClass = &resolvedTemplate->base;
    instantiation.instance = &classInstance->base;
    instantiation.templateArgs = resolvedTemplate->templateParameters;
    instantiation.instanceArgs = classInstance->targs;
    CE_ASSERT(resolvedTemplate->base.sominfo != 0, CError_FATAL(1958));
    CE_ASSERT(resolvedTemplate->base.objcinfo != 0, CError_FATAL(1959));
    CE_ASSERT(resolvedTemplate->base.vtable != 0, CError_FATAL(1960));
    classInstance->base.flags |= resolvedTemplate->base.flags & 8312;
    instantiate_bases(&instantiation, &classInstance->base, resolvedTemplate);
    instantiation.hasNewVBases =
        (classInstance->base.flags & CLASS_HAS_VBASES) != 0 && (resolvedTemplate->base.flags & CLASS_HAS_VBASES) == 0;
    for (declaration = resolvedTemplate->declarations; declaration != NULL; declaration = declaration->next) {
        switch (declaration->kind) {
            case 0:
                fn_00449d60();
                CError_SaveAndSetWrittenEntry(&declaration->source, &sourceSave);
                CTemplateClass_0051cec0(&instantiation, (TypeClassTemplate *)declaration->target.type);
                fn_00449d60();
                CError_SetWrittenEntry(&sourceSave);
                break;
            case 1:
                fn_00449d60();
                CError_SaveAndSetWrittenEntry(&declaration->source, &sourceSave);
                instantiate_enum(&instantiation, declaration);
                fn_00449d60();
                CError_SetWrittenEntry(&sourceSave);
                break;
            default:
                CError_FATAL(2005);
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                break;
        }
    }
    instantiate_ivars(&instantiation, &classInstance->base, resolvedTemplate);
    instantiate_namespace_objects(&instantiation, &classInstance->base, &resolvedTemplate->base);
    CE_ASSERT(resolvedTemplate->base.friends != 0, CError_FATAL(2016));
    for (memberDeclaration = resolvedTemplate->declarations; memberDeclaration != NULL;
         memberDeclaration = memberDeclaration->next) {
        switch (memberDeclaration->kind) {
            case 0:
                break;
            case 1:
                for (typeMapping = instantiation.mappings; typeMapping != NULL; typeMapping = typeMapping->next) {
                    if (typeMapping->declaration == memberDeclaration) {
                        fn_00449d60();
                        CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                        initialize_enum_constants(&instantiation, memberDeclaration, typeMapping->replacement);
                        fn_00449d60();
                        CError_SetWrittenEntry(&sourceSave);
                        break;
                    }
                }
                break;
            case 2:
                fn_00449d60();
                CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                instantiate_friend_declaration(&instantiation, memberDeclaration->target.friendDeclaration);
                fn_00449d60();
                CError_SetWrittenEntry(&sourceSave);
                break;
            case 5:
                for (objectMapping = instantiation.mappings;; objectMapping = objectMapping->next) {
                    CE_ASSERT(objectMapping == 0, CError_FATAL(2047));
                    if (objectMapping->declaration == memberDeclaration) {
                        fn_00449d60();
                        CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                        object = (Object *)objectMapping->object;
                        initializer = CTemplTool_DeduceExpr(&instantiation, memberDeclaration->value.initializer);
                        if (initializer->type == EINTCONST && (object->qual & Q_CONST) != 0 &&
                            (object->type->type == TYPEINT || object->type->type == TYPEENUM)) {
                            object->u.data.u.intconst = initializer->data.intval;
                            object->qual |= (Q_INLINE_DATA | Q_IMPLICIT_WEAK);
                        } else {
                            CError_ReportError(ERR_ILLEGAL_STATIC_CONST_MEMBER_INITIALIZATION, object->name->name);
                        }
                        fn_00449d60();
                        CError_SetWrittenEntry(&sourceSave);
                        break;
                    }
                }
                break;
            case 6:
                fn_00449d60();
                CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                templateType = (TypeTemplDep *)memberDeclaration->target.type;
                access = memberDeclaration->value.access;
                typeResult = 0;
                CE_ASSERT(templateType->type != TYPETEMPLATE || templateType->kind != 1, CError_FATAL(1802));
                instantiatedType =
                    CTemplateTools_ResolveType(&instantiation, (Type *)templateType->u.qual.type, &typeResult);
                if (instantiatedType->type != TYPECLASS) {
                    CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE,
                                       templateType->u.qual.name->name);
                } else {
                    CDecl_CompleteType(instantiatedType);
                    CScope_AddClassUsingDeclaration(instantiation.instance, (TypeClass *)instantiatedType,
                                                    templateType->u.qual.name, access);
                }
                fn_00449d60();
                CError_SetWrittenEntry(&sourceSave);
                break;
            case 7:
                for (baseMapping = instantiation.mappings;; baseMapping = baseMapping->next) {
                    CE_ASSERT(baseMapping == 0, CError_FATAL(2067));
                    if (baseMapping->declaration == memberDeclaration) {
                        fn_00449d60();
                        CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                        instantiate_object_type(&instantiation, memberDeclaration, baseMapping->object);
                        fn_00449d60();
                        CError_SetWrittenEntry(&sourceSave);
                        break;
                    }
                }
        }
    }
    memclrw(&classInfo, 14);
    classInfo.count = (SInt16)resolvedTemplate->virtualSlotCount;
    classInfo.hasVirtualFunction = resolvedTemplate->hasVirtualFunction;
    savedMode = copts.structalignment;
    copts.structalignment = resolvedTemplate->structAlignment;
    CDecl_CompleteClass(&classInfo, &classInstance->base);
    copts.structalignment = savedMode;
    CTemplateTools_PopObjectReferenceEntry(&templateSave);
    CScope_RestoreScope(&scopeSave);
    data_00588240 = savedState;
    return 1;
}

void instantiate_friend_declaration(TemplateContext *ctx, struct TemplateDeclarationData *declaration)
{
    DeclInfo instance;
    CTStateElem *parameter;
    void *savedScope[3]; /* BE_elf_SaveAndSetScope: retain the original three-word stack slot */
    Boolean result;
    NameSpace *scope;
    Object *object;

    CDecl_InitDeclInfoFromTemplateDeclarationData(&instance, declaration);
    if (CTemplateTools_IsDependentType(instance.dtype))
        instance.dtype = CTemplateTools_ResolveType(ctx, instance.dtype, (UInt32 *)&instance.qual);
    if (instance.parsedData != NULL) {
        instance.parsedData = CTemplateTools_CopyCTStateElemList(instance.parsedData);
        parameter = instance.parsedData;
        while (parameter != NULL) {
            if (parameter->pid.type) {
                if (CTemplateTools_IsDependentType(parameter->argument.type))
                    parameter->argument.type =
                        CTemplateTools_ResolveType(ctx, parameter->argument.type, (UInt32 *)&parameter->qualifiers);
            } else {
                if (CTemplTool_IsTypeDepExpr(parameter->argument.expression))
                    parameter->argument.expression = CTemplTool_DeduceExpr(ctx, parameter->argument.expression);
            }
            parameter = parameter->next;
        }
    }
    if (instance.dtype->type == TYPEFUNC) {
        scope = CScope_FindGlobalNS(ctx->instance->nspace);
        BE_elf_SaveAndSetScope(scope, (CScopeSave *)&savedScope); /* original stack-slot view */
        object = CDecl_GetFunctionObject(&instance, NULL, &result, 0);
        CScope_RestoreScope((CScopeSave *)&savedScope); /* original stack-slot view */
        if (object != NULL) {
            CDecl_AddFriend(ctx->instance, object, NULL);
            if (declaration->inlineTokenBuffer.count)
                CInline_AddFunctionPrecNode(object, ctx->instance, &declaration->inlineLocation,
                                            &declaration->inlineTokenBuffer, 0);
        } else {
            CError_ReportError(ERR_ILLEGAL_FRIEND_DECLARATION);
        }
    } else {
        if (instance.dtype->type != TYPECLASS)
            CError_FATAL(1881);
        CDecl_AddFriend(ctx->instance, NULL, instance.dtype);
    }
}

void instantiate_namespace_objects(TemplateContext *map, TypeClass *unused, TypeClass *obj)
{
    MemberVarAlias *copy;
    ObjType *member;
    Object *templateObject;
    NameSpaceName *entry;
    NameSpaceObjectList *objects;

    if (obj->nspace->is_hash != 0) {
        CError_FATAL(1754);
    }
    for (entry = obj->nspace->data.list; entry != NULL; entry = entry->next) {
        for (objects = &entry->first; objects != NULL; objects = objects->next) {
            switch ((member = (ObjType *)objects->object)->otype) {
                case 0:
                case 2:
                    break;
                case 4:
                    if (((ObjMemberVar *)member)->has_path) {
                        copy = (MemberVarAlias *)galloc(sizeof(MemberVarAlias));
                        *copy = *(MemberVarAlias *)member;
                        if (copy->bases != NULL && copy->bases->type == (Type *)map->templateClass) {
                            copy->bases = CClass_GetPathCopy(copy->bases, 1);
                            copy->bases->type = (Type *)map->instance;
                        }
                        CScope_AddObject(map->instance->nspace, copy->member.name, (ObjBase *)&copy->member);
                    }
                    break;
                case 1:
                    instantiate_objtype(map, member, entry->name);
                    break;
                case 3:
                    CError_FATAL(1778);
                case 5:
                    templateObject = (Object *)objects->object;
                    instantiate_template_object(map, templateObject);
                    break;
                default:
                    CError_FATAL(1785);
                    break;
            }
        }
    }
}

void instantiate_object_type(TemplateContext *context, TemplateClassDeclaration *function, ObjBase *object)
{
    if (object->otype == OT_MEMBERVAR) {
        OBJ_MEMBER_VAR(object)->type = CTemplateTools_ResolveType(
            (TemplateContext *)context, OBJ_MEMBER_VAR(object)->type, &OBJ_MEMBER_VAR(object)->qual);
        if (OBJ_MEMBER_VAR(object)->type->size == 0) {
            CDecl_CompleteType(OBJ_MEMBER_VAR(object)->type);
            if (!(copts.f96 != 0 && OBJ_MEMBER_VAR(object)->next == NULL && OBJ_MEMBER_VAR(object)->type->size == 0 &&
                  IS_TYPE_ARRAY(OBJ_MEMBER_VAR(object)->type)))
                CanAllocObject(OBJ_MEMBER_VAR(object)->type);
        }
        return;
    }
    if (object->otype == OT_TYPE) {
        OBJ_TYPE(object)->type =
            CTemplateTools_ResolveType((TemplateContext *)context, OBJ_TYPE(object)->type, &OBJ_TYPE(object)->qual);
        return;
    }
    if (object->otype != OT_OBJECT)
        CError_FATAL(1661);
    {
        Object *currentFunction = (Object *)function->target.object;
        if (IS_TYPE_FUNC(currentFunction->type) && (TYPE_FUNC(currentFunction->type)->flags & 0x400)) {
            currentFunction = (Object *)currentFunction->u.templateFunction;
            CError_ASSERT(1671, currentFunction != NULL);
            CError_ASSERT(1672, !context->modes.processingArgument);
            context->modes.processingArgument = 1;
            context->parameterNIndex = ((TypeBitfield *)((TemplateFunction *)currentFunction)->params)->offset;
            OBJECT(object)->type =
                CTemplateTools_ResolveType((TemplateContext *)context, OBJECT(object)->type, &OBJECT(object)->qual);
            context->modes.processingArgument = 0;
            CError_ASSERT(1677, IS_TYPE_FUNC(OBJECT(object)->type));
            TYPE_FUNC(OBJECT(object)->type)->flags |= 0x400;
            return;
        }
        OBJECT(object)->type =
            CTemplateTools_ResolveType((TemplateContext *)context, OBJECT(object)->type, &OBJECT(object)->qual);
        OBJECT(object)->qual |= Q_IS_TEMPLATED;
        if (IS_TYPE_FUNC(OBJECT(object)->type))
            TYPE_FUNC(OBJECT(object)->type)->flags &= ~FUNC_DEFINED;
        switch (OBJECT(object)->datatype) {
            case DFUNC:
            case DVFUNC:
                CError_ASSERT(1693, IS_TYPE_FUNC(OBJECT(object)->type));
                if (TYPE_FUNC(OBJECT(object)->type)->flags & FUNC_CONVERSION) {
                    CError_ASSERT(1696, IS_TYPE_FUNC(OBJECT(object)->type));
                    if (CTemplateTools_IsDependentType(TYPE_FUNC(currentFunction->type)->functype)) {
                        OBJECT(object)->name = CMangler_ConversionFuncName(TYPE_FUNC(OBJECT(object)->type)->functype,
                                                                           TYPE_FUNC(OBJECT(object)->type)->qual);
                        CScope_AddObject(context->instance->nspace, OBJECT(object)->name, (ObjBase *)OBJECT(object));
                    }
                }
                if ((TYPE_FUNC(OBJECT(object)->type)->flags & FUNC_IS_DTOR) && context->hasNewVBases) {
                    FuncArg *argument;
                    CError_ASSERT(1710, TYPE_FUNC(OBJECT(object)->type)->args != NULL);
                    argument = CParser_NewFuncArg();
                    argument->type = (Type *)&stsignedshort;
                    argument->next = TYPE_FUNC(OBJECT(object)->type)->args->next;
                    TYPE_FUNC(OBJECT(object)->type)->args->next = argument;
                }
                break;
        }
    }
}

void instantiate_template_object(TemplateContext *ctx, Object *templ)
{
    Boolean needsNewNamespace = 1;
    TemplateObjectInstance *obj;
    TemplateClassDeclaration *matchingInstance;
    TemplateClassDeclaration *instance;
    TemplateInstantiationMapping *link;

    if (templ->nspace != ctx->templateClass->nspace) {
        if (templ->datatype != DALIAS)
            CError_FATAL(1511);
        needsNewNamespace = 0;
    }
    if (templ->type->type == TYPEFUNC && (((TypeFunc *)templ->type)->flags & 0x400) != 0) {
        CTemplateClass_0051c680(ctx, templ);
        return;
    }
    for (matchingInstance = ((TypeClassTemplate *)ctx->templateClass)->declarations; matchingInstance != NULL;
         matchingInstance = matchingInstance->next) {
        if (matchingInstance->kind == 7 && matchingInstance->target.object == (ObjBase *)templ)
            break;
    }
    obj = galloc(sizeof(TemplateObjectInstance));
    obj->base = *templ;
    if (matchingInstance != NULL) {
        link = CompilerTools_AllocatePool(0x14);
        link->next = ctx->mappings;
        link->declaration = matchingInstance;
        ctx->mappings = (struct TemplateInstantiationMapping *)link;
        link->object = (ObjBase *)obj;
    } else {
        obj->base.type = (Type *)CTemplateTools_ResolveType(ctx, (Type *)obj->base.type, (UInt32 *)&obj->base.qual);
    }
    if (needsNewNamespace)
        obj->base.nspace = ctx->instance->nspace;
    obj->base.qual |= Q_IS_TEMPLATED;
    obj->templateObject = templ;
    if (obj->base.type->type == TYPEFUNC)
        ((TypeFunc *)obj->base.type)->flags &= ~FUNC_DEFINED;
    switch (obj->base.datatype) {
        case DDATA:
            obj->base.u.data.linkname = NULL;
            for (instance = ((TypeClassTemplate *)ctx->templateClass)->declarations; instance != NULL;
                 instance = instance->next) {
                if (instance->kind == 5 && instance->target.object == (ObjBase *)templ) {
                    link = CompilerTools_AllocatePool(0x14);
                    link->next = ctx->mappings;
                    link->declaration = instance;
                    ctx->mappings = (struct TemplateInstantiationMapping *)link;
                    link->object = (ObjBase *)obj;
                    break;
                }
            }
            break;
        case DFUNC:
        case DVFUNC:
            obj->base.u.func.linkname = NULL;
            if (obj->base.type->type != TYPEFUNC)
                CError_FATAL(1574);
            if (obj->base.u.func.u != NULL || obj->base.u.func.defargdata != NULL)
                CError_FATAL(1575);
            if ((((TypeFunc *)obj->base.type)->flags & FUNC_IS_DTOR) != 0 && ctx->hasNewVBases != 0 &&
                matchingInstance == NULL) {
                FuncArg *arg;
                if (((TypeFunc *)obj->base.type)->args == NULL)
                    CError_FATAL(1581);
                arg = CParser_NewFuncArg();
                arg->type = (Type *)&stsignedshort;
                arg->next = ((FuncArg *)((TypeFunc *)obj->base.type)->args)->next;
                ((FuncArg *)((TypeFunc *)obj->base.type)->args)->next = arg;
            }
            if ((((TypeFunc *)obj->base.type)->flags & FUNC_CONVERSION) != 0) {
                if (templ->type->type != TYPEFUNC)
                    CError_FATAL(1589);
                if (CTemplateTools_IsDependentType(((TypeFunc *)templ->type)->functype)) {
                    if (matchingInstance == NULL)
                        CError_FATAL(1592);
                    return;
                }
            }
            break;
        case DALIAS:
            if (obj->base.u.alias.member != NULL && obj->base.u.alias.member->type == (Type *)ctx->templateClass) {
                obj->base.u.alias.member = CClass_GetPathCopy(obj->base.u.alias.member, 1);
                obj->base.u.alias.member->type = (Type *)ctx->instance;
            }
            break;
        case DLOCAL:
        case DEXPR:
            CError_FATAL(1612);
        default:
            CError_FATAL(1615);
            break;
        case DABSOLUTE:
        case DINLINEFUNC:
            break;
    }
    CScope_AddObject(ctx->instance->nspace, obj->base.name, (ObjBase *)obj);
}

void CTemplateClass_0051c680(TemplateContext *ctx, Object *obj)
{
    TemplateClassDeclaration *found;
    Object *newobj;
    TemplateFunction *list;
    TemplateFunction *copy;
    struct TemplateFunction *function;
    struct TemplateFunction *copyFunction;
    TemplateInstantiationMapping *pending;
    FuncArg *arg;
    TemplateClassDeclaration *head;

    CError_ASSERT(1440, (list = obj->u.templateFunction) != NULL && list->params != NULL);

    head = ((TypeClassTemplate *)ctx->templateClass)->declarations;
    found = head;
    if (head != NULL) {
        do {
            if (found->kind == 7 && found->target.object == (ObjBase *)obj)
                break;
            found = found->next;
        } while (found != NULL);
    }

    copy = galloc(0x44);
    *copy = *list;
    function = templateFunctions;
    copyFunction = copy;
    copyFunction->next = function;
    templateFunctions = copy;
    copy->original = list;

    newobj = galloc(0x36);
    *newobj = *obj;
    newobj->u.templateFunction = copy;
    newobj->nspace = ctx->instance->nspace;

    CError_ASSERT(1465, ctx->modes.processingArgument == 0);
    ctx->modes.processingArgument = 1;
    ctx->parameterNIndex = ((TemplateParameterRecord *)list->params)->depth;

    if (found != NULL) {
        pending = CompilerTools_AllocatePool(0x14);
        pending->next = (TemplateInstantiationMapping *)ctx->mappings;
        pending->declaration = found;
        ctx->mappings = (struct TemplateInstantiationMapping *)pending;
        pending->object = (ObjBase *)newobj;
    } else {
        newobj->type =
            (Type *)CTemplateTools_ResolveType((TemplateContext *)ctx, (Type *)newobj->type, (UInt32 *)&newobj->qual);
    }
    ctx->modes.processingArgument = 0;

    CError_ASSERT(1477, newobj->type->type == TYPEFUNC);
    ((TypeFunc *)newobj->type)->flags |= 0x400;

    if ((((TypeFunc *)newobj->type)->flags & FUNC_IS_DTOR) != 0 && ctx->hasNewVBases != 0 && found == NULL) {
        CError_ASSERT(1484, ((TypeFunc *)newobj->type)->args != NULL);
        arg = CParser_NewFuncArg();
        arg->type = (Type *)&stsignedshort;
        arg->next = ((TypeFunc *)newobj->type)->args->next;
        ((TypeFunc *)newobj->type)->args->next = arg;
    }

    CScope_AddObject(ctx->instance->nspace, newobj->name, (ObjBase *)newobj);
}

void instantiate_objtype(TemplateContext *context, ObjType *type, HashNameNode *name)
{
    TemplateClassDeclaration *pendingType;
    ObjType *instantiatedType;
    TemplateInstantiationMapping *binding;
    NameSpaceObjectList *objects;
    NameSpaceObjectList *extra;

    pendingType = ((TypeClassTemplate *)context->templateClass)->declarations;
    while (pendingType != NULL) {
        if (pendingType->kind == 7 && pendingType->target.object == (ObjBase *)type)
            break;
        pendingType = pendingType->next;
    }

    instantiatedType = (ObjType *)galloc(10);
    *instantiatedType = *type;

    if (pendingType != NULL) {
        binding = (TemplateInstantiationMapping *)CompilerTools_AllocatePool(20);
        binding->next = (TemplateInstantiationMapping *)context->mappings;
        binding->declaration = pendingType;
        context->mappings = (struct TemplateInstantiationMapping *)binding;
        binding->object = (ObjBase *)instantiatedType;
    } else {
        instantiatedType->type =
            CTemplateTools_ResolveType((TemplateContext *)context, instantiatedType->type, &instantiatedType->qual);
    }

    objects = CScope_FindName(context->instance->nspace, name);
    if (objects != NULL && objects->object->otype == OT_TYPETAG) {
        CError_ASSERT(1394, objects->next == NULL);
        extra = (NameSpaceObjectList *)galloc(8);
        extra->object = objects->object;
        extra->next = NULL;
        objects->object = (ObjBase *)instantiatedType;
        objects->next = extra;
        return;
    } else {
        CScope_AddObject(context->instance->nspace, name, (ObjBase *)instantiatedType);
    }
}

void instantiate_ivars(TemplateContext *ctx, TypeClass *dst, TypeClassTemplate *src)
{
    ObjMemberVar *p;
    ObjMemberVar *m;
    struct TemplateClassDeclaration *q;
    struct TemplateInstantiationMapping *r;
    ObjMemberVar **out;

    p = src->base.ivars;
    out = &dst->ivars;
    for (; p != NULL; p = p->next) {
        CError_ASSERT(1321, !p->has_path);
        m = (ObjMemberVar *)galloc(sizeof(ObjMemberVar));
        *m = *p;
        for (q = ((TypeClassTemplate *)ctx->templateClass)->declarations; q != NULL; q = q->next) {
            if (q->kind == 7 && q->target.object == (ObjBase *)p) {
                r = (struct TemplateInstantiationMapping *)CompilerTools_AllocatePool(
                    sizeof(struct TemplateInstantiationMapping));
                r->next = ctx->mappings;
                r->declaration = q;
                ctx->mappings = r;
                r->object = (ObjBase *)m;
                break;
            }
        }
        if (q == NULL) {
            m->type = (Type *)CTemplateTools_ResolveType((TemplateContext *)ctx, m->type, &m->qual);
            if (TYPE(m->type)->size == 0) {
                CDecl_CompleteType(m->type);
                CanAllocObject(m->type);
            }
        }
        if (m->name != NULL && m->name != unnamed_name) {
            CScope_AddObject(dst->nspace, m->name, (ObjBase *)m);
        }
        *out = m;
        out = &m->next;
    }
}

void initialize_enum_constants(TemplateContext *context, struct TemplateClassDeclaration *object, TypeEnum *scope)
{
    int savedContext;
    Type *enumType;
    TypeClassTemplate *templateClass;
    ObjEnumConst *item;
    ObjEnumConst *binding;
    struct TemplateClassDeclaration *parameter;
    ENode *expression;

    enumType = object->target.type;
    templateClass = (TypeClassTemplate *)context->templateClass;
    for (parameter = templateClass->declarations; parameter != NULL; parameter = parameter->next) {
        if (parameter->kind != 3)
            continue;
        if ((item = (ObjEnumConst *)parameter->target.object)->type != enumType)
            continue;
        fn_00449d60();
        CError_SaveAndSetWrittenEntry(&parameter->source, &savedContext);
        for (binding = scope->enumlist; binding != NULL; binding = binding->next)
            if (binding->name == item->name)
                break;
        if (binding == NULL)
            CError_FATAL(1256);
        if (parameter->value.initializer != NULL) {
            expression = CTemplTool_DeduceExpr(context, parameter->value.initializer);
            if (expression->type != EINTCONST) {
                CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                fn_00449d60();
                CError_SetWrittenEntry(&savedContext);
                break;
            }
        } else {
            if (expression == NULL)
                CError_FATAL(1271);
            expression->data.intval = CInt64_Add(expression->data.intval, cint64_one);
        }
        binding->val = expression->data.intval;
        binding->type = expression->rtype;
        fn_00449d60();
        CError_SetWrittenEntry(&savedContext);
    }
    CDecl_ComputeUnderlyingEnumType(scope);
}

void instantiate_enum(TemplateContext *context, struct TemplateClassDeclaration *entry)
{
    ObjEnumConst **tail;
    ObjEnumConst *item;
    struct TemplateClassDeclaration *candidate;
    ObjEnumConst *firstItem;
    struct TemplateClassDeclaration *firstCandidate;
    ObjEnumConst *copy;
    struct TemplateInstantiationMapping *binding;
    TypeEnum *replacement;
    TypeEnum *original;
    ObjEnumConst *reference;
    TypeClassTemplate *templateClass;
    original = (TypeEnum *)entry->target.type;
    replacement = (TypeEnum *)galloc(22);
    memclrw(replacement, 22);
    replacement->type = TYPEENUM;
    replacement->size = original->size;
    replacement->nspace = context->instance->nspace;
    replacement->enumtype = original->enumtype;
    replacement->enumname = original->enumname;
    if (replacement->enumname != NULL) {
        CScope_DefineTypeTag(replacement->nspace, replacement->enumname, (Type *)replacement);
    }
    item = (firstItem = original->enumlist);
    tail = &replacement->enumlist;
    if (firstItem != NULL) {
        do {
            copy = (ObjEnumConst *)galloc(22);
            *copy = *item;
            *tail = copy;
            copy->next = NULL;
            copy->type = (Type *)replacement;
            CScope_AddObject(context->instance->nspace, copy->name, (ObjBase *)copy);
            copy = *tail;
            item = item->next;
            tail = &copy->next;
        } while (item != NULL);
    }
    templateClass = (TypeClassTemplate *)context->templateClass;
    candidate = (firstCandidate = templateClass->declarations);
    if (firstCandidate != NULL) {
        do {
            if (candidate->kind == 3 &&
                (reference = (ObjEnumConst *)candidate->target.object)->type == (Type *)original) {
                binding = (struct TemplateInstantiationMapping *)CompilerTools_AllocatePool(20);
                binding->next = context->mappings;
                binding->declaration = entry;
                context->mappings = binding;
                binding->replacement = replacement;
                return;
            }
            candidate = candidate->next;
        } while (candidate != NULL);
    }
    return;
}

void instantiate_bases(TemplateContext *context, TypeClass *instance, TypeClassTemplate *classTemplate)
{
    ClassList *resolvedTypeData = NULL;
    int savedEntry;
    ClassList *base;
    ClassList *current;
    struct TemplateClassDeclaration *declaration;
    ClassList *newBase;
    ClassList *instanceBase;
    ClassList **resolvedQualifiers;

    for (base = classTemplate->base.bases; base; base = base->next) {
        ClassList *baseCopy = galloc(sizeof(ClassList));
        *baseCopy = *base;
        baseCopy->next = NULL;
        if (instance->bases != NULL) {
            current = instance->bases;
            for (;;) {
                if (current->base == baseCopy->base) {
                    CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                    break;
                }
                if (current->next == NULL) {
                    current->next = baseCopy;
                    break;
                }
                current = current->next;
            }
        } else {
            instance->bases = baseCopy;
        }
    }

    for (declaration = ((TypeClassTemplate *)context->templateClass)->declarations; declaration;
         declaration = declaration->next) {
        if (declaration->kind == 4) {
            fn_00449d60();
            CError_SaveAndSetWrittenEntry(&declaration->source, &savedEntry);
            newBase = galloc(sizeof(ClassList));
            memclrw(newBase, sizeof(ClassList));
            newBase->base = (TypeClass *)CTemplateTools_ResolveType(context, declaration->target.type,
                                                                    (UInt32 *)(resolvedQualifiers = &resolvedTypeData));
            newBase->access = declaration->access;
            newBase->is_virtual = declaration->is_virtual;
            if (newBase->base->type == TYPECLASS) {
                if (newBase->base->size == 0) {
                    CDecl_CompleteType((Type *)newBase->base);
                    CanAllocObject((Type *)newBase->base);
                }
                if (CDecl_CheckNewBase(instance, newBase->base, newBase->is_virtual)) {
                    if (declaration->value.bases != NULL) {
                        base = classTemplate->base.bases;
                        instanceBase = instance->bases;
                        for (;;) {
                            if (base == NULL || instanceBase == NULL) {
                                CError_FATAL(1146);
                            }
                            if (base == declaration->value.bases) {
                                newBase->next = instanceBase->next;
                                instanceBase->next = newBase;
                                break;
                            }
                            base = base->next;
                            instanceBase = instanceBase->next;
                        }
                    } else {
                        newBase->next = instance->bases;
                        instance->bases = newBase;
                    }
                }
            } else {
                CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
            }
            fn_00449d60();
            CError_SetWrittenEntry(&savedEntry);
        }
    }

    if (instance->flags & CLASS_HAS_VBASES) {
        CDecl_SetVBaseOffsets(instance);
    }
}

static unsigned char qualtest(unsigned int a, unsigned int b)
{
    return (((a & 1) != 0) && ((b & 1) == 0)) || (((a & 2) != 0) && ((b & 2) == 0));
}

void CTemplateClass_0051cec0(TemplateContext *context, TypeClassTemplate *templateClass)
{
    ObjType *reference = galloc(sizeof(ObjNameSpace));
    memclrw(reference, sizeof(ObjNameSpace));
    reference->otype = OT_TYPETAG;
    reference->access = ACCESSPUBLIC;
    if (templateClass->templateParameters == NULL) {
        TypeClassExt800 *instance = create_class_template_instance(templateClass, NULL, NULL);
        instance->relatedClass = (Type *)context->instance;
        instance->base.nspace->parent = (NameSpace *)context->instance->nspace;
        reference->type = (Type *)instance; /* OT_TYPETAG carries a class type here. */
    } else {
        TypeClassTemplate *instance = galloc(sizeof(TypeClassTemplate));
        memclrw(instance, sizeof(*instance));
        instance->next = class_template_list;
        class_template_list = instance;
        instance->base = templateClass->base;
        instance->enclosingTemplate = (TypeClassTemplate *)context->templateClass;
        instance->relatedClass = (Type *)context->instance;
        instance->templateParameters = templateClass->templateParameters;
        instance->templateArgumentOverrides = NULL;
        instance->instances = NULL;
        instance->specializations = NULL;
        instance->declarations = templateClass->declarations;
        instance->virtualSlotCount = templateClass->virtualSlotCount;
        instance->structAlignment = templateClass->structAlignment;
        instance->hasVirtualFunction = templateClass->hasVirtualFunction;
        reference->type = (Type *)instance; /* OT_TYPETAG carries a class type here. */
    }
    CScope_AddObject(context->instance->nspace, templateClass->base.classname, (ObjBase *)reference);
}

/* Opaque state copied as six words. */
/* Records manipulated by this routine; intervening bytes are opaque. */

TypeClassTemplate *CTemplateClass_CreateClassTemplateDeclaration(TypeClass *owner, HashNameNode *arg1, short arg2)
{
    TypeClassTemplate *object;
    struct TemplateListRecord *record;
    struct TemplateListRecord *tail;

    object = (TypeClassTemplate *)galloc(90);
    memclrw(object, 90);
    object->next = class_template_list;
    class_template_list = object;
    object->enclosingTemplate = (TypeClassTemplate *)owner;
    object->templateParameters = NULL;
    CDecl_DefineClass(owner->nspace, arg1, &object->base, arg2, 0, 1);
    object->base.flags = FUNC_AUTO_GENERATED;

    record = (struct TemplateListRecord *)galloc(40);
    memclrw(record, 40);
    record->value_26 = 0;
    record->object = object;
    record->state = *CPrep_GetLastBufferedToken();

    if ((tail = (struct TemplateListRecord *)(*(TypeClassTemplate *)owner).declarations) != NULL) {
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = record;
    } else {
        (*(TypeClassTemplate *)owner).declarations = (struct TemplateClassDeclaration *)record;
    }

    return object;
}

/* Template class record in the candidate chain. */
/* Temporary list of matching candidates. */

char CTemplateClass_SelectSpecialization(CTStateElem *context, TypeClassTemplate **classType, CTStateElem **result)
{
    ClassTemplateSpecialization *entry;
    TemplateClassMatch *match;
    TemplateClassMatch *matches;

    {
        TypeClassExt800 *entry;
        for (entry = (TypeClassExt800 *)(*classType)->instances; entry != NULL; entry = entry->next) {
            if (((TypeClassExt800 *)entry)->instantiating != 0 ||
                ((TypeClassExt800 *)entry)->suppressImplicitInstantiation != 0) {
                CTStateElem *value = ((TypeClassExt800 *)entry)->templateArgumentOverride
                                         ? ((TypeClassExt800 *)entry)->templateArgumentOverride
                                         : ((TypeClassExt800 *)entry)->targs;
                if (CTemplTool_EqualArgs(context, value))
                    return 0;
            }
        }
    }
    matches = NULL;
    for (entry = (*classType)->specializations; entry != NULL; entry = entry->next) {
        if (match_specialization_arguments(entry, context, 0) != NULL) {
            match = (TemplateClassMatch *)CompilerTools_AllocatePool(8);
            match->next = matches;
            match->candidate = entry;
            matches = match;
        }
    }

    if (matches != NULL) {
        if (matches->next != NULL) {
            matches = remove_less_specialized_matches(matches);
            if (matches->next != NULL)
                CError_ReportError(ERR_AMBIGUOUS_USE_PARTIAL_SPECIALIZATION);
        }
        if (matches->candidate->type->templateParameters == NULL) {
            *classType = matches->candidate->type;
            *result = NULL;
            return 1;
        }
        *classType = matches->candidate->type;
        *result = match_specialization_arguments(matches->candidate, context, 1);
        return *result != NULL;
    }
    return 0;
}

struct TemplateClassMatch *remove_less_specialized_matches(struct TemplateClassMatch *list)
{
    int count;
    struct TemplateClassMatch **table;
    struct TemplateClassMatch *match;
    int i, j, index;
    int remove;
    int strictlyBetter;
    struct ClassTemplateSpecialization *candidate, *other;

    count = 0;
    for (match = list; match != NULL; match = match->next)
        count++;
    table = (struct TemplateClassMatch **)CompilerTools_AllocatePool(count * sizeof(*table));
    for (index = 0, match = list; match != NULL; match = match->next)
        table[index++] = match;
    for (i = 0; i < count; i++) {
        if (table[i] == NULL)
            continue;
        for (j = 0; j < count; j++) {
            if (table[j] == NULL || i == j)
                continue;
            other = table[j]->candidate;
            candidate = table[i]->candidate;
            remove = 0;
            if (match_template_arguments(candidate, other)) {
                strictlyBetter = 0;
                if (!match_template_arguments(other, candidate))
                    strictlyBetter = 1;
                if (strictlyBetter)
                    remove = 1;
            }
            if ((Boolean)remove)
                table[j] = NULL;
        }
    }
    list = NULL;
    for (i = 0; i < count; i++) {
        if (table[i] != NULL) {
            if (list == NULL)
                list = table[i];
            else
                table[j]->next = table[i];
            table[i]->next = NULL;
            j = i;
        }
    }
    return list;
}

unsigned char match_template_arguments(ClassTemplateSpecialization *arguments, ClassTemplateSpecialization *pattern)
{
    CTStateElem *argument;
    CTStateElem *patternArgument;
    int index;
    int matchIndex;
    struct TemplateMatchState state;
    if (CTemplTool_InitDeduceInfo(&state, pattern->type->templateParameters, NULL, 1) == 0)
        return 0;
    argument = arguments->arguments;
    patternArgument = pattern->arguments;
    for (;;) {
        if (argument == NULL) {
            if (patternArgument != NULL)
                CError_FATAL(796);
            index = 0;
            while (index < state.nslots) {
                if (state.slots[index].bound == 0)
                    return 0;
                index = index + 1;
            }
            return 1;
        }
        if (patternArgument == NULL)
            CError_FATAL(805);
        if (argument->pid.type != patternArgument->pid.type)
            CError_FATAL(806);
        if (argument->pid.type != 0) {
            if (qualtest(patternArgument->qualifiers, argument->qualifiers))
                return 0;
            if (argument->qualifiers != patternArgument->qualifiers &&
                patternArgument->argument.type->type != TYPEPOINTER &&
                (patternArgument->argument.type->type != TYPETEMPLATE ||
                 ((TypeIntegral *)patternArgument->argument.type)->integral != IT_BOOL))
                return 0;
            if (CTemplateFunc_MatchType(patternArgument->argument.type, patternArgument->qualifiers,
                                        argument->argument.type, argument->qualifiers, state.slots, 0) == 0)
                return 0;
        } else if (CTemplTool_IsTypeDepExpr(patternArgument->argument.expression) != 0) {
            matchIndex = CTemplateFunc_GetArgumentParameterIndex(patternArgument);
            if (matchIndex < 0)
                CError_FATAL(845);
            if (state.slots[matchIndex].bound != 0) {
                if (argument->argument.type != NULL) {
                    if (state.slots[matchIndex].argument.type == NULL ||
                        CTemplateTools_00517a40(argument->argument.expression,
                                                state.slots[matchIndex].argument.expression) == 0)
                        return 0;
                } else {
                    if (state.slots[matchIndex].argument.type != NULL ||
                        argument->pid.index != state.slots[matchIndex].pid.index)
                        return 0;
                }
            } else {
                state.slots[matchIndex].argument.expression = argument->argument.expression;
                state.slots[matchIndex].pid.index = argument->pid.index;
                state.slots[matchIndex].pid.type = 0;
                state.slots[matchIndex].bound = 1;
            }
        } else {
            if (argument->argument.type == NULL ||
                CTemplateTools_00517a40(argument->argument.expression, patternArgument->argument.expression) == 0)
                return 0;
        }
        argument = argument->next;
        patternArgument = patternArgument->next;
    }
}

CTStateElem *match_specialization_arguments(ClassTemplateSpecialization *arguments, CTStateElem *actual,
                                            char instantiate)
{
    CTStateElem *pattern;
    CTStateElem *candidate;
    long mask;
    long patternQualifiers;
    long actualQualifiers;
    int missingQualifier;
    long missing;
    int matched;
    CTStateElem *result;
    unsigned long candidateQualifiers;
    int index;
    struct TemplateMatchState state;

    if (!CTemplTool_InitDeduceInfo(&state, arguments->type->templateParameters, NULL, 1))
        return NULL;
    pattern = (CTStateElem *)arguments->arguments;
    candidate = actual;
    matched = 0;
    for (;;) {
        if (!pattern) {
            if (candidate)
                return NULL;
            patternQualifiers = 0;
            while (patternQualifiers < state.nslots) {
                if (!state.slots[patternQualifiers].bound)
                    return NULL;
                patternQualifiers = patternQualifiers + 1;
            }
            if (instantiate)
                result = CTemplateTools_CopySlotsToList(&state);
            else
                result = actual;
            return result;
        }
        if (!candidate)
            return NULL;
        if (pattern->pid.type != candidate->pid.type)
            return NULL;
        if (pattern->pid.type != 0) {
            if (CTemplateTools_IsDependentType(pattern->argument.type)) {
                actualQualifiers = candidateQualifiers = candidate->qualifiers;
                mask = patternQualifiers = pattern->qualifiers;
                missingQualifier = 0;
                missing = 1;
                if ((patternQualifiers & 1) != 0 && (actualQualifiers & 1) == 0)
                    missingQualifier = missingQualifier + 1;
                if (missingQualifier == 0) {
                    missingQualifier = 0;
                    if ((mask & 2) != 0 && (actualQualifiers & 2) == 0)
                        missingQualifier = 1;
                    if (missingQualifier == 0)
                        missing = 0;
                }
                if ((char)missing != 0)
                    return NULL;
                if (patternQualifiers != candidateQualifiers && (pattern->argument.type)->type != TYPEPOINTER &&
                    ((pattern->argument.type)->type != TYPETEMPLATE ||
                     ((TypeIntegral *)pattern->argument.type)->integral != IT_BOOL))
                    return NULL;
                if (!CTemplateFunc_MatchType(pattern->argument.type, patternQualifiers, candidate->argument.type,
                                             candidateQualifiers, state.slots, 0))
                    return NULL;
            } else {
                if (!iscpp_typeequal(pattern->argument.type, candidate->argument.type) ||
                    pattern->qualifiers != candidate->qualifiers)
                    return NULL;
            }
        } else {
            if (CTemplTool_IsTypeDepExpr(pattern->argument.expression)) {
                index = CTemplateFunc_GetArgumentParameterIndex(pattern);
                if (index < 0)
                    CError_FATAL(749);
                if (state.slots[index].bound != 0) {
                    if (!CTemplateTools_00517a40(candidate->argument.expression,
                                                 state.slots[index].argument.expression))
                        return NULL;
                } else {
                    state.slots[index].argument.expression = candidate->argument.expression;
                    state.slots[index].pid.type = 0;
                    state.slots[index].bound = 1;
                }
            } else if (!CTemplateTools_00517a40(candidate->argument.expression, pattern->argument.expression))
                return NULL;
        }
        pattern = pattern->next;
        candidate = candidate->next;
        matched = matched + 1;
    }
}

struct TemplateComparisonEntry;
/* Class-template data extending the ordinary class type. */
/* Deferred member declaration and its source position. */

void CTemplateClass_ParseClassDeclaration(TemplateScopeState *scope, TemplateParameterRecord *parameters, short access,
                                          SInt32 *state)
{
    struct TypeClass *existing;
    TypeClassTemplate *record;
    struct TemplateMemberData *entry;
    struct TemplateMemberData **tail;
    TypeClassTemplate *owner;
    DeclInfo context;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    existing = (struct TypeClass *)CScope_GetTagType(scope->scope->parent, data_00587fa0);
    if (existing == NULL) {
        record = (TypeClassTemplate *)galloc(sizeof(*record));
        memclrw(record, sizeof(*record));
        record->next = class_template_list;
        class_template_list = record;
        record->templateParameters = parameters;
        CDecl_DefineClass(scope->scope->parent, data_00587fa0, &record->base, access, 0, 1);
        record->base.flags = CLASS_IS_TEMPL;
        tk = CPrepTokenizer_GetNextToken();
        if (scope->scope->parent->theclass != NULL && (scope->scope->parent->theclass->flags & CLASS_IS_TEMPL) != 0) {
            record->enclosingTemplate = (TypeClassTemplate *)scope->scope->parent->theclass;
            entry = galloc(sizeof(*entry));
            memclrw(entry, sizeof(*entry));
            entry->kind = 0;
            entry->record = record;
            owner = record->enclosingTemplate;
            entry->sourcePosition = *CPrep_GetLastBufferedToken();
            if ((tail = (struct TemplateMemberData **)owner->declarations) != NULL) {
                while (*tail != NULL) {
                    tail = &(*tail)->next;
                }
                *tail = entry;
            } else {
                owner->declarations = (struct TemplateClassDeclaration *)entry;
            }
        }
    } else {
        if (existing->type != TYPECLASS || (existing->flags & CLASS_IS_TEMPL) == 0) {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, data_00587fa0->name);
            return;
        }
        record = (TypeClassTemplate *)existing;
        if (CTemplTool_EqualParams(record->templateParameters, parameters, 0) == 0) {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, record->base.classname->name);
            return;
        }
        CTemplTool_MergeDefaultArgs(record->templateParameters, parameters);
        tk = CPrepTokenizer_GetNextToken();
        if ((record->base.flags & CLASS_COMPLETED) != 0 && tk != ';') {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, record->base.classname->name);
            return;
        }
        if (tk != ';') {
            CTemplTool_EqualParams(record->templateParameters, parameters, 1);
        }
    }
    switch (tk) {
        case ':':
        case '{':
        case TK_UU_DECLSPEC:
            record->base.nspace->parent = scope->scope;
            scope->linkedNamespace = record->base.nspace;
            record->structAlignment = copts.structalignment;
            memclrw(&context, sizeof(context));
            context.browseFile = CPrep_GetPFile();
            CPrep_GetBrowseFilePosition(&context.sourceFile, &context.sourceLine);
            context.sourceLine = *state;
            context.pendingClass = &record->base;
            CDecl_ParseClass(&context, access, 1, 0);
            if (tk != ';') {
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
            }
            CBrowse_RecordClassLocation(&record->base, context.sourceFile, context.sourceLine,
                                        CPrep_GetCurrentTextOffset() + 1);
            break;
        default:
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        case ';':
            break;
    }
}

/* Template-specific data following the class type. */
/* Parser state for a class declaration. */

void CTemplateClass_ParsePartialSpecialization(TemplateScopeState *scope, struct TemplateParameterRecord *parameters,
                                               short access, SInt32 *position)
{
    TypeClass *type;
    TypeClassTemplate *templateClass;
    TemplateParameterRecord *parameter;
    ClassTemplateSpecialization *specialization;
    CTStateElem *argument;
    TemplateParameterRecord *templateParameter;
    TypeClassTemplate *instance;
    CTStateElem *arguments;
    DeclInfo declaration;
    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    type = (TypeClass *)CScope_GetTagType(scope->scope->parent, data_00587fa0);
    if (type == NULL) {
        CError_ReportError(ERR_UNDEFINED_IDENTIFIER, data_00587fa0->name);
        return;
    }
    if (type->type != TYPECLASS || (type->flags & CLASS_IS_TEMPL) == 0) {
        CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, data_00587fa0->name);
        return;
    }
    templateClass = (TypeClassTemplate *)type;
    tk = CPrepTokenizer_GetNextToken();
    CE_ASSERT(tk != '<', CError_FATAL(461));
    parameter = parameters;
    for (; parameter != NULL; parameter = parameter->next) {
        if (parameter->isTypeParameter != 0) {
            if (parameter->value == NULL) {
                continue;
            }
            CError_ReportError(ERR_ILLEGAL_PARTIAL_SPECIALIZATION);
            break;
        }
        if (parameter->defaultValue.expression != NULL) {
            CError_ReportError(ERR_ILLEGAL_PARTIAL_SPECIALIZATION);
            break;
        }
    }
    arguments = CTemplateNew_ParseTemplateArguments((TemplateParameterRecord *)templateClass->templateParameters, 0);
    tk = CPrepTokenizer_GetNextToken();
    argument = arguments;
    templateParameter = templateClass->templateParameters;
    for (;;) {
        if (argument == NULL) {
            if (templateParameter == NULL) {
                break;
            }
            CError_ReportError(ERR_ILLEGAL_PARTIAL_SPECIALIZATION);
            return;
        }
        if (templateParameter == NULL) {
            CError_ReportError(ERR_ILLEGAL_PARTIAL_SPECIALIZATION);
            return;
        }
        if (templateParameter->isTypeParameter != (char)argument->pid.type) {
            CError_ReportError(ERR_ILLEGAL_PARTIAL_SPECIALIZATION);
            return;
        }
        argument = argument->next;
        templateParameter = templateParameter->next;
    }
    specialization = templateClass->specializations;
    if (specialization != NULL) {
        do {
            if (CTemplTool_EqualParams(specialization->type->templateParameters, parameters, 0) != 0 &&
                CTemplTool_EqualArgs(specialization->arguments, arguments) != 0) {
                break;
            }
            specialization = specialization->next;
        } while (specialization != NULL);
    }
    if (specialization == NULL) {
        instance = (TypeClassTemplate *)galloc(90);
        memclrw(instance, 90);
        instance->templateParameters = parameters;
        CDecl_DefineClass(scope->scope->parent, templateClass->base.classname, &instance->base, access, 0, 0);
        instance->base.flags = CLASS_IS_TEMPL;
        instance->replacementTemplate = (Type *)templateClass;
        specialization = (ClassTemplateSpecialization *)galloc(12);
        memclrw(specialization, 12);
        specialization->type = instance;
        specialization->arguments = CTemplateTools_CopyCTStateElemList(arguments);
        specialization->next = templateClass->specializations;
        templateClass->specializations = specialization;
    } else {
        if ((specialization->type->base.flags & CLASS_COMPLETED) != 0 && tk != ';') {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, (int)templateClass->base.classname + 10);
            return;
        }
        if (tk == ':' || tk == '{') {
            CTemplTool_EqualParams(specialization->type->templateParameters, parameters, 1);
        }
    }
    switch (tk) {
        case ':':
        case '{':
            specialization->type->base.nspace->parent = scope->scope;
            scope->linkedNamespace = specialization->type->base.nspace;
            instance = specialization->type;
            instance->structAlignment = copts.structalignment;
            memclrw(&declaration, 92);
            declaration.browseFile = CPrep_GetPFile();
            CPrep_GetBrowseFilePosition(&declaration.sourceFile, &declaration.sourceLine);
            declaration.sourceLine = *position;
            declaration.pendingClass = &instance->base;
            CDecl_ParseClass(&declaration, access, 1, 0);
            if (tk != ';') {
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
            }
            CBrowse_RecordClassLocation(&instance->base, declaration.sourceFile, declaration.sourceLine,
                                        CPrep_GetCurrentTextOffset() + 1);
            break;
        default:
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        case ';':
            break;
    }
}

/* Fixed-size hash-name header, without the variable-length name. */
/* Two-word payload associated with a keyed list entry. */

struct KeyedEntry *CTemplateClass_AddTemplateArgumentOverride(TypeClassTemplate *owner, Object *key, FOI *name,
                                                              struct PrepTokenBuffer *payload)
{
    enum { KeyedEntryAllocationSize = 0x2a };
    KeyedEntry *entry;

    entry = owner->templateArgumentOverrides;
    while (entry != NULL) {
        if (entry->key == key) {
            CError_ReportError(ERR_OBJECT_REDEFINED, key);
            return entry;
        }
        entry = entry->next;
    }
    entry = (KeyedEntry *)galloc(KeyedEntryAllocationSize);
    memclrw((unsigned char *)entry, KeyedEntryAllocationSize);
    entry->next = owner->templateArgumentOverrides;
    owner->templateArgumentOverrides = entry;
    entry->templateParameters = NULL;
    entry->key = key;
    entry->name = *name;
    entry->payload = *payload;
    return entry;
}

TypeClassExt800 *CTemplateClass_GetInstance(TypeClassTemplate *cls, CTStateElem *key, CTStateElem *flag)
{
    TypeClassExt800 *instance = cls->instances;

    while (instance != NULL) {
        if (flag != NULL)
            CError_FATAL(353);
        if (CTemplTool_EqualArgs(key, instance->templateArgumentOverride ? instance->templateArgumentOverride
                                                                         : instance->targs))
            return instance;
        instance = instance->next;
    }
    return create_class_template_instance(cls, key, flag);
}

TypeClassExt800 *create_class_template_instance(TypeClassTemplate *definition, void *argument, void *alternate_argument)
{
    TypeClassExt800 *instance;
    NameSpace *scope;
    ObjType *entry;
    NameSpace *parent;
    HashNameNode *name;

    if (definition->replacementTemplate != 0U)
        CError_FATAL(288);

    instance = (TypeClassExt800 *)galloc(74U);
    memclrw(instance, 74U);
    instance->next = definition->instances;
    definition->instances = instance;

    if (definition->templateParameters != 0U) {
        CTStateElem *name_argument = (CTStateElem *)((alternate_argument != NULL) ? alternate_argument : argument);
        name = CMangler_TemplateInstanceName(definition->base.classname, name_argument);
    } else {
        name = definition->base.classname;
    }

    instance->targs = argument;
    instance->templateArgumentOverride = alternate_argument;
    instance->relatedClass = (Type *)definition->relatedClass;

    scope = CScope_NewListNameSpace(name, 1);
    scope->theclass = &instance->base;

    if (definition->enclosingTemplate != 0U && definition->relatedClass != NULL) {
        scope->parent = ((TypeClass *)definition->relatedClass)->nspace;
    } else {
        parent = definition->base.nspace->parent;
        while (parent->is_templ != 0U)
            parent = parent->parent;
        scope->parent = parent;
    }

    instance->base.type = TYPECLASS;
    instance->base.flags = 0x800U;
    instance->base.nspace = scope;
    instance->base.classname = definition->base.classname;
    instance->base.mode = definition->base.mode;
    instance->base.eflags = definition->base.eflags;
    instance->classTemplate = (Type *)definition;

    entry = (ObjType *)galloc(6U);
    memclrw(entry, 6U);
    entry->otype = OT_TYPETAG;
    entry->access = ACCESSPUBLIC;
    entry->type = (Type *)instance;

    CScope_AddObject(scope, definition->base.classname, (ObjBase *)entry);
    return instance;
}

unsigned char CTemplateClass_CompleteClassLayout(TypeClassTemplate *state, ClassLayoutInput *values)
{
    unsigned char byteValue;
    state->virtualSlotCount = values->count;
    byteValue = values->hasVirtualFunction;
    state->hasVirtualFunction = byteValue;
    state->base.flags |= CLASS_COMPLETED;
    return byteValue;
}

void CTemplateClass_AppendObjectDeclaration(TypeClassTemplate *ctx, Object *obj)
{
    struct TemplateObjectDeclaration *record;

    record = galloc(sizeof(*record));
    memclrw(record, sizeof(*record));
    record->kind = 7;
    record->argument = obj;
    record->data = *CPrep_GetLastBufferedToken();

    if (ctx->declarations != NULL) {
        struct TemplateObjectDeclaration *last = (struct TemplateObjectDeclaration *)ctx->declarations;
        while (last->next != NULL)
            last = last->next;
        last->next = record;
    } else {
        ctx->declarations = (struct TemplateClassDeclaration *)record;
    }
}

void CTemplateClass_AppendExpressionRecord(TypeClassTemplate *type, Object *object, ENode *expression)
{
    struct TemplateExpressionRecord *record;
    record = galloc(40U);
    memclrw(record, 40U);
    record->kind = 5U;
    record->declaration.object = object;
    record->expression = fn_00513040(expression, 1U);
    record->parserState = *CPrep_GetLastBufferedToken();
    if (type->declarations != NULL) {
        struct TemplateExpressionRecord *last = (struct TemplateExpressionRecord *)type->declarations;
        while (last->next != NULL)
            last = last->next;
        last->next = record;
    } else {
        type->declarations = (struct TemplateClassDeclaration *)record;
    }
}

void CTemplateClass_AppendEnumConstDeclaration(TypeClassTemplate *self, ObjEnumConst *a, ENode *str)
{
    struct TemplateExpressionRecord *p;
    struct TemplateExpressionRecord *r;

    r = galloc(sizeof(struct TemplateExpressionRecord));
    memclrw(r, sizeof(struct TemplateExpressionRecord));
    r->kind = 3;
    r->declaration.object = (Object *)a;
    r->expression = str ? fn_00513040(str, 1) : NULL;
    r->parserState = *CPrep_GetLastBufferedToken();

    if (self->declarations != NULL) {
        p = (struct TemplateExpressionRecord *)self->declarations;
        while (p->next != NULL)
            p = p->next;
        p->next = r;
    } else {
        self->declarations = (struct TemplateClassDeclaration *)r;
    }
}

/* Saved opaque template context returned by CPrep_GetLastBufferedToken. */
/* Pending template instantiation, linked in declaration order. */

void CTemplateClass_AppendEnumDeclaration(TypeClassTemplate *type, TypeEnum *value)
{
    struct PendingTemplateInstantiation *record;
    struct PendingTemplateInstantiation *tail;
    record = galloc(40U);
    memclrw(record, 40U);
    record->kind = 1;
    record->enumType = value;
    record->context = *CPrep_GetLastBufferedToken();
    if (type->declarations != NULL) {
        tail = (struct PendingTemplateInstantiation *)type->declarations;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = record;
    } else {
        type->declarations = (struct TemplateClassDeclaration *)record;
    }
}

/* Private list records and the containing object's unexamined storage. */
unsigned int CTemplateClass_PrependTemplateRecordEntry(TypeClassTemplate *list, Type *value, unsigned char value24,
                                                       unsigned char value25)
{
    struct ClassList *last;
    struct TemplateRecordEntry *entry;
    struct TemplateRecordEntry *oldHead;

    if ((last = list->base.bases) != NULL) {
        while (last->next != NULL) {
            last = last->next;
        }
    }
    entry = galloc(sizeof(*entry));
    memclrw(entry, sizeof(*entry));
    entry->tag = 4;
    entry->value = value;
    entry->lastBase = last;
    entry->access = value24;
    entry->is_virtual = value25;
    entry->sourcePosition = *CPrep_GetLastBufferedToken();
    oldHead = (struct TemplateRecordEntry *)list->declarations;
    entry->next = oldHead;
    list->declarations = (struct TemplateClassDeclaration *)entry;
    return (unsigned int)oldHead;
}

void CTemplateClass_AddDeferredFunctionDeclaration(TypeClassTemplate *classTemplate, DeclInfo *declInfo)
{
    NewFunc *function;
    TemplateExpressionRecord *declaration;
    TemplateExpressionRecord *tail;
    TypeFunc *functionType;

    function = galloc(sizeof(*function));
    memclrw(function, sizeof(*function));

    if (tk == '{' && declInfo->dtype->type == TYPEFUNC) {
        declInfo->qual |= Q_INLINE;
        functionType = (TypeFunc *)declInfo->dtype;
        functionType->flags |= (FUNC_DEFINED | 0x8000000);
        function->ot = function_fileinfo;
        CPrep_SaveFunctionBodyTokens(&function->bodyTokens, NULL, 1);
        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == ';')
            tk = CPrepTokenizer_GetNextToken();
        else
            tk = ';';
    }

    CDecl_CopyDeclInfoToNewFunc(function, declInfo);

    declaration = galloc(sizeof(*declaration));
    memclrw(declaration, sizeof(*declaration));
    declaration->kind = 2;
    declaration->declaration.functionDeclaration = function;
    declaration->parserState = *CPrep_GetLastBufferedToken();

    if ((tail = (TemplateExpressionRecord *)classTemplate->declarations) != NULL) {
        while (tail->next)
            tail = tail->next;
        tail->next = declaration;
    } else {
        classTemplate->declarations = (struct TemplateClassDeclaration *)declaration;
    }
}

/* Namespace data copied without its trailing status bytes. */
/* Record in the class's appended instance chain. */
/* ClassChainEntry is defined in structs/ClassChainEntry.h. */

void CTemplateClass_AppendFuncDeclaration(TypeClassTemplate *owner, TypeTemplDep *value, unsigned char kind)
{
    struct ClassChainEntry *entry;
    struct ClassChainEntry *tail;
    entry = galloc(40U);
    memclrw(entry, 40U);
    entry->type = TYPEFUNC;
    entry->value = (UInt32)value;
    entry->kind = kind;
    entry->sourcePosition = *CPrep_GetLastBufferedToken();
    if (owner->declarations != NULL) {
        for (tail = (struct ClassChainEntry *)owner->declarations; tail->next != NULL; tail = tail->next)
            ;
        tail->next = entry;
    } else {
        owner->declarations = (struct TemplateClassDeclaration *)entry;
    }
}

TypeClassTemplate *CTemplateClass_ResolveRelatedClass(TypeClassTemplate *record)
{
    if (record->relatedClass != 0U) {
        record = (TypeClassTemplate *)record->base.nspace->theclass;
        if ((record->base.flags & 256U) == 0U)
            CError_FATAL(42);
    }
    return record;
}

char *CTemplateClass_ParseDouble(char *value, double *result, char *error)
{
    char *status;

    errno = 0;
    *result = strtod(value, &status);
    *error = errno != 0;
    return status;
}

#pragma opt_propagation reset
