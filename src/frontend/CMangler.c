#define CERROR_FILE "CMangler.c"
#include "compiler/common.h"
#include "compiler/CMangler.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Registers.h"
#include "compiler/StrengthReduction.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include <string.h>
#include <stdio.h>

#include "compiler/Objects.h"
/* Declarations gathered from the merged files. */

static SInt32 ElemSize(TypePointer *t);

static void MangleQualifiers(UInt32 q);

static inline void appendObjectName(const void *name)
{
    CompilerTools_AppendGListString(&data_00583548, name);
}

static inline unsigned int mangledNameHandle(void)
{
    return (unsigned int)*data_00583548.data;
}

static inline void appendMangledName(const void *name)
{
    CompilerTools_AppendGListString(&data_00583548, name);
}

/* Compute (and cache) the link name of an object, following aliases. */
static inline HashNameNode *CMangler_LinkName(Object *obj)
{
    while (obj->datatype == DALIAS)
        obj = obj->u.alias.object;
    switch (obj->datatype) {
        case DFUNC:
        case DVFUNC:
            if (obj->u.func.linkname == NULL)
                obj->u.func.linkname = CMangler_GetLinkName(obj);
            return obj->u.func.linkname;
        case DDATA:
            if (obj->u.data.linkname == NULL)
                obj->u.data.linkname = get_object_link_name(obj);
            return obj->u.data.linkname;
        case DINLINEFUNC:
            return CMangler_GetLinkName(obj);
        case DLOCAL:
        case DABSOLUTE:
        case DUNUSED:
            return obj->name;
        default:
            CError_FATAL(1012);
            return NULL;
    }
}

void CMangler_Setup(void)

{
    typedef UInt8 *NameString;
    constructor_name = GetHashNameNode("__ct");
    destructor_name = GetHashNameNode("__dt");
    assignment_operator_name = GetHashNameNode("__as");
    return;
}

HashNameNode *CMangler_BasicDtorName(void)
{
    return GetHashNameNode("__dtb");
}

HashNameNode *CMangler_VBaseDtorName(void)
{
    return GetHashNameNode("__dtv");
}

HashNameNode *CMangler_ArrayDtorName(void)
{
    return GetHashNameNode("__dta");
}

HashNameNode *CMangler_SDeleteDtorName(void)
{
    return GetHashNameNode("__dts");
}

HashNameNode *CMangler_DeleteDtorName(void)
{
    return GetHashNameNode("__dt");
}

char *CMangler_GetOperator(HashNameNode *name)
{
    char *operatorCode;
    if (name == assignment_operator_name) {
        return "operator=";
    }
    operatorCode = name->name;
    if (memcmp(name->name, "__nw", 5) == 0) {
        return "operator new";
    }
    if (memcmp(operatorCode, "__dl", 5) == 0) {
        return "operator delete";
    }
    if (memcmp(operatorCode, "__nwa", 6) == 0) {
        return "operator new[]";
    }
    if (memcmp(operatorCode, "__dla", 6) == 0) {
        return "operator delete[]";
    }
    if (memcmp(operatorCode, "__pl", 5) == 0) {
        return "operator+";
    }
    if (memcmp(operatorCode, "__mi", 5) == 0) {
        return "operator-";
    }
    if (memcmp(operatorCode, "__ml", 5) == 0) {
        return "operator*";
    }
    if (memcmp(operatorCode, "__dv", 5) == 0) {
        return "operator/";
    }
    if (memcmp(operatorCode, "__md", 5) == 0) {
        return "operator%";
    }
    if (memcmp(operatorCode, "__er", 5) == 0) {
        return "operator^";
    }
    if (memcmp(operatorCode, "__ad", 5) == 0) {
        return "operator&";
    }
    if (memcmp(operatorCode, "__or", 5) == 0) {
        return "operator|";
    }
    if (memcmp(operatorCode, "__co", 5) == 0) {
        return "operator~";
    }
    if (memcmp(operatorCode, "__nt", 5) == 0) {
        return "operator!";
    }
    if (memcmp(operatorCode, "__lt", 5) == 0) {
        return "operator<";
    }
    if (memcmp(operatorCode, "__gt", 5) == 0) {
        return "operator>";
    }
    if (memcmp(operatorCode, "__apl", 6) == 0) {
        return "operator+=";
    }
    if (memcmp(operatorCode, "__ami", 6) == 0) {
        return "operator-=";
    }
    if (memcmp(operatorCode, "__amu", 6) == 0) {
        return "operator*=";
    }
    if (memcmp(operatorCode, "__adv", 6) == 0) {
        return "operator/=";
    }
    if (memcmp(operatorCode, "__amd", 6) == 0) {
        return "operator%=";
    }
    if (memcmp(operatorCode, "__aer", 6) == 0) {
        return "operator^=";
    }
    if (memcmp(operatorCode, "__aad", 6) == 0) {
        return "operator&=";
    }
    if (memcmp(operatorCode, "__aor", 6) == 0) {
        return "operator|=";
    }
    if (memcmp(operatorCode, "__ls", 5) == 0) {
        return "operator<<";
    }
    if (memcmp(operatorCode, "__rs", 5) == 0) {
        return "operator>>";
    }
    if (memcmp(operatorCode, "__als", 6) == 0) {
        return "operator<<=";
    }
    if (memcmp(operatorCode, "__ars", 6) == 0) {
        return "operator>>=";
    }
    if (memcmp(operatorCode, "__eq", 5) == 0) {
        return "operator==";
    }
    if (memcmp(operatorCode, "__ne", 5) == 0) {
        return "operator!=";
    }
    if (memcmp(operatorCode, "__le", 5) == 0) {
        return "operator<=";
    }
    if (memcmp(operatorCode, "__ge", 5) == 0) {
        return "operator>=";
    }
    if (memcmp(operatorCode, "__aa", 5) == 0) {
        return "operator&&";
    }
    if (memcmp(operatorCode, "__oo", 5) == 0) {
        return "operator||";
    }
    if (memcmp(operatorCode, "__pp", 5) == 0) {
        return "operator++";
    }
    if (memcmp(operatorCode, "__mm", 5) == 0) {
        return "operator--";
    }
    if (memcmp(operatorCode, "__cm", 5) == 0) {
        return "operator,";
    }
    if (memcmp(operatorCode, "__rm", 5) == 0) {
        return "operator->*";
    }
    if (memcmp(operatorCode, "__rf", 5) == 0) {
        return "operator*";
    }
    if (memcmp(operatorCode, "__cl", 5) == 0) {
        return "operator()";
    }
    if (memcmp(operatorCode, "__vc", 5) == 0) {
        return "operator[]";
    }
    return (char *)0;
}

HashNameNode *CMangler_OperatorName(short token)
{
    switch (token) {
        case 0x147:
            return GetHashNameNode("__nw");
        case 0x145:
            return GetHashNameNode("__dl");
        case 0x182:
            return GetHashNameNode("__nwa");
        case 0x183:
            return GetHashNameNode("__dla");
        case 0x2b:
            return GetHashNameNode("__pl");
        case 0x2d:
            return GetHashNameNode("__mi");
        case 0x2a:
            return GetHashNameNode("__ml");
        case 0x2f:
            return GetHashNameNode("__dv");
        case 0x25:
            return GetHashNameNode("__md");
        case 0x5e:
            return GetHashNameNode("__er");
        case 0x26:
            return GetHashNameNode("__ad");
        case 0x7c:
            return GetHashNameNode("__or");
        case 0x7e:
            return GetHashNameNode("__co");
        case 0x21:
            return GetHashNameNode("__nt");
        case 0x3d:
            return assignment_operator_name;
        case 0x3c:
            return GetHashNameNode("__lt");
        case 0x3e:
            return GetHashNameNode("__gt");
        case 0x15f:
            return GetHashNameNode("__apl");
        case 0x160:
            return GetHashNameNode("__ami");
        case 0x15c:
            return GetHashNameNode("__amu");
        case 0x15d:
            return GetHashNameNode("__adv");
        case 0x15e:
            return GetHashNameNode("__amd");
        case 0x164:
            return GetHashNameNode("__aer");
        case 0x163:
            return GetHashNameNode("__aad");
        case 0x165:
            return GetHashNameNode("__aor");
        case 0x16c:
            return GetHashNameNode("__ls");
        case 0x16d:
            return GetHashNameNode("__rs");
        case 0x161:
            return GetHashNameNode("__als");
        case 0x162:
            return GetHashNameNode("__ars");
        case 0x168:
            return GetHashNameNode("__eq");
        case 0x169:
            return GetHashNameNode("__ne");
        case 0x16a:
            return GetHashNameNode("__le");
        case 0x16b:
            return GetHashNameNode("__ge");
        case 0x167:
            return GetHashNameNode("__aa");
        case 0x166:
            return GetHashNameNode("__oo");
        case 0x16e:
            return GetHashNameNode("__pp");
        case 0x16f:
            return GetHashNameNode("__mm");
        case 0x2c:
            return GetHashNameNode("__cm");
        case 0x173:
            return GetHashNameNode("__rm");
        case 0x170:
            return GetHashNameNode("__rf");
        case 0x28:
            return GetHashNameNode("__cl");
        case 0x5b:
            return GetHashNameNode("__vc");
    }
    return NULL;
}

HashNameNode *CMangler_VTableName(TypeClass *entry)
{
    char **buffer;
    char **current;
    char **resultBuffer;
    HashNameNode *name;
    data_00583548.size = 0;
    CompilerTools_AppendGListString(&data_00583548, "__vt__");
    if (entry->classname == NULL)
        mangle_qualified_name(entry->nspace->parent, "class");
    else
        mangle_qualified_name(entry->nspace->parent, entry->nspace->name->name);
    AppendGListByte(&data_00583548, 0);
    buffer = data_00583548.data;
    COS_LockHandle(buffer);
    current = data_00583548.data;
    name = GetHashNameNode(*current);
    resultBuffer = data_00583548.data;
    COS_UnlockHandle(resultBuffer);
    return name;
}

HashNameNode *CMangler_RTTIObjectName(Type *type, unsigned int flags)
{
    char **buffer;
    HashNameNode *name;

    data_00583548.size = 0;
    CompilerTools_AppendGListString(&data_00583548, "__RTTI__");
    mangle_type(type, flags);
    AppendGListByte(&data_00583548, 0);
    COS_LockHandle(data_00583548.data);
    buffer = data_00583548.data;
    name = GetHashNameNode(*buffer);
    COS_UnlockHandle(data_00583548.data);
    return name;
}

HashNameNode *CMangler_ThunkName(Object *input, int offset, int adjustment, int index)
{
    Object *object = input;
    char buffer[64];
    unsigned char type;
    char **nameBuffer;
    HashNameNode *result;
    HashNameNode *name;
    while ((type = object->datatype) == 6)
        object = object->u.alias.object;
    switch (type) {
        case 3:
        case 4:
            if (!object->u.func.linkname)
                object->u.func.linkname = CMangler_GetLinkName(object);
            name = object->u.func.linkname;
            break;
        case 0:
            if (!object->u.data.linkname)
                object->u.data.linkname = get_object_link_name(object);
            name = object->u.data.linkname;
            break;
        case 5:
            name = CMangler_GetLinkName(object);
            break;
        case 1:
        case 2:
        case 8:
            name = object->name;
            break;
        default:
            CError_FATAL(1012);
            name = NULL;
            break;
    }
    data_00583548.size = 0;
    if (adjustment == 0) {
        if (index < 0)
            sprintf(buffer, "@%ld@", -offset);
        else
            sprintf(buffer, "@%ld@%ld@", -offset, index);
    } else {
        sprintf(buffer, "@%ld@%ld@%ld@", -offset, index, adjustment);
    }
    CompilerTools_AppendGListString(&data_00583548, buffer);
    AppendGListName(&data_00583548, name->name);
    COS_LockHandle(data_00583548.data);
    nameBuffer = data_00583548.data;
    result = GetHashNameNode(*nameBuffer);
    COS_UnlockHandle(data_00583548.data);
    return result;
}

HashNameNode *CMangler_TemplateInstanceName(HashNameNode *name, TemplArg *list)
{
    TemplArg *argument;
    ENode *expression;
    HashNameNode *link;
    char decimal[32];
    HashNameNode *result;
    char **buffer;

    for (argument = list; argument != NULL; argument = argument->next) {
        if (argument->pid.type == 0) {
            if ((expression = argument->data.paramdecl.expr) == NULL)
                CError_FATAL(361);
            if (expression->rtype->type != TYPETEMPLDEPEXPR) {
                switch (expression->type) {
                    case EINTCONST:
                        break;
                    case EOBJREF:
                        (void)CMangler_LinkName(expression->data.objref);
                        break;
                    default:
                        CError_FATAL(374);
                }
            }
        }
    }

    data_00583548.size = 0;
    CompilerTools_AppendGListString(&data_00583548, name->name);
    AppendGListByte(&data_00583548, '<');

    for (argument = list; argument != NULL; argument = argument->next) {
        if (argument->pid.type == 0) {
            if ((expression = argument->data.paramdecl.expr) == NULL)
                CError_FATAL(392);
            if (expression->rtype->type != TYPETEMPLDEPEXPR) {
                switch (expression->type) {
                    case EINTCONST:
                        CInt64_PrintDec(decimal, expression->data.intval);
                        CompilerTools_AppendGListString(&data_00583548, decimal);
                        break;
                    case EOBJREF:
                        AppendGListByte(&data_00583548, '&');
                        link = CMangler_LinkName(expression->data.objref);
                        CompilerTools_AppendGListString(&data_00583548, link->name);
                        break;
                    default:
                        CError_FATAL(409);
                }
            } else {
                AppendGListByte(&data_00583548, 'T');
            }
        } else {
            mangle_type(argument->data.typeparam.type, argument->data.typeparam.qual);
        }
        if (argument->next != NULL)
            AppendGListByte(&data_00583548, ',');
    }

    AppendGListByte(&data_00583548, '>');
    AppendGListByte(&data_00583548, 0);
    COS_LockHandle(data_00583548.data);
    buffer = data_00583548.data;
    result = GetHashNameNode(*buffer);
    COS_UnlockHandle(data_00583548.data);
    return result;
}

void mangle_qualified_name(NameSpace *nameSpace, const char *name)
{
    NameSpace *scope;
    int length;
    int count;
    const char *parts[10];
    char lengthBuffer[16];
    const char *part;

    parts[0] = name;
    count = 1;
    scope = nameSpace;
    if (nameSpace != NULL) {
        do {
            if (scope->name != NULL) {
                parts[count] = scope->name->name;
                count = count + 1;
                if (count >= 9)
                    break;
            }
            scope = scope->parent;
        } while (scope != NULL);
    }
    if (1 < count) {
        AppendGListByte(&data_00583548, 'Q');
        AppendGListByte(&data_00583548, count + '0');
    }
    while (0 <= --count) {
        part = parts[count];
        length = strlen(part);
        sprintf(lengthBuffer, "%d", length);
        CompilerTools_AppendGListString(&data_00583548, lengthBuffer);
        CompilerTools_AppendGListString(&data_00583548, part);
    }
}

void mangle_type(Type *type, UInt32 flags)
{
    char arraySize[16];
    char nameLength[16];

    switch ((SInt8)type->type) {
        case TYPEVOID:
            MangleQualifiers(flags);
            AppendGListByte(&data_00583548, 'v');
            return;
        case TYPEINT:
        case TYPEFLOAT:
            MangleQualifiers(flags);
            switch (TYPE_INTEGRAL(type)->integral) {
                case IT_BOOL:
                    AppendGListByte(&data_00583548, 'b');
                    return;
                case IT_CHAR:
                    AppendGListByte(&data_00583548, 'c');
                    return;
                case IT_WCHAR_T:
                    AppendGListByte(&data_00583548, 'w');
                    return;
                case IT_UCHAR:
                    CompilerTools_AppendGListString(&data_00583548, "Uc");
                    return;
                case IT_SCHAR:
                    CompilerTools_AppendGListString(&data_00583548, "Sc");
                    return;
                case IT_SHORT:
                    AppendGListByte(&data_00583548, 's');
                    return;
                case IT_USHORT:
                    CompilerTools_AppendGListString(&data_00583548, "Us");
                    return;
                case IT_INT:
                    AppendGListByte(&data_00583548, 'i');
                    return;
                case IT_UINT:
                    CompilerTools_AppendGListString(&data_00583548, "Ui");
                    return;
                case IT_LONG:
                    AppendGListByte(&data_00583548, 'l');
                    return;
                case IT_ULONG:
                    CompilerTools_AppendGListString(&data_00583548, "Ul");
                    return;
                case IT_LONGLONG:
                    AppendGListByte(&data_00583548, 'x');
                    return;
                case IT_ULONGLONG:
                    CompilerTools_AppendGListString(&data_00583548, "Ux");
                    return;
                case IT_FLOAT:
                    AppendGListByte(&data_00583548, 'f');
                    return;
                case IT_SHORTDOUBLE:
                    AppendGListByte(&data_00583548, 'D');
                    return;
                case IT_DOUBLE:
                    AppendGListByte(&data_00583548, 'd');
                    return;
                case IT_LONGDOUBLE:
                    AppendGListByte(&data_00583548, 'r');
                    return;
                default:
                    CError_FATAL(544);
            }
        case TYPEENUM:
            MangleQualifiers(flags);
            if (TYPE_ENUM(type)->enumname == NULL)
                mangle_qualified_name(TYPE_ENUM(type)->nspace, "enum");
            else
                mangle_qualified_name(TYPE_ENUM(type)->nspace, TYPE_ENUM(type)->enumname->name);
            return;
        case TYPEPOINTER:
            MangleQualifiers(TYPE_POINTER(type)->qual);
            if (TYPE_POINTER(type)->qual & Q_REFERENCE)
                AppendGListByte(&data_00583548, 'R');
            else
                AppendGListByte(&data_00583548, 'P');
            mangle_type(TYPE_POINTER(type)->target, flags);
            return;
        case TYPEMEMBERPOINTER: {
            TypeClass *memberClass;
            if (TYPE_MEMBER_POINTER(type)->ty2->type != TYPECLASS) {
                CompilerTools_AppendGListString(&data_00583548, "3<T>");
                return;
            }
            MangleQualifiers(TYPE_MEMBER_POINTER(type)->qual);
            AppendGListByte(&data_00583548, 'M');
            memberClass = TYPE_CLASS(TYPE_MEMBER_POINTER(type)->ty2);
            if (memberClass->classname == NULL)
                mangle_qualified_name(memberClass->nspace->parent, "class");
            else
                mangle_qualified_name(memberClass->nspace->parent, memberClass->nspace->name->name);
            mangle_type(TYPE_MEMBER_POINTER(type)->ty1, flags);
            return;
        }
        case TYPEARRAY:
            AppendGListByte(&data_00583548, 'A');
            if (ElemSize(TYPE_POINTER(type)) != 0) {
                sprintf(arraySize, "%ld", type->size / ElemSize(TYPE_POINTER(type)));
                CompilerTools_AppendGListString(&data_00583548, arraySize);
            } else {
                AppendGListByte(&data_00583548, '0');
            }
            AppendGListByte(&data_00583548, '_');
            mangle_type(TPTR_TARGET(type), flags);
            return;
        case TYPEFUNC:
            MangleQualifiers(flags);
            AppendGListByte(&data_00583548, 'F');
            mangle_args(TYPE_FUNC(type)->args);
            AppendGListByte(&data_00583548, '_');
            mangle_type(TYPE_FUNC(type)->functype, TYPE_FUNC(type)->qual);
            return;
        case TYPESTRUCT: {
            TypeStruct *structType = TYPE_STRUCT(type);
            MangleQualifiers(flags);
            switch (structType->stype) {
                case 4:
                    CompilerTools_AppendGListString(&data_00583548, "XUc");
                    return;
                case 5:
                    CompilerTools_AppendGListString(&data_00583548, "Xc");
                    return;
                case 6:
                    CompilerTools_AppendGListString(&data_00583548, "XC");
                    return;
                case 7:
                    CompilerTools_AppendGListString(&data_00583548, "XUs");
                    return;
                case 8:
                    CompilerTools_AppendGListString(&data_00583548, "Xs");
                    return;
                case 9:
                    CompilerTools_AppendGListString(&data_00583548, "XS");
                    return;
                case 10:
                    CompilerTools_AppendGListString(&data_00583548, "XUi");
                    return;
                case 11:
                    CompilerTools_AppendGListString(&data_00583548, "Xi");
                    return;
                case 12:
                    CompilerTools_AppendGListString(&data_00583548, "XI");
                    return;
                case 13:
                    CompilerTools_AppendGListString(&data_00583548, "Xf");
                    return;
                case 14:
                    CompilerTools_AppendGListString(&data_00583548, "Xp");
                    return;
            }
            if (structType->name != NULL && !CParser_IsNullOrAtOrDollarPrefixedName(structType->name)) {
                char *name = structType->name->name;
                sprintf(nameLength, "%d", strlen(name));
                CompilerTools_AppendGListString(&data_00583548, nameLength);
                CompilerTools_AppendGListString(&data_00583548, name);
            } else {
                switch (structType->stype) {
                    case 0:
                        CompilerTools_AppendGListString(&data_00583548, "struct");
                        return;
                    case 1:
                        CompilerTools_AppendGListString(&data_00583548, "union");
                        return;
                    case 2:
                        CompilerTools_AppendGListString(&data_00583548, "class");
                        return;
                    default:
                        CError_FATAL(626);
                }
            }
            return;
        }
        case TYPECLASS:
            MangleQualifiers(flags);
            if (TYPE_CLASS(type)->classname == NULL)
                mangle_qualified_name(TYPE_CLASS(type)->nspace->parent, "class");
            else
                mangle_qualified_name(TYPE_CLASS(type)->nspace->parent, TYPE_CLASS(type)->nspace->name->name);
            return;
        case TYPETEMPLATE:
            CompilerTools_AppendGListString(&data_00583548, "1T");
            return;
        default:
            CError_FATAL(641);
            return;
    }
}

static SInt32 ElemSize(TypePointer *t)
{
    return t->target->size;
}

static void MangleQualifiers(UInt32 q)
{
    if (q & Q_CONST)
        AppendGListByte(&data_00583548, 'C');
    if (q & Q_VOLATILE)
        AppendGListByte(&data_00583548, 'V');
}

void fn_004c2ac0(Type *type, SInt32 flag)
{
    data_00583548.size = 0;
    mangle_type(type, flag);
}

void mangle_args(FuncArg *args)
{
    TypePointer tptr;

    if (args != NULL) {
        if (args->type != NULL) {
            while (args != NULL) {
                if (args != &data_00583098 && args != &data_00584748) {
                    if (args->type->type == TYPEPOINTER) {
                        tptr = *TYPE_POINTER(args->type);
                        tptr.qual &= ~Q_CV;
                        mangle_type(TYPE(&tptr), args->qual);
                    } else {
                        mangle_type(args->type, 0);
                    }
                } else {
                    AppendGListByte(&data_00583548, 0x65);
                }
                args = args->next;
            }
        } else {
            AppendGListByte(&data_00583548, 0x65);
        }
    } else {
        AppendGListByte(&data_00583548, 0x76);
    }
}

void mangle_function_name(HashNameNode *name, NameSpace *chain, Type *func)
{
    TypeClass *cls;
    FuncArg *arg;
    UInt32 qual;

    CompilerTools_AppendGListString(&data_00583548, name->name);
    CompilerTools_AppendGListString(&data_00583548, "__");

    while (chain != NULL && chain->name == NULL)
        chain = chain->parent;

    if (chain != NULL) {
        mangle_qualified_name(chain->parent, chain->name->name);
        if ((cls = chain->theclass) != NULL) {
            if (name == destructor_name) {
                CompilerTools_AppendGListString(&data_00583548, "Fv");
                return;
            }

            if ((arg = TYPE_FUNC(func)->args) != NULL) {
                if (name == constructor_name) {
                    arg = arg->next;
                    if (arg != NULL && (cls->flags & CLASS_HAS_VBASES) != 0)
                        arg = arg->next;
                } else if ((TYPE_FUNC(func)->flags & FUNC_METHOD) != 0 && !TYPE_METHOD(func)->is_static) {
                    qual = arg->qual;
                    if (qual & Q_CONST)
                        AppendGListByte(&data_00583548, 'C');
                    if (qual & Q_VOLATILE)
                        AppendGListByte(&data_00583548, 'V');
                    arg = arg->next;
                }
                AppendGListByte(&data_00583548, 'F');
                mangle_args(arg);
                return;
            }
        }
    }

    AppendGListByte(&data_00583548, 'F');
    mangle_args(TYPE_FUNC(func)->args);
}

HashNameNode *CMangler_ConversionFuncName(Type *type, UInt32 qual)
{
    char **buffer;
    HashNameNode *name;
    data_00583548.size = 0;
    CompilerTools_AppendGListString(&data_00583548, "__op");
    mangle_type(type, qual);
    AppendGListByte(&data_00583548, 0);
    COS_LockHandle(data_00583548.data);
    buffer = data_00583548.data;
    name = GetHashNameNode(*buffer);
    COS_UnlockHandle(data_00583548.data);
    return name;
}

HashNameNode *CMangler_GetLinkName(Object *obj)
{
    NameSpace *nspace;
    HashNameNode *result;

    nspace = obj->nspace;
    while (nspace != NULL && nspace->name == NULL)
        nspace = nspace->parent;
    data_00583548.size = 0;
    if (is_pascal_object(obj) && (nspace == NULL || nspace->theclass == NULL))
        return obj->name;
    if ((obj->qual & Q_MANGLE_NAME) != 0 && (memcmp("main", obj->name->name, 5) != 0 || obj->nspace != cscope_root)) {
        mangle_function_name(obj->name, nspace, obj->type);
        AppendGListByte(&data_00583548, 0);
    } else {
        return obj->name;
    }
    COS_LockHandle(data_00583548.data);
    result = GetHashNameNode(*data_00583548.data);
    COS_UnlockHandle(data_00583548.data);
    return (HashNameNode *)result;
}

HashNameNode *CMangler_GetCovariantFunctionName(Object *object, Type *type)
{
    Object *target = object;
    unsigned char datatype;
    char **buffer;
    HashNameNode *result;
    while ((datatype = target->datatype) == DALIAS)
        target = target->u.alias.object;
    switch (datatype) {
        case DFUNC:
        case DVFUNC:
            if (target->u.func.linkname == NULL)
                target->u.func.linkname = CMangler_GetLinkName(target);
            result = target->u.func.linkname;
            break;
        case DDATA:
            if (target->u.data.linkname == NULL)
                target->u.data.linkname = get_object_link_name(target);
            result = target->u.data.linkname;
            break;
        case DINLINEFUNC:
            result = CMangler_GetLinkName(target);
            break;
        case DLOCAL:
        case DABSOLUTE:
        case DUNUSED:
            result = target->name;
            break;
        default:
            CError_FATAL(1012);
            result = NULL;
            break;
    }
    data_00583548.size = 0;
    appendMangledName(result->name);
    appendMangledName((unsigned char *)"@@");
    mangle_type(type, 0);
    AppendGListByte(&data_00583548, 0);
    COS_LockHandle(data_00583548.data);
    buffer = data_00583548.data;
    result = GetHashNameNode(*buffer);
    COS_UnlockHandle(data_00583548.data);
    return result;
}

HashNameNode *get_object_link_name(Object *object)
{
    NameSpace *scope = object->nspace;
    char **buffer;
    HashNameNode *result;

    while (scope && !scope->name)
        scope = scope->parent;
    data_00583548.size = 0;
    if (!scope)
        return object->name;

    CompilerTools_AppendGListData(&data_00583548, "", 0);
    appendObjectName(object->name->name);
    while (scope && !scope->name)
        scope = scope->parent;
    if (scope && (object->qual & Q_MANGLE_NAME)) {
        CompilerTools_AppendGListString(&data_00583548, "__");
        mangle_qualified_name(scope->parent, scope->name->name);
    }
    AppendGListByte(&data_00583548, 0);
    COS_LockHandle(data_00583548.data);
    buffer = data_00583548.data;
    result = GetHashNameNode(*buffer);
    COS_UnlockHandle(data_00583548.data);
    return result;
}

HashNameNode *COptimizer_GetFunctionObject(Object *obj)
{
    while (obj->datatype == DALIAS)
        obj = obj->u.alias.object;
    switch (obj->datatype) {
        case DFUNC:
        case DVFUNC:
            if (obj->u.func.linkname == NULL)
                obj->u.func.linkname = CMangler_GetLinkName(obj);
            return obj->u.func.linkname;
        case DDATA:
            if (obj->u.data.linkname == NULL)
                obj->u.data.linkname = get_object_link_name(obj);
            return obj->u.data.linkname;
        case DINLINEFUNC:
            return CMangler_GetLinkName(obj);
        case DLOCAL:
        case DABSOLUTE:
        case DUNUSED:
            return obj->name;
        default:
            CError_FATAL(1012);
            return NULL;
    }
}
