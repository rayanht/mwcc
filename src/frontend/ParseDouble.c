#include "compiler/common.h"
#include "compiler/ParseDouble.h"
#include <errno.h>
#include <stdlib.h>

char *ParseDouble(char *value, double *result, char *error)
{
    char *status;

    errno = 0;
    *result = strtod(value, &status);
    *error = errno != 0;
    return status;
}
