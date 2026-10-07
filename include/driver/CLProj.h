#ifndef DRIVER_CLPROJ_H
#define DRIVER_CLPROJ_H

#include "compiler/common.h"
#include "compiler/win32.h"
#include "driver/CLAccessPaths.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char CLProj_FreeTargets(void *value);
extern unsigned char CLProj_InitializeCWD(char *a0);

#ifdef __cplusplus
}
#endif

#endif
