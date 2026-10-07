#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/OptimizerHelpers.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/win32.h"
#include "driver/Memory.h"
#include "driver/Option.h"
#include "driver/StringUtils.h"
#include "driver/TargetOptimizer-ppc-eabi.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers-cc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
typedef char *(*TextFormatFunction)(char *buffer, unsigned int capacity, const char *format, va_list arguments);
#pragma auto_inline off
int fn_0040d8c0(short option, int input, int output, int flags)
{
    if (data_0054a2f8 != '\0') {
        Option_ForwardVarArgs(0x3e);
    }
    memset(&data_0054a2fc, 0, 7);
    fn_00420700();
    return 1;
}
#pragma auto_inline reset

#pragma scheduling off
int parse_optimizer_settings(SInt32 option, unsigned char *options, int unused, UInt32 flags)
{
    const char forceOn = 1;
    const char forceOff = 2;
    const char clear = 0;
    unsigned char *start;
    OptFlag enabled;
    volatile int defaultEnabled;
    volatile int alternateEnabled;
    volatile OptFlag initialEnabled;
    unsigned char *cursor;
    cursor = start = options;
    if (flags & 8) {
        initialEnabled.b = 1;
        defaultEnabled = initialEnabled.b;
    } else {
        initialEnabled.b = 0;
        defaultEnabled = initialEnabled.b;
    }
    alternateEnabled = defaultEnabled;
    alternateEnabled ^= 1;
    enabled.b = alternateEnabled;

    for (; *cursor != 0; cursor++) {
        if (*cursor == '+') {
            enabled.b = !initialEnabled.b;
        } else if (*cursor == '-') {
            enabled.b = defaultEnabled;
        } else if (*cursor == '|') {
            enabled.b = alternateEnabled;
        } else {
            UInt16 optionCode = (cursor[0] << 8) | cursor[1];
            if ((optionCode >= 0x4730 && optionCode <= 0x4734) || optionCode == 0x4773 || optionCode == 0x4770)
                fn_0040d8c0(option, 0, 0, 0);
            switch (optionCode) {
                case 0x4373: /* Cs */
                    data_0054a2fc = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x4c69: /* Li */
                    data_0054a2fd = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x5072: /* Pr */
                    data_0054a2fe = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x4473: /* Ds */
                    data_0054a2ff = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x5372: /* Sr */
                    data_0054a300 = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x4463: /* Dc */
                    data_0054a301 = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x4c74: /* Lt */
                    data_0054a302 = enabled.b ? forceOn : forceOff;
                    data_0054a2f8 = 1;
                    break;
                case 0x4730: /* G0 */
                    data_00540b26 = enabled.b ? 0 : 0;
                    break;
                case 0x4731: /* G1 */
                    data_00540b26 = enabled.b ? 1 : 0;
                    break;
                case 0x4732: /* G2 */
                    data_00540b26 = enabled.b ? 2 : 0;
                    break;
                case 0x4733: /* G3 */
                    data_00540b26 = enabled.b ? 3 : 0;
                    break;
                case 0x4734: /* G4 */
                    data_00540b26 = enabled.b ? 4 : 0;
                    break;
                case 0x4773: /* Gs */
                    data_00540b27 = enabled.b ? forceOn : clear;
                    break;
                case 0x4770: /* Gp */
                    data_00540b27 = enabled.b ? clear : forceOn;
                    break;
                default:
                    if (TargetOptimizer_ppc_eabi_SetOption(optionCode, enabled.b) == 0)
                        Targets_ForwardVarArgsAndLongjmp("Bad optimizer settings in %s (%c%c)\n", start, cursor[0],
                                                         cursor[1]);
                    break;
            }
            cursor++;
        }
    }
    return 1;
}
#pragma scheduling reset

#pragma scheduling off

int report_optimizer_options(void)
{
    SInt32 len;
    struct StorageHandle *buf;

    buf = (struct StorageHandle *)Memory_NewHandle(0);
    if (buf == NULL)
        longjmp(plugin_request_jmp_buf, 7);
    HPrintF(buf, "\t- global optimizer level %d\n", data_00540b26);
    HPrintF(buf, "\t- global optimize for %s\n", data_00540b27 == 0 ? "speed" : "size");
    len = Memory_GetHandleSize(buf);
    if (data_0054a2fc)
        HPrintF(buf, "\t- common subexpression elimination %s\n", data_0054a2fc == 1 ? "on" : "off");
    {
        UInt8 flag;
        if ((flag = data_0054a2fd) != 0)
            HPrintF(buf, "\t- loop invariants %s\n", flag == 1 ? "on" : "off");
    }
    {
        UInt8 flag;
        if ((flag = data_0054a2fe) != 0)
            HPrintF(buf, "\t- constant propagation %s\n", flag == 1 ? "on" : "off");
    }
    {
        UInt8 flag;
        if ((flag = data_0054a2ff) != 0)
            HPrintF(buf, "\t- dead store elimination %s\n", flag == 1 ? "on" : "off");
    }
    {
        UInt8 flag;
        if ((flag = data_0054a301) != 0)
            HPrintF(buf, "\t- dead code elimination %s\n", flag == 1 ? "on" : "off");
    }
    {
        UInt8 flag;
        if ((flag = data_0054a300) != 0)
            HPrintF(buf, "\t- strength reduction %s\n", flag == 1 ? "on" : "off");
    }
    {
        UInt8 flag;
        if ((flag = data_0054a302) != 0)
            HPrintF(buf, "\t- variable lifetimes %s\n", flag == 1 ? "on" : "off");
    }
    if (len == Memory_GetHandleSize(buf))
        HPrintF(buf, "\t- no extra global optimizations\n");
    HPrintF(buf, "Backend-specific optimizer options:\n");
    len = Memory_GetHandleSize(buf);
    TargetOptimizer_ppc_eabi_ReportScheduling(buf);
    if (len == Memory_GetHandleSize(buf))
        HPrintF(buf, "\t- no extra backend-specific optimizations\n");
    ToolHelpers_cc_CallValuePairCallback(NULL, buf);
    Memory_FreeHandle(buf);
    return 1;
}

#pragma scheduling reset
