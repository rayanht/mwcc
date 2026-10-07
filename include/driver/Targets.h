#ifndef DRIVER_TARGETS_H
#define DRIVER_TARGETS_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DispatchObject {
    unsigned char opaque_state[614];
    struct DispatchTable *dispatch;
};
struct DispatchTable {
    unsigned char reserved[8];
    int(__stdcall *invoke)(struct DispatchObject *, unsigned int, long *);
};
struct IndexedValueTable {
    unsigned char opaquePrefix[32];
    int count;
    unsigned int *values;
};
struct PtrList {
    int count;
    int size;
    char **items;
};
extern int Targets_SetTool(int *tool);
extern Boolean Targets_MatchTool(int cpu, int os, int lang, int type);
extern Boolean Targets_MatchCommandLineOptions(int argc, char **argv);
extern int Targets_RegisterOptionLists(void);
extern int data_005876ac;
extern char *data_00587eb0;
extern char *data_00587eec;
extern char argument_space;
extern char argument_space_char;
extern char data_00588519;
extern char data_0058852c;
extern char data_00588505;
struct DispatchTable;
struct DispatchObject;

#ifdef __cplusplus
}
#endif

#endif
