#define CERROR_FILE "CSOM.c"
#include "compiler/common.h"
#include "compiler/CSOM.h"
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
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
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
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include <stdio.h>
#include "compiler/Objects.h"

#include <string.h>

static FuncArg lbl_005646a0 = {NULL, NULL, NULL, TYPE(&void_ptr), 0, 0, 0};
static TypeFunc data_005646b8 = {TYPEFUNC, 0, &lbl_005646a0, NULL, TYPE(&void_ptr), 0, 0};

static struct HashNameNode *data_00581c48;
static struct HashNameNode *space_name;
static struct HashNameNode *spaces_name;
static struct HashNameNode *csom_blank_name;

struct S2;

ENode *CSOM_MakeMethodReference(BClassList *classPath, Object *method, Boolean parentResolve)
{
    TypeClass *originalClass;
    TypeClass *targetClass;
    TypeClass *methodClass;
    SInt32 methodOffset;
    SInt32 parentIndex;
    ENode *methodRef;
    ENode *result;
    Object *resolveFunction;

    CError_ASSERT(2107, classPath != NULL);
    originalClass = (TypeClass *)classPath->type;
    if (classPath->next != NULL)
        classPath = classPath->next;
    targetClass = (TypeClass *)classPath->type;
    if (parentResolve) {
        parentIndex = 0;
        if (originalClass != targetClass) {
            ClassList *base;
            for (base = originalClass->bases; base != NULL; base = base->next) {
                parentIndex++;
                if (base->base == targetClass)
                    break;
            }
            if (base == NULL)
                CError_ReportError(ERR_SOM_CLASS_ACCESS_QUALIFICATION_ONLY_ALLOWED);
        }
        find_method_vtbl_class_and_offset(targetClass, method, &methodClass, &methodOffset);
        {
            ENode *classDataRef = create_objectrefnode(methodClass->sominfo->classDataObject);
            methodRef = makediadicnode(classDataRef, intconstnode((Type *)&stsignedlong, methodOffset), EADD);
            methodRef = makemonadicnode(methodRef, EINDIRECT);
        }
        methodRef->rtype = CDecl_NewPointerType(method->type);
        resolveFunction = CSOM_004e45b0("somParentNumResolve", "ppip");
        if (resolveFunction == NULL)
            return nullnode();
        result = funccallexpr(resolveFunction, create_objectrefnode(originalClass->sominfo->classDataObject),
                              intconstnode((Type *)&stsignedint, parentIndex), methodRef, NULL);
        result->rtype = methodRef->rtype;
        if (copts.f73 != 0 && methodClass->sominfo->omitEnvironmentParameter == 0)
            result->flags |= 0x10;
    } else {
        find_method_vtbl_class_and_offset(targetClass, method, &methodClass, &methodOffset);
        if (copts.f74 != 0 && CSOM_004e3cd0(method->type) != 0)
            return create_glue_objectrefnode(methodClass, methodOffset, method);
        {
            ENode *classDataRef = create_objectrefnode(methodClass->sominfo->classDataObject);
            result = makediadicnode(classDataRef, intconstnode((Type *)&stsignedlong, methodOffset), EADD);
            result = makemonadicnode(result, EINDIRECT);
        }
        result->rtype = CDecl_NewPointerType(method->type);
        if (copts.f73 != 0 && methodClass->sominfo->omitEnvironmentParameter == 0)
            result->flags |= 0x10;
    }
    return result;
}

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

struct S3;

static inline ENode *CSOM_004e38b0_inline1(Type *v4)
{
    ObjectList *v5;
    Object *t1;
    v5 = locals;
    while ((int)v5 != 0) {
        if (v5->object.value->name == csom_blank_name) {
            return create_objectnode(v5->object.value);
        }
        v5 = v5->next;
    }
    t1 = CParser_NewLocalDataObject(NULL, 1);
    t1->name = csom_blank_name;
    t1->type = CDecl_NewPointerType(v4);
    CFunc_SetupLocalVarInfo(t1);
    return create_objectnode(t1);
}

ENode *CSOM_CreateMemberAccessExpr(BClassList *classList, ObjMemberVar *request, ENode *expr)
{
    TypeClass *base;
    TypeClass *currentClass;
    ENode *result;
    ENode *value;
    ENode *call;
    ENode *node;
    ENode *operand;
    ENode *converted;
    Object functionObject;
    if (expr == NULL && (data_00588238 == NULL || data_00588040 == NULL || data_005884f8 == 0 ||
                         (expr = CClass_CreateThisSelfExpr()) == NULL)) {
        CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
        return NULL;
    }
    CE_ASSERT(expr->type != EINDIRECT, CError_FATAL(2069));
    expr = expr->data.monadic;
    do {
        if (classList->next == NULL) {
            currentClass = data_00588040;
            if (currentClass == (base = TYPE_CLASS(classList->type)) && expr->type == EOBJREF &&
                expr->data.objref->name == this_arg_name) {
                result = (ENode *)CSOM_004e38b0_inline1((Type *)currentClass);
                break;
            }
        }
        CClass_CheckBaseAccess(classList, request->access);
        if (request->has_path != 0)
            classList = ((ObjMemberVarPath *)request)->path;
        while (classList->next != NULL)
            classList = classList->next;
        operand = (ENode *)create_objectrefnode(TYPE_CLASS(classList->type)->sominfo->classDataObject);
        node = makediadicnode(operand, intconstnode((Type *)&stsignedlong, 8), EADD);
        node->rtype = CDecl_NewPointerType(TYPE(&data_005646b8));
        value = makemonadicnode(node, EINDIRECT);
        memclrw(&functionObject, 54);
        functionObject.otype = OT_OBJECT;
        functionObject.name = unnamed_name;
        functionObject.datatype = DFUNC;
        functionObject.type = TYPE(&data_005646b8);
        call = funccallexpr(&functionObject, expr, NULL, NULL, NULL);
        CE_ASSERT(call->type != EFUNCCALL, CError_FATAL(1761));
        call->data.monadic = value;
        result = call;
    } while (0);
    converted = makemonadicnode(result, EINDIRECT);
    converted->rtype = classList->type;
    return CClass_AccessMember(converted, request->type, request->qual, request->offset);
}

static char *CSOM_CopyName(char *dst, const char *src)
{
    char c;

    while ((c = *src++) != 0)
        *dst++ = c;
    return dst;
}

ENode *create_glue_objectrefnode(TypeClass *cls, SInt32 id, Object *obj)
{
    struct CSOMRefNode *ref;
    char *argumentCode;
    char *cursor;
    char *buffer;
    Boolean memoryReturn;
    UInt32 length;
    char work[256];
    char number[16];
    Object *function;
    ENode *node;

    ref = somReferences;
    while (ref != NULL) {
        if (ref->theclass == cls && ref->id == id)
            break;
        ref = ref->next;
    }
    if (ref == NULL) {
        memoryReturn = CMachine_FunctionRequiresMemoryReturn((TypeFunc *)obj->type);
        length = strlen(cls->sominfo->classDataObject->name->name) + 32;
        if (length > sizeof(work))
            buffer = (char *)CompilerTools_AllocatePool(length);
        else
            buffer = work;
        argumentCode = CSOM_CopyName(buffer, "__glue_");
        cursor = argumentCode;
        if (cls->sominfo->omitEnvironmentParameter == 0) {
            if (memoryReturn == 0)
                *argumentCode = '4';
            else
                *argumentCode = '5';
        } else {
            *argumentCode = '_';
        }
        *++cursor = '_';
        cursor++;
        sprintf(number, "%ld", (long)strlen(cls->sominfo->classDataObject->name->name));
        cursor = CSOM_CopyName(cursor, number);
        cursor = CSOM_CopyName(cursor, cls->sominfo->classDataObject->name->name);
        *cursor = '_';
        cursor++;
        sprintf(number, "%ld", id);
        cursor = CSOM_CopyName(cursor, number);
        *cursor = 0;

        function = CParser_NewCompilerDefFunctionObject();
        function->nspace = registration_context;
        function->name = GetHashNameNode(buffer);
        function->u.func.linkname = function->name;
        function->type = obj->type;
        function->qual = obj->qual | Q_IMPLICIT_WEAK;
        function->flags = 0x10;
        CScope_AddObject(function->nspace, function->name, (ObjBase *)function);

        ref = (struct CSOMRefNode *)galloc(0x12);
        ref->next = somReferences;
        ref->object = function;
        ref->theclass = cls;
        ref->id = id;
        somReferences = ref;
        if (cls->sominfo->omitEnvironmentParameter == 0) {
            if (memoryReturn == 0)
                ref->kind = 0;
            else
                ref->kind = 1;
        } else {
            ref->kind = 2;
        }
    }
    node = create_objectrefnode(ref->object);
    node->rtype = CDecl_NewPointerType(obj->type);
    return node;
}

Boolean CSOM_004e3cd0(Type *ftype)
{
    SInt32 integerRegisters = 8;
    SInt32 floatRegisters = 13;
    FuncArg *arg;

    if (CMachine_FunctionRequiresMemoryReturn((TypeFunc *)ftype))
        integerRegisters--;

    arg = TYPE_FUNC(ftype)->args;
    while (arg != NULL) {
        if (arg == &data_00583098 || arg == &data_00584748)
            return 0;
        switch ((SInt8)arg->type->type) {
            case TYPEINT:
            case TYPEENUM:
            case TYPEPOINTER:
                if (--integerRegisters < 0)
                    return 0;
                break;
            case TYPEFLOAT:
                if (--floatRegisters < 0)
                    return 0;
                break;
            default:
                return 0;
        }
        arg = arg->next;
    }
    return 1;
}

/* In this build the temp-node kind byte compared by the original is 0x3c. */
#define ETEMP_KIND 60

#include <string.h>

ENode *CSOM_AppendPointerArgCall(ENode *node, ENodeList *spec)
{
    Type *resultType = node->rtype;
    ENodeList *pointerArg;
    ENode *pointerExpr;
    ENode *resultExpr;

    CError_ASSERT(1842, (pointerArg = node->data.funccall.args) != NULL);
    CError_ASSERT(1845, pointerArg != spec || (pointerArg = pointerArg->next) != NULL);
    pointerArg = pointerArg->next;
    CError_ASSERT(1847, pointerArg != NULL);
    CError_ASSERT(1850, pointerArg != spec || (pointerArg = pointerArg->next) != NULL);
    CError_ASSERT(1852, pointerArg->node->rtype->type == TYPEPOINTER);

    if (node->data.funccall.functype->functype->type != TYPEVOID) {
        if (spec != NULL) {
            if (spec->node->type == ETEMP_KIND) {
                if (spec->node->data.temp.uniqueid == 0)
                    spec->node->data.temp.uniqueid = CParser_GetUniqueID();
                resultExpr = CompilerTools_AllocatePool(sizeof(ENode));
                *resultExpr = *spec->node;
                resultExpr->data.temp.needs_dtor = 0;
            } else {
                resultExpr = CExpr2_RewriteExprToTemp(spec->node);
            }
        } else {
            resultExpr = CExpr2_RewriteExprToTemp(node);
        }
    } else {
        resultExpr = NULL;
    }

    if (pointerArg->node->type != EOBJREF) {
        if (pointerArg->node->type == EINDIRECT && pointerArg->node->data.monadic->type == EOBJREF &&
            pointerArg->node->data.monadic->data.objref->datatype == DLOCAL) {
            pointerExpr = CompilerTools_AllocatePool(sizeof(ENode));
            *pointerExpr = *pointerArg->node;
        } else {
            pointerExpr = CExpr2_RewriteExprToTemp(pointerArg->node);
        }
    } else {
        pointerExpr = CompilerTools_AllocatePool(sizeof(ENode));
        *pointerExpr = *pointerArg->node;
    }

    if (copts.f74 != 0) {
        ENode *appendCall;
        appendCall = funccallexpr(DAT_00588278, pointerExpr, NULL, NULL, NULL);
        node = makediadicnode(node, appendCall, ECOMMA);
        if (resultExpr != NULL)
            node = makediadicnode(node, resultExpr, ECOMMA);
    } else {
        ENode *conditional;
        ENode *appendCall;
        ENode *pointerValue;
        ENode *pointerCopy;

        pointerCopy = CompilerTools_AllocatePool(sizeof(ENode));
        *pointerCopy = *pointerExpr;
        pointerValue = makemonadicnode(pointerCopy, EINDIRECT);
        pointerValue->rtype = (Type *)&stsignedlong;
        appendCall = funccallexpr(DAT_00588278, pointerExpr, NULL, NULL, NULL);
        conditional = CompilerTools_AllocatePool(sizeof(ENode));
        conditional->type = ECOND;
        conditional->cost = 0;
        conditional->flags = 0;
        conditional->rtype = &stvoid;
        conditional->data.cond.cond = pointerValue;
        conditional->data.cond.expr1 = appendCall;
        conditional->data.cond.expr2 = nullnode();
        conditional->data.cond.expr2->rtype = &stvoid;
        if (node != NULL)
            conditional = makediadicnode(node, conditional, ECOMMA);
        if (resultExpr != NULL) {
            conditional = makediadicnode(conditional, resultExpr, ECOMMA);
            conditional->rtype = resultExpr->rtype;
        }
        node = conditional;
    }

    node->rtype = resultType;
    return node;
}

void CSOM_GenerateSomselfAssignment(TypeClass *tclass, Statement *stmt)
{
    HashNameNode *name;
    ObjectList *ivar;
    ENode *somself;
    ENode *expr;
    Object obj;
    Statement *s;
    ENode *call;

    name = GetHashNameNode("__somself");
    for (ivar = locals; ivar; ivar = ivar->next) {
        if (ivar->object.value->name == name) {
            somself = CClass_CreateThisSelfExpr();
            CError_ASSERT(1811, somself != NULL);
            expr = create_objectrefnode(tclass->sominfo->classDataObject);
            expr = makediadicnode(expr, intconstnode((Type *)&stsignedlong, 8), EADD);
            expr->rtype = CDecl_NewPointerType(TYPE(&data_005646b8));
            expr = makemonadicnode(expr, EINDIRECT);
            memclrw(&obj, sizeof(Object));
            obj.otype = 5;
            obj.name = unnamed_name;
            obj.datatype = DFUNC;
            obj.type = TYPE(&data_005646b8);
            call = funccallexpr(&obj, somself, NULL, NULL, NULL);
            CError_ASSERT(1761, call->type == EFUNCCALL);
            call->data.funccall.funcref = expr;
            s = CFunc_InsertAfterStatement(4, stmt);
            s->expr.expression = makediadicnode(create_objectnode(ivar->object.value), call, EASS);
            break;
        }
    }
}

ENode *CSOM_GetOrCreateLocalObjectNode(TypeClass *value)
{
    Object *object;
    ObjectList *node;

    for (node = locals; node != NULL; node = node->next) {
        if (node->object.value->name == csom_blank_name) {
            object = node->object.value;
            return create_objectnode(object);
        }
    }
    object = CParser_NewLocalDataObject(NULL, 1);
    object->name = csom_blank_name;
    object->type = (Type *)CDecl_NewPointerType((Type *)value);
    CFunc_SetupLocalVarInfo(object);
    return create_objectnode(object);
}

#define METHODTYPE(ty) ((TypeMemberFunc *)(ty))

void find_method_vtbl_class_and_offset(TypeClass *cls, Object *method, TypeClass **outcls, SInt32 *outofs)
{
    struct ScopeSearch state;
    VClassList *vbase;
    Object *found;
    UInt16 vtblIndex;

    if ((METHODTYPE(method->type)->flags & 0x20) == 0) {
        CScope_InitScopeSearch(&state, cls->nspace);
        for (;;) {
            found = CScope_NextObject(&state);
            if (found == NULL)
                break;
            if (found == method) {
                *outcls = cls;
                CError_ASSERT(173, found->type->type == TYPEFUNC && (METHODTYPE(found->type)->flags & FUNC_METHOD));
                vtblIndex = METHODTYPE(found->type)->vtbl_index;
                *outofs = vtblIndex * 4 + 0x18;
                return;
            }
        }
        for (vbase = cls->vbases; vbase != NULL; vbase = vbase->next) {
            CScope_InitScopeSearch(&state, vbase->base->nspace);
            for (;;) {
                found = CScope_NextObject(&state);
                if (found == NULL)
                    break;
                if (found == method) {
                    *outcls = vbase->base;
                    CError_ASSERT(173, found->type->type == TYPEFUNC && (METHODTYPE(found->type)->flags & FUNC_METHOD));
                    vtblIndex = METHODTYPE(found->type)->vtbl_index;
                    *outofs = vtblIndex * 4 + 0x18;
                    return;
                }
            }
        }
    } else {
        for (vbase = cls->vbases; vbase != NULL; vbase = vbase->next) {
            CScope_InitScopeSearch(&state, vbase->base->nspace);
            for (;;) {
                found = CScope_NextObject(&state);
                if (found == NULL)
                    break;
                if (found->name == method->name) {
                    if (found->type->type == TYPEFUNC && found->datatype == DVFUNC &&
                        (METHODTYPE(found->type)->flags & 0x20) == 0 &&
                        CClass_GetOverrideKind(TYPE_FUNC(method->type), TYPE_FUNC(found->type), 0)) {
                        *outcls = vbase->base;
                        CError_ASSERT(173,
                                      found->type->type == TYPEFUNC && (METHODTYPE(found->type)->flags & FUNC_METHOD));
                        vtblIndex = METHODTYPE(found->type)->vtbl_index;
                        *outofs = vtblIndex * 4 + 0x18;
                        return;
                    }
                    break;
                }
            }
        }
    }
    CError_FATAL(1731);
}

void CSOM_004e4390(Object *obj)
{
    TypeClass *type;
    Statement *node;
    Object *method;

    method = CSOM_004e45b0("somReleaseObjectReference", "pp");
    if (method != NULL) {
        type = TYPE_CLASS(obj->type);
        obj->type = CDecl_NewPointerType((Type *)type);
        TYPE_POINTER(obj->type)->qual = Q_REFERENCE;
        node = CFunc_AppendStatement(EINDIRECT);
        node->expr.expression = makediadicnode(CExpr_New_EINDIRECT_Node(obj), CSOM_BuildNewObjectInstance(type), EASS);
        CExcept_RegisterDeleteObject(node, obj, method);
    }
}

static TypeClass *GetOwner(void)
{
    TypeClass *o;
    if (!(o = currentNameSpace->theclass) || !o->sominfo) {
        CError_ReportError(ERR_ILLEGAL_USE_PRAGMA_OUTSIDE_SOM_CLASS);
        o = NULL;
    }
    return o;
}

static inline TypeClass *CSOM_004e4a30_inline1(void)
{
    TypeClass *v0;
    if (((int)(v0 = currentNameSpace->theclass)) == 0 || v0->sominfo == NULL) {
        CError_ReportError(ERR_ILLEGAL_USE_PRAGMA_OUTSIDE_SOM_CLASS);
        v0 = (TypeClass *)0;
    }
    return v0;
}

/* 0x441a70, byte-swap 32 (conditional) */
/* 0x441ab0, byte-swap 16 (conditional) */

static inline Object *MakeKinds(SOMClassBuildState *info)
{
    SOMEntry *n;
    int size;
    UInt8 *bits;
    UInt8 *p;
    unsigned int i;
    int t;

    size = (info->memberCount + 1) / 2;
    memclrw(bits = (UInt8 *)CompilerTools_AllocatePool(size), size);
    for (n = (SOMEntry *)info->members, i = 0; n != NULL; n = n->next, i++) {
        switch (n->kind) {
            case 0:
            case 2:
                t = i;
                p = bits;
                p += t >> 1;
                if (t & 1)
                    *p = (*p & 0xf0) | 3;
                else
                    *p = (*p & 0x0f) | 0x30;
                break;
            case 1:
                t = i;
                p = bits;
                p += t >> 1;
                if (t & 1)
                    *p = *p & 0xf0;
                else
                    *p = *p & 0x0f;
                break;

            default:
                CError_FATAL(1048);
                break;
        }
    }
    return CInit_DeclareString((char *)bits, size, 0, 0);
}

static Object *FlushData(void)
{
    Object *data;
    fn_00443190(data_00583548.data);
    data = CInit_DeclareString(*data_00583548.data, data_00583548.size, 0, 0);
    fn_004431b0(data_00583548.data);
    return data;
}

static void *MakeOverrides(SOMClassBuildState *rec)
{
    SOMEntry *p;
    SOMEntry *ps;

    data_00583548.size = 0;
    p = ps = (SOMEntry *)rec->members;
    if (ps) {
        do {
            if (p->kind == 1)
                encode_member_function_types(TYPE_METHOD(p->u.object->type), 1);
        } while ((p = p->next) != NULL);
    }
    return FlushData();
}

static void *MakeWords(SOMClassBuildState *rec)
{
    SOMEntry *p;
    SOMEntry *ps;
    int i;

    data_00583548.size = 0;
    p = ps = (SOMEntry *)rec->members;
    i = 0;
    if (ps) {
        do {
            if (p->kind == 2) {
                AppendGListWord(&data_00583548, CTool_EndianConvertWord16(p->u.vt.offset));
                AppendGListWord(&data_00583548, CTool_EndianConvertWord16(p->u.vt.index));
                AppendGListWord(&data_00583548, CTool_EndianConvertWord16(i));
            }
            i++;
        } while ((p = p->next) != NULL);
    }
    return FlushData();
}

/* The object is a nibble bit-vector builder over a linked list of records
 * that carry a one-byte kind at +0xc; the record count lives at +0x2a. */

static inline void CSOM_SetNibble(UInt8 *bits, int i, int value)
{
    UInt8 *p;
    p = bits;
    p += i >> 1;
    if (i & 1)
        *p = (*p & 0xf0) | value;
    else
        *p = (*p & 0x0f) | (value << 4);
}

static void setnumber(int raw, int n)
{
    Object *item = (Object *)raw;
    if (n == 0 && copts.f9d != 0)
        CError_Warning(ERR_SOM_CLASS_NO_RELEASE_ORDER_LIST);
    ((TypeMemberFunc *)item->type)->vtbl_index = n;
}

static SOMInfoEntry *listhead(VClassList *v8)
{
    struct SOMInfo *p = ((TypeClass *)v8->base)->sominfo;
    SOMInfoEntry *h = p->methodNameList;
    (void)h;
    return h;
}

static int basehead(TypeClass *p)
{
    int h = (int)p->vbases;
    (void)h;
    return h;
}

ENode *CSOM_CallReleaseObjectReference(TypeClass *unused, ENode *argument)
{
    Object *object;

    object = CSOM_004e45b0("somReleaseObjectReference", "pp");
    if (object != NULL) {
        return funccallexpr(object, argument, NULL, NULL, NULL);
    }
    return nullnode();
}

ENode *CSOM_BuildNewObjectInstance(TypeClass *cls)
{
    Object *obj;
    ENode *callnode;
    ENode *temp;
    ENode *monadic;
    ENode *call2;

    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == ')') {
            tk = CPrepTokenizer_GetNextToken();
        } else {
            CError_ReportError(ERR_NO_PARAMETERS_ALLOWED_SOM_CLASS_CONSTRUCTORS);
        }
    }

    if (copts.f73 == 0 || copts.f74 == 0) {
        obj = CSOM_004e45b0("somNewObjectInstance", "ppll");
        if (obj == NULL)
            return nullnode();
    } else {
        obj = DAT_00588060;
    }

    callnode = funccallexpr(obj, create_objectrefnode(cls->sominfo->classDataObject),
                            intconstnode((Type *)&stunsignedlong, cls->sominfo->descriptorValue0),
                            intconstnode((Type *)&stunsignedlong, cls->sominfo->descriptorValue1), NULL);
    callnode->rtype = CDecl_NewPointerType((Type *)cls);

    if (copts.f73 != 0 && copts.f74 == 0) {
        temp = CExpr2_RewriteExprToTemp(callnode);
        call2 = funccallexpr(DAT_005876c0, nullnode(), NULL, NULL, NULL);
        monadic = makemonadicnode(callnode, ELOGNOT);
        monadic->rtype = CParser_GetBoolType();
        callnode = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        callnode->type = ECOND;
        callnode->cost = 0;
        callnode->flags = 0;
        callnode->rtype = &stvoid;
        callnode->data.cond.cond = monadic;
        callnode->data.cond.expr1 = call2;
        callnode->data.cond.expr2 = nullnode();
        callnode->data.cond.expr2->rtype = &stvoid;
        if (temp != NULL) {
            callnode = makediadicnode(callnode, temp, ECOMMA);
            callnode->rtype = temp->rtype;
        }
    }
    return callnode;
}

Object *CSOM_004e45b0(char *name, char *signature)
{
    ObjectList *list;
    Object *obj;
    FuncArg *arg;

    list = CScope_FindObjectListInNameSpace(registration_context, GetHashNameNode(name));
    if (list != NULL && (obj = list->object.value)->otype == OT_OBJECT) {
        if (obj->type->type == TYPEFUNC && *signature++ == 'p' && TYPE_FUNC(obj->type)->functype->type == TYPEPOINTER) {
            for (arg = TYPE_FUNC(obj->type)->args; arg != NULL; arg = arg->next) {
                switch (*signature++) {
                    case 'p':
                        if (arg->type->type != TYPEPOINTER)
                            break;
                        continue;
                    case 'i':
                        if (arg->type != (Type *)&stsignedint)
                            break;
                        continue;
                    case 'I':
                        if (&arg->type->type != &stunsignedint.type)
                            break;
                        continue;
                    case 'l':
                        if (arg->type != (Type *)&stsignedlong)
                            break;
                        continue;
                    case 'L':
                        if (arg->type != (Type *)&stunsignedlong)
                            break;
                        continue;
                    default:
                        break;
                }
                break;
            }
            if (arg == NULL && *signature == '\0')
                return obj;
        }
        CError_ReportError(ERR_SOM_RUNTIME_FUNCTION_UNEXPECTED_TYPE, name);
    } else {
        CError_ReportError(ERR_SOM_RUNTIME_FUNCTION_NOT_DEFINED_SHOULD, name);
    }
    return NULL;
}

void CSOM_PrependTheClassArg(TypeFunc *function)
{
    Type *type;
    TypeFunc *func;
    FuncArg *arg;
    HashNameNode *name;
    func = function;
    arg = CParser_NewFuncArg();
    arg->name = GetHashNameNode("__theclass");
    name = GetHashNameNode("SOMClass");
    type = CScope_FindTagType(currentNameSpace, name);
    if (type == NULL) {
        fn_0043f3e0(281U, name->name);
        type = &stvoid;
    }
    arg->type = CDecl_NewPointerType(type);
    arg->next = func->args;
    func->args = arg;
}

void set_owner_target_flag(void)
{
    TypeClass *owner;
    if (!(owner = GetOwner()))
        return;
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(0x6b);
        return;
    }
    if (!memcmp(data_00587fa0->name, "IDL", 4)) {
        owner->sominfo->omitEnvironmentParameter = 0;
        return;
    }
    if (!memcmp(data_00587fa0->name, "OIDL", 5)) {
        owner->sominfo->omitEnvironmentParameter = 1;
        return;
    }
    CPrep_ReportError(0xba);
}

void CSOM_ParseBaseClass(void)
{
    Type *cls;
    Type *base;

    if (CPrep_ExpectEndLine(0) != 0x28) {
        CPrep_ReportError(0x72);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(0x6b);
        return;
    }
    cls = CScope_FindTagType(currentNameSpace, data_00587fa0);
    if (cls == NULL || !IS_TYPE_CLASS(cls) || TYPE_CLASS(cls)->sominfo == NULL) {
        fn_0043f3e0(0x114, data_00587fa0->name);
        return;
    }
    if (CPrep_ExpectEndLine(0) != 0x2c) {
        CPrep_ReportError(0x74);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(0x6b);
        return;
    }
    base = CScope_FindTagType(currentNameSpace, data_00587fa0);
    if (base == NULL || !IS_TYPE_CLASS(base) || TYPE_CLASS(base)->sominfo == NULL) {
        fn_0043f3e0(0x114, data_00587fa0->name);
        return;
    }
    TYPE_CLASS(cls)->sominfo->baseClass = TYPE_CLASS(base);
    if (CPrep_ExpectEndLine(0) != 0x29) {
        CPrep_ReportError(0x73);
        return;
    }
}

void CSOM_ParseDescriptorValues(void)
{
    Type *theclass;
    if (CPrep_ExpectEndLine(0) != '(') {
        CPrep_ReportError(114);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(107);
        return;
    }
    theclass = CScope_FindTagType(currentNameSpace, data_00587fa0);
    if (!theclass || theclass->type != TYPECLASS || !TYPE_CLASS(theclass)->sominfo) {
        fn_0043f3e0(276, data_00587fa0->name);
        return;
    }
    if (CPrep_ExpectEndLine(0) != ',') {
        CPrep_ReportError(116);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -1) {
        CPrep_ReportError(186);
        return;
    }
    TYPE_CLASS(theclass)->sominfo->descriptorValue0 = intconst_lo;
    if (CPrep_ExpectEndLine(0) != ',') {
        CPrep_ReportError(116);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -1) {
        CPrep_ReportError(186);
        return;
    }
    TYPE_CLASS(theclass)->sominfo->descriptorValue1 = intconst_lo;
    if (CPrep_ExpectEndLine(0) != ')') {
        CPrep_ReportError(115);
        return;
    }
}

void CSOM_ParseMethodNameList(void)
{
    char bare;
    short token;
    TypeClass *theclass;
    SOMInfoEntry **tail;
    SOMInfoEntry *entry;
    short delimiter;
    SOMInfoEntry *newEntry;
    struct SOMPragmaNames names;
    int keywordLength;
    keywordLength = sizeof("list");
    if (!(theclass = CSOM_004e4a30_inline1())) {
        return;
    }
    token = CPrep_ExpectEndLine(0);
    if (token != 40) {
        if (token != -3) {
            CPrep_ReportError(114);
            return;
        }
        if (memcmp(data_00587fa0->name, "list", keywordLength) == 0) {
            token = CPrep_ExpectEndLine(0);
            if (token != -3) {
                CPrep_ReportError(107);
                return;
            }
        }
        bare = 1;
    } else {
        bare = 0;
        token = CPrep_ExpectEndLine(0);
    }
    names.head = NULL;
    if (bare != 0 || token != 41) {
        tail = &names.head;
        for (;;) {
            if (token != -3) {
                CPrep_ReportError(107);
                return;
            }
            for (entry = names.head; entry != NULL; entry = entry->next) {
                if (entry->name == data_00587fa0) {
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, data_00587fa0->name);
                    return;
                }
            }
            newEntry = (SOMInfoEntry *)galloc(10);
            *tail = newEntry;
            newEntry->next = NULL;
            tail = &newEntry->next;
            newEntry->name = data_00587fa0;
            newEntry->kind = 0;
            if (bare != 0) {
                delimiter = CPrep_ExpectEndLine(1);
                if (delimiter == 0) {
                    break;
                }
            } else {
                delimiter = CPrep_ExpectEndLine(0);
                if (delimiter == 41) {
                    break;
                }
            }
            if (delimiter != 44) {
                CPrep_ReportError(116);
                return;
            }
            token = CPrep_ExpectEndLine(bare);
        }
    }
    theclass->sominfo->methodNameList = names.head;
}

void CSOM_BuildClass(TypeClass *func)
{
    SOMClassBuildState state;
    TypeFunc *ft;
    Object *obj;

    memclrw(&state, sizeof(state));
    build_class_vtables_and_members(&state, func);
    create_ancestor_object(&state, func);
    create_override_methods_object(&state, func);

    ft = galloc(sizeof(TypeFunc));
    memclrw(ft, sizeof(TypeFunc));
    ft->type = TYPEFUNC;
    ft->functype = &stvoid;

    obj = CParser_NewCompilerDefFunctionObject();
    obj->type = (Type *)ft;
    obj->sclass = TK_STATIC;
    obj->name = CParser_NameConcat(func->classname->name, "DLLD");
    if (CScope_FindObjectListInNameSpace(registration_context, obj->name) != NULL)
        CError_ReportError(ERR_OBJECT_REDEFINED, obj);
    state.registrationFunction = obj;
    CFunc_GenerateDummyFunction(obj);
    create_special_functions_object(&state, func);
    make_class_descriptor(&state, func);
    initialize_class_data_object(&state, func);
}

void initialize_class_data_object(SOMClassBuildState *methods, TypeClass *tclass)
{
    RelocationList *list;
    SInt32 offset;
    RelocationList *init;
    SOMEntry *m;
    ENode *buf;

    list = NULL;
    offset = 0x18;
    for (m = (SOMEntry *)methods->members; m; m = m->next) {
        if (m->kind == 1) {
            init = (RelocationList *)(CompilerTools_AllocatePool(sizeof(RelocationList)));
            init->next = list;
            list = init;
            init->object = m->u.object;
            init->offset = offset;
            init->addend = 0;
        }
        offset += 4;
    }

    memclrw(buf = CompilerTools_AllocatePool(offset), offset);

    init = (RelocationList *)(CompilerTools_AllocatePool(sizeof(RelocationList)));
    init->next = list;
    init->object = methods->object;
    init->offset = 4;
    init->addend = 0;

    tclass->sominfo->classDataObject->type->size = offset;
    fn_004ceab0(tclass->sominfo->classDataObject, buf, init, tclass->sominfo->classDataObject->type->size);
}

void make_class_descriptor(SOMClassBuildState *record, TypeClass *classType)
{
    SOMClassDescriptor descriptor;
    struct SOMDescriptorOutput descriptorText;
    char *className;
    Object *descriptorObject;
    RelocationList *head;
    RelocationList *node;
    SInt32 index;
    SInt32 *baseValues;
    SOMVTable *entry;
    SOMVTable *firstEntry;
    SInt32 size;
    HashNameNode *memberName;
    SOMEntry *memberEntry;

    className = classType->classname->name;
    descriptorObject = CParser_NewCompilerDefDataObject();
    descriptorObject->name = CParser_NameConcat(className, "SCI");
    descriptorObject->type = CDecl_NewStructType(sizeof(descriptor), 4);
    CScope_AddObject(descriptorObject->nspace, descriptorObject->name, (ObjBase *)descriptorObject);
    descriptorObject->sclass = TK_STATIC;
    memclrw(&descriptor, sizeof(descriptor));
    descriptor.value0 = CTool_EndianConvertWord32(0x46);

    head = NULL, node = CompilerTools_AllocatePool(sizeof(*node));
    node->next = head;
    node->object = classType->sominfo->classDataObject;
    head = node;
    node->offset = 4;
    node->addend = 0;
    if (record->overrideMethodsObject != NULL) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = record->overrideMethodsObject;
        node->offset = 8;
        node->addend = 0;
    }
    if (record->ancestorObject != NULL) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = record->ancestorObject;
        node->offset = 0xc;
        node->addend = 0;
    }
    if (record->registrationFunction != NULL) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = record->registrationFunction;
        node->offset = 0x10;
        node->addend = 0;
    }
    if (record->specialFunctionsObject != NULL) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = record->specialFunctionsObject;
        node->offset = 0x14;
        node->addend = 0;
    }

    build_descriptor_output(record, classType, &descriptorText);
    node = CompilerTools_AllocatePool(sizeof(*node));
    node->next = head;
    head = node;
    node->object = CInit_DeclareString((char *)&descriptorText, sizeof(descriptorText), 0, 0);
    node->offset = 0x34;
    node->addend = 0;

    node = CompilerTools_AllocatePool(sizeof(*node));
    node->next = head;
    head = node;
    node->object = CInit_DeclareString(classType->classname->name, strlen(classType->classname->name) + 1, 0, 0);
    node->offset = 0x38;
    node->addend = 0;

    descriptor.classSize = CTool_EndianConvertWord32(classType->size);

    node = CompilerTools_AllocatePool(sizeof(*node));
    node->next = head;
    head = node;
    size = (record->directBaseCount + record->implicitBaseCount) * (2 * sizeof(*baseValues));
    baseValues = CompilerTools_AllocatePool(size);
    firstEntry = (SOMVTable *)record->bases;
    entry = firstEntry;
    index = 0;
    for (; entry; entry = entry->next) {
        if (entry->isImplicitBase != 0 || entry->isDirectBase != 0) {
            baseValues[index++] = CTool_EndianConvertWord32(entry->base->sominfo->descriptorValue0);
            baseValues[index++] = CTool_EndianConvertWord32(entry->base->sominfo->descriptorValue1);
        }
    }
    node->object = CInit_DeclareString((char *)baseValues, size, 0, 0);
    node->offset = 0x40;
    node->addend = 0;

    if (record->members != NULL) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = MakeKinds((SOMClassBuildState *)record);
        node->offset = 0x44;
        node->addend = 0;

        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = MakeOverrides((SOMClassBuildState *)record);
        node->offset = 0x48;
        node->addend = 0;

        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        data_00583548.size = 0;
        memberEntry = record->members;
        for (; memberEntry; memberEntry = memberEntry->next) {
            if ((memberName = memberEntry->key) != NULL) {
                if (memberName == constructor_name)
                    memberName = data_00581c48;
                else if (memberName == destructor_name)
                    memberName = space_name;
                AppendGListName(&data_00583548, memberName->name);
            }
        }
        node->object = FlushData();
        node->offset = 0x4c;
        node->addend = 0;
    }

    if (record->overrideMethodsObject != NULL) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = build_base_method_vtbl_index_object(record);
        node->offset = 0x50;
        node->addend = 0;
    }
    if (record->inheritedMemberCount != 0) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        node->next = head;
        head = node;
        node->object = MakeWords((SOMClassBuildState *)record);
        node->offset = 0x54;
        node->addend = 0;
    }

    descriptor.value88 = 0;
    fn_004ceab0(descriptorObject, &descriptor, head, descriptorObject->type->size);
    record->object = descriptorObject;
}

void build_descriptor_output(SOMClassBuildState *desc, TypeClass *cls, struct SOMDescriptorOutput *out)
{
    desc->descriptorValues[0] = cls->sominfo->descriptorValue0;
    desc->descriptorValues[1] = cls->sominfo->descriptorValue1;
    desc->descriptorFlags = 1;
    if (desc->hasNewOperator)
        desc->descriptorFlags |= 0x100;
    if (desc->hasDeleteOperator)
        desc->descriptorFlags |= 0x200;

    switch (cls->align) {
        case 1:
            desc->alignmentKind = 0;
            break;
        case 2:
            desc->alignmentKind = 1;
            break;
        case 4:
            desc->alignmentKind = 2;
            break;
        case 8:
            desc->alignmentKind = 3;
            break;
        default:
            desc->alignmentKind = 4;
    }

    desc->descriptorAttribute5 = 0;

    memclrw(out, sizeof(*out));

    out->descriptorValues[0] = CTool_EndianConvertWord32(desc->descriptorValues[0]);
    out->descriptorValues[1] = CTool_EndianConvertWord32(desc->descriptorValues[1]);
    out->descriptorFlags = CTool_EndianConvertWord32(desc->descriptorFlags);
    out->alignmentKind = CTool_EndianConvertWord16(desc->alignmentKind);
    out->memberCount = CTool_EndianConvertWord16(desc->memberCount);
    out->directBaseCount = CTool_EndianConvertWord16(desc->directBaseCount);
    out->implicitBaseCount = CTool_EndianConvertWord16(desc->implicitBaseCount);
    out->overrideBaseCount = CTool_EndianConvertWord16(desc->overrideBaseCount);
    out->inheritedMemberCount = CTool_EndianConvertWord16(desc->inheritedMemberCount);
    out->descriptorAttribute5 = CTool_EndianConvertWord16(desc->descriptorAttribute5);
}

void emit_som_kind_nibbles(SOMClassBuildState *info)
{
    SOMEntry *n;
    int size;
    UInt8 *bits;
    int i;

    size = (info->memberCount + 1) / 2;
    memclrw(bits = CompilerTools_AllocatePool(size), size);
    for (n = info->members, i = 0; n != NULL; n = n->next, i++) {
        switch (n->kind) {
            case 0:
            case 2:
                CSOM_SetNibble(bits, i, 3);
                break;
            case 1:
                CSOM_SetNibble(bits, i, 0);
                break;
            default:
                CError_FATAL(1048);
                break;
        }
    }
    CInit_DeclareString((char *)bits, size, 0, 0);
}

void create_special_functions_object(SOMClassBuildState *info, TypeClass *cls)
{
    ScopeSearch search;
    char initialData[16];
    RelocationList *relocations;
    int dataSize;
    Object *object;
    Object *newOperator;
    Object *deleteOperator;
    RelocationList *relocation;
    char *className;

    newOperator = deleteOperator = NULL;
    CScope_InitScopeSearch(&search, cls->nspace);
    for (;;) {
        object = CScope_NextObject(&search);
        if (object == NULL)
            break;
        if (object->type->type == TYPEFUNC) {
            if (object->name == CMangler_OperatorName(TK_NEW)) {
                newOperator = object;
                info->hasNewOperator = 1;
            } else if (object->name == CMangler_OperatorName(TK_DELETE)) {
                deleteOperator = object;
                info->hasDeleteOperator = 1;
            }
        }
    }
    if (newOperator != NULL || deleteOperator != NULL) {
        className = cls->classname->name;
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat(className, "SpecialProcs");
        object->type = CDecl_NewStructType(4, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        object->sclass = TK_STATIC;
        relocations = NULL;
        dataSize = 0;
        if (newOperator != NULL) {
            relocation = (RelocationList *)CompilerTools_AllocatePool(sizeof(RelocationList));
            relocation->next = relocations;
            relocations = relocation;
            relocation->object = newOperator;
            relocation->offset = dataSize;
            relocation->addend = 0;
            dataSize += 4;
        }
        if (deleteOperator != NULL) {
            relocation = (RelocationList *)CompilerTools_AllocatePool(sizeof(RelocationList));
            relocation->next = relocations;
            relocations = relocation;
            relocation->object = deleteOperator;
            relocation->offset = dataSize;
            relocation->addend = 0;
            dataSize += 4;
        }
        memclrw(initialData, sizeof(initialData));
        object->type->size = dataSize;
        fn_004ceab0(object, initialData, relocations, object->type->size);
        info->specialFunctionsObject = object;
    }
}

Object *build_base_method_vtbl_index_object(SOMClassBuildState *groups)
{
    SOMVTable *group;
    int groupIndex;
    SOMSlot *method;
    Object *object;
    unsigned int count;
    Object *result;

    data_00583548.size = 0;
    groupIndex = 0;
    group = groups->bases;
    while (group != NULL) {
        if (group->slots != NULL) {
            AppendGListWord(&data_00583548, CTool_EndianConvertWord16(groupIndex));
            method = group->slots;
            count = 0;
            while (method != NULL) {
                method = method->next;
                count++;
            }
            AppendGListWord(&data_00583548, CTool_EndianConvertWord16(count));
            method = group->slots;
            while (method != NULL) {
                object = method->baseMethod;
                if (!(object->type->type == TYPEFUNC && (((TypeMemberFunc *)object->type)->flags & FUNC_METHOD)))
                    CError_FATAL(173);
                AppendGListWord(&data_00583548,
                                CTool_EndianConvertWord16(((TypeMemberFunc *)object->type)->vtbl_index));
                method = method->next;
            }
        }
        group = group->next;
        groupIndex++;
    }
    fn_00443190(data_00583548.data);
    result = CInit_DeclareString(*data_00583548.data, data_00583548.size, 0, 0);
    fn_004431b0(data_00583548.data);
    return result;
}

void create_override_methods_object(SOMClassBuildState *cls, TypeClass *func)
{
    Object *obj;
    SInt32 size;
    char *s;
    SOMSlot *e;
    SOMVTable *b;
    RelocationList *list;
    SInt32 offset;
    RelocationList *op;
    ENode *array;

    if (cls->overrideMethodCount) {
        size = cls->overrideMethodCount * 4;
        s = func->classname->name;
        obj = CParser_NewCompilerDefDataObject();
        obj->name = CParser_NameConcat(s, "OverrideProcs");
        obj->type = CDecl_NewStructType(size, 4);
        CScope_AddObject(obj->nspace, obj->name, (ObjBase *)obj);
        obj->sclass = TK_STATIC;

        list = NULL;
        offset = 0;
        for (b = cls->bases; b != NULL; b = b->next) {
            if (b->slots != NULL) {
                for (e = b->slots; e != NULL; e = e->next) {
                    op = (RelocationList *)CompilerTools_AllocatePool(0x10);
                    op->next = list;
                    list = op;
                    op->object = e->overrideMethod;
                    op->offset = offset;
                    op->addend = 0;
                    offset += 4;
                }
            }
        }
        memclrw(array = CompilerTools_AllocatePool(size), size);
        fn_004ceab0(obj, array, list, obj->type->size);
        cls->overrideMethodsObject = obj;
    }
}

void create_ancestor_object(SOMClassBuildState *info, TypeClass *cls)
{
    SOMVTable *base;
    RelocationList *head;
    SInt32 count;
    Object *obj;

    if (info->bases != NULL) {
        char *clsname = cls->classname->name;
        obj = CParser_NewCompilerDefDataObject();
        obj->name = CParser_NameConcat(clsname, "ClassAncestors");
        obj->type = CDecl_NewStructType(4, 4);
        CScope_AddObject(obj->nspace, obj->name, (ObjBase *)obj);
        obj->sclass = TK_STATIC;

        head = NULL;
        count = 0;
        for (base = info->bases; base != NULL; base = base->next) {
            RelocationList *n = (RelocationList *)CompilerTools_AllocatePool(0x10);
            n->next = head;
            head = n;
            n->object = base->base->sominfo->classDataObject;
            n->offset = count;
            n->addend = 0;
            count += 4;
        }
        {
            char *buf;
            memclrw(buf = (char *)CompilerTools_AllocatePool(count), count);
            obj->type->size = count;
            fn_004ceab0(obj, buf, head, obj->type->size);
        }
        info->ancestorObject = obj;
    }
}

#include <stddef.h>

void build_class_vtables_and_members(SOMClassBuildState *layout, TypeClass *cls)
{
    ClassList *baseClass;
    SOMVTable *vtable;
    Object *method;
    VClassList *virtualBase;
    Object *candidate;
    ScopeSearch classScope;
    ScopeSearch baseScope;
    SOMEntry *newEntry;
    SOMEntry **entryLink;
    SOMSlot *overrideSlot;
    SInt32 entryIndex;
    SOMInfoEntry *infoEntry;
    SOMInfoEntry *baseEntry;
    SInt32 baseIndex;
    Object **methods;
    SInt32 methodCount;
    SInt32 methodIndex;
    Object *object;
    HashNameNode *key;

    for (baseClass = cls->bases; baseClass != NULL; baseClass = baseClass->next) {
        vtable = find_or_add_base(layout, cls, baseClass->base, NULL);
        vtable->isDirectBase = 1;
        layout->directBaseCount++;
    }

    if (cls->sominfo->baseClass != NULL && cls->sominfo->baseClass->sominfo != NULL) {
        vtable = find_or_add_base(layout, cls, cls->sominfo->baseClass, NULL);
        vtable->isImplicitBase = 1;
        layout->implicitBaseCount++;
    }

    CScope_InitScopeSearch(&classScope, cls->nspace);
    for (;;) {
        method = CScope_NextObject(&classScope);
        if (method == NULL)
            break;
        if (method->type->type != TYPEFUNC)
            continue;
        if ((TYPE_FUNC(method->type)->flags & 0x20) == 0)
            continue;
        for (virtualBase = cls->vbases; virtualBase != NULL; virtualBase = virtualBase->next) {
            CScope_InitScopeSearch(&baseScope, virtualBase->base->nspace);
            for (;;) {
                candidate = CScope_NextObject(&baseScope);
                if (candidate == NULL)
                    break;
                if (candidate->type->type != TYPEFUNC)
                    continue;
                if (method->name != candidate->name)
                    continue;
                if (candidate->datatype != DVFUNC)
                    continue;
                if ((TYPE_FUNC(candidate->type)->flags & 0x20) != 0)
                    continue;
                if (CClass_GetOverrideKind(TYPE_FUNC(method->type), TYPE_FUNC(candidate->type), 0) == 0)
                    continue;
                vtable = find_or_add_base(layout, cls, virtualBase->base, NULL);
                if ((overrideSlot = vtable->slots) != NULL) {
                    overrideSlot = (SOMSlot *)CompilerTools_AllocatePool(sizeof(*overrideSlot));
                    memclrw(overrideSlot, sizeof(*overrideSlot));
                    overrideSlot->next = vtable->slots;
                    vtable->slots = overrideSlot;
                } else {
                    overrideSlot = (SOMSlot *)CompilerTools_AllocatePool(sizeof(*overrideSlot));
                    memclrw(overrideSlot, sizeof(*overrideSlot));
                    vtable->slots = overrideSlot;
                    layout->overrideBaseCount++;
                }
                overrideSlot->overrideMethod = method;
                overrideSlot->baseMethod = candidate;
                break;
            }
        }
        layout->overrideMethodCount++;
    }

    entryLink = &layout->members;
    if (cls->sominfo->methodNameList != NULL) {
        for (infoEntry = cls->sominfo->methodNameList, entryIndex = 0; infoEntry != NULL;
             infoEntry = infoEntry->next, entryIndex++) {
            newEntry = (SOMEntry *)CompilerTools_AllocatePool(offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
            memclrw(newEntry, offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
            *entryLink = newEntry;
            entryLink = &newEntry->next;
            newEntry->key = infoEntry->name;
            newEntry->kind = infoEntry->kind;
            switch (infoEntry->kind) {
                case 2:
                    for (virtualBase = cls->vbases; virtualBase != NULL; virtualBase = virtualBase->next) {
                        baseEntry = virtualBase->base->sominfo->methodNameList;
                        baseIndex = 0;
                        while (baseEntry != NULL) {
                            if (infoEntry->name == baseEntry->name && baseEntry->kind == 1) {
                                find_or_add_base(layout, cls, virtualBase->base, &newEntry->u.vt.offset);
                                newEntry->u.vt.index = (UInt16)baseIndex;
                                break;
                            }
                            baseEntry = baseEntry->next;
                            baseIndex++;
                        }
                        if (baseEntry != NULL)
                            break;
                    }
                    layout->inheritedMemberCount++;
                    break;
                case 1:
                    CScope_InitScopeSearch(&classScope, cls->nspace);
                    for (;;) {
                        candidate = CScope_NextObject(&classScope);
                        if (candidate == NULL)
                            break;
                        if (candidate->type->type != TYPEFUNC)
                            continue;
                        key = candidate->name;
                        if (key == constructor_name)
                            key = data_00581c48;
                        else if (key == destructor_name)
                            key = space_name;
                        if (key != infoEntry->name)
                            continue;
                        CError_ASSERT(733, TYPE_METHOD(candidate->type)->vtbl_index == entryIndex);
                        newEntry->u.object = candidate;
                        break;
                    }
                    CError_ASSERT(737, candidate != NULL);
                    break;
                default:
                    break;
            }
            layout->memberCount++;
        }
    } else {
        methods = build_vtbl_index_table(cls, &methodCount);
        for (methodIndex = 0; methodIndex < methodCount; methodIndex++) {
            if ((object = methods[methodIndex]) != NULL) {
                newEntry = (SOMEntry *)CompilerTools_AllocatePool(offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
                memclrw(newEntry, offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
                *entryLink = newEntry;
                entryLink = &newEntry->next;
                newEntry->u.object = object;
                newEntry->key = object->name;
                newEntry->kind = 1;
                layout->memberCount++;
            }
        }
    }
}

struct SOMVTable *find_or_add_base(SOMClassBuildState *list, TypeClass *unused, TypeClass *id, UInt16 *index)
{
    struct SOMVTable *p;
    struct SOMVTable *node;
    SInt16 count;

    p = list->bases;
    count = 0;
    while (p != NULL) {
        if (p->base == id) {
            if (index != NULL)
                *index = count;
            return p;
        }
        p = p->next;
        count++;
    }

    if (list->bases != NULL) {
        count = 1;
        p = list->bases;
        while (p->next != NULL) {
            p = p->next;
            count++;
        }
        p->next = (struct SOMVTable *)CompilerTools_AllocatePool(sizeof(struct SOMVTable));
        memclrw(p->next, sizeof(struct SOMVTable));
        node = p->next;
    } else {
        count = 0;
        node = (struct SOMVTable *)CompilerTools_AllocatePool(sizeof(struct SOMVTable));
        memclrw(node, sizeof(struct SOMVTable));
        list->bases = node;
    }
    node->base = id;
    if (index != NULL)
        *index = count;
    return node;
}

void CSOM_CompleteClass(TypeClass *tclass)
{
    NameSpaceObjectList *next;
    int methodIndex;
    Object *method;
    char eligible;
    SOMInfoEntry *entry;
    NameSpaceObjectList *objects;
    HashNameNode *name;
    Type *requiredType;
    SOMInfoEntry *baseEntry;
    long index;
    Object *otherMethod;
    Object *object;
    HashNameNode *methodName;
    Type *lookupType;
    Object *firstMethod;
    Object *checkedMethod;
    FuncArg *args;
    SOMInfoEntry *entries;
    SOMInfoEntry *methodList;
    Object *numbered;
    VClassList *base;
    HashNameNode *loadedName;
    Object **numberedEntries;
    ScopeSearch iterator;
    SInt32 count;

    if (tclass->sominfo->methodNameList) {
        CScope_InitScopeSearch(&iterator, tclass->nspace);
        for (;;) {
            method = CScope_NextObject(&iterator);
            if (!method)
                break;
            if (method->type->type == TYPEFUNC && (((TypeMemberFunc *)method->type)->flags & 32) == 0 &&
                ((TypeMemberFunc *)method->type)->is_static == 0 &&
                ((method->qual & Q_INLINE) == 0 || method->datatype == DVFUNC))
                eligible = 1;
            else
                eligible = 0;
            if (!eligible)
                continue;
            loadedName = method->name;
            methodName = (HashNameNode *)(int)loadedName;
            if (loadedName == constructor_name)
                methodName = data_00581c48;
            else if (methodName == destructor_name)
                methodName = space_name;
            methodIndex = 0;
            entries = tclass->sominfo->methodNameList;
            entry = entries;
            if (entries)
                do {
                    if (entry->name == methodName) {
                        entry->kind = 1;
                        ((TypeMemberFunc *)method->type)->vtbl_index = methodIndex;
                        break;
                    }
                    entry = entry->next;
                    methodIndex++;
                } while (entry);
            if (!entry)
                CError_ReportError(ERR_INTRODUCED_METHOD_NOT_SPECIFIED_RELEASE_ORDER, method);
        }
        entry = (methodList = tclass->sominfo->methodNameList);
        if (methodList)
            do {
                if (entry->kind == 0) {
                    if ((base = (VClassList *)basehead(tclass), base != NULL))
                        do {
                            for (baseEntry = listhead(base); baseEntry; baseEntry = baseEntry->next) {
                                if (entry->name == baseEntry->name && baseEntry->kind == 1) {
                                    entry->kind = 2;
                                    break;
                                }
                            }
                            base = base->next;
                        } while (base != NULL);
                }
                entry = entry->next;
            } while (entry);
    } else {
        numberedEntries = build_vtbl_index_table(tclass, &count);
        index = methodIndex = 0;
        if (index < count)
            do {
                if ((numbered = numberedEntries[index]) != NULL) {
                    setnumber((int)numbered, methodIndex);
                    methodIndex++;
                }
                index++;
            } while (index < count);
    }
    CScope_InitScopeSearch(&iterator, tclass->nspace);
    for (;;) {
        objects = CScope_NextNameSpaceObjectList(&iterator);
        if (!objects)
            break;
        if (objects->object->otype != OT_OBJECT || (next = objects->next) == NULL || next->object->otype != OT_OBJECT ||
            objects == NULL)
            continue;
        do {
            if ((object = (Object *)objects->object)->otype == OT_OBJECT &&
                ((object->qual & Q_INLINE) == 0 || object->datatype == DVFUNC))
                CError_ReportError(ERR_ILLEGAL_SOM_FUNCTION_OVERLOAD, object);
            objects = objects->next;
        } while (objects);
    }
    CScope_InitScopeSearch(&iterator, tclass->nspace);
    for (;;) {
        firstMethod = CScope_NextObject(&iterator);
        if (!firstMethod)
            break;
        if ((firstMethod->qual & Q_INLINE) != 0)
            continue;
        CE_ASSERT(firstMethod->type->type != TYPEFUNC, CError_FATAL(529));
        ((TypeFunc *)firstMethod->type)->flags |= 4;
        tclass->action = 1;
        for (;;) {
            otherMethod = CScope_NextObject(&iterator);
            if (!otherMethod)
                break;
            if (otherMethod->type->type != TYPEFUNC)
                continue;
            ((TypeFunc *)otherMethod->type)->flags &= ~4;
        }
        break;
    }
    if (tclass->sominfo->omitEnvironmentParameter == 0) {
        name = spaces_name;
        lookupType = CScope_FindTagType(currentNameSpace, name);
        if (!lookupType) {
            fn_0043f3e0(281, name->name);
            requiredType = &stvoid;
        } else {
            requiredType = lookupType;
        }
        CScope_InitScopeSearch(&iterator, tclass->nspace);
        for (;;) {
            checkedMethod = CScope_NextObject(&iterator);
            if (!checkedMethod)
                break;
            if (checkedMethod->type->type != TYPEFUNC || ((TypeMemberFunc *)checkedMethod->type)->is_static != 0 ||
                (((TypeMemberFunc *)checkedMethod->type)->flags & 32) != 0)
                continue;
            if ((args = ((TypeMemberFunc *)checkedMethod->type)->args) != NULL && args->next != NULL) {
                if (args->next->type->type == TYPEPOINTER && ((TypePointer *)args->next->type)->target == requiredType)
                    continue;
            }
            CError_ReportError(ERR_NEW_SOM_CALLSTYLE_METHOD_MUST_EXPLICIT, checkedMethod);
        }
    }
    if (tclass->action == 0)
        CError_ReportError(ERR_SOM_CLASS_MUST_ONE_NON_INLINE);
}

Object **build_vtbl_index_table(TypeClass *theclass, SInt32 *count)
{
    int maxIndex = 0;
    Object *object;
    Object *methodObject;
    Boolean eligible;
    Boolean eligibleAgain;
    Object **table;
    int tableSize;
    ScopeSearch scope;
    TypeMemberFunc *method;

    CScope_InitScopeSearch(&scope, theclass->nspace);
    for (;;) {
        object = CScope_NextObject(&scope);
        if (!object)
            break;
        if (object->type->type != TYPEFUNC || (((TypeMemberFunc *)object->type)->flags & FUNC_METHOD) == 0)
            continue;
        if (object->type->type == TYPEFUNC && (((TypeMemberFunc *)object->type)->flags & 32) == 0 &&
            !((TypeMemberFunc *)object->type)->is_static &&
            (!(object->qual & Q_INLINE) || object->datatype == DVFUNC)) {
            eligible = 1;
        } else {
            eligible = 0;
        }
        if (eligible) {
            if (((TypeMemberFunc *)object->type)->vtbl_index <= maxIndex)
                continue;
            maxIndex = ((TypeMemberFunc *)object->type)->vtbl_index;
            continue;
        }
        ((TypeMemberFunc *)object->type)->vtbl_index = 0;
    }
    tableSize = maxIndex + 1;
    *count = tableSize;
    table = (Object **)CompilerTools_AllocatePool(tableSize * sizeof(*table));
    memclrw(table, tableSize * sizeof(*table));
    CScope_InitScopeSearch(&scope, theclass->nspace);
    for (;;) {
        methodObject = CScope_NextObject(&scope);
        if (!methodObject)
            break;
        if (methodObject->type->type == TYPEFUNC && (((TypeMemberFunc *)methodObject->type)->flags & 32) == 0 &&
            !((TypeMemberFunc *)methodObject->type)->is_static &&
            (!(methodObject->qual & Q_INLINE) || methodObject->datatype == DVFUNC)) {
            eligibleAgain = 1;
        } else {
            eligibleAgain = 0;
        }
        if (!eligibleAgain)
            continue;
        method = (TypeMemberFunc *)methodObject->type;
        table[method->vtbl_index] = methodObject;
    }
    return table;
}

void CSOM_InitSOMInfo(TypeClass *type)
{
    ClassList *base;
    SOMInfo *info;
    Object *object;
    char *name;

    for (base = type->bases; base != NULL; base = base->next) {
        if (base->base->sominfo == NULL) {
            CError_ReportError(ERR_SOM_CLASSES_ONLY_INHERIT_FROM_OTHER);
            break;
        }
    }
    if (type->sominfo == NULL) {
        info = (SOMInfo *)galloc(22);
        memclrw(info, 22);
        type->sominfo = info;
        name = type->classname->name;
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat(name, "ClassData");
        object->type = CDecl_NewStructType(0x1c, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        info->classDataObject = object;
        info->classDataObject->flags |= OBJECT_EXPORT;
    }
}

void CSOM_EncodeMemberFunctionTypes(TypeMemberFunc *function)
{
    encode_member_function_types(function, 0);
}

void encode_member_function_types(TypeMemberFunc *t, Boolean flag)
{
    FuncArg *arg;
    UInt8 buf[3];
    UInt8 acc;
    Boolean odd;

    buf[2] = encode_som_type(buf, t->functype, 1);
    buf[1] = 0;
    buf[0] = 0;

    for (arg = t->args; arg != NULL; arg = arg->next) {
        if (arg == &data_00583098 || arg == &data_00584748 || ++buf[0] == 0) {
            CError_ReportError((SInt32)0x111);
            break;
        }
        encode_som_type(buf, arg->type, 0);
    }

    if (flag) {
        if ((arg = t->args) != NULL) {
            if (t->is_static == 0)
                arg = arg->next;
            if (arg != NULL && CMachine_FunctionRequiresMemoryReturn((TypeFunc *)t))
                arg = arg->next;
        }

        AppendGListByte(&data_00583548, buf[0]);
        AppendGListByte(&data_00583548, (buf[1] << 4) | buf[2]);
        if (buf[1] != 0) {
            odd = 0;
            acc = 0;
            while (arg != NULL) {
                acc = (acc << 4) | encode_som_type(buf, arg->type, 0);
                if (odd) {
                    AppendGListByte(&data_00583548, acc);
                    odd = 0;
                    acc = 0;
                } else {
                    odd = 1;
                }
                arg = arg->next;
            }
            if (odd)
                AppendGListByte(&data_00583548, acc << 4);
        }
    }
}

UInt8 encode_som_type(UInt8 *p, Type *ty, Boolean flag)
{
    int result;
    if (ty->size > 4)
        p[1] |= 8;

    switch (*(SInt8 *)ty) {
        case TYPEVOID:
            if (flag)
                return (result = 7);
            break;
        case TYPEINT:
        case TYPEENUM:
            if (Type_IsUnsigned(ty)) {
                switch (ty->size) {
                    case 1:
                        p[1] |= 1;
                        return (result = 0);
                    case 2:
                        p[1] |= 1;
                        return (result = 2);
                    case 4:
                        return (result = 4);
                    case 8:
                        return (result = 6);
                }
            } else {
                switch (ty->size) {
                    case 1:
                        p[1] |= 1;
                        return (result = 1);
                    case 2:
                        p[1] |= 1;
                        return (result = 3);
                    case 4:
                        return (result = 5);
                    case 8:
                        return (result = 6);
                }
            }
            break;
        case TYPEFLOAT:
            p[1] |= 4;
            switch (ty->size) {
                case 4:
                    p[1] |= 2;
                    return (result = 8);
                case 8:
                    return (result = 9);
                case 12:
                case 16:
                    return (result = 10);
            }
            break;
        case TYPEPOINTER:
            return (result = 12);
        case TYPESTRUCT:
        case TYPECLASS:
            if (flag) {
                if (ty->size <= 2) {
                    p[1] |= 1;
                    return (result = 11);
                }
                if (ty->size <= 4)
                    return (result = 14);
                return (result = 15);
            }
            break;
    }
    CError_ReportError(ERR_ILLEGAL_SOM_FUNCTION_PARAMETERS_RETURN_TYPE);
    return (result = 5);
}

void CSOM_GenerateRefNodeCode(void)
{
    struct CSOMRefNode *entry;

    if (cprep_cu[0xe0] != 1) {
        for (entry = somReferences; entry != NULL; entry = entry->next) {
            switch (entry->kind) {
                case 0:
                    CodeGen_EmitLoadAndBranchFunction(entry->object, DAT_00588260,
                                                      entry->theclass->sominfo->classDataObject, entry->id);
                    break;
                case 1:
                    CodeGen_EmitLoadAndBranchFunction(entry->object, som_ref_node_rtfunc,
                                                      entry->theclass->sominfo->classDataObject, entry->id);
                    break;
                case 2:
                    CodeGen_EmitLoadAndBranchFunction(entry->object, som_ref_node_runtime_object,
                                                      entry->theclass->sominfo->classDataObject, entry->id);
                    break;
                default:
                    CError_FATAL(132);
                    break;
            }
        }
    }
}

void CSOM_Init(char flag)
{
    if (flag == '\0') {
        somReferences = NULL;
    }
    data_00581c48 = GetHashNameNode("somInit");
    space_name = GetHashNameNode("somUninit");
    spaces_name = GetHashNameNode("Environment");
    csom_blank_name = GetHashNameNode("__somself");
}

void fn_004e67a0(void)
{
    return;
}

void CSOM_NoOp(void)
{
    return;
}
