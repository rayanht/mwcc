#define CERROR_FILE "CDecl.c"
#include "compiler/common.h"
#include "compiler/CDecl.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CBrowse.h"
#include "compiler/CClass.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
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
#include "compiler/CTemplateClass.h"
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
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "compiler/BitVector.h"
#include "compiler/Types.h"
#include <string.h>

typedef enum { bt_false, bt_true } btype;

static inline void begin_class_instantiation(TemplClassInst *instance, const DeclInfo *ctx)
{
    instance->is_instantiated = 1;
    if (ctx->pendingClass == NULL)
        instance->is_specialized = 1;
}

static inline Boolean class_browse_enabled(const DeclInfo *ctx)
{
    const CPrepCU *compilationUnit = (const CPrepCU *)cprep_cu;
    return compilationUnit->browseOptions.browseOption != 0 && ctx->file->recordbrowseinfo != 0;
}

#define SETFUNC(f, t, q)                                                                                               \
    (f)->functype = (t);                                                                                               \
    (f)->qual = (q)

static void AddShortArg(TypeFunc *tfunc)
{
    FuncArg *arg;

    arg = CParser_NewFuncArg();
    arg->type = (Type *)&stsignedshort;
    arg->next = tfunc->args;
    tfunc->args = arg;
    if (arg->next && arg->next->type == &stvoid)
        arg->next = NULL;
}

static inline Boolean CheckMemberType1(Type *type)
{
    switch ((SInt8)type->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            return 0;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            return 0;
        case TYPECLASS:
            if (((TypeClass *)type)->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                return 0;
            }
            break;
    }
    return 1;
}

static inline Boolean CheckMemberType(Type *type)
{
    if (!CheckMemberType1(type))
        return 0;
    if (type->type == TYPECLASS) {
        if (((TypeClass *)type)->flags & CLASS_HANDLEOBJECT) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            return 0;
        }
        if (((TypeClass *)type)->objcinfo) {
            CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
            return 0;
        }
    }
    return 1;
}

#define MAKE_NODE(dst, sp)                                                                                             \
    do {                                                                                                               \
        (dst) = galloc(0x12);                                                                                          \
        memclrw((dst), 0x12);                                                                                          \
        (dst)->type = TYPESTRUCT;                                                                                      \
        TYPE_STRUCT(dst)->align = 1;                                                                                   \
        TYPE_STRUCT(dst)->stype = (sp);                                                                                \
    } while (0)

static void attach_node(Type *p, HashNameNode *name)
{
    SInt32 b;
    TYPE_STRUCT(p)->name = name;
    b = (in_parameter_type_list != 0 && !copts.cplusplus);
    CScope_DefineTypeTag((b ? cscope_root : cscope_current), name, p);
}

static Boolean HighIsNegative(SInt32 hi)
{
    return (hi & 0x80000000) != 0;
}

static TypeIntegral *SignedIntType(SInt32 size)
{
    if (stsignedchar.size == size)
        return &stsignedchar;
    if (stsignedshort.size == size)
        return &stsignedshort;
    if (stsignedint.size == size)
        return &stsignedint;
    if (stsignedlong.size != size && copts.longlong != 0 && copts.longlong_enums != 0 && stsignedlonglong.size == size)
        return &stsignedlonglong;
    return &stsignedlong;
}

static Type *UnsignedIntType(SInt32 size)
{
    if (stunsignedchar.size == size)
        return (Type *)&stunsignedchar;
    if (stunsignedshort.size == size)
        return (Type *)&stunsignedshort;
    if (stunsignedint.size == size)
        return (Type *)&stunsignedint;
    if (stunsignedlong.size != size && copts.longlong != 0 && copts.longlong_enums != 0 &&
        stunsignedlonglong.size == size)
        return (Type *)&stunsignedlonglong;
    return (Type *)&stunsignedlong;
}

static Boolean IsNegative(SInt32 hi)
{
    return (hi & (SInt32)0x80000000) != 0;
}

static Boolean IsZero(CInt64 *v)
{
    return v->hi == 0 && v->lo == 0;
}

static TypeIntegral *FindSignedType(SInt32 size)
{
    if (stsignedchar.size == size)
        return &stsignedchar;
    if (stsignedshort.size == size)
        return &stsignedshort;
    if (stsignedint.size == size)
        return &stsignedint;
    if (stsignedlong.size == size)
        return &stsignedlong;
    if (copts.longlong != 0 && copts.longlong_enums != 0 && stsignedlonglong.size == size)
        return &stsignedlonglong;
    return &stsignedlong;
}

static TypeIntegral *FindUnsignedType(SInt32 size)
{
    if (stunsignedchar.size == size)
        return &stunsignedchar;
    if (stunsignedshort.size == size)
        return &stunsignedshort;
    if (stunsignedint.size == size)
        return &stunsignedint;
    if (stunsignedlong.size == size)
        return &stunsignedlong;
    if (copts.longlong != 0 && copts.longlong_enums != 0 && stunsignedlonglong.size == size)
        return &stunsignedlonglong;
    return &stunsignedlong;
}

static Type *CDecl_LargerType(Type *a, Type *b)
{
    if (a->size > b->size)
        return a;
    if (b->size > a->size)
        return b;
    return a;
}

static inline UInt8 CDecl_UseIntEnums(void)
{
    return copts.enumsalwaysint;
}

/* A constructor's hidden argument: a signed short added in front of the parameter list (a leading void list ends). */
static void CDecl_AddArgument(TypeFunc *tfunc)
{
    FuncArg *arg;

    arg = CParser_NewFuncArg();
    arg->type = (Type *)&stsignedshort;
    arg->next = tfunc->args;
    tfunc->args = arg;
    if (arg->next && arg->next->type == (Type *)&stvoid)
        arg->next = NULL;
}

static inline int parsePointerQualifiers(void)
{
    int qualifiers = tk == TK_BITAND ? 32 : 0;
    tk = CPrepTokenizer_GetNextToken();
    for (;;) {
        switch (tk) {
            default:
                return qualifiers;
            case TK_CONST:
                if (qualifiers & Q_CONST)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                qualifiers |= Q_CONST;
                break;
            case TK_VOLATILE:
                if (qualifiers & Q_VOLATILE)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                qualifiers |= Q_VOLATILE;
                break;
            case TK_RESTRICT:
                if (qualifiers & Q_RESTRICT)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                qualifiers |= Q_RESTRICT;
                break;
        }
        tk = CPrepTokenizer_GetNextToken();
    }
}

static UInt8 IsClassEnumOrRef(Type *p)
{
    if (p->type == TYPECLASS || p->type == TYPEENUM)
        return 1;
    if (!(p->type == TYPEPOINTER && (((TypePointer *)p)->qual & Q_REFERENCE)))
        return 0;
    p = ((TypePointer *)p)->target;
    return p->type == TYPECLASS || p->type == TYPEENUM;
}

static int IsSingleArg(FuncArg *p)
{
    return p != &data_00583098 && p->next == NULL;
}

static int IsMethod(DeclInfo *a, char f)
{
    return f && (((TypeMemberFunc *)a->thetype)->flags & FUNC_METHOD);
}

static int IsNonStatic(DeclInfo *a)
{
    return !((TypeMemberFunc *)a->thetype)->is_static;
}

inline Boolean IsValidReturnType(Type *ctype)
{
    switch ((SInt8)ctype->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            return 0;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            return 0;
        case TYPECLASS:
            if (TYPE_CLASS(ctype)->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(ctype));
                return 0;
            }
            break;
    }
    return 1;
}

inline Boolean CheckReturnType(Type *ctype)
{
    if (ctype->type == TYPECLASS && TYPE_CLASS(ctype)->sominfo != NULL) {
        CError_ReportError(ERR_FUNCTIONS_CANNOT_RETURN_SOM_CLASSES);
        return 0;
    }
    if (!IsValidReturnType(ctype))
        return 0;
    if (ctype->type == TYPECLASS) {
        if (TYPE_CLASS(ctype)->flags & CLASS_HANDLEOBJECT) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            return 0;
        }
        if (TYPE_CLASS(ctype)->objcinfo != NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
            return 0;
        }
    }
    return 1;
}

static Boolean checkClass(Type *type, Boolean result)
{
    if (!result)
        return 0;
    if (type->type != TYPECLASS)
        return 1;
    if (TYPE_CLASS(type)->flags & CLASS_HANDLEOBJECT) {
        CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
        return 0;
    }
    if (TYPE_CLASS(type)->objcinfo != NULL) {
        CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
        return 0;
    }
    return 1;
}

static inline unsigned char checkObjectType(TypeClass *type)
{
    switch ((signed char)type->type) {
        case TYPEVOID:
            CError_ReportError(126U);
            return 0;
        case TYPEFUNC:
            CError_ReportError(146U);
            return 0;
        case TYPECLASS:
            if (type->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                return 0;
            }
            break;
    }
    return 1;
}

inline Boolean checkType(Type *t)
{
    if (!CanAllocObject(t))
        return 0;
    if (t->type == TYPECLASS && TYPE_CLASS(t)->sominfo != NULL) {
        CError_ReportError(ERR_SOM_CLASS_ARRAYS_NOT_SUPPORTED);
        return 0;
    }
    if (t->type == TYPEPOINTER && (TYPE_POINTER(t)->qual & Q_REFERENCE)) {
        CError_ReportError(ERR_ILLEGAL_AMPERSAND_REFERENCE);
        return 0;
    }
    if (!IsValidReturnType(t))
        return 0;
    if (t->type == TYPECLASS) {
        if (TYPE_CLASS(t)->flags & CLASS_HANDLEOBJECT) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            return 0;
        }
        if (TYPE_CLASS(t)->objcinfo != NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
            return 0;
        }
    }
    return 1;
}

Type *CDecl_NewStructType(SInt32 size, SInt16 align)
{
    struct TypeStruct *type;
    type = (struct TypeStruct *)galloc(18U);
    memclrw(type, 18U);
    type->type = TYPESTRUCT;
    type->size = size;
    type->align = (short)align;
    type->stype = 0;
    return (Type *)type;
}

Type *CDecl_NewArrayType(Type *elementType, SInt32 size)
{
    TypePointer *arrayType;
    arrayType = galloc(sizeof(TypePointer));
    memclrw(arrayType, sizeof(TypePointer));
    arrayType->type = TYPEARRAY;
    arrayType->size = size;
    arrayType->target = elementType;
    arrayType->qual = 0;
    return (Type *)arrayType;
}

Type *CDecl_NewPointerType(Type *targetType)
{
    TypePointer *pointerType;

    pointerType = (TypePointer *)galloc(14);
    memclrw(pointerType, 14);
    pointerType->type = TYPEPOINTER;
    pointerType->size = 4;
    pointerType->target = targetType;
    return (Type *)pointerType;
}

TypeTemplDep *CDecl_NewTemplDepType(UInt8 templateKind)
{
    TypeTemplDep *templateType;

    templateType = (TypeTemplDep *)galloc(sizeof(TypeTemplDep));
    memclrw(templateType, sizeof(TypeTemplDep));
    templateType->type = TYPETEMPLATE;
    templateType->size = 1;
    templateType->dtype = templateKind;
    return templateType;
}

void CDecl_NewConvFuncType(DeclInfo *state)
{
    TypeFunc *functionType;
    Type *originalType;
    unsigned int qualifiers;
    HashNameNode *name;
    Type *returnType;
    unsigned int returnQualifiers;
    functionType = (TypeFunc *)galloc(26U);
    memclrw(functionType, 26U);
    originalType = state->thetype;
    qualifiers = state->qual;
    name = CMangler_ConversionFuncName(originalType, qualifiers);
    state->name = name;
    functionType->type = TYPEFUNC;
    returnType = state->thetype;
    functionType->functype = returnType;
    returnQualifiers = state->qual;
    functionType->qual = (returnQualifiers & Q_CV);
    state->isType = 0U;
    state->thetype = (Type *)functionType;
    state->storageclass = 0U;
}

int CDecl_CompleteType(Type *type)
{
    int result;
    TypeClass *classType;
    switch ((signed char)type->type) {
        case TYPEPOINTER:
            if ((((TypePointer *)type)->qual & Q_REFERENCE) != 0) {
                type = ((TypePointer *)type)->target;
                if (type->type == TYPECLASS) {
                    break;
                }
            }
            return;
        case TYPEARRAY:
            do {
                type = ((TypePointer *)type)->target;
            } while (type->type == TYPEARRAY);
            if (type->type == TYPECLASS) {
                break;
            }
            return;
        default:
            return result;
        case TYPECLASS:
            break;
    }
    classType = (TypeClass *)type;
    if ((result = classType->flags & (0x800 | CLASS_COMPLETED)) == 0x800) {
        CTemplateClass_InstantiateClass((TypeClass *)type);
    }
    return;
}

Boolean CanAllocObject(Type *theclass)
{
    switch ((SInt8)theclass->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            return 0;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            return 0;
        case TYPESTRUCT:
            if (theclass->size == 0) {
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, theclass, 0);
                return 0;
            }
            return 1;
        case TYPECLASS:
            if ((TYPE_CLASS(theclass)->flags & CLASS_COMPLETED) == 0 &&
                ((TYPE_CLASS(theclass)->flags & CLASS_IS_TEMPL_INST) == 0 ||
                 CTemplateClass_InstantiateClass(TYPE_CLASS(theclass)) == 0)) {
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, theclass, 0);
                return 0;
            }
            return 1;
        default:
            if (theclass->size == 0) {
                CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
                return 0;
            }
            return 1;
    }
}

Boolean CanCreateObject(Type *tc)
{
    switch ((SInt8)tc->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            return 0;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            return 0;
        case TYPECLASS:
            if (TYPE_CLASS(tc)->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(tc));
                return 0;
            }
            break;
    }
    return 1;
}

unsigned char CDecl_CheckObjectType(Type *type)
{
    if (!checkObjectType((TypeClass *)type))
        return 0;
    if (type->type == TYPECLASS) {
        if (TYPE_CLASS(type)->flags & CLASS_HANDLEOBJECT) {
            CError_ReportError(191U);
            return 0;
        }
        if (TYPE_CLASS(type)->objcinfo != NULL) {
            CError_ReportError(307U);
            return 0;
        }
    }
    return 1;
}

Boolean check_object_creation_type(Type *type)
{
    Boolean result;

    while (IS_TYPE_ARRAY(type))
        type = TPTR_TARGET(type);
    switch ((SInt8)type->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            result = FALSE;
            break;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            result = FALSE;
            break;
        case TYPECLASS:
            if (TYPE_CLASS(type)->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                result = FALSE;
                break;
            }
            result = TRUE;
            break;
        default:
            result = TRUE;
            break;
    }
    if (!result)
        result = FALSE;
    else {
        if (type->type == TYPECLASS) {
            if (TYPE_CLASS(type)->flags & CLASS_HANDLEOBJECT) {
                CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
                result = FALSE;
            } else if (TYPE_CLASS(type)->objcinfo != NULL) {
                CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
                result = FALSE;
            } else
                result = TRUE;
        } else
            result = TRUE;
    }
    if (!result)
        return FALSE;
    if (type->type == TYPECLASS && (CClass_Destructor(TYPE_CLASS(type)) || CClass_Constructor(TYPE_CLASS(type)))) {
        CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
        return FALSE;
    }
    return TRUE;
}

void CDecl_WrapTypePointer(Type **type, unsigned int flags)
{
    Type *wrappedType;
    TypePointer *pointerType;
    TypePointer *result;
    wrappedType = *type;
    pointerType = (TypePointer *)galloc(14U);
    memclrw(pointerType, 14U);
    pointerType->type = TYPEPOINTER;
    pointerType->size = 4U;
    pointerType->target = wrappedType;
    *type = (Type *)pointerType;
    result = (TypePointer *)*type;
    result->qual = flags;
}

void prepend_class_pointer_argument(TypeFunc *owner, TypeClass *classType, char parseModifiers)
{
    FuncArg *argument;
    TypePointer *pointerType;
    int token;

    argument = CParser_NewFuncArg();
    argument->name = this_arg_name;
    if (parseModifiers != '\0') {
        while (1) {
            if ((token = tk) == 0x121) {
                tk = CPrepTokenizer_GetNextToken();
                argument->qual |= Q_CONST;
            } else if (token == 0x122) {
                tk = CPrepTokenizer_GetNextToken();
                argument->qual |= Q_VOLATILE;
            } else {
                break;
            }
        }
        if (token == 0x14e) {
            CExcept_ScanExceptionSpecification(owner);
        }
    }
    pointerType = (TypePointer *)galloc(sizeof(TypePointer));
    memclrw(pointerType, sizeof(TypePointer));
    pointerType->type = TYPEPOINTER;
    pointerType->size = 4;
    pointerType->qual = Q_CONST;
    if (classType->sominfo == NULL) {
        pointerType->target = (Type *)classType;
    } else {
        pointerType->target = &stvoid;
    }
    argument->type = (Type *)pointerType;
    argument->next = owner->args;
    owner->args = argument;
}

void CDecl_MakePTMFuncType(TypeFunc *func)
{
    FuncArg *arg;
    TypePointer *tp;
    SInt32 q;

    tp = (TypePointer *)galloc(sizeof(TypePointer));
    memclrw(tp, sizeof(TypePointer));
    tp->type = TYPEPOINTER;
    tp->size = 4;
    tp->target = &stvoid;
    tp->qual = Q_CONST;

    arg = CParser_NewFuncArg();
    arg->name = this_arg_name;
    arg->type = (Type *)tp;
    for (;;) {
        q = tk;
        if (q == 0x121) {
            tk = CPrepTokenizer_GetNextToken();
            arg->qual |= Q_CONST;
        } else if (q == 0x122) {
            tk = CPrepTokenizer_GetNextToken();
            arg->qual |= Q_VOLATILE;
        } else {
            break;
        }
    }
    arg->next = func->args;
    func->args = arg;

    arg = CParser_NewFuncArg();
    arg->name = this_arg_name;
    arg->type = (Type *)tp;
    arg->qual = Q_CONST;
    arg->next = func->args;
    func->args = arg;

    func->flags |= 0x80;
}

void CDecl_PrependFuncArg(TypeFunc *type, TypeIntegral *argtype)
{
    FuncArg *arg;
    arg = CParser_NewFuncArg();
    arg->type = (Type *)argtype;
    arg->next = type->args;
    type->args = arg;
    if (arg->next != NULL && arg->next->type == &stvoid)
        arg->next = NULL;
}

Boolean CDecl_CheckArrayIntegr(Type *type)
{
    Boolean ok;
    Boolean result;
    Type *theclass = type;

    if (!CanAllocObject(theclass))
        return 0;

    if (type->type == TYPECLASS && TYPE_CLASS(type)->sominfo != NULL) {
        CError_ReportError(ERR_SOM_CLASS_ARRAYS_NOT_SUPPORTED);
        return 0;
    }

    if (type->type == TYPEPOINTER && (TYPE_POINTER(type)->qual & Q_REFERENCE)) {
        CError_ReportError(ERR_ILLEGAL_AMPERSAND_REFERENCE);
        return 0;
    }

    switch ((SInt8)type->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            ok = 0;
            break;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            ok = 0;
            break;
        case TYPECLASS:
            if (TYPE_CLASS(type)->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                ok = 0;
            } else {
                ok = 1;
            }
            break;
        default:
            ok = 1;
            break;
    }

    if (!ok) {
        result = 0;
    } else if (type->type == TYPECLASS) {
        if (TYPE_CLASS(type)->flags & CLASS_HANDLEOBJECT) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            result = 0;
        } else if (TYPE_CLASS(type)->objcinfo != NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
            result = 0;
        } else {
            result = 1;
        }
    } else {
        result = 1;
    }

    return result;
}

Boolean check_function_return_type(Type *type)
{
    Boolean result;

    if (type->type == TYPEVOID)
        return 1;
    if (type->type == TYPECLASS && TYPE_CLASS(type)->sominfo != NULL) {
        CError_ReportError(ERR_FUNCTIONS_CANNOT_RETURN_SOM_CLASSES);
        return 0;
    }
    switch ((SInt8)type->type) {
        case TYPEVOID:
            CError_ReportError(ERR_ILLEGAL_USE_VOID);
            result = 0;
            break;
        case TYPEFUNC:
            CError_ReportError(ERR_ILLEGAL_TYPE);
            result = 0;
            break;
        case TYPECLASS:
            if (TYPE_CLASS(type)->flags & CLASS_ABSTRACT) {
                CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                result = 0;
                break;
            }
            /* fall through */
        default:
            result = 1;
            break;
    }
    return checkClass(type, result);
}

void fn_00504240(TypeFunc *type)
{
    return;
}

void CDecl_ParseDirectFuncDecl(DeclInfo *d)
{
    TypeFunc *ft;
    Type *t;
    FuncArg *args;
    Boolean ok;

    if (copts.cplusplus != 0 && d->name != NULL && d->thetype->type != TYPEVOID &&
        !CParser_TryParamList(d->thetype->type != TYPECLASS))
        return;

    tk = CPrepTokenizer_GetNextToken();
    if (tk == ')') {
        if (copts.cplusplus == 0)
            args = &data_00584748;
        else
            args = NULL;
    } else {
        args = parameter_type_list(d);
    }

    if (tk != ')')
        CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
    else
        tk = CPrepTokenizer_GetNextToken();

    ft = (TypeFunc *)galloc(sizeof(TypeFunc));
    memclrw(ft, sizeof(TypeFunc));
    ft->type = TYPEFUNC;
    ft->args = args;

    if (d->qual & Q_PASCAL) {
        d->qual &= ~Q_PASCAL;
        ft->flags = FUNC_PASCAL;
    }

    if (tk == TK_THROW) {
        if (d->storageclass == TK_TYPEDEF)
            CError_ReportError(ERR_ILLEGAL_EXCEPTION_SPECIFICATION);
        CExcept_ScanExceptionSpecification(ft);
        if ((SInt32)tk == TK_CONST || (SInt32)tk == TK_VOLATILE)
            CError_ReportError(ERR_ILLEGAL_TYPE_QUALIFIERS);
    }

    scandeclarator(d);

    t = d->thetype;
    if (t->type == TYPEVOID)
        ok = 1;
    else
        ok = CheckReturnType(t);
    if (!ok)
        d->thetype = &stvoid;
    ft->functype = d->thetype;
    ft->qual = d->qual & Q_CV;
    d->thetype = (Type *)ft;
    d->isType = 0;
}

/* Type descriptor for an array with a nonconstant bound. */
void scandeclarator(DeclInfo *decl)
{
    Boolean unsized;
    CInt64 count;
    ENode *bound;
    Type *array;
    SInt32 size;
    Type *elementType;

    unsized = 0;
    if (tk == '[') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == ']') {
            count = cint64_zero;
            tk = CPrepTokenizer_GetNextToken();
            unsized = 1;
        } else {
            if (decl->isNewExpression == 0 || decl->hasArrayDimension != 0) {
                bound = CExpr_IntegralConstOrDepExpr();
                if (bound->type != EINTCONST) {
                    if (tk != ']')
                        CError_ReportErrorAndUpdateToken(ERR_RBRACKET_EXPECTED);
                    else
                        tk = CPrepTokenizer_GetNextToken();
                    decl->hasArrayDimension = 1;
                    scandeclarator(decl);
                    if (!checkType(decl->thetype))
                        decl->thetype = (Type *)&stsignedchar;
                    {
                        TypeTemplDep *boundType = (TypeTemplDep *)galloc(sizeof(TypeTemplDep));
                        memclrw(boundType, sizeof(TypeTemplDep));
                        boundType->type = TYPETEMPLATE;
                        boundType->size = 1;
                        boundType->dtype = 3;
                        boundType->u.array.type = decl->thetype;
                        boundType->u.array.index = fn_00513040(bound, 1);
                        decl->thetype = (Type *)boundType;
                    }
                    return;
                }
                count = bound->data.intval;
                if (CInt64_IsNegative(&count)) {
                    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                    count = cint64_one;
                } else if (CInt64_IsZero(&count)) {
                    if (copts.ANSIstrict == 0 && decl->isStructMemberDeclarator != 0) {
                        unsized = 1;
                    } else {
                        CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                        count = cint64_one;
                    }
                }
            } else {
                count = cint64_one;
                bound = s_expression();
                if (bound->rtype->type == TYPEINT) {
                    if (bound->type != EINTCONST)
                        decl->arrayBound = bound;
                    else
                        count = bound->data.intval;
                } else {
                    CError_ReportError(ERR_ILLEGAL_CONSTANT_EXPRESSION);
                }
            }
            if (tk != ']')
                CError_ReportErrorAndUpdateToken(ERR_RBRACKET_EXPECTED);
            else
                tk = CPrepTokenizer_GetNextToken();
        }
        decl->hasArrayDimension = 1;
        scandeclarator(decl);
        if (!unsized && !checkType(decl->thetype))
            decl->thetype = (Type *)&stsignedchar;
        elementType = decl->thetype;
        size = elementType->size * count.lo;
        array = (Type *)galloc(sizeof(TypePointer));
        memclrw(array, sizeof(TypePointer));
        array->type = TYPEARRAY;
        array->size = size;
        TPTR_TARGET(array) = elementType;
        TYPE_POINTER(array)->qual = 0;
        decl->thetype = array;
    } else if (tk == '(') {
        CDecl_ParseDirectFuncDecl(decl);
    }
}

void replace_nested_type(Type *oldtype, Type *ty, Type *newtype)
{
    SInt32 oldsize;
    Boolean ok;

    for (;;) {
        switch ((SInt8)ty->type) {
            case TYPEPOINTER:
                if (TYPE_POINTER(ty)->target == oldtype) {
                    TYPE_POINTER(ty)->target = newtype;
                    return;
                }
                ty = TYPE_POINTER(ty)->target;
                break;

            case TYPEMEMBERPOINTER:
                if (TYPE_MEMBER_POINTER(ty)->ty1 == oldtype) {
                    TYPE_MEMBER_POINTER(ty)->ty1 = newtype;
                    if (newtype->type == TYPEFUNC) {
                        CDecl_MakePTMFuncType(TYPE_FUNC(newtype));
                        ty->size = 0xc;
                    } else {
                        ty->size = 4;
                    }
                    return;
                }
                ty = TYPE_MEMBER_POINTER(ty)->ty1;
                break;

            case TYPEARRAY:
                oldsize = TYPE_POINTER(ty)->target->size;
                if (TYPE_POINTER(ty)->target == oldtype) {
                    if (!CanAllocObject(newtype)) {
                        ok = 0;
                    } else {
                        if (newtype->type == TYPECLASS && TYPE_CLASS(newtype)->sominfo != NULL) {
                            CError_ReportError(ERR_SOM_CLASS_ARRAYS_NOT_SUPPORTED);
                            ok = 0;
                        } else if (newtype->type == TYPEPOINTER && (TYPE_POINTER(newtype)->qual & Q_REFERENCE)) {
                            CError_ReportError(ERR_ILLEGAL_AMPERSAND_REFERENCE);
                            ok = 0;
                        } else {
                            switch ((SInt8)newtype->type) {
                                case TYPEVOID:
                                    CError_ReportError(ERR_ILLEGAL_USE_VOID);
                                    ok = 0;
                                    break;
                                case TYPEFUNC:
                                    CError_ReportError(ERR_ILLEGAL_TYPE);
                                    ok = 0;
                                    break;
                                case TYPECLASS:
                                    if (TYPE_CLASS(newtype)->flags & CLASS_ABSTRACT) {
                                        CError_IllegalUseAbstractClass(TYPE_CLASS(newtype));
                                        ok = 0;
                                    } else {
                                        ok = 1;
                                    }
                                    break;
                                default:
                                    ok = 1;
                                    break;
                            }
                            if (!ok) {
                                ok = 0;
                            } else {
                                if (newtype->type == TYPECLASS) {
                                    if (TYPE_CLASS(newtype)->flags & CLASS_HANDLEOBJECT) {
                                        CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
                                        ok = 0;
                                    } else if (TYPE_CLASS(newtype)->objcinfo != NULL) {
                                        CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
                                        ok = 0;
                                    } else {
                                        ok = 1;
                                    }
                                } else {
                                    ok = 1;
                                }
                            }
                        }
                    }
                    if (!ok)
                        newtype = (Type *)&stsignedchar;
                    if (oldsize != 0)
                        ty->size = (ty->size / oldsize) * newtype->size;
                    TYPE_POINTER(ty)->target = newtype;
                    return;
                }
                replace_nested_type(oldtype, TYPE_POINTER(ty)->target, newtype);
                if (oldsize != TYPE_POINTER(ty)->target->size && oldsize != 0)
                    ty->size = (ty->size / oldsize) * TYPE_POINTER(ty)->target->size;
                return;

            case TYPEFUNC:
                if (TYPE_FUNC(ty)->functype == oldtype) {
                    if (newtype->type == TYPEVOID)
                        ok = 1;
                    else
                        ok = CheckReturnType(newtype);
                    if (!ok)
                        newtype = &stvoid;
                    TYPE_FUNC(ty)->functype = newtype;
                    return;
                }
                ty = TYPE_FUNC(ty)->functype;
                break;

            default:
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
        }
    }
}

void replace_type_placeholder(Type *type, Type *ctype)
{
    for (;;) {
        SInt8 kind = type->type;
        switch (kind) {
            case TYPEPOINTER:
                if (TYPE_POINTER(type)->target == &type_placeholder) {
                    TYPE_POINTER(type)->target = ctype;
                    return;
                }
                type = TYPE_POINTER(type)->target;
                continue;
            case TYPEMEMBERPOINTER:
                if (TYPE_MEMBER_POINTER(type)->ty1 == &type_placeholder) {
                    TYPE_MEMBER_POINTER(type)->ty1 = ctype;
                    if (ctype->type == TYPEFUNC) {
                        CDecl_MakePTMFuncType(TYPE_FUNC(ctype));
                        type->size = 12;
                    } else
                        type->size = 4;
                    return;
                }
                type = TYPE_MEMBER_POINTER(type)->ty1;
                continue;
            case TYPEARRAY:
                if (TYPE_POINTER(type)->target == &type_placeholder) {
                    Boolean ok;
                    do {
                        ok = CanAllocObject(ctype);
                        if (!ok) {
                            ok = FALSE;
                            break;
                        }
                        if (ctype->type == TYPECLASS && TYPE_CLASS(ctype)->sominfo != NULL) {
                            CError_ReportError(ERR_SOM_CLASS_ARRAYS_NOT_SUPPORTED);
                            ok = FALSE;
                            break;
                        }
                        if (ctype->type == TYPEPOINTER && (TYPE_POINTER(ctype)->qual & Q_REFERENCE) != 0) {
                            CError_ReportError(ERR_ILLEGAL_AMPERSAND_REFERENCE);
                            ok = FALSE;
                            break;
                        }
                        {
                            SInt8 elementKind = ctype->type;
                            switch (elementKind) {
                                case TYPEVOID:
                                    CError_ReportError(ERR_ILLEGAL_USE_VOID);
                                    ok = FALSE;
                                    break;
                                case TYPEFUNC:
                                    CError_ReportError(ERR_ILLEGAL_TYPE);
                                    ok = FALSE;
                                    break;
                                case TYPECLASS:
                                    if (TYPE_CLASS(ctype)->flags & CLASS_ABSTRACT) {
                                        CError_IllegalUseAbstractClass(TYPE_CLASS(ctype));
                                        ok = FALSE;
                                    } else {
                                        ok = TRUE;
                                    }
                                    break;
                                default:
                                    ok = TRUE;
                                    break;
                            }
                        }
                        if (!ok) {
                            ok = FALSE;
                            break;
                        }
                        if (ctype->type == TYPECLASS) {
                            if (TYPE_CLASS(ctype)->flags & CLASS_HANDLEOBJECT) {
                                CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
                                ok = FALSE;
                                break;
                            }
                            if (TYPE_CLASS(ctype)->objcinfo != NULL) {
                                CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
                                ok = FALSE;
                                break;
                            }
                        }
                        ok = TRUE;
                    } while (FALSE);
                    if (!ok)
                        ctype = (Type *)&stsignedchar;
                    type->size = type->size * ctype->size;
                    TYPE_POINTER(type)->target = ctype;
                    return;
                } else {
                    SInt32 oldSize = TYPE_POINTER(type)->target->size;
                    replace_type_placeholder(TYPE_POINTER(type)->target, ctype);
                    if (oldSize != TYPE_POINTER(type)->target->size && oldSize != 0)
                        type->size = type->size / oldSize * TYPE_POINTER(type)->target->size;
                    return;
                }
            case TYPEFUNC:
                if (TYPE_FUNC(type)->functype == &type_placeholder) {
                    Boolean ok;
                    if (ctype->type == TYPEVOID)
                        ok = TRUE;
                    else
                        ok = CheckReturnType(ctype);
                    if (!ok)
                        ctype = &stvoid;
                    TYPE_FUNC(type)->functype = ctype;
                    return;
                }
                type = TYPE_FUNC(type)->functype;
                continue;
            case TYPETEMPLATE:
                if (((TypeTemplDep *)type)->dtype == 3) {
                    if (((TypeTemplDep *)type)->u.array.type == &type_placeholder) {
                        Boolean ok;
                        do {
                            ok = CanAllocObject(ctype);
                            if (!ok) {
                                ok = FALSE;
                                break;
                            }
                            if (ctype->type == TYPECLASS && TYPE_CLASS(ctype)->sominfo != NULL) {
                                CError_ReportError(ERR_SOM_CLASS_ARRAYS_NOT_SUPPORTED);
                                ok = FALSE;
                                break;
                            }
                            if (ctype->type == TYPEPOINTER && (TYPE_POINTER(ctype)->qual & Q_REFERENCE) != 0) {
                                CError_ReportError(ERR_ILLEGAL_AMPERSAND_REFERENCE);
                                ok = FALSE;
                                break;
                            }
                            {
                                SInt8 elementKind = ctype->type;
                                switch (elementKind) {
                                    case TYPEVOID:
                                        CError_ReportError(ERR_ILLEGAL_USE_VOID);
                                        ok = FALSE;
                                        break;
                                    case TYPEFUNC:
                                        CError_ReportError(ERR_ILLEGAL_TYPE);
                                        ok = FALSE;
                                        break;
                                    case TYPECLASS:
                                        if (TYPE_CLASS(ctype)->flags & CLASS_ABSTRACT) {
                                            CError_IllegalUseAbstractClass(TYPE_CLASS(ctype));
                                            ok = FALSE;
                                        } else {
                                            ok = TRUE;
                                        }
                                        break;
                                    default:
                                        ok = TRUE;
                                        break;
                                }
                            }
                            if (!ok) {
                                ok = FALSE;
                                break;
                            }
                            if (ctype->type == TYPECLASS) {
                                if (TYPE_CLASS(ctype)->flags & CLASS_HANDLEOBJECT) {
                                    CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
                                    ok = FALSE;
                                    break;
                                }
                                if (TYPE_CLASS(ctype)->objcinfo != NULL) {
                                    CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
                                    ok = FALSE;
                                    break;
                                }
                            }
                            ok = TRUE;
                        } while (FALSE);
                        if (!ok)
                            ctype = (Type *)&stsignedchar;
                        ((TypeTemplDep *)type)->u.array.type = ctype;
                        return;
                    }
                    type = ((TypeTemplDep *)type)->u.array.type;
                    continue;
                }
                CError_ReportError(ERR_ILLEGAL_TYPE);
                return;
            default:
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return;
        }
    }
}

unsigned int parse_parenthesized_declarator(DeclInfo *args)
{
    Type *savedNext;
    Type *newNext;
    savedNext = args->thetype;
    args->thetype = &type_placeholder;
    CDecl_ParseDeclarator(args);
    if (tk != ')') {
        CError_ReportErrorAndUpdateToken(115U);
    } else {
        tk = CPrepTokenizer_GetNextToken();
    }
    if ((newNext = args->thetype) == &type_placeholder) {
        args->thetype = savedNext;
        scandeclarator(args);
        return;
    }
    args->thetype = savedNext;
    scandeclarator(args);
    replace_type_placeholder(newNext, args->thetype);
    args->thetype = newNext;
}

Boolean check_operator_declaration(DeclInfo *declaration, Boolean isMember)
{
    short argumentCount;
    FuncArg *arguments;
    FuncArg *secondArgument;
    Type *resultType;
    int nonStaticMethod;

    if (((TypeFunc *)declaration->thetype)->type != TYPEFUNC) {
        CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
        return 0;
    }
    resultType = ((TypeFunc *)declaration->thetype)->functype;
    if ((arguments = ((TypeFunc *)declaration->thetype)->args) != NULL) {
        if (arguments != &data_00583098 && arguments != &data_00584748) {
            argumentCount = 1;
            if (arguments->dexpr != NULL) {
                switch (declaration->operator_token) {
                    case 0x145:
                    case 0x147:
                    case 0x182:
                    case 0x183:
                        break;
                    default:
                        CError_ReportError(ERR_ILLEGAL_DEFAULT_ARGUMENTS);
                }
            }
            if ((secondArgument = arguments->next) != NULL) {
                if (IsSingleArg(secondArgument) != 0)
                    argumentCount = 2;
                else
                    argumentCount = 3;
                if (secondArgument->dexpr != NULL) {
                    switch (declaration->operator_token) {
                        case 0x28:
                        case 0x145:
                        case 0x147:
                        case 0x182:
                        case 0x183:
                            break;
                        default:
                            CError_ReportError(ERR_ILLEGAL_DEFAULT_ARGUMENTS);
                    }
                }
            }
        } else {
            argumentCount = 3;
        }
    } else {
        CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
        return 0;
    }
    nonStaticMethod = 0;
    if (IsMethod((DeclInfo *)declaration, isMember) != 0) {
        if (IsNonStatic((DeclInfo *)declaration) != 0)
            nonStaticMethod = 1;
    }
    switch (declaration->operator_token) {
        case 0x147:
        case 0x182:
            if ((btype)nonStaticMethod != bt_false || iscpp_typeequal(resultType, (Type *)&void_ptr) == 0 ||
                argumentCount < 1 || arguments->type != CABI_GetSizeTType()) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            return 1;
        case 0x145:
        case 0x183:
            if ((btype)nonStaticMethod != bt_false || resultType->type != TYPEVOID || argumentCount < 1 ||
                iscpp_typeequal(arguments->type, (Type *)&void_ptr) == 0) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            if (argumentCount == 2 && (secondArgument->type != CABI_GetSizeTType() || isMember == 0)) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            return 1;
        case 0x3d:
            if ((btype)nonStaticMethod == bt_false) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            break;
        case 0x28:
            if ((btype)nonStaticMethod == bt_false) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            return 1;
        case 0x5b:
            if ((btype)nonStaticMethod == bt_false) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            break;
        case 0x170:
            if (argumentCount != 1 || (btype)nonStaticMethod == bt_false) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            return 1;
        case 0x16e:
        case 0x16f:
            if (argumentCount == 2 && secondArgument->type != (Type *)&stsignedint) {
                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                return 0;
            }
            break;
    }
    if (isMember != 0 && (btype)nonStaticMethod == bt_false) {
        CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
        return 0;
    }
    do {
        switch (declaration->operator_token) {
            case 0x26:
            case 0x2a:
            case 0x2b:
            case 0x2d:
            case 0x16e:
            case 0x16f:
                if (argumentCount != 1)
                    break;
            case 0x21:
            case 0x7e:
                if (argumentCount != 1)
                    continue;
                if (isMember == 0) {
                    if (!IsClassEnumOrRef(arguments->type))
                        continue;
                }
                return 1;
            case 0x25:
            case 0x2c:
            case 0x2f:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x5b:
            case 0x5e:
            case 0x7c:
            case 0x15c:
            case 0x15d:
            case 0x15e:
            case 0x15f:
            case 0x160:
            case 0x161:
            case 0x162:
            case 0x163:
            case 0x164:
            case 0x165:
            case 0x166:
            case 0x167:
            case 0x168:
            case 0x169:
            case 0x16a:
            case 0x16b:
            case 0x16c:
            case 0x16d:
            case 0x170:
            case 0x172:
            case 0x173:
                break;
            default:
                continue;
        }
        if (argumentCount == 2 &&
            (isMember != 0 || IsClassEnumOrRef(arguments->type) || IsClassEnumOrRef(secondArgument->type))) {
            return 1;
        }
    } while (0);
    CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
    return 0;
}

void parse_direct_declarator(DeclInfo *state, NameSpace *function)
{
    CScopeSave save;
    Boolean hasParserSlot;
    Boolean parsed;
    SInt32 parserOption;

    if (function != NULL) {
        CScope_SetNameSpaceScope(function, &save);
    }
    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == ')') {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            if (function != NULL) {
                CScope_RestoreScope(&save);
            }
            return;
        }
        if ((SInt32)tk >= 0x100 && (SInt32)tk <= 0x131) {
            if (state->name != NULL) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
            CDecl_ParseDirectFuncDecl(state);
        } else {
            parse_parenthesized_declarator(state);
        }
        if (function != NULL) {
            CScope_RestoreScope(&save);
        }
        return;
    }
    if (function != NULL) {
        if (tk == TK_OPERATOR) {
            if (state->operator_token != 0) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                parsed = 0;
            } else {
                state->operator_token = 0;
                parserOption = 0;
                if (state->parserOption != 0 && cscope_current->theclass != NULL) {
                    parserOption = 1;
                }
                parsed = CParser_00490660(&state->operator_token, parserOption);
            }
            if (parsed == 0) {
                CScope_RestoreScope(&save);
                return;
            }
            hasParserSlot = 1;
        } else {
            if (tk != TK_IDENTIFIER) {
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                CScope_RestoreScope(&save);
                return;
            }
            hasParserSlot = 0;
        }
        if (state->name != NULL) {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            CScope_RestoreScope(&save);
            return;
        }
        state->nspace = function;
        state->name = data_00587fa0;
        if (!hasParserSlot) {
            tk = CPrepTokenizer_GetNextToken();
        }
    } else {
        if (tk == TK_IDENTIFIER) {
            if (state->name != NULL) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
            state->name = data_00587fa0;
            tk = CPrepTokenizer_GetNextToken();
        } else if (tk == TK_OPERATOR) {
            if (state->operator_token != 0) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                parsed = 0;
            } else {
                state->operator_token = 0;
                parserOption = 0;
                if (state->parserOption != 0 && cscope_current->theclass != NULL) {
                    parserOption = 1;
                }
                parsed = CParser_00490660(&state->operator_token, parserOption);
            }
            if (parsed == 0) {
                return;
            }
            state->name = data_00587fa0;
        }
    }
    if (tk == '<' && state->allowTemplateArguments != 0) {
        state->expltargs = CTemplateNew_ParseTemplateArguments(NULL, 0);
        state->has_expltargs = 1;
        state->allowTemplateArguments = 0;
        tk = CPrepTokenizer_GetNextToken();
    }
    scandeclarator(state);
    if (function != NULL) {
        CScope_RestoreScope(&save);
    }
}

void CDecl_MakeMemberPointerType(Type **result, TypeClass *owner, unsigned int value)
{
    TypeMemberPointer *node;
    if (owner->flags & CLASS_HANDLEOBJECT) {
        CError_ReportError(191U);
        *result = (Type *)&stsignedint;
        return;
    }
    if (owner->sominfo != NULL) {
        CError_ReportError(290U);
        *result = (Type *)&stsignedint;
        return;
    }
    node = (TypeMemberPointer *)galloc(18U);
    node->type = TYPEMEMBERPOINTER;
    if ((*result)->type == TYPEFUNC) {
        TypeFunc *func = (TypeFunc *)*result;
        CDecl_MakePTMFuncType(func);
        node->size = 12U;
    } else {
        node->size = 4U;
    }
    node->ty1 = *result;
    node->ty2 = TYPE(owner);
    node->qual = value;
    *result = (Type *)node;
}

void CDecl_ScanPointer(DeclInfo *declarator, NameSpace *nspace, char finish)
{
    int qualifiers;
    TypePointer *type;
    char kind;
    NameSpace *foundNamespace;
    TypePointer *pointerType;
    NameResult lookup;
    for (;;) {
        qualifiers = parsePointerQualifiers();
        if ((kind = (type = (TypePointer *)declarator->thetype)->type) == TYPEPOINTER &&
                (type->qual & Q_REFERENCE) != 0 ||
            (qualifiers & Q_REFERENCE) != 0 && kind == TYPEVOID) {
            CError_ReportError(ERR_ILLEGAL_AMPERSAND_REFERENCE);
            return;
        }
        if (nspace != NULL) {
            CDecl_MakeMemberPointerType(&declarator->thetype, nspace->theclass, qualifiers);
            nspace = NULL;
        } else {
            pointerType = (TypePointer *)galloc(14);
            memclrw(pointerType, 14);
            pointerType->type = TYPEPOINTER;
            pointerType->size = 4;
            pointerType->target = (Type *)type;
            declarator->thetype = (Type *)pointerType;
            ((TypePointer *)declarator->thetype)->qual = qualifiers;
        }
        switch (tk) {
            case TK_BITAND:
                if (copts.cplusplus != 0)
                    continue;
                if (finish != 0)
                    parse_direct_declarator(declarator, NULL);
                return;
            case '*':
                continue;
            case TK_IDENTIFIER:
                if (copts.cplusplus == 0)
                    break;
                if (copts.cpp_extensions != 0 && cscope_current->theclass != NULL &&
                    cscope_current->theclass->classname == data_00587fa0 &&
                    CPrepTokenizer_GetNextTokenAndRestorePosition() == 372) {
                    tk = CPrepTokenizer_GetNextToken();
                    tk = CPrepTokenizer_GetNextToken();
                    break;
                }
            case TK_COLON_COLON:
                if (!CScope_ParseQualifiedScope(&lookup, 1))
                    break;
                nspace = lookup.nspace;
                foundNamespace = nspace;
                if (foundNamespace != NULL) {
                    if (foundNamespace->theclass == NULL)
                        break;
                    if (tk == '*')
                        continue;
                    break;
                }
                if (lookup.type != NULL && lookup.type->type == TYPETEMPLATE &&
                    declarator->templateParameters != NULL) {
                    if (CTemplateNew_LinkTemplateScope(declarator, (TypeTemplDep *)lookup.type, &nspace)) {
                        parse_direct_declarator(declarator, nspace);
                        return;
                    }
                    declarator->templateType = lookup.type;
                    return;
                }
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                break;
        }
        break;
    }
    if (finish != 0)
        parse_direct_declarator(declarator, nspace);
}

void CDecl_ParseDeclarator(DeclInfo *decl)
{
    NameResult info;
    NameSpace *nspace;

    switch (tk) {
        case TK_BITAND:
            if (!copts.cplusplus)
                break;
            /* fall through */
        case '*':
            CDecl_ScanPointer(decl, NULL, 1);
            if (tk == TK_UU_ATTRIBUTE)
                CParser_ParseAttribute(NULL, decl);
            return;

        case TK_IDENTIFIER:
            if (!copts.cplusplus)
                break;
            /* fall through */
        case TK_COLON_COLON:
            if (CScope_ParseQualifiedScope(&info, 1)) {
                nspace = info.nspace;
                if (info.nspace != NULL) {
                    if (info.nspace->theclass != NULL && tk == '*')
                        CDecl_ScanPointer(decl, info.nspace, 1);
                    else
                        parse_direct_declarator(decl, info.nspace);
                    return;
                }
                if (info.type != NULL && info.type->type == TYPETEMPLATE) {
                    if (decl->templateParameters != NULL &&
                        CTemplateNew_LinkTemplateScope(decl, (TypeTemplDep *)info.type, &nspace)) {
                        parse_direct_declarator(decl, nspace);
                        return;
                    }
                    if (decl->templateParameters != NULL && tk == TK_OPERATOR) {
                        decl->templateType = info.type;
                        return;
                    }
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_COLON_COLON && (tk = CPrepTokenizer_GetNextToken()) == 0x2a) {
                        Type *ownerType = info.type;
                        TypeMemberPointer *memberPointer = (TypeMemberPointer *)galloc(sizeof(TypeMemberPointer));
                        memberPointer->type = TYPEMEMBERPOINTER;
                        if (decl->thetype->type == TYPEFUNC) {
                            CDecl_MakePTMFuncType(TYPE_FUNC(decl->thetype));
                            memberPointer->size = 0xc;
                        } else {
                            memberPointer->size = 4;
                        }
                        memberPointer->ty1 = decl->thetype;
                        memberPointer->ty2 = ownerType;
                        memberPointer->qual = 0;
                        decl->thetype = (Type *)memberPointer;
                        tk = CPrepTokenizer_GetNextToken();
                        break;
                    }
                    if (decl->templateParameters != NULL) {
                        decl->templateType = info.type;
                        return;
                    }
                }
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
            break;
    }

    parse_direct_declarator(decl, NULL);
    if (tk == TK_UU_ATTRIBUTE)
        CParser_ParseAttribute(NULL, decl);
}

void conversion_type_name(DeclInfo *result)
{
    DeclInfo declaration;
    NameResult lookup;

    memclrw(&declaration, sizeof(DeclInfo));
    CParser_GetDeclSpecs((DeclInfo *)&declaration, 0);

    switch (tk) {
        case TK_BITAND:
        case '*':
            CDecl_ScanPointer(&declaration, NULL, 0);
            break;
        case TK_IDENTIFIER:
        case TK_COLON_COLON: {
            NameSpace *object;
            if (CScope_ParseQualifiedScope(&lookup, 0)) {
                object = lookup.nspace;
                if (object != NULL && object->theclass != NULL && tk == '*')
                    CDecl_ScanPointer(&declaration, object, 0);
                else
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
            break;
        }
    }

    result->name = declaration.name;
    result->thetype = declaration.thetype;
    result->qual |= declaration.qual;
}

Object *find_or_create_function_object(ObjectList *list, DeclInfo *ref, Boolean *found, UInt8 mode, Boolean conv)
{
    SInt32 m;
    Boolean flag;
    ObjectList *l;
    TypeMemberFunc *func;
    FuncArg *args;
    Object *obj;
    Object *result;
    TypeMemberFunc *newfunc;
    FuncArg *newargs;
    SInt16 t;
    if (found != NULL)
        *found = 0;
    newfunc = (TypeMemberFunc *)ref->thetype;
    newargs = newfunc->args;
    if (ref->has_expltargs != 0)
        return CTemplateFunc_FindSpecializationObject(ref, list);
    flag = 0;
    for (l = list; l != NULL; l = l->next) {
        m = mode;
        if (l->object->otype != OT_OBJECT)
            continue;
        obj = l->object;
        if (obj->type->type != TYPEFUNC)
            continue;
        func = (TypeMemberFunc *)obj->type;
        args = func->args;
        if (func->flags & 0x400)
            flag = 1;
        if (func->flags & Q_INLINE) {
            switch (m) {
                case 0:
                    CError_ReportError(ERR_ILLEGAL_FUNCTION_OVERLOADING);
                    break;
                case 1:
                    if (!func->is_static)
                        continue;
                    break;
                case 2:
                    if (func->is_static)
                        continue;
                    break;
                case 3:
                    if (!func->is_static) {
                        if (args->qual & Q_CV)
                            continue;
                        args = args->next;
                    }
                    break;
            }
        } else {
            if (mode != 0)
                CError_ReportError(ERR_ILLEGAL_FUNCTION_OVERLOADING);
        }
        if ((t = CParser_CompareArgLists(newargs, args)) == 1) {
            NameSpaceName *entry1;
            NameSpaceName *entry2;
            NameSpaceName *entry3;
            NameSpaceName *entry4;
            if (iscpp_typeequal(newfunc->functype, func->functype) == 0 ||
                (newfunc->qual & (Q_CONST | Q_PASCAL)) != (func->qual & (Q_CONST | Q_PASCAL)) ||
                (newfunc->flags & 0x17000001) != (func->flags & 0x17000001)) {
                CError_ReportError(ERR_ILLEGAL_FUNCTION_OVERLOADING);
                break;
            }
            if (newfunc->exspecs != NULL || func->exspecs != NULL) {
                entry1 = runtime_operator_namespace_name;
                if (obj->name != entry1->name) {
                    entry2 = data_00588008;
                    if (obj->name != entry2->name) {
                        entry3 = data_00587680;
                        if (obj->name != entry3->name) {
                            entry4 = data_00587e64;
                            if (obj->name != entry4->name)
                                CExcept_CompareSpecifications(newfunc->exspecs, func->exspecs);
                        }
                    }
                }
            }
            return obj;
        }
        if (t == 2) {
            CError_ReportError(ERR_ILLEGAL_FUNCTION_OVERLOADING);
            break;
        }
    }
    if (found == NULL) {
        CError_ReportError(ERR_ILLEGAL_FUNCTION_OVERLOADING);
        return NULL;
    }
    if (conv && flag) {
        result = CTemplateFunc_FindSpecializationObject(ref, list);
        if (result != NULL)
            return result;
    }
    if (ref->nspace != NULL)
        CError_ReportError(ERR_NAME_NOT_BEEN_DECLARED_NAMESPACE_CLASS);
    *found = 1;
    result = CParser_NewFunctionObject(ref);
    CheckDefaultArgs(((TypeMemberFunc *)result->type)->args);
    if (newfunc->flags & FUNC_PASCAL) {
        TypeMemberFunc *func2;
        for (l = list; l != NULL; l = l->next) {
            if (l->object->otype == OT_OBJECT) {
                if ((func2 = (TypeMemberFunc *)l->object->type)->type == TYPEFUNC && (func2->flags & FUNC_PASCAL))
                    CError_ReportError(ERR_PASCAL_FUNCTION_CANNOT_OVERLOADED);
            }
        }
    }
    if (copts.cplusplus != 0 && ref->requireMangledName != 0) {
        for (l = list; l != NULL; l = l->next) {
            if ((obj = l->object)->otype == OT_OBJECT && (obj->qual & Q_MANGLE_NAME) == 0)
                CError_ReportError(ERR_ILLEGAL_FUNCTION_OVERLOADING);
        }
    }
    CScope_AddObject(cscope_current, ref->name, (ObjBase *)result);
    if (cscope_current->theclass != NULL && (cscope_current->theclass->flags & CLASS_IS_TEMPL) != 0 &&
        CTemplateTools_IsDependentType(ref->thetype))
        CTemplateClass_AppendObjectDeclaration((TemplClass *)cscope_current->theclass, result);
    return result;
}

/* Object lookup result and the remaining lookup state. */
void MergeDefaultArgs(FuncArg *args, FuncArg *otherArgs)
{
    FuncArg *other;
    ENode *defaultExpr;
    FuncArg *mergeArgs;
    FuncArg *clearOtherAgain;
    FuncArg *clearArgs;
    FuncArg *clearOther;
    FuncArg *arg;
    FuncArg *clearArgsAgain;
    FuncArg *mergeOther;

    if (args == &data_00584748 || otherArgs == &data_00584748)
        return;

    arg = args;
    other = otherArgs;
    while (arg != NULL && other != NULL) {
        if (arg->dexpr != NULL) {
            while (other != NULL) {
                if (other->dexpr != NULL) {
                    for (clearArgs = args; clearArgs != NULL; clearArgs = clearArgs->next)
                        clearArgs->dexpr = NULL;
                    for (clearOther = otherArgs; clearOther != NULL; clearOther = clearOther->next)
                        clearOther->dexpr = NULL;
                    CError_ReportError(ERR_ILLEGAL_DEFAULT_ARGUMENTS);
                    return;
                }
                other = other->next;
            }
            break;
        }
        if (other->dexpr != NULL) {
            for (;;) {
                arg = arg->next;
                other = other->next;
                if (arg == NULL || arg == &data_00583098)
                    break;
                if (!(((defaultExpr = arg->dexpr) == NULL || other->dexpr == NULL) &&
                      (defaultExpr != NULL || other->dexpr != NULL))) {
                    for (clearArgsAgain = args; clearArgsAgain != NULL; clearArgsAgain = clearArgsAgain->next)
                        clearArgsAgain->dexpr = NULL;
                    for (clearOtherAgain = otherArgs; clearOtherAgain != NULL; clearOtherAgain = clearOtherAgain->next)
                        clearOtherAgain->dexpr = NULL;
                    CError_ReportError(ERR_ILLEGAL_DEFAULT_ARGUMENTS);
                    return;
                }
            }
            break;
        }
        arg = arg->next;
        other = other->next;
    }
    mergeArgs = args;
    mergeOther = otherArgs;
    while (mergeArgs != NULL && mergeOther != NULL) {
        if (mergeOther->dexpr != NULL)
            mergeArgs->dexpr = mergeOther->dexpr;
        else
            mergeOther->dexpr = mergeArgs->dexpr;
        mergeArgs = mergeArgs->next;
        mergeOther = mergeOther->next;
    }
}

void CheckDefaultArgs(FuncArg *args)
{
    FuncArg *arg = args;
    while (arg && !arg->dexpr)
        arg = arg->next;
    while (arg && arg != &data_00583098 && arg != &data_00584748) {
        if (!arg->dexpr) {
            arg = args;
            while (arg) {
                arg->dexpr = NULL;
                arg = arg->next;
            }
            CError_ReportError(205U);
            return;
        }
        arg = arg->next;
    }
}

Object *CDecl_GetFunctionObject(DeclInfo *decl, NameSpace *target_scope, Boolean *is_new_object, char lookup_mode)
{
    ObjectList *lookup;
    NameSpace *scope;
    Object *object;
    Boolean mismatch;
    FunctionTypeBuffer diagnostic_type;
    Boolean overload_created;
    Boolean object_created;
    SInt8 return_kind;
    TypeMemberFunc *member_type;

    mismatch = 0;
    if (is_new_object)
        *is_new_object = 0;
    if (!(scope = decl->nspace))
        scope = cscope_current;
    CError_ReportIllegalFlags(
        decl->qual & ~(Q_CV | Q_ASM | Q_PASCAL | Q_INLINE | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK | Q_INTERRUPT));
    return_kind = TYPE_FUNC(decl->thetype)->functype->type;
    switch (return_kind) {
        case TYPEFUNC:
        case TYPEARRAY:
            CError_ReportError(ERR_ILLEGAL_FUNCTION_RETURN_TYPE);
            TYPE_FUNC(decl->thetype)->functype = (Type *)&stsignedint;
            break;
    }
    if (scope->theclass) {
        if (!decl->name)
            CError_FATAL(1877);
        lookup = CScope_FindObjectListInNameSpace(scope, decl->name);
        if (!lookup) {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, decl->name->name);
            return NULL;
        }
        object = lookup->object;
        if (object->type->type != TYPEFUNC) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(object),
                               object->type, object->qual, decl->thetype, decl->qual);
            return NULL;
        }
        if (lookup->next && lookup->next->object->otype == OT_OBJECT) {
            if (decl->has_expltargs) {
                CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
                return NULL;
            }
            if ((SInt32)tk == TK_CONST || (SInt32)tk == TK_VOLATILE) {
                prepend_class_pointer_argument(TYPE_FUNC(decl->thetype), scope->theclass, 1);
                object = find_or_create_function_object(lookup, decl, NULL, 2, lookup_mode);
                if (!object)
                    return NULL;
            } else {
                object = find_or_create_function_object(lookup, decl, NULL, 3, lookup_mode);
                if (!object)
                    return NULL;
                member_type = (TypeMemberFunc *)object->type;
                if (!member_type->is_static)
                    prepend_class_pointer_argument(TYPE_FUNC(decl->thetype), scope->theclass, 1);
            }
        } else {
            if (decl->has_expltargs) {
                CError_ReportError(ERR_UNIMPLEMENTED_C_FEATURE);
                return NULL;
            }
            member_type = (TypeMemberFunc *)object->type;
            if (member_type->is_static) {
                if (scope->theclass->sominfo)
                    CSOM_PrependTheClassArg(TYPE_FUNC(decl->thetype));
            } else {
                prepend_class_pointer_argument(TYPE_FUNC(decl->thetype), scope->theclass, 1);
            }
            if (copts.cpp_extensions) {
                decl->qual |= object->qual & (Q_CONST | Q_PASCAL);
                TYPE_FUNC(decl->thetype)->qual |= TYPE_FUNC(object->type)->qual & (Q_CONST | Q_PASCAL);
                TYPE_FUNC(decl->thetype)->flags |= TYPE_FUNC(object->type)->flags & 0x4010000;
            }
            if (!iscpp_typeequal(decl->thetype, object->type) ||
                (decl->qual & (Q_CONST | Q_PASCAL)) != (object->qual & (Q_CONST | Q_PASCAL))) {
                member_type = (TypeMemberFunc *)object->type;
                diagnostic_type.member_function = *member_type;
                diagnostic_type.function = *TYPE_FUNC(decl->thetype);
                diagnostic_type.function.flags |= FUNC_METHOD;
                CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(object),
                                   object->type, object->qual, &diagnostic_type, decl->qual);
            }
        }
        if (decl->storageclass == TK_STATIC && object->sclass != TK_STATIC) {
            if (copts.cplusplus)
                CError_ReportError(ERR_INCONSISTENT_LINKAGE_EXTERN_OBJECT_REDECLARED_AS);
            else
                object->sclass = TK_STATIC;
        }
        object->qual |= decl->qual;
        MergeDefaultArgs(TYPE_FUNC(object->type)->args, TYPE_FUNC(decl->thetype)->args);
        if (!decl->oldStyleParameters)
            TYPE_FUNC(object->type)->args = TYPE_FUNC(decl->thetype)->args;
        if (decl->requireTemplateClassMember && object->nspace->theclass &&
            (object->nspace->theclass->flags & CLASS_IS_TEMPL_INST))
            decl->requireTemplateClassMember = 0;
    } else {
        if (decl->operator_token && !check_operator_declaration(decl, 0))
            return NULL;
        lookup = CScope_FindObjectListInNameSpace(scope, decl->name);
        if (lookup) {
            if (copts.cplusplus) {
                object = find_or_create_function_object(lookup, decl, &overload_created, 0, lookup_mode);
                if (!object)
                    return NULL;
                if (is_new_object)
                    *is_new_object = overload_created;
                if (target_scope)
                    object->nspace = target_scope;
            } else {
                object = lookup->object;
                if (!iscpp_typeequal(decl->thetype, object->type) ||
                    (decl->qual & (Q_CONST | Q_PASCAL)) != (object->qual & (Q_CONST | Q_PASCAL))) {
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(object),
                                       object->type, object->qual, decl->thetype, decl->qual);
                    mismatch = 1;
                    if (object->type->type != TYPEFUNC)
                        return NULL;
                }
            }
            if (!mismatch && is_new_object) {
                object_created = *is_new_object;
                if (decl->storageclass == TK_STATIC && object->sclass != TK_STATIC) {
                    if (copts.cplusplus)
                        CError_ReportError(ERR_INCONSISTENT_LINKAGE_EXTERN_OBJECT_REDECLARED_AS);
                    else
                        object->sclass = TK_STATIC;
                }
                object->qual |= decl->qual;
                if (object_created) {
                    CheckDefaultArgs(TYPE_FUNC(object->type)->args);
                } else
                    MergeDefaultArgs(TYPE_FUNC(object->type)->args, TYPE_FUNC(decl->thetype)->args);
                if (!decl->oldStyleParameters)
                    TYPE_FUNC(object->type)->args = TYPE_FUNC(decl->thetype)->args;
            }
        } else {
            if (decl->nspace)
                CError_ReportError(ERR_NAME_NOT_BEEN_DECLARED_NAMESPACE_CLASS);
            if (decl->has_expltargs) {
                if (decl->name)
                    CError_ReportError(ERR_UNDEFINED_IDENTIFIER, decl->name->name);
                else
                    CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
            }
            object = CParser_NewFunctionObject(decl);
            if (target_scope)
                object->nspace = target_scope;
            if (is_new_object)
                *is_new_object = 1;
            else
                CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);
            CheckDefaultArgs(TYPE_FUNC(object->type)->args);
            CScope_AddObject(scope, decl->name, (ObjBase *)object);
        }
    }
    return object;
}

void CDecl_TypedefDeclarator(DeclInfo *decl)
{
    ObjType *existingType;
    NameSpace *scope;
    ObjType *newType;
    NameSpaceObjectList *matches;
    Boolean warnDuplicate;

    if ((scope = decl->nspace) == NULL)
        scope = cscope_current;

    CError_ReportIllegalFlags(decl->qual & ~(Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK));

    if (decl->missingTypeSpecifier || decl->hasParameterNames)
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
    if (decl->operator_token)
        CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);

    existingType = NULL;
    matches = CScope_FindName(scope, decl->name);
    if (matches != NULL) {
        switch (matches->object->otype) {
            case OT_TYPE:
                existingType = (ObjType *)matches->object;
                break;
            case OT_TYPETAG:
                break;
            case OT_NAMESPACE:
                CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                return;
            case OT_ENUMCONST:
            case OT_OBJECT:
                CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
                return;
            default:
                CError_FATAL(2046);
        }
    }

    if (existingType != NULL) {
        if (iscpp_typeequal(existingType->type, decl->thetype) == 0 ||
            (existingType->qual & (Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK)) !=
                (decl->qual & (Q_CV | Q_PASCAL | Q_REFERENCE | Q_ALIGNED_MASK))) {
            CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, decl->name->name, existingType->type,
                               existingType->qual, decl->thetype, decl->qual);
        } else if (copts.cplusplus == 0) {
            if ((warnDuplicate = copts.extended_errorcheck) || copts.ANSIstrict) {
                if (warnDuplicate)
                    CError_Warning(ERR_IDENTIFIER_REDECLARED, decl->name->name);
                else
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, decl->name->name);
            }
        }
    } else {
        newType = (ObjType *)galloc(sizeof(ObjType));
        memclrw(newType, sizeof(ObjType));
        newType->otype = OT_TYPE;
        newType->access = ACCESSPUBLIC;
        newType->type = decl->thetype;
        newType->qual = decl->qual;
        CScope_AddObject(scope, decl->name, (ObjBase *)newType);
        if (scope->theclass != NULL && (scope->theclass->flags & CLASS_IS_TEMPL) != 0 &&
            CTemplateTools_IsDependentType(decl->thetype))
            CTemplateClass_AppendObjectDeclaration((TemplClass *)scope->theclass, (Object *)newType);
        if (copts.cplusplus != 0) {
            if (decl->thetype->type == TYPECLASS &&
                CParser_IsNullOrAtOrDollarPrefixedName(TYPE_CLASS(decl->thetype)->classname)) {
                TYPE_CLASS(decl->thetype)->classname = decl->name;
                TYPE_CLASS(decl->thetype)->nspace->name = decl->name;
            }
            if (decl->thetype->type == TYPEENUM &&
                CParser_IsNullOrAtOrDollarPrefixedName(TYPE_ENUM(decl->thetype)->enumname)) {
                TYPE_ENUM(decl->thetype)->enumname = decl->name;
            }
        }
        if (cprep_cu[0xe9] != 0 && decl->file->recordbrowseinfo != 0) {
            UInt32 sourcePosition = CPrep_GetCurrentTextOffset();
            CBrowse_WriteNameLineRange(scope, decl->name, decl->file, decl->file2, decl->sourceoffset, sourcePosition);
        }
    }
}

void declare_object(DeclInfo *d, UInt8 b, Boolean c)
{
    Object *found;
    NameSpace *nspace;
    Type *type;
    TemplClass *templateClass;
    NameSpaceObjectList *res;
    Boolean ok;
    ENode *p;
    Boolean r;
    CScopeSave save;
    if ((nspace = d->nspace) == NULL)
        nspace = cscope_current;
    CError_ReportIllegalFlags(d->qual & ~(Q_CV | Q_PASCAL | Q_IMPLICIT_WEAK | Q_WEAK | Q_ALIGNED_MASK));
    if (d->missingTypeSpecifier != 0 || d->hasParameterNames != 0)
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
    if (d->operator_token != 0)
        CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
    found = NULL;
    res = CScope_FindName(nspace, d->name);
    if (res != NULL) {
        switch (res->object->otype) {
            case OT_OBJECT:
                found = (Object *)res->object;
                if (c != 0)
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, d->name->name);
                break;
            case OT_NAMESPACE:
                CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                return;
            case OT_TYPETAG:
                break;
            case OT_ENUMCONST:
            case OT_TYPE:
                CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
                break;
            case OT_MEMBERVAR:
                CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
                break;
            default:
                CError_FATAL(2171);
                break;
        }
    }
    if (copts.cplusplus != 0) {
        if (c == 0)
            CDecl_CompleteType(d->thetype);
        switch ((SInt16)d->storageclass) {
            case 0x103:
                if (tk == '=' || tk == '(')
                    d->storageclass = TK_EOF;
                break;
            case 0:
                if (CParser_IsConst(d->thetype, d->qual) &&
                    ((found == NULL && nspace->theclass == NULL) ||
                     (found != NULL && found->sclass != TK_EXTERN && found->nspace->theclass == NULL)))
                    d->storageclass = TK_STATIC;
                break;
        }
    } else {
        if (d->storageclass == TK_EXTERN && tk == '=')
            d->storageclass = TK_EOF;
    }
    if (d->thetype->type == TYPEARRAY && d->thetype->size == 0 && d->storageclass == TK_EOF && tk != '=')
        d->storageclass = TK_EXTERN;
    if (found != NULL) {
        r = iscpp_typeequal(d->thetype, found->type);
        if (r == 0 || (found->qual & (Q_CV | Q_PASCAL)) != (d->qual & (Q_CV | Q_PASCAL)))
            CError_ReportError(ERR_IDENTIFIER_REDECLARED_WAS_DECLARED_AS_NOW, CError_GetObjectString(found),
                               found->type, found->qual, d->thetype, d->qual);
        if (found->qual & Q_INLINE_DATA) {
            if (tk == ',' || tk == ';')
                return;
            CError_ReportError(ERR_OBJECT_REDEFINED, found);
        }
        if (d->storageclass != TK_EXTERN) {
            if (found->sclass != TK_EXTERN && d->storageclass != TK_EOF)
                CError_ReportError(ERR_OBJECT_REDEFINED, found);
            if (r != 0) {
                found->sclass = d->storageclass;
                found->qual |= d->qual;
                if (d->thetype->size != 0)
                    found->type = d->thetype;
            }
            CParser_UpdateObject(found, d);
        } else {
            c = 1;
        }
    } else {
        if (d->nspace != NULL)
            CError_ReportError(ERR_NAME_NOT_BEEN_DECLARED_NAMESPACE_CLASS);
        if (d->thetype->type == TYPECLASS && TYPE_CLASS(d->thetype)->sominfo != NULL)
            CError_ReportError(ERR_GLOBAL_SOM_CLASS_OBJECTS_NOT_SUPPORTED);
        type = d->thetype;
        switch ((SInt8)type->type) {
            case TYPEVOID:
                CError_ReportError(ERR_ILLEGAL_USE_VOID);
                ok = 0;
                break;
            case TYPEFUNC:
                CError_ReportError(ERR_ILLEGAL_TYPE);
                ok = 0;
                break;
            case TYPECLASS:
                if (TYPE_CLASS(type)->flags & CLASS_ABSTRACT) {
                    CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                    ok = 0;
                    break;
                }
            default:
                ok = 1;
                break;
        }
        if (!ok) {
            ok = 0;
        } else if (type->type != TYPECLASS) {
            ok = 1;
        } else if (TYPE_CLASS(type)->flags & CLASS_HANDLEOBJECT) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            ok = 0;
        } else if (TYPE_CLASS(type)->objcinfo != NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
            ok = 0;
        } else {
            ok = 1;
        }
        if (!ok)
            d->thetype = (Type *)&stsignedint;
        found = CParser_NewObject(d);
        found->access = b;
        CScope_AddObject(nspace, d->name, (ObjBase *)found);
        if (nspace->theclass != NULL && (TYPE_CLASS(nspace->theclass)->flags & CLASS_IS_TEMPL) != 0 &&
            CTemplateTools_IsDependentType(d->thetype))
            CTemplateClass_AppendObjectDeclaration((TemplClass *)nspace->theclass, found);
        if (c != 0 && nspace->theclass != NULL && cprep_cu[0xe6] != 0)
            CBrowse_RecordDataObject(found, member_foi.tokenline + 1, CPrep_GetCurrentTextOffset());
    }
    if (c == 0) {
        if (!(d->nspace == NULL)) {
            CScope_SetNameSpaceScope(d->nspace, &save);
            CInit_InitializeData(found);
            CScope_RestoreScope(&save);
            if (d->requireTemplateClassMember != 0 && found->nspace->theclass != NULL &&
                (TYPE_CLASS(found->nspace->theclass)->flags & CLASS_IS_TEMPL_INST) != 0)
                d->requireTemplateClassMember = 0;
        } else {
            CInit_InitializeData(found);
        }
        if (d->file->recordbrowseinfo != 0 && found->sclass != TK_EXTERN)
            CBrowse_WriteObjectBrowseInfo(found, d->file, d->file2, d->sourceoffset, CPrep_GetCurrentTextOffset());
    } else if (tk == '=') {
        tk = CPrepTokenizer_GetNextToken();
        p = CExpr_IntegralConstOrDepExpr();
        if (found->type->type == TYPETEMPLATE || p->type != EINTCONST) {
            if (nspace->theclass == NULL || (TYPE_CLASS(nspace->theclass)->flags & CLASS_IS_TEMPL) == 0)
                CError_FATAL(2300);
            templateClass = (TemplClass *)nspace->theclass;
            CTemplateClass_AppendExpressionRecord(templateClass, found, p);
        } else if ((found->qual & Q_CONST) != 0 && (found->type->type == TYPEINT || found->type->type == TYPEENUM)) {
            found->u.data.u.intconst = p->data.intval;
            found->qual |= (Q_INLINE_DATA | Q_IMPLICIT_WEAK);
        } else {
            CError_ReportError(ERR_ILLEGAL_STATIC_CONST_MEMBER_INITIALIZATION, found->name->name);
        }
    }
}

Boolean CDecl_FunctionDeclarator(DeclInfo *decl, NameSpace *mode, Boolean allow_definition, int options)
{
    Boolean needsPrototype;
    Object *object;

    object = CDecl_GetFunctionObject(decl, mode, &needsPrototype, 1);
    if (object != NULL && (decl->hasParameterNames || tk == '{' || tk == TK_TRY || (decl->isConstructor && tk == ':') ||
                           (!copts.cplusplus && isdeclaration(0, 0, 0, 0)))) {
        if (!allow_definition || cscope_currentfunc)
            CError_ReportError(ERR_ILLEGAL_FUNCTION_DEFINITION);

        if (object->nspace == cscope_root && memcmp(object->name->name, "main", 5) == 0) {
            if (object->sclass == TK_STATIC ||
                (copts.ANSIstrict && TYPE_FUNC(object->type)->functype != (Type *)&stsignedint))
                CError_ReportError(ERR_MAIN_NOT_DEFINED_AS_EXTERNAL_INT);
        } else {
            if (copts.checkprotos && needsPrototype && object->sclass != TK_STATIC && (object->qual & Q_INLINE) == 0 &&
                !object->nspace->is_unnamed)
                CError_Warning(ERR_FUNCTION_NO_PROTOTYPE);
        }

        CFunc_ParseFuncDef(object, decl, NULL, 0, 0, NULL);
        if (decl->file->recordbrowseinfo) {
            int endOffset = CPrep_GetCurrentTextOffset();
            CBrowse_ForwardObjectFileRange(object, decl->file, decl->file2, decl->sourceoffset, endOffset);
        }
        if (copts.cplusplus) {
            if (CPrepTokenizer_GetNextTokenAndRestorePosition() == ';')
                tk = CPrepTokenizer_GetNextToken();
        }
        return 0;
    }
    return 1;
}

/* A declarator that names a special member function (constructor, destructor or conversion) of a class, out of the
   class: its type, scope and name come from the member; the parameter list is parsed in the class's scope. */
void parse_resolved_member_function_decl(DeclInfo *di, Boolean define)
{
    TypeClass *tclass;
    Object *obj;
    NameSpace *save;
    TypeFunc *tfunc;

    if ((obj = OBJECT(di->resolvedObject)) == NULL) {
        if (!di->resolvedObjects)
            CError_FATAL(2410);
        tclass = (TypeClass *)((NameSpaceObjectList *)di->resolvedObjects)->object;
        obj = (Object *)tclass;
        if (((Object *)tclass)->otype != OT_OBJECT)
            CError_FATAL(2412);
    }
    if (!(tclass = obj->nspace->theclass)) {
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        return;
    }
    if (obj->type->type == TYPEFUNC) {
        if (TYPE_FUNC(obj->type)->flags & 0x6000) {
            if (tclass->sominfo)
                di->thetype = (Type *)&stvoid;
            else
                di->thetype = (Type *)&void_ptr;
            di->nspace = obj->nspace;
            di->name = obj->name;
            if (TYPE_FUNC(obj->type)->flags & FUNC_IS_DTOR)
                di->isConstructor = 1;
            tk = CPrepTokenizer_GetNextToken();
            save = cscope_current;
            cscope_current = obj->nspace;
            CDecl_ParseDirectFuncDecl(di);
            cscope_current = save;
            if (di->thetype->type == TYPEFUNC) {
                if (TYPE_FUNC(obj->type)->flags & FUNC_IS_DTOR) {
                    if (((tclass = obj->nspace->theclass)->flags & CLASS_HAS_VBASES) && !tclass->sominfo)
                        CDecl_AddArgument(tfunc = TYPE_FUNC(di->thetype));
                } else {
                    if (!obj->nspace->theclass->sominfo)
                        CDecl_AddArgument(tfunc = TYPE_FUNC(di->thetype));
                }
                if (define)
                    CDecl_FunctionDeclarator(di, NULL, 1, 1);
            } else
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        } else if (TYPE_FUNC(obj->type)->flags & FUNC_CONVERSION) {
            di->thetype = TYPE_FUNC(obj->type)->functype;
            di->qual |= TYPE_FUNC(obj->type)->qual;
            di->nspace = obj->nspace;
            di->name = obj->name;
            tk = CPrepTokenizer_GetNextToken();
            CDecl_ParseDirectFuncDecl(di);
            if (di->thetype->type == TYPEFUNC) {
                if (define)
                    CDecl_FunctionDeclarator(di, NULL, 1, 1);
            } else
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            return;
        } else {
            tk = CPrepTokenizer_GetNextToken();
            di->thetype = (Type *)&stsignedint;
            di->nspace = obj->nspace;
            di->name = obj->name;
            save = cscope_current;
            cscope_current = obj->nspace;
            CDecl_ParseDirectFuncDecl(di);
            cscope_current = save;
            if (di->thetype->type == TYPEFUNC) {
                if (define)
                    CDecl_FunctionDeclarator(di, NULL, 1, 1);
                return;
            }
        }
    }
    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
}

void CDecl_ScanDeclarator(DeclInfo *p)
{
    Boolean first;
    SInt32 saved;
    CScopeSave save;

    if (((DeclInfo *)p)->resolvedObjects != NULL || ((DeclInfo *)p)->resolvedObject != NULL) {
        parse_resolved_member_function_decl(p, 1);
        return;
    }
    CScope_GetScope(&save);
    if (p->thetype == NULL)
        CError_FATAL(2559);
    first = 1;
    for (;;) {
        Type *node;

        node = p->thetype;
        saved = p->qual;
        p->nspace = NULL;
        p->operator_token = 0;
        if (node->type == TYPEFUNC) {
            p->thetype = (Type *)galloc(0x1a);
            *(TypeFunc *)p->thetype = *(TypeFunc *)node;
        }
        p->name = NULL;
        CDecl_ParseDeclarator(p);
        if (p->name == NULL) {
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            break;
        }
        if (p->storageclass != TK_TYPEDEF) {
            if (p->thetype->type == TYPEFUNC) {
                if (CDecl_FunctionDeclarator(p, NULL, first, 1) == 0)
                    return;
            } else {
                declare_object(p, 0, 0);
            }
        } else {
            CDecl_TypedefDeclarator(p);
        }
        CScope_RestoreScope(&save);
        p->thetype = node;
        p->qual = saved;
        if (tk != ',')
            break;
        tk = CPrepTokenizer_GetNextToken();
        first = 0;
    }
    if (tk != ';')
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
}

void *parse_enum_body(TypeEnum *enumType, HashNameNode *name)
{
    ObjEnumConst *enumerator;
    TypeIntegral *underlyingType;
    Type *expressionType;
    CInt64 currentValue, maximumValue, minimumValue, nextValue;
    ObjEnumConst *tail;
    CPrepFileInfo *sourceFile;
    SInt32 sourceOffset;
    CPrepFileInfo *browseFile;
    Boolean rangeChanged, overflow, isSigned;
    UInt8 access;
    HashNameNode *enumName;

    if (enumType == NULL) {
        enumType = (TypeEnum *)galloc(sizeof(TypeEnum));
        memclrw(enumType, sizeof(TypeEnum));
        enumType->type = TYPEENUM;
        enumType->nspace = cscope_current;
        if (name != NULL) {
            enumType->enumname = name;
            CScope_DefineTypeTag(cscope_current, name, (Type *)enumType);
        }
        if (cscope_current->is_global == 0) {
            do
                enumType->nspace = enumType->nspace->parent;
            while (enumType->nspace->is_global == 0);
            if ((enumName = enumType->enumname) != NULL)
                enumType->enumname = CParser_AppendUniqueNameFile(enumName->name);
        }
    }

    if (cscope_current->theclass != NULL && (TYPE_CLASS(cscope_current->theclass)->flags & Q_VIRTUAL) != 0)
        CTemplateClass_AppendEnumDeclaration((TemplClass *)cscope_current->theclass, enumType);

    if (cscope_current->theclass != NULL)
        access = member_access;
    else
        access = 0;

    tail = NULL;
    currentValue = cint64_zero;
    maximumValue = cint64_zero;
    minimumValue = cint64_zero;
    overflow = 0;
    if (copts.enumsalwaysint != 0) {
        underlyingType = &stsignedint;
        isSigned = 1;
    } else {
        underlyingType = &stunsignedchar;
        isSigned = 0;
    }

    tk = CPrepTokenizer_GetNextToken();
    if (copts.cplusplus == 0 || tk != '}') {
        do {
            if (tk != TK_IDENTIFIER) {
                if (tk == '}') {
                    if (copts.cpp_extensions != 0)
                        break;
                    if (copts.warn_extracomma == 0)
                        break;
                }
                CError_Warning(ERR_IDENTIFIER_EXPECTED);
                break;
            }
            enumerator = galloc(sizeof(ObjEnumConst));
            memclrw(enumerator, sizeof(ObjEnumConst));
            enumerator->otype = OT_ENUMCONST;
            enumerator->access = access;
            enumerator->type = (Type *)enumType;
            enumerator->name = data_00587fa0;
            CPrep_GetBrowseFilePosition(&sourceFile, &sourceOffset);
            rangeChanged = 0;
            tk = CPrepTokenizer_GetNextToken();
            if (tk == '=') {
                tk = CPrepTokenizer_GetNextToken();
                currentValue = CExpr_IntegralConstExprType(&expressionType);
                if (!IsNegative(currentValue.hi) || Type_IsUnsigned(expressionType) != 0) {
                    if (CInt64_GreaterU(currentValue, maximumValue)) {
                        maximumValue = currentValue;
                        rangeChanged = 1;
                    }
                } else {
                    if (CInt64_Less(currentValue, minimumValue)) {
                        minimumValue = currentValue;
                        rangeChanged = 1;
                    }
                    if (isSigned == 0) {
                        underlyingType = &stsignedchar;
                        isSigned = 1;
                    }
                }
                overflow = 0;
            } else {
                if (overflow != 0)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                if (isSigned == 0 || !IsNegative(currentValue.hi)) {
                    if (CInt64_GreaterU(currentValue, maximumValue)) {
                        maximumValue = currentValue;
                        rangeChanged = 1;
                    }
                } else {
                    if (CInt64_Less(currentValue, minimumValue)) {
                        minimumValue = currentValue;
                        rangeChanged = 1;
                    }
                }
            }

            if (copts.enumsalwaysint != 0) {
                if (copts.ANSIstrict != 0) {
                    if (CInt64_IsInRange(currentValue, stsignedint.size) == 0)
                        CError_ReportError(ERR_NUMBER_OUT_RANGE);
                } else {
                    if (CInt64_IsInRange(currentValue, stsignedint.size) == 0) {
                        if (CInt64_IsInURange(currentValue, stunsignedint.size) == 0)
                            CError_ReportError(ERR_NUMBER_OUT_RANGE);
                    }
                }
            } else if (isSigned != 0) {
                switch (underlyingType->size) {
                    case 1:
                        if (CInt64_IsInRange(maximumValue, 1)) {
                            if (CInt64_IsInRange(minimumValue, 1))
                                break;
                        }
                        underlyingType = FindSignedType(2);
                        /* fall through */
                    case 2:
                        if (CInt64_IsInRange(maximumValue, 2)) {
                            if (CInt64_IsInRange(minimumValue, 2))
                                break;
                        }
                        underlyingType = FindSignedType(4);
                        /* fall through */
                    case 4:
                        if (CInt64_IsInRange(maximumValue, 4)) {
                            if (CInt64_IsInRange(minimumValue, 4))
                                break;
                        }
                        underlyingType = FindSignedType(8);
                        if (underlyingType->size != 8) {
                            if (copts.ANSIstrict == 0) {
                                if (CInt64_IsInRange(minimumValue, 4)) {
                                    if (CInt64_IsInURange(maximumValue, 4))
                                        break;
                                }
                            }
                            if (rangeChanged != 0)
                                CError_ReportError(ERR_NUMBER_OUT_RANGE);
                            break;
                        }
                        /* fall through */
                    case 8:
                        if (CInt64_Equal(currentValue, maximumValue)) {
                            if (IsNegative(currentValue.hi))
                                CError_ReportError(ERR_NUMBER_OUT_RANGE);
                        }
                        break;
                    default:
                        CError_FATAL(2832);
                }
            } else {
                switch (underlyingType->size) {
                    case 1:
                        if (CInt64_IsInURange(maximumValue, 1))
                            break;
                        underlyingType = FindUnsignedType(2);
                        /* fall through */
                    case 2:
                        if (CInt64_IsInURange(maximumValue, 2))
                            break;
                        underlyingType = FindUnsignedType(4);
                        /* fall through */
                    case 4:
                        if (CInt64_IsInURange(maximumValue, 4))
                            break;
                        underlyingType = FindUnsignedType(8);
                        if (underlyingType->size != 8) {
                            if (rangeChanged != 0)
                                CError_ReportError(ERR_NUMBER_OUT_RANGE);
                        }
                        break;
                    case 8:
                        break;
                    default:
                        CError_FATAL(2860);
                }
            }

            enumType->size = underlyingType->size;
            enumType->enumtype = (Type *)underlyingType;
            enumerator->val = currentValue;
            CScope_AddObject(cscope_current, enumerator->name, (ObjBase *)enumerator);
            if (tail != NULL) {
                tail->next = enumerator;
                tail = enumerator;
            } else {
                enumType->enumlist = tail = enumerator;
            }
            if (((CPrepCU *)cprep_cu)->browseOptions.browseEnums != 0) {
                browseFile = CPrep_GetPFile();
                if (browseFile->recordbrowseinfo != 0)
                    CBrowse_WriteRelatedRecord(cscope_current, enumerator->name, browseFile, sourceFile, sourceOffset,
                                               CPrep_GetCurrentTextOffset());
            }
            nextValue = CInt64_Add(currentValue, cint64_one);
            if (isSigned != 0) {
                if (IsNegative(nextValue.hi) && !IsNegative(currentValue.hi))
                    overflow = 1;
            } else {
                if (IsZero(&nextValue))
                    overflow = 1;
            }
            currentValue = nextValue;
            if (tk != ',')
                break;
            tk = CPrepTokenizer_GetNextToken();
        } while (1);
    }

    enumType->size = underlyingType->size;
    enumType->enumtype = (Type *)underlyingType;
    for (enumerator = enumType->enumlist; enumerator != NULL; enumerator = enumerator->next)
        enumerator->type = (Type *)enumType;
    if (tk != '}')
        CError_ReportErrorAndUpdateToken(ERR_RBRACE_EXPECTED);
    else
        tk = CPrepTokenizer_GetNextToken();
    return enumType;
}

void CDecl_ComputeUnderlyingEnumType(TypeEnum *res)
{
    ObjEnumConst *n;
    ObjEnumConst *m;
    Type *t;

    if (copts.enumsalwaysint == 0) {
        for (n = res->enumlist; n != NULL; n = n->next) {
            if (HighIsNegative(n->val.hi) && !Type_IsUnsigned(n->type))
                break;
        }
        if (n != NULL) {
            CInt64 a, b;
            b = a = cint64_zero;
            for (m = res->enumlist; m != NULL; m = m->next) {
                if (HighIsNegative(m->val.hi) && !Type_IsUnsigned(m->type)) {
                    if (CInt64_Less(m->val, a))
                        a = m->val;
                } else {
                    if (CInt64_GreaterU(m->val, b))
                        b = m->val;
                }
            }
            if (HighIsNegative(b.hi))
                CError_ReportError(ERR_NUMBER_OUT_RANGE);
            if (CInt64_IsInRange(b, 1) && CInt64_IsInRange(a, 1)) {
                t = (Type *)SignedIntType(1);
            } else if (CInt64_IsInRange(b, 2) && CInt64_IsInRange(a, 2)) {
                t = (Type *)SignedIntType(2);
            } else if (CInt64_IsInRange(b, 4) && CInt64_IsInRange(a, 4)) {
                t = (Type *)SignedIntType(4);
            } else if (copts.ANSIstrict == 0 && CInt64_IsInRange(a, 4) && CInt64_IsInURange(b, 4)) {
                t = (Type *)SignedIntType(4);
            } else {
                t = (Type *)SignedIntType(8);
                if (t->size != 8)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
            }
        } else {
            CInt64 c;
            c = cint64_zero;
            for (m = res->enumlist; m != NULL; m = m->next) {
                if (CInt64_GreaterU(m->val, c))
                    c = m->val;
            }
            if (CInt64_IsInURange(c, 1)) {
                t = UnsignedIntType(1);
            } else if (CInt64_IsInURange(c, 2)) {
                t = UnsignedIntType(2);
            } else if (CInt64_IsInURange(c, 4)) {
                t = UnsignedIntType(4);
            } else {
                t = UnsignedIntType(8);
                if (t->size != 8)
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
            }
        }
    } else {
        t = (Type *)&stsignedint;
    }
    res->size = t->size;
    res->enumtype = t;
    for (n = res->enumlist; n != NULL; n = n->next)
        n->type = (Type *)res;
}

TypeEnum *parse_enum_definition(TypeEnum *decl, HashNameNode *name)
{
    CPrepFileInfo *context;
    UInt8 overflow;
    UInt8 dependent;
    AccessType access;
    TemplClass *templateClass;
    Type *expressionType;
    CInt64 value;
    CInt64 nextValue;
    CPrepFileInfo *sourceFile;
    SInt32 sourcePosition;
    ObjEnumConst *enumerator;
    ObjEnumConst *last;
    Type *currentType;

    if (decl == NULL) {
        decl = galloc(sizeof(TypeEnum));
        memclrw(decl, sizeof(TypeEnum));
        decl->type = TYPEENUM;
        decl->nspace = cscope_current;
        if (name != NULL) {
            decl->enumname = name;
            CScope_DefineTypeTag(cscope_current, name, (Type *)decl);
        }
        if (cscope_current->is_global == 0) {
            do
                decl->nspace = decl->nspace->parent;
            while (decl->nspace->is_global == 0);
            if (decl->enumname != NULL)
                decl->enumname = CParser_AppendUniqueNameFile(decl->enumname->name);
        }
    }

    if (cscope_current->theclass != NULL && (cscope_current->theclass->flags & CLASS_IS_TEMPL) != 0) {
        templateClass = (TemplClass *)cscope_current->theclass;
        CTemplateClass_AppendEnumDeclaration(templateClass, decl);
    } else
        templateClass = NULL;

    if (cscope_current->theclass != NULL)
        access = member_access;
    else
        access = 0;

    last = NULL;
    value = cint64_zero;
    overflow = 0;
    dependent = 0;
    if (CDecl_UseIntEnums())
        currentType = (Type *)&stsignedint;
    else
        currentType = (Type *)&stunsignedchar;
    expressionType = currentType;

    tk = CPrepTokenizer_GetNextToken();

    if (copts.cplusplus == 0 || tk != '}') {
        for (;;) {
            if (tk != TK_IDENTIFIER) {
                if (tk != '}' || (copts.c9x == 0 && copts.cpp_extensions == 0 && copts.warn_extracomma != 0))
                    CError_Warning(ERR_IDENTIFIER_EXPECTED);
                break;
            }
            enumerator = (ObjEnumConst *)galloc(sizeof(ObjEnumConst));
            memclrw(enumerator, sizeof(ObjEnumConst));
            enumerator->otype = OT_ENUMCONST;
            enumerator->access = access;
            enumerator->name = (HashNameNode *)data_00587fa0;
            CPrep_GetBrowseFilePosition(&sourceFile, &sourcePosition);
            tk = CPrepTokenizer_GetNextToken();
            if (tk == '=') {
                tk = CPrepTokenizer_GetNextToken();
                if (templateClass != NULL) {
                    ENode *expression;
                    expression = CExpr_IntegralConstOrDepExpr();
                    if (expression->type == EINTCONST) {
                        value = expression->data.intval;
                        enumerator->val = value;
                        enumerator->type = expression->rtype;
                        currentType = CDecl_LargerType(currentType, expressionType);
                        dependent = 0;
                    } else {
                        enumerator->type = (Type *)decl;
                        CTemplateClass_AppendEnumConstDeclaration(templateClass, enumerator, expression);
                        dependent = 1;
                    }
                } else {
                    value = CExpr_IntegralConstExprType(&expressionType);
                    enumerator->val = value;
                    enumerator->type = expressionType;
                    currentType = CDecl_LargerType(currentType, expressionType);
                    dependent = 0;
                }
                overflow = 0;
            } else {
                if (overflow && !CDecl_UseIntEnums())
                    CError_ReportError(ERR_NUMBER_OUT_RANGE);
                enumerator->val = value;
                enumerator->type = currentType;
                if (dependent)
                    CTemplateClass_AppendEnumConstDeclaration(templateClass, enumerator, NULL);
            }
            if (CDecl_UseIntEnums()) {
                if (copts.ANSIstrict != 0) {
                    if (!CInt64_IsInRange(value, stsignedint.size))
                        CError_ReportError(ERR_NUMBER_OUT_RANGE);
                } else {
                    if (!CInt64_IsInRange(value, stsignedint.size)) {
                        if (!CInt64_IsInURange(value, stunsignedint.size))
                            CError_ReportError(ERR_NUMBER_OUT_RANGE);
                    }
                }
            }
            decl->size = currentType->size;
            decl->enumtype = currentType;
            CScope_AddObject(cscope_current, enumerator->name, (ObjBase *)enumerator);
            if (last != NULL) {
                last->next = enumerator;
                last = enumerator;
            } else {
                decl->enumlist = last = enumerator;
            }
            {
                CPrepCU *compilationUnit = (CPrepCU *)cprep_cu;
                if (compilationUnit->browseOptions.browseEnums != 0) {
                    context = CPrep_GetPFile();
                    if (context->recordbrowseinfo != 0)
                        CBrowse_WriteRelatedRecord(cscope_current, enumerator->name, context, sourceFile,
                                                   sourcePosition, CPrep_GetCurrentTextOffset());
                }
            }
            if (!dependent) {
                nextValue = CInt64_Add(value, cint64_one);
                if (Type_IsUnsigned(currentType)) {
                    if (IsZero(&nextValue))
                        overflow = 1;
                    if (!CInt64_IsInURange(nextValue, currentType->size)) {
                        if (CInt64_IsInURange(nextValue, 2)) {
                            currentType = (Type *)FindUnsignedType(2);
                        } else if (CInt64_IsInURange(nextValue, 4)) {
                            currentType = (Type *)FindUnsignedType(4);
                        } else {
                            currentType = (Type *)FindUnsignedType(8);
                            if (currentType->size != 8)
                                overflow = 1;
                        }
                    }
                } else {
                    if (IsNegative(nextValue.hi) && !IsNegative(value.hi))
                        overflow = 1;
                    if (!CInt64_IsInRange(nextValue, currentType->size)) {
                        if (CInt64_IsInRange(nextValue, 2)) {
                            currentType = (Type *)FindSignedType(2);
                        } else if (CInt64_IsInRange(nextValue, 4)) {
                            currentType = (Type *)FindSignedType(4);
                        } else {
                            currentType = (Type *)FindSignedType(8);
                            if (currentType->size != 8)
                                overflow = 1;
                        }
                    }
                }
                value = nextValue;
            }
            if (tk != ',')
                break;
            tk = CPrepTokenizer_GetNextToken();
        }
    }

    CDecl_ComputeUnderlyingEnumType(decl);
    if (tk != '}')
        CError_ReportErrorAndUpdateToken(ERR_RBRACE_EXPECTED);
    else
        tk = CPrepTokenizer_GetNextToken();
    return decl;
}

void scanenum(DeclInfo *result)
{
    NameResult info;
    HashNameNode *saved;
    Type *type;
    TypeEnum *decl;

    if (tk == '{') {
        decl = parse_enum_definition(NULL, NULL);
        result->thetype = (Type *)decl;
        ((TypeEnum *)(result->thetype))->enumname = CParser_AppendUniqueNameFile("@enum");
        return;
    }
    if (tk == TK_IDENTIFIER) {
        saved = (HashNameNode *)data_00587fa0;
        if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x7b) {
            type = CScope_GetTagType(cscope_current, saved);
            if (type != NULL) {
                CPrepTokenizer_GetNextToken();
            checktype:
                if (type->size != 0 || type->type != TYPEENUM) {
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, (char *)saved + 0xa);
                    decl = parse_enum_definition(NULL, NULL);
                    result->thetype = (Type *)decl;
                    return;
                }
                decl = parse_enum_definition((TypeEnum *)type, NULL);
                result->thetype = (Type *)decl;
            } else {
                CPrepTokenizer_GetNextToken();
                decl = parse_enum_definition(NULL, saved);
                result->thetype = (Type *)decl;
            }
            if (cprep_cu[0xe7] != 0 && result->file->recordbrowseinfo != 0) {
                CBrowse_RecordNameRange(cscope_current, ((TypeEnum *)result->thetype)->enumname, result->file,
                                        result->file2, result->sourceoffset, CPrep_GetCurrentTextOffset());
            }
            return;
        } else {
            if (copts.cplusplus != 0 && tk == ';')
                CError_FATAL(3280);
            data_00587fa0 = saved;
        }
    }
    if (CScope_ParseElaborateName(&info)) {
        if ((type = info.type) != NULL) {
            if (type->type != TYPEENUM)
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '{') {
                result->thetype = type;
                return;
            }
            goto checktype;
        }
        if (info.name == NULL)
            CError_FATAL(3294);
        tk = CPrepTokenizer_GetNextToken();
        if (tk == '{') {
            decl = parse_enum_definition(NULL, info.name);
            result->thetype = (Type *)decl;
            return;
        }
        CError_ReportError(ERR_UNDEFINED_IDENTIFIER, info.name->name);
    } else {
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
    }
    result->thetype = (Type *)&stsignedint;
}

void CDecl_ScanStructDeclarator(BigDeclInfo *member)
{
    SInt16 size;
    UInt8 unnamed;
    ENode *node;
    TypeTemplDep *dependentType;
    TypeBitfield *bitFieldType;
    SInt16 bits;

    member->declinfo2 = member->declinfo;
    member->declinfo2.name = NULL;
    member->declinfo2.operator_token = 0;
    member->valid = 0;
    unnamed = 0;

    do {
        if (tk == ':') {
            member->declinfo2.name = unnamed_name;
            unnamed = 1;
        } else {
            member->declinfo2.isStructMemberDeclarator = 1;
            CDecl_ParseDeclarator(&member->declinfo2);
            if (member->declinfo2.name == NULL) {
                CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                return;
            }
            if ((copts.ANSIstrict == 0 || copts.c9x != 0) && member->declinfo2.thetype->size == 0 &&
                member->declinfo2.thetype->type == TYPEARRAY) {
                if (member->declinfo2.storageclass != 0x102) {
                    if (tk != ';' || CPrepTokenizer_GetNextTokenAndRestorePosition() != 0x7d) {
                        CError_ReportError(ERR_DATA_TYPE_INCOMPLETE);
                        return;
                    }
                }
            } else {
                SInt32 specifier = member->declinfo2.storageclass;
                if (specifier != 0x102 && specifier != 0x104 && member->declinfo2.thetype->type != TYPEFUNC &&
                    !CanAllocObject(member->declinfo2.thetype)) {
                    return;
                }
            }
            if (member->declinfo2.thetype->type == TYPECLASS &&
                ((TypeClass *)member->declinfo2.thetype)->sominfo != NULL) {
                CError_ReportError(ERR_SOM_CLASSES_CANNOT_CLASS_MEMBERS);
                return;
            }
            if (tk != ':') {
                break;
            }
        }

        do {
            if (member->declinfo2.thetype->type != TYPEINT && member->declinfo2.thetype->type != TYPEENUM) {
                if (CTemplateTools_IsDependentType(member->declinfo2.thetype)) {
                    break;
                }
                CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
                member->declinfo2.thetype = (Type *)&stunsignedint;
            } else if (copts.ANSIstrict != 0 && copts.cplusplus == 0 &&
                       member->declinfo2.thetype != (Type *)&stsignedint &&
                       member->declinfo2.thetype != (Type *)&stunsignedint) {
                CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
                member->declinfo2.thetype = (Type *)&stunsignedint;
            }
            switch (member->declinfo2.thetype->size) {
                case 1:
                    size = 8;
                    break;
                case 2:
                    size = 16;
                    break;
                case 4:
                    size = 32;
                    break;
                default:
                    CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
                    return;
            }
        } while (0);
        tk = CPrepTokenizer_GetNextToken();
        node = CExpr_IntegralConstOrDepExpr();
        if (node->type != EINTCONST) {
            dependentType = galloc(sizeof(*dependentType));
            memclrw(dependentType, sizeof(*dependentType));
            dependentType->type = TYPETEMPLATE;
            dependentType->size = 1;
            dependentType->dtype = 5;
            dependentType->u.bitfield.type = member->declinfo2.thetype;
            dependentType->u.bitfield.size = fn_00513040(node, 1);
            member->declinfo2.thetype = (Type *)dependentType;
            member->valid = 1;
            return;
        }
        bits = node->data.intval.lo;
        if (unnamed != 0) {
            if (bits < 0 || bits > size) {
                CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
                return;
            }
        } else {
            if (bits <= 0 || bits > size) {
                CError_ReportError(ERR_ILLEGAL_BITFIELD_DECLARATION);
                return;
            }
        }
        bitFieldType = galloc(sizeof(*bitFieldType));
        memclrw(bitFieldType, sizeof(*bitFieldType));
        bitFieldType->type = 7;
        bitFieldType->size = member->declinfo2.thetype->size;
        bitFieldType->bitfieldtype = member->declinfo2.thetype;
        bitFieldType->bitlength = (UInt8)bits;
        bitFieldType->suppressAlignment = unnamed;
        member->declinfo2.thetype = (Type *)bitFieldType;
        if (tk == TK_UU_ATTRIBUTE) {
            CParser_ParseAttribute(NULL, &member->declinfo2);
        }
    } while (0);
    member->valid = 1;
    return;
}

void compute_struct_layout(Type *str)
{
    SInt32 maxsize;
    SInt32 base;
    SInt32 size;
    Boolean has_anon;
    Boolean first;
    TypeBitfield *bitfield;
    StructMember *member;
    StructMember *inner;
    StructMember *newmember;

    maxsize = 0;
    has_anon = 0;
    CMach_StructLayoutInitOffset(0);
    for (member = TYPE_STRUCT(str)->members; member != NULL; member = member->next) {
        if (TYPE_STRUCT(str)->stype == 1)
            CMach_StructLayoutInitOffset(0);
        if (member->type->type == TYPEBITFIELD)
            member->offset = CMach_StructLayoutBitfield(TYPE_BITFIELD(member->type), member->qual);
        else
            member->offset = CMach_StructLayoutGetOffset(member->type, member->qual);
        if (TYPE_STRUCT(str)->stype == 1) {
            size = CMach_StructLayoutGetCurSize();
            if (size > maxsize)
                maxsize = size;
        }
        if (member->name == unnamed_name)
            has_anon = 1;
        if (member->name == NULL) {
            if (member->type->type != TYPESTRUCT)
                CError_FATAL(3483);
            base = member->offset;
            inner = TYPE_STRUCT(member->type)->members;
            first = 1;
            for (; inner != NULL; inner = inner->next) {
                if (ismember(str, inner->name) != NULL)
                    CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED, inner->name->name);
                if (first) {
                    member->type = inner->type;
                    member->name = inner->name;
                    member->qual = inner->qual;
                    member->offset = base + inner->offset;
                } else {
                    newmember = galloc(sizeof(*newmember));
                    memclrw(newmember, sizeof(*newmember));
                    newmember->next = member->next;
                    newmember->type = inner->type;
                    newmember->name = inner->name;
                    newmember->qual = inner->qual | Q_WEAK;
                    newmember->offset = base + inner->offset;
                    member->next = newmember;
                    member = newmember;
                }
                if (copts.reverse_bitfields && member->type->type == TYPEBITFIELD) {
                    bitfield = galloc(sizeof(*bitfield));
                    *bitfield = *TYPE_BITFIELD(member->type);
                    CABI_ReverseBitField(bitfield);
                    member->type = (Type *)bitfield;
                }
                first = 0;
            }
            has_anon = 1;
        }
    }
    if (has_anon) {
        StructMember **link;
        StructMember *next;

        link = &TYPE_STRUCT(str)->members;
        while ((next = *link) != NULL) {
            if (next->name == unnamed_name || next->name == NULL)
                *link = next->next;
            else
                link = &next->next;
        }
    }
    if (TYPE_STRUCT(str)->stype != 1)
        maxsize = CMach_StructLayoutGetCurSize();
    str->size = maxsize;
    TYPE_STRUCT(str)->align = fn_004a8400(TYPE_STRUCT(str));
    str->size = maxsize + CABI_ComputeAlignmentPadding(str, maxsize);
    if (copts.reverse_bitfields) {
        for (member = TYPE_STRUCT(str)->members; member != NULL; member = member->next) {
            if (member->type->type == TYPEBITFIELD)
                CABI_ReverseBitField(TYPE_BITFIELD(member->type));
        }
    }
    if (copts.warn_padding && TYPE_STRUCT(str)->stype != 1) {
        StructMember *previous;
        previous = NULL;
        for (member = TYPE_STRUCT(str)->members; member != NULL; member = member->next) {
            if (previous != NULL && previous->offset + previous->type->size < member->offset)
                CError_Warning(ERR_PAD_BYTES_INSERTED_AFTER_DATA_MEMBER,
                               member->offset - (previous->offset + previous->type->size), previous->name->name);
            previous = member;
        }
        if (previous != NULL && previous->offset + previous->type->size < str->size)
            CError_Warning(ERR_PAD_BYTES_INSERTED_AFTER_DATA_MEMBER,
                           str->size - (previous->offset + previous->type->size), previous->name->name);
    }
}

int parse_struct_members(TypeStruct *obj, Boolean block)
{
    BigDeclInfo memberDecl;
    StructMember *member;
    Type *type;
    SInt32 result;
    SInt32 textOffset;
    Boolean validType;

    result = -1;
    memclrw(&memberDecl, sizeof(memberDecl));
    if (tk == TK_AT_DEFS) {
        CPrep_GetFOI(&member_foi, NULL);
        CObjC_005082b0(obj);
        tk = CPrepTokenizer_GetNextToken();
        if (tk != '}') {
            CError_ReportError(ERR_RBRACE_EXPECTED);
        }
    } else {
        do {
            CPrep_GetFOI(&member_foi, NULL);
            memclrw(&memberDecl.declinfo, sizeof(memberDecl.declinfo));
            CParser_GetDeclSpecs(&memberDecl.declinfo, 0);
            if (memberDecl.declinfo.storageclass != 0 || memberDecl.declinfo.hasParameterNames != 0) {
                CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                obj->members = NULL;
                return -1;
            }
            if (tk != ';') {
                for (;;) {
                    CDecl_ScanStructDeclarator(&memberDecl);
                    type = memberDecl.declinfo2.thetype;
                    switch ((SInt8)type->type) {
                        case TYPEVOID:
                            CError_ReportError(ERR_ILLEGAL_USE_VOID);
                            validType = 0;
                            break;
                        case TYPEFUNC:
                            CError_ReportError(ERR_ILLEGAL_TYPE);
                            validType = 0;
                            break;
                        case TYPECLASS:
                            if (TYPE_CLASS(type)->flags & CLASS_ABSTRACT) {
                                CError_IllegalUseAbstractClass(TYPE_CLASS(type));
                                validType = 0;
                                break;
                            }
                            /* fall through */
                        default:
                            validType = 1;
                            break;
                    }
                    if (!validType) {
                        validType = 0;
                    } else {
                        if (type->type == TYPECLASS) {
                            if (TYPE_CLASS(type)->flags & CLASS_HANDLEOBJECT) {
                                CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
                                validType = 0;
                            } else if (TYPE_CLASS(type)->objcinfo != NULL) {
                                CError_ReportError(ERR_ILLEGAL_USE_OBJECTIVE_C_OBJECT);
                                validType = 0;
                            } else {
                                validType = 1;
                            }
                        } else {
                            validType = 1;
                        }
                    }
                    if (!validType) {
                        CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                        memberDecl.valid = 0;
                    }
                    if (memberDecl.declinfo2.operator_token != 0) {
                        CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                        memberDecl.valid = 0;
                    }
                    if (memberDecl.valid) {
                        if (memberDecl.declinfo2.name == unnamed_name ||
                            ismember((Type *)obj, memberDecl.declinfo2.name) == NULL) {
                            member = galloc(sizeof(*member));
                            memclrw(member, sizeof(*member));
                            member->type = memberDecl.declinfo2.thetype;
                            member->name = memberDecl.declinfo2.name;
                            member->qual = memberDecl.declinfo2.qual;
                            appendmember(obj, member);
                            if (block) {
                                textOffset = CPrep_GetCurrentTextOffset();
                                CBrowse_WriteStructMember(member, member_foi.tokenline + 1, textOffset);
                            }
                        } else {
                            CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED,
                                               memberDecl.declinfo2.name->name);
                        }
                    }
                    if (tk != ',')
                        break;
                    tk = CPrepTokenizer_GetNextToken();
                }
            } else if (copts.ANSIstrict == 0 && memberDecl.declinfo.thetype->type == TYPESTRUCT) {
                member = galloc(sizeof(*member));
                memclrw(member, sizeof(*member));
                member->type = memberDecl.declinfo.thetype;
                appendmember(obj, member);
            } else {
                CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
            }
            if (tk != ';') {
                obj->members = NULL;
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
                return -1;
            }
            CPrep_ResetBufferedTokenPosition();
            tk = CPrepTokenizer_GetNextToken();
        } while (tk != '}');
        compute_struct_layout((Type *)obj);
    }
    if (block) {
        result = CPrep_GetCurrentTextOffset();
        if (tk == ';')
            result++;
    }
    tk = CPrepTokenizer_GetNextToken();
    return result;
}

void scanstruct(DeclInfo *state, SInt16 spec)
{
    Boolean saved;
    DeclInfo *context = state;
    TypeStruct temporary;
    GList save;
    Type *node;
    HashNameNode *name;
    SInt32 result;

    if (copts.cplusplus != 0) {
        CDecl_ParseClass(context, spec, 1, 0);
        return;
    }
    if (tk == TK_IDENTIFIER) {
        name = data_00587fa0;
        node = CScope_FindTagType(cscope_current, name);
        if (node != NULL) {
            if (node->type == TYPECLASS) {
                CDecl_ParseClass(context, spec, 1, 0);
                return;
            }
            tk = CPrepTokenizer_GetNextToken();
            if (CScope_GetTagType(cscope_current, name) == NULL && (tk == ';' || tk == '{')) {
                MAKE_NODE(node, spec);
                if (name != NULL)
                    attach_node(node, name);
            }
            if (node->type != TYPESTRUCT || TYPE_STRUCT(node)->stype != spec) {
                CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, name->name);
                context->thetype = node;
                return;
            }
            if (tk != '{') {
                context->thetype = node;
                return;
            }
            if (node->size != 0) {
                CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, name->name);
                MAKE_NODE(node, spec);
            }
        } else {
            MAKE_NODE(node, spec);
            if (name != NULL)
                attach_node(node, name);
            tk = CPrepTokenizer_GetNextToken();
            if (tk != '{') {
                context->thetype = node;
                return;
            }
        }
    } else {
        if (tk != '{') {
            CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
            context->thetype = (Type *)&stsignedint;
            return;
        }
        MAKE_NODE(node, spec);
    }
    saved = (cprep_cu[0xe6] != 0 && context->file->recordbrowseinfo != 0);
    if (saved)
        CBrowse_BuildTypeStructBrowseInfo(context, TYPE_STRUCT(node), &save);
    temporary = *TYPE_STRUCT(node);
    tk = CPrepTokenizer_GetNextToken();
    result = parse_struct_members(&temporary, saved);
    *TYPE_STRUCT(node) = temporary;
    context->thetype = node;
    if (saved)
        CBrowse_FlushAndRestoreMemberList(result, &save);
}

void scan_inline_definition(Object *object, TypeClass *classType)
{
    TokenStream declaration;
    int position;
    CPrepFileInfo *parseResult;
    short token;

    object->qual |= Q_INLINE;
    ((TypeFunc *)object->type)->flags |= FUNC_DEFINED;
    CPrep_SaveFunctionBodyTokens(&declaration, NULL, 1);
    if (declaration.tokens != 0) {
        if (((((TypeFunc *)object->type)->flags & FUNC_METHOD) != 0) &&
            ((((TypeMemberFunc *)object->type)->theclass->flags & CLASS_IS_TEMPL) != 0)) {
            ((TypeFunc *)object->type)->flags |= 0x8000000;
            CTemplateClass_AddTemplateArgumentOverride((TemplClass *)((TypeMemberFunc *)object->type)->theclass, object,
                                                       &member_foi, &declaration);
        } else {
            CInline_AddFunctionPrecNode(object, classType, &member_foi, &declaration, '\0');
        }
    }
    parseResult = CPrep_GetPFile();
    if (parseResult->recordbrowseinfo != '\0') {
        position = CPrep_GetCurrentTextOffset();
        CBrowse_ForwardObjectFileRange(object, parseResult, member_foi.file, member_foi.tokenline + 1, position);
    }
    token = CPrepTokenizer_GetNextTokenAndRestorePosition();
    if (token == 0x3b) {
        tk = CPrepTokenizer_GetNextToken();
    } else {
        tk = ';';
    }
}

TypeMemberFunc *CDecl_NewTypeMemberFunc(TypeFunc *type, TypeClass *theclass, Boolean is_static, Boolean arg)
{
    TypeMemberFunc *member;
    member = galloc(40U);
    memclrw(member, 40U);
    *(TypeFunc *)member = *type;
    member->theclass = theclass;
    member->is_static = (unsigned char)is_static;
    member->flags |= FUNC_METHOD;
    if ((unsigned char)is_static == 0U)
        prepend_class_pointer_argument(TYPE_FUNC(member), theclass, arg);
    return member;
}

void declare_member_function(ClassLayout *layout, TypeClass *cls, struct DeclInfo *info, UInt8 access, UInt8 allowPure,
                             UInt8 specialMember, UInt8 parseBody, UInt8 declarationOnly)
{
    Boolean isNew;
    TypeMemberFunc *memberType;
    NameSpaceObjectList *found;
    Object *member;
    Boolean wasVirtual;
    TypeMemberFunc *func;
    UInt8 returnKind;

    if ((returnKind = (func = (TypeMemberFunc *)info->thetype)->functype->type) == TYPEARRAY ||
        returnKind == TYPEFUNC) {
        CError_ReportError(ERR_ILLEGAL_FUNCTION_RETURN_TYPE);
        func = (TypeMemberFunc *)info->thetype;
        func->functype = (Type *)&stsignedint;
    }
    if (cls->sominfo != NULL) {
        func = (TypeMemberFunc *)info->thetype;
        CSOM_EncodeMemberFunctionTypes(func);
    }

    wasVirtual = 0;
    if (info->qual & Q_VIRTUAL) {
        info->qual &= ~Q_VIRTUAL;
        wasVirtual = 1;
        allowPure = 1;
    }

    found = CScope_FindName(cls->nspace, info->name);
    if (found != NULL) {
        UInt8 objectType;
        Object *existing;
        if ((objectType = (existing = (Object *)found->object)->otype) != OT_TYPETAG) {
            if (objectType != OT_OBJECT || existing->type->type != TYPEFUNC)
                CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED, info->name->name);
        } else {
            found = NULL;
        }
    }

    if (!((memberType = (TypeMemberFunc *)info->thetype)->flags & FUNC_METHOD)) {
        TypeMemberFunc *newType = (TypeMemberFunc *)galloc(sizeof(TypeMemberFunc));
        memclrw(newType, sizeof(TypeMemberFunc));
        {
            TypeFunc *dest = (TypeFunc *)newType;
            TypeFunc *source = (TypeFunc *)memberType;
            *dest = *source;
        }
        newType->theclass = cls;
        newType->is_static = declarationOnly;
        newType->flags |= FUNC_METHOD;
        if (declarationOnly == 0)
            prepend_class_pointer_argument(TYPE_FUNC(newType), cls, parseBody);
        memberType = newType;
        info->thetype = (Type *)newType;
    } else {
        CError_ASSERT(3992, cls->sominfo == 0);
    }

    {
        UInt8 classFlags = cls->eflags;
        if (classFlags & CLASS_EFLAGS_INTERNAL)
            info->exportflags |= 0x10;
        if (classFlags & CLASS_EFLAGS_IMPORT)
            info->exportflags |= 0x20;
        if (classFlags & CLASS_EFLAGS_EXPORT)
            info->exportflags |= 0x40;
    }

    CError_ASSERT(4010, cscope_current == cls->nspace);

    if (found != NULL) {
        UInt8 kind;
        if (declarationOnly != 0)
            kind = 1;
        else
            kind = 2;
        member = find_or_create_function_object((ObjectList *)found, info, &isNew, kind, 1);
        if (member == NULL)
            return;
        if (isNew != 0) {
            memberType->vtbl_index = ++layout->lex_order_count;
        } else {
            CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED, CError_GetObjectString(member));
        }
    } else {
        memberType->vtbl_index = ++layout->lex_order_count;
        member = CParser_NewFunctionObject(info);
        if (cls->flags & CLASS_IS_TEMPL) {
            if (CTemplateTools_IsDependentType(info->thetype))
                CTemplateClass_AppendObjectDeclaration((TemplClass *)cls, member);
        }
        CScope_AddObject(cls->nspace, info->name, (ObjBase *)member);
    }

    member->access = access;
    func = (TypeMemberFunc *)member->type;
    CheckDefaultArgs(func->args);
    if (specialMember != 0) {
        memberType->flags |= FUNC_CONVERSION;
        cls->flags |= CLASS_IS_CONVERTIBLE;
    }
    if (wasVirtual) {
        if (is_pascal_object(member))
            CError_ReportError(ERR_VIRTUAL_FUNCTIONS_CANNOT_PASCAL_FUNCTIONS);
        if (cls->mode == 1)
            CError_ReportError(ERR_ILLEGAL_VIRTUAL_FUNCTION_UNION, member);
        member->datatype = DVFUNC;
        layout->has_vtable = 1;
    }
    if (allowPure != 0 || wasVirtual) {
        if (parseBody != 0 && tk == '=') {
            tk = CPrepTokenizer_GetNextToken();
            if (tk == TK_INTCONST) {
                Boolean isZero = (token_integer.hi == 0 && intconst_lo == 0);
                if (!isZero)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                memberType->flags |= FUNC_PURE;
                cls->flags |= CLASS_ABSTRACT;
                tk = CPrepTokenizer_GetNextToken();
            } else {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            }
        }
    }
    if (parseBody != 0) {
        if (tk == '{' || tk == TK_TRY || (tk == ':' && CClass_IsDestructor(member)))
            scan_inline_definition(member, NULL);
    }
    {
        CPrepCU *compileUnit = (CPrepCU *)cprep_cu;
        if (compileUnit->browseOptions.browseOption != 0) {
            SInt32 position = CPrep_GetCurrentTextOffset();
            CBrowse_RecordFunction(member, member_foi.tokenline + 1, position);
        }
    }
}

Boolean check_qualified_identifier_or_operator(TypeClass *tclass, AccessType access)
{
    SInt32 save;
    Boolean found;

    CPrep_GetBufferedTokenPosition(&save);
    found = 0;

    for (;;) {
        switch (tk) {
            case TK_IDENTIFIER:
                tk = CPrepTokenizer_GetNextToken();
                if (tk != ';')
                    break;
                /* fall through */
            case TK_OPERATOR:
                CPrep_SetPosition(&save);
                if (found) {
                    CScope_ParseUsingDeclaration(tclass->nspace, access, 1);
                    return 1;
                }
                return 0;
            default:
                CPrep_SetPosition(&save);
                return 0;
        }
        switch (tk) {
            case TK_COLON_COLON:
                found = 1;
                tk = CPrepTokenizer_GetNextToken();
                break;
            case '<':
                tk = CPrepTokenizer_GetNextToken();
                for (;;) {
                    switch (tk) {
                        case 0:
                        case ';':
                        case '{':
                        case '}':
                            CPrep_SetPosition(&save);
                            return 0;
                        case '>':
                            tk = CPrepTokenizer_GetNextToken();
                            if (tk == TK_COLON_COLON) {
                                found = 1;
                                tk = CPrepTokenizer_GetNextToken();
                                break;
                            }
                            /* fall through */
                        default:
                            tk = CPrepTokenizer_GetNextToken();
                            continue;
                    }
                    break;
                }
                break;
            default:
                CPrep_SetPosition(&save);
                return 0;
        }
    }
}

/* Compact projection of the declaration record's selected values. */
/* Layout of the input record; reserved regions are not read here. */

unsigned char CDecl_PackDeclInfo(PackedDeclInfo *destination, DeclInfo *source)
{
    unsigned char hasTemplateArguments;
    destination->thetype = source->thetype;
    destination->qual = source->qual;
    destination->nspace = source->nspace;
    destination->name = source->name;
    destination->expltargs = CTemplTool_MakeGlobalTemplArgCopy(source->expltargs);
    destination->storageclass = source->storageclass;
    destination->section = source->section;
    destination->exportflags = source->exportflags;
    hasTemplateArguments = source->has_expltargs;
    destination->has_expltargs = hasTemplateArguments;
    return hasTemplateArguments;
}

/* Declaration record; intervening storage is cleared but not populated here. */
void CDecl_UnpackDeclInfo(DeclInfo *dst, PackedDeclInfo *src)
{
    memclrw(dst, sizeof(*dst));
    dst->thetype = src->thetype;
    dst->qual = src->qual;
    dst->nspace = src->nspace;
    dst->name = src->name;
    dst->expltargs = src->expltargs;
    dst->storageclass = src->storageclass;
    dst->section = src->section;
    dst->exportflags = src->exportflags;
    dst->has_expltargs = src->has_expltargs;
}

void CDecl_AddFriend(TypeClass *typeClass, Object *object, TypeClass *type)
{
    ClassFriend *entry;

    if (object != NULL) {
        for (entry = (ClassFriend *)typeClass->friends; entry != NULL; entry = entry->next) {
            if (entry->isclass == 0 && entry->u.obj == object)
                break;
        }
        if (entry == NULL) {
            entry = galloc(sizeof(ClassFriend));
            memclrw(entry, sizeof(ClassFriend));
            entry->next = (ClassFriend *)typeClass->friends;
            typeClass->friends = (ClassFriend *)entry;
            entry->u.obj = object;
            entry->isclass = 0;
        }
    }
    if (type != NULL) {
        for (entry = (ClassFriend *)typeClass->friends; entry != NULL; entry = entry->next) {
            if (entry->isclass != 0 && entry->u.theclass == type)
                break;
        }
        if (entry == NULL) {
            entry = galloc(sizeof(ClassFriend));
            memclrw(entry, sizeof(ClassFriend));
            entry->next = (ClassFriend *)typeClass->friends;
            typeClass->friends = (ClassFriend *)entry;
            entry->u.theclass = type;
            entry->isclass = 1;
        }
    }
}

void parse_friend_declaration(TemplClass *cls)
{
    DeclInfo decl;
    Boolean isNewFunction;
    CScopeSave scopeSave;
    Boolean isTemplateClass;
    NameSpace *globalNamespace;
    SInt32 isStructOrClass;
    SInt32 declarationToken;
    Boolean isClassDeclaration;
    Type *baseType;
    UInt32 baseQualifiers;
    Object *function;

    isTemplateClass = (cls->theclass.flags & CLASS_IS_TEMPL) != 0;
    declarationToken = tk;
    isClassDeclaration =
        (isStructOrClass = declarationToken == 0x112 || declarationToken == 0x10f) || declarationToken == 0x110;

    memclrw(&decl, sizeof(decl));
    decl.allowForeignNamespace = 1;
    CParser_GetDeclSpecs(&decl, 1);
    if (decl.storageclass != 0) {
        CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
        decl.storageclass = 0;
    }
    decl.allowForeignNamespace = 0;

    if (tk == ';') {
        if (!isClassDeclaration)
            CError_ReportError(ERR_ILLEGAL_FRIEND_DECLARATION);
        if (decl.thetype->type == TYPECLASS) {
            if ((((TypeClass *)decl.thetype)->flags & CLASS_IS_TEMPL) == 0 ||
                CParser_CheckTemplateClassScope(decl.thetype) != 0) {
                if (!isTemplateClass)
                    CDecl_AddFriend(&cls->theclass, NULL, TYPE_CLASS(decl.thetype));
                else
                    CTemplateClass_AddDeferredFunctionDeclaration(cls, &decl);
            }
        } else if (decl.thetype->type == TYPETEMPLATE && isTemplateClass) {
            CTemplateClass_AddDeferredFunctionDeclaration(cls, &decl);
        } else {
            CError_ReportError(ERR_ILLEGAL_FRIEND_DECLARATION);
        }
    } else if (decl.resolvedObjects != NULL || decl.resolvedObject != NULL) {
        parse_resolved_member_function_decl(&decl, 0);
        if (decl.name == NULL)
            return;
        function = CDecl_GetFunctionObject(&decl, NULL, &isNewFunction, 0);
        if (function != NULL)
            CDecl_AddFriend(&cls->theclass, function, NULL);
        if (tk != ';')
            CError_ReportError(ERR_SEMICOLON_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
        return;
    } else {
        globalNamespace = CScope_FindGlobalNS(cscope_current);
        baseType = decl.thetype;
        baseQualifiers = decl.qual;
        for (;;) {
            memclrw(&decl, sizeof(decl));
            decl.thetype = baseType;
            decl.qual = baseQualifiers;
            decl.in_friend_decl = 1;
            decl.allowTemplateArguments = 1;
            CDecl_ParseDeclarator(&decl);
            if (decl.thetype->type == TYPEFUNC) {
                if (!isTemplateClass) {
                    CScope_SetNameSpaceScope(globalNamespace, &scopeSave);
                    function = CDecl_GetFunctionObject(&decl, NULL, &isNewFunction, 0);
                    CScope_RestoreScope(&scopeSave);
                    if (function != NULL) {
                        CDecl_AddFriend(&cls->theclass, function, NULL);
                        if (decl.nspace == NULL && tk == '{')
                            scan_inline_definition(function, &cls->theclass);
                        else if (function->sclass == TK_EOF)
                            function->sclass = TK_EXTERN;
                    }
                } else {
                    CTemplateClass_AddDeferredFunctionDeclaration(cls, &decl);
                }
            } else {
                CError_ReportError(ERR_ILLEGAL_FRIEND_DECLARATION);
            }
            if (tk != ',')
                break;
            tk = CPrepTokenizer_GetNextToken();
        }
    }

    if (tk == ';')
        tk = CPrepTokenizer_GetNextToken();
    else
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
}

ObjMemberVar *add_member_var(ClassLayout *declaration, TypeClass *cls, Type *type, UInt32 qual, HashNameNode *name,
                             AccessType access)
{
    NameSpaceObjectList *objects;
    ObjMemberVar *lastMember;
    ObjMemberVar *member;
    SInt32 position;

    if (name != NULL && (objects = CScope_FindName(cls->nspace, name)) != NULL) {
        switch (objects->object->otype) {
            case OT_NAMESPACE:
                CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                return NULL;
            case OT_ENUMCONST:
            case OT_TYPE:
            case OT_OBJECT:
                CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
                return NULL;
            case OT_MEMBERVAR:
                CError_ReportError(ERR_IDENTIFIER_REDECLARED, name->name);
                return NULL;
            case OT_TYPETAG:
                break;
            default:
                CError_FATAL(4406);
        }
    }
    member = (ObjMemberVar *)galloc(sizeof(ObjMemberVar));
    memclrw(member, sizeof(ObjMemberVar));
    member->otype = OT_MEMBERVAR;
    member->access = access;
    member->name = name;
    member->type = type;
    member->qual = qual;
    if (cls->sominfo == NULL) {
        declaration->lex_order_count += 1;
    }
    if (cls->ivars != NULL) {
        for (lastMember = cls->ivars; lastMember->next != NULL; lastMember = lastMember->next)
            ;
        lastMember->next = member;
    } else {
        cls->ivars = member;
    }
    if (name != NULL && name != unnamed_name) {
        CScope_AddObject(cls->nspace, name, (ObjBase *)member);
        if ((cls->flags & CLASS_IS_TEMPL) != 0 && CTemplateTools_IsDependentType(type)) {
            CTemplateClass_AppendObjectDeclaration((TemplClass *)cls, (Object *)member);
        }
        {
            CPrepCU *compilationUnit = (CPrepCU *)cprep_cu;
            if (compilationUnit->browseOptions.browseOption != 0) {
                position = CPrep_GetCurrentTextOffset();
                CBrowse_WriteObjMemberVar(member, member_foi.tokenline + 1, position);
            }
        }
    }
    return member;
}

void parse_class_members(ClassLayout *decle, TypeClass *tclass, SInt16 mode)
{
    BigDeclInfo md;
    DeclInfo ds;
    SInt16 t;
    UInt32 qual;
    UInt32 dsqual;
    SInt16 dsx3c;
    UInt8 dsx3e;
    UInt8 access;
    Boolean member;
    Boolean ctor;
    Boolean special;
    NameSpace *scope;
    Type *type;
    TypeFunc *tfunc;
    TypeBitfield *copy, *source;
    ObjMemberVar *unionMember;
    ObjMemberVar *memberVar;
    UInt8 eflags;

    member = (tclass->flags & CLASS_IS_TEMPL) && ((TemplClass *)tclass)->pspec_owner;
    memclrw(&md, sizeof(BigDeclInfo));
    access = (mode == 2) ? ACCESSPRIVATE : ACCESSPUBLIC;
    member_access = access;

    while (tk != '}') {
        CPrep_GetFOI(&member_foi, NULL);
        dsqual = 0;
        qual = 0;
        dsx3e = 0;
        dsx3c = 0;
        if (tk == TK_TEMPLATE) {
            scope = cscope_current;
            if (!scope->theclass || !(scope->theclass->flags & CLASS_IS_TEMPL)) {
                for (; scope; scope = scope->parent) {
                    if (!scope->name && !scope->theclass && scope->parent && !scope->is_templ) {
                        CError_ReportError(ERR_LOCAL_CLASSES_SHALL_NOT_MEMBER_TEMPLATES);
                        break;
                    }
                }
            }
            CTemplateNew_ParseTemplateDeclaration(tclass);
            tk = CPrepTokenizer_GetNextToken();
            continue;
        }

    restart:
        ctor = 0;
        switch (tk) {
            case TK_UU_DECLSPEC:
                if ((tk = CPrepTokenizer_GetNextToken()) != '(')
                    CError_ReportError(ERR_LPAREN_EXPECTED);
                memclrw(&ds, sizeof(DeclInfo));
                CParser_ParseDeclSpec(&ds, 1);
                dsqual |= ds.qual;
                dsx3e |= ds.exportflags;
                dsx3c = ds.section;
                if ((tk = CPrepTokenizer_GetNextToken()) != ')')
                    CError_ReportError(ERR_RPAREN_EXPECTED);
                tk = CPrepTokenizer_GetNextToken();
                goto restart;
                do {
                    case 0x149:
                        member_access = access = ACCESSPRIVATE;
                        break;
                    case 0x14a:
                        member_access = access = ACCESSPROTECTED;
                        break;
                    case 0x14b:
                        member_access = access = ACCESSPUBLIC;
                } while (0);
                if (qual || dsx3e)
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                if ((tk = CPrepTokenizer_GetNextToken()) != ':') {
                    CError_ReportError(ERR_COLON_EXPECTED);
                    continue;
                }
                tk = CPrepTokenizer_GetNextToken();
                continue;
            case TK_EXPLICIT:
                CError_ReportIllegalFlags(qual & Q_EXPLICIT);
                qual |= Q_EXPLICIT;
                tk = CPrepTokenizer_GetNextToken();
                goto restart;
            case TK_INLINE:
                CError_ReportIllegalFlags(qual & Q_INLINE);
                qual |= Q_INLINE;
                tk = CPrepTokenizer_GetNextToken();
                goto restart;
            case TK_ASM:
                CError_ReportIllegalFlags(qual & Q_ASM);
                qual |= Q_ASM;
                tk = CPrepTokenizer_GetNextToken();
                goto restart;
            case TK_VIRTUAL:
                CError_ReportIllegalFlags(qual & Q_VIRTUAL);
                qual |= Q_VIRTUAL;
                tk = CPrepTokenizer_GetNextToken();
                goto restart;
            case TK_IDENTIFIER:
            identifier:
                if (data_00587fa0 == tclass->classname) {
                    t = CPrepTokenizer_GetNextTokenAndRestorePosition();
                    data_00587fa0 = tclass->classname;
                    ctor = 1;
                    if (copts.cpp_extensions && t == 0x174) {
                        CPrepTokenizer_GetNextToken();
                        if ((tk = CPrepTokenizer_GetNextToken()) == -3)
                            goto identifier;
                        if (tk == '~')
                            goto restart;
                        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                    }
                    if (t == '(') {
                    constructor:
                        CError_ReportIllegalFlags(qual & ~(Q_ASM | Q_INLINE | Q_EXPLICIT));
                        memclrw(&md.declinfo2, sizeof(DeclInfo));
                        if (tclass->sominfo)
                            md.declinfo2.thetype = &stvoid;
                        else
                            md.declinfo2.thetype = (Type *)&void_ptr;
                        md.declinfo2.qual = qual;
                        md.declinfo2.exportflags = dsx3e;
                        md.declinfo2.section = dsx3c;
                        md.declinfo2.isConstructor = 1;
                        CDecl_ParseDeclarator(&md.declinfo2);
                        if (md.declinfo2.thetype->type == TYPEFUNC) {
                            if (member)
                                md.declinfo2.thetype = CTemplTool_ResolveMemberSelfRefs(
                                    (TemplClass *)tclass, md.declinfo2.thetype, &md.declinfo2.qual);
                            if (tclass->sominfo) {
                                if (((TypeFunc *)md.declinfo2.thetype)->args)
                                    CError_ReportError(ERR_NO_PARAMETERS_ALLOWED_SOM_CLASS_CONSTRUCTORS);
                                md.declinfo2.qual |= Q_VIRTUAL;
                            } else {
                                if (((TypeFunc *)md.declinfo2.thetype)->args &&
                                    !((TypeFunc *)md.declinfo2.thetype)->args->next &&
                                    ((TypeFunc *)md.declinfo2.thetype)->args->type == (Type *)tclass) {
                                    CError_ReportError(ERR_ILLEGAL_COPY_CONSTRUCTOR);
                                    ((TypeFunc *)md.declinfo2.thetype)->args = NULL;
                                }
                                if (tclass->flags & CLASS_HAS_VBASES)
                                    AddShortArg((TypeFunc *)md.declinfo2.thetype);
                                md.declinfo2.qual &= ~Q_VIRTUAL;
                            }
                            ((TypeFunc *)md.declinfo2.thetype)->flags |= FUNC_IS_DTOR;
                            md.declinfo2.name = constructor_name;
                            md.declinfo2.qual |= dsqual;
                            declare_member_function(decle, tclass, &md.declinfo2, access, 0, 0, 1, 0);
                        } else {
                            CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                        }
                        if (tk == ';') {
                            tk = CPrepTokenizer_GetNextToken();
                            continue;
                        }
                        CError_ReportError(ERR_SEMICOLON_EXPECTED);
                        continue;
                    }
                }
                if (!qual && check_qualified_identifier_or_operator(tclass, access)) {
                    tk = CPrepTokenizer_GetNextToken();
                    continue;
                }
                break;
            case TK_USING:
                CError_ReportIllegalFlags(qual);
                tk = CPrepTokenizer_GetNextToken();
                CScope_ParseUsingDeclaration(tclass->nspace, access, 0);
                tk = CPrepTokenizer_GetNextToken();
                continue;
            case '~':
                if ((tk = CPrepTokenizer_GetNextToken()) != -3 || data_00587fa0 != tclass->classname) {
                    CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                    continue;
                }
                CError_ReportIllegalFlags(qual & ~(Q_ASM | Q_INLINE | Q_VIRTUAL));
                if (tclass->flags & (CLASS_IS_TEMPL | CLASS_IS_TEMPL_INST)) {
                    t = CPrepTokenizer_GetNextTokenAndRestorePosition();
                    data_00587fa0 = tclass->classname;
                    if (t == '<') {
                        memclrw(&md.declinfo, sizeof(DeclInfo));
                        CParser_GetDeclSpecs((DeclInfo *)&md.declinfo, 0);
                        if (tk != '(' || md.declinfo.thetype != (Type *)tclass || md.declinfo.nspace) {
                            CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                            continue;
                        }
                        CPrep_UngetToken();
                        tk = TK_IDENTIFIER;
                        data_00587fa0 = tclass->classname;
                    }
                }
                memclrw(&md.declinfo2, sizeof(DeclInfo));
                md.declinfo2.qual = qual;
                md.declinfo2.exportflags = dsx3e;
                md.declinfo2.section = dsx3c;
                if (tclass->sominfo)
                    md.declinfo2.thetype = &stvoid;
                else
                    md.declinfo2.thetype = (Type *)&void_ptr;
                CDecl_ParseDeclarator(&md.declinfo2);
                if (md.declinfo2.thetype->type == TYPEFUNC && !((TypeFunc *)md.declinfo2.thetype)->args) {
                    if (!CScope_FindName(tclass->nspace, destructor_name)) {
                        if (tclass->sominfo) {
                            md.declinfo2.qual |= Q_VIRTUAL;
                        } else {
                            AddShortArg((TypeFunc *)md.declinfo2.thetype);
                        }
                        md.declinfo2.name = destructor_name;
                        ((TypeFunc *)md.declinfo2.thetype)->flags |= 0x4000;
                        md.declinfo2.qual |= dsqual;
                        declare_member_function(decle, tclass, &md.declinfo2, access, 1, 0, 1, 0);
                    } else {
                        CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED,
                                           CError_BuildNameSpaceNameTypeString(tclass->nspace, destructor_name, NULL));
                    }
                } else {
                    CError_ReportError(ERR_ILLEGAL_TYPE);
                }
                if (tk == ';') {
                    tk = CPrepTokenizer_GetNextToken();
                    continue;
                }
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
                continue;
            case TK_OPERATOR:
                if (CMangler_OperatorName(CPrepTokenizer_GetNextTokenAndRestorePosition())) {
                    memclrw(&md.declinfo, sizeof(DeclInfo));
                    md.declinfo.thetype = (Type *)&stsignedint;
                    goto declspecs_done;
                }
                tk = CPrepTokenizer_GetNextToken();
                CError_ReportIllegalFlags(qual & ~(Q_ASM | Q_INLINE | Q_VIRTUAL));
                memclrw(&md.declinfo2, sizeof(DeclInfo));
                md.declinfo2.exportflags = dsx3e;
                md.declinfo2.section = dsx3c;
                md.declinfo2.qual = qual;
                conversion_type_name(&md.declinfo2);
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
                md.declinfo2.qual |= dsqual;
                type = md.declinfo2.thetype;
                tfunc = galloc(sizeof(TypeFunc));
                memclrw(tfunc, sizeof(TypeFunc));
                md.declinfo2.name = CMangler_ConversionFuncName(md.declinfo2.thetype, md.declinfo2.qual);
                tfunc->type = TYPEFUNC;
                SETFUNC(tfunc, md.declinfo2.thetype, md.declinfo2.qual & Q_CV);
                md.declinfo2.isType = 0;
                md.declinfo2.thetype = (Type *)tfunc;
                md.declinfo2.storageclass = 0;
                if ((tclass->flags & CLASS_IS_TEMPL) && CTemplateTools_IsDependentType(type))
                    md.declinfo2.name = CParser_GetUniqueName();
                declare_member_function(decle, tclass, &md.declinfo2, access, 1, 1, 1, 0);
                if (tk == ';') {
                    tk = CPrepTokenizer_GetNextToken();
                    continue;
                }
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
                continue;
            case TK_FRIEND:
                tk = CPrepTokenizer_GetNextToken();
                parse_friend_declaration((TemplClass *)tclass);
                continue;
        }

        CError_ReportIllegalFlags(qual & Q_EXPLICIT);
        member_access = access;
        memclrw(&md.declinfo, sizeof(DeclInfo));
        md.declinfo.exportflags = dsx3e;
        md.declinfo.section = dsx3c;
        md.declinfo.qual = qual;
        CParser_GetDeclSpecs((DeclInfo *)&md.declinfo, 0);
        if (ctor && tk == '(' && (tclass->flags & (CLASS_IS_TEMPL | CLASS_IS_TEMPL_INST)) &&
            md.declinfo.thetype == (Type *)tclass && !md.declinfo.nspace) {
            CPrep_UngetToken();
            tk = TK_IDENTIFIER;
            data_00587fa0 = tclass->classname;
            qual = md.declinfo.qual;
            goto constructor;
        }

    declspecs_done:
        switch (md.declinfo.storageclass) {
            case 0:
            case 0x102:
            case 0x104:
            case 0x12b:
                break;
            default:
                CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                md.declinfo.storageclass = 0;
        }

        if (tk != ';') {
            for (;;) {
                CDecl_ScanStructDeclarator(&md);
                if (member)
                    md.declinfo2.thetype = CTemplTool_ResolveMemberSelfRefs((TemplClass *)tclass, md.declinfo2.thetype,
                                                                            &md.declinfo2.qual);
                if (md.declinfo2.nspace)
                    CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
                if (md.declinfo2.operator_token) {
                    if (md.declinfo.storageclass == 0x12b)
                        CError_ReportIllegalFlags(0x80);
                    special = 0;
                    switch (md.declinfo2.operator_token) {
                        case 0x147:
                        case 0x182:
                            CError_ReportIllegalFlags(md.declinfo2.qual & Q_VIRTUAL);
                            special = 1;
                            break;
                        case 0x145:
                        case 0x183:
                            CError_ReportIllegalFlags(md.declinfo2.qual & Q_VIRTUAL);
                            if (CClass_MemberObject(tclass, md.declinfo2.name))
                                CError_ReportError(ERR_STRUCT_UNION_CLASS_MEMBER_REDEFINED, md.declinfo2.name->name);
                            special = 1;
                            break;
                        default:
                            if (md.declinfo2.storageclass == 0x102)
                                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                            if (tclass->sominfo)
                                CError_ReportError(ERR_ILLEGAL_OPERATOR_DECLARATION);
                    }
                    md.declinfo2.storageclass = 0;
                    if (md.declinfo2.thetype->type == TYPEFUNC) {
                        md.declinfo2.qual |= dsqual;
                        declare_member_function(decle, tclass, &md.declinfo2, access, !special, 0, 1, special);
                        check_operator_declaration(&md.declinfo2, 1);
                        if (tclass->sominfo)
                            CSOM_PrependTheClassArg(TYPE_FUNC(md.declinfo2.thetype));
                    } else {
                        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    }
                } else if (md.valid) {
                    if (md.declinfo2.name == constructor_name || md.declinfo2.name == destructor_name)
                        CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                    switch (md.declinfo2.storageclass) {
                        case 0x104:
                            CError_ReportIllegalFlags(md.declinfo2.qual & Q_VIRTUAL);
                            CDecl_TypedefDeclarator(&md.declinfo2);
                            break;
                        case 0x102:
                            CError_ReportIllegalFlags(md.declinfo2.qual & Q_VIRTUAL);
                            if (tclass->sominfo)
                                CError_ReportError(ERR_NO_STATIC_MEMBERS_ALLOWED_SOM_CLASSES);
                            if (md.declinfo2.thetype->type == TYPEFUNC) {
                                md.declinfo2.storageclass = 0;
                                md.declinfo2.qual |= dsqual;
                                if (md.declinfo2.name == tclass->classname)
                                    CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                                declare_member_function(decle, tclass, &md.declinfo2, access, 0, 0, 1, 1);
                            } else {
                                eflags = tclass->eflags;
                                if (eflags & CLASS_EFLAGS_INTERNAL)
                                    md.declinfo2.exportflags |= 0x10;
                                if (eflags & CLASS_EFLAGS_IMPORT)
                                    md.declinfo2.exportflags |= 0x20;
                                if (eflags & CLASS_EFLAGS_EXPORT)
                                    md.declinfo2.exportflags |= 0x40;
                                md.declinfo2.storageclass = 0;
                                declare_object(&md.declinfo2, access, 1);
                            }
                            break;
                        case 0:
                        case 0x12b:
                            if (md.declinfo2.thetype->type == TYPEFUNC) {
                                if (md.declinfo2.name == tclass->classname)
                                    CError_ReportError(ERR_ILLEGAL_CONSTRUCTOR_DESTRUCTOR_DECLARATION);
                                if (md.declinfo.storageclass == 0x12b)
                                    CError_ReportIllegalFlags(0x80);
                                md.declinfo2.qual |= dsqual;
                                declare_member_function(decle, tclass, &md.declinfo2, access, 1, 0, 1, 0);
                            } else {
                                CDecl_CompleteType(md.declinfo2.thetype);
                                (void)CheckMemberType(md.declinfo2.thetype);
                                CError_ReportIllegalFlags(md.declinfo2.qual & (Q_INLINE | Q_VIRTUAL));
                                if (md.declinfo2.storageclass == 0x12b)
                                    md.declinfo2.qual |= Q_MUTABLE;
                                add_member_var(decle, tclass, md.declinfo2.thetype, md.declinfo2.qual,
                                               md.declinfo2.name, access);
                            }
                            break;
                        default:
                            CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
                    }
                }
                if (tk != ',')
                    break;
                tk = CPrepTokenizer_GetNextToken();
            }
        } else {
            if (CParser_IsAnonymousClass(&md.declinfo.thetype, 1)) {
                if ((memberVar = add_member_var(decle, tclass, md.declinfo.thetype, 0, NULL, access)))
                    memberVar->anonunion = 1;
                for (unionMember = (ObjMemberVar *)((TypeClass *)md.declinfo.thetype)->ivars; unionMember;
                     unionMember = unionMember->next) {
                    if ((type = unionMember->type)->type == TYPEBITFIELD && copts.reverse_bitfields) {
                        copy = galloc(sizeof(TypeBitfield));
                        source = TYPE_BITFIELD(type);
                        *copy = *source;
                        CABI_ReverseBitField(copy);
                        type = (Type *)copy;
                    }
                    if ((memberVar = add_member_var(decle, tclass, type, unionMember->qual, unionMember->name, access)))
                        memberVar->offset = unionMember->offset | 0x80000000;
                }
            }
        }

        if (tk != ';') {
            CError_ReportError(ERR_SEMICOLON_EXPECTED);
            return;
        }
        CPrep_ResetBufferedTokenPosition();
        tk = CPrepTokenizer_GetNextToken();
    }
}

VClassList *append_unique_vbase(TypeClass *cls, TypeClass *base)
{
    VClassList *node;
    node = cls->vbases;
    while (node != NULL) {
        if (node->base == base)
            return NULL;
        node = node->next;
    }
    node = (VClassList *)galloc(sizeof(VClassList));
    memclrw(node, sizeof(VClassList));
    node->base = base;
    if (cls->vbases != NULL) {
        VClassList *p = cls->vbases;
        while (p->next != NULL)
            p = p->next;
        p->next = node;
    } else {
        cls->vbases = node;
    }
    return node;
}

void CDecl_SetVBaseOffsets(TypeClass *cls)
{
    ClassList *base;
    VClassList *virtualBase;
    VClassList *layout;
    SInt32 offset;

    if (copts.vbase_ctor_offset)
        cls->flags |= CLASS_SOM_INIT;

    base = cls->bases;
    offset = cls->size;
    for (; base != NULL; base = base->next) {
        for (virtualBase = base->base->vbases; virtualBase != NULL; virtualBase = virtualBase->next) {
            layout = append_unique_vbase(cls, virtualBase->base);
            if (layout != NULL) {
                layout->offset = CMach_MemberAlignValue(TYPE(virtualBase->base), offset) + offset;
                offset = layout->offset + virtualBase->base->size;
            }
        }
        if (base->is_virtual) {
            layout = append_unique_vbase(cls, base->base);
            if (layout != NULL) {
                layout->offset = CMach_MemberAlignValue(TYPE(base->base), offset) + offset;
                offset = layout->offset + base->base->size;
            }
        }
    }
}

Boolean CDecl_CheckNewBase(TypeClass *cls, TypeClass *base, Boolean flag)
{
    ClassList *cl;

    if (cls == base) {
        CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
        return 0;
    }
    if ((base->flags & CLASS_COMPLETED) == 0) {
        CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, base, 0);
        return 0;
    }
    if (base->flags & CLASS_SINGLE_OBJECT) {
        if (flag || cls->bases != NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            return 0;
        }
        cls->flags |= CLASS_SINGLE_OBJECT;
    }
    if (base->flags & CLASS_HANDLEOBJECT) {
        if (flag || cls->bases != NULL) {
            CError_ReportError(ERR_ILLEGAL_USE_HANDLEOBJECT);
            return 0;
        }
        cls->flags |= CLASS_HANDLEOBJECT;
    }
    if (base->sominfo != NULL) {
        if (!flag)
            CError_ReportError(ERR_SOM_CLASSES_INHERTIANCE_MUST_VIRTUAL);
        CSOM_InitSOMInfo(cls);
    } else {
        if (cls->sominfo != NULL)
            CError_ReportError(ERR_SOM_CLASSES_ONLY_INHERIT_FROM_OTHER);
    }
    if (cls->bases != NULL && (cls->flags & CLASS_SINGLE_OBJECT) && (cls->flags & CLASS_SINGLE_OBJECT)) {
        CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
        return 0;
    }
    if (copts.ecplusplus && (flag || cls->bases != NULL)) {
        CError_ReportError(ERR_ILLEGAL_USE_C_FEATURE_EC);
        return 0;
    }
    cl = cls->bases;
    while (cl != NULL) {
        if (cl->base == base) {
            CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
            return 0;
        }
        cl = cl->next;
    }
    if (base->flags & 0x2000)
        cls->flags |= 0x2000;
    if (base->flags & CLASS_IS_CONVERTIBLE)
        cls->flags |= CLASS_IS_CONVERTIBLE;
    if (base->flags & CLASS_HAS_VBASES)
        cls->flags |= CLASS_HAS_VBASES;
    if (flag)
        cls->flags |= CLASS_HAS_VBASES;

    return 1;
}

void parse_class_bases(TemplClass *classType, short mode, char allowDependent)
{
    char access;
    TypeClass *baseType;
    int defaultMode;
    ClassList *tail;
    ObjType *typeObject;
    char isVirtual;
    ClassList *base;
    NameResult lookup;
    defaultMode = mode;
    do {
        if (defaultMode == 2) {
            access = 1;
        } else {
            access = 0;
        }
        isVirtual = 0;
        tk = CPrepTokenizer_GetNextToken();
        if (tk == TK_VIRTUAL) {
            tk = CPrepTokenizer_GetNextToken();
            isVirtual = 1;
        }
        switch (tk) {
            case TK_PRIVATE:
                access = 1;
                tk = CPrepTokenizer_GetNextToken();
                break;
            case TK_PUBLIC:
                access = 0;
                tk = CPrepTokenizer_GetNextToken();
                break;
            case TK_PROTECTED:
                if (copts.ARM_conform == 0) {
                    access = 2;
                    tk = CPrepTokenizer_GetNextToken();
                }
                break;
        }
        {
            if (tk == TK_VIRTUAL) {
                if (isVirtual != 0) {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                }
                isVirtual = 1;
                tk = CPrepTokenizer_GetNextToken();
            }
            if (CScope_ParseDeclName(&lookup) != 0) {
                if (lookup.type == NULL) {
                    if (lookup.name == NULL) {
                        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    } else if (tk == TK_IDENTIFIER && lookup.name == data_00587fa0) {
                        goto specialBase;
                    }
                    CError_ReportError(ERR_UNDEFINED_IDENTIFIER, data_00587fa0->name);
                    continue;
                }
                CDecl_CompleteType(lookup.type);
                if (allowDependent != 0 && CTemplateTools_IsDependentType(lookup.type) != 0) {
                    CTemplateClass_PrependTemplateRecordEntry(classType, lookup.type, access, isVirtual);
                    if (isVirtual == 0) {
                        continue;
                    }
                    classType->theclass.flags |= CLASS_HAS_VBASES;
                    continue;
                }
                if (lookup.type->type != TYPECLASS) {
                    CError_ReportError(ERR_ILLEGAL_STRUCT_UNION_ENUM_CLASS_DEFINITION);
                    continue;
                }
                baseType = (TypeClass *)lookup.type;
            } else {
            specialBase:;
                if (memcmp(data_00587fa0->name, "__somobject", 12) == 0) {
                    if (isVirtual == 0) {
                        CError_ReportError(ERR_SOM_CLASSES_INHERTIANCE_MUST_VIRTUAL);
                    }
                    CSOM_InitSOMInfo(&classType->theclass);
                    tk = CPrepTokenizer_GetNextToken();
                    break;
                }
                if (memcmp(data_00587fa0->name, "__javaobject", 13) == 0) {
                    tk = CPrepTokenizer_GetNextToken();
                    classType->theclass.action = 3;
                    break;
                }
                CError_ReportError(ERR_UNDEFINED_IDENTIFIER, data_00587fa0->name);
                continue;
            }
            if (CDecl_CheckNewBase(&classType->theclass, baseType, isVirtual) != 0) {
                base = galloc(sizeof(ClassList));
                memclrw(base, sizeof(ClassList));
                base->base = baseType;
                base->access = access;
                base->is_virtual = isVirtual;
                if (classType->theclass.bases != NULL) {
                    tail = classType->theclass.bases;
                    while (tail->next != NULL) {
                        tail = tail->next;
                    }
                    tail->next = base;
                } else {
                    classType->theclass.bases = base;
                }
            }
        }
    } while ((tk = CPrepTokenizer_GetNextToken()) == 44);
    if ((classType->theclass.flags & CLASS_HAS_VBASES) != 0) {
        CDecl_SetVBaseOffsets(&classType->theclass);
    }
    if (copts.def_inherited != 0 && classType->theclass.bases != NULL && classType->theclass.bases->next == NULL) {
        typeObject = galloc(sizeof(ObjType));
        memclrw(typeObject, sizeof(ObjType));
        typeObject->otype = OT_TYPE;
        typeObject->access = ACCESSPUBLIC;
        typeObject->type = (Type *)classType->theclass.bases->base;
        CScope_AddObject(classType->theclass.nspace, GetHashNameNode("inherited"), (ObjBase *)typeObject);
    }
    return;
}

void make_defarg_function(TypeClass *cls)
{
    Object *function;
    NameSpaceObjectList *member;
    Object *memberObject;
    FuncArg *defaultArg;
    TypeMemberFunc *functionType;
    TypeMemberFunc *memberType;
    FuncArg *virtualBaseArg;
    DefArgCtorInfo *defaultArgData;

    member = CClass_Constructor(cls);
    if (member == NULL)
        return;
    while (member != NULL) {
        if ((memberObject = (Object *)member->object)->otype == OT_OBJECT && memberObject->type->type == TYPEFUNC) {
            memberType = (TypeMemberFunc *)memberObject->type;
            defaultArg = memberType->args->next;
            if (cls->flags & CLASS_HAS_VBASES)
                defaultArg = defaultArg->next;
            if (defaultArg != NULL && defaultArg->dexpr != NULL) {
                cls->flags |= 0x80;
                function = CParser_NewFunctionObject(NULL);
                functionType = (TypeMemberFunc *)galloc(sizeof(TypeMemberFunc));
                memclrw(functionType, sizeof(*functionType));
                functionType->type = TYPEFUNC;
                functionType->functype = (Type *)&void_ptr;
                functionType->flags = (FUNC_METHOD | FUNC_AUTO_GENERATED | FUNC_IS_DTOR);
                functionType->theclass = cls;
                functionType->is_static = 0;
                if (cls->flags & CLASS_HAS_VBASES) {
                    virtualBaseArg = CParser_NewFuncArg();
                    virtualBaseArg->type = (Type *)&stsignedshort;
                    virtualBaseArg->next = functionType->args;
                    functionType->args = virtualBaseArg;
                    if (virtualBaseArg->next != NULL && virtualBaseArg->next->type == &stvoid)
                        virtualBaseArg->next = NULL;
                }
                prepend_class_pointer_argument(TYPE_FUNC(functionType), cls, 0);
                function->type = (Type *)functionType;
                function->qual = Q_MANGLE_NAME;
                function->name = constructor_name;
                function->access = memberObject->access;
                function->section = memberObject->section;
                function->nspace = cls->nspace;
                function->qual |= Q_INLINE;
                defaultArgData = (DefArgCtorInfo *)galloc(sizeof(DefArgCtorInfo));
                defaultArgData->default_func = memberObject;
                defaultArgData->default_arg = defaultArg->dexpr;
                function->u.func.defargdata = defaultArgData;
                defaultArg->dexpr = NULL;
                CScope_AddObject(cls->nspace, function->name, (ObjBase *)function);
                return;
            }
        }
        member = member->next;
    }
}

void make_auto_generated_dtor(ClassLayout *context, TypeClass *cls)
{
    ClassList *base;
    ObjMemberVar *member;
    Type *type;
    Object *info;
    HashNameNode *name;
    TypeMemberFunc *func;
    Object *obj;
    Boolean found;
    UInt8 access;

    if (CClass_Constructor(cls) != NULL) {
        make_defarg_function(cls);
        return;
    }

    found = 0;
    access = ACCESSPUBLIC;

    if (cls->flags & CLASS_HAS_VBASES)
        found = 1;
    if (context->has_vtable != 0)
        found = 1;

    for (base = cls->bases; base != NULL; base = base->next) {
        if (CClass_Constructor(base->base) != NULL) {
            info = CClass_DefaultConstructor(base->base);
            if (info == NULL) {
                CError_ReportError(ERR_CANNOT_CONSTRUCT_BASE_CLASS, base->base->classname->name);
            } else {
                UInt8 baseAccess = info->access;
                if (access == ACCESSNONE || baseAccess == ACCESSNONE || baseAccess == ACCESSPRIVATE)
                    access = ACCESSNONE;
                else
                    access = ACCESSPUBLIC;
            }
            found = 1;
        }
    }

    for (member = cls->ivars; member != NULL; member = member->next) {
        type = member->type;
        while (type->type == TYPEARRAY)
            type = TYPE_POINTER(type)->target;
        if (type->type == TYPECLASS && CClass_Constructor((TypeClass *)type) != NULL) {
            info = CClass_DefaultConstructor((TypeClass *)type);
            if (info == NULL) {
                CError_ReportError(ERR_CANNOT_CONSTRUCT_DIRECT_MEMBER, member->name->name);
            } else {
                UInt8 memberAccess = info->access;
                if (access == ACCESSNONE || memberAccess == ACCESSNONE || memberAccess == ACCESSPRIVATE)
                    access = ACCESSNONE;
                else if (memberAccess != ACCESSPUBLIC)
                    access = ACCESSNONE;
                else
                    access = ACCESSPUBLIC;
            }
            found = 1;
        }
    }

    if (found) {
        func = galloc(sizeof(*func));
        memclrw(func, sizeof(*func));
        func->type = TYPEFUNC;
        func->functype = (Type *)&void_ptr;
        func->flags = FUNC_IS_DTOR | FUNC_AUTO_GENERATED | FUNC_METHOD;
        func->theclass = cls;
        func->is_static = 0;
        if (cls->flags & CLASS_HAS_VBASES) {
            FuncArg *arg = CParser_NewFuncArg();
            arg->type = (Type *)&stsignedshort;
            arg->next = func->args;
            func->args = arg;
            if (arg->next != NULL && arg->next->type == &stvoid)
                arg->next = NULL;
        }
        prepend_class_pointer_argument(TYPE_FUNC(func), cls, 0);
        name = constructor_name;
        obj = CParser_NewCompilerDefFunctionObject();
        obj->name = name;
        obj->type = (Type *)func;
        obj->qual = Q_MANGLE_NAME;
        obj->access = access;
        obj->nspace = cls->nspace;
        obj->qual |= Q_INLINE;
        CScope_AddObject(cls->nspace, obj->name, (ObjBase *)obj);
    }
}

TypeMemberFunc *CDecl_MakeDefaultDtorType(TypeClass *theclass, char is_const)
{
    TypeMemberFunc *func;
    FuncArg *arg;
    TypePointer *class_pointer;
    FuncArg *extra_arg;

    func = (TypeMemberFunc *)galloc(sizeof(TypeMemberFunc));
    memclrw(func, sizeof(TypeMemberFunc));
    func->type = TYPEFUNC;
    func->functype = (Type *)((char *)&stvoid + 8);
    func->flags = FUNC_METHOD | FUNC_AUTO_GENERATED | FUNC_IS_DTOR;
    func->theclass = theclass;
    func->is_static = 0;
    arg = CParser_NewFuncArg();
    if (is_const) {
        arg->qual = Q_CONST;
    }
    class_pointer = (TypePointer *)galloc(sizeof(TypePointer));
    memclrw(class_pointer, sizeof(TypePointer));
    class_pointer->type = TYPEPOINTER;
    class_pointer->size = 4;
    class_pointer->target = (Type *)theclass;
    class_pointer->qual = Q_REFERENCE;
    arg->type = (Type *)class_pointer;
    func->args = arg;
    if (theclass->flags & CLASS_HAS_VBASES) {
        extra_arg = CParser_NewFuncArg();
        extra_arg->type = (Type *)&stsignedshort;
        extra_arg->next = func->args;
        func->args = extra_arg;
        if (extra_arg->next && extra_arg->next->type == &stvoid) {
            extra_arg->next = NULL;
        }
    }
    prepend_class_pointer_argument(TYPE_FUNC(func), theclass, 0);
    return func;
}

void declare_default_copy_constructor(ClassLayout *decl, TypeClass *type)
{
    UInt8 access;
    ClassList *base;
    Object *baseCtor;
    UInt8 baseAccess;
    FuncArg *baseArg;
    ObjMemberVar *member;
    Type *memberType;
    Object *memberCtor;
    UInt8 memberAccess;
    FuncArg *memberArg;
    Object *ctor;
    HashNameNode *name;
    Boolean needed;
    Boolean constArg;
    TypeMemberFunc *ctorType;

    if (CClass_CopyConstructor(type) != NULL) {
        return;
    }
    access = ACCESSPUBLIC;
    constArg = 1;
    needed = 0;
    if (CClass_Constructor(type) != NULL) {
        needed = 1;
    }
    if ((type->flags & CLASS_HAS_VBASES) != 0 || decl->has_vtable != 0) {
        needed = 1;
    }
    for (base = type->bases; base != NULL; base = base->next) {
        baseCtor = CClass_CopyConstructor(base->base);
        if (baseCtor != NULL) {
            needed = 1;
            baseAccess = baseCtor->access;
            if (access == ACCESSNONE || baseAccess == ACCESSNONE || baseAccess == ACCESSPRIVATE) {
                access = ACCESSNONE;
            } else {
                access = ACCESSPUBLIC;
            }
            baseArg = TYPE_METHOD(baseCtor->type)->args->next;
            if ((base->base->flags & CLASS_HAS_VBASES) != 0) {
                baseArg = baseArg->next;
            }
            if ((baseArg->qual & Q_CONST) == 0) {
                constArg = 0;
            }
        }
    }
    for (member = type->ivars; member != NULL; member = member->next) {
        memberType = member->type;
        while (memberType->type == TYPEARRAY) {
            memberType = TYPE_POINTER(memberType)->target;
        }
        if (memberType->type == TYPECLASS) {
            memberCtor = CClass_CopyConstructor(TYPE_CLASS(memberType));
            if (memberCtor != NULL) {
                needed = 1;
                memberAccess = memberCtor->access;
                if (access == ACCESSNONE || memberAccess == ACCESSNONE || memberAccess == ACCESSPRIVATE) {
                    access = ACCESSNONE;
                } else if (memberAccess != ACCESSPUBLIC) {
                    access = ACCESSNONE;
                } else {
                    access = ACCESSPUBLIC;
                }
                memberArg = TYPE_METHOD(memberCtor->type)->args->next;
                if ((TYPE_CLASS(memberType)->flags & CLASS_HAS_VBASES) != 0) {
                    memberArg = memberArg->next;
                }
                if ((memberArg->qual & Q_CONST) == 0) {
                    constArg = 0;
                }
            }
        }
    }
    if (needed != 0) {
        ctorType = CDecl_MakeDefaultDtorType(type, constArg);
        name = constructor_name;
        ctor = CParser_NewCompilerDefFunctionObject();
        ctor->name = name;
        ctor->type = (Type *)ctorType;
        ctor->qual = Q_MANGLE_NAME;
        ctor->access = access;
        ctor->nspace = type->nspace;
        ctor->qual |= Q_INLINE;
        CScope_AddObject(type->nspace, ctor->name, (ObjBase *)ctor);
    }
}

void generate_copy_constructor(ClassLayout *type, TypeClass *cls)
{
    DeclInfo decl;
    Boolean keepconst = 1;
    Boolean generate = 0;
    UInt8 access = 0;
    Object *constructor;
    ClassList *base;
    ObjMemberVar *member;
    Type *membertype;
    UInt8 baseaccess;
    TypePointer *resultptr;
    TypeMemberFunc *func;
    FuncArg *arg;
    TypePointer *argptr;

    constructor = CClass_AssignmentOperator(cls);
    if (constructor == NULL) {
        if ((cls->flags & CLASS_HAS_VBASES) || type->has_vtable != 0 ||
            CClass_MemberObject(cls, assignment_operator_name) != NULL)
            generate = 1;
        for (base = cls->bases; base != NULL; base = base->next) {
            constructor = CClass_AssignmentOperator(base->base);
            if (constructor != NULL) {
                generate = 1;
                baseaccess = constructor->access;
                if (access == 3 || baseaccess == 3 || baseaccess == 1)
                    access = 3;
                else
                    access = 0;
                if ((TYPE_FUNC(constructor->type)->args->next->qual & Q_CONST) == 0)
                    keepconst = 0;
            }
        }
        for (member = cls->ivars; member != NULL; member = member->next) {
            membertype = member->type;
            while (membertype->type == TYPEARRAY)
                membertype = TPTR_TARGET(membertype);
            if (membertype->type == TYPECLASS) {
                constructor = CClass_AssignmentOperator(TYPE_CLASS(membertype));
                if (constructor != NULL) {
                    generate = 1;
                    baseaccess = constructor->access;
                    if (access == 3 || baseaccess == 3 || baseaccess == 1)
                        access = 3;
                    else if (baseaccess != 0)
                        access = 3;
                    else
                        access = 0;
                    if ((TYPE_FUNC(constructor->type)->args->next->qual & Q_CONST) == 0)
                        keepconst = 0;
                }
            }
        }
    }
    if (generate) {
        memclrw(&decl, sizeof(decl));
        decl.qual |= Q_INLINE;
        func = galloc(sizeof(*func));
        memclrw(func, sizeof(*func));
        func->type = TYPEFUNC;
        resultptr = galloc(sizeof(*resultptr));
        memclrw(resultptr, sizeof(*resultptr));
        resultptr->type = TYPEPOINTER;
        resultptr->size = sizeof(Type *);
        resultptr->target = TYPE(cls);
        resultptr->qual = Q_REFERENCE;
        func->functype = TYPE(resultptr);
        func->flags = FUNC_METHOD | FUNC_AUTO_GENERATED;
        func->theclass = cls;
        func->is_static = 0;
        arg = CParser_NewFuncArg();
        if (keepconst)
            arg->qual = Q_CONST;
        argptr = galloc(sizeof(*argptr));
        memclrw(argptr, sizeof(*argptr));
        argptr->type = TYPEPOINTER;
        argptr->size = sizeof(Type *);
        argptr->target = TYPE(cls);
        argptr->qual = Q_REFERENCE;
        arg->type = TYPE(argptr);
        func->args = arg;
        prepend_class_pointer_argument(TYPE_FUNC(func), cls, 0);
        decl.thetype = TYPE(func);
        decl.name = assignment_operator_name;
        declare_member_function(type, cls, &decl, access, 1, 0, 0, 0);
    }
}

void declare_auto_generated_destructor(ClassLayout *type, TypeClass *cls)
{
    ClassList *base;
    ObjMemberVar *member;
    Object *object;
    Type *memberType;
    Boolean isVirtual;
    Boolean hasFunction;
    AccessType access;
    AccessType inheritedAccess;
    DeclInfo spec;
    TypeMemberFunc *func;
    FuncArg *arg;

    if (CClass_Destructor(cls) != NULL)
        return;

    hasFunction = 0;
    isVirtual = 0;
    access = ACCESSPUBLIC;

    for (base = cls->bases; base != NULL; base = base->next) {
        object = CClass_Destructor(base->base);
        if (object != NULL) {
            hasFunction = 1;
            if (object->datatype == DVFUNC)
                isVirtual = 1;
            inheritedAccess = object->access;
            if (access == ACCESSNONE || inheritedAccess == ACCESSNONE || inheritedAccess == ACCESSPRIVATE)
                access = ACCESSNONE;
            else
                access = ACCESSPUBLIC;
        }
    }

    for (member = cls->ivars; member != NULL; member = member->next) {
        memberType = member->type;
        while (memberType->type == TYPEARRAY)
            memberType = TPTR_TARGET(memberType);
        if (memberType->type == TYPECLASS) {
            object = CClass_Destructor((TypeClass *)memberType);
            if (object != NULL) {
                hasFunction = 1;
                inheritedAccess = object->access;
                if (access == ACCESSNONE || inheritedAccess == ACCESSNONE || inheritedAccess == ACCESSPRIVATE)
                    access = ACCESSNONE;
                else
                    access = inheritedAccess ? ACCESSNONE : ACCESSPUBLIC;
            }
        }
    }

    if (hasFunction) {
        memclrw(&spec, sizeof(spec));
        spec.qual |= Q_INLINE;
        if (isVirtual)
            spec.qual |= Q_VIRTUAL;
        func = galloc(sizeof(TypeMemberFunc));
        memclrw(func, sizeof(TypeMemberFunc));
        func->type = TYPEFUNC;
        func->functype = (Type *)&void_ptr;
        func->flags = FUNC_METHOD | FUNC_AUTO_GENERATED | 0x4000;
        func->theclass = cls;
        func->is_static = 0;
        arg = CParser_NewFuncArg();
        arg->type = (Type *)&stsignedshort;
        arg->next = func->args;
        func->args = arg;
        if (arg->next != NULL && arg->next->type == &stvoid)
            arg->next = NULL;
        prepend_class_pointer_argument(TYPE_FUNC(func), cls, 0);
        spec.thetype = (Type *)func;
        spec.name = destructor_name;
        declare_member_function(type, cls, &spec, access, 1, 0, 0, 0);
    }
}

void fill_class_layout_entries(ClassLayout *table, TypeClass *type, ObjBase **entries)
{
    unsigned int bytes;
    Object *member;
    ObjMemberVar *variable;
    int slot;
    int i;
    unsigned int filled;
    CScopeObjectIterator scope;
    Object *obj;

    if (table->lex_order_count > 32) {
        bytes = table->lex_order_count * sizeof(*entries);
        entries = (ObjBase **)CompilerTools_AllocatePool(bytes);
    } else {
        bytes = 32 * sizeof(*entries);
    }
    memclrw(entries, bytes);
    table->objlist = entries;
    CScope_InitObjectIterator(&scope, type->nspace);
    filled = 0;
    for (;;) {
        obj = CScope_NextObjectIteratorObject(&scope);
        if (obj == NULL)
            break;
        if (obj->type->type == TYPEFUNC && (((TypeFunc *)obj->type)->flags & FUNC_METHOD) != 0 &&
            obj->datatype != DALIAS) {
            if ((slot = ((TypeMemberFunc *)obj->type)->vtbl_index) > 0) {
                --slot;
                if (slot >= table->lex_order_count || entries[slot] != NULL)
                    CError_FATAL(5811);
                entries[slot] = OBJ_BASE(obj);
                ++filled;
                if (obj->datatype != DVFUNC && !((TypeMemberFunc *)obj->type)->is_static) {
                    if (CClass_OverridesBaseMember(type, obj->name, obj)) {
                        if (is_pascal_object(obj))
                            CError_ReportError(ERR_VIRTUAL_FUNCTIONS_CANNOT_PASCAL_FUNCTIONS);
                        if (type->mode == 1)
                            CError_ReportError(ERR_ILLEGAL_VIRTUAL_FUNCTION_UNION, obj);
                        obj->datatype = DVFUNC;
                    }
                }
                if (obj->datatype == DVFUNC) {
                    table->has_vtable = 1;
                    if (type->vtable == NULL) {
                        CABI_AddVTable(type);
                        table->firstVirtualSlot = slot;
                    } else if (slot < table->firstVirtualSlot) {
                        table->firstVirtualSlot = slot;
                    }
                } else if (((TypeFunc *)obj->type)->flags & FUNC_PURE) {
                    CError_ReportError(ERR_PURE_FUNCTION_NOT_VIRTUAL, obj);
                    ((TypeFunc *)obj->type)->flags &= ~FUNC_PURE;
                }
            } else if (slot != 0) {
                CError_FATAL(5860);
            }
            if (type->sominfo == NULL)
                ((TypeMemberFunc *)obj->type)->vtbl_index = 0;
        }
    }
    if (type->action == 0) {
        for (i = 0; i < table->lex_order_count; ++i) {
            if ((member = OBJECT(entries[i])) != NULL && member->datatype == DVFUNC && (member->qual & Q_INLINE) == 0 &&
                (((TypeFunc *)member->type)->flags & FUNC_PURE) == 0) {
                type->action = 1;
                ((TypeFunc *)member->type)->flags |= 4;
                break;
            }
        }
    }
    if (type->sominfo == NULL) {
        i = 0;
        variable = type->ivars;
        for (; i < table->lex_order_count; ++i) {
            if (entries[i] == NULL) {
                if (variable == NULL)
                    CError_FATAL(5897);
                entries[i] = OBJ_BASE(variable);
                variable = variable->next;
                ++filled;
            }
        }
        if (variable != NULL)
            CError_FATAL(5903);
    }
    if (filled != table->lex_order_count)
        CError_FATAL(5906);
}

void CDecl_CompleteClass(ClassLayout *ctx, TypeClass *cls)
{
    void fn_004e9ca0(TypeClass *);
    ObjBase *buf[32];
    ClassList *cl;
    TypeClass *base;

    for (cl = cls->bases; cl != NULL; cl = cl->next) {
        if ((base = cl->base)->vtable != NULL)
            ctx->has_vtable = 1;
    }

    if (cls->sominfo == NULL) {
        fn_004ebae0(cls);
        declare_auto_generated_destructor(ctx, cls);
        generate_copy_constructor(ctx, cls);
        make_auto_generated_dtor(ctx, cls);
        declare_default_copy_constructor(ctx, cls);
    }

    fill_class_layout_entries(ctx, cls, buf);

    if (ctx->has_vtable)
        CClass_CheckOverrides(cls);

    CABI_LayoutClass(ctx, cls);

    if (cls->sominfo != NULL)
        CSOM_CompleteClass(cls);

    if ((cls->flags & CLASS_IS_TEMPL_INST) && (((TemplClassInst *)cls)->is_specialized == 0))
        cls->action = 0;

    if (cls->action == 0)
        CClass_MakeStaticActionClass(cls);

    fn_004e9ca0(cls);
}

TypeClass *CDecl_DefineClass(struct NameSpace *nspace, struct HashNameNode *name, struct TypeClass *type, short mode,
                             char flag4, char flag5)
{
    struct NameSpace *classSpace;
    struct ObjType *typeObject;
    if (type == NULL && nspace->theclass != NULL && (nspace->theclass->flags & CLASS_IS_TEMPL) != 0) {
        CError_ASSERT(6004, flag4 == 0);
        return &CTemplateClass_CreateClassTemplateDeclaration(nspace->theclass, name, mode)->theclass;
    }
    classSpace = CScope_NewListNameSpace(name, 1);
    if (type == NULL) {
        type = (struct TypeClass *)galloc(sizeof(*type));
        memclrw(type, sizeof(*type));
    }
    type->type = TYPECLASS;
    type->align = 1;
    type->mode = mode;
    type->action = 0;
    if (name != NULL) {
        type->classname = name;
        if (flag5 != 0) {
            if (flag4 != 0) {
                typeObject = (struct ObjType *)galloc(sizeof(*typeObject));
                memclrw(typeObject, sizeof(*typeObject));
                typeObject->otype = OT_TYPE;
                typeObject->access = ACCESSPUBLIC;
                typeObject->type = (struct Type *)type;
                CScope_AddObject(nspace, name, (ObjBase *)typeObject);
            } else {
                CScope_DefineTypeTag(nspace, name, (Type *)type);
            }
        }
        CScope_DefineTypeTag(classSpace, name, (Type *)type);
        if (cscope_currentfunc != NULL) {
            classSpace->name = CParser_AppendUniqueNameFile(name->name);
        }
        if (copts.direct_to_som != 0 && nspace == cscope_root) {
            if (memcmp(name->name, "SOMObject", 10) == 0) {
                CSOM_InitSOMInfo(type);
            }
        }
    } else {
        classSpace->name = type->classname = CParser_AppendUniqueNameFile("@class");
    }
    type->nspace = classSpace;
    classSpace->theclass = type;
    classSpace->parent = nspace;
    if (nspace->is_global == 0) {
        CParser_PrependClassParseRec(type);
    }
    return type;
}

UInt8 CDecl_ParseDeclarationAttributeFlags(void)
{
    UInt8 flags;
    DeclInfo declaration;

    flags = 0;
    tk = CPrepTokenizer_GetNextToken();
    if (tk == '(') {
        memclrw(&declaration, sizeof(declaration));
        CParser_ParseDeclSpec(&declaration, 1);
        if ((declaration.exportflags & 0x10) != 0) {
            flags |= 1;
        }
        if ((declaration.exportflags & 0x20) != 0) {
            flags |= 2;
        }
        if ((declaration.exportflags & 0x40) != 0) {
            flags |= 4;
        }
        tk = CPrepTokenizer_GetNextToken();
        if (tk != ')') {
            CError_ReportError(ERR_RPAREN_EXPECTED);
        } else {
            tk = CPrepTokenizer_GetNextToken();
        }
    } else {
        CError_ReportError(ERR_LPAREN_EXPECTED);
    }
    return flags;
}

void CDecl_ParseClass(DeclInfo *ctx, SInt16 kind, Boolean advanceToken, UInt8 extraFlags)
{
    TypeClass *obj;
    HashNameNode *name;
    Type *existing;
    Boolean isTemplate;
    Boolean savedContext;
    SInt32 textOffset;
    SInt16 nextToken;
    NameResult spec;
    ClassLayout declarationState;
    CScopeSave scopeSave;
    GList contextSave;
    FileOffsetInfo nameSave;
    memclrw(&declarationState, 14);
    if (tk == TK_UU_DECLSPEC)
        extraFlags |= CDecl_ParseDeclarationAttributeFlags();
    if ((obj = ctx->pendingClass) == NULL) {
        switch (tk) {
            case ':':
            case '{':
                obj = CDecl_DefineClass(cscope_current, NULL, NULL, kind, 0, 1);
                obj->eflags |= extraFlags;
                break;
            case TK_IDENTIFIER:
                name = data_00587fa0;
                if (ctx->allowForeignNamespace == 0 && ctx->isNewTypeId == 0 &&
                    ((nextToken = CPrepTokenizer_GetNextTokenAndRestorePosition()) == ':' || nextToken == ';' ||
                     nextToken == '{')) {
                    tk = CPrepTokenizer_GetNextToken();
                    existing = CScope_GetTagType(cscope_current, name);
                    if (existing != NULL) {
                    resolveDeclaration:
                        if (existing->type != TYPECLASS) {
                            if (existing->type == TYPETEMPLATE || existing->type == TYPESTRUCT) {
                                if (tk != '{' && tk != ':') {
                                    ctx->thetype = existing;
                                    return;
                                }
                            }
                            CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, name->name);
                            obj = CDecl_DefineClass(cscope_current, NULL, NULL, kind, 0, 1);
                        } else {
                            SInt8 declaredKind;
                            obj = (TypeClass *)existing;
                            declaredKind = obj->mode;
                            if (declaredKind != kind) {
                                int classKind = kind;
                                if ((classKind == 2 && obj->mode == 0) || (classKind == 0 && obj->mode == 2)) {
                                    if (copts.warn_structclass != 0)
                                        CError_Warning(ERR_INCONSISTENT_USE_CLASS_STRUCT_KEYWORDS);
                                } else {
                                    CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, name);
                                }
                            }
                            obj->eflags |= extraFlags;
                        }
                    } else {
                        obj = CDecl_DefineClass(cscope_current, name, NULL, kind, 0, 1);
                        obj->eflags |= extraFlags;
                    }
                    break;
                }
                data_00587fa0 = name;
            default:
                if (!CScope_ParseElaborateName(&spec)) {
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    ctx->thetype = (Type *)&stsignedint;
                    return;
                }
                tk = CPrepTokenizer_GetNextToken();
                if ((existing = spec.type) != NULL)
                    goto resolveDeclaration;
                CError_ASSERT(6192, spec.name != NULL);
                obj = CDecl_DefineClass(CScope_FindNonClassNonTemplNameSpace(cscope_current), spec.name, NULL, kind, 0,
                                        1);
                obj->eflags |= extraFlags;
        }
    }
    ctx->thetype = (Type *)obj;
    if (tk != ':' && tk != '{')
        return;
    if ((obj->flags & CLASS_COMPLETED) != 0) {
        CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, obj->classname->name);
        obj = CDecl_DefineClass(cscope_current, NULL, NULL, kind, 0, 1);
    }
    {
        NameSpace *ns = cscope_current;
        while (ns != NULL) {
            if (ns == obj->nspace) {
                CError_ReportError(ERR_STRUCT_UNION_ENUM_CLASS_TAG_REDEFINED, obj->classname->name);
                obj = CDecl_DefineClass(cscope_current, NULL, NULL, kind, 0, 1);
                break;
            }
            ns = ns->parent;
        }
    }
    isTemplate = (obj->flags & CLASS_IS_TEMPL) != 0;
    if ((obj->flags & CLASS_IS_TEMPL_INST) != 0)
        begin_class_instantiation((TemplClassInst *)obj, ctx);
    if (copts.structalignment < 0 || copts.structalignment > 0xe)
        CError_FATAL(6245);
    obj->eflags |= ((copts.structalignment + 1) << 4) & CLASS_EFLAGS_F0;
    if (tk == ':')
        parse_class_bases((TemplClass *)obj, kind, isTemplate);
    CScope_SetClassDefScope(obj, &scopeSave);
    if (tk == '{') {
        tk = CPrepTokenizer_GetNextToken();
        savedContext = class_browse_enabled(ctx);
        if (savedContext) {
            nameSave = member_foi;
            CBrowse_GenerateClassRecord(ctx, &contextSave);
        }
        parse_class_members(&declarationState, obj, kind);
        textOffset = CPrep_GetCurrentTextOffset();
        if (advanceToken != 0)
            tk = CPrepTokenizer_GetNextToken();
        if (savedContext) {
            member_foi = nameSave;
            if (advanceToken != 0 && tk == ';')
                CPrep_GetCurrentTextOffset();
            CBrowse_RestoreScope(textOffset, &contextSave);
        }
    } else {
        CError_ReportError(ERR_LBRACE_EXPECTED);
    }
    if (isTemplate) {
        TemplClass *templateClass = (TemplClass *)obj;
        CTemplateClass_CompleteClassLayout(templateClass, &declarationState);
    } else
        CDecl_CompleteClass(&declarationState, obj);
    CScope_RestoreScope(&scopeSave);
}
