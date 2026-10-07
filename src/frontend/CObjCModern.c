#define CERROR_FILE "CObjCModern.c"
#include "compiler/common.h"
#include "compiler/CObjCModern.h"
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
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
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
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include <string.h>
#include <stdio.h>
static HashEntry *FindHashNode(HashNameNode *name)
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

static Boolean SamePtr(void *x, void *y)
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
        object = entry->object.value;
        if (object->type->type == TYPEFUNC)
            return object;
        CError_ReportError(ERR_IDENTIFIER_REDECLARED, name);
    }
    copts.cplusplus = 0;
    savedContext = currentNameSpace;
    currentNameSpace = registration_context;
    newObject = CParser_NewFunctionObject(NULL);
    currentNameSpace = savedContext;
    newObject->type = &data_0055d5e8;
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
        obj->extraQualifiers = 0xb;
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
    object->extraQualifiers = kind;
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

ENode *CObjCModern_MakeDeallocMessage(TypeClass *type, ENode *object)
{
    MessageArgument *record;

    record = (MessageArgument *)CompilerTools_AllocatePool(sizeof(MessageArgument));
    memclrw(record, sizeof(MessageArgument));
    record->name = GetHashNameNodeExport("dealloc");
    return CObjC_MakeMessageSend(object, type, record, NULL, 0, '\0');
}

ENode *CObjCModern_CreateAllocMessage(TypeClass *object)
{
    ENode *result;
    struct MessageArgument *name;
    name = (struct MessageArgument *)CompilerTools_AllocatePool(12U);
    memclrw(name, 12U);
    name->name = GetHashNameNodeExport("alloc");
    result = create_objectrefnode(object->objcinfo->classobject);
    return CObjC_MakeMessageSend(result, object, name, NULL, 1, 0);
}

ENode *CObjCModern_TryParseMethodCall(TypeClass *a, ENode *b)
{
    SInt32 state;
    HashNameNode *name;

    CPrep_GetBufferedTokenPosition(&state);
    tk = CPrepTokenizer_GetNextToken();
    CObjC_ConvertKeywordToIdentifier();
    if (tk == TK_IDENTIFIER) {
        if (!strcmp(data_00587fa0->name, "super")) {
            if (CPrepTokenizer_GetNextToken() == 0x174) {
                tk = CPrepTokenizer_GetNextToken();
                CObjC_ConvertKeywordToIdentifier();
                name = data_00587fa0;
                if (tk == TK_IDENTIFIER && CPrepTokenizer_GetNextToken() == '(')
                    return CObjCModern_0050a000(a, b, name, 1);
            }
        } else {
            name = data_00587fa0;
            if (CPrepTokenizer_GetNextToken() == '(')
                return CObjCModern_0050a000(a, b, name, 0);
        }
    }
    CPrep_SetBufferedTokenPosition(&state);
    return NULL;
}

ENode *CObjCModern_0050a000(TypeClass *context, ENode *receiver, HashNameNode *name, char option)
{
    MessageArgument *args;
    MessageArgument *arg;
    ENodeList *extra;
    ENode *result;
    char argumentFlag;

    CError_ASSERT(89, receiver->type == EINDIRECT);
    receiver = receiver->data.monadic;
    args = CABI_SplitNameIntoMessageArguments(name, &argumentFlag);
    tk = CPrepTokenizer_GetNextToken();
    if (tk == ')') {
        if (argumentFlag == 0) {
            CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
            return nullnode();
        }
        result = CObjC_MakeMessageSend(receiver, context, args, NULL, 1, option);
        tk = CPrepTokenizer_GetNextToken();
        return result;
    }
    if (argumentFlag != 0) {
        CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
        return nullnode();
    }
    extra = NULL;
    arg = args;
    for (;;) {
        arg->expression = assignment_expression();
        if (tk == ')') {
            if (arg->next != NULL) {
                CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
                return nullnode();
            }
            break;
        }
        if (tk != ',') {
            CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
            return nullnode();
        }
        tk = CPrepTokenizer_GetNextToken();
        arg = arg->next;
        if (arg != NULL)
            continue;
        extra = CExpr_ScanExpressionList(0);
        break;
    }
    result = CObjC_MakeMessageSend(receiver, context, args, extra, 1, option);
    tk = CPrepTokenizer_GetNextToken();
    return result;
}
