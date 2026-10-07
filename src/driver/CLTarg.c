#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLTarg.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/AssertionFailure.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLDependencies.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLOverlays.h"
#include "driver/CLPlugins.h"
#include "driver/CLSegs.h"
#include "driver/Files.h"
#include "driver/MemUtils.h"
#include "driver/MsDos.h"
// "CLTarg.c"
#include <string.h>

#include <stdlib.h>
typedef SInt32(__stdcall *DispatchOperation)(CWPluginPrivateContext *, void *, int, void *, void *);
struct CLTarget *CLTarg_CreateTarget(char *targetName, int processor, int operatingSystem, int targetKind)
{
    struct CLTarget *target;
    int initialized;

    target = xcalloc("target", sizeof(struct CLTarget));
    strncpy(target->name, targetName, sizeof(target->name));
    target->settings = xcalloc("target info", 0x136);
    target->cpu = processor;
    target->os = operatingSystem;
    target->targetKind = targetKind;
    OS_GetCWD(&target->outputDirectory);
    if (!CLSegs_InitSegments(&target->lookupPaths))
        CLIO_ReportAssertionFailure("Segments_Initialize(&targ->linkage.segs)", "CLTarg.c", 25);
    if (!CLOverlays_Init(&target->overlays))
        CLIO_ReportAssertionFailure("Overlays_Initialize(&targ->linkage.overlays)", "CLTarg.c", 28);
    initialized = CLFiles_AssertNonNullIndexedListLink(&target->files) &&
                  CLFiles_AssertNonNullIndexedListLink(&target->generatedFiles) &&
                  CLFiles_InitChain(&target->fileLookup) && CLAccessPaths_Init(&target->userPaths) &&
                  CLAccessPaths_Init(&target->systemPaths) && CLDependencies_InitDeps(&target->dependencyTable, target);
    if (!initialized)
        CLIO_ReportAssertionFailure(
            "Files_Initialize(&targ->files) && Files_Initialize(&targ->pchs) && VFiles_Initialize(&targ->virtualFiles) && Paths_Initialize(&targ->sysPaths) && Paths_Initialize(&targ->userPaths) && Incls_Initialize(&targ->incls, targ)",
            "CLTarg.c", 36);
    return target;
}

void free_target(CLTarget *target)
{
    CLSegs_FreeValues(&target->lookupPaths);
    CLOverlays_FreeGroups(&target->overlays);
    CLAccessPaths_FreeItems(&target->userPaths);
    CLAccessPaths_FreeItems(&target->systemPaths);
    CLFiles_FreeAllocationRecords(&target->files);
    CLFiles_FreeAllocationRecords(&target->generatedFiles);
    CLFiles_FreeChainNodes(&target->fileLookup);
    CLDependencies_FreeDeps(&target->dependencyTable);
    free(target);
}

void CLTarg_FreeTargets(CLTarget *head)
{
    CLTarget *entry;
    CLTarget *next;
    entry = head;
    if (head != NULL) {
        do {
            next = entry->next;
            free_target(entry);
            entry = next;
        } while (next != NULL);
    }
}

void CLTarg_AppendEntry(CLTarget **list, CLTarget *target)
{
    for (; *list != NULL; list = &(*list)->next) {
    }
    *list = target;
}
