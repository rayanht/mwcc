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
    TemplateObjectInstance *node;
    TypeClassExt800 *theclass;
    Object *func;
    TemplateSourceRecordTyped *entry;

    CError_ASSERT(1981, obj->type->type == TYPEFUNC && (obj->qual & Q_IS_TEMPLATED));
    if (!(TYPE_FUNC(obj->type)->flags & FUNC_DEFINED)) {
        theclass = (TypeClassExt800 *)TYPE_METHOD(obj->type)->theclass;
        if (!theclass->suppressImplicitInstantiation) {
            node = (TemplateObjectInstance *)obj;
            func = node->templateObject;
            if (func->qual & Q_INLINE) {
                entry = (TemplateSourceRecordTyped *)CTemplateClass_ResolveRelatedClass(
                            (TypeClassTemplate *)theclass->classTemplate)
                            ->templateArgumentOverrides;
                for (; entry != NULL; entry = entry->next) {
                    obj->qual |= Q_INLINE;
                    if (entry->object == func) {
                        CTemplTool_MergeArgNames(entry->object->type, obj->type);
                        CInline_AddFunctionPrecNode(obj, &theclass->base, &entry->sourceInfo, &entry->state, 0);
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
    TypeClassTemplate *node;
    TemplateFunction *nspace;
    TypeClassExt800 *classEntry;
    Type *classTemplate;
    ClassTemplateSpecialization *pair;
    TypeClassExt800 *nestedClass;
    NameSpaceObjectList *found;
    TemplateSpecializationData *entry;

    result = 0;
    node = class_template_list;
    if (node != NULL) {
        do {
            for (classEntry = (TypeClassExt800 *)node->instances; classEntry != NULL; classEntry = classEntry->next) {
                if ((classEntry->base.flags & CLASS_IS_TEMPL_INST) != 0 &&
                    classEntry->suppressImplicitInstantiation == 0 &&
                    (classTemplate = classEntry->classTemplate, instantiate_members(classTemplate, classEntry, 0)))
                    result = 1;
            }
            for (pair = node->specializations; pair != NULL; pair = pair->next) {
                for (nestedClass = (TypeClassExt800 *)pair->type->instances; nestedClass != NULL;
                     nestedClass = nestedClass->next) {
                    if ((nestedClass->base.flags & CLASS_IS_TEMPL_INST) != 0 &&
                        nestedClass->suppressImplicitInstantiation == 0 &&
                        instantiate_members((Type *)pair->type, nestedClass, 0))
                        result = 1;
                }
            }
            node = node->next;
        } while (node != NULL);
    }

    for (nspace = templateFunctions; nspace != NULL; nspace = nspace->next) {
        for (entry = (TemplateSpecializationData *)nspace->objects; entry != NULL; entry = entry->next) {
            if (entry->active == 0 && entry->internalStorage == 0) {
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
                    entry->active = 1;
                    if (CTemplateNew_InstantiateFunction(nspace, entry, 0))
                        result = 1;
                }
            }
        }
    }
    return result;
}

static inline Object *TemplateObjectInstance_Source(Object *object)
{
    return ((TemplateObjectInstance *)object)->templateObject;
}

static inline char CTemplateNew_004ed8e0_inline1(TypeClassExt800 *a1, Object *v1, char a2)
{
    Object *v3;
    TypeClassTemplate *t1;
    TemplateSourceRecordTyped *v5;
    char v6;
    t1 = (TypeClassTemplate *)a1->classTemplate;
    v3 = TemplateObjectInstance_Source(v1);
    v5 = (TemplateSourceRecordTyped *)CTemplateClass_ResolveRelatedClass(t1)->templateArgumentOverrides;
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

static inline char CTemplateNew_004ed8e0_inline2(TypeClassExt800 *a1, Object *v1, char a2)
{
    TypeClassTemplate *t2;
    Object *v7;
    TemplateSourceRecordTyped *v9;
    char v10;
    t2 = (TypeClassTemplate *)a1->classTemplate;
    v7 = TemplateObjectInstance_Source(v1);
    v9 = (TemplateSourceRecordTyped *)CTemplateClass_ResolveRelatedClass(t2)->templateArgumentOverrides;
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

unsigned char instantiate_members(Type *unused, TypeClassExt800 *state, char force)
{
    Object *entry;
    char functionResult;
    char entryResult;
    char changed;
    ScopeSearch iterator;
    changed = 0;
    if (force == 0 && state->memberInstantiationState[0] != 0)
        return (char)0;
    CScope_InitScopeSearch(&iterator, state->base.nspace);
    entry = CScope_NextObject(&iterator);
    while (entry != NULL) {
        if (entry->type->type == TYPEFUNC) {
            if ((force != 0 || (entry->flags & 2) != 0) &&
                (((TypeFunc *)entry->type)->flags & (FUNC_AUTO_GENERATED | FUNC_DEFINED)) == 0) {
                functionResult = CTemplateNew_004ed8e0_inline1(state, entry, force);
                if (functionResult != 0 && (((TypeFunc *)entry->type)->flags & FUNC_DEFINED) != 0)
                    changed = 1;
            }
        } else if (state->memberInstantiationState[1] == 0 && entry->datatype == DDATA &&
                   (entry->qual & Q_INLINE_DATA) == 0 && (entry->flags & 4) == 0) {
            entryResult = CTemplateNew_004ed8e0_inline2(state, entry, force);
            if (entryResult != 0)
                changed = 1;
        }
        entry = CScope_NextObject(&iterator);
    }
    state->memberInstantiationState[1] = 1;
    return changed;
}

/* Source record used by template instantiation. */
/* Working record passed to fn_004763e0. */

void CTemplateNew_CompileObject(TypeClassTemplate *templateClass, TypeClassExt800 *context,
                                TemplateSourceRecordTyped *source, Object *object, Boolean reset)
{
    CScopeSave scope;
    SInt32 savedState;
    DeclInfo compilation;
    NameSpace *arguments;
    UInt8 savedFileSymInfo;
    UInt8 savedTemplateState;
    struct ObjectReferenceEntry sa;

    FunctionCalls_PushObjectReferenceEntry(&sa, NULL, object);
    arguments = CTemplateTools_InsertTemplateArgs(
        source->context ? source->context : (FuncArg *)templateClass->templateParameters, context, &scope);
    CPrep_InsertTokenBuffer(&source->state, &savedState);
    savedTemplateState = template_recordbrowseinfo;
    template_recordbrowseinfo = 1;
    source_line = source->sourceLine;
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
    compilation.sourceFile = source->sourceFile;
    compilation.browseFile = CPrep_GetPFile();
    compilation.sourceLine = source->sourceLine;
    switch (object->datatype) {
        case DFUNC:
        case DVFUNC:
            CTemplTool_MergeArgNames(source->object->type, object->type);
            CFunc_ParseFuncDef(object, &compilation, &context->base, 0, 0, NULL);
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
    CPrep_RemoveBufferedTokens(&source->state.count, &savedState);
    copts.filesyminfo = savedFileSymInfo;
    template_recordbrowseinfo = savedTemplateState;
}

Boolean CTemplateNew_InstantiateFunction(TemplateFunction *definition, TemplateSpecializationData *specialization,
                                         Boolean report)
{
    UInt8 savedFileSymbolInfo;
    SInt32 savedStream;
    DeclInfo workspace;
    struct ObjectReferenceEntry parserState;
    NameSpace *namespace;

    if (specialization->suppressImplicitInstantiation != 0 && !report)
        return 0;
    if (definition->stream.count == 0) {
        if (report) {
            CError_SetBufferedToken(&definition->fileoffset);
            CError_ReportError(ERR_CANNOT_INSTANTIATE, specialization->object);
        }
        return 0;
    }
    specialization->active = 1;
    CPrep_InsertTokenBuffer(&definition->stream, &savedStream);
    savedFileSymbolInfo = copts.filesyminfo;
    if (copts.fbf != 0 || definition->fileoffset.tokenfile == NULL)
        copts.filesyminfo = 0;
    tk = CPrepTokenizer_GetNextToken();
    if (tk != '{' && tk != ':' && tk != TK_TRY)
        CError_FATAL(1689);
    declaration_token = *CPrep_GetLastBufferedToken();
    if (copts.filesyminfo != 0) {
        CPrep_GetFOI(&function_fileinfo, &definition->fileoffset);
        data_00587184 = data_00588454;
    }
    if (specialization->object->sclass != TK_STATIC) {
        if (report)
            specialization->object->sclass = TK_EXTERN;
        else
            specialization->object->qual |= Q_WEAK;
    }
    memclrw(&workspace, sizeof(workspace));
    workspace.sourceFile = definition->srcfile;
    workspace.browseFile = CPrep_GetPFile();
    workspace.sourceLine = definition->startoffset;
    FunctionCalls_PushObjectReferenceEntry(&parserState, NULL, specialization->object);
    CTemplTool_MergeArgNames(definition->tfunc->type, specialization->object->type);
    namespace =
        CTemplTool_SetupTemplateArgumentNameSpace((FuncArg *)definition->params, specialization->templateArguments, 0);
    namespace->parent = specialization->object->nspace;
    specialization->object->nspace = namespace;
    CTemplTool_SetupOuterTemplateArgumentNameSpace(namespace);
    CFunc_ParseFuncDef(specialization->object, &workspace, NULL, 0, 0, namespace);
    CTemplTool_RemoveOuterTemplateArgumentNameSpace(namespace);
    specialization->object->nspace = namespace->parent;
    CTemplateTools_PopObjectReferenceEntry(&parserState);
    CPrep_RemoveBufferedTokens(&definition->stream.count, &savedStream);
    copts.filesyminfo = savedFileSymbolInfo;
    if (workspace.browseFile->recordbrowseinfo != 0)
        CBrowse_ForwardObjectFileRange(specialization->object, workspace.browseFile, workspace.sourceFile,
                                       workspace.sourceLine, definition->endoffset);
    return 1;
}

void CTemplateNew_ParseFuncDef(Object *object, TypeClassExt800 *context, TplSpec *specialization)
{
    DeclInfo declInfo;
    CScopeSave contextSave;
    struct ObjectReferenceEntry objectSave;
    FuncArg *templateParameters;
    NameSpace *arguments;
    TemplateArgumentOverride *node;
    SInt32 key;
    TypeClassTemplate *templateClass;

    templateClass = (TypeClassTemplate *)context->classTemplate;
    templateParameters = (FuncArg *)templateClass->templateParameters;
    if ((object->qual & Q_IS_TEMPLATED) != 0) {
        templateClass = (TypeClassTemplate *)context->classTemplate;
        node = (TemplateArgumentOverride *)CTemplateClass_ResolveRelatedClass(templateClass)->templateArgumentOverrides;
        key = object->templateMember[0].templateArgumentKey;
        while (node != NULL) {
            if (node->templateArgumentKey == key) {
                if (node->templateParameters != NULL)
                    templateParameters = node->templateParameters;
                break;
            }
            node = node->next;
        }
    }
    FunctionCalls_PushObjectReferenceEntry(&objectSave, NULL, object);
    arguments = CTemplateTools_InsertTemplateArgs(templateParameters, context, &contextSave);
    if (specialization != NULL)
        currentNameSpace = (NameSpace *)specialization->val;
    memclrw(&declInfo, sizeof(declInfo));
    CFunc_ParseFuncDef(object, &declInfo, NULL, 0, 0, specialization != NULL ? currentNameSpace : NULL);
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
    TemplateParameterRecord *parameters;
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
    TypeClassExt800 *templateClass;

    memclrw(&ctx, sizeof(ctx));
    ctx.requireTemplateClassMember = 1;
    ctx.allowTemplateArguments = 1;
    CParser_GetDeclSpecs(&ctx, 1);
    if (tk == ';') {
        if (ctx.dtype->type == TYPECLASS && ((classType = (TypeClass *)ctx.dtype)->flags & CLASS_IS_TEMPL_INST) != 0) {
            templateClass = (TypeClassExt800 *)ctx.dtype;
            templateClass->suppressImplicitInstantiation = 1;
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
    ScopeSearch iter;
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
        templ->fileoffset = declaration_token;                                                                         \
        obj = CParser_NewFunctionObject(NULL);                                                                         \
        obj->name = (di).name;                                                                                         \
        obj->u.func.linkname = CParser_GetUniqueName();                                                                \
        obj->type = (di).dtype;                                                                                        \
        obj->qual = (di).qual | Q_MANGLE_NAME;                                                                         \
        obj->sclass = (di).storage;                                                                                    \
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

static void *FindInst(TypeClassTemplate **p8c, CTStateElem **p88)
{
    Type *p;
    CTStateElem *t2;

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
    CScopeParseResult lookupResult;
    DeclInfo declaration;
    TypeClassTemplate *templateClass;
    CTStateElem *arguments;
    NameSpaceObjectList *objects;
    UInt8 declarationFlags;
    UInt8 hasQualifiedName;
    UInt8 instantiate;
    CTStateElem *instanceData;
    TypeClassTemplate *templateData;
    TypeClassExt800 *instance;
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
            declaration.dtype = CScope_FindTagType(currentNameSpace, data_00587fa0);
            if (declaration.dtype == NULL) {
                name = data_00587fa0;
                CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
                return;
            }
            if (declaration.dtype->type == TYPECLASS &&
                (((TypeClassExt800 *)declaration.dtype)->base.flags & CLASS_IS_TEMPL) != 0) {
                templateClass = (TypeClassTemplate *)declaration.dtype;
                tk = CPrepTokenizer_GetNextToken();
                if (tk != '<') {
                    CError_ReportError(ERR_LESS_EXPECTED);
                    return;
                }
                templateData = (TypeClassTemplate *)(TypeClass *)templateClass;
                if (templateData->replacementTemplate)
                    templateData = (TypeClassTemplate *)templateData->replacementTemplate;
                declaration.dtype = FindInst(&templateData, &instanceData);
                tk = CPrepTokenizer_GetNextToken();
                if (tk == ';') {
                    if (declaration.dtype->type == TYPECLASS) {
                        TypeClassExt800 *instantiated;
                        instantiated = (TypeClassExt800 *)declaration.dtype;
                        CTemplateClass_InstantiateClass(&instantiated->base);
                        if ((((TypeClassExt800 *)declaration.dtype)->base.flags & CLASS_COMPLETED) != 0) {
                            if ((declarationFlags & 2) == 0) {
                                instance = (TypeClassExt800 *)((TypeClassTemplate *)templateClass)->instances;
                                while (instance) {
                                    if (instance == (TypeClassExt800 *)declaration.dtype) {
                                        if (instantiate)
                                            instantiate_members((Type *)templateClass, instance, 1);
                                        else
                                            instance->memberInstantiationState[0] = 1;
                                        break;
                                    }
                                    instance = instance->next;
                                }
                                if (instance == NULL)
                                    CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
                            }
                        } else
                            CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, declaration.dtype, 0);
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
    if (declaration.dtype->type != TYPEFUNC) {
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
        return;
    }
    if (tk != ';')
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
    for (; objects != NULL; objects = objects->next) {
        Object *object;
        TemplateSpecializationData *result;
        TemplateFunction *nameSpace;

        if ((object = (Object *)objects->object)->otype != OT_OBJECT || object->type->type != TYPEFUNC ||
            (((TypeFunc *)object->type)->flags & 0x400) == 0)
            continue;
        nameSpace = CTemplTool_GetFuncTempl(object);
        result = CTemplateFunc_FindOrCreateMatchedSpecialization(object, declaration.dtype, arguments, NULL);
        if (result == NULL)
            continue;
        if (!instantiate)
            result->suppressImplicitInstantiation = 1;
        else
            CTemplateNew_InstantiateFunction(nameSpace, result, 1);
        break;
    }
    if (objects == NULL)
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_TEMPLATE_INSTANTIATION);
}

#pragma opt_propagation reset

void parse_function_template_declaration(TemplateScopeState *stack, TemplateParameterRecord *params, TypeClass *tclass,
                                         SInt32 *startOffset)
{
    Boolean isstatic;
    DeclInfo di;
    DeclInfo destructorDecl;
    PFile *file;
    SInt32 offset;
    Boolean save;
    TypeClassTemplate *templclass;
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
    if (di.storage) {
        if (tclass) {
            if (di.storage == 0x102)
                isstatic = 1;
            else
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            di.storage = 0;
        } else if ((SInt32)di.storage != 0x102 && (SInt32)di.storage != 0x103) {
            CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            di.storage = 0;
        }
    }
    CError_ReportIllegalFlags(di.qual & ~(Q_CV | Q_ASM | Q_PASCAL | Q_INLINE | Q_EXPLICIT | Q_IMPLICIT_WEAK | Q_WEAK |
                                          Q_ALIGNED_MASK | Q_INTERRUPT));

    if (!di.hasTypename) {
        if (tclass && di.dtype->type == TYPECLASS && di.dtype == (Type *)tclass && tk == '(') {
            CError_ASSERT(1027, currentNameSpace->parent == tclass->nspace);
            CError_ReportIllegalFlags(di.qual & ~(Q_INLINE | Q_EXPLICIT));
            di.dtype = (Type *)&void_ptr;
            di.isConstructor = 1;
            CDecl_ParseDirectFuncDecl(&di);
            if (di.dtype->type == TYPEFUNC) {
                FuncArg *args;
                if ((args = TYPE_FUNC(di.dtype)->args) && !args->next && args->type == (Type *)tclass) {
                    CError_ReportError(ERR_ILLEGAL_COPY_CONSTRUCTOR);
                    TYPE_FUNC(di.dtype)->args = NULL;
                }
                if (tclass->flags & CLASS_HAS_VBASES)
                    CDecl_PrependFuncArg(TYPE_FUNC(di.dtype), &stsignedshort);
                TYPE_FUNC(di.dtype)->flags |= FUNC_IS_DTOR;
                di.name = constructor_name;
                currentNameSpace = stack->scope->parent;
                goto declare;
            }
            CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
        }

        if (di.dtype->type == TYPETEMPLATE) {
            if (tk == '(' && ((TypeTemplDep *)di.dtype)->kind == 1 &&
                (templclass = CTemplTool_IsTemplate(((TypeTemplDep *)di.dtype)->u.qual.type)) &&
                ((TypeTemplDep *)di.dtype)->u.qual.name == templclass->base.classname) {
                if (tclass)
                    CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
                di.dtype = (Type *)&void_ptr;
                di.isConstructor = 1;
                CTempl_PushScope(stack, &templclass->base);
                CDecl_ParseDirectFuncDecl(&di);
                if (di.dtype->type == TYPEFUNC) {
                    di.name = constructor_name;
                    if (templclass->base.flags & CLASS_HAS_VBASES)
                        CDecl_PrependFuncArg(TYPE_FUNC(di.dtype), &stsignedshort);
                    TYPE_FUNC(di.dtype)->flags |= FUNC_IS_DTOR;
                    parse_template_member_definition(params, &templclass->base, &di, startOffset);
                    return;
                }
                CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                return;
            }

            if (tk == TK_COLON_COLON && (dependentType = (TypeTemplDep *)di.dtype)->kind == 2 &&
                (templclass = CTemplTool_IsTemplate(dependentType))) {
                if (tclass)
                    CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '~') {
                    tk = CPrepTokenizer_GetNextToken();
                    if (!(tk == TK_IDENTIFIER && data_00587fa0 == templclass->base.classname &&
                          (tk = CPrepTokenizer_GetNextToken()) == '(')) {
                        if (tk == '<') {
                            CPrep_UngetToken();
                            tk = TK_IDENTIFIER;
                            data_00587fa0 = templclass->base.classname;
                            memclrw(&destructorDecl, sizeof(destructorDecl));
                            CParser_GetDeclSpecs((DeclInfo *)&destructorDecl, 0);
                            if (tk != '(')
                                CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                            if (destructorDecl.dtype != (Type *)templclass &&
                                !((destructorType = (TypeTemplDep *)destructorDecl.dtype)->type == TYPETEMPLATE &&
                                  CTemplateTools_GetTemplClass(destructorType) == (TypeClass *)templclass))
                                CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                        } else {
                            CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                        }
                    }
                    di.dtype = (Type *)&void_ptr;
                    CTempl_PushScope(stack, &templclass->base);
                    CDecl_ParseDirectFuncDecl(&di);
                    if (di.dtype->type == TYPEFUNC) {
                        if (templclass->base.sominfo)
                            di.qual |= Q_VIRTUAL;
                        else
                            CDecl_PrependFuncArg(TYPE_FUNC(di.dtype), &stsignedshort);
                        di.name = destructor_name;
                        TYPE_FUNC(di.dtype)->flags |= 0x4000;
                        parse_template_member_definition(params, &templclass->base, &di, startOffset);
                        return;
                    }
                    CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                    return;
                }
                if (tk == TK_OPERATOR) {
                    CTempl_PushScope(stack, &templclass->base);
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
                    conversionType = di.dtype;
                    conversionQual = di.qual;
                    CDecl_NewConvFuncType(&di);
                    if (CTemplateTools_IsDependentType(conversionType) &&
                        !(di.name = CTempl_FindConversion(&templclass->base, conversionType, conversionQual))) {
                        CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, "conversion function");
                        return;
                    }
                    parse_template_member_definition(params, &templclass->base, &di, startOffset);
                    return;
                }
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }

            if (((TypeTemplDep *)di.dtype)->kind == 1 && !di.isType)
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

    if (!di.name || di.dtype->type != TYPEFUNC || (di.nspace && di.nspace->theclass)) {
        CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
        return;
    }

    for (list = CScope_FindName(currentNameSpace, di.name); list; list = list->next) {
        if ((obj = OBJECT(list->object))->otype == OT_OBJECT && obj->type->type == TYPEFUNC &&
            (TYPE_FUNC(obj->type)->flags & 0x400)) {
            templ = CTemplTool_GetFuncTempl(obj);
            if (CTemplTool_EqualParams(templ->params, params, 0) && iscpp_typeequal(obj->type, di.dtype)) {
                if (tk != ';' && templ->stream.count)
                    CError_ReportError(ERR_TEMPLATE_REDEFINED);
                if (tk == '{' || tk == ':' || tk == TK_TRY)
                    CError_ASSERT(1227, CTemplTool_EqualParams(templ->params, params, 1));
                TYPE_FUNC(obj->type)->args = TYPE_FUNC(di.dtype)->args;
                break;
            }
        }
    }

    if (!list) {
    declare:
        if (tclass) {
            CError_ASSERT(1240, currentNameSpace->theclass);
            member = CDecl_NewTypeMemberFunc(TYPE_FUNC(di.dtype), currentNameSpace->theclass, isstatic, 1);
            di.dtype = (Type *)member;
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
    struct KeyedEntry *instance;
    char saved_state;
    TemplateFunction *function_info;
    PrepTokenBuffer parsed_body;
    Boolean option;
    PFile *start;
    SInt32 end;

    declaration->dtype = CTemplTool_ResolveMemberSelfRefs(template_info, declaration->dtype, &declaration->qual);
    if (declaration->dtype->type == TYPEFUNC) {
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
        object = entry->object.value;
        if (object->otype != OT_OBJECT) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED, declaration->name->name);
            return;
        }
        if (iscpp_typeequal(declaration->dtype, object->type) == 0 ||
            (object->qual & (Q_CV | Q_PASCAL)) != (declaration->qual & (Q_CV | Q_PASCAL))) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(object),
                               object->type, object->qual, declaration->dtype, declaration->qual);
            return;
        }
        CE_ASSERT(object->datatype != DDATA, CError_FATAL(869));
        CPrep_BufferTokensThroughSemicolon(&parsed_body, NULL);
    }
    if (parsed_body.count != 0) {
        is_saved_function = object->type->type == TYPEFUNC && (((TypeFunc *)object->type)->flags & 1024) != 0;
        if (is_saved_function) {
            if (CTemplTool_EqualParams(object->u.templateFunction->params, context, 0) == 0) {
                CError_ReportError(ERR_TEMPLATE_PARAMETER_MISMATCH);
            }
            function_info = object->u.templateFunction;
            function_info->stream = parsed_body;
        } else {
            instance = CTemplateClass_AddTemplateArgumentOverride((TypeClassTemplate *)template_info, object,
                                                                  &function_fileinfo, &parsed_body);
            if (((TypeClassTemplate *)template_info)->templateParameters != NULL) {
                if (CTemplTool_EqualParams(((TypeClassTemplate *)template_info)->templateParameters, context, 0) == 0) {
                    CError_ReportError(ERR_TEMPLATE_PARAMETER_MISMATCH);
                } else {
                    instance->templateParameters = context;
                }
            }
        }
    }
}

UInt8 CTemplateNew_LinkTemplateScope(DeclInfo *context, TypeTemplDep *request, NameSpace **destination)
{
    TemplateScopeState *entry;
    TypeClass *slot;
    TypeClassTemplate *result;

    if (request->kind == 1) {
        result = CTemplTool_IsTemplate(request->u.qual.type);
        if (result != NULL) {
            *destination = result->base.nspace;
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

Type *CTemplTool_GetSelfRefTemplate(TypeClassTemplate *record)
{
    CTStateElem *key;
    Type *result;
    Type *underlying;
    TypeClassTemplate *selected;
    CTStateElem *lookupContext;
    underlying = (selected = record)->replacementTemplate;
    if (underlying != NULL) {
        selected = (TypeClassTemplate *)underlying;
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

CTStateElem *parse_template_arguments(TypeClassTemplate **classType, CTStateElem **result)
{
    CTStateElem *argument;
    TemplateParameterRecord *parameter;
    CTStateElem **tail;
    Type *substituted;
    short token;
    TemplateParameterRecord *parameters;
    CTStateElem *arguments;
    DeclInfo parse;
    unsigned int substitutionInfo;
    TemplateContext context;
    TemplateContext defaultContext;
    parameters = (TemplateParameterRecord *)(*classType)->templateParameters;
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
        argument = (CTStateElem *)galloc(sizeof(*argument));
        memclrw(argument, sizeof(*argument));
        *tail = argument;
        tail = &argument->next;
        argument->pid = parameter->pid;
        if (tk != '>') {
            if (argument->pid.type != 0) {
                memclrw(&parse, sizeof(parse));
                CParser_GetDeclSpecs(&parse, 0);
                if (parse.storage != 0) {
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
                CTemplTool_CheckTemplArgType(parse.dtype);
                argument->argument.type = parse.dtype;
                argument->qualifiers = parse.qual;
            } else if (CTemplateTools_IsDependentType(parameter->value) != 0) {
                substituted = CTemplateTools_GetArgumentType(arguments, (TypeTemplDep *)parameter->value,
                                                             parameter->flags, &substitutionInfo);
                argument->argument.expression = parse_non_type_template_argument(substituted, substitutionInfo);
            } else {
                argument->argument.expression = parse_non_type_template_argument(parameter->value, parameter->flags);
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
            if (parameter->value == NULL) {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
                return NULL;
            }
            argument->argument.type = parameter->value;
            argument->qualifiers = parameter->flags;
            if (parameter->defaultValue.isTypeDependent != 0) {
                memclrw(&context, sizeof(context));
                context.templateClass = (TypeClass *)*classType;
                context.templateArgs = parameters;
                context.instance = NULL;
                context.instanceArgs = arguments;
                argument->qualifiers = parameter->flags;
                argument->argument.type =
                    CTemplateTools_ResolveType(&context, argument->argument.type, (UInt32 *)&argument->qualifiers);
            }
        } else {
            if (parameter->defaultValue.expression == NULL) {
                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENTS);
                return NULL;
            }
            if (parameter->defaultValue.expression->rtype->type == TYPETEMPLDEPEXPR) {
                memclrw(&defaultContext, sizeof(defaultContext));
                defaultContext.templateClass = (TypeClass *)*classType;
                defaultContext.templateArgs = parameters;
                defaultContext.instance = NULL;
                defaultContext.instanceArgs = arguments;
                argument->argument.expression =
                    fn_00513040(CTemplTool_DeduceExpr(&defaultContext, parameter->defaultValue.expression), 1);
            } else {
                argument->argument.expression = parameter->defaultValue.expression;
            }
        }
    }
    if (tk != '>') {
        CError_ReportError(ERR_GREATER_EXPECTED);
        return NULL;
    }
    if ((*classType)->specializations != NULL) {
        return arguments;
    }
    return arguments;
}

CTStateElem *CTemplateNew_ParseTemplateArguments(TemplateParameterRecord *parameter, char useGlobalAllocator)
{
    CTStateElem *head;
    CTStateElem *tail;
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
            if (declaration.storage != 0)
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            CError_ReportIllegalFlags(declaration.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
            CDecl_ParseDeclarator(&declaration);
            if (declaration.name != NULL)
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            CTemplTool_CheckTemplArgType(declaration.dtype);
            tail->argument.type = declaration.dtype;
            tail->qualifiers = declaration.qual;
        } else {
            tail->argument.expression = parse_non_type_template_argument(NULL, 0);
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

TemplateParameterRecord *parse_template_parameter_list(NameSpace *parserState, unsigned char mode)
{
    TemplateParameterRecord **tail;
    TemplateParameterRecord *argument;
    TemplateParameterRecord *previous;
    short index;
    TemplateParameterRecord *arguments;

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

TemplateParameterRecord *parse_template_parameter(NameSpace *owner, TemplateParameterRecord *value, short memberValue,
                                                  char memberByte)
{
    TemplateParameterRecord *declaration;
    DeclInfo parsed;
    DeclInfo initializerState;

    declaration = (TemplateParameterRecord *)galloc(sizeof(TemplateParameterRecord));
    memclrw(declaration, sizeof(*declaration));
    declaration->index = memberValue;
    switch (declaration->depth = memberByte, tk) {
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
                if (initializerState.storage != 0) {
                    CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                }
                CError_ReportIllegalFlags(initializerState.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
                CDecl_ParseDeclarator(&initializerState);
                declaration->value = initializerState.dtype;
                declaration->flags = initializerState.qual,
                declaration->defaultValue.isTypeDependent = CTemplateTools_IsDependentType(initializerState.dtype);
            }
            declaration->isTypeParameter = 1;
            break;
        case 0x14c:
            CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
            return NULL;
        default:
            memclrw(&parsed, sizeof(parsed));
            CParser_GetDeclSpecs((DeclInfo *)&parsed, '\0');
            if (parsed.storage != 0) {
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            }
            CError_ReportIllegalFlags(parsed.qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));
            CDecl_ParseDeclarator(&parsed);
            switch (*(char *)parsed.dtype) {
                case '\x01':
                case '\x03':
                case '\t':
                case '\v':
                case '\f':
                    break;
                case '\n':
                    CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
                    parsed.dtype = (Type *)(&stsignedint);
                    break;
                default:
                    CError_ReportError(ERR_ILLEGAL_TEMPLATE_DECLARATION);
                    parsed.dtype = (Type *)(&stsignedint);
                    break;
            }
            declaration->name = parsed.name, declaration->value = parsed.dtype, declaration->flags = parsed.qual;
            if (tk == '=') {
                tk = CPrepTokenizer_GetNextToken();
                declaration->defaultValue.expression = parse_non_type_template_argument(parsed.dtype, parsed.qual);
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
