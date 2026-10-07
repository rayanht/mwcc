#ifndef DRIVER_CLTARG_H
#define DRIVER_CLTARG_H

#include "compiler/common.h"
#include "driver/OS.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLDependencies.h"
#include "driver/CLFiles.h"
#include "driver/CLOverlays.h"
#include "driver/CLSegs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The target the command line builds (CLTarg_CreateTarget makes it; default_target is the current one): its counts, file
   lists, access paths, overlays and the plugins that link it. Members are placed as every reader of default_target
   reads them; the unnamed stretches are not read anywhere. */
struct CLTarget {
    SInt32 total00;
    UInt32 count04;
    UInt32 count08;
    UInt32 count0c;
    struct TgtRec *settings;
    Segments lookupPaths;
    Overlays overlays;
    SInt32 linkage;
    struct IndexedListLink files;
    struct IndexedListLink generatedFiles;
    Deps dependencyTable;
    AccessPaths userPaths;
    AccessPaths systemPaths;
    SInt32 targetKind;
    SInt32 cpu;
    SInt32 os;
    char name[0x40];
    struct Plugin *preLinker;
    struct Plugin *linker;
    struct Plugin *postLinker;
    UInt32 preLinkerFlags;
    UInt32 linkerFlags;
    UInt32 postLinkerFlags;
    OSPathSpec outputDirectory;
    struct ChainRecord *fileLookup;
    struct CLTarget *next;
};
extern struct CLTarget *CLTarg_CreateTarget(char *targetName, int processor, int operatingSystem, int targetKind);
extern void free_target(CLTarget *a0);
extern void CLTarg_FreeTargets(CLTarget *head);
extern void CLTarg_AppendEntry(CLTarget **list, CLTarget *target);

#ifdef __cplusplus
}
#endif

#endif
