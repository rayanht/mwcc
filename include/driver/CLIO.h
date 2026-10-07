#ifndef DRIVER_CLIO_H
#define DRIVER_CLIO_H

#include <setjmp.h>
#include "compiler/common.h"
#include "driver/MsDos.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ByteBuffer {
    char *data;
    int size;
    int capacity;
    short column;
    unsigned char callerOwned;
    char *prefix;
};
struct DiagnosticDetails {
    unsigned char precedingData[324];
    char text[324];
    int formatValue;
    int diagnosticValue;
};
#pragma pack(push, 1)
struct DiagnosticSourcePosition {
    OSSpec primaryFile;
    OSSpec
        file; /* 0x144: format_and_print_message passes the diagnostic file to OS_SpecToStringRelative; CLIO_ReportDiagnostic copies it from record. */
    char *sourceLine;
    SInt32 line; /* 0x28c: format_and_print_message prints the source line number after the file path. */
    int column;
    short length;
    char reserved662[2];
    int selectionOffset; /* 0x298: report_message copies the diagnostic selection position; print_pipe_delimited_diagnostic emits it after column and length. */
    short
        selectionLength; /* 0x29c: report_message copies and clamps the selection extent; print_pipe_delimited_diagnostic emits it after selectionOffset. */
    char reserved670[2];
};
#pragma pack(pop)

extern char data_005880e0[];
extern Boolean write_text_buffer(struct _FILE *fp, StorageHandle *bufp, SInt32 len);
extern unsigned char CLIO_WriteStorageToStdout(StorageHandle *first, SInt32 second, unsigned int reset);
extern Boolean CLIO_WriteTextFile(OSSpec *fileRef, StorageHandle *text, SInt32 textLength, SInt32 fileType,
                                  SInt32 creator);
extern unsigned char fn_00415090(OSSpec *a0, unsigned int a1, unsigned int a2, MemBuffer *a3);
extern unsigned char CLIO_AppendStorageToFile(OSSpec *a0, struct StorageHandle *a1, SInt32 a2, SInt32 a3, SInt32 a4);
extern void fn_004151c0(void);
extern void fn_004151d0(int);
extern void fn_004151e0(void);
extern unsigned char fn_004151f0(void);
extern void reset_column_and_append_prefix(struct ByteBuffer *node);
extern void wrap_line(struct ByteBuffer *p);
extern void append_prefix_separator(struct ByteBuffer *state);
extern char *format_prefixed_text(char *output, int remaining, char *prefix, char *format, char **args);
extern char *forward_format_arguments(char *output, int size, char *prefix, char *format, ...);
extern unsigned int write_text_to_stdout_or_stderr(int unused, short messageType, const char *textAddress);
extern void append_byte(struct ByteBuffer *buffer, char value);
extern void append_text(struct ByteBuffer *buffer, char *text);
extern int report_user_break(unsigned int value);
extern void initialize_console(void);
extern void clear_global(void);
extern void CLIO_ExchangeClearGlobal(void);
extern char CLIO_InitializeStreamBuffering(void);
extern unsigned char fn_00414e20(void);
extern unsigned char clear_global_byte(void);
extern short CLIO_ReportDiagnostic(Plugin *type, DiagnosticSourcePosition *record, int message, short severity,
                                   char *argument, ...);
extern char *make_source_position_carets(DiagnosticSourcePosition *sourcePosition);
extern void update_cached_specs(OSSpec *recordAddress);
extern unsigned char nonmatching_plugin_with_clear_high_bit(Plugin *type);
extern char *get_plugin_type_name(Plugin *tool);
extern unsigned char *select_plugin_type_data(Plugin *value);
extern void format_and_print_message(Plugin *type, struct DiagnosticSourcePosition *obj, int messageCode, SInt16 kind,
                                     char *formatFlags, char **formatOptions);
extern void print_diagnostic_with_details(Plugin *unused, struct DiagnosticDetails *details, int unused2,
                                          short diagnostic, char *formatArg, char **formatArgs);
extern void print_pipe_delimited_diagnostic(Plugin *kind, DiagnosticSourcePosition *record, int unused, short level,
                                            char *argument1, char **argument2);
extern void emit_formatted_message(short kind, char *format, ...);
extern void CLIO_WriteFormattedText(const char *format, ...);
extern void emit_formatted_diagnostic(Plugin *source, DiagnosticSourcePosition *diagnostic, int unused, short kind,
                                      char *format, char **arguments);
extern void print_diagnostic(Plugin *object, DiagnosticSourcePosition *dump, SInt32 code, SInt16 level,
                             char *messageArg1, char **messageArg2);
extern void CLIO_FormatAndDispatchText(char *fmt, ...);
extern void extract_diagnostic_source_line(DiagnosticSourcePosition *info);
extern int CLIO_CompareStringsIgnoreCase(char *left, char *right);
extern NameTableEntry *create_data_block(char *name, const void *source, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
