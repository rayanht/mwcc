#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/LibImportMessages.h"
#include "compiler/objects.h"
#include "compiler/Unmangle.h"
#include "driver/COSToolsCLT.h"
#include "driver/libimp-eabi-ppc.h"
#include <stdio.h>

#define VA_ARG(ap, T) (*(T *)((ap += 4) - 4))

SInt16 limited_diagnostic_limit = 100;
SInt16 data_005511b4 = 2000;
static char message_buffer[0x800];

void format_string(char *buf, int size, char *fmt, char *ap)
{
    char c;
    char *s;
    unsigned char *p;
    int len;
    int n;
    char ch;
    char tmp[256];
    while ((c = *fmt++) != 0) {
        if (c == '%') {
            switch (c = *fmt++) {
                case 'c':
                    s = VA_ARG(ap, char *);
                    while ((ch = *s) != 0 && size > 1) {
                        *buf = ch;
                        s++;
                        buf++;
                        size--;
                    }
                    break;
                case 'p':
                    p = VA_ARG(ap, unsigned char *);
                    len = *p++;
                    while (len != 0 && size > 1) {
                        *buf++ = *p++;
                        len--;
                        size--;
                    }
                    break;
                case 's':
                    Unmangle_UnmangleSymbolName(VA_ARG(ap, char *), tmp, sizeof(tmp));
                    s = tmp;
                    while ((ch = *s) != 0 && size > 1) {
                        *buf = ch;
                        s++;
                        buf++;
                        size--;
                    }
                    break;
                case 'n':
                    n = VA_ARG(ap, int);
                    if (size > 10) {
                        n = sprintf(buf, "%ld", n);
                        buf += n;
                        size -= n;
                    }
                    break;
                case 'h':
                    n = VA_ARG(ap, int);
                    if (size > 11) {
                        n = sprintf(buf, "0x%.8X", n);
                        buf += n;
                        size -= n;
                    }
                    break;
                default:
                    if (size > 1) {
                        *buf++ = c;
                        size--;
                    }
                    break;
            }
        } else if (size > 1) {
            *buf++ = c;
            size--;
        }
    }
    *buf = 0;
}

unsigned char CompilerTools_ReportDiagnostic(SInt32 diagnosticCode, ...)
{
    char message[0x800];
    char format[0x100];
    va_list args;

    if (data_00587958 == 0 && data_00588228 < data_005511b4) {
        COS_GetString(format, 0x2af9, diagnosticCode);
        args = (va_list)&diagnosticCode + ((va_list)(&diagnosticCode + 1) - (va_list)&diagnosticCode + 3) / 4 * 4;
        format_string(message, sizeof(message), format, args);
        fn_0041e990(message, "");
        data_00588228++;
    }
}

void CompilerTools_ReportLimitedDiagnostic(SInt32 diagnosticCode, ...)
{
    va_list args;
    char message[2048];
    char format[256];

    if (limited_diagnostic_count < limited_diagnostic_limit) {
        COS_GetString(format, 0x2af9, diagnosticCode);
        args = (va_list)&diagnosticCode + (((va_list)(&diagnosticCode + 1) - (va_list)&diagnosticCode + 3) / 4) * 4;
        format_string(message, sizeof(message), format, args);
        fn_0041e970(message, "");
        limited_diagnostic_count++;
    } else if (limited_diagnostic_count == limited_diagnostic_limit) {
        COS_GetString(format, 0x2af9, 25);
        args = (va_list)&diagnosticCode + (((va_list)(&diagnosticCode + 1) - (va_list)&diagnosticCode + 3) / 4) * 4;
        format_string(message, sizeof(message), format, args);
        fn_0041e970(message, NULL);
        limited_diagnostic_count++;
    }
}

void CompilerTools_FormatMessageAndLongjmp(int message, int status)
{
    va_list args;
    char buf[256];

    COS_GetString(buf, 0x2af9, message);
    args = (va_list)&status + (((va_list)(&status + 1) - (va_list)&status + 3) / 4) * 4;
    format_string(message_buffer, 0x800, buf, args);
    longjmp(file_input_jmpbuf, status);
}

void CompilerTools_DispatchMessageBufferByMode(int mode)
{
    if (mode != 1) {
        if (mode == 2) {
            fn_0041ede0(message_buffer);
        } else {
            fn_0041ee00(message_buffer, mode);
        }
    }
}

/* The linker stripped the function that used this literal; it stays in the unit's .data. */
static const char *CompilerTools_MemoryErrorMessage(void)
{
    return "Internal Memory Error";
}
