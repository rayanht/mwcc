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

#define false 0
#define true 1

static void unmangle_error(int *jb)
{
    longjmp(jb, 1);
}

static void init_buffer(MOutBuf *buf, char *data, int size)
{
    buf->ptr = data;
    buf->left = size - 1;
}

static void put_c(MOutBuf *s, char c)
{
    if (s && s->left) {
        *s->ptr++ = c;
        s->left--;
    }
}

static void put_s(MOutBuf *s, const char *str)
{
    unsigned int n;

    if (s) {
        n = strlen(str);
        if (s->left < n)
            n = s->left;
        memcpy(s->ptr, str, n);
        s->ptr += n;
        s->left -= n;
    }
}

static void terminate(MOutBuf *s)
{
    *s->ptr = 0;
    s->left = 0;
}

char *unmangle_template_arguments(int *ctx, MOutBuf *buf, char *p)
{
    char *q;

    put_c(buf, '<');
    for (;;) {
        switch (*p) {
            case '&':
            case '=':
                if (*p++ == '&')
                    put_c(buf, '&');
                for (;;) {
                    switch (*p) {
                        case '\0':
                            unmangle_error(ctx);
                            break;
                        case ',':
                        case '>':
                            break;
                        default:
                            put_c(buf, *p++);
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
                        unmangle_error(ctx);
                    if (*q == ',' || *q == '>') {
                        do {
                            put_c(buf, *p++);
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
            unmangle_error(ctx);
        put_s(buf, ", ");
    }
    put_c(buf, '>');
    return p + 1;
}

char *fn_004e8800(int *a, MOutBuf *out, char *p)
{
    char *str;
    char flag = 0;

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
                    put_s(out, "operator ");
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
            put_s(out, str);
            if (*p == '<')
                p = unmangle_template_arguments(a, out, p + 1);
            if (*p++ != '_' || *p++ != '_')
                return NULL;
        }
    } else {
        if (*p++ != '_' || *p++ != '_')
            return NULL;
    }
    terminate(out);
    return p;
}

static char *copy_name(int *jb, MOutBuf *buf, char *p, int n, char templates)
{
    char c;
    char *r;

    while (n-- != 0) {
        if ((c = *p++) == 0)
            unmangle_error(jb);
        if (c == '<') {
            r = unmangle_template_arguments(jb, templates ? buf : NULL, p);
            n -= r - p;
            if (n == 0)
                return r;
            unmangle_error(jb);
        } else {
            put_c(buf, c);
        }
    }
    return p;
}

char *unmangle_qualified_name(int *context, MOutBuf *output, char *cursor, char append, char alternate)
{
    char *next;
    char *lastComponent;
    int length;
    int component;
    int componentCount;
    int lastLength;
    if (*cursor == 'Q') {
        componentCount = (int)cursor[1];
        cursor += 2;
        componentCount = componentCount - '0';
        if (componentCount < 1 || componentCount > 9) {
            unmangle_error(context);
        }
    } else {
        componentCount = 1;
    }
    component = 0;
    while (component < componentCount) {
        if (component != 0 && output != NULL)
            put_s(output, "::");
        if (*cursor < '0' || *cursor > '9') {
            unmangle_error(context);
        }
        length = 0;
        for (; *cursor >= '0' && *cursor <= '9'; cursor = cursor + 1) {
            length = *cursor + length * 10 - '0';
        }
        if (length == 0) {
            unmangle_error(context);
        }
        lastComponent = cursor;
        lastLength = length;
        next = copy_name(context, output, cursor, length, 1);
        component = component + 1;
        cursor = next;
    }
    if (append != 0 && output != NULL) {
        if (alternate)
            put_s(output, "::~");
        else
            put_s(output, "::");
        (void)copy_name(context, output, lastComponent, lastLength, 0);
    }
    return cursor;
}

/* Output cursor and remaining buffer capacity. */
char *write_qualifiers(MOutBuf *output, char *qualifiers)
{
    char *keyword;
    Boolean first;

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
        if (!first)
            put_c(output, ' ');
        put_s(output, keyword);
        first = false;
        qualifiers = qualifiers + 1;
    } while (true);
}

char *unmangle_indirect_declarator(int *context, MOutBuf *output, char *name)
{
    char *prefix;
    prefix = name;
    name = write_qualifiers(NULL, name);
    if (prefix == name)
        prefix = NULL;

    switch (*name) {
        case 'P':
            name = unmangle_indirect_declarator(context, output, name + 1);
            if (output != NULL) {
                put_c(output, '*');
                if (prefix != NULL)
                    write_qualifiers(output, prefix);
            }
            return name;

        case 'R':
            name = unmangle_indirect_declarator(context, output, name + 1);
            if (output != NULL) {
                put_c(output, '&');
                if (prefix != NULL)
                    write_qualifiers(output, prefix);
            }
            return name;

        case 'M':
            name = unmangle_qualified_name(context, output, name + 1, 0, 0);
            if (output != NULL) {
                put_s(output, "::*");
                if (prefix != NULL)
                    write_qualifiers(output, prefix);
            }
            return name;

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
                unmangle_error(context);
                break;
        }
        break;
    }
    name = unmangle_type(context, stream, name);
    if (stream != NULL) {
        if (format != NULL) {
            put_s(stream, " (");
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
                    put_s(stream, "][");
                    dimensions += 2;
                    continue;
                default:
                    unmangle_error(context);
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

    cursor = cursor + 1;
    arguments = cursor;
    while (*cursor != '_')
        cursor = unmangle_type(context, NULL, cursor);
    cursor = unmangle_type(context, output, cursor + 1);
    if (output != NULL) {
        if (suffix != NULL) {
            put_s(output, " (");
            unmangle_indirect_declarator(context, output, suffix);
            put_c(output, ')');
        }
        put_c(output, '(');
        if (*arguments != '_') {
            do {
                arguments = unmangle_type(context, output, arguments);
                if (*arguments == '_')
                    break;
                put_s(output, ", ");
            } while (1);
        }
        put_c(output, ')');
    }
    return cursor;
}

/* Cursor and remaining capacity for the output buffer. */
char *unmangle_indirect_type(int *context, MOutBuf *output, char *input, char *suffix, char marker)
{
    char *argument = input;
    char *kind;
    char *result;

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
                    put_c(output, ' ');
                    unmangle_qualified_name(context, output, (argument + 1), 0, 0);
                    put_s(output, "::*");
                    if (suffix != NULL) {
                        write_qualifiers(output, suffix);
                    }
                }
            } else {
                result = unmangle_type(context, output, (input + 1));
                if (output != NULL) {
                    put_c(output, marker);
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
                    put_s(buf, "vector unsigned char");
                    break;
                case 's':
                    p++;
                    put_s(buf, "vector unsigned short");
                    break;
                case 'i':
                    p++;
                    put_s(buf, "vector unsigned int");
                    break;
            }
            break;
        case 'c':
            put_s(buf, "vector signed char");
            break;
        case 'C':
            put_s(buf, "vector bool char");
            break;
        case 's':
            put_s(buf, "vector signed short");
            break;
        case 'S':
            put_s(buf, "vector bool short");
            break;
        case 'i':
            put_s(buf, "vector signed int");
            break;
        case 'I':
            put_s(buf, "vector bool int ");
            break;
        case 'f':
            put_s(buf, "vector float");
            break;
        case 'p':
            put_s(buf, "vector pixel");
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
    const char *name;
    char *qualifiers = type;
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
                put_c(output, ' ');
            }
            return unmangle_qualified_name(context, output, type, 0, 0);
        default:
            unmangle_error(context);
            break;
    }
    if (qualifiers) {
        write_qualifiers(output, qualifiers);
        put_c(output, ' ');
    }
    put_s(output, name);
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
        unmangle_error(context);
    }

    put_c(output, '(');

    if (!(*encoding == 'v' && encoding[1] == '\0')) {
        for (;;) {
            if (*encoding == 'e') {
                if (encoding[1] != '\0') {
                    unmangle_error(context);
                }
                put_s(output, "...");
                break;
            }
            encoding = unmangle_type(context, output, encoding);
            if (*encoding == '\0') {
                break;
            }
            put_c(output, ',');
        }
    }

    put_c(output, ')');
    if (isConst)
        put_s(output, " const");
    if (isVolatile)
        put_s(output, " volatile");
    terminate(output);
}

void Unmangle_UnmangleName(char *name, char *out, UInt32 size)
{
    MOutBuf ob;
    jmp_buf jb;
    Boolean templ;
    char *p;

    if (name[0] == 'Q' && name[1] >= '1' && name[1] <= '9') {
        if (!_Setjmp(jb)) {
            init_buffer(&ob, out, size);
            unmangle_qualified_name(jb, &ob, name, 0, 0);
            terminate(&ob);
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
        init_buffer(&ob, out, size);
        (void)copy_name(jb, &ob, name, p - name, 1);
        terminate(&ob);
        return;
    }
    strncpy(out, name, size);
    out[size - 1] = 0;
}

static char data_005649b8 = 0;

void Unmangle_UnmangleSymbolName(char *name, char *output, int outputSize)
{
    char *suffix;
    char hasSuffix;
    char ch;
    char *memberSuffix;
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
        init_buffer(&identifierBuffer, identifier, sizeof(identifier));
        init_buffer(&outputBuffer, output, outputSize);
        if (*name != '_' || name[1] != '_' || (suffix = fn_004e8800(state, &identifierBuffer, name + 2)) == NULL) {
            hasSuffix = 0;
            init_buffer(&identifierBuffer, identifier, sizeof(identifier));
            suffix = name;
            for (; *suffix == '_'; ++suffix)
                put_c(&identifierBuffer, '_');
            while ((ch = *suffix++) != 0) {
                if (ch == '_' && *suffix == '_') {
                    ++suffix;
                    while (*suffix == '_') {
                        put_c(&identifierBuffer, '_');
                        ++suffix;
                    }
                    hasSuffix = 1;
                    break;
                }
                put_c(&identifierBuffer, ch);
            }
            terminate(&identifierBuffer);
            if (!hasSuffix) {
                put_s(&outputBuffer, identifier);
                terminate(&outputBuffer);
                return;
            }
        }
        switch (*suffix) {
            case 'F':
                if (identifier[1] == 0 && (identifier[0] == '!' || identifier[0] == '~'))
                    break;
                put_s(&outputBuffer, identifier);
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
                    put_s(&outputBuffer, "::");
                    put_s(&outputBuffer, identifier);
                    if (*memberSuffix == 0) {
                        terminate(&outputBuffer);
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
