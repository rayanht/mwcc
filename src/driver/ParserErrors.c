#include "compiler/common.h"
#include "driver/ParserErrors.h"
#include "compiler/objects.h"
#include "compiler/CError.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ParserFace.h"
#include "driver/Projects.h"
#include "driver/TargetOptimizer-ppc-eabi.h"
#include "driver/TextUtils.h"
#include <stdio.h>

#define va_start(ap, parm) ap = (char *)&parm + ((((char *)(&parm + 1) - (char *)&parm) + 3) / 4 * 4)

static char formatted_message[1024];

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
