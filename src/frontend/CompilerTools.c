#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CompilerTools.h"
#include "compiler/objects.h"
#include "compiler/BE_elf.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "driver/COSToolsCLT.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>
#include <ctype.h>

UInt32 low_bit_masks[32] = {0,        0x1,       0x3,       0x7,       0xF,       0x1F,       0x3F,       0x7F,
                            0xFF,     0x1FF,     0x3FF,     0x7FF,     0xFFF,     0x1FFF,     0x3FFF,     0x7FFF,
                            0xFFFF,   0x1FFFF,   0x3FFFF,   0x7FFFF,   0xFFFFF,   0x1FFFFF,   0x3FFFFF,   0x7FFFFF,
                            0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF};

static struct HeapMem galloc_pool;
static struct HeapMem data_0057fd84;
static struct HeapMem block_pool;
static struct HeapMem data_0057fdac;
static struct HeapMem heap_pool;
static void (*data_0057fdd4)();
static SInt16 data_0057fdd8;

static unsigned short swap16(unsigned short x)
{
    union {
        unsigned short w;
        unsigned char b[2];
    } in, out;
    in.w = x;
    out.b[0] = in.b[1];
    out.b[1] = in.b[0];
    return out.w;
}

static unsigned int Swap32(unsigned int x)
{
    union {
        unsigned int w;
        unsigned char b[4];
    } in, out;
    in.w = x;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];
    return out.w;
}

static UInt32 swap32(UInt32 x)
{
    UInt32ByteSwapStorage s, d;
    if (copts.littleendian)
        return x;
    s.word = x;
    d.bytes[0] = s.bytes[3];
    d.bytes[1] = s.bytes[2];
    d.bytes[2] = s.bytes[1];
    d.bytes[3] = s.bytes[0];
    return d.word;
}

static inline void reset_pool_block(HeapBlock *block)
{
    block_pool.curblock = block;
    block_pool.curfreep = (char *)(block + 1);
    block_pool.curfree = block->blocksize - sizeof(HeapBlock);
}

static UInt16 SwapOutputWord(UInt16 x)
{
    union {
        UInt16 w;
        UInt8 b[2];
    } t, r;
    if (copts.littleendian)
        return x;
    t.w = x;
    r.b[0] = t.b[1];
    r.b[1] = t.b[0];
    return r.w;
}

static UInt32 MaybeSwap32(UInt32 x)
{
    union {
        UInt32 l;
        UInt8 b[4];
    } t, r;
    if (copts.littleendian)
        return x;
    t.l = x;
    r.b[0] = t.b[3];
    r.b[1] = t.b[2];
    r.b[2] = t.b[1];
    r.b[3] = t.b[0];
    return r.l;
}

static inline void CopyFourBytes(UInt8 *dest, const UInt8 *source)
{
    dest[0] = source[0];
    dest[1] = source[1];
    dest[2] = source[2];
    dest[3] = source[3];
}

void CompilerGetCString(short value, char *destination)
{
    COS_GetString(destination, 0x2774, value);
}

void CompilerTools_ConvertCStringToPString(unsigned char *text)
{
    unsigned char *end;
    int shifted;
    int length;

    for (end = text, length = 0; *end != '\0'; ++length, ++end) {
    }
    shifted = 0;
    if (0 < length) {
        do {
            *end = end[-1];
            ++shifted;
            --end;
        } while (shifted < length);
    }
    *text = length;
}

SInt16 InitGList(GList *allocation, SInt32 size)
{
    allocation->data = COS_NewOSHandle(size);
    if (!allocation->data) {
        allocation->data = COS_NewHandle(size);
        if (!allocation->data)
            return -1;
    }
    allocation->size = 0;
    allocation->growsize = (int)size >> 1;
    allocation->hndlsize = size;
    return 0;
}

void FreeGList(GList *storage)
{
    if (storage->data != NULL) {
        COS_FreeHandle(storage->data);
        storage->data = NULL;
    }
    storage->hndlsize = 0;
    storage->size = storage->hndlsize;
}

void fn_00442c00(GList *entry)
{
    COS_LockHandleHi(entry->data);
}

void ShrinkGList(GList *list)
{
    list->hndlsize = list->size;
    COS_ResizeHandle((struct StorageHandle *)list->data, list->hndlsize);
}

void *CompilerTools_AppendGListData(GList *buffer, const void *source, SInt32 count)
{
    char *data;
    Boolean result;
    if (buffer->size + count > buffer->hndlsize) {
        buffer->hndlsize += count + buffer->growsize;
        result = COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (result == 0 && data_00587708 != NULL) {
            (*data_00587708)();
        }
    }
    data = memcpy(*buffer->data + buffer->size, source, count);
    buffer->size += count;
    return data;
}

void AppendGListNoData(GList *buffer, SInt32 additionalLength)
{
    if (buffer->size + additionalLength > buffer->hndlsize) {
        buffer->hndlsize += additionalLength + buffer->growsize;
        if (!COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize) && data_00587708 != NULL) {
            data_00587708();
        }
    }
    buffer->size += additionalLength;
}

void AppendGListByte(GList *buffer, SInt8 value)
{
    if (buffer->size + 1 > buffer->hndlsize) {
        Boolean success;
        buffer->hndlsize += buffer->growsize + 1;
        success = COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!success && data_00587708 != NULL) {
            (*data_00587708)();
        }
    }
    (*buffer->data)[buffer->size++] = value;
}

void AppendGListWord(GList *buffer, SInt16 value)
{
    Boolean resized;
    char *destination;
    const char *valueBytes;

    if (buffer->size + (SInt32)sizeof(value) > buffer->hndlsize) {
        buffer->hndlsize += buffer->growsize + (SInt32)sizeof(value);
        resized = COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!resized && data_00587708) {
            (*data_00587708)();
        }
    }
    destination = *buffer->data + buffer->size;
    buffer->size += sizeof(value);
    valueBytes = (const char *)&value;
    *destination = *valueBytes;
    valueBytes = (const char *)&value;
    destination[1] = valueBytes[1];
}

void AppendGListTargetEndianWord(GList *buf, UInt16 word)
{
    char *dest;
    const U16Bytes *bytes;
    if (buf->size + 2 > buf->hndlsize) {
        buf->hndlsize += buf->growsize + 2;
        if (!COS_ResizeHandle((StorageHandle *)buf->data, buf->hndlsize)) {
            if (data_00587708 != NULL)
                data_00587708();
        }
    }
    dest = *buf->data + buf->size;
    buf->size += 2;
    word = SwapOutputWord(word);
    bytes = (const U16Bytes *)&word;
    dest[0] = bytes->b[0];
    dest[1] = ((const U16Bytes *)&word)->b[1];
}

void AppendGListLong(GList *buffer, SInt32 value)
{
    Boolean allocated;
    char *destination;

    if (buffer->size + (SInt32)sizeof(value) > buffer->hndlsize) {
        buffer->hndlsize += buffer->growsize + (SInt32)sizeof(value);
        allocated = COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!allocated && data_00587708 != NULL) {
            (*data_00587708)();
        }
    }
    destination = *buffer->data + buffer->size;
    buffer->size += sizeof(value);
    destination[0] = ((const NativeLongBytes *)&value)->byte0;
    destination[1] = ((const NativeLongBytes *)&value)->byte1;
    destination[2] = ((const NativeLongBytes *)&value)->byte2;
    destination[3] = ((const NativeLongBytes *)&value)->byte3;
}

void AppendGListTargetEndianLong(GList *buf, UInt32 value)
{
    UInt8 *dest;
    if (buf->size + 4 > buf->hndlsize) {
        buf->hndlsize += buf->growsize + 4;
        if (!COS_ResizeHandle((struct StorageHandle *)buf->data, buf->hndlsize)) {
            if (data_00587708 != NULL)
                data_00587708();
        }
    }
    dest = (UInt8 *)*buf->data + buf->size;
    buf->size += 4;
    value = MaybeSwap32(value);
    CopyFourBytes(dest, (const UInt8 *)&value);
}

void AppendGListName(GList *buf, const char *str)
{
    UInt32 len = strlen(str) + 1;
    if (buf->size + len > buf->hndlsize) {
        struct StorageHandle *handle;
        buf->hndlsize += len + buf->growsize;
        handle = (struct StorageHandle *)buf->data;
        if (!COS_ResizeHandle(handle, buf->hndlsize)) {
            if (data_00587708 != NULL) {
                data_00587708();
            }
        }
    }
    memcpy(buf->data[0] + buf->size, str, len);
    buf->size += len;
}

void CompilerTools_AppendGListString(GList *buf, const char *str)
{
    UInt32 len = strlen(str);
    if ((buf->size + len) > (UInt32)buf->hndlsize) {
        buf->hndlsize += len + buf->growsize;
        if (!COS_ResizeHandle((struct StorageHandle *)buf->data, buf->hndlsize) && data_00587708 != NULL)
            data_00587708();
    }
    memcpy(*buf->data + buf->size, str, len);
    buf->size += len;
}

short CHash(const char *str)
{
    short len;
    short i;
    UInt8 c;

    if ((len = strlen(str) & 0xff)) {
        for (i = len, c = 0; i > 0; i--)
            c = ((c >> 3) | (c << 5)) + *str++;
        len = (len << 8) | c;
    }
    return len & 0x7ff;
}

HashNameNode *GetHashNameNodeExport(const char *text)
{
    short hash;
    int bucket;
    unsigned char checksum;
    const char *cursor = text;
    short remaining;
    HashNameNode *entry;
    short length = strlen(text) & 255;
    int allocationSize;
    int firstSize;
    HashNameNode *newEntry;

    if (length != 0) {
        remaining = length;
        checksum = 0;
        while (remaining > 0) {
            remaining--;
            checksum = ((checksum >> 3) | (checksum << 5)) + *cursor++;
        }
        length = checksum | (length << 8);
    }
    hash = length & 2047;
    if ((entry = data_00587f88[hash]) == NULL) {
        firstSize = ((strlen(text) + 12) & -8) + 8;
        if (galloc_pool.curfree < firstSize) {
            select_or_allocate_pool_node(&galloc_pool, firstSize);
        }
        galloc_pool.curfree -= firstSize;
        entry = (HashNameNode *)galloc_pool.curfreep;
        galloc_pool.curfreep += firstSize;
        data_00587f88[bucket = hash] = entry;
        entry->next = NULL;
        entry->id = next_name_id++;
        entry->hashval = hash;
        strcpy(entry->name, text);
        return entry;
    }
    for (;; entry = entry->next) {
        if (strcmp(text, entry->name) == 0) {
            if (entry->id < 0) {
                entry->id = next_name_id++;
            }
            return entry;
        }
        if (entry->next == NULL) {
            allocationSize = strlen(text) + 12;
            newEntry = galloc(allocationSize);
            entry->next = newEntry;
            entry = entry->next;
            entry->next = NULL;
            entry->id = next_name_id++;
            entry->hashval = hash;
            strcpy(entry->name, text);
            return entry;
        }
    }
}

HashNameNode *GetHashNameNode(const char *text)
{
    short bucket;
    unsigned char hash;
    const char *cursor;
    short remaining;
    HashNameNode *entry;
    short key;
    int allocationSize;
    HashNameNode *storage;
    HashNameNode *tailEntry;

    cursor = text;
    key = strlen(text) & 255;
    if (key != 0) {
        remaining = key;
        hash = 0;
        while (remaining > 0) {
            --remaining;
            hash = ((hash >> 3) | (hash << 5)) + *cursor;
            cursor = cursor + 1;
        }
        key = hash | (key << 8);
    }
    bucket = key & 2047;
    if ((entry = data_00587f88[bucket]) == NULL) {
        int firstAllocationSize = ((strlen(text) + 12) & -8) + 8;
        if (galloc_pool.curfree < firstAllocationSize) {
            select_or_allocate_pool_node(&galloc_pool, firstAllocationSize);
        }
        galloc_pool.curfree -= firstAllocationSize;
        entry = (HashNameNode *)galloc_pool.curfreep;
        galloc_pool.curfreep += firstAllocationSize;
        data_00587f88[bucket] = entry;
        entry->next = NULL;
        entry->id = -1;
        entry->hashval = bucket;
        strcpy(entry->name, text);
        return entry;
    }
    for (;;) {
        if (strcmp(text, entry->name) == 0) {
            return entry;
        }
        if (entry->next == NULL) {
            allocationSize = strlen(text) + 12;
            storage = galloc(allocationSize);
            entry->next = storage;
            tailEntry = entry->next;
            tailEntry->next = NULL;
            tailEntry->id = -1;
            tailEntry->hashval = bucket;
            strcpy(tailEntry->name, text);
            return tailEntry;
        }
        entry = entry->next;
    }
}

void InitNameHash(void)
{
    HashNameNode *(*buckets)[2048];

    {
        int *active;

        buckets = galloc(8192);
        active = &next_name_id;
        data_00587f88 = (HashNameNode **)buckets;
        memset(data_00587f88, 0, sizeof(*buckets));
        *active = 1;
    }
}

int CTool_TotalHeapSize(void)
{
    unsigned int total = 0;
    HeapBlock *node;

    for (node = galloc_pool.blocks; node != NULL; node = node->next)
        total += node->blocksize;
    for (node = data_0057fd84.blocks; node != NULL; node = node->next)
        total += node->blocksize;
    for (node = block_pool.blocks; node != NULL; node = node->next)
        total += node->blocksize;
    for (node = data_0057fdac.blocks; node != NULL; node = node->next)
        total += node->blocksize;
    for (node = heap_pool.blocks; node != NULL; node = node->next)
        total += node->blocksize;

    return total;
}

int select_or_allocate_pool_node(HeapMem *pool, SInt32 size)
{
    HeapBlock **block;
    HeapBlock *node;

    if ((node = pool->blocks) != NULL) {
        pool->curblock->blockfree = pool->curfree;
        while (node != NULL) {
            if (node->blockfree >= size)
                goto selected;
            node = node->next;
        }
    }
    size += pool->allocsize;
    if (!data_0054c3d0)
        goto fallback;
    block = COS_NewOSHandle(size * 2);
    if (block == NULL) {
        block = COS_NewOSHandle(size);
        if (block == NULL) {
        fallback:
            block = COS_NewHandle(size);
            if (block == NULL) {
                if (data_0057fdd4 != NULL)
                    data_0057fdd4();
                return;
            }
        }
    } else {
        size <<= 1;
    }
    COS_LockHandleHi(block);
    node = *block;
    node->next = pool->blocks;
    pool->blocks = node;
    node->blockhandle = block;
    node->blocksize = size;
    node->blockfree = size - sizeof(HeapBlock);
selected:
    pool->curblock = node;
    pool->curfree = node->blockfree;
    pool->curfreep = (char *)node + node->blocksize - node->blockfree;
}

SInt16 CompilerTools_InitHeaps(void (*param)())
{
    data_0057fdd4 = NULL;
    data_0057fdd8 = 0;
    memset(&galloc_pool, 0, 20);
    memset(&data_0057fd84, 0, 20);
    memset(&block_pool, 0, 20);
    memset(&data_0057fdac, 0, 20);
    memset(&heap_pool, 0, 20);
    galloc_pool.allocsize = 0x38000;
    data_0057fd84.allocsize = 0x10000;
    block_pool.allocsize = 0x4000;
    data_0057fdac.allocsize = 0x4000;
    heap_pool.allocsize = 0x4000;
    select_or_allocate_pool_node(&galloc_pool, 0);
    select_or_allocate_pool_node(&data_0057fd84, 0);
    select_or_allocate_pool_node(&block_pool, 0);
    select_or_allocate_pool_node(&data_0057fdac, 0);
    select_or_allocate_pool_node(&heap_pool, 0);
    galloc_pool.allocsize = 0x8000;
    data_0057fd84.allocsize = 0x8000;
    data_0057fdd4 = param;
    if (!(galloc_pool.curblock && data_0057fd84.curblock && block_pool.curblock && data_0057fdac.curblock &&
          heap_pool.curblock))
        return -1;
    return 0;
}

SInt16 initheaps(void (*param)())
{
    HeapMem *pool = &galloc_pool;
    data_0057fdd4 = NULL;
    data_0057fdd8 = 0;
    memset(&galloc_pool, 0, sizeof(galloc_pool));
    memset(&data_0057fd84, 0, sizeof(data_0057fd84));
    memset(&block_pool, 0, sizeof(block_pool));
    memset(&data_0057fdac, 0, sizeof(data_0057fdac));
    memset(&heap_pool, 0, sizeof(heap_pool));
    galloc_pool.allocsize = 0x38000;
    select_or_allocate_pool_node(pool, 0);
    galloc_pool.allocsize = 0x8000;
    data_0057fdd4 = param;
    if (galloc_pool.curblock == NULL)
        return -1;
    return 0;
}

void releaseheaps(void)
{
    HeapBlock *node;
    HeapBlock **block;

    node = galloc_pool.blocks;
    while (node != NULL) {
        block = node->blockhandle;
        node = node->next;
        COS_FreeHandle(block);
    }
    memset(&galloc_pool, 0, sizeof(galloc_pool));
    node = data_0057fd84.blocks;
    while (node != NULL) {
        block = node->blockhandle;
        node = node->next;
        COS_FreeHandle(block);
    }
    memset(&data_0057fd84, 0, sizeof(data_0057fd84));
    node = block_pool.blocks;
    while (node != NULL) {
        block = node->blockhandle;
        node = node->next;
        COS_FreeHandle(block);
    }
    memset(&block_pool, 0, sizeof(block_pool));
    node = data_0057fdac.blocks;
    while (node != NULL) {
        block = node->blockhandle;
        node = node->next;
        COS_FreeHandle(block);
    }
    memset(&data_0057fdac, 0, sizeof(data_0057fdac));
    node = heap_pool.blocks;
    while (node != NULL) {
        block = node->blockhandle;
        node = node->next;
        COS_FreeHandle(block);
    }
    memset(&heap_pool, 0, sizeof(heap_pool));
}

void CompilerTools_ClearPoolBlocks(void)
{
    HeapBlock *link;

    link = galloc_pool.blocks;
    while (link != NULL) {
        HeapBlock **block = link->blockhandle;
        link = link->next;
        COS_FreeHandle(block);
    }
    memset(&galloc_pool, 0, sizeof(galloc_pool));
}

void *galloc(SInt32 size)
{
    char *result;

    size = (size & ~7) + 8;
    if (galloc_pool.curfree < size) {
        select_or_allocate_pool_node(&galloc_pool, size);
    }
    galloc_pool.curfree -= size;
    result = galloc_pool.curfreep;
    galloc_pool.curfreep += size;
    return result;
}

void *CompilerTools_AllocatePool(unsigned int size)
{
    char *allocation;
    size = (size & ~7U) + 8U;
    if (data_0057fd84.curfree < (SInt32)size) {
        HeapMem *pool = &data_0057fd84;
        select_or_allocate_pool_node(pool, size);
    }
    data_0057fd84.curfree -= size;
    allocation = data_0057fd84.curfreep;
    data_0057fd84.curfreep += size;
    return allocation;
}

void *CompilerTools_AllocateBlock(SInt32 size)
{
    char *block;
    size = (size & ~7U) + 8U;
    if (block_pool.curfree < size) {
        select_or_allocate_pool_node(&block_pool, size);
    }
    block_pool.curfree -= size;
    block = block_pool.curfreep;
    block_pool.curfreep += size;
    return block;
}

void *CompilerTools_AllocatePoolMemory(UInt32 requestedSize)
{
    char *result;

    requestedSize = (requestedSize & ~7U) + 8U;

    if (data_0057fdac.curfree < (SInt32)requestedSize)
        select_or_allocate_pool_node(&data_0057fdac, requestedSize);

    data_0057fdac.curfree -= requestedSize;
    result = data_0057fdac.curfreep;
    data_0057fdac.curfreep += requestedSize;
    return result;
}

void fn_00441f10(void)
{
    data_0057fdd8 += 1;
    return;
}

void CompilerTools_DecrementPositiveCounter(void)

{
    if (0 < data_0057fdd8) {
        --data_0057fdd8;
    }
    return;
}

#pragma sym off

void freelheap(void)
{
    HeapBlock *block;

    if (data_0057fdd8 == 0) {
        block = data_0057fd84.blocks;
        data_0057fd84.curblock = block;
        data_0057fd84.curfreep = (char *)block + sizeof(*block);
        data_0057fd84.curfree = block->blocksize - sizeof(HeapBlock);

        while (block != NULL) {
            block->blockfree = block->blocksize - sizeof(HeapBlock);
            block = block->next;
        }
    }
}

#pragma sym reset

void CompilerTools_ResetPoolAvail(void)
{
    HeapBlock *block;
    block = block_pool.blocks;
    reset_pool_block(block);
    for (; block; block = block->next)
        block->blockfree = block->blocksize - sizeof(HeapBlock);
}

#pragma sym off

void CompilerTools_ResetPool(void)
{
    HeapBlock *block = data_0057fdac.blocks;

    data_0057fdac.curblock = block;
    data_0057fdac.curfreep = (char *)&block[1];
    data_0057fdac.curfree = block->blocksize - sizeof(HeapBlock);
    while (block) {
        block->blockfree = block->blocksize - sizeof(HeapBlock);
        block = block->next;
    }
}

#pragma sym reset

char *ScanDec(char *src, SInt32 *value, Boolean *flag)
{
    unsigned int val = 0U;
    short digit;
    *flag = 0;
    for (;;) {
        digit = (short)(*src - '0');
        if (digit < 0 || digit > 9)
            break;
        if (val >= 0x19999999U) {
            if (val > 0x19999999U || digit > 5)
                *flag = 1;
        }
        val = (val << 3) + (val << 1) + digit;
        src++;
    }
    *value = val;
    return src;
}

void memclrw(void *buffer, unsigned int size)
{
    memset(buffer, 0, size);
}

void CToLowercase(char *src, char *dst)
{
    while ((*dst++ = (char)tolower(*src++)) != 0)
        ;
}

SInt16 getbit(UInt32 value)
{
    switch (value) {
        case 0:
            return -1;
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 2;
        case 8:
            return 3;
        case 0x10:
            return 4;
        case 0x20:
            return 5;
        case 0x40:
            return 6;
        case 0x80:
            return 7;
        case 0x100:
            return 8;
        case 0x200:
            return 9;
        case 0x400:
            return 10;
        case 0x800:
            return 11;
        case 0x1000:
            return 12;
        case 0x2000:
            return 13;
        case 0x4000:
            return 14;
        case 0x8000:
            return 15;
        case 0x10000:
            return 16;
        case 0x20000:
            return 17;
        case 0x40000:
            return 18;
        case 0x80000:
            return 19;
        case 0x100000:
            return 20;
        case 0x200000:
            return 21;
        case 0x400000:
            return 22;
        case 0x800000:
            return 23;
        case 0x1000000:
            return 24;
        case 0x2000000:
            return 25;
        case 0x4000000:
            return 26;
        case 0x8000000:
            return 27;
        case 0x10000000:
            return 28;
        case 0x20000000:
            return 29;
        case 0x40000000:
            return 30;
        case 0x80000000:
            return 31;
    }
    return -2;
}

UInt16 CTool_EndianConvertWord16(UInt16 word)
{
    union {
        struct {
            UInt16 value;
            UInt16 swapped;
        } words;
        unsigned char bytes[4];
    } result;
    if (copts.littleendian != 0) {
        return word;
    }
    result.words.value = word;
    result.bytes[2] = result.bytes[1];
    result.bytes[3] = result.bytes[0];
    return result.words.swapped;
}

unsigned int CTool_EndianConvertWord32(unsigned int value)
{
    union {
        unsigned int words[2];
        unsigned char bytes[8];
    } converted;
    if (copts.littleendian != 0) {
        return value;
    }
    converted.words[0] = value;
    converted.bytes[4] = converted.bytes[3];
    converted.bytes[5] = converted.bytes[2];
    converted.bytes[6] = converted.bytes[1];
    converted.bytes[7] = converted.bytes[0];
    return converted.words[1];
}

UInt32 CTool_EndianConvertMem(void *buffer, short size)
{
    unsigned char *front;
    unsigned char *back;
    unsigned char saved;

    if (copts.littleendian != 0)
        return;

    front = buffer;
    back = front + size;

    for (;;) {
        back--;
        if (back <= front)
            break;
        saved = *back;
        *back = *front;
        *front = saved;
        front++;
    }
}

void CTool_EndianConvertWord64(CInt64 ci, char *result)
{
    UInt32 buf[2];
    UInt32 h;
    if (copts.littleendian == 0) {
        h = ci.hi;
        buf[0] = swap32(h);
        buf[1] = swap32(ci.lo);
    } else {
        buf[0] = swap32(ci.lo);
        h = ci.hi;
        buf[1] = swap32(h);
    }
    memcpy(result, buf, 8);
}

short CTool_EndianConvertInPlaceWord16Ptr(short *word)
{
    short *destination = word;
    short value = *destination;
    if (copts.littleendian == 0) {
        value = swap16(value);
    }
    *destination = value;
    return *destination;
}

unsigned int CTool_EndianConvertInPlaceWord32Ptr(unsigned int *p)
{
    unsigned int value = *p;
    value = copts.littleendian ? value : Swap32(value);
    *p = value;
    value = *p;
    return value;
}

/* The linker stripped the functions that used these literals; they stay in the unit's .data. */
static void CompilerTools_NameLiterals(const char **literals)
{
    literals[0] = "operator new";
    literals[1] = "operator delete";
    literals[2] = "(void)";
    literals[3] = "(...)";
}
