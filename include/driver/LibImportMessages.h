#ifndef DRIVER_LIBIMPORTMESSAGES_H
#define DRIVER_LIBIMPORTMESSAGES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char data_00587958;
extern void format_string(char *buf, int size, char *fmt, char *ap);
extern unsigned char CompilerTools_ReportDiagnostic(SInt32 code, ...);
extern SInt16 data_005511b4;
extern SInt32 data_00588228;
extern void CompilerTools_ReportLimitedDiagnostic(SInt32 code, ...);
extern SInt16 limited_diagnostic_limit;
extern SInt32 limited_diagnostic_count;
extern void CompilerTools_FormatMessageAndLongjmp(int arg1, int arg2);
extern void CompilerTools_DispatchMessageBufferByMode(int a0);

#ifdef __cplusplus
}
#endif

#endif
