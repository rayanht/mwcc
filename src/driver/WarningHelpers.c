#include "compiler/common.h"
#include "driver/WarningHelpers.h"
#include "driver/TargetWarningHelpers-ppc-cc.h"
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
