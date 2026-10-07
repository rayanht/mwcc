#include "compiler/common.h"
#include "driver/Targets.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/win32.h"
#include "compiler/CError.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLToolExec.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ClientGlue.h"
#include "driver/Help.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/Option.h"
#include "driver/TargetOptimizer-ppc-eabi.h"
#include "driver/ToolHelpers-cc.h"
#include "driver/ToolHelpers.h"
#include <stdio.h>
#include <setjmp.h>
#include <stdio.h>

#define pTool driverTool

#define va_start(ap, parm) ap = (char *)&parm + ((((char *)(&parm + 1) - (char *)&parm) + 3) / 4 * 4)

#include <string.h>
void Targets_FormatAndDispatchMessage(char *message, char *arguments)
{
    vsprintf(formatted_message, message, (char *)(unsigned int *)arguments);
    CWPluginsPrivate_InvokeMessageCallback((struct DispatchObject_0041b830 *)pluginPrivateContext, NULL,
                                           formatted_message, NULL, 2, 0);
    data_00587e1d = 1;
}

void Targets_ReportFormattedMessage(char *format, char *arguments)
{
    vsprintf(formatted_message, format, arguments);
    CWPluginsPrivate_InvokeMessageCallback(pluginPrivateContext, NULL, formatted_message, NULL, 1, 0);
}

#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

void format_and_dispatch_message(char *first, char *second)
{
    vsprintf(formatted_message, first, (char *)(unsigned int *)second);
    CWPluginsPrivate_InvokeMessageCallback((struct DispatchObject_0041b830 *)pluginPrivateContext, NULL,
                                           formatted_message, NULL, 0, 0);
}
#undef CERROR_FILE
#define CERROR_FILE __FILE__

void format_and_forward_message(char *message, unsigned int *arguments)
{
    vsprintf(formatted_message, message, (char *)arguments);
    fn_0041b8d0((CWPluginPrivateContext *)pluginPrivateContext, formatted_message, NULL);
}

void format_and_report_message(const char *text, va_list position)
{
    vsprintf(formatted_message, text, position);
    CWPluginsPrivate_CallCallback9(pluginPrivateContext, formatted_message, NULL, NULL, 0);
    data_00587e1d = 1;
}

void report_operating_system_error(char *name, DWORD value, unsigned int *result)
{
    vsprintf(formatted_message, name, (char *)result);
    CWPluginsPrivate_CallCallback9(pluginPrivateContext, formatted_message,
                                   "Operating system error:", (unsigned char *)OS_GetErrText(value), 0);
}

#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

char *Targets_GetResourceCString(SInt32 argument, char *buffer)
{
    CLIO_GetResourceCString(buffer, 0x2eea, argument);
    return buffer;
}
#undef CERROR_FILE
#define CERROR_FILE __FILE__

#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

void fn_0040ecb1(SInt32 messageId, ...)
{
    char message[256];
    va_list arguments;

    Targets_GetResourceCString(messageId, message);
    arguments = (char *)&messageId + (((char *)((SInt16 *)&messageId + 1) - (char *)&messageId + 3) / 4 * 4);
    Targets_FormatAndDispatchMessage(message, arguments);
}
#undef CERROR_FILE
#define CERROR_FILE __FILE__

#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

void Targets_ReportMessage(int messageId, ...)
{
    char buffer[256];
    va_list args;

    Targets_GetResourceCString(messageId, buffer);
    args = (char *)&messageId + (((char *)((SInt16 *)&messageId + 1) - (char *)&messageId + 3) / 4 * 4);
    Targets_ReportFormattedMessage(buffer, args);
}
#undef CERROR_FILE
#define CERROR_FILE __FILE__

void Targets_DispatchVariadicMessage(int messageId, ...)
{
    char buffer[256];
    va_list arguments;
    char *argumentAddress;
    char *argumentEnd;

    Targets_GetResourceCString(messageId, buffer);
    argumentAddress = (va_list)&messageId;
    argumentEnd = (va_list)&messageId + sizeof(short);
    arguments = (va_list)&messageId + (argumentEnd - argumentAddress + 3) / 4 * 4;
    format_and_dispatch_message(buffer, arguments);
}

unsigned char Targets_ReportOperatingSystemError(int resourceId, short errorCode, ...)
{
    unsigned char report_operating_system_error(char *message, int errorCode, va_list arguments);
    char message[256];
    va_list arguments;

    Targets_GetResourceCString(resourceId, message);
    va_start(arguments, errorCode);
    return report_operating_system_error(message, errorCode, arguments);
}
#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

void Targets_FormatAndForwardMessage(int messageId, ...)
{
    char buffer[256];
    va_list arguments;

    Targets_GetResourceCString(messageId, buffer);
    arguments = (char *)&messageId + ((((char *)((short *)&messageId + 1) - (char *)&messageId) + 3) / 4 * 4);
    format_and_forward_message(buffer, (unsigned int *)arguments);
}
#undef CERROR_FILE
#define CERROR_FILE __FILE__

void Targets_ForwardVarArgsAndLongjmp(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    format_and_report_message(fmt, ap);
    longjmp(plugin_request_jmp_buf, -123);
}

void append_coalesced_argument(short kind, char *text)
{
    long count;
    TokenText *last;
    TokenText *previous;
    TokenText *third;
    TokenText *fourth;
    TokenText *entry;
    if (coalesced_argument_count > 0) {
        last = token_texts + (coalesced_argument_count - 1);
    } else {
        last = NULL;
    }
    if (last != NULL && last->kind == 2 && last->text[0] == 0) {
        previous = NULL;
        third = NULL;
        fourth = NULL;
        if (coalesced_argument_count > 3) {
            fourth = token_texts + (coalesced_argument_count - 4);
        }
        if (coalesced_argument_count > 2) {
            third = token_texts + (coalesced_argument_count - 3);
        }
        if (coalesced_argument_count > 1) {
            previous = token_texts + (coalesced_argument_count - 2);
        }
        if (previous != NULL) {
            if (kind == 1 && (previous->kind == 5 || previous->kind == 4) && (third->kind != 2 || fourth->kind != 3)) {
                if (data_00588519 != 0) {
                    printf("Coalescing args with '%s'\n", Targets_GetTokenTextDescription(previous));
                }
                {
                    short previousKind = previous->kind;
                    coalesced_argument_count -= 2;
                    kind = previousKind;
                }
            } else if (previous->kind == 1) {
                if (!(kind != 5 && kind != 4)) {
                    if (data_00588519 != 0) {
                        printf("Coalescing args, removing '%s'\n", Targets_GetTokenTextDescription(previous));
                    }
                    coalesced_argument_count -= 2;
                }
            }
        }
    }
    count = coalesced_argument_count;
    if (count >= coalesced_argument_capacity) {
        token_texts = ToolHelpers_ResizeBuffer("argument list", token_texts, coalesced_argument_capacity + 16 << 3);
        coalesced_argument_capacity += 16;
    }
    entry = token_texts + coalesced_argument_count;
    entry->kind = kind;
    if (text) {
        entry->text = ClientGlue_DuplicateString(text);
    } else {
        entry->text = NULL;
    }
    coalesced_argument_count += 1;
}

void initialize_arguments(unsigned int value, char **otherValue)
{
    data_0057e06c = 0;
    token_cursor = NULL;
    data_0057e070 = value;
    data_0057e078 = (char **)otherValue;
    arg_index = 1;
}

void skip_whitespace_and_comments(void)
{
    for (;;) {
        while (*token_cursor && (__ctype_map[(unsigned char)*token_cursor] & 6))
            token_cursor++;
        if (*token_cursor == 0x1a)
            token_cursor = data_0057e1d0 + data_0057e1d4 - 1;
        if (token_cursor[0] == '\\' && token_cursor[1] == '#') {
            token_cursor++;
            return;
        }
        if (*token_cursor == '#' && (token_cursor > data_0057e1d0 ? token_cursor[-1] != '\\' : 1)) {
            while (*token_cursor && *token_cursor != '\n' && *token_cursor != '\r')
                token_cursor++;
            while (*token_cursor == '\r' || *token_cursor == '\n')
                token_cursor++;
        } else
            return;
    }
}

#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

Boolean initialize_file_token_cursor(char *name)
{
    char buffer[0x144];
    unsigned int error;
    Boolean result;

    if ((error = OS_MakeFileSpec(name, (OSSpec *)buffer)) != 0 ||
        (error = TargetOptimizer_ppc_eabi_InitOperationRecord((OSSpec *)buffer, NULL, 0,
                                                              (OperationRecord *)&operation_record)) != 0 ||
        (error = CLFileOps_AppendMemBuffer(&data_0057e1c0, &empty_string, 1)) != 0 ||
        (error = TargetOptimizer_ppc_eabi_GetMemBufferPtrAndSize((unsigned char *)&operation_record, &token_cursor,
                                                                 &data_0057e1d4)) != 0) {
        Targets_ReportOperatingSystemError(0x4a, error, name);
        result = 0;
    } else {
        data_0057e1d0 = token_cursor;
        skip_whitespace_and_comments();
        data_0057e06c = 1;
        result = 1;
    }
    return result;
}
#undef CERROR_FILE
#define CERROR_FILE __FILE__

unsigned int fn_0040f1df(void)
{
    data_0057e06c = (unsigned char)(0U);
    TargetOptimizer_ppc_eabi_UnloadOperationRecord((struct OperationRecord *)operation_record);
}

char *get_next_token(void)
{
    int quoted = 0;
    char *start;
    char *d;
    if (!*token_cursor)
        return NULL;
    start = d = token_cursor;
    while (*token_cursor) {
        if (!quoted && (__ctype_map[(unsigned char)*token_cursor] & 6))
            break;
        if (*token_cursor == '"') {
            quoted = !quoted;
            token_cursor++;
        } else if (token_cursor[0] == '\\' && token_cursor[1] == '"') {
            *d++ = '"';
            token_cursor += 2;
        } else
            *d++ = *token_cursor++;
    }
    if (*token_cursor)
        skip_whitespace_and_comments();
    *d = 0;
    return start;
}

char *get_next_arg(Boolean expand)
{
    char *arg;
    int len;
    for (;;) {
        if (!data_0057e06c) {
            char *p;
            len = 1;
            p = NULL;
            arg = data_0057e078[arg_index++];
            if ((p = strrchr(arg, '\r')) && p[1] == 0)
                *p = 0;
            if (arg[0] == '\\' && arg[1] == data_0058852c) {
                arg++;
                break;
            }
            if (!expand)
                break;
            if (*arg != data_0058852c) {
                if (!*data_00587eb0)
                    break;
                len = strlen(data_00587eb0);
                if (fn_004050e0(arg, data_00587eb0, len))
                    break;
                if (!arg[len])
                    break;
            }
            if ((p = strchr(arg + len, '=')))
                len = p + 1 - arg;
            arg += len;
            if (initialize_file_token_cursor(arg))
                continue;
            arg = NULL;
            break;
        } else {
            if ((arg = get_next_token()))
                break;
            fn_0040f1df();
        }
    }
    if (data_00588519)
        fprintf(stderr, "Got arg = '%s'\n", arg ? arg : "<NULL>");
    return arg;
}

unsigned char has_more_tokens_or_args(void)
{
    if (!data_0057e06c)
        return arg_index < data_0057e070;
    if (*token_cursor)
        return 1;
    return arg_index < data_0057e070;
}

void tokenize_arguments(void)
{
    char buf[0x1000];
    unsigned char flagA;
    unsigned char flagB;
    char *tok;
    char *src;
    char *dst;
    char ch;
    unsigned char kind;
    int i;
    int eq;

    flagA = 0;
    flagB = 0;
    while (has_more_tokens_or_args()) {
        src = get_next_arg(1);
        tok = src;
        if (src == NULL)
            break;
        dst = buf;
        *dst = 0;
        flagB = 0;
        if (*src != 0 && src[1] != 0 && strchr(data_00587eec, *src) != NULL) {
            if (flagA || flagB)
                append_coalesced_argument(1, NULL);
            *dst = src[1];
            dst++;
            *dst = 0;
            src += 2;
            flagA = 1;
            flagB = 0;
        } else {
            flagA = 0;
        }
        for (; src != NULL && *src != 0; src++) {
            ch = *src;
            if (ch == 0x5c && (src[1] == argument_space || src[1] == argument_space_char || src[1] == data_00588505)) {
                src++;
                ch = *src;
                ch = ch | 0x80;
            } else if (data_0054aa78 == 1 && ch == 0x3a && src[1] == 0x5c) {
                ch = ch | 0x80;
            }
            if (ch != argument_space && ch != argument_space_char && ch != data_00588505) {
                if ((ch & 0x7f) == argument_space || (ch & 0x7f) == argument_space_char || (ch & 0x7f) == data_00588505)
                    ch = ch & 0x7f;
                *dst = ch;
                dst++;
                if (dst >= buf + 0x1000)
                    fn_0040ecb1(2, tok, tok + strlen(tok) - 15, 0x1000);
                *dst = 0;
            } else {
                if (flagA)
                    append_coalesced_argument(3, buf);
                append_coalesced_argument(2, buf);
                if (ch == 0x2c)
                    kind = 5;
                else {
                    eq = (ch == 0x3d || ch == data_00588505);
                    if (eq)
                        kind = 4;
                    else
                        kind = 0;
                }
                append_coalesced_argument(kind, NULL);
                dst = buf;
                *dst = 0;
                if (ch == argument_space || ch == argument_space_char || ch == data_00588505)
                    flagA = 0;
                flagB = 1;
            }
        }
        if (flagA && dst > buf) {
            i = 0;
            if (flagB && strchr(data_00587eec, buf[0]) != NULL)
                i = 1;
            append_coalesced_argument(3, buf + i);
            i = 0;
            if (flagB && strchr(data_00587eec, buf[0]) != NULL)
                i = 1;
            append_coalesced_argument(2, buf + i + 1);
        } else {
            append_coalesced_argument(2, buf);
            append_coalesced_argument(1, NULL);
        }
    }
    if (flagA || flagB)
        append_coalesced_argument(1, NULL);
    append_coalesced_argument(0, NULL);
}

void Targets_ParseArguments(int argc, char **argv)
{
    int argumentIndex;
    int resultIndex;

    coalesced_argument_capacity = 0;
    coalesced_argument_count = 0;
    data_00588519 = 0;
    if (argc > 1) {
        if (memcmp(argv[1], "--parser-debug", 15) == 0) {
            data_00588519 = 1;
            memmove((argv + 1), (argv + 2), (argc - 1) * sizeof(*argv));
            argc--;
        }
    }
    if (data_0054aa78 == 0) {
        data_00587eec = "-";
        data_005876ac = (int)"=";
        help_option_separator = " ";
        argument_space = ',';
        argument_space_char = '=';
        data_00588505 = '=';
        data_0058852c = '@';
        data_00587eb0 = empty_string;
    } else if (data_0054aa78 == 1) {
        data_00587eec = "/-";
        data_005876ac = (int)":";
        help_option_separator = ":";
        argument_space = ',';
        argument_space_char = '=';
        data_00588505 = ':';
        data_0058852c = '@';
        data_00587eb0 = empty_string;
    } else if (data_0054aa78 == 2) {
        if (data_00587eec == NULL)
            data_00587eec = "-";
        if (data_005876ac == 0)
            data_005876ac = (int)"=";
        if (help_option_separator == NULL)
            help_option_separator = " ";
        if (argument_space == 0)
            argument_space = ',';
        if (argument_space_char == 0)
            argument_space_char = '=';
        if (data_00588505 == 0)
            data_00588505 = '=';
        if (data_0058852c == 0)
            data_0058852c = '@';
        if (data_00587eb0 == NULL)
            data_00587eb0 = empty_string;
    } else {
        Targets_ForwardVarArgsAndLongjmp("Unknown parser compatibility type (%d)\n", data_0054aa78);
    }
    if (data_00588519 != 0) {
        printf("Incoming arguments: \n");
        for (argumentIndex = 0; argumentIndex < argc; argumentIndex++)
            printf("[%s] ", argv[argumentIndex]);
        printf("\n");
    }
    initialize_arguments(argc, argv);
    tokenize_arguments();
    fn_0040f960();
    if (data_00588519 != 0) {
        for (resultIndex = 0; resultIndex < coalesced_argument_count; resultIndex++) {
            printf("TOKEN:  '%s'\n", Targets_GetTokenTextDescription(token_texts + resultIndex));
        }
    }
}

void Targets_FreeTokenText(void)
{
    struct TokenText *p;

    while (coalesced_argument_count > 0) {
        p = token_texts + --coalesced_argument_count;
        if (p->text != NULL)
            free(p->text);
    }
    if (coalesced_argument_capacity != 0)
        free(token_texts);
    coalesced_argument_capacity = 0;
}

void fn_0040f960(void)
{
    data_0057e1d8 = 0;
    return;
}

unsigned int fn_0040f969(void)
{
    int i = data_0057e1d8;
    if (i >= coalesced_argument_count) {
        return 0U;
    }
    return (unsigned int)(token_texts + data_0057e1d8);
}

TokenText *Targets_AdvanceArgument(void)
{
    int c;
    if ((c = data_0057e1d8) < coalesced_argument_count)
        data_0057e1d8++;
    return (TokenText *)fn_0040f969();
}

int Targets_IsValueNullOrZero(void)
{
    unsigned short *value = (unsigned short *)fn_0040f969();
    return value == NULL || *value == 0;
}

int Targets_IncrementGlobalOnNonzeroResult(void)

{
    int result = fn_0040f969();
    if (result != 0) {
        data_0057e1d8 = data_0057e1d8 + 1;
    }
    return result;
}

TokenText *Targets_DecrementCountAndGetTokenText(void)
{
    if (data_0057e1d8 > 0) {
        --data_0057e1d8;
        return (TokenText *)fn_0040f969();
    }
    return NULL;
}

#undef CERROR_FILE
#define CERROR_FILE "unknown.c"

/* Token kind and its printable text. */

char *Targets_GetTokenTextDescription(TokenText *token)
{
    char *result;
    int matchesKind;
    if (token->kind == 2)
        return token->text;
    if (token->kind == 3)
        result = "option";
    else if (token->kind == 5)
        result = "comma";
    else {
        matchesKind = (data_0054aa78 == 1) && (token->kind == 4);
        if (matchesKind)
            result = "colon or equals";
        else {
            matchesKind = (data_0054aa78 != 1) && (token->kind == 4);
            if (matchesKind)
                result = "equals";
            else if (token->kind == 1)
                result = "end of argument";
            else if (token->kind == 0)
                result = "end of command line";
            else
                result = "<error>";
        }
    }
    return result;
}

#undef CERROR_FILE
#define CERROR_FILE __FILE__
char *Targets_CopyTokenText(TokenText *arg, char *buffer, int maxlen, Boolean warn)
{
    char *d = buffer;
    int n = 0;
    char *s;
    if (arg->kind == 2 || arg->kind == 3)
        s = arg->text;
    else
        s = Targets_GetTokenTextDescription(arg);
    while (*s && n++ < maxlen)
        *d++ = *s++;
    if (n < maxlen)
        *d = 0;
    else {
        d[-1] = 0;
        if (warn)
            Targets_ReportMessage(0x38, buffer, s + strlen(s) - (maxlen > 32 ? 32 : maxlen), maxlen);
    }
    return buffer;
}

void grow_ptr_list(PtrList *list)
{
    int i;
    if (!list->items || list->count + 1 >= list->size) {
        i = list->size;
        list->size += 16;
        list->items = ToolHelpers_ResizeBuffer("argument list", list->items, (list->size + 1) * 4);
        for (; i <= list->size; i++)
            list->items[i] = NULL;
    }
    list->count++;
}
void append_text_to_ptrlist_item(struct PtrList *list, char *text)
{
    char *value = text;
    char **slot = &list->items[list->count];
    if (!*slot) {
        *slot = ClientGlue_DuplicateString(value);
    } else {
        char *string = value;
        *slot = ToolHelpers_ResizeBuffer("command line", *slot, strlen(*slot) + strlen(string) + 1);
        strcpy(*slot + strlen(*slot), string);
    }
}

void Targets_InitPtrList(PtrList *state)
{
    state->count = 0;
    state->size = 0;
    state->items = NULL;
    grow_ptr_list(state);
}

void fn_0040fbe1(PtrList *list, short kind, char *text)
{
    if (kind == 0)
        terminate_ptr_list(list);
    else if (kind == 1) {
        if (list->items && list->items[list->count])
            grow_ptr_list(list);
    } else if (kind == 2)
        append_text_to_ptrlist_item(list, text);
    else if (kind == 3)
        append_text_to_ptrlist_item(list, "-");
    else if (kind == 4)
        append_text_to_ptrlist_item(list, "=");
    else if (kind == 5)
        append_text_to_ptrlist_item(list, ",");
}

void terminate_ptr_list(PtrList *list)
{
    grow_ptr_list(list);
    list->items[list->count] = NULL;
}

/* Records used to describe a zero-terminated integer sequence. */

void Targets_InitIntegerSequenceResult(PtrList *source, IntegerSequenceResult *result)
{
    unsigned int value;
    result->count = 1;
    result->entries = (int *)source->items;
    while (result->entries[result->count] != 0) {
        result->count++;
    }
    value = 0;
    result->value = value;
    return;
}

int Targets_SetTool(int *tool)
{
    pTool = tool;
    ((pTool[6] && pTool[7]) ? (void)0
                            : CLIO_ReportAssertionFailure("pTool->toolInfo && pTool->copyright", "Targets.c", 16));
    return 1;
}

#define MATCH(x, y) ((x) == 0x2a2a2a2a || (y) == 0x2a2a2a2a || (y) == (x))
Boolean Targets_MatchTool(int cpu, int os, int lang, int type)
{
    if (!pTool) {
        Targets_ForwardVarArgsAndLongjmp("No options loaded for command line\n");
        return 0;
    }
    if (MATCH(cpu, pTool[0]) && MATCH(os, pTool[1]) && MATCH(lang, pTool[2]) && MATCH(type, pTool[3]))
        return 1;
    return 0;
}

#undef CERROR_FILE
#define CERROR_FILE "options.c"

Boolean Targets_MatchCommandLineOptions(int argc, char **argv)
{
    int i, j;
    Boolean ok;

    if (driverTool == NULL)
        Targets_ForwardVarArgsAndLongjmp("No options loaded for command line\n");

    for (i = 0; i < argc; i++) {
        for (j = 0; j < driverTool[4]; j++) {
            if (ClientGlue_CompareLowercaseStrings(((char **)driverTool[5])[j], argv[i]) == 0)
                break;
        }
        if (j >= driverTool[4])
            break;
    }

    if (i >= argc)
        ok = 1;
    else
        ok = 0;
    return ok;
}

#undef CERROR_FILE
#define CERROR_FILE __FILE__

/* Indexed values with an opaque 32-byte prefix. */

int Targets_RegisterOptionLists(void)
{
    int index;
    Option_ResetOptionLists();
    for (index = 0; index < ((struct IndexedValueTable *)driverTool)->count; index++) {
        OptionList **values = (OptionList **)((struct IndexedValueTable *)driverTool)->values;
        Option_RegisterOptionList(values[index]);
    }
    return 1;
}
