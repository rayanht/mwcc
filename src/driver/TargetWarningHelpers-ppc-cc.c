#include "compiler/common.h"
#include "driver/TargetWarningHelpers-ppc-cc.h"
#include "driver/StringUtils.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"

Pragma data_0054a690[] = {
    {&data_00588528, "warn_largeargs", 0},
    {0, 0, 0},
};

int set_warning_option(short option, char enabled)
{
    char mode;

    switch (option) {
        case 18800:
            pWarningC.warn_illpragma = enabled;
            break;
        case 17764:
            pWarningC.warn_emptydecl = enabled;
            break;
        case 20597:
            pWarningC.warn_possunwant = enabled;
            break;
        case 21878:
            pWarningC.warn_unusedvar = enabled;
            break;
        case 21857:
            pWarningC.warn_unusedarg = enabled;
            break;
        case 17763:
            pWarningC.warn_extracomma = enabled;
            break;
        case 20580:
            pWarningC.pedantic = enabled;
            break;
        case 18550:
            pWarningC.warn_hidevirtual = enabled;
            break;
        case 18787:
            pWarningC.warn_implicitconv = enabled;
            break;
        case 20073:
            pWarningC.warn_notinlined = enabled;
            break;
        case 21347:
            pWarningC.warn_structclass = enabled;
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
            pWarningC.warningerrors = enabled;
            break;
        case 17264:
            pFrontEndC.checkprotos = enabled;
            break;
        default:
            return 0;
    }
    return 1;
}

void ToolHelpers_cc_PrintCLanguageWarningOptions(void *self)
{
    HPrintF(self, "C language warning options:\n");
    if (pWarningC.warn_illpragma)
        HPrintF(self, "\t- illegal pragmas\n");
    if (pWarningC.warn_emptydecl)
        HPrintF(self, "\t- empty declarations\n");
    if (pWarningC.warn_possunwant)
        HPrintF(self, "\t- possible unwanted effects\n");
    if (pWarningC.warn_unusedvar)
        HPrintF(self, "\t- unused variables\n");
    if (pWarningC.warn_unusedarg)
        HPrintF(self, "\t- unused arguments\n");
    if (pWarningC.warn_extracomma)
        HPrintF(self, "\t- extra commas\n");
    if (pWarningC.pedantic)
        HPrintF(self, "\t- pedantic\n");
    if (pWarningC.warn_hidevirtual)
        HPrintF(self, "\t- hidden virtual functions\n");
    if (pWarningC.warn_implicitconv)
        HPrintF(self, "\t- implicit conversions\n");
    if (pWarningC.warn_notinlined)
        HPrintF(self, "\t- 'inline' not performed\n");
    if (pWarningC.warn_structclass)
        HPrintF(self, "\t- struct/class conflict\n");
    if (data_00588528 == 1)
        HPrintF(self, "\t- large args passed to unprototyped functions\n");
    if (pFrontEndC.checkprotos)
        HPrintF(self, "\t- checking prototypes\n");
    if (pWarningC.warningerrors)
        HPrintF(self, "\t- warnings are errors\n");
    else
        HPrintF(self, "\t- warnings are not errors\n");
}
