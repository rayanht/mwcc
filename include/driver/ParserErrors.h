#ifndef DRIVER_PARSERERRORS_H
#define DRIVER_PARSERERRORS_H

#include "compiler/common.h"
#include "driver/Targets.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void Targets_FormatAndDispatchMessage(char *message, char *arguments);
extern void Targets_ReportFormattedMessage(char *format, char *arguments);
extern void format_and_dispatch_message(char *first, char *second);
extern void format_and_forward_message(char *message, unsigned int *arguments);
extern void format_and_report_message(const char *text, va_list position);
extern void report_operating_system_error(char *name, DWORD value, unsigned int *result);
extern char *Targets_GetResourceCString(SInt32 argument, char *buffer);
extern void fn_0040ecb1(SInt32 a, ...);
extern void Targets_ReportMessage(int a, ...);
extern void Targets_DispatchVariadicMessage(int argument, ...);
extern unsigned char Targets_ReportOperatingSystemError(int id, short code, ...);
extern void Targets_FormatAndForwardMessage(int s, ...);
extern void Targets_ForwardVarArgsAndLongjmp(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
