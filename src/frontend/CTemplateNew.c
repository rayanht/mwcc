#define CERROR_FILE "CTemplateNew.c"
#include "compiler/common.h"
#include "compiler/CTemplateNew.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CBrowse.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/FunctionCalls.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroDump.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroPropagate.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <setjmp.h>
#include <string.h>
#include <stdio.h>

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

static jmp_buf template_declaration_jmpbuf;
static Boolean data_00582108;

Boolean CTemplateNew_InstantiateInlineTemplateObject(Object *obj)
{
    ObjectTemplated *node;
    TemplClassInst *theclass;
    Object *func;
    TemplateMember *entry;

    CError_ASSERT(1981, obj->type->type == TYPEFUNC && (obj->qual & Q_IS_TEMPLATED));
    if (!(TYPE_FUNC(obj->type)->flags & FUNC_DEFINED)) {
        theclass = (TemplClassInst *)TYPE_METHOD(obj->type)->theclass;
        if (!theclass->is_specialized) {
            node = (ObjectTemplated *)obj;
            func = node->parent;
            if (func->qual & Q_INLINE) {
                entry = CTemplateClass_ResolveRelatedClass(theclass->templ)->members;
                for (; entry != NULL; entry = entry->next) {
                    obj->qual |= Q_INLINE;
                    if (entry->object == func) {
                        CTemplTool_MergeArgNames(entry->object->type, obj->type);
                        CInline_AddFunctionPrecNode(obj, &theclass->theclass, &entry->fileoffset, &entry->stream, 0);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

Boolean CTemplateNew_InstantiatePendingTemplates(void)
{
    Boolean result;
    TemplClass *node;
    TemplateFunction *nspace;
    TemplClassInst *classEntry;
    TemplClass *classTemplate;
    TemplPartialSpec *pair;
    TemplClassInst *nestedClass;
    NameSpaceObjectList *found;
    TemplFuncInstance *entry;

    result = 0;
    node = class_template_list;
    if (node != NULL) {
        do {
            for (classEntry = node->instances; classEntry != NULL; classEntry = classEntry->next) {
                if ((classEntry->theclass.flags & CLASS_IS_TEMPL_INST) != 0 && classEntry->is_specialized == 0 &&
                    (classTemplate = classEntry->templ, instantiate_members(classTemplate, classEntry, 0)))
                    result = 1;
            }
            for (pair = node->pspecs; pair != NULL; pair = pair->next) {
                for (nestedClass = pair->templ->instances; nestedClass != NULL; nestedClass = nestedClass->next) {
                    if ((nestedClass->theclass.flags & CLASS_IS_TEMPL_INST) != 0 && nestedClass->is_specialized == 0 &&
                        instantiate_members(pair->templ, nestedClass, 0))
                        result = 1;
                }
            }
            node = node->next;
        } while (node != NULL);
    }

    for (nspace = templateFunctions; nspace != NULL; nspace = nspace->next) {
        for (entry = nspace->instances; entry != NULL; entry = entry->next) {
            if (entry->is_instantiated == 0 && entry->is_specialized == 0) {
                found = CScope_FindName(entry->object->nspace, entry->object->name);
                while (found != NULL) {
                    if (found->object->otype == OT_OBJECT &&
                        iscpp_typeequal(entry->object->type, ((Object *)found->object)->type) != 0)
                        break;
                    found = found->next;
                }
                if ((found == NULL && (entry->object->flags & 2) != 0 &&
                     (((TypeFunc *)entry->object->type)->flags & FUNC_DEFINED) == 0) ||
                    (found != NULL && (((TypeFunc *)((Object *)found->object)->type)->flags & FUNC_DEFINED) == 0 &&
                     (((Object *)found->object)->flags & 2) != 0)) {
                    entry->is_instantiated = 1;
                    if (CTemplateNew_InstantiateFunction(nspace, entry, 0))
                        result = 1;
                }
            }
        }
    }
    return result;
}

static inline char CTemplateNew_004ed8e0_inline1(TemplClassInst *a1, Object *v1, char a2)
{
    Object *v3;
    TemplClass *t1;
    TemplateMember *v5;
    char v6;
    t1 = a1->templ;
    v3 = ((ObjectTemplated *)v1)->parent;
    v5 = CTemplateClass_ResolveRelatedClass(t1)->members;
    while ((int)v5 != 0) {
        if (v5->object == v3) {
            CTemplateNew_CompileObject(t1, a1, v5, v1, a2);
            return (char)1;
        }
        v5 = v5->next;
    }
    if (a2 != 0) {
        CError_Warning(ERR_CANNOT_INSTANTIATE, v1);
    }
    v6 = (char)0;
    return v6;
}

static inline char CTemplateNew_004ed8e0_inline2(TemplClassInst *a1, Object *v1, char a2)
{
    TemplClass *t2;
    Object *v7;
    TemplateMember *v9;
    char v10;
    t2 = a1->templ;
    v7 = ((ObjectTemplated *)v1)->parent;
    v9 = CTemplateClass_ResolveRelatedClass(t2)->members;
    while ((int)v9 != 0) {
        if (v9->object == v7) {
            CTemplateNew_CompileObject(t2, a1, v9, v1, a2);
            return (char)1;
        }
        v9 = v9->next;
    }
    if (a2 != 0) {
        CError_Warning(ERR_CANNOT_INSTANTIATE, v1);
    }
    v10 = (char)0;
    return v10;
}

unsigned char instantiate_members(TemplClass *templ, TemplClassInst *state, char force)
{
    Object *entry;
    char functionResult;
    char entryResult;
    char changed;
    CScopeObjectIterator iterator;
    changed = 0;
    if (force == 0 && state->is_extern != 0)
        return (char)0;
    CScope_InitScopeSearch(&iterator, state->theclass.nspace);
    entry = CScope_NextObject(&iterator);
    while (entry != NULL) {
        if (entry->type->type == TYPEFUNC) {
            if ((force != 0 || (entry->flags & 2) != 0) &&
                (((TypeFunc *)entry->type)->flags & (FUNC_AUTO_GENERATED | FUNC_DEFINED)) == 0) {
                functionResult = CTemplateNew_004ed8e0_inline1(state, entry, force);
                if (functionResult != 0 && (((TypeFunc *)entry->type)->flags & FUNC_DEFINED) != 0)
                    changed = 1;
            }
        } else if (state->static_instantiated == 0 && entry->datatype == DDATA && (entry->qual & Q_INLINE_DATA) == 0 &&
                   (entry->flags & 4) == 0) {
            entryResult = CTemplateNew_004ed8e0_inline2(state, entry, force);
            if (entryResult != 0)
                changed = 1;
        }
        entry = CScope_NextObject(&iterator);
    }
    state->static_instantiated = 1;
    return changed;
}

/* Source record used by template instantiation. */
/* Working record passed to fn_004763e0. */

void CTemplateNew_CompileObject(TemplClass *templateClass, TemplClassInst *context, TemplateMember *source,
                                Object *object, Boolean reset)
{
    CScopeSave scope;
    SInt32 savedState;
    DeclInfo compilation;
    NameSpace *arguments;
    UInt8 savedFileSymInfo;
    UInt8 savedTemplateState;
    struct TemplStack sa;

    FunctionCalls_PushObjectReferenceEntry(&sa, NULL, object);
    arguments = CTemplateTools_InsertTemplateArgs(source->params ? source->params : templateClass->templ__params,
                                                  context, &scope);
    CPrep_InsertTokenBuffer(&source->stream, &savedState);
    savedTemplateState = template_recordbrowseinfo;
    template_recordbrowseinfo = 1;
    source_line = source->startoffset;
    tk = CPrepTokenizer_GetNextToken();
    declaration_token = *CPrep_GetLastBufferedToken();
    savedFileSymInfo = copts.filesyminfo;
    if (savedFileSymInfo) {
        CPrep_GetFOI(&function_fileinfo, &declaration_token);
        data_00587184 = data_00588454;
    }
    if (object->sclass != TK_STATIC) {
        if (reset)
            object->sclass = TK_EOF;
        else
            object->qual |= Q_WEAK;
    }
    memclrw(&compilation, sizeof(compilation));
    compilation.file2 = source->srcfile;
    compilation.file = CPrep_GetPFile();
    compilation.sourceoffset = source->startoffset;
    switch (object->datatype) {
        case DFUNC:
        case DVFUNC:
            CTemplTool_MergeArgNames(source->object->type, object->type);
            CFunc_ParseFuncDef(object, &compilation, &context->theclass, 0, 0, NULL);
            break;
        case DDATA:
            CDecl_CompleteType(object->type);
            CInit_InitializeData(object);
            break;
        default:
            CError_FATAL(1796);
    }
    CTemplateTools_PopObjectReferenceEntry(&sa);
    CTemplTool_RemoveTemplateArgumentNameSpace(arguments, context, &scope);
    CPrep_RemoveBufferedTokens(&source->stream, &savedState);
    copts.filesyminfo = savedFileSymInfo;
    template_recordbrowseinfo = savedTemplateState;
}

Boolean CTemplateNew_InstantiateFunction(TemplateFunction *definition, TemplFuncInstance *specialization,
                                         Boolean report)
{
    UInt8 savedFileSymbolInfo;
    SInt32 savedStream;
    DeclInfo workspace;
    struct TemplStack parserState;
    NameSpace *namespace;

    if (specialization->is_extern != 0 && !report)
        return 0;
    if (definition->stream.tokens == 0) {
        if (report) {
            CError_SetBufferedToken(&definition->deftoken);
            CError_ReportError(ERR_CANNOT_INSTANTIATE, specialization->object);
        }
        return 0;
    }
    specialization->is_instantiated = 1;
    CPrep_InsertTokenBuffer(&definition->stream, &savedStream);
    savedFileSymbolInfo = copts.filesyminfo;
    if (copts.fbf != 0 || definition->deftoken.tokenfile == NULL)
        copts.filesyminfo = 0;
    tk = CPrepTokenizer_GetNextToken();
    if (tk != '{' && tk != ':' && tk != TK_TRY)
        CError_FATAL(1689);
    declaration_token = *CPrep_GetLastBufferedToken();
    if (copts.filesyminfo != 0) {
        CPrep_GetFOI(&function_fileinfo, &definition->deftoken);
        data_00587184 = data_00588454;
    }
    if (specialization->object->sclass != TK_STATIC) {
        if (report)
            specialization->object->sclass = TK_EXTERN;
        else
            specialization->object->qual |= Q_WEAK;
    }
    memclrw(&workspace, sizeof(workspace));
    workspace.file2 = definition->srcfile;
    workspace.file = CPrep_GetPFile();
    workspace.sourceoffset = definition->startoffset;
    FunctionCalls_PushObjectReferenceEntry(&parserState, NULL, specialization->object);
    CTemplTool_MergeArgNames(definition->tfunc->type, specialization->object->type);
    namespace = CTemplTool_SetupTemplateArgumentNameSpace(definition->params, specialization->args, 0);
    namespace->parent = specialization->object->nspace;
    specialization->object->nspace = namespace;
    CTemplTool_SetupOuterTemplateArgumentNameSpace(namespace);
    CFunc_ParseFuncDef(specialization->object, &workspace, NULL, 0, 0, namespace);
    CTemplTool_RemoveOuterTemplateArgumentNameSpace(namespace);
    specialization->object->nspace = namespace->parent;
    CTemplateTools_PopObjectReferenceEntry(&parserState);
    CPrep_RemoveBufferedTokens(&definition->stream, &savedStream);
    copts.filesyminfo = savedFileSymbolInfo;
    if (workspace.file->recordbrowseinfo != 0)
        CBrowse_ForwardObjectFileRange(specialization->object, workspace.file, workspace.file2, workspace.sourceoffset,
                                       definition->endoffset);
    return 1;
}

void CTemplateNew_ParseFuncDef(Object *object, TemplClassInst *context, TypeClass *tclass)
{
    DeclInfo declInfo;
    CScopeSave contextSave;
    TemplStack objectSave;
    TemplParam *templateParameters;
    NameSpace *arguments;
    TemplateMember *node;
    Object *parent;
    TemplClass *templateClass;

    templateClass = context->templ;
    templateParameters = templateClass->templ__params;
    if ((object->qual & Q_IS_TEMPLATED) != 0) {
        templateClass = context->templ;
        node = CTemplateClass_ResolveRelatedClass(templateClass)->members;
        parent = ((ObjectTemplated *)object)->parent;
        while (node != NULL) {
            if (node->object == parent) {
                if (node->params != NULL)
                    templateParameters = node->params;
                break;
            }
            node = node->next;
        }
    }
    FunctionCalls_PushObjectReferenceEntry(&objectSave, NULL, object);
    arguments = CTemplateTools_InsertTemplateArgs(templateParameters, context, &contextSave);
    if (tclass != NULL)
        currentNameSpace = tclass->nspace;
    memclrw(&declInfo, sizeof(declInfo));
    CFunc_ParseFuncDef(object, &declInfo, NULL, 0, 0, tclass != NULL ? currentNameSpace : NULL);
    CTemplTool_RemoveTemplateArgumentNameSpace(arguments, context, &contextSave);
    CTemplateTools_PopObjectReferenceEntry(&objectSave);
}

/* Namespace state for a template declaration. */
/* Saved parser position. */

void CTemplateNew_ParseTemplateDeclaration(TypeClass *templateClass)
{
    char templateDepth;
    char emptyParameters;
    NameSpace *scope;
    TemplParam *parameters;
    NameSpace *linkedNamespace;
    char isSpecialization;
    short classKind;
    int token;
    CScopeSave scopeSave;
    TemplateScopeState namespaceState;
    struct ParserPosition savedPosition;
    SInt32 tokenState[1];

    savedPosition.position = CPrep_GetCurrentTextOffset();
    BE_elf_SaveScope(&scopeSave);
    tk = CPrepTokenizer_GetNextToken();
    if (tk != '<') {
        if (templateClass != NULL)
            CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
        parse_explicit_template_instantiation();
        CScope_RestoreScope(&scopeSave);
        return;
    }
    emptyParameters = 0;
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '>') {
        if (templateClass == NULL || (templateClass->flags & CLASS_IS_TEMPL) == 0) {
            tk = CPrepTokenizer_GetNextToken();
            parse_explicit_template_specialization();
            CScope_RestoreScope(&scopeSave);
            return;
        }
        emptyParameters = 1;
    }
    namespaceState.scope = CScope_NewListNameSpace(NULL, 0);
    namespaceState.linkedNamespace = NULL;
    templateDepth = 0;
    namespaceState.scope->parent = currentNameSpace;
    parameters = NULL;
    namespaceState.scope->is_templ = 1;
    currentNameSpace = namespaceState.scope;
    scope = namespaceState.scope;
    if (scope) {
        do {
            if (scope->theclass != NULL && (scope->theclass->flags & CLASS_IS_TEMPL) != 0)
                ++templateDepth;
            scope = scope->parent;
        } while (scope);
    }
    (void)scope;
    for (;;) {
        if (tk == '>') {
            parameters = NULL;
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_TEMPLATE)
                break;
            CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
            break;
        }
        parameters = parse_template_parameter_list(namespaceState.scope, templateDepth);
        if (tk != '>')
            CError_ReportError(ERR_GREATER_EXPECTED);
        tk = CPrepTokenizer_GetNextToken();
        if (tk != TK_TEMPLATE)
            break;
        if (templateClass != NULL)
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        tk = CPrepTokenizer_GetNextToken();
        if (tk != '<')
            CError_ReportError(ERR_LESS_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
        ++templateDepth;
    }
    switch (tk) {
        case TK_CLASS:
            classKind = 2;
            break;
        case TK_UNION:
            classKind = 1;
            break;
        case TK_STRUCT:
            classKind = 0;
            break;
        default:
            classKind = -1;
    }
    do {
        if (classKind >= 0) {
            isSpecialization = 0;
            CPrep_GetBufferedTokenPosition(tokenState);
            if (CPrepTokenizer_GetNextToken() == -3) {
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '<' && _Setjmp(template_declaration_jmpbuf) == 0) {
                    skip_balanced_angle_tokens();
                    isSpecialization = 1;
                    tk = CPrepTokenizer_GetNextToken();
                }
                (token = (int)tk) == 372;
                switch (token) {
                    case 58:
                    case 59:
                    case 123:
                    case 292:
                    case 335:
                        CPrep_SetPosition(tokenState);
                        if (isSpecialization != 0)
                            CTemplateClass_ParsePartialSpecialization(&namespaceState, parameters, classKind,
                                                                      &savedPosition.position);
                        else
                            CTemplateClass_ParseClassDeclaration(&namespaceState, parameters, classKind,
                                                                 &savedPosition.position);
                        continue;
                }
            }
            CPrep_SetPosition(tokenState);
        }
        if (emptyParameters != 0) {
            CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
            return;
        }
        parse_function_template_declaration(&namespaceState, parameters, templateClass, &savedPosition.position);
    } while (0);
    if ((linkedNamespace = namespaceState.linkedNamespace) != NULL) {
        CE_ASSERT(linkedNamespace->parent != namespaceState.scope, CError_FATAL(1609));
        namespaceState.linkedNamespace->parent = namespaceState.scope->parent;
    }
    CScope_RestoreScope(&scopeSave);
}

void parse_explicit_template_specialization(void)
{
    DeclInfo ctx;
    TypeClass *classType;
    TemplClassInst *templateClass;

    memclrw(&ctx, sizeof(ctx));
    ctx.requireTemplateClassMember = 1;
    ctx.allowTemplateArguments = 1;
    CParser_GetDeclSpecs(&ctx, 1);
    if (tk == ';') {
        if (ctx.thetype->type == TYPECLASS &&
            ((classType = (TypeClass *)ctx.thetype)->flags & CLASS_IS_TEMPL_INST) != 0) {
            templateClass = (TemplClassInst *)ctx.thetype;
            templateClass->is_specialized = 1;
        } else {
            CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_SPECIALIZATION);
        }
    } else {
        CDecl_ScanDeclarator(&ctx);
        if ((tk != ';' && tk != '}') || ctx.requireTemplateClassMember != 0)
            CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_SPECIALIZATION);
    }
}

static inline HashNameNode *CTempl_FindConversion(TypeClass *tclass, Type *type, UInt32 qual)
{
    CScopeObjectIterator iter;
    Object *obj;

    qual &= Q_CV;
    CScope_InitScopeSearch(&iter, tclass->nspace);
    for (;;) {
        if (!(obj = CScope_NextObject(&iter)))
            return NULL;
        if (obj->type->type == TYPEFUNC && (TYPE_FUNC(obj->type)->flags & FUNC_CONVERSION) &&
            (TYPE_FUNC(obj->type)->qual & Q_CV) == qual && iscpp_typeequal(TYPE_FUNC(obj->type)->functype, type))
            return obj->name;
    }
}

#define CTEMPL_RECORDSOURCE(templ, startOffset)                                                                        \
    do {                                                                                                               \
        save = template_recordbrowseinfo;                                                                              \
        template_recordbrowseinfo = 1;                                                                                 \
        CPrep_GetBrowseFilePosition(&file, &offset);                                                                   \
        template_recordbrowseinfo = save;                                                                              \
        if (file && *(startOffset) >= 0 && offset > *(startOffset)) {                                                  \
            (templ)->srcfile = file;                                                                                   \
            (templ)->startoffset = *(startOffset);                                                                     \
            (templ)->endoffset = offset + 1;                                                                           \
            if (cprep_cu[0xeb] && file->recordbrowseinfo)                                                              \
                write_template_function_browse_record(templ);                                                          \
        }                                                                                                              \
    } while (0)

#define CTEMPL_DECLARE(di, params)                                                                                     \
    do {                                                                                                               \
        templ = galloc(sizeof(TemplateFunction));                                                                      \
        memclrw(templ, sizeof(TemplateFunction));                                                                      \
        templ->next = templateFunctions;                                                                               \
        templateFunctions = templ;                                                                                     \
        templ->params = params;                                                                                        \
        templ->name = (di).name;                                                                                       \
        templ->deftoken = declaration_token;                                                                           \
        obj = CParser_NewFunctionObject(NULL);                                                                         \
        obj->name = (di).name;                                                                                         \
        obj->u.func.linkname = CParser_GetUniqueName();                                                                \
        obj->type = (di).thetype;                                                                                      \
        obj->qual = (di).qual | Q_MANGLE_NAME;                                                                         \
        obj->sclass = (di).storageclass;                                                                               \
        TYPE_FUNC(obj->type)->flags |= 0x400;                                                                          \
        obj->u.templateFunction = templ;                                                                               \
        if ((di).qual & Q_INLINE)                                                                                      \
            obj->sclass = 0x102;                                                                                       \
        templ->tfunc = obj;                                                                                            \
        CScope_AddObject(currentNameSpace, (di).name, (ObjBase *)obj);                                                 \
    } while (0)

static inline void CTempl_PushScope(TemplateScopeState *stack, TypeClass *tclass)
{
    CError_ASSERT(734, !stack->linkedNamespace);
    stack->scope->parent = tclass->nspace->parent;
    tclass->nspace->parent = stack->scope;
    stack->linkedNamespace = tclass->nspace;
    currentNameSpace = tclass->nspace;
}

#pragma opt_propagation reset

static void *FindInst(TemplClass **p8c, TemplArg **p88)
{
    Type *p;
    TemplArg *t2;

    t2 = parse_template_arguments(p8c, p88);
    if (t2 == NULL)
        p = &stvoid;
    else {
        p = CTemplTool_IsDependentTemplate(*p8c, t2);
        if (p == NULL)
            p = (Type *)CTemplateClass_GetInstance(*p8c, t2, *p88);
    }
    return p;
}

void parse_explicit_template_instantiation(void)
{
    NameResult lookupResult;
    DeclInfo declaration;
    TemplClass *templateClass;
    TemplArg *arguments;
    NameSpaceObjectList *objects;
    UInt8 declarationFlags;
    UInt8 hasQualifiedName;
    UInt8 instantiate;
    TemplArg *instanceData;
    TemplClass *templateData;
    TemplClassInst *instance;
    HashNameNode *name;

    memclrw(&declaration, sizeof(declaration));
    declaration.allowTemplateArguments = 1;
    instantiate = 1;
    if (tk == TK_IDENTIFIER) {
        name = data_00587fa0;
        if (memcmp(name->name, "__dont_instantiate", 19) == 0) {
            instantiate = 0;
            tk = CPrepTokenizer_GetNextToken();
        }
    }
    switch (tk) {
        case TK_STRUCT:
        case TK_UNION:
        case TK_CLASS:
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_UU_DECLSPEC)
                declarationFlags = CDecl_ParseDeclarationAttributeFlags();
            else
                declarationFlags = 0;
            if (tk != TK_IDENTIFIER) {
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return;
            }
            declaration.thetype = CScope_FindTagType(currentNameSpace, data_00587fa0);
            if (declaration.thetype == NULL) {
                name = data_00587fa0;
                CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
                return;
            }
            if (declaration.thetype->type == TYPECLASS &&
                (((TemplClassInst *)declaration.thetype)->theclass.flags & CLASS_IS_TEMPL) != 0) {
                templateClass = (TemplClass *)declaration.thetype;
                tk = CPrepTokenizer_GetNextToken();
                if (tk != '<') {
                    CError_ReportError(ERR_LESS_EXPECTED);
                    return;
                }
                templateData = (TemplClass *)(TypeClass *)templateClass;
                if (templateData->pspec_owner)
                    templateData = templateData->pspec_owner;
                declaration.thetype = FindInst(&templateData, &instanceData);
                tk = CPrepTokenizer_GetNextToken();
                if (tk == ';') {
                    if (declaration.thetype->type == TYPECLASS) {
                        TemplClassInst *instantiated;
                        instantiated = (TemplClassInst *)declaration.thetype;
                        CTemplateClass_InstantiateClass(&instantiated->theclass);
                        if ((((TemplClassInst *)declaration.thetype)->theclass.flags & CLASS_COMPLETED) != 0) {
                            if ((declarationFlags & 2) == 0) {
                                instance = (templateClass)->instances;
                                while (instance) {
                                    if (instance == (TemplClassInst *)declaration.thetype) {
                                        if (instantiate)
                                            instantiate_members(templateClass, instance, 1);
                                        else
                                            instance->is_extern = 1;
                                        break;
                                    }
                                    instance = instance->next;
                                }
                                if (instance == NULL)
                                    CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
                            }
                        } else
                            CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, declaration.thetype, 0);
                    } else
                        CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
                    return;
                }
            }
            break;
        default:
            memclrw(&declaration, sizeof(declaration));
            CParser_GetDeclSpecs(&declaration, 0);
            break;
    }
    if (tk == '*' || tk == TK_BITAND)
        CDecl_ScanPointer(&declaration, NULL, 0);
    if (tk != TK_IDENTIFIER) {
        if (tk != TK_OPERATOR) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return;
        }
        if (!CParser_00490660(NULL, 1))
            return;
        hasQualifiedName = 1;
    } else
        hasQualifiedName = 0;
    declaration.name = data_00587fa0;
    objects = CScope_FindObjectList(&lookupResult, data_00587fa0);
    if (objects == NULL) {
        name = data_00587fa0;
        CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
        return;
    }
    if (!hasQualifiedName)
        tk = CPrepTokenizer_GetNextToken();
    if (tk == '<') {
        arguments = CTemplateNew_ParseTemplateArguments(NULL, 0);
        if (tk != '>') {
            CError_ReportError(ERR_GREATER_EXPECTED);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
    } else
        arguments = NULL;
    if (tk != '(') {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return;
    }
    CDecl_ParseDirectFuncDecl(&declaration);
    if (declaration.thetype->type != TYPEFUNC) {
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
        return;
    }
    if (tk != ';')
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
    for (; objects != NULL; objects = objects->next) {
        Object *object;
        TemplFuncInstance *result;
        TemplateFunction *nameSpace;

        if ((object = (Object *)objects->object)->otype != OT_OBJECT || object->type->type != TYPEFUNC ||
            (((TypeFunc *)object->type)->flags & 0x400) == 0)
            continue;
        nameSpace = CTemplTool_GetFuncTempl(object);
        result = CTemplateFunc_FindOrCreateMatchedSpecialization(object, declaration.thetype, arguments, NULL);
        if (result == NULL)
            continue;
        if (!instantiate)
            result->is_extern = 1;
        else
            CTemplateNew_InstantiateFunction(nameSpace, result, 1);
        break;
    }
    if (objects == NULL)
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
}

#pragma opt_propagation reset

void parse_function_template_declaration(TemplateScopeState *stack, TemplParam *params, TypeClass *tclass,
                                         SInt32 *startOffset)
{
    Boolean isstatic;
    DeclInfo di;
    DeclInfo destructorDecl;
    CPrepFileInfo *file;
    SInt32 offset;
    Boolean save;
    TemplClass *templclass;
    NameSpaceObjectList *list;
    Object *obj;
    TemplateFunction *templ;
    TypeMemberFunc *member;
    Type *conversionType;
    TypeTemplDep *destructorType;
    TypeTemplDep *dependentType;
    UInt32 conversionQual;

    isstatic = 0;
    data_00582108 = 1;
    memclrw(&di, sizeof(di));
    CParser_GetDeclSpecs((DeclInfo *)&di, 1);
    if (di.storageclass) {
        if (tclass) {
            if (di.storageclass == 0x102)
                isstatic = 1;
            else
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            di.storageclass = 0;
        } else if ((SInt32)di.storageclass != 0x102 && (SInt32)di.storageclass != 0x103) {
            CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            di.storageclass = 0;
        }
    }
    CError_ReportIllegalFlags(di.qual & ~(Q_CV | Q_ASM | Q_PASCAL | Q_INLINE | Q_EXPLICIT | Q_IMPLICIT_WEAK | Q_WEAK |
                                          Q_ALIGNED_MASK | Q_INTERRUPT));

    if (!di.hasTypename) {
        if (tclass && di.thetype->type == TYPECLASS && di.thetype == (Type *)tclass && tk == '(') {
            CError_ASSERT(1027, currentNameSpace->parent == tclass->nspace);
            CError_ReportIllegalFlags(di.qual & ~(Q_INLINE | Q_EXPLICIT));
            di.thetype = (Type *)&void_ptr;
            di.isConstructor = 1;
            CDecl_ParseDirectFuncDecl(&di);
            if (di.thetype->type == TYPEFUNC) {
                FuncArg *args;
                if ((args = TYPE_FUNC(di.thetype)->args) && !args->next && args->type == (Type *)tclass) {
                    CError_ReportError(ERR_ILLEGAL_COPY_CONSTRUCTOR);
                    TYPE_FUNC(di.thetype)->args = NULL;
                }
                if (tclass->flags & CLASS_HAS_VBASES)
                    CDecl_PrependFuncArg(TYPE_FUNC(di.thetype), &stsignedshort);
                TYPE_FUNC(di.thetype)->flags |= FUNC_IS_DTOR;
                di.name = constructor_name;
                currentNameSpace = stack->scope->parent;
                goto declare;
            }
            CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
        }

        if (di.thetype->type == TYPETEMPLATE) {
            if (tk == '(' && ((TypeTemplDep *)di.thetype)->dtype == 1 &&
                (templclass = CTemplTool_IsTemplate(((TypeTemplDep *)di.thetype)->u.qual.type)) &&
                ((TypeTemplDep *)di.thetype)->u.qual.name == templclass->theclass.classname) {
                if (tclass)
                    CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
                di.thetype = (Type *)&void_ptr;
                di.isConstructor = 1;
                CTempl_PushScope(stack, &templclass->theclass);
                CDecl_ParseDirectFuncDecl(&di);
                if (di.thetype->type == TYPEFUNC) {
                    di.name = constructor_name;
                    if (templclass->theclass.flags & CLASS_HAS_VBASES)
                        CDecl_PrependFuncArg(TYPE_FUNC(di.thetype), &stsignedshort);
                    TYPE_FUNC(di.thetype)->flags |= FUNC_IS_DTOR;
                    parse_template_member_definition(params, &templclass->theclass, &di, startOffset);
                    return;
                }
                CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                return;
            }

            if (tk == TK_COLON_COLON && (dependentType = (TypeTemplDep *)di.thetype)->dtype == 2 &&
                (templclass = CTemplTool_IsTemplate(dependentType))) {
                if (tclass)
                    CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '~') {
                    tk = CPrepTokenizer_GetNextToken();
                    if (!(tk == TK_IDENTIFIER && data_00587fa0 == templclass->theclass.classname &&
                          (tk = CPrepTokenizer_GetNextToken()) == '(')) {
                        if (tk == '<') {
                            CPrep_UngetToken();
                            tk = TK_IDENTIFIER;
                            data_00587fa0 = templclass->theclass.classname;
                            memclrw(&destructorDecl, sizeof(destructorDecl));
                            CParser_GetDeclSpecs((DeclInfo *)&destructorDecl, 0);
                            if (tk != '(')
                                CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                            if (destructorDecl.thetype != (Type *)templclass &&
                                !((destructorType = (TypeTemplDep *)destructorDecl.thetype)->type == TYPETEMPLATE &&
                                  CTemplateTools_GetTemplClass(destructorType) == (TypeClass *)templclass))
                                CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                        } else {
                            CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                        }
                    }
                    di.thetype = (Type *)&void_ptr;
                    CTempl_PushScope(stack, &templclass->theclass);
                    CDecl_ParseDirectFuncDecl(&di);
                    if (di.thetype->type == TYPEFUNC) {
                        if (templclass->theclass.sominfo)
                            di.qual |= Q_VIRTUAL;
                        else
                            CDecl_PrependFuncArg(TYPE_FUNC(di.thetype), &stsignedshort);
                        di.name = destructor_name;
                        TYPE_FUNC(di.thetype)->flags |= 0x4000;
                        parse_template_member_definition(params, &templclass->theclass, &di, startOffset);
                        return;
                    }
                    CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                    return;
                }
                if (tk == TK_OPERATOR) {
                    CTempl_PushScope(stack, &templclass->theclass);
                    tk = CPrepTokenizer_GetNextToken();
                    if (CMangler_OperatorName(tk)) {
                        CError_ReportError(ERR_IMPLICIT_INT_NO_LONGER_SUPPORTED_C);
                        return;
                    }
                    CError_ReportIllegalFlags(di.qual & ~(Q_INLINE | Q_VIRTUAL));
                    conversion_type_name(&di);
                    if (tk != '(')
                        CError_ReportError(ERR_LPAREN_EXPECTED);
                    else
                        tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_VOID)
                        tk = CPrepTokenizer_GetNextToken();
                    if (tk != ')')
                        CError_ReportError(ERR_RPAREN_EXPECTED);
                    else
                        tk = CPrepTokenizer_GetNextToken();
                    conversionType = di.thetype;
                    conversionQual = di.qual;
                    CDecl_NewConvFuncType(&di);
                    if (CTemplateTools_IsDependentType(conversionType) &&
                        !(di.name = CTempl_FindConversion(&templclass->theclass, conversionType, conversionQual))) {
                        CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, "conversion function");
                        return;
                    }
                    parse_template_member_definition(params, &templclass->theclass, &di, startOffset);
                    return;
                }
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }

            if (((TypeTemplDep *)di.thetype)->dtype == 1 && !di.isType)
                CError_Warning(ERR_TYPENAME_MISSING_TEMPLATE_ARGUMENT_DEPENDENT_QUALIFIED);
        }
    }

    di.templateParameters = params;
    di.templateScope = stack;
    CDecl_ParseDeclarator(&di);
    data_00582108 = 0;
    if (currentNameSpace->is_templ) {
        CError_ASSERT(1192, currentNameSpace == stack->scope);
        currentNameSpace = stack->scope->parent;
    }

    if (di.name && di.nspace && di.nspace->theclass && (di.nspace->theclass->flags & CLASS_IS_TEMPL)) {
        if (tclass)
            CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
        parse_template_member_definition(params, di.nspace->theclass, &di, startOffset);
        return;
    }

    if (!di.name || di.thetype->type != TYPEFUNC || (di.nspace && di.nspace->theclass)) {
        CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
        return;
    }

    for (list = CScope_FindName(currentNameSpace, di.name); list; list = list->next) {
        if ((obj = OBJECT(list->object))->otype == OT_OBJECT && obj->type->type == TYPEFUNC &&
            (TYPE_FUNC(obj->type)->flags & 0x400)) {
            templ = CTemplTool_GetFuncTempl(obj);
            if (CTemplTool_EqualParams(templ->params, params, 0) && iscpp_typeequal(obj->type, di.thetype)) {
                if (tk != ';' && templ->stream.tokens)
                    CError_ReportError(ERR_TEMPLATE_REDEFINED);
                if (tk == '{' || tk == ':' || tk == TK_TRY)
                    CError_ASSERT(1227, CTemplTool_EqualParams(templ->params, params, 1));
                TYPE_FUNC(obj->type)->args = TYPE_FUNC(di.thetype)->args;
                break;
            }
        }
    }

    if (!list) {
    declare:
        if (tclass) {
            CError_ASSERT(1240, currentNameSpace->theclass);
            member = CDecl_NewTypeMemberFunc(TYPE_FUNC(di.thetype), currentNameSpace->theclass, isstatic, 1);
            di.thetype = (Type *)member;
        }
        CTEMPL_DECLARE(di, params);
    }

    if (tk == '{' || tk == ':' || tk == TK_TRY) {
        if (tclass) {
            obj->qual |= Q_INLINE;
            obj->sclass = TK_STATIC;
        }
        if (TYPE_FUNC(obj->type)->flags & FUNC_DEFINED)
            CError_ReportError(ERR_OBJECT_REDEFINED, obj);
        TYPE_FUNC(obj->type)->flags |= 0x8000002;
        CPrep_SaveFunctionBodyTokens(&templ->stream, NULL, 0);
        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == ';')
            tk = CPrepTokenizer_GetNextToken();
        else
            tk = ';';
        CTEMPL_RECORDSOURCE(templ, startOffset);
    } else if (tk != ';') {
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
    }
}

void parse_template_member_definition(void *context, TypeClass *template_info, DeclInfo *declaration, SInt32 *position)
{
    Object *object;
    ObjectList *entry;
    int is_saved_function;
    struct TemplateMember *instance;
    char saved_state;
    TemplateFunction *function_info;
    TokenStream parsed_body;
    Boolean option;
    CPrepFileInfo *start;
    SInt32 end;

    declaration->thetype =
        CTemplTool_ResolveMemberSelfRefs((TemplClass *)template_info, declaration->thetype, &declaration->qual);
    if (declaration->thetype->type == TYPEFUNC) {
        TypeFunc *function_type;
        object = CDecl_GetFunctionObject(declaration, NULL, &option, 0);
        if (object == NULL) {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, declaration->name->name);
            return;
        }
        if (tk != '{' && tk != TK_TRY && tk != ':') {
            if (tk != ';') {
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
            } else {
                tk = CPrepTokenizer_GetNextToken();
            }
            return;
        }
        function_type = (TypeFunc *)object->type;
        if ((function_type->flags & FUNC_DEFINED) != 0) {
            CError_ReportError(ERR_OBJECT_REDEFINED, object);
        }
        function_type = (TypeFunc *)object->type;
        function_type->flags |= 134217728 | FUNC_DEFINED;
        CPrep_SaveFunctionBodyTokens(&parsed_body, NULL, 1);
        saved_state = template_recordbrowseinfo;
        template_recordbrowseinfo = 1;
        CPrep_GetBrowseFilePosition(&start, &end);
        template_recordbrowseinfo = saved_state;
        if (start != NULL && start->recordbrowseinfo != 0 && *position >= 0 && end > *position) {
            CBrowse_ForwardObjectFileRange(object, start, start, *position, end + 1);
        }
    } else {
        entry = CScope_FindObjectListInNameSpace(template_info->nspace, declaration->name);
        if (entry == NULL) {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, declaration->name->name);
            return;
        }
        object = entry->object;
        if (object->otype != OT_OBJECT) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED, declaration->name->name);
            return;
        }
        if (iscpp_typeequal(declaration->thetype, object->type) == 0 ||
            (object->qual & (Q_CV | Q_PASCAL)) != (declaration->qual & (Q_CV | Q_PASCAL))) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(object),
                               object->type, object->qual, declaration->thetype, declaration->qual);
            return;
        }
        CE_ASSERT(object->datatype != DDATA, CError_FATAL(869));
        CPrep_BufferTokensThroughSemicolon(&parsed_body, NULL);
    }
    if (parsed_body.tokens != 0) {
        is_saved_function = object->type->type == TYPEFUNC && (((TypeFunc *)object->type)->flags & 1024) != 0;
        if (is_saved_function) {
            if (CTemplTool_EqualParams(object->u.templateFunction->params, context, 0) == 0) {
                CError_ReportError(ERR_TEMPLATE_PARAMETER_MISMATCH);
            }
            function_info = object->u.templateFunction;
            function_info->stream = parsed_body;
        } else {
            instance = CTemplateClass_AddTemplateArgumentOverride((TemplClass *)template_info, object,
                                                                  &function_fileinfo, &parsed_body);
            if (((TemplClass *)template_info)->templ__params != NULL) {
                if (CTemplTool_EqualParams(((TemplClass *)template_info)->templ__params, context, 0) == 0) {
                    CError_ReportError(ERR_TEMPLATE_PARAMETER_MISMATCH);
                } else {
                    instance->params = context;
                }
            }
        }
    }
}

UInt8 CTemplateNew_LinkTemplateScope(DeclInfo *context, TypeTemplDep *request, NameSpace **destination)
{
    TemplateScopeState *entry;
    TypeClass *slot;
    TemplClass *result;

    if (request->dtype == 1) {
        result = CTemplTool_IsTemplate(request->u.qual.type);
        if (result != NULL) {
            *destination = result->theclass.nspace;
            if (context->templateScope == NULL || (*destination)->theclass == NULL) {
                CError_FATAL(757);
            }
            slot = (*destination)->theclass;
            entry = context->templateScope;
            if (entry->linkedNamespace != NULL) {
                CError_FATAL(734);
            }
            entry->scope->parent = slot->nspace->parent;
            slot->nspace->parent = entry->scope;
            entry->linkedNamespace = slot->nspace;
            currentNameSpace = (NameSpace *)slot->nspace;
            context->templateScope = NULL;
            return 1;
        }
    }
    return 0;
}

Type *CTemplTool_GetSelfRefTemplate(TemplClass *record)
{
    TemplArg *key;
    Type *result;
    TemplClass *underlying;
    TemplClass *selected;
    TemplArg *lookupContext;
    underlying = (selected = record)->pspec_owner;
    if (underlying != NULL) {
        selected = underlying;
    }
    key = parse_template_arguments(&selected, &lookupContext);
    if (key == NULL) {
        return &stvoid;
    }
    result = CTemplTool_IsDependentTemplate(selected, key);
    if (result != NULL) {
        return result;
    }
    return (Type *)CTemplateClass_GetInstance(selected, key, lookupContext);
}

TemplArg *parse_template_arguments(TemplClass **classType, TemplArg **result)
{
    TemplArg *argument;
    TemplParam *parameter;
    TemplArg **tail;
    Type *substituted;
    short token;
    TemplParam *parameters;
    TemplArg *arguments;
    DeclInfo parse;
    unsigned int substitutionInfo;
    TypeDeduce context;
    TypeDeduce defaultContext;
    parameters = (*classType)->templ__params;
    *result = NULL;
    if (tk != '<') {
        CError_ReportError(ERR_LESS_EXPECTED);
        return NULL;
    }
    token = CPrepTokenizer_GetNextToken();
    arguments = NULL;
    tk = token;
    parameter = parameters;
    tail = &arguments;
    for (; parameter != NULL; parameter = parameter->next) {
        argument = (TemplArg *)galloc(sizeof(*argument));
        memclrw(argument, sizeof(*argument));
        *tail = argument;
        tail = &argument->next;
        argument->pid = parameter->pid;
        if (tk != '>') {
            if (argument->pid.type != 0) {
                memclrw(&parse, sizeof(parse));
                CParser_GetDeclSpecs(&parse, 0);
                if (parse.storageclass != 0) {
                    CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                }
                if (parse.missingTypeSpecifier != 0) {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                }
                CError_ReportIllegalFlags(parse.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
                CDecl_ParseDeclarator(&parse);
                if (parse.name != NULL) {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                }
                CTemplTool_CheckTemplArgType(parse.thetype);
                argument->data.typeparam.type = parse.thetype;
                argument->data.typeparam.qual = parse.qual;
            } else if (CTemplateTools_IsDependentType(parameter->data.paramdecl.type) != 0) {
                substituted = CTemplateTools_GetArgumentType(arguments, (TypeTemplDep *)parameter->data.paramdecl.type,
                                                             parameter->data.paramdecl.qual, &substitutionInfo);
                argument->data.paramdecl.expr = parse_non_type_template_argument(substituted, substitutionInfo);
            } else {
                argument->data.paramdecl.expr =
                    parse_non_type_template_argument(parameter->data.paramdecl.type, parameter->data.paramdecl.qual);
            }
            if (tk == '>') {
                continue;
            }
            if (tk != ',') {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
                return NULL;
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '>') {
                continue;
            }
            CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
            return NULL;
        }
        if (argument->pid.type != 0) {
            if (parameter->data.typeparam.type == NULL) {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
                return NULL;
            }
            argument->data.typeparam.type = parameter->data.typeparam.type;
            argument->data.typeparam.qual = parameter->data.typeparam.qual;
            if (parameter->data.typeparam.isTypeDependent != 0) {
                memclrw(&context, sizeof(context));
                context.tmclass = *classType;
                context.params = parameters;
                context.inst = NULL;
                context.args = arguments;
                argument->data.typeparam.qual = parameter->data.typeparam.qual;
                argument->data.typeparam.type = CTemplateTools_ResolveType(&context, argument->data.typeparam.type,
                                                                           (UInt32 *)&argument->data.typeparam.qual);
            }
        } else {
            if (parameter->data.paramdecl.defaultarg == NULL) {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
                return NULL;
            }
            if (parameter->data.paramdecl.defaultarg->rtype->type == TYPETEMPLDEPEXPR) {
                memclrw(&defaultContext, sizeof(defaultContext));
                defaultContext.tmclass = *classType;
                defaultContext.params = parameters;
                defaultContext.inst = NULL;
                defaultContext.args = arguments;
                argument->data.paramdecl.expr =
                    fn_00513040(CTemplTool_DeduceExpr(&defaultContext, parameter->data.paramdecl.defaultarg), 1);
            } else {
                argument->data.paramdecl.expr = parameter->data.paramdecl.defaultarg;
            }
        }
    }
    if (tk != '>') {
        CError_ReportError(ERR_GREATER_EXPECTED);
        return NULL;
    }
    if ((*classType)->pspecs != NULL) {
        return arguments;
    }
    return arguments;
}

TemplArg *CTemplateNew_ParseTemplateArguments(TemplParam *parameter, char useGlobalAllocator)
{
    TemplArg *head;
    TemplArg *tail;
    SInt16 index;
    char argumentKind;
    DeclInfo declaration;

    if (tk != '<') {
        CError_ReportError(ERR_LESS_EXPECTED);
        return NULL;
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '>')
        return NULL;
    head = NULL;
    index = 0;
    do {
        if (useGlobalAllocator) {
            if (head != NULL) {
                tail->next = galloc(sizeof(*tail));
                tail = tail->next;
            } else {
                tail = galloc(sizeof(*tail));
                head = tail;
            }
        } else {
            if (head != NULL) {
                tail->next = CompilerTools_AllocatePool(sizeof(*tail));
                tail = tail->next;
            } else {
                tail = galloc(sizeof(*tail));
                head = tail;
            }
        }
        tail->next = NULL;
        if (parameter == NULL) {
            tail->pid.index = index;
            tail->pid.nindex = 0;
            argumentKind = isdeclaration(1, 1, 0, 0);
            tail->pid.type = argumentKind;
        } else {
            tail->pid = parameter->pid;
            argumentKind = parameter->pid.type;
            parameter = parameter->next;
        }
        if (argumentKind != 0) {
            memclrw(&declaration, sizeof(declaration));
            CParser_GetDeclSpecs(&declaration, 0);
            if (declaration.storageclass != 0)
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            CError_ReportIllegalFlags(declaration.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
            CDecl_ParseDeclarator(&declaration);
            if (declaration.name != NULL)
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            CTemplTool_CheckTemplArgType(declaration.thetype);
            tail->data.typeparam.type = declaration.thetype;
            tail->data.typeparam.qual = declaration.qual;
        } else {
            tail->data.paramdecl.expr = parse_non_type_template_argument(NULL, 0);
        }
        if (tk == '>')
            return head;
        if (tk != ',') {
            CError_ReportError(ERR_COMMA_EXPECTED);
            return NULL;
        }
        tk = CPrepTokenizer_GetNextToken();
        index++;
    } while (1);
}

TemplParam *parse_template_parameter_list(NameSpace *parserState, unsigned char mode)
{
    TemplParam **tail;
    TemplParam *argument;
    TemplParam *previous;
    short index;
    TemplParam *arguments;

    arguments = NULL;
    index = 0;
    tail = &arguments;
    for (;;) {
        argument = parse_template_parameter(parserState, arguments, index, mode);
        if (argument == NULL)
            break;
        if (argument->name != NULL) {
            for (previous = arguments; previous != NULL; previous = previous->next) {
                if (previous->name == argument->name) {
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, argument->name->name);
                }
            }
        }
        *tail = argument;
        tail = &argument->next;
        if (tk != ',')
            break;
        tk = CPrepTokenizer_GetNextToken();
        index = index + 1;
    }
    if (arguments == NULL) {
        CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
    }
    return arguments;
}

TemplParam *parse_template_parameter(NameSpace *owner, TemplParam *value, short memberValue, char memberByte)
{
    TemplParam *declaration;
    DeclInfo parsed;
    DeclInfo initializerState;

    declaration = (TemplParam *)galloc(sizeof(TemplParam));
    memclrw(declaration, sizeof(*declaration));
    declaration->pid.index = memberValue;
    switch (declaration->pid.nindex = memberByte, tk) {
        case 0x112:
        case 0x120:
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_IDENTIFIER) {
                declaration->name = data_00587fa0;
                tk = CPrepTokenizer_GetNextToken();
            }
            if (tk == '=') {
                tk = CPrepTokenizer_GetNextToken();
                memclrw(&initializerState, sizeof(initializerState));
                CParser_GetDeclSpecs((DeclInfo *)&initializerState, '\0');
                if (initializerState.missingTypeSpecifier != '\0') {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                }
                if (initializerState.storageclass != 0) {
                    CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                }
                CError_ReportIllegalFlags(initializerState.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
                CDecl_ParseDeclarator(&initializerState);
                declaration->data.typeparam.type = initializerState.thetype;
                declaration->data.typeparam.qual = initializerState.qual,
                declaration->data.typeparam.isTypeDependent = CTemplateTools_IsDependentType(initializerState.thetype);
            }
            declaration->pid.type = 1;
            break;
        case 0x14c:
            CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
            return NULL;
        default:
            memclrw(&parsed, sizeof(parsed));
            CParser_GetDeclSpecs((DeclInfo *)&parsed, '\0');
            if (parsed.storageclass != 0) {
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            }
            CError_ReportIllegalFlags(parsed.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
            CDecl_ParseDeclarator(&parsed);
            switch (*(char *)parsed.thetype) {
                case '\x01':
                case '\x03':
                case '\t':
                case '\v':
                case '\f':
                    break;
                case '\n':
                    CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
                    parsed.thetype = (Type *)(&stsignedint);
                    break;
                default:
                    CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
                    parsed.thetype = (Type *)(&stsignedint);
                    break;
            }
            declaration->name = parsed.name, declaration->data.paramdecl.type = parsed.thetype,
            declaration->data.paramdecl.qual = parsed.qual;
            if (tk == '=') {
                tk = CPrepTokenizer_GetNextToken();
                declaration->data.paramdecl.defaultarg = parse_non_type_template_argument(parsed.thetype, parsed.qual);
            }
            break;
    }
    if (declaration->name != NULL) {
        CTemplTool_InsertTemplateParameter(owner, declaration);
    }
    return declaration;
}

ENode *parse_non_type_template_argument(Type *targetType, unsigned int qualifiers)
{
    ENode *expr;
    ENode *copy;
    non_type_template_argument_mode = 1;
    expr = conv_assignment_expression();
    non_type_template_argument_mode = 0;
    if (targetType != NULL && !CTemplateTools_IsDependentType(targetType) && expr->rtype->type != TYPETEMPLDEPEXPR) {
        expr = CExpr_AssignmentPromotion(expr, targetType, qualifiers, 1);
        if (targetType->type == TYPEPOINTER) {
            if (expr->type == ETYPCON && expr->data.monadic->type == EINTCONST) {
                expr = expr->data.monadic;
            }
            if (expr->type == EINTCONST) {
                expr->rtype = targetType;
            }
        }
    }
    if (expr->rtype->type != TYPETEMPLDEPEXPR) {
        switch (expr->type) {
            case EOBJREF:
                if (CParser_HasInternalLinkage(expr->data.objref)) {
                    CError_ReportError(ERR_TEMPLATE_NON_TYPE_ARGUMENT_OBJECTS_SHALL);
                }
                break;
            case EINTCONST:
            case ENEWEXCEPTION:
                break;
            default:
                CError_ReportError(ERR_ILLEGAL_NON_TYPE_TEMPLATE_ARGUMENT);
                expr = nullnode();
                break;
        }
        copy = (ENode *)galloc(26);
        *(ENode *)copy = *(ENode *)expr;
        return copy;
    }
    return fn_00513040(expr, 1);
}

void skip_balanced_angle_tokens(void)
{
    struct {
        char ch;
        char flag;
    } stack[256];
    int i;

    stack[0].ch = '>';
    stack[0].flag = 1;
    i = 0;
    for (;;) {
        tk = CPrepTokenizer_GetNextToken();
        switch (tk) {
            case ';':
            case '=':
            case '{':
            case '}':
                longjmp(template_declaration_jmpbuf, 1);
                break;
        }
        switch (tk) {
            case '<':
                if (stack[i].flag != 0) {
                    if (i >= 0xff)
                        longjmp(template_declaration_jmpbuf, 1);
                    i++;
                    stack[i].ch = '>';
                    stack[i].flag = 1;
                }
                break;
            case '(':
                if (i >= 0xff)
                    longjmp(template_declaration_jmpbuf, 1);
                i++;
                stack[i].ch = ')';
                stack[i].flag = 0;
                break;
            case '[':
                if (i >= 0xff)
                    longjmp(template_declaration_jmpbuf, 1);
                i++;
                stack[i].ch = ']';
                stack[i].flag = 0;
                break;
            case '>':
                if (stack[i].flag == 0)
                    break;
            case ')':
            case ']':
                if (tk != stack[i].ch)
                    longjmp(template_declaration_jmpbuf, 1);
                i--;
                if (i < 0)
                    return;
                break;
        }
    }
}

void fn_004f0000(void)
{
    return;
}

void CTemplateNew_Reset(void)
{
    object_reference_stack = NULL;
    class_template_list = NULL;
    templateFunctions = NULL;
    objectReferenceEntryCount = 0;
    data_00582108 = 0;
    return;
}
