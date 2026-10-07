#include "compiler/common.h"
#include "driver/CLProj.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/CLTarg.h"
#include "driver/MsDos.h"

unsigned char CLProj_InitializeCWD(char *a0)
{
    *(unsigned int *)a0 = 0;
    OS_GetCWD(a0 + 4);
    OS_MakeNameSpec("", a0 + 264);
    return 1;
}

unsigned char CLProj_FreeTargets(void *value)
{
    if (value == NULL) {
        CLIO_ReportAssertionFailure("this != NULL", "CLProj.c", 25U);
    }
    CLTarg_FreeTargets(*(struct CLTarget **)value);
    return 1;
}
