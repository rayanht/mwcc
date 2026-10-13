#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CompilerTools.h"
#include "compiler/objects.h"
#include "compiler/BE_elf.h"
#include "compiler/CPrep.h"
#include "compiler/CodeGen.h"
#include "driver/COSToolsCLT.h"
#include "driver/cc-eabi-ppc.h"
#include "version.h"
#include <string.h>
#include <ctype.h>

void (*data_00587708)(void);
int next_name_id;
struct HashNameNode **data_00587f88;

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

static inline void reset_pool_block(HeapBlock *block)
{
    block_pool.curblock = block;
    block_pool.curfreep = (char *)(block + 1);
    block_pool.curfree = block->blocksize - sizeof(HeapBlock);
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

void CTool_CtoPstr(unsigned char *text)
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

static void call_memory_error_handler(void)
{
    if (data_00587708 != NULL)
        data_00587708();
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

void LockGList(GList *entry)
{
    COS_LockHandleHi(entry->data);
}

void ShrinkGList(GList *list)
{
    list->hndlsize = list->size;
    COS_ResizeHandle((struct StorageHandle *)list->data, list->hndlsize);
}

void *AppendGListData(GList *buffer, const void *source, SInt32 count)
{
    char *data;
    Boolean result;
    if (buffer->size + count > buffer->hndlsize) {
        buffer->hndlsize += count + buffer->growsize;
        result = COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (result == 0)
            call_memory_error_handler();
    }
    data = memcpy(*buffer->data + buffer->size, source, count);
    buffer->size += count;
    return data;
}

void AppendGListNoData(GList *buffer, SInt32 additionalLength)
{
    if (buffer->size + additionalLength > buffer->hndlsize) {
        buffer->hndlsize += additionalLength + buffer->growsize;
        if (!COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize))
            call_memory_error_handler();
    }
    buffer->size += additionalLength;
}

void AppendGListByte(GList *buffer, SInt8 value)
{
    if (buffer->size + 1 > buffer->hndlsize) {
        Boolean success;
        buffer->hndlsize += buffer->growsize + 1;
        success = COS_ResizeHandle((struct StorageHandle *)buffer->data, buffer->hndlsize);
        if (!success)
            call_memory_error_handler();
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
        if (!resized)
            call_memory_error_handler();
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
        if (!COS_ResizeHandle((StorageHandle *)buf->data, buf->hndlsize))
            call_memory_error_handler();
    }
    dest = *buf->data + buf->size;
    buf->size += 2;
    word = CTool_EndianConvertWord16(word);
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
        if (!allocated)
            call_memory_error_handler();
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
        if (!COS_ResizeHandle((struct StorageHandle *)buf->data, buf->hndlsize))
            call_memory_error_handler();
    }
    dest = (UInt8 *)*buf->data + buf->size;
    buf->size += 4;
    value = CTool_EndianConvertWord32(value);
    CopyFourBytes(dest, (const UInt8 *)&value);
}

void AppendGListID(GList *buf, const char *str)
{
    UInt32 len = strlen(str) + 1;
    if (buf->size + len > buf->hndlsize) {
        struct StorageHandle *handle;
        buf->hndlsize += len + buf->growsize;
        handle = (struct StorageHandle *)buf->data;
        if (!COS_ResizeHandle(handle, buf->hndlsize))
            call_memory_error_handler();
    }
    memcpy(buf->data[0] + buf->size, str, len);
    buf->size += len;
}

void AppendGListName(GList *buf, const char *str)
{
    UInt32 len = strlen(str);
    if ((buf->size + len) > (UInt32)buf->hndlsize) {
        buf->hndlsize += len + buf->growsize;
        if (!COS_ResizeHandle((struct StorageHandle *)buf->data, buf->hndlsize))
            call_memory_error_handler();
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
    HashNameNode *entry;
    short hash;

    hash = CHash(text);
    if ((entry = data_00587f88[(int)hash]) == NULL) {
        data_00587f88[hash] = entry = (HashNameNode *)galloc(strlen(text) + 12);
        entry->next = NULL;
        entry->id = next_name_id++;
        entry->hashval = hash;
        strcpy(entry->name, text);
        return entry;
    }
    for (;; entry = entry->next) {
        if (strcmp(text, entry->name) == 0) {
            if (entry->id < 0)
                entry->id = next_name_id++;
            return entry;
        }
        if (entry->next == NULL) {
            entry->next = galloc(strlen(text) + 12);
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
    HashNameNode *entry;
    short hash;

    hash = CHash(text);
    if ((entry = data_00587f88[(int)hash]) == NULL) {
        entry = galloc(strlen(text) + 12);
        data_00587f88[hash] = entry;
        entry->next = NULL;
        entry->id = -1;
        entry->hashval = hash;
        strcpy(entry->name, text);
        return entry;
    }
    for (;;) {
        if (strcmp(text, entry->name) == 0)
            return entry;
        if (entry->next == NULL) {
            entry->next = galloc(strlen(text) + 12);
            entry = entry->next;
            entry->next = NULL;
            entry->id = -1;
            entry->hashval = hash;
            strcpy(entry->name, text);
            return entry;
        }
        entry = entry->next;
    }
}

void InitNameHash(void)
{
#if VERSION == VERSION_GC_1_0
    data_00587f88 = galloc(2048 * sizeof(HashNameNode *));
    memclrw(data_00587f88, 2048 * sizeof(HashNameNode *));
#else
    memclrw(data_00587f88 = galloc(2048 * sizeof(HashNameNode *)), 2048 * sizeof(HashNameNode *));
#endif
    next_name_id = 1;
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
    memclrw(&galloc_pool, sizeof(HeapMem));
    memclrw(&data_0057fd84, sizeof(HeapMem));
    memclrw(&block_pool, sizeof(HeapMem));
    memclrw(&data_0057fdac, sizeof(HeapMem));
    memclrw(&heap_pool, sizeof(HeapMem));
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
    data_0057fdd4 = NULL;
    data_0057fdd8 = 0;
    memclrw(&galloc_pool, sizeof(HeapMem));
    memclrw(&data_0057fd84, sizeof(HeapMem));
    memclrw(&block_pool, sizeof(HeapMem));
    memclrw(&data_0057fdac, sizeof(HeapMem));
    memclrw(&heap_pool, sizeof(HeapMem));
    galloc_pool.allocsize = 0x38000;
    select_or_allocate_pool_node(&galloc_pool, 0);
    galloc_pool.allocsize = 0x8000;
    data_0057fdd4 = param;
    if (galloc_pool.curblock == NULL)
        return -1;
    return 0;
}

static void release_heap(HeapMem *heap)
{
    HeapBlock *block;
    HeapBlock **handle;

    block = heap->blocks;
    while (block != NULL) {
        handle = block->blockhandle;
        block = block->next;
        COS_FreeHandle(handle);
    }
    memclrw(heap, sizeof(HeapMem));
}

void releaseheaps(void)
{
    release_heap(&galloc_pool);
    release_heap(&data_0057fd84);
    release_heap(&block_pool);
    release_heap(&data_0057fdac);
    release_heap(&heap_pool);
}

void releasegheap(void)
{
    release_heap(&galloc_pool);
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

void *lalloc(unsigned int size)
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

void *aalloc(SInt32 size)
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

void *oalloc(UInt32 requestedSize)
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

void locklheap(void)
{
    data_0057fdd8 += 1;
    return;
}

void unlocklheap(void)

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

void freeaheap(void)
{
    HeapBlock *block;
    block = block_pool.blocks;
    reset_pool_block(block);
    for (; block; block = block->next)
        block->blockfree = block->blocksize - sizeof(HeapBlock);
}

#pragma sym off

void freeoheap(void)
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
        UInt16 word;
        unsigned char bytes[2];
    } in, out;
    if (copts.littleendian)
        return word;
    in.word = word;
    out.bytes[0] = in.bytes[1];
    out.bytes[1] = in.bytes[0];
    return out.word;
}

UInt32 CTool_EndianConvertWord32(UInt32 value)
{
    union {
        unsigned int word;
        unsigned char bytes[4];
    } in, out;
    if (copts.littleendian)
        return value;
    in.word = value;
    out.bytes[0] = in.bytes[3];
    out.bytes[1] = in.bytes[2];
    out.bytes[2] = in.bytes[1];
    out.bytes[3] = in.bytes[0];
    return out.word;
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
    UInt32 lo = ci.lo;

    if (copts.littleendian == 0) {
        h = ci.hi;
        buf[0] = CTool_EndianConvertWord32(h);
        buf[1] = CTool_EndianConvertWord32(lo);
    } else {
        buf[0] = CTool_EndianConvertWord32(lo);
        h = ci.hi;
        buf[1] = CTool_EndianConvertWord32(h);
    }
    memcpy(result, buf, 8);
}

short CTool_EndianConvertInPlaceWord16Ptr(short *word)
{
    *word = CTool_EndianConvertWord16(*word);
    return *word;
}

unsigned int CTool_EndianConvertInPlaceWord32Ptr(unsigned int *p)
{
    *p = CTool_EndianConvertWord32(*p);
    return *p;
}

#if VERSION == VERSION_GC_1_0
void CTool_EndianConvertVector128(MWVector128 *vector)
{
    MWVector128 in;
    MWVector128 out;

    if (copts.littleendian == 0)
        return;
    memcpy(&in, vector, sizeof(in));
    out.ul[0] = in.ul[3];
    out.ul[1] = in.ul[2];
    out.ul[2] = in.ul[1];
    out.ul[3] = in.ul[0];
    memcpy(vector, &out, sizeof(out));
}
#endif

/* The linker stripped the functions that used these literals; they stay in the unit's .data. */
static void CompilerTools_NameLiterals(const char **literals)
{
    literals[0] = "operator new";
    literals[1] = "operator delete";
    literals[2] = "(void)";
    literals[3] = "(...)";
}

HashNameNode *fn_00441850(CPrepFileInfo *file, SInt32 *position)
{
    char fileName[256];
    COS_FileGetPathName(fileName, file, position);
    return GetHashNameNode(fileName);
}

int CTool_AppendStringBounded(char *dst, char *src, int size)
{
    int len = strlen(dst);

    dst += len;
    size -= len;
    if (size < 0)
        return 1;
    while (size) {
        if ((*dst++ = *src++) == '\0')
            break;
        size--;
    }
    if (size == 0 && dst[-1] != '\0') {
        dst[-1] = '\0';
        return 1;
    }
    return 0;
}
