#ifndef DRIVER_CLERRORS_H
#define DRIVER_CLERRORS_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void format_and_emit_diagnostic(int kind, int messageId, va_list arguments);
extern char *fn_004087d0(unsigned int a0, char *a1);
extern unsigned char CLErrors_EmitDiagnostic(volatile int argument, ...);
extern void CLErrors_ForwardMessageArguments(int value, ...);
extern void CLErrors_ForwardMessage(short a0, ...);
extern void CLErrors_ReportOSError(SInt32 code, UInt32 osErr, ...);
extern void CLErrors_ReportFormattedOSError(SInt32 code, SInt32 osErr, ...);
extern void CLErrors_ReportInternalError(const char *file, int line, const char *fmt, ...);
extern void CLErrors_FatalError(char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
