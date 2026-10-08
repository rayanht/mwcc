#ifndef DRIVER_PARSERHELPERS_CC_H
#define DRIVER_PARSERHELPERS_CC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int fn_0040d283(int unused, char *first, char *second);
extern int append_define_directive(char *name, char *value);
extern int append_undef_directive(char *option, int unused, char *symbol);
extern int append_include_directive(char *option, void *handle, char *filename);
extern int ParserHelpers_cc_EmitPragmas(Pragma *pragmas);

#ifdef __cplusplus
}
#endif

#endif
