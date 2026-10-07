#ifndef DRIVER_CLDEPENDENCIES_H
#define DRIVER_CLDEPENDENCIES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AccessPathEntry {
    OSPathSpec *path;
    struct AccessPaths *children;
    unsigned int auxiliary2;
};
#pragma options align = mac68k
struct DepRecord {
    SInt32 nameOffset; /* 0x00: append_dep_record stores textused; make_dependency_osspec reads the dependency name */
    struct AccessPathEntry *
        searchPath; /* 0x04: CLDependencies_FindFile caches the access path found by find_dependency_access_path_entry through append_dep_record */
    struct AccessPathEntry
        *resolvedPath; /* 0x08: append_dep_record resolves the directory; make_dependency_osspec reads its path */
    struct AccessPathEntry *
        contextPath; /* 0x0c: append_dep_record saves get_access_path_entry; CLDependencies_FindFile compares data_0054d898 */
    UInt8 flag; /* 0x10: append_dep_record stores flag; fn_00427ad0 filters cached entries */
    UInt8 trailingPadding
        [3]; /* 0x11: append_dep_record leaves these trailing bytes unused; sizeof(DepRecord) sets the record stride */
};
#pragma options align = reset
struct DependencyCollection {
    int count;
    int capacity;
    int *entries;
    struct Deps *dependencyTable;
};
#pragma options align = mac68k
struct Deps {
    struct CLTarget
        *target;       /* 0x00: CLDependencies_InitDeps sets the target; CLDependencies_FindFile searches its paths */
    SInt32 count;      /* 0x04: fn_00427ad0 bounds the dependency search */
    SInt32 alloccount; /* 0x08: append_dep_record grows record capacity */
    struct DepRecord
        *records;    /* 0x0c: make_dependency_osspec indexes dependencies; get_record_flag reads the flag word */
    SInt32 textsize; /* 0x10: append_dep_record grows text storage */
    SInt32 textused; /* 0x14: append_dep_record assigns record text offsets */
    char *text;      /* 0x18: make_dependency_osspec reads dependency names */
    struct AccessPaths *
        scope; /* 0x1c: CLDependencies_InitDeps allocates and initializes AccessPaths; CLDependencies_FreeDeps frees its items */
};
#pragma options align = reset
extern Boolean find_dependency_access_path_entry(AccessPaths *dependencies, char *a2, AccessPathEntry **a3, char *a4);
extern void append_dep_record(Deps *deps, char *name, UInt8 flag, AccessPathEntry *obj, AccessPathEntry *type,
                              AccessPathEntry *unused, SInt32 *out);
extern unsigned char CLDependencies_FindFile(Deps *dependencies, char *file, char searchFirst, OSSpec *context,
                                             SInt32 *index);
extern SInt32 CLDependencies_SetAccessPath(OSSpec *name, Boolean flag);
extern Boolean initialize_four_words(DependencyCollection *value, struct Deps *fourth);
extern char *escape_spaces(char escapeSpaces, char *destination, char *source);
extern void CLDependencies_WriteDependencies(Deps *ctx, DropinFileRecord *file, OSHandle *stream);
extern unsigned char CLDependencies_InitDeps(Deps *state, CLTarget *input);
extern void CLDependencies_FreeDeps(Deps *block);
extern unsigned char get_record_flag(struct Deps *table, unsigned int index);
extern void make_dependency_osspec(Deps *table, int index, char *output);
extern unsigned char fn_00427ad0(Deps *table, char flags, char *key, char *argument, SInt32 *index_out,
                                 DepRecord **entry_out);
extern Boolean dep_records_equal(Deps *table, SInt32 firstIndex, SInt32 secondIndex);
extern AccessPathEntry *find_or_create_access_path_entry(AccessPaths *scope, OSPathSpec *name);
extern unsigned char find_access_path_entry(AccessPathEntry *entry, char *comparison, AccessPathEntry **result,
                                            char *context);
extern AccessPathEntry *get_access_path_entry(void);
extern UInt8 contains_dependency(DependencyCollection *collection, int key);
extern void append_dependency_entry(DependencyCollection *v, int x, signed char flag);
extern void CLDependencies_InsertDependencyIfAbsent(DependencyCollection *collection, SInt32 index, OSSpec *name,
                                                    unsigned char arg4, signed char arg5, char *result);

#ifdef __cplusplus
}
#endif

#endif
