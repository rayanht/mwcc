#define CERROR_FILE "CLBrowser.c"
#include "compiler/common.h"
#include "driver/CLBrowser.h"
#include "compiler/win32.h"
#include "compiler/CPrep.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLPlugins.h"
#include "driver/CLProj.h"
#include "driver/CLTarg.h"
#include "driver/Files.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include <setjmp.h>
#include <string.h>

/* An entry in the browser's name table. */

/* A browser table entry associates a value with a name. */
#include <stdlib.h>
int fn_004286d0(DropinFileRecord *input, unsigned int processingMode, unsigned int processingFlags)
{
    MemBuffer lookupResult;
    OSSpec state;
    OutputSuffixes *type;
    char *extension;

    type = CLPlugins_GetObjectFlags(input->selectedPlugin);
    state = input->outputPath;
    if (data_00541b95[0] != 0)
        extension = data_00541b95;
    else
        extension = type->suffix0;
    CLProj_ChangeFileExtension(state.name, extension);
    if (DAT_00541b28 != 0) {
        char *result = CLProj_MakeRelativePath(&state, NULL, data_005880e0, 260);
        CLErrors_ForwardMessage(17, result);
    }
    if (build_browser_file_buffer(input->secondaryReferenceHandle, &data_00587570, &lookupResult) == 0)
        return 0;
    if (fn_00415090(&state, processingMode, processingFlags, &lookupResult) == 0)
        return 0;
    return 1;
}

void fn_004287c0(MemBuffer *value, struct CLBrowserLookupEntry **result, unsigned int *shifted_value,
                 unsigned int *raw_value)
{
    DWORD extracted_value;

    OS_GetHandleSize(value, &extracted_value);
    if (result != NULL) {
        *result = MsDos_GetValidMemBufferPtr(value);
    }
    if (shifted_value != NULL) {
        *shifted_value = extracted_value >> 3;
    }
    if (raw_value != NULL) {
        *raw_value = extracted_value;
    }
}

unsigned int CLBrowser_InitMemBuffer(MemBuffer *buffer)
{
    unsigned int error = OS_NewHandle(0U, buffer);
    if (error != 0U) {
        CLErrors_ReportOSError(63, error, (unsigned char *)"allocate", (unsigned char *)"browse file table");
        return 0U;
    }
    return 1U;
}

unsigned int free_lookup_entries(MemBuffer *container)
{
    struct CLBrowserLookupEntry *entry;
    unsigned int count;

    fn_004287c0(container, &entry, &count, NULL);
    while (count--) {
        free(entry->name);
        entry++;
    }
    fn_004129c0(container);
    return 1;
}

unsigned int CLBrowser_FreeMemBuffer(MemBuffer *value)
{
    if (!free_lookup_entries(value))
        return 0U;
    OS_FreeHandle(value);
    return 1U;
}

int CLBrowser_LookupValue(void *table, char *name, short *value)
{
    int index;
    int comparison;
    UInt8 found;
    CLBrowserLookupEntry *entry;
    int count;
    unsigned int tableInfo;

    found = 0;
    index = MsDos_IsAbsolutePath(name);
    if (index == 0) {
        CLIO_ReportAssertionFailure("OS_IsFullPath(fullpath)", "CLBrowser.c", 0x73);
    }
    if (!table) {
        CLIO_ReportAssertionFailure("browsetable!=NULL", "CLBrowser.c", 0x74);
    }
    fn_004287c0(table, &entry, (unsigned int *)&count, &tableInfo);
    index = 0;
    if (0 < count) {
        do {
            comparison = CLIO_CompareStringsIgnoreCase(name, entry->name);
            if (comparison == 0) {
                found = 1;
                break;
            }
            ++index;
            ++entry;
        } while (index < count);
    }
    if (found != 0) {
        *value = entry->value;
    } else {
        *value = 0;
    }
    fn_004129c0(table);
    return found;
}

int CLBrowser_FindOrAddLookupEntry(MemBuffer *browser, char *name, short *result)
{
    CLBrowserLookupEntry *entry;
    unsigned int count, offset;
    char *savedName;
    if (CLBrowser_LookupValue(browser, name, result) == 0) {
        savedName = xstrdup(name);
        fn_004287c0(browser, &entry, &count, &offset);
        fn_004129c0(browser);
        if (OS_ResizeHandle(browser, (count + 1) << 3) != 0) {
            CLIO_FormatAndDispatchText("\nOut of memory\n");
            longjmp(driver_jmp_buf, 1);
        }
        entry = (CLBrowserLookupEntry *)((char *)MsDos_GetValidMemBufferPtr(browser) + offset);
        entry->name = savedName;
        entry->value = count + 1;
        fn_004129c0(browser);
        *result = entry->value;
        return -1;
    }
    return 1;
}

static inline struct BrowserCacheEntry *fn_00428e00_inline1(void)
{
    struct BrowserCacheEntry *cursor;
    struct BrowserCacheEntry *entry;
    entry = NULL;
    cursor = browser_cache_entries;
    while (cursor != NULL) {
        if (cursor->inUse == 0 && (entry == NULL || (unsigned int)cursor->lastUsed < (unsigned int)entry->lastUsed))
            entry = cursor;
        cursor = cursor->next;
    }
    return entry;
}

static inline void fn_00428e00_unlink(struct BrowserCacheEntry *entry)
{
    void (*unlinkEntry)(struct BrowserCacheEntry *) = unlink_browser_cache_entry;
    unlinkEntry(entry);
}

static inline void fn_00428e00_inline2(struct BrowserCacheEntry *entry)
{
    Memory_FreeHandle(entry->buffer);
    cache_free_size += entry->size;
    fn_00428e00_unlink(entry);
    entry->next = browser_cache_free_list;
    browser_cache_free_list = entry;
}

static inline void fn_00428e00_inline3(struct OSSpec *path, struct StorageHandle *buffer, unsigned char flag, int size)
{
    BrowserCacheEntry *newEntry;
    BrowserCacheEntry *(*allocateEntry)(void) = allocate_browser_cache_entry;
    void (*linkEntry)(struct BrowserCacheEntry *) = prepend_browser_cache_entry;
    newEntry = allocateEntry();
    newEntry->inUse = 1;
    newEntry->flag = flag;
    newEntry->path = *path;
    newEntry->buffer = buffer;
    newEntry->size = size;
    newEntry->lastUsed = OS_GetMilliseconds();
    linkEntry(newEntry);
    cache_free_size -= size;
}

unsigned int calculate_lookup_entries_size(CLBrowserLookupEntry *entries, unsigned int count)
{
    unsigned int total = 0;
    while (count--) {
        const char *name = entries->name;
        total += 16U;
        total += (strlen(name) + 7U) & ~7U;
        ++entries;
    }
    return total;
}

int write_lookup_entries(CLBrowserLookupEntry *entries, DstRec *output, UInt32 count)
{
    SInt32 nameLength;
    SInt32 paddedLength;
    while (count--) {
        output->id = entries->value;
        memset(&output->id + 1, 0, 3 * sizeof(UInt32));
        nameLength = strlen(entries->name);
        paddedLength = (nameLength + 7) & ~7;
        output->len = nameLength;
        memset(output->data, 0, paddedLength);
        strncpy(output->data, entries->name, nameLength);
        entries++;
        output = (DstRec *)(output->data + paddedLength);
    }
    return 1;
}

/* Serialized browser file header. */

/* Opaque two-word allocation handle. */

/* Count returned as a word, serialized as a short. */

unsigned int build_browser_file_buffer(struct StorageHandle *dataHandle, void *indexHandle, MemBuffer *result)
{
    int totalSize;
    int indexOffset;
    DWORD error;
    unsigned char *buffer;
    DstRec *indexBuffer;
    int dataSize;
    unsigned int indexSize;
    MemBuffer output;
    struct BrowserFileHeader header;
    CLBrowserLookupEntry *indexData;
    unsigned int itemCount;
    int byteOrder = 1;

    if (dataHandle == NULL) {
        CLIO_ReportAssertionFailure("browsedata!=NULL", "CLBrowser.c", 258);
    }
    if (indexHandle == NULL) {
        CLIO_ReportAssertionFailure("browsetable!=NULL", "CLBrowser.c", 259);
    }
    dataSize = Memory_GetHandleSize(dataHandle);
    indexOffset = (dataSize + sizeof(header) + 7) & -8;
    fn_004287c0(indexHandle, &indexData, &itemCount, NULL);
    indexSize = calculate_lookup_entries_size(indexData, itemCount);
    fn_004129c0(indexHandle);
    totalSize = indexSize + indexOffset;
    memcpy(header.signature, "DubL", sizeof(header.signature));
    header.version = 1;
    header.flag = (*(unsigned char *)&byteOrder == 0);
    memset(header.reserved, 0, sizeof(header.reserved));
    header.dataSize = dataSize;
    header.indexSize = indexSize;
    header.dataOffset = sizeof(header);
    header.indexOffset = indexOffset;
    header.itemCount = itemCount;
    error = OS_NewHandle(totalSize, &output);
    *result = output;
    if (error != 0) {
        CLIO_FormatAndDispatchText("\nOut of memory\n");
        longjmp(driver_jmp_buf, 1);
    }
    buffer = MsDos_GetValidMemBufferPtr(&output);
    memcpy(buffer, &header, sizeof(header));
    fn_00413a00(dataHandle);
    memcpy(buffer + sizeof(header), dataHandle->data, dataSize);
    memset((unsigned char *)((int)buffer + sizeof(header) + dataSize), 0, indexOffset - sizeof(header) - dataSize);
    fn_00413a50(dataHandle);
    fn_004287c0(indexHandle, &indexData, &itemCount, NULL);
    indexBuffer = (DstRec *)(buffer + indexOffset);
    write_lookup_entries(indexData, indexBuffer, itemCount);
    memset(buffer + indexOffset + indexSize, 0, totalSize - indexOffset - indexSize);
    fn_004129c0(indexHandle);
    fn_004129c0(&output);
    return 1;
}

BrowserCacheEntry *allocate_browser_cache_entry(void)
{
    BrowserCacheEntry *entry;
    if (browser_cache_free_list != NULL) {
        entry = browser_cache_free_list;
        browser_cache_free_list = entry->next;
    } else {
        entry = (BrowserCacheEntry *)malloc(0x160);
    }
    return entry;
}

void prepend_browser_cache_entry(struct BrowserCacheEntry *node)
{
    if (browser_cache_entries != NULL) {
        browser_cache_entries->previous = node;
    }
    node->next = browser_cache_entries;
    node->previous = NULL;
    browser_cache_entries = node;
}

void unlink_browser_cache_entry(BrowserCacheEntry *links)
{
    if (links->next != NULL) {
        links->next->previous = links->previous;
    }
    if (links->previous != NULL) {
        links->previous->next = links->next;
    } else {
        browser_cache_entries = links->next;
    }
}

unsigned int CLBrowser_InitCache(void)
{
    unsigned int result;
    BrowserCacheEntry **head = &browser_cache_free_list;
    *head = NULL;
    result = (unsigned int)browser_cache_free_list;
    browser_cache_entries = (BrowserCacheEntry *)result;
    cache_free_size = 8U * 1024 * 1024;
    return result;
}

void CLBrowser_FreeCacheEntries(void)
{
    BrowserCacheEntry *node;
    BrowserCacheEntry *next;
    BrowserCacheEntry *allocation;
    BrowserCacheEntry *nextAllocation;
    StorageHandle *record;
    node = browser_cache_entries;
    if (node != NULL) {
        do {
            next = node->next;
            record = node->buffer;
            Memory_FreeHandle(record);
            free(node);
            node = next;
        } while (next != NULL);
    }
    allocation = browser_cache_free_list;
    while (allocation != NULL) {
        nextAllocation = allocation->next;
        free(allocation);
        allocation = nextAllocation;
    }
    CLBrowser_InitCache();
}

void CLBrowser_CacheFileText(struct OSSpec *path, struct StorageHandle *buffer, unsigned char flag)
{
    int size;
    struct BrowserCacheEntry *entry;
    size = Memory_GetHandleSize(buffer);
    if (size > 8388608) {
        return;
    }
    while ((unsigned int)size > cache_free_size) {
        entry = fn_00428e00_inline1();
        if (entry != NULL) {
            fn_00428e00_inline2(entry);
        } else {
            return;
        }
    }
    fn_00428e00_inline3(path, buffer, flag, size);
}

StorageHandle *CLBrowser_FindCacheEntryBuffer(OSSpec *key, unsigned char *value)
{
    BrowserCacheEntry *entry;
    int matched;
    entry = browser_cache_entries;
    if (browser_cache_entries != NULL) {
        do {
            matched = OS_EqualSpec(&entry->path, key);
            if (matched != 0) {
                matched = OS_GetMilliseconds();
                entry->lastUsed = matched;
                *value = (char)entry->flag;
                ++entry->inUse;
                return entry->buffer;
            }
            entry = entry->next;
        } while (entry != NULL);
    }
    return NULL;
}

void CLBrowser_ReleaseBuffer(StorageHandle *value)
{
    BrowserCacheEntry *entry;

    for (entry = browser_cache_entries; entry != NULL; entry = entry->next) {
        if (entry->buffer == value) {
            --entry->inUse;
            return;
        }
    }
    Memory_FreeHandle(value);
}
