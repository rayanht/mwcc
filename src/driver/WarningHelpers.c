#include "compiler/common.h"
#include "driver/WarningHelpers.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Memory.h"
#include "driver/StringUtils.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers-cc.h"
#include "driver/ToolHelpers.h"
#include <setjmp.h>

int parse_warning_settings(int option, char *settings, int argument, int flags)
{
    unsigned char *cursor;
    char enabled;
    unsigned short optionCode;
    unsigned char defaultEnabled;
    int minusEnabled;
    int barEnabled;

    cursor = (unsigned char *)settings;
    if ((flags & 8) != 0)
        defaultEnabled = 1;
    else
        defaultEnabled = 0;
    enabled = barEnabled = (minusEnabled = defaultEnabled) ^ 1;
    while (*cursor != 0) {
        if (*cursor == '+') {
            enabled = !defaultEnabled;
        } else if (*cursor == '-') {
            enabled = minusEnabled;
        } else if (*cursor == '|') {
            enabled = barEnabled;
        } else {
            optionCode = cursor[1] | *cursor << 8;
            if (enabled)
                data_00537770 = 0;
            switch (optionCode) {
                case 0x4e77:
                    data_00537770 = enabled;
                    break;
                case 0x4177:
                    set_warning_option(optionCode, enabled);
                    break;
                case 0x4377:
                    data_0053777a = !enabled;
                    break;
                case 0x5765:
                    data_00537771 = enabled;
                    set_warning_option(optionCode, enabled);
                    break;
                default:
                    if (set_warning_option(optionCode, enabled) == 0)
                        Targets_ForwardVarArgsAndLongjmp("Bad warning settings in %s (%c%c)\n", settings, cursor[0],
                                                         cursor[1]);
            }
            cursor++;
        }
        cursor++;
    }
    fn_0040ba99(pluginPrivateContext);
    return 1;
}

unsigned int print_command_line_warning_options(void)
{
    struct StorageHandle *output;

    output = (struct StorageHandle *)Memory_NewHandle(0);
    if (output == NULL) {
        longjmp(plugin_request_jmp_buf, 7);
    }
    HPrintF(output, "Command-line warning options:\n");
    if (data_0053777a != '\0') {
        HPrintF(output, "\t- no command-line warnings\n");
    } else {
        HPrintF(output, "\t- command-line warnings\n");
    }
    if (data_00537771 != '\0') {
        HPrintF(output, "\t- warnings are errors\n");
    } else {
        HPrintF(output, "\t- warnings are not errors\n");
    }
    if (data_00537770 != '\0') {
        HPrintF(output, "\t- no warnings at all\n");
    }
    ToolHelpers_cc_PrintCLanguageWarningOptions(output);
    ToolHelpers_cc_CallValuePairCallback(NULL, output);
    Memory_FreeHandle(output);
    return 1;
}

int set_warning_option(short option, char enabled)
{
    char mode;

    switch (option) {
        case 18800:
            data_00540b16 = enabled;
            break;
        case 17764:
            data_00540b17 = enabled;
            break;
        case 20597:
            data_00540b18 = enabled;
            break;
        case 21878:
            data_00540b19 = enabled;
            break;
        case 21857:
            data_00540b1a = enabled;
            break;
        case 17763:
            data_00540b1b = enabled;
            break;
        case 20580:
            data_00540b1c = enabled;
            break;
        case 18550:
            data_00540b1e = enabled;
            break;
        case 18787:
            data_00540b1f = enabled;
            break;
        case 20073:
            data_00540b20 = enabled;
            break;
        case 21347:
            data_00540b21 = enabled;
            break;
        case 19553:
            if (enabled != 0) {
                mode = 1;
            } else {
                mode = 2;
            }
            data_00588528 = mode;
            break;
        case 22373:
            data_00540b1d = enabled;
            break;
        case 17264:
            data_00540ad7 = enabled;
            break;
        default:
            return 0;
    }
    return 1;
}

void ToolHelpers_cc_PrintCLanguageWarningOptions(void *self)
{
    HPrintF(self, "C language warning options:\n");
    if (data_00540b16)
        HPrintF(self, "\t- illegal pragmas\n");
    if (data_00540b17)
        HPrintF(self, "\t- empty declarations\n");
    if (data_00540b18)
        HPrintF(self, "\t- possible unwanted effects\n");
    if (data_00540b19)
        HPrintF(self, "\t- unused variables\n");
    if (data_00540b1a)
        HPrintF(self, "\t- unused arguments\n");
    if (data_00540b1b)
        HPrintF(self, "\t- extra commas\n");
    if (data_00540b1c)
        HPrintF(self, "\t- pedantic\n");
    if (data_00540b1e)
        HPrintF(self, "\t- hidden virtual functions\n");
    if (data_00540b1f)
        HPrintF(self, "\t- implicit conversions\n");
    if (data_00540b20)
        HPrintF(self, "\t- 'inline' not performed\n");
    if (data_00540b21)
        HPrintF(self, "\t- struct/class conflict\n");
    if (data_00588528 == 1)
        HPrintF(self, "\t- large args passed to unprototyped functions\n");
    if (data_00540ad7)
        HPrintF(self, "\t- checking prototypes\n");
    if (data_00540b1d)
        HPrintF(self, "\t- warnings are errors\n");
    else
        HPrintF(self, "\t- warnings are not errors\n");
}
