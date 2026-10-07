#define CERROR_FILE "CObjC.c"
#include "compiler/common.h"
#include "compiler/CObjC.h"
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
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
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
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"

#include <string.h>
#include <stdio.h>

#define MATCH_BODY()                                                                                                   \
    do {                                                                                                               \
        MessageArgument *matchedArgument;                                                                              \
        ObjCParameterNode *matchedParameter;                                                                           \
        ENodeList *extra;                                                                                              \
        extra = extraArgs;                                                                                             \
        if (arguments->expression != 0) {                                                                              \
            matchedParameter = method->args;                                                                           \
            matchedArgument = arguments;                                                                               \
            for (;;) {                                                                                                 \
                matchedArgument->expression = CExpr_AssignmentPromotion(                                               \
                    matchedArgument->expression, matchedParameter->type, matchedParameter->qual, 1);                   \
                matchedArgument = matchedArgument->next;                                                               \
                if (matchedArgument == 0)                                                                              \
                    break;                                                                                             \
                matchedParameter = matchedParameter->next;                                                             \
                if (matchedParameter == 0)                                                                             \
                    CError_FATAL(2998);                                                                                \
            }                                                                                                          \
        }                                                                                                              \
        while (extra != 0) {                                                                                           \
            extra->node = CExpr_VarArgPromotion(extra->node, 1);                                                       \
            extra = extra->next;                                                                                       \
        }                                                                                                              \
    } while (0)

static inline Type *FindNamedPointerType(char *name, Boolean required)
{
    NameSpaceObjectList *node;
    Type *t;

    node = CScope_FindName(registration_context, GetHashNameNode(name));
    if (node != NULL && node->object->otype == OT_TYPE) {
        if ((t = OBJ_TYPE(node->object)->type)->type == TYPEPOINTER)
            return t;
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNEXPECTED_TYPE, name);
    } else if (required) {
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNDEFINED_SHOULD_DEFINED, name);
    }
    return NULL;
}

static inline Type *GetSelType(Boolean required)
{
    Type *t;

    if (sel_type)
        return sel_type;
    if ((t = FindNamedPointerType("SEL", required)) == NULL)
        return (Type *)&void_ptr;
    return sel_type = t;
}

static inline CRec *FindProtocol(HashNameNode *name)
{
    CRec *p;

    for (p = data_00588064; p; p = p->next) {
        if (p->name == name)
            break;
    }
    return p;
}

ENode *CDecl_ParseSelectorExpression(void)
{
    HashNameNode *name;
    HashEntry *entry;
    HashEntry **slot;
    ENode *node;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        data_00583548.size = 0;
        for (;;) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_IDENTIFIER) {
                CompilerTools_AppendGListString(&data_00583548, data_00587fa0->name);
                tk = CPrepTokenizer_GetNextToken();
            }
            if (tk == ')') {
                if (data_00583548.size == 0)
                    CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                tk = CPrepTokenizer_GetNextToken();
                break;
            }
            if (tk == ':') {
                AppendGListByte(&data_00583548, ':');
            } else {
                CError_ReportError(ERR_RPAREN_EXPECTED);
                break;
            }
        }
        AppendGListByte(&data_00583548, 0);
        fn_00443190(data_00583548.data);
        name = GetHashNameNode(*data_00583548.data);
        fn_004431b0(data_00583548.data);

        if (selector_hash == NULL)
            entry = NULL;
        else {
            entry = selector_hash[name->hashval & 0x3ff];
            while (entry != NULL) {
                if (entry->name == name)
                    break;
                entry = entry->next;
            }
        }
        if (entry == NULL) {
            if (selector_hash == NULL) {
                selector_hash = galloc(1024 * sizeof(*selector_hash));
                memclrw(selector_hash, 1024 * sizeof(*selector_hash));
            }
            entry = galloc(sizeof(*entry));
            entry->obj = NULL;
            entry->name = name;
            entry->methods = NULL;
            slot = &selector_hash[name->hashval & 0x3ff];
            entry->next = *slot;
            *slot = entry;
        }
        node = create_objectnode(CObjCModern_GetSelectorReference(entry));
        node->rtype = GetSelType(1);
        return node;
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return nullnode();
    }
}

ENode *CDecl_ParseProtocolExpression(void)
{
    CRec *proto;
    ENode *expr;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == TK_IDENTIFIER) {
            if ((proto = FindProtocol(data_00587fa0))) {
                expr = create_objectrefnode(CObjC_GetProtocolInfo(proto));
                tk = CPrepTokenizer_GetNextToken();
                if (tk != ')')
                    CError_ReportError(ERR_RPAREN_EXPECTED);
                else
                    tk = CPrepTokenizer_GetNextToken();
                return expr;
            }
        } else {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        }
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
    }
    return nullnode();
}

static inline TypeClass *fn_00504c90_inline1(void)
{
    HashNameNode *v0;
    NameSpaceObjectList *v1;
    TypeClass *v3;
    v0 = GetHashNameNode("NSConstantString");
    v1 = CScope_FindName(registration_context, v0);
    if ((int)v1 != 0 && ((ObjType *)v1->object)->otype == 1) {
        if ((v3 = (TypeClass *)((ObjType *)v1->object)->type)->type == 5 && v3->objcinfo != NULL) {
            return v3;
        }
    }
    CError_ReportError(ERR_NOT_OBJECTIVE_C_CLASS, v0->name);
    return NULL;
}

ENode *CObjC_ParseStringConstant(void)
{
    TypeClass *type;
    ENode *result;
    Object *object;
    OLinkList *entries;
    OLinkList *tail;
    OLinkList *entry;
    UInt32 values[3];
    char name[16];
    tk = CPrepTokenizer_GetNextToken();
    if (tk == TK_STRING) {
        if ((type = fn_00504c90_inline1()) != NULL) {
            values[0] = CTool_EndianConvertWord32(0);
            tail = entry = (OLinkList *)CompilerTools_AllocatePool(16);
            entry->next = NULL;
            entry->obj = type->objcinfo->classobject;
            entry->offset = 0;
            entry->addend = 0;
            values[1] = CTool_EndianConvertWord32(0);
            entries = (OLinkList *)CompilerTools_AllocatePool(16);
            entries->next = tail;
            entries->obj = CInit_DeclareString(string_token_data, token_value_kind_or_string_length, 0, 0);
            entries->offset = 4;
            entries->addend = 0;
            values[2] = CTool_EndianConvertWord32(token_value_kind_or_string_length - 1);
            sprintf(name, "%ld", objc_string_constant_count++);
            object = CParser_NewCompilerDefDataObject();
            object->name = CParser_NameConcat("L_NSConstantString_", name);
            object->type = CDecl_NewStructType(TYPEARRAY, 4);
            CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
            object->type = (Type *)type;
            object->sclass = TK_STATIC;
            object->section = 10;
            fn_004ceab0(object, values, entries, object->type->size);
            result = create_objectrefnode(object);
        } else {
            result = nullnode();
        }
        tk = CPrepTokenizer_GetNextToken();
    } else {
        CError_ReportError(ERR_ILLEGAL_STRING_CONSTANT);
        result = nullnode();
    }
    return result;
}

ENode *CObjC_ParseEncodeExpression(void)
{
    DeclInfo decl;
    ENode *stringNode;
    ENode *indirectNode;
    char *encoding;
    char **encodingHandle;
    Type *type;
    UInt32 qualifiers;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != '(') {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return nullnode();
    }
    tk = CPrepTokenizer_GetNextToken();
    memclrw(&decl, sizeof(decl));
    CParser_GetDeclSpecs(&decl, 0);
    CDecl_ParseDeclarator(&decl);
    if (tk != ')') {
        CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
    } else {
        tk = CPrepTokenizer_GetNextToken();
    }
    if (decl.name != NULL) {
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
    }
    if (decl.parserOption != 0) {
        type = CObjC_GetIdType(1);
    } else {
        type = decl.thetype;
    }
    qualifiers = decl.qual;
    data_00583548.size = 0;
    CObjC_005074f0(type, qualifiers, 1);
    AppendGListByte(&data_00583548, 0);
    encoding = galloc(data_00583548.size);
    encodingHandle = data_00583548.data;
    memcpy(encoding, *encodingHandle, data_00583548.size);
    stringNode = CompilerTools_AllocatePool(sizeof(ENode));
    stringNode->type = ESTRINGCONST;
    stringNode->cost = 0;
    stringNode->flags = 0;
    stringNode->rtype = CDecl_NewArrayType((Type *)&stchar, strlen(encoding) + 1);
    stringNode->data.string.size = stringNode->rtype->size;
    stringNode->data.string.data = encoding;
    stringNode->data.string.useExplicitSize = 0;
    indirectNode = makemonadicnode(stringNode, EINDIRECT);
    indirectNode->data.monadic->rtype = CDecl_NewPointerType(indirectNode->rtype);
    return indirectNode;
}

static inline Type *FindNamedPointerType_504fb0(char *name, Boolean required)
{
    NameSpaceObjectList *node;
    Type *t;

    node = CScope_FindName(registration_context, GetHashNameNode(name));
    if (node != NULL && node->object->otype == OT_TYPE) {
        if ((t = OBJ_TYPE(node->object)->type)->type == TYPEPOINTER)
            return t;
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNEXPECTED_TYPE, name);
    } else if (required) {
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNDEFINED_SHOULD_DEFINED, name);
    }
    return NULL;
}

static inline Type *GetIdType_504fb0(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return id_type;
    if ((t = FindNamedPointerType_504fb0("id", required)) == NULL)
        return (Type *)&void_ptr;
    return id_type = t;
}

static inline Type *GetClassType_504fb0(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return class_pointer_type;
    if ((t = FindNamedPointerType_504fb0("Class", required)) == NULL)
        return (Type *)&void_ptr;
    return class_pointer_type = t;
}

static inline Boolean IsIdType_504fb0(Type *ty)
{
    if (ty->type == TYPEPOINTER) {
        if (&ty->type == &void_ptr.type)
            return 0;
        ty = TPTR_TARGET(ty);
        if (ty == TPTR_TARGET(GetIdType_504fb0(0)))
            return 1;
        if (ty == TPTR_TARGET(GetClassType_504fb0(0)))
            return 1;
    }
    return 0;
}

ENode *CObjC_ParseMessageExpression(void)
{
    Boolean isSuper;
    TypeClass *receiverClass;
    ENodeList *extraArguments;
    ENode *receiver;
    MessageArgument *arguments;
    MessageArgument *argument;
    Boolean receiverMode;
    NameSpaceObjectList *found;

    isSuper = 0;
    receiverClass = NULL;
    tk = CPrepTokenizer_GetNextToken();
    switch (tk) {
        case TK_IDENTIFIER:
            if (memcmp(data_00587fa0->name, "super", 6) == 0) {
                case 0x181:
                    receiver = CClass_CreateThisSelfExpr();
                    if (receiver == NULL || data_00588040->bases == NULL) {
                        CError_ReportError(ERR_ILLEGAL_USE_SUPER);
                        receiver = nullnode();
                    } else {
                        receiverClass = data_00588040->bases->base;
                        isSuper = 1;
                    }
                    receiverMode = !data_005884f8 ? (Boolean)1 : (Boolean)0;
                    tk = CPrepTokenizer_GetNextToken();
                    break;
            }
            found = CScope_FindName(registration_context, data_00587fa0);
            if (found != NULL && OBJ_BASE(found->object)->otype == OT_TYPE &&
                (receiverClass = TYPE_CLASS(OBJ_TYPE(found->object)->type))->type == TYPECLASS &&
                receiverClass->objcinfo != NULL) {
                receiverMode = 1;
                receiver = create_objectrefnode(receiverClass->objcinfo->classobject);
                tk = CPrepTokenizer_GetNextToken();
                break;
            }
            receiverClass = NULL;
        default:
            receiver = s_expression();
            if (IsIdType_504fb0(receiver->rtype)) {
                receiverMode = 2;
            } else {
                if (receiver->rtype->type != TYPEPOINTER || TPTR_TARGET(receiver->rtype)->type != TYPECLASS ||
                    TYPE_CLASS(TPTR_TARGET(receiver->rtype))->objcinfo == NULL) {
                    CError_ReportError(ERR_ILLEGAL_MESSAGE_RECEIVER);
                    receiver = nullnode();
                    receiverMode = 2;
                } else {
                    if (data_00588040 == TYPE_CLASS(TPTR_TARGET(receiver->rtype)) && data_005884f8 == 0 &&
                        receiver->type == EINDIRECT && receiver->data.monadic->type == EOBJREF &&
                        receiver->data.monadic->data.objref->name == this_self_name)
                        receiverMode = 1;
                    else
                        receiverMode = 0;
                    receiverClass = TYPE_CLASS(TPTR_TARGET(receiver->rtype));
                }
            }
            break;
    }
    argument = CompilerTools_AllocatePool(sizeof(MessageArgument));
    memclrw(argument, sizeof(MessageArgument));
    arguments = argument;
    extraArguments = NULL;
    for (;;) {
        CObjC_ConvertKeywordToIdentifier();
        if (tk == TK_IDENTIFIER) {
            argument->name = data_00587fa0;
            tk = CPrepTokenizer_GetNextToken();
            if (tk == ']' && arguments == argument)
                break;
        }
        if (tk != ':') {
            CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
            return nullnode();
        }
        tk = CPrepTokenizer_GetNextToken();
        argument->expression = assignment_expression();
        if (tk == ']')
            break;
        if (tk == ',') {
            tk = CPrepTokenizer_GetNextToken();
            extraArguments = CExpr_ScanExpressionList(0);
            break;
        }
        argument->next = CompilerTools_AllocatePool(sizeof(MessageArgument));
        memclrw(argument->next, sizeof(MessageArgument));
        argument = argument->next;
    }
    {
        ENode *result =
            CObjC_MakeMessageSend(receiver, receiverClass, arguments, extraArguments, receiverMode, isSuper);
        tk = CPrepTokenizer_GetNextToken();
        return result;
    }
}

ENode *CObjC_MakeMessageSend(ENode *receiver, TypeClass *obj, MessageArgument *arguments, ENodeList *extraArgs,
                             UInt8 mode, Boolean flag)
{
    ENodeList *extra;
    TypeClass *currentClass;
    CRec *entry;
    HashEntry *selector;
    SelectorMethod *candidate;
    TypeFunc *functionType;
    Object *callee;
    Boolean resultKind;
    ENode *temporary;
    ENode *expression;
    ENode *call;
    MessageArgument *argument;
    ENodeList *firstArg;
    ENodeList *tail;
    MethRec *method;
    MessageArgument *matchedArgument;
    ObjCParameterNode *matchedParameter;
    ObjectList *protocol;

    method = NULL;
    if (obj != NULL) {
        currentClass = obj;
        for (;;) {
            CError_ASSERT(3106, currentClass->objcinfo != 0);
            for (entry = ((ObjCInfo *)currentClass->objcinfo)->vars; entry != NULL; entry = entry->next) {
                for (method = entry->methods; method != NULL; method = method->next) {
                    switch (mode) {
                        case 0:
                            if (method->isinst != 0)
                                continue;
                            break;
                        case 1:
                            if (method->isinst == 0)
                                continue;
                            break;
                        case 2:
                            break;
                    }
                    if (match_message_arguments(method, arguments, extraArgs != NULL)) {
                        MATCH_BODY();
                        selector = CObjCModern_RegisterMethodSelector(method);
                        break;
                    }
                }
                if (method != NULL)
                    break;
            }
            if (method != NULL)
                break;
            method = ((ObjCInfo *)currentClass->objcinfo)->methods;
            if (method != NULL) {
                do {
                    if (match_message_arguments(method, arguments, extraArgs != NULL)) {
                        MATCH_BODY();
                        selector = CObjCModern_RegisterMethodSelector(method);
                        break;
                    }
                    method = method->next;
                } while (method != NULL);
            }
            if (method != NULL)
                break;
            if (currentClass->bases == NULL) {
                CError_Warning(ERR_RECEIVER_CANNOT_HANDLE_MESSAGE);
                break;
            }
            currentClass = currentClass->bases->base;
        }
    }
    if (method == NULL) {
        if (receiver->rtype->type == TYPEPOINTER && (((TypePointer *)receiver->rtype)->qual & Q_IS_OBJC_ID) != 0) {
            for (protocol = ((TypePointer *)receiver->rtype)->protocols[0]; protocol != NULL;
                 protocol = protocol->next) {
                method = ((CRec *)protocol->object)->methods;
                if (method != NULL) {
                    do {
                        if (match_message_arguments(method, arguments, extraArgs != NULL)) {
                            MATCH_BODY();
                            selector = CObjCModern_RegisterMethodSelector(method);
                            break;
                        }
                        method = method->next;
                    } while (method != NULL);
                }
            }
            if (method == NULL)
                CError_Warning(ERR_RECEIVER_CANNOT_HANDLE_MESSAGE);
        }
        if (method == NULL) {
            selector = CObjCModern_FindMessageArgumentHashEntry(arguments);
            if (selector != NULL) {
                for (candidate = selector->methods; candidate != NULL; candidate = candidate->next) {
                    if (candidate->method->isinst == 0) {
                        if (method != NULL)
                            break;
                        method = candidate->method;
                    }
                }
                if (method == NULL) {
                    for (candidate = selector->methods; candidate != NULL; candidate = candidate->next) {
                        if (candidate->method->isinst != 0) {
                            if (method != NULL)
                                break;
                            method = candidate->method;
                        }
                    }
                }
                if (method != NULL) {
                    if (candidate != NULL)
                        CError_Warning(ERR_AMBIGUOUS_MESSAGE_SELECTOR_USED_ALSO_HAD, method, candidate->method);
                    if (match_message_arguments(method, arguments, extraArgs != NULL)) {
                        MATCH_BODY();
                    } else {
                        CError_Warning(ERR_RECEIVER_CANNOT_HANDLE_MESSAGE);
                    }
                } else {
                    CError_ReportError(ERR_UNKNOWN_MESSAGE_SELECTOR);
                }
            } else {
                CError_ReportError(ERR_UNKNOWN_MESSAGE_SELECTOR);
            }
        }
    }
    if (method != NULL) {
        functionType = get_method_ftype(method);
        resultKind = CMachine_FunctionRequiresMemoryReturn(functionType) != 0;
        if (flag != 0) {
            if (resultKind)
                callee = CObjCModern_GetOrCreateFunctionObject("objc_msgSendSuper_stret", "objc_msgSendSuper_stret");
            else
                callee = CObjCModern_GetOrCreateFunctionObject("objc_msgSendSuper", "objc_msgSendSuper");
        } else {
            if (resultKind)
                callee = CObjCModern_GetOrCreateFunctionObject("objc_msgSend_stret", "objc_msgSend_stret");
            else
                callee = CObjCModern_GetOrCreateFunctionObject("objc_msgSend", "objc_msgSend");
        }
        if (flag != 0) {
            temporary = CExpr2_NewESCOPEBEGINNode(CDecl_NewStructType(8, 4), 1);
            expression = makemonadicnode(temporary, EINDIRECT);
            expression->rtype = (Type *)&void_ptr;
            receiver = makediadicnode(expression, receiver, EASS);
            expression = CompilerTools_AllocatePool(sizeof(ENode));
            *expression = *temporary;
            expression = makediadicnode(expression, intconstnode((Type *)&stunsignedlong, 4), EADD);
            expression = makemonadicnode(expression, EINDIRECT);
            expression->rtype = (Type *)&void_ptr;
            expression =
                makediadicnode(expression, create_objectrefnode(((ObjCInfo *)obj->objcinfo)->metaclassobject), EASS);
            receiver = makediadicnode(receiver, expression, ECOMMA);
            expression = CompilerTools_AllocatePool(sizeof(ENode));
            *expression = *temporary;
            receiver = makediadicnode(receiver, expression, ECOMMA);
            receiver->rtype = temporary->rtype;
        }
        call = CompilerTools_AllocatePool(sizeof(ENode));
        call->type = EFUNCCALL;
        call->cost = 0xc8;
        call->rtype = method->rtype;
        call->flags = (UInt16)method->rqual & ENODE_FLAG_QUALS;
        call->data.funccall.funcref = create_objectrefnode(callee);
        call->data.funccall.functype = functionType;
        CError_ASSERT(3280, call->data.funccall.functype->type == TYPEFUNC);
        firstArg = CompilerTools_AllocatePool(sizeof(ENodeList));
        call->data.funccall.args = firstArg;
        firstArg->node = receiver;
        tail = firstArg->next = CompilerTools_AllocatePool(sizeof(ENodeList));
        tail->node = create_objectnode(CObjCModern_GetSelectorReference(selector));
        for (argument = arguments; argument != NULL; argument = argument->next) {
            if (argument->expression != NULL) {
                ENodeList *nextArg = CompilerTools_AllocatePool(sizeof(ENodeList));
                tail->next = nextArg;
                tail = tail->next;
                tail->node = argument->expression;
            }
        }
        tail->next = extraArgs;
        expression = CExpr_AdjustFunctionCall(call);
    } else {
        expression = nullnode();
    }
    return expression;
}

#undef MATCH_BODY
#define MATCH_BODY()                                                                                                   \
    do {                                                                                                               \
        Statement *mlp;                                                                                                \
        ObjCParameterNode *map;                                                                                        \
        Statement *p;                                                                                                  \
        p = a;                                                                                                         \
        if (l->f08 != 0) {                                                                                             \
            map = ob->f18;                                                                                             \
            mlp = l;                                                                                                   \
            for (;;) {                                                                                                 \
                mlp->f08 = CExpr_AssignmentPromotion(mlp->f08, map->f0c, map->f10, 1);                                 \
                mlp = mlp->next;                                                                                       \
                if (mlp == 0)                                                                                          \
                    break;                                                                                             \
                map = map->next;                                                                                       \
                if (map == 0)                                                                                          \
                    CError_FATAL(2998);                                                                                \
            }                                                                                                          \
        }                                                                                                              \
        while (p != 0) {                                                                                               \
            p->f04 = CExpr_VarArgPromotion(p->f04, 1);                                                                 \
            p = p->next;                                                                                               \
        }                                                                                                              \
    } while (0)

Boolean match_message_arguments(register MethRec *info, MessageArgument *b, Boolean flag)
{
    ObjCParameterNode *p;
    MessageArgument *bb;

    if (flag && info->isvararg == 0)
        return 0;
    p = (ObjCParameterNode *)info->args;
    bb = b;
    for (;;) {
        if (p->selectorName != bb->name)
            return 0;
        if (p->type == NULL) {
            if (bb->expression != NULL)
                return 0;
        }
        if (p->type != NULL) {
            if (bb->expression == NULL)
                return 0;
        }
        bb = bb->next;
        if (bb == NULL) {
            if (p->next != NULL)
                return 0;
            return 1;
        }
        p = p->next;
        if (p == NULL)
            return 0;
    }
}

void CObjC_ParseIdentifierList(void)

{
    do {
        tk = CPrepTokenizer_GetNextToken();
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return;
        }
        find_or_create_objc_class(data_00587fa0);
        tk = CPrepTokenizer_GetNextToken();
    } while (tk == ',');
    if (tk != ';') {
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
    }
    return;
}

/* 0x492070, returns the current token */
/* 0x5882d8, current token */

static inline CRec *FindRec(HashNameNode *nm)
{
    CRec *q;

    for (q = data_00588064; q != NULL; q = q->next) {
        if (q->name == nm)
            break;
    }
    return q;
}

static inline void CopyInheritedItem(MethRec *dest, MethRec *src)
{
    dest->selector = src->selector;
    dest->rtype = src->rtype;
    dest->rqual = src->rqual;
    dest->args = src->args;
    dest->isvararg = src->isvararg;
    dest->isinst = src->isinst;
}

void CObjC_ParseProtocol(void)
{
    MethRec *item;
    MethRec *newItem;
    ObjectList *base;
    CRec *rec;
    CRec *protocol;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    if ((rec = FindRec(data_00587fa0)) != NULL)
        CError_ReportError(ERR_PROTOCOL_REDEFINED, rec->name->name);

    rec = (CRec *)galloc(sizeof(CRec));
    memclrw(rec, sizeof(CRec));
    rec->name = data_00587fa0;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '<') {
        rec->bases = parse_protocol_list();
        for (base = rec->bases; base != NULL; base = base->next) {
            protocol = (CRec *)base->object; /* parse_protocol_list stores protocol records in ObjectList. */
            for (item = protocol->methods; item != NULL; item = item->next) {
                if (fn_00508940(rec->methods, item, 1, 1) == NULL) {
                    newItem = (MethRec *)galloc(sizeof(MethRec));
                    memclrw(newItem, sizeof(MethRec));
                    CopyInheritedItem(newItem, item);
                    newItem->defined = 0;
                    newItem->next = rec->methods;
                    rec->methods = newItem;
                }
            }
        }
    }
    rec->next = data_00588064;
    data_00588064 = rec;

    for (;;) {
        switch (tk) {
            case TK_AT_END:
                break;
            case '+':
            case '-':
                newItem = parse_method_declaration(1);
                if (newItem != NULL) {
                    if (fn_00508940(rec->methods, newItem, 0, 1) == NULL) {
                        newItem->next = rec->methods;
                        rec->methods = newItem;
                        (void)CObjCModern_RegisterMethodSelector(newItem);
                    } else {
                        CError_ReportError(ERR_METHOD_REDECLARED, newItem);
                    }
                }
                if (tk != ';') {
                    CError_ReportError(ERR_SEMICOLON_EXPECTED);
                } else {
                    tk = CPrepTokenizer_GetNextToken();
                }
                continue;
            default:
                CParser_ParseGlobalDeclaration();
                continue;
        }
        break;
    }
}

void fn_00505cb0(void)

{
    parse_class_interface_or_implementation();
    return;
}

void fn_00505cc0(void)

{
    parse_class_interface_or_implementation();
    return;
}

static inline TypeClass *CObjC_FindClass(NameSpaceObjectList *nsol, HashNameNode *nm)
{
    TypeClass *theclass;

    if (nsol == NULL || OBJ_BASE(nsol->object)->otype != OT_TYPE ||
        (theclass = (TypeClass *)OBJ_TYPE(nsol->object)->type)->type != TYPECLASS || theclass->objcinfo == NULL) {
        CError_ReportError(ERR_NOT_OBJECTIVE_C_CLASS, nm->name);
        theclass = NULL;
    }
    return theclass;
}

static inline Boolean CObjC_SameMemberList(ObjectList *a, ObjectList *b)
{
    Boolean eq;

    for (;;) {
        if (a == NULL) {
            eq = (b == NULL);
            break;
        }
        if (b == NULL || a->object != b->object) {
            eq = 0;
            break;
        }
        a = a->next;
        b = b->next;
    }
    return eq;
}

static inline MethRec *CObjC_NewMemberNode(MethRec *q)
{
    MethRec *n = (MethRec *)galloc(0x20);
    memclrw(n, 0x20);
    n->selector = q->selector;
    n->rtype = q->rtype;
    n->rqual = q->rqual;
    n->args = q->args;
    n->isvararg = q->isvararg;
    n->isinst = q->isinst;
    n->defined = 0;
    return n;
}

static inline void CObjC_PrependMember(TypeClass *cls, MethRec *member)
{
    member->next = cls->objcinfo->methods;
    cls->objcinfo->methods = member;
}

void parse_class_interface_or_implementation(void)
{
    Boolean isComplete;
    ClassLayout layout;
    MethRec *member;
    TypeClass *objcClass;
    ObjectList *protocol;
    Boolean isInterface;

    isInterface = (tk == TK_AT_INTERFACE);
    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    objcClass = find_or_create_objc_class(data_00587fa0);
    if (objcClass == NULL)
        return;
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        if (isInterface)
            parse_category(objcClass);
        else
            parse_category_methods_and_check_defined(objcClass);
        return;
    }
    isComplete = (objcClass->flags & CLASS_COMPLETED) != 0;
    if (isComplete && isInterface) {
        CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, objcClass->classname->name);
        return;
    }
    if (tk == ':') {
        HashNameNode *name;
        NameSpaceObjectList *lookup;
        ClassList *base;
        TypeClass *superclass;

        tk = CPrepTokenizer_GetNextToken();
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return;
        }
        name = data_00587fa0;
        lookup = CScope_FindName(registration_context, name);
        if ((superclass = CObjC_FindClass(lookup, name)) != NULL) {
            if (superclass->flags & CLASS_COMPLETED) {
                if (!isComplete) {
                    base = (ClassList *)galloc(sizeof(*base));
                    memclrw(base, sizeof(*base));
                    base->base = superclass;
                    base->access = ACCESSPUBLIC;
                    objcClass->bases = base;
                } else {
                    if (objcClass->bases == NULL || objcClass->bases->base != superclass)
                        CError_ReportError(ERR_SUPER_CLASS_DOES_NOT_MATCH_INTERFACE);
                }
            } else {
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, superclass, 0);
            }
        }
        tk = CPrepTokenizer_GetNextToken();
    }
    if (tk == '<') {
        protocol = parse_protocol_list();
        if (isComplete) {
            if (!CObjC_SameMemberList(protocol, objcClass->objcinfo->protocols))
                CError_ReportError(ERR_PROTOCOL_LIST_DOES_NOT_MATCH_INTERFACE);
        } else {
            MethRec *protocolMethod;
            MethRec **methods;
            objcClass->objcinfo->protocols = protocol;
            for (protocol = objcClass->objcinfo->protocols, methods = &objcClass->objcinfo->methods; protocol != NULL;
                 protocol = protocol->next) {
                for (protocolMethod = ((CRec *)protocol->object)->methods; protocolMethod != NULL;
                     protocolMethod = protocolMethod->next) {
                    if (fn_00508940(*methods, protocolMethod, 1, 1) == NULL) {
                        MethRec *newMethod = CObjC_NewMemberNode(protocolMethod);
                        newMethod->next = *methods;
                        *methods = newMethod;
                        if (objcClass != NULL)
                            CObjCModern_RegisterMethodSelector(newMethod);
                    }
                }
            }
        }
    }
    if (tk == '{')
        parse_ivars(objcClass, isComplete);
    if (!isComplete) {
        memclrw(&layout, 0xe);
        CABI_LayoutClass(&layout, objcClass);
    }
    data_00587140 = objcClass;
    for (;;) {
        switch (tk) {
            case TK_AT_END:
                break;
            case '+':
            case '-':
                if (isInterface) {
                    MethRec *method = parse_method_declaration(1);
                    if (method != NULL) {
                        MethRec *existing;
                        existing = fn_00508940(objcClass->objcinfo->methods, method, 0, 1);
                        if (existing == NULL) {
                            CObjC_PrependMember(objcClass, method);
                            (void)CObjCModern_RegisterMethodSelector(method);
                        } else if (copts.f5d != 0 || CObjCModern_CompareMethRecs(method, existing) == 0) {
                            CError_ReportError(ERR_METHOD_REDECLARED, method);
                        }
                    }
                    if (tk != ';')
                        CError_ReportError(ERR_SEMICOLON_EXPECTED);
                    else
                        tk = CPrepTokenizer_GetNextToken();
                } else {
                    parse_method_definition(objcClass, NULL, &objcClass->objcinfo->methods);
                }
                continue;
            default:
                CParser_ParseGlobalDeclaration();
                continue;
        }
        break;
    }
    data_00587140 = NULL;
    if (!isInterface) {
        for (member = objcClass->objcinfo->methods; member != NULL; member = member->next) {
            if (member->defined == 0)
                CError_Warning(ERR_METHOD_NOT_DEFINED, member);
        }
        emit_classobject_and_metaclassobject(objcClass);
        {
            struct PrecTypeEntry *link = galloc(sizeof(*link));
            link->next = class_type_entries;
            link->type = objcClass;
            class_type_entries = link;
        }
    }
}

void parse_ivars(TypeClass *classType, char checkExisting)
{
    char access;
    ObjMemberVar *member;
    ObjMemberVar *newMember;
    ObjMemberVar *existing;
    char matches;
    BigDeclInfo declaration;
    ObjMemberVar *members;

    tk = CPrepTokenizer_GetNextToken();
    members = NULL;
    access = 2;
    for (;; tk = CPrepTokenizer_GetNextToken()) {
        if (tk == '}') {
            tk = CPrepTokenizer_GetNextToken();
            break;
        }
        switch (tk) {
            case TK_AT_PRIVATE:
                access = 1;
                continue;
            case TK_AT_PROTECTED:
                access = 2;
                continue;
            case TK_AT_PUBLIC:
                access = 0;
                continue;
            default:
                memclrw(&declaration, sizeof(declaration));
                CParser_GetDeclSpecs(&declaration.declinfo, 0);
                if (declaration.declinfo.storageclass != 0 || declaration.declinfo.hasParameterNames != 0) {
                    CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                    return;
                }
                if (tk != ';') {
                    for (;;) {
                        CDecl_ScanStructDeclarator(&declaration);
                        if (CDecl_CheckObjectType(declaration.declinfo2.thetype) == 0) {
                            CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                            declaration.valid = 0;
                        }
                        if (declaration.declinfo2.operator_token != 0) {
                            CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                            declaration.valid = 0;
                        }
                        if (declaration.declinfo.missingTypeSpecifier != 0) {
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                        }
                        if (declaration.valid != 0) {
                            member = members;
                            while (member != NULL) {
                                if (member->name == declaration.declinfo2.name) {
                                    break;
                                }
                                member = member->next;
                            }
                            if (member != NULL || declaration.declinfo2.name == unnamed_name) {
                                CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED,
                                                   declaration.declinfo2.name->name);
                            } else {
                                newMember = (ObjMemberVar *)galloc(sizeof(ObjMemberVar));
                                memclrw(newMember, sizeof(*newMember));
                                newMember->otype = OT_MEMBERVAR;
                                newMember->access = access;
                                newMember->type = declaration.declinfo2.thetype;
                                newMember->name = declaration.declinfo2.name;
                                newMember->qual = declaration.declinfo2.qual;
                                if ((member = members) != NULL) {
                                    while (member->next != NULL) {
                                        member = member->next;
                                    }
                                    member->next = newMember;
                                } else {
                                    members = newMember;
                                }
                                if (checkExisting == 0) {
                                    CScope_AddObject(classType->nspace, newMember->name, (ObjBase *)newMember);
                                }
                            }
                        }
                        if (tk != ',') {
                            break;
                        }
                        tk = CPrepTokenizer_GetNextToken();
                    }
                }
                if (tk == ';') {
                    continue;
                }
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
                break;
        }
        break;
    }
    if (checkExisting != 0) {
        existing = classType->ivars;
        for (;;) {
            if (existing == NULL) {
                matches = members == NULL;
                break;
            }
            if (members == NULL || existing->name != members->name || existing->qual != members->qual ||
                existing->access != members->access || iscpp_typeequal(existing->type, members->type) == 0) {
                matches = 0;
                break;
            }
            existing = existing->next;
            members = members->next;
        }
        if (matches == 0) {
            CError_ReportError(ERR_INSTANCE_VARIABLE_LIST_DOES_NOT_MATCH);
        }
    } else {
        classType->ivars = members;
    }
}

#include <stddef.h>

static struct ObjCDefinition *category_definitions;

void parse_category_methods_and_check_defined(TypeClass *theclass)
{
    MethRec *method;
    CRec *category;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    for (category = theclass->objcinfo->vars; category != NULL; category = category->next) {
        if (category->name == data_00587fa0)
            break;
    }
    if (category == NULL) {
        if (copts.f5d) {
            CError_Warning(ERR_CATEGORY_UNDEFINED, data_00587fa0->name);
        }
        category = galloc(offsetof(CRec, info));
        memclrw(category, offsetof(CRec, info));
        category->name = data_00587fa0;
        category->next = theclass->objcinfo->vars;
        theclass->objcinfo->vars = category;
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk != ')') {
        CError_ReportError(ERR_RPAREN_EXPECTED);
        return;
    }
    tk = CPrepTokenizer_GetNextToken();
    for (;;) {
        switch (tk) {
            case TK_AT_END:
                break;
            case '+':
            case '-':
                parse_method_definition(theclass, category, &category->methods);
                continue;
            default:
                CParser_ParseGlobalDeclaration();
                continue;
        }
        break;
    }
    for (method = category->methods; method != NULL; method = method->next) {
        if (!method->defined)
            CError_Warning(ERR_METHOD_NOT_DEFINED, method);
    }
    create_category_definition(theclass, category);
}

void create_category_definition(TypeClass *classType, CRec *category)
{
    Object *categoryObject;
    OLinkList *relocations;
    OLinkList *nameRelocation;
    OLinkList *relocation;
    ObjCDefinition *definition;
    int data[5];
    char *categoryName;
    Object *instanceMethods;
    Object *classMethods;

    categoryName = CObjCModern_ConcatStrings(classType->classname->name, "_", category->name->name);
    categoryObject = CParser_NewCompilerDefDataObject();
    categoryObject->name = CParser_NameConcat("L_OBJC_CATEGORY_", categoryName);
    categoryObject->type = CDecl_NewStructType(sizeof(data), 4);
    CScope_AddObject(categoryObject->nspace, categoryObject->name, (ObjBase *)categoryObject);
    categoryObject->sclass = TK_STATIC;
    categoryObject->section = 0x16;
    data[0] = CTool_EndianConvertWord32(0);
    relocation = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
    relocation->next = NULL;
    nameRelocation = relocation;
    relocation->obj = fn_00509c40(category->name->name, 0x13);
    relocation->offset = 0;
    relocation->addend = 0;
    data[1] = CTool_EndianConvertWord32(0);
    relocation = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
    relocation->next = nameRelocation;
    relocations = relocation;
    relocation->obj = fn_00509c40(classType->classname->name, 0x13);
    relocation->offset = 4;
    relocation->addend = 0;
    data[2] = CTool_EndianConvertWord32(0);
    instanceMethods = create_method_list_object(classType, category, category->methods,
                                                (UInt8 *)"L_OBJC_CATEGORY_INSTANCE_METHODS_", categoryName, 8, 0);
    if (instanceMethods) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = instanceMethods;
        relocation->offset = 8;
        relocation->addend = 0;
    }
    data[3] = CTool_EndianConvertWord32(0);
    classMethods = create_method_list_object(classType, category, category->methods,
                                             (UInt8 *)"L_OBJC_CATEGORY_CLASS_METHODS_", categoryName, 7, 1);
    if (classMethods) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = classMethods;
        relocation->offset = 0xc;
        relocation->addend = 0;
    }
    data[4] = CTool_EndianConvertWord32(0);
    if (category->bases) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = create_protocol_list(category->bases, categoryName);
        relocation->offset = 0x10;
        relocation->addend = 0;
    }
    fn_004ceab0(categoryObject, data, relocations, categoryObject->type->size);
    definition = (ObjCDefinition *)galloc(sizeof(ObjCDefinition));
    definition->identity.category = category;
    definition->value = (int)categoryObject;
    definition->next = category_definitions;
    category_definitions = definition;
}

void emit_classobject_and_metaclassobject(TypeClass *cls)
{
    Object *metaclassObject;
    Object *classObject;
    Object *className;
    Object *protocols;
    OLinkList *relocations;
    OLinkList *relocation;
    Object *metadata;
    UInt32 buffer[10];

    metaclassObject = cls->objcinfo->metaclassobject;
    classObject = cls->objcinfo->classobject;
    className = fn_00509c40(cls->classname->name, 0x13);
    protocols = create_protocol_list(cls->objcinfo->protocols, cls->classname->name);

    relocations = NULL;
    buffer[0] = CTool_EndianConvertWord32(0);
    buffer[1] = CTool_EndianConvertWord32(0);
    if (cls->bases != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = NULL;
        relocations = relocation;
        relocation->obj = cls->bases->base->objcinfo->metaclassobject;
        relocation->offset = 4;
        relocation->addend = 0;
    }
    relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
    relocation->next = relocations;
    relocation->obj = metaclassObject;
    relocation->offset = 0;
    relocation->addend = 0;
    relocations = relocation;
    buffer[2] = CTool_EndianConvertWord32(0);
    relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
    relocation->next = relocations;
    relocation->obj = className;
    relocation->offset = 8;
    relocation->addend = 0;
    relocations = relocation;
    buffer[3] = CTool_EndianConvertWord32(0);
    buffer[4] = CTool_EndianConvertWord32(2);
    buffer[5] = CTool_EndianConvertWord32(0x28);
    buffer[6] = CTool_EndianConvertWord32(0);
    buffer[7] = CTool_EndianConvertWord32(0);
    metadata = create_method_list_object(cls, NULL, cls->objcinfo->methods, (UInt8 *)"L_OBJC_CLASS_METHODS_",
                                         cls->classname->name, 0x10, 1);
    if (metadata != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = metadata;
        relocation->offset = 0x1c;
        relocation->addend = 0;
    }
    buffer[8] = CTool_EndianConvertWord32(0);
    buffer[9] = CTool_EndianConvertWord32(0);
    if (protocols != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = relocations;
        relocation->obj = protocols;
        relocation->offset = 0x24;
        relocation->addend = 0;
        relocations = relocation;
    }
    CError_ASSERT(2364, metaclassObject->type->size == 0x28);
    fn_004ceab0(metaclassObject, buffer, relocations, metaclassObject->type->size);

    buffer[0] = CTool_EndianConvertWord32(0);
    relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
    relocation->next = NULL;
    relocation->obj = metaclassObject;
    relocation->offset = 0;
    relocation->addend = 0;
    relocations = relocation;
    buffer[1] = CTool_EndianConvertWord32(0);
    if (cls->bases != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = cls->bases->base->objcinfo->classobject;
        relocation->offset = 4;
        relocation->addend = 0;
    }
    buffer[2] = CTool_EndianConvertWord32(0);
    relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
    relocation->next = relocations;
    relocation->obj = className;
    relocation->offset = 8;
    relocation->addend = 0;
    relocations = relocation;
    buffer[3] = CTool_EndianConvertWord32(0);
    buffer[4] = CTool_EndianConvertWord32(1);
    buffer[5] = CTool_EndianConvertWord32(cls->size);
    buffer[6] = CTool_EndianConvertWord32(0);
    metadata = create_ivar_list(cls);
    if (metadata != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = metadata;
        relocation->offset = 0x18;
        relocation->addend = 0;
    }
    buffer[7] = CTool_EndianConvertWord32(0);
    metadata = create_method_list_object(cls, NULL, cls->objcinfo->methods, (UInt8 *)"L_OBJC_INSTANCE_METHODS_",
                                         cls->classname->name, 0x11, 0);
    if (metadata != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = relocations;
        relocations = relocation;
        relocation->obj = metadata;
        relocation->offset = 0x1c;
        relocation->addend = 0;
    }
    buffer[8] = CTool_EndianConvertWord32(0);
    buffer[9] = CTool_EndianConvertWord32(0);
    if (protocols != NULL) {
        relocation = (OLinkList *)CompilerTools_AllocatePool(0x10);
        relocation->next = relocations;
        relocation->obj = protocols;
        relocation->offset = 0x24;
        relocation->addend = 0;
        relocations = relocation;
    }
    CError_ASSERT(2434, classObject->type->size == 0x28);
    fn_004ceab0(classObject, buffer, relocations, classObject->type->size);
}

static inline CObjCInfoRec *register_info0(void *i)
{
    return (CObjCInfoRec *)i;
}

static inline CObjCInfoRec *CObjC_RegisterInfo(TypeClass *i)
{
    return register_info0(i);
}

static inline void fillinstance(OLinkList *node, Object *value)
{
    node->obj = value;
    node->offset = 12;
    node->addend = 0;
}

static inline OLinkList *allocsection(void)
{
    OLinkList *n = CompilerTools_AllocatePool(16);
    return n;
}

#pragma opt_propagation off

Object *CObjC_GetProtocolInfo(CRec *protocol)
{
    OLinkList *previous;
    char *name;
    CObjCInfoRec *registration;
    TypeClass *protocolClass;
    struct {
        OLinkList *first;
        struct CObjCListHead {
            OLinkList *ptr;
        } list;
        OLinkList *last;
    } head;
    SInt32 offsets[5];
    OLinkList *section;
    OLinkList *node;
    Object *info;
    OLinkList *first;
    Object *methods;

    if (protocol->info == NULL) {
        HashNameNode *protocolName = GetHashNameNode("Protocol");
        NameSpaceObjectList *objects = CScope_FindName(registration_context, protocolName);
        if (objects == NULL || objects->object->otype != OT_TYPE ||
            (protocolClass = (TypeClass *)((ObjType *)objects->object)->type)->type != TYPECLASS ||
            protocolClass->objcinfo == NULL) {
            CError_ReportError(ERR_NOT_OBJECTIVE_C_CLASS, protocolName->name);
            protocolClass = NULL;
        }

        if ((registration = CObjC_RegisterInfo(protocolClass)) != NULL) {
            offsets[0] = CTool_EndianConvertWord32(0);
            first = allocsection();
            first->next = NULL;
            first->obj = protocolClass->objcinfo->classobject;
            first->offset = 0;
            first->addend = 0;
            node = first;

            offsets[1] = CTool_EndianConvertWord32(0);
            section = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
            section->next = node;
            head.list.ptr = section;
            section->obj = fn_00509c40(protocol->name->name, 0x13);
            section->offset = 4;
            section->addend = 0;

            offsets[2] = CTool_EndianConvertWord32(0);
            if (protocol->bases != NULL) {
                name = CObjCModern_ConcatStrings(protocol->name->name, "_PROTOCOLS", NULL);
                section = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
                section->next = head.list.ptr;
                head.list.ptr = section;
                section->obj = create_protocol_list(protocol->bases, name);
                section->offset = 8;
                section->addend = 0;
            }

            offsets[3] = CTool_EndianConvertWord32(0);
            methods = create_protocol_method_list(protocol, "L_OBJC_PROTOCOL_INSTANCE_METHODS_", 8, 0);
            if (methods != NULL) {
                node = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
                node->next = previous = head.list.ptr;
                head.list.ptr = node;
                fillinstance(node, (methods = methods));
            }

            (void)methods;
            offsets[4] = CTool_EndianConvertWord32(0);
            if ((info = create_protocol_method_list(protocol, "L_OBJC_PROTOCOL_CLASS_METHODS_", 7, 1)) != NULL) {
                section = (OLinkList *)CompilerTools_AllocatePool(sizeof(OLinkList));
                section->next = previous = head.list.ptr;
                head.list.ptr = section;
                section->obj = info;
                section->offset = 16;
                section->addend = 0;
            }

            name = protocol->name->name;
            info = CParser_NewCompilerDefDataObject();
            info->name = CParser_NameConcat("L_OBJC_PROTOCOL_", name);
            info->type = CDecl_NewStructType(0x14, 4);
            CScope_AddObject(info->nspace, info->name, (ObjBase *)info);
            protocol->info = (Object *)CObjC_RegisterInfo((TypeClass *)info);
            info->type = (Type *)protocolClass;
            info->sclass = TK_STATIC;
            info->section = 0x12;
            if (info->type->size != 0x14) {
                if (!info->type->size)
                    info->type = CDecl_NewStructType(0x14, 4);
                else
                    CError_FATAL(2259);
            }
            ((void (*)(Object *, SInt32 *, struct CObjCListHead, SInt32))fn_004ceab0)(info, offsets, head.list,
                                                                                      info->type->size);
            info->type = (Type *)protocolClass;
        }

        if (protocol->info == NULL)
            protocol->info = fn_00509c40(protocol->name->name, 0x12);
    }
    (void)info;
    (void)previous;
    return protocol->info;
}

#pragma opt_propagation reset

Object *create_protocol_method_list(CRec *cls, char *nm, SInt16 val, UInt8 kind)
{
    SInt32 count;
    OLinkList *entry;
    char *names;
    OLinkList *head;
    MethRec *member = cls->methods;
    SInt32 size;
    char *className;
    Object *result;
    Object *obj;
    char *buffer;
    char *ptr;

    count = 0;
    for (; member != NULL; member = member->next) {
        if (kind == member->isinst)
            count++;
    }

    if (count != 0) {
        size = (count - 1) * 8 + 12;
        buffer = (char *)CompilerTools_AllocatePool(size);
        memclrw(buffer, size);
        className = cls->name->name;
        obj = CParser_NewCompilerDefDataObject();
        obj->name = CParser_NameConcat(nm, className);
        obj->type = CDecl_NewStructType(size, 4);
        CScope_AddObject(obj->nspace, obj->name, (ObjBase *)obj);
        result = obj;
        obj->sclass = TK_STATIC;
        obj->section = val;
        head = NULL;
        *(UInt32 *)buffer = CTool_EndianConvertWord32(count);
        member = cls->methods;
        ptr = buffer + 4;
        if (member != NULL) {
            do {
                if (kind == member->isinst) {
                    entry = (OLinkList *)CompilerTools_AllocatePool(0x10);
                    entry->next = head;
                    head = entry;
                    entry->obj = fn_00509c40(member->selector->name->name, 0x15);
                    entry->offset = ptr - buffer;
                    entry->addend = 0;
                    entry = (OLinkList *)CompilerTools_AllocatePool(0x10);
                    entry->next = head;
                    head = entry;
                    data_00588507 = 1;
                    data_00583548.size = 0;
                    emit_method_type_encoding(member, 1);
                    AppendGListByte(&data_00583548, 0);
                    names = galloc(data_00583548.size);
                    memcpy(names, *data_00583548.data, data_00583548.size);
                    data_00588507 = 0;
                    entry->obj = fn_00509c40(names, 0x14);
                    entry->offset = ptr + 4 - buffer;
                    entry->addend = 0;
                    ptr += 8;
                }
                member = member->next;
            } while (member != NULL);
        }
        fn_004ceab0(obj, buffer, head, obj->type->size);
    } else {
        result = NULL;
    }
    return result;
}

Object *create_protocol_list(ObjectList *entries, char *name)
{
    int count;
    ObjectList *scan;
    OLinkList *relocations;
    int size;
    int offset;
    int lastIndex;
    int entriesSize;
    OLinkList *relocation;
    char *slot;
    ObjectList *entry;
    Object *object;
    int index;
    struct ObjCProtocolList *descriptor;
    Object *result;

    scan = entries;
    count = 0;
    while (scan) {
        scan = scan->next;
        count++;
    }

    if (count) {
        lastIndex = count - 1;
        entriesSize = lastIndex * sizeof(descriptor->protocols[0]);
        descriptor = (struct ObjCProtocolList *)CompilerTools_AllocatePool(size = entriesSize + sizeof(*descriptor));
        memclrw(descriptor, size);
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat("L_OBJC_PROTOCOLS_", name);
        object->type = CDecl_NewStructType(size, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        result = object;
        object->sclass = TK_STATIC;
        relocations = NULL;
        object->section = 18;
        descriptor->count = CTool_EndianConvertWord32(count);
        entry = entries;
        index = 0;
        if (entry) {
            slot = (char *)descriptor;
            do {
                relocation = (OLinkList *)CompilerTools_AllocatePool(sizeof(*relocation));
                relocation->next = relocations;
                relocations = relocation;
                relocation->obj = CObjC_GetProtocolInfo((CRec *)entry->object);
                offset = (slot + 8) - (char *)descriptor;
                slot += sizeof(descriptor->protocols[0]);
                relocation->offset = offset;
                relocation->addend = 0;
                entry = entry->next;
                index++;
            } while (entry);
        }
        fn_004ceab0(object, descriptor, relocations, object->type->size);
    } else {
        result = NULL;
    }
    return result;
}

Object *create_method_list_object(TypeClass *owner, CRec *category, MethRec *methods, UInt8 *namePrefix,
                                  char *nameSuffix, UInt16 qualifiers, char methodKind)
{
    SInt32 count;
    MethRec *method;
    SInt32 size;
    ObjCMethodList *methodList;
    Object *object;
    Object *result;
    OLinkList *relocation;
    OLinkList *relocations;
    ObjCMethodEntry *entry;
    char *encoding;
    NameSpace *savedContext;
    UInt8 savedState;

    count = 0;
    for (method = methods; method != NULL; method = method->next) {
        SInt8 isInstanceMethod = method->isinst;
        if (methodKind == isInstanceMethod)
            count++;
    }
    if (count != 0) {
        size = (count - 1) * sizeof(ObjCMethodEntry) + sizeof(ObjCMethodList);
        methodList = CompilerTools_AllocatePool(size);
        memclrw(methodList, size);
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat((char *)namePrefix, nameSuffix);
        object->type = CDecl_NewStructType(size, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        result = object;
        object->sclass = TK_STATIC;
        relocations = NULL;
        object->section = qualifiers;
        methodList->count = CTool_EndianConvertWord32(count);
        method = methods;
        entry = methodList->methods;
        if (methods != NULL) {
            do {
                SInt8 isInstanceMethod = method->isinst;
                if (methodKind == isInstanceMethod) {
                    relocation = CompilerTools_AllocatePool(sizeof(OLinkList));
                    relocation->next = relocations;
                    relocations = relocation;
                    relocation->obj = fn_00509c40(method->selector->name->name, 0x15);
                    relocation->offset = (char *)&entry->selector - (char *)methodList;
                    relocation->addend = 0;

                    relocation = CompilerTools_AllocatePool(sizeof(OLinkList));
                    relocation->next = relocations;
                    relocations = relocation;

                    data_00588507 = 1;
                    data_00583548.size = 0;
                    emit_method_type_encoding(method, 1);
                    AppendGListByte(&data_00583548, 0);
                    encoding = galloc(data_00583548.size);
                    memcpy(encoding, *data_00583548.data, data_00583548.size);
                    data_00588507 = 0;

                    relocation->obj = fn_00509c40(encoding, 0x14);
                    relocation->offset = (char *)&entry->encoding - (char *)methodList;
                    relocation->addend = 0;

                    relocation = CompilerTools_AllocatePool(sizeof(OLinkList));
                    relocation->next = relocations;
                    relocations = relocation;

                    if (method->function == NULL) {
                        Object *function;
                        TypeFunc *functionType;

                        savedState = copts.cplusplus;
                        copts.cplusplus = 0;
                        savedContext = currentNameSpace;
                        currentNameSpace = registration_context;
                        function = CParser_NewFunctionObject(NULL);
                        function->nspace = owner->nspace;
                        functionType = get_method_ftype(method);
                        function->type = (Type *)functionType;
                        function->name = CObjC_00508810(owner, category, method);
                        function->u.func.linkname = function->name;
                        function->sclass = TK_STATIC;
                        method->function = function;
                        currentNameSpace = savedContext;
                        copts.cplusplus = savedState;
                    }
                    relocation->obj = method->function;
                    relocation->offset = (char *)&entry->implementation - (char *)methodList;
                    relocation->addend = 0;
                    entry++;
                }
                method = method->next;
            } while (method != NULL);
        }
        fn_004ceab0(object, methodList, relocations, object->type->size);
    } else {
        result = NULL;
    }
    return result;
}

Object *create_ivar_list(TypeClass *cls)
{
    char *encoding;
    OLinkList *relocations;
    OLinkList *relocation;
    ObjMemberVar *scan;
    SInt32 size;
    SInt32 count;
    Object *result;
    Object *object;
    char *buffer;
    IvarEntry *entry;
    char *className;
    ObjMemberVar *ivar;
    Type *ivarType;
    UInt32 ivarQualifiers;

    count = 0;
    for (scan = cls->ivars; scan != NULL; scan = scan->next)
        count++;

    if (count != 0) {
        size = (count - 1) * sizeof(IvarEntry) + sizeof(UInt32) + sizeof(IvarEntry);
        buffer = CompilerTools_AllocatePool(size);
        memclrw(buffer, size);
        className = cls->classname->name;
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat("L_OBJC_INSTANCE_VARIABLES_", className);
        object->type = CDecl_NewStructType(size, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        result = object;
        object->sclass = TK_STATIC;
        object->section = 0x18;
        relocations = NULL;
        *(UInt32 *)buffer = CTool_EndianConvertWord32(count);
        ivar = cls->ivars;
        entry = (IvarEntry *)(buffer + sizeof(UInt32));
        while (ivar != NULL) {
            relocation = CompilerTools_AllocatePool(sizeof(OLinkList));
            relocation->next = relocations;
            relocations = relocation;
            relocation->obj = fn_00509c40(ivar->name->name, 0x15);
            relocation->offset = (char *)&entry->name - buffer;
            relocation->addend = 0;
            ivarQualifiers = ivar->qual;
            ivarType = ivar->type;
            data_00583548.size = 0;
            CObjC_005074f0(ivarType, ivarQualifiers, 1);
            AppendGListByte(&data_00583548, 0);
            encoding = galloc(data_00583548.size);
            memcpy(encoding, *data_00583548.data, data_00583548.size);
            relocation = CompilerTools_AllocatePool(sizeof(OLinkList));
            relocation->next = relocations;
            relocations = relocation;
            relocation->obj = fn_00509c40(encoding, 0x14);
            relocation->offset = (char *)&entry->typeEncoding - buffer;
            relocation->addend = 0;
            entry->offset = CTool_EndianConvertWord32(ivar->offset);
            entry++;
            ivar = ivar->next;
        }
        fn_004ceab0(object, buffer, relocations, object->type->size);
    } else {
        result = NULL;
    }
    return result;
}

static inline Type *GetIdType(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return id_type;
    if ((t = FindNamedPointerType("id", required)) == NULL)
        return (Type *)&void_ptr;
    return id_type = t;
}

static inline Type *GetClassType(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return class_pointer_type;
    if ((t = FindNamedPointerType("Class", required)) == NULL)
        return (Type *)&void_ptr;
    return class_pointer_type = t;
}

static inline Boolean IsIdType(Type *ty)
{
    if (ty->type == TYPEPOINTER) {
        if (&ty->type == &void_ptr.type)
            return 0;
        ty = TPTR_TARGET(ty);
        if (ty == TPTR_TARGET(GetIdType(0)))
            return 1;
        if (ty == TPTR_TARGET(GetClassType(0)))
            return 1;
    }
    return 0;
}

static inline Type *GetSelTypeWrapper(Boolean required)
{
    return GetSelType(required);
}

static inline Boolean IsSelType(Type *ty)
{
    Type *t;
    Type *sel;

    if (ty->type != TYPEPOINTER)
        return 0;
    t = TPTR_TARGET(ty);
    sel = (struct Type *)(SInt32)GetSelTypeWrapper(1);
    CError_ASSERT(844, sel->type == TYPEPOINTER);
    return t == TPTR_TARGET((Type *)sel);
}

void CObjC_005074f0(Type *type, UInt32 qual, Boolean flag)
{
    char buf[16];

    for (;;) {
        switch ((SInt8)type->type) {
            case TYPEVOID:
                AppendGListByte(&data_00583548, 'v');
                return;
            case TYPEINT:
            case TYPEFLOAT:
                switch (TYPE_INTEGRAL(type)->integral) {
                    case IT_BOOL:
                        AppendGListByte(&data_00583548, 'C');
                        return;
                    case IT_CHAR:
                        AppendGListByte(&data_00583548, copts.unsignedChar ? 'C' : 'c');
                        return;
                    case IT_UCHAR:
                        AppendGListByte(&data_00583548, 'C');
                        return;
                    case IT_SCHAR:
                        AppendGListByte(&data_00583548, 'c');
                        return;
                    case IT_WCHAR_T:
                        AppendGListByte(&data_00583548, 'S');
                        return;
                    case IT_SHORT:
                        AppendGListByte(&data_00583548, 's');
                        return;
                    case IT_USHORT:
                        AppendGListByte(&data_00583548, 'S');
                        return;
                    case IT_INT:
                        AppendGListByte(&data_00583548, 'i');
                        return;
                    case IT_UINT:
                        AppendGListByte(&data_00583548, 'I');
                        return;
                    case IT_LONG:
                        AppendGListByte(&data_00583548, 'l');
                        return;
                    case IT_ULONG:
                        AppendGListByte(&data_00583548, 'L');
                        return;
                    case IT_LONGLONG:
                        AppendGListByte(&data_00583548, 'q');
                        return;
                    case IT_ULONGLONG:
                        AppendGListByte(&data_00583548, 'Q');
                        return;
                    case IT_FLOAT:
                        AppendGListByte(&data_00583548, 'f');
                        return;
                    case IT_SHORTDOUBLE:
                        AppendGListByte(&data_00583548, 'd');
                        return;
                    case IT_DOUBLE:
                        AppendGListByte(&data_00583548, 'd');
                        return;
                    case IT_LONGDOUBLE:
                        AppendGListByte(&data_00583548, 'D');
                        return;
                    default:
                        CError_FATAL(1838);
                }
            case TYPEENUM:
                type = TYPE_ENUM(type)->enumtype;
                break;
            case TYPEPOINTER:
                if (IsIdType(type)) {
                    AppendGListByte(&data_00583548, '@');
                    return;
                }
                if (IsSelType(type)) {
                    AppendGListByte(&data_00583548, ':');
                    return;
                }
                type = TYPE_POINTER(type)->target;
                if (&type->type == &stchar.type) {
                    AppendGListByte(&data_00583548, '*');
                    return;
                }
                if (type->type == TYPECLASS && TYPE_CLASS(type)->objcinfo != NULL) {
                    AppendGListByte(&data_00583548, '@');
                    return;
                }
                AppendGListByte(&data_00583548, '^');
                flag = data_00588507;
                break;
            case TYPEARRAY:
                AppendGListByte(&data_00583548, '[');
                if (TYPE_POINTER(type)->target->size) {
                    sprintf(buf, "%ld", type->size / TYPE_POINTER(type)->target->size);
                    CompilerTools_AppendGListString(&data_00583548, buf);
                } else {
                    AppendGListByte(&data_00583548, '0');
                }
                CObjC_005074f0(TYPE_POINTER(type)->target, 0, 1);
                AppendGListByte(&data_00583548, ']');
                return;
            case TYPEBITFIELD:
                AppendGListByte(&data_00583548, 'b');
                sprintf(buf, "%ld", TYPE_BITFIELD(type)->bitlength);
                CompilerTools_AppendGListString(&data_00583548, buf);
                return;
            case TYPESTRUCT:
                AppendGListByte(&data_00583548, TYPE_STRUCT(type)->stype == 1 ? '(' : '{');
                if (data_00588507 != 0) {
                    AppendGListByte(&data_00583548, '?');
                } else if (TYPE_STRUCT(type)->name != NULL) {
                    CompilerTools_AppendGListString(&data_00583548, TYPE_STRUCT(type)->name->name);
                }
                if (flag) {
                    AppendGListByte(&data_00583548, '=');
                    {
                        StructMember *member = TYPE_STRUCT(type)->members;
                        while (member != NULL) {
                            CObjC_005074f0(member->type, member->qual, 1);
                            member = member->next;
                        }
                    }
                }
                AppendGListByte(&data_00583548, TYPE_STRUCT(type)->stype == 1 ? ')' : '}');
                return;
            case TYPECLASS:
                encode_class(TYPE_CLASS(type), flag);
                return;
            case TYPEFUNC:
            case TYPETEMPLATE:
            case TYPEMEMBERPOINTER:
                AppendGListByte(&data_00583548, '?');
                return;
            default:
                CError_FATAL(1889);
                return;
        }
    }
}

/* 0x492070, returns the current token */
/* 0x5882d8, current token */

static inline void CopyItem(MethRec *n, MethRec *e)
{
    n->selector = e->selector;
    n->rtype = e->rtype;
    n->rqual = e->rqual;
    n->args = e->args;
    n->isvararg = e->isvararg;
    n->isinst = e->isinst;
    n->defined = 0;
}

void emit_method_type_encoding(MethRec *p, int b)
{
    char buf[16];
    ObjCParameterNode *n;

    if (p->rqual & Q_IN)
        AppendGListByte(&data_00583548, 0x6e);
    if (p->rqual & Q_OUT)
        AppendGListByte(&data_00583548, 0x6f);
    if (p->rqual & Q_INOUT)
        AppendGListByte(&data_00583548, 0x4e);
    if (p->rqual & Q_BYCOPY)
        AppendGListByte(&data_00583548, 0x4f);
    if ((p->rqual & Q_BYREF), (p->rqual & Q_ONEWAY))
        AppendGListByte(&data_00583548, 0x56);

    if (p->rtype)
        CObjC_005074f0(p->rtype, 0, b);
    else
        AppendGListByte(&data_00583548, 0x40);

    sprintf(buf, "%ld", CodeGen_GetMethRecRtypeAndArgsSize(p));
    CompilerTools_AppendGListString(&data_00583548, buf);

    AppendGListByte(&data_00583548, 0x40);

    sprintf(buf, "%ld", CodeGen_GetMethRecRTypeSize(p));
    CompilerTools_AppendGListString(&data_00583548, buf);

    AppendGListByte(&data_00583548, 0x3a);

    sprintf(buf, "%ld", fn_00432480(p));
    CompilerTools_AppendGListString(&data_00583548, buf);

    for (n = (ObjCParameterNode *)p->args; n != NULL; n = n->next) {
        if (n->type) {
            if (n->qual & Q_CONST)
                AppendGListByte(&data_00583548, 0x72);
            CObjC_005074f0(n->type, 0, b);
            sprintf(buf, "%ld", CodeGen_GetObjCParameterOffset(p, n));
            CompilerTools_AppendGListString(&data_00583548, buf);
        }
    }
}

void encode_class(TypeClass *cls, Boolean flag)
{
    ObjMemberVar *iv;

    if (!(cls->flags & 0x4000)) {
        AppendGListByte(&data_00583548, cls->mode == 1 ? 0x28 : 0x7b);
        if (data_00588507 != 0)
            AppendGListByte(&data_00583548, 0x3f);
        else if (cls->classname != NULL)
            CompilerTools_AppendGListString(&data_00583548, cls->classname->name);
        if (flag) {
            AppendGListByte(&data_00583548, 0x3d);
            for (iv = cls->ivars; iv != NULL; iv = iv->next)
                CObjC_005074f0(iv->type, iv->qual, 1);
        }
        AppendGListByte(&data_00583548, cls->mode == 1 ? 0x29 : 0x7d);
    } else {
        AppendGListByte(&data_00583548, 0x3f);
    }
}

/* Objective-C method parameter list entry. */
/* Objective-C method list entry. */

void parse_method_definition(TypeClass *object, CRec *kind, MethRec **methods)
{
    DeclInfo declaration;
    FuncArg *argument;
    FuncArg *arguments;
    FuncArg *parameter;
    MethRec *method;
    MethRec *parsedMethod;
    NameSpace *savedNameSpace;
    UInt8 savedCPlusPlus;
    Object *function;
    Object *newFunction;
    TypeFunc *functionType;
    ObjCParameterNode *sourceParameter;

    parsedMethod = parse_method_declaration(1);
    if (parsedMethod == NULL) {
        return;
    }
    method = fn_00508940(*methods, parsedMethod, 1, 1);
    if (method == NULL) {
        parsedMethod->next = *methods;
        *methods = parsedMethod;
        method = parsedMethod;
        CObjCModern_RegisterMethodSelector(parsedMethod);
    }
    if (method->defined != 0) {
        CError_ReportError(ERR_METHOD_REDEFINED, method);
    }
    if (method->function == NULL) {
        savedCPlusPlus = copts.cplusplus;
        copts.cplusplus = 0;
        savedNameSpace = currentNameSpace;
        currentNameSpace = registration_context;
        newFunction = CParser_NewFunctionObject(NULL);
        newFunction->nspace = object->nspace;
        functionType = get_method_ftype(method);
        newFunction->type = (Type *)functionType;
        newFunction->name = CObjC_00508810(object, kind, method);
        newFunction->u.func.linkname = newFunction->name;
        newFunction->sclass = TK_STATIC;
        method->function = newFunction;
        currentNameSpace = savedNameSpace;
        copts.cplusplus = savedCPlusPlus;
    }
    function = method->function;
    CError_ASSERT(1631, function->type->type == TYPEFUNC);
    argument = arguments = ((TypeFunc *)function->type)->args;
    if (arguments != NULL) {
        argument->type = (Type *)CDecl_NewPointerType((Type *)object);
        if ((parameter = argument->next) != NULL && (parameter = parameter->next) != NULL) {
            for (sourceParameter = parsedMethod->args; sourceParameter != NULL && parameter != NULL;
                 sourceParameter = sourceParameter->next, parameter = parameter->next) {
                parameter->name = sourceParameter->name;
                parameter->type = sourceParameter->type;
                parameter->qual |= sourceParameter->qual;
            }
        }
    }
    if (tk == ';') {
        if (copts.f5d != 0) {
            CError_Warning(ERR_LBRACE_EXPECTED);
        }
        tk = CPrepTokenizer_GetNextToken();
    }
    memclrw(&declaration, sizeof(declaration));
    CFunc_ParseFuncDef(function, &declaration, object, 1, method->isinst, NULL);
    method->defined = 1;
    tk = CPrepTokenizer_GetNextToken();
}

void parse_category(TypeClass *owner)
{
    CRec *existingCategory;
    MethRec *method;
    MethRec *newMethod;
    ObjectList *base;
    CRec *category;
    CRec *protocol;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return;
    }
    existingCategory = owner->objcinfo->vars;
    if (existingCategory != NULL) {
        do {
            if (existingCategory->name == data_00587fa0) {
                CError_ReportError(ERR_CATEGORY_REDEFINED, data_00587fa0->name);
                break;
            }
            existingCategory = existingCategory->next;
        } while (existingCategory != NULL);
    }

    category = galloc(sizeof(*category) - sizeof(category->info));
    memclrw(category, sizeof(*category) - sizeof(category->info));
    category->name = data_00587fa0;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != ')') {
        CError_ReportError(ERR_RPAREN_EXPECTED);
        return;
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '<') {
        category->bases = parse_protocol_list();
        for (base = category->bases; base != NULL; base = base->next) {
            protocol = (CRec *)base->object;
            for (method = protocol->methods; method != NULL; method = method->next) {
                if (fn_00508940(category->methods, method, 1, 1) == NULL) {
                    newMethod = galloc(sizeof(*newMethod));
                    memclrw(newMethod, sizeof(*newMethod));
                    CopyItem(newMethod, method);
                    newMethod->next = category->methods;
                    category->methods = newMethod;
                    if (owner != NULL)
                        CObjCModern_RegisterMethodSelector(newMethod);
                }
            }
        }
    }

    for (;;) {
        switch (tk) {
            case TK_AT_END:
                break;
            case '+':
            case '-':
                newMethod = parse_method_declaration(1);
                if (newMethod != NULL) {
                    if (fn_00508940(category->methods, newMethod, 0, 1) == NULL) {
                        newMethod->next = category->methods;
                        category->methods = newMethod;
                        (void)CObjCModern_RegisterMethodSelector(newMethod);
                    } else {
                        CError_ReportError(ERR_METHOD_REDECLARED, newMethod);
                    }
                }
                if (tk != ';') {
                    CError_ReportError(ERR_SEMICOLON_EXPECTED);
                } else {
                    tk = CPrepTokenizer_GetNextToken();
                }
                continue;
            default:
                CParser_ParseGlobalDeclaration();
                continue;
        }
        break;
    }
    category->next = owner->objcinfo->vars;
    owner->objcinfo->vars = category;
}

Type *CObjC_ParseProtocolList(Type *type)
{
    if ((UInt16)tk != 60U)
        CError_Internal(CERROR_FILE, 1530U);
    parse_protocol_list();
    return type;
}

static inline CRec *FindNamedRec(void *obj, CRec *list)
{
    for (; list != NULL; list = list->next) {
        if (list->name == (HashNameNode *)obj)
            break;
    }
    return list;
}

static inline ObjectList *FindObject(void *obj, ObjectList *list)
{
    for (; list != NULL; list = list->next) {
        if (list->object == (Object *)obj)
            break;
    }
    return list;
}

inline Type *FindIdType(void)
{
    NameSpaceObjectList *node;
    Type *t;

    node = CScope_FindName(registration_context, GetHashNameNode("id"));
    if (node != NULL && OBJ_TYPE(node->object)->otype == OT_TYPE) {
        t = OBJ_TYPE(node->object)->type;
        if (t->type == TYPEPOINTER)
            return t;
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNEXPECTED_TYPE, "id");
    } else {
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNDEFINED_SHOULD_DEFINED, "id");
    }
    return NULL;
}

#pragma opt_propagation off

Type *CObjC_ParseIdType(void)
{
    Type *type;
    Type *result;
    Type *found;
    TypePointer *qualifiedType;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '<') {
        qualifiedType = (TypePointer *)galloc((sizeof(TypePointer) + sizeof(qualifiedType->protocols[0]) + 3) & ~3);
        memclrw(qualifiedType, (sizeof(TypePointer) + sizeof(qualifiedType->protocols[0]) + 3) & ~3);
        if (class_pointer_type != NULL) {
            type = id_type;
        } else {
            found = FindIdType();
            type = found;
            if (type == NULL) {
                type = (Type *)&void_ptr;
            } else {
                type = id_type = found;
            }
        }
        *qualifiedType = *(TypePointer *)type;
        qualifiedType->protocols[0] = parse_protocol_list();
        qualifiedType->qual |= Q_IS_OBJC_ID;
        return (Type *)qualifiedType;
    }

    if (class_pointer_type != NULL) {
        result = id_type;
    } else {
        found = FindIdType();
        type = found;
        if (type == NULL) {
            type = (Type *)&void_ptr;
        } else {
            type = id_type = found;
        }
        result = type;
    }
    return result;
}

#pragma opt_propagation reset

static inline TypeClass *find_objc_class(HashNameNode *name)
{
    NameSpaceObjectList *list;
    TypeClass *type;

    list = CScope_FindName(registration_context, name);
    if (list != NULL && OBJ_BASE(list->object)->otype == OT_TYPE &&
        (type = (TypeClass *)OBJ_TYPE(list->object)->type)->type == TYPECLASS && type->objcinfo != NULL)
        return type;
    CError_ReportError(ERR_NOT_OBJECTIVE_C_CLASS, name->name);
    return NULL;
}

void CObjC_005082b0(TypeStruct *classInfo)
{
    TypeClass *baseClass;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == TK_IDENTIFIER) {
            if ((baseClass = find_objc_class(data_00587fa0)) != NULL) {
                if (baseClass->flags & CLASS_COMPLETED) {
                    classInfo->size = baseClass->size;
                    classInfo->align = baseClass->align;
                    copy_ivars_to_struct(classInfo, baseClass);
                } else {
                    CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, baseClass, 0);
                }
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ')') {
                CError_ReportError(ERR_RPAREN_EXPECTED);
            }
        } else {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        }
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
    }
}

void copy_ivars_to_struct(TypeStruct *type, TypeClass *cls)
{
    ObjMemberVar *iv;
    StructMember *sm;

    if (cls->bases != NULL)
        copy_ivars_to_struct(type, cls->bases->base);

    for (iv = cls->ivars; iv != NULL; iv = iv->next) {
        sm = (StructMember *)galloc(sizeof(StructMember));
        memclrw(sm, sizeof(StructMember));
        sm->name = iv->name;
        sm->type = iv->type;
        sm->qual = iv->qual;
        sm->offset = iv->offset;
        appendmember(type, sm);
    }
}

ObjectList *parse_protocol_list(void)
{
    ObjectList *result = NULL;
    CRec *protocol;
    ObjectList *entry;

    tk = CPrepTokenizer_GetNextToken();
    for (;;) {
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            break;
        }
        if ((protocol = FindNamedRec(data_00587fa0, data_00588064)) != NULL) {
            entry = FindObject(protocol, result);
            if (entry == NULL) {
                entry = (ObjectList *)galloc(sizeof(ObjectList));
                entry->next = result;
                entry->object = (Object *)protocol;
                result = entry;
            } else {
                CError_ReportError(ERR_PROTOCOL_ALREADY_PROTOCOL_LIST, data_00587fa0->name);
            }
        } else {
            CError_ReportError(ERR_PROTOCOL_UNDEFINED, data_00587fa0->name);
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk == '>') {
            tk = CPrepTokenizer_GetNextToken();
            break;
        }
        if (tk != ',') {
            CError_ReportError(ERR_COMMA_EXPECTED);
            break;
        }
        tk = CPrepTokenizer_GetNextToken();
    }
    return result;
}

static inline FuncArg *ObjC_NewArg(TypeFunc *f, HashNameNode *name, Type *type, UInt32 qual)
{
    FuncArg *a;
    if ((a = f->args) != NULL) {
        while (a->next != NULL)
            a = a->next;
        a->next = CParser_NewFuncArg();
        a = a->next;
    } else {
        f->args = CParser_NewFuncArg();
        a = f->args;
    }
    a->name = name;
    a->type = type;
    a->qual = qual;
    return a;
}

static inline FuncArg *AppendFirst(TypeFunc *f, Type *type)
{
    FuncArg *a;
    HashNameNode *name = this_self_name;
    if ((a = f->args) != NULL) {
        while (a->next != NULL)
            a = a->next;
        a->next = CParser_NewFuncArg();
        a = a->next;
    } else {
        f->args = CParser_NewFuncArg();
        a = f->args;
    }
    a->name = name;
    a->type = type;
    a->qual = 0;
    return a;
}

static inline FuncArg *NewFirstArg(TypeFunc *f)
{
    SInt32 type = (SInt32)GetIdType(1);
    return AppendFirst(f, (Type *)type);
}

TypeFunc *get_method_ftype(MethRec *spec)
{
    FuncArg *p;
    ObjCParameterNode *a;
    TypeFunc *f;

    if (spec->ftype == NULL) {
        f = galloc(sizeof(TypeFunc));
        memclrw(f, sizeof(TypeFunc));
        f->type = TYPEFUNC;
        f->functype = spec->rtype;
        f->qual = spec->rqual;
        f->flags = 0x8000;
        fn_00504240(f);
        NewFirstArg(f);
        ObjC_NewArg(f, GetHashNameNode("_cmd"), GetSelType(1), 0);
        for (a = (ObjCParameterNode *)spec->args; a != NULL; a = a->next) {
            if (a->type != NULL)
                ObjC_NewArg(f, a->name, a->type, a->qual);
        }
        if (spec->isvararg) {
            p = f->args;
            for (;;) {
                if (p->next == NULL) {
                    p->next = &data_00583098;
                    break;
                }
                p = p->next;
            }
        }
        spec->ftype = f;
    }
    return spec->ftype;
}

HashNameNode *CObjC_00508810(TypeClass *obj, CRec *ns, MethRec *info)
{
    HashNameNode *name;
    ObjCParameterNode *p;

    data_00583548.size = 0;
    CompilerTools_AppendGListString(&data_00583548, "");
    if (info->isinst != 0)
        CompilerTools_AppendGListString(&data_00583548, "+[");
    else
        CompilerTools_AppendGListString(&data_00583548, "-[");
    CompilerTools_AppendGListString(&data_00583548, obj->classname->name);
    if (ns != NULL) {
        AppendGListByte(&data_00583548, 0x28);
        CompilerTools_AppendGListString(&data_00583548, ns->name->name);
        AppendGListByte(&data_00583548, 0x29);
    }
    AppendGListByte(&data_00583548, 0x20);
    for (p = info->args; p != NULL; p = p->next) {
        if (p->selectorName != NULL)
            CompilerTools_AppendGListString(&data_00583548, p->selectorName->name);
        if (p->type != NULL)
            AppendGListByte(&data_00583548, 0x3a);
    }
    AppendGListName(&data_00583548, "]");
    fn_00443190(data_00583548.data);
    name = GetHashNameNode(*data_00583548.data);
    fn_004431b0(data_00583548.data);
    return name;
}

static inline Boolean ObjCMethodTypesEqual(Type *type, Type *other)
{
    if (copts.f5d == 0 && CObjC_IsIdCompatiblePointerPair(type, other) != 0)
        return 1;
    return iscpp_typeequal(type, other);
}

MethRec *fn_00508940(MethRec *methods, MethRec *method, char check_types, char allow_missing)
{
    Type *name_type;
    MethRec *candidate;
    ObjCParameterNode *argument;
    ObjCParameterNode *candidate_argument;
    ObjCParameterNode *argument_selector;
    ObjCParameterNode *candidate_selector;
    Type *candidate_type;
    Type *return_type;
    Type *candidate_return_type;
    char compatible;
    char return_compatible;
    Type *argument_type;
    for (candidate = methods; candidate != NULL; candidate = candidate->next) {
        if (method->isinst == candidate->isinst) {
            candidate_selector = candidate->args;
            candidate_argument = candidate_selector;
            argument_selector = method->args;
            argument = argument_selector;
            for (;;) {
                if (argument_selector == NULL || candidate_selector == NULL) {
                    if (argument_selector != NULL || candidate_selector != NULL) {
                        break;
                    }
                    if (check_types != 0) {
                        for (;;) {
                            if (argument == NULL || argument->qual != candidate_argument->qual) {
                                break;
                            }
                            if ((argument_type = argument->type) != NULL) {
                                if ((candidate_type = candidate_argument->type) == NULL) {
                                    break;
                                }
                                compatible = ObjCMethodTypesEqual(argument_type, candidate_type);
                                if (compatible == 0) {
                                    break;
                                }
                            } else if (candidate_argument->type != NULL) {
                                break;
                            }
                            candidate_argument->name = argument->name;
                            argument = argument->next;
                            candidate_argument = candidate_argument->next;
                        }
                        if (argument == NULL && method->isvararg == candidate->isvararg &&
                            method->rqual == candidate->rqual) {
                            candidate_return_type = candidate->rtype;
                            return_type = method->rtype;
                            return_compatible = ObjCMethodTypesEqual(return_type, candidate_return_type);
                            if (return_compatible == 0) {
                                CError_ReportError(ERR_METHOD_REDECLARED, candidate);
                            }
                        } else {
                            CError_ReportError(ERR_METHOD_REDECLARED, candidate);
                        }
                    }
                    return candidate;
                }
                if (argument_selector->selectorName != candidate_selector->selectorName ||
                    (name_type = argument_selector->type) == NULL && candidate_selector->type != NULL ||
                    name_type != NULL && candidate_selector->type == NULL) {
                    break;
                }
                argument_selector = argument_selector->next;
                candidate_selector = candidate_selector->next;
            }
        }
    }
    if (allow_missing == 0) {
        CError_ReportError(ERR_UNDEFINED_METHOD, method);
    }
    return NULL;
}

/* "id" */
/* 0x5882d8, token */

static inline Type *find_named_pointer_type(char *name, Boolean required)
{
    NameSpaceObjectList *node;
    Type *t;

    node = CScope_FindName(registration_context, GetHashNameNode(name));
    if (node != NULL && node->object->otype == OT_TYPE) {
        if ((t = OBJ_TYPE(node->object)->type)->type == TYPEPOINTER)
            return t;
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNEXPECTED_TYPE, name);
    } else if (required) {
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNDEFINED_SHOULD_DEFINED, name);
    }
    return NULL;
}

static inline Type *get_id_type(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return id_type;
    if ((t = find_named_pointer_type("id", required)) == NULL)
        return (Type *)&void_ptr;
    return id_type = t;
}

/* State shared by the Objective-C type parsing helpers. Unused portions
   remain opaque; value0c and flag45 have no known symbolic names. */

MethRec *parse_method_declaration(char useGlobalAllocation)
{
    MethRec *method;
    ObjCParameterNode *argument;
    ObjCParameterNode **argumentLink;
    DeclInfo returnType;
    DeclInfo argumentType;
    Boolean firstArgument;

    if (useGlobalAllocation) {
        method = (MethRec *)galloc(sizeof(*method));
        memclrw(method, sizeof(*method));
    } else {
        method = (MethRec *)CompilerTools_AllocatePool(sizeof(*method));
        memclrw(method, sizeof(*method));
    }
    switch (tk) {
        case '+':
            method->isinst = 1;
            break;
        case '-':
            method->isinst = 0;
            break;
        default:
            CError_Internal(CERROR_FILE, 0x3cd);
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        memclrw(&returnType, sizeof(returnType));
        CParser_GetDeclSpecs(&returnType, 0);
        CDecl_ParseDeclarator(&returnType);
        if (tk != ')')
            CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
        if (returnType.name != NULL)
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        if (returnType.parserOption)
            method->rtype = CObjC_GetIdType(1);
        else
            method->rtype = returnType.thetype;
        method->rqual = returnType.qual;
        CError_ReportIllegalFlags(method->rqual & ~(Q_CV | Q_BYCOPY | Q_BYREF | Q_ONEWAY));
    } else {
        method->rtype = get_id_type(1);
    }
    argumentLink = &method->args;
    firstArgument = 1;
    for (;;) {
        if (useGlobalAllocation) {
            argument = (ObjCParameterNode *)galloc(sizeof(*argument));
            memclrw(argument, sizeof(*argument));
        } else {
            argument = (ObjCParameterNode *)CompilerTools_AllocatePool(sizeof(*argument));
            memclrw(argument, sizeof(*argument));
        }
        *argumentLink = argument;
        argumentLink = &argument->next;
        CObjC_ConvertKeywordToIdentifier();
        if (tk == TK_IDENTIFIER) {
            argument->selectorName = data_00587fa0;
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ':') {
                if (firstArgument)
                    break;
                CError_ReportError(ERR_COLON_EXPECTED);
                return NULL;
            }
        }
        if (tk != ':') {
            CError_ReportError(ERR_COLON_EXPECTED);
            return NULL;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk == '(') {
            tk = CPrepTokenizer_GetNextToken();
            memclrw(&argumentType, sizeof(argumentType));
            CParser_GetDeclSpecs(&argumentType, 0);
            CDecl_ParseDeclarator(&argumentType);
            if (tk != ')')
                CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
            if (argumentType.name != NULL)
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            if (argumentType.parserOption)
                argument->type = CObjC_GetIdType(1);
            else
                argument->type = argumentType.thetype;
            argument->qual = argumentType.qual;
            if (argument->type->type == TYPEARRAY)
                argument->type = CDecl_NewPointerType(TYPE_POINTER(argument->type)->target);
            CError_ReportIllegalFlags(argument->qual & ~(Q_CV | Q_IN | Q_OUT | Q_INOUT | Q_BYCOPY | Q_BYREF));
            if ((argument->qual & (Q_OUT | Q_INOUT)) != 0 && argument->type->type != TYPEPOINTER)
                CError_ReportIllegalFlags(argument->qual & (Q_OUT | Q_INOUT));
        } else {
            argument->type = get_id_type(1);
        }
        if (tk != TK_IDENTIFIER) {
            CObjC_ConvertKeywordToIdentifier();
            if (tk != TK_IDENTIFIER) {
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                data_00587fa0 = unnamed_name;
            }
        }
        argument->name = data_00587fa0;
        tk = CPrepTokenizer_GetNextToken();
        if (tk == ',') {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_ELLIPSIS) {
                method->isvararg = 1;
                tk = CPrepTokenizer_GetNextToken();
                break;
            }
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        }
        CObjC_ConvertKeywordToIdentifier();
        if (tk != ':' && tk != TK_IDENTIFIER)
            break;
        firstArgument = 0;
    }
    return method;
}

void CObjC_ConvertKeywordToIdentifier(void)
{
    switch (tk) {
        case TK_CLASS:
            data_00587fa0 = GetHashNameNode("class");
            tk = TK_IDENTIFIER;
            break;
        case 0x180:
            data_00587fa0 = GetHashNameNode("self");
            tk = TK_IDENTIFIER;
            break;
        case TK_IN:
            data_00587fa0 = GetHashNameNode("in");
            tk = TK_IDENTIFIER;
            break;
        case TK_BREAK:
            data_00587fa0 = GetHashNameNode("break");
            tk = TK_IDENTIFIER;
            break;
        case TK_NEW:
            data_00587fa0 = GetHashNameNode("new");
            tk = TK_IDENTIFIER;
            break;
        case TK_DELETE:
            data_00587fa0 = GetHashNameNode("delete");
            tk = TK_IDENTIFIER;
            break;
    }
}

TypeClass *find_or_create_objc_class(HashNameNode *name)
{
    TypeClass *classType;
    NameSpaceObjectList *found = CScope_FindName(registration_context, name);
    if (found != NULL) {
        if (found->object->otype != OT_TYPE ||
            (classType = (TypeClass *)((ObjType *)found->object)->type)->type != TYPECLASS ||
            classType->objcinfo == NULL) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED, name->name);
            return NULL;
        }
        return classType;
    } else {
        /* The legacy allocation includes a trailing word beyond ObjCInfo. */
        ObjCInfo *info = (ObjCInfo *)galloc(sizeof(*info) + sizeof(UInt16));
        Object *metaclassObject;
        Object *classObject;
        memclrw(info, sizeof(*info) + sizeof(UInt16));
        classType = CDecl_DefineClass(registration_context, name, NULL, 2, 1, 1);
        classType->flags |= CLASS_SINGLE_OBJECT;
        classType->objcinfo = info;
        classObject = CParser_NewCompilerDefDataObject();
        classObject->name = CParser_NameConcat("L_OBJC_CLASS_", name->name);
        classObject->type = CDecl_NewStructType(0x28, 4);
        CScope_AddObject(classObject->nspace, classObject->name, (ObjBase *)classObject);
        info->classobject = classObject;
        info->classobject->section = 0xe;
        metaclassObject = CParser_NewCompilerDefDataObject();
        metaclassObject->name = CParser_NameConcat("L_OBJC_METACLASS_", name->name);
        metaclassObject->type = CDecl_NewStructType(0x28, 4);
        CScope_AddObject(metaclassObject->nspace, metaclassObject->name, (ObjBase *)metaclassObject);
        info->metaclassobject = metaclassObject;
        info->metaclassobject->section = 0xf;
        return classType;
    }
}

Boolean CObjC_IsIdCompatiblePointerPair(Type *leftType, Type *rightType)
{
    Boolean leftIsId;
    Boolean rightIsId;

    if (leftType->type == TYPEPOINTER && rightType->type == TYPEPOINTER) {
        leftIsId = IsIdType(leftType);
        rightIsId = IsIdType(rightType);
        if (leftIsId && rightIsId)
            return 1;
        if (leftIsId && TPTR_TARGET(rightType)->type == TYPECLASS &&
            TYPE_CLASS(TPTR_TARGET(rightType))->objcinfo != NULL)
            return 1;
        if (rightIsId && TPTR_TARGET(leftType)->type == TYPECLASS &&
            TYPE_CLASS(TPTR_TARGET(leftType))->objcinfo != NULL)
            return 1;
    }
    return 0;
}

static inline Type *lookup_named_pointer_type(char *name, Boolean required)
{
    NameSpaceObjectList *node;
    Type *t;

    node = CScope_FindName(registration_context, GetHashNameNode(name));
    if (node != NULL && node->object->otype == OT_TYPE) {
        if ((t = OBJ_TYPE(node->object)->type)->type == TYPEPOINTER)
            return t;
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNEXPECTED_TYPE, name);
    } else if (required) {
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNDEFINED_SHOULD_DEFINED, name);
    }
    return NULL;
}

static inline Type *lookup_id_type(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return id_type;
    if ((t = lookup_named_pointer_type("id", required)) == NULL)
        return (Type *)&void_ptr;
    return id_type = t;
}

static inline Type *get_class_pointer_type(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return class_pointer_type;
    if ((t = lookup_named_pointer_type("Class", required)) == NULL)
        return (Type *)&void_ptr;
    return class_pointer_type = t;
}

Boolean CObjC_IsIdOrSelType(Type *ty)
{
    if (ty->type == TYPEPOINTER) {
        if (ty == (Type *)&void_ptr)
            return FALSE;
        ty = TPTR_TARGET(ty);
        if (ty == TPTR_TARGET(lookup_id_type(0)))
            return TRUE;
        if (ty == TPTR_TARGET(get_class_pointer_type(0)))
            return TRUE;
    }
    return FALSE;
}

static inline Type *find_id_type(Boolean required)
{
    NameSpaceObjectList *node;
    Type *t;

    node = CScope_FindName(registration_context, GetHashNameNode("id"));
    if (node != NULL && node->object->otype == OT_TYPE) {
        if ((t = OBJ_TYPE(node->object)->type)->type == TYPEPOINTER)
            return t;
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNEXPECTED_TYPE, "id");
    } else if (required) {
        CError_ReportError(ERR_OBJECTIVE_C_TYPE_UNDEFINED_SHOULD_DEFINED, "id");
    }
    return NULL;
}

Type *CObjC_GetIdType(Boolean required)
{
    Type *t;

    if (class_pointer_type)
        return id_type;
    if ((t = find_id_type(required)) == NULL)
        return (Type *)&void_ptr;
    return id_type = t;
}

void CObjCModern_GenerateSymbolTableAndModule(void)
{
    int count2;
    int index;
    int count1;
    int size;
    ObjCSymbolTable *symbols;
    OLinkList *references;
    Object *object;
    OLinkList *reference;
    struct PrecTypeEntry *definition;
    ObjCDefinition *categoryDefinition;
    ObjcModule module;

    if (copts.f5c != 0 || class_type_entries != NULL) {
        definition = class_type_entries;
        count1 = 0;
        while (definition != NULL) {
            definition = definition->next;
            count1++;
        }
        categoryDefinition = category_definitions;
        count2 = 0;
        while (categoryDefinition != NULL) {
            categoryDefinition = categoryDefinition->next;
            count2++;
        }
        size = (count1 + count2 - 1) * 4 + 16;
        symbols = (ObjCSymbolTable *)CompilerTools_AllocatePool(size);
        memclrw(symbols, size);
        symbols->word0 = CTool_EndianConvertWord32(0);
        symbols->word4 = CTool_EndianConvertWord32(0);
        symbols->word0 = CTool_EndianConvertWord16(0);
        symbols->count1 = CTool_EndianConvertWord16(count1);
        symbols->count2 = CTool_EndianConvertWord16(count2);
        references = NULL;
        index = 0;
        definition = class_type_entries;
        while (definition != NULL) {
            reference = (OLinkList *)CompilerTools_AllocatePool(16);
            reference->next = references;
            references = reference;
            reference->obj = definition->type->objcinfo->classobject;
            reference->offset = (char *)&symbols->definitions[index] - (char *)symbols;
            reference->addend = 0;
            definition = definition->next;
            index++;
        }
        categoryDefinition = category_definitions;
        while (categoryDefinition != NULL) {
            reference = (OLinkList *)CompilerTools_AllocatePool(16);
            reference->next = references;
            references = reference;
            reference->obj = (Object *)categoryDefinition->value;
            reference->offset = (char *)&symbols->definitions[index] - (char *)symbols;
            reference->addend = 0;
            categoryDefinition = categoryDefinition->next;
            index++;
        }
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat("", "L_OBJC_SYMBOLS");
        object->type = CDecl_NewStructType(size, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        object->sclass = TK_STATIC;
        object->section = 0x1a;
        fn_004ceab0(object, symbols, references, object->type->size);
        module.version = CTool_EndianConvertWord32(5);
        module.size = CTool_EndianConvertWord32(0x10);
        module.name = CTool_EndianConvertWord32(0);
        reference = (OLinkList *)CompilerTools_AllocatePool(16);
        reference->next = NULL;
        references = reference;
        reference->obj = fn_00509c40(CPrep_GetFileName(NULL, 1, 0), 0x13);
        reference->offset = 8;
        reference->addend = 0;
        module.symtab = CTool_EndianConvertWord32(0);
        reference = (OLinkList *)CompilerTools_AllocatePool(16);
        reference->next = references;
        reference->obj = object;
        reference->offset = 0xc;
        reference->addend = 0;
        object = CParser_NewCompilerDefDataObject();
        object->name = CParser_NameConcat("", "L_OBJC_MODULES");
        object->type = CDecl_NewStructType(0x10, 4);
        CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
        object->sclass = TK_STATIC;
        object->section = 0x19;
        fn_004ceab0(object, &module, reference, object->type->size);
    }
}

static struct NamedObjectCacheEntry *named_object_cache;

/* Data of the original file that none of its linked code uses. */
static char lbl_00573138[] = "L_OBJC_CLASS_REFERENCES_%ld";

static inline HashEntry *FindHashNode(HashNameNode *name)
{
    struct HashEntry *rec;

    if (selector_hash == NULL)
        return NULL;
    rec = selector_hash[name->hashval & 0x3ff];
    while (rec != NULL) {
        if (rec->name == name)
            break;
        rec = rec->next;
    }
    return (HashEntry *)rec;
}

static inline Boolean SamePtr(void *x, void *y)
{
    if (copts.f5d == 0) {
        if (CObjC_IsIdCompatiblePointerPair(x, y))
            return 1;
    }
    return iscpp_typeequal(x, y);
}

static inline void clear_word(unsigned char *storage)
{
    *(unsigned int *)storage = 0;
}

Object *CObjCModern_GetOrCreateFunctionObject(char *identifier, char *identifier2)
{
    UInt8 savedState;
    ObjectList *found;
    HashNameNode *name;
    NameSpace *savedContext;
    Object *newObject;
    Object *object;
    ObjectList *entry;

    savedState = copts.cplusplus;
    name = GetHashNameNode(identifier);
    found = CScope_FindObjectListInNameSpace(registration_context, name);
    if (found != NULL) {
        entry = found;
        object = entry->object;
        if (object->type->type == TYPEFUNC)
            return object;
        CError_ReportError(ERR_IDENTIFIER_REDECLARED, name);
    }
    copts.cplusplus = 0;
    savedContext = currentNameSpace;
    currentNameSpace = registration_context;
    newObject = CParser_NewFunctionObject(NULL);
    currentNameSpace = savedContext;
    newObject->type = (Type *)&data_0055d5e8;
    newObject->name = name;
    if (found == NULL)
        CScope_AddObject(registration_context, name, (ObjBase *)newObject);
    copts.cplusplus = savedState;
    return newObject;
}

Object *CObjCModern_GetSelectorReference(HashEntry *p)
{
    Object *obj;
    Object *id;
    SInt32 block[4];
    char buf[32];
    SInt32 local14[2];

    if (p->obj == NULL) {
        id = fn_00509c40(p->name->name, 0x15);
        sprintf(buf, "L_OBJC_SELECTOR_REFERENCES_%ld", selector_reference_count++);
        obj = CParser_NewCompilerDefDataObject();
        obj->name = GetHashNameNode(buf);
        obj->sclass = TK_STATIC;
        obj->type = (Type *)&void_ptr;
        obj->section = 0xb;
        if (CScope_FindObjectListInNameSpace(registration_context, obj->name))
            CError_ReportError(ERR_OBJECT_REDEFINED, obj);
        else
            CScope_AddGlobalObject(obj);
        p->obj = obj;
        memclrw(local14, 4);
        block[0] = 0;
        block[1] = (SInt32)id;
        block[2] = 0;
        block[3] = 0;
        fn_004ceab0(obj, local14, block, obj->type->size);
    }
    return p->obj;
}

HashEntry *CObjCModern_RegisterMethodSelector(MethRec *method)
{
    ObjCParameterNode *parameter;
    HashNameNode *name;
    HashEntry *record;
    HashEntry **bucket;
    SelectorMethod *methodNode;

    if (method->args->next == NULL && method->args->type == NULL) {
        name = method->args->selectorName;
    } else {
        data_00583548.size = 0;
        for (parameter = method->args; parameter != NULL; parameter = parameter->next) {
            if (parameter->selectorName != NULL)
                CompilerTools_AppendGListString(&data_00583548, parameter->selectorName->name);
            AppendGListByte(&data_00583548, ':');
        }
        AppendGListByte(&data_00583548, 0);
        fn_00443190(data_00583548.data);
        name = GetHashNameNode(*data_00583548.data);
        fn_004431b0(data_00583548.data);
    }

    if ((record = FindHashNode(name)) == NULL) {
        HashEntry *newRecord;
        methodNode = galloc(sizeof(SelectorMethod));
        methodNode->next = NULL;
        methodNode->method = method;
        if (selector_hash == NULL) {
            selector_hash = galloc(1024 * sizeof(*selector_hash));
            memclrw(selector_hash, 1024 * sizeof(*selector_hash));
        }
        newRecord = galloc(sizeof(HashEntry));
        newRecord->obj = NULL;
        newRecord->name = name;
        newRecord->methods = NULL;
        bucket = &selector_hash[name->hashval & 0x3ff];
        newRecord->next = *bucket;
        *bucket = newRecord;
        record = newRecord;
        newRecord->methods = methodNode;
    } else {
        for (methodNode = record->methods; methodNode != NULL; methodNode = methodNode->next) {
            if (CObjCModern_CompareMethRecs(methodNode->method, method))
                break;
        }
        if (methodNode == NULL) {
            methodNode = galloc(sizeof(SelectorMethod));
            methodNode->method = method;
            methodNode->next = record->methods;
            record->methods = methodNode;
        }
    }

    method->selector = record;
    return record;
}

Boolean CObjCModern_CompareMethRecs(struct MethRec *left, struct MethRec *right)
{
    ObjCParameterNode *leftEntry;
    ObjCParameterNode *rightEntry;

    if (SamePtr(left->rtype, right->rtype) && left->rqual == right->rqual && left->isvararg == right->isvararg) {
        leftEntry = left->args;
        rightEntry = right->args;
        for (;;) {
            if (leftEntry == NULL)
                return rightEntry == NULL;
            if (rightEntry == NULL)
                return 0;
            if (leftEntry->selectorName != rightEntry->selectorName || leftEntry->qual != rightEntry->qual)
                return 0;
            if (leftEntry->type != NULL) {
                if (rightEntry->type == NULL || !SamePtr(rightEntry->type, rightEntry->type))
                    return 0;
            } else if (rightEntry->type != NULL) {
                return 0;
            }
            leftEntry = leftEntry->next;
            rightEntry = rightEntry->next;
        }
    }
    return 0;
}

HashEntry *CObjCModern_FindMessageArgumentHashEntry(struct MessageArgument *p)
{
    HashNameNode *node;
    HashEntry *e;

    if (p->next == NULL && p->expression == NULL) {
        node = p->name;
    } else {
        data_00583548.size = 0;
        while (p != NULL) {
            if (p->name != NULL)
                CompilerTools_AppendGListString(&data_00583548, p->name->name);
            AppendGListByte(&data_00583548, 0x3a);
            p = p->next;
        }
        AppendGListByte(&data_00583548, 0);
        fn_00443190(data_00583548.data);
        node = GetHashNameNode(*data_00583548.data);
        fn_004431b0(data_00583548.data);
    }

    if (selector_hash == NULL) {
        e = NULL;
    } else {
        e = selector_hash[node->hashval & 0x3ff];
        while (e != NULL) {
            if (e->name == node)
                break;
            e = e->next;
        }
    }
    return e;
}

Object *fn_00509c40(char *name, short kind)
{
    int length;
    Object *object;
    NamedObjectCacheEntry *entry;
    NamedObjectCacheEntry *cached;

    cached = named_object_cache;
    while (cached != NULL) {
        if (cached->kind == kind) {
            length = strcmp(name, (char *)cached->name);
            if (length == 0) {
                return cached->object;
            }
        }
        cached = cached->next;
    }
    object = CParser_NewCompilerDefDataObject();
    object->nspace = registration_context;
    object->name = CParser_GetUniqueName();
    length = strlen(name);
    object->type = CDecl_NewArrayType((Type *)&stchar, length + 1);
    object->sclass = TK_STATIC;
    object->section = kind;
    fn_004ceab0(object, name, NULL, object->type->size);
    entry = (NamedObjectCacheEntry *)galloc(sizeof(NamedObjectCacheEntry));
    entry->next = named_object_cache;
    named_object_cache = entry;
    entry->name = (UInt8 *)name;
    entry->kind = kind;
    entry->object = object;
    return entry->object;
}

char *CObjCModern_ConcatStrings(char *first, char *second, char *third)
{
    unsigned int size;
    unsigned int offset;
    char *buffer;

    size = 1;
    if (first)
        size += strlen(first);
    if (second)
        size += strlen(second);
    if (third)
        size += strlen(third);

    buffer = galloc(size);

    offset = 0;
    if (first) {
        strcpy(buffer, first);
        offset += strlen(first);
    }
    if (second) {
        strcpy(buffer + offset, second);
        offset += strlen(second);
    }
    if (third) {
        strcpy(buffer + offset, third);
        offset += strlen(third);
    }
    buffer[offset] = '\0';
    return buffer;
}

void fn_00509df0(void)
{
    return;
}

void CObjCModern_ResetGlobals(void)
{
    class_pointer_type = NULL;
    id_type = NULL;
    sel_type = NULL;
    data_00587140 = NULL;
    selector_hash = NULL;
    named_object_cache = NULL;
    class_type_entries = NULL;
    data_00588064 = NULL;
    category_definitions = NULL;
    selector_reference_count = 0;
    data_00587f6c = 0;
    objc_string_constant_count = 0;
    data_00588507 = 0;
}
