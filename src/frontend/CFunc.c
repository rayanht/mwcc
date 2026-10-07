#define CERROR_FILE "CFunc.c"
#include "compiler/common.h"
#include "compiler/CFunc.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/FuncLevelAsmPPC.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>
#include <stdio.h>

#include "compiler/Types.h"

#pragma options align = mac68k
static void *PTR_00580870;
static struct SavedGlobalValues *saved_global_values_tail;
static UInt16 data_00580878;
static struct Object *localstatic_init_guard;
static struct CLabel *data_0058087e;
static unsigned char data_00580882;
static struct ENode *deferred_expression;
static struct FuncArg *default_arg;
static UInt8 data_0058088c;
static SInt16 local_name_counter;
static struct CleanNode *data_00580890;
#pragma options align = reset

/* Declarations gathered from the merged files. */
enum FlagVal { FLAGVAL_FALSE, FLAGVAL_TRUE };

typedef enum { CFUNC_UNUSED_A, CFUNC_UNUSED_B } CFuncUnused;

/* Return type and auxiliary state used while parsing a function body. */

/* Function label resolution records. */

static void SetLong(CInt64 *pN, long n)
{
    pN->lo = n;
    pN->hi = (n < 0) ? 0xFFFFFFFF : 0;
}

void CFunc_GenerateSingleExprFunc(Object *func, ENode *expr)
{
    Statement stmt;
    Statement *node;
    NameSpace *savedNamespace;
    UInt8 oldflag;

    if (cprep_cu[0xe0] == 1) {
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);
        return;
    }
    if (!anyerrors) {
        savedNamespace = CFunc_FuncGenSetup(&stmt, func);
        oldflag = copts.filesyminfo;
        copts.filesyminfo = 0;
        node = (Statement *)CompilerTools_AllocatePool(0x1a);
        node->next = NULL;
        node->type = ST_EXPRESSION;
        node->value = current_statement_number;
        node->flags = 0;
        node->sourceoffset = statement_sourceoffset;
        node->dobjstack = UINT_00587fc4;
        PTR_00587644->next = node;
        PTR_00587644 = node;
        node->expr.expression = expr;
        if (data_00588040 != NULL && data_00588040->sominfo != NULL)
            CSOM_GenerateSomselfAssignment(data_00588040, &stmt);
        CFunc_WarnUnused();
        CExcept_ExceptionTansform(&stmt);
        CInline_0050ee60(&stmt, func, 0);
        currentNameSpace = savedNamespace->parent;
        copts.filesyminfo = oldflag;
    }
}

void CFunc_GenerateDummyFunction(Object *functionObject)
{
    Boolean restoreInlineState;
    unsigned char savedState;
    NameSpace *savedValue;
    Statement statements;
    CInlineInfo inlineState;

    if (anyerrors != '\0') {
        return;
    }
    savedValue = CFunc_FuncGenSetup(&statements, NULL);
    savedState = copts.filesyminfo;
    copts.filesyminfo = 0;
    if (data_00588040 != NULL && data_00588040->sominfo != NULL) {
        CSOM_GenerateSomselfAssignment(data_00588040, &statements);
    }
    CFunc_WarnUnused();
    CExcept_ExceptionTansform(&statements);
    if ((TYPE_FUNC(functionObject->type)->flags & 0x20000000) != 0 && anyerrors == '\0') {
        CInline_SaveInfo(&inlineState, &statements, functionObject);
        restoreInlineState = TRUE;
    } else {
        restoreInlineState = FALSE;
    }
    CInline_0050ee60(&statements, functionObject, FALSE);
    if (restoreInlineState) {
        CClass_DefineCovariantFuncs(functionObject, &inlineState);
    }
    currentNameSpace = savedValue->parent;
    copts.filesyminfo = savedState;
}

void InitExpr_Register(ENode *expr, Object *cls)
{
    PendingFunction *pending;
    PendingFunction *last;
    Object *func;
    if (cprep_cu[0xe0] == 1 && cls->sclass != TK_STATIC && (cls->qual & (Q_IMPLICIT_WEAK | Q_WEAK)) == 0) {
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);
        return;
    }
    if (copts.f8e != 0)
        return;
    pending = galloc(sizeof(PendingFunction));
    pending->next = NULL;
    pending->cls = cls;
    func = (Object *)fn_00513040(expr, 1);
    pending->func = func;
    if (pending_functions != NULL) {
        last = pending_functions;
        while (last->next != NULL)
            last = last->next;
        last->next = pending;
    } else {
        pending_functions = pending;
    }
}

void CFunc_ParseFuncDef(Object *func, DeclInfo *definition, TypeClass *scopeObject, Boolean isMember,
                        unsigned char scopeFlag, NameSpace *scope)
{
    Boolean isSpecialMember;
    Boolean functionTryBlock;
    Statement *previousStatement;
    CScopeSave save;
    Statement state;
    StatementContext gen;
    CInlineInfo savedState;
    ObjectList *argument;
    Statement *returnStatement;
    CLabel *label;
    Boolean hasSavedState;
    Boolean savedStateCopy;
    Boolean oldStyleArguments;

    if (func->type->type != TYPEFUNC)
        (void)CError_FATAL(3402);
    if (TYPE_FUNC(func->type)->flags & FUNC_AUTO_GENERATED)
        CError_ReportError(ERR_OBJECT_REDEFINED, func);
    if ((TYPE_FUNC(func->type)->flags & FUNC_DEFINED) && func->datatype != DINLINEFUNC)
        CError_ReportError(ERR_OBJECT_REDEFINED, func);
    TYPE_FUNC(func->type)->flags |= FUNC_DEFINED;
    CParser_UpdateObject(func, definition);
    if (!isMember) {
        CScope_SetFunctionScope(func, &save);
        if (scopeObject != NULL)
            currentNameSpace = scopeObject->nspace;
    } else {
        CScope_SetMethodScope(func, scopeObject, scopeFlag, &save);
    }
    if (scope != NULL)
        currentNameSpace = scope;
    if (data_00588040 != NULL)
        CClass_MemberDef(func, data_00588040);
    func_errors = 0;
    data_0058088c = definition->requireMangledName;
    CError_ASSERT(3421, func->type->type == TYPEFUNC);
    if (definition->oldStyleParameters && (func->qual & Q_ASM))
        CError_ReportError(ERR_ILLEGAL_TYPE_QUALIFIERS);
    if (cprep_cu[0xe0] == 1 && !(func->qual & Q_INLINE))
        CError_FatalError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);
    if (definition->isType)
        CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
    CFunc_FuncGenSetup(&state, func);
    if (TYPE_FUNC(func->type)->functype->type != TYPEVOID)
        CanAllocObject(TYPE_FUNC(func->type)->functype);
    setup_function_arguments(func, definition, &state);
    state.sourceoffset = function_tokenoffset = statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
    if (definition->oldStyleParameters) {
        argument = arguments;
        while (argument != NULL) {
            if (argument->object.value->type->type == TYPEFLOAT && argument->object.value->type->size < stdouble.size)
                create_local_object_copy(argument->object.value, &stdouble, argument->object.value->type, 0);
            if (CMach_PassResultInHiddenArg(argument->object.value->type)) {
                argument->object.value->type = CDecl_NewPointerType(argument->object.value->type);
                TYPE_POINTER(argument->object.value->type)->qual = Q_REFERENCE;
            }
            argument = argument->next;
        }
    }
    if (definition->parameterScope != NULL)
        CScope_MergeNameSpace(currentNameSpace, definition->parameterScope);
    if (tk == TK_TRY) {
        tk = (UInt16)CPrepTokenizer_GetNextToken();
        functionTryBlock = 1;
    } else {
        functionTryBlock = 0;
    }
    if (CClass_IsDestructor(func)) {
        if (data_00588040 == NULL)
            CError_FATAL(3466);
        parse_ctor_initializers();
        CFunc_00476e70(data_00588040, ctor_initializers);
    }
    CPrep_ResetBufferedTokenPosition();
    if (!(func->qual & Q_ASM)) {
        if (tk == '{') {
            if (!functionTryBlock)
                tk = (UInt16)CPrepTokenizer_GetNextToken();
        } else {
            CError_ReportErrorAndUpdateToken(ERR_LBRACE_EXPECTED);
            functionTryBlock = 0;
        }
        if (!copts.cplusplus)
            parse_declarations(0, 0, 0, 0);
        gen.switchInfo = NULL;
        gen.continueLabel = NULL;
        gen.breakLabel = NULL;
        gen.returnType = TYPE_FUNC(func->type)->functype;
        gen.returnQual = TYPE_FUNC(func->type)->qual;
        if (functionTryBlock) {
            isSpecialMember = CClass_IsDestructor(func) || CClass_HasTypeFuncFlag16384(func);
            CExcept_ScanTryBlock(&gen, isSpecialMember);
            if (tk != 0)
                CPrep_UngetToken();
            tk = '}';
        } else {
            while (tk != '}') {
                parse_statement(&gen);
            }
        }
        if (PTR_00587644->type != ST_RETURN && PTR_00587644->type != ST_GOTO) {
            previousStatement = PTR_00587644;
            statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
            returnStatement = (Statement *)CompilerTools_AllocatePool(sizeof(*returnStatement));
            returnStatement->next = NULL;
            returnStatement->type = ST_RETURN;
            returnStatement->value = current_statement_number;
            returnStatement->flags = 0;
            returnStatement->sourceoffset = statement_sourceoffset;
            returnStatement->dobjstack = UINT_00587fc4;
            PTR_00587644->next = returnStatement;
            PTR_00587644 = returnStatement;
            returnStatement->dobjstack = NULL;
            PTR_00587644->expr.expression = NULL;
            if (copts.cplusplus || copts.f90) {
                if (memcmp(func->name->name, "main", 5) == 0 &&
                    &TYPE_FUNC(func->type)->functype->type == &stsignedint.type)
                    PTR_00587644->expr.expression = intconstnode((Type *)&stsignedint, 0);
            }
            if (previousStatement->type == ST_EXPRESSION) {
                ENode *expression;
                if ((expression = previousStatement->expr.expression)->type == EFUNCCALL &&
                    expression->rtype == &stvoid && (expression->flags & 2))
                    PTR_00587644->flags |= 8;
            }
        }
        for (label = clabels; label != NULL; label = label->next) {
            if (label->target.stmt == NULL)
                CError_ReportError(ERR_UNDEFINED_LABEL, label->name->name);
        }
        if (!func_errors) {
            if (CClass_IsDestructor(func))
                CABI_InsertConstructorInitialization(func, &state, data_00588040, NULL, functionTryBlock);
            if (CClass_HasTypeFuncFlag16384(func))
                CABI_TransDestructor(func, func, &state, data_00588040, 0);
            CFunc_DestructorCleanup(&state);
            if (data_00588040 != NULL && data_00588040->sominfo != NULL)
                CSOM_GenerateSomselfAssignment(data_00588040, &state);
            CFunc_WarnUnused();
            CExcept_ExceptionTansform(&state);
            function_token_line = CPrep_UpdateTokenLine(&function_fileinfo);
            oldStyleArguments = definition->oldStyleParameters;
            if ((TYPE_FUNC(func->type)->flags & 0x20000000) && !anyerrors) {
                CInline_SaveInfo(&savedState, &state, func);
                hasSavedState = savedStateCopy = FLAGVAL_TRUE;
            } else {
                hasSavedState = FLAGVAL_FALSE;
            }
            CInline_0050ee60(&state, func, oldStyleArguments);
            if (hasSavedState) {
                CClass_DefineCovariantFuncs(func, &savedState);
            }
        }
    } else {
        if (tk == '{') {
            data_005884fd = 1;
            tk = (UInt16)CPrepTokenizer_GetNextToken();
            data_005884fd = 0;
        } else {
            CError_ReportErrorAndUpdateToken(ERR_LBRACE_EXPECTED);
        }
        parse_declarations(1, 0, 0, 0);
        FuncLevelAsmPPC_GenerateFunction(func);
    }
    if (tk != '}')
        CError_ReportError(ERR_RBRACE_EXPECTED);
    CScope_RestoreScope(&save);
}

void parse_ctor_initializers(void)
{
    Type *type;
    ClassList *base;
    TypeClass *cls;
    VClassList *vbase;
    ENodeList *args;
    CtorInit *entry;
    CtorInit *previous;
    ENode *expr;
    ObjMemberVar *member;

    ctor_initializers = NULL;
    if (tk != ':')
        return;
    do {
        cls = NULL;
        tk = CPrepTokenizer_GetNextToken();
        switch (tk) {
            case TK_IDENTIFIER:
                for (base = data_00588040->bases; base != NULL; base = base->next)
                    if (base->base->classname == data_00587fa0) {
                        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x28) {
                            cls = base->base;
                            tk = CPrepTokenizer_GetNextToken();
                        } else
                            data_00587fa0 = base->base->classname;
                        break;
                    }
                for (member = data_00588040->ivars; member != NULL; member = member->next)
                    if (member->name == data_00587fa0) {
                        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x28)
                            goto member_found;
                    }
                break;
            case TK_COLON_COLON:
                break;
            default:
                CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                return;
        }
        if (cls == NULL)
            cls = CClass_GetQualifiedClass();
        if (cls != NULL) {
            for (vbase = data_00588040->vbases; vbase != NULL; vbase = vbase->next)
                if (vbase->base == cls)
                    break;
            if (vbase != NULL) {
                for (previous = ctor_initializers; previous != NULL; previous = previous->next)
                    if (previous->kind == 1 && previous->u.virtualBase == vbase) {
                        CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                        return;
                    }
                entry = CompilerTools_AllocatePool(sizeof(CtorInit));
                entry->kind = 1;
                entry->u.virtualBase = vbase;
            } else {
                for (base = data_00588040->bases; base != NULL; base = base->next)
                    if (base->base == cls)
                        break;
                if (base != NULL) {
                    for (previous = ctor_initializers; previous != NULL; previous = previous->next)
                        if (previous->kind == 0 && previous->u.base == base) {
                            CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                            return;
                        }
                    entry = CompilerTools_AllocatePool(sizeof(CtorInit));
                    entry->kind = 0;
                    entry->u.base = base;
                } else {
                    CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                    return;
                }
            }
        } else {
            for (member = data_00588040->ivars; member != NULL; member = member->next)
                if (member->name == data_00587fa0)
                    break;
            if (member != NULL) {
            member_found:
                for (previous = ctor_initializers; previous != NULL; previous = previous->next)
                    if (previous->kind == 2 && previous->u.member == member)
                        CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                entry = CompilerTools_AllocatePool(sizeof(CtorInit));
                entry->kind = 2;
                entry->u.member = member;
            } else {
                CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                return;
            }
            tk = CPrepTokenizer_GetNextToken();
        }
        if (tk != '(') {
            CError_ReportError(ERR_LPAREN_EXPECTED);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
        args = CExpr_ScanExpressionList(1);
        if (tk != ')') {
            CError_ReportError(ERR_RPAREN_EXPECTED);
            return;
        }
        switch (entry->kind) {
            case 0:
                expr = CABI_MakeThisExpr(NULL, entry->u.base->offset);
                entry->expr = CExpr_ConstructObject(TYPE(entry->u.base->base), expr, args, 1, 0, 0, 0, 1);
                break;
            case 1:
                expr = CABI_MakeThisExpr(entry->u.virtualBase->base, entry->u.virtualBase->offset);
                entry->expr = CExpr_ConstructObject(TYPE(entry->u.virtualBase->base), expr, args, 1, 0, 0, 0, 1);
                break;
            case 2:
                expr = CABI_MakeThisExpr(data_00588040, entry->u.member->offset);
                expr->flags = entry->u.member->qual & Q_CV;
                type = entry->u.member->type;
                switch ((SInt8)type->type) {
                    case TYPECLASS:
                        entry->expr = CExpr_ConstructObject(entry->u.member->type, expr, args, 1, 1, 0, 1, 1);
                        break;
                    case TYPEARRAY:
                        CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                        tk = CPrepTokenizer_GetNextToken();
                        continue;
                    default: {
                        TypeBitfield *bitfieldType;
                        ENode *lhs;
                        if (args != NULL) {
                            if (args->next != NULL) {
                                CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                                return;
                            }
                            lhs = makemonadicnode(expr, EINDIRECT);
                            lhs->rtype = entry->u.member->type;
                            if (lhs->rtype->type == TYPEBITFIELD) {
                                bitfieldType = (TypeBitfield *)lhs->rtype;
                                lhs->data.monadic = makemonadicnode(lhs->data.monadic, EBITFIELD);
                                lhs->data.monadic->rtype = TYPE(bitfieldType);
                                lhs->rtype = bitfieldType->bitfieldtype;
                            }
                            entry->expr = makediadicnode(
                                lhs, oldassignmentpromotion(args->node, lhs->rtype, lhs->flags, 1), EASS);
                        } else
                            entry->expr = nullnode();
                        break;
                    }
                }
                break;
            default:
                CError_FATAL(3316);
        }
        entry->next = ctor_initializers;
        ctor_initializers = entry;
        tk = CPrepTokenizer_GetNextToken();
    } while (tk == ',');
}
void fn_00476e60(TypeClass *type)
{
    CFunc_00476e70(type, NULL);
}

/* 0x4463d0, error report */

/* A linked list of member initializer records: next, a discriminator byte at
 * 0x04 (2) and the member variable pointer at 0x0a. */

void CFunc_00476e70(TypeClass *theclass, struct CtorInit *inits)
{
    ObjMemberVar *member;
    CtorInit *node;

    if (theclass->mode == 1)
        return;
    member = theclass->ivars;
    while (member != NULL) {
        if (member->type->type == TYPEPOINTER && (TYPE_POINTER(member->type)->qual & Q_REFERENCE)) {
            node = inits;
            while (node != NULL) {
                if (node->kind == 2 && node->u.member == member)
                    break;
                node = node->next;
            }
            if (node == NULL)
                CError_ReportError(ERR_AMPERSAND_REFERENCE_MEMBER_NOT_INITIALIZED, member->name->name);
        } else if (CParser_IsConst(member->type, member->qual)) {
            node = inits;
            while (node != NULL) {
                if (node->kind == 2 && node->u.member == member)
                    break;
                node = node->next;
            }
            if (node == NULL && member->type->type != TYPECLASS)
                CError_ReportError(ERR_CONST_MEMBER_NOT_INITIALIZED, member->name->name);
        }
        member = member->next;
    }
}

void CFunc_Gen(Statement *context, Object *object, unsigned int options)
{
    CInlineInfo buf;
    unsigned char flag;
    unsigned int flags;

    flags = TYPE_METHOD(object->type)->flags;

    if ((flags & 0x20000000U) != 0U && anyerrors == 0U) {
        CInline_SaveInfo(&buf, context, object);
        flag = 1;
    } else {
        flag = 0;
    }

    CInline_0050ee60(context, object, options);

    if (flag) {
        CClass_DefineCovariantFuncs(object, &buf);
    }
}

/* Statement view exposing the 16-bit field at 0x08. */

/* Link in the active function-generation scope stack. */

NameSpace *CFunc_FuncGenSetup(Statement *stmt, Object *func)
{
    NameSpace *scope;
    struct SavedGlobalValues *node;

    scope = CScope_NewListNameSpace(NULL, 0);
    scope->parent = currentNameSpace;
    currentNameSpace = scope;
    arguments = NULL;
    locals = NULL;
    clabels = NULL;
    next_varnumber = 0;
    local_name_counter = 0;
    CExcept_Setup();
    memclrw(stmt, sizeof(*stmt));
    PTR_00587644 = stmt;
    stmt->type = ST_NOP;
    current_statement_number = 1;
    stmt->value = *(UInt16 *)&current_statement_number;
    data_00580878 = 0;
    node = CompilerTools_AllocatePool(offsetof(struct SavedGlobalValues, index) + sizeof(node->index));
    memclrw(node, offsetof(struct SavedGlobalValues, index) + sizeof(node->index));
    node->index = data_00580878++;
    node->savedNameSpace = currentNameSpace;
    saved_global_values_tail = PTR_00580870 = node;
    return scope;
}

void setup_function_arguments(Object *function, DeclInfo *body, Statement *state)
{
    Type *declarationType;
    ObjectList *candidate;
    HashNameNode *name;
    Object *result;
    ObjectList *argument;
    Object *parameter;
    Object *matched;
    ObjectList *entry;
    ObjectList *newEntry;
    DeclInfo declaration;
    if (((TypeFunc *)function->type)->args != NULL) {
        if (body->oldStyleParameters != 0) {
            arguments = create_arg_object_list(body->parameterNames);
            for (;;) {
                if (tk == '{') {
                    break;
                }
                memclrw(&declaration, sizeof(declaration));
                CParser_GetDeclSpecs((DeclInfo *)&declaration, 0);
                declarationType = declaration.dtype;
                if ((short)declaration.storage != 0 && (short)declaration.storage != 257) {
                    CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
                }
                for (;;) {
                    declaration.dtype = declarationType;
                    declaration.name = NULL;
                    CDecl_ParseDeclarator(&declaration);
                    if (declaration.name == NULL) {
                        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                        break;
                    }
                    switch ((signed char)declaration.dtype->type) {
                        case TYPECLASS:
                            if (((TypeClass *)declaration.dtype)->sominfo != NULL) {
                                CError_ReportError(ERR_FUNCTIONS_CANNOT_SOM_CLASS_ARGUMENTS);
                                declaration.dtype = (Type *)&stsignedint;
                            }
                            if (CDecl_CheckObjectType(declaration.dtype) == 0) {
                                declaration.dtype = (Type *)&stsignedint;
                            }
                            break;
                        case TYPEFUNC:
                            CDecl_WrapTypePointer(&declaration.dtype, 0);
                            break;
                        case TYPEARRAY:
                            declaration.dtype = CDecl_NewPointerType(((TypePointer *)declaration.dtype)->target);
                            break;
                        default:
                            if (CDecl_CheckObjectType(declaration.dtype) == 0) {
                                declaration.dtype = (Type *)&stsignedint;
                            }
                            break;
                    }
                    CanAllocObject(declaration.dtype);
                    candidate = arguments;
                    name = declaration.name;
                    while (candidate != NULL) {
                        if (name == (parameter = candidate->object.value)->name) {
                            goto parameterFound;
                        }
                        candidate = candidate->next;
                    }
                    parameter = NULL;
                parameterFound:
                    if ((matched = parameter) != NULL) {
                        if (parameter->type != NULL) {
                            CError_ReportError(ERR_OBJECT_REDEFINED, parameter);
                        }
                        parameter->type = declaration.dtype;
                        parameter->sclass = declaration.storage;
                        parameter->qual = declaration.qual;
                    } else {
                        CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
                    }
                    if (tk != ',') {
                        break;
                    }
                    tk = CPrepTokenizer_GetNextToken();
                }
                if (tk != ';') {
                    CError_ReportErrorAndUpdateToken(ERR_SEMICOLON_EXPECTED);
                    continue;
                }
                tk = CPrepTokenizer_GetNextToken();
            }
            argument = arguments;
            while (argument != NULL) {
                if (argument->object.value->type == NULL) {
                    argument->object.value->type = (Type *)&stsignedint;
                }
                argument = argument->next;
            }
        } else {
            CFunc_SetupNewFuncArgs(function, ((TypeFunc *)function->type)->args);
        }
    }
    if (CMachine_FunctionRequiresMemoryReturn((TypeFunc *)function->type) == 1) {
        result = CParser_NewLocalDataObject(NULL, 0);
        result->name = blank_argument_name;
        result->type = (Type *)CDecl_NewPointerType(((TypeFunc *)function->type)->functype);
        result->u.var.info = CPrep_AllocateVarInfo();
        result->u.var.info->func = data_00588238;
        if (result->sclass == TK_REGISTER) {
            if (copts.uniformSpillBlockWeight == 0) {
                result->u.var.info->usage = 100;
            } else {
                result->u.var.info->usage = 5;
            }
        }
        if (result->type != NULL && is_volatile_object(result) != 0) {
            result->u.var.info->noregister = 1;
        }
        newEntry = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
        newEntry->object.value = result;
        if (CInline_ReturnZero(function->type) != 0) {
            if (arguments == NULL)
                CError_FATAL(2846);
            newEntry->next = (arguments)->next;
            (arguments)->next = newEntry;
        } else {
            newEntry->next = arguments;
            arguments = newEntry;
        }
    }
    entry = arguments;
    while (entry != NULL) {
        CScope_InsertNameSpaceName(currentNameSpace, entry->object.value->name)->object =
            (ObjBase *)entry->object.value;
        entry = entry->next;
    }
}

/* Variable bookkeeping record hung off Object.u.var.info. */

/* Builds a list of parameter Objects from a FuncArg
 * chain; each Object also gets a VarInfo. */
ObjectList *create_arg_object_list(FuncArg *arg)
{
    ObjectList *res = NULL;
    ObjectList *cur;
    Object *obj;

    while (arg != NULL) {
        if (res != NULL) {
            cur = cur->next = (ObjectList *)CompilerTools_AllocatePool(8);
        } else {
            res = cur = (ObjectList *)CompilerTools_AllocatePool(8);
        }
        obj = CParser_NewLocalDataObject(NULL, 0);
        obj->name = arg->name;
        obj->type = arg->type;
        obj->qual = arg->qual;
        obj->sclass = arg->sclass;
        obj->u.var.info = CPrep_AllocateVarInfo();
        obj->u.var.info->func = data_00588238;
        if (obj->sclass == TK_REGISTER) {
            if (copts.uniformSpillBlockWeight == 0) {
                obj->u.var.info->usage = 100;
            } else {
                obj->u.var.info->usage = 5;
            }
        }
        if (obj->type != NULL && is_volatile_object(obj)) {
            obj->u.var.info->noregister = 1;
        }
        cur->object.value = obj;
        cur->next = NULL;
        arg = arg->next;
    }
    return res;
}

void CFunc_SetupNewFuncArgs(Object *func, FuncArg *args)
{
    Object *obj;
    ObjectList *arglist;

    arguments = NULL;
    if (args != &data_00583098 && args != &data_00584748) {
        for (arglist = NULL; args != NULL && args != &data_00583098; args = args->next) {
            CanAllocObject(args->type);
            obj = CParser_NewLocalDataObject(NULL, 0);
            if (args->name == NULL)
                obj->name = unnamed_name;
            else
                obj->name = args->name;
            obj->type = args->type;
            obj->qual = args->qual;
            obj->sclass = args->sclass;
            obj->u.var.info = CPrep_AllocateVarInfo();
            obj->u.var.info->func = data_00588238;
            if (obj->sclass == TK_REGISTER) {
                if (copts.uniformSpillBlockWeight == 0)
                    obj->u.var.info->usage = 100;
                else
                    obj->u.var.info->usage = 5;
            }
            if (obj->type != NULL && is_volatile_object(obj))
                obj->u.var.info->noregister = 1;
            if ((obj->type->type == TYPECLASS && CClass_ReferenceArgument(TYPE_CLASS(obj->type))) ||
                CMach_PassResultInHiddenArg(obj->type)) {
                obj->type = CDecl_NewPointerType(obj->type);
                TYPE_POINTER(obj->type)->qual = Q_REFERENCE;
            }
            if (obj->name == unnamed_name && copts.rejectZeroLengthArrayMembers != 0 && copts.cplusplus == 0 &&
                (func->qual & Q_MANGLE_NAME) == 0)
                CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
            if (arglist != NULL) {
                arglist->next = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
                arglist = arglist->next;
            } else {
                arglist = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
                arguments = arglist;
            }
            arglist->next = NULL;
            arglist->object.value = obj;
        }
    }
}

/* Per-variable bookkeeping returned by CPrep_AllocateVarInfo. */

static void CFunc_InitVariableInfo(Object *obj)
{
    obj->u.var.info = CPrep_AllocateVarInfo();
    obj->u.var.info->func = data_00588238;
}

static struct Statement *CFunc_NewAssignmentStatement(void)
{
    struct Statement *stmt;
    stmt = (struct Statement *)CompilerTools_AllocatePool(sizeof(struct Statement));
    stmt->next = NULL;
    stmt->type = ST_EXPRESSION;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = UINT_00587fc4;
    PTR_00587644->next = stmt;
    PTR_00587644 = stmt;
    return stmt;
}

void create_local_object_copy(Object *func, TypeIntegral *type, Type *type2, Boolean flag)
{
    Object *newfunc;
    NameSpaceObjectList *list;
    ObjectList *listnode;
    struct Statement *stmt;
    ENode *expr;

    newfunc = (Object *)CompilerTools_AllocatePool(sizeof(Object));
    *newfunc = *func;
    newfunc->type = type2;
    CFunc_InitVariableInfo(newfunc);
    if (newfunc->sclass == TK_REGISTER) {
        if (copts.uniformSpillBlockWeight == 0)
            newfunc->u.var.info->usage = 100;
        else
            newfunc->u.var.info->usage = 5;
    }
    if (newfunc->type != NULL && is_volatile_object(newfunc))
        newfunc->u.var.info->noregister = 1;
    func->name = CParser_GetUniqueName();
    func->type = flag ? CDecl_NewPointerType((Type *)type) : (Type *)type;
    list = CScope_FindName(currentNameSpace, newfunc->name);
    if (list == NULL || list->object != (ObjBase *)func)
        CError_FATAL(2577);
    list->object = (ObjBase *)newfunc;
    listnode = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
    listnode->object.value = newfunc;
    listnode->next = locals;
    locals = listnode;
    stmt = CFunc_NewAssignmentStatement();
    expr = create_objectnode(func);
    if (flag) {
        expr->rtype = CDecl_NewPointerType((Type *)type);
        expr = makemonadicnode(expr, EINDIRECT);
    }
    expr->rtype = (Type *)type;
    if ((Type *)type != type2)
        expr = CExpr2_00473720(expr, type2);
    stmt->expr.expression = makediadicnode(create_objectnode(newfunc), expr, EASS);
}

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

static void *NewScope(void)
{
    struct SavedGlobalValues *s = CompilerTools_AllocatePool(0xe);
    NameSpace *ns;
    if (PTR_00580870) {
        saved_global_values_tail->next = s;
        saved_global_values_tail = s;
    } else {
        saved_global_values_tail = PTR_00580870 = s;
    }
    s->index = data_00580878++;
    s->savedNameSpace = currentNameSpace;
    s->savedException = UINT_00587fc4;
    ns = CScope_NewListNameSpace(NULL, 0);
    ns->parent = (NameSpace *)currentNameSpace;
    currentNameSpace = ns;
    return s;
}

inline void RestoreBlock(void *block)
{
    currentNameSpace = ((struct SavedGlobalValues *)block)->savedNameSpace;
    UINT_00587fc4 = ((struct SavedGlobalValues *)block)->savedException;
}

void CFunc_ParseScopedStatement(struct StatementContext *context)
{
    struct ScopeRec *scope = NewScope();

    if (tk == '{') {
        tk = CPrepTokenizer_GetNextToken();
        if (copts.cplusplus == '\0' && isdeclaration(0, 0, 0, 0) != 0) {
            parse_declarations('\0', 0, '\0', '\0');
        }
        while (tk != '}') {
            parse_statement(context);
        }
        statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
        tk = CPrepTokenizer_GetNextToken();
    } else {
        parse_statement(context);
    }
    RestoreBlock(scope);
}

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

static CLabel *NewLabel(void)
{
    CLabel *label = (CLabel *)CompilerTools_AllocatePool(0x14);
    memclrw(label, 0x14);
    label->uniquename = CParser_GetUniqueName();
    label->name = label->uniquename;
    return label;
}

static unsigned char IsLabel(void)
{
    HashNameNode *id;
    short save;
    short t;
    id = data_00587fa0;
    save = token_value_kind_or_string_length;
    t = CPrepTokenizer_GetNextTokenAndRestorePosition();
    data_00587fa0 = id;
    token_value_kind_or_string_length = save;
    return t == 0x3a;
}

static CLabel *FindLabel(void)
{
    CLabel *label;
    for (label = clabels; label; label = label->next)
        if ((HashNameNode *)data_00587fa0 == label->name)
            return label;
    return NULL;
}

inline void ScopedBody(StatementContext *ctx)
{
    void *s = NewScope();
    if (tk == '{') {
        tk = (short)CPrepTokenizer_GetNextToken();
        if ((copts.cplusplus == '\0') && (isdeclaration(0, 0, 0, 0) != '\0')) {
            parse_declarations('\0', 0, '\0', '\0');
        }
        while (tk != '}') {
            parse_statement(ctx);
        }
        statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
        tk = (short)CPrepTokenizer_GetNextToken();
    } else {
        parse_statement(ctx);
    }
    RestoreBlock(s);
}

static Statement *AppendStmt(unsigned char kind)
{
    Statement *s = (Statement *)CompilerTools_AllocatePool(0x1a);
    s->next = NULL;
    s->type = kind;
    s->value = current_statement_number;
    s->flags = 0;
    s->sourceoffset = statement_sourceoffset;
    s->dobjstack = UINT_00587fc4;
    PTR_00587644->next = s;
    PTR_00587644 = s;
    return s;
}

static void CondJump(ENode *expr, CLabel *truelabel, char a, char b)
{
    CLabel *label = NewLabel();
    generate_conditional_jump(expr, truelabel, label, a, b);
    label->target.stmt = (Statement *)AppendStmt(2); /* AppendStmt statement view */
    ((Statement *)label->target.stmt)->target.label = label;
}

/* Context passed through statement parsing. */

static inline char use_legacy_condition_scope(void)
{
    return copts.f5f;
}

static inline char warn_missing_return_value(void)
{
    return copts.f9d;
}

static inline Boolean warn_empty_control_statement(void)
{
    return copts.fa1;
}

void parse_statement(StatementContext *context)
{
    SavedGlobalValues *ifScope;
    CLabel *endLabel;
    SavedGlobalValues *whileScope;
    HashNameNode *namespaceName;
    Statement *jumpStmt;
    SavedGlobalValues *forScope;
    CLabel *conditionLabel;
    ENode *expr;
    ENode *conditionExpr;
    SavedGlobalValues *bodyScope;
    SavedGlobalValues *newScope;
    Statement *stmt;
    SavedGlobalValues *switchScope;
    CLabel *topLabel;
    ENode *stepExpr;
    ENode *testExpr;
    StatementContext bodyContext;
    CLabel *breakLabel;
    CLabel *continueLabel;

    statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
    switch (tk) {
        case TK_RETURN:
            tk = CPrepTokenizer_GetNextToken();
            if (((context->returnType == &stvoid && !copts.cplusplus) || CClass_IsDestructor(data_00588238)) ||
                CClass_HasTypeFuncFlag16384(data_00588238)) {
                if (tk != ';') {
                    CError_ReportError(ERR_ILLEGAL_RETURN_VALUE_VOID_CONSTRUCTOR_DESTRUCTOR);
                    s_expression();
                }
                stmt = AppendStmt(8);
                stmt->expr.expression = NULL;
                fn_00449d60();
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            if (tk == ';') {
                if (context->returnType != &stvoid && (warn_missing_return_value() || copts.cplusplus))
                    CError_Warning(ERR_RETURN_VALUE_EXPECTED);
                stmt = AppendStmt(8);
                stmt->expr.expression = NULL;
                fn_00449d60();
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            expr = s_expression();
            if (context->returnType == &stvoid) {
                if (expr->rtype != &stvoid)
                    CError_ReportError(ERR_ILLEGAL_RETURN_VALUE_VOID_CONSTRUCTOR_DESTRUCTOR);
                stmt = AppendStmt(4);
                stmt->expr.expression = expr;
                stmt = AppendStmt(8);
                stmt->expr.expression = NULL;
            } else {
                if (CMachine_FunctionRequiresMemoryReturn((TypeFunc *)data_00588238->type) == 1)
                    expr = initialize_argument_object(expr, context->returnType, context->returnQual);
                else
                    expr = oldassignmentpromotion(expr, context->returnType, context->returnQual, 1);
                stmt = AppendStmt(8);
                stmt->expr.expression = expr;
                if (context->returnType->type == TYPEPOINTER)
                    check_function_result_automatic_variable(expr);
            }
            break;
        case TK_CASE:
            parse_case_statement(context);
            return;
        case TK_DEFAULT:
            if (!context->switchInfo) {
                CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
                return;
            }
            if (CPrepTokenizer_GetNextToken() != ':')
                CError_ReportErrorAndUpdateToken(ERR_COLON_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (context->switchInfo->defaultlabel)
                CError_ReportErrorAndUpdateToken(ERR_DEFAULT_LABEL_DEFINED_MORE_THAN_ONCE);
            stmt = AppendStmt(2);
            stmt->target.label = NewLabel();
            stmt->target.label->target.stmt = stmt;
            context->switchInfo->defaultlabel = stmt->target.label;
            parse_statement(context);
            return;
        case TK_SWITCH:
            if (CPrepTokenizer_GetNextToken() != '(')
                CError_ReportErrorAndUpdateToken(ERR_LPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (copts.cplusplus && !use_legacy_condition_scope() && isdeclaration(1, 0, 0, 0x3d)) {
                newScope = NewScope();
                switchScope = newScope;
                conditionExpr = parse_declarations(0, 1, 0, 0);
                if (CScope_IsEmptyNameSpace(currentNameSpace)) {
                    RestoreBlock(newScope);
                    switchScope = NULL;
                }
            } else {
                conditionExpr = s_expression();
                switchScope = NULL;
            }
            expr = CExpr2_ConvertScalarOperand(conditionExpr, 1, 0);
            if (expr->rtype->type != TYPEINT && expr->rtype->type != TYPEFLOAT && expr->rtype->type != TYPEPOINTER)
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            stmt = AppendStmt(5);
            stmt->expr.expression = forceintegral(expr);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            stmt->target.switchDescriptor = (SwitchInfo *)CompilerTools_AllocatePool(sizeof(SwitchInfo));
            stmt->target.switchDescriptor->defaultlabel = NULL;
            stmt->target.switchDescriptor->cases = NULL;
            stmt->target.switchDescriptor->sizetype = stmt->expr.expression->rtype;
            breakLabel = NewLabel();
            bodyContext = *context;
            bodyContext.switchInfo = stmt->target.switchDescriptor;
            bodyContext.breakLabel = breakLabel;
            ScopedBody(&bodyContext);
            if (!bodyContext.switchInfo->defaultlabel)
                bodyContext.switchInfo->defaultlabel = breakLabel;
            if (!bodyContext.switchInfo->cases) {
                stmt->type = ST_EXPRESSION;
                jumpStmt = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
                *jumpStmt = *stmt;
                stmt->next = jumpStmt;
                jumpStmt->type = ST_GOTO;
                jumpStmt->target.label = bodyContext.switchInfo->defaultlabel;
                jumpStmt->dobjstack = UINT_00587fc4;
            }
            stmt = AppendStmt(2);
            stmt->target.label = breakLabel;
            breakLabel->target.stmt = stmt;
            if (switchScope)
                RestoreBlock(switchScope);
            return;
        case 0x13f:
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER) {
                if (tk == '*' && !copts.rejectZeroLengthArrayMembers) {
                    tk = CPrepTokenizer_GetNextToken();
                    stmt = AppendStmt(0xf);
                    stmt->expr.expression = s_expression();
                    if (stmt->expr.expression->rtype->type != TYPEPOINTER) {
                        CError_ReportError(ERR_ILLEGAL_TYPE);
                        stmt->expr.expression = nullnode();
                        stmt->expr.expression->rtype = (Type *)&void_ptr;
                    }
                    break;
                }
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return;
            }
            stmt = AppendStmt(3);
            stmt->target.label = FindLabel();
            if (!stmt->target.label) {
                stmt->target.label = NewLabel();
                stmt->target.label->next = clabels;
                clabels = stmt->target.label;
                stmt->target.label->name = data_00587fa0;
            }
            tk = CPrepTokenizer_GetNextToken();
            break;
        case TK_BREAK:
            if (context->breakLabel) {
                stmt = AppendStmt(3);
                stmt->target.label = context->breakLabel;
            } else
                CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
            tk = CPrepTokenizer_GetNextToken();
            break;
        case TK_CONTINUE:
            if (context->continueLabel) {
                stmt = AppendStmt(3);
                stmt->target.label = context->continueLabel;
            } else
                CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
            tk = CPrepTokenizer_GetNextToken();
            break;
        case TK_FOR:
            if (CPrepTokenizer_GetNextToken() != '(')
                CError_ReportErrorAndUpdateToken(ERR_LPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            forScope = NULL;
            if (tk != ';') {
                if (!copts.cplusplus || !isdeclaration(1, 0, 0, 0)) {
                    expr = s_expression();
                    CExpr_CheckUnusedExpression(expr);
                } else {
                    if (!use_legacy_condition_scope())
                        forScope = NewScope();
                    expr = parse_declarations(0, 1, 1, 0);
                    if (forScope && CScope_IsEmptyNameSpace(currentNameSpace)) {
                        RestoreBlock(forScope);
                        forScope = NULL;
                    }
                }
                if (expr) {
                    stmt = AppendStmt(4);
                    stmt->expr.expression = expr;
                }
                if (tk != ';')
                    CError_ReportError(ERR_SEMICOLON_EXPECTED);
                else
                    fn_00449d60();
            } else
                fn_00449d60();
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ';') {
                if (copts.cplusplus && !use_legacy_condition_scope() && isdeclaration(1, 0, 0, 0x3d)) {
                    if (!forScope)
                        forScope = NewScope();
                    conditionExpr = parse_declarations(0, 1, 0, 0);
                    if (CScope_IsEmptyNameSpace(currentNameSpace)) {
                        RestoreBlock(forScope);
                        forScope = NULL;
                    }
                } else {
                    conditionExpr = s_expression();
                    CExpr_CheckUnwantedAssignment(conditionExpr);
                }
                testExpr = CExpr2_ConvertScalarOperand(conditionExpr, 0, 1);
                if (testExpr->rtype->type != TYPEINT && testExpr->rtype->type != TYPEFLOAT &&
                    testExpr->rtype->type != TYPEPOINTER)
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
                if (tk == ';')
                    fn_00449d60();
                else
                    CError_ReportError(ERR_SEMICOLON_EXPECTED);
            } else {
                fn_00449d60();
                testExpr = NULL;
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ')') {
                stepExpr = s_expression();
                CExpr_CheckUnusedExpression(stepExpr);
                if (tk == ')')
                    fn_00449d60();
                else
                    CError_ReportError(ERR_RPAREN_EXPECTED);
            } else {
                fn_00449d60();
                stepExpr = NULL;
            }
            if (warn_empty_control_statement()) {
                DAT_00588523 = 0;
                tk = CPrepTokenizer_GetNextToken();
                if (tk == ';' && !DAT_00588523)
                    CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
            } else
                tk = CPrepTokenizer_GetNextToken();
            if (testExpr) {
                stmt = AppendStmt(3);
                stmt->target.label = NewLabel();
                conditionLabel = stmt->target.label;
            } else
                conditionLabel = NewLabel();
            if (current_statement_number >= 0x1000) {
                if (current_statement_number >= 0xf000)
                    current_statement_number++;
                else
                    current_statement_number += 0x1000;
            } else
                current_statement_number <<= 3;
            stmt = AppendStmt(2);
            stmt->target.label = NewLabel();
            (topLabel = stmt->target.label)->target.stmt = stmt;
            breakLabel = NewLabel();
            continueLabel = NewLabel();
            bodyContext = *context;
            bodyContext.continueLabel = continueLabel;
            bodyContext.breakLabel = breakLabel;
            if (tk != '{') {
                bodyScope = NewScope();
                ScopedBody(&bodyContext);
                RestoreBlock(bodyScope);
            } else
                ScopedBody(&bodyContext);
            stmt = AppendStmt(2);
            stmt->target.label = continueLabel;
            continueLabel->target.stmt = stmt;
            if (stepExpr) {
                stmt = AppendStmt(4);
                stmt->expr.expression = stepExpr;
            }
            stmt = AppendStmt(2);
            stmt->target.label = conditionLabel;
            conditionLabel->target.stmt = stmt;
            CondJump(testExpr, topLabel, 1, 1);
            if (current_statement_number > 0x1000) {
                if (current_statement_number > 0xf000)
                    current_statement_number--;
                else
                    current_statement_number -= 0x1000;
            } else
                current_statement_number >>= 3;
            if (current_statement_number < 1)
                current_statement_number = 1;
            stmt = AppendStmt(2);
            stmt->target.label = breakLabel;
            breakLabel->target.stmt = stmt;
            if (forScope)
                RestoreBlock(forScope);
            return;
        case 0x13d:
            if (current_statement_number >= 0x1000) {
                if (current_statement_number >= 0xf000)
                    current_statement_number++;
                else
                    current_statement_number += 0x1000;
            } else
                current_statement_number <<= 3;
            stmt = AppendStmt(2);
            stmt->target.label = NewLabel();
            (topLabel = stmt->target.label)->target.stmt = stmt;
            continueLabel = NewLabel();
            breakLabel = NewLabel();
            bodyContext = *context;
            bodyContext.continueLabel = continueLabel;
            bodyContext.breakLabel = breakLabel;
            tk = CPrepTokenizer_GetNextToken();
            ScopedBody(&bodyContext);
            stmt = AppendStmt(2);
            stmt->target.label = continueLabel;
            continueLabel->target.stmt = stmt;
            if (tk != TK_WHILE)
                CError_ReportError(ERR_ILLEGAL_TOKEN);
            if (CPrepTokenizer_GetNextToken() != '(')
                CError_ReportErrorAndUpdateToken(ERR_LPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            expr = CExpr2_ConvertScalarOperand(s_expression(), 0, 1);
            if (expr->rtype->type != TYPEINT && expr->rtype->type != TYPEFLOAT && expr->rtype->type != TYPEPOINTER)
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            CExpr_CheckUnwantedAssignment(expr);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            CondJump(expr, topLabel, 1, 1);
            if (current_statement_number > 0x1000) {
                if (current_statement_number > 0xf000)
                    current_statement_number--;
                else
                    current_statement_number -= 0x1000;
            } else
                current_statement_number >>= 3;
            if (current_statement_number < 1)
                current_statement_number = 1;
            stmt = AppendStmt(2);
            stmt->target.label = breakLabel;
            breakLabel->target.stmt = stmt;
            break;
        case TK_WHILE:
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '(')
                CError_ReportErrorAndUpdateToken(ERR_LPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (copts.cplusplus && !use_legacy_condition_scope() && isdeclaration(1, 0, 0, 0x3d)) {
                newScope = NewScope();
                whileScope = newScope;
                conditionExpr = parse_declarations(0, 1, 0, 0);
                if (CScope_IsEmptyNameSpace(currentNameSpace)) {
                    RestoreBlock(newScope);
                    whileScope = NULL;
                }
            } else {
                conditionExpr = s_expression();
                whileScope = NULL;
                CExpr_CheckUnwantedAssignment(conditionExpr);
            }
            expr = CExpr2_ConvertScalarOperand(conditionExpr, 0, 1);
            if (expr->rtype->type != TYPEINT && expr->rtype->type != TYPEFLOAT && expr->rtype->type != TYPEPOINTER)
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else {
                if (warn_empty_control_statement()) {
                    DAT_00588523 = 0;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';' && !DAT_00588523)
                        CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
                } else
                    tk = CPrepTokenizer_GetNextToken();
            }
            stmt = AppendStmt(3);
            stmt->target.label = NewLabel();
            continueLabel = stmt->target.label;
            if (current_statement_number >= 0x1000) {
                if (current_statement_number >= 0xf000)
                    current_statement_number++;
                else
                    current_statement_number += 0x1000;
            } else
                current_statement_number <<= 3;
            stmt = AppendStmt(2);
            stmt->target.label = NewLabel();
            (topLabel = stmt->target.label)->target.stmt = stmt;
            breakLabel = NewLabel();
            bodyContext = *context;
            bodyContext.continueLabel = continueLabel;
            bodyContext.breakLabel = breakLabel;
            ScopedBody(&bodyContext);
            stmt = AppendStmt(2);
            stmt->target.label = continueLabel;
            continueLabel->target.stmt = stmt;
            CondJump(expr, topLabel, 1, 1);
            if (current_statement_number > 0x1000) {
                if (current_statement_number > 0xf000)
                    current_statement_number--;
                else
                    current_statement_number -= 0x1000;
            } else
                current_statement_number >>= 3;
            if (current_statement_number < 1)
                current_statement_number = 1;
            stmt = AppendStmt(2);
            stmt->target.label = breakLabel;
            breakLabel->target.stmt = stmt;
            if (whileScope)
                RestoreBlock(whileScope);
            return;
        case TK_IF: {
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '(')
                CError_ReportErrorAndUpdateToken(ERR_LPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (copts.cplusplus && !use_legacy_condition_scope() && isdeclaration(1, 0, 0, 0x3d)) {
                newScope = NewScope();
                ifScope = newScope;
                conditionExpr = parse_declarations(0, 1, 0, 0);
                if (CScope_IsEmptyNameSpace(currentNameSpace)) {
                    RestoreBlock(newScope);
                    ifScope = NULL;
                }
            } else {
                conditionExpr = s_expression();
                ifScope = NULL;
                CExpr_CheckUnwantedAssignment(conditionExpr);
            }
            expr = CExpr2_ConvertScalarOperand(conditionExpr, 0, 1);
            if (expr->rtype->type != TYPEINT && expr->rtype->type != TYPEFLOAT && expr->rtype->type != TYPEPOINTER)
                CError_ReportError(ERR_ILLEGAL_OPERAND);
            CondJump(expr, endLabel = continueLabel = NewLabel(), 0, 0);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else {
                if (warn_empty_control_statement()) {
                    DAT_00588523 = 0;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';' && !DAT_00588523)
                        CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
                } else
                    tk = CPrepTokenizer_GetNextToken();
            }
            ScopedBody(context);
            if (tk == TK_ELSE) {
                if (warn_empty_control_statement()) {
                    DAT_00588523 = 0;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';' && !DAT_00588523)
                        CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
                } else
                    tk = CPrepTokenizer_GetNextToken();
                stmt = AppendStmt(3);
                stmt->target.label = NewLabel();
                endLabel = stmt->target.label;
                stmt = AppendStmt(2);
                stmt->target.label = continueLabel;
                continueLabel->target.stmt = stmt;
                ScopedBody(context);
            }
            stmt = AppendStmt(2);
            stmt->target.label = endLabel;
            endLabel->target.stmt = stmt;
            if (ifScope)
                RestoreBlock(ifScope);
            return;
        }
        case '{':
            ScopedBody(context);
            return;
        case ';':
            break;
        case TK_ASM:
            if (copts.cplusplus || !copts.rejectZeroLengthArrayMembers) {
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '(') {
                    InlineAsm_ParseAsmStatement();
                    if (tk == ')') {
                        tk = CPrepTokenizer_GetNextToken();
                        break;
                    }
                    CError_ReportError(ERR_RPAREN_EXPECTED);
                    return;
                }
                if (tk == '{') {
                    InlineAsm_ParseAsmStatement();
                    if (tk != '}') {
                        CError_ReportError(ERR_RBRACE_EXPECTED);
                        return;
                    }
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';')
                        tk = CPrepTokenizer_GetNextToken();
                    fn_00449d60();
                    return;
                }
                CError_ReportError(ERR_LPAREN_EXPECTED);
                return;
            }
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        case TK_TRY:
            tk = CPrepTokenizer_GetNextToken();
            CExcept_ScanTryBlock(context, 0);
            return;
        case TK_USING:
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_NAMESPACE) {
                tk = CPrepTokenizer_GetNextToken();
                CScope_ParseUsingDirective(currentNameSpace);
            } else
                CScope_ParseUsingDeclaration(currentNameSpace, ACCESSPUBLIC, 0);
            return;
        case TK_NAMESPACE:
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER) {
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return;
            }
            namespaceName = data_00587fa0;
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '=') {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
            CScope_ParseNameSpaceAlias(namespaceName);
            break;
        case TK_IDENTIFIER:
            if (IsLabel()) {
                stmt = AppendStmt(2);
                if ((stmt->target.label = FindLabel()) != NULL) {
                    if (stmt->target.label->target.stmt)
                        CError_ReportError(ERR_LABEL_REDEFINED, data_00587fa0->name);
                } else {
                    stmt->target.label = NewLabel();
                    stmt->target.label->next = clabels;
                    clabels = stmt->target.label;
                    stmt->target.label->name = data_00587fa0;
                }
                stmt->target.label->target.stmt = stmt;
                tk = CPrepTokenizer_GetNextToken();
                tk = CPrepTokenizer_GetNextToken();
                parse_statement(context);
                return;
            }
            tk = TK_IDENTIFIER;
            /* fallthrough */
        default:
            if (copts.cplusplus && isdeclaration(1, 0, 0, 0)) {
                parse_declarations(0, 0, 0, 1);
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            stmt = AppendStmt(4);
            stmt->expr.expression = s_expression();
            CExpr_CheckUnusedExpression(stmt->expr.expression);
            break;
    }
    if (tk == ';') {
        CPrep_ResetBufferedTokenPosition();
        tk = CPrepTokenizer_GetNextToken();
        fn_00449d60();
    } else
        CError_ReportErrorAndUpdateToken(ERR_SEMICOLON_EXPECTED);
}

void check_function_result_automatic_variable(ENode *e)
{
    for (;;) {
        while (e->type == ECOMMA)
            e = e->data.diadic.right;

        switch (e->type) {
            case EOBJREF:
                if (e->data.objref->datatype != DLOCAL)
                    break;
                /* fall through */
            case EPRECOMP:
                CError_Warning(ERR_FUNCTION_RESULT_POINTER_REFERENCE_AUTOMATIC_VARIABLE);
                break;
            case EADD:
            case ESUB:
                check_function_result_automatic_variable(e->data.diadic.left);
                e = e->data.diadic.right;
                continue;
        }
        break;
    }
}

/* Additional information passed to class initialization. */

ENode *initialize_argument_object(ENode *initData, Type *type, UInt32 flags)
{
    ObjectList *argument;
    Object *object;
    ENode *node;
    ENode *local;
    ENodeList *info;

    for (argument = arguments; argument != NULL; argument = argument->next) {
        if (argument->object.value->name == blank_argument_name)
            break;
    }
    if (argument == NULL)
        CError_FATAL(1958);
    object = argument->object.value;

    node = CExpr_IsTempConstruction(initData, type, &local);
    if (node != NULL && local->type == EPRECOMP) {
        *local = *create_objectnode(object);
        return node;
    }

    if (type->type == TYPECLASS) {
        node = create_objectnode(object);
        info = CompilerTools_AllocatePool(sizeof(ENodeList));
        info->next = NULL;
        info->node = initData;
        return CExpr_ConstructObject(type, node, info, 1, 1, 0, 1, 0);
    }

    node = makemonadicnode(create_objectnode(object), EINDIRECT);
    node->rtype = type;
    return makediadicnode(node, oldassignmentpromotion(initData, type, flags, 1), EASS);
}

static Statement *NewStmt(UInt8 type)
{
    Statement *stmt = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));

    stmt->next = NULL;
    stmt->type = type;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = UINT_00587fc4;
    PTR_00587644->next = stmt;
    PTR_00587644 = stmt;
    return stmt;
}

void generate_conditional_jump(ENode *expr, CLabel *dest, CLabel *other, Boolean sense, Boolean flag)
{
    CLabel *label;
    ENode *operand;
    Statement *stmt;

    if (expr == NULL) {
        if (sense) {
            NewStmt(3)->target.label = dest;
            return;
        }
        return;
    }
    if (expr->type == ETYPCON && expr->rtype->type == TYPEINT && expr->data.monadic->rtype->type == TYPEINT &&
        expr->rtype->size >= expr->data.monadic->rtype->size)
        expr = expr->data.monadic;
    if (isnotzero(expr)) {
        if (sense) {
            NewStmt(3)->target.label = dest;
            return;
        }
        return;
    }
    if (CExpr2_IsZero(expr)) {
        if (!sense) {
            NewStmt(3)->target.label = dest;
            return;
        }
        return;
    }
    if (expr->type == ELOGNOT) {
        generate_conditional_jump(expr->data.diadic.left, dest, other, !sense, flag);
        return;
    }
    if (expr->type == ELOR) {
        label = (CLabel *)CompilerTools_AllocatePool(sizeof(CLabel));
        memclrw(label, sizeof(CLabel));
        label->uniquename = CParser_GetUniqueName();
        label->name = label->uniquename;
        if (sense) {
            generate_conditional_jump(expr->data.diadic.left, dest, label, 1, flag);
            label->target.stmt = (Statement *)NewStmt(2);
            label->target.stmt->target.label = label;
            generate_conditional_jump(expr->data.diadic.right, dest, other, 1, flag);
            return;
        }
        generate_conditional_jump(expr->data.diadic.left, other, label, 1, flag);
        label->target.stmt = (Statement *)NewStmt(2);
        label->target.stmt->target.label = label;
        generate_conditional_jump(expr->data.diadic.right, dest, other, 0, flag);
        return;
    }
    if (expr->type == ELAND) {
        label = (CLabel *)CompilerTools_AllocatePool(sizeof(CLabel));
        memclrw(label, sizeof(CLabel));
        label->uniquename = CParser_GetUniqueName();
        label->name = label->uniquename;
        if (sense) {
            generate_conditional_jump(expr->data.diadic.left, other, label, 0, flag);
            label->target.stmt = (Statement *)NewStmt(2);
            label->target.stmt->target.label = label;
            generate_conditional_jump(expr->data.diadic.right, dest, other, 1, flag);
            return;
        }
        generate_conditional_jump(expr->data.diadic.left, dest, label, 0, flag);
        label->target.stmt = (Statement *)NewStmt(2);
        label->target.stmt->target.label = label;
        generate_conditional_jump(expr->data.diadic.right, dest, other, 0, flag);
        return;
    }
    stmt = NewStmt(6);
    stmt->target.label = dest;
    stmt->expr.expression = expr;
    if (!sense)
        stmt->type = ST_IFNGOTO;
    if (flag)
        stmt->flags |= 4;
}

/* State passed between declaration parsing routines. */

ENode *parse_declarations(char mode, int singleDeclaration, char allowEmpty, char stopAfterDeclaration)
{
    DeclInfo declarationState;
    BufferedToken snapshot;
    Type *baseType;
    UInt32 baseQualifiers;

    deferred_expression = NULL;
    data_00580882 = singleDeclaration;
    while ((char)singleDeclaration != 0 || isdeclaration(copts.cplusplus, 0, 0, 0)) {
        statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
        memclrw(&declarationState, sizeof(declarationState));
        declarationState.requireMangledName = data_0058088c;
        CParser_GetDeclSpecs(&declarationState, 0);
        if (declarationState.dtype->type == TYPETEMPLATE) {
            CError_ReportError(ERR_ILLEGAL_TYPE);
            declarationState.dtype = (Type *)&stsignedint;
        }
        baseType = declarationState.dtype;
        baseQualifiers = declarationState.qual;
        if (tk != ';') {
            for (;;) {
                snapshot = *CPrep_GetLastBufferedToken();
                declarationState.name = NULL;
                CDecl_ParseDeclarator(&declarationState);
                if (declarationState.name != NULL) {
                    if (declarationState.storage != TK_TYPEDEF) {
                        if (declarationState.dtype->type == TYPEFUNC) {
                            if (CDecl_FunctionDeclarator(&declarationState, CScope_FindGlobalNS(currentNameSpace), 0,
                                                         0) == 0)
                                break;
                        } else {
                            declare_local_object(&declarationState, &snapshot, mode, singleDeclaration);
                        }
                    } else {
                        CDecl_TypedefDeclarator(&declarationState);
                    }
                } else {
                    CError_ReportError(ERR_DECLARATOR_EXPECTED);
                }
                if (tk == ';')
                    break;
                if (tk != ',') {
                    if ((char)singleDeclaration == 0)
                        CError_ReportError(ERR_SEMICOLON_EXPECTED);
                    break;
                }
                declarationState.nspace = NULL;
                declarationState.dtype = baseType;
                declarationState.qual = baseQualifiers;
                tk = CPrepTokenizer_GetNextToken();
            }
        } else {
            CParser_CheckAnonymousUnion(&declarationState, 1);
        }
        if ((char)singleDeclaration != 0 || stopAfterDeclaration)
            break;
        tk = CPrepTokenizer_GetNextToken();
    }
    if ((char)singleDeclaration != 0) {
        if (deferred_expression == NULL) {
            if (allowEmpty == 0) {
                CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                deferred_expression = nullnode();
            }
        } else {
            deferred_expression = checkreference(deferred_expression);
        }
    }
    return deferred_expression;
}

static Statement *allocate_and_append_statement(UInt8 type)
{
    Statement *stmt = (Statement *)CompilerTools_AllocatePool(0x1a);
    stmt->next = NULL;
    stmt->type = type;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = UINT_00587fc4;
    PTR_00587644->next = stmt;
    PTR_00587644 = stmt;
    return stmt;
}

void declare_local_object(DeclInfo *declaration, BufferedToken *declarationToken, char isParameter,
                          char forbidInitialization)
{
    char nameBuffer[64];
    char *name;
    NameSpace *scope;
    NameSpaceObjectList *objects;
    Object *existing;
    char *nameSuffix;
    Object *object;
    ObjectList *local;
    Statement *statement;
    Object *found;
    NameSpaceName *entry;

    if (declaration->nspace != NULL)
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
    CDecl_CompleteType(declaration->dtype);
    existing = NULL;
    objects = CScope_FindName(currentNameSpace, declaration->name);
    if (objects != NULL) {
        switch (objects->object->otype) {
            case OT_OBJECT:
                existing = (Object *)objects->object;
                break;
            case OT_NAMESPACE:
                CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                return;
            case OT_ENUMCONST:
            case OT_TYPE:
                CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
                break;
            default:
                CError_FATAL(1541);
                break;
            case OT_TYPETAG:
                break;
        }
    }
    if (existing != NULL)
        CError_ReportError(ERR_OBJECT_REDEFINED, existing);

    if (declaration->storage == STORAGE_EXTERN) {
        found = NULL;
        scope = CScope_FindGlobalNS(currentNameSpace);
        objects = CScope_FindName(scope, declaration->name);
        if (objects != NULL) {
            switch (objects->object->otype) {
                case OT_OBJECT:
                    found = (Object *)objects->object;
                    break;
                case OT_NAMESPACE:
                    CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                    return;
                case OT_ENUMCONST:
                case OT_TYPE:
                    CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
                    break;
                default:
                    CError_FATAL(1578);
                    break;
                case OT_TYPETAG:
                    break;
            }
        }
        if (found != NULL) {
            if (iscpp_typeequal(declaration->dtype, found->type) == 0 ||
                (declaration->qual & (Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK)) !=
                    (found->qual & (Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK)))
                CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, declaration->name->name, found->type,
                                   found->qual, declaration->dtype, declaration->qual);
        } else {
            found = CParser_NewObject(declaration);
            found->nspace = scope;
        }
        CParser_NewAliasObject(found, 0);
        return;
    }

    if (declaration->storage != STORAGE_STATIC)
        object = CParser_CreateObject(declaration);
    else
        object = CParser_NewObject(declaration);
    object->name = declaration->name;
    object->type = declaration->dtype;
    object->qual = declaration->qual;
    object->sclass = declaration->storage;

    switch (declaration->storage) {
        case STORAGE_STATIC:
            if (isParameter != 0) {
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                break;
            }
            if (forbidInitialization != 0)
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            if (CDecl_CheckObjectType(declaration->dtype) == 0)
                break;
            CError_ReportIllegalFlags(declaration->qual &
                                      ~(Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK));
            CParser_NewAliasObject(object, 0);
            object->nspace = registration_context;
            object->datatype = DDATA;
            name = object->name->name;
            if (data_00588238 == NULL || (data_00588238->qual & Q_INLINE) == 0 ||
                CParser_HasInternalLinkage(data_00588238) != 0) {
                object->name = CParser_AppendUniqueName(name);
            } else {
                sprintf(nameBuffer, "$localstatic%ld$", local_name_counter++);
                nameSuffix = COptimizer_GetFunctionObject(data_00588238)->name;
                object->name = CParser_NameConcat(name, CParser_NameConcat(nameBuffer, nameSuffix)->name);
                object->qual |= Q_IMPLICIT_WEAK;
                object->sclass = TK_EOF;
            }
            if (copts.cplusplus != 0) {
                localstatic_init_guard = NULL;
                CInit_InitializeStaticData(object, append_localstatic_init_expr);
                if (localstatic_init_guard != NULL) {
                    statement = CompilerTools_AllocatePool(sizeof(Statement));
                    statement->next = NULL;
                    statement->type = ST_EXPRESSION;
                    statement->value = current_statement_number;
                    statement->flags = 0;
                    statement->sourceoffset = statement_sourceoffset;
                    statement->dobjstack = UINT_00587fc4, PTR_00587644->next = statement;
                    PTR_00587644 = statement;
                    statement->expr.expression = makediadicnode(create_objectnode(localstatic_init_guard),
                                                                intconstnode((Type *)&stsignedchar, 1), EASS);
                    statement = allocate_and_append_statement(2);
                    statement->target.label = data_0058087e;
                    data_0058087e->target.stmt = statement;
                }
            } else {
                CInit_InitializeData(object);
            }
            break;
        case 0:
        case TK_AUTO:
        case TK_REGISTER: {
            if (CDecl_CheckObjectType(declaration->dtype) != 0) {
                CError_ReportIllegalFlags(declaration->qual & ~(Q_CV | Q_PASCAL | Q_ALIGNED_MASK));
                object->datatype = DLOCAL;
                object->u.var.info = CPrep_AllocateVarInfo();
                object->u.var.info->func = data_00588238;
                if (object->sclass == TK_REGISTER) {
                    if (copts.uniformSpillBlockWeight == 0)
                        object->u.var.info->usage = 100;
                    else
                        object->u.var.info->usage = 5;
                }
                if (object->type != NULL && is_volatile_object(object) != 0)
                    object->u.var.info->noregister = 1;
                object->u.var.info->deftoken = *declarationToken;
                if (object->sclass == TK_REGISTER && isParameter != 0)
                    object->u.var.info->usage = 100;
                CScope_AddObject(currentNameSpace, object->name, (ObjBase *)object);
                if (isParameter == 0) {
                    if (declaration->dtype->type == TYPECLASS && TYPE_CLASS(declaration->dtype)->sominfo != NULL) {
                        CSOM_004e4390(object);
                    } else {
                        CInit_InitializeAutoData(object, append_or_defer_expression_statement,
                                                 register_destructor_object);
                        if (object->type != declaration->dtype) {
                            if (object->type->type == TYPESTRUCT || object->type->type == TYPECLASS) {
                                CError_ASSERT(1675, currentNameSpace->is_hash == 0);
                                entry = CScope_FindNameSpaceName(currentNameSpace, object->name);
                                CError_ASSERT(1676, entry != 0);
                                CError_ASSERT(1677, entry->first.object == (ObjBase *)object);
                                CError_ASSERT(1678, entry->first.next == 0);
                                entry->name = CParser_AppendUniqueName(object->name->name);
                                found = CParser_NewAliasObject(object, 0);
                                found->type = declaration->dtype;
                            }
                        }
                    }
                }
                if (object->datatype == DLOCAL) {
                    local = CompilerTools_AllocatePool(sizeof(ObjectList));
                    local->object.value = object;
                    local->next = locals;
                    locals = local;
                }
                (void)CanAllocObject(declaration->dtype);
            }
            break;
        }
        default:
            CError_FATAL(1701);
    }
}

void register_destructor_object(Type *type, Object *object, long offset, long flags)
{
    CExcept_RegisterDestructorObject(object, offset, CClass_Destructor((TypeClass *)type), flags);
}

void append_or_defer_expression_statement(ENode *node)
{
    struct Statement *record;
    if (data_00580882 != 0) {
        if (deferred_expression != NULL) {
            record = (struct Statement *)CompilerTools_AllocatePool(sizeof(struct Statement));
            record->next = NULL;
            record->type = 4U;
            record->value = current_statement_number;
            record->flags = 0U;
            record->sourceoffset = statement_sourceoffset;
            record->dobjstack = UINT_00587fc4;
            PTR_00587644->next = record;
            PTR_00587644 = record;
            record->expr.expression = deferred_expression;
        }
        deferred_expression = (ENode *)node;
    } else {
        record = (struct Statement *)CompilerTools_AllocatePool(sizeof(struct Statement));
        record->next = NULL;
        record->type = 4U;
        record->value = current_statement_number;
        record->flags = 0U;
        record->sourceoffset = statement_sourceoffset;
        record->dobjstack = UINT_00587fc4;
        PTR_00587644->next = record;
        PTR_00587644 = record;
        record->expr.expression = (ENode *)node;
    }
}

/* Statement metadata holding the two initial position references. */

/* Function-body statement with source-position metadata. */

static inline Statement *CFunc_NewStatement(UInt8 kind)
{
    Statement *stmt = CompilerTools_AllocatePool(sizeof(Statement));
    stmt->next = NULL;
    stmt->type = kind;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = UINT_00587fc4;
    PTR_00587644->next = stmt;
    PTR_00587644 = stmt;
    return stmt;
}

void append_localstatic_init_expr(ENode *expr)
{
    char buf[64];
    Object *obj;
    Statement *stmt;
    CLabel *info;

    if (localstatic_init_guard == NULL) {
        localstatic_init_guard = CParser_NewCompilerDefDataObject();
        localstatic_init_guard->type = (Type *)&stsignedchar;
        localstatic_init_guard->sclass = 0x102;
        obj = localstatic_init_guard;
        if (data_00588238 == NULL || (data_00588238->qual & Q_INLINE) == 0 ||
            CParser_HasInternalLinkage(data_00588238) != 0) {
            obj->name = CParser_AppendUniqueName("init");
        } else {
            sprintf(buf, "$localstatic%ld$", local_name_counter++);
            obj->name = CParser_NameConcat(
                "init", CParser_NameConcat(buf, COptimizer_GetFunctionObject(data_00588238)->name)->name);
            obj->qual |= Q_IMPLICIT_WEAK;
            obj->sclass = TK_EOF;
        }
        fn_004ceab0(localstatic_init_guard, NULL, NULL, localstatic_init_guard->type->size);
        info = CompilerTools_AllocatePool(sizeof(CLabel));
        memclrw(info, sizeof(CLabel));
        info->uniquename = CParser_GetUniqueName();
        info->name = info->uniquename;
        data_0058087e = info;
        stmt = CFunc_NewStatement(6);
        stmt->expr.expression = create_objectnode(localstatic_init_guard);
        stmt->target.label = data_0058087e;
    }
    stmt = CFunc_NewStatement(4);
    stmt->expr.expression = expr;
}

/* Labels and statements used while parsing a switch. */

static inline Statement *MakeCaseStatement(void)
{
    Statement *statement = CompilerTools_AllocatePool(0x1a);
    statement->next = NULL;
    statement->type = ST_LABEL;
    statement->value = current_statement_number;
    statement->flags = 0;
    statement->sourceoffset = statement_sourceoffset;
    statement->dobjstack = UINT_00587fc4;
    PTR_00587644->next = statement;
    PTR_00587644 = statement;
    return statement;
}

void parse_case_statement(struct StatementContext *context)
{
    SwitchCase *entry;
    CLabel *label;
    Statement *statement;
    SwitchCase *newCase;
    CInt64 value;

    if (context->switchInfo == NULL) {
        CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
        return;
    }
    tk = CPrepTokenizer_GetNextToken();
    value = CExpr_IntConstConvert(context->switchInfo->sizetype, context->switchInfo->sizetype, fn_004f0b30());
    for (entry = context->switchInfo->cases; entry != NULL; entry = entry->next) {
        if (CInt64_GreaterEqual(entry->min, value) && CInt64_LessEqual(entry->min, value)) {
            CError_ReportError(ERR_CASE_CONSTANT_DEFINED_MORE_THAN_ONCE);
        }
    }
    statement = MakeCaseStatement();
    label = CompilerTools_AllocatePool(sizeof(*label));
    memclrw(label, sizeof(*label));
    label->uniquename = CParser_GetUniqueName();
    label->name = label->uniquename;
    statement->target.label = label;
    statement->target.label->target.stmt = statement;
    newCase = CompilerTools_AllocatePool(sizeof(*newCase));
    newCase->min = value;
    newCase->label = statement->target.label;
    newCase->next = context->switchInfo->cases;
    context->switchInfo->cases = newCase;
    if (tk != ':') {
        CError_ReportErrorAndUpdateToken(ERR_COLON_EXPECTED);
    } else {
        tk = CPrepTokenizer_GetNextToken();
    }
    parse_statement(context);
}

static inline Boolean CFunc_ScopeContains(CException *from, CException *to)
{
    if (!to)
        return 1;
    while (from) {
        if (from == to)
            return 1;
        from = from->next;
    }
    return 0;
}

/* A jump from STMT to DEST that enters the scope of an object with a destructor is warned about. */
static inline void CFunc_CheckJump(Statement *stmt, Statement *dest)
{
    CException *to = dest->dobjstack;

    if (stmt->dobjstack != to && !CFunc_ScopeContains(stmt->dobjstack, to)) {
        while (to) {
            if (CExcept_ActionNeedsDestruction(to) && to->kind != 14) {
                CError_Warning(ERR_ILLEGAL_JUMP_PAST_INITIALIZER);
                break;
            }
            to = to->next;
        }
    }
}

/* Whether leaving STMT for DEST, whose scopes differ, ends the scope of an object that needs cleaning up. */
static inline Boolean CFunc_NeedsCleanup(Statement *stmt, Statement *dest)
{
    CException *from, *p;

    from = stmt->dobjstack;
    for (p = dest->dobjstack; p; p = p->next)
        if (p == from)
            return 0;
    for (from = stmt->dobjstack, p = dest->dobjstack; from && from != p; from = from->next)
        if (CExcept_ActionNeedsDestruction(from))
            return 1;
    return 0;
}

static inline Boolean CFunc_AnyCleanup(CException *p)
{
    while (p) {
        if (CExcept_ActionNeedsDestruction(p))
            return 1;
        if (!p)
            break;
        p = p->next;
    }
    return 0;
}

/* The innermost action of STMT's stack that lies just inside one of TO's. */
static inline CException *CFunc_CommonScope(Statement *stmt, CException *to)
{
    CException *p;

    while (to) {
        for (p = stmt->dobjstack; p; p = p->next)
            if (p->next == to)
                return p;
        to = to->next;
    }
    return NULL;
}

static inline Statement *CFunc_EmitCleanups(Statement *stmt, CException *p, CException *stop)
{
    for (; p; p = p->next) {
        stmt = CExcept_ActionCleanup(p, stmt);
        if (p == stop)
            break;
    }
    return stmt;
}

static inline Statement *CFunc_EmitAllCleanups(Statement *stmt, CException *p)
{
    for (; p; p = p->next) {
        stmt = CExcept_ActionCleanup(p, stmt);
        if (!p)
            break;
    }
    return stmt;
}

/* C++: a jump into the scope of an object with a destructor is warned about; with exceptions on, the destructor
   calls are inserted where control leaves such a scope (falling through, jumping out or returning). */
void CFunc_DestructorCleanup(Statement *first)
{
    SwitchCase *c;
    Statement *stmt, *next;

    if (!copts.cplusplus)
        return;
    for (next = first; next; next = next->next) {
        switch (next->type) {
            case ST_SWITCH:
                CFunc_CheckJump(next, next->target.switchDescriptor->defaultlabel->target.stmt);
                for (c = next->target.switchDescriptor->cases; c; c = c->next)
                    CFunc_CheckJump(next, c->label->target.stmt);
                break;
            case ST_GOTO:
            case ST_IFGOTO:
            case ST_IFNGOTO:
                CFunc_CheckJump(next, next->target.label->target.stmt);
                break;
            case ST_NOP:
            case ST_LABEL:
            case ST_EXPRESSION:
            case ST_RETURN:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_GOTOEXPR:
            case ST_ASM:
                break;
            default:
                CError_FATAL(1246);
                break;
        }
    }
    if (!exception_cleanup_registered)
        return;
    stmt = first;
    for (;;) {
        if (!(next = stmt->next)) {
            if (stmt->type != ST_RETURN && stmt->dobjstack)
                CFunc_EmitAllCleanups(stmt, stmt->dobjstack);
            break;
        }
        switch (next->type) {
            case ST_GOTO:
                if (stmt->dobjstack != next->target.label->target.stmt->dobjstack &&
                    CFunc_NeedsCleanup(stmt, next->target.label->target.stmt))
                    CFunc_EmitCleanups(stmt, stmt->dobjstack,
                                       CFunc_CommonScope(stmt, next->target.label->target.stmt->dobjstack));
                stmt = next;
                break;
            case ST_RETURN:
                if (next->expr.expression && CFunc_AnyCleanup(stmt->dobjstack))
                    CFunc_0047b9a0(stmt, next);
                else if (stmt->dobjstack)
                    CFunc_EmitAllCleanups(stmt, stmt->dobjstack);
                stmt = next;
                break;
            default:
                switch (stmt->type) {
                    case ST_NOP:
                    case ST_LABEL:
                    case ST_EXPRESSION:
                    case ST_IFGOTO:
                    case ST_IFNGOTO:
                    case ST_BEGINCATCH:
                    case ST_ENDCATCH:
                    case ST_ENDCATCHDTOR:
                    case ST_ASM:
                        if (stmt->dobjstack != next->dobjstack && CFunc_NeedsCleanup(stmt, next))
                            CFunc_EmitCleanups(stmt, stmt->dobjstack, CFunc_CommonScope(stmt, next->dobjstack));
                        break;
                    case ST_GOTO:
                    case ST_SWITCH:
                    case ST_RETURN:
                    case ST_GOTOEXPR:
                        break;
                    default:
                        CError_FATAL(1310);
                        break;
                }
                switch (next->type) {
                    case ST_NOP:
                    case ST_LABEL:
                    case ST_EXPRESSION:
                    case ST_SWITCH:
                    case ST_BEGINCATCH:
                    case ST_ENDCATCH:
                    case ST_ENDCATCHDTOR:
                    case ST_ASM:
                        stmt = next;
                        break;
                    case ST_IFGOTO:
                    case ST_IFNGOTO:
                        if (next->dobjstack != next->target.label->target.stmt->dobjstack &&
                            CFunc_NeedsCleanup(next, next->target.label->target.stmt))
                            stmt = insert_conditional_goto_cleanup(next);
                        else
                            stmt = next;
                        break;
                    default:
                        CError_FATAL(1339);
                        break;
                }
        }
    }
}

static inline Statement *CFunc_0047b880_inline1(char type, Statement *after)
{
    Statement *stmt;
    stmt = (Statement *)CompilerTools_AllocatePool(26);
    stmt->next = after->next;
    after->next = stmt;
    stmt->type = type;
    stmt->value = after->value;
    stmt->flags = 0;
    stmt->sourceoffset = after->sourceoffset;
    stmt->dobjstack = after->dobjstack;
    return stmt;
}

static inline void FindStatementReference(Statement *statement, CException **candidate, CException **match)
{
    while (*candidate != NULL) {
        *match = statement->dobjstack;
        if (*match != NULL) {
            do {
                if ((*match)->next == *candidate) {
                    return;
                }
                *match = (*match)->next;
            } while (*match != NULL);
        }
        *candidate = (*candidate)->next;
    }
    *match = NULL;
}

Statement *insert_conditional_goto_cleanup(Statement *statement)
{
    CException *reference;
    CException *candidate;
    Statement *tail;
    CException *match;
    Statement *target;
    Statement *jump;
    CLabel *data;

    if (statement->type == ST_IFGOTO) {
        statement->type = ST_IFNGOTO;
    } else {
        statement->type = ST_IFGOTO;
    }
    data = (CLabel *)CompilerTools_AllocatePool(sizeof(*data));
    memclrw(data, sizeof(*data));
    data->name = data->uniquename = CParser_GetUniqueName();
    candidate = statement->target.label->target.stmt->dobjstack;
    FindStatementReference(statement, &candidate, &match);
    tail = statement;
    reference = statement->dobjstack;
    while (reference != NULL) {
        tail = CExcept_ActionCleanup(reference, tail);
        if (reference == match) {
            break;
        }
        reference = reference->next;
    }
    jump = CFunc_0047b880_inline1(ST_GOTO, tail);
    jump->target.label = statement->target.label;
    target = CFunc_0047b880_inline1(ST_LABEL, jump);
    target->target.label = data;
    data->target.stmt = target;
    statement->target.label = data;
    target->dobjstack = statement->dobjstack;
    return target;
}

static inline char CFunc_0047b9a0_inline1(Statement *a1)
{
    CException *v12;
    CException *v12s;
    v12 = (v12s = a1->dobjstack);
    if (v12s != NULL) {
        do {
            if (CExcept_ActionNeedsDestruction(v12) != 0) {
                return (char)1;
            }
            if ((int)v12 == 0) {
                break;
            }
            v12 = v12->next;
        } while ((int)v12 != 0);
    }
    return (char)0;
}

static inline int CFunc_0047b9a0_inline2(Statement *a0, Statement *a1)
{
    int p7;
    int p5;
    for (p7 = (int)a1->dobjstack; p7 != 0; p7 = *(int *)p7) {
        for (p5 = (int)a0->dobjstack; p5 != 0; p5 = *(int *)p5) {
            if (*(int *)p5 == p7) {
                return p5;
            }
        }
    }
    return 0;
}

static inline Statement *CFunc_0047b9a0_inline3(CException *node, Statement *carry, CException *stop)
{
    CException *p;
    if (node != NULL) {
        p = node;
        do {
            carry = CExcept_ActionCleanup(p, carry);
            if (p == stop) {
                break;
            }
            p = p->next;
        } while (p != NULL);
    }
    return carry;
}

static inline Statement *CFunc_InsertStatement(Statement *statement)
{
    Statement *inserted;
    inserted = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
    inserted->next = statement->next;
    statement->next = inserted;
    inserted->type = ST_EXPRESSION;
    inserted->value = statement->value;
    inserted->flags = 0;
    inserted->sourceoffset = statement->sourceoffset;
    inserted->dobjstack = statement->dobjstack;
    return inserted;
}

void CFunc_0047b9a0(Statement *statement, Statement *expression)
{
    CException *targetScope;
    Object *temporary;
    CException *scope;
    char needsCleanup;
    int originalScope;
    CException *currentScope;
    char needsTemporary;
    Statement *inserted;
    Statement *direct;
    CException *savedScope;
    Type *value;

    if ((currentScope = statement->dobjstack) != (targetScope = expression->dobjstack)) {
        originalScope = (int)currentScope, scope = targetScope;
        while (scope != NULL) {
            if ((int)scope == originalScope)
                goto sharedScope;
            scope = scope->next;
        }
        while (currentScope != NULL && currentScope != targetScope) {
            if (CExcept_ActionNeedsDestruction(currentScope) != 0) {
                needsCleanup = 1;
                goto scopeChecked;
            }
            currentScope = currentScope->next;
        }
    sharedScope:
        needsCleanup = 0;
    scopeChecked:
        if (needsCleanup != 0) {
            scope = (CException *)CFunc_0047b9a0_inline2(statement, expression);
            savedScope = statement->dobjstack;
            statement = CFunc_0047b9a0_inline3(savedScope, statement, scope);
        }
    }
    needsTemporary = CFunc_0047b9a0_inline1(expression);
    if (needsTemporary != 0) {
        if (CMachine_FunctionRequiresMemoryReturn((TypeFunc *)data_00588238->type) != 1) {
            value = expression->expr.expression->rtype;
            temporary = CParser_NewLocalDataObject(NULL, 1);
            temporary->name = (HashNameNode *)CParser_GetUniqueName();
            temporary->type = value;
            CFunc_SetupLocalVarInfo(temporary);
            inserted = CFunc_InsertStatement(statement);
            inserted->expr.expression = makediadicnode(create_objectnode(temporary), expression->expr.expression, 30);
            inserted->sourceoffset = expression->sourceoffset;
            CFunc_0047b9a0_inline3(expression->dobjstack, inserted, NULL);
            expression->expr.expression = create_objectnode(temporary);
        } else {
            direct = CFunc_InsertStatement(statement);
            direct->expr.expression = expression->expr.expression;
            direct->sourceoffset = expression->sourceoffset;
            CFunc_0047b9a0_inline3(expression->dobjstack, direct, NULL);
            expression->expr.expression = nullnode();
        }
    }
}

/* reference prototype (types may differ here): extern void CFunc_CodeCleanup(Statement *stmt); */
void CFunc_CodeCleanup(Statement *stmt)
{
    if ((data_00588040 != NULL) && (data_00588040->sominfo != NULL)) {
        CSOM_GenerateSomselfAssignment(data_00588040, stmt);
    }
    CFunc_WarnUnused();
    CExcept_ExceptionTansform(stmt);
}

void CFunc_WarnUnused(void)
{
    ObjectList *local;
    ObjectList *argument;

    if (copts.fa2) {
        for (local = locals; local; local = local->next) {
            if (!(local->object.value->flags & 1) &&
                !CParser_IsNullOrAtOrDollarPrefixedName(local->object.value->name) &&
                !(local->object.value->qual & Q_INLINE_DATA)) {
                CError_SetBufferedToken(&local->object.value->u.var.info->deftoken);
                CError_Warning(ERR_VARIABLE_ARGUMENT_NOT_USED_FUNCTION, local->object.value->name->name);
            }
        }
    }
    if (copts.fa3) {
        for (argument = arguments; argument; argument = argument->next) {
            if (!(argument->object.value->flags & 1) &&
                !CParser_IsNullOrAtOrDollarPrefixedName(argument->object.value->name) &&
                argument->object.value->name != this_arg_name && argument->object.value->name != this_self_name) {
                CError_SetBufferedToken(&declaration_token);
                CError_Warning(ERR_VARIABLE_ARGUMENT_NOT_USED_FUNCTION, argument->object.value->name->name);
            }
        }
    }
}

ENode *sub_47bca0(ENode *node)
{
    switch (node->type) {
        case EPRECOMP: {
            Object *n = CException_GetTempObject(node);
            if (node->data.temp.needs_dtor) {
                CleanNode *r = (CleanNode *)CompilerTools_AllocatePool(12);
                r->next = data_00580890;
                data_00580890 = r;
                r->object = n;
                if (node->data.temp.type->type != TYPECLASS ||
                    (r->dtor = CClass_Destructor((TypeClass *)node->data.temp.type)) == NULL) {
                    CError_FATAL(758);
                }
            }
            node->type = EOBJREF;
            node->data.objref = n;
            return node;
        }
        case EMFPOINTER:
            return fn_0047bff0(node);
        case ECOND:
            return rewrite_cond_with_cleannodes(node);
        case ELAND:
        case ELOR:
            return isolate_diadic_right_cleanup(node);
        case EFUNCCALL:
        case EFUNCCALLP:
            rewrite_enode_list_nodes(node->data.funccall.args);
            node->data.diadic.left = sub_47bca0(node->data.diadic.left);
            return node;
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
        case EASS:
        case EMULASS:
        case EDIVASS:
        case EMODASS:
        case EADDASS:
        case ESUBASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
        case ECOMMA:
            node->data.diadic.left = sub_47bca0(node->data.diadic.left);
            node->data.diadic.right = sub_47bca0(node->data.diadic.right);
            return node;
        case EPOSTINC:
        case EPOSTDEC:
        case EPREINC:
        case EPREDEC:
        case EINDIRECT:
        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
        case ETYPCON:
        case EBITFIELD:
            node->data.diadic.left = sub_47bca0(node->data.diadic.left);
            return node;
        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ENULLCHECK:
        case ELOCOBJ:
        case EMEMBER:
        case EASSBLK:
            return node;
        default:
            CError_FATAL(923);
            return node;
    }
}

/* A linked entry whose payload is transformed by fn_0047bca0. */

void rewrite_enode_list_nodes(ENodeList *entry)
{
    ENodeList *next3;
    ENodeList *next1;
    ENodeList *next2;
    ENodeList *next5;
    ENodeList *next6;
    ENodeList *next4;
    ENodeList *next7;
    ENodeList *next8;

    if (entry != NULL) {
        next1 = entry->next;
        if (next1 != NULL) {
            next2 = next1->next;
            if (next2 != NULL) {
                next3 = next2->next;
                if (next3 != NULL) {
                    next4 = next3->next;
                    if (next4 != NULL) {
                        next5 = next4->next;
                        if (next5 != NULL) {
                            next6 = next5->next;
                            if (next6 != NULL) {
                                next7 = next6->next;
                                if (next7 != NULL) {
                                    next8 = next7->next;
                                    if (next8 != NULL) {
                                        rewrite_enode_list_nodes(next8->next);
                                        next8->node = sub_47bca0(next8->node);
                                    }
                                    next7->node = sub_47bca0(next7->node);
                                }
                                next6->node = sub_47bca0(next6->node);
                            }
                            next5->node = sub_47bca0(next5->node);
                        }
                        next4->node = sub_47bca0(next4->node);
                    }
                    next3->node = sub_47bca0(next3->node);
                }
                next2->node = sub_47bca0(next2->node);
            }
            next1->node = sub_47bca0(next1->node);
        }
        entry->node = sub_47bca0(entry->node);
    }
}

ENode *isolate_diadic_right_cleanup(ENode *node)
{
    CleanNode *saved;
    node->data.diadic.left = sub_47bca0(node->data.diadic.left);
    saved = data_00580890;
    data_00580890 = NULL;
    node->data.diadic.right = sub_47bca0(node->data.diadic.right);
    if (data_00580890 != NULL) {
        node->data.diadic.right = sub_47c050(node->data.diadic.right, data_00580890, 1);
    }
    data_00580890 = saved;
    return node;
}

ENode *rewrite_cond_with_cleannodes(ENode *node)
{
    CleanNode *saved;
    node->data.cond.cond = sub_47bca0(node->data.cond.cond);
    saved = data_00580890;
    data_00580890 = NULL;
    node->data.cond.expr1 = sub_47bca0(node->data.cond.expr1);
    if (data_00580890 != NULL) {
        node->data.cond.expr1 = sub_47c050(node->data.cond.expr1, data_00580890, 1);
        data_00580890 = NULL;
    }
    node->data.cond.expr2 = sub_47bca0(node->data.cond.expr2);
    if (data_00580890 != NULL) {
        node->data.cond.expr2 = sub_47c050(node->data.cond.expr2, data_00580890, 1);
    }
    data_00580890 = saved;
    return node;
}

ENode *fn_0047bff0(ENode *statement)
{
    CleanNode *savedState;
    statement->data.diadic.left = sub_47bca0(statement->data.diadic.left);
    savedState = data_00580890;
    data_00580890 = NULL;
    statement->data.diadic.right = sub_47bca0(statement->data.diadic.right);
    if (data_00580890 != NULL) {
        statement->data.diadic.right = sub_47c050(statement->data.diadic.right, data_00580890, 1);
    }
    data_00580890 = savedState;
    return statement;
}

ENode *sub_47c050(ENode *node, struct CleanNode *args, Boolean flag)
{
    Object *obj;
    Object *obj2;
    Type *type;
    if (flag) {
        if (IS_TYPE_CLASS(node->rtype))
            CError_ASSERT(723, !CClass_Destructor((TypeClass *)node->rtype));
        type = node->rtype;
        obj = CParser_NewLocalDataObject(NULL, 1);
        obj->name = CParser_GetUniqueName();
        obj->type = type;
        CFunc_SetupLocalVarInfo(obj);
        obj2 = obj;
        node = makediadicnode(create_objectnode(obj), node, 0x1e);
    }
    node = append_cleannode_dtors(node, args);
    if (flag) {
        node = makediadicnode(node, create_objectnode(obj2), 0x29);
        node->rtype = obj2->type;
    }
    return node;
}

ENode *append_cleannode_dtors(ENode *left, struct CleanNode *list)
{
    ENode *right;
    Object *dtor;

    right = CABI_DestroyObject((dtor = list->dtor), create_objectrefnode(list->object), 1, 1, 0);
    left = makediadicnode(left, right, ECOMMA);
    left->rtype = &stvoid;
    if (list->next != NULL)
        left = append_cleannode_dtors(left, list->next);
    return left;
}

static inline FuncArg *fn_0047c620_inline1(long v7, int v0)
{
    int v8;
    FuncArg *v9;
    v8 = v7;
    v9 = (FuncArg *)v0;
    while ((int)v9 != 0) {
        if ((HashNameNode *)v8 == v9->name) {
            return v9;
        }
        v9 = v9->next;
    }
    return NULL;
}

static FuncArg *FindNameLink(FuncArg *list, HashNameNode *name)
{
    FuncArg *p;

    for (p = list; p != NULL; p = p->next)
        if (name == p->name)
            return p;
    return NULL;
}

ENode *create_temp_node2(Type *type)
{
    ENode *result;

    if (data_0058757c != NULL) {
        return data_0058757c(type, 1);
    }
    result = CExpr2_NewESCOPEBEGINNode(type, 0);
    result->data.temp.needs_dtor = 1;
    return result;
}

ENode *create_temp_node(Type *type)
{
    ENode *result;
    Object *object;
    if (data_0058757c != NULL) {
        return data_0058757c(type, 0);
    }
    result = CExpr2_NewESCOPEBEGINNode(type, 0);
    if (type->type == TYPECLASS) {
        object = CClass_Destructor((TypeClass *)type);
        if (object != NULL) {
            result->data.temp.needs_dtor = 1;
        }
    }
    return result;
}

Object *create_temp_object(Type *type)
{
    Object *object;
    object = CParser_NewLocalDataObject(NULL, 1);
    object->name = CParser_GetUniqueName();
    object->type = type;
    object->u.var.info = CPrep_AllocateVarInfo();
    object->u.var.info->func = data_00588238;
    if (object->sclass == TK_REGISTER) {
        if (copts.uniformSpillBlockWeight == 0)
            object->u.var.info->usage = 100;
        else
            object->u.var.info->usage = 5;
    }
    if (object->type && is_volatile_object(object))
        object->u.var.info->noregister = 1;
    return object;
}

void CheckCLabels(void)
{
    CLabel *label;

    label = clabels;
    if (clabels != NULL) {
        do {
            if (label->target.stmt == NULL) {
                CError_ReportError(ERR_UNDEFINED_LABEL, label->name->name);
            }
            label = label->next;
        } while (label != NULL);
    }
}

/* A new statement of kind TYPE after AFTER, at its source position and with its exception actions. */
Statement *CFunc_InsertAfterStatement(int type, Statement *after)
{
    Statement *stmt;
    stmt = (Statement *)CompilerTools_AllocatePool(26U);
    stmt->next = after->next;
    after->next = stmt;
    stmt->type = type;
    stmt->value = after->value;
    stmt->flags = 0;
    stmt->sourceoffset = after->sourceoffset;
    stmt->dobjstack = after->dobjstack;
    return stmt;
}

Statement *CFunc_AppendStatement(int kind)
{
    Statement *record;
    unsigned short value8;
    unsigned int value22;
    struct CException *value18;
    record = (Statement *)CompilerTools_AllocatePool(26U);
    record->next = NULL;
    record->type = (unsigned char)kind;
    value8 = current_statement_number;
    record->value = value8;
    record->flags = 0;
    value22 = statement_sourceoffset;
    record->sourceoffset = value22;
    value18 = UINT_00587fc4;
    record->dobjstack = value18;
    PTR_00587644->next = record;
    PTR_00587644 = record;
    return record;
}

CLabel *newlabel(void)
{
    CLabel *label;
    label = (CLabel *)CompilerTools_AllocatePool(20U);
    memclrw(label, 20U);
    label->uniquename = CParser_GetUniqueName();
    label->name = label->uniquename;
    return label;
}

CLabel *findlabel(void)
{
    CLabel *label;

    for (label = clabels; label != NULL; label = label->next) {
        if (data_00587fa0 == label->name) {
            return label;
        }
    }
    return NULL;
}

FuncArg *parameter_type_list(DeclInfo *state)
{
    FuncArg *args;
    NameSpace *scope;
    unsigned char savedParameterList;

    state->hasParameterNames = 0;
    state->oldStyleParameters = 0;
    state->parameterScope = NULL;
    if (tk == TK_ELLIPSIS || isdeclaration(0, 0, 0, 0)) {
        if (!copts.cplusplus) {
            scope = CScope_NewListNameSpace(NULL, 0);
            scope->parent = currentNameSpace;
            currentNameSpace = scope;
            savedParameterList = in_parameter_type_list;
            in_parameter_type_list = 1;
            args = ((FuncArg * (*)(DeclInfo *)) parse_func_args)(state);
            in_parameter_type_list = savedParameterList;
            currentNameSpace = scope->parent;
            if (!CScope_IsEmptyNameSpace(scope))
                state->parameterScope = scope;
        } else {
            args = ((FuncArg * (*)(DeclInfo *)) parse_func_args)(state);
        }
    } else if (copts.cplusplus) {
        args = NULL;
        if (tk != ')')
            CError_ReportError(127U);
    } else {
        parse_old_style_parameter_names(state);
        args = &data_00584748;
    }
    return args;
}

UInt8 CFunc_ParseFakeArgList(char stop_at_comma)
{
    DeclInfo declaration_data;

    if (tk == TK_ELLIPSIS) {
        return 1;
    }
    while (TRUE) {
        memclrw(&declaration_data, sizeof(declaration_data));
        CParser_GetDeclSpecs(&declaration_data, 0);
        if (declaration_data.missingTypeSpecifier != 0) {
            return 0;
        }
        CDecl_ParseDeclarator(&declaration_data);
        if (tk == '=') {
            tk = CPrepTokenizer_GetNextToken();
            assignment_expression();
        }
        switch (tk) {
            case TK_ELLIPSIS:
                return 1;
            case ',':
                if (stop_at_comma == 0) {
                    tk = CPrepTokenizer_GetNextToken();
                } else {
                    return 1;
                }
                break;
            default:
                return 0;
        }
        if (tk == TK_ELLIPSIS) {
            return 1;
        }
    }
}

unsigned int parse_func_args(int parameter)
{
    char isArray;
    FuncArg *tail;
    short storageClass;
    HashNameNode *name;
    FuncArg *arg;
    FuncArg *existing;
    FuncArg *args;
    FuncArg *last;
    char first;
    DeclInfo decl;
    args = NULL;
    first = 1;
    for (;;) {
        if (tk == TK_ELLIPSIS) {
            if (first == 0 && (tail = args) != NULL) {
                while (tail->next != NULL) {
                    tail = tail->next;
                }
                tail->next = &data_00583098;
            } else {
                args = &data_00583098;
            }
            tk = CPrepTokenizer_GetNextToken();
            return (unsigned int)args;
        }
        memclrw(&decl, sizeof(decl));
        CParser_GetDeclSpecs(&decl, 0);
        if (decl.missingTypeSpecifier != 0) {
            CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
        }
        if ((storageClass = decl.storage) != 0 && (int)storageClass != 257 &&
            (copts.cplusplus == 0 || (int)storageClass != 256)) {
            CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
            decl.storage = 0;
        }
        decl.name = NULL;
        CDecl_ParseDeclarator(&decl);
        if (first != 0) {
            first = 0;
            if (decl.dtype == &stvoid) {
                if (decl.storage != 0 || decl.qual != 0 || decl.name != NULL) {
                    CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
                }
                return 0;
            }
        }
        isArray = decl.dtype->type == TYPEARRAY;
        switch ((signed char)decl.dtype->type) {
            case TYPECLASS:
                if (((TypeClass *)decl.dtype)->sominfo != NULL) {
                    CError_ReportError(ERR_FUNCTIONS_CANNOT_SOM_CLASS_ARGUMENTS);
                    decl.dtype = (Type *)&stsignedint;
                }
                if (CDecl_CheckObjectType(decl.dtype) == 0) {
                    decl.dtype = (Type *)&stsignedint;
                }
                break;
            case TYPEFUNC:
                CDecl_WrapTypePointer(&decl.dtype, 0);
                break;
            case TYPEARRAY:
                decl.dtype = CDecl_NewPointerType(decl.dtype->array[0].element);
                break;
            default:
                if (CDecl_CheckObjectType(decl.dtype) == 0) {
                    decl.dtype = (Type *)&stsignedint;
                }
                break;
        }
        if ((name = decl.name) != NULL) {
            if (args != NULL) {
                existing = fn_0047c620_inline1((long)name, (int)args);
                if (existing != NULL) {
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, name->name);
                }
            }
        } else {
            decl.name = unnamed_name;
        }
        if (decl.dtype == &stvoid) {
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
        }
        arg = CParser_NewFuncArg();
        arg->name = decl.name;
        arg->type = decl.dtype;
        arg->qual = decl.qual;
        arg->sclass = decl.storage;
        arg->is_array = isArray;
        if ((last = args) != NULL) {
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = arg;
        } else {
            args = arg;
        }
        if (copts.cplusplus != 0 && tk == '=') {
            tk = CPrepTokenizer_GetNextToken();
            arg->dexpr = CFunc_DefaultArg(arg->type, arg->qual, args);
        }
        if (tk != ',') {
            if (tk == TK_ELLIPSIS && copts.cplusplus != 0) {
                continue;
            }
            return (unsigned int)args;
        }
        tk = CPrepTokenizer_GetNextToken();
    }
}

ENode *CFunc_DefaultArg(Type *destination, SInt32 flags, FuncArg *value)
{
    BufferedToken *record;
    ENode *expr;
    ENode *statement;

    DAT_00587fd8 = check_default_argument_reference;
    default_arg = value;
    expr = conv_assignment_expression();
    DAT_00587fd8 = NULL;

    if (CTemplTool_IsTypeDepExpr(expr) == 0 && CTemplateTools_IsDependentType(destination) == 0) {
        expr = CExpr_AssignmentPromotion(expr, destination, flags, 1);
    } else {
        record = CPrep_GetLastBufferedToken();
        if (record != NULL && record->tokenfile != NULL) {
            statement = CExpr2_NewENEWEXCEPTIONARRAYNode(ST_IFGOTO);
            statement->data.defaultargument.expression = expr;
            statement->data.defaultargument.sourcePosition = galloc(0x18);
            *statement->data.defaultargument.sourcePosition = *record;
            expr = statement;
        }
    }
    return fn_00513040(expr, 1);
}

Boolean check_default_argument_reference(int value, Object *object)
{
    FuncArg *entry;

    if ((value != 0) && (entry = default_arg, default_arg != NULL)) {
        do {
            if ((int)entry->name == value) {
                CError_ReportError(ERR_ILLEGAL_DEFAULT_ARGUMENTS);
                return 0;
            }
            entry = entry->next;
        } while (entry != NULL);
    }
    if ((object != NULL) && (object->datatype == DLOCAL)) {
        CError_ReportError(ERR_ILLEGAL_DEFAULT_ARGUMENTS);
        return 0;
    }
    return 1;
}

void parse_old_style_parameter_names(DeclInfo *scope)
{
    FuncArg *link;
    FuncArg *q;

    scope->oldStyleParameters = 1;
    for (;;) {
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        } else {
            link = FindNameLink(scope->parameterNames, data_00587fa0);
            if (link != NULL)
                CError_ReportError(ERR_IDENTIFIER_REDECLARED, data_00587fa0->name);
            link = CParser_NewFuncArg();
            link->name = data_00587fa0;
            if (scope->parameterNames != NULL) {
                q = scope->parameterNames;
                while (q->next != NULL)
                    q = q->next;
                q->next = link;
            } else {
                scope->parameterNames = link;
            }
            scope->hasParameterNames = 1;
        }
        tk = (SInt16)CPrepTokenizer_GetNextToken();
        if (tk != ',')
            return;
        tk = (SInt16)CPrepTokenizer_GetNextToken();
    }
}

void fn_0047ca70(Type **pt)
{
    Type *t;
    t = *pt;
    switch ((SInt8)t->type) {
        case TYPECLASS:
            if (TYPE_CLASS(*pt)->sominfo != NULL) {
                CError_ReportError(ERR_FUNCTIONS_CANNOT_SOM_CLASS_ARGUMENTS);
                *pt = (Type *)&stsignedint;
            }
            break;
        case TYPEFUNC: {
            Type **typeptr = pt;
            CDecl_WrapTypePointer(typeptr, 0);
            return;
        }
        case TYPEARRAY:
            *pt = CDecl_NewPointerType(TPTR_TARGET(*pt));
            return;
    }
    if (!CDecl_CheckObjectType(*pt)) {
        *pt = (Type *)&stsignedint;
    }
}

void CFunc_SetupLocalVarInfo(Object *object)
{
    object->u.var.info = CPrep_AllocateVarInfo();
    object->u.var.info->func = data_00588238;
    if (object->sclass == 257U) {
        if (copts.uniformSpillBlockWeight == 0U)
            object->u.var.info->usage = 100;
        else
            object->u.var.info->usage = 5;
    }
    if (object->type != NULL && is_volatile_object(object) != 0U)
        object->u.var.info->noregister = 1;
}

void PPCError_RestoreGlobalValues(const struct SavedGlobalValues *values)
{
    currentNameSpace = (NameSpace *)values->savedNameSpace;
    UINT_00587fc4 = (struct CException *)values->savedException;
}

struct SavedGlobalValues *fn_0047cb60(void)
{
    SavedGlobalValues *node;
    SavedGlobalValues *tail;
    struct NameSpace *obj;

    node = (SavedGlobalValues *)CompilerTools_AllocatePool(0xe);
    if (PTR_00580870 != NULL) {
        tail = saved_global_values_tail;
        tail->next = node;
        saved_global_values_tail = node;
    } else {
        saved_global_values_tail = PTR_00580870 = node;
    }
    node->index = data_00580878++;
    node->savedNameSpace = currentNameSpace;
    node->savedException = UINT_00587fc4;
    obj = CScope_NewListNameSpace(NULL, 0);
    obj->parent = currentNameSpace;
    currentNameSpace = obj;
    return node;
}
