#include "compiler/common.h"
#include "driver/AssertionFailure.h"
#include <stdio.h>
#include <stdlib.h>

void CLIO_ReportAssertionFailure(char *message, char *file, unsigned int line)
{
    fprintf(stderr, "Assertion (%s) failed in \"%s\" on line %d\n", message, file, line);
    abort();
}
