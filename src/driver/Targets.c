#include "compiler/common.h"
#include "driver/Targets.h"
#include "compiler/objects.h"
#include "compiler/CError.h"
#include "driver/AssertionFailure.h"
#include "driver/ClientGlue.h"
#include "driver/Option.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"

#define pTool driverTool

int Targets_SetTool(int *tool)
{
    pTool = tool;
    ((pTool[6] && pTool[7]) ? (void)0
                            : CLIO_ReportAssertionFailure("pTool->toolInfo && pTool->copyright", "Targets.c", 16));
    return 1;
}

#define MATCH(x, y) ((x) == 0x2a2a2a2a || (y) == 0x2a2a2a2a || (y) == (x))
Boolean Targets_MatchTool(int cpu, int os, int lang, int type)
{
    if (!pTool) {
        Targets_ForwardVarArgsAndLongjmp("No options loaded for command line\n");
        return 0;
    }
    if (MATCH(cpu, pTool[0]) && MATCH(os, pTool[1]) && MATCH(lang, pTool[2]) && MATCH(type, pTool[3]))
        return 1;
    return 0;
}

#undef CERROR_FILE
#define CERROR_FILE "options.c"

Boolean Targets_MatchCommandLineOptions(int argc, char **argv)
{
    int i, j;
    Boolean ok;

    if (driverTool == NULL)
        Targets_ForwardVarArgsAndLongjmp("No options loaded for command line\n");

    for (i = 0; i < argc; i++) {
        for (j = 0; j < driverTool[4]; j++) {
            if (ClientGlue_CompareLowercaseStrings(((char **)driverTool[5])[j], argv[i]) == 0)
                break;
        }
        if (j >= driverTool[4])
            break;
    }

    if (i >= argc)
        ok = 1;
    else
        ok = 0;
    return ok;
}

#undef CERROR_FILE
#define CERROR_FILE __FILE__

int Targets_RegisterOptionLists(void)
{
    int index;
    Option_ResetOptionLists();
    for (index = 0; index < ((struct IndexedValueTable *)driverTool)->count; index++) {
        OptionList **values = (OptionList **)((struct IndexedValueTable *)driverTool)->values;
        Option_RegisterOptionList(values[index]);
    }
    return 1;
}
