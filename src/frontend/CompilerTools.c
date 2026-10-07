#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CompilerTools.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/win32.h"
#include "compiler/BE_symbol.h"
#include "compiler/BE_elf.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CodeGen.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Unmangle.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
#include "driver/CLPluginRequests.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/libimp-eabi-ppc.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <setjmp.h>

#define VA_ARG(ap, T) (*(T *)((ap += 4) - 4))

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

unsigned int CTool_EndianConvertInPlaceWord32Ptr(unsigned int *p)
{
    unsigned int value = *p;
    value = copts.nativeByteOrder ? value : Swap32(value);
    *p = value;
    value = *p;
    return value;
}

short CTool_EndianConvertInPlaceWord16Ptr(short *word)
{
    short *destination = word;
    short value = *destination;
    if (copts.nativeByteOrder == 0) {
        value = swap16(value);
    }
    *destination = value;
    return *destination;
}

static UInt32 swap32(UInt32 x)
{
    UInt32ByteSwapStorage s, d;
    if (copts.nativeByteOrder)
        return x;
    s.word = x;
    d.bytes[0] = s.bytes[3];
    d.bytes[1] = s.bytes[2];
    d.bytes[2] = s.bytes[1];
    d.bytes[3] = s.bytes[0];
    return d.word;
}

void CTool_EndianConvertWord64(CInt64 ci, char *result)
{
    UInt32 buf[2];
    UInt32 h;
    if (copts.nativeByteOrder == 0) {
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

UInt32 CTool_EndianConvertMem(void *buffer, short size)
{
    unsigned char *front;
    unsigned char *back;
    unsigned char saved;

    if (copts.nativeByteOrder != 0)
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

unsigned int CTool_EndianConvertWord32(unsigned int value)
{
    union {
        unsigned int words[2];
        unsigned char bytes[8];
    } converted;
    if (copts.nativeByteOrder != 0) {
        return value;
    }
    converted.words[0] = value;
    converted.bytes[4] = converted.bytes[3];
    converted.bytes[5] = converted.bytes[2];
    converted.bytes[6] = converted.bytes[1];
    converted.bytes[7] = converted.bytes[0];
    return converted.words[1];
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
    if (copts.nativeByteOrder != 0) {
        return word;
    }
    result.words.value = word;
    result.bytes[2] = result.bytes[1];
    result.bytes[3] = result.bytes[0];
    return result.words.swapped;
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

void CToLowercase(char *src, char *dst)
{
    while ((*dst++ = (char)tolower(*src++)) != 0)
        ;
}

void memclrw(void *buffer, unsigned int size)
{
    memset(buffer, 0, size);
}

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

#pragma sym off

void CompilerTools_ResetPool(void)
{
    PoolNode *block = data_0057fdac.head;

    data_0057fdac.cur = block;
    data_0057fdac.ptr = (char *)&block[1];
    data_0057fdac.free = block->size - sizeof(PoolNode);
    while (block) {
        block->avail = block->size - sizeof(PoolNode);
        block = block->next;
    }
}

#pragma sym reset

static inline void reset_pool_block(PoolNode *block)
{
    block_pool.cur = block;
    block_pool.ptr = (char *)(block + 1);
    block_pool.free = block->size - sizeof(PoolNode);
}

void CompilerTools_ResetPoolAvail(void)
{
    PoolNode *block;
    block = block_pool.head;
    reset_pool_block(block);
    for (; block; block = block->next)
        block->avail = block->size - sizeof(PoolNode);
}

#pragma sym off

void freelheap(void)
{
    PoolNode *block;

    if (data_0057fdd8 == 0) {
        block = data_0057fd84.head;
        data_0057fd84.cur = block;
        data_0057fd84.ptr = (char *)block + sizeof(*block);
        data_0057fd84.free = block->size - sizeof(PoolNode);

        while (block != NULL) {
            block->avail = block->size - sizeof(PoolNode);
            block = block->next;
        }
    }
}

#pragma sym reset

void CompilerTools_DecrementPositiveCounter(void)

{
    if (0 < data_0057fdd8) {
        --data_0057fdd8;
    }
    return;
}

void fn_00441f10(void)
{
    data_0057fdd8 += 1;
    return;
}

void *CompilerTools_AllocatePoolMemory(UInt32 requestedSize)
{
    char *result;

    requestedSize = (requestedSize & ~7U) + 8U;

    if (data_0057fdac.free < (SInt32)requestedSize)
        select_or_allocate_pool_node(&data_0057fdac, requestedSize);

    data_0057fdac.free -= requestedSize;
    result = data_0057fdac.ptr;
    data_0057fdac.ptr += requestedSize;
    return result;
}

void *CompilerTools_AllocateBlock(SInt32 size)
{
    char *block;
    size = (size & ~7U) + 8U;
    if (block_pool.free < size) {
        select_or_allocate_pool_node(&block_pool, size);
    }
    block_pool.free -= size;
    block = block_pool.ptr;
    block_pool.ptr += size;
    return block;
}

void *CompilerTools_AllocatePool(unsigned int size)
{
    char *allocation;
    size = (size & ~7U) + 8U;
    if (data_0057fd84.free < (SInt32)size) {
        Pool *pool = &data_0057fd84;
        select_or_allocate_pool_node(pool, size);
    }
    data_0057fd84.free -= size;
    allocation = data_0057fd84.ptr;
    data_0057fd84.ptr += size;
    return allocation;
}

void *galloc(SInt32 size)
{
    char *result;

    size = (size & ~7) + 8;
    if (galloc_pool.free < size) {
        select_or_allocate_pool_node(&galloc_pool, size);
    }
    galloc_pool.free -= size;
    result = galloc_pool.ptr;
    galloc_pool.ptr += size;
    return result;
}

void CompilerTools_ClearPoolBlocks(void)
{
    PoolNode *link;

    link = galloc_pool.head;
    while (link != NULL) {
        PoolNode **block = link->block;
        link = link->next;
        fn_00443160(block);
    }
    memset(&galloc_pool, 0, sizeof(galloc_pool));
}

void releaseheaps(void)
{
    PoolNode *node;
    PoolNode **block;

    node = galloc_pool.head;
    while (node != NULL) {
        block = node->block;
        node = node->next;
        fn_00443160(block);
    }
    memset(&galloc_pool, 0, sizeof(galloc_pool));
    node = data_0057fd84.head;
    while (node != NULL) {
        block = node->block;
        node = node->next;
        fn_00443160(block);
    }
    memset(&data_0057fd84, 0, sizeof(data_0057fd84));
    node = block_pool.head;
    while (node != NULL) {
        block = node->block;
        node = node->next;
        fn_00443160(block);
    }
    memset(&block_pool, 0, sizeof(block_pool));
    node = data_0057fdac.head;
    while (node != NULL) {
        block = node->block;
        node = node->next;
        fn_00443160(block);
    }
    memset(&data_0057fdac, 0, sizeof(data_0057fdac));
    node = heap_pool.head;
    while (node != NULL) {
        block = node->block;
        node = node->next;
        fn_00443160(block);
    }
    memset(&heap_pool, 0, sizeof(heap_pool));
}

SInt16 initheaps(void (*param)())
{
    Pool *pool = &galloc_pool;
    data_0057fdd4 = NULL;
    data_0057fdd8 = 0;
    memset(&galloc_pool, 0, sizeof(galloc_pool));
    memset(&data_0057fd84, 0, sizeof(data_0057fd84));
    memset(&block_pool, 0, sizeof(block_pool));
    memset(&data_0057fdac, 0, sizeof(data_0057fdac));
    memset(&heap_pool, 0, sizeof(heap_pool));
    galloc_pool.overhead = 0x38000;
    select_or_allocate_pool_node(pool, 0);
    galloc_pool.overhead = 0x8000;
    data_0057fdd4 = param;
    if (galloc_pool.cur == NULL)
        return -1;
    return 0;
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
    galloc_pool.overhead = 0x38000;
    data_0057fd84.overhead = 0x10000;
    block_pool.overhead = 0x4000;
    data_0057fdac.overhead = 0x4000;
    heap_pool.overhead = 0x4000;
    select_or_allocate_pool_node(&galloc_pool, 0);
    select_or_allocate_pool_node(&data_0057fd84, 0);
    select_or_allocate_pool_node(&block_pool, 0);
    select_or_allocate_pool_node(&data_0057fdac, 0);
    select_or_allocate_pool_node(&heap_pool, 0);
    galloc_pool.overhead = 0x8000;
    data_0057fd84.overhead = 0x8000;
    data_0057fdd4 = param;
    if (!(galloc_pool.cur && data_0057fd84.cur && block_pool.cur && data_0057fdac.cur && heap_pool.cur))
        return -1;
    return 0;
}

int select_or_allocate_pool_node(Pool *pool, SInt32 size)
{
    PoolNode **block;
    PoolNode *node;

    if ((node = pool->head) != NULL) {
        pool->cur->avail = pool->free;
        while (node != NULL) {
            if (node->avail >= size)
                goto selected;
            node = node->next;
        }
    }
    size += pool->overhead;
    if (!DAT_0054c3d0)
        goto fallback;
    block = CompilerTools_AllocateMemoryIfEnabled(size * 2);
    if (block == NULL) {
        block = CompilerTools_AllocateMemoryIfEnabled(size);
        if (block == NULL) {
        fallback:
            block = fn_00443110(size);
            if (block == NULL) {
                if (data_0057fdd4 != NULL)
                    data_0057fdd4();
                return;
            }
        }
    } else {
        size <<= 1;
    }
    fn_004431a0(block);
    node = *block;
    node->next = pool->head;
    pool->head = node;
    node->block = block;
    node->size = size;
    node->avail = size - sizeof(PoolNode);
selected:
    pool->cur = node;
    pool->free = node->avail;
    pool->ptr = (char *)node + node->size - node->avail;
}

int CTool_TotalHeapSize(void)
{
    unsigned int total = 0;
    PoolNode *node;

    for (node = galloc_pool.head; node != NULL; node = node->next)
        total += node->size;
    for (node = data_0057fd84.head; node != NULL; node = node->next)
        total += node->size;
    for (node = block_pool.head; node != NULL; node = node->next)
        total += node->size;
    for (node = data_0057fdac.head; node != NULL; node = node->next)
        total += node->size;
    for (node = heap_pool.head; node != NULL; node = node->next)
        total += node->size;

    return total;
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
        if (galloc_pool.free < firstAllocationSize) {
            select_or_allocate_pool_node(&galloc_pool, firstAllocationSize);
        }
        galloc_pool.free -= firstAllocationSize;
        entry = (HashNameNode *)galloc_pool.ptr;
        galloc_pool.ptr += firstAllocationSize;
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
        if (galloc_pool.free < firstSize) {
            select_or_allocate_pool_node(&galloc_pool, firstSize);
        }
        galloc_pool.free -= firstSize;
        entry = (HashNameNode *)galloc_pool.ptr;
        galloc_pool.ptr += firstSize;
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

static UInt16 SwapOutputWord(UInt16 x)
{
    union {
        UInt16 w;
        UInt8 b[2];
    } t, r;
    if (copts.nativeByteOrder)
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
    if (copts.nativeByteOrder)
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

void CompilerTools_AppendGListString(GList *buf, const char *str)
{
    UInt32 len = strlen(str);
    if ((buf->size + len) > (UInt32)buf->hndlsize) {
        buf->hndlsize += len + buf->growsize;
        if (!fn_00443170((struct StorageHandle *)buf->data, buf->hndlsize) && DAT_00587708 != NULL)
            DAT_00587708();
    }
    memcpy(*buf->data + buf->size, str, len);
    buf->size += len;
}

void AppendGListName(GList *buf, const char *str)
{
    UInt32 len = strlen(str) + 1;
    if (buf->size + len > buf->hndlsize) {
        struct StorageHandle *handle;
        buf->hndlsize += len + buf->growsize;
        handle = (struct StorageHandle *)buf->data;
        if (!fn_00443170(handle, buf->hndlsize)) {
            if (DAT_00587708 != NULL) {
                DAT_00587708();
            }
        }
    }
    memcpy(buf->data[0] + buf->size, str, len);
    buf->size += len;
}

void AppendGListTargetEndianLong(GList *buf, UInt32 value)
{
    UInt8 *dest;
    if (buf->size + 4 > buf->hndlsize) {
        buf->hndlsize += buf->growsize + 4;
        if (!fn_00443170((struct StorageHandle *)buf->data, buf->hndlsize)) {
            if (DAT_00587708 != NULL)
                DAT_00587708();
        }
    }
    dest = (UInt8 *)*buf->data + buf->size;
    buf->size += 4;
    value = MaybeSwap32(value);
    CopyFourBytes(dest, (const UInt8 *)&value);
}

void AppendGListLong(GList *buffer, SInt32 value)
{
    Boolean allocated;
    char *destination;

    if (buffer->size + (SInt32)sizeof(value) > buffer->hndlsize) {
        buffer->hndlsize += buffer->growsize + (SInt32)sizeof(value);
        allocated = fn_00443170((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!allocated && DAT_00587708 != NULL) {
            (*DAT_00587708)();
        }
    }
    destination = *buffer->data + buffer->size;
    buffer->size += sizeof(value);
    destination[0] = ((const NativeLongBytes *)&value)->byte0;
    destination[1] = ((const NativeLongBytes *)&value)->byte1;
    destination[2] = ((const NativeLongBytes *)&value)->byte2;
    destination[3] = ((const NativeLongBytes *)&value)->byte3;
}

void AppendGListTargetEndianWord(GList *buf, UInt16 word)
{
    char *dest;
    const U16Bytes *bytes;
    if (buf->size + 2 > buf->hndlsize) {
        buf->hndlsize += buf->growsize + 2;
        if (!fn_00443170((StorageHandle *)buf->data, buf->hndlsize)) {
            if (DAT_00587708 != NULL)
                DAT_00587708();
        }
    }
    dest = *buf->data + buf->size;
    buf->size += 2;
    word = SwapOutputWord(word);
    bytes = (const U16Bytes *)&word;
    dest[0] = bytes->b[0];
    dest[1] = ((const U16Bytes *)&word)->b[1];
}

void AppendGListWord(GList *buffer, SInt16 value)
{
    Boolean resized;
    char *destination;
    const char *valueBytes;

    if (buffer->size + (SInt32)sizeof(value) > buffer->hndlsize) {
        buffer->hndlsize += buffer->growsize + (SInt32)sizeof(value);
        resized = fn_00443170((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!resized && DAT_00587708) {
            (*DAT_00587708)();
        }
    }
    destination = *buffer->data + buffer->size;
    buffer->size += sizeof(value);
    valueBytes = (const char *)&value;
    *destination = *valueBytes;
    valueBytes = (const char *)&value;
    destination[1] = valueBytes[1];
}

void AppendGListByte(GList *buffer, SInt8 value)
{
    if (buffer->size + 1 > buffer->hndlsize) {
        Boolean success;
        buffer->hndlsize += buffer->growsize + 1;
        success = fn_00443170((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!success && DAT_00587708 != NULL) {
            (*DAT_00587708)();
        }
    }
    (*buffer->data)[buffer->size++] = value;
}

void AppendGListNoData(GList *buffer, SInt32 additionalLength)
{
    if (buffer->size + additionalLength > buffer->hndlsize) {
        buffer->hndlsize += additionalLength + buffer->growsize;
        if (!fn_00443170((struct StorageHandle *)buffer->data, buffer->hndlsize) && DAT_00587708 != NULL) {
            DAT_00587708();
        }
    }
    buffer->size += additionalLength;
}

void *CompilerTools_AppendGListData(GList *buffer, const void *source, SInt32 count)
{
    char *data;
    Boolean result;
    if (buffer->size + count > buffer->hndlsize) {
        buffer->hndlsize += count + buffer->growsize;
        result = fn_00443170((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (result == 0 && DAT_00587708 != NULL) {
            (*DAT_00587708)();
        }
    }
    data = memcpy(*buffer->data + buffer->size, source, count);
    buffer->size += count;
    return data;
}

void ShrinkGList(GList *list)
{
    list->hndlsize = list->size;
    fn_00443170((struct StorageHandle *)list->data, list->hndlsize);
}

void fn_00442c00(GList *entry)
{
    fn_004431a0(entry->data);
}

void FreeGList(GList *storage)
{
    if (storage->data != NULL) {
        fn_00443160(storage->data);
        storage->data = NULL;
    }
    storage->hndlsize = 0;
    storage->size = storage->hndlsize;
}

SInt16 InitGList(GList *allocation, SInt32 size)
{
    allocation->data = CompilerTools_AllocateMemoryIfEnabled(size);
    if (!allocation->data) {
        allocation->data = fn_00443110(size);
        if (!allocation->data)
            return -1;
    }
    allocation->size = 0;
    allocation->growsize = (int)size >> 1;
    allocation->hndlsize = size;
    return 0;
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

void CompilerGetCString(short value, char *destination)
{
    CompilerTools_GetResourceCString(destination, 0x2774, value);
}

#pragma optimization_level 2

void format_string(char *buf, int size, char *fmt, char *ap)
{
    char c;
    char *s;
    unsigned char *p;
    int len;
    int n;
    char ch;
    char tmp[256];
    while ((c = *fmt++) != 0) {
        if (c == '%') {
            switch (c = *fmt++) {
                case 'c':
                    s = VA_ARG(ap, char *);
                    while ((ch = *s) != 0 && size > 1) {
                        *buf = ch;
                        s++;
                        buf++;
                        size--;
                    }
                    break;
                case 'p':
                    p = VA_ARG(ap, unsigned char *);
                    len = *p++;
                    while (len != 0 && size > 1) {
                        *buf++ = *p++;
                        len--;
                        size--;
                    }
                    break;
                case 's':
                    Unmangle_UnmangleSymbolName(VA_ARG(ap, char *), tmp, sizeof(tmp));
                    s = tmp;
                    while ((ch = *s) != 0 && size > 1) {
                        *buf = ch;
                        s++;
                        buf++;
                        size--;
                    }
                    break;
                case 'n':
                    n = VA_ARG(ap, int);
                    if (size > 10) {
                        n = sprintf(buf, "%ld", n);
                        buf += n;
                        size -= n;
                    }
                    break;
                case 'h':
                    n = VA_ARG(ap, int);
                    if (size > 11) {
                        n = sprintf(buf, "0x%.8X", n);
                        buf += n;
                        size -= n;
                    }
                    break;
                default:
                    if (size > 1) {
                        *buf++ = c;
                        size--;
                    }
                    break;
            }
        } else if (size > 1) {
            *buf++ = c;
            size--;
        }
    }
    *buf = 0;
}

unsigned char CompilerTools_ReportDiagnostic(SInt32 diagnosticCode, ...)
{
    char message[0x800];
    char format[0x100];
    va_list args;

    if (data_00587958 == 0 && data_00588228 < data_005511b4) {
        CompilerTools_GetResourceCString(format, 0x2af9, diagnosticCode);
        args = (va_list)&diagnosticCode + ((va_list)(&diagnosticCode + 1) - (va_list)&diagnosticCode + 3) / 4 * 4;
        format_string(message, sizeof(message), format, args);
        fn_0041e990(message, &data_00551208);
        data_00588228++;
    }
}

#pragma optimization_level reset

#pragma optimization_level 2
#pragma sym off

void CompilerTools_ReportLimitedDiagnostic(SInt32 diagnosticCode, ...)
{
    va_list args;
    char message[2048];
    char format[256];

    if (limited_diagnostic_count < limited_diagnostic_limit) {
        CompilerTools_GetResourceCString(format, 0x2af9, diagnosticCode);
        args = (va_list)&diagnosticCode + (((va_list)(&diagnosticCode + 1) - (va_list)&diagnosticCode + 3) / 4) * 4;
        format_string(message, sizeof(message), format, args);
        fn_0041e970(message, &data_00551208);
        limited_diagnostic_count++;
    } else if (limited_diagnostic_count == limited_diagnostic_limit) {
        CompilerTools_GetResourceCString(format, 0x2af9, 25);
        args = (va_list)&diagnosticCode + (((va_list)(&diagnosticCode + 1) - (va_list)&diagnosticCode + 3) / 4) * 4;
        format_string(message, sizeof(message), format, args);
        fn_0041e970(message, NULL);
        limited_diagnostic_count++;
    }
}

#pragma sym reset
#pragma optimization_level reset

#pragma optimization_level 2

void CompilerTools_FormatMessageAndLongjmp(int message, int status)
{
    va_list args;
    char buf[256];

    CompilerTools_GetResourceCString(buf, 0x2af9, message);
    args = (va_list)&status + (((va_list)(&status + 1) - (va_list)&status + 3) / 4) * 4;
    format_string(message_buffer, 0x800, buf, args);
    longjmp(file_input_jmpbuf, status);
}

void CompilerTools_DispatchMessageBufferByMode(int mode)
{
    if (mode != 1) {
        if (mode == 2) {
            fn_0041ede0(message_buffer);
        } else {
            fn_0041ee00(message_buffer, mode);
        }
    }
}

#pragma optimization_level reset

#pragma auto_inline off

void copy_pstring(UInt8 *destination, UInt8 *source)
{
    int remaining;

    for (remaining = *source; (short)remaining >= 0; remaining--) {
        *destination++ = *source++;
    }
}

#pragma auto_inline reset

void *fn_00443110(SInt32 size)
{
    return (struct StorageHandle *)Memory_NewHandle(size);
}

void *CompilerTools_AllocateMemoryIfEnabled(SIZE_T size)
{
    char *allocation;
    short status;

    if (DAT_0054c3d0 != '\0') {
        allocation = (char *)fn_00413990(size, &status);
        if (status == 0) {
            return allocation;
        }
    }
    return NULL;
}

void fn_00443160(void *handle)
{
    Memory_FreeHandle(handle);
}

Boolean fn_00443170(struct StorageHandle *handle, UInt32 size)
{
    Memory_ResizeStorageHandle(handle, size);
    return fn_00419620() == 0;
}

void fn_00443190(void *handle)
{
    fn_00413a00(handle);
}

void fn_004431a0(void *handle)
{
    fn_00413a40((struct StorageHandle *)handle);
}

void fn_004431b0(void *entry)
{
    fn_00413a50(entry);
}

UInt32 CompilerTools_GetScaledTicks(void)
{
    return CLFileOps_GetScaledTicks();
}

void CompilerTools_GetResourceCString(char *buffer, SInt16 id, SInt16 arg)
{
    CLIO_GetResourceString((unsigned char *)buffer, id, arg);
    CLIO_ConvertPascalToCString(buffer);
}

unsigned char CompilerTools_IsByteInDBCSCharacter(unsigned char *textStart, unsigned char *bytePosition)
{
    return MacSpecs_IsByteInDBCSCharacter(textStart, bytePosition);
}

int fn_00443200(CWFileSpec *record, short *output, unsigned int argument4, unsigned int argument5)
{
    int result;

    Files_DeleteFileFromPath(record->fileData.file.volumeRef, record->fileData.file.directoryId,
                             record->fileData.file.name);
    result = Files_CallWithFileSpecFromPath(record->fileData.file.volumeRef, record->fileData.file.directoryId,
                                            record->fileData.file.name, argument4, argument5);
    if ((short)result == 0) {
        Files_OpenFileByPath(record->fileData.file.volumeRef, record->fileData.file.directoryId,
                             record->fileData.file.name, 3, output);
    }
}

short fn_00443250(CWFileSpec *fileSpec, short *fileRef)
{
    return Files_OpenFileByPath(fileSpec->fileData.file.volumeRef, fileSpec->fileData.file.directoryId,
                                fileSpec->fileData.file.name, 1, fileRef);
}

SInt16 CompilerTools_GetFileType(CWFileSpec *arguments, UInt32 *output)
{
    FileIdentifierInfo resultData;
    SInt16 result;

    result =
        Files_GetFileIdentifierInfoFromPath(arguments->fileData.file.volumeRef, arguments->fileData.file.directoryId,
                                            arguments->fileData.file.name, &resultData);
    *output = resultData.type;
    return result;
}

short CompilerTools_GetFileSize(short fileRef, SInt32 *size)
{
    return Files_GetSize(fileRef, size);
}

SInt16 CompilerTools_ReadFile(SInt16 first, void *second, SInt32 third)
{
    return Files_Read(first, &third, second);
}

SInt16 CompilerTools_Write(SInt16 first, void *second, SInt32 third)
{
    return Files_Write(first, &third, second);
}

short fn_004432f0(short value, long *result)
{
    return Files_Tell(value, result);
}

SInt16 CompilerTools_SetFilePosition(SInt16 refNum, SInt32 position)
{
    return Files_SetPosition(refNum, 1, position);
}

void CompilerTools_CloseFile(short handleIndex)
{
    Files_Close(handleIndex);
}

void CompilerTools_MakeCWFileSpecFromPString(void *result, unsigned char *name)
{
    OSSpec spec;
    char path[256];

    memcpy(path, name + 1, *name);
    path[*name] = 0;
    make_osspec_from_path(path, &spec, NULL);
    MacSpecs_MakeCWFileSpecFromString((char *)&spec, (CWFileSpec *)result);
}

void CompilerTools_GetPFileFields(CWFileSpec *record, unsigned short *tag, SInt32 *value, void *data)
{
    if (tag != NULL) {
        *tag = record->fileData.file.volumeRef;
    }
    if (value != NULL) {
        *value = record->fileData.file.directoryId;
    }
    if (data != NULL) {
        copy_pstring(data, record->fileData.file.name);
    }
}

#pragma auto_inline off

void resolve_file_name_to_pascal_string(short category, int recordId, void *inputName)
{
    CWFileSpec record;
    char resolvedName[324];

    record.fileData.file.volumeRef = category;
    record.fileData.file.directoryId = recordId;
    copy_pstring(record.fileData.file.name, inputName);
    if (MacSpecs_MakeOSSpec(&record, resolvedName) == 0) {
        OS_SpecToString((OSSpec *)resolvedName, inputName, 260);
        CLIO_ConvertToPascalString(inputName);
    }
}

#pragma auto_inline reset

void CompilerTools_ResolveFileNameToCString(void *destination, PFile *record, SInt32 *result)
{
    RecordQuery query;
    if (result) {
        query.name = record->header.fileData.file.name;
        query.kind = record->header.fileData.file.volumeRef;
        query.value = record->header.fileData.file.directoryId;
        query.flags = 0;
        if (Files_UpdateRecordQuery(&query) == 0)
            *result = query.result;
        else
            *result = 0;
    }
    copy_pstring(destination, record->header.fileData.file.name);
    resolve_file_name_to_pascal_string(record->header.fileData.file.volumeRef, record->header.fileData.file.directoryId,
                                       destination);
    CLIO_ConvertPascalToCString(destination);
}

unsigned int CompilerTools_GetTicks(void)
{
    return OS_GetMilliseconds() * 0x3cu / 1000;
}
