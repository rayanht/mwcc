#ifndef DRIVER_OS_H
#define DRIVER_OS_H

#include "compiler/common.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSPathSpec {
    char s[0x104];
};
struct OSNameSpec {
#if VERSION >= VERSION_GC_1_3
    char s[0x100];
#else
    char s[0x40];
#endif
};
struct OSSpec {
    OSPathSpec path;
    OSNameSpec name;
};

#ifdef __cplusplus
}
#endif

#endif
