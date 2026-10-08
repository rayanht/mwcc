#ifndef DRIVER_ASSERTIONFAILURE_H
#define DRIVER_ASSERTIONFAILURE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void CLIO_ReportAssertionFailure(char *a, char *b, unsigned int c);

#ifdef __cplusplus
}
#endif

#endif
