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
    TemplClass *templateClass;
    Type *instantiatedType;
    struct CParseSave *savedState;
    struct DefAction *objectMapping;
    Object *object;
    ENode *initializer;
    struct DefAction *typeMapping;
    unsigned char access;
    TypeTemplDep *templateType;
    struct DefAction *baseMapping;
    char savedMode;
    TemplClassInst *classInstance;
    struct TemplateClassDeclaration *memberDeclaration;
    struct ObjectReferenceEntry templateSave;
    CScopeSave scopeSave;
    TemplClass *resolvedTemplate;
    ClassLayout classInfo;
    TypeDeduce instantiation;
    int sourceSave;
    CTStateElem *resolvedArgs;
    UInt32 typeResult;
    CE_ASSERT((theclass->flags & CLASS_IS_TEMPL_INST) == 0, CError_FATAL(1907));
    if ((theclass->flags & CLASS_COMPLETED) != 0)
        return 1;
    if ((classInstance = (TemplClassInst *)theclass)->is_specialized != 0)
        return 0;
    templateClass = classInstance->templ;
    if (templateClass->inst_parent != NULL) {
        templateClass = (TemplClass *)templateClass->theclass.nspace->theclass;
        CE_ASSERT((templateClass->theclass.flags & CLASS_IS_TEMPL) == 0, CError_FATAL(42));
    }
    resolvedTemplate = templateClass;
    if (templateClass->pspecs != NULL &&
        CTemplateClass_SelectSpecialization(classInstance->inst_args, &resolvedTemplate, &resolvedArgs) != 0) {
        CE_ASSERT(classInstance->oargs != 0, CError_FATAL(1926));
        classInstance->templ = resolvedTemplate;
        classInstance->oargs = classInstance->inst_args;
        classInstance->inst_args = resolvedArgs;
    }
    if ((resolvedTemplate->theclass.flags & CLASS_COMPLETED) == 0)
        return 0;
    if (classInstance->is_instantiated != 0)
        return 0;
    classInstance->is_instantiated = 1;
    BE_elf_SaveScopeAndEnterClass(theclass, &scopeSave);
    FunctionCalls_PushObjectReferenceEntry(&templateSave, theclass, NULL);
    savedState = data_00588240;
    data_00588240 = NULL;
    memclrw(&instantiation, sizeof(instantiation));
    instantiation.tmclass = resolvedTemplate;
    instantiation.inst = classInstance;
    instantiation.params = resolvedTemplate->templ__params;
    instantiation.args = classInstance->inst_args;
    CE_ASSERT(resolvedTemplate->theclass.sominfo != 0, CError_FATAL(1958));
    CE_ASSERT(resolvedTemplate->theclass.objcinfo != 0, CError_FATAL(1959));
    CE_ASSERT(resolvedTemplate->theclass.vtable != 0, CError_FATAL(1960));
    classInstance->theclass.flags |= resolvedTemplate->theclass.flags & 8312;
    instantiate_bases(&instantiation, &classInstance->theclass, resolvedTemplate);
    instantiation.hasNewVBases = (classInstance->theclass.flags & CLASS_HAS_VBASES) != 0 &&
                                 (resolvedTemplate->theclass.flags & CLASS_HAS_VBASES) == 0;
    for (declaration = resolvedTemplate->actions; declaration != NULL; declaration = declaration->next) {
        switch (declaration->kind) {
            case 0:
                fn_00449d60();
                CError_SaveAndSetWrittenEntry(&declaration->source, &sourceSave);
                CTemplateClass_0051cec0(&instantiation, (TemplClass *)declaration->target.type);
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
    instantiate_ivars(&instantiation, &classInstance->theclass, resolvedTemplate);
    instantiate_namespace_objects(&instantiation, &classInstance->theclass, &resolvedTemplate->theclass);
    CE_ASSERT(resolvedTemplate->theclass.friends != 0, CError_FATAL(2016));
    for (memberDeclaration = resolvedTemplate->actions; memberDeclaration != NULL;
         memberDeclaration = memberDeclaration->next) {
        switch (memberDeclaration->kind) {
            case 0:
                break;
            case 1:
                for (typeMapping = instantiation.defActions; typeMapping != NULL; typeMapping = typeMapping->next) {
                    if (typeMapping->action == memberDeclaration) {
                        fn_00449d60();
                        CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                        initialize_enum_constants(&instantiation, memberDeclaration, typeMapping->enumtype);
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
                for (objectMapping = instantiation.defActions;; objectMapping = objectMapping->next) {
                    CE_ASSERT(objectMapping == 0, CError_FATAL(2047));
                    if (objectMapping->action == memberDeclaration) {
                        fn_00449d60();
                        CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                        object = (Object *)objectMapping->refobj;
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
                    CScope_AddClassUsingDeclaration(TYPE_CLASS(instantiation.inst), (TypeClass *)instantiatedType,
                                                    templateType->u.qual.name, access);
                }
                fn_00449d60();
                CError_SetWrittenEntry(&sourceSave);
                break;
            case 7:
                for (baseMapping = instantiation.defActions;; baseMapping = baseMapping->next) {
                    CE_ASSERT(baseMapping == 0, CError_FATAL(2067));
                    if (baseMapping->action == memberDeclaration) {
                        fn_00449d60();
                        CError_SaveAndSetWrittenEntry(&memberDeclaration->source, &sourceSave);
                        instantiate_object_type(&instantiation, memberDeclaration, baseMapping->refobj);
                        fn_00449d60();
                        CError_SetWrittenEntry(&sourceSave);
                        break;
                    }
                }
        }
    }
    memclrw(&classInfo, 14);
    classInfo.lex_order_count = (SInt16)resolvedTemplate->lex_order_count;
    classInfo.has_vtable = resolvedTemplate->flags;
    savedMode = copts.structalignment;
    copts.structalignment = resolvedTemplate->align;
    CDecl_CompleteClass(&classInfo, &classInstance->theclass);
    copts.structalignment = savedMode;
    CTemplateTools_PopObjectReferenceEntry(&templateSave);
    CScope_RestoreScope(&scopeSave);
    data_00588240 = savedState;
    return 1;
}

void instantiate_friend_declaration(TypeDeduce *ctx, struct TemplateDeclarationData *declaration)
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
        scope = CScope_FindGlobalNS(TYPE_CLASS(ctx->inst)->nspace);
        BE_elf_SaveAndSetScope(scope, (CScopeSave *)&savedScope); /* original stack-slot view */
        object = CDecl_GetFunctionObject(&instance, NULL, &result, 0);
        CScope_RestoreScope((CScopeSave *)&savedScope); /* original stack-slot view */
        if (object != NULL) {
            CDecl_AddFriend(TYPE_CLASS(ctx->inst), object, NULL);
            if (declaration->inlineTokenBuffer.tokens)
                CInline_AddFunctionPrecNode(object, TYPE_CLASS(ctx->inst), &declaration->inlineLocation,
                                            &declaration->inlineTokenBuffer, 0);
        } else {
            CError_ReportError(ERR_ILLEGAL_FRIEND_DECLARATION);
        }
    } else {
        if (instance.dtype->type != TYPECLASS)
            CError_FATAL(1881);
        CDecl_AddFriend(TYPE_CLASS(ctx->inst), NULL, instance.dtype);
    }
}

void instantiate_namespace_objects(TypeDeduce *map, TypeClass *unused, TypeClass *obj)
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
                        if (copy->bases != NULL && copy->bases->type == (Type *)map->tmclass) {
                            copy->bases = CClass_GetPathCopy(copy->bases, 1);
                            copy->bases->type = (Type *)map->inst;
                        }
                        CScope_AddObject(TYPE_CLASS(map->inst)->nspace, copy->member.name, (ObjBase *)&copy->member);
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

void instantiate_object_type(TypeDeduce *context, TemplateClassDeclaration *function, ObjBase *object)
{
    if (object->otype == OT_MEMBERVAR) {
        OBJ_MEMBER_VAR(object)->type = CTemplateTools_ResolveType((TypeDeduce *)context, OBJ_MEMBER_VAR(object)->type,
                                                                  &OBJ_MEMBER_VAR(object)->qual);
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
            CTemplateTools_ResolveType((TypeDeduce *)context, OBJ_TYPE(object)->type, &OBJ_TYPE(object)->qual);
        return;
    }
    if (object->otype != OT_OBJECT)
        CError_FATAL(1661);
    {
        Object *currentFunction = (Object *)function->target.object;
        if (IS_TYPE_FUNC(currentFunction->type) && (TYPE_FUNC(currentFunction->type)->flags & 0x400)) {
            currentFunction = (Object *)currentFunction->u.templateFunction;
            CError_ASSERT(1671, currentFunction != NULL);
            CError_ASSERT(1672, !context->processingArgument);
            context->processingArgument = 1;
            context->nindex = ((TypeBitfield *)((TemplateFunction *)currentFunction)->params)->offset;
            OBJECT(object)->type =
                CTemplateTools_ResolveType((TypeDeduce *)context, OBJECT(object)->type, &OBJECT(object)->qual);
            context->processingArgument = 0;
            CError_ASSERT(1677, IS_TYPE_FUNC(OBJECT(object)->type));
            TYPE_FUNC(OBJECT(object)->type)->flags |= 0x400;
            return;
        }
        OBJECT(object)->type =
            CTemplateTools_ResolveType((TypeDeduce *)context, OBJECT(object)->type, &OBJECT(object)->qual);
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
                        CScope_AddObject(TYPE_CLASS(context->inst)->nspace, OBJECT(object)->name,
                                         (ObjBase *)OBJECT(object));
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

void instantiate_template_object(TypeDeduce *ctx, Object *templ)
{
    Boolean needsNewNamespace = 1;
    ObjectTemplated *obj;
    TemplateClassDeclaration *matchingInstance;
    TemplateClassDeclaration *instance;
    DefAction *link;

    if (templ->nspace != TYPE_CLASS(ctx->tmclass)->nspace) {
        if (templ->datatype != DALIAS)
            CError_FATAL(1511);
        needsNewNamespace = 0;
    }
    if (templ->type->type == TYPEFUNC && (((TypeFunc *)templ->type)->flags & 0x400) != 0) {
        CTemplateClass_0051c680(ctx, templ);
        return;
    }
    for (matchingInstance = ((TemplClass *)ctx->tmclass)->actions; matchingInstance != NULL;
         matchingInstance = matchingInstance->next) {
        if (matchingInstance->kind == 7 && matchingInstance->target.object == (ObjBase *)templ)
            break;
    }
    obj = galloc(sizeof(ObjectTemplated));
    obj->object = *templ;
    if (matchingInstance != NULL) {
        link = CompilerTools_AllocatePool(0x14);
        link->next = ctx->defActions;
        link->action = matchingInstance;
        ctx->defActions = (struct DefAction *)link;
        link->refobj = (ObjBase *)obj;
    } else {
        obj->object.type =
            (Type *)CTemplateTools_ResolveType(ctx, (Type *)obj->object.type, (UInt32 *)&obj->object.qual);
    }
    if (needsNewNamespace)
        obj->object.nspace = TYPE_CLASS(ctx->inst)->nspace;
    obj->object.qual |= Q_IS_TEMPLATED;
    obj->parent = templ;
    if (obj->object.type->type == TYPEFUNC)
        ((TypeFunc *)obj->object.type)->flags &= ~FUNC_DEFINED;
    switch (obj->object.datatype) {
        case DDATA:
            obj->object.u.data.linkname = NULL;
            for (instance = ((TemplClass *)ctx->tmclass)->actions; instance != NULL; instance = instance->next) {
                if (instance->kind == 5 && instance->target.object == (ObjBase *)templ) {
                    link = CompilerTools_AllocatePool(0x14);
                    link->next = ctx->defActions;
                    link->action = instance;
                    ctx->defActions = (struct DefAction *)link;
                    link->refobj = (ObjBase *)obj;
                    break;
                }
            }
            break;
        case DFUNC:
        case DVFUNC:
            obj->object.u.func.linkname = NULL;
            if (obj->object.type->type != TYPEFUNC)
                CError_FATAL(1574);
            if (obj->object.u.func.u != NULL || obj->object.u.func.defargdata != NULL)
                CError_FATAL(1575);
            if ((((TypeFunc *)obj->object.type)->flags & FUNC_IS_DTOR) != 0 && ctx->hasNewVBases != 0 &&
                matchingInstance == NULL) {
                FuncArg *arg;
                if (((TypeFunc *)obj->object.type)->args == NULL)
                    CError_FATAL(1581);
                arg = CParser_NewFuncArg();
                arg->type = (Type *)&stsignedshort;
                arg->next = ((FuncArg *)((TypeFunc *)obj->object.type)->args)->next;
                ((FuncArg *)((TypeFunc *)obj->object.type)->args)->next = arg;
            }
            if ((((TypeFunc *)obj->object.type)->flags & FUNC_CONVERSION) != 0) {
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
            if (obj->object.u.alias.member != NULL && obj->object.u.alias.member->type == (Type *)ctx->tmclass) {
                obj->object.u.alias.member = CClass_GetPathCopy(obj->object.u.alias.member, 1);
                obj->object.u.alias.member->type = (Type *)ctx->inst;
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
    CScope_AddObject(TYPE_CLASS(ctx->inst)->nspace, obj->object.name, (ObjBase *)obj);
}

void CTemplateClass_0051c680(TypeDeduce *ctx, Object *obj)
{
    TemplateClassDeclaration *found;
    Object *newobj;
    TemplateFunction *list;
    TemplateFunction *copy;
    struct TemplateFunction *function;
    struct TemplateFunction *copyFunction;
    DefAction *pending;
    FuncArg *arg;
    TemplateClassDeclaration *head;

    CError_ASSERT(1440, (list = obj->u.templateFunction) != NULL && list->params != NULL);

    head = ((TemplClass *)ctx->tmclass)->actions;
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
    newobj->nspace = TYPE_CLASS(ctx->inst)->nspace;

    CError_ASSERT(1465, ctx->processingArgument == 0);
    ctx->processingArgument = 1;
    ctx->nindex = ((TemplateParameterRecord *)list->params)->depth;

    if (found != NULL) {
        pending = CompilerTools_AllocatePool(0x14);
        pending->next = (DefAction *)ctx->defActions;
        pending->action = found;
        ctx->defActions = (struct DefAction *)pending;
        pending->refobj = (ObjBase *)newobj;
    } else {
        newobj->type =
            (Type *)CTemplateTools_ResolveType((TypeDeduce *)ctx, (Type *)newobj->type, (UInt32 *)&newobj->qual);
    }
    ctx->processingArgument = 0;

    CError_ASSERT(1477, newobj->type->type == TYPEFUNC);
    ((TypeFunc *)newobj->type)->flags |= 0x400;

    if ((((TypeFunc *)newobj->type)->flags & FUNC_IS_DTOR) != 0 && ctx->hasNewVBases != 0 && found == NULL) {
        CError_ASSERT(1484, ((TypeFunc *)newobj->type)->args != NULL);
        arg = CParser_NewFuncArg();
        arg->type = (Type *)&stsignedshort;
        arg->next = ((TypeFunc *)newobj->type)->args->next;
        ((TypeFunc *)newobj->type)->args->next = arg;
    }

    CScope_AddObject(TYPE_CLASS(ctx->inst)->nspace, newobj->name, (ObjBase *)newobj);
}

void instantiate_objtype(TypeDeduce *context, ObjType *type, HashNameNode *name)
{
    TemplateClassDeclaration *pendingType;
    ObjType *instantiatedType;
    DefAction *binding;
    NameSpaceObjectList *objects;
    NameSpaceObjectList *extra;

    pendingType = ((TemplClass *)context->tmclass)->actions;
    while (pendingType != NULL) {
        if (pendingType->kind == 7 && pendingType->target.object == (ObjBase *)type)
            break;
        pendingType = pendingType->next;
    }

    instantiatedType = (ObjType *)galloc(10);
    *instantiatedType = *type;

    if (pendingType != NULL) {
        binding = (DefAction *)CompilerTools_AllocatePool(20);
        binding->next = (DefAction *)context->defActions;
        binding->action = pendingType;
        context->defActions = (struct DefAction *)binding;
        binding->refobj = (ObjBase *)instantiatedType;
    } else {
        instantiatedType->type =
            CTemplateTools_ResolveType((TypeDeduce *)context, instantiatedType->type, &instantiatedType->qual);
    }

    objects = CScope_FindName(TYPE_CLASS(context->inst)->nspace, name);
    if (objects != NULL && objects->object->otype == OT_TYPETAG) {
        CError_ASSERT(1394, objects->next == NULL);
        extra = (NameSpaceObjectList *)galloc(8);
        extra->object = objects->object;
        extra->next = NULL;
        objects->object = (ObjBase *)instantiatedType;
        objects->next = extra;
        return;
    } else {
        CScope_AddObject(TYPE_CLASS(context->inst)->nspace, name, (ObjBase *)instantiatedType);
    }
}

void instantiate_ivars(TypeDeduce *ctx, TypeClass *dst, TemplClass *src)
{
    ObjMemberVar *p;
    ObjMemberVar *m;
    struct TemplateClassDeclaration *q;
    struct DefAction *r;
    ObjMemberVar **out;

    p = src->theclass.ivars;
    out = &dst->ivars;
    for (; p != NULL; p = p->next) {
        CError_ASSERT(1321, !p->has_path);
        m = (ObjMemberVar *)galloc(sizeof(ObjMemberVar));
        *m = *p;
        for (q = ((TemplClass *)ctx->tmclass)->actions; q != NULL; q = q->next) {
            if (q->kind == 7 && q->target.object == (ObjBase *)p) {
                r = (struct DefAction *)CompilerTools_AllocatePool(sizeof(struct DefAction));
                r->next = ctx->defActions;
                r->action = q;
                ctx->defActions = r;
                r->refobj = (ObjBase *)m;
                break;
            }
        }
        if (q == NULL) {
            m->type = (Type *)CTemplateTools_ResolveType((TypeDeduce *)ctx, m->type, &m->qual);
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

void initialize_enum_constants(TypeDeduce *context, struct TemplateClassDeclaration *object, TypeEnum *scope)
{
    int savedContext;
    Type *enumType;
    TemplClass *templateClass;
    ObjEnumConst *item;
    ObjEnumConst *binding;
    struct TemplateClassDeclaration *parameter;
    ENode *expression;

    enumType = object->target.type;
    templateClass = (TemplClass *)context->tmclass;
    for (parameter = templateClass->actions; parameter != NULL; parameter = parameter->next) {
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

void instantiate_enum(TypeDeduce *context, struct TemplateClassDeclaration *entry)
{
    ObjEnumConst **tail;
    ObjEnumConst *item;
    struct TemplateClassDeclaration *candidate;
    ObjEnumConst *firstItem;
    struct TemplateClassDeclaration *firstCandidate;
    ObjEnumConst *copy;
    struct DefAction *binding;
    TypeEnum *replacement;
    TypeEnum *original;
    ObjEnumConst *reference;
    TemplClass *templateClass;
    original = (TypeEnum *)entry->target.type;
    replacement = (TypeEnum *)galloc(22);
    memclrw(replacement, 22);
    replacement->type = TYPEENUM;
    replacement->size = original->size;
    replacement->nspace = TYPE_CLASS(context->inst)->nspace;
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
            CScope_AddObject(TYPE_CLASS(context->inst)->nspace, copy->name, (ObjBase *)copy);
            copy = *tail;
            item = item->next;
            tail = &copy->next;
        } while (item != NULL);
    }
    templateClass = (TemplClass *)context->tmclass;
    candidate = (firstCandidate = templateClass->actions);
    if (firstCandidate != NULL) {
        do {
            if (candidate->kind == 3 &&
                (reference = (ObjEnumConst *)candidate->target.object)->type == (Type *)original) {
                binding = (struct DefAction *)CompilerTools_AllocatePool(20);
                binding->next = context->defActions;
                binding->action = entry;
                context->defActions = binding;
                binding->enumtype = replacement;
                return;
            }
            candidate = candidate->next;
        } while (candidate != NULL);
    }
    return;
}

void instantiate_bases(TypeDeduce *context, TypeClass *instance, TemplClass *classTemplate)
{
    ClassList *resolvedTypeData = NULL;
    int savedEntry;
    ClassList *base;
    ClassList *current;
    struct TemplateClassDeclaration *declaration;
    ClassList *newBase;
    ClassList *instanceBase;
    ClassList **resolvedQualifiers;

    for (base = classTemplate->theclass.bases; base; base = base->next) {
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

    for (declaration = ((TemplClass *)context->tmclass)->actions; declaration; declaration = declaration->next) {
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
                        base = classTemplate->theclass.bases;
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

void CTemplateClass_0051cec0(TypeDeduce *context, TemplClass *templateClass)
{
    ObjType *reference = galloc(sizeof(ObjNameSpace));
    memclrw(reference, sizeof(ObjNameSpace));
    reference->otype = OT_TYPETAG;
    reference->access = ACCESSPUBLIC;
    if (templateClass->templ__params == NULL) {
        TemplClassInst *instance = create_class_template_instance(templateClass, NULL, NULL);
        instance->parent = context->inst;
        instance->theclass.nspace->parent = (NameSpace *)TYPE_CLASS(context->inst)->nspace;
        reference->type = (Type *)instance; /* OT_TYPETAG carries a class type here. */
    } else {
        TemplClass *instance = galloc(sizeof(TemplClass));
        memclrw(instance, sizeof(*instance));
        instance->next = class_template_list;
        class_template_list = instance;
        instance->theclass = templateClass->theclass;
        instance->templ_parent = context->tmclass;
        instance->inst_parent = context->inst;
        instance->templ__params = templateClass->templ__params;
        instance->members = NULL;
        instance->instances = NULL;
        instance->pspecs = NULL;
        instance->actions = templateClass->actions;
        instance->lex_order_count = templateClass->lex_order_count;
        instance->align = templateClass->align;
        instance->flags = templateClass->flags;
        reference->type = (Type *)instance; /* OT_TYPETAG carries a class type here. */
    }
    CScope_AddObject(TYPE_CLASS(context->inst)->nspace, templateClass->theclass.classname, (ObjBase *)reference);
}

/* Opaque state copied as six words. */
/* Records manipulated by this routine; intervening bytes are opaque. */

TemplClass *CTemplateClass_CreateClassTemplateDeclaration(TypeClass *owner, HashNameNode *arg1, short arg2)
{
    TemplClass *object;
    struct TemplateListRecord *record;
    struct TemplateListRecord *tail;

    object = (TemplClass *)galloc(90);
    memclrw(object, 90);
    object->next = class_template_list;
    class_template_list = object;
    object->templ_parent = (TemplClass *)owner;
    object->templ__params = NULL;
    CDecl_DefineClass(owner->nspace, arg1, &object->theclass, arg2, 0, 1);
    object->theclass.flags = FUNC_AUTO_GENERATED;

    record = (struct TemplateListRecord *)galloc(40);
    memclrw(record, 40);
    record->value_26 = 0;
    record->object = object;
    record->state = *CPrep_GetLastBufferedToken();

    if ((tail = (struct TemplateListRecord *)(*(TemplClass *)owner).actions) != NULL) {
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = record;
    } else {
        (*(TemplClass *)owner).actions = (struct TemplateClassDeclaration *)record;
    }

    return object;
}

/* Template class record in the candidate chain. */
/* Temporary list of matching candidates. */

char CTemplateClass_SelectSpecialization(CTStateElem *context, TemplClass **classType, CTStateElem **result)
{
    TemplPartialSpec *entry;
    TemplateClassMatch *match;
    TemplateClassMatch *matches;

    {
        TemplClassInst *entry;
        for (entry = (TemplClassInst *)(*classType)->instances; entry != NULL; entry = entry->next) {
            if (((TemplClassInst *)entry)->is_instantiated != 0 || ((TemplClassInst *)entry)->is_specialized != 0) {
                CTStateElem *value = ((TemplClassInst *)entry)->oargs ? ((TemplClassInst *)entry)->oargs
                                                                      : ((TemplClassInst *)entry)->inst_args;
                if (CTemplTool_EqualArgs(context, value))
                    return 0;
            }
        }
    }
    matches = NULL;
    for (entry = (*classType)->pspecs; entry != NULL; entry = entry->next) {
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
        if (matches->candidate->templ->templ__params == NULL) {
            *classType = matches->candidate->templ;
            *result = NULL;
            return 1;
        }
        *classType = matches->candidate->templ;
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
    struct TemplPartialSpec *candidate, *other;

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

unsigned char match_template_arguments(TemplPartialSpec *arguments, TemplPartialSpec *pattern)
{
    CTStateElem *argument;
    CTStateElem *patternArgument;
    int index;
    int matchIndex;
    struct DeduceInfo state;
    if (CTemplTool_InitDeduceInfo(&state, pattern->templ->templ__params, NULL, 1) == 0)
        return 0;
    argument = arguments->args;
    patternArgument = pattern->args;
    for (;;) {
        if (argument == NULL) {
            if (patternArgument != NULL)
                CError_FATAL(796);
            index = 0;
            while (index < state.maxCount) {
                if (state.args[index].bound == 0)
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
                                        argument->argument.type, argument->qualifiers, state.args, 0) == 0)
                return 0;
        } else if (CTemplTool_IsTypeDepExpr(patternArgument->argument.expression) != 0) {
            matchIndex = CTemplateFunc_GetArgumentParameterIndex(patternArgument);
            if (matchIndex < 0)
                CError_FATAL(845);
            if (state.args[matchIndex].bound != 0) {
                if (argument->argument.type != NULL) {
                    if (state.args[matchIndex].argument.type == NULL ||
                        CTemplateTools_00517a40(argument->argument.expression,
                                                state.args[matchIndex].argument.expression) == 0)
                        return 0;
                } else {
                    if (state.args[matchIndex].argument.type != NULL ||
                        argument->pid.index != state.args[matchIndex].pid.index)
                        return 0;
                }
            } else {
                state.args[matchIndex].argument.expression = argument->argument.expression;
                state.args[matchIndex].pid.index = argument->pid.index;
                state.args[matchIndex].pid.type = 0;
                state.args[matchIndex].bound = 1;
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

CTStateElem *match_specialization_arguments(TemplPartialSpec *arguments, CTStateElem *actual, char instantiate)
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
    struct DeduceInfo state;

    if (!CTemplTool_InitDeduceInfo(&state, arguments->templ->templ__params, NULL, 1))
        return NULL;
    pattern = (CTStateElem *)arguments->args;
    candidate = actual;
    matched = 0;
    for (;;) {
        if (!pattern) {
            if (candidate)
                return NULL;
            patternQualifiers = 0;
            while (patternQualifiers < state.maxCount) {
                if (!state.args[patternQualifiers].bound)
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
                                             candidateQualifiers, state.args, 0))
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
                if (state.args[index].bound != 0) {
                    if (!CTemplateTools_00517a40(candidate->argument.expression, state.args[index].argument.expression))
                        return NULL;
                } else {
                    state.args[index].argument.expression = candidate->argument.expression;
                    state.args[index].pid.type = 0;
                    state.args[index].bound = 1;
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
    TemplClass *record;
    struct TemplateMemberData *entry;
    struct TemplateMemberData **tail;
    TemplClass *owner;
    DeclInfo context;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    existing = (struct TypeClass *)CScope_GetTagType(scope->scope->parent, data_00587fa0);
    if (existing == NULL) {
        record = (TemplClass *)galloc(sizeof(*record));
        memclrw(record, sizeof(*record));
        record->next = class_template_list;
        class_template_list = record;
        record->templ__params = parameters;
        CDecl_DefineClass(scope->scope->parent, data_00587fa0, &record->theclass, access, 0, 1);
        record->theclass.flags = CLASS_IS_TEMPL;
        tk = CPrepTokenizer_GetNextToken();
        if (scope->scope->parent->theclass != NULL && (scope->scope->parent->theclass->flags & CLASS_IS_TEMPL) != 0) {
            record->templ_parent = (TemplClass *)scope->scope->parent->theclass;
            entry = galloc(sizeof(*entry));
            memclrw(entry, sizeof(*entry));
            entry->kind = 0;
            entry->record = record;
            owner = record->templ_parent;
            entry->sourcePosition = *CPrep_GetLastBufferedToken();
            if ((tail = (struct TemplateMemberData **)owner->actions) != NULL) {
                while (*tail != NULL) {
                    tail = &(*tail)->next;
                }
                *tail = entry;
            } else {
                owner->actions = (struct TemplateClassDeclaration *)entry;
            }
        }
    } else {
        if (existing->type != TYPECLASS || (existing->flags & CLASS_IS_TEMPL) == 0) {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, data_00587fa0->name);
            return;
        }
        record = (TemplClass *)existing;
        if (CTemplTool_EqualParams(record->templ__params, parameters, 0) == 0) {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, record->theclass.classname->name);
            return;
        }
        CTemplTool_MergeDefaultArgs(record->templ__params, parameters);
        tk = CPrepTokenizer_GetNextToken();
        if ((record->theclass.flags & CLASS_COMPLETED) != 0 && tk != ';') {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, record->theclass.classname->name);
            return;
        }
        if (tk != ';') {
            CTemplTool_EqualParams(record->templ__params, parameters, 1);
        }
    }
    switch (tk) {
        case ':':
        case '{':
        case TK_UU_DECLSPEC:
            record->theclass.nspace->parent = scope->scope;
            scope->linkedNamespace = record->theclass.nspace;
            record->align = copts.structalignment;
            memclrw(&context, sizeof(context));
            context.browseFile = CPrep_GetPFile();
            CPrep_GetBrowseFilePosition(&context.sourceFile, &context.sourceLine);
            context.sourceLine = *state;
            context.pendingClass = &record->theclass;
            CDecl_ParseClass(&context, access, 1, 0);
            if (tk != ';') {
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
            }
            CBrowse_RecordClassLocation(&record->theclass, context.sourceFile, context.sourceLine,
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
    TemplClass *templateClass;
    TemplateParameterRecord *parameter;
    TemplPartialSpec *specialization;
    CTStateElem *argument;
    TemplateParameterRecord *templateParameter;
    TemplClass *instance;
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
    templateClass = (TemplClass *)type;
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
    arguments = CTemplateNew_ParseTemplateArguments((TemplateParameterRecord *)templateClass->templ__params, 0);
    tk = CPrepTokenizer_GetNextToken();
    argument = arguments;
    templateParameter = templateClass->templ__params;
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
    specialization = templateClass->pspecs;
    if (specialization != NULL) {
        do {
            if (CTemplTool_EqualParams(specialization->templ->templ__params, parameters, 0) != 0 &&
                CTemplTool_EqualArgs(specialization->args, arguments) != 0) {
                break;
            }
            specialization = specialization->next;
        } while (specialization != NULL);
    }
    if (specialization == NULL) {
        instance = (TemplClass *)galloc(90);
        memclrw(instance, 90);
        instance->templ__params = parameters;
        CDecl_DefineClass(scope->scope->parent, templateClass->theclass.classname, &instance->theclass, access, 0, 0);
        instance->theclass.flags = CLASS_IS_TEMPL;
        instance->pspec_owner = templateClass;
        specialization = (TemplPartialSpec *)galloc(12);
        memclrw(specialization, 12);
        specialization->templ = instance;
        specialization->args = CTemplateTools_CopyCTStateElemList(arguments);
        specialization->next = templateClass->pspecs;
        templateClass->pspecs = specialization;
    } else {
        if ((specialization->templ->theclass.flags & CLASS_COMPLETED) != 0 && tk != ';') {
            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, (int)templateClass->theclass.classname + 10);
            return;
        }
        if (tk == ':' || tk == '{') {
            CTemplTool_EqualParams(specialization->templ->templ__params, parameters, 1);
        }
    }
    switch (tk) {
        case ':':
        case '{':
            specialization->templ->theclass.nspace->parent = scope->scope;
            scope->linkedNamespace = specialization->templ->theclass.nspace;
            instance = specialization->templ;
            instance->align = copts.structalignment;
            memclrw(&declaration, 92);
            declaration.browseFile = CPrep_GetPFile();
            CPrep_GetBrowseFilePosition(&declaration.sourceFile, &declaration.sourceLine);
            declaration.sourceLine = *position;
            declaration.pendingClass = &instance->theclass;
            CDecl_ParseClass(&declaration, access, 1, 0);
            if (tk != ';') {
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
            }
            CBrowse_RecordClassLocation(&instance->theclass, declaration.sourceFile, declaration.sourceLine,
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

struct KeyedEntry *CTemplateClass_AddTemplateArgumentOverride(TemplClass *owner, Object *key, FileOffsetInfo *name,
                                                              struct TokenStream *payload)
{
    enum { KeyedEntryAllocationSize = 0x2a };
    KeyedEntry *entry;

    entry = owner->members;
    while (entry != NULL) {
        if (entry->key == key) {
            CError_ReportError(ERR_OBJECT_REDEFINED, key);
            return entry;
        }
        entry = entry->next;
    }
    entry = (KeyedEntry *)galloc(KeyedEntryAllocationSize);
    memclrw((unsigned char *)entry, KeyedEntryAllocationSize);
    entry->next = owner->members;
    owner->members = entry;
    entry->templateParameters = NULL;
    entry->key = key;
    entry->name = *name;
    entry->payload = *payload;
    return entry;
}

TemplClassInst *CTemplateClass_GetInstance(TemplClass *cls, CTStateElem *key, CTStateElem *flag)
{
    TemplClassInst *instance = cls->instances;

    while (instance != NULL) {
        if (flag != NULL)
            CError_FATAL(353);
        if (CTemplTool_EqualArgs(key, instance->oargs ? instance->oargs : instance->inst_args))
            return instance;
        instance = instance->next;
    }
    return create_class_template_instance(cls, key, flag);
}

TemplClassInst *create_class_template_instance(TemplClass *definition, void *argument, void *alternate_argument)
{
    TemplClassInst *instance;
    NameSpace *scope;
    ObjType *entry;
    NameSpace *parent;
    HashNameNode *name;

    if (definition->pspec_owner != 0U)
        CError_FATAL(288);

    instance = (TemplClassInst *)galloc(74U);
    memclrw(instance, 74U);
    instance->next = definition->instances;
    definition->instances = instance;

    if (definition->templ__params != 0U) {
        CTStateElem *name_argument = (CTStateElem *)((alternate_argument != NULL) ? alternate_argument : argument);
        name = CMangler_TemplateInstanceName(definition->theclass.classname, name_argument);
    } else {
        name = definition->theclass.classname;
    }

    instance->inst_args = argument;
    instance->oargs = alternate_argument;
    instance->parent = definition->inst_parent;

    scope = CScope_NewListNameSpace(name, 1);
    scope->theclass = &instance->theclass;

    if (definition->templ_parent != 0U && definition->inst_parent != NULL) {
        scope->parent = definition->inst_parent->theclass.nspace;
    } else {
        parent = definition->theclass.nspace->parent;
        while (parent->is_templ != 0U)
            parent = parent->parent;
        scope->parent = parent;
    }

    instance->theclass.type = TYPECLASS;
    instance->theclass.flags = 0x800U;
    instance->theclass.nspace = scope;
    instance->theclass.classname = definition->theclass.classname;
    instance->theclass.mode = definition->theclass.mode;
    instance->theclass.eflags = definition->theclass.eflags;
    instance->templ = definition;

    entry = (ObjType *)galloc(6U);
    memclrw(entry, 6U);
    entry->otype = OT_TYPETAG;
    entry->access = ACCESSPUBLIC;
    entry->type = (Type *)instance;

    CScope_AddObject(scope, definition->theclass.classname, (ObjBase *)entry);
    return instance;
}

unsigned char CTemplateClass_CompleteClassLayout(TemplClass *state, ClassLayout *values)
{
    unsigned char byteValue;
    state->lex_order_count = values->lex_order_count;
    byteValue = values->has_vtable;
    state->flags = byteValue;
    state->theclass.flags |= CLASS_COMPLETED;
    return byteValue;
}

void CTemplateClass_AppendObjectDeclaration(TemplClass *ctx, Object *obj)
{
    struct TemplateObjectDeclaration *record;

    record = galloc(sizeof(*record));
    memclrw(record, sizeof(*record));
    record->kind = 7;
    record->argument = obj;
    record->data = *CPrep_GetLastBufferedToken();

    if (ctx->actions != NULL) {
        struct TemplateObjectDeclaration *last = (struct TemplateObjectDeclaration *)ctx->actions;
        while (last->next != NULL)
            last = last->next;
        last->next = record;
    } else {
        ctx->actions = (struct TemplateClassDeclaration *)record;
    }
}

void CTemplateClass_AppendExpressionRecord(TemplClass *type, Object *object, ENode *expression)
{
    struct TemplateExpressionRecord *record;
    record = galloc(40U);
    memclrw(record, 40U);
    record->kind = 5U;
    record->declaration.object = object;
    record->expression = fn_00513040(expression, 1U);
    record->parserState = *CPrep_GetLastBufferedToken();
    if (type->actions != NULL) {
        struct TemplateExpressionRecord *last = (struct TemplateExpressionRecord *)type->actions;
        while (last->next != NULL)
            last = last->next;
        last->next = record;
    } else {
        type->actions = (struct TemplateClassDeclaration *)record;
    }
}

void CTemplateClass_AppendEnumConstDeclaration(TemplClass *self, ObjEnumConst *a, ENode *str)
{
    struct TemplateExpressionRecord *p;
    struct TemplateExpressionRecord *r;

    r = galloc(sizeof(struct TemplateExpressionRecord));
    memclrw(r, sizeof(struct TemplateExpressionRecord));
    r->kind = 3;
    r->declaration.object = (Object *)a;
    r->expression = str ? fn_00513040(str, 1) : NULL;
    r->parserState = *CPrep_GetLastBufferedToken();

    if (self->actions != NULL) {
        p = (struct TemplateExpressionRecord *)self->actions;
        while (p->next != NULL)
            p = p->next;
        p->next = r;
    } else {
        self->actions = (struct TemplateClassDeclaration *)r;
    }
}

/* Saved opaque template context returned by CPrep_GetLastBufferedToken. */
/* Pending template instantiation, linked in declaration order. */

void CTemplateClass_AppendEnumDeclaration(TemplClass *type, TypeEnum *value)
{
    struct PendingTemplateInstantiation *record;
    struct PendingTemplateInstantiation *tail;
    record = galloc(40U);
    memclrw(record, 40U);
    record->kind = 1;
    record->enumType = value;
    record->context = *CPrep_GetLastBufferedToken();
    if (type->actions != NULL) {
        tail = (struct PendingTemplateInstantiation *)type->actions;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = record;
    } else {
        type->actions = (struct TemplateClassDeclaration *)record;
    }
}

/* Private list records and the containing object's unexamined storage. */
unsigned int CTemplateClass_PrependTemplateRecordEntry(TemplClass *list, Type *value, unsigned char value24,
                                                       unsigned char value25)
{
    struct ClassList *last;
    struct TemplateRecordEntry *entry;
    struct TemplateRecordEntry *oldHead;

    if ((last = list->theclass.bases) != NULL) {
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
    oldHead = (struct TemplateRecordEntry *)list->actions;
    entry->next = oldHead;
    list->actions = (struct TemplateClassDeclaration *)entry;
    return (unsigned int)oldHead;
}

void CTemplateClass_AddDeferredFunctionDeclaration(TemplClass *classTemplate, DeclInfo *declInfo)
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

    if ((tail = (TemplateExpressionRecord *)classTemplate->actions) != NULL) {
        while (tail->next)
            tail = tail->next;
        tail->next = declaration;
    } else {
        classTemplate->actions = (struct TemplateClassDeclaration *)declaration;
    }
}

/* Namespace data copied without its trailing status bytes. */
/* Record in the class's appended instance chain. */
/* ClassChainEntry is defined in structs/ClassChainEntry.h. */

void CTemplateClass_AppendFuncDeclaration(TemplClass *owner, TypeTemplDep *value, unsigned char kind)
{
    struct ClassChainEntry *entry;
    struct ClassChainEntry *tail;
    entry = galloc(40U);
    memclrw(entry, 40U);
    entry->type = TYPEFUNC;
    entry->value = (UInt32)value;
    entry->kind = kind;
    entry->sourcePosition = *CPrep_GetLastBufferedToken();
    if (owner->actions != NULL) {
        for (tail = (struct ClassChainEntry *)owner->actions; tail->next != NULL; tail = tail->next)
            ;
        tail->next = entry;
    } else {
        owner->actions = (struct TemplateClassDeclaration *)entry;
    }
}

TemplClass *CTemplateClass_ResolveRelatedClass(TemplClass *record)
{
    if (record->inst_parent != 0U) {
        record = (TemplClass *)record->theclass.nspace->theclass;
        if ((record->theclass.flags & 256U) == 0U)
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
