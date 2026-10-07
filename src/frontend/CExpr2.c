#define CERROR_FILE "CExpr2.c"
#include "compiler/common.h"
#include "compiler/CExpr2.h"
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
#include "compiler/CFunc.h"
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
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroTransform.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PPCError.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>

#include "compiler/ENode.h"
#include "compiler/Types.h"
#include "compiler/Objects.h"
#include "Compiler/Objects.h"
#include "Compiler/Types.h"

typedef enum { ENX_A = 1 } ENodeTypeX;

typedef enum RegClass { RC_GPR = 0, RC_FPR = 1, RC_SPR = 2, RC_CRFIELD = 3, RC_CRFIELDBIT = 8, RC_VR = 9 } RegClass;

static FuncArg data_00555090 = {0};
static FuncArg right_operand_arg = {0};
static FuncArg operator_operand_arg = {&right_operand_arg};

static SInt32 conversion_score;
static void *array_bound;
static ComparisonValues bestComparison;
static UInt8 expr_search_types[75];
static UInt8 expr_replace_types[75];
static void (*data_00580748)(ENode *);
static SInt32 data_0058074c;
static Type *data_00580750;
static Type *data_00580754;

static inline int register_value(int flag)
{
    InlineAsmExpression s;
    UInt32 isConst;
    int result;
    parse_expression(&s, flag);
    isConst = (s.object == NULL && s.object_label == NULL) && (s.label == NULL);
    if (!isConst) {
        if (s.object)
            PPCError_ReportError(0x7a, ((Object *)s.object)->name->name);
        else if (s.object_label)
            PPCError_ReportError(0x7a, ((Object *)s.object_label)->name->name);
        else if (s.label)
            PPCError_ReportError(0xa6, s.label->name->name);
        result = 0;
    } else {
        switch (s.type) {
            case 8:
                result = (short)((s.value >> 16) + ((s.value >> 15) & 1));
                break;
            case 7:
                result = (short)(s.value >> 16);
                break;
            case 6:
                result = (short)s.value;
                break;
            default:
                result = s.value;
                break;
        }
    }
    return result;
}

static ENode *ce_error(void)
{
    ENode *node;
    node = CompilerTools_AllocatePool(sizeof(*node));
    memclrw(node, sizeof(*node));
    node->type = EINTCONST;
    node->rtype = (Type *)&stsignedlong;
    return node;
}

struct ENode *scandelete(char mode)
{
    ENode *expr;
    Object *constructor;
    Type *conversionType;
    Type *target;
    ENode *call;
    ENode *objectExpr;
    ENode *destructorCall;
    ENode *originalExpr;
    SInt32 referenceExpr;
    short token;
    ConversionSearchState frame;
    BClassList typeContext;
    unsigned char isArray;
    char hasReference;
    Object *objectOrType;
    ENode *result;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '[') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk != ']') {
            CError_ReportError(ERR_RBRACKET_EXPECTED);
        } else {
            tk = CPrepTokenizer_GetNextToken();
        }
        isArray = 1;
    } else {
        isArray = 0;
    }
    expr = cast_expression();
    if (expr->rtype->type != TYPEPOINTER) {
        if (expr->rtype->type == TYPECLASS) {
            TypeClass *conversionClass;
            conversionType = NULL;
            conversionClass = TYPE_CLASS(target = expr->rtype);
            memclrw(&frame, sizeof(frame));
            if ((TYPE_CLASS(target)->flags & CLASS_IS_CONVERTIBLE) != 0) {
                frame.iterator.cls = conversionClass;
                frame.current = &frame.iterator;
                build_convertible_bases_tree(&frame.iterator);
                CScope_InitScopeSearch(&frame.scope, conversionClass->nspace);
            }
            {
                TypeFunc *owner;
                Object *candidate;
                while ((candidate = CExpr_ConversionIteratorNext(&frame)) != NULL) {
                    owner = TYPE_FUNC(candidate->type);
                    if (TYPE_FUNC(candidate->type)->functype->type == TYPEPOINTER) {
                        if (conversionType != NULL) {
                            CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);
                            break;
                        }
                        conversionType = owner->functype;
                    }
                }
            }
            if (conversionType != NULL && user_assign_check(expr, conversionType, 0, 1, 0, 1) != 0) {
                expr = converted_expr;
            }
        }
        if (expr->rtype->type != TYPEPOINTER) {
            CError_ReportError(ERR_ILLEGAL_TYPE);
            expr = ce_error();
            return expr;
        }
    }
    target = TYPE_POINTER(expr->rtype)->target;
    if (TYPE_POINTER(expr->rtype)->target->size == 0) {
        CDecl_CompleteType(target);
    }
    if ((target->type == TYPEARRAY) && !isArray) {
        CError_ReportError(ERR_ILLEGAL_TYPE);
    }
    if (isArray) {
        while (target->type == TYPEARRAY) {
            target = TYPE_POINTER(target)->target;
        }
        build_destructor_aware_call(expr, target, mode);
        return;
    }
    if (copts.f5c != 0 && CObjC_IsIdOrSelType(expr->rtype) != 0) {
        return CObjCModern_MakeDeallocMessage(NULL, expr);
    }
    if (target->type != TYPECLASS) {
        objectOrType = CParser_FindClassMemberOrNamespaceFunctionObject(target, 0, mode);
        CClass_CheckObjectAccess(NULL, objectOrType);
        return make_call_with_optional_size_arg(objectOrType, expr, target);
    }
    if (TYPE_CLASS(target)->sominfo != NULL) {
        return CSOM_CallReleaseObjectReference(TYPE_CLASS(target), expr);
    }
    if (TYPE_CLASS(target)->objcinfo != NULL) {
        return CObjCModern_MakeDeallocMessage(TYPE_CLASS(target), expr);
    }
    if ((TYPE_CLASS(target)->flags & CLASS_COMPLETED) == 0 && copts.f9d != 0) {
        CError_Warning(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, TYPE_CLASS(target), 0);
    }
    objectOrType = CParser_FindClassMemberOrNamespaceFunctionObject(target, 0, mode);
    CClass_CheckObjectAccess(NULL, objectOrType);
    constructor = CClass_Destructor(TYPE_CLASS(target));
    if (constructor == NULL) {
        return make_call_with_optional_size_arg(objectOrType, expr, target);
    }
    typeContext.next = NULL;
    typeContext.type = target;
    CClass_CheckObjectAccess(&typeContext, constructor);
    call = CompilerTools_AllocatePool(sizeof(*call));
    call->type = EFUNCCALL;
    call->cost = 4;
    call->flags = 0;
    call->rtype = &stvoid;
    if (constructor->datatype == DVFUNC) {
        originalExpr = expr;
        expr = CompilerTools_AllocatePool(sizeof(*originalExpr));
        *expr = *originalExpr;
        expr->type = ENULLCHECK;
        referenceExpr = CParser_GetUniqueID();
        expr->data.longval = referenceExpr;
        hasReference = TRUE;
    } else {
        hasReference = FALSE;
    }
    if (mode == 0) {
        result = CABI_DestroyObject(constructor, expr, 2, 0, 1);
    } else {
        destructorCall = CABI_DestroyObject(constructor, expr, 2, 0, 0);
        destructorCall->rtype = (Type *)&void_ptr;
        result = CompilerTools_AllocatePool(sizeof(*result));
        result->type = EFUNCCALL;
        result->cost = 4;
        result->flags = 0;
        result->rtype = &stvoid;
        result->data.funccall.args = CompilerTools_AllocatePool(sizeof(*result->data.funccall.args));
        result->data.funccall.args->next = NULL;
        result->data.funccall.args->node = destructorCall;
        if (DAT_00587fd8 != NULL && (*DAT_00587fd8)(0, objectOrType) == 0) {
            objectExpr = CompilerTools_AllocatePool(sizeof(*objectExpr));
            memclrw(objectExpr, sizeof(*objectExpr));
            objectExpr->type = EINTCONST;
            objectExpr->rtype = (Type *)&void_ptr;
            {
                CInt64 *operands = &objectExpr->data.intval;
                operands->lo = 0;
                operands->hi = 0;
            }
        } else {
            if (objectOrType->sclass == TK_TYPEDEF) {
                CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                objectExpr = CompilerTools_AllocatePool(sizeof(*objectExpr));
                memclrw(objectExpr, sizeof(*objectExpr));
                objectExpr->type = EINTCONST;
                objectExpr->rtype = (Type *)&void_ptr;
                {
                    CInt64 *operands = &objectExpr->data.intval;
                    operands->lo = 0;
                    operands->hi = 0;
                }
            } else {
                objectExpr = CompilerTools_AllocatePool(sizeof(*objectExpr));
                memclrw(objectExpr, sizeof(*objectExpr));
                objectExpr->type = EOBJREF;
                objectExpr->data.objref = objectOrType;
                {
                    Type *pointerType = CDecl_NewPointerType(objectOrType->type);
                    objectExpr->rtype = pointerType;
                }
                if (objectOrType->type->type != TYPEFUNC) {
                    objectExpr->flags = objectOrType->qual & Q_CV;
                }
                objectOrType->flags |= 1;
            }
        }
        result->data.diadic.left = objectExpr;
        result->data.funccall.functype = (TypeFunc *)objectOrType->type;
        objectOrType->flags |= 1;
    }
    if (hasReference) {
        call = CompilerTools_AllocatePool(sizeof(*call));
        call->type = EMFPOINTER;
        call->rtype = &stvoid;
        call->cost = 4;
        call->flags = 0;
        call->data.diadic.left = originalExpr;
        call->data.diadic.right = result;
        call->data.precomp.labelId = referenceExpr;
        return call;
    }
    return result;
}

static ENode *make_dummy_node(void)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(node, 0x1a);
    node->type = EINTCONST;
    node->rtype = (Type *)&stsignedlong;
    return node;
}

void build_destructor_aware_call(ENode *expression, Type *type, Boolean skipLookup)
{
    Object *dtor;
    Object *node;
    Boolean needsCleanup;
    NameSpaceName *operatorName;

    node = CParser_FindClassMemberOrNamespaceFunctionObject(type, 1, skipLookup);
    if (type->type == TYPECLASS) {
        dtor = CClass_Destructor((TypeClass *)type);
        if (dtor != NULL) {
            dtor = CABI_GetDestructorObject(dtor, 1);
            dtor->flags |= OBJECT_USED;
            needsCleanup = 1;
        } else if (CClass_Constructor((TypeClass *)type) != NULL) {
            needsCleanup = 1;
        } else {
            dtor = NULL;
            needsCleanup = 0;
        }
    } else {
        dtor = NULL;
        needsCleanup = 0;
    }
    if (needsCleanup) {
        operatorName = data_00587e64;
        if (node != (Object *)operatorName->first.object) {
            ENode *arg;
            if (dtor != NULL)
                arg = create_objectrefnode(dtor);
            else
                arg = make_dummy_node();
            expression = funccallexpr(destructor_aware_call_func, expression, arg, NULL, NULL);
            funccallexpr(node, expression, NULL, NULL, NULL);
            return;
        } else {
            ENode *arg;
            if (dtor != NULL)
                arg = create_objectrefnode(dtor);
            else
                arg = make_dummy_node();
            funccallexpr(destructor_aware_call_rtfunc, expression, arg, NULL, NULL);
            return;
        }
    }
    make_call_with_optional_size_arg(node, expression, type);
}

static ENode *make_int_node(Type *type, SInt32 value)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    CInt64 *iv;
    memclrw(n, 0x1a);
    n->type = EINTCONST;
    n->rtype = type;
    iv = &n->data.intval;
    iv->lo = value;
    iv->hi = (value < 0) ? -1 : 0;
    return n;
}

ENode *make_call_with_optional_size_arg(Object *func, ENode *arg, Type *argtype)
{
    ENode *call;
    ENode *fnref;
    ENodeList *args;
    ENode *sizearg;

    call = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    call->type = EFUNCCALL;
    call->cost = 4;
    call->flags = 0;
    call->rtype = &stvoid;

    if (DAT_00587fd8 != NULL && DAT_00587fd8(0, func) == 0) {
        fnref = make_int_node((Type *)&void_ptr, 0);
    } else if (func->sclass == TK_TYPEDEF) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        fnref = make_int_node((Type *)&void_ptr, 0);
    } else {
        fnref = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(fnref, sizeof(ENode));
        fnref->type = EOBJREF;
        fnref->data.addr.objref = func;
        fnref->rtype = CDecl_NewPointerType(func->type);
        if (func->type->type != TYPEFUNC) {
            fnref->flags = func->qual & Q_CV;
        }
        func->flags |= 1;
    }

    call->data.funccall.funcref = fnref;
    call->data.funccall.functype = (TypeFunc *)func->type;
    func->flags |= 1;
    args = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    args->node = arg;
    call->data.funccall.args = args;
    if (((TypeFunc *)func->type)->args != NULL && ((TypeFunc *)func->type)->args->next != NULL) {
        CInt64 *sizevalue;
        SInt32 size;
        args->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
        sizearg = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(sizearg, sizeof(ENode));
        sizearg->type = EINTCONST;
        sizearg->rtype = (Type *)&stsignedlong;
        args->next->node = sizearg;
        size = argtype->size;
        sizevalue = &args->next->node->data.intval;
        sizevalue->lo = size;
        sizevalue->hi = (size < 0) ? -1 : 0;
        args->next->node->rtype = CABI_GetSizeTType();
        args->next->next = NULL;
    } else {
        args->next = NULL;
    }

    return call;
}

static ENode *mk_diadic(ENode *left, ENode *right, UInt8 ty)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(0x1a);
    node->type = ty;
    node->rtype = left->rtype;
    node->data.diadic.left = left;
    node->data.diadic.right = right;
    if (left->cost != right->cost) {
        node->cost = right->cost;
        if (left->cost > node->cost)
            node->cost = left->cost;
    } else {
        node->cost = right->cost + 1;
        if (node->cost > 200)
            node->cost = 200;
    }
    node->flags = (UInt16)((left->flags | right->flags) & 3);
    return node;
}

static ENode *mk_intconst0(Type *type)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(node, 0x1a);
    node->type = EINTCONST;
    node->rtype = type;
    return node;
}

static ENode *mk_intconst(Type *type, SInt32 value)
{
    ENode *node = mk_intconst0(type);
    {
        CInt64 *p = &node->data.intval;
        p->lo = value;
        p->hi = value < 0 ? -1 : 0;
    }
    return node;
}

static ENode *mk_zero(Type *type)
{
    return mk_intconst0(type);
}

static ENode *mk_intconst_flat(Type *type, SInt32 value)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(node, 0x1a);
    node->type = EINTCONST;
    node->rtype = type;
    {
        CInt64 *p = &node->data.intval;
        p->lo = value;
        p->hi = value < 0 ? -1 : 0;
    }
    return node;
}

static inline ENode *NewIndirect(ENode *inner)
{
    ENode *e = (ENode *)CompilerTools_AllocatePool(0x1a);
    e->type = EINDIRECT;
    e->cost = inner->cost;
    if (e->cost == 0)
        e->cost = 1;
    e->flags = inner->flags & 3;
    e->rtype = inner->rtype;
    e->data.monadic = inner;
    return e;
}

#include <string.h>

ENode *scannew(char global)
{
    ENodeList *placement;
    Type *type;
    ENodeList *args;
    ENodeList *initlist;
    ENode *result;
    Object *deleteFunction;
    Object *pointerTemp;
    ENode *objectExpr;
    ENode *constructedExpr;
    ENode *tempExpr;
    ENode *assignment;
    ENode *condition;
    unsigned int tempid;
    tk = CPrepTokenizer_GetNextToken();
    type = NULL;
    placement = NULL;
    array_bound = NULL;
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (!isdeclaration(1, 1, 1, ')')) {
            placement = CExpr_ScanExpressionList(1);
        } else {
            DeclInfo di;
            memclrw(&di, sizeof(di));
            CParser_GetDeclSpecs(&di, 0);
            di.isNewExpression = 1;
            CDecl_ParseDeclarator(&di);
            if (di.name)
                CError_ReportError(ERR_ILLEGAL_TYPE);
            array_bound = di.arrayBound;
            type = di.thetype;
        }
        if (tk != ')')
            CError_ReportError(ERR_RPAREN_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
    }
    if (!type) {
        if (tk == '(') {
            DeclInfo di;
            tk = CPrepTokenizer_GetNextToken();
            memclrw(&di, sizeof(di));
            CParser_GetDeclSpecs(&di, 0);
            di.isNewExpression = 1;
            CDecl_ParseDeclarator(&di);
            if (di.name)
                CError_ReportError(ERR_ILLEGAL_TYPE);
            array_bound = di.arrayBound;
            type = di.thetype;
            if (tk != ')')
                CError_ReportError(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
        } else {
            DeclInfo di;
            memclrw(&di, sizeof(di));
            di.isNewTypeId = 1;
            CParser_GetDeclSpecs(&di, 0);
            parse_pointer_and_array_declarator(&di.thetype, 1);
            type = di.thetype;
        }
    }
    if (type->type == TYPEARRAY)
        return build_array_allocation_expression(type, placement, global);
    if (type->type == TYPECLASS) {
        if (TYPE_CLASS(type)->sominfo)
            return CSOM_BuildNewObjectInstance(TYPE_CLASS(type));
        if (TYPE_CLASS(type)->objcinfo)
            return CObjCModern_CreateAllocMessage(TYPE_CLASS(type));
    }
    if (!CanAllocObject(type) || !CanCreateObject(type))
        return mk_intconst0((Type *)&stsignedlong);
    args = (ENodeList *)CompilerTools_AllocatePool(sizeof(*args));
    args->next = placement;
    args->node = mk_intconst((Type *)&stunsignedlong, type->size);
    result = make_class_member_or_global_call(type, args, global, 0);
    result->rtype = CDecl_NewPointerType(type);
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        initlist = CExpr_ScanExpressionList(1);
        if (!initlist && type->type != TYPECLASS) {
            initlist = (ENodeList *)CompilerTools_AllocatePool(sizeof(*initlist));
            memclrw(initlist, sizeof(*initlist));
            initlist->node = do_typecast(mk_intconst0((Type *)&stsignedlong), type, 0);
        }
        if (tk == ')')
            tk = CPrepTokenizer_GetNextToken();
        else
            CError_ReportError(ERR_RPAREN_EXPECTED);
    } else
        initlist = NULL;
    if (type->type == TYPECLASS && CClass_Constructor(TYPE_CLASS(type))) {
        if (!placement && !global && CABI_ConstructorCallsNew(TYPE_CLASS(type))) {
            objectExpr = mk_zero((Type *)&stsignedlong);
            objectExpr->rtype = CDecl_NewPointerType(type);
            return CExpr_ConstructObject(type, objectExpr, initlist, 0, 1, 1, 1, 1);
        }
        if (data_00588238 && !placement && copts.f6b &&
            (deleteFunction = CParser_FindClassMemberOrNamespaceFunctionObject(type, 0, global)) != NULL) {
            constructedExpr = (ENode *)CompilerTools_AllocatePool(sizeof(*constructedExpr));
            *constructedExpr = *result;
            pointerTemp = create_temp_object((Type *)&void_ptr);
            assignment = mk_diadic(create_objectnode(pointerTemp), result, EASS);
            constructedExpr->type = ELABEL;
            constructedExpr->data.newexception.initexpr = assignment;
            constructedExpr->data.newexception.tryexpr =
                CExpr_ConstructObject(type, create_objectnode(pointerTemp), initlist, 0, 1, 1, 1, 1);
            constructedExpr->data.newexception.pointertemp = pointerTemp;
            constructedExpr->data.newexception.deletefunc = deleteFunction;
        } else {
            objectExpr = (ENode *)CompilerTools_AllocatePool(sizeof(*objectExpr));
            *objectExpr = *result;
            objectExpr->type = ENULLCHECK;
            tempid = CParser_GetUniqueID();
            objectExpr->data.longval = tempid;
            constructedExpr = (ENode *)CompilerTools_AllocatePool(sizeof(*constructedExpr));
            *constructedExpr = *result;
            constructedExpr->type = EMFPOINTER;
            constructedExpr->cost = 4;
            constructedExpr->data.precomp.label = result;
            constructedExpr->data.precomp.expression = CExpr_ConstructObject(type, objectExpr, initlist, 0, 1, 1, 1, 1);
            constructedExpr->data.precomp.labelId = objectExpr->data.longval;
        }
        return constructedExpr;
    }
    if (initlist) {
        if (initlist->next) {
            CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
            return mk_intconst0((Type *)&stsignedlong);
        }
        tempExpr = CExpr2_RewriteExprToTemp(result);
        objectExpr = NewIndirect(tempExpr);
        objectExpr->rtype = type;
        objectExpr->flags = initlist->node->flags & 3;
        assignment = mk_diadic(objectExpr, oldassignmentpromotion(initlist->node, type, objectExpr->flags, 1), EASS);
        condition = (ENode *)CompilerTools_AllocatePool(sizeof(*condition));
        memclrw(condition, sizeof(*condition));
        condition->type = ECOND;
        condition->cost = 4;
        condition->rtype = &stvoid;
        condition->data.cond.cond = result;
        condition->data.cond.expr1 = assignment;
        condition->data.cond.expr2 = mk_intconst0((Type *)&stsignedlong);
        constructedExpr = makecommaexpression(condition, tempExpr);
        constructedExpr->rtype = tempExpr->rtype;
        constructedExpr->flags = tempExpr->flags;
        return constructedExpr;
    }
    return result;
}

ENode *build_array_allocation_expression(Type *type, ENodeList *placement, char global)
{
    Type *element;
    Object *constructor;
    Object *temporary;
    TypePointer *pointerType;
    Object *layout;
    ENodeList *pair;
    ENode *count;
    Object *destructor;
    Object *countObject;
    ENode *countAssignment;
    ENode *allocationSize;
    SInt32 elementCount;
    Boolean needsDestructor;
    ENode *result;

    pointerType = galloc(0xe);
    *pointerType = *(TypePointer *)type;
    pointerType->type = TYPEPOINTER;
    pointerType->size = 4;
    pointerType->qual = 0;

    element = TPTR_TARGET(type);
    while (element->type == TYPEARRAY)
        element = TPTR_TARGET(element);

    if (!CanCreateObject(element) || !CanAllocObject(element))
        return mk_intconst0((Type *)&stsignedlong);

    layout = NULL;
    needsDestructor = 0;
    if (element->type == TYPECLASS) {
        if (CClass_Constructor((TypeClass *)element)) {
            layout = CClass_DefaultConstructor((TypeClass *)element);
            if (layout != NULL) {
                layout->flags |= OBJECT_USED;
                needsDestructor = 1;
            } else {
                CError_ReportError(ERR_CLASS_NO_DEFAULT_CONSTRUCTOR);
            }
        } else if (CClass_Destructor((TypeClass *)element) != NULL) {
            needsDestructor = 1;
        }
    }

    if (needsDestructor) {
        ENode *allocation;
        countAssignment = NULL;
        if (element->type != TYPECLASS)
            CError_FATAL(4706);
        destructor = CClass_Destructor((TypeClass *)element);
        if (destructor != NULL)
            destructor = CABI_GetDestructorObject(destructor, 1);
        pair = CompilerTools_AllocatePool(8);
        pair->next = placement;
        if (array_bound != NULL) {
            Type *countType = (Type *)&stunsignedlong;
            countObject = create_temp_object((Type *)&stunsignedlong);
            count = CExpr2_00473720(array_bound, countType);
            if ((elementCount = element->size ? type->size / element->size : 0) > 1) {
                ENode *factor = mk_intconst((Type *)&stunsignedlong, elementCount);
                count = mk_diadic(count, factor, EMUL);
                optimizecomm(count);
            }
            countAssignment = mk_diadic(create_objectnode(countObject), count, EASS);
            {
                ENode *product = mk_diadic(create_objectnode(countObject),
                                           mk_intconst((Type *)&stunsignedlong, element->size), EMUL);
                optimizecomm(product);
                allocationSize = mk_diadic(product, mk_intconst((Type *)&stunsignedlong, 8), EADD);
                optimizecomm(allocationSize);
            }
            pair->node = allocationSize;
            array_bound = create_objectnode(countObject);
        } else {
            pair->node = mk_intconst((Type *)&stunsignedlong, type->size + 8);
            elementCount = element->size ? type->size / element->size : 0;
            array_bound = mk_intconst((Type *)&stunsignedlong, elementCount);
        }
        allocation = make_class_member_or_global_call(element, pair, global, 1);
        allocation->rtype = (Type *)pointerType;
        if (data_00588238 != NULL && placement == NULL && copts.f6b != 0 &&
            (constructor = CParser_FindClassMemberOrNamespaceFunctionObject(element, 1, global)) != NULL) {
            ENode *sizeNode;
            ENode *destructorNode;
            ENode *layoutNode;
            /* ESETCONST's allocation and initialization payload. */
            result = CompilerTools_AllocatePool(0x1a);
            *result = *(ENode *)allocation;
            temporary = create_temp_object((Type *)&void_ptr);
            sizeNode = mk_diadic(create_objectnode(temporary), allocation, EASS);
            result->type = ESETCONST;
            result->data.argobj.allocationAssignment = sizeNode;
            sizeNode = mk_intconst((Type *)&stunsignedlong, element->size);
            destructorNode = destructor ? create_objectrefnode(destructor) : mk_zero((Type *)&stsignedlong);
            layoutNode = layout ? create_objectrefnode(layout) : mk_zero((Type *)&stsignedlong);
            result->data.argobj.initialization =
                CExpr_FuncCallSix(array_allocation_runtime_function, create_objectnode(temporary), layoutNode,
                                  destructorNode, sizeNode, array_bound, NULL);
            result->data.argobj.initialization =
                mk_diadic(create_objectnode(temporary), result->data.argobj.initialization, EASS);
            result->data.argobj.temporary = temporary;
            result->data.argobj.constructor = constructor;
        } else {
            ENode *sizeNode = mk_intconst_flat((Type *)&stunsignedlong, element->size);
            ENode *destructorNode = destructor ? create_objectrefnode(destructor) : mk_intconst0((Type *)&stsignedlong);
            ENode *layoutNode = layout ? create_objectrefnode(layout) : mk_intconst0((Type *)&stsignedlong);
            result = CExpr_FuncCallSix(array_allocation_runtime_function, allocation, layoutNode, destructorNode,
                                       sizeNode, array_bound, NULL);
        }
        if (countAssignment != NULL)
            result = makecommaexpression(countAssignment, result);
        result->rtype = (Type *)pointerType;
        return (ENode *)result;
    } else {
        pair = CompilerTools_AllocatePool(8);
        pair->next = placement;
        pair->node = mk_intconst((Type *)&stunsignedlong, type->size);
        if (array_bound != NULL) {
            Type *countType = (Type *)&stunsignedlong;
            ENode *countNode = CExpr2_00473720(array_bound, countType);
            pair->node = mk_diadic(pair->node, countNode, EMUL);
            optimizecomm(pair->node);
        }
        result = make_class_member_or_global_call(element, pair, global, 1);
        result->rtype = (Type *)pointerType;
        return (ENode *)result;
    }
}

static inline Boolean array_allocation_operators_enabled(void)
{
    return copts.f86;
}

static inline NameSpaceName *array_allocation_operator_namespace(void)
{
    return data_00588008;
}

ENode *make_class_member_or_global_call(Type *classType, ENodeList *arguments, char forceGlobal, Boolean isArray)
{
    CScopeParseResult lookup;
    Boolean found = 0;
    NameSpaceObjectList *overloads;
    Object *function;

    if (!forceGlobal && classType->type == TYPECLASS) {
        NameSpaceName *operatorName;
        overloads = NULL;
        function = NULL;
        operatorName = (isArray && array_allocation_operators_enabled()) ? array_allocation_operator_namespace()
                                                                         : runtime_operator_namespace_name;
        if (CScope_FindClassMemberObject(TYPE_CLASS(classType), &lookup, operatorName->name)) {
            overloads = lookup.objects;
            function = (Object *)lookup.object;
            CError_ASSERT(4635, overloads != NULL || function != NULL);
            found = 1;
        } else if (TYPE_CLASS(classType)->flags & CLASS_HANDLEOBJECT) {
            if (isArray)
                CError_FATAL(4642);
            function = newh_func;
            found = 1;
        }
    }
    if (!found) {
        NameSpaceName *operatorName;
        if (isArray && array_allocation_operators_enabled())
            operatorName = array_allocation_operator_namespace();
        else
            operatorName = runtime_operator_namespace_name;
        overloads = &operatorName->first;
        function = NULL;
    }
    return CExpr_GenericFuncCall(NULL, NULL, 0, function, overloads, NULL, arguments, 0, 0, 1);
}

void parse_pointer_and_array_declarator(Type **type, char allowNonconstant)
{
    ENode *node;
    SInt32 count;
    BinaryOperatorResult conversion;
    CScopeParseResult lookup;

    switch (tk) {
        case '*':
            CDecl_WrapTypePointer(type, fn_0046cea0());
            if (tk != '[') {
                parse_pointer_and_array_declarator(type, allowNonconstant);
            }
            break;
        case TK_IDENTIFIER:
        case TK_COLON_COLON:
            if (CScope_ParseQualifiedScope(&lookup, 1) && lookup.nspace != NULL && lookup.nspace->theclass != NULL &&
                tk == '*') {
                CDecl_MakeMemberPointerType(type, lookup.nspace->theclass, fn_0046cea0());
                if (tk != '[') {
                    parse_pointer_and_array_declarator(type, allowNonconstant);
                }
            } else {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
            break;
    }
    if (tk == '[') {
        tk = CPrepTokenizer_GetNextToken();
        node = s_expression();
        switch ((SInt8)node->rtype->type) {
            case TYPEENUM:
                node->rtype = TYPE_ENUM(node->rtype)->enumtype;
                break;
            case TYPECLASS:
                if (try_class_conversion_to_kind(node, 0, &conversion)) {
                    if (conversion.expression != NULL) {
                        CError_FATAL(4571);
                    }
                    if ((node = conversion.left) == NULL) {
                        CError_FATAL(4572);
                    }
                }
                break;
        }
        if (node->rtype->type != TYPEINT) {
            CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
        }
        if (tk != ']') {
            CError_ReportError(ERR_RBRACKET_EXPECTED);
        } else {
            tk = CPrepTokenizer_GetNextToken();
        }
        if (tk == '[') {
            parse_pointer_and_array_declarator(type, 0);
        }
        if (CDecl_CheckObjectType(*type) && CanAllocObject(*type)) {
            *type = CDecl_NewArrayType(*type, 0);
            if (node->type != EINTCONST) {
                count = 1;
                if (allowNonconstant == 0) {
                    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                } else {
                    array_bound = node;
                }
            } else {
                count = node->data.intval.lo;
            }
            (*type)->size = count * TPTR_TARGET(*type)->size;
        } else {
            CError_ReportError(ERR_ILLEGAL_ARRAY_DEFINITION);
        }
    }
}

unsigned int fn_0046cea0(void)
{
    unsigned int flags = 0U;
    int v;
    v = (tk = (short)CPrepTokenizer_GetNextToken(), tk);
    while (v >= 289 && v <= 310) {
        switch (v) {
            case 289:
                if (flags & 1U)
                    CError_ReportError(121U);
                flags |= 1U;
                break;
            case 290:
                if (flags & 2U)
                    CError_ReportError(121U);
                flags |= 2U;
                break;
            default:
                CError_ReportError(121U);
                break;
        }
        v = (tk = (short)CPrepTokenizer_GetNextToken(), tk);
    }
    return flags;
}

static ENode *make_child(ENode *inner, UInt8 ty)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = ty;
    node->cost = inner->cost;
    if (node->cost == 0)
        node->cost = 1;
    node->flags = inner->flags & 3;
    node->rtype = inner->rtype;
    node->data.monadic = inner;
    return node;
}

static inline ENode *make_ass(ENode *left, ENode *right, UInt8 ty)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = ty;
    node->rtype = left->rtype;
    node->data.diadic.left = left;
    node->data.diadic.right = right;
    if (left->cost != right->cost) {
        node->cost = right->cost;
        if (left->cost > node->cost)
            node->cost = left->cost;
    } else {
        node->cost = right->cost + 1;
        if (node->cost > 200)
            node->cost = 200;
    }
    node->flags = (left->flags | right->flags) & 3;
    return node;
}

static ENode *wrap_child(ENode *a)
{
    return make_child(a, EINDIRECT);
}

ENode *CExpr_ConstructObject(Type *type, ENode *ptr, ENodeList *arguments, Boolean keepResult,
                             Boolean initializeVirtualBases, Boolean forceConstructor, SInt32 constructorFlags,
                             Boolean suppressAccessCheck)
{
    ENodeList *virtualBaseArguments;
    NameSpaceObjectList *constructors;
    ENode *virtualBaseFlag;
    BClassList classArgument;
    CInt64 *flagValue;
    SInt32 flagHigh;

    CError_ASSERT(4409, ptr->rtype->type == TYPEPOINTER);
    ptr = make_child(ptr, EINDIRECT);
    ptr->rtype = type;

    if (!forceConstructor && arguments != NULL && arguments->next == NULL && arguments->node->rtype == type &&
        CClass_CopyConstructor(TYPE_CLASS(type)) == NULL) {
        CError_ASSERT(4419, ptr->rtype->type == TYPECLASS);
        ptr = make_ass(ptr, arguments->node, EASS);
        if (!keepResult)
            ptr = getnodeaddress(ptr, 0);
        return ptr;
    }

    constructors = CClass_Constructor(TYPE_CLASS(type));
    if (constructors != NULL) {
        if (TYPE_CLASS(type)->flags & CLASS_HAS_VBASES) {
            virtualBaseArguments = CompilerTools_AllocatePool(sizeof(*virtualBaseArguments));
            virtualBaseArguments->next = arguments;
            arguments = virtualBaseArguments;
            virtualBaseFlag = CompilerTools_AllocatePool(sizeof(ENode));
            memclrw(virtualBaseFlag, sizeof(ENode));
            virtualBaseFlag->type = EINTCONST;
            virtualBaseFlag->rtype = (Type *)&stsignedshort;
            flagValue = &virtualBaseFlag->data.intval;
            flagValue->lo = initializeVirtualBases ? 1 : 0;
            flagHigh = initializeVirtualBases ? 1 : 0;
            flagValue->hi = flagHigh < 0 ? -1 : 0;
            virtualBaseArguments->node = virtualBaseFlag;
        }
        classArgument.next = NULL;
        classArgument.type = type;
        ptr = CExpr_GenericFuncCall(&classArgument, ptr, 0, NULL, constructors, NULL, arguments, !suppressAccessCheck,
                                    0, constructorFlags);
        if (ptr->type == EFUNCCALL || ptr->type == EFUNCCALLP)
            ptr->rtype = CDecl_NewPointerType(type);
        if (keepResult) {
            ptr = wrap_child(ptr);
            ptr->rtype = type;
        }
        return ptr;
    }

    if (arguments != NULL) {
        if (arguments->next == NULL && ptr->type == EINDIRECT)
            return make_ass(ptr, oldassignmentpromotion(arguments->node, type, 0, 1), EASS);
        CError_ReportError(ERR_ILLEGAL_INITIALIZATION);
    }
    return ptr;
}

Boolean CExpr_CheckOperator(short token, ENode *left, ENode *right, BinaryOperatorResult *out)
{
    Object *previous;
    Object *memberNode;
    HashNameNode *operatorName;
    BClassList *bclass;
    CScopeParseResult name;
    ArgMatch res;
    NameSpaceObjectList globalObject;
    NameSpaceObjectList memberObject;
    ENodeList *args;

    if (left->rtype->type != TYPECLASS) {
        if (left->rtype->type != TYPEENUM) {
            if (right == NULL)
                return FALSE;
            if (right->rtype->type != TYPECLASS && right->rtype->type != TYPEENUM)
                return FALSE;
        }
    } else
        CDecl_CompleteType(left->rtype);

    memclrw(&res, sizeof(res));
    operatorName = CMangler_OperatorName(token);

    if (token == '(') {
        EMemberInfo *call;
        ENode *node;

        if (left->rtype->type == TYPECLASS &&
            CScope_FindClassMemberObject(TYPE_CLASS(left->rtype), &name, operatorName)) {
            if (name.objects != NULL || (name.object != NULL && name.object->otype == OT_OBJECT &&
                                         ((Object *)name.object)->type->type == TYPEFUNC)) {
                call = (EMemberInfo *)CompilerTools_AllocatePool((sizeof(*call) + 3) & ~3);
                memclrw(call, (sizeof(*call) + 3) & ~3);
                call->path = name.basePath;
                call->expr = left;
                call->is_qualified = name.is_qualified;
                if (name.objects == NULL) {
                    call->list = (NameSpaceObjectList *)galloc(sizeof(*call->list));
                    call->list->next = NULL;
                    call->list->object = name.object;
                } else
                    call->list = name.objects;

                node = (ENode *)CompilerTools_AllocatePool(sizeof(*node));
                memclrw(node, sizeof(*node));
                node->type = ENEWEXCEPTIONARRAY;
                node->rtype = &stvoid;
                node->data.emember = call;
                tk = (UInt32)CPrepTokenizer_GetNextToken();
                node = CExpr_MakeFunctionCall(node, CExpr_ScanExpressionList(1));
                out->expression = checkreference(node);
                out->left = NULL;
                out->right = NULL;
                tk = (UInt32)CPrepTokenizer_GetNextToken();
                return TRUE;
            }
            CError_FATAL(4253);
        }
        return FALSE;
    }

    args = (ENodeList *)CompilerTools_AllocatePool(sizeof(*args));
    args->node = left;
    if (right != NULL) {
        args->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(*args->next));
        args->next->node = right;
        args->next->next = NULL;
    } else
        args->next = NULL;

    memberNode = NULL;

    if (left->rtype->type == TYPECLASS) {
        if (CScope_FindClassMemberObject(TYPE_CLASS(left->rtype), &name, operatorName)) {
            if (name.object != NULL) {
                memberObject.next = NULL;
                memberObject.object = name.object;
                name.objects = &memberObject;
            } else if (name.objects == NULL)
                CError_FATAL(4284);

            if (token != '=' || (name.basePath->type == left->rtype && name.basePath->next == NULL)) {
                previous = res.object;
                CExpr_FuncArgMatch(name.objects, NULL, args->next, &res, left, 0);
                if (previous != res.object) {
                    memberNode = res.object;
                    bclass = name.basePath;
                }
            }
        }
    }

    if (CScope_FindObject(currentNameSpace, &name, operatorName)) {
        if (name.object != NULL) {
            globalObject.next = NULL;
            globalObject.object = name.object;
            name.objects = &globalObject;
        }
    } else
        name.objects = NULL;

    if (copts.f89 != 0)
        name.objects = CScope_ArgumentDependentNameLookup(name.objects, operatorName, args, 1);

    if (name.objects != NULL)
        CExpr_FuncArgMatch(name.objects, NULL, args, &res, NULL, 0);

    if (res.object == NULL) {
        if (left->rtype->type != TYPECLASS && (right == NULL || right->rtype->type != TYPECLASS))
            return FALSE;
        return CExpr_CheckOperatorConversion(token, left, right, args, out);
    }

    if ((token != '&' || right != NULL) && token != ',') {
        if (left->rtype->type != TYPECLASS && (right == NULL || right->rtype->type != TYPECLASS) &&
            res.score4Count != 0)
            return FALSE;
        if (right != NULL && res.score4Count == 2) {
            if (CExpr_CheckOperatorConversion(token, left, right, args, out))
                return TRUE;
        }
    }

    if (res.list != NULL)
        CError_OverloadedFunctionError(res.object, res.list);

    if (res.object == memberNode)
        out->expression = CExpr_GenericFuncCall(bclass, args->node, 0, res.object, NULL, NULL, args->next, 0, 0, 1);
    else
        out->expression = CExpr_GenericFuncCall(NULL, NULL, 0, res.object, NULL, NULL, args, 0, 0, 1);

    out->expression = checkreference(out->expression);
    out->left = NULL;
    out->right = NULL;
    return TRUE;
}

/* A search of CLASS's conversion functions, its bases' included. */
static inline void begin_conversion_search(ConversionSearchState *search, Type *type)
{
    TypeClass *theclass = (TypeClass *)type;

    memclrw(search, sizeof(ConversionSearchState));
    if ((theclass->flags & CLASS_IS_CONVERTIBLE) != 0) {
        search->iterator.cls = theclass;
        search->current = &search->iterator;
        build_convertible_bases_tree(&search->iterator);
        CScope_InitScopeSearch(&search->scope, theclass->nspace);
    }
}

static inline Boolean is_zero(ENode *expr)
{
    return expr->data.intval.hi == 0 && expr->data.intval.lo == 0;
}

/* EXPR converted to TYPE when it is of a class type the operator found a conversion for. */
static inline ENode *convert_operand(ENode *expr, Type *type)
{
    if (expr->rtype->type == TYPECLASS && type) {
        if (user_assign_check(expr, type, 0, 1, 0, 1))
            return converted_expr;
        CError_ReportError(ERR_ILLEGAL_OPERAND);
    }
    return expr;
}

/* CONVERSION a candidate: when it ranks best, the operand types the built-in operator takes are its. */
static inline void consider_conversion(Object *conversion, ENodeList *args, ArgMatch *match)
{
    match_function_arguments(conversion, &operator_operand_arg, args, match);
    if (match->object == conversion) {
        data_00580750 = operator_operand_arg.type;
        data_00580754 = right_operand_arg.type;
    }
}

/* The built-in operator TOKEN applied to operands of class type through their conversion functions: OUT gets the operands
   converted, and its expression stays NULL (no user-defined operator is called). */
Boolean CExpr_CheckOperatorConversion(short token, ENode *left, ENode *right, ENodeList *args,
                                      BinaryOperatorResult *out)
{
    short kind;
    ArgMatch match;

    switch (token) {
        case 42:
            if (!right) {
                kind = 3;
                break;
            }
        case 47:
            kind = 1;
            break;
        case 38:
            if (!right)
                return 0;
        case 37:
        case 94:
        case 124:
        case 126:
        case 364:
        case 365:
            kind = 0;
            break;
        case 91:
            kind = 7;
            break;
        case 43:
            kind = 4;
            break;
        case 45:
            kind = 5;
            break;
        case 33:
        case 58:
        case 60:
        case 62:
        case 362:
        case 363:
            kind = 2;
            break;
        case 360:
        case 361:
            kind = 6;
            break;
        case 358:
        case 359:
            return convert_class_binary_operands(left, right, out);
        default:
            return 0;
    }
    if (!right)
        return try_class_conversion_to_kind(left, kind, out);
    data_00580750 = data_00580754 = NULL;
    memclrw(&match, sizeof(ArgMatch));
    if (left->rtype->type == TYPECLASS) {
        if (right->rtype->type == TYPECLASS) {
            ConversionSearchState rightSearch, leftSearch;
            Object *conversion, *other;
            short mode;
            mode = kind;
            if (mode == 6)
                mode = 2;
            begin_conversion_search(&leftSearch, left->rtype);
            while ((conversion = CExpr_ConversionIteratorNext(&leftSearch)) != NULL) {
                begin_conversion_search(&rightSearch, right->rtype);
                while ((other = CExpr_ConversionIteratorNext(&rightSearch)) != NULL) {
                    if (select_operator_operand_types(((TypeMemberFunc *)conversion->type)->functype,
                                                      ((TypeMemberFunc *)other->type)->functype, mode))
                        consider_conversion(conversion, args, &match);
                }
            }
        } else {
            ConversionSearchState search;
            Object *conversion;
            short mode;
            mode = kind;
            if (mode == 6) {
                if (!(right->type == EINTCONST && is_zero(right) && right->rtype->type == TYPEINT))
                    mode = 2;
            }
            begin_conversion_search(&search, left->rtype);
            while ((conversion = CExpr_ConversionIteratorNext(&search)) != NULL) {
                if (select_operator_operand_types(((TypeMemberFunc *)conversion->type)->functype, right->rtype, mode))
                    consider_conversion(conversion, args, &match);
            }
        }
    } else {
        if (right->rtype->type == TYPECLASS) {
            ConversionSearchState search;
            Object *conversion;
            short mode;
            mode = kind;
            if (mode == 6) {
                if (!(left->type == EINTCONST && is_zero(left) && left->rtype->type == TYPEINT))
                    mode = 2;
            }
            begin_conversion_search(&search, right->rtype);
            while ((conversion = CExpr_ConversionIteratorNext(&search)) != NULL) {
                if (select_operator_operand_types(left->rtype, ((TypeMemberFunc *)conversion->type)->functype, mode))
                    consider_conversion(conversion, args, &match);
            }
        } else
            return 0;
    }
    if (!match.object)
        return 0;
    if (match.list)
        CError_OverloadedFunctionError(match.object, match.list);
    out->expression = NULL;
    out->left = convert_operand(left, data_00580750);
    out->right = convert_operand(right, data_00580754);
    return 1;
}

Boolean select_operator_operand_types(Type *leftType, Type *rightType, SInt16 op)
{
    SInt16 leftKind, rightKind;

    if (leftType->type == TYPEENUM)
        leftType = TYPE_ENUM(leftType)->enumtype;
    if (rightType->type == TYPEENUM)
        rightType = TYPE_ENUM(rightType)->enumtype;
    if (leftType->type == TYPEPOINTER && (TYPE_POINTER(leftType)->qual & Q_REFERENCE))
        leftType = TYPE_POINTER(leftType)->target;
    if (rightType->type == TYPEPOINTER && (TYPE_POINTER(rightType)->qual & Q_REFERENCE))
        rightType = TYPE_POINTER(rightType)->target;

    leftKind = (SInt8)leftType->type;
    rightKind = (SInt8)rightType->type;

    switch (op) {
        case 3:
            if (leftKind != TYPEPOINTER || rightKind != TYPEPOINTER)
                return 0;
        pointer_conversion:
            operator_operand_arg.type = leftType;
            right_operand_arg.type = rightType;
            return is_typeequal(leftType, rightType);

        case 7:
            if ((leftKind == TYPEPOINTER && rightKind == TYPEINT) ||
                (leftKind == TYPEINT && rightKind == TYPEPOINTER)) {
                operator_operand_arg.type = leftType;
                right_operand_arg.type = rightType;
                return 1;
            }
            return 0;

        case 6:
            if ((leftKind == TYPEPOINTER && rightKind == TYPEINT) ||
                (leftKind == TYPEINT && rightKind == TYPEPOINTER)) {
                operator_operand_arg.type = leftType;
                right_operand_arg.type = rightType;
                return 1;
            }
            /* fall through */
        case 2:
            if (leftKind == TYPEPOINTER || rightKind == TYPEPOINTER)
                goto pointer_conversion;
            /* fall through */
        case 1:
        arithmetic_conversion:
            if (leftKind == TYPEFLOAT || rightKind == TYPEFLOAT) {
                if (leftKind == TYPEFLOAT) {
                    if (rightKind == TYPEFLOAT) {
                        if (TYPE_INTEGRAL(rightType)->integral > TYPE_INTEGRAL(leftType)->integral)
                            leftType = rightType;
                        operator_operand_arg.type = right_operand_arg.type = leftType;
                        return 1;
                    }
                    if (rightKind != TYPEINT)
                        return 0;
                    operator_operand_arg.type = right_operand_arg.type = leftType;
                    return 1;
                }
                if (leftKind != TYPEINT)
                    return 0;
                operator_operand_arg.type = right_operand_arg.type = rightType;
                return 1;
            }
            /* fall through */
        case 0:
            if (leftKind != TYPEINT || rightKind != TYPEINT)
                return 0;
            if (TYPE_INTEGRAL(rightType)->integral > TYPE_INTEGRAL(leftType)->integral)
                leftType = rightType;
            if (TYPE_INTEGRAL(leftType)->integral < IT_INT)
                leftType = (Type *)&stsignedint;
            operator_operand_arg.type = right_operand_arg.type = leftType;
            return 1;

        case 4:
            if (leftKind == TYPEPOINTER) {
                if (rightKind != TYPEINT)
                    return 0;
                operator_operand_arg.type = leftType;
                right_operand_arg.type = rightType;
                return 1;
            }
            if (rightKind == TYPEPOINTER) {
                if (leftKind != TYPEINT)
                    return 0;
                operator_operand_arg.type = leftType;
                right_operand_arg.type = rightType;
                return 1;
            }
            goto arithmetic_conversion;

        case 5:
            if (leftKind == TYPEPOINTER) {
                if (rightKind == TYPEPOINTER)
                    goto pointer_conversion;
                if (rightKind != TYPEINT)
                    return 0;
                operator_operand_arg.type = leftType;
                right_operand_arg.type = rightType;
                return 1;
            }
            goto arithmetic_conversion;

        default:
            CError_FATAL(3948);
            return 0;
    }
}

static inline char CExpr2_kind(Type *p, long k)
{
    char result;
    int mode = k;
    switch (mode) {
        case 0:
            result = (p->type == TYPEINT);
            break;
        case 1:
            result = (p->type == TYPEINT || p->type == TYPEFLOAT);
            break;
        case 2:
            result = (p->type == TYPEINT || p->type == TYPEFLOAT || p->type == TYPEPOINTER);
            break;
        case 3:
            result = (p->type == TYPEPOINTER);
            break;
        default:
            CError_FATAL(3728);
            result = 0;
    }
    return (char)result;
}

static inline ENode *tail_convert(ENode *a0, Type *v7)
{
    ENode *value;
    if (a0->rtype->type == TYPECLASS && v7 != NULL) {
        if (user_assign_check(a0, v7, 0, 1, 0, 1) != 0)
            value = converted_expr;
        else {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            value = a0;
        }
    } else
        value = a0;
    return value;
}

char try_class_conversion_to_kind(ENode *expr, short kind, BinaryOperatorResult *result)
{
    int matchKind;
    Object *other;
    Type *conversionType;
    Object *conversion;
    Type *type;
    TypeClass *theclass;
    ConversionSearchState search;

    if ((type = expr->rtype)->type != TYPECLASS)
        return 0;

    if (kind + 0 == 4)
        kind = 2;
    else if (kind + 0 == 5)
        kind = 1;

    theclass = (TypeClass *)type;
    memclrw(&search, sizeof(search));
    if ((theclass->flags & CLASS_IS_CONVERTIBLE) != 0) {
        search.iterator.cls = theclass;
        search.current = &search.iterator;
        build_convertible_bases_tree(search.current);
        CScope_InitScopeSearch(&search.scope, theclass->nspace);
    }
    while ((conversion = CExpr_ConversionIteratorNext(&search)) != NULL) {
        matchKind = kind;
        conversionType = ((TypeMemberFunc *)conversion->type)->functype;
        if (CExpr2_kind(conversionType, matchKind) != 0) {
            other = CExpr_ConversionIteratorNext(&search);
            while (other != NULL) {
                if (CExpr2_kind(((TypeMemberFunc *)other->type)->functype, (int)kind) != 0)
                    CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);
                other = CExpr_ConversionIteratorNext(&search);
            }
            result->expression = NULL;
            result->left = (ENode *)tail_convert(expr, conversionType);
            result->right = NULL;
            return 1;
        }
    }
    return 0;
}

unsigned char convert_class_binary_operands(ENode *left, ENode *right, BinaryOperatorResult *result)
{
    ENode *converted;
    Type *rightType;
    TypeClass *leftClass;
    TypeClass *rightClass;
    Object *conversion;
    Boolean eligible;
    Type *classType;
    Type *leftType;
    Object *otherConversion;
    Type *otherType;
    Boolean otherEligible;
    int otherArithmetic;
    int arithmetic;
    Type *secondClass;
    Object *rightConversion;
    Boolean rightEligible;
    Object *otherRightConversion;
    Type *otherRightType;
    Boolean otherRightEligible;
    int rightArithmetic;
    int otherRightArithmetic;
    ConversionSearchState search;
    leftType = rightType = NULL;

    if ((classType = left->rtype)->type == TYPECLASS) {
        leftClass = (TypeClass *)classType;
        memclrw(&search, sizeof(search));
        if ((leftClass->flags & CLASS_IS_CONVERTIBLE) != 0) {
            search.iterator.cls = leftClass;
            search.current = &search.iterator;
            build_convertible_bases_tree(search.current);
            CScope_InitScopeSearch(&search.scope, leftClass->nspace);
        }
        while ((conversion = CExpr_ConversionIteratorNext(&search)) != NULL) {
            leftType = ((TypeMemberFunc *)conversion->type)->functype;
            eligible = (leftType->type == TYPEINT || leftType->type == TYPEFLOAT) || leftType->type == TYPEPOINTER;
            if (!eligible)
                continue;
            otherConversion = CExpr_ConversionIteratorNext(&search);
            if (otherConversion != NULL) {
                do {
                    otherType = ((TypeMemberFunc *)otherConversion->type)->functype;
                    otherEligible =
                        (otherType->type == TYPEINT || otherType->type == TYPEFLOAT) || otherType->type == TYPEPOINTER;
                    if (otherEligible)
                        CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);
                    otherConversion = CExpr_ConversionIteratorNext(&search);
                } while (otherConversion != NULL);
            }
            goto leftDone;
        }
        return 0;
    }
leftDone:
    if ((secondClass = right->rtype)->type == TYPECLASS) {
        rightClass = (TypeClass *)secondClass;
        memclrw(&search, sizeof(search));
        if ((rightClass->flags & CLASS_IS_CONVERTIBLE) != 0) {
            search.iterator.cls = rightClass;
            search.current = &search.iterator;
            build_convertible_bases_tree(search.current);
            CScope_InitScopeSearch(&search.scope, rightClass->nspace);
        }
        while ((rightConversion = CExpr_ConversionIteratorNext(&search)) != NULL) {
            rightType = ((TypeMemberFunc *)rightConversion->type)->functype;
            rightEligible =
                (rightType->type == TYPEINT || rightType->type == TYPEFLOAT) || rightType->type == TYPEPOINTER;
            if (!rightEligible)
                continue;
            otherRightConversion = CExpr_ConversionIteratorNext(&search);
            if (otherRightConversion != NULL) {
                do {
                    otherRightType = ((TypeMemberFunc *)otherRightConversion->type)->functype;
                    otherRightEligible = (otherRightType->type == TYPEINT || otherRightType->type == TYPEFLOAT) ||
                                         otherRightType->type == TYPEPOINTER;
                    if (otherRightEligible)
                        CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);
                    otherRightConversion = CExpr_ConversionIteratorNext(&search);
                } while (otherRightConversion != NULL);
            }
            goto rightDone;
        }
        return 0;
    }
rightDone:
    result->expression = NULL;
    if (left->rtype->type == TYPECLASS && leftType != NULL) {
        if (user_assign_check(left, leftType, 0, 1, 0, 1) != 0) {
            converted = converted_expr;
        } else {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            converted = left;
        }
    } else {
        converted = left;
    }
    result->left = converted;
    if (right->rtype->type == TYPECLASS && rightType != NULL) {
        if (user_assign_check(right, rightType, 0, 1, 0, 1) != 0) {
            converted = converted_expr;
        } else {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            converted = right;
        }
    } else {
        converted = right;
    }
    result->right = converted;
    return 1;
}

Boolean CExpr2_0046e3e0(Type *type, SInt16 op)
{
    switch (op) {
        case 0:
            return type->type == TYPEINT;
        case 1:
            return type->type == TYPEINT || type->type == TYPEFLOAT;
        case 2:
            return type->type == TYPEINT || type->type == TYPEFLOAT || type->type == TYPEPOINTER;
        case 3:
            return type->type == TYPEPOINTER;
        default:
            CError_FATAL(3728);
            return 0;
    }
}

static ENode *CExpr2_MakeIntNode(void)
{
    ENode *n;

    n = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(n, 0x1a);
    n->type = EINTCONST;
    n->rtype = (Type *)&stsignedlong;
    return n;
}

ENode *CExpr_MakeFunctionCall(ENode *expr, ENodeList *args)
{
    ENode *result;
    ENode *call;
    UInt8 flag10;
    UInt8 flag12;
    Boolean variadic;
    ENodeList *arg;
    FuncArg *formal;
    Type *type;
    TypeFunc *functionType;
    BClassList *candidateObject;
    EMemberInfo *candidate;
    Object *object;
    ENode *candidateExpr;
    ENode *value;
    TypeFunc *builtinType;
    ENodeList *defaultArg;

    if (expr->type == ENEWEXCEPTION && expr->data.funccall.functype != NULL) {
        expr->data.objlist.list =
            CScope_ArgumentDependentNameLookup(expr->data.objlist.list, expr->data.objlist.name, args, 0);
        if (expr->data.funccall.funcref == NULL) {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, CError_GetQualifiedHashName(NULL, expr->data.objlist.name));
            result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            memclrw(result, sizeof(ENode));
            result->type = EINTCONST;
            result->rtype = (Type *)&stsignedlong;
            return result;
        }
        if ((object = expr->data.overloadCandidates->object.value)->otype == OT_OBJECT) {
            if (((builtinType = (TypeFunc *)object->type)->flags & FUNC_INTRINSIC) != 0) {
                value = CodeGen_MakeAltivecCall(object, args);
                if (value != NULL)
                    return value;
            }
        }
        return CExpr_GenericFuncCall(NULL, NULL, 0, NULL, expr->data.funccall.funcref, expr->data.objlist.templargs,
                                     args, 0, 0, 1);
    }

    if (expr->type == ENEWEXCEPTIONARRAY) {
        candidate = expr->data.emember;
        candidateObject = candidate->path;
        candidateExpr = candidate->expr;
        flag10 = candidate->is_qualified;
        flag12 = candidate->isambig;
        expr = convert_memberfunc_to_setconst_or_objref(expr);
        if (expr == NULL) {
            CError_ReportError(ERR_CALL_NON_FUNCTION);
            return CExpr2_MakeIntNode();
        }
    } else {
        candidateObject = NULL;
        flag10 = 0;
        flag12 = 0;
        candidateExpr = NULL;
    }

    if (expr->type == EOBJREF) {
        if ((functionType = (TypeFunc *)((Object *)expr->data.funccall.funcref)->type)->type == TYPEFUNC &&
            (functionType->flags & FUNC_INTRINSIC) != 0) {
            value = CodeGen_MakeAltivecCall(expr->data.objref, args);
            if (value == NULL)
                value = CExpr_GenericFuncCall(candidateObject, candidateExpr, flag10, expr->data.objref, NULL, NULL,
                                              args, 0, flag12, 1);
            return value;
        }
    }
    if (expr->type == ENEWEXCEPTION) {
        return CExpr_GenericFuncCall(candidateObject, candidateExpr, flag10, NULL, expr->data.funccall.funcref,
                                     expr->data.objlist.templargs, args, 0, flag12, 1);
    }
    if ((type = expr->rtype)->type != TYPEPOINTER || (functionType = (TypeFunc *)TPTR_TARGET(type))->type != TYPEFUNC) {
        CError_ReportError(ERR_CALL_NON_FUNCTION);
        return CExpr2_MakeIntNode();
    }
    if (expr->type == EOBJREF) {
        return CExpr_GenericFuncCall(candidateObject, candidateExpr, flag10, expr->data.objref, NULL, NULL, args, 0,
                                     flag12, 1);
    }

    formal = functionType->args;
    arg = args;
    variadic = 0;
    if (args != NULL) {
        do {
            if (!variadic) {
                if (formal == NULL) {
                    CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
                    return CExpr2_MakeIntNode();
                }
                if (formal == &data_00583098 || formal == &data_00584748) {
                    arg->node = CExpr_VarArgPromotion(arg->node, formal == &data_00583098);
                    variadic = 1;
                } else {
                    arg->node = CExpr_AssignmentPromotion(arg->node, formal->type, formal->qual, 1);
                    formal = formal->next;
                }
            } else {
                arg->node = CExpr_VarArgPromotion(arg->node, 0);
            }
            arg = arg->next;
        } while (arg != NULL);
    }
    if (!variadic && formal != NULL && formal != &data_00583098 && formal != &data_00584748) {
        do {
            ENodeList *tail;

            if (formal->dexpr == NULL) {
                CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
                return CExpr2_MakeIntNode();
            }
            if (args != NULL) {
                tail = args;
                while (tail->next != NULL)
                    tail = tail->next;
                defaultArg = tail->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
            } else {
                args = defaultArg = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
            }
            defaultArg->next = NULL;
            value = fn_00513040(formal->dexpr, 0);
            if (value->type == EOBJLIST) {
                if (value->data.templdep.subtype != TDE_CAST)
                    CError_FATAL(3096);
                value = CExpr_DoExplicitConversion(value->data.templdep.u.cast.type, value->data.templdep.u.cast.qual,
                                                   value->data.templdep.u.cast.args);
            }
            defaultArg->node = value;
            formal = formal->next;
        } while (formal != NULL && formal != &data_00583098 && formal != &data_00584748);
    }

    call = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(call, sizeof(ENode));
    call->type = EFUNCCALL;
    call->cost = 4;
    call->rtype = functionType->functype;
    call->flags = functionType->qual & ENODE_FLAG_QUALS;
    call->data.funccall.funcref = expr;
    call->data.funccall.args = args;
    call->data.funccall.functype = functionType;
    return CExpr_AdjustFunctionCall(call);
}

ENode *convert_memberfunc_to_setconst_or_objref(ENode *expr)
{
    ENode *result;
    ENode *node;
    Object *object;
    CInt64 *value;
    if (expr->data.emember->list->next || expr->data.emember->templargs) {
        node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(node, sizeof(ENode));
        node->type = ENEWEXCEPTION;
        node->rtype = ((Object *)expr->data.emember->list->object)->type;
        node->data.objlist.list = expr->data.emember->list;
        node->data.objlist.templargs = expr->data.emember->templargs;
        return node;
    }
    if (expr->data.emember->list->object->otype != OT_OBJECT)
        return NULL;
    object = (Object *)(char *)expr->data.emember->list->object;
    if (((Object *)expr->data.emember->list->object)->sclass == 260) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(result, sizeof(ENode));
        result->type = EINTCONST;
        result->rtype = (Type *)&void_ptr;
        value = &result->data.intval;
        value->lo = 0;
        value->hi = 0;
    } else {
        result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(result, sizeof(ENode));
        result->type = EOBJREF;
        result->data.objref = object;
        result->rtype = (Type *)CDecl_NewPointerType(object->type);
        if (object->type->type != TYPEFUNC)
            result->flags = object->qual & ENODE_FLAG_QUALS;
        object->flags |= 1;
    }
    return result;
}

ENode *CExpr2_0046e9d0(Object *obj, Type *functype, ENodeList *args)
{
    TypeFunc *ftype;
    ENodeList *list;
    FuncArg *arg;
    ENode *node;

    list = args;
    ftype = (TypeFunc *)functype;
    arg = ftype->args;
    if (args != NULL) {
        do {
            if (arg == NULL) {
                CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
                node = CompilerTools_AllocatePool(sizeof(*node));
                memclrw(node, sizeof(*node));
                node->type = EINTCONST;
                node->rtype = (Type *)&stsignedlong;
                return node;
            }
            if (arg != &data_00583098 && arg != &data_00584748) {
                list->node = CExpr_AssignmentPromotion(list->node, arg->type, arg->qual, 1);
                arg = arg->next;
            } else {
                list->node = CExpr_VarArgPromotion(list->node, arg == &data_00583098);
            }
            list = list->next;
        } while (list != NULL);
    }
    if (arg != NULL) {
        if (arg != &data_00583098 && arg != &data_00584748) {
            if (arg->dexpr == NULL) {
                CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
                arg = NULL;
            }
        } else {
            arg = NULL;
        }
    }
    if (DAT_00587fd8 != NULL && DAT_00587fd8(0, obj) == 0) {
        node = CompilerTools_AllocatePool(sizeof(*node));
        memclrw(node, sizeof(*node));
        node->type = EINTCONST;
        node->rtype = (Type *)&void_ptr;
        {
            CInt64 *val = &node->data.intval;
            val->lo = 0;
            val->hi = 0;
        }
    } else {
        if (obj->sclass == TK_TYPEDEF) {
            CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
            node = (ENode *)CompilerTools_AllocatePool(sizeof(*node));
            memclrw(node, sizeof(*node));
            node->type = EINTCONST;
            node->rtype = (Type *)&void_ptr;
            {
                CInt64 *val = &node->data.intval;
                val->lo = 0;
                val->hi = 0;
            }
        } else {
            node = (ENode *)CompilerTools_AllocatePool(sizeof(*node));
            memclrw(node, sizeof(*node));
            node->type = EOBJREF;
            node->data.objref = obj;
            node->rtype = CDecl_NewPointerType(obj->type);
            if (obj->type->type != TYPEFUNC)
                node->flags = obj->qual & Q_CV;
            obj->flags |= OBJECT_USED;
        }
    }
    make_funccall_with_dexprs(node, args, (TypeFunc *)functype, arg);
}

/* error node: an int constant of type int */
static ENode *make_slong(void)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(n, 0x1a);
    n->type = (UInt8)EINTCONST;
    n->rtype = (Type *)&stsignedlong;
    return n;
}

/* intconstnode(&void_ptr, 0) */
static ENode *make_voidptr(void)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    CInt64 *iv;
    memclrw(n, 0x1a);
    n->type = (UInt8)EINTCONST;
    n->rtype = (Type *)&void_ptr;
    iv = &n->data.intval;
    iv->lo = 0;
    iv->hi = 0;
    return n;
}

/* makemonadicnode(inner, EINDIRECT) */
static ENode *make_monadic_46eb90(ENode *inner)
{
    ENode *m = (ENode *)CompilerTools_AllocatePool(0x1a);
    m->type = (UInt8)EINDIRECT;
    m->cost = inner->cost;
    if (m->cost == 0)
        m->cost = 1;
    m->flags = (UInt16)(inner->flags & 3);
    m->rtype = inner->rtype;
    m->data.monadic = inner;
    return m;
}

static inline UInt32 CExpr_FunctionFlags(Type *type)
{
    TypeFunc *functionType = (TypeFunc *)type;
    return functionType->flags;
}

ENode *CExpr_GenericFuncCall(BClassList *scope, ENode *instance, Boolean qualified, Object *function, void *candidates,
                             void *matchContext, ENodeList *arguments, SInt32 extraArguments, Boolean ambiguousAccess,
                             Boolean checkAccess)
{
    TypeMemberFunc *type;
    UInt8 access;
    BClassList *path;
    ENode *result;
    ArgMatch resolution;
    ObjectList list;
    FuncArg *formal;
    ENodeList *argument;

    memclrw(&resolution, sizeof(resolution));

    if (function == NULL || (function->type->type == TYPEFUNC && (CExpr_FunctionFlags(function->type) & 0x400) != 0)) {
        if (instance == NULL && data_00588238 != NULL && data_00588040 != NULL && data_005884f8 != 0) {
            instance = CClass_CreateThisSelfExpr();
            if (instance != NULL) {
                ENode *indirect = make_monadic_46eb90(instance);
                indirect->rtype = (Type *)data_00588040;
                instance = indirect;
            }
        }
        if (function != NULL) {
            list.next = NULL;
            list.object.value = function;
            candidates = &list;
        }
        CExpr_FuncArgMatch(candidates, matchContext, arguments, &resolution, instance, extraArguments);
        if (resolution.object == NULL) {
            CError_FunctionCallError(ERR_FUNCTION_CALL_STAR_DOES_NOT_MATCH, candidates, arguments);
            return make_slong();
        }
        if (resolution.list != NULL)
            CError_OverloadedFunctionError(resolution.object, resolution.list);
        function = resolution.object;
    }

    if (DAT_00587fd8 != NULL && DAT_00587fd8(0, function) == 0) {
        result = make_voidptr();
    } else if (function->sclass == TK_TYPEDEF) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        result = make_voidptr();
    } else {
        result = CompilerTools_AllocatePool(sizeof(*result));
        memclrw(result, sizeof(*result));
        result->type = EOBJREF;
        result->data.objref = function;
        result->rtype = (Type *)CDecl_NewPointerType(function->type);
        if (function->type->type != TYPEFUNC)
            result->flags = function->qual & ENODE_FLAG_QUALS;
        function->flags |= OBJECT_USED;
    }

    type = (TypeMemberFunc *)function->type;
    if (type->type != TYPEFUNC) {
        CError_ReportError(ERR_CALL_NON_FUNCTION);
        return make_slong();
    }

    if ((type->flags & FUNC_METHOD) != 0 && type->is_static == 0) {
        access = function->access;
        path = NULL;
        while (function->datatype == DALIAS) {
            if (path != NULL)
                path = CClass_AppendPath(path, CClass_GetPathCopy(function->u.alias.member, 0));
            else
                path = CClass_GetPathCopy(function->u.alias.member, 0);
            function = function->u.alias.object;

            if (DAT_00587fd8 != NULL && DAT_00587fd8(0, function) == 0) {
                result = make_voidptr();
            } else if (function->sclass == TK_TYPEDEF) {
                CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                result = make_voidptr();
            } else {
                result = CompilerTools_AllocatePool(sizeof(*result));
                memclrw(result, sizeof(*result));
                result->type = EOBJREF;
                result->data.objref = function;
                result->rtype = (Type *)CDecl_NewPointerType(function->type);
                if (function->type->type != TYPEFUNC)
                    result->flags = function->qual & ENODE_FLAG_QUALS;
                function->flags |= OBJECT_USED;
            }
        }
        if (ambiguousAccess != 0)
            CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);

        if (type->theclass->sominfo != NULL &&
            (((function->qual & Q_INLINE) == 0) || (function->datatype == DVFUNC && qualified == 0))) {
            BClassList *members = CClass_GetPathCopy(scope, 0);
            instance = CExpr2_004719c0(scope, path, instance, access, checkAccess);
            if (instance == NULL)
                return make_slong();
            result = CSOM_MakeMethodReference(members, function, qualified);
        } else {
            if (function->datatype == DVFUNC &&
                (qualified != 0 ||
                 (copts.f7a == 0 && instance != NULL && has_indirect_class_objref(instance, type->theclass) != 0)))
                result->flags |= ENODE_FLAG_80;

            instance = CExpr2_004719c0(scope, path, instance, access, checkAccess);
            if (instance == NULL)
                return make_slong();
        }

        if ((type->flags & FUNC_PURE) != 0 && data_00588238 != NULL) {
            TypeFunc *enclosingType = (TypeFunc *)data_00588238->type;
            if ((enclosingType->flags & 0x6000) != 0 && data_00588040 == type->theclass &&
                instance->type == EINDIRECT && instance->data.monadic->type == EINDIRECT &&
                instance->data.monadic->data.monadic->type == EOBJREF &&
                instance->data.monadic->data.monadic->data.objref->name == this_arg_name &&
                (result->flags & ENODE_FLAG_80) == 0)
                CError_Warning(ERR_ILLEGAL_USE_PURE_FUNCTION);
        }

        {
            ENodeList *thisArgument = CompilerTools_AllocatePool(sizeof(*thisArgument));
            thisArgument->next = arguments;
            thisArgument->node = instance->data.monadic;
            if (thisArgument->node->type == EOBJREF)
                thisArgument->node->data.objref->flags |= OBJECT_FLAGS_2;

            if (((instance->flags & Q_CONST) != 0 && ((type->args->qual & Q_CONST) == 0)) ||
                ((instance->flags & Q_VOLATILE) != 0 && ((type->args->qual & Q_VOLATILE) == 0))) {
                if ((type->flags & 0x6000) == 0)
                    CError_ReportError(ERR_CANNOT_PASS_CONST_VOLATILE_DATA_OBJECT);
            }

            arguments = thisArgument;
            argument = thisArgument->next;
            formal = type->args->next;
        }
    } else {
        if (checkAccess != 0 && function->access != ACCESSPROTECTED)
            CClass_CheckObjectAccess(scope, function);
        argument = arguments;
        formal = type->args;
        if ((type->flags & FUNC_METHOD) != 0 && type->theclass->sominfo != NULL)
            CError_FATAL(3415);
    }

    while (argument != NULL) {
        if (formal != NULL && formal != &data_00583098 && formal != &data_00584748) {
            argument->node = CExpr_AssignmentPromotion(argument->node, formal->type, formal->qual, 1);
            formal = formal->next;
        } else {
            if (formal == NULL) {
                list.next = NULL;
                list.object.value = function;
                CError_FunctionCallError(ERR_FUNCTION_CALL_STAR_DOES_NOT_MATCH, &list, arguments);
            }
            argument->node = CExpr_VarArgPromotion(argument->node, formal == &data_00583098);
        }
        argument = argument->next;
    }

    if (formal != NULL) {
        if (formal != &data_00583098 && formal != &data_00584748) {
            if (formal->dexpr == NULL) {
                list.next = NULL;
                list.object.value = function;
                CError_FunctionCallError(ERR_FUNCTION_CALL_STAR_DOES_NOT_MATCH, &list, arguments);
                formal = NULL;
            }
        } else {
            formal = NULL;
        }
    }
    make_funccall_with_dexprs(result, arguments, (TypeFunc *)type, formal);
}

static inline ENode *CExpr2_0046f260_inline1(ENode *a0)
{
    ENode *v5;
    ENode *v4;
    v4 = (ENode *)(int)a0;
    if ((char)a0->rtype->type != 1) {
        if ((char)a0->rtype->type != 3) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            v4 = (ENode *)CompilerTools_AllocatePool(26);
            memclrw(v4, 26);
            v4->type = 50;
            v4->rtype = (Type *)&stsignedlong;
        } else {
            v4 = (ENode *)(int)a0;
            a0->rtype = ((TypeEnum *)a0->rtype)->enumtype;
        }
    }
    if (((TypeIntegral *)v4->rtype)->integral < 7) {
        if (v4->type != 50) {
            v5 = (ENode *)CompilerTools_AllocatePool(26);
            v5->type = 48;
            v5->cost = v4->cost;
            if (v5->cost == 0) {
                v5->cost = 1;
            }
            v5->flags = (short)(v4->flags & 3);
            v5->rtype = v4->rtype;
            v5->data.monadic = v4;
            v4 = v5;
        }
        v4->rtype = (Type *)&stsignedint;
        return v4;
    }
    return v4;
}

ENode *CExpr_VarArgPromotion(ENode *expr, Boolean allowWarning)
{
    switch ((char)expr->rtype->type) {
        case 0:
        case 6:
            CError_ReportError(ERR_CANNOT_PASS_VOID_FUNCTION_PARAMETER);
            expr = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            memclrw(expr, sizeof(ENode));
            expr->type = EINTCONST;
            expr->rtype = (Type *)&stsignedlong;
            break;
        case 1:
        case 3:
            expr = CExpr2_0046f260_inline1(expr);
            break;
        case 2:
            if (((TypeIntegral *)expr->rtype)->integral >= 15)
                break;
            {
                Type *type = (Type *)&stdouble;
                expr = CExpr2_00473720(expr, type);
            }
            break;
        case 5:
            expr = classargument(expr);
            break;
    }
    if (CMach_PassResultInHiddenArg(expr->rtype))
        expr = CExpr_AssignmentPromotion(expr, expr->rtype, expr->flags, 1);
    if (!allowWarning && copts.fa6 &&
        ((expr->rtype->type == 1 && ((TypeIntegral *)expr->rtype)->integral >= 11) || expr->rtype->type == 2))
        CError_Warning(ERR_ASSIGNING_NON_INT_NUMERIC_VALUE_UNPROTOTYPED);
    return expr;
}

unsigned char has_indirect_class_objref(ENode *expr, TypeClass *classType)
{
    unsigned int result;
    ENode *node;
    ENode *operand;
    node = expr;
    result = 0U;
    if (node->type == EINDIRECT) {
        operand = node->data.monadic;
        if (has_class_objref(operand) != 0U) {
            result = 1U;
        }
    }
    return result;
}

Boolean has_class_objref(ENode *node)
{
    if (node->type == EOBJREF) {
        Type *type = node->data.objref->type;
        while (type->type == TYPEARRAY)
            type = TPTR_TARGET(type);
        return type->type == TYPECLASS;
    }
    if (node->type == EADD || node->type == ESUB) {
        if (has_class_objref(node->data.diadic.left))
            return 1;
        if (has_class_objref(node->data.diadic.right))
            return 1;
    }
    return 0;
}

void make_funccall_with_dexprs(ENode *funcref, ENodeList *args, TypeFunc *ftype, FuncArg *arglist)
{
    ENode *n;
    ENodeList *list = args;
    ENodeList *node;
    ENodeList *p;
    ENode *t;

    while (arglist != NULL) {
        if (arglist->dexpr != NULL) {
            if (list != NULL) {
                p = list;
                while (p->next != NULL)
                    p = p->next;
                p->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                node = p->next;
            } else {
                list = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
                node = list;
            }
            node->next = NULL;
            t = fn_00513040(arglist->dexpr, 0);
            if (t->type == EOBJLIST) {
                CError_ASSERT(3096, t->data.templdep.subtype == TDE_CAST);
                t = CExpr_DoExplicitConversion(t->data.templdep.u.cast.type, t->data.templdep.u.cast.qual,
                                               t->data.templdep.u.cast.args);
            }
            node->node = t;
        }
        arglist = arglist->next;
    }

    n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = EFUNCCALL;
    n->cost = 4;
    n->rtype = ftype->functype;
    n->flags = (UInt16)(ftype->qual & ENODE_FLAG_QUALS);
    n->data.funccall.funcref = funcref;
    n->data.funccall.funcref->rtype = (Type *)CDecl_NewPointerType((Type *)ftype);
    n->data.funccall.args = list;
    n->data.funccall.functype = ftype;
    funcref->data.objref->flags |= OBJECT_USED;
    CExpr_AdjustFunctionCall(n);
}

void CExpr_FuncArgMatch(NameSpaceObjectList *source, void *context, ENodeList *objects, ArgMatch *state, ENode *mode,
                        char exclude)
{
    NameSpaceObjectList *entry;
    ENodeList *objectEntry;
    char hasFlag;
    ObjectList *copyEntry;
    ObjectList *scan;
    Object *object;
    ObjectList *copy;
    MemberCallArguments result;
    entry = source;
    copyEntry = (ObjectList *)CompilerTools_AllocatePool(8);
    copy = copyEntry;
    for (;;) {
        copyEntry->object.value = (Object *)entry->object;
        if ((entry = entry->next) == NULL) {
            copyEntry->next = NULL;
            break;
        }
        copyEntry = copyEntry->next = (ObjectList *)CompilerTools_AllocatePool(8);
    }
    objectEntry = objects;
    while (objectEntry != NULL) {
        CDecl_CompleteType(objectEntry->node->rtype);
        objectEntry = objectEntry->next;
    }
    scan = copy;
    hasFlag = 0;
    while (scan != NULL) {
        if ((object = scan->object.value)->otype == OT_OBJECT && object->type->type == TYPEFUNC &&
            (exclude == 0 || (object->qual & Q_EXPLICIT) == 0)) {
            if ((((TypeFunc *)object->type)->flags & 1024) == 0) {
                if (CExpr_GetFuncMatchArgs(object, objects, mode, &result) != 0) {
                    match_function_arguments(object, result.parameters, result.arguments, state);
                }
            } else {
                hasFlag = 1;
            }
        }
        scan = scan->next;
    }
    if (hasFlag != 0 &&
        (state->object == NULL || state->score2Count != 0 || state->score3Count != 0 || state->score4Count != 0)) {
        fn_00514380(copy, context, objects, state, mode);
    }
}

Boolean CExpr_GetFuncMatchArgs(Object *obj, ENodeList *arguments, ENode *instance, MemberCallArguments *result)
{
    if (!(((TypeMemberFunc *)obj->type)->flags & FUNC_METHOD)) {
        result->arguments = arguments;
        result->parameters = ((TypeMemberFunc *)obj->type)->args;
        return 1;
    }
    if (((TypeMemberFunc *)obj->type)->is_static) {
        result->arguments = arguments;
        result->parameters = ((TypeMemberFunc *)obj->type)->args;
        return 1;
    }
    if (((TypeMemberFunc *)obj->type)->flags & FUNC_IS_DTOR) {
        result->arguments = arguments;
        result->parameters = ((TypeMemberFunc *)obj->type)->args->next;
        return 1;
    }
    if (instance != NULL) {
        ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        node->type = EINTCONST;
        node->cost = 0;
        node->flags = instance->flags;
        node->rtype = CDecl_NewPointerType(instance->rtype);
        node->data.intval = cint64_zero;
        result->arguments = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
        result->arguments->next = arguments;
        result->arguments->node = node;
        if (obj->datatype == DALIAS) {
            result->parameters = (FuncArg *)CompilerTools_AllocatePool(sizeof(FuncArg));
            *result->parameters = *((TypeMemberFunc *)obj->type)->args;
            result->parameters->type = CDecl_NewPointerType(instance->rtype);
        } else {
            result->parameters = ((TypeMemberFunc *)obj->type)->args;
        }
        return 1;
    }
    return 0;
}

void match_function_arguments(Object *signature, FuncArg *argument, ENodeList *objects, ArgMatch *value)
{
    ArgMatch conversions;
    if (signature->type->type == TYPEFUNC && (((TypeFunc *)signature->type)->flags & 1024U) == 0U) {
        memclrw(&conversions, 30U);
        for (;;) {
            if (argument == NULL || argument->type == &stvoid) {
                if (objects == NULL)
                    break;
                return;
            }
            if (argument == &data_00583098 || argument == &data_00584748)
                break;
            if (objects == NULL) {
                if (argument->dexpr == NULL)
                    return;
                break;
            }
            if (CExpr2_UpdateArgMatchScores(argument->type, argument->qual, objects->node, &conversions) == 0U)
                return;
            objects = objects->next;
            argument = argument->next;
        }
        CExpr_MatchCompare(signature, value, &conversions);
    }
}

Boolean CExpr_MatchCompare(Object *obj, ArgMatch *dst, ArgMatch *src)
{
    MatchLink *link;

    switch (compare_short_arrays_lexicographically(&src->score1Count, &dst->score1Count, 0)) {
        case -1:
            return 0;
        case 0:
            if (dst->score4Count > src->score4Count)
                return 0;
            if (dst->score4Count == src->score4Count) {
                switch (compare_short_arrays_lexicographically(&src->score4Value0, &dst->score4Value0, 1)) {
                    case -1:
                        return 0;
                    case 0:
                        if (dst->qualificationPenalty > src->qualificationPenalty)
                            return 0;
                        if (dst->qualificationPenalty == src->qualificationPenalty && dst->object != NULL) {
                            if (dst->object->datatype == obj->datatype) {
                            insert:
                                link = (MatchLink *)CompilerTools_AllocatePool(8);
                                link->next = dst->list;
                                dst->list = link;
                                link->object = obj;
                                return 0;
                            }
                            if (obj->datatype == DALIAS)
                                return 0;
                            if (dst->object->datatype != DALIAS)
                                goto insert;
                        }
                        break;
                }
            }
            break;
    }
    *dst = *src;
    dst->object = obj;
    return 1;
}

SInt16 assign_check(ENode *operand, Type *targetType, SInt32 targetQual, Boolean convert, Boolean isExplicit,
                    Boolean checkAccess)
{
    Type *conversionType;
    SInt16 result;
    Boolean isReference;
    Boolean isArray;

    conversion_score = 1000;
    isReference = FALSE;
    isArray = FALSE;
    data_0058850e = 0;
    conversionType = targetType;
    if (targetType->type == TYPEPOINTER && (TYPE_POINTER(targetType)->qual & Q_REFERENCE) != 0 &&
        TPTR_TARGET(targetType)->type != TYPEFUNC) {
        conversionType = TPTR_TARGET(targetType);
        isReference = TRUE;
    }
    converted_expr = operand;
    if (conversionType->type == TYPEARRAY) {
        isArray = TRUE;
        conversionType = CDecl_NewPointerType(TPTR_TARGET(conversionType));
    }
    if (conversionType->size == 0) {
        CDecl_CompleteType(conversionType);
        if (conversionType->size == 0 && !isReference) {
            if (convert) {
                if (conversionType->type == TYPECLASS)
                    CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, conversionType, 0);
                else
                    CError_ReportError(ERR_CANNOT_CONVERT, operand->rtype, operand->flags & Q_CV, conversionType,
                                       targetQual);
            }
            return 0;
        }
    }
    if (copts.fa7 && convert && !isExplicit) {
        if (conversionType->type == TYPEINT || conversionType->type == TYPEFLOAT) {
            if (operand->rtype->type == TYPEINT || operand->rtype->type == TYPEFLOAT)
                CExpr_CheckArithmConversion(operand, conversionType);
        }
    }
    result = check_standard_conversion(operand, conversionType, convert, checkAccess);
    if (result == 0) {
        if (operand->rtype->type == TYPECLASS || conversionType->type == TYPECLASS) {
            result = user_assign_check(operand, conversionType, targetQual, convert, isExplicit, checkAccess);
        } else if (convert) {
            CError_ReportError(ERR_CANNOT_CONVERT, operand->rtype, operand->flags & Q_CV, conversionType, targetQual);
        }
    }
    if (isReference && result != 0) {
        if (convert) {
            if (converted_expr->type != EINDIRECT) {
                if (!isArray) {
                    converted_expr = CExpr_LValue(converted_expr, 0, 0);
                    if (converted_expr->type != EINDIRECT) {
                        converted_expr = get_address_of_temp_copy(converted_expr, 1);
                        data_0058850e = 1;
                    } else {
                        converted_expr = getnodeaddress(converted_expr, 0);
                    }
                }
            } else {
                if (!CExpr_IsLValue(converted_expr))
                    data_0058850e = 1;
                if (!isArray)
                    converted_expr = getnodeaddress(converted_expr, 0);
            }
        } else {
            if (!isArray) {
                if (!CExpr_IsLValue(converted_expr)) {
                    if (!CParser_IsConst(TPTR_TARGET(targetType), targetQual))
                        result = 0;
                }
            }
        }
    }
    return result;
}

static ENode *CExpr2_0046fde0_inline1(ENode *expr)
{
    ENode *node;
    Type *type;
    if (!data_0058757c) {
        type = expr->rtype;
        node = (ENode *)CompilerTools_AllocatePool(26);
        node->type = EPRECOMP;
        node->cost = 0;
        node->flags = 0;
        node->rtype = (Type *)CDecl_NewPointerType(type);
        node->data.temp.type = type;
        node->data.temp.uniqueid = CParser_GetUniqueID();
        *((char *)node + 18) = 0;
        return node;
    }
    return data_0058757c(expr->rtype, 0);
}

ENode *get_address_of_temp_copy(ENode *expr, char materialize)
{
    Object *object;
    ENode *address, *result, *indirect, *assignment;
    Type *type;
    CInt64 *zero, *errorZero;
    unsigned char buffer[64];

    if (materialize != 0) {
        if (expr->type == EINTCONST || expr->type == EFLOATCONST) {
            object = CParser_NewCompilerDefDataObject();
            object->type = expr->rtype;
            object->name = CParser_GetUniqueName();
            object->sclass = TK_STATIC;
            if (expr->type == EINTCONST) {
                switch ((signed char)(type = expr->rtype)->type) {
                    case TYPEENUM:
                        type = ((TypeEnum *)type)->enumtype;
                        break;
                    case TYPEPOINTER:
                        type = (Type *)&stunsignedlong;
                        break;
                    default:
                        CError_FATAL(2695);
                    case TYPEINT:
                        break;
                }
                CMach_InitIntMem(type, expr->data.intval, buffer);
            } else {
                CMach_InitFloatMem(expr->rtype, expr->data.floatval, buffer);
            }
            fn_004ceab0(object, buffer, NULL, object->type->size);
            if (DAT_00587fd8 != NULL && DAT_00587fd8(0, object) == 0) {
                result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(result, sizeof(ENode));
                result->type = EINTCONST;
                result->rtype = (Type *)&void_ptr;
                zero = &result->data.intval;
                zero->lo = 0;
                zero->hi = 0;
            } else if (object->sclass == TK_TYPEDEF) {
                CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(result, sizeof(ENode));
                result->type = EINTCONST;
                result->rtype = (Type *)&void_ptr;
                errorZero = &result->data.intval;
                errorZero->lo = 0;
                errorZero->hi = 0;
            } else {
                result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(result, sizeof(ENode));
                result->type = EOBJREF;
                result->data.objref = object;
                result->rtype = CDecl_NewPointerType(object->type);
                if (object->type->type != TYPEFUNC)
                    result->flags = object->qual & Q_CV;
                object->flags |= OBJECT_USED;
            }
            return result;
        }
        address = CExpr2_0046fde0_inline1(expr);
        indirect = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        indirect->type = EINDIRECT;
        indirect->cost = address->cost;
        if (indirect->cost == 0)
            indirect->cost = 1;
        indirect->flags = address->flags & 3, indirect->rtype = address->rtype;
        indirect->data.monadic = address;
        indirect->rtype = ((TypePointer *)indirect->rtype)->target;
        assignment = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
        assignment->type = EASS;
        assignment->rtype = indirect->rtype;
        assignment->data.diadic.left = indirect;
        assignment->data.diadic.right = expr;
        if (indirect->cost != expr->cost) {
            assignment->cost = expr->cost;
            if (indirect->cost > assignment->cost)
                assignment->cost = indirect->cost;
        } else {
            assignment->cost = expr->cost + 1;
            if (assignment->cost > 200)
                assignment->cost = 200;
        }
        assignment->flags = (indirect->flags | expr->flags) & 3;
        return makecommaexpression(assignment, indirect->data.monadic);
    }
    result = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(result, sizeof(ENode));
    result->type = EINTCONST;
    result->rtype = (Type *)&stsignedlong;
    result->data.intval.lo = -1;
    result->data.intval.hi = -1;
    result->rtype = CDecl_NewPointerType(expr->rtype);
    return result;
}

static CInt64 ConvertConst(Type *type, Type *other, CInt64 val)
{
    if (type == (Type *)&stbool)
        return CMach_CalcIntDiadic(other, val, 0x169, cint64_zero);
    else
        return CMach_CalcIntDiadic(type, val, 0x2b, cint64_zero);
}

void CExpr_CheckArithmConversion(ENode *node, Type *type)
{
    CInt64 convertedValue, restoredValue;

    if (node->rtype == type)
        return;
    if (node->rtype == (Type *)&stbool)
        return;
    if (node->rtype->type == TYPEINT) {
        if (type->type == TYPEFLOAT)
            return;
        CError_ASSERT(2616, type->type == TYPEINT);
        if (type->size > node->rtype->size)
            return;
        if (type->size == node->rtype->size) {
            if (Type_IsUnsigned(type) == Type_IsUnsigned(node->rtype))
                return;
        }
        switch (node->type) {
            case EINTCONST: {
                Boolean isNegative;
                isNegative = (node->data.intval.hi & 0x80000000) != 0;
                if (isNegative && !Type_IsUnsigned(node->rtype) && Type_IsUnsigned(type))
                    break;
                convertedValue = ConvertConst(type, node->rtype, node->data.intval);
                restoredValue = ConvertConst(node->rtype, type, convertedValue);
                if (CInt64_Equal(restoredValue, node->data.intval))
                    return;
                break;
            }
            case ELOGNOT:
            case ELESS:
            case EGREATER:
            case ELESSEQU:
            case EGREATEREQU:
            case EEQU:
            case ENOTEQU:
            case ELAND:
            case ELOR:
                return;
            default:
                break;
        }
    } else if (type->type == TYPEFLOAT && type->size >= node->rtype->size) {
        return;
    }
    CError_Warning(ERR_IMPLICIT_ARITHMETIC_CONVERSION_FROM, node->rtype, 0, type, 0);
}

static ENode *mkintconst(Type *ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(n, sizeof(ENode));
    n->type = EINTCONST;
    n->rtype = ty;
    return n;
}

static inline ENode *enumOperand(ENode *expr, UInt8 type)
{
    if (type != TYPEENUM) {
        CError_ReportError(ERR_ILLEGAL_OPERAND);
        return mkintconst((Type *)&stsignedlong);
    }
    expr->rtype = TYPE_ENUM(expr->rtype)->enumtype;
    return expr;
}

ENode *CExpr2_ConvertScalarOperand(ENode *result, Boolean integerOnly, Boolean preferBool)
{
    UInt8 type;
    result = CExpr_GeneratePointerAndRewriteConst(result);
    switch ((SInt8)(type = result->rtype->type)) {
        case TYPEENUM:
            result = enumOperand(result, type);
            break;
        case TYPECLASS: {
            ConversionSearchState state;
            TypeClass *classType;
            Boolean found;
            Boolean ambiguous;
            Object *conversion;
            TypeFunc *functionType;
            Type *selectedType;

            CDecl_CompleteType(result->rtype);
            found = ambiguous = 0;
            classType = TYPE_CLASS(result->rtype);
            memclrw(&state, sizeof(state));
            if (classType->flags & CLASS_IS_CONVERTIBLE) {
                ConvNode *list = &state.iterator;
                state.iterator.cls = classType;
                state.current = list;
                build_convertible_bases_tree(list);
                CScope_InitScopeSearch(&state.scope, classType->nspace);
            }
            while ((conversion = CExpr_ConversionIteratorNext(&state)) != NULL) {
                functionType = TYPE_FUNC(conversion->type);
                if (functionType->functype->type == TYPEINT ||
                    (integerOnly == 0 &&
                     (functionType->functype->type == TYPEFLOAT || functionType->functype->type == TYPEPOINTER))) {
                    if (preferBool != 0 && functionType->functype == (Type *)&stbool) {
                        found = 1;
                        ambiguous = 0;
                        selectedType = functionType->functype;
                        break;
                    }
                    if (found)
                        ambiguous = 1;
                    selectedType = functionType->functype;
                    found = 1;
                }
            }
            if (ambiguous)
                CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);
            if (found) {
                if (user_assign_check(result, selectedType, 0, 1, 0, 1) != 0)
                    result = converted_expr;
            }
            break;
        }
        case TYPEMEMBERPOINTER:
            if (integerOnly == 0) {
                ENode *zero = mkintconst((Type *)&stsignedlong);
                result = memberpointercompare(ENOTEQU, result, zero);
            }
            break;
    }
    return result;
}

static ENode *intconstnode_setlong_470460(Type *type, SInt32 value)
{
    CInt64 *iv;
    ENode *node;

    node = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(node, 0x1a);
    node->type = EINTCONST;
    node->rtype = type;
    iv = &node->data.intval;
    iv->lo = value;
    iv->hi = (value < 0) ? -1 : 0;
    return node;
}

static ENode *make_objref_node_470460(Object *obj)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    memclrw(n, 0x1a);
    n->type = EOBJREF;
    n->data.objref = obj;
    n->rtype = (Type *)CDecl_NewPointerType(obj->type);
    if (obj->type->type != TYPEFUNC)
        n->flags = obj->qual & Q_CV;
    obj->flags |= 1;
    return n;
}

static inline ENode *MakeMonadic_470460(ENode *inner, int type)
{
    ENode *e = (ENode *)CompilerTools_AllocatePool(0x1a);
    e->type = type;
    e->cost = inner->cost;
    if (e->cost == 0)
        e->cost = 1;
    e->flags = inner->flags & 3;
    e->rtype = inner->rtype;
    e->data.monadic = inner;
    return e;
}

static void InitLookup(ConversionSearchState *lu, TypeClass *tclass)
{
    memclrw(lu, 0x20);
    if (tclass->flags & CLASS_IS_CONVERTIBLE) {
        lu->iterator.cls = tclass;
        lu->current = &lu->iterator;
        build_convertible_bases_tree(&lu->iterator);
        CScope_InitScopeSearch(&lu->scope, tclass->nspace);
    }
}

SInt16 user_assign_check(ENode *operand, Type *targetType, UInt32 targetQual, Boolean diagnose, Boolean allowExplicit,
                         Boolean conversionMode)
{
    TypeMemberFunc *ft;
    Object *candidate;
    SInt16 bestQualCount;
    Object *bestObject;
    Boolean ambiguous;
    Object *constructor;
    ENode *constructorArg;
    Boolean hasConversion;
    Boolean hasConstructor;
    ConversionSearchState lookup;
    ComparisonValues best;
    ComparisonValues constructorBest;
    ComparisonValues rank;
    BClassList target;
    SInt32 score;
    UInt32 operandFlags;
    UInt32 argumentQual;
    Object *object;
    SInt16 qualCount;
    FuncArg *firstArg;
    FuncArg *argument;
    Type *argumentType;
    ENodeList *args;
    ENode *node;
    NameSpaceObjectList *constructors;
    ENode *expr;
    ENode *call;

    memclrw(&best, sizeof(best));
    hasConversion = 0;
    ambiguous = 0;
    hasConstructor = 0;

    if (targetType->size == 0)
        CDecl_CompleteType(targetType);
    if (operand->rtype->size == 0)
        CDecl_CompleteType(operand->rtype);

    if (operand->rtype->type == TYPECLASS) {
        bestQualCount = 0;
        InitLookup(&lookup, (TypeClass *)operand->rtype);
        while ((candidate = CExpr_ConversionIteratorNext(&lookup)) != NULL) {
            ft = (TypeMemberFunc *)candidate->type;
            call = CompilerTools_AllocatePool(sizeof(*call));
            memclrw(call, sizeof(*call));
            call->type = EPRECOMP;
            expr = call;
            call->rtype = ft->functype;
            if (call->rtype->type == TYPEPOINTER && (TYPE_POINTER(call->rtype)->qual & Q_REFERENCE)) {
                call->rtype = TYPE_POINTER(call->rtype)->target;
                if (!CParser_IsConst(call->rtype, ft->qual)) {
                    expr = MakeMonadic_470460(expr, EINDIRECT);
                    expr->data.monadic->rtype = TYPE(&void_ptr);
                    expr = MakeMonadic_470460(expr, EINDIRECT);
                    expr->data.monadic->rtype = TYPE(&void_ptr);
                }
            }
            score = check_standard_conversion(expr, targetType, 0, conversionMode);
            if ((SInt16)score != 0) {
                init_comparison_values(score, &rank, ft->functype, ft->qual, targetType, targetQual, 1);
                if ((argument = ft->args) == NULL || argument->type->type != TYPEPOINTER)
                    CError_FATAL(2344);
                operandFlags = operand->flags;
                argumentQual = ft->args->qual;
                if ((argumentQual & Q_CONST) || !(operandFlags & Q_CONST)) {
                    if ((argumentQual & Q_VOLATILE) || !(operandFlags & Q_VOLATILE)) {
                        qualCount = 0;
                        if ((argumentQual & Q_CONST) == (operandFlags & Q_CONST))
                            qualCount++;
                        if ((argumentQual & Q_VOLATILE) == (operandFlags & Q_VOLATILE))
                            qualCount++;
                        switch (compare_short_arrays_lexicographically(&rank.kind1, &best.kind1, 1)) {
                            case -1:
                                continue;
                            case 0:
                                if (bestObject == candidate || qualCount < bestQualCount)
                                    continue;
                                if (qualCount == bestQualCount) {
                                    ambiguous = 1;
                                    continue;
                                }
                        }
                        bestObject = candidate;
                        best = rank;
                        hasConversion = 1;
                        ambiguous = 0;
                        bestQualCount = qualCount;
                    }
                }
            }
        }
    }

    if (targetType->type == TYPECLASS) {
        constructors = CClass_Constructor((TypeClass *)targetType);
        if (constructors != NULL) {
            memclrw(&constructorBest, sizeof(constructorBest));
            for (; constructors != NULL; constructors = constructors->next) {
                if ((object = (Object *)constructors->object)->otype != OT_OBJECT || object->type->type != TYPEFUNC)
                    continue;
                if (!allowExplicit && (object->qual & Q_EXPLICIT))
                    continue;
                ft = (TypeMemberFunc *)object->type;
                if (!(firstArg = ft->args) || !(argument = firstArg->next))
                    continue;
                if ((((TypeClass *)targetType)->flags & CLASS_HAS_VBASES) && !(argument = argument->next))
                    continue;
                if (argument == &data_00583098)
                    continue;
                if (argument->next != NULL && argument->next->dexpr == NULL && argument->next != &data_00583098)
                    continue;
                argumentType = argument->type;
                if (argumentType->type == TYPEPOINTER && (TYPE_POINTER(argumentType)->qual & Q_REFERENCE)) {
                    argumentType = TYPE_POINTER(argumentType)->target;
                    if (!CParser_IsConst(argumentType, argument->qual) && !CExpr_IsLValue(operand))
                        continue;
                }
                score = check_standard_conversion(operand, argumentType, 0, conversionMode);
                if ((SInt16)score != 0) {
                    init_comparison_values(score, &rank, ft->functype, ft->qual, targetType, targetQual, 0);
                    switch (compare_short_arrays_lexicographically(&rank.kind1, &constructorBest.kind1, 1)) {
                        case -1:
                        case 0:
                            break;
                        default:
                            constructorArg = operand;
                            constructorBest = rank;
                            hasConstructor = 1;
                            constructor = object;
                    }
                }
            }
            if (hasConstructor) {
                if (hasConversion) {
                    switch (compare_short_arrays_lexicographically(&best.kind1, &constructorBest.kind1, 1)) {
                        case -1:
                            best = constructorBest;
                            hasConversion = 0;
                            break;
                        case 0:
                            ambiguous = 1;
                            break;
                    }
                } else {
                    best = constructorBest;
                }
            }
        }
    }

    if (ambiguous && diagnose)
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);

    if (hasConversion || hasConstructor) {
        if (diagnose) {
            if (hasConversion) {
                ft = (TypeMemberFunc *)bestObject->type;
                if (!(ft->flags & FUNC_METHOD))
                    CError_FATAL(2462);
                if (DAT_00587fd8 != NULL && (*DAT_00587fd8)(0, bestObject) == 0) {
                    node = intconstnode_setlong_470460(TYPE(&void_ptr), 0);
                } else {
                    if (bestObject->sclass == TK_TYPEDEF) {
                        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                        node = intconstnode_setlong_470460(TYPE(&void_ptr), 0);
                    } else {
                        node = make_objref_node_470460(bestObject);
                    }
                }
                bestObject->flags |= OBJECT_USED;
                args = CompilerTools_AllocatePool(sizeof(*args));
                args->next = NULL;
                operand = getnodeaddress(operand, 0);
                args->node =
                    oldassignmentpromotion(operand, CDecl_NewPointerType(TYPE(ft->theclass)), operand->flags, 0);
                call = CompilerTools_AllocatePool(sizeof(*call));
                call->type = EFUNCCALL;
                call->cost = 4;
                call->rtype = ft->functype;
                call->flags = ft->qual & Q_CV;
                call->data.funccall.funcref = node;
                call->data.funccall.args = args;
                call->data.funccall.functype = (TypeFunc *)bestObject->type;
                converted_expr = checkreference(CExpr_AdjustFunctionCall(call));
                if (converted_expr->rtype != targetType)
                    converted_expr = oldassignmentpromotion(converted_expr, targetType, targetQual, 1);
                if (!((ft->functype->type == TYPEPOINTER) && (TYPE_POINTER(ft->functype)->qual & Q_REFERENCE)))
                    data_0058850e = 1;
            } else {
                args = CompilerTools_AllocatePool(sizeof(*args));
                args->next = NULL;
                args->node = constructorArg;
                if (((TypeClass *)targetType)->flags & CLASS_HAS_VBASES) {
                    args->next = CompilerTools_AllocatePool(sizeof(*args->next));
                    args->next->node = constructorArg;
                    args->next->next = NULL;
                    args->node = intconstnode_setlong_470460(TYPE(&stsignedshort), 1);
                }
                target.next = NULL;
                target.type = targetType;
                converted_expr = MakeMonadic_470460(create_temp_node(targetType), EINDIRECT);
                converted_expr->rtype = targetType;
                converted_expr =
                    CExpr_GenericFuncCall(&target, converted_expr, 0, constructor, NULL, NULL, args, 0, 0, 1);
                if (converted_expr->type == EFUNCCALL || converted_expr->type == EFUNCCALLP) {
                    converted_expr->rtype = CDecl_NewPointerType(targetType);
                    converted_expr = MakeMonadic_470460(converted_expr, EINDIRECT);
                    converted_expr->rtype = targetType;
                }
                data_0058850e = 1;
            }
        }
        bestComparison = best;
        return 4;
    }

    if (diagnose)
        CError_ReportError(ERR_CANNOT_CONVERT, operand->rtype, operand->flags & Q_CV, targetType, targetQual);
    return 0;
}

Object *CExpr_ConversionIteratorNext(ConversionSearchState *ctx)
{
    ConvNode *current;
    Type *foundType;
    Boolean matched;
    Type *candidateType;
    ConvNode *item;
    Type *returnType;
    Type *candidateReturnType;
    Object *candidate;
    Object *found;

    if (ctx->current == NULL)
        return NULL;
    current = ctx->current;

    for (;;) {
        if ((found = CScope_NextObject(&ctx->scope)) != NULL) {
            if (found->type->type == TYPEFUNC && (TYPE_FUNC(found->type)->flags & FUNC_CONVERSION)) {
                ScopeSearch local;
                foundType = found->type;

                for (item = current->parent; item != NULL; item = item->parent) {
                    CScope_InitScopeSearch(&local, item->cls->nspace);
                    for (;;) {
                        candidate = CScope_NextObject(&local);
                        if (candidate == NULL)
                            break;
                        if ((candidateType = candidate->type)->type != TYPEFUNC ||
                            !(TYPE_FUNC(candidate->type)->flags & FUNC_CONVERSION))
                            continue;
                        returnType = TYPE_FUNC(foundType)->functype;
                        candidateReturnType = TYPE_FUNC(candidateType)->functype;
                        if (returnType->type == TYPEPOINTER && (TYPE_POINTER(returnType)->qual & Q_REFERENCE))
                            returnType = TYPE_POINTER(returnType)->target;
                        if (candidateReturnType->type == TYPEPOINTER &&
                            (TYPE_POINTER(candidateReturnType)->qual & Q_REFERENCE))
                            candidateReturnType = TYPE_POINTER(candidateReturnType)->target;
                        if (is_typeequal(candidateReturnType, returnType) != 0 &&
                            (TYPE_FUNC(foundType)->args->qual & Q_CONST) ==
                                (TYPE_FUNC(candidateType)->args->qual & Q_CONST) &&
                            (TYPE_FUNC(foundType)->qual & Q_CONST) == (TYPE_FUNC(candidateType)->qual & Q_CONST)) {
                            matched = 1;
                            goto searchDone;
                        }
                    }
                }
                matched = 0;
            searchDone:
                if (matched)
                    continue;
                return found;
            }
        } else {
            for (;;) {
                if (current->list != NULL) {
                    ctx->current = current->list->node;
                    current->list = current->list->next;
                    current = ctx->current;
                    CScope_InitScopeSearch(&ctx->scope, current->cls->nspace);
                    break;
                }
                current = current->parent;
                if (current == NULL)
                    return NULL;
            }
        }
    }
}

void build_convertible_bases_tree(ConvNode *self)
{
    ClassList *base;
    ConvItem *item;
    ConvNode *node;

    for (base = self->cls->bases, item = NULL, node = NULL; base != NULL; base = base->next) {
        if (base->base->flags & CLASS_IS_CONVERTIBLE) {
            node = (ConvNode *)galloc(sizeof(ConvNode));
            memclrw(node, sizeof(ConvNode));
            node->parent = self;
            node->cls = base->base;
            build_convertible_bases_tree(node);
            item = (ConvItem *)galloc(sizeof(ConvItem));
            memclrw(item, sizeof(ConvItem));
            item->node = node;
            item->next = self->list;
            self->list = item;
        }
    }
}

#define STBOOL ((Type *)&stbool)
#define STSINT ((Type *)&stsignedint)
#define STUINT ((Type *)&stunsignedint)
#define STSSHORT ((Type *)&stsignedshort)
#define STUSHORT ((Type *)&stunsignedshort)
#define STULONG ((Type *)&stunsignedlong)
#define STFLOAT ((Type *)&stfloat)
#define STDOUBLE ((Type *)&stdouble)

static ENode *NewMonadic(ENode *inner, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    n->type = ty;
    n->cost = inner->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = inner->flags & 3;
    n->rtype = inner->rtype;
    n->data.monadic = inner;
    return n;
}

SInt32 check_standard_conversion(ENode *node, Type *ty, Boolean convert, Boolean checkAccess)
{
    SInt32 result;
    TypePointer *sourceType;
    TypePointer *targetType;
    SInt16 conversion;
    BClassList *base;
    NameSpaceObjectList candidates;
    SInt16 pointerCost;
    Boolean pointerAmbiguous;
    SInt16 classCost;
    Boolean classAmbiguous;

    if (copts.cplusplus != 0) {
        data_0058852b = 0;
        conversion = is_typeequal(node->rtype, ty);
        if (conversion != 0) {
            converted_expr = node;
            if (conversion == -1) {
                conversion_score = 1;
                return 3;
            }
            return 1;
        }
        if (convert != 0 && data_0058852b != 0) {
            CError_ReportError(ERR_ILLEGAL_IMPLICIT_CONVERSION_FROM, node->rtype, node->flags & 3, ty, 0);
            return 0;
        }
    } else {
        conversion = is_typesame(node->rtype, ty);
        if (conversion != 0) {
            converted_expr = node;
            return 1;
        }
    }

    if (ty == STBOOL) {
        switch ((SInt8)node->rtype->type) {
            case TYPEMEMBERPOINTER:
            case TYPEPOINTER:
                conversion_score = 0;
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
                if (convert != 0)
                    converted_expr = CExpr_ConvertToBool(node, 0);
                return 3;
        }
        return 0;
    }

    if (node->rtype->type == TYPEENUM && (ty->type == TYPEINT || ty->type == TYPEFLOAT)) {
        result = 3;
        if (ty->type == TYPEINT) {
            if (TYPE_ENUM(node->rtype)->enumtype == ty) {
                result = 2;
            } else if (TYPE_INTEGRAL(TYPE_ENUM(node->rtype)->enumtype)->integral < 7) {
                switch (TYPE_INTEGRAL(ty)->integral) {
                    case IT_INT:
                        if (node->rtype->size < stsignedint.size || TYPE_ENUM(node->rtype)->enumtype == STSSHORT)
                            result = 2;
                        break;
                    case IT_UINT:
                        if (node->rtype->size >= stsignedint.size && TYPE_ENUM(node->rtype)->enumtype != STSSHORT)
                            result = 2;
                        break;
                }
            }
        }
        if (convert != 0) {
            node->rtype = TYPE_ENUM(node->rtype)->enumtype;
            converted_expr = CExpr2_00473720(node, ty);
        }
        return result;
    }

    if (node->rtype->type == TYPEINT && (ty->type == TYPEINT || ty->type == TYPEFLOAT)) {
        result = 3;
        if (TYPE_INTEGRAL(node->rtype)->integral <= 7) {
            if (ty == STSINT || ty == STUINT) {
                switch (TYPE_INTEGRAL(ty)->integral) {
                    case IT_INT:
                        if (node->rtype->size < stsignedint.size || ty != STUSHORT)
                            result = 2;
                        break;
                    case IT_UINT:
                        if (node->rtype->size == stsignedint.size && ty == STUSHORT)
                            result = 2;
                        break;
                }
            }
        }
        if (convert != 0 && ty != node->rtype)
            converted_expr = CExpr2_00473720(node, ty);
        else
            converted_expr = node;
        return result;
    }

    if (node->rtype->type == TYPEFLOAT && (ty->type == TYPEINT || ty->type == TYPEFLOAT)) {
        if (ty == STDOUBLE && (node->rtype == STFLOAT || &node->rtype->type == &stshortdouble.type))
            result = 2;
        else
            result = 3;
        if (convert != 0 && (ty->type != TYPEFLOAT || ty->size != node->rtype->size))
            converted_expr = CExpr2_00473720(node, ty);
        else
            converted_expr = node;
        return result;
    }

    if (ty->type == TYPEPOINTER) {
        ENodeTypeX nodeType = (ENodeTypeX)node->type;
        if (nodeType == 0x32) {
            Boolean isZero = (node->data.intval.hi == 0 && node->data.intval.lo == 0);
            if (isZero && (node->rtype->type == TYPEINT || (copts.cplusplus == 0 && node->rtype->type == TYPEENUM))) {
                if (convert != 0)
                    node->rtype = STULONG;
                converted_expr = node;
                return 3;
            }
        }
        if (nodeType == 0x43)
            return match_overloaded_function_pointer(node->data.objlist.list, node->data.objlist.templargs, ty,
                                                     convert);
        if (node->rtype->type == TYPEPOINTER) {
            if (nodeType == 0x38) {
                if (node->data.objref->type->type == TYPEFUNC &&
                    (TYPE_FUNC(node->data.objref->type)->flags & 0x400) != 0) {
                    candidates.next = NULL;
                    candidates.object = (ObjBase *)node->data.objref;
                    return match_overloaded_function_pointer(&candidates, NULL, ty, convert);
                }
            }
            if (copts.f5c != 0 && CObjC_IsIdCompatiblePointerPair(node->rtype, ty) != 0) {
                conversion_score = 1;
                return 3;
            }
            sourceType = TYPE_POINTER(node->rtype);
            targetType = TYPE_POINTER(ty);
            if (sourceType->target->type == TYPECLASS && targetType->target->type == TYPECLASS) {
                BClassList *base = CClass_GetBasePath(TYPE_CLASS(sourceType->target), TYPE_CLASS(targetType->target),
                                                      &pointerCost, &pointerAmbiguous);
                if (base != NULL) {
                    conversion_score = 1000 - pointerCost;
                    if (convert != 0) {
                        if (pointerAmbiguous != 0)
                            CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                        if (checkAccess != 0)
                            CClass_CheckBaseAccess(base, 0);
                        converted_expr = CExpr_ClassPointerCast(base, node, 1);
                    }
                    return 3;
                }
                if (convert != 0) {
                    if (pointerAmbiguous != 0)
                        CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                    else
                        CError_ReportError(ERR_CANNOT_CONVERT, node->rtype, node->flags & 3, ty, 0);
                }
                return 0;
            }
        }
    }

    if (ty->type == TYPEMEMBERPOINTER && node->rtype->type != TYPECLASS)
        return check_member_pointer_conversion(ty, node, convert);

    if (node->rtype->type == TYPECLASS && ty->type == TYPECLASS) {
        BClassList *base = CClass_GetBasePath(TYPE_CLASS(node->rtype), TYPE_CLASS(ty), &classCost, &classAmbiguous);
        if (base != NULL) {
            conversion_score = 1000 - classCost;
            if (convert != 0) {
                if (classAmbiguous != 0)
                    CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                CClass_CheckBaseAccess(base, 0);
                converted_expr = getnodeaddress(node, 0);
                converted_expr = CExpr_ClassPointerCast(base, converted_expr, 0);
                converted_expr = NewMonadic(converted_expr, EINDIRECT);
                converted_expr->rtype = ty;
            }
            return 3;
        }
    }

    if (ty->type == TYPEENUM) {
        switch ((SInt8)node->rtype->type) {
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
                if (copts.cplusplus == 0) {
                    if (convert != 0) {
                        if (copts.f9d != 0)
                            CError_Warning(ERR_ILLEGAL_IMPLICIT_ENUM_CONVERSION_FROM, node->rtype, node->flags & 3, ty,
                                           0);
                        converted_expr = do_typecast(node, ty, 0);
                        converted_expr->flags = node->flags;
                    }
                    return 2;
                }
                if (convert != 0)
                    CError_ReportError(ERR_ILLEGAL_IMPLICIT_ENUM_CONVERSION_FROM, node->rtype, node->flags & 3, ty, 0);
                break;
        }
    }

    return CodeGen_CheckAltivecStypeMatch(node, ty, convert, checkAccess);
}

static ENode *mkconstnode(Type *type)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EINTCONST;
    node->rtype = type;
    return node;
}

static ENode *mkmondonode(ENode *inner, UInt8 type)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = type;
    node->cost = inner->cost;
    if (node->cost == 0)
        node->cost = 1;
    node->flags = inner->flags & (Q_CONST | Q_VOLATILE);
    node->rtype = inner->rtype;
    node->data.monadic = inner;
    return node;
}

ENode *CExpr_ConvertToBool(ENode *node, Boolean flag)
{
    if (node->rtype->type == TYPEMEMBERPOINTER)
        node = CExpr2_ConvertScalarOperand(node, 0, 0);
    switch ((SInt8)node->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
        case TYPEENUM:
        case TYPEPOINTER:
            if (node->rtype->type == TYPEENUM) {
                if (node->rtype->type != TYPEENUM) {
                    CError_ReportError(ERR_ILLEGAL_OPERAND);
                    node = mkconstnode((Type *)&stsignedlong);
                } else {
                    node->rtype = TYPE_ENUM(node->rtype)->enumtype;
                }
            }
            switch (node->type) {
                case EINTCONST: {
                    Boolean iszero = (node->data.intval.hi == 0 && node->data.intval.lo == 0);
                    SInt32 value = !iszero;
                    node->data.intval.lo = value;
                    node->data.intval.hi = value < 0 ? -1 : 0;
                    break;
                }
                case EFLOATCONST: {
                    SInt32 value = !CMach_FloatIsZero(node->data.floatval.data.value);
                    node->data.intval.lo = value;
                    node->data.intval.hi = value < 0 ? -1 : 0;
                    node->type = EINTCONST;
                    break;
                }
                default:
                    node = mkmondonode(node, ELOGNOT);
                    node->rtype = (Type *)&stbool;
                    node = mkmondonode(node, ELOGNOT);
                    break;
            }
            break;
        default: {
            UInt16 error = flag ? 0xf7 : 0xd1;
            CError_ReportError(error, node->rtype, node->flags & (Q_CONST | Q_VOLATILE), &stbool, 0);
            node = mkconstnode((Type *)&stsignedlong);
            break;
        }
    }
    node->rtype = (Type *)&stbool;
    return node;
}

SInt32 match_overloaded_function_pointer(NameSpaceObjectList *list, void *templateArguments, Type *type,
                                         UInt8 reportErrors)
{
    ENode *result;
    Object *match;
    Object *object;
    TemplFuncInstance *specialization;
    ENode *node;
    Boolean ambiguous;
    Boolean haveNonTemplateMatch;

    if (type->type != TYPEPOINTER || (type = TPTR_TARGET(type))->type != TYPEFUNC)
        return 0;
    match = NULL;
    haveNonTemplateMatch = 0;
    ambiguous = 0;
    while (list != NULL) {
        if ((object = (Object *)list->object)->otype == OT_OBJECT) {
            if (object->type->type == TYPEFUNC && (((TypeFunc *)object->type)->flags & 0x400)) {
                if (!haveNonTemplateMatch) {
                    if (CTemplateTools_MatchTypeAndCheckBoundSlots(object, type, templateArguments)) {
                        specialization =
                            instantiate_accessible_template_for_type(object, type, templateArguments, NULL, 0);
                        if (specialization == NULL)
                            CError_FATAL(1736);
                        if (iscpp_typeequal(specialization->object->type, type)) {
                            if (match != NULL && match != specialization->object)
                                ambiguous = 1;
                            else
                                match = specialization->object;
                        }
                    }
                }
            } else {
                if (iscpp_typeequal(object->type, type)) {
                    if (match != NULL && haveNonTemplateMatch) {
                        Object *candidate;
                        Object *previousMatch;
                        candidate = object;
                        if (object->datatype == DALIAS)
                            candidate = object->u.alias.object;
                        previousMatch = match;
                        if (match->datatype == DALIAS)
                            previousMatch = match->u.alias.object;
                        if (candidate != previousMatch)
                            ambiguous = 1;
                    } else {
                        ambiguous = 0;
                        match = object;
                    }
                    haveNonTemplateMatch = 1;
                }
            }
        }
        list = list->next;
    }
    if (match != NULL) {
        if (reportErrors) {
            if (ambiguous)
                CError_ReportError(ERR_AMBIGUOUS_ACCESS_OVERLOADED_FUNCTION);
            if (match->sclass == TK_TYPEDEF) {
                CInt64 *value;
                CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(node, sizeof(ENode));
                node->type = EINTCONST;
                node->rtype = (Type *)&void_ptr;
                value = &node->data.intval;
                value->lo = 0;
                value->hi = 0;
            } else {
                node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                memclrw(node, sizeof(ENode));
                node->type = EOBJREF;
                node->data.objref = match;
                node->rtype = CDecl_NewPointerType(match->type);
                if (match->type->type != TYPEFUNC)
                    node->flags = match->qual & Q_CV;
                match->flags |= 1;
            }
            converted_expr = result = node;
            node->rtype = CDecl_NewPointerType(match->type);
            node->flags = object->qual & Q_CV;
            match->flags |= 1;
            if (match->datatype == DINLINEFUNC)
                CError_ReportError(ERR_ILLEGAL_USE_INLINE_FUNCTION);
        }
        return 1;
    }
    return 0;
}

static ENode *makemonadic(ENode *inner, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = ty;
    n->cost = inner->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = inner->flags & ENODE_FLAG_QUALS;
    n->rtype = inner->rtype;
    n->data.monadic = inner;
    return n;
}

ENode *CExpr2_004719c0(BClassList *scope, BClassList *baseList, ENode *node, UInt8 access, Boolean checkAccess)
{
    TypeClass *rtype;
    BClassList *classList;

    if (node == NULL) {
        if (data_00588238 == NULL || data_00588040 == NULL || data_005884f8 == 0 ||
            (node = CClass_CreateThisSelfExpr()) == NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
            return NULL;
        }
        node = makemonadic(node, EINDIRECT);
        node->rtype = (Type *)data_00588040;
    }
    CError_ASSERT(1662, scope != NULL);
    CError_ASSERT(1663, node->rtype->type == TYPECLASS);
    rtype = (TypeClass *)node->rtype;
    classList = CScope_GetClassAccessPath(scope, rtype);
    if (classList == NULL || classList->type != (Type *)rtype) {
        CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
        return NULL;
    }
    if (checkAccess > 0)
        CClass_CheckBaseAccess(classList, access);
    if (TYPE_CLASS(classList->type)->sominfo == NULL) {
        if (baseList != NULL)
            classList = CClass_AppendPath(classList, baseList);
        if (node->type != EINDIRECT)
            node = CExpr_LValue(node, 0, 1);
        if (node->type == EINDIRECT) {
            node->data.monadic->flags = node->flags;
            node = node->data.monadic;
            switch (node->type) {
                case EPOSTINC:
                case EPOSTDEC:
                case EPREINC:
                case EPREDEC:
                    node = makemonadic(node, ETYPCON);
                    break;
            }
            node = makemonadic(CExpr_ClassPointerCast(classList, node, 0), EINDIRECT);
            node->rtype = (Type *)rtype;
        }
    }
    return node;
}

static ENode *NewIntConst(Type *type, SInt32 value)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    CInt64 *iv;
    memclrw(n, 0x1a);
    n->type = EINTCONST;
    n->rtype = type;
    iv = &n->data.intval;
    iv->lo = value;
    iv->hi = (value < 0) ? -1 : 0;
    return n;
}

static ENode *create_monadic_node(ENode *inner, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    n->type = ty;
    n->cost = inner->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = inner->flags & 3;
    n->rtype = inner->rtype;
    n->data.monadic = inner;
    return n;
}

static ENode *NewDiadic(ENode *left, ENode *right, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);
    n->type = ty;
    n->rtype = left->rtype;
    n->data.diadic.left = left;
    n->data.diadic.right = right;
    if (left->cost != right->cost) {
        n->cost = right->cost;
        if (left->cost > n->cost)
            n->cost = left->cost;
    } else {
        n->cost = right->cost + 1;
        if (n->cost > 200)
            n->cost = 200;
    }
    n->flags = (left->flags | right->flags) & 3;
    return n;
}

ENode *CExpr_ClassPointerCast(BClassList *path, ENode *node, Boolean checkNull)
{
    ENode *result = node;
    TypeClass *currentClass = TYPE_CLASS(path->type);
    Boolean changed = 0;
    ClassList *base;
    ENode *constant;

    CError_ASSERT(1567, path != NULL);

    if (node->rtype->type != TYPEPOINTER) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        return node;
    }

    for (path = path->next; path != NULL; path = path->next) {
        for (base = currentClass->bases; base != NULL; base = base->next) {
            if ((Type *)base->base == path->type)
                break;
        }

        if (base == NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
            while (path->next != NULL)
                path = path->next;
            constant = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            memclrw(constant, sizeof(ENode));
            constant->type = EINTCONST;
            constant->rtype = (Type *)&stsignedlong;
            constant->rtype = CDecl_NewPointerType(path->type);
            return constant;
        }

        if (base->is_virtual) {
            if (base->base->sominfo == NULL) {
                SInt32 offset;
                changed = 1;
                offset = base->offset;
                if (offset != 0) {
                    if (canadd(result, offset) == 0) {
                        constant = NewIntConst((Type *)&stunsignedlong, offset);
                        result = NewDiadic(result, constant, EADD);
                        optimizecomm(result);
                    }
                }
                result->rtype = CDecl_NewPointerType((Type *)CDecl_NewPointerType((Type *)base->base));
                result = create_monadic_node(result, EINDIRECT);
            }
        } else {
            SInt32 offset = base->offset;
            if (offset != 0) {
                changed = 1;
                if (canadd(result, offset) == 0) {
                    constant = NewIntConst((Type *)&stunsignedlong, offset);
                    result = NewDiadic(result, constant, EADD);
                    optimizecomm(result);
                }
            }
        }

        switch (result->type) {
            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
                result = create_monadic_node(result, ETYPCON);
                break;
        }

        result->rtype = CDecl_NewPointerType((Type *)base->base);
        currentClass = TYPE_CLASS(path->type);
    }

    if (checkNull != 0 && changed != 0) {
        result = CExpr_New_EPRECOMP_Node(result, node);
    }
    return result;
}

SInt32 check_member_pointer_conversion(Type *type, ENode *expr, Boolean convert)
{
    ENode *node = CompilerTools_AllocatePool(sizeof(ENode));
    *node = *expr;
    if (node->rtype->type != TYPEMEMBERPOINTER) {
        node = CExpr_MemberPointerConversion(node, type, convert);
        if (is_typeequal(node->rtype, type) != 0) {
            if (convert != 0)
                converted_expr = node;
            return 3;
        }
    }
    if (node->rtype->type == TYPEMEMBERPOINTER) {
        CError_ASSERT(1531, TYPE_MEMBER_POINTER(type)->ty2->type == TYPECLASS);
        CError_ASSERT(1532, TYPE_MEMBER_POINTER(node->rtype)->ty2->type == TYPECLASS);
        CClass_Init();
        if (CClass_FindBasePath(TYPE_CLASS(TYPE_MEMBER_POINTER(type)->ty2),
                                TYPE_CLASS(TYPE_MEMBER_POINTER(node->rtype)->ty2), 0, 1) != 0) {
            conversion_score = 1000 - CClass_GetBasePathLevel();
            if (convert != 0)
                converted_expr =
                    CExpr_CastMemberPointer(node, TYPE_MEMBER_POINTER(node->rtype), TYPE_MEMBER_POINTER(type));
            return 3;
        }
    }
    return 0;
}

SInt16 compare_short_arrays_lexicographically(SInt16 *left, SInt16 *right, Boolean compareFifth)
{
    if (left[0] > right[0])
        return 1;
    if (left[0] == right[0]) {
        if (left[1] > right[1])
            return 1;
        if (left[1] == right[1]) {
            if (left[2] > right[2])
                return 1;
            if (left[2] == right[2]) {
                if (left[3] > right[3])
                    return 1;
                if (left[3] == right[3]) {
                    if (!compareFifth)
                        return 0;
                    if (left[4] > right[4])
                        return 1;
                    if (left[4] == right[4])
                        return 0;
                }
            }
        }
    }
    return -1;
}

Boolean CExpr2_UpdateArgMatchScores(Type *target, UInt32 qualifiers, ENode *expression, ArgMatch *scores)
{
    SInt16 score = assign_check(expression, target, qualifiers, 0, 0, 1);
    switch (score) {
        case 0:
            return 0;
        case 1:
            scores->score1Count++;
            break;
        case 2:
            scores->score2Count++;
            break;
        case 3:
            scores->score3Count++;
            scores->score3Value += conversion_score;
            break;
        case 4:
            scores->score4Count++;
            scores->score4Value0 += bestComparison.kind1;
            scores->score4Value1 += bestComparison.kind2;
            scores->score4Value2 += bestComparison.kind3;
            scores->score4Value3 += bestComparison.kind3Weight;
            scores->score4Value4 += bestComparison.qualifierMatches;
            break;
        default:
            CError_FATAL(1462);
    }
    if (target->type == TYPEPOINTER) {
        CExpr_MatchCV(expression->rtype, expression->flags & ENODE_FLAG_QUALS, target, qualifiers, scores);
    }
    return 1;
}

void CExpr_MatchCV(Type *firstType, UInt32 firstQualifiers, Type *secondType, UInt32 secondQualifiers, ArgMatch *match)
{
    Boolean comparePointerQualifiers;

    if (TYPE_POINTER(secondType)->qual & Q_REFERENCE) {
        secondType = TPTR_TARGET(secondType);
        comparePointerQualifiers = 1;
    } else {
        comparePointerQualifiers = 0;
    }

    while (secondType->type == TYPEPOINTER && firstType->type == TYPEPOINTER) {
        if (comparePointerQualifiers) {
            if ((TYPE_POINTER(firstType)->qual & Q_CONST) != (TYPE_POINTER(secondType)->qual & Q_CONST))
                match->qualificationPenalty--;
            if ((TYPE_POINTER(firstType)->qual & Q_VOLATILE) != (TYPE_POINTER(secondType)->qual & Q_VOLATILE))
                match->qualificationPenalty--;
        }
        secondType = TPTR_TARGET(secondType);
        firstType = TPTR_TARGET(firstType);
        comparePointerQualifiers = 1;
    }

    if ((firstQualifiers & Q_CONST) != (secondQualifiers & Q_CONST))
        match->qualificationPenalty--;
    if ((firstQualifiers & Q_VOLATILE) != (secondQualifiers & Q_VOLATILE))
        match->qualificationPenalty--;
}

void init_comparison_values(unsigned int kind, ComparisonValues *counts, Type *sourceType, unsigned int sourceQual,
                            Type *targetType, unsigned int targetQual, unsigned int flag)
{
    unsigned int compareQual;
    memclrw(counts, 10U);
    switch ((short)kind) {
        case 1:
            counts->kind1 += 1U;
            break;
        case 2:
            counts->kind2 += 1U;
            break;
        case 3:
            counts->kind3 += 1U;
            counts->kind3Weight += conversion_score;
            break;
        default:
            CError_FATAL(1381);
            break;
    }
    do {
        if ((unsigned char)flag == 0U) {
            if (targetType->type != TYPEPOINTER)
                break;
            compareQual = (sourceType->type == TYPEPOINTER) || ((((TypePointer *)targetType)->qual & 0x20U) != 0U);
            if (compareQual == 0U)
                break;
        }
        if ((targetQual & 1U) == (sourceQual & 1U))
            counts->qualifierMatches += 1U;
        if ((targetQual & 2U) == (sourceQual & 2U))
            counts->qualifierMatches += 1U;
    } while (0);
}

/* An integer constant node built the way CInt64_SetLong fills one: inlined where CExpr_FuncCallSix makes its null
   pointers, so it has no out-of-line copy of its own. */
static ENode *intconstnode_setlong(Type *type, SInt32 value)
{
    CInt64 *iv;
    ENode *node;

    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EINTCONST;
    node->rtype = type;
    iv = &node->data.intval;
    iv->lo = value;
    iv->hi = (value < 0) ? -1 : 0;
    return node;
}

static ENode *make_objref_node(Object *obj)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(n, sizeof(ENode));
    n->type = EOBJREF;
    n->data.objref = obj;
    n->rtype = CDecl_NewPointerType(obj->type);
    if (obj->type->type != TYPEFUNC)
        n->flags = obj->qual & Q_CV;
    obj->flags |= 1;
    return n;
}

ENode *CExpr_FuncCallSix(Object *function, ENode *firstArgument, ENode *secondArgument, ENode *thirdArgument,
                         ENode *fourthArgument, ENode *fifthArgument, ENode *sixthArgument)
{
    ENode *CExpr_AdjustFunctionCall(ENode *);
    Type *type = function->type;
    ENode *call;
    ENode *functionRef;
    ENodeList *arguments;

    CError_ASSERT(1337, type->type == TYPEFUNC);

    call = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    call->type = EFUNCCALL;
    call->cost = 4;
    call->rtype = TYPE_FUNC(type)->functype;
    call->flags = TYPE_FUNC(type)->qual & Q_CV;

    if (DAT_00587fd8 != NULL && DAT_00587fd8(0, function) == 0) {
        functionRef = intconstnode_setlong((Type *)&void_ptr, 0);
    } else if (function->sclass == TK_TYPEDEF) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        functionRef = intconstnode_setlong((Type *)&void_ptr, 0);
    } else {
        functionRef = make_objref_node(function);
    }

    call->data.funccall.funcref = functionRef;
    call->data.funccall.functype = (TypeFunc *)type;

    arguments = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    call->data.funccall.args = arguments;
    arguments->node = firstArgument;

    arguments->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    arguments = arguments->next;
    arguments->node = secondArgument;

    arguments->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    arguments = arguments->next;
    arguments->node = thirdArgument;

    arguments->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    arguments = arguments->next;
    arguments->node = fourthArgument;

    arguments->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
    arguments = arguments->next;
    arguments->node = fifthArgument;

    if (sixthArgument != NULL) {
        arguments->next = (ENodeList *)CompilerTools_AllocatePool(sizeof(ENodeList));
        arguments = arguments->next;
        arguments->node = sixthArgument;
    }
    arguments->next = NULL;

    return CExpr_AdjustFunctionCall(call);
}

ENode *funccallexpr(Object *func, ENode *firstArgument, ENode *secondArgument, ENode *thirdArgument,
                    ENode *fourthArgument)
{
    TypeFunc *functionType = (TypeFunc *)func->type;
    ENode *call;
    ENode *functionRef;
    ENodeList *arguments;

    CError_ASSERT(1288, functionType->type == TYPEFUNC);

    call = CompilerTools_AllocatePool(sizeof(ENode));
    call->type = EFUNCCALL;
    call->cost = 4;
    call->rtype = functionType->functype;
    call->flags = functionType->qual & Q_CV;

    if (DAT_00587fd8 != NULL && (*DAT_00587fd8)(0, func) == 0) {
        CInt64 *value;
        functionRef = CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(functionRef, sizeof(ENode));
        functionRef->type = EINTCONST;
        functionRef->rtype = (Type *)&void_ptr;
        value = &functionRef->data.intval;
        value->lo = 0;
        value->hi = 0;
    } else if (func->sclass == TK_TYPEDEF) {
        CInt64 *value;
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        functionRef = CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(functionRef, sizeof(ENode));
        functionRef->type = EINTCONST;
        functionRef->rtype = (Type *)&void_ptr;
        value = &functionRef->data.intval;
        value->lo = 0;
        value->hi = 0;
    } else {
        functionRef = CompilerTools_AllocatePool(sizeof(ENode));
        memclrw(functionRef, sizeof(ENode));
        functionRef->type = EOBJREF;
        functionRef->data.objref = func;
        functionRef->rtype = CDecl_NewPointerType(func->type);
        if (!IS_TYPE_FUNC(func->type))
            functionRef->flags = func->qual & Q_CV;
        func->flags |= OBJECT_USED;
    }

    call->data.funccall.funcref = functionRef;
    call->data.funccall.functype = functionType;

    if (firstArgument != NULL) {
        arguments = CompilerTools_AllocatePool(sizeof(ENodeList));
        call->data.funccall.args = arguments;
        arguments->node = firstArgument;
        if (secondArgument != NULL) {
            arguments->next = CompilerTools_AllocatePool(sizeof(ENodeList));
            arguments = arguments->next;
            arguments->node = secondArgument;
            if (thirdArgument != NULL) {
                arguments->next = CompilerTools_AllocatePool(sizeof(ENodeList));
                arguments = arguments->next;
                arguments->node = thirdArgument;
                if (fourthArgument != NULL) {
                    arguments->next = CompilerTools_AllocatePool(sizeof(ENodeList));
                    arguments = arguments->next;
                    arguments->node = fourthArgument;
                }
            }
        }
        arguments->next = NULL;
    } else {
        call->data.funccall.args = NULL;
    }

    return CExpr_AdjustFunctionCall(call);
}

static ENode *wrap_call(ENode *p)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(0x1a);
    node->type = EINDIRECT;
    node->cost = p->cost;
    if (node->cost == 0)
        node->cost = 1;
    node->flags = p->flags & 3;
    node->rtype = p->rtype;
    node->data.monadic = p;
    node->data.monadic->rtype = CDecl_NewPointerType(node->rtype);
    return node;
}

ENode *CExpr_AdjustFunctionCall(ENode *p)
{
    ENodeList *item;
    ENode *node;

    switch ((SInt8)p->data.funccall.functype->functype->type) {
        case TYPECLASS:
            CDecl_CompleteType(p->data.funccall.functype->functype);
            /* fall through */
        case TYPESTRUCT:
            if (p->data.funccall.functype->functype->size == 0)
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, p->data.funccall.functype->functype,
                                   0);
            break;
    }

    if (CMachine_FunctionRequiresMemoryReturn(p->data.funccall.functype)) {
        item = (ENodeList *)CompilerTools_AllocatePool(8);
        if (p->data.funccall.functype->functype->type == TYPECLASS) {
            CDecl_CompleteType(p->data.funccall.functype->functype);
            if (CClass_Destructor((TypeClass *)p->data.funccall.functype->functype))
                item->node = create_temp_node2(p->rtype);
            else
                item->node = create_temp_node(p->rtype);
        } else {
            item->node = create_temp_node(p->rtype);
        }
        item->next = p->data.funccall.args;
        p->data.funccall.args = item;

        if (p->data.funccall.funcref->flags & 0x10)
            p = CSOM_AppendPointerArgCall(p, item);

        return wrap_call(p);
    }

    if (p->data.funccall.funcref->flags & 0x10)
        p = CSOM_AppendPointerArgCall(p, NULL);
    return p;
}

ENode *CExpr_IsTempConstruction(ENode *e, Type *type, ENode **out)
{
    ENodeList *args;

    if (e->type != EINDIRECT || e->rtype != type || e->data.monadic->type != EFUNCCALL ||
        (args = e->data.monadic->data.funccall.args) == NULL)
        return NULL;

    if (e->data.monadic->data.funccall.funcref->type != EOBJREF ||
        CClass_IsDestructor(e->data.monadic->data.funccall.funcref->data.objref) == 0) {
        if (e->data.monadic->data.funccall.functype->functype != type)
            return NULL;
        if (CInline_ReturnZero((Type *)e->data.monadic->data.funccall.functype) == 1) {
            args = args->next;
            if (args == NULL)
                CError_FATAL(1154);
        }
    }
    if (out != NULL)
        *out = args->node;
    return e->data.monadic;
}

ENode *create_objectnode(Object *object)
{
    ENode *expr;

    expr = CExpr_New_EINDIRECT_Node(object);
    checkreference(expr);
}

static ENode *mknode0(Type *type)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EINTCONST;
    node->rtype = type;
    return node;
}

static ENode *mkptrconst(void)
{
    ENode *node;
    CInt64 *ip;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EINTCONST;
    node->rtype = (Type *)&void_ptr;
    ip = &node->data.intval;
    ip->lo = 0;
    ip->hi = 0;
    return node;
}

static ENode *mkobjref(Object *obj)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EOBJREF;
    node->data.objref = obj;
    node->rtype = CDecl_NewPointerType(obj->type);
    if (obj->type->type != TYPEFUNC)
        node->flags = obj->qual & Q_CV;
    obj->flags |= 1;
    return node;
}

static ENode *mkmono(ENode *inner, UInt8 ty)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = ty;
    node->cost = inner->cost;
    if (node->cost == 0)
        node->cost = 1;
    node->flags = inner->flags & 3;
    node->rtype = inner->rtype;
    node->data.monadic = inner;
    node->rtype = TYPE_POINTER(node->rtype)->target;
    return node;
}

ENode *CExpr_New_EINDIRECT_Node(Object *obj)
{
    ENode *val;
    if (DAT_00587fd8 != NULL) {
        if ((*DAT_00587fd8)(0, obj) == 0)
            return mknode0((Type *)&stsignedlong);
    }
    if (obj->sclass == TK_TYPEDEF) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        val = mkptrconst();
    } else {
        val = mkobjref(obj);
    }
    return mkmono(val, EINDIRECT);
}

static ENode *make_null_pointer_const(void)
{
    ENode *node;
    CInt64 *ip;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EINTCONST;
    node->rtype = (Type *)&void_ptr;
    ip = &node->data.intval;
    ip->lo = 0;
    ip->hi = 0;
    return node;
}

static ENode *create_objref(Object *obj)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EOBJREF;
    node->data.objref = obj;
    node->rtype = CDecl_NewPointerType(obj->type);
    if (obj->type->type != TYPEFUNC)
        node->flags = obj->qual & Q_CV;
    obj->flags |= 1;
    return node;
}

ENode *create_objectrefnode(Object *obj)
{
    ENode *val;
    if (DAT_00587fd8 != NULL) {
        if ((*DAT_00587fd8)(0, obj) == 0)
            return make_null_pointer_const();
    }
    if (obj->sclass == TK_TYPEDEF) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        val = make_null_pointer_const();
    } else {
        val = create_objref(obj);
    }
    return val;
}

ENode *CExpr_MakeObjRefNode(Object *obj, Boolean flag)
{
    ENode *node;
    ENodeUnion *data;
    if (obj->sclass == TK_TYPEDEF) {
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        node = (ENode *)CompilerTools_AllocatePool(sizeof(*node));
        memclrw(node, sizeof(*node));
        node->type = EINTCONST;
        node->rtype = (Type *)&void_ptr;
        data = &node->data;
        data->intval.lo = 0;
        data->intval.hi = 0;
        return node;
    }
    node = (ENode *)CompilerTools_AllocatePool(sizeof(*node));
    memclrw(node, sizeof(*node));
    node->type = EOBJREF;
    node->data.addr.objref = obj;
    node->rtype = CDecl_NewPointerType(obj->type);
    if (obj->type->type != TYPEFUNC)
        node->flags = obj->qual & Q_CV;
    if (flag)
        obj->flags |= OBJECT_USED;
    return node;
}

#define MAX_COST 200

static ENode *mkTemp_472ae0(Type *t)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = EPRECOMP;
    n->cost = 0;
    n->flags = 0;
    n->rtype = CDecl_NewPointerType(t);
    n->data.temp.type = t;
    n->data.temp.uniqueid = CParser_GetUniqueID();
    n->data.temp.needs_dtor = 0;
    return n;
}

static ENode *CExpr2_MakeMonadic(ENode *inner, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);

    n->type = ty;
    n->cost = inner->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = inner->flags & 3;
    n->rtype = inner->rtype;
    n->data.monadic = inner;
    return n;
}

static ENode *CExpr2_MakeDiadic(ENode *left, ENode *right, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(0x1a);

    n->type = ty;
    n->rtype = left->rtype;
    n->data.diadic.left = left;
    n->data.diadic.right = right;
    if (left->cost != right->cost) {
        n->cost = right->cost;
        if (left->cost > n->cost)
            n->cost = left->cost;
    } else {
        n->cost = right->cost + 1;
        if (n->cost > MAX_COST)
            n->cost = MAX_COST;
    }
    n->flags = (left->flags | right->flags) & 3;
    return n;
}

ENode *CExpr_LValue(ENode *expr, Boolean checkConst, Boolean reportError)
{
    for (;;) {
        UInt8 objtype = expr->type;
        switch (objtype) {
            case ETYPCON:
                if ((!copts.f69 && copts.rejectZeroLengthArrayMembers) || expr->rtype->type != TYPEPOINTER ||
                    expr->data.monadic->rtype->type != TYPEPOINTER)
                    break;
                switch (expr->data.monadic->type) {
                    case EINDIRECT:
                    case ETYPCON:
                        expr->data.monadic->rtype = expr->rtype;
                        expr->data.monadic->flags = expr->flags;
                        expr = expr->data.monadic;
                        continue;
                }
                break;

            case EINDIRECT: {
                UInt32 qual;
                if (reportError && !CExpr_IsLValue(expr))
                    CError_Warning(ERR_NOT_LVALUE);
                if (checkConst) {
                    switch ((SInt8)expr->rtype->type) {
                        case TYPEPOINTER:
                            qual = TYPE_POINTER(expr->rtype)->qual;
                            break;
                        case TYPEMEMBERPOINTER:
                            qual = TYPE_MEMBER_POINTER(expr->rtype)->qual;
                            break;
                        default:
                            qual = expr->flags;
                            break;
                    }
                    if (qual & Q_CONST)
                        CError_ReportError(ERR_ILLEGAL_ASSIGNMENT_CONSTANT);
                }
                return expr;
            }

            case EPREINC:
            case EPREDEC:
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
            case ECOMMA: {
                ENode *tmp, *comma, *result;
                if (!copts.cplusplus)
                    break;
                tmp = get_indirect_operand(expr);
                if (tmp != NULL) {
                    comma = CExpr2_MakeDiadic(expr, tmp, ECOMMA);
                    comma->rtype = comma->data.diadic.right->rtype;
                    result = CExpr2_MakeMonadic(comma, EINDIRECT);
                    result->rtype = expr->rtype;
                    return result;
                }
                CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
                return expr;
            }

            case ECOND: {
                ENode *result;
                if (!copts.cplusplus)
                    break;
                if (!iscpp_typeequal(expr->data.cond.expr1->rtype, expr->data.cond.expr2->rtype))
                    break;
                if (expr->data.cond.expr1->rtype->type == TYPEPOINTER &&
                    (expr->data.cond.expr1->flags & 3) != (expr->data.cond.expr2->flags & 3))
                    break;
                expr->data.cond.expr1 = CExpr_LValue(expr->data.cond.expr1, checkConst, reportError);
                expr->data.cond.expr2 = CExpr_LValue(expr->data.cond.expr2, checkConst, reportError);
                if (expr->data.cond.expr1->type != EINDIRECT || expr->data.cond.expr2->type != EINDIRECT ||
                    expr->data.cond.expr1->data.monadic->type == EBITFIELD ||
                    expr->data.cond.expr2->data.monadic->type == EBITFIELD)
                    break;
                expr->data.cond.expr1 = getnodeaddress(expr->data.cond.expr1, 0);
                expr->data.cond.expr2 = getnodeaddress(expr->data.cond.expr2, 0);
                expr->rtype = expr->data.cond.expr1->rtype;
                result = CExpr2_MakeMonadic(expr, EINDIRECT);
                result->rtype = TPTR_TARGET(result->rtype);
                return result;
            }

            case EFUNCCALL: {
                ENode *temp, *tempCopy, *indirect, *assignment, *comma, *result;
                if (expr->rtype->type != TYPECLASS && expr->rtype->type != TYPESTRUCT)
                    break;
                if (Type_RequiresMemoryReturn(expr->rtype))
                    break;
                temp = mkTemp_472ae0(expr->rtype);
                tempCopy = (ENode *)CompilerTools_AllocatePool(0x1a);
                *tempCopy = *temp;
                indirect = CExpr2_MakeMonadic(temp, EINDIRECT);
                indirect->rtype = expr->rtype;
                assignment = CExpr2_MakeDiadic(indirect, expr, EASS);
                comma = CExpr2_MakeDiadic(assignment, tempCopy, ECOMMA);
                comma->rtype = tempCopy->rtype;
                result = CExpr2_MakeMonadic(comma, EINDIRECT);
                result->rtype = expr->rtype;
                return result;
            }

            default:
                break;
        }
        break;
    }
    if (reportError)
        CError_ReportError(ERR_NOT_LVALUE);
    return expr;
}

Boolean CExpr_IsLValue(ENode *expr)
{
    Boolean flag = copts.cplusplus;

    while (expr->type != EINDIRECT) {
        switch (expr->type) {
            case EPREINC:
            case EPREDEC:
                return copts.cplusplus;
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
                if (!flag)
                    return 0;
                expr = expr->data.diadic.left;
                break;
            case ECOMMA:
                if (!flag)
                    return 0;
                expr = expr->data.diadic.right;
                break;
            case ECOND:
                if (copts.cplusplus && iscpp_typeequal(expr->data.cond.expr1->rtype, expr->data.cond.expr2->rtype) &&
                    !(expr->data.cond.expr1->rtype->type == TYPEPOINTER &&
                      (expr->data.cond.expr1->flags & ENODE_FLAG_QUALS) !=
                          (expr->data.cond.expr2->flags & ENODE_FLAG_QUALS)))
                    return CExpr_IsLValue(expr->data.cond.expr1) && CExpr_IsLValue(expr->data.cond.expr2);
                return 0;
            default:
                return 0;
        }
    }

    expr = expr->data.monadic;
    switch (expr->type) {
        case EPRECOMP:
            return 0;
        case EFUNCCALL:
            if (TYPE_FUNC(expr->data.funccall.functype)->functype->type != TYPEPOINTER)
                return 0;
            break;
    }
    return 1;
}

/* 0x4463d0; one int arg */
/* stbool 0x55f5a8, stsignedlong 0x55f5f0 declared in the headers */

static ENode *make_monadic(ENode *inner, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = ty;
    n->cost = inner->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = inner->flags & 3;
    n->rtype = inner->rtype;
    n->data.monadic = inner;
    return n;
}

static ENode *make_diadic(ENode *left, ENode *right, UInt8 ty)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = ty;
    n->rtype = left->rtype;
    n->data.diadic.left = left;
    n->data.diadic.right = right;
    if (left->cost != right->cost) {
        n->cost = right->cost;
        if (left->cost > n->cost)
            n->cost = left->cost;
    } else {
        n->cost = right->cost + 1;
        if (n->cost > 200)
            n->cost = 200;
    }
    n->flags = (left->flags | right->flags) & 3;
    return n;
}

static ENode *make_intconst(Type *type, SInt32 value)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(n, sizeof(ENode));
    n->type = EINTCONST;
    n->rtype = (Type *)&stsignedlong;
    n->rtype = type;
    n->data.intval.lo = (UInt32)value;
    n->data.intval.hi = (value < 0) ? -1 : 0;
    return n;
}

ENode *CExpr_TempModifyExpr(ENode *expr)
{
    Type *type;
    ENode *node;
    ENode *temp;
    ENode *tempRef;
    ENode *assignment;
    ENode *flagRef;
    ENode *trueValue;
    ENode *flagAssignment;
    ENode *sequence;
    ENode *resultRef;

    type = expr->rtype;
    temp = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    temp->type = EPRECOMP;
    temp->cost = 0;
    temp->flags = 0;
    temp->rtype = CDecl_NewPointerType(type);
    temp->data.temp.type = type;
    temp->data.temp.uniqueid = CParser_GetUniqueID();
    temp->data.temp.needs_dtor = 0;

    node = get_indirect_operand(expr);
    if (node == NULL) {
        CError_ReportError(ERR_NOT_LVALUE);
        return expr;
    }

    tempRef = make_monadic(temp, EINDIRECT);
    tempRef->rtype = type;
    assignment = make_diadic(tempRef, expr, EASS);
    flagRef = make_monadic(node, EINDIRECT);
    flagRef->rtype = type;
    trueValue = make_intconst((Type *)&stbool, 1);
    flagAssignment = make_diadic(flagRef, trueValue, EASS);
    sequence = make_diadic(assignment, flagAssignment, ECOMMA);
    resultRef = make_monadic(temp, EINDIRECT);
    resultRef->rtype = type;
    return make_diadic(sequence, resultRef, ECOMMA);
}

ENode *get_indirect_operand(ENode *node)
{
    for (;;) {
        switch (node->type) {
            case EINDIRECT:
                node = node->data.monadic;
                for (;;) {
                    switch (node->type) {
                        case EOBJREF: {
                            ENode *copy = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
                            *copy = *node;
                            return copy;
                        }
                        case ECOMMA:
                            node = node->data.diadic.right;
                            break;
                        default:
                            return CExpr2_RewriteExprToTemp(node);
                    }
                }
            case EPOSTINC:
            case EPOSTDEC:
            case EPREINC:
            case EPREDEC:
                node = node->data.monadic;
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
                node = node->data.diadic.left;
                break;
            case ECOMMA:
                node = node->data.diadic.right;
                break;
            default:
                return NULL;
        }
    }
}

static inline ENode *CExpr_ArithmeticError(void)
{
    ENode *node;

    CError_ReportError(ERR_ILLEGAL_OPERAND);
    node = (ENode *)CompilerTools_AllocatePool(26);
    memclrw(node, 26);
    node->type = EINTCONST;
    node->rtype = (Type *)&stsignedlong;
    return node;
}

/* An enum operand takes its underlying type; an operand that is not arithmetic is an error. */
static inline void CExpr_ArithmeticOperand(ENode **expr)
{
    switch ((SInt8)(*expr)->rtype->type) {
        case TYPEINT:
        case TYPEFLOAT:
            break;
        case TYPEENUM:
            (*expr)->rtype = TYPE_ENUM((*expr)->rtype)->enumtype;
            break;
        default:
            *expr = CExpr_ArithmeticError();
            break;
    }
}

/* A non-int integral operand: an enum takes its underlying type; anything else is an error. */
static inline ENode *CExpr_ForceIntegral(ENode *expr)
{
    if (expr->rtype->type != TYPEENUM)
        return CExpr_ArithmeticError();
    expr->rtype = TYPE_ENUM(expr->rtype)->enumtype;
    return expr;
}

/* The integral promotions: an integral type smaller than int becomes int. */
static inline ENode *CExpr_IntegralPromotion(ENode *expr)
{
    ENode *conv;

    if (expr->rtype->type != TYPEINT)
        expr = CExpr_ForceIntegral(expr);
    if (TYPE_INTEGRAL(expr->rtype)->integral >= IT_INT)
        return expr;
    if (expr->type != EINTCONST) {
        conv = (ENode *)CompilerTools_AllocatePool(26);
        conv->type = ETYPCON;
        conv->cost = expr->cost;
        if (!conv->cost)
            conv->cost = 1;
        conv->flags = expr->flags & ENODE_FLAG_QUALS;
        conv->rtype = expr->rtype;
        conv->data.monadic = expr;
        expr = conv;
    }
    expr->rtype = (Type *)&stsignedint;
    return expr;
}

/* The usual arithmetic conversions of a binary operator's operands. */
void CExpr_ArithmeticConversion(ENode **left, ENode **right)
{
    ENode **temp;

    CExpr_ArithmeticOperand(left);
    CExpr_ArithmeticOperand(right);
    if ((*left)->rtype->type == TYPEFLOAT || (*right)->rtype->type == TYPEFLOAT) {
        if ((*left)->rtype == (*right)->rtype)
            return;
        if (TYPE_INTEGRAL((*left)->rtype)->integral > TYPE_INTEGRAL((*right)->rtype)->integral)
            *right = CExpr2_00473720(*right, (*left)->rtype);
        else
            *left = CExpr2_00473720(*left, (*right)->rtype);
    } else {
        *left = CExpr_IntegralPromotion(*left);
        *right = CExpr_IntegralPromotion(*right);
        if ((*left)->rtype == (*right)->rtype)
            return;
        if (TYPE_INTEGRAL((*left)->rtype)->integral < TYPE_INTEGRAL((*right)->rtype)->integral) {
            temp = left;
            left = right;
            right = temp;
        }
        if ((*left)->rtype->size == (*right)->rtype->size && !Type_IsUnsigned((*left)->rtype) &&
            Type_IsUnsigned((*right)->rtype)) {
            if ((*left)->rtype == (Type *)&stsignedlong)
                *left = CExpr2_00473720(*left, (Type *)&stunsignedlong);
            else {
                if ((*left)->rtype != (Type *)&stsignedlonglong)
                    CError_FATAL(735);
                *left = CExpr2_00473720(*left, (Type *)&stunsignedlonglong);
            }
        }
        *right = CExpr2_00473720(*right, (*left)->rtype);
    }
}

static inline CInt64 convert_integer_constant(Type *type, Type *oldtype, CInt64 value)
{
    if (type == (Type *)&stbool)
        return CMach_CalcIntDiadic(oldtype, value, 0x169, cint64_zero);
    return CMach_CalcIntDiadic(type, value, 0x2b, cint64_zero);
}

ENode *CExpr2_00473720(ENode *expr, Type *type)
{
    if (expr->type == EINTCONST) {
        if (type->type == TYPEFLOAT) {
            expr->type = EFLOATCONST;
            expr->data.floatval = CMach_CalcFloatConvertFromInt(expr->rtype, expr->data.intval);
        } else {
            expr->data.intval = convert_integer_constant(type, expr->rtype, expr->data.intval);
        }
        expr->rtype = type;
        return expr;
    }
    if (expr->type == EFLOATCONST) {
        if (type->type == TYPEFLOAT) {
            expr->data.floatval = CMachine_RoundFloatToType(type, expr->data.floatval);
            expr->rtype = type;
            return expr;
        }
        if (type->type == TYPEINT) {
            expr->data.intval = CMach_CalcIntConvertFromFloat(type, expr->data.floatval.data.value);
            expr->type = EINTCONST;
            expr->rtype = type;
            return expr;
        }
    }
    {
        ENode *node = CompilerTools_AllocatePool(sizeof(ENode));
        node->type = ETYPCON;
        node->cost = expr->cost;
        if (node->cost == 0)
            node->cost = 1;
        node->flags = expr->flags & 3;
        node->rtype = expr->rtype;
        node->data.monadic = expr;
        node->rtype = type;
        return node;
    }
}

CInt64 CExpr_IntConstConvert(Type *type, Type *otherType, CInt64 value)
{
    if (type == (Type *)&stbool)
        return CMach_CalcIntDiadic(otherType, value, 0x169, cint64_zero);
    return CMach_CalcIntDiadic(type, value, 0x2b, cint64_zero);
}

ENode *forceintegral(ENode *node)
{
    UInt8 t;
    Type *type;

    if ((t = (type = node->rtype)->type) != TYPEINT) {
        if (t != TYPEENUM) {
            CError_ReportError(ERR_ILLEGAL_OPERAND);
            node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
            memclrw(node, sizeof(ENode));
            node->type = EINTCONST;
            node->rtype = (Type *)&stsignedlong;
        } else {
            node->rtype = TYPE_ENUM(type)->enumtype;
        }
    }

    if (TYPE_INTEGRAL(node->rtype)->integral >= IT_INT)
        return node;

    if (node->type != EINTCONST) {
        ENode *newnode = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));

        newnode->type = ETYPCON;
        newnode->cost = node->cost;
        if (newnode->cost == 0)
            newnode->cost = 1;
        newnode->flags = node->flags & ENODE_FLAG_QUALS;
        newnode->rtype = node->rtype;
        newnode->data.monadic = node;
        node = newnode;
    }
    node->rtype = (Type *)&stsignedint;

    return node;
}

static ENode *mkInd(ENode *src)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = EINDIRECT;
    n->cost = src->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = (UInt16)(src->flags & 3);
    n->rtype = src->rtype;
    n->data.diadic.left = src;
    return n;
}

static ENode *mkTemp(Type *t)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    n->type = EPRECOMP;
    n->cost = 0;
    n->flags = 0;
    n->rtype = CDecl_NewPointerType(t);
    n->data.temp.type = t;
    n->data.temp.uniqueid = CParser_GetUniqueID();
    n->data.temp.needs_dtor = 0;
    return n;
}

#include <string.h>

ENode *CExpr2_RewriteExprToTemp(ENode *expr)
{
    ENode *temp;
    ENode *tempCopy;
    ENode *target;
    ENode *assignment;
    ENode *result;

    temp = mkTemp(expr->rtype);
    temp->flags = expr->flags;

    tempCopy = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *tempCopy = *temp;

    target = mkInd(tempCopy);
    target->rtype = expr->rtype;

    assignment = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    assignment->type = EASS;
    assignment->rtype = target->rtype;
    assignment->data.diadic.left = target;
    assignment->data.diadic.right = expr;
    if (target->cost != expr->cost) {
        assignment->cost = expr->cost;
        if (target->cost > assignment->cost)
            assignment->cost = target->cost;
    } else {
        assignment->cost = expr->cost + 1;
        if (assignment->cost > 200)
            assignment->cost = 200;
    }
    assignment->flags = (UInt16)((target->flags | expr->flags) & 3);

    assignment->data.diadic.right = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    *assignment->data.diadic.right = *expr;
    *expr = *assignment;

    result = mkInd(temp);
    result->rtype = expr->rtype;

    return result;
}

ENode *CExpr2_NewESCOPEBEGINNode(Type *value, unsigned int withAuxiliary)
{
    SInt32 uniqueID;
    ENode *node;

    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    node->type = EPRECOMP;
    node->cost = 0;
    node->flags = 0;
    node->rtype = CDecl_NewPointerType(value);
    node->data.temp.type = value;
    if ((Boolean)withAuxiliary)
        uniqueID = CParser_GetUniqueID();
    else
        uniqueID = 0;
    node->data.temp.uniqueid = uniqueID;
    node->data.temp.needs_dtor = 0;
    return node;
}

UInt8 CExpr_AllBitsSet(ENode *p)
{
    if (p->type == '2') {
        if (p->rtype->size <= 2) {
            return (0xffff & (unsigned int)p->data.intval.lo) == 0xffff;
        }
        if (p->data.intval.lo != -1) {
            return '\0';
        }
        if (p->rtype->size <= 4) {
            return '\x01';
        }
        return p->data.intval.hi == -1;
    }
    return '\0';
}

UInt8 CExpr_IsOne(ENode *expr)
{
    if (expr->type == EINTCONST) {
        return CInt64_Equal(expr->data.intval, cint64_one);
    }
    if (expr->type == EFLOATCONST) {
        return CMach_FloatIsOne(((ENode *)expr)->data.floatval.data.value);
    }
    return 0;
}

SInt16 isnotzero(ENode *node)
{
    int b;
    ENode *number;
    Object *obj;

    switch (node->type) {
        case EINTCONST:
            return !(Boolean)(node->data.intval.hi == 0 && node->data.intval.lo == 0);
        case EFLOATCONST:
            number = node;
            b = FALSE;
            if (CMach_FloatIsZero(number->data.floatval.data.value) == 0)
                b = TRUE;
            return b;
        case ESTRINGCONST:
        case EPRECOMP:
            return TRUE;
        case EOBJREF:
            obj = node->data.addr.objref;
            break;
        case EADD:
        case ESUB:
            if (node->data.diadic.left->type == EOBJREF && node->data.diadic.right->type == EINTCONST)
                obj = node->data.diadic.left->data.addr.objref;
            else if (node->data.diadic.left->type == EINTCONST && node->data.diadic.right->type == EOBJREF)
                obj = node->data.diadic.right->data.addr.objref;
            else
                return FALSE;
            break;
        default:
            return FALSE;
    }

    switch (obj->datatype) {
        case DLOCAL:
            return TRUE;
        default:
            return FALSE;
    }
}

SInt16 CExpr2_IsZero(ENode *node)
{
    ENode *number;
    switch (node->type) {
        case EINTCONST:
            return (Boolean)(node->data.intval.hi == 0 && node->data.intval.lo == 0);
        case EFLOATCONST:
            number = node;
            return CMach_FloatIsZero(number->data.floatval.data.value);
    }
    return 0;
}

static ENode *MakeDiadic(ENode *left, ENode *right)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));

    n->type = ECOMMA;
    n->rtype = left->rtype;
    n->data.diadic.left = left;
    n->data.diadic.right = right;
    if (left->cost != right->cost) {
        n->cost = right->cost;
        if (left->cost > n->cost)
            n->cost = left->cost;
    } else {
        n->cost = right->cost + 1;
        if (n->cost > 200)
            n->cost = 200;
    }
    n->flags = (left->flags | right->flags) & 3;
    return n;
}

static ENode *MakeMonadic(ENode *inner)
{
    ENode *n = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));

    n->type = EINDIRECT;
    n->cost = inner->cost;
    if (n->cost == 0)
        n->cost = 1;
    n->flags = inner->flags & 3;
    n->rtype = inner->rtype;
    n->data.monadic = inner;
    return n;
}

ENode *makecommaexpression(ENode *a, ENode *b)
{
    ENode *n;

    if (b->type == EINDIRECT && b->data.monadic->type != EBITFIELD) {
        UInt8 save = copts.cplusplus;
        copts.cplusplus = 1;
        n = MakeDiadic(a, getnodeaddress(b, 0));
        copts.cplusplus = save;
        n->rtype = n->data.diadic.right->rtype;
        n = MakeMonadic(n);
    } else {
        n = MakeDiadic(a, b);
    }
    n->rtype = b->rtype;
    n->flags = b->flags;
    return n;
}

ENode *makediadicnode(ENode *left, ENode *right, UInt8 ty)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));

    node->type = ty;
    node->rtype = left->rtype;
    node->data.diadic.left = left;
    node->data.diadic.right = right;
    if (left->cost != right->cost) {
        node->cost = right->cost;
        if (left->cost > node->cost) {
            node->cost = left->cost;
        }
    } else {
        node->cost = right->cost + 1;
        if (node->cost > 200) {
            node->cost = 200;
        }
    }
    node->flags = (UInt16)((left->flags | right->flags) & 3);
    return node;
}

ENode *makemonadicnode(ENode *expr, UInt8 type)
{
    ENode *node;

    node = (ENode *)CompilerTools_AllocatePool(0x1a);
    node->type = type;
    node->cost = expr->cost;
    if (node->cost == 0) {
        node->cost = 1;
    }
    node->flags = expr->flags & ENODE_FLAG_QUALS;
    node->rtype = expr->rtype;
    node->data.monadic = expr;
    return node;
}

ENode *CExpr_ConvertToIntegral(ENode *expr)
{
    ENode *result;
    ENode *error;
    TypeEnum *enumType;
    if (expr->rtype->type != TYPEENUM) {
        CError_ReportError(144U);
        error = (ENode *)CompilerTools_AllocatePool(26U);
        memclrw(error, 26U);
        result = error;
        error->type = 50U;
        error->rtype = (Type *)&stsignedlong;
        return result;
    }
    enumType = (TypeEnum *)expr->rtype;
    expr->rtype = enumType->enumtype;
    result = expr;
    return result;
}

ENode *intconstnode(Type *valueType, SInt32 value)
{
    ENode *expr;

    expr = CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(expr, sizeof(ENode));
    expr->type = EINTCONST;
    expr->rtype = valueType;
    CInt64_SetLong(&expr->data.intval, value);
    return expr;
}

ENode *nullnode(void)
{
    ENode *node;
    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EINTCONST;
    node->rtype = (Type *)&stsignedlong;
    return node;
}

ENode *CExpr2_NewENEWEXCEPTIONARRAYNode(unsigned int value)
{
    ENode *node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = EOBJLIST;
    node->rtype = &data_0055d5c0;
    node->data.templdep.subtype = value;
    return node;
}

ENode *CExpr_NewENode(UInt8 kind)
{
    ENode *node;

    node = (ENode *)CompilerTools_AllocatePool(sizeof(ENode));
    memclrw(node, sizeof(ENode));
    node->type = kind;
    return node;
}

ENode *CExpr2_ReturnNode(ENode *node)
{
    return node;
}

ENode *CExpr2_ReturnENode(ENode *ene)
{
    return ene;
}

/* A reference to an alias: to the object it aliases, plus its offset when it has one. */
void CExpr_AliasTransform(ENode *expr)
{
    Object *obj = expr->data.objref;
    ENode *ref;

    obj->u.alias.member != NULL;
    if (obj->u.alias.offset) {
        ref = create_objectrefnode(obj->u.alias.object);
        *expr = *makediadicnode(ref, intconstnode((Type *)&stunsignedlong, obj->u.alias.offset), EADD);
    } else {
        expr->data.objref = obj->u.alias.object;
    }
}

ENode *replace_expr_tree_nodes(ENode *node)
{
    if (expr_replace_types[node->type] != 0) {
        SInt32 callback = data_0058074c;
        ENode *(*fix)(ENode *) = (ENode * (*)(ENode *)) callback;
        ENode *fixed = fix(node);
        if (fixed == NULL)
            return node;
        node = fixed;
    }

    switch (node->type) {
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
            node->data.monadic = replace_expr_tree_nodes(node->data.monadic);
            return node;

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
            node->data.diadic.left = replace_expr_tree_nodes(node->data.diadic.left);
            node->data.diadic.right = replace_expr_tree_nodes(node->data.diadic.right);
            return node;

        case EINTCONST:
        case EFLOATCONST:
        case ESTRINGCONST:
        case EOBJREF:
        case ENULLCHECK:
        case EPRECOMP:
        case ETEMP:
        case EARGOBJ:
        case ELOCOBJ:
        case ENEWEXCEPTIONARRAY:
        case EMEMBER:
        case EASSBLK:
            return node;

        case EFUNCCALL:
        case EFUNCCALLP: {
            ENodeList *l;
            for (l = node->data.funccall.args; l != NULL; l = l->next)
                l->node = replace_expr_tree_nodes(l->node);
            node->data.funccall.funcref = replace_expr_tree_nodes(node->data.funccall.funcref);
            return node;
        }

        case EMFPOINTER:
            node->data.diadic.left = replace_expr_tree_nodes(node->data.diadic.left);
            node->data.diadic.right = replace_expr_tree_nodes(node->data.diadic.right);
            return node;

        case EQUALNAME:
            node->data.diadic.left = replace_expr_tree_nodes(node->data.diadic.left);
            node->data.diadic.right = replace_expr_tree_nodes(node->data.diadic.right);
            return node;

        case ECOND:
            node->data.cond.cond = replace_expr_tree_nodes(node->data.cond.cond);
            node->data.cond.expr1 = replace_expr_tree_nodes(node->data.cond.expr1);
            node->data.cond.expr2 = replace_expr_tree_nodes(node->data.cond.expr2);
            return node;

        default:
            CError_FATAL(198);
            return NULL;
    }
}

/* 0x580748, callback pointer */
/* 0x5550d8, source file name */

void CExpr_SearchExprTree(ENode *expr, void (*value)(ENode *), SInt32 count, ...)
{
    SInt16 i;
    char *args;
    data_00580748 = value;
    args = (char *)&count + (((SInt32)((char *)&count + 4) - (SInt32)(char *)&count) + 3) / 4 * 4;
    for (i = 0; i < 75; i++)
        expr_search_types[i] = 0;
    for (i = 0; i < count; i++)
        expr_search_types[*(SInt32 *)((args += 4) - 4)] = 1;
    CExpr2_004743d0(expr);
}

/* Recursive expression-tree walker. */
void CExpr2_004743d0(ENode *e)
{
    ENodeList *list;

    for (;;) {
        if (expr_search_types[e->type] != 0)
            (*data_00580748)(e);
        switch (e->type) {
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
                e = e->data.monadic;
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
                CExpr2_004743d0(e->data.diadic.left);
                e = e->data.diadic.right;
                break;
            case EINTCONST:
            case EFLOATCONST:
            case ESTRINGCONST:
            case EOBJREF:
            case ENULLCHECK:
            case EPRECOMP:
            case ETEMP:
            case EARGOBJ:
            case ELOCOBJ:
            case ENEWEXCEPTION:
            case ENEWEXCEPTIONARRAY:
            case EMEMBER:
            case EASSBLK:
                return;
            case EFUNCCALL:
            case EFUNCCALLP:
                for (list = e->data.funccall.args; list != NULL; list = list->next)
                    CExpr2_004743d0(list->node);
                e = e->data.funccall.funcref;
                break;
            case EMFPOINTER:
                CExpr2_004743d0(e->data.monadic);
                e = e->data.diadic.right;
                break;
            case EQUALNAME:
                CExpr2_004743d0(e->data.monadic);
                e = e->data.diadic.right;
                break;
            case ECOND:
                CExpr2_004743d0(e->data.cond.cond);
                CExpr2_004743d0(e->data.cond.expr1);
                e = e->data.cond.expr2;
                break;
            default:
                CError_FATAL(106);
                return;
        }
    }
}
