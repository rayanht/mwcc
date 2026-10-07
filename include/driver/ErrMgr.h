#ifndef DRIVER_ERRMGR_H
#define DRIVER_ERRMGR_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *get_status_string(SInt16 status);
extern unsigned int __stdcall CError_GetErrorString(short a0, char *a1);

#ifdef __cplusplus
}
#endif

#endif
