#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLStaticPlugins.h"
#include "driver/cc-eabi-ppc-mw.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int CLStaticPlugins_SetIdentifiers(SInt32 *architectureIdentifier, SInt32 *abiIdentifier)
{
    *(unsigned int *)architectureIdentifier = 0x65505043U;
    *abiIdentifier = 0x45414249U;
    return (unsigned int)abiIdentifier;
}

unsigned int fn_004053a0(SInt32 *pluginType, SInt32 *pluginSubtype)
{
    *(unsigned int *)pluginType = '****';
    *pluginSubtype = 'Comp';
    return (unsigned int)pluginSubtype;
}

void fn_004053c0(SInt32 *pluginID)
{
    *pluginID = 'Seep';
}

unsigned int fn_004053d0(void)
{
    int success;

    success = 0;
    if (fn_0040bed0() != 0 && fn_0040c060() != 0) {
        success = 1;
    }
    return success;
}

int fn_004053f0(void)
{
    return fn_0040bf10() && fn_0040c070();
}
