#ifndef DRIVER_CLPREFS_H
#define DRIVER_CLPREFS_H

#include <setjmp.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NameTableEntry {
    char *name;
    union {
        struct {
            struct StorageHandle *destination;
            struct StorageHandle *temporary;
        } storage;
        struct {
            unsigned char **source;
            unsigned char **destination;
        } preference;
    } handles;
    struct NameTableEntry *next;
};
extern StorageHandle *CLPrefs_CopyDestinationToTemporary(NameTableEntry *preferences);
extern int CLPrefs_CopyStorage(NameTableEntry *storage, StorageHandle *source);
extern Boolean CLPrefs_AddPrefPanel(NameTableEntry *entry);
extern NameTableEntry *CLPrefs_FindNameTableEntry(char *name);
extern unsigned int CLPrefs_ConvertLoneLFToCR(StorageHandle *buffer);

#ifdef __cplusplus
}
#endif

#endif
