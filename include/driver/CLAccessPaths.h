#ifndef DRIVER_CLACCESSPATHS_H
#define DRIVER_CLACCESSPATHS_H

#include "compiler/common.h"
#include "driver/OS.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct AccessPaths {
    AccessPathEntry **items;
    UInt16 capacity;
    UInt16 count;
};
#pragma pack(pop)
extern void copy_access_paths_to_file_specs_checked(CWFileSpec *path, AccessPaths *value, short result);
extern unsigned char CLAccessPaths_FreeItems(AccessPaths *paths);
extern unsigned char CLAccessPaths_StoreItem(AccessPaths *ctx, void *value);
extern AccessPathEntry *CLAccessPaths_GetEntry(AccessPaths *table, unsigned short index);
extern unsigned short CLAccessPaths_GetCount(AccessPaths *table);
extern unsigned char CLAccessPaths_Init(AccessPaths *a0);
extern unsigned char CLAccessPaths_InitializeChildren(AccessPathEntry *path);
extern int count_access_paths_recursive(AccessPaths *a0);
extern void copy_access_paths_to_file_specs(CWFileSpec **destination, AccessPaths *paths, short *remaining);
extern Boolean allocate_slot(AccessPaths *table, UInt16 *slotIndex);
extern AccessPathEntry *CLAccessPaths_FindPath(AccessPaths *table, OSPathSpec *value);
extern Boolean CLAccessPaths_InsertItem(AccessPaths *paths, UInt16 index, void *value);
extern Boolean add_subdirectory_access_paths(AccessPaths *ctx, AccessPathEntry *param2);
extern unsigned char remove_access_path(AccessPaths *list, UInt16 index);
extern AccessPathEntry *init_access_path_entry(OSPathSpec *src, AccessPathEntry *result);
extern AccessPathEntry *CLAccessPaths_CreateAccessPathEntry(OSPathSpec *path);
extern void free_access_path_entry(AccessPathEntry *blocks);

#ifdef __cplusplus
}
#endif

#endif
