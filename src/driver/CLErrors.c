#define CERROR_FILE "unknown.c"
#pragma exceptions off
#include "compiler/common.h"
#include "driver/CLErrors.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/win32.h"
#include "driver/CLIO.h"
#include "driver/CLStaticMain.h"
#include "driver/MsDos.h"
#include "driver/StringUtils.h"
#include "driver/TextUtils.h"
#include <string.h>
#include <stdlib.h>
#define va_start(ap, last) ((ap) = (char *)&(last) + (((char *)(&(last) + 1) - (char *)&(last) + 3) / 4 * 4))

#include <stdio.h>

static char data_0057d5f8[256];
static char diagnostic_message_buffer[256];
char *fn_004087d0(unsigned int errorCode, char *buffer)
{
    CLIO_GetResourceCString(buffer, 12000U, errorCode);
    strcat(buffer, "\n");
    return buffer;
}

void format_and_emit_diagnostic(int kind, int messageId, va_list arguments)
{
    char *message;
    UInt8 diagnosticKind;

    fn_004087d0(messageId, data_0057d5f8);
    message = mvprintf(diagnostic_message_buffer, 0x100, data_0057d5f8, arguments);
    if (kind == 2) {
        diagnosticKind = 3;
    } else if (kind == 1) {
        diagnosticKind = 2;
    } else {
        diagnosticKind = 5;
    }
    CLIO_ReportDiagnostic(NULL, NULL, 0, diagnosticKind, "%", message);
    if (message != diagnostic_message_buffer) {
        free(message);
    }
}

unsigned char CLErrors_EmitDiagnostic(int messageId, ...)
{
    va_list arguments = ((char *)&messageId + ((char *)((short *)&messageId + 1) - (char *)&messageId + 3) / 4 * 4);
    format_and_emit_diagnostic(2, messageId, arguments);
}

void CLErrors_ForwardMessageArguments(int messageId, ...)
{
    char *base = (char *)&messageId;
    va_list arguments = (char *)&messageId + ((char *)((short *)&messageId + 1) - base + 3) / 4 * 4;
    format_and_emit_diagnostic(1, messageId, arguments);
}

void CLErrors_ForwardMessage(short messageId, ...)
{
    va_list arguments;
    void *messageSlot = &messageId;
    unsigned int *argument = messageSlot;
    va_start(arguments, messageId);
    format_and_emit_diagnostic(0, *argument, arguments);
}

void CLErrors_ReportOSError(SInt32 diagnosticCode, UInt32 osErr, ...)
{
    char messageBuffer[256];
    char formatBuffer[256];
    char *message;
    char *osMessage;
    va_list args;

    va_start(args, osErr);
    message = mvprintf(messageBuffer, sizeof(messageBuffer), fn_004087d0(diagnosticCode, formatBuffer), args);
    osMessage = OS_GetErrText(osErr);
    CLErrors_EmitDiagnostic(99, message, osMessage, osErr);
    if (message != messageBuffer)
        free(message);
}

void CLErrors_ReportFormattedOSError(SInt32 diagnosticCode, SInt32 osError, ...)
{
    char messageBuffer[256];
    char formatBuffer[256];
    char *message;
    va_list args;

    va_start(args, osError);
    message = mvprintf(messageBuffer, sizeof(messageBuffer), fn_004087d0(diagnosticCode, formatBuffer), args);
    {
        char *errorMessage = strerror(osError);
        CLErrors_EmitDiagnostic(100, message, errorMessage, osError);
    }
    if (message != messageBuffer)
        free(message);
}

void CLErrors_ReportInternalError(const char *file, int line, const char *fmt, ...)
{
    char buf[256];
    char *msg;
    va_list args;
    va_start(args, fmt);
    msg = mvprintf(buf, sizeof(buf), fmt, args);
    CLIO_WriteFormattedText("INTERNAL ERROR [%s:%d]:\n%s\n", file, line, msg);
    if (msg != buf)
        free(msg);
}

#pragma exceptions reset
void CLErrors_FatalError(char *format, ...)
{
    char *message;
    char buffer[256];
    va_list args;

    va_start(args, format);
    message = mvprintf(buffer, sizeof(buffer), format, args);
    CLIO_WriteFormattedText("FATAL ERROR:\n%s\n", message);
    if (message != buffer) {
        free(message);
    }
    exit(-123);
}
