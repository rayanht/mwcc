#ifndef DRIVER_CLFILES_H
#define DRIVER_CLFILES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ChainRecord {
    char name[0x20];
    struct StorageHandle *object;
    struct ChainRecord *next;
};
struct IndexedListLink {
    struct IndexedListLink *next;
    int index;
};
extern unsigned char CLFiles_FreeAllocationRecords(struct IndexedListLink *list);
extern unsigned char CLFiles_InsertIndexedListLinkAtFirstIndex(IndexedListLink *first, IndexedListLink *second);
extern SInt32 CLFiles_GetIndex(IndexedListLink *entry);
extern DropinFileRecord *CLFiles_FindFileByIndex(IndexedListLink *head, int index);
extern unsigned char CLFiles_AssertNonNullIndexedListLink(struct IndexedListLink *value);
extern struct DropinFileRecord *CLFiles_FindDropinFileRecord(struct IndexedListLink *a0, const struct OSSpec *a1);
extern char CLFiles_InsertIndexedListLink(IndexedListLink *list, IndexedListLink *node, int pos);
extern struct DropinFileRecord *CLFiles_AllocDropinFileRecord(void);
extern void free_allocation_record(void *ptr);
extern Boolean CLFiles_InitChain(struct ChainRecord **a0);
extern void CLFiles_FreeChainNodes(struct ChainRecord **nodes);
extern struct ChainRecord *CLFiles_CreateChainRecord(const char *source, StorageHandle *argument);
extern Boolean CLFiles_AppendChainRecord(struct ChainRecord **link, struct ChainRecord *record);
extern ChainRecord *CLFiles_FindChainRecord(ChainRecord *first, const char *value);

#ifdef __cplusplus
}
#endif

#endif
