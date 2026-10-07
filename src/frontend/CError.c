#define CERROR_FILE "CError.c"
#include "compiler/common.h"
#include "compiler/CError.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
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
#include "compiler/FuncLevelAsmPPC.h"
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
#include "compiler/Switch.h"
#include "compiler/Unmangle.h"
#include "driver/COSToolsCLT.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

typedef char *va_list;

#define va_start(ap, last)                                                                                             \
    ((ap) = (char *)((int)((char *)&(last)) + (((int)((char *)(&(last) + 1)) - (int)((char *)&(last)) + 3) / 4 * 4)))
#define va_arg(ap, type) (*(type *)(((ap) += 4) - 4))

#pragma options align = mac68k
static int buffered_token;
static short data_005805ec;
static int data_005805ee;
#pragma options align = reset

static void CError_BufferGrow(StrBuf *eb, UInt32 amount);

/* The same buffer appended to without a null check, as some callers do for their own local buffers. */
static void CErrBuf_Putc(StrBuf *b, int c)
{
    if (b->avail == 0)
        CError_BufferGrow(b, 256);
    *b->cursor++ = (char)c;
    b->avail--;
}

static void CErrBuf_Append(StrBuf *b, const char *s)
{
    UInt32 n = strlen(s);
    if (b->avail < n)
        CError_BufferGrow(b, n);
    memcpy(b->cursor, s, n);
    b->cursor += n;
    b->avail -= n;
}

/* 0x403c50, sprintf-like */
/* 0x404c30, memcpy */

static void CError_BufferAppendChar(StrBuf *eb, char ch)
{
    if (eb) {
        if (!eb->avail)
            CError_BufferGrow(eb, 256);
        *eb->cursor++ = ch;
        eb->avail--;
    }
}

static void CError_BufferInit(StrBuf *eb, char *buf, SInt32 bufSize)
{
    eb->start = eb->cursor = buf;
    eb->size = eb->avail = bufSize - 1;
}

static inline void CError_GetErrorMessage(char *message, short errorCode)
{
    char format[128];

    if (errorCode < 100 || errorCode >= 372) {
        CompilerGetCString(5, format);
        sprintf(error_message_buffer, format, "CError.c", 138);
        report_diagnostic(10001, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        data_0058715c++;
    }
    CompilerTools_GetResourceCString(message, 10000, errorCode - 99);
}

static inline int CError_GetResourceIndex(SInt16 errorNumber)
{
    char internalErrorFormat[128];

    if (errorNumber < 100 || errorNumber >= 372) {
        CompilerGetCString(5, internalErrorFormat);
        sprintf(error_message_buffer, internalErrorFormat, "CError.c", 138);
        report_diagnostic(10001, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        data_0058715c++;
    }
    return errorNumber - 99;
}

struct IRONode *CError_NewIRONode(void)
{
    IRONode *record;
    UInt16 number;
    record = (IRONode *)CompilerTools_AllocatePoolMemory(60U);
    memset(record, 0, 60U);
    number = iro_node_count;
    record->index = number;
    iro_node_count += 1U;
    record->nextnode = 0U;
    return record;
}

void fn_00449dc0(void)
{
    data_005805ec = 0;
    data_005805ee = -1;
    buffered_token = 0;
    writtenEntry = 0;
    return;
}

void CError_SetBufferedToken(TStreamElement *entry)
{
    if (entry) {
        if (entry->tokenfile) {
            buffered_token = (int)entry;
        }
    }
}

void CError_SaveAndSetWrittenEntry(TStreamElement *entry, int *savedEntry)
{
    *savedEntry = writtenEntry;
    if (entry != NULL && entry->tokenfile != NULL) {
        writtenEntry = (int)entry;
    }
}

void CError_SetWrittenEntry(int *entry)
{
    writtenEntry = *entry;
}

void fn_00449d60(void)
{
    data_005805ee = 4294967295U;
    return;
}

/* 0x404c30, memcpy */
/* 0x441fa0, lalloc */

static void CError_BufferGrow(StrBuf *eb, UInt32 amount)
{
    char *newBuf;

    newBuf = (char *)CompilerTools_AllocatePool(eb->size + amount);
    memcpy(newBuf, eb->start, eb->size);
    eb->start = newBuf;
    eb->cursor = newBuf + eb->size - eb->avail;
    eb->size += amount;
    eb->avail += amount;
}

void CError_BufferAppendString(StrBuf *eb, const char *str)
{
    UInt32 len;

    if (eb) {
        len = strlen(str);
        if (eb->avail < len)
            CError_BufferGrow(eb, len);
        memcpy(eb->cursor, str, len);
        eb->cursor += len;
        eb->avail -= len;
    }
}

void append_qualifiers(StrBuf *buf, UInt32 qual)
{
    if (qual & Q_PASCAL)
        CError_BufferAppendString(buf, "pascal ");
    if (qual & Q_CONST)
        CError_BufferAppendString(buf, "const ");
    if (qual & Q_VOLATILE)
        CError_BufferAppendString(buf, "volatile ");
    if (qual & Q_EXPLICIT)
        CError_BufferAppendString(buf, "explicit ");
    if (qual & Q_RESTRICT)
        CError_BufferAppendString(buf, "restrict ");
}

void append_targ_expr(StrBuf *buf, ENode *node)
{
    if (node != NULL) {
        switch (node->type) {
            case EINTCONST: {
                char tmp[32];
                CExpr2_FormatCInt64Decimal(tmp, node->data.intval);
                CError_BufferAppendString(buf, tmp);
                return;
            }
            case EOBJREF:
                CError_BufferAppendChar(buf, '&');
                append_object_name(buf, node->data.objref);
                return;
        }
    }
    CError_BufferAppendString(buf, "{targ_expr}");
}

void append_ctstate_argument(StrBuf *context, TemplArg *record)
{
    if (!record->pid.type)
        append_targ_expr(context, record->data.paramdecl.expr);
    else
        append_type(context, record->data.typeparam.type, record->data.typeparam.qual);
}

void append_ctstate_list(StrBuf *buf, TemplArg *node)
{
    if (node == NULL)
        return;
    CError_BufferAppendChar(buf, '<');
    while (node != NULL) {
        append_ctstate_argument(buf, node);
        if (node->next)
            CError_BufferAppendString(buf, ", ");
        node = node->next;
    }
    CError_BufferAppendChar(buf, '>');
}

void append_namespace_qualification(StrBuf *buf, NameSpace *ns)
{
    for (; ns; ns = ns->parent) {
        if (ns->name) {
            append_namespace_qualification(buf, ns->parent);
            if (ns->theclass) {
                CError_BufferAppendString(buf, ns->theclass->classname->name);
                if (ns->theclass->flags & CLASS_IS_TEMPL_INST)
                    append_ctstate_list(buf, ((TemplClassInst *)ns->theclass)->inst_args);
            } else
                CError_BufferAppendString(buf, ns->name->name);
            CError_BufferAppendString(buf, "::");
            return;
        }
    }
}

#pragma inline_depth(4)

void append_pointer_declarator(StrBuf *buf, Type *type)
{
    switch ((signed char)type->type) {
        case TYPEPOINTER:
            append_pointer_declarator(buf, TYPE_POINTER(type)->target);
            if (TYPE_POINTER(type)->qual & Q_REFERENCE)
                CError_BufferAppendString(buf, "&");
            else
                CError_BufferAppendString(buf, "*");
            append_qualifiers(buf, TYPE_POINTER(type)->qual);
            return;
        case TYPEMEMBERPOINTER:
            append_pointer_declarator(buf, TYPE_MEMBER_POINTER(type)->ty1);
            append_type(buf, TYPE_MEMBER_POINTER(type)->ty2, 0);
            CError_BufferAppendString(buf, "::*");
            append_qualifiers(buf, TYPE_MEMBER_POINTER(type)->qual);
            return;
    }
}

void append_templdep(StrBuf *buf, TypeTemplDep *node)
{
    char tmp[64];
    char msg[128];

    switch (node->dtype) {
        case 0:
            if (node->u.pid.nindex)
                sprintf(tmp, "T%ld_%ld", node->u.pid.nindex, node->u.pid.index);
            else
                sprintf(tmp, "T%ld", node->u.pid.index);
            CError_BufferAppendString(buf, tmp);
            break;
        case 1:
            append_templdep(buf, node->u.qual.type);
            CError_BufferAppendString(buf, "::");
            CError_BufferAppendString(buf, node->u.qual.name->name);
            break;
        case 2:
            append_type(buf, (Type *)node->u.templ.templ, 0);
            append_ctstate_list(buf, node->u.templ.args);
            break;
        case 3:
            append_type(buf, node->u.array.type, 0);
            CError_BufferAppendChar(buf, '[');
            append_targ_expr(buf, node->u.array.index);
            CError_BufferAppendChar(buf, ']');
            break;
        case 4:
            append_templdep(buf, node->u.qual.type);
            CError_BufferAppendString(buf, "::");
            append_ctstate_list(buf, node->u.qualtempl.args);
            break;
        case 5:
            append_type(buf, node->u.bitfield.type, 0);
            CError_BufferAppendChar(buf, '[');
            append_targ_expr(buf, node->u.bitfield.size);
            CError_BufferAppendChar(buf, ']');
            break;
        default:
            CompilerGetCString(5, msg);
            sprintf(error_message_buffer, msg, "CError.c", 0x1b5);
            report_diagnostic(0x2711, error_message_buffer, 1, 0);
            longjmp(error_jmp_buf, 1);
            data_0058715c++;
            break;
    }
}

void append_function_args(StrBuf *buf, TypeMemberFunc *type, char skip)
{
    FuncArg *arg;
    UInt32 qual;
    UInt32 flags;
    qual = 0;
    CError_BufferAppendChar(buf, '(');
    if ((arg = type->args) != NULL) {
        if (skip) {
            arg = arg->next;
            if (arg)
                arg = arg->next;
        } else {
            if (((flags = type->flags) & 0x10) && !type->is_static) {
                qual = arg->qual;
                arg = arg->next;
                if (arg && ((flags & 0x4000) || ((flags & 0x2000) && (type->theclass->flags & CLASS_HAS_VBASES))))
                    arg = arg->next;
            }
        }
        for (; arg;) {
            if (arg == &data_00583098 || arg == &data_00584748) {
                CError_BufferAppendString(buf, "...");
                break;
            }
            append_type(buf, arg->type, arg->qual);
            arg = arg->next;
            if (arg == NULL)
                break;
            CError_BufferAppendString(buf, ", ");
        }
    }
    CError_BufferAppendChar(buf, ')');
    if (qual) {
        append_qualifiers(buf, qual);
    }
}

void append_type(StrBuf *buf, Type *type, UInt32 qualifiers)
{
    TemplArg *templateArgs;
    Type *arrayType;
    Type *baseType;
    TypeMemberFunc *funcType;
    TypeTemplDep *dependentType;
    char countText[16];
    char integralError[128];
    char structError[128];
    char typeError[128];
    SInt8 typeKind;

    switch ((signed char)type->type) {
        case TYPEVOID:
            append_qualifiers(buf, qualifiers);
            CError_BufferAppendString(buf, "void");
            return;
        case TYPEINT:
        case TYPEFLOAT:
            append_qualifiers(buf, qualifiers);
            switch (TYPE_INTEGRAL(type)->integral) {
                case IT_BOOL:
                    CError_BufferAppendString(buf, "bool");
                    return;
                case IT_CHAR:
                    CError_BufferAppendString(buf, "char");
                    return;
                case IT_UCHAR:
                    CError_BufferAppendString(buf, "unsigned char");
                    return;
                case IT_SCHAR:
                    CError_BufferAppendString(buf, "signed char");
                    return;
                case IT_WCHAR_T:
                    CError_BufferAppendString(buf, "wchar_t");
                    return;
                case IT_SHORT:
                    CError_BufferAppendString(buf, "short");
                    return;
                case IT_USHORT:
                    CError_BufferAppendString(buf, "unsigned short");
                    return;
                case IT_INT:
                    CError_BufferAppendString(buf, "int");
                    return;
                case IT_UINT:
                    CError_BufferAppendString(buf, "unsigned int");
                    return;
                case IT_LONG:
                    CError_BufferAppendString(buf, "long");
                    return;
                case IT_ULONG:
                    CError_BufferAppendString(buf, "unsigned long");
                    return;
                case IT_LONGLONG:
                    CError_BufferAppendString(buf, "long long");
                    return;
                case IT_ULONGLONG:
                    CError_BufferAppendString(buf, "unsigned long long");
                    return;
                case IT_FLOAT:
                    CError_BufferAppendString(buf, "float");
                    return;
                case IT_SHORTDOUBLE:
                    CError_BufferAppendString(buf, "short double");
                    return;
                case IT_DOUBLE:
                    CError_BufferAppendString(buf, "double");
                    return;
                case IT_LONGDOUBLE:
                    CError_BufferAppendString(buf, "long double");
                    return;
                default:
                    CompilerGetCString(5, integralError);
                    sprintf(error_message_buffer, integralError, "CError.c", 0x22d);
                    report_diagnostic(0x2711, error_message_buffer, 1, 0);
                    longjmp(error_jmp_buf, 1);
                    data_0058715c = data_0058715c + 1;
            }
        case TYPEENUM:
            append_qualifiers(buf, qualifiers);
            append_namespace_qualification(buf, TYPE_ENUM(type)->nspace);
            if (TYPE_ENUM(type)->enumname != NULL)
                CError_BufferAppendString(buf, TYPE_ENUM(type)->enumname->name);
            else
                CError_BufferAppendString(buf, "{unnamed-enum}");
            return;
        case TYPESTRUCT:
            append_qualifiers(buf, qualifiers);
            switch (TYPE_STRUCT(type)->stype) {
                case 0:
                    CError_BufferAppendString(buf, "struct ");
                    break;
                case 1:
                    CError_BufferAppendString(buf, "union ");
                    break;
                default:
                    CompilerGetCString(5, structError);
                    sprintf(error_message_buffer, structError, "CError.c", 0x24f);
                    report_diagnostic(0x2711, error_message_buffer, 1, 0);
                    longjmp(error_jmp_buf, 1);
                    data_0058715c = data_0058715c + 1;
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                    break;
            }
            if (TYPE_STRUCT(type)->name != NULL)
                CError_BufferAppendString(buf, TYPE_STRUCT(type)->name->name);
            return;
        case TYPECLASS:
            append_qualifiers(buf, qualifiers);
            append_namespace_qualification(buf, TYPE_CLASS(type)->nspace->parent);
            if (TYPE_CLASS(type)->classname != NULL) {
                CError_BufferAppendString(buf, TYPE_CLASS(type)->classname->name);
                if ((TYPE_CLASS(type)->flags & CLASS_IS_TEMPL_INST) != 0) {
                    if ((templateArgs = ((TemplClassInst *)type)->oargs) == NULL)
                        templateArgs = ((TemplClassInst *)type)->inst_args;
                    append_ctstate_list(buf, templateArgs);
                }
            } else {
                CError_BufferAppendString(buf, "{unnamed-class}");
            }
            return;
        case TYPEMEMBERPOINTER:
        case TYPEPOINTER:
            do {
                baseType = type;
                for (;;) {
                    SInt8 baseKind = baseType->type;
                    switch (baseKind) {
                        case TYPEPOINTER:
                            baseType = TYPE_POINTER(baseType)->target;
                            continue;
                        case TYPEMEMBERPOINTER:
                            baseType = TYPE_MEMBER_POINTER(baseType)->ty1;
                            continue;
                    }
                    break;
                }
                append_qualifiers(buf, qualifiers);
                typeKind = baseType->type;
                switch (typeKind) {
                    case TYPEFUNC:
                        if ((TYPE_FUNC(baseType)->flags & FUNC_PASCAL) != 0)
                            CError_BufferAppendString(buf, "pascal ");
                        if ((TYPE_FUNC(baseType)->flags & 0x1000000) != 0)
                            CError_BufferAppendString(buf, "__mpwc ");
                        append_type(buf, TYPE_FUNC(baseType)->functype, 0);
                        CError_BufferAppendString(buf, " (");
                        append_pointer_declarator(buf, type);
                        CError_BufferAppendChar(buf, ')');
                        funcType = TYPE_METHOD(baseType);
                        append_function_args(buf, funcType, type->type == TYPEMEMBERPOINTER);
                        return;
                    case TYPEARRAY:
                        arrayType = baseType;
                        while (baseType->type == TYPEARRAY)
                            baseType = TYPE_POINTER(baseType)->target;
                        append_type(buf, baseType, 0);
                        CError_BufferAppendString(buf, " (");
                        append_pointer_declarator(buf, type);
                        CError_BufferAppendChar(buf, ')');
                        type = arrayType;
                        break;
                    default:
                        append_type(buf, baseType, 0);
                        CError_BufferAppendChar(buf, ' ');
                        append_pointer_declarator(buf, type);
                        return;
                }
                break;
                case TYPEFUNC:
                    if ((TYPE_FUNC(type)->flags & FUNC_PASCAL) != 0)
                        CError_BufferAppendString(buf, "pascal ");
                    if ((TYPE_FUNC(type)->flags & 0x1000000) != 0)
                        CError_BufferAppendString(buf, "__mpwc ");
                    append_qualifiers(buf, qualifiers);
                    append_type(buf, TYPE_FUNC(type)->functype, 0);
                    CError_BufferAppendChar(buf, ' ');
                    funcType = TYPE_METHOD(type);
                    append_function_args(buf, funcType, 0);
                    return;
                case TYPEARRAY:
                    append_qualifiers(buf, qualifiers);
                    for (arrayType = type; arrayType->type == TYPEARRAY; arrayType = TYPE_POINTER(arrayType)->target) {
                    }
                    append_type(buf, arrayType, 0);
            } while (0);
            while (type->type == TYPEARRAY) {
                CError_BufferAppendChar(buf, '[');
                if (type->size != 0 && TYPE_POINTER(type)->target->size != 0) {
                    sprintf(countText, "%ld", type->size / TYPE_POINTER(type)->target->size);
                    CError_BufferAppendString(buf, countText);
                }
                CError_BufferAppendChar(buf, ']');
                type = TYPE_POINTER(type)->target;
            }
            return;
        case TYPETEMPLATE:
            append_qualifiers(buf, qualifiers);
            dependentType = (TypeTemplDep *)type;
            append_templdep(buf, dependentType);
            return;
        case TYPETEMPLDEPEXPR:
            CError_BufferAppendString(buf, "T");
            return;
        case TYPEBITFIELD:
            sprintf(countText, "bitfield:%ld", TYPE_BITFIELD(type)->bitlength);
            CError_BufferAppendString(buf, countText);
            return;
        default:
            CompilerGetCString(5, typeError);
            sprintf(error_message_buffer, typeError, "CError.c", 0x2d0);
            report_diagnostic(0x2711, error_message_buffer, 1, 0);
            longjmp(error_jmp_buf, 1);
            data_0058715c = data_0058715c + 1;
            return;
    }
}

char *CError_GetTypeString(Type *type, int qualifiers, char useAlternateAllocator)
{
    StrBuf ctx;
    char buffer[256];
    char *text;

    ctx.cursor = buffer;
    ctx.start = ctx.cursor;
    ctx.avail = 0xff;
    ctx.size = ctx.avail;
    append_type(&ctx, type, qualifiers);
    *ctx.cursor = '\0';
    ctx.avail = 0;
    if (useAlternateAllocator) {
        text = galloc(ctx.size + 1);
    } else {
        text = (char *)CompilerTools_AllocatePool(ctx.size + 1);
    }
    return (char *)strcpy(text, ctx.start);
}

void append_function_name(StrBuf *buf, NameSpace *ns, HashNameNode *name, Type *type)
{
    char *n;
    Boolean done;

    done = 0;
    if (ns != NULL && ns->theclass != NULL) {
        if (name == constructor_name) {
            CError_BufferAppendString(buf, ns->theclass->classname->name);
            done = 1;
        } else if (name == destructor_name) {
            CError_BufferAppendChar(buf, '~');
            CError_BufferAppendString(buf, ns->theclass->classname->name);
            done = 1;
        }
    }
    if (!done) {
        n = CMangler_GetOperator(name);
        if (n == NULL) {
            if (type != NULL && (TYPE_FUNC(type)->flags & FUNC_CONVERSION) != 0) {
                CError_BufferAppendString(buf, "operator ");
                append_type(buf, TYPE_FUNC(type)->functype, TYPE_FUNC(type)->qual);
            } else {
                CError_BufferAppendString(buf, name->name);
            }
        } else {
            CError_BufferAppendString(buf, n);
        }
    }
}

void append_qualified_function_signature(StrBuf *buf, NameSpace *ns, HashNameNode *name, Type *type)
{
    TypeMemberFunc *funcType;
    append_namespace_qualification(buf, ns);
    append_function_name(buf, ns, name, type);
    if (type) {
        funcType = TYPE_METHOD(type);
        append_function_args(buf, funcType, 0);
    } else
        CError_BufferAppendString(buf, "()");
}

void append_object_name(StrBuf *sb, Object *obj)
{
    if (TYPEFUNC != obj->type->type) {
        char *s;
        unsigned long n;
        char *newbuf;
        append_namespace_qualification(sb, obj->nspace);
        s = obj->name->name;
        if (sb != NULL) {
            n = strlen(s);
            if (sb->avail < n) {
                newbuf = CompilerTools_AllocatePool(sb->size + n);
                memcpy(newbuf, sb->start, sb->size);
                sb->start = newbuf;
                sb->cursor = newbuf + sb->size - sb->avail;
                sb->size += n;
                sb->avail += n;
            }
            memcpy(sb->cursor, s, n);
            sb->cursor += n;
            sb->avail -= n;
        }
    } else {
        append_qualified_function_signature(sb, obj->nspace, obj->name, obj->type);
    }
}

void append_func_type_info(StrBuf *buf, MethRec *func)
{
    ObjCParameterNode *arg;
    CError_BufferAppendChar(buf, func->isinst ? '+' : '-');
    CError_BufferAppendChar(buf, '(');
    append_type(buf, func->rtype, func->rqual);
    CError_BufferAppendChar(buf, ')');
    for (arg = func->args; arg != NULL; arg = arg->next) {
        if (arg->selectorName != NULL)
            CError_BufferAppendString(buf, arg->selectorName->name);
        if (arg->type != NULL) {
            CError_BufferAppendString(buf, ":(");
            append_type(buf, arg->type, arg->qual);
            CError_BufferAppendChar(buf, ')');
        }
    }
    if (func->isvararg)
        CError_BufferAppendString(buf, ",...");
}

char *CError_GetQualifiedName(NameSpace *nameSpace, HashNameNode *name)
{
    StrBuf sb;
    char buf[256];
    UInt32 len;
    char *newdata;

    sb.start = sb.cursor = buf;
    sb.avail = sizeof(buf) - 1;
    sb.size = sb.avail;
    append_namespace_qualification(&sb, nameSpace);
    len = strlen(name->name);
    if (sb.avail < len) {
        newdata = (char *)CompilerTools_AllocatePool(sb.size + len);
        memcpy(newdata, sb.start, sb.size);
        sb.start = newdata;
        sb.cursor = newdata + sb.size - sb.avail;
        sb.size += len;
        sb.avail += len;
    }
    memcpy(sb.cursor, name->name, len);
    sb.cursor += len;
    sb.avail -= len;
    *sb.cursor = 0;
    sb.avail = 0;
    newdata = (char *)CompilerTools_AllocatePool(sb.size + 1);
    return strcpy(newdata, sb.start);
}

char *CError_BuildNameSpaceNameTypeString(NameSpace *nspace, HashNameNode *name, Type *x)
{
    StrBuf s;
    char buf[256];
    char *p;

    s.start = s.cursor = buf;
    s.avail = 255;
    s.size = s.avail;
    append_qualified_function_signature(&s, nspace, name, x);
    *s.cursor = 0;
    s.avail = 0;
    p = (char *)CompilerTools_AllocatePool(s.size + 1);
    return strcpy(p, s.start);
}

long CError_GetObjectString(Object *object)
{
    char *result;
    StrBuf buffer;
    char text[256];
    buffer.start = buffer.cursor = text;
    buffer.avail = 255;
    buffer.size = buffer.avail;
    append_object_name(&buffer, object);
    *buffer.cursor = 0;
    buffer.avail = 0;
    result = CompilerTools_AllocatePool(buffer.size + 1);
    return (long)strcpy(result, buffer.start);
}

char *CError_GetQualifiedHashName(NameSpace *nspace, HashNameNode *nameRef)
{
    StrBuf str;
    char strbuf[256];
    char msgbuf[128];
    char *name;

    if (nameRef == NULL) {
        CompilerGetCString(5, msgbuf);
        sprintf(error_message_buffer, msgbuf, "CError.c", 0x39c);
        report_diagnostic(0x2711, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        data_0058715c++;
    }
    name = CMangler_GetOperator(nameRef);
    if (name == NULL)
        name = nameRef->name;
    if (nspace != NULL && nspace->name != NULL) {
        str.start = str.cursor = strbuf;
        str.avail = sizeof(strbuf) - 1;
        str.size = str.avail;
        append_namespace_qualification(&str, nspace);
        CErrBuf_Append(&str, name);
        *str.cursor = 0;
        str.avail = 0;
        return (char *)strcpy((char *)CompilerTools_AllocatePool(str.size + 1), str.start);
    }
    return name;
}

#pragma opt_lifetimes off

void report_diagnostic(int message, char *argument, char force, char mode)
{
    int token;
    struct MessageContext *location;
    short errorType;
    struct MessageContext details;
    MessagePosition *position;
    char text[130];
    short value;

    if (data_005884fd == 0 && force == 0 && data_005805ee == line_count) {
        if (data_005805ec++ >= 50) {
            if (data_005805ec > 60)
                longjmp(error_jmp_buf, 1);
            tk = CPrepTokenizer_GetNextToken();
            data_005805ec = 0;
            if (tk == 0) {
                CompilerGetCString(1, text);
                CWPluginsPrivate_InvokeMessageCallback(((struct CPrepCU *)cprep_cu)->context, NULL, text, NULL, 2,
                                                       message);
                longjmp(error_jmp_buf, 1);
            }
        }
    } else {
        if (mode == 0) {
            data_005805ee = line_count;
            data_005805ec = 0;
        }
        if (copts.warningerrors != 0)
            mode = 0;

        if (buffered_token != 0)
            token = buffered_token;
        else if (writtenEntry != 0)
            token = writtenEntry;
        else
            token = 0;

        if (token == -1) {
            location = NULL;
        } else {
            CPrep_GetTokenLocation(token, &position, &details.auxiliaryD, &value, &details.auxiliaryA, text,
                                   &details.auxiliaryB, &details.auxiliaryC, data_005830c8, data_005883ec);
            details.value = value;
            details.position = *position;
            location = &details;
        }

        errorType = mode != 0 ? (unsigned char)1 : (unsigned char)2;
        if (mode == 0)
            func_errors = anyerrors = 1;
        if (CWPluginsPrivate_InvokeMessageCallback(((struct CPrepCU *)cprep_cu)->context, location, argument, text,
                                                   errorType, message) != 0)
            longjmp(error_jmp_buf, 1);
    }
    buffered_token = 0;
}

#pragma opt_lifetimes reset

/* memcpy */
/* lalloc */
/* 0x5519f4, the 17-byte string */
/* 0x551a08, the 2-byte string */

void append_instantiation_stack(StrBuf *buf)
{
    SInt32 j;
    SInt32 count;
    struct TemplStack *stack[64];

    {
        struct TemplStack *p = object_reference_stack;
        count = 0;
        while (p != NULL && count < 64) {
            stack[count] = p;
            count++;
            p = p->next;
        }
    }

    {
        int i;
        for (i = count - 1; i >= 0; i--) {
            CError_BufferAppendChar(buf, '\n');
            for (j = i; j < count; j++)
                CError_BufferAppendChar(buf, ' ');
            CError_BufferAppendString(buf, "(instantiating: '");
            if (stack[i]->is_func)
                append_object_name(buf, stack[i]->u.func);
            else
                append_type(buf, TYPE(stack[i]->u.theclass), 0);
            CError_BufferAppendString(buf, "')");
        }
    }
    *buf->cursor = 0;
    buf->avail = 0;
}

/* Formats an error message and reports it: %n a mangled name, %u a string, %o an object, %m a function, %t a type
   with its qualifiers, %i a number, %f a file, %% a percent sign. */
void CError_FormatAndReportDiagnostic(int errorCode, const char *format, char *args, int force, int mode)
{
    StrBuf eb;
    char buffer[256];
    char temp[256];
    SInt32 position;
    char err[128];
    char c;
    Type *type;
    UInt32 qualifiers;
    char *name;
    HashNameNode *file;
    CPrepFileInfo *pfile;
    const char *fmt;

    eb.cursor = buffer;
    eb.avail = 255;
    eb.start = eb.cursor;
    eb.size = eb.avail;
    fmt = format;
    for (;;) {
        switch (c = *fmt) {
            case '%':
                switch (fmt[1]) {
                    case 'n':
                        Unmangle_UnmangleSymbolName(va_arg(args, char *), temp, sizeof(temp));
                        CErrBuf_Append(&eb, temp);
                        fmt += 2;
                        continue;
                    case 'u':
                        name = va_arg(args, char *);
                        CError_BufferAppendString(&eb, name);
                        fmt += 2;
                        continue;
                    case 'o':
                        append_object_name(&eb, va_arg(args, Object *));
                        fmt += 2;
                        continue;
                    case 'm':
                        append_func_type_info(&eb, va_arg(args, MethRec *));
                        fmt += 2;
                        continue;
                    case 't':
                        type = va_arg(args, Type *);
                        qualifiers = va_arg(args, UInt32);
                        append_type(&eb, type, qualifiers);
                        fmt += 2;
                        continue;
                    case '%':
                        CError_BufferAppendChar(&eb, '%');
                        fmt += 2;
                        continue;
                    case 'i':
                        sprintf(temp, "%ld", va_arg(args, SInt32));
                        CErrBuf_Append(&eb, temp);
                        fmt += 2;
                        continue;
                    case 'f':
                        pfile = va_arg(args, CPrepFileInfo *);
                        file = fn_00441850(pfile, &position);
                        CError_BufferAppendString(&eb, file->name);
                        fmt += 2;
                        continue;
                    default:
                        CompilerGetCString(5, err);
                        sprintf(error_message_buffer, err, "CError.c", 1110);
                        report_diagnostic(10001, error_message_buffer, 1, 0);
                        longjmp(error_jmp_buf, 1);
                        data_0058715c++;
                        break;
                }
                break;
            default:
                fmt++;
                CErrBuf_Putc(&eb, c);
                continue;
            case 0:
                break;
        }
        break;
    }
    append_instantiation_stack(&eb);
    report_diagnostic(errorCode, eb.start, force, mode);
}

NameSpaceList *CError_ReportError(int errorNumber, ...)
{
    int diagnosticCode;
    char format[256];
    va_list args;

    if (data_00588240 != NULL)
        longjmp(data_00588240->jmpbuf, 1);
    va_start(args, errorNumber);
    diagnosticCode = errorNumber;
    CompilerTools_GetResourceCString(format, 10000, CError_GetResourceIndex(diagnosticCode));
    CError_FormatAndReportDiagnostic(diagnosticCode + 10000, format, args, 0, 0);
    if (data_005884fd != 0)
        InlineAsm_LongJump();
}

void CError_FatalError(short errorNumber)
{
    char message[128];

    if (+errorNumber < 100 || +errorNumber >= 372) {
        CompilerGetCString(5, message);
        sprintf(error_message_buffer, (char *)message, "CError.c", 138);
        report_diagnostic(10001, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        ++data_0058715c;
    }
    CompilerTools_GetResourceCString(error_message_buffer, 10000, errorNumber - 99);
    report_diagnostic(errorNumber + 10000, error_message_buffer, 0, 0);
    longjmp(error_jmp_buf, 1);
}

void CError_ReportErrorAndUpdateToken(int errorNumber, ...)
{
    char message[256];
    va_list args;
    int errorCode;

    if (data_00588240 != NULL)
        longjmp(data_00588240->jmpbuf, 1);
    va_start(args, errorNumber);
    errorCode = errorNumber;
    CError_GetErrorMessage(message, errorCode);
    CError_FormatAndReportDiagnostic(errorCode + 10000, message, args, 0, 0);
    if (tk != ';' && tk != ')' && tk != '}' && tk != ',' && tk != ']')
        tk = CPrepTokenizer_GetNextToken();
}

void CError_FunctionCallError(short errorCode, ObjectList *objects, ENodeList *arguments)
{
    StrBuf message;
    char messageBuffer[256];
    char resourceBuffer[128];
    char *cursor;
    int diagnosticCode;
    ENodeList *argument;

    if (data_00588240 != NULL)
        longjmp(data_00588240->jmpbuf, 1);

    if ((diagnosticCode = errorCode) < 100 || diagnosticCode >= 0x174) {
        CompilerGetCString(5, resourceBuffer);
        sprintf(error_message_buffer, resourceBuffer, "CError.c", 0x8a);
        report_diagnostic(0x2711, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        data_0058715c++;
    }
    CompilerTools_GetResourceCString(error_message_buffer, 10000, diagnosticCode - 99);

    CError_BufferInit(&message, messageBuffer, sizeof(messageBuffer));
    cursor = error_message_buffer;
    for (;; cursor++) {
        switch (*cursor) {
            case 0:
                break;
            case '*':
                if (objects->object->type->type == TYPEFUNC) {
                    append_function_name(&message, objects->object->nspace, objects->object->name,
                                         objects->object->type);
                    if ((TYPE_METHOD(objects->object->type)->flags & FUNC_METHOD) != 0 &&
                        (TYPE_METHOD(objects->object->type)->flags & FUNC_IS_DTOR) != 0 &&
                        (TYPE_CLASS(TYPE_METHOD(objects->object->type)->theclass)->flags & CLASS_HAS_VBASES) != 0 &&
                        arguments != NULL)
                        arguments = arguments->next;
                } else {
                    CError_BufferAppendString(&message, objects->object->name->name);
                }
                CError_BufferAppendChar(&message, '(');
                argument = arguments;
                while (argument) {
                    append_type(&message, argument->node->rtype, argument->node->flags & 3);
                    argument = argument->next;
                    if (!argument)
                        break;
                    CError_BufferAppendString(&message, ", ");
                }
                CError_BufferAppendChar(&message, ')');
                continue;
            default:
                CError_BufferAppendChar(&message, *cursor);
                continue;
        }
        break;
    }
    for (; objects != NULL; objects = objects->next) {
        if (objects->object->otype == OT_OBJECT) {
            CError_BufferAppendChar(&message, '\n');
            CError_BufferAppendChar(&message, '\'');
            append_object_name(&message, objects->object);
            CError_BufferAppendChar(&message, '\'');
        }
    }
    append_instantiation_stack(&message);
    diagnosticCode += 10000;
    report_diagnostic(diagnosticCode, message.start, 0, 0);
}

void CError_OverloadedFunctionError(Object *name, struct ObjectList *names)
{
    StrBuf message;
    char buffer[256];

    if (data_00588240 != NULL)
        longjmp(data_00588240->jmpbuf, 1);
    CompilerTools_GetResourceCString(error_message_buffer, 10000, 100);
    message.start = message.cursor = buffer;
    message.avail = 255;
    message.size = message.avail;
    CErrBuf_Append(&message, error_message_buffer);
    if (name != NULL) {
        CErrBuf_Putc(&message, '\n');
        CErrBuf_Putc(&message, '\'');
        append_object_name(&message, name);
        CErrBuf_Putc(&message, '\'');
    }
    while (names != NULL) {
        CErrBuf_Putc(&message, '\n');
        CErrBuf_Putc(&message, '\'');
        append_object_name(&message, names->object);
        CErrBuf_Putc(&message, '\'');
        names = names->next;
    }
    append_instantiation_stack(&message);
    report_diagnostic(0x27d7, message.start, 0, 0);
}

void CError_IllegalUseAbstractClass(TypeClass *type)
{
    char buf[128];
    Object *result;

    result = CClass_CheckPures(type);
    if (result == NULL) {
        CompilerGetCString(5, buf);
        sprintf(error_message_buffer, buf, "CError.c", 0x518);
        report_diagnostic(0x2711, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        data_0058715c++;
    }
    CError_ReportError(ERR_ILLEGAL_USE_ABSTRACT_CLASS, result);
}

void CError_Warning(SInt32 diagnosticCode, ...)
{
    char format[256];
    char message[128];
    va_list args;
    SInt32 diagnosticID;
    SInt16 errorCode;

    if (data_00588240)
        return;

    va_start(args, diagnosticCode);
    diagnosticID = diagnosticCode;

    if ((SInt16)diagnosticID < 100 || (SInt16)diagnosticID >= 372) {
        CompilerGetCString(5, message);
        sprintf(error_message_buffer, message, "CError.c", 138);
        report_diagnostic(0x2711, error_message_buffer, 1, 0);
        longjmp(error_jmp_buf, 1);
        data_0058715c++;
    }

    errorCode = diagnosticID;
    CompilerTools_GetResourceCString(format, 10000, errorCode - 99);
    CError_FormatAndReportDiagnostic(diagnosticID + 10000, format, args, 0, 1);
}

int CError_Internal(const char *file, int line)
{
    char message[128];
    CompilerGetCString(5, message);
    sprintf(error_message_buffer, message, file, line);
    report_diagnostic(0x2711, error_message_buffer, 1, 0);
    longjmp(error_jmp_buf, 1);
    ++data_0058715c;
}

void CError_LongJump(void)

{
    DAT_00588516 = 1;
    longjmp(error_jmp_buf, 1);
    return;
}

void CError_Longjmp(void)

{
    CompilerGetCString(8, error_message_buffer);
    longjmp(error_jmp_buf, 1);
    return;
}

void CError_DispatchAndLongJump(void)
{
    unsigned int lift_value_1;
    struct DispatchObject_0041b830 *lift_value_2;
    unsigned int lift_call_3;
    CompilerGetCString(9, error_message_buffer);
    lift_value_1 = ((unsigned int)cprep_cu);
    lift_value_2 = *(struct DispatchObject_0041b830 **)(lift_value_1);
    lift_call_3 = CWPluginsPrivate_InvokeMessageCallback(lift_value_2, 0U, error_message_buffer, 0U, 2U, 0U);
    longjmp(error_jmp_buf, 1);
}

void CError_ReportIllegalFlags(UInt32 flags)
{
    if (flags != 0) {
        Boolean found = 0;

        if (flags & 0x1) {
            CError_ReportError(ERR_ILLEGAL_USE, "const");
            found = 1;
        }
        if (flags & 0x2) {
            CError_ReportError(ERR_ILLEGAL_USE, "volatile");
            found = 1;
        }
        if (flags & 0x200000) {
            CError_ReportError(ERR_ILLEGAL_USE, "restrict");
            found = 1;
        }
        if (flags & 0x4) {
            CError_ReportError(ERR_ILLEGAL_USE, "asm");
            found = 1;
        }
        if (flags & 0x8) {
            CError_ReportError(ERR_ILLEGAL_USE, "pascal");
            found = 1;
        }
        if (flags & 0x10) {
            CError_ReportError(ERR_ILLEGAL_USE, "inline");
            found = 1;
        }
        if (flags & 0x20) {
            CError_ReportError(ERR_ILLEGAL_USE, "& reference type");
            found = 1;
        }
        if (flags & 0x40) {
            CError_ReportError(ERR_ILLEGAL_USE, "explicit");
            found = 1;
        }
        if (flags & 0x80) {
            CError_ReportError(ERR_ILLEGAL_USE, "mutable");
            found = 1;
        }
        if (flags & 0x100) {
            CError_ReportError(ERR_ILLEGAL_USE, "virtual");
            found = 1;
        }
        if (flags & 0x200) {
            CError_ReportError(ERR_ILLEGAL_USE, "friend");
            found = 1;
        }
        if (flags & 0x400) {
            CError_ReportError(ERR_ILLEGAL_USE, "in");
            found = 1;
        }
        if (flags & 0x800) {
            CError_ReportError(ERR_ILLEGAL_USE, "out");
            found = 1;
        }
        if (flags & 0x1000) {
            CError_ReportError(ERR_ILLEGAL_USE, "inout");
            found = 1;
        }
        if (flags & 0x2000) {
            CError_ReportError(ERR_ILLEGAL_USE, "bycopy");
            found = 1;
        }
        if (flags & 0x4000) {
            CError_ReportError(ERR_ILLEGAL_USE, "byref");
            found = 1;
        }
        if (flags & 0x8000) {
            CError_ReportError(ERR_ILLEGAL_USE, "oneway");
            found = 1;
        }
        if (flags & 0x80000000) {
            CError_ReportError(ERR_ILLEGAL_USE, "__declspec(interrupt)");
            found = 1;
        }
        if (flags & 0x1e000000) {
            CError_ReportError(ERR_ILLEGAL_USE, "__attribute__((aligned(?)))");
            found = 1;
        }
        if (!found) {
            CError_ReportError(ERR_ILLEGAL_TYPE_QUALIFIERS);
        }
    }
}
