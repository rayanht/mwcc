#ifndef DRIVER_MACSPECS_H
#define DRIVER_MACSPECS_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

union MacSpecParent {
    struct MacSpecEntry *
        entry; /* 0x00: find_or_create_child_entry sets this for index != 2; lookup_spec_and_advance_parent reads it when kind != 2 */
    struct NameRegistryEntry *
        registry; /* 0x00: find_or_create_name_registry_entry sets this for root.index == 2; lookup_spec_and_advance_parent handles kind == 2 separately */
};

struct MacSpecEntry {
    char *name;
    unsigned int index;
    union MacSpecParent
        parent; /* 0x08: index == 2 selects registry; otherwise find_or_create_child_entry stores the parent entry */
    struct MacSpecEntry *children;
    struct MacSpecEntry *next;
};
struct NameRegistryEntry {
    unsigned short id;
    struct MacSpecEntry root;
    struct NameRegistryEntry *next;
};
extern DWORD __stdcall fn_00413670(short kind, int value, OSPathSpec *path);
extern int __stdcall MacSpecs_MakeResourceForkSpec(OSSpec *source, OSSpec *destination, char retryOnError);
extern int __stdcall MacSpecs_MakeOSSpec(CWFileSpec *record, OSSpec *spec);
extern struct MacSpecEntry *lookup_dir_id(unsigned int a0);
extern MacSpecEntry *find_or_create_child_entry(MacSpecEntry *table, char *name);
extern int find_or_create_spec_entry(OSPathSpec *spec, unsigned int *typePtr, unsigned int *offsetPtr);
extern int find_or_create_spec_entry_negated(OSPathSpec *input, unsigned int *firstResult, unsigned int *secondResult);
extern int build_name_and_backslash_path(int a, int b, void *buffer1, void *buffer2);
extern int __stdcall parse_value_and_offset(OSPathSpec *text, unsigned short *value, unsigned int *offset);
extern int __stdcall OS_OSSpec_To_FSSpec(OSSpec *input, CWFileSpec *output);
extern void lookup_spec_and_advance_parent(int *id, int *kind, unsigned int **result);
extern int store_mac_spec_entry(MacSpecEntry *entry);
extern struct NameRegistryEntry *find_or_create_name_registry_entry(struct NameRegistryEntry **entries, char *name);

#ifdef __cplusplus
}
#endif

#endif
