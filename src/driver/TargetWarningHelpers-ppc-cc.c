#include "compiler/common.h"
#include "driver/TargetWarningHelpers-ppc-cc.h"
#include "driver/StringUtils.h"

Pragma data_0054a690[] = {
    {&data_00588528, "warn_largeargs", 0},
    {0, 0, 0},
};

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
