#ifndef DRIVER_STRINGEXTRAS_H
#define DRIVER_STRINGEXTRAS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *__stdcall CLProj_AppendString(char *dest, char *src, int size);
extern char *__stdcall CLProj_CopyStringBounded(char *destination, const char *source, unsigned int count,
                                                int capacity);

#ifdef __cplusplus
}
#endif

#endif
