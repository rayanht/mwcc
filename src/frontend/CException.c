#define CERROR_FILE "CException.c"
#include "compiler/common.h"
#include "compiler/CException.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
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
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include <string.h>
#include <stdio.h>
#include "compiler/ENode.h"

#pragma options align = mac68k
static UInt8 data_00581c30;
static struct TemporaryObject *temporary_object_list;
static struct Statement *data_00581c36;
static struct CException *currentDobjstack;
static struct CException *current_dobjstack;
#pragma options align = reset

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

static inline Boolean CException_HasThrow(Statement *s)
{
    data_00581c30 = 0;
    for (; s != NULL; s = s->next) {
        switch (s->type) {
            case ST_RETURN:
                if (s->expr.expression == NULL)
                    break;
                /* fallthrough */
            case ST_EXPRESSION:
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_GOTOEXPR:
                CExpr_SearchExprTree(s->expr.expression, fn_004e0b20, 2, 0x36, 0x37);
                if (data_00581c30 != 0)
                    return 1;
                break;
            case ST_NOP:
            case ST_LABEL:
            case ST_GOTO:
            case ST_ASM:
                break;
            default:
                CError_FATAL(2414);
        }
    }
    return 0;
}

void CExcept_ExceptionTansform(Statement *stmt)
{
    update_statement_dobjstacks(stmt);
    if (data_00588238 != NULL && copts.fb3 != 0 && TYPE_FUNC(data_00588238->type)->exspecs != NULL) {
        if (CException_HasThrow(stmt)) {
            setup_exception_specification(stmt, TYPE_FUNC(data_00588238->type)->exspecs);
        }
    }
}

static inline void reverse_one(ENodeList **arr, SInt32 i, ENodeList *found)
{
    if (arr[i] == found)
        return;
    {
        ENode *result = rewrite_expr_temporaries(arr[i]->node);
        arr[i]->node = result;
    }
}

/* Objects and initialization flags registered for temporary cleanup. */

static inline Statement *NewTemporaryStatement(void)
{
    Statement *statement = CFunc_InsertAfterStatement(4, data_00581c36);
    statement->sourceoffset = statement->next->sourceoffset;
    statement->dobjstack = currentDobjstack;
    return statement;
}

static inline Object *findTemporaryObject(SInt32 uniqueID, ENode *temporaryExpr)
{
    ECacheNode *cached;
    for (cached = cached_objects; cached != NULL; cached = cached->next) {
        if (cached->key == uniqueID)
            return cached->obj;
    }
    cached = galloc(sizeof(ECacheNode));
    cached->next = cached_objects;
    cached_objects = cached;
    cached->key = uniqueID;
    cached->obj = create_temp_object(temporaryExpr->data.temp.type);
    return cached->obj;
}

static TemporaryObject *CException_NewTypeNode(void)
{
    TemporaryObject *t = CompilerTools_AllocatePool(sizeof(TemporaryObject));
    t->next = temporary_object_list;
    temporary_object_list = t;
    return t;
}

static CException *CException_NewStmtNode(void)
{
    CException *s = CompilerTools_AllocatePool(sizeof(CException));
    s->next = currentDobjstack;
    currentDobjstack = s;
    return s;
}

static CException *CException_CopyStmtNode(void)
{
    CException *s = CompilerTools_AllocatePool(sizeof(CException));
    *s = *currentDobjstack;
    s->next = current_dobjstack;
    current_dobjstack = s;
    return s;
}

static inline Object *CException_CachedObject(ENode *node)
{
    SInt32 key;
    if ((key = node->data.temp.uniqueid)) {
        ECacheNode *cache = cached_objects;
        while (cache) {
            if (cache->key == key)
                return cache->obj;
            cache = cache->next;
        }
        cache = (ECacheNode *)galloc(sizeof(ECacheNode));
        cache->next = cached_objects;
        cached_objects = cache;
        cache->key = key;
        cache->obj = create_temp_object(node->data.temp.type);
        return cache->obj;
    }
    return create_temp_object(node->data.temp.type);
}

static Statement *InsertPrevStatement(int type)
{
    Statement *stmt = CFunc_InsertAfterStatement(type, data_00581c36);
    stmt->sourceoffset = stmt->next->sourceoffset;
    stmt->dobjstack = currentDobjstack;
    return stmt;
}

static inline SInt32 object_statement(Object *o)
{
    Statement *n = CFunc_AppendStatement(0xc);
    n->expr.expression = create_objectrefnode(o);
    return (SInt32)n;
}

static inline void finish_label(CLabel *p)
{
    Statement *n = CFunc_AppendStatement(2);
    n->target.label = p;
    p->target.stmt = (Statement *)n; /* exception statement view */
}

static Boolean CException_IsClassType(Type *ty)
{
    if (ty != NULL) {
        if (ty->type == TYPECLASS) {
            if (CClass_Destructor((TypeClass *)ty) != NULL)
                return 1;
            return 0;
        }
        if (ty->type == TYPEPOINTER) {
            if ((TYPE_POINTER(ty)->qual & Q_REFERENCE) != 0 && TPTR_TARGET(ty)->type == TYPECLASS)
                return 1;
            return 0;
        }
        return 0;
    }
    return 1;
}

/* Exception cleanup record for an object or array destructor. */

static inline ENode *CException_004e2c40_inline1(Object *p0, ENode *p1)
{
    ENode *v3;
    v3 = CABI_DestroyObject(p0, p1, 1, 1, 0);
    CE_ASSERT(v3->type != EFUNCCALL || v3->data.funccall.funcref->type != EOBJREF, CError_FATAL(609));
    if (v3->data.funccall.funcref->data.objref->datatype == DVFUNC) {
        v3->data.funccall.funcref->flags |= 128;
    }
    return v3;
}

static inline Statement *CException_004e2c40_inline2(Statement *p0, ENode *p1)
{
    Statement *t3;
    t3 = CFunc_InsertAfterStatement(4, p0);
    t3->expr.expression = p1;
    return t3;
}

static Object *CException_StdType(void *name)
{
    CScopeParseResult lookup;
    NameSpaceObjectList *obj;
    Object *type;
    obj = CScope_FindObjectList(&lookup, GetHashNameNode(name));
    if (obj == NULL || (type = (Object *)obj->object)->otype != 5 || type->datatype != DLOCAL) {
        CError_FATAL(502);
        type = NULL;
    }
    return type;
}

unsigned char fn_004e0ab0(Statement *node)
{
    data_00581c30 = 0;
    while (node != NULL) {
        switch (node->type) {
            case ST_NOP:
            case ST_LABEL:
            case ST_GOTO:
            case ST_ASM:
                break;
            case ST_RETURN:
                if (node->expr.expression == NULL) {
                    break;
                }
            case ST_EXPRESSION:
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_GOTOEXPR:
                CExpr_SearchExprTree(node->expr.expression, fn_004e0b20, 2U, 54U, 55U);
                if (data_00581c30 != 0) {
                    return 1;
                }
                break;
            default:
                CError_FATAL(2414);
                break;
        }
        node = node->next;
    }
    return 0;
}

void fn_004e0b20(ENode *expr)
{
    data_00581c30 = 1U;
    return;
}

/* Label metadata used by exception statements. */

/* Expression data used to describe an exception handler. */

void setup_exception_specification(struct Statement *statements, struct ExceptSpecList *handlers)
{
    CException *region;
    Statement *statement;
    Statement *label_statement;
    CException *tail;
    CLabel *end_label;
    CLabel *start_label;
    Object *object;
    ExceptSpecList *handler;
    SInt32 handler_count;
    SInt32 handler_index;
    CException *object_region;
    Statement *end_statement;
    Statement *assignment;
    ENode *expression;
    Statement *return_statement;

    region = CompilerTools_AllocatePool(sizeof(*region));
    memclrw(region, sizeof(*region));
    region->kind = 0x0f;
    for (statement = statements; statement != NULL; statement = statement->next) {
        if (statement->dobjstack != NULL) {
            tail = statement->dobjstack;
            do {
                if (tail->kind == 0x0f)
                    break;
                if (tail->next == NULL) {
                    tail->next = region;
                    break;
                }
                tail = tail->next;
            } while (1);
        } else {
            statement->dobjstack = region;
        }
    }
    for (statement = statements; statement->next != NULL; statement = statement->next)
        ;
    if (statement->type != ST_GOTO && statement->type != ST_RETURN) {
        statement = CFunc_InsertAfterStatement(8, statement);
        statement->expr.expression = NULL;
        statement->dobjstack = NULL;
        if (TYPE_FUNC(data_00588238->type)->functype != &stvoid && (copts.f9d != 0 || copts.cplusplus != 0)) {
            CError_Warning(ERR_RETURN_VALUE_EXPECTED);
        }
    }
    label_statement = CFunc_InsertAfterStatement(2, statement);
    start_label = newlabel();
    label_statement->target.label = start_label;
    label_statement->target.label->target.stmt = label_statement;
    label_statement->flags = 1;
    label_statement->dobjstack = NULL;
    object = create_temp_object(&exception_temp_object_type);
    if (handlers->type == NULL)
        handlers = NULL;
    handler_count = 0;
    handler = handlers;
    if (handler != NULL) {
        do {
            handler = handler->next;
            handler_count++;
        } while (handler != NULL);
    }
    region->data.specification.count = handler_count;
    region->data.specification.ids = galloc(handler_count * sizeof(*region->data.specification.ids));
    handler_index = 0;
    region->data.specification.label = label_statement->target.label;
    region->data.specification.info = object;
    for (handler = handlers; handler != NULL; handler = handler->next) {
        expression = create_type_stringconst(handler->type, handler->qual, 0);
        if (expression->type != ESTRINGCONST)
            CError_FATAL(1009);
        region->data.specification.ids[handler_index++] =
            CInit_DeclareString(expression->data.string.data, expression->data.string.size, 0, 0);
    }
    assignment = CFunc_InsertAfterStatement(4, label_statement);
    assignment->expr.expression = funccallexpr(DAT_00587654, create_objectrefnode(object), NULL, NULL, NULL);
    object_region = CompilerTools_AllocatePool(sizeof(*object_region));
    memclrw(object_region, sizeof(*object_region));
    object_region->kind = 0x0e;
    object_region->data.active_catch.info = object;
    object_region->data.active_catch.call_dtor = 1;
    assignment->dobjstack = object_region;
    end_statement = CFunc_InsertAfterStatement(2, assignment);
    end_label = newlabel();
    end_statement->target.label = end_label;
    end_statement->target.label->target.stmt = end_statement;
    end_statement->dobjstack = NULL;
    return_statement = CFunc_InsertAfterStatement(3, end_statement);
    return_statement->target.label = end_label;
}

void update_statement_dobjstacks(Statement *node)
{
    Statement *p;
    CException *info;
    ENode *e;
    ENodeList *args;

    p = data_00581c36 = node;
    while (p != NULL) {
        currentDobjstack = current_dobjstack = p->dobjstack;
        if ((p->flags & 2) != 0) {
            currentDobjstack = currentDobjstack->next;
        } else if (p->type == ST_EXPRESSION) {
            e = p->expr.expression;
            while (e->type == ECOMMA)
                e = e->data.diadic.left;
            if (e->type == EINDIRECT)
                e = e->data.monadic;
            if (e->type == EFUNCCALL) {
                if (e->data.funccall.funcref->type == EOBJREF &&
                    CClass_IsDestructor(e->data.funccall.funcref->data.objref) && (info = p->dobjstack) != NULL &&
                    info->kind == 1 && (args = e->data.funccall.args) != NULL && args->node->type == EOBJREF &&
                    args->node->data.objref == info->data.local.object) {
                    currentDobjstack = currentDobjstack->next;
                }
            }
        }
        switch (p->type) {
            case ST_EXPRESSION:
                insert_temporary_object_destruction(p, 1, 0);
                break;
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:

                insert_temporary_object_destruction(p, 0, 1);
                break;
            case ST_RETURN:
                if (p->expr.expression != NULL)
                    insert_temporary_object_destruction(
                        p, 0, CMachine_FunctionRequiresMemoryReturn((TypeFunc *)data_00588238->type) != 1);
                break;
        }
        p->dobjstack = currentDobjstack;
        data_00581c36 = p;
        p = p->next;
    }
}

void insert_temporary_object_destruction(Statement *statement, char flag1, char flag2)
{
    Object *object;
    ENode *expr;
    Statement *result;
    union {
        Statement statement;
        unsigned int words[7]; /* Raw statement storage, including unknown bytes. */
    } saved;
    temporary_object_list = NULL;
    statement->expr.expression = fn_004e1050(statement->expr.expression);
    if (temporary_object_list != NULL) {
        if (flag1 == 0) {
            if (flag2 != 0) {
                expr = statement->expr.expression;
                CE_ASSERT(statement->expr.expression->rtype->type == TYPECLASS &&
                              CClass_Destructor((TypeClass *)expr->rtype) != 0,
                          CError_FATAL(2194));
                object = create_temp_object(expr->rtype);
                statement->expr.expression = makediadicnode(create_objectnode(object), expr, 30);
            }
            saved.statement = *statement;
            statement->type = ST_EXPRESSION;
            statement = generate_temporary_object_destruction(statement);
            result = CFunc_InsertAfterStatement(saved.words[1], statement);
            result->target.label = saved.statement.target.label;
            if (flag2 != 0)
                result->expr.expression = create_objectnode(object);
            else
                result->expr.expression = nullnode();
        } else {
            generate_temporary_object_destruction(statement);
        }
    }
}

Statement *generate_temporary_object_destruction(Statement *arg)
{
    TemporaryObject *p = temporary_object_list;
    Statement *stmt = arg;
    CLabel *label;
    while (p != NULL) {
        if (current_dobjstack != NULL && (current_dobjstack->kind == 1 || current_dobjstack->kind == 2) &&
            current_dobjstack->data.local.object == p->object)
            current_dobjstack = current_dobjstack->next;
        else
            CError_FATAL(2151);
        if (p->initializationFlag != NULL) {
            stmt = CFunc_InsertAfterStatement(ST_IFNGOTO, stmt);
            stmt->expr.expression = create_objectnode(p->initializationFlag);
            stmt->dobjstack = current_dobjstack;
            stmt->target.label = newlabel();
            label = stmt->target.label;
        }
        stmt = CFunc_InsertAfterStatement(ST_EXPRESSION, stmt);
        stmt->expr.expression = CABI_DestroyObject(p->classObject, create_objectrefnode(p->object), 1, 1, 0);
        stmt->dobjstack = current_dobjstack;
        if (p->initializationFlag != NULL) {
            stmt = CFunc_InsertAfterStatement(ST_LABEL, stmt);
            stmt->target.label = label;
            label->target.stmt = stmt;
        }
        p = p->next;
    }
    return stmt;
}

/* Expression-node prefix used by exception rewriting. */

ENode *fn_004e1050(ENode *expression)
{
    switch (expression->type) {
        case EPRECOMP:
            return CException_004e1940(expression);
        case ELABEL:
        case ESETCONST:
            lower_newexception(expression, 0);
            return;
        case EMFPOINTER:
            expression->data.diadic.left = fn_004e1050(expression->data.diadic.left);
            expression->data.diadic.right = rewrite_expr_temporaries(expression->data.diadic.right);
            return expression;
        case ECOND:
            expression->data.diadic.left = fn_004e1050(expression->data.diadic.left);
            expression->data.diadic.right = rewrite_expr_temporaries(expression->data.diadic.right);
            expression->data.cond.expr2 = rewrite_expr_temporaries(expression->data.cond.expr2);
            return expression;
        case ELAND:
        case ELOR:
        case ECOMMA:
            expression->data.diadic.left = fn_004e1050(expression->data.diadic.left);
            expression->data.diadic.right = rewrite_expr_temporaries(expression->data.diadic.right);
            return expression;
        case EFUNCCALL:
        case EFUNCCALLP:
            return rewrite_funccall_temporaries(expression, 0);
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
        case EROTL:
        case EROTR:
            expression->data.diadic.left = fn_004e1050(expression->data.diadic.left);
            expression->data.diadic.right = fn_004e1050(expression->data.diadic.right);
            return expression;
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
            expression->data.diadic.left = fn_004e1050(expression->data.diadic.left);
            return expression;
        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case EQUALNAME:
        case ENULLCHECK:
        case ELOCOBJ:
        case ENEWEXCEPTION:
        case ENEWEXCEPTIONARRAY:
        case EMEMBER:
        case EASSBLK:
            return expression;
        default:
            CError_FATAL(2127);
            return expression;
    }
}

ENode *rewrite_expr_temporaries(ENode *expr)
{
    switch (expr->type) {
        case EPRECOMP:
            return CException_004e1940(expr);
        case ELABEL:
        case ESETCONST:
            lower_newexception(expr, 1);
            return;
        case EMFPOINTER:
            expr->data.diadic.left = rewrite_expr_temporaries(expr->data.diadic.left);
            expr->data.diadic.right = rewrite_expr_temporaries(expr->data.diadic.right);
            return expr;
        case ECOND:
            expr->data.cond.cond = rewrite_expr_temporaries(expr->data.cond.cond);
            expr->data.cond.expr1 = rewrite_expr_temporaries(expr->data.cond.expr1);
            expr->data.cond.expr2 = rewrite_expr_temporaries(expr->data.cond.expr2);
            return expr;
        case ELAND:
        case ELOR:
            expr->data.diadic.left = rewrite_expr_temporaries(expr->data.diadic.left);
            expr->data.diadic.right = rewrite_expr_temporaries(expr->data.diadic.right);
            return expr;
        case EFUNCCALL:
        case EFUNCCALLP:
            return rewrite_funccall_temporaries(expr, 1);
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
            expr->data.diadic.left = rewrite_expr_temporaries(expr->data.diadic.left);
            expr->data.diadic.right = rewrite_expr_temporaries(expr->data.diadic.right);
            return expr;
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
            expr->data.monadic = rewrite_expr_temporaries(expr->data.monadic);
            return expr;
        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ENULLCHECK:
        case ELOCOBJ:
        case EMEMBER:
        case EASSBLK:
            return expr;
        default:
            CError_FATAL(2016);
            return expr;
    }
}

ENode *rewrite_funccall_temporaries(ENode *node, Boolean reverse)
{
    ENodeList **args;
    ENodeList **reverseArgs;
    ENodeList *temporaryArg = NULL;
    ENodeList *argument;
    Type *resultType;
    ENode *result;
    TemporaryObject *temporary;
    SInt32 uniqueID;
    SInt32 index;
    SInt32 count;
    CException *cleanupCopy;
    CException *cleanup;
    Statement *statement;
    ENode *temporaryExpr;

    temporaryArg = NULL;
    if (node->data.funccall.args != NULL) {
        if ((temporaryExpr = node->data.funccall.args->node)->type == EPRECOMP) {
            if (temporaryExpr->data.temp.needs_dtor)
                temporaryArg = node->data.funccall.args;
        } else if (node->data.funccall.args->next != NULL) {
            if ((temporaryExpr = node->data.funccall.args->next->node)->type == EPRECOMP &&
                temporaryExpr->data.temp.needs_dtor)
                temporaryArg = node->data.funccall.args->next;
        }
    }

    if (temporaryArg != NULL) {
        if (reverse) {
            argument = node->data.funccall.args;
            count = 0;
            while (argument != NULL) {
                count++;
                argument = argument->next;
            }
            reverseArgs = CompilerTools_AllocatePool(count * sizeof(ENodeList *));
            argument = node->data.funccall.args;
            index = 0;
            while (argument != NULL) {
                reverseArgs[index++] = argument;
                argument = argument->next;
            }
            while (index > 0) {
                index--;
                reverse_one(reverseArgs, index, temporaryArg);
            }
        } else {
            for (argument = node->data.funccall.args; argument != NULL; argument = argument->next) {
                if (argument != temporaryArg)
                    argument->node = fn_004e1050(argument->node);
            }
        }
        temporary = CompilerTools_AllocatePool(sizeof(TemporaryObject));
        temporary->next = temporary_object_list;
        temporary_object_list = temporary;

        uniqueID = temporaryExpr->data.temp.uniqueid;
        if (uniqueID != 0) {
            temporary->object = findTemporaryObject(uniqueID, temporaryExpr);
        } else {
            temporary->object = create_temp_object(temporaryExpr->data.temp.type);
        }
        temporary->initializationFlag = NULL;
        if (temporaryExpr->data.temp.type->type == TYPECLASS) {
            temporary->classObject = CClass_Destructor((TypeClass *)temporaryExpr->data.temp.type);
            CError_ASSERT(1825, temporary->classObject != NULL);
        } else {
            CError_FATAL(1825);
        }
        temporaryExpr->type = EOBJREF;
        temporaryExpr->data.objref = temporary->object;

        if (reverse) {
            resultType = node->rtype;
            temporary->initializationFlag = create_temp_object((Type *)&stchar);
            node = makediadicnode(node,
                                  makediadicnode(create_objectnode(temporary->initializationFlag),
                                                 intconstnode((Type *)&stchar, 1), EASS),
                                  ECOMMA);
            result = makediadicnode(node, create_objectrefnode(temporary->object), ECOMMA);
            result->rtype = resultType;
            statement = NewTemporaryStatement();
            statement->expr.expression = makediadicnode(create_objectnode(temporary->initializationFlag),
                                                        intconstnode((Type *)&stchar, 0), EASS);
            data_00581c36 = statement;
            cleanup = CompilerTools_AllocatePool(sizeof(CException));
            cleanup->next = currentDobjstack;
            currentDobjstack = cleanup;
            cleanup->kind = 2;
            cleanup->data.local_cond.object = temporary->object;
            cleanup->data.local_cond.dtor = CABI_GetDestructorObject(temporary->classObject, 1);
            cleanup->data.local_cond.cond = temporary->initializationFlag;
            cleanupCopy = CompilerTools_AllocatePool(sizeof(CException));
            *cleanupCopy = *currentDobjstack;
            cleanupCopy->next = current_dobjstack;
            current_dobjstack = cleanupCopy;
        } else {
            statement = NewTemporaryStatement();
            statement->expr.expression = node;
            data_00581c36 = statement;
            cleanup = CompilerTools_AllocatePool(sizeof(CException));
            cleanup->next = currentDobjstack;
            currentDobjstack = cleanup;
            cleanup->kind = 1;
            cleanup->data.local.object = temporary->object;
            cleanup->data.local.dtor = CABI_GetDestructorObject(temporary->classObject, 1);
            cleanupCopy = CompilerTools_AllocatePool(sizeof(CException));
            *cleanupCopy = *currentDobjstack;
            cleanupCopy->next = current_dobjstack;
            current_dobjstack = cleanupCopy;
            result = CompilerTools_AllocatePool(sizeof(ENode));
            *result = *statement->expr.expression;
            result->type = EOBJREF;
            result->data.objref = temporaryArg->node->data.objref;
        }
        return result;
    }

    if (reverse) {
        argument = node->data.funccall.args;
        count = 0;
        while (argument != NULL) {
            count++;
            argument = argument->next;
        }
        args = CompilerTools_AllocatePool(count * sizeof(ENodeList *));
        argument = node->data.funccall.args;
        index = 0;
        while (argument != NULL) {
            args[index++] = argument;
            argument = argument->next;
        }
        while (index > 0) {
            index--;
            args[index]->node = rewrite_expr_temporaries(args[index]->node);
        }
        node->data.funccall.funcref = rewrite_expr_temporaries(node->data.funccall.funcref);
    } else {
        for (argument = node->data.funccall.args; argument != NULL; argument = argument->next)
            argument->node = fn_004e1050(argument->node);
        node->data.funccall.funcref = fn_004e1050(node->data.funccall.funcref);
    }
    return node;
}

void lower_newexception(ENode *node, Boolean useExpression)
{
    Statement *initialStatement;
    Statement *statement;
    Boolean isArgumentObject;
    Object *cleanupFlag;
    CLabel *label;
    CException *cleanup;
    ENode *expression;
    ENode *result;

    isArgumentObject = (node->type == ESETCONST);

    if (useExpression) {
        node->data.newexception.initexpr = rewrite_expr_temporaries(node->data.newexception.initexpr);
        node->data.newexception.tryexpr = rewrite_expr_temporaries(node->data.newexception.tryexpr);
        cleanupFlag = create_temp_object((Type *)&stchar);
        initialStatement = InsertPrevStatement(ST_EXPRESSION);
        initialStatement->expr.expression =
            makediadicnode(create_objectnode(cleanupFlag), intconstnode((Type *)&stchar, 0), EASS);
        data_00581c36 = initialStatement;
        cleanup = (CException *)CompilerTools_AllocatePool(sizeof(CException));
        cleanup->next = currentDobjstack;
        currentDobjstack = cleanup;
        cleanup->kind = 0xc;
        cleanup->data.delete_pointer_cond.pointer = node->data.newexception.pointertemp;
        cleanup->data.delete_pointer_cond.deletefunc = node->data.newexception.deletefunc;
        cleanup->data.delete_pointer_cond.cond = cleanupFlag;
        if (isArgumentObject) {
            expression = makediadicnode(create_objectnode(cleanupFlag), intconstnode((Type *)&stchar, 1), EASS);
            expression = makediadicnode(node->data.newexception.initexpr, expression, ECOMMA);
            expression = makediadicnode(expression, node->data.newexception.tryexpr, ECOMMA);
            expression = makediadicnode(
                expression, makediadicnode(create_objectnode(cleanupFlag), intconstnode((Type *)&stchar, 0), EASS),
                ECOMMA);
            result = makediadicnode(expression, create_objectnode(node->data.newexception.pointertemp), ECOMMA);
        } else {
            expression = makediadicnode(create_objectnode(cleanupFlag), intconstnode((Type *)&stchar, 1), EASS);
            expression = makediadicnode(expression, node->data.newexception.tryexpr, ECOMMA);
            expression = makediadicnode(
                expression, makediadicnode(create_objectnode(cleanupFlag), intconstnode((Type *)&stchar, 0), EASS),
                ECOMMA);
            expression = makediadicnode(node->data.newexception.initexpr, expression, ELAND);
            result = makediadicnode(expression, create_objectnode(node->data.newexception.pointertemp), ECOMMA);
        }
    } else {
        node->data.newexception.initexpr = fn_004e1050(node->data.newexception.initexpr);
        node->data.newexception.tryexpr = fn_004e1050(node->data.newexception.tryexpr);
        if (isArgumentObject) {
            statement = InsertPrevStatement(ST_EXPRESSION);
            statement->expr.expression = node->data.newexception.initexpr;
        } else {
            statement = InsertPrevStatement(ST_IFNGOTO);
            statement->expr.expression = node->data.newexception.initexpr;
            label = newlabel();
            statement->target.label = label;
        }
        statement = CFunc_InsertAfterStatement(ST_EXPRESSION, statement);
        statement->expr.expression = node->data.newexception.tryexpr;
        cleanup = (CException *)CompilerTools_AllocatePool(sizeof(CException));
        cleanup->next = currentDobjstack;
        cleanup->kind = 0xa;
        cleanup->data.delete_pointer.pointer = node->data.newexception.pointertemp;
        cleanup->data.delete_pointer.deletefunc = node->data.newexception.deletefunc;
        statement->dobjstack = cleanup;
        if (!isArgumentObject) {
            statement = CFunc_InsertAfterStatement(ST_LABEL, statement);
            statement->target.label = label;
            label->target.stmt = statement;
            statement->dobjstack = currentDobjstack;
        }
        data_00581c36 = statement;
        result = create_objectnode(node->data.newexception.pointertemp);
    }
    result->rtype = node->rtype;
}

ENode *CException_004e1940(ENode *node)
{
    Object *obj;
    obj = CException_CachedObject(node);

    if (node->data.temp.needs_dtor) {
        TemporaryObject *type = CException_NewTypeNode();
        CException *stmt;
        type->object = obj;
        type->initializationFlag = NULL;
        CError_ASSERT(1643, node->data.objref->otype == OT_OBJECT &&
                                (type->classObject = CClass_Destructor((TypeClass *)node->data.objref)) != 0);
        stmt = CException_NewStmtNode();
        stmt->kind = 1;
        stmt->data.local.object = type->object;
        stmt->data.local.dtor = CABI_GetDestructorObject(type->classObject, 1);
        CException_CopyStmtNode();
    }

    node->type = EOBJREF;
    node->data.objref = obj;
    return node;
}

Object *CException_GetTempObject(ENode *obj)
{
    ECacheNode *p;
    SInt32 key;

    if ((key = obj->data.temp.uniqueid) != 0) {
        p = cached_objects;
        while (p != NULL) {
            if (p->key == key)
                return p->obj;
            p = p->next;
        }
        p = (ECacheNode *)galloc(sizeof(ECacheNode));
        p->next = cached_objects;
        cached_objects = p;
        p->key = key;
        p->obj = create_temp_object(obj->data.temp.type);
        return p->obj;
    }
    return create_temp_object(obj->data.temp.type);
}

/* Linked record describing an exception handler. */

void CExcept_ScanTryBlock(void *context, char rethrow)
{
    Statement *tryBody;
    ExceptionHandlerRecord *previous;
    CLabel *handlerLabel;
    Statement *handlerStart;
    Object *exceptionObject;
    CLabel *endLabel;
    ExceptionHandlerRecord *handler;
    Statement *entry;
    struct SavedGlobalValues *cleanup;
    Statement *lastStatement;
    ENode *initializer;
    DeclInfo declaration;

    if (copts.fb3 == 0)
        CError_ReportError(ERR_EXCEPTION_HANDLING_OPTION_DISABLED);

    exceptionObject = create_temp_object(&exception_temp_object_type);
    if (data_005884fa != 0) {
        exceptionObject->name = GetHashNameNode("__exception_magic");
        CScope_AddObject(currentNameSpace, exceptionObject->name, (ObjBase *)exceptionObject);
    }

    entry = CFunc_AppendStatement(2);
    entry->flags = 1;
    handlerLabel = newlabel();
    entry->target.label = handlerLabel;
    entry->target.label->target.stmt = entry;

    tryBody = (Statement *)object_statement(exceptionObject);

    if (tk != '{') {
        CError_ReportError(ERR_LBRACE_EXPECTED);
        return;
    }
    CFunc_ParseScopedStatement(context);
    if (tk != TK_CATCH) {
        CError_ReportError(ERR_CATCH_EXPECTED);
        return;
    }

    handlerStart = CFunc_AppendStatement(3);
    handlerStart->target.label = newlabel();
    handlerLabel = handlerStart->target.label;

    endLabel = newlabel();
    previous = NULL;

    for (;;) {
        statement_sourceoffset = CPrep_UpdateTokenLine(&function_fileinfo);

        lastStatement = CFunc_AppendStatement(2);
        lastStatement->flags = 1;
        lastStatement->target.label = newlabel();
        cleanup = NULL;
        lastStatement->target.label->target.stmt = lastStatement;

        handler = (ExceptionHandlerRecord *)CompilerTools_AllocatePool(sizeof(*handler));
        memclrw(handler, sizeof(*handler));
        handler->previous = previous;
        previous = handler;
        handler->handlerEntry = lastStatement;
        handler->exceptionObject = exceptionObject;

        tk = CPrepTokenizer_GetNextToken();
        if (tk != '(') {
            CError_ReportError(ERR_LPAREN_EXPECTED);
            break;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk == TK_ELLIPSIS) {
            tk = CPrepTokenizer_GetNextToken();
        } else {
            memclrw(&declaration, sizeof(declaration));
            CParser_GetDeclSpecs(&declaration, 0);
            if (declaration.missingTypeSpecifier != 0)
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            if (declaration.storage != 0)
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            CError_ReportIllegalFlags(declaration.qual & 0xe1fffff4u);
            CDecl_ParseDeclarator(&declaration);
            if (declaration.dtype->type == TYPEFUNC)
                declaration.dtype = CDecl_NewPointerType(declaration.dtype);
            else if (declaration.dtype->type == TYPEARRAY)
                declaration.dtype = CDecl_NewPointerType(TPTR_TARGET(declaration.dtype));
            CanAllocObject(declaration.dtype);
            if (declaration.dtype->type == TYPECLASS && (TYPE_CLASS(declaration.dtype)->flags & CLASS_ABSTRACT) != 0)
                CError_IllegalUseAbstractClass(TYPE_CLASS(declaration.dtype));
            handler->exceptionType = declaration.dtype;
            handler->declarationData = declaration.qual;
            if (declaration.name != NULL) {
                cleanup = fn_0047cb60();
                initializer = create_catch_object_init(&declaration, handler);
                lastStatement = CFunc_AppendStatement(4);
                lastStatement->expr.expression = initializer;
            }
        }

        if (tk != ')') {
            CError_ReportError(ERR_RPAREN_EXPECTED);
            break;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk != '{') {
            CError_ReportError(ERR_LBRACE_EXPECTED);
            break;
        }
        CFunc_ParseScopedStatement(context);
        if (rethrow != 0) {
            lastStatement = CFunc_AppendStatement(4);
            lastStatement->expr.expression = funccallexpr(throw_func, nullnode(), nullnode(), nullnode(), NULL);
        }
        handler->handlerEnd = lastStatement;
        if (cleanup != NULL)
            PPCError_RestoreGlobalValues(cleanup);
        if (tk != TK_CATCH)
            break;
        {
            Statement *branch = CFunc_AppendStatement(3);
            branch->target.label = endLabel;
        }
    }

    {
        Statement *end = CFunc_AppendStatement(2);
        end->target.label = endLabel;
        endLabel->target.stmt = end;
        CException_004e1fb0(tryBody, handlerStart, end, handler);
    }
    finish_label(handlerLabel);
}

/* Record in front of the created exception object: a type and a name. */

/* Two object slots handed in by the caller. */

ENode *create_catch_object_init(DeclInfo *info, ExceptionHandlerRecord *args)
{
    Object *obj;
    ENode *node;
    ENode *monad;
    Object *cls;
    Object *cls2;
    ENode *objnode;
    ENode *m;

    if (CScope_FindName(currentNameSpace, info->name) != NULL)
        CError_ReportError(ERR_IDENTIFIER_REDECLARED, info->name->name);

    obj = CParser_NewLocalDataObject(info, 1);
    CFunc_SetupLocalVarInfo(obj);
    CScope_AddObject(currentNameSpace, info->name, (ObjBase *)obj);
    args->catchObject = obj;

    node = makediadicnode(create_objectrefnode(args->exceptionObject), intconstnode((Type *)&stunsignedlong, 12), EADD);
    node->rtype = CDecl_NewPointerType(CDecl_NewPointerType(info->dtype));
    monad = makemonadicnode(node, EINDIRECT);
    monad->rtype = CDecl_NewPointerType(info->dtype);

    if (info->dtype->type == TYPEPOINTER && (TYPE_POINTER(info->dtype)->qual & Q_REFERENCE) != 0) {
        return makediadicnode(CExpr_New_EINDIRECT_Node(obj), monad, EASS);
    }

    if (info->dtype->type == TYPECLASS) {
        cls = CClass_CopyConstructor(TYPE_CLASS(info->dtype));
        if (cls != NULL) {
            cls2 = CClass_Destructor(TYPE_CLASS(info->dtype));
            if (cls2 == NULL)
                objnode = create_objectrefnode(obj);
            else
                objnode = CExcept_RegisterDestructorObject(obj, 0, cls2, 0);
            return create_call_with_arg_and_default_args(cls, TYPE_CLASS(info->dtype), objnode, monad);
        }
    }

    m = makemonadicnode(monad, EINDIRECT);
    m->rtype = info->dtype;
    return makediadicnode(create_objectnode(obj), m, EASS);
}

void CException_004e1fb0(Statement *firstScope, Statement *insertionScope, Statement *lastScope,
                         ExceptionHandlerRecord *entries)
{
    CException *originalList;
    SInt32 entryValue;
    Boolean hasClassType;
    CException *firstEntry;
    CException *replacementList;
    CException *entry;
    ENode *stringNode;
    Statement *scope;
    CException *current;

    firstEntry = NULL;
    hasClassType = 0;
    entryValue = (SInt32)entries->exceptionObject;
    originalList = firstScope->dobjstack;
    replacementList = originalList;

    if (entries != NULL) {
        do {
            entry = (CException *)CompilerTools_AllocatePool(sizeof(CException));
            memclrw(entry, sizeof(CException));
            entry->next = replacementList;
            replacementList = entry;
            if (firstEntry == NULL)
                firstEntry = entry;
            entry->kind = 0xd;
            entry->data.catch_block.object = (Object *)entries->catchObject;
            entry->data.catch_block.label = entries->handlerEntry->target.label;
            if (entries->exceptionType != NULL) {
                stringNode = create_type_stringconst(entries->exceptionType, entries->declarationData, 0);
                CError_ASSERT(1009, stringNode->type == ESTRINGCONST);
                entry->data.catch_block.typeInfo =
                    CInit_DeclareString(stringNode->data.string.data, stringNode->data.string.size, 0, 0);
            }
            entry->data.catch_block.info = (Object *)entryValue;
            if (!hasClassType && CException_IsClassType(entries->exceptionType))
                hasClassType = 1;
            entry->data.catch_block.exceptionType = entries->exceptionType;
            entry->data.catch_block.declarationData = entries->declarationData;
            entries = entries->previous;
        } while (entries != NULL);
    }

    scope = firstScope;
    for (;;) {
        if ((current = scope->dobjstack) != originalList) {
            for (;;) {
                if (current == NULL)
                    CError_FATAL(1320);
                if (current->next == replacementList)
                    break;
                if (current->next == originalList) {
                    current->next = replacementList;
                    break;
                }
                current = current->next;
            }
        } else {
            scope->dobjstack = replacementList;
        }
        if (scope == lastScope)
            break;
        if (scope == insertionScope) {
            CException *scopeEntry = (CException *)CompilerTools_AllocatePool(sizeof(CException));
            memclrw(scopeEntry, sizeof(CException));
            scopeEntry->next = originalList;
            scopeEntry->kind = 0xe;
            scopeEntry->data.active_catch.info = (Object *)entryValue;
            scopeEntry->data.active_catch.call_dtor = hasClassType;
            replacementList = scopeEntry;
        }
        scope = scope->next;
    }
    exception_cleanup_registered = 1;
}

/* 0x4463d0, one pushed argument */
/* 0x492070, returns ax */
/* 0x473f70, no arguments, EINTCONST node */

ENode *CExcept_ScanThrowExpression(void)
{
    Object *cls;
    ENode *e;
    ENode *nd;
    ENode *objref;
    Object *obj;
    ENode *node;

    if (!copts.fb3)
        CError_ReportError(ERR_EXCEPTION_HANDLING_OPTION_DISABLED);
    tk = CPrepTokenizer_GetNextToken();
    switch (tk) {
        case ')':
        case ',':
        case ':':
        case ';':
            e = funccallexpr(throw_func, nullnode(), nullnode(), nullnode(), NULL);
            break;
        default:
            node = CExpr_GeneratePointerAndRewriteConst(assignment_expression());
            obj = create_temp_object(node->rtype);
            if (node->rtype->type != TYPECLASS || (nd = CExpr_IsTempConstruction(node, node->rtype, &e)) == NULL) {
                objref = create_objectrefnode(obj);
                if (node->rtype->type == TYPECLASS && (cls = CClass_CopyConstructor(TYPE_CLASS(node->rtype))) != NULL) {
                    nd = create_call_with_arg_and_default_args(cls, TYPE_CLASS(node->rtype), objref,
                                                               getnodeaddress(node, 0));
                } else {
                    if (node->rtype->size == 0)
                        CError_ReportError(ERR_ILLEGAL_TYPE);
                    nd = makemonadicnode(objref, EINDIRECT);
                    nd->rtype = node->rtype;
                    nd = makediadicnode(nd, node, EASS);
                    nd = makediadicnode(nd, create_objectrefnode(obj), ECOMMA);
                    nd->rtype = (Type *)&void_ptr;
                }
            } else {
                *e = *create_objectrefnode(obj);
            }
            e = create_type_stringconst(node->rtype, node->flags & 3, 1);
            if (node->rtype->type == TYPECLASS && (cls = CClass_Destructor((TypeClass *)node->rtype)) != NULL) {
                e = funccallexpr(throw_func, e, nd, create_objectrefnode(CABI_GetDestructorObject(cls, 1)), NULL);
            } else {
                e = funccallexpr(throw_func, e, nd, nullnode(), NULL);
            }
            break;
    }
    e->flags |= Q_VOLATILE;
    return e;
}

ENode *create_call_with_arg_and_default_args(Object *func, TypeClass *cls, ENode *which, ENode *arg)
{
    ENodeList *list;
    FuncArg *fa;
    ENode *call;
    ENodeList *p;

    CError_ASSERT(1092, IS_TYPE_FUNC(func->type) && (fa = TYPE_FUNC(func->type)->args) != 0 && (fa = fa->next) != 0);

    call = funccallexpr(func, which, NULL, NULL, NULL);
    list = call->data.funccall.args;

    if (cls->flags & CLASS_HAS_VBASES) {
        CError_ASSERT(1102, (fa = fa->next) != 0);
        list->next = (ENodeList *)CompilerTools_AllocatePool(8);
        list = list->next;
        list->next = NULL;
        list->node = intconstnode((Type *)&stsignedshort, 1);
    }

    list->next = (ENodeList *)CompilerTools_AllocatePool(8);
    p = list->next;
    p->next = NULL;
    p->node = arg;
    while ((fa = fa->next) != NULL) {
        CError_ASSERT(1118, fa->dexpr != 0);
        p->next = (ENodeList *)CompilerTools_AllocatePool(8);
        p = p->next;
        p->next = NULL;
        p->node = fn_00513040(fa->dexpr, 0);
    }
    return call;
}

void CExcept_ScanExceptionSpecification(TypeFunc *func)
{
    ExceptSpecList *list;
    ExceptSpecList *existing;
    ExceptSpecList *node;
    TypePointer *unqualifiedType;
    DeclInfo decl;

    list = NULL;
    if (CPrepTokenizer_GetNextToken() != '(') {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return;
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk != ')') {
        for (;;) {
            memclrw(&decl, sizeof(decl));
            CParser_GetDeclSpecs(&decl, 0);
            if (decl.storage != 0)
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            CError_ReportIllegalFlags(decl.qual & ~(Q_CV | Q_PASCAL | Q_ALIGNED_MASK));
            CDecl_ParseDeclarator(&decl);
            if (decl.name != NULL)
                CError_ReportError(ERR_ILLEGAL_TYPE);
            if (decl.dtype->type == TYPEPOINTER) {
                if (TYPE_POINTER(decl.dtype)->qual & Q_CV) {
                    unqualifiedType = (TypePointer *)galloc(sizeof(TypePointer));
                    *unqualifiedType = *TYPE_POINTER(decl.dtype);
                    unqualifiedType->qual = 0;
                    decl.dtype = (Type *)unqualifiedType;
                }
            } else {
                decl.qual = 0;
            }
            for (existing = list; existing != NULL; existing = existing->next) {
                if (iscpp_typeequal(existing->type, decl.dtype) != 0 && existing->qual == decl.qual)
                    break;
            }
            if (existing == NULL) {
                node = (ExceptSpecList *)galloc(sizeof(ExceptSpecList));
                memclrw(node, sizeof(ExceptSpecList));
                node->next = list;
                node->type = decl.dtype;
                node->qual = decl.qual;
                list = node;
            }
            if (tk == ')')
                break;
            if (tk != ',') {
                CError_ReportError(ERR_RPAREN_EXPECTED);
                break;
            }
            tk = CPrepTokenizer_GetNextToken();
        }
    }
    if (list == NULL) {
        list = (ExceptSpecList *)galloc(sizeof(ExceptSpecList));
        memclrw(list, sizeof(ExceptSpecList));
    }
    func->exspecs = list;
    tk = CPrepTokenizer_GetNextToken();
}

ENode *create_type_stringconst(Type *type, UInt32 qualifiers, Boolean flag)
{
    TypePointer unqualifiedPointer;
    ENode *node;
    UInt32 manglingQualifiers = qualifiers;

    if (type->type == TYPEPOINTER && (TYPE_POINTER(type)->qual & Q_REFERENCE))
        type = TYPE_POINTER(type)->target;

    if (type->type == TYPECLASS || (type->type == TYPEPOINTER && TYPE_POINTER(type)->target->type == TYPECLASS)) {
        data_00583548.size = 0;
        if (type->type == TYPEPOINTER) {
            AppendGListByte(&data_00583548, 0x2a);
            type = TYPE_POINTER(type)->target;
        } else {
            AppendGListByte(&data_00583548, 0x21);
        }
        if (flag) {
            TypeClass *classType = TYPE_CLASS(type);
            emit_flagged_class_offsets(classType);
        } else {
            TypeClass *classType = TYPE_CLASS(type);
            fn_004e2940(classType);
            AppendGListByte(&data_00583548, 0x21);
        }
    } else {
        if (type->type == TYPEPOINTER) {
            if (TYPE_POINTER(type)->qual & Q_CV) {
                unqualifiedPointer = *TYPE_POINTER(type);
                unqualifiedPointer.qual = 0;
                manglingQualifiers = 0;
                type = (Type *)&unqualifiedPointer;
            }
        } else {
            manglingQualifiers = 0;
        }
        fn_004c2ac0(type, manglingQualifiers);
    }

    AppendGListByte(&data_00583548, 0);

    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = ESTRINGCONST;
    node->cost = 0;
    node->flags = 0;
    node->rtype = (Type *)&void_ptr;
    node->data.string.size = data_00583548.size;
    node->data.string.data = (char *)galloc(data_00583548.size);
    node->data.string.useExplicitSize = 0;
    memcpy(node->data.string.data, *data_00583548.data, data_00583548.size);
    return node;
}

/* Temporary list produced by exception traversal. */

void emit_flagged_class_offsets(TypeClass *type)
{
    char buf[16];
    ClassNode *node = add_class_and_bases(NULL, type, type, 0U, 0U, 1U);
    while (node != NULL) {
        if (node->flagd != 0 && node->flage == 0) {
            fn_004e2940(node->cls);
            AppendGListByte(&data_00583548, 33);
            if ((UInt32)node->offset != 0U) {
                sprintf(buf, "%ld!", node->offset);
                CompilerTools_AppendGListString(&data_00583548, buf);
            } else {
                AppendGListByte(&data_00583548, 33);
            }
        }
        node = node->next;
    }
}

ClassNode *add_class_and_bases(ClassNode *list, TypeClass *mostDerivedClass, TypeClass *cls, SInt32 offset,
                               Boolean isVirtual, Boolean isPublic)
{
    ClassNode *node;
    ClassList *base;

    for (node = list; node != NULL; node = node->next) {
        if (node->cls == cls) {
            if (isVirtual != 0 && node->flagc != 0) {
                if (isPublic != 0)
                    node->flagd = 1;
            } else {
                mark_class_and_bases(list, cls);
            }
            return list;
        }
    }
    node = (ClassNode *)CompilerTools_AllocatePool(sizeof(*node));
    node->cls = cls;
    node->offset = offset;
    node->flagc = isVirtual;
    node->flagd = isPublic;
    node->flage = 0;
    node->next = list;
    list = node;
    for (base = cls->bases; base != NULL; base = base->next) {
        int baseIsPublic;

        baseIsPublic = 0;
        if (isPublic != 0 && base->access == ACCESSPUBLIC)
            baseIsPublic = 1;
        if (base->is_virtual != 0) {
            list = add_class_and_bases(list, mostDerivedClass, base->base,
                                       CClass_FindVBaseOffset(mostDerivedClass, base->base), 1, baseIsPublic);
        } else {
            list = add_class_and_bases(list, mostDerivedClass, base->base, offset + base->offset, 0, baseIsPublic);
        }
    }
    return list;
}

/* One entry in the list of exception-specification classes: next pointer at
 * 0x00, the class it denotes at 0x04, and a byte flag at 0x0e.  Only the
 * fields touched by the original body are named. */

/* * Mark every entry of `list` whose class is `cls` or one of its transitive
 * base classes. */
void mark_class_and_bases(ClassNode *list, TypeClass *cls)
{
    ClassNode *node;
    ClassList *base;

    for (node = list; node != NULL; node = node->next) {
        if (node->cls == cls)
            node->flage = 1;
    }
    for (base = cls->bases; base != NULL; base = base->next)
        mark_class_and_bases(list, base->base);
}

/* Records consumed by the exception-name writer. */

void fn_004e2940(TypeClass *exceptionData)
{
    NameSpace *entry;
    char name[64];

    append_namespace_names(exceptionData->nspace->parent);
    CompilerTools_AppendGListString(&data_00583548, exceptionData->classname->name);
    entry = exceptionData->nspace->parent;
    while (entry != NULL) {
        if (entry->is_global == 0 && entry->is_templ == 0 && entry->name == NULL) {
            if (data_00588238 == NULL) {
                CError_FATAL(790);
            }
            sprintf(name, "*%lx*%lx*", &data_00588238, (int)&entry);
            CompilerTools_AppendGListString(&data_00583548, name);
            break;
        }
        entry = entry->parent;
    }
}

void append_namespace_names(NameSpace *p)
{
    for (; p; p = p->parent)
        if (p->name) {
            append_namespace_names(p->parent);
            CompilerTools_AppendGListString(&data_00583548, p->name->name);
            CompilerTools_AppendGListString(&data_00583548, "::");
            return;
        }
}

Statement *CExcept_ActionCleanup(CException *cleanup, Statement *statement)
{
    Object *context;
    ENode *expression;
    ENode *localDtorCall;
    Object *arrayDtor;
    ENode *memberDtorCall;
    ENode *dtorExpression;
    ENode *localObjectExpression;
    Object *localDtor;
    Object *deleteObject;
    Statement *localStatement;
    Statement *memberStatement;
    SInt32 offset;
    Object *deleteFunc;
    Object *memberDtor;
    SInt32 value1;
    SInt32 value2;

    switch (cleanup->kind) {
        case 1:
            localDtor = cleanup->data.local.dtor;
            localObjectExpression = create_objectrefnode(cleanup->data.local.object);
            localDtorCall = CException_004e2c40_inline1(localDtor, localObjectExpression);
            localStatement = CException_004e2c40_inline2(statement, localDtorCall);
            statement = localStatement;
            localStatement->dobjstack = cleanup->next;
            break;
        case 3:
            offset = cleanup->data.member.offset;
            memberDtor = cleanup->data.member.dtor;
            expression = create_objectrefnode(cleanup->data.member.objectptr);
            if (offset != 0) {
                expression = makediadicnode(expression, intconstnode((Type *)&stunsignedlong, offset), EADD);
            }
            memberDtorCall = CException_004e2c40_inline1(memberDtor, expression);
            memberStatement = CException_004e2c40_inline2(statement, memberDtorCall);
            statement = memberStatement;
            memberStatement->dobjstack = cleanup->next;
            break;
        case 11:
            deleteFunc = cleanup->data.local.dtor;
            deleteObject = cleanup->data.local.object;
            statement = CFunc_InsertAfterStatement(4, statement);
            statement->expr.expression =
                funccallexpr(deleteFunc, CExpr_New_EINDIRECT_Node(deleteObject), NULL, NULL, NULL);
            statement->dobjstack = cleanup->next;
            break;
        case 5:
            value2 = (SInt32)cleanup->data.member_array.count;
            value1 = (SInt32)cleanup->data.member_array.offset;
            arrayDtor = cleanup->data.member_array.dtor;
            context = cleanup->data.member_array.objectptr;
            statement = CFunc_InsertAfterStatement(4, statement);
            if (arrayDtor != NULL) {
                dtorExpression = create_objectrefnode(CABI_GetDestructorObject(arrayDtor, 1));
            } else {
                dtorExpression = nullnode();
            }
            statement->expr.expression = funccallexpr(data_0058717c, create_objectrefnode(context), dtorExpression,
                                                      intconstnode((Type *)&stunsignedlong, value2),
                                                      intconstnode((Type *)&stunsignedlong, value1));
            statement->dobjstack = cleanup->next;
            break;
        case 14:
            statement = CFunc_InsertAfterStatement(14, statement);
            statement->expr.expression = create_objectrefnode(cleanup->data.active_catch.info);
            statement->dobjstack = cleanup->next;
            if (cleanup->data.active_catch.call_dtor == 0) {
                statement->type = ST_ENDCATCH;
            }
            break;
        default:
            CError_FATAL(749);
        case 2:
        case 4:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
        case 15:
        case 16:
        case 17:
            break;
    }
    return statement;
}

void CExcept_RegisterMemberArray(Statement *stmt, Object *object, SInt32 offset, Object *dtor, SInt32 count,
                                 SInt32 size)
{
    CException *entry;

    entry = (CException *)CompilerTools_AllocatePool(sizeof(CException));
    memclrw((unsigned char *)entry, sizeof(CException));
    entry->kind = 9;
    entry->data.member_array.objectptr = object;
    entry->data.member_array.dtor = CABI_GetDestructorObject(dtor, 1);
    entry->data.member_array.offset = offset;
    entry->data.member_array.count = count;
    entry->data.member_array.size = size;
    insert_exception_action(stmt, entry);
    stmt->flags |= 2;
}

void CExcept_RegisterMember(Statement *statement, Object *objectptr, SInt32 offset, Object *destructor,
                            Object *condition, Boolean complete)
{
    CException *node;

    node = (CException *)CompilerTools_AllocatePool(sizeof(CException));
    memclrw(node, sizeof(CException));
    if (condition == NULL) {
        if (complete) {
            node->kind = 7;
            node->data.member.dtor = CABI_GetDestructorObject(destructor, 1);
        } else {
            node->kind = 0x11;
            node->data.member.dtor = CABI_GetDestructorObject(destructor, 0);
        }
        node->data.member.objectptr = objectptr;
        node->data.member.offset = offset;
    } else {
        CError_ASSERT(554, &condition->type->type == &stsignedshort.type);
        node->kind = 8;
        node->data.member_cond.objectptr = objectptr;
        node->data.member_cond.cond = condition;
        node->data.member_cond.dtor = CABI_GetDestructorObject(destructor, 1);
        node->data.member_cond.offset = offset;
    }
    insert_exception_action(statement, node);
    statement->flags |= 2;
}

void CException_AddStdTypeRecord(void)
{
    CException *rec;

    rec = CompilerTools_AllocatePool(sizeof(*rec));
    memclrw(rec, sizeof(*rec));
    rec->kind = 6;
    rec->data.types.type[0] = CException_StdType("ptr");
    rec->data.types.type[1] = CException_StdType("i");
    rec->data.types.type[2] = CException_StdType("dtor");
    rec->data.types.type[3] = CException_StdType("size");
    rec->next = UINT_00587fc4;
    UINT_00587fc4 = rec;
}

void fn_004e30c0(void)
{
    data_005884fa = 1;
    return;
}

void CException_PushEntry(void)
{
    CException *entry;

    entry = CompilerTools_AllocatePool(30U);
    memclrw(entry, 30U);
    entry->kind = 16U;
    entry->next = UINT_00587fc4;
    UINT_00587fc4 = entry;
}

void insert_exception_action(Statement *stmt, CException *action)
{
    CException *act;
    CException *scan;

    for (act = (CException *)stmt->dobjstack; act; act = act->next) {
        switch (act->kind) {
            case 7:
            case 8:
            case 9:
            case 15:
            case 17:
                break;
            default:
                continue;
        }
        break;
    }
    if (!act) {
        for (; stmt; stmt = stmt->next) {
            if ((scan = (CException *)stmt->dobjstack)) {
                for (;;) {
                    if (scan == action)
                        break;
                    if (!scan->next) {
                        scan->next = action;
                        break;
                    }
                    scan = scan->next;
                }
            } else {
                stmt->dobjstack = (CException *)action; /* insert_exception_action: exception action chain */
            }
        }
    } else {
        action->next = act;
        for (; stmt; stmt = stmt->next) {
            if ((scan = (CException *)stmt->dobjstack) != act) {
                for (;;) {
                    if (scan == action)
                        break;
                    if (scan->next == act) {
                        scan->next = action;
                        break;
                    }
                    if (!(scan = scan->next))
                        CError_FATAL(455);
                }
            } else {
                stmt->dobjstack = (CException *)action; /* insert_exception_action: exception action chain */
            }
        }
    }
}

/* Record whose byte at offset 28 selects exception handling. */

void CExcept_RegisterDeleteObject(Statement *expr, Object *first, Object *second)
{
    CException *record;
    record = CompilerTools_AllocatePool(30U);
    memclrw(record, 30U);
    record->next = UINT_00587fc4;
    UINT_00587fc4 = record;
    record->kind = 11U;
    record->data.pair.first = first;
    record->data.pair.second = second;
    exception_cleanup_registered = 1U;
}

/* 0x58851e, flag */

/* Destructor entry in the exception cleanup list. */

void CException_RegisterMemberArray(Statement *unused, Object *context, Object *destructor, SInt32 offset, SInt32 count)
{
    CException *entry;

    entry = CompilerTools_AllocatePool(sizeof(*entry));
    memclrw(entry, sizeof(*entry));
    entry->next = UINT_00587fc4;
    UINT_00587fc4 = entry;
    destructor = CABI_GetDestructorObject(destructor, 1);
    entry->kind = 5;
    entry->data.member_array.objectptr = context;
    entry->data.member_array.dtor = destructor;
    entry->data.member_array.offset = offset;
    entry->data.member_array.count = count;
    exception_cleanup_registered = 1;
}

ENode *CExcept_RegisterDestructorObject(Object *obj, SInt32 value, Object *dtorobj, int flag)
{
    CException *rec;
    Object *dtor;
    ENode *result;

    rec = CompilerTools_AllocatePool(sizeof(CException));
    memclrw(rec, sizeof(CException));
    rec->next = UINT_00587fc4;
    UINT_00587fc4 = rec;
    result = create_objectrefnode(obj);
    dtorobj = CABI_GetDestructorObject(dtorobj, 1);
    if (value == 0) {
        rec->kind = 1;
        rec->data.local.object = obj;
        rec->data.local.dtor = dtorobj;
    } else {
        rec->kind = 3;
        rec->data.local.object = obj;
        rec->data.local.dtor = dtorobj;
        rec->data.local.offset = value;
        result = makediadicnode(result, intconstnode(TYPE(&stunsignedlong), value), EADD);
    }
    exception_cleanup_registered = 1;
    return result;
}

unsigned char CExcept_ActionNeedsDestruction(CException *entry)
{
    unsigned char result;
    switch (entry->kind) {
        case 13:
            result = 0;
            return result;
        case 1:
        case 3:
        case 5:
        case 11:
        case 14:
            result = 1;
            return result;
        case 0:
        case 7:
        case 8:
        case 9:
        case 17:
            break;
        default:
            CError_FATAL(302);
            break;
    }
    result = 0;
    return result;
}

Boolean CExcept_ActionCompare(CException *left, CException *right)
{
    if (left->kind == right->kind) {
        switch (left->kind) {
            case 1:
                return left->data.local.object == right->data.local.object;
            case 2:
                return left->data.local_cond.object == right->data.local_cond.object;
            case 3:
                return left->data.local.object == right->data.local.object &&
                       left->data.local.offset == right->data.local.offset;
            case 4:
                return left->data.local.object == right->data.local.object;
            case 5:
                return left->data.member_array.objectptr == right->data.member_array.objectptr;
            case 6:
                return left->data.types.type[0] == right->data.types.type[0];
            case 7:
            case 17:
                return left->data.member.objectptr == right->data.member.objectptr &&
                       left->data.member.offset == right->data.member.offset;
            case 8:
                return left->data.member_cond.objectptr == right->data.member_cond.objectptr &&
                       left->data.member_cond.offset == right->data.member_cond.offset;
            case 9:
                return left->data.member_array.objectptr == right->data.member_array.objectptr &&
                       left->data.member_array.offset == right->data.member_array.offset;
            case 10:
            case 11:
                return left->data.pair.first == right->data.pair.first &&
                       left->data.pair.second == right->data.pair.second;
            case 12:
                return left->data.delete_pointer_cond.cond == right->data.delete_pointer_cond.cond;
            case 13:
                return left->data.catch_block.label == right->data.catch_block.label;
            case 14:
                return left->data.active_catch.info == right->data.active_catch.info;
            case 16:
                return 1;
            case 15:
                return left->data.specification.ids == right->data.specification.ids;
            default:
                CError_FATAL(253);
        }
    }
    return 0;
}

void CExcept_CompareSpecifications(ExceptSpecList *firstSpecs, ExceptSpecList *secondSpecs)
{
    ExceptSpecList *firstCursor;
    ExceptSpecList *secondCursor;
    ExceptSpecList *spec;
    ExceptSpecList *match;

    for (firstCursor = firstSpecs, secondCursor = secondSpecs;;) {
        if (firstCursor == NULL) {
            if (secondCursor == NULL) {
                break;
            }
            CError_ReportError(ERR_EXCEPTION_SPECIFICATION_LIST_MISMATCH);
            return;
        }
        if (secondCursor == NULL) {
            CError_ReportError(ERR_EXCEPTION_SPECIFICATION_LIST_MISMATCH);
            return;
        }
        firstCursor = firstCursor->next;
        secondCursor = secondCursor->next;
    }
    if (firstSpecs->type == NULL) {
        if (secondSpecs->type != NULL) {
            CError_ReportError(ERR_EXCEPTION_SPECIFICATION_LIST_MISMATCH);
        }
        return;
    }
    if (secondSpecs->type == NULL) {
        CError_ReportError(ERR_EXCEPTION_SPECIFICATION_LIST_MISMATCH);
        return;
    }
    for (spec = firstSpecs; spec != NULL; spec = spec->next) {
        for (match = secondSpecs; match != NULL; match = match->next) {
            if (iscpp_typeequal(spec->type, match->type) != 0 && spec->qual == match->qual) {
                break;
            }
        }
        if (match == NULL) {
            CError_ReportError(ERR_EXCEPTION_SPECIFICATION_LIST_MISMATCH);
            return;
        }
    }
}

/* 0x5641d0, "CException.c" */

void CException_004e35b0(CException *node)
{
    while (node != NULL) {
        switch (node->kind) {
            case 1:
                CInline_0050f240(node->data.local.dtor);
                break;
            case 2:
                CInline_0050f240(node->data.types.type[2]);
                break;
            case 3:
                CInline_0050f240(node->data.local.dtor);
                break;
            case 4:
                CInline_0050f240(node->data.local_pointer.dtor);
                break;
            case 5:
                CInline_0050f240(node->data.member_array.dtor);
                break;
            case 6:
                CInline_0050f240(node->data.types.type[2]);
                break;
            case 7:
            case 17:
                CInline_0050f240(node->data.member.dtor);
                break;
            case 8:
                CInline_0050f240(node->data.types.type[2]);
                break;
            case 9:
                CInline_0050f240(node->data.member_array.dtor);
                break;
            case 10:
            case 11:
                CInline_0050f240(node->data.pair.second);
                break;
            case 12:
                CInline_0050f240(node->data.delete_pointer.deletefunc);
                break;
            case 13:
            case 14:
            case 15:
            case 16:
                break;
            default:
                CError_FATAL(131);
                break;
        }
        node = node->next;
    }
}

void CExcept_Setup(void)
{
    UINT_00587fc4 = NULL;
    exception_cleanup_registered = 0U;
    data_005884fa = 0U;
    return;
}
