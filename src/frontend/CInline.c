#define CERROR_FILE "CInline.c"
#include "compiler/common.h"
#include "compiler/CInline.h"
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
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/COptimizer.h"
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
#include "compiler/DumpIR.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsm.h"
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
#include "driver/CLPluginRequests.h"
#include "driver/CLPlugins.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include <string.h>

#include "compiler/ENode.h"

#pragma auto_inline off

SInt16 CInline_ReturnZero(Type *type)
{
    return 0;
}

#pragma auto_inline reset

void CInline_GeneratePendingFunctionBody(void)
{
    Statement body;
    PendingFunction *pending;
    NameSpace *functionNamespace;
    Statement *statement;
    UInt8 savedFileSymInfo;
    struct InlineObjectEntry *objects;

    if (pending_functions == NULL || anyerrors != 0) {
        return;
    }
    undefined_function_objects = NULL;
    for (pending = pending_functions; pending != NULL; pending = pending->next) {
        CExpr_SearchExprTree((ENode *)pending->func, forward_objref, 1, 0x38);
    }
    objects = undefined_function_objects;
    CInline_0050f120(objects);
    while (CInline_DispatchNextDeferredNode()) {
    }
    functionNamespace = CFunc_FuncGenSetup(&body, NULL);
    savedFileSymInfo = copts.filesyminfo;
    copts.filesyminfo = 0;
    pending = pending_functions;
    while (pending != NULL) {
        if (pending->cls->nspace->theclass != NULL &&
            (pending->cls->nspace->theclass->flags & CLASS_IS_TEMPL_INST) != 0) {
            pending = generate_guarded_initializers(pending);
        } else {
            ENode *expression;
            statement = CFunc_AppendStatement(4);
            expression = (ENode *)pending->func;
            evalMode = 0;
            memo_list = NULL;
            alloc_state = 0;
            statement->expr.expression = CInline_00513240(expression);
            pending = pending->next;
        }
    }
    CFunc_CodeCleanup(&body);
    inline_statement_list(&body);
    if (!anyerrors) {
        if (copts.filesyminfo != 0) {
            fn_0043f1f0(&function_fileinfo);
        }
        CodeGen_Generator(&body, NULL, 0, 1);
    }
    currentNameSpace = functionNamespace->parent;
    copts.filesyminfo = savedFileSymInfo;
}

PendingFunction *generate_guarded_initializers(PendingFunction *pending)
{
    Statement *lastStatement;
    CLabel *label;
    Object *group;
    Statement *statement;
    ENode *initializer;
    Object *function;
    HashNameNode *groupName;

    group = pending->cls;
    function = CParser_NewCompilerDefDataObject();
    function->type = (Type *)&stsignedchar;
    groupName = COptimizer_GetFunctionObject(group);
    function->name = CParser_NameConcat("__init__", groupName->name);
    function->qual = Q_WEAK;
    fn_004ceab0(function, NULL, NULL, function->type->size);
    statement = CFunc_AppendStatement(6);
    statement->expr.expression = create_objectnode(function);
    label = newlabel();
    statement->target.label = label;
    do {
        statement = CFunc_AppendStatement(4);
        initializer = (ENode *)pending->func;
        evalMode = 0;
        memo_list = NULL;
        alloc_state = 0;
        statement->expr.expression = CInline_00513240(initializer);
        pending = pending->next;
    } while (pending && pending->cls == group);
    lastStatement = CFunc_AppendStatement(4);
    lastStatement->expr.expression =
        makediadicnode(create_objectnode(function), intconstnode((Type *)&stsignedchar, 1), 0x1e);
    lastStatement = CFunc_AppendStatement(2);
    lastStatement->target.label = label;
    label->target.stmt = lastStatement;
    return pending;
}

Boolean CInline_DispatchNextDeferredNode(void)
{
    CPrecNode *work;
    TypeFunc *functionType;

    if (!anyerrors) {
        work = pending_prec_nodes;
        if (work != NULL) {
            pending_prec_nodes = pending_prec_nodes->next;
            dispatching_deferred_node = 1;
            switch (work->kind) {
                case 3:
                    make_auto_generated_method(work->obj);
                    break;
                case 0:
                    if (!(work->obj->flags & 4))
                        parse_inline_definition(work);
                    break;
                case 1:
                    functionType = (TypeFunc *)work->obj->type;
                    if (!(functionType->flags & FUNC_DEFINED))
                        CTemplateNew_CompileObject(work->u.k1.classTemplate, work->u.k1.context, work->u.k1.source,
                                                   work->obj, 0);
                    break;
                case 2:
                    functionType = (TypeFunc *)work->obj->type;
                    if (!(functionType->flags & FUNC_DEFINED))
                        CTemplateNew_InstantiateFunction(work->u.k2.definition, work->u.k2.specialization, 0);
                    break;
                default:
                    CError_FATAL(4292);
            }
            dispatching_deferred_node = 0;
            return 1;
        }
        if (deferredInlineNodes != NULL && copts.f71 == 0) {
            InlineNode *deferred = deferredInlineNodes;

            deferredInlineNodes = deferred->next;
            generate_inline_code(deferred->func, deferred->body, deferred->flag);
            return 1;
        }
    }
    return 0;
}

static inline TypeClassExt800 *CInline_0050ebf0_inline1(Object *v1)
{
    TypeClass *v5s;
    NameSpace *v7;
    TypeClass *v5;
    v5 = (v5s = ((TypeMemberFunc *)v1->type)->theclass);
    if (v5s != NULL) {
        do {
            if ((v5->flags & CLASS_IS_TEMPL_INST) != 0) {
                return (TypeClassExt800 *)v5;
            }
            if (copts.f83 == 0) {
                break;
            }
            v7 = v5->nspace->parent;
            v5 = (TypeClass *)0;
            while ((int)v7 != 0) {
                if (v7->theclass != NULL) {
                    v5 = v7->theclass;
                    break;
                }
                v7 = v7->parent;
            }
        } while ((int)v5 != 0);
    }
    return NULL;
}

static inline TypeClassExt800 *CInline_0050ebf0_inline2(struct CPrecNode *a0)
{
    NameSpace *v12;
    TypeClass *v10;
    v10 = (TypeClass *)a0->u.k0.contextClass;
    while ((int)v10 != 0) {
        if ((v10->flags & CLASS_IS_TEMPL_INST) != 0) {
            return (TypeClassExt800 *)v10;
        }
        if (copts.f83 == 0) {
            break;
        }
        v12 = v10->nspace->parent;
        v10 = (TypeClass *)0;
        while ((int)v12 != 0) {
            if (v12->theclass != NULL) {
                v10 = v12->theclass;
                break;
            }
            v12 = v12->parent;
        }
    }
    return NULL;
}

static inline Boolean CInline_Cleanup(Statement *stmt)
{
    struct InlineObjectEntry *list;
    while (stmt != NULL) {
        if (stmt->dobjstack != NULL)
            add_undefined_exception_function_objects(stmt->dobjstack);
        switch (stmt->type) {
            case ST_NOP:
            case ST_LABEL:
            case ST_GOTO:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_ASM:
                break;
            case ST_RETURN:
                if (stmt->expr.expression == NULL)
                    break;
                /* fall through */
            case ST_EXPRESSION:
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_GOTOEXPR:
                CExpr_SearchExprTree(stmt->expr.expression, forward_objref, 1, 0x38);
                break;
            default:
                CError_FATAL(3658);
        }
        stmt = stmt->next;
    }
    list = undefined_function_objects;
    return CInline_0050f120(list);
}

#define CERROR_FILE ("CInline.c")

static inline void CInline_EnsurePending(Object *obj)
{
    CPrecNode *p;
    for (p = pending_prec_nodes; p != NULL; p = p->next) {
        if (p->obj == obj)
            return;
    }
    p = (CPrecNode *)galloc(0x20);
    memclrw(p, 0x20);
    p->kind = 3;
    p->obj = obj;
    p->next = pending_prec_nodes;
    pending_prec_nodes = p;
}

static inline void CInline_MovePending(Object *obj)
{
    CPrecNode **prev = &pendingInlineWork;
    CPrecNode *node;
    for (; (node = *prev) != NULL; prev = &node->next) {
        if (obj == node->obj) {
            *prev = node->next;
            ((CPrecNode *)node)->next = pending_prec_nodes;
            pending_prec_nodes = (CPrecNode *)node;
            TYPE_FUNC(obj->type)->flags &= ~0x800000;
            return;
        }
    }
    CError_FATAL(3789);
}

static inline void CInline_0050f240_inline1(Object *a0)
{
    CPrecNode *v4;
    CPrecNode *t2;
    v4 = pending_prec_nodes;
    while ((int)v4 != 0) {
        if (v4->obj == a0) {
            return;
        }
        v4 = v4->next;
    }
    t2 = (CPrecNode *)galloc(32);
    memclrw(t2, 32);
    t2->kind = 3;
    t2->obj = a0;
    t2->next = pending_prec_nodes;
    pending_prec_nodes = t2;
    return;
}

static inline void CInline_0050f240_inline2(Object *a0)
{
    CPrecNode **link;
    CPrecNode *v6;
    link = &pendingInlineWork;
    while ((v6 = *link) != NULL) {
        if (a0 == v6->obj) {
            *link = v6->next;
            ((CPrecNode *)v6)->next = pending_prec_nodes;
            pending_prec_nodes = (CPrecNode *)v6;
            ((TypeFunc *)a0->type)->flags &= 0xff7fffff;
            return;
        }
        link = &v6->next;
    }
    CError_FATAL(3789);
    return;
}

/* 0x573204: file name string */

static inline SInt32 CInline_FindIndex(UInt32 target, Statement **list)
{
    SInt32 i;
    Statement *p;
    UInt32 t = target;

    i = 0;
    p = *list;
    while (p) {
        if ((UInt32)p == t)
            return i;
        p = p->next;
        i++;
    }
    CError_FATAL(2820);
    return 0;
}

static inline void CInline_StoreIndex(IStmtRec *sp, Statement **list, Statement *target)
{
    sp->data.targetIndex = (SInt16)CInline_FindIndex((UInt32)target, list);
}

static inline void CInline_SaveVars(ObjectList *ol, CInlineVar **cursor, UInt8 first, CInlineVar *initial)
{
    Object *o;
    UInt8 v;
    *cursor = initial;

    for (; ol != NULL; ol = ol->next) {
        if (first || ol->object.value->datatype == DLOCAL) {
            (*cursor)->name = ol->object.value->name;
            (*cursor)->type = ol->object.value->type;
            (*cursor)->qual = ol->object.value->qual;
            o = ol->object.value;
            switch ((SInt16)o->sclass) {
                case 0:
                    v = 0;
                    break;
                case 0x101:
                    v = 1;
                    break;
                case 0x100:
                    v = 2;
                    break;
                default:
                    CError_FATAL(1068);
            }
            if (o->flags & 2)
                v |= 0x80;
            (*cursor)->storageFlags = v;
            (*cursor)->used = 0;
            (*cursor)->dirty = first;
            (*cursor)++;
        }
    }
}

static inline ENode *CInline_GetName0(ENode *name)
{
    evalMode = 2;
    memo_list = NULL;
    alloc_state = 1;
    return CInline_00513240(name);
}

static inline ENode *CInline_GetName(ENode *name)
{
    return CInline_GetName0(name);
}

static inline void SaveName(IStmtRec *sp, ENode *name)
{
    ENode *str = CInline_GetName(name);
    CInline_005130b0(str, 0);
    sp->data.operand = str;
}

#pragma opt_lifetimes off

/* 0x573204, "CInline.c" */
/* 0x58245e, register record array pointer */
/* 0x582462, register record array pointer */

/* Map an object reference to its scope slot.  Objects still bound to the
 * argument list come back as negative (0x80000000 | (i+1)) indices. */
static inline SInt32 MapObj(SInt32 arg)
{
    SInt32 obj;
    SInt32 i;
    ObjectList *p;

    obj = arg;
    if (obj != 0) {
        p = arguments, i = 0;
        while (p != NULL) {
            if ((SInt32)p->object.value == obj) {
                data_0058245e[i].used = 1;
                data_0058245e[i].dirty = 0;
                return i + 0x80000001;
            }
            p = p->next;
            i++;
        }

        p = locals, i = 0;
        while (p != NULL) {
            if (p->object.value->datatype == DLOCAL) {
                if ((SInt32)p->object.value == obj)
                    goto found;
                i++;
            }
            p = p->next;
        }
        i = -1;
    found:
        CError_ASSERT(455, i >= 0);
        data_00582462[i].used = 1;
        return i + 1;
    }
    return 0;
}

/* Index of a node inside the inline argument list. */
static inline SInt32 FindIndex(Statement *listp, SInt32 target)
{
    SInt16 i = 0;
    Statement *p = listp->next;

    while (p != NULL) {
        if ((SInt32)p == target)
            goto done;
        p = p->next;
        i++;
    }
    CError_FATAL(2820);
    i = 0;
done:
    return i;
}

#pragma opt_lifetimes reset

/* 0x5824b2 (its -64 preimage is the table at 0x582472) */
#define CERROR_FILE ("CInline.c")

static Object *CInline_MakeTemp(Type *t)
{
    Object *o;
    o = CParser_NewLocalDataObject(NULL, 1);
    o->name = CParser_GetUniqueName();
    o->type = t;
    o->qual = 0;
    set_object_sclass(o, 0);
    CFunc_SetupLocalVarInfo(o);
    return o;
}

static ENode *gen_expr(void *x)
{
    evalMode = 4;
    memo_list = NULL;
    alloc_state = 0;
    return fold_constants(CInline_00513240(x));
}

static ENode *gen_expr_save(void *x)
{
    UInt8 v1;
    UInt8 v0;
    struct MemoNode *v2;

    v1 = alloc_state;
    alloc_state = 1;
    v0 = evalMode;
    evalMode = 4;
    v2 = memo_list;
    memo_list = NULL;
    x = CInline_00513240(x);
    alloc_state = v1;
    evalMode = v0;
    memo_list = v2;
    return (ENode *)x;
}

static void add_chain(ChainRec **head, Statement *node, IStmtRec *ent)
{
    ChainRec *r = (ChainRec *)CompilerTools_AllocatePool(12);
    r->next = *head;
    *head = r;
    r->node = node;
    r->ent = ent;
}

/* 0x58246a, fixup list head */

static inline void CopyStatementExpression(CException *copy, CException *source, char copyExpressions)
{
    copy->data.slots[0] = CInline_GetObjectByIndex(source->data.operands[0].value, copyExpressions);
    copy->data.slots[1] = source->data.slots[1];
}

/* 0x582468, 16-bit counter */
/* 0x584278, 16-bit flag */
/* 0x5842d6, byte flag */

static inline SInt16 inline_statement_count(const CInlineInfo *info)
{
    return info->nstmts;
}

static inline ENode *InlineArgument(ENode *expr)
{
    char savedFlag = alloc_state;
    char savedMode;
    struct MemoNode *savedState;
    ENode *result;
    alloc_state = 1;
    savedMode = evalMode;
    evalMode = 4;
    savedState = memo_list;
    memo_list = NULL;
    result = CInline_00513240(expr);
    alloc_state = savedFlag;
    evalMode = savedMode;
    memo_list = savedState;
    return result;
}

static inline ENode *InlineWrapResult(ENode *expr)
{
    ENode *wrapped;
    if (expr->type == EFORCELOAD) {
        wrapped = expr;
    } else {
        data_005824c3 = 0;
        CExpr_SearchExprTree(expr, fn_005129f0, 3, 4, 54, 55);
        if (data_005824c3 == 0) {
            wrapped = expr;
        } else {
            wrapped = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            *wrapped = *expr;
            wrapped->type = EFORCELOAD;
            wrapped->data.monadic = expr;
        }
    }
    return wrapped;
}

/* 0x573204, "CInline.c" */

/* Create the Object representing one inlined variable. */
static Object *NewInlineVar(Type *type, SInt16 offset, UInt8 info)
{
    Object *obj = CParser_NewLocalDataObject(NULL, 1);

    obj->name = CParser_GetUniqueName();
    obj->type = type;
    obj->qual = offset;
    set_object_sclass(obj, info);
    CFunc_SetupLocalVarInfo(obj);
    return obj;
}

/* 0x573204, "CInline.c" */

/* Copy the node if it is a constant object reference. */
static ENode *CInline_CopyConst(ENode *e)
{
    ENode *r;

    switch (e->type) {
        case EOBJREF:
            r = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            *r = *e;
            return r;
        case EPRECOMP:
            CError_FATAL(1136);
            break;
    }
    return NULL;
}

unsigned char fn_0050ebc0(void)
{
    CPrecNode *node;

    if (anyerrors == 0U) {
        for (node = pending_prec_nodes; node != NULL; node = node->next) {
            if (node->kind == 0U) {
                return 0;
            }
        }
    }
    return 1;
}

void parse_inline_definition(struct CPrecNode *inlineInfo)
{
    Object *object;
    TypeClassExt800 *methodClass;
    TypeClassExt800 *contextClass;
    DeclInfo parseState;
    SInt32 inputState;

    object = inlineInfo->obj;
    CPrep_InsertTokenBuffer(&inlineInfo->u.k0.tokenBuffer, &inputState);
    function_fileinfo = inlineInfo->u.k0.location;
    data_00587184 = data_00588454;
    tk = CPrepTokenizer_GetNextToken();
    switch (tk) {
        default:
            CError_FATAL(4152);
        case ':':
        case '{':
        case TK_TRY: {
            TypeMemberFunc *functionType;
            declaration_token = *CPrep_GetLastBufferedToken();
            functionType = (TypeMemberFunc *)object->type;
            functionType->flags &= ~FUNC_DEFINED;
            if (object->type->type == TYPEFUNC && (((TypeMemberFunc *)object->type)->flags & FUNC_METHOD) != 0 &&
                (methodClass = CInline_0050ebf0_inline1(object)) != NULL) {
                CTemplateNew_ParseFuncDef(object, methodClass, NULL);
            } else {
                memclrw(&parseState, sizeof(parseState));
                if (inlineInfo->u.k0.contextClass != NULL) {
                    if ((contextClass = CInline_0050ebf0_inline2(inlineInfo)) != NULL) {
                        CTemplateNew_ParseFuncDef(object, contextClass, (TplSpec *)inlineInfo->u.k0.contextClass);
                    } else {
                        CFunc_ParseFuncDef(object, &parseState, inlineInfo->u.k0.contextClass, 0, 0, NULL);
                    }
                } else {
                    CFunc_ParseFuncDef(object, &parseState, NULL, 0, 0, NULL);
                }
            }
        }
    }
    CPrep_RemoveBufferedTokens(&inlineInfo->u.k0.tokenBuffer.count, &inputState);
}

static SInt16 CIB_FindIndex(Statement *p, Statement *target)
{
    SInt16 i = 0;
    while (p != NULL) {
        if (p == target)
            return i;
        p = p->next;
        i++;
    }
    CError_FATAL(2820);
    return 0;
}

static ENode *gen_name(ENode *x)
{
    evalMode = 2;
    memo_list = NULL;
    alloc_state = 1;
    return CInline_00513240(x);
}

static SInt32 CInline_Memo(ENode *key)
{
    ENode *k;
    MemoNode *m;
    k = key, m = memo_list;
    for (; m != NULL; m = m->next)
        if (m->key == k)
            return m->val;
    (m = (MemoNode *)CompilerTools_AllocatePool(12))->next = memo_list;
    memo_list = m;
    m->key = key;
    m->val = CParser_GetUniqueID();
    {
        SInt32 result = m->val;
        return result;
    }
}

static SInt32 MemoFirst(ENode *key)
{
    MemoNode *fresh;
    MemoNode *m;
    ENode *k;
    k = key, m = memo_list;
    for (; m; m = m->next)
        if (m->key == k)
            return m->val;
    fresh = (MemoNode *)CompilerTools_AllocatePool(12);
    fresh->next = memo_list;
    memo_list = fresh;
    fresh->key = key;
    fresh->val = CParser_GetUniqueID();
    return fresh->val;
}

static ENode *get_input(SInt32 index)
{
    return data_0058245a[index].expr;
}

static ENode *adjust(ENode *f, ENode *e)
{
    Type *et;
    Type *ft;
    ENode *r = f;
    ft = f->rtype;
    et = e->rtype;
    if (ft != et) {
        if (ft->type == TYPEINT && et->type == TYPEINT)
            r = makemonadicnode(r, 0x30);
        r->rtype = e->rtype;
    }
    return r;
}

/* Payload copied by the ENEWEXCEPTIONARRAY expression case. */

static inline SInt16 inline_member_index(SInt32 key)
{
    Statement *entry = inline_statements;
    SInt16 index = 0;
    for (; entry != NULL; entry = entry->next, index++)
        if (entry->type == ST_LABEL && ((SInt32 *)entry->target.label)[2] == key)
            return index;
    CError_FATAL(629);
    return 0;
}

static inline void inline_local_index(Object *object, int *index)
{
    ObjectList *local = locals;
    *index = 0;
    while (local != NULL) {
        if (local->object.value->datatype == DLOCAL) {
            if (local->object.value == object)
                return;
            (*index)++;
        }
        local = local->next;
    }
    *index = -1;
}

static inline int FindInlineObjectIndex(void *object)
{
    ObjectList *candidate;
    int index;

    candidate = locals;
    for (index = 0; candidate != NULL; candidate = candidate->next) {
        if (candidate->object.value->datatype == DLOCAL) {
            if (candidate->object.value == object)
                return index;
            index++;
        }
    }
    return -1;
}

static inline Boolean InlineObjectModifiable(Object *obj)
{
    if (obj->datatype == DLOCAL && !(obj->flags & 2))
        return 0;
    if ((signed char)obj->type->type >= 11) {
        if (((TypePointer *)obj->type)->qual & Q_CONST)
            return 0;
    } else if (obj->qual & Q_CONST)
        return 0;
    return 1;
}

static inline ENode *CInline_ArrayInitializer(ENode *expr)
{
    return ((ENodeList *)expr->data.newexception.initexpr)->node;
}

void make_auto_generated_method(Object *func)
{
    TypeClass *tclass;

    CError_ASSERT(4062, TYPE_METHOD(func->type)->flags & FUNC_AUTO_GENERATED);
    CError_ASSERT(4063, TYPE_METHOD(func->type)->flags & FUNC_METHOD);

    tclass = TYPE_METHOD(func->type)->theclass;

    if (func == CClass_DefaultConstructor(tclass)) {
        if (func->u.func.defargdata != NULL)
            CABI_MakeDefaultArgConstructor(tclass, func);
        else
            CABI_GenerateClassFunction(tclass, func);
        return;
    }
    if (func == CClass_CopyConstructor(tclass)) {
        CABI_GenClassFunction(tclass, func);
        return;
    }
    if (func == CClass_AssignmentOperator(tclass)) {
        CABI_MakeDefaultConstructor(tclass, func);
        return;
    }
    if (func == CClass_Destructor(tclass)) {
        CABI_MakeDefaultDestructor(tclass, func);
        return;
    }
    CError_FATAL(4097);
}

void CInline_0050ee60(Statement *stmt, Object *func, Boolean flag)
{
    Boolean autoInline;
    Boolean isInline;
    CInlineInfo *tmp;

    TYPE_FUNC(func->type)->flags |= FUNC_DEFINED;
    isInline = autoInline = 0;

    if (!(func->qual & Q_INLINE)) {
        if (copts.f70 && !copts.disableInlining && fn_00511180(func, stmt->next) &&
            check_statement_count_and_locals_size(func, stmt->next)) {
            isInline = autoInline = 1;
            TYPE_FUNC(func->type)->flags |= FUNC_IS_CTOR;
        }
    } else {
        isInline = 1;
    }

    if (isInline) {
        DumpIR_OptimizeStatementList(func, stmt);
        tmp = galloc(0x2a);
        CInline_SaveInfo(tmp, stmt, func);
        func->u.func.u = tmp;
        if (!autoInline && !(func->flags & 2)) {
            if (dispatching_deferred_node) {
                undefined_function_objects = NULL;
                CInline_Cleanup(stmt);
            }
            return;
        }
    }

    func->flags |= 4;
    undefined_function_objects = NULL;

    if (CInline_Cleanup(stmt) || copts.f71) {
        CInlineInfo *body;
        InlineNode *node;

        if (!isInline) {
            body = galloc(0x2a);
            CInline_SaveInfo(body, stmt, func);
        } else {
            body = func->u.func.u;
        }
        node = (InlineNode *)galloc(0xe);
        node->func = func;
        node->body = body;
        node->flag = flag;
        node->next = deferredInlineNodes;
        deferredInlineNodes = node;
    } else {
        inline_statement_list(stmt);
        if (copts.filesyminfo)
            fn_0043f1f0(&function_fileinfo);
        if (!anyerrors)
            CodeGen_Generator(stmt, func, flag, 0);
    }
}

Boolean check_statement_count_and_locals_size(Object *func, Statement *stmt)
{
    ObjectList *list;
    SInt32 count = 0;
    SInt32 size;
    UInt8 type;

    while (stmt != NULL) {
        if ((type = stmt->type) != ST_NOP && type != ST_LABEL)
            count++;
        if (count > 15)
            return 0;
        stmt = stmt->next;
    }

    for (list = locals, size = 0; list != NULL; list = list->next)
        size += list->object.value->type->size;

    if (size > 0x400)
        return 0;
    return 1;
}

Boolean CInline_0050f120(struct InlineObjectEntry *list)
{
    Boolean result = 0;
    while (list != NULL) {
        Object *obj = list->object;
        CPrecNode *p;
        if ((TYPE_FUNC(obj->type)->flags & FUNC_AUTO_GENERATED) && !(TYPE_FUNC(obj->type)->flags & FUNC_DEFINED)) {
            CInline_EnsurePending(obj);
            result = 1;
        } else if ((obj->qual & Q_IS_TEMPLATED) != 0 && CTemplateNew_InstantiateInlineTemplateObject(obj)) {
            result = 1;
        } else {
            for (p = pending_prec_nodes; p != NULL; p = p->next) {
                if (obj == p->obj) {
                    result = 1;
                    break;
                }
            }
            if ((TYPE_FUNC(obj->type)->flags & 0x800000) != 0) {
                CInline_MovePending(obj);
                result = 1;
            }
        }
        list = list->next;
    }
    return result;
}

/* An inline function awaiting expansion. */

void CInline_0050f240(Object *object)
{
    CInlineInfo *body;
    InlineNode *pending;
    UInt32 functionFlags;
    TypeFunc *functionType;
    object->flags |= OBJECT_FLAGS_2;
    switch (object->datatype) {
        case DFUNC:
        case DVFUNC:
            if ((object->qual & Q_INLINE) != 0) {
                if ((body = (CInlineInfo *)object->u.func.u) != NULL && (object->flags & OBJECT_DEFINED) == 0 &&
                    ((functionType = (TypeFunc *)object->type)->flags & 1024) == 0) {
                    pending = (InlineNode *)galloc(sizeof(InlineNode));
                    pending->func = object;
                    pending->body = body;
                    pending->flag = 0;
                    pending->next = deferredInlineNodes;
                    deferredInlineNodes = pending;
                    object->flags |= OBJECT_DEFINED;
                    return;
                }
            }
            functionType = (TypeFunc *)object->type;
            if (((functionFlags = functionType->flags) & FUNC_AUTO_GENERATED) != 0 &&
                (functionFlags & FUNC_DEFINED) == 0) {
                CInline_0050f240_inline1(object);
                return;
            }
            if ((functionFlags & 8388608) != 0) {
                CInline_0050f240_inline2(object);
                return;
            }
            return;
        case DALIAS:
            CInline_0050f240(object->u.alias.object);
            return;
        case DDATA:
            if ((object->qual & Q_INLINE_DATA) != 0) {
                CInit_ExportConst(object);
            }
            if ((object->flags & OBJECT_LAZY) != 0) {
                object->flags &= ~OBJECT_LAZY;
                CParser_CallBackAction(object);
            }
            return;
        default:
            return;
    }
}

void collect_undefined_function_objects(CInlineInfo *inlineData)
{
    SInt16 statementIndex;
    CException *action;

    for (statementIndex = 0; statementIndex < (SInt16)inlineData->nstmts; statementIndex++) {
        switch (inlineData->stmtinfo[statementIndex].type) {
            case 4:
            case 12:
            case 13:
            case 14:
            case 15:
                CExpr_SearchExprTree((ENode *)inlineData->stmtinfo[statementIndex].data.operand, forward_objref, 1,
                                     0x38);
                break;
            case 8:
                if (inlineData->stmtinfo[statementIndex].data.operand != NULL)
                    CExpr_SearchExprTree((ENode *)inlineData->stmtinfo[statementIndex].data.operand, forward_objref, 1,
                                         0x38);
                break;
            case 6:
            case 7:
                CExpr_SearchExprTree((ENode *)inlineData->stmtinfo[statementIndex].data.operand, forward_objref, 1,
                                     0x38);
                break;
            case 5:
                CExpr_SearchExprTree(
                    ((InlineSwitchData *)inlineData->stmtinfo[statementIndex].data.operand)->expression, forward_objref,
                    1, 0x38);
                break;
            case 1:
            case 2:
            case 3:
            case 16:
                break;
            default:
                CError_FATAL(3710);
        }
        for (action = (CException *)inlineData->stmtinfo[statementIndex].exceptionActions; action != NULL;
             action = action->next) {
            switch (action->kind) {
                case 1:
                    add_undefined_function_object(action->data.local.dtor);
                    break;
                case 2:
                    add_undefined_function_object(action->data.local_cond.dtor);
                    break;
                case 3:
                    add_undefined_function_object(action->data.local_pointer.dtor);
                    break;
                case 4:
                    add_undefined_function_object(action->data.member_array.dtor);
                    break;
                case 5:
                    add_undefined_function_object(action->data.member.dtor);
                    break;
                case 7:
                case 17:
                    add_undefined_function_object(action->data.delete_pointer.deletefunc);
                    break;
                case 8:
                    add_undefined_function_object(action->data.member_cond.dtor);
                    break;
                case 9:
                    add_undefined_function_object(action->data.member_array.dtor);
                    break;
                case 10:
                case 11:
                    add_undefined_function_object(action->data.pair.second);
                    break;
                case 12:
                    add_undefined_function_object(action->data.delete_pointer_cond.deletefunc);
                    break;
                case 6:
                case 13:
                case 14:
                case 15:
                case 16:
                    break;
                default:
                    CError_FATAL(3760);
            }
        }
    }
}

/* Linked list node: next at 0x00, statement kind byte at 0x04, two
 * pointer members at 0x0a and 0x12. */

void forward_statement_objrefs(Statement *stmt)
{
    while (stmt != NULL) {
        if (stmt->dobjstack != NULL)
            add_undefined_exception_function_objects(stmt->dobjstack);
        switch (stmt->type) {
            case ST_NOP:
            case ST_LABEL:
            case ST_GOTO:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_ASM:
                break;
            case ST_RETURN:
                if (stmt->expr.expression == NULL)
                    break;
            case ST_EXPRESSION:
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_GOTOEXPR:
                CExpr_SearchExprTree(stmt->expr.expression, forward_objref, 1, 0x38);
                break;
            default:
                CError_FATAL(3658);
        }
        stmt = stmt->next;
    }
}

void forward_objref(ENode *expr)
{
    Object *object;
    object = expr->data.objref;
    add_undefined_function_object(object);
}

/* Linked entries visited by the inline traversal. */

void add_undefined_exception_function_objects(CException *entry)
{
    if (entry != NULL) {
        do {
            switch (entry->kind) {
                case 1:
                    add_undefined_function_object(entry->data.local.dtor);
                    break;
                case 2:
                    add_undefined_function_object(entry->data.local_cond.dtor);
                    break;
                case 3:
                    add_undefined_function_object(entry->data.local.dtor);
                    break;
                case 4:
                    add_undefined_function_object((Object *)entry->data.slots[1]);
                    break;
                case 5:
                    add_undefined_function_object((Object *)entry->data.slots[1]);
                    break;
                case 6:
                    add_undefined_function_object((Object *)entry->data.slots[2]);
                    break;
                case 7:
                case 17:
                    add_undefined_function_object((Object *)entry->data.slots[1]);
                    break;
                case 8:
                    add_undefined_function_object((Object *)entry->data.slots[2]);
                    break;
                case 9:
                    add_undefined_function_object((Object *)entry->data.slots[1]);
                    break;
                case 10:
                case 11:
                    add_undefined_function_object((Object *)entry->data.slots[1]);
                    break;
                case 12:
                    add_undefined_function_object((Object *)entry->data.slots[1]);
                    break;
                case 13:
                case 14:
                case 15:
                case 16:
                    break;
                default:
                    CError_FATAL(3597);
            }
            entry = entry->next;
        } while (entry != NULL);
    }
}

/* Objects queued for inline processing. */

void add_undefined_function_object(Object *object)
{
    InlineObjectEntry *entry;
    InlineObjectEntry *newEntry;
    if (!(((object->datatype == DFUNC) || (object->datatype == DVFUNC)) && ((object->flags & OBJECT_DEFINED) == 0) &&
          ((object->type->type != TYPEFUNC) || ((((TypeFunc *)object->type)->flags & 0x400) == 0)))) {
        return;
    }
    entry = undefined_function_objects;
    if (undefined_function_objects != NULL) {
        do {
            if (entry->object == object) {
                return;
            }
            entry = entry->next;
        } while (entry != NULL);
    }
    newEntry = (InlineObjectEntry *)CompilerTools_AllocatePool(sizeof(InlineObjectEntry));
    newEntry->object = object;
    newEntry->next = undefined_function_objects;
    undefined_function_objects = newEntry;
    if ((object->qual & Q_INLINE) != 0 && (object->u.func.u != NULL)) {
        collect_undefined_function_objects(object->u.func.u);
    }
}

/* Record allocated by the original: 0x20 bytes, fields at 0x00 (next),
 * 0x04 (function object), 0x08 / 0x0c (two stored pointers) and a byte
 * state at 0x1e. */

void CInline_AddSpecialization(Object *func, void *definition, void *specialization)
{
    CPrecNode *work;

    for (work = pendingInlineWork; work != NULL; work = work->next) {
        if (work->obj == func)
            return;
    }

    work = galloc(sizeof(*work));
    memclrw(work, sizeof(*work));
    work->kind = 2;
    work->obj = func;
    work->u.k2.definition = definition;
    work->u.k2.specialization = specialization;
    work->next = pendingInlineWork;
    pendingInlineWork = work;
    TYPE_FUNC(func->type)->flags |= 0x800000;
}

void CInline_AddFunctionPrecNode(Object *func, TypeClass *value, FOI *key, PrepTokenBuffer *pair, Boolean flag)
{
    CPrecNode *entry = flag ? pendingInlineWork : pending_prec_nodes;
    CPrecNode *node;

    while (entry != NULL) {
        if (entry->obj == func)
            return;
        entry = entry->next;
    }

    CError_ASSERT(3422, func->type->type == TYPEFUNC);
    TYPE_FUNC(func->type)->flags |= 0x8000000;

    node = (CPrecNode *)galloc(sizeof(CPrecNode));
    memclrw(node, sizeof(CPrecNode));
    node->kind = 0;
    node->obj = func;
    node->u.k0.contextClass = value;
    node->u.k0.location = *key;
    node->u.k0.tokenBuffer = *pair;
    if (flag) {
        node->next = pendingInlineWork;
        pendingInlineWork = (CPrecNode *)node;
        TYPE_FUNC(func->type)->flags |= 0x800000;
    } else {
        node->next = pending_prec_nodes;
        pending_prec_nodes = (CPrecNode *)node;
    }
}

void generate_inline_code(Object *object, CInlineInfo *input, char mode)
{
    char savedFlag;
    Statement statement;
    CScopeSave savedScope;

    if (cprep_cu[0xe0] == 1) {
        return;
    }
    if (input == NULL) {
        return;
    }
    fn_0048b2c0();
    CScope_SetFunctionScope(object, &savedScope);
    CFunc_FuncGenSetup(&statement, object);
    CInline_ReconstructFunction(object, input, &statement);
    savedFlag = copts.filesyminfo;
    if ((copts.fbf != 0) || ((data_00587184 == 0 && (function_token_line == 0)))) {
        copts.filesyminfo = 0;
    }
    inline_statement_list(&statement);
    if (anyerrors == 0) {
        if (copts.filesyminfo != 0) {
            fn_0043f1f0(&function_fileinfo);
        }
        CodeGen_Generator(&statement, object, mode, 0);
    }
    CScope_RestoreScope(&savedScope);
    copts.filesyminfo = savedFlag;
}

void CInline_ReconstructFunction(Object *function, CInlineInfo *rec, Statement *out)
{
    CInlineVar *arg;
    CLabel **table;
    Statement *cursor;
    IStmtRec *record;
    Statement *stmt;
    SInt32 i;
    ObjectList *node;
    Object *obj;
    ENode *expression;

    i = 0;
    function_fileinfo = rec->fileinfo;
    data_00587184 = rec->f1c;
    function_tokenoffset = rec->tokenoffset;
    function_token_line = rec->tokenline;

    arg = rec->arginfo;
    for (i = 0; i < rec->nargs; i++, arg++) {
        if (i == 0) {
            arguments = node = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
        } else {
            node = node->next = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
        }
        obj = (Object *)galloc(sizeof(Object));
        memclrw(obj, sizeof(Object));
        node->object.value = obj;
        node->next = NULL;
        {
            UInt8 flags;
            obj->otype = OT_OBJECT;
            obj->access = ACCESSPUBLIC;
            obj->datatype = DLOCAL;
            obj->name = arg->name;
            obj->type = arg->type;
            obj->qual = arg->qual;
            flags = arg->storageFlags;
            if (flags & 0x80) {
                obj->flags |= 2;
                flags &= 0x7f;
            }
            switch (flags) {
                case 0:
                    obj->sclass = TK_EOF;
                    break;
                case 1:
                    obj->sclass = TK_REGISTER;
                    break;
                case 2:
                    obj->sclass = TK_AUTO;
                    break;
                default:
                    CError_FATAL(1093);
            }
            CFunc_SetupLocalVarInfo(obj);
        }
        if (rec->fileinfo.file) {
            obj->u.var.info->deftoken.tokenfile = (struct PFile *)rec->fileinfo.file;
            obj->u.var.info->deftoken.tokenoffset = rec->tokenoffset;
        }
    }

    arg = rec->localinfo;
    for (i = 0; i < rec->nlocals; i++, arg++) {
        if (i == 0) {
            locals = node = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
        } else {
            node = node->next = (ObjectList *)CompilerTools_AllocatePool(sizeof(ObjectList));
        }
        obj = (Object *)galloc(sizeof(Object));
        memclrw(obj, sizeof(Object));
        node->object.value = obj;
        node->next = NULL;
        {
            UInt8 flags;
            obj->otype = OT_OBJECT;
            obj->access = ACCESSPUBLIC;
            obj->datatype = DLOCAL;
            obj->name = arg->name;
            obj->type = arg->type;
            obj->qual = arg->qual;
            flags = arg->storageFlags;
            if (flags & 0x80) {
                obj->flags |= 2;
                flags &= 0x7f;
            }
            switch (flags) {
                case 0:
                    obj->sclass = TK_EOF;
                    break;
                case 1:
                    obj->sclass = TK_REGISTER;
                    break;
                case 2:
                    obj->sclass = TK_AUTO;
                    break;
                default:
                    CError_FATAL(1093);
            }
            CFunc_SetupLocalVarInfo(obj);
        }
        if (rec->fileinfo.file) {
            obj->u.var.info->deftoken.tokenfile = (struct PFile *)rec->fileinfo.file;
            obj->u.var.info->deftoken.tokenoffset = rec->tokenoffset;
        }
    }

    fixup_list = NULL;
    table = (CLabel **)CompilerTools_AllocatePool((SInt16)rec->nstmts * sizeof(*table));
    memclrw(table, (SInt16)rec->nstmts * sizeof(*table));
    for (i = 0, cursor = out, record = rec->stmtinfo; i < (SInt16)rec->nstmts; i++, record++) {
        cursor->next = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
        cursor = cursor->next;
        cursor->type = record->type;
        cursor->value = record->value;
        cursor->flags = record->flags;
        cursor->sourceoffset = record->sourceoffset;
        cursor->dobjstack = copy_exception_actions(record, 0);
        cursor->next = NULL;
        switch (cursor->type) {
            case ST_NOP:
            case ST_GOTO:
                break;
            case ST_EXPRESSION:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_GOTOEXPR:
                expression = (ENode *)record->data.operand;
                evalMode = 3;
                memo_list = NULL;
                alloc_state = 0;
                cursor->expr.expression = CInline_00513240(expression);
                break;
            case ST_RETURN:
                if ((expression = (ENode *)record->data.operand) != NULL) {
                    evalMode = 3;
                    memo_list = NULL;
                    alloc_state = 0;
                    cursor->expr.expression = CInline_00513240(expression);
                } else {
                    cursor->expr.expression = NULL;
                }
                break;
            case ST_LABEL:
                cursor->target.label = newlabel();
                table[i] = cursor->target.label;
                cursor->target.label->target.stmt = cursor;
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                expression = (ENode *)record->data.operand;
                evalMode = 3;
                memo_list = NULL;
                alloc_state = 0;
                cursor->expr.expression = CInline_00513240(expression);
                break;
            case ST_SWITCH:
                expression = record->data.switchInfo->expression;
                evalMode = 3;
                memo_list = NULL;
                alloc_state = 0;
                cursor->expr.expression = CInline_00513240(expression);
                break;
            case ST_ASM:
                break;
            default:
                CError_FATAL(3305);
                break;
        }
    }

    stmt = out->next;
    record = rec->stmtinfo;
    while (stmt != NULL) {
        switch (stmt->type) {
            case ST_GOTO:
                if ((stmt->target.label = table[record->data.targetIndex]) == NULL)
                    CError_FATAL(3312);
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                if ((stmt->target.label = table[record->secondaryOperand.targetIndex]) == NULL)
                    CError_FATAL(3317);
                break;
            case ST_SWITCH:
                reconstruct_switch_info(stmt, record, table);
                break;
            case ST_ASM:
                InlineAsm_CopyInstructionAndResolveOperands(stmt, table, 0, record->data.assembly,
                                                            record->secondaryOperand.assemblyData);
                break;
        }
        stmt = stmt->next;
        record++;
    }

    inline_statements = out->next;
    while (fixup_list != NULL) {
        if (!(*fixup_list->destination = table[fixup_list->labelIndex]))
            CError_FATAL(3333);
        (void)table;
        (void)table;
        fixup_list = fixup_list->next;
    }
}

void CInline_SaveInfo(CInlineInfo *out, Statement *list, Object *function)
{
    Statement *statement;
    CInlineVar *savedVar;
    IStmtRec *savedStatement;
    ObjectList *objects;
    ENode *expression;
    SInt32 count, nlocals;

    inline_statements = list->next;
    memclrw(out, sizeof(*out));
    out->kind = fn_00511180(function, list->next);
    if (copts.filesyminfo) {
        out->fileinfo = function_fileinfo;
        out->fileinfo.isInline = 1;
        out->f1c = data_00587184;
        out->tokenoffset = function_tokenoffset;
        out->tokenline = function_token_line;
    }
    for (objects = arguments, count = 0; objects != NULL; objects = objects->next)
        count++;
    out->nargs = count;
    if (out->nargs > 0) {
        out->arginfo = galloc(count * sizeof(CInlineVar));
        data_0058245e = out->arginfo;
        memclrw(out->arginfo, count * sizeof(CInlineVar));
        CInline_SaveVars(arguments, &savedVar, 1, out->arginfo);
    }
    for (objects = locals, nlocals = 0; objects != NULL; objects = objects->next)
        if (objects->object.value->datatype == DLOCAL)
            nlocals++;
    out->nlocals = nlocals;
    if (out->nlocals > 0) {
        out->localinfo = galloc(nlocals * sizeof(CInlineVar));
        data_00582462 = out->localinfo;
        memclrw(out->localinfo, nlocals * sizeof(CInlineVar));
        CInline_SaveVars(locals, &savedVar, 0, out->localinfo);
    }
    count = 0;
    for (statement = list->next; statement != NULL; statement = statement->next)
        count++;
    out->nstmts = count;
    out->stmtinfo = galloc(count * sizeof(IStmtRec));
    statement = list->next;
    savedStatement = out->stmtinfo;
    for (; statement != NULL; statement = statement->next) {
        savedStatement->type = statement->type;
        savedStatement->flags = statement->flags;
        savedStatement->value = statement->value;
        savedStatement->exceptionActions = CInline_005102f0(list, statement);
        savedStatement->sourceoffset = statement->sourceoffset;
        switch (statement->type) {
            case ST_EXPRESSION:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_GOTOEXPR:
                SaveName(savedStatement, statement->expr.expression);
                break;
            case ST_RETURN:
                if (statement->expr.expression) {
                    expression = CInline_GetName(statement->expr.expression);
                    CInline_005130b0(expression, 0);
                    savedStatement->data.operand = expression;
                } else {
                    savedStatement->data.operand = NULL;
                }
                break;
            case ST_GOTO:
                CInline_StoreIndex(savedStatement, (Statement **)list, statement->target.label->target.stmt);
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                expression = CInline_GetName(statement->expr.expression);
                CInline_005130b0(expression, 0);
                savedStatement->data.operand = expression;
                savedStatement->secondaryOperand.targetIndex =
                    CInline_FindIndex((UInt32)statement->target.label->target.stmt, (Statement **)list);
                break;
            case ST_SWITCH:
                savedStatement->data.switchInfo = create_inline_switch_data(list->next, statement);
                break;
            case ST_ASM:
                InlineAsm_CopyAndRemapParsedAsmInstruction(statement, list->next, &savedStatement->data.assembly,
                                                           &savedStatement->secondaryOperand.assemblyData);
                break;
            case ST_LABEL:
            case ST_NOP:
                break;
            default:
                CError_FATAL(3160);
        }
        savedStatement++;
    }
}

#pragma opt_lifetimes off

/* Inline records carry kind-dependent object, value, and index operands. */

CException *CInline_005102f0(Statement *indexMap, Statement *info)
{
    CException *src = info->dobjstack; /* CInline_005102f0: serialization view of exception actions */
    CException *dst = NULL;

    while (src != NULL) {
        CException *copy = (CException *)galloc(0x1e);

        copy->next = dst;
        dst = copy;
        copy->kind = src->kind;
        switch (src->kind) {
            case 1:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                break;
            case 2:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[2].value = src->data.operands[2].value;
                copy->data.operands[1].value = MapObj(src->data.operands[1].value);
                break;
            case 3:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                copy->data.operands[2].value = src->data.operands[2].value;
                break;
            case 4:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                break;
            case 5:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                copy->data.operands[2].value = src->data.operands[2].value;
                copy->data.operands[3].value = src->data.operands[3].value;
                break;
            case 6:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = MapObj(src->data.operands[1].value);
                copy->data.operands[2].value = MapObj(src->data.operands[2].value);
                copy->data.operands[3].value = MapObj(src->data.operands[3].value);
                break;
            case 7:
            case 0x11:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                copy->data.operands[2].value = src->data.operands[2].value;
                break;
            case 8:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = MapObj(src->data.operands[1].value);
                copy->data.operands[2].value = src->data.operands[2].value;
                copy->data.operands[3].value = src->data.operands[3].value;
                break;
            case 9:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                copy->data.operands[2].value = src->data.operands[2].value;
                copy->data.operands[3].value = src->data.operands[3].value;
                copy->data.operands[4].value = src->data.operands[4].value;
                break;
            case 10:
            case 11:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                break;
            case 12:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = src->data.operands[1].value;
                copy->data.operands[2].value = MapObj(src->data.operands[2].value);
                break;
            case 13:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].value = MapObj(src->data.operands[1].value);
                copy->data.operands[2].value = FindIndex(indexMap, src->data.operands[2].reference->index);
                copy->data.operands[3].value = src->data.operands[3].value;
                copy->data.operands[4].value = src->data.operands[4].value;
                copy->data.operands[5].value = src->data.operands[5].value;
                break;
            case 14:
                copy->data.operands[0].value = MapObj(src->data.operands[0].value);
                copy->data.operands[1].byte = src->data.operands[1].byte;
                break;
            case 15:
                copy->data.operands[0].value = src->data.operands[0].value;
                copy->data.operands[1].value = src->data.operands[1].value;
                copy->data.operands[2].value = FindIndex(indexMap, src->data.operands[2].reference->index);
                copy->data.operands[3].value = MapObj(src->data.operands[3].value);
                break;
            case 16:
                break;
            default:
                CError_FATAL(3023);
                break;
        }
        src = src->next;
    }
    return dst;
}

#pragma opt_lifetimes reset

unsigned char fn_00511180(Object *function, Statement *statement)
{
    FuncArg *arg;
    Boolean status;
    unsigned char result;

    status = CMachine_FunctionRequiresMemoryReturn((TypeFunc *)function->type);
    if (status != 0) {
        if (status != 1 || (((TypeFunc *)function->type)->functype->type == TYPECLASS &&
                            CClass_Destructor((TypeClass *)((TypeFunc *)function->type)->functype) != NULL))
            return 0;
    }
    for (arg = ((TypeFunc *)function->type)->args; arg != NULL; arg = arg->next) {
        if (arg == &data_00583098)
            return 0;
        if (arg == &data_00584748)
            break;
        if (arg->type->type == TYPECLASS) {
            if (CClass_Destructor((TypeClass *)arg->type) != NULL)
                return 0;
        }
    }
    result = 6;
    for (; statement != NULL; statement = statement->next) {
        if (statement->dobjstack != NULL)
            return 3;
        switch (statement->type) {
            case ST_EXPRESSION:
                break;
            case ST_RETURN:
                if (statement->next == NULL) {
                    if (statement->expr.expression != NULL)
                        break;
                    if (((TypeFunc *)function->type)->functype == &stvoid)
                        break;
                }
            default:
                result = 3;
                break;
        }
    }
    return result;
}

void *create_inline_switch_data(Statement *base, Statement *classInfo)
{
    SwitchInfo *list;
    ENode *name;
    SwitchCase *node;
    InlineSwitchData *result;
    SInt16 count;

    list =
        classInfo->target.switchDescriptor /* create_inline_switch_data: ST_SWITCH stores its switch descriptor here */;

    count = 0;
    for (node = list->cases; node != NULL; node = node->next)
        count++;

    result = (InlineSwitchData *)galloc(count * 10 + 12);

    name = gen_name(classInfo->expr.expression);
    CInline_005130b0(name, 0);
    result->expression = name;

    result->defaultStatementIndex = CIB_FindIndex(base, list->defaultlabel->target.stmt);
    result->valueType = list->sizetype;
    result->caseCount = count;

    count = 0;
    for (node = list->cases; node != NULL; node = node->next) {
        result->entries[count].statementIndex = CIB_FindIndex(base, node->label->target.stmt);
        result->entries[count].caseValue = node->min;
        count++;
    }
    return result;
}

/* Link used to traverse the inline list. */

SInt16 CInline_GetStatementIndex(Statement *link, Statement *target)
{
    UInt16 index;

    index = 0;
    if (link != NULL) {
        do {
            if (link == target) {
                return index;
            }
            link = link->next;
            index = index + 1;
        } while (link != NULL);
    }
    CError_FATAL(2820);
    return 0;
}

void inline_statement_list(Statement *list)
{
    SInt16 limit;
    Statement *statement;
    Statement *result;
    struct CPrepCU *compilation;

    if (copts.disableInlining == 0 && copts.inlineLimit >= 0) {
        data_00582468 = 0;
        do {
            data_00582467 = 0;
            for (statement = list; statement != NULL; statement = statement->next) {
                switch (statement->type) {
                    case ST_RETURN:
                        if (statement->expr.expression == NULL)
                            break;
                        /* fall through */
                    case ST_EXPRESSION:
                    case ST_SWITCH:
                    case ST_IFGOTO:
                    case ST_IFNGOTO:
                    case ST_GOTOEXPR:
                        result = inline_statement(statement);
                        statement = result;
                        break;
                    case ST_NOP:
                    case ST_LABEL:
                    case ST_GOTO:
                    case ST_BEGINCATCH:
                    case ST_ENDCATCH:
                    case ST_ENDCATCHDTOR:
                    case ST_ASM:
                        break;
                    default:
                        CError_FATAL(2742);
                }
            }
            if (data_00582467 == 0)
                break;
            if (copts.fb6 == 0) {
                if ((limit = copts.inlineLimit) == 0) {
                    if (data_00582468 >= 3)
                        break;
                } else if (data_00582468 + 1 >= limit) {
                    break;
                }
            }
            compilation = (struct CPrepCU *)cprep_cu;
            if (CPrep_CallCompilerCallback(compilation->context, line_count) != 0)
                CError_Longjmp();
            data_00582468++;
        } while (1);
    }
    for (statement = list; statement != NULL; statement = statement->next) {
        if (statement->dobjstack != NULL)
            CException_004e35b0(statement->dobjstack);
        switch (statement->type) {
            case ST_RETURN:
                if (statement->expr.expression == NULL)
                    break;
                /* fall through */
            case ST_EXPRESSION:
            case ST_SWITCH:
            case ST_IFGOTO:
            case ST_IFNGOTO:
            case ST_GOTOEXPR:
                CInline_005114e0(statement->expr.expression);
                break;
            case ST_NOP:
            case ST_LABEL:
            case ST_GOTO:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_ASM:
                break;
            default:
                CError_FATAL(2804);
        }
    }
}

/* Auxiliary member-pointer target metadata; only the flags are accessed here. */

/* Auxiliary data attached to an ELOCOBJ expression. */

void CInline_005114e0(ENode *node)
{
    TypeClass *value1;
    UInt32 value2;

    for (;;) {
        switch (node->type) {
            case EOBJREF:
                CInline_0050f240(node->data.objref);
                if (node->data.objref->datatype == TYPEFUNC) {
                    CExpr_AliasTransform(node);
                    break;
                }
                return;

            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
            case EINDIRECT:
            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case EFORCELOAD:
            case ETYPCON:
            case EBITFIELD:
                node = node->data.monadic;
                break;

            case EMUL:
            case EMULV:
            case EDIV:
            case EMODULO:
            case EADDV:
            case ESUBV:
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
            case EPMODULO:
            case EROTL:
            case EROTR:
            case EBCLR:
            case EBTST:
            case EBSET:
                CInline_005114e0(node->data.diadic.left);
                node = node->data.diadic.right;
                break;

            case EINTCONST:
            case EFLOATCONST:
            case ESTRINGCONST:
            case ENULLCHECK:
            case EMEMBER:
            case EASSBLK:
                return;

            case ELOCOBJ: {
                InlineMemberPointerTarget *target;
                if ((target = node->data.memberFunctionPointer->target) != NULL)
                    target->flags |= 1;
                return;
            }

            case EFUNCCALL:
            case EFUNCCALLP: {
                ENodeList *argument;
                for (argument = node->data.funccall.args; argument != NULL; argument = argument->next)
                    CInline_005114e0(argument->node);
                node = node->data.funccall.funcref;
                if (copts.fa8 && !copts.disableInlining && node->type == EOBJREF &&
                    (node->data.objref->qual & Q_INLINE) && node->data.objref->datatype != TYPECLASS &&
                    !CParser_IsVirtualFunction(node->data.objref, &value1, &value2))
                    CError_Warning(ERR_INLINE_FUNCTION_CALL_NOT_INLINED, node->data.objref);
                break;
            }

            case EMFPOINTER:
                CInline_005114e0(node->data.diadic.left);
                node = node->data.diadic.right;
                break;

            case EQUALNAME:
                *node = *nullnode();
                break;

            case ECOND:
                CInline_005114e0(node->data.cond.cond);
                CInline_005114e0(node->data.cond.expr1);
                node = node->data.cond.expr2;
                break;

            case ENEWEXCEPTIONARRAY: {
                ENode *expression;
                if ((expression = node->data.memberfunc->expression) != NULL) {
                    *node = *expression;
                    break;
                }
            }
                /* fall through */
            case ENEWEXCEPTION:
                *node = *nullnode();
                break;

            default:
                CError_FATAL(2682);
                break;
        }
    }
}

Statement *inline_statement(Statement *statement)
{
    ENode *expression;
    Statement *result;
    Statement *split;
    char changed;
    do {
        changed = 0;
        if (statement->type == ST_EXPRESSION && ((ENode *)statement->expr.expression)->type == 4 &&
            CParser_IsVolatile(((ENode *)statement->expr.expression)->rtype,
                               ((ENode *)statement->expr.expression)->flags & 3) == 0) {
            statement->expr.expression = ((ENode *)statement->expr.expression)->data.diadic.left;
            changed = 1;
            if ((char)((ENode *)statement->expr.expression)->type == 56 ||
                (char)((ENode *)statement->expr.expression)->type == 49) {
                statement->expr.expression = nullnode();
            }
        }
        if (((ENode *)statement->expr.expression)->type == 41) {
            split = (Statement *)CompilerTools_AllocatePool(sizeof(Statement));
            *split = *statement;
            statement->next = split;
            statement->type = ST_EXPRESSION;
            statement->expr.expression = ((ENode *)statement->expr.expression)->data.diadic.left;
            split->expr.expression = ((ENode *)split->expr.expression)->data.diadic.right;
            changed = 1;
        }
    } while (changed != 0);
    if (((expression = (ENode *)statement->expr.expression)->type == 54 || expression->type == EFUNCCALLP) &&
        ((ENode *)expression->data.diadic.left)->type == 56 && can_inline(expression->data.diadic.left) != 0) {
        result = try_inline_statement(statement, &changed);
        statement = result;
        if (changed != 0) {
            data_00582467 = 1;
            return result;
        }
    }
    inline_call_seen = 0;
    inline_statement_mode = 1;
    data_005824b5 = 0;
    inline_call_count = 0;
    statement->expr.expression = inline_expression(statement->expr.expression);
    if (inline_call_seen != 0) {
        statement->expr.expression = fold_constants(statement->expr.expression);
        data_00582467 = 1;
    }
    if (inline_call_count != 0 && data_005824b5 == 0) {
        statement = expand_inline_calls(statement);
        data_00582467 = 1;
    }
    return statement;
}

Statement *expand_inline_calls(Statement *stmt)
{
    TypeFunc *ftype;
    SInt16 i;
    CInlineInfo *expr;
    ENode *node;
    Object *tempobj;
    ENode *x;
    Object *obj;

    i = 0;
    if (inline_call_count > 0) {
        do {
            x = inline_call_expressions[i];
            obj = x->data.funccall.funcref->data.addr.objref;
            if ((expr = obj->u.func.u) != NULL) {
                ftype = TYPE_FUNC(obj->type);
                CError_ASSERT(2459, IS_TYPE_FUNC(ftype));
                if (ftype->functype->type != TYPEVOID) {
                    if (CMachine_FunctionRequiresMemoryReturn((TypeFunc *)obj->type) == 1)
                        tempobj = CInline_MakeTemp(CDecl_NewPointerType(ftype->functype));
                    else
                        tempobj = CInline_MakeTemp(ftype->functype);
                } else {
                    tempobj = NULL;
                }
                stmt = generate_inline_statements(obj, stmt, expr, x, newlabel(), tempobj, 1);
                if (tempobj != NULL)
                    node = CExpr_New_EINDIRECT_Node(tempobj);
                else
                    node = nullnode();
                *x = *node;
            }
            i++;
        } while (i < inline_call_count);
    }
    return stmt;
}

Statement *try_inline_statement(Statement *obj, char *flag)
{
    CInlineInfo *rec;
    Object *t;
    SInt16 n;
    SInt16 i;
    CLabel *v;

    *flag = 0;
    t = obj->expr.expression->data.funccall.funcref->data.objref;
    if ((rec = t->u.func.u) == NULL || rec->kind < 3)
        return obj;
    if (obj->type != ST_EXPRESSION) {
        n = rec->nstmts;
        for (i = 0; i < n - 1; i++) {
            if (rec->stmtinfo[i].type == 8)
                return obj;
        }
        if (rec->stmtinfo[n - 1].type != 8)
            return obj;
        v = NULL;
    } else {
        v = newlabel();
    }
    *flag = 1;
    return generate_inline_statements(t, obj, rec, obj->expr.expression, v, NULL, 0);
}

Statement *generate_inline_statements(Object *function, Statement *tail, CInlineInfo *args, ENode *result,
                                      CLabel *returnLabel, Object *returnObject, UInt8 appendStatement)
{
    ChainRec *chain;
    CLabel **labels;
    SInt16 i;
    UInt8 originalType;
    Boolean convertReturn;
    Statement templateStmt;
    IStmtRec *entry;
    ENode *initializer;

    templateStmt = *tail;
    convertReturn = (CMachine_FunctionRequiresMemoryReturn((TypeFunc *)function->type) == 1);
    initializer = setup_inline_locals_and_arguments(function, args, result->data.funccall.args);
    if (initializer != NULL) {
        tail->type = ST_EXPRESSION;
        tail->expr.expression = fold_constants(initializer);
    } else {
        tail->type = ST_NOP;
    }
    chain = NULL;
    fixup_list = NULL;
    labels = (CLabel **)CompilerTools_AllocatePool((SInt16)args->nstmts * sizeof(*labels));
    memclrw(labels, (SInt16)args->nstmts * sizeof(*labels));
    i = 0;
    entry = (IStmtRec *)args->stmtinfo;
    originalType = templateStmt.type;
    for (; i < (SInt16)args->nstmts; i++, entry++) {
        Statement *node;

        tail->next = (Statement *)CompilerTools_AllocatePool(sizeof(*tail));
        tail = tail->next;
        *tail = templateStmt;
        node = tail;
        node->type = entry->type;
        node->flags = entry->flags;
        node->value += (SInt16)entry->value;
        if (entry->exceptionActions != NULL) {
            CException *list = copy_exception_actions(entry, 1);

            if (node->dobjstack != NULL) {
                CException *last = list;

                while (last->next != NULL)
                    last = last->next;
                last->next = node->dobjstack;
            }
            node->dobjstack = list; /* generate_inline_statements: copied exception actions */
        }
        switch (node->type) {
            case ST_EXPRESSION:
            case ST_BEGINCATCH:
            case ST_ENDCATCH:
            case ST_ENDCATCHDTOR:
            case ST_GOTOEXPR:
                node->expr.expression = gen_expr(entry->data.operand);
                break;
            case ST_RETURN:
                if (entry->data.operand != NULL) {
                    node->expr.expression = gen_expr(entry->data.operand);
                    if (convertReturn) {
                        SInt32 index = CInline_ReturnZero(function->type);
                        ENode *conversion;

                        if (data_0058245a[index].var == NULL)
                            conversion = gen_expr_save(data_0058245a[index].expr);
                        else
                            conversion = create_objectnode(data_0058245a[index].var);
                        node->expr.expression = makecommaexpression(node->expr.expression, conversion);
                    }
                    if (returnObject != NULL) {
                        node->type = ST_EXPRESSION;
                        node->expr.expression =
                            makediadicnode(CExpr_New_EINDIRECT_Node(returnObject), node->expr.expression, 0x1e);
                    } else {
                        node->type = originalType;
                    }
                    if (returnLabel != NULL) {
                        tail->next = (Statement *)CompilerTools_AllocatePool(sizeof(*tail));
                        tail = tail->next;
                        *tail = templateStmt;
                        tail->type = ST_GOTO;
                        tail->target.label = returnLabel;
                    }
                } else if (returnLabel != NULL) {
                    node->type = ST_GOTO;
                    node->target.label = returnLabel;
                } else {
                    node->type = ST_NOP;
                }
                break;
            case ST_LABEL:
                node->target.label = newlabel();
                labels[i] = node->target.label;
                node->target.label->target.stmt = node;
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                node->expr.expression = gen_expr(entry->data.operand);
                /* fall through */
            case ST_GOTO:
                add_chain(&chain, node, entry);
                break;
            case ST_SWITCH:
                node->expr.expression = gen_expr(*(void **)entry->data.switchInfo);
                /* fall through */
            case ST_ASM:
                add_chain(&chain, node, entry);
                break;
            case ST_NOP:
                break;
            default:
                CError_FATAL(2360);
        }
    }
    if (returnLabel != NULL) {
        tail->next = (Statement *)CompilerTools_AllocatePool(sizeof(*tail));
        tail = tail->next;
        *tail = templateStmt;
        tail->type = ST_LABEL;
        tail->target.label = returnLabel;
        returnLabel->target.stmt = tail;
        if (appendStatement != 0) {
            tail->next = (Statement *)CompilerTools_AllocatePool(sizeof(*tail));
            tail = tail->next;
            *tail = templateStmt;
        }
    }
    while (chain != NULL) {
        Statement *node = chain->node;
        IStmtRec *entry = chain->ent;

        switch (node->type) {
            case ST_GOTO:
                node->target.label = labels[entry->data.targetIndex];
                if (node->target.label == NULL)
                    CError_FATAL(2380);
                break;
            case ST_IFGOTO:
            case ST_IFNGOTO:
                node->target.label = labels[entry->secondaryOperand.targetIndex];
                if (node->target.label == NULL)
                    CError_FATAL(2385);
                break;
            case ST_SWITCH:
                reconstruct_switch_info(node, entry, labels);
                break;
            case ST_ASM:
                InlineAsm_CopyInstructionAndResolveOperands(node, labels, 1, entry->data.assembly,
                                                            entry->secondaryOperand.assemblyData);
                break;
            default:
                CError_FATAL(2396);
        }
        chain = chain->next;
    }
    while (fixup_list != NULL) {
        if ((*fixup_list->destination = labels[fixup_list->labelIndex]) == NULL)
            CError_FATAL(2403);
        fixup_list = fixup_list->next;
    }
    return tail;
}

CException *copy_exception_actions(IStmtRec *parent, char copyExpressions)
{
    CException *source;
    CException *copy;
    CException *copies;

    for (source = (CException *)parent->exceptionActions, copies = NULL; source != NULL; source = source->next) {
        copy = galloc(sizeof(CException));
        copy->next = copies;
        copies = copy;
        copy->kind = source->kind;
        switch (source->kind) {
            case 1:
                CopyStatementExpression(copy, source, copyExpressions);
                break;
            case 2:
                copy->data.slots[0] = CInline_GetObjectByIndex(source->data.operands[0].value, copyExpressions);
                copy->data.slots[2] = source->data.slots[2];
                copy->data.slots[1] = CInline_GetObjectByIndex(source->data.operands[1].value, copyExpressions);
                break;
            case 3:
                CopyStatementExpression(copy, source, copyExpressions);
                copy->data.slots[2] = source->data.slots[2];
                break;
            case 4:
                CopyStatementExpression(copy, source, copyExpressions);
                break;
            case 5:
                CopyStatementExpression(copy, source, copyExpressions);
                copy->data.slots[2] = source->data.slots[2];
                copy->data.slots[3] = source->data.slots[3];
                break;
            case 6:
                copy->data.slots[0] = CInline_GetObjectByIndex(source->data.operands[0].value, copyExpressions);
                copy->data.slots[1] = CInline_GetObjectByIndex(source->data.operands[1].value, copyExpressions);
                copy->data.slots[2] = CInline_GetObjectByIndex(source->data.operands[2].value, copyExpressions);
                copy->data.slots[3] = CInline_GetObjectByIndex(source->data.operands[3].value, copyExpressions);
                break;
            case 7:
            case 17:
                CopyStatementExpression(copy, source, copyExpressions);
                copy->data.slots[2] = source->data.slots[2];
                break;
            case 8:
                copy->data.slots[0] = CInline_GetObjectByIndex(source->data.operands[0].value, copyExpressions);
                copy->data.slots[1] = CInline_GetObjectByIndex(source->data.operands[1].value, copyExpressions);
                copy->data.slots[2] = source->data.slots[2];
                copy->data.slots[3] = source->data.slots[3];
                break;
            case 9:
                CopyStatementExpression(copy, source, copyExpressions);
                copy->data.slots[2] = source->data.slots[2];
                copy->data.slots[3] = source->data.slots[3];
                copy->data.slots[4] = source->data.slots[4];
                break;
            case 10:
            case 11:
                CopyStatementExpression(copy, source, copyExpressions);
                break;
            case 12:
                CopyStatementExpression(copy, source, copyExpressions);
                copy->data.slots[2] = CInline_GetObjectByIndex(source->data.operands[2].value, copyExpressions);
                break;
            case 13:
                copy->data.slots[0] = CInline_GetObjectByIndex(source->data.operands[0].value, copyExpressions);
                copy->data.slots[1] = CInline_GetObjectByIndex(source->data.operands[1].value, copyExpressions);
                {
                    IFixup *fixup = CompilerTools_AllocatePool(sizeof(IFixup));
                    fixup->next = fixup_list;
                    fixup_list = fixup;
                    fixup->labelIndex = (UInt16)source->data.slots[2];
                    fixup->destination = &copy->data.catch_block.label;
                }
                copy->data.slots[3] = source->data.slots[3];
                copy->data.slots[4] = source->data.slots[4];
                copy->data.slots[5] = source->data.slots[5];
                break;
            case 14:
                copy->data.slots[0] = CInline_GetObjectByIndex(source->data.operands[0].value, copyExpressions);
                break;
            case 15:
                copy->data.slots[0] = source->data.slots[0];
                copy->data.slots[1] = source->data.slots[1];
                {
                    IFixup *fixup = CompilerTools_AllocatePool(sizeof(IFixup));
                    fixup->next = fixup_list;
                    fixup_list = fixup;
                    fixup->labelIndex = (UInt16)source->data.slots[2];
                    fixup->destination = &copy->data.specification.label;
                }
                copy->data.slots[3] = CInline_GetObjectByIndex(source->data.operands[3].value, copyExpressions);
                break;
            case 16:
                break;
            default:
                CError_FATAL(2228);
        }
    }
    return copies;
}

Object *CInline_GetObjectByIndex(UInt32 index, char useTable)
{
    ObjectList *node;
    ObjectList *localNode;
    if (index != 0) {
        if (index & 0x80000000u) {
            index = (index & 0x7fffffffu) - 1;
            if ((unsigned char)useTable != 0) {
                if (data_0058245a[index].var == NULL)
                    CError_FATAL(2085);
                return data_0058245a[index].var;
            } else {
                node = arguments;
                while (node != NULL) {
                    if (index == 0)
                        return node->object.value;
                    node = node->next;
                    index--;
                }
                CError_FATAL(2089);
            }
        } else {
            index--;
            if ((unsigned char)useTable != 0) {
                if (data_00582456[index] == NULL)
                    CError_FATAL(2096);
                return data_00582456[index];
            } else {
                localNode = locals;
                while (localNode != NULL) {
                    if (index == 0)
                        return localNode->object.value;
                    localNode = localNode->next;
                    index--;
                }
                CError_FATAL(2100);
            }
        }
    }
    return NULL;
}

void reconstruct_switch_info(Statement *statement, IStmtRec *record, CLabel **labelTable)
{
    SwitchInfo *switchInfo;
    SwitchCase *switchCase;
    SInt16 caseIndex;

    switchInfo = (SwitchInfo *)CompilerTools_AllocatePool(sizeof(SwitchInfo));
    statement->target.switchDescriptor = switchInfo;
    switchInfo->defaultlabel = labelTable[record->data.switchInfo->defaultStatementIndex];
    CError_ASSERT(2054, switchInfo->defaultlabel != NULL);
    switchInfo->sizetype = record->data.switchInfo->valueType;

    for (caseIndex = 0; caseIndex < record->data.switchInfo->caseCount; caseIndex++) {
        if (caseIndex == 0) {
            switchCase = (SwitchCase *)CompilerTools_AllocatePool(sizeof(SwitchCase));
            switchInfo->cases = switchCase;
        } else {
            switchCase->next = (SwitchCase *)CompilerTools_AllocatePool(sizeof(SwitchCase));
            switchCase = switchCase->next;
        }
        switchCase->next = NULL;
        switchCase->min = record->data.switchInfo->entries[caseIndex].caseValue;
        switchCase->label = labelTable[record->data.switchInfo->entries[caseIndex].statementIndex];
        CError_ASSERT(2064, switchCase->label != NULL);
    }
}

ENode *inline_expression(ENode *node)
{
    switch (node->type) {
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
            node->data.monadic = inline_expression(node->data.monadic);
            break;

        case EFORCELOAD:
            node->data.monadic = inline_expression(node->data.monadic);
            if (node->data.monadic->type == EFORCELOAD)
                node->data.monadic = node->data.monadic->data.monadic;
            break;

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
        case EPMODULO:
        case EROTL:
        case EROTR:
            node->data.diadic.left = inline_expression(node->data.diadic.left);
            node->data.diadic.right = inline_expression(node->data.diadic.right);
            break;

        case ELAND:
        case ELOR:
        case ECOMMA:
            node->data.diadic.left = inline_expression(node->data.diadic.left);
            {
                Boolean save = inline_statement_mode;
                inline_statement_mode = 0;
                node->data.diadic.right = inline_expression(node->data.diadic.right);
                inline_statement_mode = save;
            }
            break;

        case EFUNCCALL:
        case EFUNCCALLP:
            node->data.funccall.funcref = inline_expression(node->data.funccall.funcref);
            {
                ENodeList *p;
                for (p = node->data.funccall.args; p != NULL; p = p->next)
                    p->node = inline_expression(p->node);
            }
            if (node->data.funccall.funcref->type == EOBJREF && can_inline(node->data.funccall.funcref))
                node = inline_call_expression(node);
            break;

        case EMFPOINTER:
            node->data.diadic.left = inline_expression(node->data.diadic.left);
            {
                Boolean save = inline_statement_mode;
                inline_statement_mode = 0;
                node->data.diadic.right = inline_expression(node->data.diadic.right);
                inline_statement_mode = save;
            }
            break;

        case EQUALNAME:
            node->data.diadic.left = inline_expression(node->data.diadic.left);
            node->data.diadic.right = inline_expression(node->data.diadic.right);
            break;

        case ECOND:
            node->data.cond.cond = inline_expression(node->data.cond.cond);
            {
                Boolean save = inline_statement_mode;
                inline_statement_mode = 0;
                node->data.cond.expr1 = inline_expression(node->data.cond.expr1);
                node->data.cond.expr2 = inline_expression(node->data.cond.expr2);
                inline_statement_mode = save;
            }
            break;

        case ENEWEXCEPTIONARRAY:
            node = ((ENodeList *)node->data.monadic)->node;
            if (node != NULL)
                node = inline_expression(node);
            else
                node = nullnode();
            break;

        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ENULLCHECK:
        case ELOCOBJ:
        case ENEWEXCEPTION:
        case EMEMBER:
        case EASSBLK:
            break;

        default:
            CError_FATAL(2035);
    }
    return node;
}

Boolean can_inline(ENode *node)
{
    Object *function = node->data.objref;
    CInlineInfo *inlineInfo;

    if (function->type->type == TYPEFUNC &&
        ((function->qual & Q_INLINE) || (TYPE_METHOD(function->type)->flags & FUNC_IS_CTOR)) &&
        (function->datatype == DFUNC || (function->datatype == DVFUNC && (node->flags & ENODE_FLAG_80)))) {
        if (copts.fb6 == 0 && data_00582468 > 0 && copts.inlineLimit == 0) {
            inlineInfo = function->u.func.u;
            if (inlineInfo == NULL)
                return 0;
            if (inline_statement_count(inlineInfo) > 10)
                return 0;
            if (data_00582468 > 1 && inline_statement_count(inlineInfo) > 7)
                return 0;
            if (data_00582468 > 2 && inline_statement_count(inlineInfo) > 3)
                return 0;
        }
        return 1;
    }
    return 0;
}

ENode *inline_call_expression(ENode *expr)
{
    ENode *source;
    SInt32 argumentIndex;
    ENode *statementExpr;
    ENode *argument;
    ENode *statementSource;
    ENode *firstSource;
    ENode *result;
    CInlineInfo *body;
    Object *object;
    SInt16 index;
    Boolean memoryReturn;

    object = expr->data.monadic->data.objref;
    if ((body = (CInlineInfo *)object->u.func.u) == NULL) {
        return expr;
    }
    if (body->kind < 6) {
        if (inline_statement_mode != 0 && inline_call_count < 16 && body->kind == 3) {
            inline_call_expressions[inline_call_count] = expr;
            inline_call_count += 1;
        }
        return expr;
    }
    memoryReturn = CMachine_FunctionRequiresMemoryReturn((TypeFunc *)object->type) == 1;
    result = setup_inline_locals_and_arguments(object, body, expr->data.funccall.args);
    for (index = 0; index < (SInt16)body->nstmts; ++index) {
        switch (body->stmtinfo[index].type) {
            case 8:
                if ((source = body->stmtinfo[index].data.operand) == NULL) {
                    break;
                }
                evalMode = 4;
                memo_list = NULL;
                alloc_state = 0;
                statementExpr = CInline_00513240(source);
                if (memoryReturn != 0) {
                    if (result != NULL) {
                        statementExpr = makecommaexpression(result, statementExpr);
                    }
                    argumentIndex = CInline_ReturnZero(object->type);
                    if (data_0058245a[argumentIndex].var == NULL) {
                        argument = InlineArgument(data_0058245a[argumentIndex].expr);
                    } else {
                        argument = create_objectnode(data_0058245a[argumentIndex].var);
                    }
                    result = makecommaexpression(statementExpr, argument);
                    break;
                }
                if (result != NULL) {
                    result = makecommaexpression(result, InlineWrapResult(statementExpr));
                    break;
                }
                result = InlineWrapResult(statementExpr);
                break;
            case 4:
                if (result != NULL) {
                    statementSource = body->stmtinfo[index].data.operand;
                    evalMode = 4;
                    memo_list = NULL;
                    alloc_state = 0;
                    result = makecommaexpression(result, CInline_00513240(statementSource));
                    break;
                }
                firstSource = body->stmtinfo[index].data.operand;
                evalMode = 4;
                memo_list = NULL;
                alloc_state = 0;
                result = CInline_00513240(firstSource);
                break;
            default:
                CError_FATAL(1437);
        }
    }
    if (result == NULL) {
        result = nullnode();
    }
    if (expr->rtype->type != TYPEVOID) {
        result->rtype = expr->rtype;
    }
    inline_call_seen = 1;
    return result;
}

void fn_005129f0(ENode *expr)
{
    data_005824c3 = 1U;
    return;
}

ENode *setup_inline_locals_and_arguments(Object *function, CInlineInfo *inlineInfo, ENodeList *arguments)
{
    Boolean targetMatches;
    CInlineVar *parameter;
    ENodeList *argument;
    ENode *initializers;
    ENode *expression;
    Object *variable;
    Object **locals;
    int i;

    targetMatches = 0;
    if (TYPE_FUNC(function->type)->args == &data_00584748)
        targetMatches = 1;

    locals = CompilerTools_AllocatePool(inlineInfo->nlocals << 2);
    data_00582456 = locals;
    data_0058245a = CompilerTools_AllocatePool(inlineInfo->nargs * 0xc);

    i = 0;
    parameter = inlineInfo->localinfo;
    while (i < inlineInfo->nlocals) {
        if (parameter->used != 0) {
            data_00582456[i] =
                (variable = NewInlineVar(parameter->type, (SInt16)parameter->qual, parameter->storageFlags));
            if (parameter->dirty == 0)
                variable->flags |= OBJECT_FLAGS_2;
        } else {
            data_00582456[i] = NULL;
        }
        i++, parameter++;
    }

    i = 0;
    parameter = inlineInfo->arginfo;
    argument = arguments;
    for (; i < inlineInfo->nargs; i++, parameter++) {
        data_0058245a[i].arg = NULL;
        if (parameter->used == 0) {
            data_0058245a[i].var = NULL;
            data_0058245a[i].expr = NULL;
        } else if (argument != NULL && parameter->dirty != 0 && CInline_00513910(argument->node) == 0 &&
                   !(targetMatches && argument->node->rtype->size != parameter->type->size)) {
            data_0058245a[i].var = NULL;
            data_0058245a[i].expr = argument->node;
        } else if (argument != NULL && parameter->dirty != 0 && parameter->type->type == TYPEPOINTER &&
                   (TYPE_POINTER(parameter->type)->qual & Q_REFERENCE) != 0 &&
                   (expression = copy_result_reference(argument->node)) != NULL) {
            data_0058245a[i].var = NULL;
            data_0058245a[i].expr = expression;
            data_0058245a[i].arg = argument->node;
        } else {
            data_0058245a[i].var = NewInlineVar(parameter->type, (SInt16)parameter->qual, parameter->storageFlags);
            data_0058245a[i].expr = NULL;
        }
        if (argument != NULL)
            argument = argument->next;
    }

    initializers = NULL;
    i = 0;
    argument = arguments;
    while (argument != NULL) {
        if (i >= inlineInfo->nargs) {
            if (initializers == NULL)
                initializers = argument->node;
            else
                initializers = makecommaexpression(argument->node, initializers);
        } else if (data_0058245a[i].var == NULL) {
            if (data_0058245a[i].arg != NULL) {
                if (initializers == NULL)
                    initializers = data_0058245a[i].arg;
                else
                    initializers = makecommaexpression(data_0058245a[i].arg, initializers);
            } else if (data_0058245a[i].expr == NULL) {
                if (CInline_00513910(argument->node) != 0) {
                    if (initializers == NULL)
                        initializers = argument->node;
                    else
                        initializers = makecommaexpression(argument->node, initializers);
                    if (argument->node->type == ENULLCHECK)
                        CError_FATAL(1283);
                }
            }
        } else {
            if (targetMatches && argument->node->rtype->size != data_0058245a[i].var->type->size) {
                argument->node = makemonadicnode(argument->node, ETYPCON);
                argument->node->rtype = data_0058245a[i].var->type;
            }
            expression = makediadicnode(CExpr_New_EINDIRECT_Node(data_0058245a[i].var), argument->node, EASS);
            if (initializers == NULL)
                initializers = expression;
            else
                initializers = makecommaexpression(expression, initializers);
        }
        argument = argument->next;
        i++;
    }
    return initializers;
}

ENode *copy_result_reference(ENode *e)
{
    ENodeList *args;
    SInt16 k;

    while (e->type == ECOMMA)
        e = e->data.diadic.right;

    switch (e->type) {
        case EOBJREF:
        case EPRECOMP:
            return CInline_CopyConst(e);

        case EFUNCCALL:
            if (e->rtype->type != TYPEPOINTER || TPTR_TARGET(e->rtype)->type != TYPECLASS)
                break;
            if (e->data.funccall.funcref->type == EOBJREF &&
                CClass_IsDestructor(e->data.funccall.funcref->data.addr.objref) &&
                (args = e->data.funccall.args) != NULL)
                return CInline_CopyConst(args->node);
            if (TPTR_TARGET(e->rtype) != e->data.funccall.functype->functype)
                break;
            if (CMachine_FunctionRequiresMemoryReturn(e->data.funccall.functype) != 1)
                break;
            if ((args = e->data.funccall.args) == NULL)
                break;
            switch (CInline_ReturnZero((Type *)e->data.funccall.functype)) {
                case 0:
                    break;
                case 1:
                    args = args->next;
                    if (args != NULL)
                        break;
                    CError_FATAL(1177);
                    /* fall through */
                default:
                    CError_FATAL(1178);
                    break;
            }
            return CInline_CopyConst(args->node);
    }
    return NULL;
}

Object *create_local_object(Type *type, unsigned int qual, unsigned int storageClassFlags)
{
    unsigned char storageClass;
    Object *object = CParser_NewLocalDataObject(NULL, 1);
    object->name = CParser_GetUniqueName();
    object->type = type;
    object->qual = (short)qual;
    storageClass = (unsigned char)storageClassFlags;
    if (storageClass & 0x80) {
        object->flags |= 2;
        storageClass &= 0x7f;
    }
    switch (storageClass) {
        case 0:
            object->sclass = TK_EOF;
            break;
        case 1:
            object->sclass = TK_REGISTER;
            break;
        case 2:
            object->sclass = TK_AUTO;
            break;
        default:
            CError_FATAL(1093);
            break;
    }
    CFunc_SetupLocalVarInfo(object);
    return object;
}

void set_object_sclass(Object *object, UInt8 kind)
{
    if (kind & 0x80) {
        object->flags |= OBJECT_FLAGS_2;
        kind &= 0x7f;
    }
    switch (kind) {
        case 0:
            object->sclass = TK_EOF;
            break;
        case 1:
            object->sclass = TK_REGISTER;
            break;
        case 2:
            object->sclass = TK_AUTO;
            break;
        default:
            CError_FATAL(1093);
    }
}

ENode *fn_00513040(ENode *expr, UInt8 mode)
{
    evalMode = mode;
    memo_list = NULL;
    switch (mode) {
        case 0:
        case 3:
        case 4:
            alloc_state = 0;
            expr = CInline_00513240(expr);
            break;
        case 1:
            alloc_state = 1;
            expr = CInline_00513240(expr);
            break;
        case 2:
            alloc_state = 1;
            expr = CInline_00513240(expr);
            CInline_005130b0(expr, 0);
    }
    return expr;
}

void CInline_005130b0(ENode *node, Boolean flag)
{
    ENodeList *l;

    for (;;) {
        switch (node->type) {
            case ETEMP:
                data_0058245e[node->data.longval].used = 1;
                data_0058245e[node->data.longval].dirty = 0;
                return;

            case EARGOBJ:
                data_00582462[node->data.longval].used = 1;
                return;

            case EINDIRECT:
                node = node->data.monadic;
                if (node->type == ETEMP) {
                    data_0058245e[node->data.longval].used = 1;
                    if (flag)
                        data_0058245e[node->data.longval].dirty = 0;
                    return;
                }
                flag = 0;
                break;

            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case EFORCELOAD:
            case ETYPCON:
            case EBITFIELD:
                node = node->data.monadic;
                flag = 0;
                break;

            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
                node = node->data.monadic;
                flag = 1;
                break;

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
                CInline_005130b0(node->data.diadic.left, 1);
                node = node->data.diadic.right;
                flag = 0;
                break;

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
            case ECOMMA:
            case EROTL:
            case EROTR:
                CInline_005130b0(node->data.diadic.left, 0);
                node = node->data.diadic.right;
                flag = 0;
                break;

            case EINTCONST:
            case EFLOATCONST:
            case ESTRINGCONST:
            case EOBJREF:
            case ENULLCHECK:
            case EPRECOMP:
            case ELOCOBJ:
            case ENEWEXCEPTION:
            case EASSBLK:
                return;

            case ENEWEXCEPTIONARRAY:
                node = ((ENodeList *)node->data.monadic)->node;
                if (node != NULL)
                    CInline_005130b0(node, 0);
                return;

            case EFUNCCALL:
            case EFUNCCALLP:
                CInline_005130b0(node->data.funccall.funcref, 0);
                for (l = node->data.funccall.args; l != NULL; l = l->next)
                    CInline_005130b0(l->node, 0);
                return;

            case EMFPOINTER:
                CInline_005130b0(node->data.diadic.left, 0);
                node = node->data.diadic.right;
                flag = 0;
                break;

            case EQUALNAME:
                CInline_005130b0(node->data.diadic.left, 0);
                node = node->data.diadic.right;
                flag = 0;
                break;

            case ECOND:
                CInline_005130b0(node->data.cond.cond, 0);
                CInline_005130b0(node->data.cond.expr1, 0);
                node = node->data.cond.expr2;
                flag = 0;
                break;

            case EMEMBER:
                return;

            default:
                CError_FATAL(1021);
        }
    }
}

ENode *CInline_00513240(ENode *expr)
{
    ENode *node;
    ObjectList *objects;
    int index;
    Object *object;

    if (alloc_state)
        node = (ENode *)galloc(sizeof(ENode));
    else
        node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    for (;;) {
        *node = *expr;
        switch (node->type) {
            case EOBJLIST:
                switch (node->data.templatecomparison.tag) {
                    case 3: {
                        ENodeList *values = node->data.explicitconversion.arguments;
                        values = copy_enode_list(values);
                        node->data.explicitconversion.arguments = values;
                        break;
                    }
                    case 6:
                        node->data.monadic = CInline_00513240(node->data.monadic);
                        break;
                    case 0:
                    case 1:
                    case 4:
                    case 5:
                        break;
                    default:
                        CError_FATAL(722);
                }
                break;
            case EPRECOMP:
                if (node->data.diadic.right != NULL)
                    node->data.diadic.right = (ENode *)MemoFirst(node->data.diadic.right);
                break;
            case ELOCOBJ:
                switch (evalMode) {
                    case EM_REG: {
                        SInt16 memberIndex = inline_member_index((SInt32)node->data.memberFunctionPointer->name);
                        node->data.longval = memberIndex;
                        return node;
                    }
                    case EM_ARG:
                    case EM_LOCAL: {
                        IFixup *entry;
                        entry = (IFixup *)CompilerTools_AllocatePool(sizeof(IFixup));
                        entry->next = fixup_list;
                        fixup_list = entry;
                        entry->labelIndex = node->data.longval;
                        entry->destination = (CLabel **)&node->data.longval;
                        return node;
                    }
                }
                break;
            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case EFORCELOAD:
            case ETYPCON:
            case EBITFIELD:
                node->data.monadic = CInline_00513240(node->data.monadic);
                break;
            case EMUL:
            case EMULV:
            case EDIV:
            case EMODULO:
            case EADDV:
            case ESUBV:
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
            case EPMODULO:
            case EROTL:
            case EROTR:
            case EBCLR:
            case EBTST:
            case EBSET:
                node->data.diadic.left = CInline_00513240(node->data.diadic.left);
                node->data.diadic.right = CInline_00513240(node->data.diadic.right);
                break;
            case ECOND:
                node->data.cond.cond = CInline_00513240(node->data.cond.cond);
                node->data.cond.expr1 = CInline_00513240(node->data.cond.expr1);
                node->data.cond.expr2 = CInline_00513240(node->data.cond.expr2);
                break;
            case EQUALNAME:
                node->data.diadic.left = CInline_00513240(node->data.diadic.left);
                node->data.diadic.right = CInline_00513240(node->data.diadic.right);
                break;
            case EFUNCCALL:
            case EFUNCCALLP:
                node->data.funccall.funcref = CInline_00513240(node->data.funccall.funcref);
                node->data.funccall.args = copy_enode_list(node->data.funccall.args);
                break;
            case EMFPOINTER:
                node->data.cond.expr2 = (ENode *)CInline_Memo(node->data.cond.expr2);
                node->data.diadic.left = CInline_00513240(node->data.diadic.left);
                node->data.diadic.right = CInline_00513240(node->data.diadic.right);
                break;
            case ENULLCHECK:
                node->data.monadic = (ENode *)CInline_Memo(node->data.monadic);
                break;
            case EINDIRECT: {
                MemoNode *saved_list;
                EvalMode saved_mode;
                AllocState saved_flag;
                ENode *input;
                ENode *copy;
                ENode *result;
                if (evalMode == EM_LOCAL) {
                    if (node->data.monadic->type == ETEMP) {
                        if (data_0058245a[node->data.monadic->data.longval].var == NULL) {
                            if (data_0058245a[node->data.monadic->data.longval].expr == NULL)
                                CError_FATAL(792);
                            (void)data_0058245a[node->data.monadic->data.longval].expr;
                            input = get_input(node->data.monadic->data.longval);
                            saved_flag = alloc_state;
                            alloc_state = AS_GLOBAL;
                            saved_mode = evalMode;
                            evalMode = EM_LOCAL;
                            saved_list = memo_list;
                            memo_list = NULL;
                            copy = CInline_00513240(input);
                            result = copy;
                            alloc_state = saved_flag;
                            evalMode = saved_mode;
                            memo_list = saved_list;
                            (void)expr->rtype, (void)copy->rtype;
                            result = adjust(copy, expr);
                            return result;
                        }
                    }
                }
                node->data.monadic = CInline_00513240(node->data.monadic);
                break;
            }
            case EOBJREF: {
                UInt8 datatype;
                if (evalMode != EM_REG)
                    break;
                if ((datatype = expr->data.objref->datatype) == DALIAS) {
                    CExpr_AliasTransform(expr);
                    continue;
                }
                if (datatype == DDATA)
                    return node;
                for (objects = arguments, index = 0; objects != NULL; objects = objects->next, index++)
                    if (objects->object.value == node->data.objref) {
                        node->type = ETEMP;
                        node->data.longval = index;
                        return node;
                    }
                object = node->data.objref;
                inline_local_index(object, &index);
                if (index >= 0) {
                    node->type = EARGOBJ;
                    node->data.longval = index;
                    return node;
                }
                if (datatype == DLOCAL)
                    CError_FATAL(831);
                break;
            }
            case ETEMP:
                switch (evalMode) {
                    case EM_LOCAL:
                        CError_ASSERT(839, data_0058245a[node->data.longval].var != NULL);
                        node->type = EOBJREF;
                        node->data.objref = data_0058245a[node->data.longval].var;
                        return node;
                    case EM_ARG:
                        for (objects = arguments, index = 0; objects != NULL; objects = objects->next, index++)
                            if (index == node->data.longval) {
                                node->type = EOBJREF;
                                node->data.objref = objects->object.value;
                                CError_ASSERT(848, node->data.objref != NULL);
                                return node;
                            }
                        break;
                }
                CError_FATAL(853);
            case EARGOBJ:
                switch (evalMode) {
                    case EM_LOCAL:
                        node->type = EOBJREF;
                        node->data.objref = data_00582456[node->data.longval];
                        return node;
                    case EM_ARG:
                        objects = locals;
                        index = 0;
                        for (; objects != NULL; objects = objects->next, index++)
                            if (index == node->data.longval) {
                                node->type = EOBJREF;
                                node->data.objref = objects->object.value;
                                CError_ASSERT(868, node->data.objref != NULL);
                                return node;
                            }
                        break;
                }
                CError_FATAL(873);
                break;
            case ELABEL:
            case ESETCONST:
                node->data.diadic.left = CInline_00513240(node->data.diadic.left);
                node->data.diadic.right = CInline_00513240(node->data.diadic.right);
                break;
            case ENEWEXCEPTIONARRAY: {
                MemberFuncRef *dst;
                MemberFuncRef *src = node->data.memberfunc;
                if (alloc_state)
                    dst = (MemberFuncRef *)galloc(sizeof(MemberFuncRef));
                else
                    dst = (MemberFuncRef *)CompilerTools_AllocatePool(sizeof(MemberFuncRef));
                *dst = *src;
                if (dst->bcl != NULL)
                    dst->bcl = CClass_GetPathCopy(dst->bcl, alloc_state);
                if (dst->expression != NULL)
                    dst->expression = CInline_00513240(dst->expression);
                node->data.memberfunc = dst;
                break;
            }
            case EINTCONST:
            case EFLOATCONST:
            case ESTRINGCONST:
            case ENEWEXCEPTION:
            case EMEMBER:
            case EASSBLK:
                break;
            default:
                CError_FATAL(890);
        }
        break;
    }
    return node;
}

ENodeList *copy_enode_list(ENodeList *values)
{
    ENodeList *head;
    ENodeList *tail;
    ENodeList *node;

    head = NULL;
    if (values != NULL) {
        do {
            if (alloc_state != '\0') {
                node = (ENodeList *)galloc(sizeof(ENodeList));
            } else {
                node = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
            }
            node->node = CInline_00513240(values->node);
            node->next = NULL;
            if (head != NULL) {
                tail->next = node;
                tail = node;
            } else {
                head = tail = node;
            }
            values = values->next;
        } while (values != NULL);
    }
    return head;
}

Boolean CInline_00513910(ENode *expr)
{
    Object *object;
    for (;;) {
        switch (expr->type) {
            case EINTCONST:
            case EFLOATCONST:
            case EOBJREF:
            case ETEMP:
            case EARGOBJ:
            case EASSBLK:
            case ENEWEXCEPTION:
                return 0;
            case ESTRINGCONST:
                return copts.faf;
            case ENEWEXCEPTIONARRAY:
                if (CInline_ArrayInitializer(expr) != NULL)
                    return CInline_00513910(CInline_ArrayInitializer(expr));
                return 0;
            case EINDIRECT:
                if (expr->data.monadic->type == EOBJREF) {
                    object = expr->data.monadic->data.objref;
                    return InlineObjectModifiable(object);
                }
                return 1;
            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
            case EFORCELOAD:
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
            case EFUNCCALL:
            case EFUNCCALLP:
            case EQUALNAME:
            case EMFPOINTER:
            case ENULLCHECK:
            case EPRECOMP:
            case ELOCOBJ:
            case EMEMBER:
                return 1;
            case EMONMIN:
            case EBINNOT:
            case ELOGNOT:
            case ETYPCON:
            case EBITFIELD:
                expr = expr->data.monadic;
                break;
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
            case ECOMMA:
            case EROTL:
            case EROTR:
                if (CInline_00513910(expr->data.diadic.left))
                    return 1;
                expr = expr->data.diadic.right;
                break;
            case ECOND:
                if (CInline_00513910(expr->data.cond.cond))
                    return 1;
                if (CInline_00513910(expr->data.cond.expr1))
                    return 1;
                expr = expr->data.cond.expr2;
                break;
            default:
                CError_FATAL(576);
                break;
        }
    }
}

unsigned int CInline_GetObjectIndex(void *object)
{
    ObjectList *entry;
    int index;

    if (object != NULL) {
        entry = arguments;
        for (index = 0; entry != NULL; entry = entry->next, index++) {
            if (entry->object.value == object) {
                data_0058245e[index].used = 1;
                data_0058245e[index].dirty = 0;
                return index + 0x80000001U;
            }
        }

        index = FindInlineObjectIndex(object);
        if (index < 0)
            CError_FATAL(455);
        data_00582462[index].used = 1;
        return index + 1;
    }
    return 0;
}

ENode *fold_constants(ENode *node)
{
    ENode *operand;
    ENode *right;
    ENode *left;
    ENodeList *arg;

    switch (node->type) {
        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ENULLCHECK:
        case EPRECOMP:
        case ELOCOBJ:
        case ENEWEXCEPTION:
        case EMEMBER:
        case EASSBLK:
            return node;

        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
            node->data.monadic = fold_constants(node->data.monadic);
            switch ((operand = node->data.monadic)->type) {
                case EINTCONST:
                    if (((UInt8 *)node)[0] != ELOGNOT) {
                        operand->data.intval =
                            CMach_CalcIntMonadic(node->rtype, CParser_GetOperator(node->type), operand->data.intval);
                    } else {
                        operand->data.intval = CFunc_LogicalNotCInt64(operand->data.intval);
                    }
                    operand->rtype = node->rtype;
                    return operand;
                case EFLOATCONST:
                    if (((UInt8 *)node)[0] == ELOGNOT) {
                        operand->type = EINTCONST;
                        {
                            SInt32 value = CMach_FloatIsZero(operand->data.floatval.data.value);
                            operand->data.intval.lo = value;
                            operand->data.intval.hi = value < 0 ? -1 : 0;
                        }
                    } else {
                        operand->data.floatval = CMach_CalcFloatMonadic(node->rtype, CParser_GetOperator(node->type),
                                                                        operand->data.floatval.data.value);
                    }
                    operand->rtype = node->rtype;
                    return operand;
            }
            return node;

        case ETYPCON:
            node->data.monadic = fold_constants(node->data.monadic);
            switch (node->data.monadic->type) {
                case EINTCONST: {
                    SInt8 typeKind = node->rtype->type;
                    switch (typeKind) {
                        case TYPEFLOAT:
                            node->type = EFLOATCONST;
                            node->data.floatval = CMach_CalcFloatConvertFromInt(node->data.monadic->rtype,
                                                                                node->data.monadic->data.intval);
                            return node;
                        case TYPEINT:
                            node->type = EINTCONST;
                            node->data.intval = CExpr_IntConstConvert(node->rtype, node->data.monadic->rtype,
                                                                      node->data.monadic->data.intval);
                            break;
                    }
                    break;
                }
                case EFLOATCONST: {
                    SInt8 typeKind = node->rtype->type;
                    switch (typeKind) {
                        case TYPEFLOAT:
                            node->type = EFLOATCONST;
                            node->data.floatval =
                                CMachine_RoundFloatToType(node->rtype, node->data.monadic->data.floatval);
                            return node;
                        case TYPEINT:
                            node->type = EINTCONST;
                            node->data.intval = CMach_CalcIntConvertFromFloat(
                                node->rtype, node->data.monadic->data.floatval.data.value);
                            return node;
                    }
                    break;
                }
            }
            return node;

        case EPOSTINC:
        case EPOSTDEC:
            node->data.monadic = fold_constants(node->data.monadic);
            switch ((operand = node->data.monadic)->type) {
                case EINTCONST:
                case EFLOATCONST:
                    operand->rtype = node->rtype;
                    return node->data.monadic;
            }
            return node;

        case EPREINC:
        case EPREDEC:
        case EINDIRECT:
        case EFORCELOAD:
        case EBITFIELD:
            node->data.monadic = fold_constants(node->data.monadic);
            return node;

        case EMUL:
        case EDIV:
        case EMODULO:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case EAND:
        case EXOR:
        case EOR:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            if (node->data.diadic.left->type == node->data.diadic.right->type) {
                left = node->data.diadic.left;
                switch (left->type) {
                    case EINTCONST:
                        left->data.intval =
                            CMach_CalcIntDiadic(node->rtype, node->data.diadic.left->data.intval,
                                                CParser_GetOperator(node->type), node->data.diadic.right->data.intval);
                        left->rtype = node->rtype;
                        return left;
                    case EFLOATCONST:
                        left->data.floatval = CMach_CalcFloatDiadic(node->rtype, node->data.diadic.left->data.floatval,
                                                                    CParser_GetOperator(node->type),
                                                                    node->data.diadic.right->data.floatval);
                        left->rtype = node->rtype;
                        return left;
                }
            }
            return node;

        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            if (node->data.diadic.left->type == node->data.diadic.right->type) {
                left = node->data.diadic.left;
                switch (left->type) {
                    case EINTCONST:
                        left->data.intval =
                            CMach_CalcIntDiadic(left->rtype, node->data.diadic.left->data.intval,
                                                CParser_GetOperator(node->type), node->data.diadic.right->data.intval);
                        left->rtype = node->rtype;
                        return left;
                    case EFLOATCONST: {
                        ENode *floatRight = node->data.diadic.right;
                        Boolean value = CMach_CalcFloatDiadicBool(
                            left->rtype, node->data.diadic.left->data.floatval.data.value,
                            CParser_GetOperator(node->type), floatRight->data.floatval.data.value);
                        SInt32 integerValue = value;
                        left->data.intval.lo = integerValue;
                        left->data.intval.hi = integerValue < 0 ? -1 : 0;
                        left->type = EINTCONST;
                        left->rtype = node->rtype;
                        return left;
                    }
                }
            }
            return node;

        case ELAND:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            if (CExpr2_IsZero(node->data.diadic.left)) {
                return node->data.diadic.left;
            }
            if (isnotzero(node->data.diadic.left)) {
                right = fold_constants(node->data.diadic.right);
                if (right->type != EINTCONST) {
                    operand = makemonadicnode(right, ELOGNOT);
                    operand->rtype = CParser_GetBoolType();
                    right = makemonadicnode(operand, ELOGNOT);
                } else {
                    right->data.intval = CFunc_LogicalNotCInt64(CFunc_LogicalNotCInt64(right->data.intval));
                }
                return right;
            }
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            if (isnotzero(node->data.diadic.right)) {
                left = fold_constants(node->data.diadic.left);
                if (left->type != EINTCONST) {
                    operand = makemonadicnode(left, ELOGNOT);
                    operand->rtype = CParser_GetBoolType();
                    left = makemonadicnode(operand, ELOGNOT);
                } else {
                    left->data.intval = CFunc_LogicalNotCInt64(CFunc_LogicalNotCInt64(left->data.intval));
                }
                return left;
            }
            return node;

        case ELOR:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            if (CExpr2_IsZero(node->data.diadic.left)) {
                right = fold_constants(node->data.diadic.right);
                if (right->type != EINTCONST) {
                    operand = makemonadicnode(right, ELOGNOT);
                    operand->rtype = CParser_GetBoolType();
                    right = makemonadicnode(operand, ELOGNOT);
                } else {
                    right->data.intval = CFunc_LogicalNotCInt64(CFunc_LogicalNotCInt64(right->data.intval));
                }
                return right;
            }
            if (isnotzero(node->data.diadic.left)) {
                left = fold_constants(node->data.diadic.left);
                if (left->type != EINTCONST) {
                    operand = makemonadicnode(left, ELOGNOT);
                    operand->rtype = CParser_GetBoolType();
                    left = makemonadicnode(operand, ELOGNOT);
                } else {
                    left->data.intval = CFunc_LogicalNotCInt64(CFunc_LogicalNotCInt64(left->data.intval));
                }
                return left;
            }
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            if (CExpr2_IsZero(node->data.diadic.right)) {
                left = fold_constants(node->data.diadic.left);
                if (left->type != EINTCONST) {
                    operand = makemonadicnode(left, ELOGNOT);
                    operand->rtype = CParser_GetBoolType();
                    left = makemonadicnode(operand, ELOGNOT);
                } else {
                    left->data.intval = CFunc_LogicalNotCInt64(CFunc_LogicalNotCInt64(left->data.intval));
                }
                return left;
            }
            return node;

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
        case EROTL:
        case EROTR:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            return node;

        case ECOND:
            node->data.cond.cond = fold_constants(node->data.cond.cond);
            if (isnotzero(node->data.cond.cond)) {
                return fold_constants(node->data.cond.expr1);
            }
            if (CExpr2_IsZero(node->data.cond.cond)) {
                return fold_constants(node->data.cond.expr2);
            }
            node->data.cond.expr1 = fold_constants(node->data.cond.expr1);
            node->data.cond.expr2 = fold_constants(node->data.cond.expr2);
            return node;

        case EQUALNAME:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            return node;

        case EFUNCCALL:
        case EFUNCCALLP:
            node->data.funccall.funcref = fold_constants(node->data.funccall.funcref);
            for (arg = node->data.funccall.args; arg; arg = arg->next)
                arg->node = fold_constants(arg->node);
            return node;

        case EMFPOINTER:
            node->data.diadic.left = fold_constants(node->data.diadic.left);
            node->data.diadic.right = fold_constants(node->data.diadic.right);
            return node;

        case ENEWEXCEPTIONARRAY:
            arg = (ENodeList *)node->data.funccall.funcref;
            if ((operand = arg->node) != NULL) {
                operand = fold_constants(operand);
                arg = (ENodeList *)node->data.funccall.funcref;
                arg->node = operand;
            }
            return node;

        default:
            CError_FATAL(411);
            return node;
    }
}
