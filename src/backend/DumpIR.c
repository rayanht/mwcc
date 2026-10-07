#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/DumpIR.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CException.h"
#include "compiler/CFunc.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include <stdio.h>
#include <string.h>

static struct _FILE *data_005811b0;
static char lbl_005811b4[4];
static int enode_tree_depth_index;

/* The name of each ENode type. */
static char *data_00560cb4[75] = {
    "EPOSTINC",
    "EPOSTDEC",
    "EPREINC",
    "EPREDEC",
    "EINDIRECT",
    "EMONMIN",
    "EBINNOT",
    "ELOGNOT",
    "EFORCELOAD",
    "EMUL",
    "EMULV",
    "EDIV",
    "EMODULO",
    "EADDV",
    "ESUBV",
    "EADD",
    "ESUB",
    "ESHL",
    "ESHR",
    "ELESS",
    "EGREATER",
    "ELESSEQU",
    "EGREATEREQU",
    "EEQU",
    "ENOTEQU",
    "EAND",
    "EXOR",
    "EOR",
    "ELAND",
    "ELOR",
    "EASS",
    "EMULASS",
    "EDIVASS",
    "EMODASS",
    "EADDASS",
    "ESUBASS",
    "ESHLASS",
    "ESHRASS",
    "EANDASS",
    "EXORASS",
    "EORASS",
    "ECOMMA",
    "EPMODULO",
    "EROTL",
    "EROTR",
    "EBCLR",
    "EBTST",
    "EBSET",
    "ETYPCON",
    "EBITFIELD",
    "EINTCONST",
    "EFLOATCONST",
    "ESTRINGCONST",
    "ECOND",
    "EFUNCCALL",
    "EFUNCCALLP",
    "EOBJREF",
    "EMFPOINTER",
    "ENULLCHECK",
    "EPRECOMP",
    "ETEMP",
    "EARGOBJ",
    "ELOCOBJ",
    "ELABEL",
    "ESETCONST",
    "ENEWEXCEPTION",
    "ENEWEXCEPTIONARRAY",
    "EOBJLIST",
    "EMEMBER",
    "ETEMPLDEP",
    "EINSTRUCTION",
    "EDEFINE",
    "EREUSE",
    "EASSBLK",
    "EVECTOR128CONST",
};

static void PrintTypeLine(Type *type);
static void PrintType(Type *type);
static void WriteString(void *file, char *str);

void write_escaped_string(void *stream, char *string, SInt32 length)
{
    FILE *output = stream;
    while (length--) {
        switch (*string) {
            case '\0':
                fputs("\\x00", output);
                break;
            case '\a':
                fputs("\\a", output);
                break;
            case '\b':
                fputs("\\b", output);
                break;
            case '\f':
                fputs("\\f", output);
                break;
            case '\n':
                fputs("\\n", output);
                break;
            case '\r':
                fputs("\\r", output);
                break;
            case '\t':
                fputs("\\t", output);
                break;
            case '\v':
                fputs("\\v", output);
                break;
            case '"':
            case '\'':
            case '?':
            case '\\':
                fputc('\\', output);
                /* fallthrough */
            default:
                fputc(*string, output);
                break;
        }
        string++;
    }
}

void fn_004be840(void)
{
    return;
}

void fn_004be830(void *context, void *node)
{
}

void print_enode_tree(ENode *node, int depth)
{
    char buffer[64];
    char *name;
    ENodeList *arg;

    for (;;) {
        for (enode_tree_depth_index = 0; enode_tree_depth_index < depth; enode_tree_depth_index++)
            fputc(9, data_005811b0);

        if (node->flags != 0)
            fprintf(data_005811b0, "%s {%02X}", data_00560cb4[node->type], node->flags);
        else
            fprintf(data_005811b0, "%s", data_00560cb4[node->type]);

        switch (node->type) {
            case EINTCONST:
                if (node->rtype->size > 4)
                    fprintf(data_005811b0, "[0x%.8lX%.8lX]", node->data.intval.hi, node->data.intval.lo);
                else
                    fprintf(data_005811b0, "[%ld]", node->data.intval.lo);
                PrintType(node->rtype);
                break;
            case EFLOATCONST:
                CMach_PrintFloat(buffer, ((ENode *)node)->data.floatval);
                fprintf(data_005811b0, "[%s]", (unsigned int)buffer);
                PrintType(node->rtype);
                break;
            case ESTRINGCONST:
                if (((ENode *)node)->data.string.useExplicitSize != 0) {
                    fputs("[\"", data_005811b0);
                    write_escaped_string(data_005811b0, node->data.string.data, node->data.string.size);
                    fputs("\"]", data_005811b0);
                } else {
                    fputs("[\"", data_005811b0);
                    WriteString(data_005811b0, node->data.string.data);
                    fputs("\"]", data_005811b0);
                }
                PrintType(node->rtype);
                break;
            case EVECTOR128CONST:
                fprintf(data_005811b0, "[0x%.8lX%.8lX%.8lX%.8lX]", node->data.intval.hi, node->data.intval.lo,
                        ((ENode *)node)->data.vector128val.ul[2], ((ENode *)node)->data.vector128val.ul[3]);
                PrintType(node->rtype);
                break;
            case ECOND:
                PrintType(node->rtype);
                print_enode_tree(node->data.cond.cond, depth + 1);
                print_enode_tree(node->data.cond.expr1, depth + 1);
                node = node->data.cond.expr2;
                depth++;
                continue;
            case EFUNCCALL:
            case EFUNCCALLP:
                PrintType(node->rtype);
                print_enode_tree(node->data.funccall.funcref, depth + 1);
                for (arg = node->data.funccall.args; arg != NULL; arg = arg->next)
                    print_enode_tree(arg->node, depth + 1);
                break;
            case EOBJREF:
                switch (((const Object *)node->data.objref)->datatype) {
                    case DFUNC:
                        name = COptimizer_GetFunctionObject(node->data.objref)->name;
                        fprintf(data_005811b0, "[%s{PR}]", name);
                        break;
                    case DDATA:
                        if (PCodeUtilities_Require(node->data.objref) != 0) {
                            name = COptimizer_GetFunctionObject(node->data.objref)->name;
                            fprintf(data_005811b0, "[%s{TD}]", name);
                        } else {
                            name = COptimizer_GetFunctionObject(node->data.objref)->name;
                            fprintf(data_005811b0, "[%s{RW}]", name);
                        }
                        break;
                    default:
                        name = node->data.objref->name->name;
                        fprintf(data_005811b0, "[%s]", name);
                        break;
                }
                PrintType(node->rtype);
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
                PrintType(node->rtype);
                print_enode_tree(node->data.diadic.left, depth + 1);
                node = node->data.diadic.right;
                depth++;
                continue;
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
                PrintType(node->rtype);
                depth++;
                node = node->data.monadic;
                continue;
            case EMFPOINTER:
                PrintType(node->rtype);
                print_enode_tree(node->data.diadic.left, depth + 1);
                node = node->data.diadic.right;
                depth++;
                continue;
            case ENULLCHECK:
                fprintf(data_005811b0, " unique [%ld]", node->data.precomp.labelId);
                PrintType(node->rtype);
                print_enode_tree(node->data.diadic.left, depth + 1);
                node = node->data.diadic.right;
                depth++;
                continue;
            case EPRECOMP:
                fprintf(data_005811b0, " unique [%ld]", node->data.intval.hi);
                PrintType(node->rtype);
                break;
            case ELABEL:
                fprintf(data_005811b0, "[%s]", node->data.memberFunctionPointer->name->name);
                PrintType(node->rtype);
                break;
            case EDEFINE:
                fprintf(data_005811b0, "[%.8lX]", node);
                PrintTypeLine(node->rtype);
                depth++;
                node = node->data.monadic;
                continue;
            case EREUSE:
                fprintf(data_005811b0, "[%.8lX]", node->data.intval.hi);
                PrintTypeLine(node->rtype);
                break;
            default:
                break;
        }
        return;
    }
}

static void PrintTypeLine(Type *type)
{
    char buf[256];
    format_type(type, buf);
    fprintf(data_005811b0, " (%s)", (unsigned int)buf);
    fputs("\r\n", data_005811b0);
}

static void PrintType(Type *type)
{
    char buf[256];
    format_type(type, buf);
    fprintf(data_005811b0, " (%s)", (unsigned int)buf);
    fprintf(data_005811b0, "\r\n");
}

static void WriteString(void *file, char *str)
{
    write_escaped_string(file, str, strlen(str));
}

/* The linker stripped the function that used these literals; they stay in the unit's .data. */
static void DumpIR_StrippedLiterals(const char **literals)
{
    literals[0] = "\t\t%11s: %s\r\n";
    literals[1] = "\t\t    default: %s\r\n";
}

void format_type(Type *type, char *buf)
{
    char targetName[256];
    char ownerName[256];

    switch ((SInt8)type->type) {
        case TYPEVOID:
            strcpy(buf, "void");
            return;

        case TYPEINT:
            switch (TYPE_INTEGRAL(type)->integral) {
                case IT_BOOL:
                    strcpy(buf, "bool");
                    break;
                case IT_CHAR:
                    strcpy(buf, "char");
                    break;
                case IT_WCHAR_T:
                    strcpy(buf, "wchar_t");
                    break;
                case IT_SCHAR:
                    strcpy(buf, "signed char");
                    break;
                case IT_UCHAR:
                    strcpy(buf, "unsigned char");
                    break;
                case IT_SHORT:
                    strcpy(buf, "short");
                    break;
                case IT_USHORT:
                    strcpy(buf, "unsigned short");
                    break;
                case IT_INT:
                    strcpy(buf, "int");
                    break;
                case IT_UINT:
                    strcpy(buf, "unsigned int");
                    break;
                case IT_LONG:
                    strcpy(buf, "long");
                    break;
                case IT_ULONG:
                    strcpy(buf, "unsigned long");
                    break;
                case IT_LONGLONG:
                    strcpy(buf, "long long");
                    break;
                case IT_ULONGLONG:
                    strcpy(buf, "unsigned long long");
                    break;
            }
            break;

        case TYPEFLOAT:
            switch (TYPE_INTEGRAL(type)->integral) {
                case IT_FLOAT:
                    strcpy(buf, "float");
                    break;
                case IT_SHORTDOUBLE:
                    strcpy(buf, "short double");
                    break;
                case IT_DOUBLE:
                    strcpy(buf, "double");
                    break;
                case IT_LONGDOUBLE:
                    strcpy(buf, "long double");
                    break;
            }
            break;

        case TYPEENUM:
            strcpy(buf, "enum ");
            if (TYPE_ENUM(type)->enumname != NULL)
                strcat(buf, TYPE_ENUM(type)->enumname->name);
            break;

        case TYPESTRUCT: {
            if ((int)(TYPE_STRUCT(type)->stype) >= 4 && (int)(TYPE_STRUCT(type)->stype) <= 14) {
                switch (TYPE_STRUCT(type)->stype) {
                    case STRUCT_VECTOR_UCHAR:
                        strcpy(buf, "vector unsigned char ");
                        break;
                    case STRUCT_VECTOR_SCHAR:
                        strcpy(buf, "vector signed char ");
                        break;
                    case STRUCT_VECTOR_BCHAR:
                        strcpy(buf, "vector bool char ");
                        break;
                    case STRUCT_VECTOR_USHORT:
                        strcpy(buf, "vector unsigned short ");
                        break;
                    case STRUCT_VECTOR_SSHORT:
                        strcpy(buf, "vector signed short ");
                        break;
                    case STRUCT_VECTOR_BSHORT:
                        strcpy(buf, "vector bool short ");
                        break;
                    case STRUCT_VECTOR_UINT:
                        strcpy(buf, "vector unsigned int ");
                        break;
                    case STRUCT_VECTOR_SINT:
                        strcpy(buf, "vector signed int ");
                        break;
                    case STRUCT_VECTOR_BINT:
                        strcpy(buf, "vector bool int ");
                        break;
                    case STRUCT_VECTOR_FLOAT:
                        strcpy(buf, "vector float ");
                        break;
                    case STRUCT_VECTOR_PIXEL:
                        strcpy(buf, "vector pixel ");
                        break;
                }
            } else {
                strcpy(buf, "struct ");
                if (TYPE_STRUCT(type)->name != NULL)
                    strcat(buf, TYPE_STRUCT(type)->name->name);
            }
        } break;

        case TYPECLASS:
            strcpy(buf, "class ");
            if (TYPE_CLASS(type)->classname != NULL)
                strcat(buf, TYPE_CLASS(type)->classname->name);
            break;

        case TYPEFUNC:
            format_type(TYPE_FUNC(type)->functype, targetName);
            strcpy(buf, "freturns(");
            strcat(buf, targetName);
            strcat(buf, ")");
            break;

        case TYPEBITFIELD:
            format_type(TYPE_BITFIELD(type)->bitfieldtype, targetName);
            sprintf(buf, "bitfield(%s){%d:%d}", targetName, TYPE_BITFIELD(type)->offset,
                    TYPE_BITFIELD(type)->bitlength);
            break;

        case TYPELABEL:
            strcpy(buf, "label");
            break;

        case TYPEPOINTER:
            format_type(TYPE_POINTER(type)->target, targetName);
            strcpy(buf, "pointer(");
            strcat(buf, targetName), strcat(buf, ")");
            break;

        case TYPEARRAY:
            format_type(TYPE_POINTER(type)->target, targetName);
            strcpy(buf, "array(");
            strcat(buf, targetName);
            strcat(buf, ")");
            break;

        case TYPEMEMBERPOINTER:
            format_type(TYPE_MEMBER_POINTER(type)->ty2, targetName);
            format_type(TYPE_MEMBER_POINTER(type)->ty1, ownerName);
            strcpy(buf, "memberpointer(");
            strcat(buf, targetName);
            strcat(buf, ",");
            strcat(buf, ownerName);
            strcat(buf, ")");
            break;
    }
}

void dump_eat_nodes(ExceptionAction *p)
{
    char buf[256];

    while (p != NULL) {
        fprintf(data_005811b0, "\t\t:");
        switch (p->kind) {
            case 1:
                fprintf(data_005811b0, "EAT_DESTROYLOCAL %s(&%s)%s",
                        COptimizer_GetFunctionObject(p->data.local.dtor)->name, p->data.local.object->name->name,
                        "\r\n");
                break;
            case 2:
                fprintf(data_005811b0, "EAT_DESTROYLOCALCOND%s", "\r\n");
                break;
            case 3:
                fprintf(data_005811b0, "EAT_DESTROYLOCALOFFSET %s(&%s+%ld)%s",
                        COptimizer_GetFunctionObject(p->data.local.dtor)->name, p->data.local.object->name->name,
                        p->data.delete_pointer_cond.cond, "\r\n");
                break;
            case 4:
                fprintf(data_005811b0, "EAT_DESTROYLOCALPOINTER%s", "\r\n");
                break;
            case 5:
                fprintf(data_005811b0, "EAT_DESTROYLOCALARRAY%s", "\r\n");
                break;
            case 17:
                fprintf(data_005811b0, "EAT_DESTROYBASE %s(this+%ld)%s",
                        COptimizer_GetFunctionObject(p->data.local.dtor)->name, p->data.delete_pointer_cond.cond,
                        "\r\n");
                break;
            case 7:
                fprintf(data_005811b0, "EAT_DESTROYMEMBER %s(%s+%ld)%s",
                        COptimizer_GetFunctionObject(p->data.local.dtor)->name, p->data.local.object->name->name,
                        p->data.delete_pointer_cond.cond, "\r\n");
                break;
            case 8:
                fprintf(data_005811b0, "EAT_DESTROYMEMBERCOND if(%s) %s(this+%ld)%s", p->data.local.dtor->name->name,
                        COptimizer_GetFunctionObject(p->data.delete_pointer_cond.cond)->name,
                        p->data.member_cond.offset, "\r\n");
                break;
            case 9:
                fprintf(data_005811b0, "EAT_DESTROYMEMBERARRAY %s(this+%ld)[%ld] size: %ld%s",
                        COptimizer_GetFunctionObject(p->data.local.dtor)->name, p->data.delete_pointer_cond.cond,
                        p->data.member_cond.offset, p->data.catch_block.exceptionType, "\r\n");
                break;
            case 10:
                fprintf(data_005811b0, "EAT_DELETEPOINTER(%s)%s", p->data.local.object->name->name, "\r\n");
                break;
            case 11:
                fprintf(data_005811b0, "EAT_DELETELOCALPOINTER(%s)%s", p->data.local.object->name->name, "\r\n");
                break;
            case 12:
                fprintf(data_005811b0, "EAT_DELETEPOINTERCOND if (%s)(%s)%s",
                        p->data.delete_pointer_cond.cond->name->name, p->data.local.object->name->name, "\r\n");
                break;
            case 13:
                fprintf(data_005811b0, "EAT_CATCHBLOCK ");
                if (p->data.catch_block.exceptionType != NULL) {
                    if (p->data.local.object != NULL) {
                        fprintf(data_005811b0, "[%s]", p->data.local.object->name->name);
                    } else {
                        fprintf(data_005811b0, "[]");
                    }
                    format_type(p->data.catch_block.exceptionType, buf);
                    fprintf(data_005811b0, " (%s)", buf);
                } else {
                    fprintf(data_005811b0, "[...] ");
                }
                fprintf(data_005811b0, " Label: %s%s", p->data.catch_block.label->uniquename->name, "\r\n");
                break;
            case 15:
                fprintf(data_005811b0, "EAT_SPECIFICATION%s", "\r\n");
                break;
            case 14:
                fprintf(data_005811b0, "EAT_ACTIVECATCHBLOCK%s", "\r\n");
                break;
            case 16:
                fprintf(data_005811b0, "EAT_TERMINATE%s", "\r\n");
                break;
        }
        p = p->next;
    }
}
