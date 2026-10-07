#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/ClientGlue.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLMain.h"
#include "driver/CLPlugins.h"
#include "driver/ResourceStrings.h"
#include "driver/Resources.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
int fn_004050e0(char *left, char *right, int count)
{
    unsigned int rightValue;
    unsigned int leftValue;
    while (count-- != 0) {
        rightValue = tolower((int)*right++);
        leftValue = tolower((int)*left);
        if (leftValue - rightValue != 0) {
            return leftValue - rightValue;
        }
        if (*left++ == '\0') {
            return 0;
        }
    }
    return 0;
}

int ClientGlue_CompareLowercaseStrings(const char *left, const char *right)
{
    unsigned int rightValue;
    int difference;

    do {
        rightValue = tolower((unsigned int)*right++);
        difference = tolower((unsigned int)*left) - rightValue;
        if (difference != 0) {
            return difference;
        }
    } while (*left++ != '\0');
    return 0;
}

char *ClientGlue_DuplicateString(const char *a0)
{
    unsigned int n;
    char *p;
    n = strlen(a0) + 1U;
    p = (char *)malloc(n);
    if (p != NULL) {
        memcpy(p, a0, n);
    }
    return p;
}

int __stdcall ClientGlue_AddResourceStrings(char *arg1, SInt16 arg2, char **arg3)
{
    unsigned char **h;
    if (arg3 == NULL) {
        h = Resources_GetHand(0x53545223, arg2);
        if (h == NULL) {
            CLErrors_FatalError("Resource ('STR#',%d) '%s' not found in executable\n", arg2, arg1);
            return 0;
        }
        Resources_ClearError(h);
        return 1;
    }
    return ResourceStrings_AddResource(arg1, arg2, arg3);
}

void __stdcall ClientGlue_AddPlugin(PluginRequiredInputRecord *a0)
{
    Plugin *lift_call_0;
    lift_call_0 = CLPlugins_CreatePluginDataCopy(a0, 0U, 0U);
    CLPlugins_AddPlugin(lift_call_0);
}

int __stdcall ClientGlue_CreateAndAddPlugin(void *a0, void *a1)
{
    Plugin *lift_call_0;
    int lift_call_1;
    lift_call_0 = CLPlugins_CreatePluginDataCopy(a0, a1, 0U);
    lift_call_1 = CLPlugins_AddPlugin(lift_call_0);
    return lift_call_1;
}

void __stdcall fn_00405280(PluginRequiredInputRecord *a0, PluginQueryTable *a1)
{
    Plugin *lift_call_0;
    int lift_call_1;
    lift_call_0 = CLPlugins_CreatePluginDataCopy(a0, 0U, a1);
    lift_call_1 = CLPlugins_AddPlugin(lift_call_0);
    return;
}

#pragma auto_inline off

#pragma auto_inline reset

void __stdcall fn_004052a0(unsigned int a0, unsigned int a1)
{
    data_005871c4 = a0;
    data_005871c8 = a1;
    return;
}

unsigned int __stdcall fn_004052c0(unsigned int a0)
{
    data_005871d4 = a0;
    return a0;
}

void __stdcall fn_004052d0(unsigned int a0, unsigned int a1)
{
    data_005871d0 = a0;
    plugin_type = a1;
    return;
}

unsigned int __stdcall ClientGlue_SetNamesAndRun(unsigned int argumentCount, char **arguments, unsigned int inputName,
                                                 unsigned int outputName)
{
    strncpy(input_name, (char *)inputName, 32U);
    strncpy(output_name, (char *)outputName, 32U);
    return CLMain_Initialize(argumentCount, arguments);
}

int ClientGlue_InitializeAndParseCommandLine(void)
{
    return CLMain_InitializeAndParseCommandLine();
}

unsigned int __stdcall fn_00405340(unsigned int result)
{
    return CLMain_FreePlugins(result);
}
