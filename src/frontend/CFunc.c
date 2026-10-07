#define CERROR_FILE "CFunc.c"
#include "compiler/common.h"
#include "compiler/CFunc.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CABI.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CInit.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/FuncLevelAsmPPC.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include <string.h>
#include <stdio.h>

#pragma options align = mac68k
static void *data_00580870;
static struct DeclBlock *saved_global_values_tail;
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

static FuncArg *FindNameLink(FuncArg *list, HashNameNode *name);
static Statement *allocate_and_append_statement(UInt8 type);
static Statement *NewStmt(UInt8 type);
static void CondJump(ENode *expr, CLabel *truelabel, char a, char b);
static Statement *AppendStmt(unsigned char kind);
static CLabel *FindLabel(void);
static unsigned char IsLabel(void);
static CLabel *NewLabel(void);
static void *NewScope(void);
static struct Statement *CFunc_NewAssignmentStatement(void);
static void CFunc_InitVariableInfo(Object *obj);

inline void RestoreBlock(void *block)
{
    cscope_current = ((struct DeclBlock *)block)->parent_nspace;
    cexcept_dobjstack = ((struct DeclBlock *)block)->dobjstack;
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

static inline char use_legacy_condition_scope(void)
{
    return copts.ARMscoping;
}

static inline char warn_missing_return_value(void)
{
    return copts.extended_errorcheck;
}

static inline Boolean warn_empty_control_statement(void)
{
    return copts.warn_possunwant;
}

static inline Statement *CFunc_NewStatement(UInt8 kind)
{
    Statement *stmt = lalloc(sizeof(Statement));
    stmt->next = NULL;
    stmt->type = kind;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = cexcept_dobjstack;
    data_00587644->next = stmt;
    data_00587644 = stmt;
    return stmt;
}

static inline Statement *MakeCaseStatement(void)
{
    Statement *statement = lalloc(0x1a);
    statement->next = NULL;
    statement->type = ST_LABEL;
    statement->value = current_statement_number;
    statement->flags = 0;
    statement->sourceoffset = statement_sourceoffset;
    statement->dobjstack = cexcept_dobjstack;
    data_00587644->next = statement;
    data_00587644 = statement;
    return statement;
}

static inline Boolean CFunc_ScopeContains(ExceptionAction *from, ExceptionAction *to)
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
    ExceptionAction *to = dest->dobjstack;

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
    ExceptionAction *from, *p;

    from = stmt->dobjstack;
    for (p = dest->dobjstack; p; p = p->next)
        if (p == from)
            return 0;
    for (from = stmt->dobjstack, p = dest->dobjstack; from && from != p; from = from->next)
        if (CExcept_ActionNeedsDestruction(from))
            return 1;
    return 0;
}

static inline Boolean CFunc_AnyCleanup(ExceptionAction *p)
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
static inline ExceptionAction *CFunc_CommonScope(Statement *stmt, ExceptionAction *to)
{
    ExceptionAction *p;

    while (to) {
        for (p = stmt->dobjstack; p; p = p->next)
            if (p->next == to)
                return p;
        to = to->next;
    }
    return NULL;
}

static inline Statement *CFunc_EmitCleanups(Statement *stmt, ExceptionAction *p, ExceptionAction *stop)
{
    for (; p; p = p->next) {
        stmt = CExcept_ActionCleanup(p, stmt);
        if (p == stop)
            break;
    }
    return stmt;
}

static inline Statement *CFunc_EmitAllCleanups(Statement *stmt, ExceptionAction *p)
{
    for (; p; p = p->next) {
        stmt = CExcept_ActionCleanup(p, stmt);
        if (!p)
            break;
    }
    return stmt;
}

static inline Statement *CFunc_0047b880_inline1(char type, Statement *after)
{
    Statement *stmt;
    stmt = (Statement *)lalloc(26);
    stmt->next = after->next;
    after->next = stmt;
    stmt->type = type;
    stmt->value = after->value;
    stmt->flags = 0;
    stmt->sourceoffset = after->sourceoffset;
    stmt->dobjstack = after->dobjstack;
    return stmt;
}

static inline void FindStatementReference(Statement *statement, ExceptionAction **candidate, ExceptionAction **match)
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

static inline char CFunc_0047b9a0_inline1(Statement *a1)
{
    ExceptionAction *v12;
    ExceptionAction *v12s;
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

static inline Statement *CFunc_0047b9a0_inline3(ExceptionAction *node, Statement *carry, ExceptionAction *stop)
{
    ExceptionAction *p;
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
    inserted = (Statement *)lalloc(sizeof(Statement));
    inserted->next = statement->next;
    statement->next = inserted;
    inserted->type = ST_EXPRESSION;
    inserted->value = statement->value;
    inserted->flags = 0;
    inserted->sourceoffset = statement->sourceoffset;
    inserted->dobjstack = statement->dobjstack;
    return inserted;
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

struct DeclBlock *CFunc_NewDeclBlock(void)
{
    DeclBlock *node;
    DeclBlock *tail;
    struct NameSpace *obj;

    node = (DeclBlock *)lalloc(0xe);
    if (data_00580870 != NULL) {
        tail = saved_global_values_tail;
        tail->next = node;
        saved_global_values_tail = node;
    } else {
        saved_global_values_tail = data_00580870 = node;
    }
    node->index = data_00580878++;
    node->parent_nspace = cscope_current;
    node->dobjstack = cexcept_dobjstack;
    obj = CScope_NewListNameSpace(NULL, 0);
    obj->parent = cscope_current;
    cscope_current = obj;
    return node;
}

void CFunc_RestoreBlock(const struct DeclBlock *values)
{
    cscope_current = (NameSpace *)values->parent_nspace;
    cexcept_dobjstack = (struct ExceptionAction *)values->dobjstack;
}

#pragma auto_inline off
void CFunc_SetupLocalVarInfo(Object *object)
{
    object->u.var.info = CPrep_AllocateVarInfo();
    object->u.var.info->func = cscope_currentfunc;
    if (object->sclass == 257U) {
        if (copts.optimizesize == 0U)
            object->u.var.info->usage = 100;
        else
            object->u.var.info->usage = 5;
    }
    if (object->type != NULL && is_volatile_object(object) != 0U)
        object->u.var.info->noregister = 1;
}
#pragma auto_inline reset

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
            makethetypepointer(typeptr, 0);
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

ENode *CFunc_DefaultArg(Type *destination, SInt32 flags, FuncArg *value)
{
    TStreamElement *record;
    ENode *expr;
    ENode *statement;

    data_00587fd8 = check_default_argument_reference;
    default_arg = value;
    expr = conv_assignment_expression();
    data_00587fd8 = NULL;

    if (CTemplTool_IsTemplateArgumentDependentExpression(expr) == 0 &&
        CTemplTool_IsTemplateArgumentDependentType(destination) == 0) {
        expr = argumentpromotion(expr, destination, flags, 1);
    } else {
        record = CPrep_GetLastBufferedToken();
        if (record != NULL && record->tokenfile != NULL) {
            statement = CExpr_NewTemplDepENode(TDE_SOURCEREF);
            statement->data.templdep.u.sourceref.expr = expr;
            statement->data.templdep.u.sourceref.token = galloc(sizeof(TStreamElement));
            *statement->data.templdep.u.sourceref.token = *record;
            expr = statement;
        }
    }
    return fn_00513040(expr, 1);
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
                tail->next = &elipsis;
            } else {
                args = &elipsis;
            }
            tk = CPrepTokenizer_GetNextToken();
            return (unsigned int)args;
        }
        memclrw(&decl, sizeof(decl));
        CParser_GetDeclSpecs(&decl, 0);
        if (decl.missingTypeSpecifier != 0) {
            CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
        }
        if ((storageClass = decl.storageclass) != 0 && (int)storageClass != 257 &&
            (copts.cplusplus == 0 || (int)storageClass != 256)) {
            CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
            decl.storageclass = 0;
        }
        decl.name = NULL;
        scandeclarator(&decl);
        if (first != 0) {
            first = 0;
            if (decl.thetype == &stvoid) {
                if (decl.storageclass != 0 || decl.qual != 0 || decl.name != NULL) {
                    CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
                }
                return 0;
            }
        }
        isArray = decl.thetype->type == TYPEARRAY;
        switch ((signed char)decl.thetype->type) {
            case TYPECLASS:
                if (((TypeClass *)decl.thetype)->sominfo != NULL) {
                    CError_ReportError(ERR_FUNCTIONS_CANNOT_SOM_CLASS_ARGUMENTS);
                    decl.thetype = (Type *)&stsignedint;
                }
                if (CDecl_CheckObjectType(decl.thetype) == 0) {
                    decl.thetype = (Type *)&stsignedint;
                }
                break;
            case TYPEFUNC:
                makethetypepointer(&decl.thetype, 0);
                break;
            case TYPEARRAY:
                decl.thetype = CDecl_NewPointerType(TPTR_TARGET(decl.thetype));
                break;
            default:
                if (CDecl_CheckObjectType(decl.thetype) == 0) {
                    decl.thetype = (Type *)&stsignedint;
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
        if (decl.thetype == &stvoid) {
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
        }
        arg = CParser_NewFuncArg();
        arg->name = decl.name;
        arg->type = decl.thetype;
        arg->qual = decl.qual;
        arg->sclass = decl.storageclass;
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
        scandeclarator(&declaration_data);
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
            scope->parent = cscope_current;
            cscope_current = scope;
            savedParameterList = in_parameter_type_list;
            in_parameter_type_list = 1;
            args = ((FuncArg * (*)(DeclInfo *)) parse_func_args)(state);
            in_parameter_type_list = savedParameterList;
            cscope_current = scope->parent;
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
        args = &oldstyle;
    }
    return args;
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

CLabel *newlabel(void)
{
    CLabel *label;
    label = (CLabel *)lalloc(20U);
    memclrw(label, 20U);
    label->uniquename = CParser_GetUniqueName();
    label->name = label->uniquename;
    return label;
}

Statement *CFunc_AppendStatement(int kind)
{
    Statement *record;
    unsigned short value8;
    unsigned int value22;
    struct ExceptionAction *value18;
    record = (Statement *)lalloc(26U);
    record->next = NULL;
    record->type = (unsigned char)kind;
    value8 = current_statement_number;
    record->value = value8;
    record->flags = 0;
    value22 = statement_sourceoffset;
    record->sourceoffset = value22;
    value18 = cexcept_dobjstack;
    record->dobjstack = value18;
    data_00587644->next = record;
    data_00587644 = record;
    return record;
}

/* A new statement of kind TYPE after AFTER, at its source position and with its exception actions. */
Statement *CFunc_InsertAfterStatement(int type, Statement *after)
{
    Statement *stmt;
    stmt = (Statement *)lalloc(26U);
    stmt->next = after->next;
    after->next = stmt;
    stmt->type = type;
    stmt->value = after->value;
    stmt->flags = 0;
    stmt->sourceoffset = after->sourceoffset;
    stmt->dobjstack = after->dobjstack;
    return stmt;
}

void CheckCLabels(void)
{
    CLabel *label;

    label = clabels;
    if (clabels != NULL) {
        do {
            if (label->stmt == NULL) {
                CError_ReportError(ERR_UNDEFINED_LABEL, label->name->name);
            }
            label = label->next;
        } while (label != NULL);
    }
}

Object *create_temp_object(Type *type)
{
    Object *object;
    object = CParser_NewLocalDataObject(NULL, 1);
    object->name = CParser_GetUniqueName();
    object->type = type;
    object->u.var.info = CPrep_AllocateVarInfo();
    object->u.var.info->func = cscope_currentfunc;
    if (object->sclass == TK_REGISTER) {
        if (copts.optimizesize == 0)
            object->u.var.info->usage = 100;
        else
            object->u.var.info->usage = 5;
    }
    if (object->type && is_volatile_object(object))
        object->u.var.info->noregister = 1;
    return object;
}

ENode *create_temp_node(Type *type)
{
    ENode *result;
    Object *object;
    if (data_0058757c != NULL) {
        return data_0058757c(type, 0);
    }
    result = CExpr_NewETEMPNode(type, 0);
    if (type->type == TYPECLASS) {
        object = CClass_Destructor((TypeClass *)type);
        if (object != NULL) {
            result->data.temp.needs_dtor = 1;
        }
    }
    return result;
}

ENode *create_temp_node2(Type *type)
{
    ENode *result;

    if (data_0058757c != NULL) {
        return data_0058757c(type, 1);
    }
    result = CExpr_NewETEMPNode(type, 0);
    result->data.temp.needs_dtor = 1;
    return result;
}

static FuncArg *FindNameLink(FuncArg *list, HashNameNode *name)
{
    FuncArg *p;

    for (p = list; p != NULL; p = p->next)
        if (name == p->name)
            return p;
    return NULL;
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

#pragma auto_inline off
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
#pragma auto_inline reset

#pragma auto_inline off
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
#pragma auto_inline reset

#pragma auto_inline off
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
#pragma auto_inline reset

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

ENode *sub_47bca0(ENode *node)
{
    switch (node->type) {
        case ETEMP: {
            Object *n = CException_GetTempObject(node);
            if (node->data.temp.needs_dtor) {
                CleanNode *r = (CleanNode *)lalloc(12);
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
        case ENULLCHECK:
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
        case EPRECOMP:
        case ELABEL:
        case EINSTRUCTION:
        case EVECTOR128CONST:
            return node;
        default:
            CError_FATAL(923);
            return node;
    }
}

void CFunc_WarnUnused(void)
{
    ObjectList *local;
    ObjectList *argument;

    if (copts.warn_unusedvar) {
        for (local = locals; local; local = local->next) {
            if (!(local->object->flags & 1) && !CParser_IsNullOrAtOrDollarPrefixedName(local->object->name) &&
                !(local->object->qual & Q_INLINE_DATA)) {
                CError_SetBufferedToken(&local->object->u.var.info->deftoken);
                CError_Warning(ERR_VARIABLE_ARGUMENT_NOT_USED_FUNCTION, local->object->name->name);
            }
        }
    }
    if (copts.warn_unusedarg) {
        for (argument = arguments; argument; argument = argument->next) {
            if (!(argument->object->flags & 1) && !CParser_IsNullOrAtOrDollarPrefixedName(argument->object->name) &&
                argument->object->name != this_arg_name && argument->object->name != this_self_name) {
                CError_SetBufferedToken(&declaration_token);
                CError_Warning(ERR_VARIABLE_ARGUMENT_NOT_USED_FUNCTION, argument->object->name->name);
            }
        }
    }
}

/* reference prototype (types may differ here): extern void CFunc_CodeCleanup(Statement *stmt); */
void CFunc_CodeCleanup(Statement *stmt)
{
    if ((cscope_currentclass != NULL) && (cscope_currentclass->sominfo != NULL)) {
        CSOM_GenerateSomselfAssignment(cscope_currentclass, stmt);
    }
    CFunc_WarnUnused();
    CExcept_ExceptionTansform(stmt);
}

void fn_0047b9a0(Statement *statement, Statement *expression)
{
    ExceptionAction *targetScope;
    Object *temporary;
    ExceptionAction *scope;
    char needsCleanup;
    int originalScope;
    ExceptionAction *currentScope;
    char needsTemporary;
    Statement *inserted;
    Statement *direct;
    ExceptionAction *savedScope;
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
            scope = (ExceptionAction *)CFunc_0047b9a0_inline2(statement, expression);
            savedScope = statement->dobjstack;
            statement = CFunc_0047b9a0_inline3(savedScope, statement, scope);
        }
    }
    needsTemporary = CFunc_0047b9a0_inline1(expression);
    if (needsTemporary != 0) {
        if (CMach_GetFunctionResultClass((TypeFunc *)cscope_currentfunc->type) != 1) {
            value = expression->expr->rtype;
            temporary = CParser_NewLocalDataObject(NULL, 1);
            temporary->name = (HashNameNode *)CParser_GetUniqueName();
            temporary->type = value;
            CFunc_SetupLocalVarInfo(temporary);
            inserted = CFunc_InsertStatement(statement);
            inserted->expr = makediadicnode(create_objectnode(temporary), expression->expr, 30);
            inserted->sourceoffset = expression->sourceoffset;
            CFunc_0047b9a0_inline3(expression->dobjstack, inserted, NULL);
            expression->expr = create_objectnode(temporary);
        } else {
            direct = CFunc_InsertStatement(statement);
            direct->expr = expression->expr;
            direct->sourceoffset = expression->sourceoffset;
            CFunc_0047b9a0_inline3(expression->dobjstack, direct, NULL);
            expression->expr = nullnode();
        }
    }
}

Statement *insert_conditional_goto_cleanup(Statement *statement)
{
    ExceptionAction *reference;
    ExceptionAction *candidate;
    Statement *tail;
    ExceptionAction *match;
    Statement *target;
    Statement *jump;
    CLabel *data;

    if (statement->type == ST_IFGOTO) {
        statement->type = ST_IFNGOTO;
    } else {
        statement->type = ST_IFGOTO;
    }
    data = (CLabel *)lalloc(sizeof(*data));
    memclrw(data, sizeof(*data));
    data->name = data->uniquename = CParser_GetUniqueName();
    candidate = statement->label->stmt->dobjstack;
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
    jump->label = statement->label;
    target = CFunc_0047b880_inline1(ST_LABEL, jump);
    target->label = data;
    data->stmt = target;
    statement->label = data;
    target->dobjstack = statement->dobjstack;
    return target;
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
                CFunc_CheckJump(next, ((SwitchInfo *)next->label)->defaultlabel->stmt);
                for (c = ((SwitchInfo *)next->label)->cases; c; c = c->next)
                    CFunc_CheckJump(next, c->label->stmt);
                break;
            case ST_GOTO:
            case ST_IFGOTO:
            case ST_IFNGOTO:
                CFunc_CheckJump(next, next->label->stmt);
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
                if (stmt->dobjstack != next->label->stmt->dobjstack && CFunc_NeedsCleanup(stmt, next->label->stmt))
                    CFunc_EmitCleanups(stmt, stmt->dobjstack, CFunc_CommonScope(stmt, next->label->stmt->dobjstack));
                stmt = next;
                break;
            case ST_RETURN:
                if (next->expr && CFunc_AnyCleanup(stmt->dobjstack))
                    fn_0047b9a0(stmt, next);
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
                        if (next->dobjstack != next->label->stmt->dobjstack &&
                            CFunc_NeedsCleanup(next, next->label->stmt))
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

void parse_case_statement(struct StatementContext *context)
{
    SwitchCase *entry;
    CLabel *label;
    Statement *statement;
    SwitchCase *newCase;
    CInt64 value;

    if (context->switchinfo == NULL) {
        CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
        return;
    }
    tk = CPrepTokenizer_GetNextToken();
    value =
        CExpr_IntConstConvert(context->switchinfo->sizetype, context->switchinfo->sizetype, CExpr_IntegralConstExpr());
    for (entry = context->switchinfo->cases; entry != NULL; entry = entry->next) {
        if (CInt64_GreaterEqual(entry->min, value) && CInt64_LessEqual(entry->min, value)) {
            CError_ReportError(ERR_CASE_CONSTANT_DEFINED_MORE_THAN_ONCE);
        }
    }
    statement = MakeCaseStatement();
    label = lalloc(sizeof(*label));
    memclrw(label, sizeof(*label));
    label->uniquename = CParser_GetUniqueName();
    label->name = label->uniquename;
    statement->label = label;
    statement->label->stmt = statement;
    newCase = lalloc(sizeof(*newCase));
    newCase->min = value;
    newCase->label = statement->label;
    newCase->next = context->switchinfo->cases;
    context->switchinfo->cases = newCase;
    if (tk != ':') {
        CError_ReportErrorAndUpdateToken(ERR_COLON_EXPECTED);
    } else {
        tk = CPrepTokenizer_GetNextToken();
    }
    parse_statement(context);
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
        if (cscope_currentfunc == NULL || (cscope_currentfunc->qual & Q_INLINE) == 0 ||
            CParser_HasInternalLinkage(cscope_currentfunc) != 0) {
            obj->name = CParser_AppendUniqueName("init");
        } else {
            sprintf(buf, "$localstatic%ld$", local_name_counter++);
            obj->name = CParser_NameConcat(
                "init", CParser_NameConcat(buf, COptimizer_GetFunctionObject(cscope_currentfunc)->name)->name);
            obj->qual |= Q_IMPLICIT_WEAK;
            obj->sclass = TK_EOF;
        }
        CInit_DeclareData(localstatic_init_guard, NULL, NULL, localstatic_init_guard->type->size);
        info = lalloc(sizeof(CLabel));
        memclrw(info, sizeof(CLabel));
        info->uniquename = CParser_GetUniqueName();
        info->name = info->uniquename;
        data_0058087e = info;
        stmt = CFunc_NewStatement(6);
        stmt->expr = create_objectnode(localstatic_init_guard);
        stmt->label = data_0058087e;
    }
    stmt = CFunc_NewStatement(4);
    stmt->expr = expr;
}

void append_or_defer_expression_statement(ENode *node)
{
    struct Statement *record;
    if (data_00580882 != 0) {
        if (deferred_expression != NULL) {
            record = (struct Statement *)lalloc(sizeof(struct Statement));
            record->next = NULL;
            record->type = 4U;
            record->value = current_statement_number;
            record->flags = 0U;
            record->sourceoffset = statement_sourceoffset;
            record->dobjstack = cexcept_dobjstack;
            data_00587644->next = record;
            data_00587644 = record;
            record->expr = deferred_expression;
        }
        deferred_expression = (ENode *)node;
    } else {
        record = (struct Statement *)lalloc(sizeof(struct Statement));
        record->next = NULL;
        record->type = 4U;
        record->value = current_statement_number;
        record->flags = 0U;
        record->sourceoffset = statement_sourceoffset;
        record->dobjstack = cexcept_dobjstack;
        data_00587644->next = record;
        data_00587644 = record;
        record->expr = (ENode *)node;
    }
}

void register_destructor_object(Type *type, Object *object, long offset, long flags)
{
    CExcept_RegisterDestructorObject(object, offset, CClass_Destructor((TypeClass *)type), flags);
}

void declare_local_object(DeclInfo *declaration, TStreamElement *declarationToken, char isParameter,
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
    CDecl_CompleteType(declaration->thetype);
    existing = NULL;
    objects = CScope_FindName(cscope_current, declaration->name);
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

    if (declaration->storageclass == STORAGE_EXTERN) {
        found = NULL;
        scope = CScope_FindGlobalNS(cscope_current);
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
            if (iscpp_typeequal(declaration->thetype, found->type) == 0 ||
                (declaration->qual & (Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK)) !=
                    (found->qual & (Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK)))
                CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, declaration->name->name, found->type,
                                   found->qual, declaration->thetype, declaration->qual);
        } else {
            found = CParser_NewGlobalDataObject(declaration);
            found->nspace = scope;
        }
        CParser_NewAliasObject(found, 0);
        return;
    }

    if (declaration->storageclass != STORAGE_STATIC)
        object = CParser_NewObject(declaration);
    else
        object = CParser_NewGlobalDataObject(declaration);
    object->name = declaration->name;
    object->type = declaration->thetype;
    object->qual = declaration->qual;
    object->sclass = declaration->storageclass;

    switch (declaration->storageclass) {
        case STORAGE_STATIC:
            if (isParameter != 0) {
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                break;
            }
            if (forbidInitialization != 0)
                CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            if (CDecl_CheckObjectType(declaration->thetype) == 0)
                break;
            CError_ReportIllegalFlags(declaration->qual &
                                      ~(Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK));
            CParser_NewAliasObject(object, 0);
            object->nspace = cscope_root;
            object->datatype = DDATA;
            name = object->name->name;
            if (cscope_currentfunc == NULL || (cscope_currentfunc->qual & Q_INLINE) == 0 ||
                CParser_HasInternalLinkage(cscope_currentfunc) != 0) {
                object->name = CParser_AppendUniqueName(name);
            } else {
                sprintf(nameBuffer, "$localstatic%ld$", local_name_counter++);
                nameSuffix = COptimizer_GetFunctionObject(cscope_currentfunc)->name;
                object->name = CParser_NameConcat(name, CParser_NameConcat(nameBuffer, nameSuffix)->name);
                object->qual |= Q_IMPLICIT_WEAK;
                object->sclass = TK_EOF;
            }
            if (copts.cplusplus != 0) {
                localstatic_init_guard = NULL;
                CInit_InitializeStaticData(object, append_localstatic_init_expr);
                if (localstatic_init_guard != NULL) {
                    statement = lalloc(sizeof(Statement));
                    statement->next = NULL;
                    statement->type = ST_EXPRESSION;
                    statement->value = current_statement_number;
                    statement->flags = 0;
                    statement->sourceoffset = statement_sourceoffset;
                    statement->dobjstack = cexcept_dobjstack, data_00587644->next = statement;
                    data_00587644 = statement;
                    statement->expr = makediadicnode(create_objectnode(localstatic_init_guard),
                                                     intconstnode((Type *)&stsignedchar, 1), EASS);
                    statement = allocate_and_append_statement(2);
                    statement->label = data_0058087e;
                    data_0058087e->stmt = statement;
                }
            } else {
                CInit_InitializeData(object);
            }
            break;
        case 0:
        case TK_AUTO:
        case TK_REGISTER: {
            if (CDecl_CheckObjectType(declaration->thetype) != 0) {
                CError_ReportIllegalFlags(declaration->qual & ~(Q_CV | Q_PASCAL | Q_ALIGNED_MASK));
                object->datatype = DLOCAL;
                object->u.var.info = CPrep_AllocateVarInfo();
                object->u.var.info->func = cscope_currentfunc;
                if (object->sclass == TK_REGISTER) {
                    if (copts.optimizesize == 0)
                        object->u.var.info->usage = 100;
                    else
                        object->u.var.info->usage = 5;
                }
                if (object->type != NULL && is_volatile_object(object) != 0)
                    object->u.var.info->noregister = 1;
                object->u.var.info->deftoken = *declarationToken;
                if (object->sclass == TK_REGISTER && isParameter != 0)
                    object->u.var.info->usage = 100;
                CScope_AddObject(cscope_current, object->name, (ObjBase *)object);
                if (isParameter == 0) {
                    if (declaration->thetype->type == TYPECLASS && TYPE_CLASS(declaration->thetype)->sominfo != NULL) {
                        fn_004e4390(object);
                    } else {
                        CInit_InitializeAutoData(object, append_or_defer_expression_statement,
                                                 register_destructor_object);
                        if (object->type != declaration->thetype) {
                            if (object->type->type == TYPESTRUCT || object->type->type == TYPECLASS) {
                                CError_ASSERT(1675, cscope_current->is_hash == 0);
                                entry = CScope_FindNameSpaceName(cscope_current, object->name);
                                CError_ASSERT(1676, entry != 0);
                                CError_ASSERT(1677, entry->first.object == (ObjBase *)object);
                                CError_ASSERT(1678, entry->first.next == 0);
                                entry->name = CParser_AppendUniqueName(object->name->name);
                                found = CParser_NewAliasObject(object, 0);
                                found->type = declaration->thetype;
                            }
                        }
                    }
                }
                if (object->datatype == DLOCAL) {
                    local = lalloc(sizeof(ObjectList));
                    local->object = object;
                    local->next = locals;
                    locals = local;
                }
                (void)CanAllocObject(declaration->thetype);
            }
            break;
        }
        default:
            CError_FATAL(1701);
    }
}

static Statement *allocate_and_append_statement(UInt8 type)
{
    Statement *stmt = (Statement *)lalloc(0x1a);
    stmt->next = NULL;
    stmt->type = type;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = cexcept_dobjstack;
    data_00587644->next = stmt;
    data_00587644 = stmt;
    return stmt;
}

ENode *parse_declarations(char mode, int singleDeclaration, char allowEmpty, char stopAfterDeclaration)
{
    DeclInfo declarationState;
    TStreamElement snapshot;
    Type *baseType;
    UInt32 baseQualifiers;

    deferred_expression = NULL;
    data_00580882 = singleDeclaration;
    while ((char)singleDeclaration != 0 || isdeclaration(copts.cplusplus, 0, 0, 0)) {
        statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
        memclrw(&declarationState, sizeof(declarationState));
        declarationState.requireMangledName = data_0058088c;
        CParser_GetDeclSpecs(&declarationState, 0);
        if (declarationState.thetype->type == TYPETEMPLATE) {
            CError_ReportError(ERR_ILLEGAL_TYPE);
            declarationState.thetype = (Type *)&stsignedint;
        }
        baseType = declarationState.thetype;
        baseQualifiers = declarationState.qual;
        if (tk != ';') {
            for (;;) {
                snapshot = *CPrep_GetLastBufferedToken();
                declarationState.name = NULL;
                scandeclarator(&declarationState);
                if (declarationState.name != NULL) {
                    if (declarationState.storageclass != TK_TYPEDEF) {
                        if (declarationState.thetype->type == TYPEFUNC) {
                            if (CDecl_FunctionDeclarator(&declarationState, CScope_FindGlobalNS(cscope_current), 0,
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
                declarationState.thetype = baseType;
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

void generate_conditional_jump(ENode *expr, CLabel *dest, CLabel *other, Boolean sense, Boolean flag)
{
    CLabel *label;
    ENode *operand;
    Statement *stmt;

    if (expr == NULL) {
        if (sense) {
            NewStmt(3)->label = dest;
            return;
        }
        return;
    }
    if (expr->type == ETYPCON && expr->rtype->type == TYPEINT && expr->data.monadic->rtype->type == TYPEINT &&
        expr->rtype->size >= expr->data.monadic->rtype->size)
        expr = expr->data.monadic;
    if (isnotzero(expr)) {
        if (sense) {
            NewStmt(3)->label = dest;
            return;
        }
        return;
    }
    if (iszero(expr)) {
        if (!sense) {
            NewStmt(3)->label = dest;
            return;
        }
        return;
    }
    if (expr->type == ELOGNOT) {
        generate_conditional_jump(expr->data.diadic.left, dest, other, !sense, flag);
        return;
    }
    if (expr->type == ELOR) {
        label = (CLabel *)lalloc(sizeof(CLabel));
        memclrw(label, sizeof(CLabel));
        label->uniquename = CParser_GetUniqueName();
        label->name = label->uniquename;
        if (sense) {
            generate_conditional_jump(expr->data.diadic.left, dest, label, 1, flag);
            label->stmt = (Statement *)NewStmt(2);
            label->stmt->label = label;
            generate_conditional_jump(expr->data.diadic.right, dest, other, 1, flag);
            return;
        }
        generate_conditional_jump(expr->data.diadic.left, other, label, 1, flag);
        label->stmt = (Statement *)NewStmt(2);
        label->stmt->label = label;
        generate_conditional_jump(expr->data.diadic.right, dest, other, 0, flag);
        return;
    }
    if (expr->type == ELAND) {
        label = (CLabel *)lalloc(sizeof(CLabel));
        memclrw(label, sizeof(CLabel));
        label->uniquename = CParser_GetUniqueName();
        label->name = label->uniquename;
        if (sense) {
            generate_conditional_jump(expr->data.diadic.left, other, label, 0, flag);
            label->stmt = (Statement *)NewStmt(2);
            label->stmt->label = label;
            generate_conditional_jump(expr->data.diadic.right, dest, other, 1, flag);
            return;
        }
        generate_conditional_jump(expr->data.diadic.left, dest, label, 0, flag);
        label->stmt = (Statement *)NewStmt(2);
        label->stmt->label = label;
        generate_conditional_jump(expr->data.diadic.right, dest, other, 0, flag);
        return;
    }
    stmt = NewStmt(6);
    stmt->label = dest;
    stmt->expr = expr;
    if (!sense)
        stmt->type = ST_IFNGOTO;
    if (flag)
        stmt->flags |= 4;
}

static Statement *NewStmt(UInt8 type)
{
    Statement *stmt = (Statement *)lalloc(sizeof(Statement));

    stmt->next = NULL;
    stmt->type = type;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = cexcept_dobjstack;
    data_00587644->next = stmt;
    data_00587644 = stmt;
    return stmt;
}

ENode *initialize_argument_object(ENode *initData, Type *type, UInt32 flags)
{
    ObjectList *argument;
    Object *object;
    ENode *node;
    ENode *local;
    ENodeList *info;

    for (argument = arguments; argument != NULL; argument = argument->next) {
        if (argument->object->name == blank_argument_name)
            break;
    }
    if (argument == NULL)
        CError_FATAL(1958);
    object = argument->object;

    node = CExpr_IsTempConstruction(initData, type, &local);
    if (node != NULL && local->type == ETEMP) {
        *local = *create_objectnode(object);
        return node;
    }

    if (type->type == TYPECLASS) {
        node = create_objectnode(object);
        info = lalloc(sizeof(ENodeList));
        info->next = NULL;
        info->node = initData;
        return CExpr_ConstructObject(type, node, info, 1, 1, 0, 1, 0);
    }

    node = makemonadicnode(create_objectnode(object), EINDIRECT);
    node->rtype = type;
    return makediadicnode(node, oldassignmentpromotion(initData, type, flags, 1), EASS);
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
            case ETEMP:
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

void parse_statement(StatementContext *context)
{
    DeclBlock *ifScope;
    CLabel *endLabel;
    DeclBlock *whileScope;
    HashNameNode *namespaceName;
    Statement *jumpStmt;
    DeclBlock *forScope;
    CLabel *conditionLabel;
    ENode *expr;
    ENode *conditionExpr;
    DeclBlock *bodyScope;
    DeclBlock *newScope;
    Statement *stmt;
    DeclBlock *switchScope;
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
            if (((context->thetype == &stvoid && !copts.cplusplus) || CClass_IsDestructor(cscope_currentfunc)) ||
                CClass_HasTypeFuncFlag16384(cscope_currentfunc)) {
                if (tk != ';') {
                    CError_ReportError(ERR_ILLEGAL_RETURN_VALUE_VOID_CONSTRUCTOR_DESTRUCTOR);
                    s_expression();
                }
                stmt = AppendStmt(8);
                stmt->expr = NULL;
                fn_00449d60();
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            if (tk == ';') {
                if (context->thetype != &stvoid && (warn_missing_return_value() || copts.cplusplus))
                    CError_Warning(ERR_RETURN_VALUE_EXPECTED);
                stmt = AppendStmt(8);
                stmt->expr = NULL;
                fn_00449d60();
                tk = CPrepTokenizer_GetNextToken();
                return;
            }
            expr = s_expression();
            if (context->thetype == &stvoid) {
                if (expr->rtype != &stvoid)
                    CError_ReportError(ERR_ILLEGAL_RETURN_VALUE_VOID_CONSTRUCTOR_DESTRUCTOR);
                stmt = AppendStmt(4);
                stmt->expr = expr;
                stmt = AppendStmt(8);
                stmt->expr = NULL;
            } else {
                if (CMach_GetFunctionResultClass((TypeFunc *)cscope_currentfunc->type) == 1)
                    expr = initialize_argument_object(expr, context->thetype, context->qual);
                else
                    expr = oldassignmentpromotion(expr, context->thetype, context->qual, 1);
                stmt = AppendStmt(8);
                stmt->expr = expr;
                if (context->thetype->type == TYPEPOINTER)
                    check_function_result_automatic_variable(expr);
            }
            break;
        case TK_CASE:
            parse_case_statement(context);
            return;
        case TK_DEFAULT:
            if (!context->switchinfo) {
                CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
                return;
            }
            if (CPrepTokenizer_GetNextToken() != ':')
                CError_ReportErrorAndUpdateToken(ERR_COLON_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (context->switchinfo->defaultlabel)
                CError_ReportErrorAndUpdateToken(ERR_DEFAULT_LABEL_DEFINED_MORE_THAN_ONCE);
            stmt = AppendStmt(2);
            stmt->label = NewLabel();
            stmt->label->stmt = stmt;
            context->switchinfo->defaultlabel = stmt->label;
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
                if (CScope_IsEmptyNameSpace(cscope_current)) {
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
            stmt->expr = integralpromote(expr);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            stmt->label = (CLabel *)lalloc(sizeof(SwitchInfo));
            ((SwitchInfo *)stmt->label)->defaultlabel = NULL;
            ((SwitchInfo *)stmt->label)->cases = NULL;
            ((SwitchInfo *)stmt->label)->sizetype = stmt->expr->rtype;
            breakLabel = NewLabel();
            bodyContext = *context;
            bodyContext.switchinfo = (SwitchInfo *)stmt->label;
            bodyContext.loopBreak = breakLabel;
            ScopedBody(&bodyContext);
            if (!bodyContext.switchinfo->defaultlabel)
                bodyContext.switchinfo->defaultlabel = breakLabel;
            if (!bodyContext.switchinfo->cases) {
                stmt->type = ST_EXPRESSION;
                jumpStmt = (Statement *)lalloc(sizeof(Statement));
                *jumpStmt = *stmt;
                stmt->next = jumpStmt;
                jumpStmt->type = ST_GOTO;
                jumpStmt->label = bodyContext.switchinfo->defaultlabel;
                jumpStmt->dobjstack = cexcept_dobjstack;
            }
            stmt = AppendStmt(2);
            stmt->label = breakLabel;
            breakLabel->stmt = stmt;
            if (switchScope)
                RestoreBlock(switchScope);
            return;
        case 0x13f:
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER) {
                if (tk == '*' && !copts.ANSIstrict) {
                    tk = CPrepTokenizer_GetNextToken();
                    stmt = AppendStmt(0xf);
                    stmt->expr = s_expression();
                    if (stmt->expr->rtype->type != TYPEPOINTER) {
                        CError_ReportError(ERR_ILLEGAL_TYPE);
                        stmt->expr = nullnode();
                        stmt->expr->rtype = (Type *)&void_ptr;
                    }
                    break;
                }
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return;
            }
            stmt = AppendStmt(3);
            stmt->label = FindLabel();
            if (!stmt->label) {
                stmt->label = NewLabel();
                stmt->label->next = clabels;
                clabels = stmt->label;
                stmt->label->name = data_00587fa0;
            }
            tk = CPrepTokenizer_GetNextToken();
            break;
        case TK_BREAK:
            if (context->loopBreak) {
                stmt = AppendStmt(3);
                stmt->label = context->loopBreak;
            } else
                CError_ReportError(ERR_ILLEGAL_USE_KEYWORD);
            tk = CPrepTokenizer_GetNextToken();
            break;
        case TK_CONTINUE:
            if (context->loopContinue) {
                stmt = AppendStmt(3);
                stmt->label = context->loopContinue;
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
                    if (forScope && CScope_IsEmptyNameSpace(cscope_current)) {
                        RestoreBlock(forScope);
                        forScope = NULL;
                    }
                }
                if (expr) {
                    stmt = AppendStmt(4);
                    stmt->expr = expr;
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
                    if (CScope_IsEmptyNameSpace(cscope_current)) {
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
                data_00588523 = 0;
                tk = CPrepTokenizer_GetNextToken();
                if (tk == ';' && !data_00588523)
                    CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
            } else
                tk = CPrepTokenizer_GetNextToken();
            if (testExpr) {
                stmt = AppendStmt(3);
                stmt->label = NewLabel();
                conditionLabel = stmt->label;
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
            stmt->label = NewLabel();
            (topLabel = stmt->label)->stmt = stmt;
            breakLabel = NewLabel();
            continueLabel = NewLabel();
            bodyContext = *context;
            bodyContext.loopContinue = continueLabel;
            bodyContext.loopBreak = breakLabel;
            if (tk != '{') {
                bodyScope = NewScope();
                ScopedBody(&bodyContext);
                RestoreBlock(bodyScope);
            } else
                ScopedBody(&bodyContext);
            stmt = AppendStmt(2);
            stmt->label = continueLabel;
            continueLabel->stmt = stmt;
            if (stepExpr) {
                stmt = AppendStmt(4);
                stmt->expr = stepExpr;
            }
            stmt = AppendStmt(2);
            stmt->label = conditionLabel;
            conditionLabel->stmt = stmt;
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
            stmt->label = breakLabel;
            breakLabel->stmt = stmt;
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
            stmt->label = NewLabel();
            (topLabel = stmt->label)->stmt = stmt;
            continueLabel = NewLabel();
            breakLabel = NewLabel();
            bodyContext = *context;
            bodyContext.loopContinue = continueLabel;
            bodyContext.loopBreak = breakLabel;
            tk = CPrepTokenizer_GetNextToken();
            ScopedBody(&bodyContext);
            stmt = AppendStmt(2);
            stmt->label = continueLabel;
            continueLabel->stmt = stmt;
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
            stmt->label = breakLabel;
            breakLabel->stmt = stmt;
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
                if (CScope_IsEmptyNameSpace(cscope_current)) {
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
                    data_00588523 = 0;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';' && !data_00588523)
                        CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
                } else
                    tk = CPrepTokenizer_GetNextToken();
            }
            stmt = AppendStmt(3);
            stmt->label = NewLabel();
            continueLabel = stmt->label;
            if (current_statement_number >= 0x1000) {
                if (current_statement_number >= 0xf000)
                    current_statement_number++;
                else
                    current_statement_number += 0x1000;
            } else
                current_statement_number <<= 3;
            stmt = AppendStmt(2);
            stmt->label = NewLabel();
            (topLabel = stmt->label)->stmt = stmt;
            breakLabel = NewLabel();
            bodyContext = *context;
            bodyContext.loopContinue = continueLabel;
            bodyContext.loopBreak = breakLabel;
            ScopedBody(&bodyContext);
            stmt = AppendStmt(2);
            stmt->label = continueLabel;
            continueLabel->stmt = stmt;
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
            stmt->label = breakLabel;
            breakLabel->stmt = stmt;
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
                if (CScope_IsEmptyNameSpace(cscope_current)) {
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
                    data_00588523 = 0;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';' && !data_00588523)
                        CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
                } else
                    tk = CPrepTokenizer_GetNextToken();
            }
            ScopedBody(context);
            if (tk == TK_ELSE) {
                if (warn_empty_control_statement()) {
                    data_00588523 = 0;
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == ';' && !data_00588523)
                        CError_Warning(ERR_POSSIBLE_UNWANTED_SEMICOLON);
                } else
                    tk = CPrepTokenizer_GetNextToken();
                stmt = AppendStmt(3);
                stmt->label = NewLabel();
                endLabel = stmt->label;
                stmt = AppendStmt(2);
                stmt->label = continueLabel;
                continueLabel->stmt = stmt;
                ScopedBody(context);
            }
            stmt = AppendStmt(2);
            stmt->label = endLabel;
            endLabel->stmt = stmt;
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
            if (copts.cplusplus || !copts.ANSIstrict) {
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
                CScope_ParseUsingDirective(cscope_current);
            } else
                CScope_ParseUsingDeclaration(cscope_current, ACCESSPUBLIC, 0);
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
                if ((stmt->label = FindLabel()) != NULL) {
                    if (stmt->label->stmt)
                        CError_ReportError(ERR_LABEL_REDEFINED, data_00587fa0->name);
                } else {
                    stmt->label = NewLabel();
                    stmt->label->next = clabels;
                    clabels = stmt->label;
                    stmt->label->name = data_00587fa0;
                }
                stmt->label->stmt = stmt;
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
            stmt->expr = s_expression();
            CExpr_CheckUnusedExpression(stmt->expr);
            break;
    }
    if (tk == ';') {
        CPrep_ResetBufferedTokenPosition();
        tk = CPrepTokenizer_GetNextToken();
        fn_00449d60();
    } else
        CError_ReportErrorAndUpdateToken(ERR_SEMICOLON_EXPECTED);
}

static void CondJump(ENode *expr, CLabel *truelabel, char a, char b)
{
    CLabel *label = NewLabel();
    generate_conditional_jump(expr, truelabel, label, a, b);
    label->stmt = (Statement *)AppendStmt(2); /* AppendStmt statement view */
    ((Statement *)label->stmt)->label = label;
}

static Statement *AppendStmt(unsigned char kind)
{
    Statement *s = (Statement *)lalloc(0x1a);
    s->next = NULL;
    s->type = kind;
    s->value = current_statement_number;
    s->flags = 0;
    s->sourceoffset = statement_sourceoffset;
    s->dobjstack = cexcept_dobjstack;
    data_00587644->next = s;
    data_00587644 = s;
    return s;
}

static CLabel *FindLabel(void)
{
    CLabel *label;
    for (label = clabels; label; label = label->next)
        if ((HashNameNode *)data_00587fa0 == label->name)
            return label;
    return NULL;
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

static CLabel *NewLabel(void)
{
    CLabel *label = (CLabel *)lalloc(0x14);
    memclrw(label, 0x14);
    label->uniquename = CParser_GetUniqueName();
    label->name = label->uniquename;
    return label;
}

void CFunc_CompoundStatement(struct StatementContext *context)
{
    struct NameSpaceLookupList *scope = NewScope();

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

static void *NewScope(void)
{
    struct DeclBlock *s = lalloc(0xe);
    NameSpace *ns;
    if (data_00580870) {
        saved_global_values_tail->next = s;
        saved_global_values_tail = s;
    } else {
        saved_global_values_tail = data_00580870 = s;
    }
    s->index = data_00580878++;
    s->parent_nspace = cscope_current;
    s->dobjstack = cexcept_dobjstack;
    ns = CScope_NewListNameSpace(NULL, 0);
    ns->parent = (NameSpace *)cscope_current;
    cscope_current = ns;
    return s;
}

void create_local_object_copy(Object *func, TypeIntegral *type, Type *type2, Boolean flag)
{
    Object *newfunc;
    NameSpaceObjectList *list;
    ObjectList *listnode;
    struct Statement *stmt;
    ENode *expr;

    newfunc = (Object *)lalloc(sizeof(Object));
    *newfunc = *func;
    newfunc->type = type2;
    CFunc_InitVariableInfo(newfunc);
    if (newfunc->sclass == TK_REGISTER) {
        if (copts.optimizesize == 0)
            newfunc->u.var.info->usage = 100;
        else
            newfunc->u.var.info->usage = 5;
    }
    if (newfunc->type != NULL && is_volatile_object(newfunc))
        newfunc->u.var.info->noregister = 1;
    func->name = CParser_GetUniqueName();
    func->type = flag ? CDecl_NewPointerType((Type *)type) : (Type *)type;
    list = CScope_FindName(cscope_current, newfunc->name);
    if (list == NULL || list->object != (ObjBase *)func)
        CError_FATAL(2577);
    list->object = (ObjBase *)newfunc;
    listnode = (ObjectList *)lalloc(sizeof(ObjectList));
    listnode->object = newfunc;
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
        expr = promote(expr, type2);
    stmt->expr = makediadicnode(create_objectnode(newfunc), expr, EASS);
}

static struct Statement *CFunc_NewAssignmentStatement(void)
{
    struct Statement *stmt;
    stmt = (struct Statement *)lalloc(sizeof(struct Statement));
    stmt->next = NULL;
    stmt->type = ST_EXPRESSION;
    stmt->value = current_statement_number;
    stmt->flags = 0;
    stmt->sourceoffset = statement_sourceoffset;
    stmt->dobjstack = cexcept_dobjstack;
    data_00587644->next = stmt;
    data_00587644 = stmt;
    return stmt;
}

static void CFunc_InitVariableInfo(Object *obj)
{
    obj->u.var.info = CPrep_AllocateVarInfo();
    obj->u.var.info->func = cscope_currentfunc;
}

void CFunc_SetupNewFuncArgs(Object *func, FuncArg *args)
{
    Object *obj;
    ObjectList *arglist;

    arguments = NULL;
    if (args != &elipsis && args != &oldstyle) {
        for (arglist = NULL; args != NULL && args != &elipsis; args = args->next) {
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
            obj->u.var.info->func = cscope_currentfunc;
            if (obj->sclass == TK_REGISTER) {
                if (copts.optimizesize == 0)
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
            if (obj->name == unnamed_name && copts.ANSIstrict != 0 && copts.cplusplus == 0 &&
                (func->qual & Q_MANGLE_NAME) == 0)
                CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
            if (arglist != NULL) {
                arglist->next = (ObjectList *)lalloc(sizeof(ObjectList));
                arglist = arglist->next;
            } else {
                arglist = (ObjectList *)lalloc(sizeof(ObjectList));
                arguments = arglist;
            }
            arglist->next = NULL;
            arglist->object = obj;
        }
    }
}

/* Builds a list of parameter Objects from a FuncArg
 * chain; each Object also gets a VarInfo. */
ObjectList *create_arg_object_list(FuncArg *arg)
{
    ObjectList *res = NULL;
    ObjectList *cur;
    Object *obj;

    while (arg != NULL) {
        if (res != NULL) {
            cur = cur->next = (ObjectList *)lalloc(8);
        } else {
            res = cur = (ObjectList *)lalloc(8);
        }
        obj = CParser_NewLocalDataObject(NULL, 0);
        obj->name = arg->name;
        obj->type = arg->type;
        obj->qual = arg->qual;
        obj->sclass = arg->sclass;
        obj->u.var.info = CPrep_AllocateVarInfo();
        obj->u.var.info->func = cscope_currentfunc;
        if (obj->sclass == TK_REGISTER) {
            if (copts.optimizesize == 0) {
                obj->u.var.info->usage = 100;
            } else {
                obj->u.var.info->usage = 5;
            }
        }
        if (obj->type != NULL && is_volatile_object(obj)) {
            obj->u.var.info->noregister = 1;
        }
        cur->object = obj;
        cur->next = NULL;
        arg = arg->next;
    }
    return res;
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
                declarationType = declaration.thetype;
                if ((short)declaration.storageclass != 0 && (short)declaration.storageclass != 257) {
                    CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
                }
                for (;;) {
                    declaration.thetype = declarationType;
                    declaration.name = NULL;
                    scandeclarator(&declaration);
                    if (declaration.name == NULL) {
                        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                        break;
                    }
                    switch ((signed char)declaration.thetype->type) {
                        case TYPECLASS:
                            if (((TypeClass *)declaration.thetype)->sominfo != NULL) {
                                CError_ReportError(ERR_FUNCTIONS_CANNOT_SOM_CLASS_ARGUMENTS);
                                declaration.thetype = (Type *)&stsignedint;
                            }
                            if (CDecl_CheckObjectType(declaration.thetype) == 0) {
                                declaration.thetype = (Type *)&stsignedint;
                            }
                            break;
                        case TYPEFUNC:
                            makethetypepointer(&declaration.thetype, 0);
                            break;
                        case TYPEARRAY:
                            declaration.thetype = CDecl_NewPointerType(((TypePointer *)declaration.thetype)->target);
                            break;
                        default:
                            if (CDecl_CheckObjectType(declaration.thetype) == 0) {
                                declaration.thetype = (Type *)&stsignedint;
                            }
                            break;
                    }
                    CanAllocObject(declaration.thetype);
                    candidate = arguments;
                    name = declaration.name;
                    while (candidate != NULL) {
                        if (name == (parameter = candidate->object)->name) {
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
                        parameter->type = declaration.thetype;
                        parameter->sclass = declaration.storageclass;
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
                if (argument->object->type == NULL) {
                    argument->object->type = (Type *)&stsignedint;
                }
                argument = argument->next;
            }
        } else {
            CFunc_SetupNewFuncArgs(function, ((TypeFunc *)function->type)->args);
        }
    }
    if (CMach_GetFunctionResultClass((TypeFunc *)function->type) == 1) {
        result = CParser_NewLocalDataObject(NULL, 0);
        result->name = blank_argument_name;
        result->type = (Type *)CDecl_NewPointerType(((TypeFunc *)function->type)->functype);
        result->u.var.info = CPrep_AllocateVarInfo();
        result->u.var.info->func = cscope_currentfunc;
        if (result->sclass == TK_REGISTER) {
            if (copts.optimizesize == 0) {
                result->u.var.info->usage = 100;
            } else {
                result->u.var.info->usage = 5;
            }
        }
        if (result->type != NULL && is_volatile_object(result) != 0) {
            result->u.var.info->noregister = 1;
        }
        newEntry = (ObjectList *)lalloc(sizeof(ObjectList));
        newEntry->object = result;
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
        CScope_InsertNameSpaceName(cscope_current, entry->object->name)->object = (ObjBase *)entry->object;
        entry = entry->next;
    }
}

NameSpace *CFunc_FuncGenSetup(Statement *stmt, Object *func)
{
    NameSpace *scope;
    struct DeclBlock *node;

    scope = CScope_NewListNameSpace(NULL, 0);
    scope->parent = cscope_current;
    cscope_current = scope;
    arguments = NULL;
    locals = NULL;
    clabels = NULL;
    next_varnumber = 0;
    local_name_counter = 0;
    CExcept_Setup();
    memclrw(stmt, sizeof(*stmt));
    data_00587644 = stmt;
    stmt->type = ST_NOP;
    current_statement_number = 1;
    stmt->value = *(UInt16 *)&current_statement_number;
    data_00580878 = 0;
    node = lalloc(offsetof(struct DeclBlock, index) + sizeof(node->index));
    memclrw(node, offsetof(struct DeclBlock, index) + sizeof(node->index));
    node->index = data_00580878++;
    node->parent_nspace = cscope_current;
    saved_global_values_tail = data_00580870 = node;
    return scope;
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

    fn_0050ee60(context, object, options);

    if (flag) {
        CClass_DefineCovariantFuncs(object, &buf);
    }
}

/* A linked list of member initializer records: next, a discriminator byte at
 * 0x04 (2) and the member variable pointer at 0x0a. */

void fn_00476e70(TypeClass *theclass, struct CtorChain *inits)
{
    ObjMemberVar *member;
    CtorChain *node;

    if (theclass->mode == 1)
        return;
    member = theclass->ivars;
    while (member != NULL) {
        if (member->type->type == TYPEPOINTER && (TYPE_POINTER(member->type)->qual & Q_REFERENCE)) {
            node = inits;
            while (node != NULL) {
                if (node->what == 2 && node->u.membervar == member)
                    break;
                node = node->next;
            }
            if (node == NULL)
                CError_ReportError(ERR_AMPERSAND_REFERENCE_MEMBER_NOT_INITIALIZED, member->name->name);
        } else if (CParser_IsConst(member->type, member->qual)) {
            node = inits;
            while (node != NULL) {
                if (node->what == 2 && node->u.membervar == member)
                    break;
                node = node->next;
            }
            if (node == NULL && member->type->type != TYPECLASS)
                CError_ReportError(ERR_CONST_MEMBER_NOT_INITIALIZED, member->name->name);
        }
        member = member->next;
    }
}

void CFunc_CheckClassCtors(TypeClass *type)
{
    fn_00476e70(type, NULL);
}

void parse_ctor_initializers(void)
{
    Type *type;
    ClassList *base;
    TypeClass *cls;
    VClassList *vbase;
    ENodeList *args;
    CtorChain *entry;
    CtorChain *previous;
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
                for (base = cscope_currentclass->bases; base != NULL; base = base->next)
                    if (base->base->classname == data_00587fa0) {
                        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x28) {
                            cls = base->base;
                            tk = CPrepTokenizer_GetNextToken();
                        } else
                            data_00587fa0 = base->base->classname;
                        break;
                    }
                for (member = cscope_currentclass->ivars; member != NULL; member = member->next)
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
            for (vbase = cscope_currentclass->vbases; vbase != NULL; vbase = vbase->next)
                if (vbase->base == cls)
                    break;
            if (vbase != NULL) {
                for (previous = ctor_initializers; previous != NULL; previous = previous->next)
                    if (previous->what == 1 && previous->u.vbase == vbase) {
                        CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                        return;
                    }
                entry = lalloc(sizeof(CtorChain));
                entry->what = 1;
                entry->u.vbase = vbase;
            } else {
                for (base = cscope_currentclass->bases; base != NULL; base = base->next)
                    if (base->base == cls)
                        break;
                if (base != NULL) {
                    for (previous = ctor_initializers; previous != NULL; previous = previous->next)
                        if (previous->what == 0 && previous->u.base == base) {
                            CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                            return;
                        }
                    entry = lalloc(sizeof(CtorChain));
                    entry->what = 0;
                    entry->u.base = base;
                } else {
                    CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                    return;
                }
            }
        } else {
            for (member = cscope_currentclass->ivars; member != NULL; member = member->next)
                if (member->name == data_00587fa0)
                    break;
            if (member != NULL) {
            member_found:
                for (previous = ctor_initializers; previous != NULL; previous = previous->next)
                    if (previous->what == 2 && previous->u.membervar == member)
                        CError_ReportError(ERR_ILLEGAL_CTOR_INITIALIZER);
                entry = lalloc(sizeof(CtorChain));
                entry->what = 2;
                entry->u.membervar = member;
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
        switch (entry->what) {
            case 0:
                expr = CABI_MakeThisExpr(NULL, entry->u.base->offset);
                entry->objexpr = CExpr_ConstructObject(TYPE(entry->u.base->base), expr, args, 1, 0, 0, 0, 1);
                break;
            case 1:
                expr = CABI_MakeThisExpr(entry->u.vbase->base, entry->u.vbase->offset);
                entry->objexpr = CExpr_ConstructObject(TYPE(entry->u.vbase->base), expr, args, 1, 0, 0, 0, 1);
                break;
            case 2:
                expr = CABI_MakeThisExpr(cscope_currentclass, entry->u.membervar->offset);
                expr->flags = entry->u.membervar->qual & Q_CV;
                type = entry->u.membervar->type;
                switch ((SInt8)type->type) {
                    case TYPECLASS:
                        entry->objexpr = CExpr_ConstructObject(entry->u.membervar->type, expr, args, 1, 1, 0, 1, 1);
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
                            lhs->rtype = entry->u.membervar->type;
                            if (lhs->rtype->type == TYPEBITFIELD) {
                                bitfieldType = (TypeBitfield *)lhs->rtype;
                                lhs->data.monadic = makemonadicnode(lhs->data.monadic, EBITFIELD);
                                lhs->data.monadic->rtype = TYPE(bitfieldType);
                                lhs->rtype = bitfieldType->bitfieldtype;
                            }
                            entry->objexpr = makediadicnode(
                                lhs, oldassignmentpromotion(args->node, lhs->rtype, lhs->flags, 1), EASS);
                        } else
                            entry->objexpr = nullnode();
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
            cscope_current = scopeObject->nspace;
    } else {
        CScope_SetMethodScope(func, scopeObject, scopeFlag, &save);
    }
    if (scope != NULL)
        cscope_current = scope;
    if (cscope_currentclass != NULL)
        CClass_MemberDef(func, cscope_currentclass);
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
            if (argument->object->type->type == TYPEFLOAT && argument->object->type->size < stdouble.size)
                create_local_object_copy(argument->object, &stdouble, argument->object->type, 0);
            if (CMach_PassResultInHiddenArg(argument->object->type)) {
                argument->object->type = CDecl_NewPointerType(argument->object->type);
                TYPE_POINTER(argument->object->type)->qual = Q_REFERENCE;
            }
            argument = argument->next;
        }
    }
    if (definition->parameterScope != NULL)
        CScope_MergeNameSpace(cscope_current, definition->parameterScope);
    if (tk == TK_TRY) {
        tk = (UInt16)CPrepTokenizer_GetNextToken();
        functionTryBlock = 1;
    } else {
        functionTryBlock = 0;
    }
    if (CClass_IsDestructor(func)) {
        if (cscope_currentclass == NULL)
            CError_FATAL(3466);
        parse_ctor_initializers();
        fn_00476e70(cscope_currentclass, ctor_initializers);
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
        gen.switchinfo = NULL;
        gen.loopContinue = NULL;
        gen.loopBreak = NULL;
        gen.thetype = TYPE_FUNC(func->type)->functype;
        gen.qual = TYPE_FUNC(func->type)->qual;
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
        if (data_00587644->type != ST_RETURN && data_00587644->type != ST_GOTO) {
            previousStatement = data_00587644;
            statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);
            returnStatement = (Statement *)lalloc(sizeof(*returnStatement));
            returnStatement->next = NULL;
            returnStatement->type = ST_RETURN;
            returnStatement->value = current_statement_number;
            returnStatement->flags = 0;
            returnStatement->sourceoffset = statement_sourceoffset;
            returnStatement->dobjstack = cexcept_dobjstack;
            data_00587644->next = returnStatement;
            data_00587644 = returnStatement;
            returnStatement->dobjstack = NULL;
            data_00587644->expr = NULL;
            if (copts.cplusplus || copts.c9x) {
                if (memcmp(func->name->name, "main", 5) == 0 &&
                    &TYPE_FUNC(func->type)->functype->type == &stsignedint.type)
                    data_00587644->expr = intconstnode((Type *)&stsignedint, 0);
            }
            if (previousStatement->type == ST_EXPRESSION) {
                ENode *expression;
                if ((expression = previousStatement->expr)->type == EFUNCCALL && expression->rtype == &stvoid &&
                    (expression->flags & 2))
                    data_00587644->flags |= 8;
            }
        }
        for (label = clabels; label != NULL; label = label->next) {
            if (label->stmt == NULL)
                CError_ReportError(ERR_UNDEFINED_LABEL, label->name->name);
        }
        if (!func_errors) {
            if (CClass_IsDestructor(func))
                CABI_InsertConstructorInitialization(func, &state, cscope_currentclass, NULL, functionTryBlock);
            if (CClass_HasTypeFuncFlag16384(func))
                CABI_TransDestructor(func, func, &state, cscope_currentclass, 0);
            CFunc_DestructorCleanup(&state);
            if (cscope_currentclass != NULL && cscope_currentclass->sominfo != NULL)
                CSOM_GenerateSomselfAssignment(cscope_currentclass, &state);
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
            fn_0050ee60(&state, func, oldStyleArguments);
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

void InitExpr_Register(ENode *expr, Object *cls)
{
    PendingFunction *pending;
    PendingFunction *last;
    Object *func;
    if (cprep_cu[0xe0] == 1 && cls->sclass != TK_STATIC && (cls->qual & (Q_IMPLICIT_WEAK | Q_WEAK)) == 0) {
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);
        return;
    }
    if (copts.suppress_init_code != 0)
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
    if (cscope_currentclass != NULL && cscope_currentclass->sominfo != NULL) {
        CSOM_GenerateSomselfAssignment(cscope_currentclass, &statements);
    }
    CFunc_WarnUnused();
    CExcept_ExceptionTansform(&statements);
    if ((TYPE_FUNC(functionObject->type)->flags & 0x20000000) != 0 && anyerrors == '\0') {
        CInline_SaveInfo(&inlineState, &statements, functionObject);
        restoreInlineState = TRUE;
    } else {
        restoreInlineState = FALSE;
    }
    fn_0050ee60(&statements, functionObject, FALSE);
    if (restoreInlineState) {
        CClass_DefineCovariantFuncs(functionObject, &inlineState);
    }
    cscope_current = savedValue->parent;
    copts.filesyminfo = savedState;
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
        node = (Statement *)lalloc(0x1a);
        node->next = NULL;
        node->type = ST_EXPRESSION;
        node->value = current_statement_number;
        node->flags = 0;
        node->sourceoffset = statement_sourceoffset;
        node->dobjstack = cexcept_dobjstack;
        data_00587644->next = node;
        data_00587644 = node;
        node->expr = expr;
        if (cscope_currentclass != NULL && cscope_currentclass->sominfo != NULL)
            CSOM_GenerateSomselfAssignment(cscope_currentclass, &stmt);
        CFunc_WarnUnused();
        CExcept_ExceptionTansform(&stmt);
        fn_0050ee60(&stmt, func, 0);
        cscope_current = savedNamespace->parent;
        copts.filesyminfo = oldflag;
    }
}

static void SetLong(CInt64 *pN, long n)
{
    pN->lo = n;
    pN->hi = (n < 0) ? 0xFFFFFFFF : 0;
}
