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
    SInt32 nameOffset;
    struct AccessPathEntry *searchPath;
    struct AccessPathEntry *resolvedPath;
    struct AccessPathEntry *contextPath;
    UInt8 flag;
    UInt8 trailingPadding[3];
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
    struct CLTarget *target;
    SInt32 count;
    SInt32 alloccount;
    struct DepRecord *records;
    SInt32 textsize;
    SInt32 textused;
    char *text;
    struct AccessPaths *scope;
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
