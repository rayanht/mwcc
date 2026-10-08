#include "compiler/common.h"
#include "driver/CLIO.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "driver/AssertionFailure.h"
#include "driver/CLBrowser.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLMain.h"
#include "driver/CLPlugins.h"
#include "driver/CLPrefs.h"
#include "driver/Generic.h"
#include "driver/MacFileTypes.h"
#include "driver/MemUtils.h"
#include "driver/StringUtils.h"
#include "driver/TargetOptimizer-ppc-eabi.h"
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char data_005880e0[260];

static char data_0057eb68;
static char data_0057eb69;
static int data_0057eb6c; /* (its users were stripped) */
static UInt8 data_0057eb70;
static UInt8 data_0057eb71;
static OSSpec cachedRecordSpec;
static OSSpec data_0057ecb6;
static char specs_equal;
static char data_0057edfb;
static char data_0057edfc;
static char data_0057edfd[256];

/* The lines written since the last page prompt. */
static int data_0054b988 = 0;

int report_user_break(unsigned int value)
{
    if (value <= 1) {
        CLIO_WriteFormattedText("\nUser break, cancelled...\n");
        clState.userBreak = 1;
        return 1;
    }
    return 0;
}

void initialize_console(void)
{
    HANDLE consoleOutput;
    BOOL gotBufferInfo;
    PHANDLER_ROUTINE handler;
    _CONSOLE_SCREEN_BUFFER_INFO bufferInfo;

    consoleOutput = GetStdHandle((DWORD)-11);
    handler = (PHANDLER_ROUTINE)report_user_break;
    SetConsoleCtrlHandler(handler, 1);
    data_0057eb68 = 0;
    gotBufferInfo = GetConsoleScreenBufferInfo(consoleOutput, &bufferInfo);
    if (!gotBufferInfo) {
        optsEnvir.rows = 0;
        optsEnvir.cols = 80;
    } else {
        optsEnvir.rows = bufferInfo.dwSize.Y;
        optsEnvir.cols = (unsigned short)bufferInfo.dwSize.X;
        if (optsEnvir.cols > 256) {
            optsEnvir.cols = 256;
        }
    }
}

void clear_global(void)

{
    *(unsigned char *)NULL = 0;
    return;
}

void CLIO_ExchangeClearGlobal(void)
{
    signal(SIGABRT, (__signal_func_ptr)clear_global);
}

char CLIO_InitializeStreamBuffering(void)
{
    data_0057eb68 = 0;
    data_0057eb69 = 0;
    data_0054b988 = 0;
    initialize_console();
    if (data_0057eb68 != 0) {
        setvbuf(stdout, NULL, 2, 4096U);
        setvbuf(stderr, NULL, 2, 4096U);
    } else {
        setvbuf(stdout, NULL, 1, 4096U);
        setvbuf(stderr, NULL, 1, 4096U);
    }
    fn_004151c0();
    return 1;
}

unsigned char fn_00414e20(void)
{
    if (data_0057eb69) {
        clear_global_byte();
    }
    fn_004151e0();
    return 1;
}

unsigned char clear_global_byte(void)
{
    data_0057eb69 = 0U;
    return 1U;
}

static inline Boolean writeBufferLines(struct _FILE *fp, StorageHandle *bufp, char *cursor, SInt32 len)
{
    char *lineEnd;
    SInt32 lineLength;
    if (len <= 0)
        return 1;
    else {
        while (cursor < bufp->data + len) {
            lineEnd = cursor;
            while (*lineEnd != 0 && *lineEnd != '\n' && *lineEnd != '\r')
                lineEnd++;
            lineLength = lineEnd - cursor;
            if (lineLength != 0) {
                if (fwrite(cursor, lineLength, 1, fp) != 1) {
                    CLErrors_ReportFormattedOSError(10, errno);
                    return 0;
                }
            }
            if (fn_004151f0())
                break;
            if (fwrite("\r\n", 2, 1, fp) != 1) {
                CLErrors_ReportFormattedOSError(10, errno);
                return 0;
            }
            if (*lineEnd != 0) {
                if (*lineEnd++ == '\r' && *lineEnd == '\n')
                    lineEnd++;
            }
            cursor = lineEnd;
        }
    }
    return 1;
}

Boolean write_text_buffer(struct _FILE *fp, StorageHandle *bufp, SInt32 len)
{
    Boolean ok;
    char *cursor;
    if (fp != stdout && fp != stderr)
        setvbuf(fp, NULL, 2, 0x1000);
    fn_00413a00(bufp);
    cursor = bufp->data;
    if (cursor[len - 1] == 0)
        len--;
    ok = writeBufferLines(fp, bufp, cursor, len);
    fn_00413a50(bufp);
    fflush(fp);
    if (fp != stdout && fp != stderr)
        setvbuf(fp, NULL, 0, 0x1000);
    return ok;
}

static const char *data_0054b9c0 = "===============\n";

unsigned char CLIO_WriteStorageToStdout(StorageHandle *first, SInt32 second, unsigned int reset)
{
    FILE *state;
    if ((unsigned char)reset)
        fprintf(stdout, data_0054b9c0);
    if (!write_text_buffer(state = stdout, first, second)) {
        return 0;
    }
    if ((unsigned char)reset)
        fprintf(state, data_0054b9c0);
    fflush(state);
    return 1;
}

Boolean CLIO_WriteTextFile(OSSpec *fileRef, StorageHandle *text, SInt32 textLength, SInt32 fileType, SInt32 creator)
{
    char path[256];
    FILE *file;

    OS_SpecToString(fileRef, path, 0x104);
    file = fopen(path, "w+b");
    if (file == NULL) {
        CLErrors_ReportFormattedOSError(8, errno, path);
        return 0;
    }
    write_text_buffer(file, text, textLength);
    fclose(file);
    fn_00421d30(fileRef, fileType, creator);
    return 1;
}

unsigned char fn_00415090(OSSpec *fileSpec, unsigned int fileType, unsigned int creator, OSHandle *buffer)
{
    struct OperationRecord recovery;
    unsigned int result;
    if ((result = TargetOptimizer_ppc_eabi_InitOperationRecord(fileSpec, buffer, 1U, &recovery)) != 0U ||
        (result = TargetOptimizer_ppc_eabi_UnloadOperationRecord(&recovery)) != 0U ||
        (result = fn_00421d30(fileSpec, fileType, creator)) != 0U) {
        char *message = OS_SpecToString(fileSpec, data_005880e0, 260U);
        CLErrors_ReportOSError(8U, result, message);
        return 0;
    }
    return 1;
}

unsigned char CLIO_AppendStorageToFile(OSSpec *fileSpec, struct StorageHandle *storage, SInt32 textLength,
                                       SInt32 fileType, SInt32 fileCreator)
{
    char path[256];
    FILE *file;

    OS_SpecToString(fileSpec, path, 0x104);
    file = fopen(path, "a+b");
    if (file == NULL) {
        CLErrors_ReportFormattedOSError(8, errno, path);
        return 0;
    }
    write_text_buffer(file, storage, textLength);
    fclose(file);
    fn_00421d30(fileSpec, fileType, fileCreator);
    return 1;
}

void fn_004151c0(void)
{
    return;
}

void fn_004151d0(int)
{
    return;
}

void fn_004151e0(void)
{
    return;
}

unsigned char fn_004151f0(void)
{
    fn_004151d0(4);
    return clState.userBreak;
}

void reset_column_and_append_prefix(struct ByteBuffer *node)
{
    node->column = 0;
    if ((optsEnvir.underIDE == '\0') && (node->prefix != NULL)) {
        append_text(node, node->prefix);
    }
}

void append_text(struct ByteBuffer *buffer, char *text)
{
    int length;
    length = strlen(text);
    if (buffer->size + length >= buffer->capacity) {
        buffer->capacity = buffer->capacity * 2 + length;
        if (buffer->callerOwned != 0) {
            char *oldData = buffer->data;
            buffer->data = xmalloc("message buffer", buffer->capacity);
            memcpy(buffer->data, oldData, buffer->size);
        } else {
            buffer->data =
                xrealloc("message buffer", (buffer->callerOwned != 0) ? NULL : buffer->data, buffer->capacity);
        }
        buffer->callerOwned = 0;
    }
    memcpy(buffer->data + buffer->size, text, length);
    buffer->size += length;
    buffer->column += length;
}

void wrap_line(struct ByteBuffer *p)
{
    char buf[256];
    SInt32 n = 0;

    if (optsCmdLine.noWrapOutput || p->prefix == NULL || strlen(p->prefix) > optsEnvir.cols / 2)
        return;

    while (n < optsEnvir.cols - 1 && p->column > strlen(p->prefix)) {
        char c = p->data[p->size - 1];
        if (c == ' ' || c == '/' || c == '-')
            break;
        buf[n] = c;
        n++;
        p->size--;
        p->column--;
    }

    if (p->column <= strlen(p->prefix)) {
        while (p->column < optsEnvir.cols - 1 && n > 0) {
            n--;
            append_byte(p, buf[n]);
        }
    }

    append_prefix_separator(p);

    while (n > 0 && p->capacity > 0) {
        n--;
        append_byte(p, buf[n]);
    }
}

void append_prefix_separator(struct ByteBuffer *state)
{
    if (state->prefix != NULL) {
        append_byte(state, '\n');
    } else {
        append_byte(state, ' ');
    }
    reset_column_and_append_prefix(state);
}

void append_byte(struct ByteBuffer *buffer, char value)
{
    char *old_data;
    if (buffer->size >= buffer->capacity) {
        buffer->capacity <<= 1;
        if (buffer->callerOwned) {
            old_data = buffer->data;
            buffer->data = xmalloc("message buffer", buffer->capacity);
            memcpy(buffer->data, old_data, buffer->size);
        } else {
            buffer->data = xrealloc("message buffer", buffer->data, buffer->capacity);
        }
        buffer->callerOwned = 0;
    }
    buffer->data[buffer->size++] = (unsigned char)value;
    buffer->column++;
}

char *format_prefixed_text(char *output, int remaining, char *prefix, char *format, char **args)
{
    char *text;
    struct ByteBuffer state;
    int column;

    state.data = output;
    state.size = 0;
    state.capacity = remaining;
    state.column = 0;
    state.callerOwned = 1;
    state.prefix = prefix;
    reset_column_and_append_prefix(&state);

    while (*format != '\0' && state.capacity > 0) {
        if (*format == '%') {
            args++;
            text = *(char **)((char *)args - 4);
            while (*text != '\0' && state.capacity > 0) {
                if (*text == '\r' || *text == '\n') {
                    append_prefix_separator(&state);
                    text++;
                } else if (*text == '\t') {
                    if (state.capacity > 0) {
                        do {
                            append_byte(&state, ' ');
                        } while ((state.column & 7) != 0);
                    }
                    text++;
                } else {
                    append_byte(&state, *text++);
                    if (optsCmdLine.noWrapOutput == 0 && state.prefix != NULL && state.column >= optsEnvir.cols - 1)
                        wrap_line(&state);
                }
            }
            format++;
        } else if (*format == '\r' || *format == '\n') {
            format++;
            append_prefix_separator(&state);
        } else {
            append_byte(&state, *format++);
            if (optsCmdLine.noWrapOutput == 0 && state.column >= optsEnvir.cols - 1)
                wrap_line(&state);
        }
    }

    if (state.prefix != NULL) {
        if ((column = state.column) == strlen((const char *)state.prefix)) {
            state.size -= column;
            state.column = 0;
        }
    }
    append_byte(&state, 0);
    return (char *)state.data;
}

char *forward_format_arguments(char *output, int size, char *prefix, char *format, ...)
{
    char **arguments = &format + (((char *)(&format + 1) - (char *)&format + 3) / 4);
    return format_prefixed_text(output, size, prefix, format, arguments);
}

static char data_0054b9dc = 0;
static char *data_0054ba14 = "\n[Press enter for next page, 'q'+enter to quit]\r"; /* (its users were stripped) */

unsigned int write_text_to_stdout_or_stderr(int unused, short messageType, const char *textAddress)
{
    FILE *output;
    int type;
    const char *cursor;
    const char *lineEnd;
    char mode;

    if ((mode = optsCmdLine.stderr2stdout) == 1 || mode == 2 || (type = messageType) == 1 || type == 5) {
        output = stdout;
    } else if (type == 2 || type == 3 || type == 4) {
        output = stderr;
    } else {
        CLIO_ReportAssertionFailure("0", "CLIO.c", 845);
    }
    if (data_0054b9dc == 0 && data_0057eb68 == 0) {
        data_0054b9dc = 1;
        data_0054b988 = 0;
    }
    cursor = textAddress;
    while (*cursor != 0) {
        lineEnd = cursor;
        while (*lineEnd != 0 && *lineEnd != '\n' && *lineEnd != '\r') {
            ++lineEnd;
        }
        if ((int)(lineEnd - cursor) != 0 && fwrite(cursor, lineEnd - cursor, 1, output) != 1) {
            clState.userBreak = 1;
        }
        if (*lineEnd != 0) {
            data_0054b988 += 1;
            fwrite("\r\n", 2, 1, output);
            while (*lineEnd == '\n' || *lineEnd == '\r') {
                ++lineEnd;
            }
        }
        cursor = lineEnd;
    }
    return (unsigned int)cursor;
}

static char *diagnostic_level_names[] = {"", "Note", "Warning", "Error", "Alert", "Status"};

void update_cached_specs(OSSpec *recordAddress)
{
    if (recordAddress == NULL)
        return;
    specs_equal = OS_EqualSpec(recordAddress, recordAddress + 1);
    if (data_0057eb70 == 0 || OS_EqualSpec(recordAddress, &cachedRecordSpec) == 0) {
        const OSSpec *records = recordAddress;
        OSSpec *cachedRecord = &cachedRecordSpec;
        data_0057edfb = 1;
        *cachedRecord = *records;
        data_0057eb70 = 1;
    } else {
        data_0057edfb = 0;
    }
    if (data_0057eb71 == 0 || OS_EqualSpec(recordAddress + 1, &data_0057ecb6) == 0) {
        const OSSpec *records = recordAddress;
        OSSpec *cachedRecord = &data_0057ecb6;
        data_0057edfc = 1;
        *cachedRecord = records[1];
        data_0057eb71 = 1;
    } else {
        data_0057edfc = 0;
    }
}

static inline void emitDiagnosticText(short kind, const char *text)
{
    write_text_to_stdout_or_stderr(0, kind, text);
}

static inline void formatDiagnosticDetail(char *buffer, const char *format, const char *label, char *location)
{
    sprintf(buffer, format, label, location);
}

char *make_source_position_carets(DiagnosticSourcePosition *sourcePosition)
{
    int column;
    int length;

    data_0057edfd[0] = 0;
    column = sourcePosition->column;
    column %= optsEnvir.cols;
    if ((column >= 0) && ((unsigned)column < 0x100)) {
        length = (int)sourcePosition->length;
        if (0x100 < (unsigned)(length + column)) {
            length = 0x100 - column;
        }
        if (length == 0) {
            length = 1;
        }
        memset(data_0057edfd, ' ', column);
        memset(data_0057edfd + column, '^', length);
        data_0057edfd[column + length] = 0;
    }
    return data_0057edfd;
}

static inline char *formatDiagnosticPath(const char *path)
{
    return OS_SpecToStringRelative((OSSpec *)path, NULL, data_005880e0, 260);
}

unsigned char nonmatching_plugin_with_clear_high_bit(Plugin *type)
{
    struct CLTarget *entries;
    const CWObjectFlags *flags;

    flags = CLPlugins_GetObjectFlags(type);
    if ((flags->flags & 0x80000000U) != 0U)
        return 0;

    entries = default_target;

    if (type ==
        CLPlugins_FindMatchingTargetPlugin(NULL, entries->cpu, entries->os, clState.plugintype, clState.language))
        return 0;
    return 1;
}

char *get_plugin_type_name(Plugin *tool)
{
    PluginDesc *identification;

    if (tool != NULL) {
        identification = CLPlugins_GetPluginDesc(tool);
        switch (identification->type) {
            case 'cldr':
                return "Driver";
            case 'Pars':
                return "Usage";
            case 'Comp':
                if (identification->lang == 'c++ ' || identification->lang == 'pasc')
                    return "Compiler";
                if (identification->lang == 'Asm ')
                    return "Assembler";
                if (nonmatching_plugin_with_clear_high_bit(tool) != 0)
                    return "Importer";
                return "Compiler";
            case 'Link':
                return "Linker";
        }
    }
    return "Driver";
}

unsigned char *select_plugin_type_data(Plugin *value)
{
    PluginDesc *record;
    if (value != NULL) {
        record = CLPlugins_GetPluginDesc(value);
        switch (record->type) {
            case 0x50617273:
                return (unsigned char *)"parsing";
            case 0x436F6D70:
                return (unsigned char *)"compiling";
            case 0x4C696E6B:
                return (unsigned char *)"linking";
        }
    }
    return (unsigned char *)"processing";
}

void print_diagnostic(Plugin *object, DiagnosticSourcePosition *dump, SInt32 diagnosticCode, SInt16 level,
                      char *messageArg1, char **messageArg2)
{
    char buffer[256];
    char *message;

    message = format_prefixed_text(buffer, sizeof(buffer), "#   ", messageArg1, messageArg2);
    if (level != 5) {
        emit_formatted_message(level, "### %s %s %s:\n", clState.programName, get_plugin_type_name(object),
                               diagnostic_level_names[level]);
    }
    if (dump != NULL) {
        emit_formatted_message(level, "#\t%s\n", dump->sourceLine);
        emit_formatted_message(level, "#\t%s\n", make_source_position_carets(dump));
    }
    write_text_to_stdout_or_stderr(0, level, message);
    if (dump != NULL) {
        emit_formatted_message(level, "#----------------------------------------------------------\n");
        emit_formatted_message(level, "    File \"%s\"; Line %d\n", OS_SpecToString(&dump->file, data_005880e0, 0x104),
                               dump->line);
        update_cached_specs(&dump->primaryFile);
        if (specs_equal == 0) {
            emit_formatted_message(level, "#\twhile %s \"%s\"\n", select_plugin_type_data(object),
                                   OS_SpecToString(&dump->primaryFile, data_005880e0, 0x104));
        }
        emit_formatted_message(level, "#----------------------------------------------------------\n");
    }
    if (message != buffer) {
        free(message);
    }
}

void emit_formatted_diagnostic(Plugin *source, DiagnosticSourcePosition *diagnostic, int unused, short kind,
                               char *format, char **arguments)
{
    char *message;
    char *cursor;
    short limit;
    char detail[324];
    char buffer[256];

    message = format_prefixed_text(buffer, sizeof(buffer), "#   ", format, arguments);
    if (diagnostic != NULL) {
        emit_formatted_message(kind, "### %s %s:\n", clState.programName, get_plugin_type_name(source));
    } else if (kind != 5) {
        emit_formatted_message(kind, "### %s %s %s:\n", clState.programName, get_plugin_type_name(source),
                               diagnostic_level_names[kind]);
    }
    if (diagnostic != NULL) {
        update_cached_specs(&diagnostic->primaryFile);
        if (data_0057edfc != 0 || (data_0057edfb != 0 && specs_equal != 0)) {
            if (specs_equal != 0) {
                formatDiagnosticDetail(detail, "#%8s: %s\n", "File",
                                       OS_SpecToStringRelative(&data_0057ecb6, NULL, data_005880e0, 260));
            } else {
                formatDiagnosticDetail(detail, "#%8s: %s\n", "In",
                                       OS_SpecToStringRelative(&data_0057ecb6, NULL, data_005880e0, 260));
            }
            emitDiagnosticText(kind, detail);
        }
        if (data_0057edfb != 0 && specs_equal == 0) {
            formatDiagnosticDetail(detail, "# %7s: %s\n", "From",
                                   OS_SpecToStringRelative(&cachedRecordSpec, NULL, data_005880e0, 260));
            emitDiagnosticText(kind, detail);
        }
        if (data_0057edfc != 0 || data_0057edfb != 0) {
            cursor = detail + 2;
            while (*cursor != 0 && *cursor != '\n') {
                *cursor = '-';
                ++cursor;
            }
            *cursor = 0;
            if (cursor - detail >= (limit = optsEnvir.cols) - 1) {
                detail[limit - 1] = 0;
            }
            strcat(detail, "\n");
            emitDiagnosticText(kind, detail);
        }
        emit_formatted_message(kind, "#%8d: %s\n", diagnostic->line, diagnostic->sourceLine);
        emit_formatted_message(kind, "#%8s: %s\n", diagnostic_level_names[kind],
                               make_source_position_carets(diagnostic));
    }
    emitDiagnosticText(kind, message);
    if (message != buffer) {
        free(message);
    }
}

void format_and_print_message(Plugin *type, DiagnosticSourcePosition *obj, int messageCode, SInt16 kind,
                              char *formatFlags, char **formatOptions)
{
    char formattedBuffer[256];
    char messageBuffer[256];
    char *formatted;
    char *message;
    char *line;
    char *end;

    if (obj != NULL) {
        if (kind != 3) {
            message = mprintf(messageBuffer, sizeof(messageBuffer),
                              "%s:%d:%s: ", OS_SpecToStringRelative(&obj->file, NULL, data_005880e0, 0x104), obj->line,
                              diagnostic_level_names[kind]);
        } else {
            message = mprintf(messageBuffer, sizeof(messageBuffer),
                              "%s:%d: ", OS_SpecToStringRelative(&obj->file, NULL, data_005880e0, 0x104), obj->line);
        }
    } else {
        if (kind != 5) {
            message = mprintf(messageBuffer, sizeof(messageBuffer), "%s: ", clState.programName);
        } else {
            messageBuffer[0] = 0;
            message = messageBuffer;
        }
    }

    formatted = format_prefixed_text(formattedBuffer, sizeof(formattedBuffer), message, formatFlags, formatOptions);
    if (message != messageBuffer)
        free(message);

    line = formatted;
    while (*line != 0) {
        end = line;
        while (*end != 0 && *end != '\n')
            end++;
        emit_formatted_message(kind, "%.*s\n", end - line, line);
        if (*end != 0)
            end++;
        line = end;
    }
    if (formatted != formattedBuffer)
        free(formatted);
}

void print_diagnostic_with_details(Plugin *unused, struct DiagnosticDetails *details, int unused2, short diagnostic,
                                   char *formatArg, char **formatArgs)
{
    char *message;
    short savedDiagnostic;
    char *detailMessage;
    char buffer[256];

    savedDiagnostic = optsEnvir.cols;
    message = format_prefixed_text(buffer, sizeof(buffer), "           ", formatArg, formatArgs);
    optsEnvir.cols = savedDiagnostic;
    emit_formatted_message(diagnostic, "%8s : %s\n", diagnostic_level_names[diagnostic], message + 11);
    if (message != buffer) {
        free(message);
    }
    if (details != NULL) {
        detailMessage = forward_format_arguments(buffer, sizeof(buffer), "\t", "%", details->formatValue);
        emit_formatted_message(diagnostic, "%s line %d%s\n",
                               OS_SpecToStringRelative((OSSpec *)details->text, NULL, data_005880e0, 260),
                               details->diagnosticValue, detailMessage);
        if (detailMessage != buffer) {
            free(detailMessage);
        }
    }
}

void print_pipe_delimited_diagnostic(Plugin *kind, DiagnosticSourcePosition *record, int unused, short level,
                                     char *argument1, char **argument2)
{
    char *message;
    char messageBuffer[256];

    emit_formatted_message(level, "%s|%s|%s\n", clState.programName, get_plugin_type_name(kind),
                           diagnostic_level_names[level]);
    if (record != NULL) {
        emit_formatted_message(level, "(%s|%d|%d|%d|%d|%d)\n", OS_SpecToString(&record->file, data_005880e0, 260),
                               record->line, record->column, record->length, record->selectionOffset,
                               record->selectionLength);
        emit_formatted_message(level, "=%s\n", record->sourceLine);
    }
    message = format_prefixed_text(messageBuffer, sizeof(messageBuffer), ">", argument1, argument2);
    emit_formatted_message(level, "%s\n", message);
    if (message != messageBuffer) {
        free(message);
    }
}

void emit_formatted_message(short kind, char *format, ...)
{
    char buffer[256];
    char *message;
    int argumentSize;
    va_list args;

    argumentSize = (va_list)(&format + 1) - (va_list)&format;
    args = (va_list)&format + ((argumentSize + 3) / 4) * 4;
    message = mvprintf(buffer, sizeof(buffer), format, args);
    write_text_to_stdout_or_stderr(0, kind, message);
    if (message != buffer)
        free(message);
}

void CLIO_FormatAndDispatchText(char *fmt, ...)
{
    char buf[256];
    char *text;
    va_list args;

    args = (va_list)&fmt + (((va_list)(&fmt + 1) - (va_list)&fmt + 3) / 4 * 4);
    text = mvprintf(buf, sizeof(buf), fmt, args);
    write_text_to_stdout_or_stderr(0, 1, text);
    if (text != buf)
        free(text);
}

void CLIO_WriteFormattedText(const char *format, ...)
{
    char buffer[256];
    va_list args;
    char *text;

    args = (va_list)&format + (((va_list)(&format + 1) - (va_list)&format) + 3) / 4 * 4;
    text = mvprintf(buffer, sizeof(buffer), format, args);
    write_text_to_stdout_or_stderr(0, 3, text);
    if (text != buffer)
        free(text);
}

void extract_diagnostic_source_line(DiagnosticSourcePosition *info)
{
    char *start, *end;
    SInt32 len, i;
    if (info->sourceLine != NULL && *info->sourceLine != '\0') {
        start = info->sourceLine + info->column;
        end = info->sourceLine + info->column + info->length - 1;
        if (end < start)
            end = start;
        while (start > info->sourceLine && start[-1] != '\r')
            start--;
        while (*end != '\0' && *end != '\r' && *end != '\n')
            end++;
        len = end - start;
        info->sourceLine = xmalloc("text buffer", len + 1);
        strncpy(info->sourceLine, start, len);
        info->sourceLine[len] = '\0';
        for (i = 0; i < len; i++) {
            if (info->sourceLine[i] < ' ' || info->sourceLine[i] >= 0x7f)
                info->sourceLine[i] = ' ';
        }
    } else {
        info->sourceLine = NULL;
    }
}

short CLIO_ReportDiagnostic(Plugin *type, DiagnosticSourcePosition *record, int message, short severity, char *argument,
                            ...)
{
    unsigned int language;
    char **args;
    unsigned short limit;
    long diagnosticKind;
    union {
        DiagnosticSourcePosition record;
        DiagnosticSourcePosition dump;
        DiagnosticSourcePosition msg;
        struct DiagnosticDetails details;
        DiagnosticSourcePosition diagnostic;
    } diagnostic;
    if (severity == 2) {
        if (type != NULL) {
            language = CLPlugins_GetType(type);
        } else {
            language = 1668047986;
        }
        if (optsCmdLine.noWarnings != 0) {
            return 0;
        }
        if ((language == 1668047986 || language == 1348563571) && optsCmdLine.noCmdLineWarnings != 0) {
            return 0;
        }
        if (optsCmdLine.warningsAreErrors != 0) {
            severity = 3;
        }
    }
    diagnosticKind = severity;
    if (diagnosticKind == 3 && clState.withholdErrors != 0) {
        return 0;
    }
    if (diagnosticKind == 2 && clState.withholdWarnings != 0) {
        return 0;
    }
    if (record != NULL) {
        diagnostic.record = *record;
        extract_diagnostic_source_line(&diagnostic.record);
    }
    args = &argument + (((char *)(&argument + 1) - (char *)&argument + 3) / 4);
    {
        if ((language = optsCmdLine.msgStyle) == 2) {
            print_diagnostic(type, record != NULL ? &diagnostic.dump : NULL, message, severity, argument, args);
        } else if (language == 1) {
            format_and_print_message(type, record != NULL ? &diagnostic.msg : NULL, message, severity, argument, args);
        } else if (language == 3) {
            print_diagnostic_with_details(type, record != NULL ? &diagnostic.details : NULL, message, severity,
                                          argument, args);
        } else if (language == 4) {
            print_pipe_delimited_diagnostic(type, record != NULL ? &diagnostic.record : NULL, message, severity,
                                            argument, args);
        } else {
            emit_formatted_diagnostic(type, record != NULL ? &diagnostic.diagnostic : NULL, message, severity, argument,
                                      args);
        }
    }
    if (record != NULL && diagnostic.record.sourceLine != NULL) {
        free(diagnostic.record.sourceLine);
    }
    if (diagnosticKind == 3) {
        if ((limit = optsCmdLine.maxErrors) != 0 && ++clState.countErrors >= limit) {
            clState.withholdErrors = 1;
            if (optsCompiler.noFail == 0) {
                CLErrors_ForwardMessage(70);
                clState.userBreak = 1;
            } else {
                CLErrors_ForwardMessage(71);
            }
        }
    }
    if (diagnosticKind == 2) {
        if ((limit = optsCmdLine.maxWarnings) != 0 && ++clState.countWarnings >= limit) {
            clState.withholdWarnings = 1;
            CLErrors_ForwardMessage(72);
        }
    }
    return severity;
}

int CLIO_CompareStringsIgnoreCase(char *left, char *right)
{
    do {
        if (tolower(*left) != tolower(*right++)) {
            return 1;
        }
    } while (*left++ != '\0');
    return 0;
}

static int data_0054bc34 = 0; /* (its users were stripped) */

/* The linker stripped the function that used these literals; they stay in the unit's .data. */
static void CLIO_StrippedLiterals(char *buffer, char *text)
{
    sprintf(buffer, "%s%s", " ... ", text);
}

NameTableEntry *create_data_block(char *name, const void *source, unsigned int size)
{
    NameTableEntry *block;
    char *end;
    block = xmalloc(NULL, 16U);
    if (!block)
        return block;
    block->name = xstrdup(name);
    block->handles.storage.destination = (StorageHandle *)Memory_NewHandle(size);
    if (!block->handles.storage.destination) {
        CLIO_FormatAndDispatchText("\nOut of memory\n");
        longjmp(driver_jmp_buf, 1U);
    }
    block->handles.storage.temporary = NULL;
    fn_00413a00(block->handles.storage.destination);
    if (source) {
        memcpy(block->handles.storage.destination->data, source, size);
    } else {
        end = block->handles.storage.destination->data;
        memset(end, 0, size);
        end += size;
    }
    fn_00413a50(block->handles.preference.source);
    block->next = NULL;
    return block;
}
