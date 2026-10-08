#ifndef DRIVER_CLPROJ_H
#define DRIVER_CLPROJ_H

#include "compiler/common.h"
#include "compiler/win32.h"
#include "driver/CLAccessPaths.h"
#include "driver/MsDos.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The project: its targets, and the directory its relative paths start from. */
struct Project {
    struct CLTarget *targets;
    OSSpec projectDirectory;
};
extern unsigned char CLProj_FreeTargets(Project *project);
extern unsigned char CLProj_InitializeCWD(Project *project);

#ifdef __cplusplus
}
#endif

#endif
