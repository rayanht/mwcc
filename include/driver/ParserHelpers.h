#ifndef DRIVER_PARSERHELPERS_H
#define DRIVER_PARSERHELPERS_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int get_spec_from_signature_callback(const char *path, OSSpec *spec);
extern char *ParserHelpers_GetFirstEnvironmentVariable(char *list, char verbose, char **out);
extern Boolean match_extension_pattern(char *pattern, char *name);
extern int fn_0040c9e7(char *name, int recursive, char *override);
extern char data_0058851b;
extern void report_environment_variable_message(void (*report)(char *msg, va_list ap), char *where, int id, ...);
extern int ParserHelpers_ParsePathList(char *path, char separator, char alternateSeparator, int mode, char *name,
                                       char option, int flags, unsigned char toggle);
extern int fn_0040cd55(char *name, char *filter, char *override);
extern int data_00587e14;
extern int data_00587e18;
extern Boolean data_00587e20;
extern void ParserHelpers_AppendText(struct StorageHandle **hp, char *text);
extern int fn_0040cff6(void);
extern int fn_0040d002(void);
extern unsigned int fn_0040d012(unsigned int reportError);
extern SInt16 data_00537766;
extern UInt8 data_00587e1c;
extern int fn_0040d0eb(void);
extern UInt16 data_0053776c;
extern int parse_stage_settings(int unused1, unsigned char *opt, int unused2, int flags);
extern short data_00537764;
extern short data_0054a0b8;
extern int fn_0040d283(int unused, char *first, char *second);
extern int append_define_directive(char *name, char *value);
extern Boolean targets_value_null_or_zero;

#ifdef __cplusplus
}
#endif

#endif
