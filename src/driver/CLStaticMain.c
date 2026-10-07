#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLStaticMain.h"
#include "compiler/win32.h"
#include "driver/CLMain.h"
#include "driver/CLStaticPlugins.h"
#include "driver/CLToolExec.h"
#include "driver/ClientGlue.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/StaticParserGlue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    SInt32 primaryToolIdentifier, secondaryToolIdentifier;
    SInt32 primaryPluginIdentifier, secondaryPluginIdentifier;
    SInt32 result;

    if (ClientGlue_SetNamesAndRun(argc, argv, data_0053601c, data_0053600c) != 0)
        exit(1);

    if (!fn_0040535e() || !fn_004053f0()) {
        fprintf(stderr, "\r\nFATAL ERROR:  Could not initialize resource strings\r\n");
        exit(1);
    }

    if (!fn_0040534e() || !fn_004053d0()) {
        fprintf(stderr, "\r\nFATAL ERROR:  Could not initialize built-in plugins\r\n");
        exit(1);
    }

    if (!fn_00405840()) {
        fprintf(stderr, "\r\nFATAL ERROR:  Could not initialize options\r\n");
        exit(1);
    }

    CLStaticPlugins_SetIdentifiers(&primaryPluginIdentifier, &secondaryPluginIdentifier);
    fn_004052a0(primaryPluginIdentifier, secondaryPluginIdentifier);
    fn_004053a0(&primaryToolIdentifier, &secondaryToolIdentifier);
    fn_004052d0(primaryToolIdentifier, secondaryToolIdentifier);
    fn_004053c0(&primaryToolIdentifier);
    fn_004052c0(primaryToolIdentifier);

    result = ClientGlue_InitializeAndParseCommandLine();
    if (result != 0) {
        if (result == 2)
            fprintf(stderr, "\r\nUser break, cancelled...\r\n");
        else
            fprintf(stderr, "\r\nErrors caused tool to abort.\r\n");
    }
    fn_00405340(result);
    exit(result);
    return 0;
}
