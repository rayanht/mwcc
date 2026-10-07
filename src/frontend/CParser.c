#define CERROR_FILE "CParser.c"
#include "compiler/common.h"
#include "compiler/CParser.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/CABI.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CodeGen.h"
#include "compiler/StackFrameEABI.h"
#include "driver/COSToolsCLT.h"
#include <string.h>
#define va_start(ap, last) (ap = (va_list)((char *)&(last) + (((char *)(&(last) + 1) - (char *)&(last)) + 3) / 4 * 4))
#define va_arg(ap, type) (*(type *)((ap += sizeof(type)) - sizeof(type)))

Type data_0055d5c0 = {TYPETEMPLDEPEXPR, 0};
Type type_placeholder = {0xFF, 1};
Type stvoid = {TYPEVOID, 0};
TypePointer void_ptr = {TYPEPOINTER, 4, &stvoid, 0};
TypeFunc data_0055d5e8 = {TYPEFUNC, 0, NULL, NULL, &stvoid, 0, 0};

static int data_00580dc0;
static struct ClassTypeLink *class_type_links;
static struct CParseRec *class_parse_recs;
static struct CParseCacheNode *single_expr_functions;

static void restore(ParserTryBlock *s);
static void save(ParserTryBlock *s, unsigned char *flag);
static void CParser_RestoreState(ParserTryBlock *sv);
static void CParser_SaveState(ParserTryBlock *sv);
static Boolean IsAnonymousUnion(DeclInfo *context);
static int IsUnionType(DeclInfo *context);
static Boolean IsAnonymousName(HashNameNode *name);
static int IsTempName(HashNameNode *name);

static inline Object *fn_0048be40_inline1(TypeFunc *functionType, NameSpaceObjectList *result)
{
    NameSpaceObjectList *candidate = result;
    while (candidate != NULL) {
        if (candidate->object->otype == OT_OBJECT && ((Object *)candidate->object)->type->type == TYPEFUNC &&
            CParser_CompareArgLists(functionType->args, ((TypeFunc *)((Object *)candidate->object)->type)->args) == 1)
            return (Object *)candidate->object;
        candidate = candidate->next;
    }
    return NULL;
}

static inline Boolean CheckVectorKeyword(void)
{
    HashNameNode *savedid;
    SInt16 t;
    Boolean bVar3;
    savedid = data_00587fa0;
    t = CPrepTokenizer_GetNextTokenAndRestorePosition();
    switch (t) {
        case 0x107:
        case 0x108:
        case 0x109:
        case 0x10a:
        case 0x10b:
        case 0x10d:
        case 0x10e:
        case 0x11c:
            bVar3 = 1;
            break;
        case -3:
            if (strcmp(data_00587fa0->name, "bool") == 0 || strcmp(data_00587fa0->name, "pixel") == 0 ||
                strcmp(data_00587fa0->name, "__pixel") == 0) {
                bVar3 = 1;
                break;
            }
        default:
            data_00587fa0 = savedid;
            bVar3 = 0;
    }
    return bVar3;
}

static inline Boolean CheckClassAccess(void *node)
{
    TemplClass *templateClass = node;
    NameSpace *qv;
    if (templateClass->templ__params != NULL) {
        for (qv = cscope_current;; qv = qv->parent) {
            if (qv == NULL) {
                CError_ReportError(ERR_LESS_EXPECTED);
                return 0;
            }
            if (qv->theclass == node)
                break;
        }
    }
    return 1;
}

static inline Boolean CParser_AlternateFunctionNamesEnabled(void)
{
    return copts.array_new_delete;
}

static inline TypeFunc *function_type(Type *type)
{
    return (TypeFunc *)type;
}

Object *CParser_NewRTFunc(Type *returnType, HashNameNode *name, char mangleName, int argumentCount, ...)
{
    FuncArg *current;
    FuncArg *arguments;
    va_list ap;
    FuncArg *argument;
    TypeFunc *function;
    Object *result;
    arguments = NULL;
    if (argumentCount != 0) {
        va_start(ap, argumentCount);
        for (argumentCount = argumentCount - 1; argumentCount >= 0; argumentCount = argumentCount - 1) {
            if (arguments != NULL) {
                argument = galloc(sizeof(FuncArg));
                memclrw(argument, sizeof(FuncArg));
                current = current->next = argument;
            } else {
                current = galloc(sizeof(FuncArg));
                memclrw(current, sizeof(FuncArg));
                arguments = current;
            }
            current->type = va_arg(ap, Type *);
        }
    }
    result = CParser_NewFunctionObject(NULL);
    function = galloc(sizeof(TypeFunc));
    memclrw(function, sizeof(TypeFunc));
    function->type = TYPEFUNC;
    function->functype = returnType;
    function->args = arguments;
    fn_00504240(function);
    result->name = name;
    result->type = (Type *)function;
    if (mangleName == '\x01') {
        result->qual = Q_MANGLE_NAME;
    }
    CodeGen_SetObjectSectionAndInterruptInfo(result);
    return result;
}

Boolean CParser_IsPublicRuntimeObject(ObjBase *object)
{
    if (runtime_operator_namespace_name->first.object == object && runtime_operator_namespace_name->first.next == NULL)
        return 1;
    if (data_00588008->first.object == object && data_00588008->first.next == NULL)
        return 1;
    if (data_00587680->first.object == object && data_00587680->first.next == NULL)
        return 1;
    if (data_00587e64->first.object == object && data_00587e64->first.next == NULL)
        return 1;
    return CodeGen_IsRegisteredObject(object);
}

static void *GetF8(NameSpace *r)
{
    return r->usings;
}

Boolean CParser_ReInitRuntimeObjects(Boolean flag)
{
    if ((runtime_operator_namespace_name = CScope_FindNameSpaceName(cscope_root, CMangler_OperatorName(0x147))) == NULL)
        return 0;
    if ((data_00588008 = CScope_FindNameSpaceName(cscope_root, CMangler_OperatorName(0x182))) == NULL)
        return 0;
    if ((data_00587680 = CScope_FindNameSpaceName(cscope_root, CMangler_OperatorName(0x145))) == NULL)
        return 0;
    if ((data_00587e64 = CScope_FindNameSpaceName(cscope_root, CMangler_OperatorName(0x183))) == NULL)
        return 0;
    newh_func->name = GetHashNameNode("__new_hdl");
    data_00587ed0->name = GetHashNameNode("__del_hdl");
    data_005870d8->name = GetHashNameNode("__copy");
    typeid_func->name = GetHashNameNode("__get_typeid");
    dynamic_cast_object->name = GetHashNameNode("__dynamic_cast");
    cast_member_pointer_func->name = GetHashNameNode("__ptmf_cast");
    rt_memberpointercompare->name = GetHashNameNode("__ptmf_cmpr");
    memberpointercompare_func->name = GetHashNameNode("__ptmf_test");
    data_0058769c->name = GetHashNameNode("__ptmf_call");
    data_00587fd0->name = GetHashNameNode("__ptmf_scall");
    data_00587f80->name = GetHashNameNode("__ptmf_call4");
    member_function_pointer_call_rtfunc->name = GetHashNameNode("__ptmf_scall4");
    data_00587678->name = GetHashNameNode("__ptmf_null");
    data_00588060->name = GetHashNameNode("__som_new");
    data_005876c0->name = GetHashNameNode("__som_check_new");
    data_00588278->name = GetHashNameNode("__som_check_ev");
    data_00588260->name = GetHashNameNode("_som_ptrgl4");
    som_ref_node_rtfunc->name = GetHashNameNode("_som_ptrgl5");
    som_ref_node_runtime_object->name = GetHashNameNode("_som_ptrgl_");
    class_array_initializer->name = GetHashNameNode("__construct_array");
    array_allocation_runtime_function->name = GetHashNameNode("__construct_new_array");
    data_0058717c->name = GetHashNameNode("__destroy_arr");
    destructor_aware_call_rtfunc->name = GetHashNameNode("__destroy_new_array");
    destructor_aware_call_func->name = GetHashNameNode("__destroy_new_array2");
    destructor_registration_func->name = GetHashNameNode("__register_global_object");
    throw_func->name = GetHashNameNode("__throw");
    data_005882a4->name = GetHashNameNode("__init__catch");
    data_005875a0->name = GetHashNameNode("__end__catch");
    data_00587654->name = GetHashNameNode("__unexpected");
    CMangler_Setup();
    unnamed_name = GetHashNameNode("@no_name@");
    blank_argument_name = GetHashNameNode("@temp_ptr@");
    this_arg_name = GetHashNameNode("this");
    this_self_name = GetHashNameNode("self");
    vtable_name = GetHashNameNode("__vptr$");
    CSOM_Init(flag);
    fn_00434660(flag);
}

void initialize_runtime_objects(void)
{
    Type *sizeType;
    ExceptSpecList *emptyExceptionSpecs;
    Object *runtimeObject;
    TypeFunc *functionType;

    emptyExceptionSpecs = (ExceptSpecList *)galloc(sizeof(*emptyExceptionSpecs));
    memclrw(emptyExceptionSpecs, sizeof(*emptyExceptionSpecs));
    sizeType = CABI_GetSizeTType();

    runtimeObject = CParser_NewRTFunc((Type *)&void_ptr, CMangler_OperatorName(TK_NEW), 1, 1, sizeType);
    CScope_AddGlobalObject(runtimeObject);
    runtimeObject = CParser_NewRTFunc((Type *)&void_ptr, CMangler_OperatorName(0x182), 1, 1, sizeType);
    CScope_AddGlobalObject(runtimeObject);
    runtimeObject = CParser_NewRTFunc(&stvoid, CMangler_OperatorName(TK_DELETE), 1, 1, &void_ptr);
    if (runtimeObject->type->type != TYPEFUNC)
        CError_FATAL(368);
    functionType = (TypeFunc *)runtimeObject->type;
    functionType->exspecs = emptyExceptionSpecs;
    CScope_AddGlobalObject(runtimeObject);
    runtimeObject = CParser_NewRTFunc(&stvoid, CMangler_OperatorName(0x183), 1, 1, &void_ptr);
    if (runtimeObject->type->type != TYPEFUNC)
        CError_FATAL(376);
    functionType = (TypeFunc *)runtimeObject->type;
    functionType->exspecs = emptyExceptionSpecs;
    CScope_AddGlobalObject(runtimeObject);

    newh_func = CParser_NewRTFunc((Type *)&void_ptr, NULL, 0, 1, sizeType);
    data_00587ed0 = CParser_NewRTFunc(&stvoid, NULL, 0, 1, &void_ptr);
    typeid_func = CParser_NewRTFunc((Type *)&void_ptr, NULL, 0, 2, &void_ptr, &stsignedlong);
    dynamic_cast_object = CParser_NewRTFunc((Type *)&void_ptr, NULL, 0, 5, &void_ptr, &stsignedlong, &void_ptr,
                                            &void_ptr, &stsignedshort);
    data_005870d8 = CParser_NewRTFunc((Type *)&void_ptr, NULL, 2, 3, &void_ptr, &void_ptr, sizeType);
    cast_member_pointer_func = CParser_NewRTFunc((Type *)&void_ptr, NULL, 2, 3, &stsignedlong, &void_ptr, &void_ptr);
    rt_memberpointercompare = CParser_NewRTFunc((Type *)&stsignedlong, NULL, 2, 2, &void_ptr, &void_ptr);
    memberpointercompare_func = CParser_NewRTFunc((Type *)&stsignedlong, NULL, 2, 1, &void_ptr);
    data_0058769c = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    data_00587fd0 = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    data_00587f80 = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    member_function_pointer_call_rtfunc = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    data_00587678 = CParser_NewGlobalDataObject(NULL);
    data_00587678->type = &stvoid;
    data_00588060 = CParser_NewRTFunc((Type *)&void_ptr, NULL, 2, 3, &void_ptr, &stsignedlong, &stsignedlong);
    data_005876c0 = CParser_NewRTFunc(&stvoid, NULL, 0, 1, &void_ptr);
    data_00588278 = CParser_NewRTFunc(&stvoid, NULL, 0, 1, &void_ptr);
    data_00588260 = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    som_ref_node_rtfunc = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    som_ref_node_runtime_object = CParser_NewRTFunc(&stvoid, NULL, 2, 0);
    class_array_initializer =
        CParser_NewRTFunc(&stvoid, NULL, 0, 5, &void_ptr, &void_ptr, &void_ptr, sizeType, sizeType);
    array_allocation_runtime_function =
        CParser_NewRTFunc((Type *)&void_ptr, NULL, 0, 5, &void_ptr, &void_ptr, &void_ptr, sizeType, sizeType);
    data_0058717c = CParser_NewRTFunc(&stvoid, NULL, 0, 4, &void_ptr, &void_ptr, sizeType, sizeType);
    destructor_aware_call_rtfunc = CParser_NewRTFunc(&stvoid, NULL, 0, 2, &void_ptr, &void_ptr);
    destructor_aware_call_func = CParser_NewRTFunc((Type *)&void_ptr, NULL, 0, 2, &void_ptr, &void_ptr);
    destructor_registration_func = CParser_NewRTFunc((Type *)&void_ptr, NULL, 0, 3, &void_ptr, &void_ptr, &void_ptr);
    throw_func = CParser_NewRTFunc(&stvoid, NULL, 0, 3, &void_ptr, &void_ptr, &void_ptr);
    data_005882a4 = CParser_NewRTFunc(&stvoid, NULL, 0, 1, &void_ptr);
    data_005875a0 = CParser_NewRTFunc(&stvoid, NULL, 0, 1, &void_ptr);
    data_00587654 = CParser_NewRTFunc(&stvoid, NULL, 0, 1, &void_ptr);

    CodeGen_InitializeLists();
    if (!CParser_ReInitRuntimeObjects(0))
        CError_FATAL(508);
}

void CParser_Setup(void)
{
    CScope_Setup();
    data_00583548.data = NULL;
    if (InitGList(&data_00583548, 0x100) != 0) {
        CError_LongJump();
    }
    fn_00449dc0();
    CInit_Init();
    CClass_Init();
    fn_0051b810();
    CObjCModern_ResetGlobals();
    fn_00514220();
    data_005884fd = 0;
    in_parameter_type_list = 0;
    data_00580dc0 = 1;
    data_0058852e = 0;
    copts.sideeffects = 1;
    class_type_links = NULL;
    data_00587fd8 = NULL;
    pending_object_classes = NULL;
    pending_functions = NULL;
    single_expr_functions = NULL;
    trychain = NULL;
    cached_objects = NULL;
    class_parse_recs = NULL;
    memclrw(&function_fileinfo, sizeof(function_fileinfo));
    memclrw(&exception_temp_object_type, sizeof(TypeStruct));
    exception_temp_object_type.type = TYPESTRUCT;
    _DAT_0058843e = 0x18;
    data_0058844a = 0;
    _DAT_0058844c = 4;
    memclrw(&data_0058847c, sizeof(TypeStruct));
    data_0058847c.type = TYPESTRUCT;
    _DAT_0058847e = 0xc;
    data_0058848a = 0;
    _DAT_0058848c = 4;
    fn_004a9c70();
    CTemplateNew_Reset();
    non_type_template_argument_mode = 0;
    initialize_runtime_objects();
}

void CParser_Cleanup(void)

{
    fn_004f0000();
    fn_0051b800();
    fn_00509df0();
    CScope_Cleanup();
    FreeGList(&data_00583548);
    return;
}

SInt16 GetPrec(short token)
{
    switch (token) {
        case '%':
        case '*':
        case '/':
            return 11;
        case '+':
        case '-':
            return 10;
        case 0x16c:
        case 0x16d:
            return 9;
        case '<':
        case '>':
        case 0x16a:
        case 0x16b:
            return 8;
        case 0x168:
        case 0x169:
            return 7;
        case '&':
            return 6;
        case '^':
            return 5;
        case '|':
            return 4;
        case 0x167:
            return 3;
        case 0x166:
            return 2;
    }
    return 0;
}

Boolean CParser_ParseOperatorName(SInt16 *operatorToken, Boolean allowConversion)
{
    HashNameNode *name;
    DeclInfo nameData;

    tk = CPrepTokenizer_GetNextToken();
    switch (tk) {
        case TK_DELETE:
        case TK_NEW:
            if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x5b) {
                CPrepTokenizer_GetNextToken();
                if (CPrepTokenizer_GetNextToken() != 0x5d)
                    CError_ReportError(ERR_RBRACKET_EXPECTED);
                tk = (tk == TK_NEW) ? 0x182 : 0x183;
            }
            break;
        case '(':
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ')') {
                CError_ReportError(ERR_ILLEGAL_OPERATOR);
                return 0;
            }
            tk = '(';
            break;
        case '[':
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ']') {
                CError_ReportError(ERR_ILLEGAL_OPERATOR);
                return 0;
            }
            tk = '[';
            break;
    }
    name = CMangler_OperatorName(tk);
    if (name != NULL) {
        if (operatorToken != NULL)
            *operatorToken = tk;
        tk = CPrepTokenizer_GetNextToken();
        data_00587fa0 = name;
        return 1;
    }
    if (allowConversion) {
        memclrw(&nameData, sizeof(nameData));
        conversion_type_name(&nameData);
        data_00587fa0 = CMangler_ConversionFuncName(nameData.thetype, nameData.qual);
        if (operatorToken != NULL)
            *operatorToken = 0;
        return 1;
    }
    CError_ReportError(ERR_ILLEGAL_OPERATOR);
    return 0;
}

SInt32 CParser_GetUniqueID(void)
{
    SInt32 lift_value_0;
    lift_value_0 = data_00580dc0;
    data_00580dc0 += 1;
    return lift_value_0;
}

#pragma auto_inline off
void CParser_PrintUniqueID(char *p)
{
    char tmp[16];
    char *q;
    long n;
    long id;

    q = tmp;
    id = data_00580dc0++;
    n = id;
    while (n) {
        *q++ = '0' + n % 10;
        n /= 10;
    }
    while (q > tmp)
        *p++ = *--q;
    *p = 0;
}
#pragma auto_inline reset

unsigned int CParser_SetUniqueID(unsigned int value)
{
    data_00580dc0 = value;
    return value;
}

HashNameNode *CParser_GetUniqueName(void)
{
    char buf[16];
    char tmp[16];
    char *q;
    char *p;
    SInt32 n;
    SInt32 id;

    buf[0] = '@';
    p = buf;
    q = tmp;
    p++;
    id = data_00580dc0++;
    n = id;
    while (n) {
        *q++ = '0' + n % 10;
        n /= 10;
    }
    while (q > tmp)
        *p++ = *--q;
    *p = 0;
    return GetHashNameNode(buf);
}

HashNameNode *CParser_NameConcat(char *first, char *second)
{
    char buffer[256];
    char *name;
    char *dest;
    unsigned int length;

    length = strlen(first) + strlen(second);
    if (length > 255U)
        dest = name = (char *)lalloc(length + 1);
    else
        dest = name = buffer;

    while (*first)
        *dest++ = *first++;
    while (*second)
        *dest++ = *second++;
    *dest = 0;

    return GetHashNameNode(name);
}

HashNameNode *CParser_AppendUniqueName(char *name)
{
    char buf[256];
    char tmp[16];
    char *q;
    char *p;
    long n;
    long id;
    int i;
    char c;

    p = buf;
    for (i = 0; (c = *name) != 0 && i < 0xf0; i++) {
        *p = c;
        name++;
        p++;
    }
    *p = '$';
    p++;
    q = tmp;
    id = data_00580dc0++;
    n = id;
    while (n) {
        *q++ = '0' + n % 10;
        n /= 10;
    }
    while (q > tmp)
        *p++ = *--q;
    *p = 0;
    return GetHashNameNode(buf);
}

HashNameNode *CParser_AppendUniqueNameFile(char *prefix)
{
    int i;
    char *d;
    int len;
    char *p_s;
    int n;
    int v;
    int id;
    char c;
    char *p;
    char name[256];
    char buf[256];
    char num[16];
    char *q;

    d = buf;
    len = 0;
    while (*prefix != 0 && len < 200) {
        *d++ = *prefix++;
        len++;
    }
    *d++ = '$';
    q = d;
    p = num;
    id = data_00580dc0++;
    v = id;
    while (v) {
        *p++ = v % 10 + '0';
        v /= 10;
    }
    while (p > num)
        *q++ = *--p;
    *q = 0;
    while (*d != 0) {
        d++;
        len++;
    }
    COS_FileGetFSSpecInfo(&((CPrepCU *)cprep_cu)->mainFile, NULL, NULL, (UInt8 *)name);
    n = (UInt8)name[0];
    p_s = name + 1;
    i = 0;
    while (i < n && len < 0xff) {
        c = *p_s++;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
            c = '_';
        *d++ = c;
        i++;
        len++;
    }
    *d = 0;
    return GetHashNameNode(buf);
}

Boolean CParser_IsNullOrAtOrDollarPrefixedName(HashNameNode *name)
{
    unsigned int result = 1U;
    unsigned int matches = result;
    if (name && name->name[0] != 64)
        matches = 0U;
    if (!matches && name->name[0] != 36)
        result = 0U;
    return result;
}

void fn_00490210(Object *object, volatile DeclInfo *record)
{
    if (record != NULL) {
        UInt8 attributes = record->exportflags;
        if (attributes != 0) {
            object->flags |= attributes;
        }
    }
    if (object->datatype == DDATA) {
        if (copts.cfm_export != 0) {
            object->flags |= OBJECT_EXPORT;
        }
        if (copts.cfm_internal != 0) {
            object->flags |= OBJECT_INTERNAL;
        }
        return;
    } else {
        if (copts.cfm_internal != 0) {
            object->flags |= OBJECT_INTERNAL;
            return;
        }
        if (copts.cfm_import != 0) {
            object->flags |= OBJECT_IMPORT;
        }
        if (copts.cfm_export != 0) {
            object->flags |= OBJECT_EXPORT;
        }
        if (copts.cfm_lib_export != 0) {
            object->flags |= OBJECT_IMPORT | OBJECT_EXPORT;
        }
    }
}

void CParser_UpdateObject(Object *object, volatile DeclInfo *record)
{
    if (record && record->section)
        object->section = record->section;
    fn_00490210(object, record);
    CodeGen_SetObjectSectionAndInterruptInfo(object);
}

Object *CParser_NewObject(struct DeclInfo *record)
{
    Object *object;

    object = (Object *)galloc(sizeof(*object));
    memclrw(object, sizeof(*object));
    fn_00490210(object, record);
    object->otype = OT_OBJECT;
    object->access = ACCESSPUBLIC;
    object->section = 0;
    return object;
}

Object *CParser_NewLocalDataObject(DeclInfo *declaration, unsigned int addToList)
{
    Object *object;
    ObjectList *entry;
    object = (Object *)lalloc(54U);
    memclrw(object, 54U);
    object->otype = OT_OBJECT;
    object->access = 0U;
    object->datatype = DLOCAL;
    if (declaration != NULL) {
        object->type = declaration->thetype;
        object->name = declaration->name;
        object->qual = declaration->qual;
        object->sclass = declaration->storageclass;
    }
    if ((unsigned char)addToList != 0U) {
        entry = (ObjectList *)lalloc(8U);
        entry->object = object;
        entry->next = locals;
        locals = entry;
    }
    return object;
}

Object *CParser_NewGlobalDataObject(DeclInfo *declaration)
{
    Object *object;
    volatile DeclInfo *alignmentDeclaration = declaration;

    object = (Object *)galloc(sizeof(Object));
    memclrw(object, sizeof(Object));
    object->otype = OT_OBJECT;
    object->access = ACCESSPUBLIC;
    object->section = 0;
    object->datatype = DDATA;
    object->nspace = cscope_current;
    if (declaration != NULL) {
        object->type = declaration->thetype;
        object->name = declaration->name;
        object->qual = declaration->qual;
        object->sclass = declaration->storageclass;
        if (copts.cplusplus && !declaration->requireMangledName) {
            object->qual |= Q_MANGLE_NAME;
        }
    }
    if (declaration != NULL && declaration->section != 0) {
        object->section = alignmentDeclaration->section;
    }
    fn_00490210(object, declaration);
    CodeGen_SetObjectSectionAndInterruptInfo(object);
    return object;
}

Object *CParser_NewCompilerDefDataObject(void)
{
    Object *object;

    object = (Object *)galloc(sizeof(Object));
    memclrw(object, sizeof(Object));
    object->otype = OT_OBJECT;
    object->access = ACCESSPUBLIC;
    object->section = 0;
    object->datatype = DDATA;
    object->nspace = cscope_root;
    return object;
}

Object *CParser_NewFunctionObject(volatile DeclInfo *decl)
{
    Object *obj;

    obj = (Object *)galloc(sizeof(Object));
    memclrw(obj, sizeof(Object));
    obj->otype = OT_OBJECT;
    obj->access = ACCESSPUBLIC;
    obj->section = 0;
    obj->datatype = DFUNC;
    obj->nspace = cscope_current;
    if (decl != NULL) {
        obj->type = decl->thetype;
        obj->name = decl->name;
        obj->qual = decl->qual;
        obj->sclass = decl->storageclass;
        if (copts.cplusplus && decl->requireMangledName == 0)
            obj->qual |= Q_MANGLE_NAME;
    }
    if (decl != NULL) {
        if (decl->section != 0)
            obj->section = decl->section;
    }
    fn_00490210(obj, decl);
    CodeGen_SetObjectSectionAndInterruptInfo(obj);
    return obj;
}

Object *CParser_NewCompilerDefFunctionObject(void)
{
    Object *object;

    object = (Object *)galloc(sizeof(Object));
    memclrw(object, sizeof(Object));
    object->otype = OT_OBJECT;
    object->access = ACCESSPUBLIC;
    object->section = 0;
    object->datatype = DFUNC;
    object->nspace = cscope_root;
    return object;
}

Object *CParser_NewAliasObject(Object *object, int offset)
{
    Object *alias;

    alias = (Object *)galloc(sizeof(Object));
    *alias = *object;
    alias->datatype = DALIAS;
    alias->u.alias.object = object;
    alias->u.alias.member = NULL;
    alias->u.alias.offset = offset;
    CScope_AddObject(cscope_current, alias->name, (ObjBase *)alias);
    return alias;
}

FuncArg *CParser_NewFuncArg(void)
{
    FuncArg *storage;

    storage = galloc(24);
    memclrw(storage, 24);
    return storage;
}

TypeIntegral *atomtype(void)
{
    switch (token_value_kind_or_string_length) {
        default:
            CError_FATAL(1060);
        case 1:
            return (TypeIntegral *)&stvoid;
        case 2:
            return &stchar;
        case 4:
            return &stwchar;
        case 3:
            return &stunsignedchar;
        case 5:
            return &stsignedshort;
        case 6:
            return &stunsignedshort;
        case 7:
            return &stsignedint;
        case 8:
            return &stunsignedint;
        case 9:
            return &stsignedlong;
        case 10:
            return &stunsignedlong;
        case 11:
            return &stsignedlonglong;
        case 12:
            return &stunsignedlonglong;
        case 13:
            return &stfloat;
        case 14:
            return &stshortdouble;
        case 15:
            return &stdouble;
        case 16:
            return &stlongdouble;
    }
}

Object *CParser_FindDeallocationObject(Type *ownerType, Boolean useAlternate, Boolean skipLookup)
{
    Boolean memberFound = 0;
    Object *object;
    NameResult result;

    if (!skipLookup && ownerType->type == TYPECLASS) {
        NameSpaceName *name = (useAlternate && CParser_AlternateFunctionNamesEnabled()) ? data_00587e64 : data_00587680;
        if (CScope_FindClassMemberObject(TYPE_CLASS(ownerType), &result, name->name)) {
            if ((object = (Object *)result.object) == NULL) {
                CError_ASSERT(1100, result.objects != NULL);
                object = (Object *)result.objects->object;
            }
            memberFound = 1;
        } else if (TYPE_CLASS(ownerType)->flags & CLASS_HANDLEOBJECT) {
            if (useAlternate)
                CError_FATAL(1109);
            return data_00587ed0;
        }
    }
    if (!memberFound) {
        NameSpaceName *name;
        if (useAlternate && CParser_AlternateFunctionNamesEnabled())
            name = data_00587e64;
        else
            name = data_00587680;
        object = (Object *)name->first.object;
    }
    CError_ASSERT(1130, object != NULL && object->otype == OT_OBJECT && object->type->type == TYPEFUNC &&
                            TYPE_FUNC(object->type)->args != NULL &&
                            iscpp_typeequal(TYPE_FUNC(object->type)->args->type, (Type *)&void_ptr));
    return object;
}

#pragma auto_inline off
Boolean is_arglist_default_promoted(FuncArg *arg)
{
    if (copts.ignore_oldstyle)
        return 1;
    while (arg != NULL) {
        if (arg == &elipsis)
            return 0;
        switch ((SInt8)arg->type->type) {
            case TYPEINT:
                if (TYPE_INTEGRAL(arg->type)->integral < IT_INT)
                    return 0;
                break;
            case TYPEFLOAT:
                if (TYPE_INTEGRAL(arg->type)->integral < IT_DOUBLE)
                    return 0;
                break;
        }
        arg = arg->next;
    }
    return 1;
}
#pragma auto_inline reset

Boolean is_funcarg_list_same(FuncArg *left, FuncArg *right)
{
    SInt16 typesMatch;

    if (left == &oldstyle) {
        if (right == &oldstyle) {
            return 1;
        }
        return is_arglist_default_promoted(right);
    }
    if (right == &oldstyle) {
        return is_arglist_default_promoted(left);
    }
    for (;;) {
        if (left == &elipsis || right == &elipsis) {
            return 1;
        }
        if (left == NULL) {
            if (right == NULL) {
                return 1;
            }
            return 0;
        }
        if (right == NULL) {
            return 0;
        }
        if (copts.mpwc_relax != 0 && copts.cplusplus == 0) {
            typesMatch = is_typesame(left->type, right->type);
            if (typesMatch == 0) {
                return 0;
            }
        } else {
            typesMatch = iscpp_typeequal(left->type, right->type);
            if (typesMatch == 0) {
                return 0;
            }
        }
        if (left->type->type == TYPEPOINTER && left->qual != right->qual) {
            return 0;
        }
        left = left->next;
        right = right->next;
    }
}

#pragma auto_inline off
unsigned short is_memberpointerequal(Type *type, Type *other)
{
    FuncArg *args;
    FuncArg *otherArgs;

    if (type->type != other->type)
        return 0;
    if (type->type != TYPEFUNC)
        return is_typesame(type, other);

    if (!is_typesame(function_type(type)->functype, function_type(other)->functype))
        return 0;
    if ((function_type(type)->flags & 0x17000001u) != (function_type(other)->flags & 0x17000001u))
        return 0;

    args = function_type(type)->args->next;
    if (function_type(type)->flags & 0x80)
        args = args->next;
    otherArgs = function_type(other)->args->next;
    if (function_type(other)->flags & 0x80)
        otherArgs = otherArgs->next;
    return is_funcarg_list_same(args, otherArgs);
}
#pragma auto_inline reset

SInt16 is_typesame(Type *left, Type *right)
{
    SInt8 relaxed = copts.mpwc_relax;
    SInt8 cplusplus = copts.cplusplus;
    Boolean relaxedTypeChecking = relaxed;
    Boolean cppTypeChecking = cplusplus;

    for (;;) {
        if (left->type != right->type)
            return 0;
        switch ((SInt8)left->type) {
            case TYPEVOID:
                return 1;
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPECLASS:
                return left == right;
            case TYPESTRUCT:
                return left == right;
            case TYPEPOINTER:
                left = TYPE_POINTER(left)->target;
                if (left == &stvoid || (right = TYPE_POINTER(right)->target) == &stvoid)
                    return 1;
                if (relaxed && !cplusplus)
                    return 1;
                break;
            case TYPEMEMBERPOINTER:
                if (TYPE_MEMBER_POINTER(left)->ty2 != TYPE_MEMBER_POINTER(right)->ty2)
                    return 0;
                return is_memberpointerequal(TYPE_MEMBER_POINTER(left)->ty1, TYPE_MEMBER_POINTER(right)->ty1);
            case TYPEARRAY:
                if (left->size != 0 && right->size != 0 && left->size != right->size)
                    return 0;
                left = TYPE_POINTER(left)->target;
                right = TYPE_POINTER(right)->target;
                break;
            case TYPEFUNC:
                if (cppTypeChecking || !copts.cpp_extensions) {
                    if (relaxedTypeChecking && !cppTypeChecking) {
                        if (!is_typesame(TYPE_FUNC(left)->functype, TYPE_FUNC(right)->functype))
                            return 0;
                    } else if (!iscpp_typeequal(TYPE_FUNC(left)->functype, TYPE_FUNC(right)->functype))
                        return 0;
                    if ((TYPE_FUNC(left)->flags & 0x17000001) != (TYPE_FUNC(right)->flags & 0x17000001))
                        return 0;
                }
                return is_funcarg_list_same(TYPE_FUNC(left)->args, TYPE_FUNC(right)->args);
            case TYPETEMPLATE:
                return CTemplTool_TemplDepTypeCompare((TypeTemplDep *)left, (TypeTemplDep *)right);
            default:
                CError_FATAL(1276);
                return 0;
        }
    }
}

#define TYPE_ARRAY(ty) ((Type *)(ty))
SInt16 is_typeequal(Type *leftType, Type *rightType)
{
    for (;;) {
        if (leftType->type != rightType->type)
            return 0;
        switch ((SInt8)leftType->type) {
            case TYPEVOID:
                return 1;
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPECLASS:
                return leftType == rightType;
            case TYPESTRUCT:
                return leftType == rightType;
            case TYPEPOINTER:
                if (TPTR_TARGET(rightType) == &stvoid) {
                    if (TPTR_TARGET(leftType) == &stvoid)
                        return 1;
                    return -1;
                }
                if (TPTR_TARGET(leftType) == &stvoid) {
                    data_0058852b = 1;
                    return 0;
                }
                leftType = TPTR_TARGET(leftType);
                rightType = TPTR_TARGET(rightType);
                break;
            case TYPEMEMBERPOINTER:
                if (TYPE_MEMBER_POINTER(leftType)->ty2 != TYPE_MEMBER_POINTER(rightType)->ty2)
                    return 0;
                return is_memberpointerequal(TYPE_MEMBER_POINTER(leftType)->ty1, TYPE_MEMBER_POINTER(rightType)->ty1);
            case TYPEARRAY:
                if (leftType->size != 0 && rightType->size != 0 && leftType->size != rightType->size)
                    return 0;
                leftType = TPTR_TARGET(leftType);
                rightType = TPTR_TARGET(rightType);
                break;
            case TYPEFUNC:
                if (iscpp_typeequal(TYPE_FUNC(leftType)->functype, TYPE_FUNC(rightType)->functype) == 0)
                    return 0;
                if ((TYPE_FUNC(leftType)->flags & 0x17000001) != (TYPE_FUNC(rightType)->flags & 0x17000001))
                    return 0;
                return is_funcarg_list_same(TYPE_FUNC(leftType)->args, TYPE_FUNC(rightType)->args);
            case TYPETEMPLATE:
                return CTemplTool_TemplDepTypeCompare((TypeTemplDep *)leftType, (TypeTemplDep *)rightType);
            default:
                CError_FATAL(1335);
                return 0;
        }
    }
}

SInt16 CParser_CompareArgLists(FuncArg *a, FuncArg *b)
{
    Boolean flag = 0;

    if (a == &oldstyle) {
        if (b == &oldstyle)
            return 1;
        return 2;
    }
    if (b == &oldstyle)
        return 2;
    for (;;) {
        if (a == &elipsis) {
            if (b != &elipsis)
                return 0;
            break;
        }
        if (b == &elipsis)
            return 0;
        if (a == NULL) {
            if (b != NULL)
                return 0;
            break;
        }
        if (b == NULL)
            return 0;
        if (a->type->type == TYPEPOINTER) {
            if (TYPE_POINTER(a->type)->qual & Q_REFERENCE) {
                if (b->type->type == TYPEPOINTER && (TYPE_POINTER(b->type)->qual & Q_REFERENCE)) {
                    if (iscpp_typeequal(TPTR_TARGET(a->type), TPTR_TARGET(b->type)) == 0)
                        return 0;
                    if ((a->qual & Q_CV) != (b->qual & Q_CV))
                        return 0;
                } else {
                    if (iscpp_typeequal(TPTR_TARGET(a->type), b->type) == 0)
                        return 0;
                    if (b->type->type == TYPEPOINTER && (a->qual & Q_CV) != (b->qual & Q_CV))
                        return 0;
                    flag = 1;
                }
            } else {
                if (b->type->type == TYPEPOINTER) {
                    if (TYPE_POINTER(b->type)->qual & Q_REFERENCE) {
                        if (iscpp_typeequal(a->type, TPTR_TARGET(b->type)) == 0)
                            return 0;
                        if (a->type->type == TYPEPOINTER && (a->qual & Q_CV) != (b->qual & Q_CV))
                            return 0;
                        flag = 1;
                    } else {
                        if (iscpp_typeequal(TPTR_TARGET(a->type), TPTR_TARGET(b->type)) == 0)
                            return 0;
                        if ((a->qual & Q_CV) != (b->qual & Q_CV))
                            return 0;
                    }
                } else {
                    return 0;
                }
            }
        } else {
            if (b->type->type == TYPEPOINTER) {
                if (TYPE_POINTER(b->type)->qual & Q_REFERENCE) {
                    if (iscpp_typeequal(a->type, TPTR_TARGET(b->type)) == 0)
                        return 0;
                    flag = 1;
                } else {
                    return 0;
                }
            } else {
                if (iscpp_typeequal(a->type, b->type) == 0)
                    return 0;
            }
        }
        a = a->next;
        b = b->next;
    }
    if (flag)
        return 2;
    return 1;
}

Boolean is_arglistsame(FuncArg *a, FuncArg *b)
{
    if (a == &oldstyle) {
        if (b == &oldstyle)
            return 1;
        return is_arglist_default_promoted(b);
    }
    if (b == &oldstyle)
        return is_arglist_default_promoted(a);
    for (;;) {
        if (a == NULL || b == NULL || a == &elipsis || b == &elipsis)
            return a == b;
        if (a->type->type == TYPEPOINTER) {
            if (b->type->type != TYPEPOINTER || (a->qual & Q_CV) != (b->qual & Q_CV) ||
                iscpp_typeequal(TYPE_POINTER(a->type)->target, TYPE_POINTER(b->type)->target) == 0 ||
                (TYPE_POINTER(a->type)->qual & Q_REFERENCE) != (TYPE_POINTER(b->type)->qual & Q_REFERENCE))
                return 0;
        } else {
            if (iscpp_typeequal(a->type, b->type) == 0)
                return 0;
        }
        a = a->next;
        b = b->next;
    }
}

SInt16 iscpp_typeequal(Type *leftType, Type *rightType)
{
    for (;;) {
        if (leftType->type != rightType->type) {
            if (leftType->type == TYPETEMPLATE && rightType->type == TYPECLASS &&
                (TYPE_CLASS(rightType)->flags & CLASS_IS_TEMPL))
                return CTemplTool_IsSameTemplateType(TYPE_CLASS(rightType), (TypeTemplDep *)leftType);
            if (rightType->type == TYPETEMPLATE && leftType->type == TYPECLASS &&
                (TYPE_CLASS(leftType)->flags & CLASS_IS_TEMPL))
                return CTemplTool_IsSameTemplateType(TYPE_CLASS(leftType), (TypeTemplDep *)rightType);
            return 0;
        }
        switch ((SInt8)leftType->type) {
            case TYPEVOID:
                return 1;
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPECLASS:
                return leftType == rightType;
            case TYPETEMPLATE:
                return CTemplTool_TemplDepTypeCompare((TypeTemplDep *)leftType, (TypeTemplDep *)rightType);
            case TYPESTRUCT:
                return leftType == rightType;
            case TYPEPOINTER:
                if ((TYPE_POINTER(leftType)->qual & (Q_CV | Q_REFERENCE | Q_RESTRICT)) !=
                    (TYPE_POINTER(rightType)->qual & (Q_CV | Q_REFERENCE | Q_RESTRICT)))
                    return 0;
                leftType = TYPE_POINTER(leftType)->target;
                rightType = TYPE_POINTER(rightType)->target;
                break;
            case TYPEMEMBERPOINTER:
                if (!iscpp_typeequal(TYPE_MEMBER_POINTER(leftType)->ty2, TYPE_MEMBER_POINTER(rightType)->ty2))
                    return 0;
                return is_memberpointerequal(TYPE_MEMBER_POINTER(leftType)->ty1, TYPE_MEMBER_POINTER(rightType)->ty1);
            case TYPEARRAY:
                if (TYPE_POINTER(leftType)->size != 0 && TYPE_POINTER(rightType)->size != 0 &&
                    TYPE_POINTER(leftType)->size != TYPE_POINTER(rightType)->size)
                    return 0;
                leftType = TYPE_POINTER(leftType)->target;
                rightType = TYPE_POINTER(rightType)->target;
                break;
            case TYPEFUNC:
                if (!iscpp_typeequal(TYPE_FUNC(leftType)->functype, TYPE_FUNC(rightType)->functype))
                    return 0;
                if (TYPE_FUNC(leftType)->qual != TYPE_FUNC(rightType)->qual)
                    return 0;
                if ((TYPE_FUNC(leftType)->flags & 0x17000001) != (TYPE_FUNC(rightType)->flags & 0x17000001))
                    return 0;
                return is_arglistsame(TYPE_FUNC(leftType)->args, TYPE_FUNC(rightType)->args);
            default:
                CError_FATAL(1548);
                return 0;
        }
    }
}

Type *CParser_GetBoolType(void)
{
    if (copts.cplusplus && copts.booltruefalse) {
        return (Type *)&stbool;
    }
    return (Type *)&stsignedint;
}

Type *CParser_GetWCharType(void)
{
    if (copts.cplusplus && copts.wchar_type) {
        return (Type *)&stwchar;
    }
    return (Type *)&stunsignedshort;
}

SInt32 CParser_GetOperator(UInt8 kind)
{
    switch ((unsigned char)kind) {
        default:
            CError_FATAL(1587);
        case 5:
            return 45U;
        case 6:
            return 126U;
        case 7:
            return 33U;
        case 15:
            return 43U;
        case 16:
            return 45U;
        case 9:
            return 42U;
        case 11:
            return 47U;
        case 12:
            return 37U;
        case 25:
            return 38U;
        case 26:
            return 94U;
        case 27:
            return 124U;
        case 17:
            return 364U;
        case 18:
            return 365U;
        case 19:
            return 60U;
        case 20:
            return 62U;
        case 21:
            return 362U;
        case 22:
            return 363U;
        case 23:
            return 360U;
        case 24:
            return 361U;
    }
}

Boolean is_unsigned(Type *type)
{
    if (IS_TYPE_ENUM(type))
        type = TYPE_ENUM(type)->enumtype;

    if (&type->type == &stunsignedchar.type || &type->type == &stwchar.type || &type->type == &stunsignedshort.type ||
        &type->type == &stunsignedint.type || &type->type == &stunsignedlong.type ||
        &type->type == &stunsignedlonglong.type || (TypeIntegral *)type == &stbool ||
        (copts.unsigned_char && type == (Type *)&stchar) || type->type == TYPEPOINTER)
        return 1;
    return 0;
}

StructMember *ismember(Type *type, HashNameNode *name)
{
    StructMember *member;
    HashNameNode *target;
    TypeStruct *owner;
    owner = (TypeStruct *)type;
    owner = (TypeStruct *)owner->members;
    target = name;
    member = (StructMember *)owner;
    while (member) {
        if (member->name == target)
            return member;
        member = member->next;
    }
    return NULL;
}

/* probe: bare while loop, caller does the null check elsewhere */
void appendmember(TypeStruct *s, StructMember *m)
{
    StructMember *p;

    if (s->members == NULL) {
        s->members = m;
        return;
    }
    p = s->members;
    while (p->next != NULL)
        p = p->next;
    p->next = m;
}

unsigned char test_declaration(Boolean parseDeclaration, Boolean requireValue, Boolean declarationFlag,
                               short terminator)
{
    unsigned char result;
    ParserTryBlock savedState;
    DeclInfo declaration;
    switch ((int)tk) {
        case 262:
        case 263:
        case 264:
        case 265:
        case 266:
        case 267:
        case 268:
        case 269:
        case 270:
        case 275:
        case 276:
        case 277:
        case 278:
        case 279:
        case 280:
        case 281:
        case 282:
        case 284:
        case 285:
            if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 40) {
                break;
            }
            return 1;
        case -3:
        case 372:
            break;
        default:
            return 1;
    }
    save(&savedState, &result);
    if (_Setjmp(savedState.jmpbuf) == 0) {
        memclrw(&declaration, sizeof(declaration));
        CParser_GetDeclSpecs((DeclInfo *)&declaration, 0);
        if (declaration.thetype->type != TYPETEMPLATE || ((TypeIntegral *)declaration.thetype)->integral != IT_CHAR ||
            declaration.hasTypename != 0 || declaration.isType != 0) {
            if (parseDeclaration != 0) {
                declaration.isNewExpression = declarationFlag;
                scandeclarator(&declaration);
                if (requireValue == 0 || declaration.name == NULL) {
                    if (terminator == 0) {
                        if (tk == ';' || tk == ',' || tk == '(' || tk == ')' || tk == '=' || tk == '>') {
                            result = 1;
                        }
                    } else {
                        result = (char)(tk == terminator);
                    }
                }
            } else {
                result = 1;
            }
        }
    }
    restore(&savedState);
    return result;
}

Boolean isdeclaration(Boolean option1, Boolean option2, Boolean option3, short option4)
{
    Boolean result;
    SInt32 savedState;
    SInt32 token;

    token = tk;
    if (((token < 0x100 || token > 0x131) && token != 0x174) &&
        (tk != TK_IDENTIFIER || CScope_PossibleTypeName(data_00587fa0) == 0)) {
        if (tk != TK_IDENTIFIER || copts.altivec_model == 0 ||
            memcmp(data_00587fa0->name, "vector", sizeof("vector")) != 0) {
            return 0;
        }
    } else if (copts.cplusplus == 0) {
        return 1;
    }

    CPrep_GetBufferedTokenPosition(&savedState);
    result = test_declaration(option1, option2, option3, option4);
    if (result != 0) {
        CPrep_SetPosition(&savedState);
        return 1;
    }
    CPrep_SetPosition(&savedState);
    return 0;
}

UInt8 islookaheaddeclaration(void)
{
    UInt8 result;
    SInt32 savedState;
    SInt32 token;

    CPrep_GetBufferedTokenPosition(&savedState);
    tk = CPrepTokenizer_GetNextToken();
    token = tk;
    if ((token < 0x100 || token > 0x131) && token != 0x174 &&
        (tk != TK_IDENTIFIER || CScope_PossibleTypeName(data_00587fa0) == 0)) {
        if (tk != TK_IDENTIFIER || copts.altivec_model == 0 || memcmp(data_00587fa0->name, "vector", 7) != 0) {
            CPrep_SetPosition(&savedState);
            return 0;
        }
    } else if (copts.cplusplus == 0) {
        CPrep_SetPosition(&savedState);
        return 1;
    }
    result = test_declaration(1, 1, 0, 0x29);
    if (result != 0) {
        CPrep_SetPosition(&savedState);
        return 1;
    }
    CPrep_SetPosition(&savedState);
    return 0;
}

Boolean CParser_TryParamList(int parserOption)
{
    ParserTryBlock savedState;
    SInt32 state;
    Boolean result = 0;

    CPrep_GetBufferedTokenPosition(&state);
    tk = CPrepTokenizer_GetNextToken();
    switch (tk) {
        case ')':
        case TK_ELLIPSIS:
            result = 1;
            break;
        default:
            CParser_SaveState(&savedState);
            if (_Setjmp(savedState.jmpbuf) == 0) {
                if (CFunc_ParseFakeArgList(parserOption) != 0 || tk == ')') {
                    result = 1;
                }
            }
            CParser_RestoreState(&savedState);
            break;
    }
    CPrep_SetPosition(&state);
    return result;
}

/* Layouts of the type records copied by this routine. */
Type *CParser_RemoveTopMostQualifiers(Type *type, UInt32 *qual)
{
    Type *copy;
    switch ((signed char)type->type) {
        case TYPEARRAY:
            TPTR_TARGET(type) = CParser_RemoveTopMostQualifiers(TPTR_TARGET(type), qual);
            return type;
        case TYPEPOINTER:
            if (((TypePointer *)type)->qual & Q_CONST) {
                copy = galloc(sizeof(TypePointer));
                *(TypePointer *)copy = *(TypePointer *)type;
                ((TypePointer *)copy)->qual = 0;
                return copy;
            }
            return type;
        case TYPEMEMBERPOINTER:
            if (((TypeMemberPointer *)type)->qual & Q_CONST) {
                copy = galloc(sizeof(TypeMemberPointer));
                *(TypeMemberPointer *)copy = *(TypeMemberPointer *)type;
                ((TypeMemberPointer *)copy)->qual = 0;
                return copy;
            }
            return type;
        default:
            *qual = 0;
            return type;
    }
}

unsigned int CParser_GetTypeQualifiers(Type *type, unsigned int qual)
{
    signed char kind;
    while ((kind = (signed char)type->type) == TYPEARRAY) {
        type = ((TypePointer *)type)->target;
    }
    switch (kind) {
        case TYPEPOINTER:
            qual = ((TypePointer *)type)->qual;
            break;
        case TYPEMEMBERPOINTER:
            qual = ((TypeMemberPointer *)type)->qual;
            break;
    }
    return qual;
}

UInt32 CParser_GetCVTypeQualifiers(Type *type, SInt32 qual)
{
    while (type->type == TYPEARRAY)
        type = ((TypePointer *)type)->target;
    switch ((signed char)type->type) {
        case TYPEPOINTER:
            qual = ((TypePointer *)type)->qual;
            break;
        case TYPEMEMBERPOINTER:
            qual = ((TypeMemberPointer *)type)->qual;
            break;
    }
    return qual & Q_CV;
}

UInt8 CParser_IsConst(Type *type, unsigned int qual)
{
    char kind;

    while ((kind = type->type) == TYPEARRAY) {
        type = ((TypePointer *)type)->target;
    }
    switch (kind) {
        case TYPEPOINTER:
            qual = ((TypePointer *)type)->qual;
            break;
        case TYPEMEMBERPOINTER:
            qual = ((TypeMemberPointer *)type)->qual;
            break;
    }
    return (qual & Q_CONST) != 0;
}

UInt8 CParser_IsVolatile(Type *type, unsigned int qualifiers)
{
    char kind;
    while ((kind = type->type) == TYPEARRAY)
        type = ((TypeMemberPointer *)type)->ty1;
    switch (kind) {
        case TYPEPOINTER:
            qualifiers = ((TypePointer *)type)->qual;
            break;
        case TYPEMEMBERPOINTER:
            qualifiers = ((TypeMemberPointer *)type)->qual;
            break;
    }
    return (qualifiers & Q_VOLATILE) != 0;
}

Boolean is_const_object(Object *object)
{
    Type *type;
    char kind;
    UInt32 qual;

    qual = object->qual;
    type = object->type;
    while ((kind = type->type) == 12) {
        type = TYPE_POINTER(type)->target;
    }
    switch (kind) {
        case 11:
            qual = TYPE_POINTER(type)->qual;
            break;
        case 10:
            qual = TYPE_MEMBER_POINTER(type)->qual;
            break;
    }
    return (qual & Q_CONST) != 0;
}

UInt8 is_volatile_object(Object *object)
{
    char kind;
    UInt32 qualifiers;
    Type *type;

    qualifiers = object->qual;
    type = object->type;
    while (type->type == '\f') {
        type = TPTR_TARGET(type);
    }
    kind = type->type;
    switch (kind) {
        case '\v':
            qualifiers = TYPE_POINTER(type)->qual;
            break;
        case '\n':
            qualifiers = TYPE_MEMBER_POINTER(type)->qual;
            break;
    }
    return (qualifiers & Q_VOLATILE) != 0;
}

Boolean CParserIsConstExpr(ENode *expr)
{
    Type *type;
    unsigned int qualifiers;
    char kind;

    qualifiers = expr->flags & ENODE_FLAG_QUALS;
    type = expr->rtype;
    while ((kind = type->type) == 12)
        type = ((TypePointer *)type)->target;
    switch (kind) {
        case 11:
            qualifiers = ((TypePointer *)type)->qual;
            break;
        case 10:
            qualifiers = ((TypeMemberPointer *)type)->qual;
            break;
    }
    return (qualifiers & ENODE_FLAG_CONST) != 0;
}

UInt8 CParserIsVolatileExpr(ENode *node)
{
    unsigned int qualifiers;
    Type *type;

    qualifiers = node->flags & (Q_CONST | Q_VOLATILE);
    type = node->rtype;
    while (type->type == TYPEARRAY)
        type = ((TypePointer *)type)->target;
    switch ((char)type->type) {
        case TYPEPOINTER:
            qualifiers = ((TypePointer *)type)->qual;
            break;
        case TYPEMEMBERPOINTER:
            qualifiers = ((TypeMemberPointer *)type)->qual;
            break;
    }
    return (qualifiers & Q_VOLATILE) != 0;
}

char CParser_HasInternalLinkage(Object *obj)
{
    if (obj->nspace != NULL) {
        if (obj->nspace->is_unnamed != 0) {
            return 1;
        }
    }
    if ((obj->qual & 0x60000U) != 0) {
        return 0;
    }
    if (obj->sclass == TK_STATIC) {
        return 1;
    }
    if ((obj->qual & 0x10U) != 0) {
        obj->qual |= 0x20000U;
    }
    return 0;
}

Boolean CParser_IsVirtualFunction(Object *object, TypeClass **firstValue, UInt32 *secondValue)
{
    if (object->datatype == DVFUNC) {
        *firstValue = ((TypeMemberFunc *)object->type)->theclass;
        *secondValue = ((TypeMemberFunc *)object->type)->vtbl_index;
        return 1;
    }
    return 0;
}

Boolean is_pascal_object(Object *object)
{
    unsigned int isPascal;
    isPascal = 0U;
    if (object->type->type == TYPEFUNC) {
        if (((TypeFunc *)object->type)->flags & FUNC_PASCAL) {
            isPascal = 1U;
        }
    }
    return isPascal;
}

TypeIntegral *select_builtin_type(SInt16 token, SInt16 lengthModifier, SInt16 signModifier)
{
    switch (token) {
        case 0:
        case 0x109:
            if (signModifier == 1) {
                switch (lengthModifier) {
                    case 1:
                        return &stunsignedshort;
                    case 2:
                        return &stunsignedlong;
                    case 3:
                        return &stunsignedlonglong;
                    default:
                        return &stunsignedint;
                }
            }
            switch (lengthModifier) {
                case 1:
                    return &stsignedshort;
                case 2:
                    return &stsignedlong;
                case 3:
                    return &stsignedlonglong;
                default:
                    return &stsignedint;
            }
        case 0x11c:
            return &stbool;
        case 0x11d:
            return &stwchar;
        case 0x107:
            switch (signModifier) {
                case 1:
                    return &stunsignedchar;
                default:
                    return &stchar;
                case -1:
                    return &stsignedchar;
            }
        case 0x10c:
            switch (lengthModifier) {
                case 1:
                    return &stshortdouble;
                case 2:
                    return &stlongdouble;
                default:
                    return &stdouble;
            }
        case 0x10b:
            return &stfloat;
        case 0x106:
            return (TypeIntegral *)&stvoid;
        default:
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return (TypeIntegral *)&stvoid;
    }
}

void TypedefDeclInfo(DeclInfo *slot, Type *type, UInt32 quals)
{
    TypePointer *nt;
    if (type->type == TYPEPOINTER) {
        TypePointer *t = (TypePointer *)type;
        if (t->qual & Q_REFERENCE) {
            TypedefDeclInfo(slot, t->target, quals);
            nt = galloc(0xe);
            *nt = *(TypePointer *)type;
            nt->target = slot->thetype;
            slot->thetype = (Type *)nt;
            return;
        }
        slot->thetype = (Type *)galloc(0xe);
        *(TypePointer *)slot->thetype = *t;
        ((TypePointer *)slot->thetype)->qual |= slot->qual & (Q_CV | Q_REFERENCE | Q_RESTRICT);
        slot->qual &= ~(Q_CV | Q_REFERENCE | Q_RESTRICT);
        slot->qual |= quals & (Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK);
    } else {
        slot->thetype = type;
        slot->qual |= quals & (Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK);
        if (slot->thetype->type == TYPEARRAY && slot->thetype->size == 0) {
            slot->thetype = (Type *)galloc(0xe);
            *(TypePointer *)slot->thetype = *(TypePointer *)type;
        }
    }
    slot->isType = 1;
}

void CParser_ParseAttribute(Type *type, DeclInfo *function)
{
    do {
        tk = CPrepTokenizer_GetNextToken();
        if (tk != '(') {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk != '(') {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        }

        if (memcmp(data_00587fa0->name, "aligned", 8) == 0 || memcmp(data_00587fa0->name, "__aligned__", 12) == 0) {
            SInt32 alignment;
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '(') {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_INTCONST) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
            alignment = intconst_lo;
            switch (alignment) {
                case 1:
                case 2:
                case 4:
                case 8:
                case 16:
                case 32:
                case 64:
                case 128:
                case 256:
                case 512:
                case 1024:
                case 2048:
                case 4096:
                case 8192:
                    break;
                default:
                    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                    return;
            }
            if (type != NULL) {
                if (type->type == TYPESTRUCT) {
                    if (alignment > (*(TypeStruct *)type).align) {
                        (*(TypeStruct *)type).align = alignment;
                        type->size += CABI_ComputeAlignmentPadding(type, type->size);
                    }
                } else if (type->type == TYPECLASS) {
                    if (alignment > (*(TypeClass *)type).align) {
                        (*(TypeClass *)type).align = alignment;
                        type->size += CABI_ComputeAlignmentPadding(type, type->size);
                    }
                } else {
                    CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS);
                }
            } else if (function != NULL) {
                function->qual &= ~Q_ALIGNED_MASK;
                switch (alignment = intconst_lo) {
                    case 1:
                        function->qual |= Q_ALIGNED_1;
                        break;
                    case 2:
                        function->qual |= Q_ALIGNED_2;
                        break;
                    case 4:
                        function->qual |= Q_ALIGNED_4;
                        break;
                    case 8:
                        function->qual |= Q_ALIGNED_8;
                        break;
                    case 16:
                        function->qual |= Q_ALIGNED_16;
                        break;
                    case 32:
                        function->qual |= Q_ALIGNED_32;
                        break;
                    case 64:
                        function->qual |= Q_ALIGNED_64;
                        break;
                    case 128:
                        function->qual |= Q_ALIGNED_128;
                        break;
                    case 256:
                        function->qual |= Q_ALIGNED_256;
                        break;
                    case 512:
                        function->qual |= Q_ALIGNED_512;
                        break;
                    case 1024:
                        function->qual |= Q_ALIGNED_1024;
                        break;
                    case 2048:
                        function->qual |= Q_ALIGNED_2048;
                        break;
                    case 4096:
                        function->qual |= Q_ALIGNED_4096;
                        break;
                    case 8192:
                        function->qual |= Q_ALIGNED_8192;
                        break;
                    default:
                        CError_FATAL(2423);
                        break;
                }
            } else {
                CError_ReportError(ERR_ILLEGAL_UNSUPPORTED_ATTRIBUTE);
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ')') {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
        } else if (memcmp(data_00587fa0->name, "section", 8) == 0 ||
                   memcmp(data_00587fa0->name, "__section__", 12) == 0) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '(') {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_STRING) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
            if (function != NULL) {
                BE_elf_SetDeclSection(string_token_data, function);
            } else {
                CError_ReportError(ERR_ILLEGAL_TYPE_QUALIFIERS);
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ')') {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
        } else {
            CError_ReportError(ERR_ILLEGAL_UNSUPPORTED_ATTRIBUTE);
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk != ')') {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk != ')') {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        }
        tk = CPrepTokenizer_GetNextToken();
    } while (tk == TK_UU_ATTRIBUTE);
}

void CParser_ParseDeclSpec(DeclInfo *decl, int unused)
{
    tk = CPrepTokenizer_GetNextToken();
    if (tk != TK_IDENTIFIER) {
        if (tk != TK_EXPORT) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return;
        }
        decl->exportflags |= 0x40;
        return;
    }
    if (memcmp("internal", data_00587fa0->name, sizeof("internal")) == 0) {
        decl->exportflags |= 0x10;
        return;
    }
    if (memcmp("import", data_00587fa0->name, sizeof("import")) == 0 ||
        memcmp("dllimport", data_00587fa0->name, sizeof("dllimport")) == 0) {
        decl->exportflags |= 0x20;
        return;
    }
    if (memcmp("export", data_00587fa0->name, sizeof("export")) == 0 ||
        memcmp("dllexport", data_00587fa0->name, sizeof("dllexport")) == 0) {
        decl->exportflags |= 0x40;
        return;
    }
    if (memcmp("lib_export", data_00587fa0->name, sizeof("lib_export")) == 0) {
        decl->exportflags |= 0x60;
        return;
    }
    if (memcmp("weak", data_00587fa0->name, sizeof("weak")) == 0) {
        decl->qual |= Q_WEAK;
        return;
    }
    CodeGen_ParseDeclspecSection(data_00587fa0, decl);
}

int parse_dtype_specifiers(DeclInfo *state)
{
    tk = CPrepTokenizer_GetNextToken();
    switch (tk) {
        case TK_CHAR:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_BOOL:
                    state->thetype = TYPE(&stvectorboolchar);
                    tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_UNSIGNED:
                    state->thetype = TYPE(&stvectorunsignedchar);
                    tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_SIGNED:
                    state->thetype = TYPE(&stvectorsignedchar);
                    tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_IDENTIFIER:
                    if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                        state->thetype = TYPE(&stvectorboolchar);
                        tk = CPrepTokenizer_GetNextToken();
                        return 1;
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case TK_SIGNED:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_CHAR:
                    state->thetype = TYPE(&stvectorsignedchar);
                    tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_SHORT:
                    state->thetype = TYPE(&stvectorsignedshort);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case 0x10a:
                    state->thetype = TYPE(&stvectorsignedlong);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_INT:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_SHORT:
                            state->thetype = TYPE(&stvectorsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case 0x10a:
                            state->thetype = TYPE(&stvectorsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        default:
                            state->thetype = TYPE(&stvectorsignedlong);
                            return 1;
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case TK_UNSIGNED:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_CHAR:
                    state->thetype = TYPE(&stvectorunsignedchar);
                    tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_SHORT:
                    state->thetype = TYPE(&stvectorunsignedshort);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case 0x10a:
                    state->thetype = TYPE(&stvectorunsignedlong);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_INT:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_SHORT:
                            state->thetype = TYPE(&stvectorunsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case 0x10a:
                            state->thetype = TYPE(&stvectorunsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        default:
                            state->thetype = TYPE(&stvectorunsignedlong);
                            return 1;
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case TK_BOOL:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_CHAR:
                    state->thetype = TYPE(&stvectorboolchar);
                    tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_SHORT:
                    state->thetype = TYPE(&stvectorboolshort);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case 0x10a:
                    state->thetype = TYPE(&stvectorboollong);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_INT:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_SHORT:
                            state->thetype = TYPE(&stvectorboolshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case 0x10a:
                            state->thetype = TYPE(&stvectorboollong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        default:
                            state->thetype = TYPE(&stvectorboollong);
                            return 1;
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case TK_SHORT:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_BOOL:
                    state->thetype = TYPE(&stvectorboolshort);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_SIGNED:
                    state->thetype = TYPE(&stvectorsignedshort);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_UNSIGNED:
                    state->thetype = TYPE(&stvectorunsignedshort);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_INT:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_BOOL:
                            state->thetype = TYPE(&stvectorboolshort);
                            tk = CPrepTokenizer_GetNextToken();
                            if (tk == TK_INT)
                                tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_SIGNED:
                            state->thetype = TYPE(&stvectorsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_UNSIGNED:
                            state->thetype = TYPE(&stvectorunsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_IDENTIFIER:
                            if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                                state->thetype = TYPE(&stvectorboolshort);
                                tk = CPrepTokenizer_GetNextToken();
                                if (tk == TK_INT)
                                    tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            }
                        default:
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                            break;
                    }
                    break;
                case TK_IDENTIFIER:
                    if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                        state->thetype = TYPE(&stvectorboolshort);
                        tk = CPrepTokenizer_GetNextToken();
                        if (tk == TK_INT)
                            tk = CPrepTokenizer_GetNextToken();
                        return 1;
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case 0x10a:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_BOOL:
                    state->thetype = TYPE(&stvectorboollong);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_SIGNED:
                    state->thetype = TYPE(&stvectorsignedlong);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_UNSIGNED:
                    state->thetype = TYPE(&stvectorunsignedlong);
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_INT)
                        tk = CPrepTokenizer_GetNextToken();
                    return 1;
                case TK_INT:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_BOOL:
                            state->thetype = TYPE(&stvectorboollong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_SIGNED:
                            state->thetype = TYPE(&stvectorsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_UNSIGNED:
                            state->thetype = TYPE(&stvectorunsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_IDENTIFIER:
                            if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                                state->thetype = TYPE(&stvectorboollong);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            }
                        default:
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                            break;
                    }
                    break;
                case TK_IDENTIFIER:
                    if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                        state->thetype = TYPE(&stvectorboollong);
                        tk = CPrepTokenizer_GetNextToken();
                        if (tk == TK_INT)
                            tk = CPrepTokenizer_GetNextToken();
                        return 1;
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case TK_INT:
            tk = CPrepTokenizer_GetNextToken();
            switch (tk) {
                case TK_BOOL:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_SHORT:
                            state->thetype = TYPE(&stvectorboolshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case 0x10a:
                            state->thetype = TYPE(&stvectorboollong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        default:
                            state->thetype = TYPE(&stvectorboollong);
                            return 1;
                    }
                case TK_SIGNED:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_SHORT:
                            state->thetype = TYPE(&stvectorsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case 0x10a:
                            state->thetype = TYPE(&stvectorsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        default:
                            state->thetype = TYPE(&stvectorsignedlong);
                            return 1;
                    }
                case TK_UNSIGNED:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_SHORT:
                            state->thetype = TYPE(&stvectorunsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case 0x10a:
                            state->thetype = TYPE(&stvectorunsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        default:
                            state->thetype = TYPE(&stvectorunsignedlong);
                            return 1;
                    }
                case TK_SHORT:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_BOOL:
                            state->thetype = TYPE(&stvectorboolshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_SIGNED:
                            state->thetype = TYPE(&stvectorsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_UNSIGNED:
                            state->thetype = TYPE(&stvectorunsignedshort);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_IDENTIFIER:
                            if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                                state->thetype = TYPE(&stvectorboolshort);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            }
                        default:
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                            break;
                    }
                    break;
                case 0x10a:
                    tk = CPrepTokenizer_GetNextToken();
                    switch (tk) {
                        case TK_BOOL:
                            state->thetype = TYPE(&stvectorboollong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_SIGNED:
                            state->thetype = TYPE(&stvectorsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_UNSIGNED:
                            state->thetype = TYPE(&stvectorunsignedlong);
                            tk = CPrepTokenizer_GetNextToken();
                            return 1;
                        case TK_IDENTIFIER:
                            if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                                state->thetype = TYPE(&stvectorboollong);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            }
                            break;
                    }
                    /* An unrecognized suffix continues through the identifier case. */
                case TK_IDENTIFIER:
                    if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                        tk = CPrepTokenizer_GetNextToken();
                        switch (tk) {
                            case 0x10a:
                                state->thetype = TYPE(&stvectorboollong);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            case TK_SHORT:
                                state->thetype = TYPE(&stvectorboolshort);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            default:
                                state->thetype = TYPE(&stvectorboolshort);
                                return 1;
                        }
                    }
                default:
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    break;
            }
            break;

        case TK_FLOAT:
            state->thetype = TYPE(&stvectorfloat);
            tk = CPrepTokenizer_GetNextToken();
            return 1;

        case TK_IDENTIFIER:
            if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("pixel") ||
                (HashNameNode *)data_00587fa0 == GetHashNameNodeExport("__pixel")) {
                state->thetype = TYPE(&stvectorpixel);
                tk = CPrepTokenizer_GetNextToken();
                return 1;
            }
            if ((HashNameNode *)data_00587fa0 == GetHashNameNodeExport("bool")) {
                tk = CPrepTokenizer_GetNextToken();
                switch (tk) {
                    case TK_CHAR:
                        state->thetype = TYPE(&stvectorboolchar);
                        tk = CPrepTokenizer_GetNextToken();
                        return 1;
                    case TK_SHORT:
                        state->thetype = TYPE(&stvectorboolshort);
                        tk = CPrepTokenizer_GetNextToken();
                        if (tk == TK_INT)
                            tk = CPrepTokenizer_GetNextToken();
                        return 1;
                    case 0x10a:
                        state->thetype = TYPE(&stvectorboollong);
                        tk = CPrepTokenizer_GetNextToken();
                        if (tk == TK_INT)
                            tk = CPrepTokenizer_GetNextToken();
                        return 1;
                    case TK_INT:
                        tk = CPrepTokenizer_GetNextToken();
                        switch (tk) {
                            case TK_SHORT:
                                state->thetype = TYPE(&stvectorboolshort);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            case 0x10a:
                                state->thetype = TYPE(&stvectorboollong);
                                tk = CPrepTokenizer_GetNextToken();
                                return 1;
                            default:
                                state->thetype = TYPE(&stvectorboollong);
                                return 1;
                        }
                    default:
                        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                        break;
                }
                break;
            }
        default:
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            break;
    }

    return 0;
}

unsigned char CParser_CheckTemplateClassScope(Type *type)
{
    NameSpace *space;
    TemplClass *classType = (TemplClass *)type;
    if (classType->templ__params != NULL) {
        space = cscope_current;
        for (;;) {
            if (space == NULL) {
                CError_ReportError(230U);
                return 0;
            }
            if (space->theclass == TYPE_CLASS(type))
                break;
            space = space->parent;
        }
    }
    return 1;
}

void CParser_GetDeclSpecs(DeclInfo *state, Boolean allowObject)
{
    NameResult scope;
    Type *type;
    SInt16 typeToken;
    int tokenValue;
    SInt16 sizeModifier;
    SInt16 signModifier;
    Boolean firstToken;
    Boolean requireType;
    UInt8 vectorKeyword;

    state->file = CPrep_GetPFile();
    CPrep_GetBrowseFilePosition(&state->file2, &state->sourceoffset);
    firstToken = 1;
    requireType = copts.cplusplus;
    sizeModifier = signModifier = typeToken = 0;

    for (;;) {
        switch (tk) {
            case TK_AUTO:
            case TK_REGISTER:
            case TK_STATIC:
            case TK_EXTERN:
            case TK_TYPEDEF:
            case TK_MUTABLE:
                if (state->storageclass != TK_EOF)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                state->storageclass = tk;
                break;
            case TK_CONST:
                if (state->thetype != NULL && state->thetype->type == TYPEPOINTER) {
                    if (TYPE_POINTER(state->thetype)->qual & Q_CONST)
                        CError_ReportIllegalFlags(Q_CONST);
                    TYPE_POINTER(state->thetype)->qual |= Q_CONST;
                } else {
                    if (state->qual & Q_CONST)
                        CError_ReportIllegalFlags(Q_CONST);
                    state->qual |= Q_CONST;
                }
                break;
            case TK_VOLATILE:
                if (state->thetype != NULL && state->thetype->type == TYPEPOINTER) {
                    if (TYPE_POINTER(state->thetype)->qual & Q_VOLATILE)
                        CError_ReportIllegalFlags(Q_VOLATILE);
                    TYPE_POINTER(state->thetype)->qual |= Q_VOLATILE;
                } else {
                    if (state->qual & Q_VOLATILE)
                        CError_ReportIllegalFlags(Q_VOLATILE);
                    state->qual |= Q_VOLATILE;
                }
                break;
            case TK_UU_FAR:
                (void)&state->qual;
                break;
            case TK_PASCAL:
                if (state->qual & Q_PASCAL)
                    CError_ReportIllegalFlags(Q_PASCAL);
                state->qual |= Q_PASCAL;
                break;
            case TK_EXPLICIT:
                CError_ReportIllegalFlags(state->qual & Q_EXPLICIT);
                state->qual |= Q_EXPLICIT;
                break;
            case TK_VIRTUAL:
                CError_ReportIllegalFlags(state->qual & Q_VIRTUAL);
                state->qual |= Q_VIRTUAL;
                break;
            case TK_IN:
                CError_ReportIllegalFlags(state->qual & Q_IN);
                state->qual |= Q_IN;
                break;
            case TK_OUT:
                CError_ReportIllegalFlags(state->qual & Q_OUT);
                state->qual |= Q_OUT;
                break;
            case TK_INOUT:
                CError_ReportIllegalFlags(state->qual & Q_INOUT);
                state->qual |= Q_INOUT;
                break;
            case TK_BYCOPY:
                CError_ReportIllegalFlags(state->qual & Q_BYCOPY);
                state->qual |= Q_BYCOPY;
                break;
            case TK_BYREF:
                CError_ReportIllegalFlags(state->qual & Q_BYREF);
                state->qual |= Q_BYREF;
                break;
            case TK_ONEWAY:
                CError_ReportIllegalFlags(state->qual & Q_ONEWAY);
                state->qual |= Q_ONEWAY;
                break;
            case TK_UU_DECLSPEC:
                tk = CPrepTokenizer_GetNextToken();
                if (tk != '(')
                    CError_ReportError(ERR_LPAREN_EXPECTED);
                CParser_ParseDeclSpec(state, 0);
                tk = CPrepTokenizer_GetNextToken();
                if (tk != ')')
                    CError_ReportError(ERR_RPAREN_EXPECTED);
                break;
            case TK_ASM:
                if (state->qual & Q_ASM)
                    CError_ReportIllegalFlags(Q_ASM);
                state->qual |= Q_ASM;
                break;
            case TK_INLINE:
                if (state->qual & Q_INLINE)
                    CError_ReportIllegalFlags(Q_INLINE);
                state->qual |= Q_INLINE;
                break;
            case TK_SHORT:
                if (sizeModifier != 0 ||
                    (typeToken != 0 && (tokenValue = typeToken) != TK_INT && tokenValue != TK_DOUBLE))
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                sizeModifier = 1;
                break;
            case 0x10a:
                if (copts.longlong != 0) {
                    if (typeToken != 0 && (tokenValue = typeToken) != TK_INT && tokenValue != TK_DOUBLE)
                        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    if (sizeModifier != 0) {
                        if (sizeModifier != 2 || typeToken == TK_DOUBLE)
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                        sizeModifier = 3;
                    } else {
                        sizeModifier = 2;
                    }
                } else {
                    if (sizeModifier != 0 ||
                        (typeToken != 0 && (tokenValue = typeToken) != TK_INT && tokenValue != TK_DOUBLE))
                        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    sizeModifier = 2;
                }
                break;
            case TK_SIGNED:
                if (signModifier != 0 ||
                    (typeToken != 0 && (tokenValue = typeToken) != TK_INT && tokenValue != TK_CHAR))
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                signModifier = -1;
                break;
            case TK_UNSIGNED:
                if (signModifier != 0 ||
                    (typeToken != 0 && (tokenValue = typeToken) != TK_INT && tokenValue != TK_CHAR))
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                signModifier = 1;
                break;
            case TK_VOID:
                if (typeToken != 0 || sizeModifier != 0 || signModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_VOID;
                break;
            case TK_FLOAT:
                if (typeToken != 0 || sizeModifier != 0 || signModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_FLOAT;
                break;
            case TK_BOOL:
                if (typeToken != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_BOOL;
                break;
            case TK_CHAR:
                if (typeToken != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_CHAR;
                break;
            case TK_WCHAR_T:
                if (typeToken != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_WCHAR_T;
                break;
            case TK_INT:
                if (typeToken != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_INT;
                break;
            case TK_DOUBLE:
                if (typeToken != 0 || signModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                typeToken = TK_DOUBLE;
                break;
            case TK_STRUCT:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                tk = CPrepTokenizer_GetNextToken();
                scanstruct(state, 0);
                if (tk == TK_UU_ATTRIBUTE)
                    CParser_ParseAttribute(state->thetype, NULL);
                if ((tokenValue = tk) == TK_CONST || tokenValue == TK_VOLATILE || tokenValue == TK_UU_FAR ||
                    tokenValue == 0x124) {
                    typeToken = -1;
                    continue;
                }
                return;
            case TK_CLASS:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                tk = CPrepTokenizer_GetNextToken();
                CDecl_ParseClass(state, 2, 1, 0);
                if (tk == TK_UU_ATTRIBUTE)
                    CParser_ParseAttribute(state->thetype, NULL);
                if ((tokenValue = tk) == TK_CONST || tokenValue == TK_VOLATILE || tokenValue == TK_UU_FAR ||
                    tokenValue == 0x124) {
                    typeToken = -1;
                    continue;
                }
                return;
            case TK_UNION:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                tk = CPrepTokenizer_GetNextToken();
                scanstruct(state, 1);
                if (tk == TK_UU_ATTRIBUTE)
                    CParser_ParseAttribute(state->thetype, NULL);
                if ((tokenValue = tk) == TK_CONST || tokenValue == TK_VOLATILE || tokenValue == TK_UU_FAR ||
                    tokenValue == 0x124) {
                    typeToken = -1;
                    continue;
                }
                return;
            case TK_ENUM:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                tk = CPrepTokenizer_GetNextToken();
                scanenum(state);
                if (tk == TK_UU_ATTRIBUTE)
                    CParser_ParseAttribute(state->thetype, NULL);
                if ((tokenValue = tk) == TK_CONST || tokenValue == TK_VOLATILE || tokenValue == TK_UU_FAR ||
                    tokenValue == 0x124) {
                    typeToken = -1;
                    continue;
                }
                return;
            case TK_TYPENAME:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0 || state->hasTypename != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                state->hasTypename = 1;
                tk = CPrepTokenizer_GetNextToken();
                if (tk != TK_COLON_COLON && tk != TK_IDENTIFIER) {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    return;
                }
                goto lookup_type;
            case TK_COLON_COLON:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0)
                    goto finish_type;
                goto lookup_type;
            case TK_UU_VECTOR:
                if (typeToken != 0 || signModifier != 0 || sizeModifier != 0)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            parse_vector:
                if (parse_dtype_specifiers(state) != 0) {
                    if (tk == TK_CONST) {
                        if (state->qual == 0) {
                            state->qual |= Q_CONST;
                            tk = CPrepTokenizer_GetNextToken();
                        } else {
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                        }
                    }
                    if (tk == TK_VOLATILE) {
                        if (state->qual == 0) {
                            state->qual |= Q_VOLATILE;
                            tk = CPrepTokenizer_GetNextToken();
                        } else {
                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                        }
                    }
                    return;
                }
                break;
            case TK_IDENTIFIER:
                if (copts.altivec_model != 0 && typeToken == 0 && signModifier == 0 && sizeModifier == 0) {
                    if (strcmp(data_00587fa0->name, "vector") == 0) {
                        vectorKeyword = CheckVectorKeyword();
                        if (vectorKeyword)
                            goto parse_vector;
                    }
                }
                if (typeToken == 0 && signModifier == 0 && sizeModifier == 0) {
                    if (copts.objective_c && strcmp(data_00587fa0->name, "id") == 0) {
                        state->thetype = CObjC_ParseIdType();
                        typeToken = -1;
                        goto reset_first_token;
                    }
                lookup_type:
                    if (CScope_ParseDeclName(&scope)) {
                        if (scope.type != NULL) {
                            type = scope.type;
                            do {
                                if (type->type == TYPETEMPLATE) {
                                    switch (TYPE_TEMPLATE(type)->dtype) {
                                        case 0:
                                            if (TYPE_TEMPLATE(type)->u.pid.type == 0)
                                                CError_ReportError(ERR_ILLEGAL_TEMPLATE_ARGUMENT_DEPENDENT_EXPRESSION);
                                            break;
                                        case 1:
                                            if (state->hasTypename != 0)
                                                continue;
                                            break;
                                        case 2:
                                        case 3:
                                        case 4:
                                        case 5:
                                            break;
                                        default:
                                            CError_FATAL(3501);
                                    }
                                }
                            } while (0);
                            if (scope.type->type == TYPECLASS &&
                                (TYPE_CLASS(scope.type)->flags & CLASS_IS_TEMPL) != 0) {
                                if (!CheckClassAccess(scope.type))
                                    scope.type = (Type *)&stsignedint;
                            }
                            TypedefDeclInfo(state, scope.type, scope.qual);
                            state->isType = scope.is_type;
                            typeToken = -1;
                            tk = CPrepTokenizer_GetNextToken();
                            if (tk == '<' && copts.objective_c != 0 && state->thetype->type == TYPECLASS &&
                                TYPE_CLASS(state->thetype)->objcinfo != NULL) {
                                state->thetype = CObjC_ParseProtocolList(state->thetype);
                            }
                            goto reset_first_token;
                        }
                        if (scope.objects != NULL) {
                            if (scope.is_qualified != 0) {
                                if (allowObject != 0 && (((Object *)scope.objects->object)->nspace == scope.nspace ||
                                                         state->allowForeignNamespace != 0)) {
                                    state->resolvedObjects = scope.objects;
                                    if (((Object *)((NameSpaceObjectList *)state->resolvedObjects)->object)
                                                ->type->type == TYPEFUNC &&
                                        ((TYPE_FUNC(
                                              ((Object *)((NameSpaceObjectList *)state->resolvedObjects)->object)->type)
                                              ->flags &
                                          0x2000) |
                                         0x4000) != 0) {
                                        requireType = 0;
                                    }
                                } else {
                                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                                }
                            }
                        } else if (scope.object != NULL) {
                            switch (scope.object->otype) {
                                case 5:
                                    if (scope.is_qualified != 0) {
                                        if (allowObject != 0 && (((Object *)scope.object)->nspace == scope.nspace ||
                                                                 state->allowForeignNamespace != 0)) {
                                            state->resolvedObject = scope.object;
                                            if (((Object *)state->resolvedObject)->type->type == TYPEFUNC &&
                                                ((TYPE_FUNC(((Object *)state->resolvedObject)->type)->flags &
                                                  FUNC_IS_DTOR) |
                                                 0x4000) != 0) {
                                                requireType = 0;
                                            }
                                        } else {
                                            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                                        }
                                    }
                                    break;
                                case 0:
                                case 4:
                                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                                    break;
                                default:
                                    CError_FATAL(3568);
                            }
                        } else if (scope.name != NULL) {
                            if (copts.cplusplus != 0)
                                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                        } else {
                            CError_FATAL(3577);
                        }
                    }
                }
            default:
            finish_type:
                if (typeToken == 0 && signModifier == 0 && sizeModifier == 0)
                    state->parserOption = 1;
                if (typeToken >= 0)
                    state->thetype = (Type *)select_builtin_type(typeToken, sizeModifier, signModifier);
                if (firstToken) {
                    if (requireType != 0) {
                        if (state->storageclass == TK_EOF && state->qual == 0 && state->exportflags == 0) {
                            if (tk == TK_IDENTIFIER) {
                                HashNameNode *name = data_00587fa0;
                                if (CPrepTokenizer_GetNextTokenAndRestorePosition() != '(') {
                                    CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
                                } else {
                                    CError_Warning(ERR_IMPLICIT_INT_NO_LONGER_SUPPORTED_C);
                                }
                                data_00587fa0 = name;
                            } else {
                                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                            }
                        }
                    }
                    state->missingTypeSpecifier = 1;
                }
                return;
            case ';':
                if (typeToken == 0 && signModifier == 0 && sizeModifier == 0 && copts.warn_emptydecl != 0)
                    CError_Warning(ERR_ILLEGAL_EMPTY_DECLARATION);
                if (typeToken >= 0)
                    state->thetype = (Type *)select_builtin_type(typeToken, sizeModifier, signModifier);
                return;
        }
        tk = CPrepTokenizer_GetNextToken();
    reset_first_token:
        firstToken = 0;
    }
}

void CParser_RegisterNonGlobalClass(TypeClass *type)
{
    CParseRec *entry;
    CParseRec *previous;
    entry = (CParseRec *)lalloc(8U);
    previous = class_parse_recs;
    entry->next = previous;
    entry->listOwner = type;
    class_parse_recs = entry;
}

void CParser_RegisterSingleExprFunction(Object *object, ENode *expr)
{
    CParseCacheNode *entry;
    CParseCacheNode *previous;
    entry = (CParseCacheNode *)lalloc(sizeof(CParseCacheNode));
    previous = single_expr_functions;
    entry->next = previous;
    entry->object = object;
    entry->expr = expr;
    single_expr_functions = entry;
}

void fn_0048c290(void)
{
    CParseCacheNode *node;
    CParseRec *record;

    while (single_expr_functions != NULL) {
        node = single_expr_functions;
        single_expr_functions = node->next;
        CFunc_GenerateSingleExprFunc(node->object, node->expr);
    }

    if (class_parse_recs != NULL) {
        if (!fn_0050ebc0())
            return;

        for (record = class_parse_recs; record != NULL; record = record->next) {
            while (record->listOwner->nspace->parent->is_global == 0)
                record->listOwner->nspace->parent = record->listOwner->nspace->parent->parent;
        }
        class_parse_recs = NULL;
    }

    freelheap();
}

void fn_0048c220(char processInput)
{
    Boolean repeat;
    struct ClassTypeLink *input;

    do {
        fn_0048c290();
        repeat = 0;
        if (processInput != 0) {
            freelheap();
            if (class_type_links != NULL) {
                input = class_type_links;
                CClass_ClassAction(input->type);
                input = class_type_links;
                class_type_links = input->next;
                repeat = 1;
            } else if (CTemplateNew_InstantiatePendingTemplates() != 0) {
                repeat = 1;
            }
        }
        while (CInline_DispatchNextDeferredNode() != 0) {
            fn_0048c290();
            repeat = 1;
        }
    } while (repeat);
}

Boolean CParser_IsAnonymousUnion(Type **ptype, Boolean flag)
{
    SInt32 result = 0;
    SInt32 isClass =
        (*ptype)->type == TYPECLASS && (TYPE_CLASS(*ptype)->mode == 1 || (flag > 0 && copts.cpp_extensions != 0));
    if (isClass) {
        HashNameNode *name = TYPE_CLASS(*ptype)->classname;
        Boolean isAnonymous = name == NULL || name->name[0] == '@' || name->name[0] == '$';
        if (isAnonymous)
            result = 1;
    }
    return result;
}

void CParser_CheckAnonymousUnion(DeclInfo *context, char flag)
{
    Object *object;
    char name1[16];
    char name2[16];

    if (!IsAnonymousUnion(context)) {
        if (copts.warn_emptydecl) {
            char type = context->thetype->type;

            switch (type) {
                case 3:
                case 4:
                case 5:
                    if (context->storageclass == TK_EOF && context->qual == 0)
                        break;
                default:
                    CError_Warning(ERR_ILLEGAL_EMPTY_DECLARATION);
            }
        }
        return;
    }
    if (flag == 0 && context->storageclass != TK_STATIC)
        CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
    if (flag != 0 && context->storageclass != TK_STATIC) {
        object = CParser_NewLocalDataObject(context, 1);
        name1[0] = '@';
        CParser_PrintUniqueID(name1 + 1);
        object->name = GetHashNameNode(name1);
        CFunc_SetupLocalVarInfo(object);
        object->u.alias.object->aliasOrVarRecord.flag = 1;
    } else {
        object = CParser_NewGlobalDataObject(context);
        name2[0] = '@';
        CParser_PrintUniqueID(name2 + 1);
        object->name = GetHashNameNode(name2);
        object->sclass = TK_STATIC;
        CInit_DeclareData(object, NULL, NULL, object->type->size);
    }
    {
        ObjMemberVar *member;
        for (member = ((TypeClass *)context->thetype)->ivars; member != NULL; member = member->next) {
            Object *alias = (Object *)galloc(sizeof(Object));
            *(Object *)alias = *(Object *)object;
            alias->name = member->name;
            alias->type = member->type;
            alias->qual = member->qual;
            alias->datatype = DALIAS;
            alias->u.alias.object = object;
            alias->u.alias.offset = member->offset;
            alias->u.alias.member = NULL;
            CScope_AddObject(cscope_current, alias->name, (ObjBase *)alias);
        }
    }
}

void CParser_NewCallBackAction(Object *object, TypeClass *theclass)
{
    struct CallbackAction *entry;
    struct CallbackAction *head;
    entry = (struct CallbackAction *)galloc(sizeof(struct CallbackAction));
    head = pending_object_classes;
    entry->next = head;
    entry->obj = object;
    entry->tclass = theclass;
    pending_object_classes = entry;
    object->flags |= OBJECT_LAZY;
}

unsigned int CParser_NewClassAction(TypeClass *type)
{
    struct ClassTypeLink *link;
    struct ClassTypeLink *head;
    link = (struct ClassTypeLink *)galloc(8U);
    head = class_type_links;
    link->next = head;
    link->type = type;
    class_type_links = link;
    return (unsigned int)link;
}

void CParser_CallBackAction(Object *key)
{
    struct CallbackAction *entry;
    struct ClassTypeLink *node;
    TypeClass *value;

    entry = pending_object_classes;
    if (pending_object_classes != NULL) {
        do {
            if (entry->obj == key) {
                value = entry->tclass;
                node = (struct ClassTypeLink *)galloc(sizeof(struct ClassTypeLink));
                node->next = class_type_links;
                node->type = value;
                class_type_links = node;
                return;
            }
            entry = entry->next;
        } while (entry != NULL);
    }
    CError_FATAL(3854);
}

static void restore(ParserTryBlock *s)
{
    TypeClass *t3;
    t3 = s->cscope_currentclass;
    cscope_current = s->cscope_current;
    cscope_currentclass = t3;
    cscope_currentfunc = s->cscope_currentfunc;
    ctempl_curinstance = s->ctempl_curinstance;
    cerror_locktoken = s->cerror_locktoken;
    cscope_is_member_func = s->cscope_is_member_func;
    trychain = s->next;
}

static void save(ParserTryBlock *s, unsigned char *flag)
{
    NameSpace *x;
    x = cscope_current;
    s->cscope_current = x;
    s->cscope_currentclass = cscope_currentclass;
    *flag = 0;
    s->cscope_currentfunc = cscope_currentfunc;
    s->ctempl_curinstance = ctempl_curinstance;
    s->cerror_locktoken = cerror_locktoken;
    s->cscope_is_member_func = cscope_is_member_func;
    s->next = trychain;
    trychain = s;
    (void)s->cscope_current;
}

static void CParser_RestoreState(ParserTryBlock *sv)
{
    cscope_current = sv->cscope_current;
    cscope_currentclass = sv->cscope_currentclass;
    cscope_currentfunc = sv->cscope_currentfunc;
    ctempl_curinstance = sv->ctempl_curinstance;
    cerror_locktoken = sv->cerror_locktoken;
    cscope_is_member_func = sv->cscope_is_member_func;
    trychain = sv->next;
}

static void CParser_SaveState(ParserTryBlock *sv)
{
    sv->cscope_current = cscope_current;
    sv->cscope_currentclass = cscope_currentclass;
    sv->cscope_currentfunc = cscope_currentfunc;
    sv->ctempl_curinstance = ctempl_curinstance;
    sv->cerror_locktoken = cerror_locktoken;
    sv->cscope_is_member_func = cscope_is_member_func;
    sv->next = trychain;
    trychain = sv;
}

Object *CParser_ParseObject(void)
{
    HashNameNode *name;
    NameSpaceObjectList *result;
    Object *object;
    DeclInfo declaration;
    NameResult lookupState;
    memclrw(&declaration, sizeof(declaration));
    CParser_GetDeclSpecs(&declaration, 1);
    scandeclarator(&declaration);
    if ((name = declaration.name) != NULL) {
        result = CScope_FindObjectList(&lookupState, name);
        if (result != NULL && result->object->otype == OT_OBJECT) {
            if (declaration.thetype->type == TYPEFUNC) {
                return fn_0048be40_inline1((TypeFunc *)declaration.thetype, result);
            }
            if (iscpp_typeequal(declaration.thetype, ((Object *)result->object)->type) != 0) {
                if (((Object *)result->object)->qual == declaration.qual) {
                    return (Object *)result->object;
                }
            }
            CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(object),
                               object->type, (object = (Object *)result->object)->qual, declaration.thetype,
                               declaration.qual);
        }
    }
    return NULL;
}

void CParser_ParseGlobalDeclaration(void)
{
    DeclInfo buf;

    if (tk != 0) {
        CPrep_GetFOI(&function_fileinfo, NULL);
        data_00587184 = data_00588454;
        declaration_token = *CPrep_GetLastBufferedToken();
        memclrw(&buf, sizeof(buf));
        CParser_GetDeclSpecs(&buf, 1);
        if ((SInt32)buf.storageclass == 0x101 || (SInt32)buf.storageclass == 0x100) {
            CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            buf.storageclass = 0;
        }
        if (tk != ';')
            CDecl_ScanDeclarator(&buf);
        else
            CParser_CheckAnonymousUnion(&buf, 0);
        tk = CPrepTokenizer_GetNextToken();
    } else {
        CError_ReportError(ERR_UNEXPECTED_END_FILE);
    }
}

void parse_linkage_specification(DeclInfo *decl)
{
    UInt8 linkageFlag;
    SInt32 language;

    if (memcmp(string_token_data, "C", 2) == 0 || memcmp(string_token_data, "Objective C", 12) == 0) {
        language = 0;
        linkageFlag = 1;
    } else if (memcmp(string_token_data, "C++", 4) == 0) {
        language = 0;
        linkageFlag = 0;
    } else if (memcmp(string_token_data, "Pascal", 7) == 0) {
        language = 8;
        linkageFlag = 1;
    } else {
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        language = 0;
        linkageFlag = 1;
    }

    tk = CPrepTokenizer_GetNextToken();
    if (tk == '{') {
        for (;;) {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == 0) {
                CError_ReportError(ERR_RBRACE_EXPECTED);
                break;
            }
            if (tk == '}')
                break;
            CPrep_GetFOI(&function_fileinfo, NULL);
            data_00587184 = data_00588454;
            declaration_token = *CPrep_GetLastBufferedToken();
            memclrw(decl, sizeof(*decl));
            decl->requireMangledName = linkageFlag;
            decl->qual = language;
            parse_declaration(decl);
        }
    } else {
        if (tk == TK_EXTERN && copts.cpp_extensions && CPrepTokenizer_GetNextTokenAndRestorePosition() == -4) {
            tk = CPrepTokenizer_GetNextToken();
            parse_linkage_specification(decl);
            return;
        }
        memclrw(decl, sizeof(*decl));
        decl->requireMangledName = linkageFlag;
        decl->qual = language;
        CParser_GetDeclSpecs(decl, 1);
        if (decl->storageclass != TK_TYPEDEF) {
            if (decl->storageclass != TK_EOF && copts.extended_errorcheck)
                CError_Warning(ERR_ILLEGAL_STORAGE_CLASS);
            if (decl->storageclass == TK_EOF)
                decl->storageclass = TK_EXTERN;
        }
        if (copts.cpp_extensions)
            decl->missingTypeSpecifier = 0;
        if (tk != ';')
            CDecl_ScanDeclarator(decl);
    }
}

void parse_namespace_declaration(void *declarationData)
{
    DeclInfo *declaration = declarationData;
    NameSpaceList *usingEntry;
    CScopeSave save;
    Boolean isUnnamed;
    NameSpace *nspace;
    ObjNameSpace *object;
    HashNameNode *name;
    NameSpaceObjectList *found;
    TStreamElement *location;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == TK_IDENTIFIER) {
        name = data_00587fa0;
        isUnnamed = 0;
        tk = CPrepTokenizer_GetNextToken();
        if (tk == '=') {
            CScope_ParseNameSpaceAlias(name);
            return;
        }
    } else {
        if (tk != '{') {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return;
        }
        name = GetHashNameNode("@unnamed@");
        isUnnamed = 1;
    }
    nspace = cscope_current;
    found = CScope_FindName(nspace, name);
    if (found == NULL) {
        object = galloc(sizeof(*object));
        memclrw(object, sizeof(*object));
        object->otype = OT_NAMESPACE;
        object->access = ACCESSPUBLIC;
        if (isUnnamed) {
            nspace = CScope_NewListNameSpace(name, 1);
            nspace->is_unnamed = 1;
            usingEntry = galloc(sizeof(*usingEntry));
            usingEntry->next = cscope_current->usings;
            usingEntry->nspace = nspace;
            cscope_current->usings = usingEntry;
        } else {
            nspace = CScope_NewHashNameSpace(name);
            if (cscope_current->is_unnamed != 0)
                nspace->is_unnamed = 1;
        }
        nspace->parent = cscope_current;
        object->nspace = nspace;
        CScope_AddObject(cscope_current, name, (ObjBase *)object);
    } else {
        if (found->object->otype != OT_NAMESPACE) {
            CError_ReportError(ERR_ILLEGAL_NAMESPACE);
        } else {
            object = (ObjNameSpace *)found->object;
            nspace = object->nspace;
        }
    }
    if (tk != '{') {
        CError_ReportError(ERR_LBRACE_EXPECTED);
        return;
    }
    CScope_SetNameSpaceScope(nspace, &save);
    for (;;) {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == 0) {
            CError_ReportError(ERR_RBRACE_EXPECTED);
            break;
        }
        if (tk == '}')
            break;
        CPrep_GetFOI(&function_fileinfo, NULL);
        data_00587184 = data_00588454;
        location = CPrep_GetLastBufferedToken();
        declaration_token = *location;
        memclrw(declaration, sizeof(*declaration));
        parse_declaration(declaration);
    }
    CScope_RestoreScope(&save);
}

void parse_declaration(DeclInfo *p)
{
    switch (tk) {
        case TK_AT_INTERFACE:
            fn_00505cc0();
            return;
        case TK_AT_IMPLEMENTATION:
            fn_00505cb0();
            return;
        case TK_AT_PROTOCOL:
            CObjC_ParseProtocol();
            return;
        case TK_AT_CLASS:
            CObjC_ParseIdentifierList();
            return;
        case TK_NAMESPACE:
            parse_namespace_declaration(p);
            return;
        case TK_EXPORT:
            CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_TEMPLATE) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
            }
        case TK_TEMPLATE:
            CTemplateNew_ParseTemplateDeclaration(NULL);
            return;
        case TK_USING:
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_NAMESPACE) {
                tk = CPrepTokenizer_GetNextToken();
                CScope_ParseUsingDirective(cscope_current);
            } else {
                CScope_ParseUsingDeclaration(cscope_current, ACCESSPUBLIC, 0);
            }
            return;
        case TK_EXTERN:
            if (copts.cplusplus != 0) {
                p->storageclass = TK_EXTERN;
                tk = CPrepTokenizer_GetNextToken();
                if (tk == TK_STRING) {
                    parse_linkage_specification(p);
                    return;
                }
            }
        default:
            CParser_GetDeclSpecs(p, 1);
            if ((SInt32)p->storageclass == TK_REGISTER || (SInt32)p->storageclass == TK_AUTO) {
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                p->storageclass = TK_EOF;
            }
            if (tk != ';') {
                CDecl_ScanDeclarator(p);
            } else {
                CParser_CheckAnonymousUnion(p, 0);
            }
            fn_0048c220(0);
            return;
    }
}

void cparser(void)
{
    DeclInfo local;

    tk = CPrepTokenizer_GetNextToken();
    if (tk != 0) {
        do {
            CPrep_GetFOI(&function_fileinfo, NULL);
            data_00587184 = data_00588454;
            declaration_token = *CPrep_GetLastBufferedToken();
            memclrw(&local, sizeof(local));
            parse_declaration(&local);
            if (tk == 0)
                break;
            tk = CPrepTokenizer_GetNextToken();
        } while (tk != 0);
    } else if (copts.cplusplus == 0 && copts.ANSIstrict != 0) {
        CError_ReportError(ERR_UNEXPECTED_END_FILE);
    }
    CInit_DefineTentativeData();
    copts.defer_codegen = 0;
    fn_0048c220(1);
    if (cprep_cu[0xe0] != 1) {
        CInline_GeneratePendingFunctionBody();
        fn_0048c220(1);
    }
    CClass_GenThunks();
    if (cprep_cu[0xe0] != 1) {
        CObjCModern_GenerateSymbolTableAndModule();
    }
    CSOM_GenerateRefNodeCode();
    CInit_DefineTentativeData();
}

static Boolean IsAnonymousUnion(DeclInfo *context)
{
    return IsUnionType(context) && IsAnonymousName(((TypeClass *)context->thetype)->classname);
}

static int IsUnionType(DeclInfo *context)
{
    return context->thetype->type == TYPECLASS && ((TypeClass *)context->thetype)->mode == 1;
}

static Boolean IsAnonymousName(HashNameNode *name)
{
    return IsTempName(name) || name->name[0] == '$';
}

static int IsTempName(HashNameNode *name)
{
    return name == NULL || name->name[0] == '@';
}
