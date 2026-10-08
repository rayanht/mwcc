#ifndef DRIVER_MEMUTILS_H
#define DRIVER_MEMUTILS_H

#include <setjmp.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *xmalloc(const char *text, unsigned int size);
extern void *xcalloc(const char *pool, unsigned int size);
extern void *xrealloc(const char *detail, void *block, unsigned int size);
extern char *xstrdup(const char *s);
extern int __stdcall MemUtils_CallPluginEntry(Plugin *entry);

#ifdef __cplusplus
}
#endif

#endif
