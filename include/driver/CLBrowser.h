#ifndef DRIVER_CLBROWSER_H
#define DRIVER_CLBROWSER_H

#include <setjmp.h>
#include "compiler/common.h"
#include "driver/MsDos.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BrowserCacheEntry {
    struct BrowserCacheEntry *next;
    struct BrowserCacheEntry *previous;
    int inUse;
    int flag;
    struct StorageHandle *buffer;
    OSSpec path;
    int size;
    int lastUsed;
};
struct BrowserFileHeader {
    char signature[4];
    char version;
    char flag;
    char reserved[16];
    short itemCount;
    int dataOffset;
    int dataSize;
    int indexOffset;
    int indexSize;
};
struct CLBrowserLookupEntry {
    unsigned short value;
    char *name;
};
#pragma options align = mac68k
struct DstRec {
    UInt16 id;             /* 0x00: write_lookup_entries stores entries->value */
    UInt32 zeroFillFirst;  /* 0x02: write_lookup_entries zeroes three UInt32 words following id */
    UInt32 zeroFillSecond; /* 0x06: write_lookup_entries zeroes three UInt32 words following id */
    UInt32 zeroFillThird;  /* 0x0a: write_lookup_entries zeroes three UInt32 words following id */
    UInt16 len;            /* 0x0e: write_lookup_entries stores the name length */
    char data[1];          /* 0x10: write_lookup_entries writes the eight-byte-padded name */
};
#pragma options align = reset
extern unsigned int calculate_lookup_entries_size(CLBrowserLookupEntry *entries, unsigned int count);
extern int CLBrowser_LookupValue(void *table, char *name, short *value);
extern int CLBrowser_FindOrAddLookupEntry(MemBuffer *browser, char *name, short *result);
extern int write_lookup_entries(CLBrowserLookupEntry *src, DstRec *dst, UInt32 count);
extern unsigned int build_browser_file_buffer(struct StorageHandle *dataHandle, void *indexHandle, MemBuffer *result);
extern void fn_004287c0(MemBuffer *value, struct CLBrowserLookupEntry **result, unsigned int *shifted_value,
                        unsigned int *raw_value);
extern unsigned int CLBrowser_InitMemBuffer(MemBuffer *a0);
extern unsigned int free_lookup_entries(MemBuffer *container);
extern unsigned int CLBrowser_FreeMemBuffer(MemBuffer *value);
extern BrowserCacheEntry *allocate_browser_cache_entry(void);
extern void prepend_browser_cache_entry(struct BrowserCacheEntry *node);
extern void unlink_browser_cache_entry(BrowserCacheEntry *links);
extern unsigned int CLBrowser_InitCache(void);
extern void CLBrowser_FreeCacheEntries(void);
extern void CLBrowser_CacheFileText(struct OSSpec *path, struct StorageHandle *buffer, unsigned char flag);
extern StorageHandle *CLBrowser_FindCacheEntryBuffer(OSSpec *key, unsigned char *value);
extern void CLBrowser_ReleaseBuffer(StorageHandle *value);
extern int fn_004286d0(DropinFileRecord *input, unsigned int processingMode, unsigned int processingFlags);
extern jmp_buf driver_jmp_buf;

#ifdef __cplusplus
}
#endif

#endif
