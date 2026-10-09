#ifndef COMPILER_PARSEDOUBLE_H
#define COMPILER_PARSEDOUBLE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *ParseDouble(char *value, double *result, char *error);

#ifdef __cplusplus
}
#endif

#endif
