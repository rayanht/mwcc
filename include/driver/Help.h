#ifndef DRIVER_HELP_H
#define DRIVER_HELP_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FlagTextBuffer {
    char text[64];
};
/* A column of the driver's option-help output (firstHelpColumn, third_help_column, helpTextColumn; laid out by
   Help_InitColumns): text queued in a 1 KB ring and flushed a screen line at a time by write_column_line. */
struct HelpColumn {
    SInt16 left;
    SInt16 width;
    char text[1024];
    SInt16 base;
    SInt16 count;
    SInt16 column;
    SInt16 lines;
    SInt16 linepos;
    UInt8 ended;
    UInt8 indented;
};
extern void write_column_line(HelpColumn *source);
extern void advance_column(HelpColumn *p, SInt16 n);
extern void fn_0042ae0a(HelpColumn *block, short amount);
extern unsigned char fn_0042ae56(unsigned char c);
extern void append_formatted_text(HelpColumn *buf, char *format, ...);
extern int append_column_newline(HelpColumn *column);
extern void print_string_line(void);
extern void format_help_columns(HelpColumn *a, HelpColumn *b);
extern void drain_column(HelpColumn *buf);
extern void format_first_and_third_help_columns(void);
extern void append_columns_newline(void);
extern unsigned int Help_FormatOption(OptionList *scope, Option *pragma, unsigned int formatMode, char *nameFilter);
extern void Help_PrintOptionList(OptionList *list, int subprint, char *filter);
extern void Help_PrintOptionUsageNotes(void);
extern void Help_InitColumns(void);
extern void Help_PrintRepeatedCharLine(char c);
extern void fn_0042c0a0(void);
extern unsigned short help_width;
extern struct HelpColumn firstHelpColumn;
extern struct HelpColumn third_help_column;
extern struct HelpColumn helpTextColumn;
extern void *help_output;
extern int data_00587ce0;
extern unsigned char data_00587e23;
extern char *help_option_separator;

#ifdef __cplusplus
}
#endif

#endif
