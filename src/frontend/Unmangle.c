#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/Unmangle.h"
#include "compiler/CParser.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CError.h"
#include "compiler/CodeGen.h"
#include "compiler/InlineAsmRegisters.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
/* Type extension carrying the byte-sized kind at offset 0x0e. */

#include <string.h>

static char data_005649b8 = 0;
#define false 0
#define true 1

static inline char *CopyTemplate(int *jb, MOutBuf *ob, char *q, unsigned int n)
{
    char c;
    char *r;
    MOutBuf *buf;

    while (n-- != 0) {
        if ((c = *q++) == 0)
            longjmp(jb, 1);
        if (c == '<') {
            buf = ob;
            r = unmangle_template_arguments(jb, buf, q);
            n -= r - q;
            if (n == 0)
                return r;
            longjmp(jb, 1);
        } else if (ob->left != 0) {
            *ob->ptr++ = c;
            ob->left--;
        }
    }
    return q;
}

static void AppendText(MOutBuf *buf, const char *str, unsigned int len)
{
    if (buf != NULL) {
        unsigned int n = len;
        if (n > buf->left)
            n = buf->left;
        memcpy(buf->ptr, str, n);
        buf->ptr += n;
        buf->left -= n;
    }
}

static void put_2(MOutBuf *s, const char *str)
{
    unsigned int n;
    if (!s)
        return;
    n = 2;
    if (s->left < n)
        n = s->left;
    memcpy(s->ptr, str, n);
    s->ptr += n;
    s->left -= n;
}

static void put_c(MOutBuf *s, char c)
{
    if (s && s->left) {
        *s->ptr++ = c;
        s->left--;
    }
}

static inline void put(MOutBuf *b, char *s, unsigned int n)
{
    unsigned int c;
    if (b == NULL)
        return;
    if ((unsigned int)b->left < (c = n))
        c = (unsigned int)b->left;
    memcpy(b->ptr, s, c);
    b->ptr += c;
    b->left -= c;
}

static inline char *scan(int *a0, MOutBuf *a1, char *arg, int n, MOutBuf *inner)
{
    char *p;
    char c;
    char *r;
    p = arg;
    while (n-- != 0) {
        if ((c = *p++) == 0)
            longjmp(a0, 1);
        if (c == 60) {
            r = unmangle_template_arguments(a0, inner, p);
            if ((n = n - ((int)r - (int)p)) == 0)
                return r;
            longjmp(a0, 1);
        } else if (a1 != NULL && a1->left != 0) {
            *a1->ptr++ = c;
            a1->left -= 1;
        }
    }
    return p;
}

static inline char *scan_component(int *context, MOutBuf *output, char *cursor, int length, MOutBuf *inner)
{
    char *p;
    char c;
    char *r;
    p = cursor;
    while (length-- != 0) {
        if ((c = *p++) == 0)
            longjmp(context, 1);
        if (c == 60) {
            r = unmangle_template_arguments(context, inner, p);
            if ((length = length - ((int)r - (int)p)) == 0)
                return r;
            longjmp(context, 1);
        } else if (output != NULL && output->left != 0) {
            *output->ptr++ = c;
            output->left -= 1;
        }
    }
    return p;
}

static void PutChar(MOutBuf *buf, char c)
{
    if (buf != NULL && buf->left != 0) {
        *buf->ptr++ = c;
        buf->left--;
    }
}

char *unmangle_template_arguments(int *ctx, MOutBuf *buf, char *p)
{
    char *q;

    PutChar(buf, '<');
    for (;;) {
        switch (*p) {
            case '&':
            case '=':
                if (*p++ == '&')
                    PutChar(buf, '&');
                for (;;) {
                    switch (*p) {
                        case '\0':
                            longjmp(ctx, 1);
                            break;
                        case ',':
                        case '>':
                            break;
                        default:
                            PutChar(buf, *p++);
                            continue;
                    }
                    break;
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                q = p + 1;
                for (;;) {
                    if (*q == '\0')
                        longjmp(ctx, 1);
                    if (*q == ',' || *q == '>') {
                        do {
                            PutChar(buf, *p++);
                        } while (*p != '>' && *p != ',');
                        break;
                    }
                    if (*q < '0' || *q > '9') {
                        p = unmangle_type(ctx, buf, p);
                        break;
                    }
                    q++;
                }
                break;
            default:
                p = unmangle_type(ctx, buf, p);
                break;
        }
        if (*p == '>')
            break;
        if (*p++ != ',')
            longjmp(ctx, 1);
        if (buf != NULL) {
            unsigned int n = 2;
            if (buf->left < n)
                n = buf->left;
            memcpy(buf->ptr, ", ", n);
            buf->ptr += n;
            buf->left -= n;
        }
    }
    PutChar(buf, '>');
    return p + 1;
}

char *fn_004e8800(int *a, MOutBuf *out, char *p)
{
    char *str;
    char flag = 0;
    unsigned n;

    switch (*p++) {
        case 'a':
            switch (*p++) {
                case 'a':
                    switch (*p++) {
                        case '_':
                            p--;
                            str = "operator &&";
                            break;
                        case 'd':
                            str = "operator &=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'd':
                    switch (*p++) {
                        case '_':
                            p--;
                            str = "operator &";
                            break;
                        case 'v':
                            str = "operator /=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'e':
                    switch (*p++) {
                        case 'r':
                            str = "operator ^=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'l':
                    switch (*p++) {
                        case 's':
                            str = "operator <<=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'm':
                    switch (*p++) {
                        case 'd':
                            str = "operator %=";
                            break;
                        case 'i':
                            str = "operator -=";
                            break;
                        case 'u':
                            str = "operator *=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'o':
                    switch (*p++) {
                        case 'r':
                            str = "operator |=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'p':
                    switch (*p++) {
                        case 'l':
                            str = "operator +=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 'r':
                    switch (*p++) {
                        case 's':
                            str = "operator >>=";
                            break;
                        default:
                            return NULL;
                    }
                    break;
                case 's':
                    str = "operator =";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'c':
            switch (*p++) {
                case 'l':
                    str = "operator ()";
                    break;
                case 'm':
                    str = "operator ,";
                    break;
                case 'o':
                    str = "operator ~";
                    break;
                case 't':
                    str = "!";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'd':
            switch (*p++) {
                case 'l':
                    if (*p == 'a') {
                        p++;
                        str = "operator delete[]";
                    } else {
                        str = "operator delete";
                    }
                    break;
                case 't':
                    str = "~";
                    break;
                case 'v':
                    str = "operator /";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'e':
            switch (*p++) {
                case 'q':
                    str = "operator ==";
                    break;
                case 'r':
                    str = "operator ^";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'g':
            switch (*p++) {
                case 'e':
                    str = "operator >=";
                    break;
                case 't':
                    str = "operator >";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'l':
            switch (*p++) {
                case 'e':
                    str = "operator <=";
                    break;
                case 's':
                    str = "operator <<";
                    break;
                case 't':
                    str = "operator <";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'm':
            switch (*p++) {
                case 'd':
                    str = "operator %";
                    break;
                case 'i':
                    str = "operator -";
                    break;
                case 'l':
                    str = "operator *";
                    break;
                case 'm':
                    str = "operator --";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'n':
            switch (*p++) {
                case 'e':
                    str = "operator !=";
                    break;
                case 't':
                    str = "operator !";
                    break;
                case 'w':
                    if (*p == 'a') {
                        p++;
                        str = "operator new[]";
                    } else {
                        str = "operator new";
                    }
                    break;
                default:
                    return NULL;
            }
            break;
        case 'o':
            switch (*p++) {
                case 'r':
                    str = "operator |";
                    break;
                case 'o':
                    str = "operator ||";
                    break;
                case 'p':
                    if (out != NULL) {
                        n = 9;
                        if (out->left < 9)
                            n = out->left;
                        memcpy(out->ptr, "operator ", n);
                        out->ptr += n;
                        out->left -= n;
                    }
                    p = unmangle_type(a, out, p);
                    flag = 1;
                    break;
                default:
                    return NULL;
            }
            break;
        case 'p':
            switch (*p++) {
                case 'l':
                    str = "operator +";
                    break;
                case 'p':
                    str = "operator ++";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'r':
            switch (*p++) {
                case 'f':
                    str = "operator ->";
                    break;
                case 's':
                    str = "operator >>";
                    break;
                case 'm':
                    str = "operator ->*";
                    break;
                default:
                    return NULL;
            }
            break;
        case 'v':
            switch (*p++) {
                case 'c':
                    str = "operator []";
                    break;
                default:
                    return NULL;
            }
            break;
        default:
            return NULL;
    }
    if (!flag) {
        if (*p != '\0') {
            if (out != NULL) {
                n = strlen(str);
                if (out->left < n)
                    n = out->left;
                memcpy(out->ptr, str, n);
                out->ptr += n;
                out->left -= n;
            }
            if (*p == '<')
                p = unmangle_template_arguments(a, out, p + 1);
            if (*p++ != '_' || *p++ != '_')
                return NULL;
        }
    } else {
        if (*p++ != '_' || *p++ != '_')
            return NULL;
    }
    *out->ptr = 0;
    out->left = 0;
    return p;
}

char *unmangle_qualified_name(int *context, MOutBuf *output, char *cursor, char append, char alternate)
{
    int length;
    char *next;
    int lastLength;
    int component;
    int componentCount;
    char *lastComponent;
    if (*cursor == 'Q') {
        componentCount = (int)cursor[1];
        cursor += 2;
        componentCount = componentCount - '0';
        if (componentCount < 1 || componentCount > 9) {
            longjmp(context, 1);
        }
    } else {
        componentCount = 1;
    }
    component = 0;
    while (component < componentCount) {
        if (component != 0 && output != NULL)
            put(output, "::", 2);
        if (*cursor < '0' || *cursor > '9') {
            longjmp(context, 1);
        }
        length = 0;
        for (; *cursor >= '0' && *cursor <= '9'; cursor = cursor + 1) {
            length = *cursor + length * 10 - '0';
        }
        if (length == 0) {
            longjmp(context, 1);
        }
        lastComponent = cursor;
        lastLength = length;
        next = scan_component(context, output, cursor, length, output);
        component = component + 1;
        cursor = next;
    }
    if (append != 0 && output != NULL) {
        if (alternate != 0) {
            put(output, "::~", 3);
        } else {
            put(output, "::", 2);
        }
        (void)scan_component(context, output, lastComponent, lastLength, NULL);
    }
    return cursor;
}

/* Output cursor and remaining buffer capacity. */
char *write_qualifiers(MOutBuf *output, char *qualifiers)
{
    char *keyword;
    char *space;
    Boolean first;
    unsigned int length;

    first = true;
    do {
        switch (*qualifiers) {
            case 'U':
                keyword = "unsigned";
                break;
            case 'C':
                keyword = "const";
                break;
            case 'V':
                keyword = "volatile";
                break;
            case 'S':
                keyword = "signed";
                break;
            default:
                return qualifiers;
        }
        if (((!first) && (output != NULL)) && (output->left != 0)) {
            space = output->ptr;
            output->ptr++;
            *space = ' ';
            output->left--;
        }
        if (output != NULL) {
            length = strlen(keyword);
            if (output->left < length) {
                length = output->left;
            }
            memcpy((unsigned int *)output->ptr, (unsigned int *)keyword, length);
            output->ptr += length;
            output->left -= length;
        }
        first = false;
        qualifiers = qualifiers + 1;
    } while (true);
}

char *unmangle_indirect_declarator(int *context, MOutBuf *output, char *name)
{
    char *prefix;
    char *referenceEnd;
    char *pointerEnd;
    unsigned int copyLength;
    char *memberEnd;
    char *pointerSlot;
    char *referenceSlot;
    prefix = name;
    name = write_qualifiers(NULL, name);
    if (prefix == name)
        prefix = NULL;

    switch (*name) {
        case 'P':
            pointerEnd = unmangle_indirect_declarator(context, output, name + 1);
            if (output != NULL) {
                if (output != NULL && output->left != 0) {
                    pointerSlot = output->ptr;
                    output->ptr += 1;
                    *pointerSlot = '*';
                    output->left -= 1;
                }
                if (prefix != NULL)
                    write_qualifiers(output, prefix);
            }
            return pointerEnd;

        case 'R':
            referenceEnd = unmangle_indirect_declarator(context, output, name + 1);
            if (output != NULL) {
                if (output != NULL && output->left != 0) {
                    referenceSlot = output->ptr;
                    output->ptr += 1;
                    *referenceSlot = '&';
                    output->left -= 1;
                }
                if (prefix != NULL)
                    write_qualifiers(output, prefix);
            }
            return referenceEnd;

        case 'M':
            memberEnd = unmangle_qualified_name(context, output, name + 1, 0, 0);
            if (output != NULL) {
                if (output != NULL) {
                    copyLength = 3;
                    if (output->left < 3)
                        copyLength = output->left;
                    memcpy(output->ptr, "::*", copyLength);
                    output->ptr += copyLength;
                    output->left -= copyLength;
                }
                if (prefix != NULL)
                    write_qualifiers(output, prefix);
            }
            return memberEnd;

        default:
            return name;
    }
}

char *unmangle_array(int *context, MOutBuf *stream, char *name, char *format)
{
    char *dimensions;

    name++;
    dimensions = name;
    for (;;) {
        switch (*name++) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                continue;
            case '_':
                if (*name != 'A')
                    break;
                name++;
                continue;
            default:
                longjmp(context, 1);
                break;
        }
        break;
    }
    name = unmangle_type(context, stream, name);
    if (stream != NULL) {
        if (format != NULL) {
            put_2(stream, " (");
            unmangle_indirect_declarator(context, stream, format);
            put_c(stream, ')');
        }
        put_c(stream, '[');
        for (;;) {
            switch (*dimensions) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    put_c(stream, *dimensions++);
                    continue;
                case '_':
                    if (dimensions[1] != 'A')
                        break;
                    put_2(stream, "][");
                    dimensions += 2;
                    continue;
                default:
                    longjmp(context, 1);
                    break;
            }
            break;
        }
        put_c(stream, ']');
    }
    return name;
}

char *unmangle_function_type(int *context, MOutBuf *output, char *cursor, char *suffix)
{
    char *arguments;
    unsigned int count;

    cursor = cursor + 1;
    arguments = cursor;
    while (*cursor != '_')
        cursor = unmangle_type(context, NULL, cursor);
    cursor = unmangle_type(context, output, cursor + 1);
    if (output != NULL) {
        if (suffix != NULL) {
            if (output != NULL) {
                count = 2;
                if (output->left < count)
                    count = output->left;
                memcpy(output->ptr, " (", count);
                output->ptr += count;
                output->left -= count;
            }
            {
                MOutBuf *formatOutput = output;
                char *name = suffix;
                unmangle_indirect_declarator(context, formatOutput, name);
            }
            if (output != NULL && output->left != 0) {
                *output->ptr++ = ')';
                output->left--;
            }
        }
        if (output != NULL && output->left != 0) {
            *output->ptr++ = '(';
            output->left--;
        }
        if (*arguments != '_') {
            do {
                arguments = unmangle_type(context, output, arguments);
                if (*arguments == '_')
                    break;
                if (output != NULL) {
                    count = 2;
                    if (output->left < count)
                        count = output->left;
                    memcpy(output->ptr, ", ", count);
                    output->ptr += count;
                    output->left -= count;
                }
            } while (1);
        }
        if (output != NULL && output->left != 0) {
            *output->ptr++ = ')';
            output->left--;
        }
    }
    return cursor;
}

/* Cursor and remaining capacity for the output buffer. */
char *unmangle_indirect_type(int *context, MOutBuf *output, char *input, char *suffix, char marker)
{
    char *argument = input;
    char *kind;
    char *result;
    int count;

    kind = unmangle_indirect_declarator(context, NULL, input);
    switch (*kind) {
        case 'A':
            if (suffix != NULL) {
                argument = suffix;
            }
            unmangle_array(context, output, kind, argument);
            return;
        case 'F':
            if (suffix != NULL) {
                argument = suffix;
            }
            unmangle_function_type(context, output, kind, argument);
            return;
        default:
            if (marker == 'M') {
                input = unmangle_qualified_name(context, NULL, (input + 1), 0, 0);
                result = unmangle_type(context, output, input);
                if (output != NULL) {
                    if (output != NULL && output->left != 0) {
                        *output->ptr++ = ' ';
                        output->left -= 1;
                    }
                    unmangle_qualified_name(context, output, (argument + 1), 0, 0);
                    if (output != NULL) {
                        count = 3;
                        if (output->left < 3u) {
                            count = output->left;
                        }
                        memcpy(output->ptr, "::*", count);
                        output->ptr += count;
                        output->left -= count;
                    }
                    if (suffix != NULL) {
                        write_qualifiers(output, suffix);
                    }
                }
            } else {
                result = unmangle_type(context, output, (input + 1));
                if (output != NULL) {
                    if (output != NULL && output->left != 0) {
                        *output->ptr++ = marker;
                        output->left -= 1;
                    }
                    if (suffix != NULL) {
                        write_qualifiers(output, suffix);
                    }
                }
            }
    }
    return result;
}

char *unmangle_vector_type(int *unused, MOutBuf *buf, char *s, void *loc)
{
    char *p = s + 2;

    switch (s[1]) {
        case 'U':
            switch (s[2]) {
                case 'c':
                    p++;
                    AppendText(buf, "vector unsigned char", 20);
                    break;
                case 's':
                    p++;
                    AppendText(buf, "vector unsigned short", 21);
                    break;
                case 'i':
                    p++;
                    AppendText(buf, "vector unsigned int", 19);
                    break;
            }
            break;
        case 'c':
            AppendText(buf, "vector signed char", 18);
            break;
        case 'C':
            AppendText(buf, "vector bool char", 16);
            break;
        case 's':
            AppendText(buf, "vector signed short", 19);
            break;
        case 'S':
            AppendText(buf, "vector bool short", 17);
            break;
        case 'i':
            AppendText(buf, "vector signed int", 17);
            break;
        case 'I':
            AppendText(buf, "vector bool int ", 16);
            break;
        case 'f':
            AppendText(buf, "vector float", 12);
            break;
        case 'p':
            AppendText(buf, "vector pixel", 12);
            break;
        default:
            p = s;
            p++;
            break;
    }
    return p;
}

char *unmangle_type(int *context, MOutBuf *output, char *type)
{
    char *qualifiers = type;
    unsigned int length;
    const char *name;
    type = write_qualifiers(NULL, type);
    if (qualifiers == type)
        qualifiers = NULL;
    switch (*type) {
        case 'b':
            name = "bool";
            break;
        case 'v':
            name = "void";
            break;
        case 'c':
            name = "char";
            break;
        case 'w':
            name = "wchar_t";
            break;
        case 's':
            name = "short";
            break;
        case 'i':
            name = "int";
            break;
        case 'l':
            name = "long";
            break;
        case 'x':
            name = "long long";
            break;
        case 'f':
            name = "float";
            break;
        case 'D':
            name = "short double";
            break;
        case 'd':
            name = "double";
            break;
        case 'r':
            name = "long double";
            break;
        case 'X':
            return unmangle_vector_type(context, output, type, qualifiers);
        case 'P':
            unmangle_indirect_type(context, output, type, qualifiers, '*');
            return;
        case 'R':
            unmangle_indirect_type(context, output, type, qualifiers, '&');
            return;
        case 'M':
            unmangle_indirect_type(context, output, type, qualifiers, 'M');
            return;
        case 'A':
            unmangle_array(context, output, type, NULL);
            return;
        case 'F':
            unmangle_function_type(context, output, type, NULL);
            return;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'Q':
            if (qualifiers) {
                write_qualifiers(output, qualifiers);
                if (output != NULL && output->left != 0) {
                    *output->ptr++ = ' ';
                    output->left--;
                }
            }
            return unmangle_qualified_name(context, output, type, 0, 0);
        default:
            longjmp(context, 1);
            break;
    }
    if (qualifiers) {
        write_qualifiers(output, qualifiers);
        if (output != NULL && output->left != 0) {
            *output->ptr++ = ' ';
            output->left--;
        }
    }
    if (output) {
        length = strlen(name);
        if (output->left < length)
            length = output->left;
        memcpy(output->ptr, name, length);
        output->ptr += length;
        output->left -= length;
    }
    return type + 1;
}

void unmangle_function_parameters(int *context, MOutBuf *output, char *encoding)
{
    Boolean isConst;
    Boolean isVolatile;

    isVolatile = isConst = 0;

    if (*encoding == 'C') {
        encoding++;
        isConst = 1;
    }
    if (*encoding == 'V') {
        encoding++;
        isVolatile = 1;
    }
    if (*encoding++ != 'F') {
        longjmp(context, 1);
    }

    if (output != NULL && output->left != 0) {
        *output->ptr++ = '(';
        output->left--;
    }

    if (!(*encoding == 'v' && encoding[1] == '\0')) {
        for (;;) {
            if (*encoding == 'e') {
                if (encoding[1] != '\0') {
                    longjmp(context, 1);
                }
                if (output != NULL) {
                    unsigned int length = 3;
                    if (output->left < length) {
                        length = output->left;
                    }
                    memcpy(output->ptr, "...", length);
                    output->ptr += length;
                    output->left -= length;
                }
                break;
            }
            encoding = unmangle_type(context, output, encoding);
            if (*encoding == '\0') {
                break;
            }
            if (output != NULL && output->left != 0) {
                *output->ptr++ = ',';
                output->left--;
            }
        }
    }

    if (output != NULL && output->left != 0) {
        *output->ptr++ = ')';
        output->left--;
    }
    if (isConst && output != NULL) {
        unsigned int length = 6;
        if (output->left < length) {
            length = output->left;
        }
        memcpy(output->ptr, " const", length);
        output->ptr += length;
        output->left -= length;
    }
    if (isVolatile && output != NULL) {
        unsigned int length = 9;
        if (output->left < length) {
            length = output->left;
        }
        memcpy(output->ptr, " volatile", length);
        output->ptr += length;
        output->left -= length;
    }
    *output->ptr = '\0';
    output->left = 0;
}

void Unmangle_UnmangleName(char *name, char *out, UInt32 size)
{
    MOutBuf ob;
    jmp_buf jb;
    Boolean templ;
    char *p;

    if (name[0] == 'Q' && name[1] >= '1' && name[1] <= '9') {
        if (!_Setjmp(jb)) {
            ob.ptr = out;
            ob.left = size - 1;
            unmangle_qualified_name(jb, &ob, name, 0, 0);
            *ob.ptr = 0;
            ob.left = 0;
            return;
        }
    }
    templ = 0;
    for (p = name;; p++) {
        switch (*p) {
            case 0:
                break;
            case '<':
                templ = 1;
            default:
                continue;
        }
        break;
    }
    if (templ && !_Setjmp(jb)) {
        ob.ptr = out;
        ob.left = size - 1;
        (void)CopyTemplate(jb, &ob, name, p - name);
        *ob.ptr = 0;
        ob.left = 0;
        return;
    }
    strncpy(out, name, size);
    out[size - 1] = 0;
}

void Unmangle_UnmangleSymbolName(char *name, char *output, int outputSize)
{
    char *suffix;
    char hasSuffix;
    char ch;
    unsigned int length;
    unsigned int functionLength;
    char *memberSuffix;
    int separatorLength;
    unsigned int memberLength;
    jmp_buf state;
    MOutBuf identifierBuffer;
    MOutBuf outputBuffer;
    char identifier[256];

    switch (name[0]) {
        case '.':
            if (data_005649b8 == '.')
                ++name;
            break;
        case '_':
            switch (name[1]) {
                case '#':
                case '%':
                case '@':
                    name += 2;
            }
    }

    if (_Setjmp(state) == 0) {
        identifierBuffer.left = sizeof(identifier) - 1;
        identifierBuffer.ptr = identifier;
        outputBuffer.ptr = output;
        outputBuffer.left = outputSize - 1;
        if (*name != '_' || name[1] != '_' || (suffix = fn_004e8800(state, &identifierBuffer, name + 2)) == NULL) {
            hasSuffix = 0;
            identifierBuffer.ptr = identifier;
            identifierBuffer.left = sizeof(identifier) - 1;
            suffix = name;
            for (; *suffix == '_'; ++suffix) {
                if (identifierBuffer.left != 0) {
                    *identifierBuffer.ptr++ = '_';
                    --identifierBuffer.left;
                }
            }
            while ((ch = *suffix++) != 0) {
                if (ch == '_' && *suffix == '_') {
                    ++suffix;
                    while (*suffix == '_') {
                        if (identifierBuffer.left != 0) {
                            *identifierBuffer.ptr++ = '_';
                            --identifierBuffer.left;
                        }
                        ++suffix;
                    }
                    hasSuffix = 1;
                    break;
                }
                if (identifierBuffer.left != 0) {
                    *identifierBuffer.ptr++ = ch;
                    --identifierBuffer.left;
                }
            }
            *identifierBuffer.ptr = 0;
            identifierBuffer.left = 0;
            if (!hasSuffix) {
                length = strlen(identifier);
                if (outputBuffer.left < length)
                    length = outputBuffer.left;
                memcpy(outputBuffer.ptr, identifier, length);
                outputBuffer.ptr += length;
                outputBuffer.left -= length;
                *outputBuffer.ptr = 0;
                outputBuffer.left = 0;
                return;
            }
        }
        switch (*suffix) {
            case 'F':
                if (identifier[1] == 0 && (identifier[0] == '!' || identifier[0] == '~'))
                    break;
                functionLength = strlen(identifier);
                if (outputBuffer.left < functionLength)
                    functionLength = outputBuffer.left;
                memcpy(outputBuffer.ptr, identifier, functionLength);
                outputBuffer.ptr += functionLength;
                outputBuffer.left -= functionLength;
                unmangle_function_parameters(state, &outputBuffer, suffix);
                return;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'Q':
                if (identifier[1] == 0 && (identifier[0] == '!' || identifier[0] == '~')) {
                    memberSuffix = unmangle_qualified_name(state, &outputBuffer, suffix, 1, identifier[0] == '~');
                } else {
                    memberSuffix = unmangle_qualified_name(state, &outputBuffer, suffix, 0, 0);
                    separatorLength = 2;
                    if (outputBuffer.left < 2U)
                        separatorLength = outputBuffer.left;
                    memcpy(outputBuffer.ptr, "::", separatorLength);
                    outputBuffer.ptr += separatorLength;
                    outputBuffer.left -= separatorLength;
                    memberLength = strlen(identifier);
                    if (outputBuffer.left < memberLength)
                        memberLength = outputBuffer.left;
                    memcpy(outputBuffer.ptr, identifier, memberLength);
                    outputBuffer.ptr += memberLength;
                    outputBuffer.left -= memberLength;
                    if (*memberSuffix == 0) {
                        *outputBuffer.ptr = 0;
                        outputBuffer.left = 0;
                        return;
                    }
                }
                unmangle_function_parameters(state, &outputBuffer, memberSuffix);
                return;
        }
    }
    strncpy(output, name, outputSize);
    output[outputSize - 1] = 0;
}

void assign_object_register(Object *func, SInt16 register_number)
{
    VarInfo *regs;
    Type *type;
    SInt16 physical_register;
    SInt32 kind;

    regs = Registers_GetInfo(func);
    type = func->type;
    regs->used = 1;
    if (data_00588521) {
        if (func->sclass == TK_REGISTER) {
            if (register_number == 0) {
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, func->name->name);
            }
            regs->is_vector = 0;
            if (copts.operandsDebug && type->type == TYPEFLOAT) {
                if (type->size == 4) {
                    regs->is_fpr = 1;
                    regs->reg = register_number;
                    physical_register = regs->reg;
                } else {
                    regs->is_fpr = 1;
                    if (copts.littleendian != 0) {
                        regs->reg = register_number;
                        physical_register = regs->reg;
                        regs->regHi = register_number + 1;
                    } else {
                        regs->reg = register_number + 1;
                        physical_register = regs->reg;
                        regs->regHi = register_number;
                    }
                }
                CTemplateNew_InsertRegisterBinding(func->name->name, 0, physical_register, func);
            } else if ((type->type == TYPEINT || type->type == TYPEENUM) && type->size == 8) {
                regs->is_fpr = 0;
                if (copts.littleendian != 0) {
                    regs->regHi = register_number + 1;
                    physical_register = 0;
                    if (register_number < 10) {
                        regs->reg = register_number;
                        physical_register = regs->reg;
                    }
                } else {
                    regs->reg = register_number + 1;
                    physical_register = regs->reg;
                    if (register_number < 10) {
                        regs->regHi = register_number;
                    }
                }
                CTemplateNew_InsertRegisterBinding(func->name->name, 0, physical_register, func);
            } else if (type->type == TYPEFLOAT) {
                regs->is_fpr = 1;
                regs->reg = register_number;
                CTemplateNew_InsertRegisterBinding(func->name->name, 1, register_number, func);
            } else if (type->type == TYPESTRUCT && (kind = ((TypeStruct *)type)->stype) >= 4 && kind <= 0xe) {
                regs->is_vector = 1;
                regs->is_fpr = 0;
                regs->reg = register_number;
                CTemplateNew_InsertRegisterBinding(func->name->name, 9, register_number, func);
            } else {
                regs->is_fpr = 0;
                regs->reg = register_number;
                CTemplateNew_InsertRegisterBinding(func->name->name, 0, register_number, func);
            }
        }
    } else if (func->sclass == TK_REGISTER) {
        UInt8 typecode;
        UInt8 use_gpr;
        if ((((typecode = type->type) == TYPEINT || typecode == TYPEENUM) && type->size == 8) ||
            ((use_gpr = copts.operandsDebug) && typecode == TYPEFLOAT && type->size != 4)) {
            if (gAvailableSavedGPRs < 2) {
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, func->name->name);
            }
            Registers_AllocateGPRPair(func);
            if (Registers_GetInfo(func)) {
                Registers_GetInfo(func);
            }
            physical_register = Registers_GetInfo(func) ? (Registers_GetInfo(func))->reg : 0;
            CTemplateNew_InsertRegisterBinding(func->name->name, 0, physical_register, func);
        } else if (typecode == TYPEINT || typecode == TYPEENUM || typecode == TYPEPOINTER ||
                   (typecode == TYPEMEMBERPOINTER && type->size == 4) ||
                   (use_gpr && typecode == TYPEFLOAT && type->size == 4)) {
            if (gAvailableSavedGPRs == 0) {
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, func->name->name);
            }
            Registers_AllocateGPR(func);
            physical_register = Registers_GetInfo(func) ? (Registers_GetInfo(func))->reg : 0;
            CTemplateNew_InsertRegisterBinding(func->name->name, 0, physical_register, func);
        } else if (typecode == TYPEFLOAT) {
            if (gAvailableSavedFPRs == 0) {
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, func->name->name);
            }
            Registers_AllocateFPR(func);
            physical_register = Registers_GetInfo(func) ? (Registers_GetInfo(func))->reg : 0;
            CTemplateNew_InsertRegisterBinding(func->name->name, 1, physical_register, func);
        } else if (typecode == TYPESTRUCT && (kind = ((TypeStruct *)type)->stype) >= 4 && kind <= 0xe) {
            if (gAvailableSavedVRs == 0) {
                CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, func->name->name);
            }
            Registers_AllocateVR(func);
            physical_register = Registers_GetInfo(func) ? (Registers_GetInfo(func))->reg : 0;
            CTemplateNew_InsertRegisterBinding(func->name->name, 9, physical_register, func);
        } else {
            CError_ReportError(ERR_COULD_NOT_ASSIGNED_REGISTER, func->name->name);
        }
    }
}
