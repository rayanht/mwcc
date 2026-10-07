#include "compiler/common.h"
#include "driver/CLProj.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/CLTarg.h"
#include "driver/MsDos.h"

unsigned char CLProj_InitializeCWD(Project *project)
{
    project->targets = NULL;
    OS_GetCWD(&project->projectDirectory.path);
    OS_MakeNameSpec("", &project->projectDirectory.name);
    return 1;
}

unsigned char CLProj_FreeTargets(Project *project)
{
    if (project == NULL) {
        CLIO_ReportAssertionFailure("this != NULL", "CLProj.c", 25U);
    }
    CLTarg_FreeTargets(project->targets);
    return 1;
}
