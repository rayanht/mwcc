#include "compiler/common.h"
#include "driver/WarningHelpers.h"
#include "driver/Memory.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"
#include "driver/Projects.h"
#include "driver/StringUtils.h"
#include "driver/TargetWarningHelpers-ppc-cc.h"

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
                pCmdLine.noWarnings = 0;
            switch (optionCode) {
                case 0x4e77:
                    pCmdLine.noWarnings = enabled;
                    break;
                case 0x4177:
                    set_warning_option(optionCode, enabled);
                    break;
                case 0x4377:
                    pCmdLine.noCmdLineWarnings = !enabled;
                    break;
                case 0x5765:
                    pCmdLine.warningsAreErrors = enabled;
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
    if (pCmdLine.noCmdLineWarnings != '\0') {
        HPrintF(output, "\t- no command-line warnings\n");
    } else {
        HPrintF(output, "\t- command-line warnings\n");
    }
    if (pCmdLine.warningsAreErrors != '\0') {
        HPrintF(output, "\t- warnings are errors\n");
    } else {
        HPrintF(output, "\t- warnings are not errors\n");
    }
    if (pCmdLine.noWarnings != '\0') {
        HPrintF(output, "\t- no warnings at all\n");
    }
    ToolHelpers_cc_PrintCLanguageWarningOptions(output);
    ToolHelpers_cc_CallValuePairCallback(NULL, output);
    Memory_FreeHandle(output);
    return 1;
}
