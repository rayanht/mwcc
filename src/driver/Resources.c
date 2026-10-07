#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CompilerTools.h"
#include "driver/Resources.h"
#include "compiler/types.h"
#include "compiler/win32.h"
#include "compiler/CExpr2.h"
#include "compiler/CPrep.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/StringUtils.h"
#include <stdlib.h>
#include <string.h>

#define SWAP16(x) ((((x) & 0xff00) >> 8) | (((x) & 0xff) << 8))
#define SWAP32(x)                                                                                                      \
    (((((UINT)(x)) & 0xff000000u) >> 24) | ((((UINT)(x)) & 0x00ff0000u) >> 8) | ((((UINT)(x)) & 0x0000ff00u) << 8) |   \
     ((((UINT)(x)) & 0x000000ffu) << 24))

static UInt8 data_00541170 = 0;

static short resource_error;
static SInt16 current_resfile_refnum;
static struct ResFile *resfile_list;
static struct IdentifierListNode *identifier_list;
static struct IdentifierListNode *identifier_list_tail;

unsigned char fn_00406610(void)
{
    return data_00541170;
}

static UInt32 SwapRecord32(UInt32 v)
{
    return ((v & 0xff000000) >> 24) | ((v & 0xff0000) >> 8) | ((v & 0xff00) << 8) | ((v & 0xff) << 24);
}

unsigned int Resources_OpenResourceFile(char *path)
{
    DWORD resourceStatus;
    int queryStatus;
    short openStatus;
    CWFileSpec pathInfo;
    void *resourceData;
    DWORD resourceSize;

    resfile_list = NULL;
    current_resfile_refnum = 0;
    resourceStatus = MacSpecs_LoadMacResource(path, &resourceData, &resourceSize);
    if (resourceStatus == 0) {
        read_resource_file(-1, '\x01', resourceData, resourceSize);
        current_resfile_refnum = 0xffff;
        resource_error = 0;
        return 0;
    }
    queryStatus = MacSpecs_MakeCWFileSpecFromString(path, &pathInfo);
    if (queryStatus == 0) {
        openStatus = open_resource_file(&pathInfo, '\x01');
        if (openStatus == 0) {
            resource_error = 0;
            return 0;
        }
    }
    resource_error = 0xffd5;
    return 0xffffffd5;
}

struct ResFile *create_resfile(short refnum, int attrs, int types, int unused)
{
    ResFile *record;
    ResFile **head = &resfile_list;

    record = (ResFile *)malloc(sizeof(ResFile));
    if (!record) {
        resource_error = -108;
        return NULL;
    }
    record->refnum = refnum;
    record->attrs = attrs;
    record->types = (struct ResType *)types;
    record->next = *head;
    *head = record;
    return record;
}

ResType *append_res_type(ResType **link, unsigned int type, ResEntry *entries, ResType *next)
{
    ResType *entry;

    for (; *link != NULL; link = &(*link)->next) {
    }
    entry = (ResType *)malloc(sizeof(ResType));
    if (entry == NULL) {
        resource_error = 0xff94;
        return NULL;
    }
    entry->type = type;
    entry->entries = entries;
    entry->next = next;
    *link = entry;
    return entry;
}

static inline void StoreBigEndian32(UInt8 *bytes, UInt32 value)
{
    bytes[0] = (UInt8)((value >> 24) & 0xff);
    bytes[1] = (UInt8)((value >> 16) & 0xff);
    bytes[2] = (UInt8)((value >> 8) & 0xff);
    bytes[3] = (UInt8)(value & 0xff);
}

struct ResEntry *insert_res_entry(struct ResEntry **listAddress, UInt16 key, UInt32 value, UInt8 *name, UInt8 flag,
                                  void **extra)
{
    struct ResEntry *previous = NULL;
    struct ResEntry **link = listAddress;
    struct ResEntry *entry;
    char nameBuffer[256];

    if (name != NULL)
        p2cstrcpy(nameBuffer, name);
    else
        strcpy(nameBuffer, "(none)");

    while ((entry = *link) != NULL && entry->id != key) {
        previous = entry;
        link = &entry->next;
    }

    entry = (struct ResEntry *)malloc(sizeof(*entry));
    if (entry == NULL) {
        resource_error = 0xff94;
        return NULL;
    }
    entry->id = key;
    entry->resourceType = value;
    entry->name = name;
    entry->attrs = flag;
    entry->hand = (struct StorageHandle *)extra;
    if (previous != NULL) {
        entry->next = previous->next;
        previous->next = entry;
    } else {
        entry->next = NULL;
    }
    *link = entry;
    return entry;
}

struct ResFile *find_resfile_by_refnum(short refnum)
{
    ResFile *file = resfile_list;

    while (file != NULL) {
        if (file->refnum == refnum) {
            return file;
        }
        file = file->next;
    }
    return NULL;
}

ResFile *find_resfile_with_previous(short value, ResFile **previous)
{
    ResFile *node;

    node = resfile_list;
    *previous = NULL;
    if (node != NULL) {
        do {
            if (node->refnum == value) {
                return node;
            }
            *previous = node;
            node = node->next;
        } while (node != NULL);
    }
    return NULL;
}

void free_res_file(short key)
{
    ResType *group;
    ResEntry *item;
    ResFile *entry;
    ResFile *previous;

    entry = find_resfile_with_previous(key, &previous);
    if (entry == NULL) {
        return;
    }
    group = entry->types;
    while (group != NULL) {
        item = group->entries;
        while (item != NULL) {
            ResEntry *releasedItem;
            if (item->hand != NULL) {
                Memory_FreeHandle(item->hand);
            }
            if (item->name != NULL) {
                free(item->name);
            }
            releasedItem = item;
            item = item->next;
            free(releasedItem);
        }
        {
            ResType *releasedGroup = group;
            group = group->next;
            free(releasedGroup);
        }
    }
    if (previous != NULL) {
        previous->next = entry->next;
    } else {
        resfile_list = entry->next;
    }
    free(entry);
}

void read_resource_file(short refnum, char readonly, unsigned char *buf, SInt32 size)
{
    ResHead hdr;
    ResData rdata;
    SInt32 mapcount;
    SInt32 count;
    SInt32 val;
    SInt32 cnt4;
    RefEntry *refs;
    ResData *pd;
    signed pos;
    ResHead *ph;
    ResMap *rmap;
    ResFile *rec;
    unsigned char *map;
    ResType *rt;
    ResourceTypeEntry *types;
    unsigned char *names;
    unsigned char *name;
    unsigned char *str;
    void **hand;
    UINT dofs;
    signed fpos;
    int flags;
    int j;

    do {
        if (buf == NULL) {
            if ((resource_error = Files_GetSize(refnum, &size)) != 0)
                break;
        }
        if (size < 0x5a)
            break;

        pos = 0;
        count = sizeof(ResHead);
        if (buf == NULL) {
            if ((resource_error = Files_SetPosition(refnum, 1, pos)) != 0)
                break;
            if ((resource_error = Files_Read(refnum, &count, &hdr)) != 0)
                break;
        } else {
            if (size < 16)
                break;
            memcpy(&hdr, buf, sizeof(ResHead));
        }
        pos += count;

        hdr.map_offs = SWAP32(hdr.map_offs);
        hdr.map_len = SWAP32(hdr.map_len);
        hdr.data_offs = SWAP32(hdr.data_offs);
        hdr.data_len = SWAP32(hdr.data_len);

        count = (hdr.map_offs < hdr.data_offs ? hdr.map_offs : hdr.data_offs) - 0x10;
        if (count > sizeof(ResData))
            count = sizeof(ResData);
        if (buf == NULL) {
            if ((resource_error = Files_SetPosition(refnum, 1, pos)) != 0)
                break;
            if ((resource_error = Files_Read(refnum, &count, &rdata)) != 0)
                break;
        } else {
            if (pos + count > size)
                break;
            memcpy(&rdata, buf + pos, count);
        }

        mapcount = hdr.map_len;
        map = (unsigned char *)malloc(mapcount);
        if (map == NULL)
            goto nomem;
        pos = hdr.map_offs;
        if (buf == NULL) {
            if ((resource_error = Files_SetPosition(refnum, 1, pos)) != 0)
                break;
            if ((resource_error = Files_Read(refnum, &mapcount, map)) != 0)
                break;
        } else {
            if (pos + mapcount > size)
                break;
            memcpy(map, buf + pos, mapcount);
        }

        ph = &hdr;
        if (ph->map_offs > size)
            break;
        if (ph->map_offs + ph->map_len > size)
            break;
        if (ph->data_offs > size)
            break;
        if (ph->data_offs + ph->data_len > size)
            break;
        if (ph->map_offs < ph->data_offs ? ph->map_offs + ph->map_len > ph->data_offs
                                         : ph->data_offs + ph->data_len > ph->map_offs)
            break;

        rmap = (ResMap *)map;
        pd = &rdata;
        flags = (readonly == 1) ? 0x80 : 0;
        rec = create_resfile(refnum, flags | 0x40, 0, 0);
        if (rec == NULL)
            goto nomem;
        rec->data = *pd;

        rmap->attrs = SWAP16(rmap->attrs);
        rmap->typelist_offs = SWAP16(rmap->typelist_offs);
        rmap->namelist_offs = SWAP16(rmap->namelist_offs);
        rmap->num_types = SWAP16(rmap->num_types);

        do {
            if (rmap->typelist_offs + ph->map_offs > size)
                break;
            types = (ResourceTypeEntry *)((unsigned char *)rmap + rmap->typelist_offs + 2);
            if (rmap->namelist_offs + ph->map_offs > size)
                break;
            names = (unsigned char *)rmap + rmap->namelist_offs;

            if (rmap->num_types != 0xffff) {
                for (count = 0; count <= rmap->num_types; count++) {
                    types[count].type = SWAP32(types[count].type);
                    types[count].resourceCountMinusOne = SWAP16(types[count].resourceCountMinusOne);
                    types[count].referenceListOffset = SWAP16(types[count].referenceListOffset);
                    rt = append_res_type(&rec->types, types[count].type, NULL, NULL);
                    if (rt == NULL)
                        goto fail;
                    if (types[count].referenceListOffset + rmap->typelist_offs + ph->map_offs > size)
                        goto bad2;
                    refs = (RefEntry *)((unsigned char *)types + types[count].referenceListOffset - 2);
                    if (types[count].resourceCountMinusOne != 0xffff) {
                        for (j = 0; j <= types[count].resourceCountMinusOne; j++) {
                            refs[j].id = SWAP16(refs[j].id);
                            refs[j].nameOffset = SWAP16(refs[j].nameOffset);
                            dofs = (refs[j].dataOffset[0] << 16) | (refs[j].dataOffset[1] << 8) | refs[j].dataOffset[2];
                            if (dofs + ph->data_offs > size)
                                goto bad2;
                            cnt4 = 4;
                            fpos = dofs;
                            fpos += ph->data_offs;
                            if (buf == NULL) {
                                if ((resource_error = Files_SetPosition(refnum, 1, fpos)) != 0)
                                    goto bad2;
                                if ((resource_error = Files_Read(refnum, &cnt4, &val)) != 0)
                                    goto bad2;
                            } else {
                                if (fpos + 4 > size)
                                    goto bad2;
                                memcpy(&val, buf + fpos, 4);
                            }
                            fpos += cnt4;
                            val = SWAP32(val);
                            if (val + dofs + ph->data_offs > size)
                                goto bad2;
                            hand = (void **)Memory_NewHandle(val);
                            if (hand == NULL)
                                goto bad2;
                            fn_00413a00((struct StorageHandle *)hand);
                            if (buf == NULL) {
                                if ((resource_error = Files_SetPosition(refnum, 1, fpos)) != 0)
                                    goto bad2;
                                if ((resource_error = Files_Read(refnum, &val, *hand)) != 0)
                                    goto bad2;
                            } else {
                                if (fpos + *(signed *)&val > size)
                                    goto bad2;
                                memcpy(*hand, buf + fpos, *(signed *)&val);
                            }
                            fn_00413a50(hand);
                            if (refs[j].nameOffset != 0xffff) {
                                str = names + refs[j].nameOffset;
                                name = (unsigned char *)malloc(*str + 1);
                                memcpy(name, str, *str + 1);
                            } else {
                                name = NULL;
                            }
                            if (insert_res_entry(&rt->entries, refs[j].id, types[count].type, name, refs[j].attributes,
                                                 hand) == NULL)
                                goto fail;
                        }
                    }
                }
            }
            free(map);
            resource_error = 0;
            return;

        } while (0);

    bad2:
        resource_error = 0xff39;
    fail:
        free_res_file(refnum);
        if (map != NULL)
            free(map);
        return;

    nomem:
        resource_error = 0xff94;
        return;

    } while (0);

    resource_error = 0xff39;
    return;
}

unsigned int calculate_resource_sizes(ResFile *root, unsigned int *dataSize, unsigned int *nameSize,
                                      unsigned int *entrySize, unsigned int *typeSize)
{
    ResType *type;
    ResEntry *entry;

    *typeSize = 0;
    *entrySize = *typeSize;
    *nameSize = *entrySize;
    *dataSize = *nameSize;

    for (type = root->types; type; type = type->next) {
        for (entry = type->entries; entry; entry = entry->next) {
            *nameSize += entry->name == NULL ? 0 : entry->name[0] + 1;
            *dataSize += (entry->hand == NULL ? 0 : Memory_GetHandleSize(entry->hand)) + 4;
            *entrySize += 12;
        }
        *typeSize += 8;
    }
}

int count_types(ResFile *chain)
{
    ResType *entry;
    int count;

    count = 0;
    entry = chain->types;
    while (entry != NULL) {
        entry = entry->next;
        count = count + 1;
    }
    return count;
}

int count_entries(ResType *list)
{
    int count = 0;
    ResEntry *entry;

    for (entry = list->entries; entry != NULL; entry = entry->next) {
        ++count;
    }
    return count;
}

#include <stddef.h>

#include <stddef.h>

#include <stddef.h>

void write_resource_file(short refnum)
{
    const SInt32 resourceHeaderSize = sizeof(ResHead);
    const SInt32 resourceMapSize = offsetof(ResMap, num_types) + sizeof(UInt16);
    const SInt32 typeEntrySize = sizeof(ResourceTypeEntry);
    const SInt32 referenceEntrySize = sizeof(RefEntry);
    const SInt32 dataHeaderSize = sizeof(ResourceDataHeader);
    StorageHandle *handle;
    UINT totalSize;
    ResFile *file;
    ResHead header, diskHeader;
    ResData reservedData;
    ResMap map, diskMap;
    UINT referenceListSize, typeListSize, nameListSize, dataSize;
    ResourceTypeEntry typeEntry, diskTypeEntry;
    RefEntry reference, diskReference;
    ResourceDataHeader dataHeader, diskDataHeader;
    SInt32 size;
    SInt32 typelist_offs, reflist_offs;
    UINT name_offs;
    SInt32 offset;
    SInt32 length;
    SInt32 data_offs;
    ResType *type;
    ResEntry *entry;

    file = find_resfile_by_refnum(refnum);
    if (file == NULL) {
        resource_error = 0xff3f;
        return;
    }
    if (file->refnum == -1) {
        resource_error = 0xff3f;
        return;
    }
    if (file->attrs & 0x80) {
        resource_error = 0xff3a;
        return;
    }
    if (!(file->attrs & 0x20)) {
        return;
    }

    calculate_resource_sizes(file, &dataSize, &nameListSize, &referenceListSize, &typeListSize);

    header.data_offs = 0x100;
    header.data_len = dataSize;
    header.map_offs = dataSize + header.data_offs;
    header.map_len =
        nameListSize + referenceListSize + typeListSize + offsetof(ResMap, num_types) + sizeof(map.num_types);
    handle = (StorageHandle *)Memory_NewHandle(totalSize = header.map_offs + header.map_len);
    if (handle == NULL) {
        resource_error = Memory_GetError();
        return;
    }

    fn_00413a00(handle);
    memset(handle->data, 0, totalSize);

    diskHeader = header;
    diskHeader.data_offs = SWAP32(diskHeader.data_offs);
    diskHeader.map_offs = SWAP32(diskHeader.map_offs);
    diskHeader.data_len = SWAP32(diskHeader.data_len);
    diskHeader.map_len = SWAP32(diskHeader.map_len);

    if (Memory_GetHandleSize(handle) < resourceHeaderSize) {
        resource_error = 0xff39;
        return;
    }
    memcpy(handle->data, &diskHeader, sizeof(diskHeader));

    reservedData = file->data;

    if (Memory_GetHandleSize(handle) < 0x100) {
        resource_error = 0xff39;
        return;
    }
    memcpy(handle->data + sizeof(diskHeader), &reservedData, sizeof(reservedData));

    map.hdr = diskHeader;
    map.next = 0;
    map.file_ref = 0;
    map.attrs = file->attrs & 0xfffd;
    map.typelist_offs = offsetof(ResMap, num_types);
    map.namelist_offs = typeListSize + referenceListSize + offsetof(ResMap, num_types) + sizeof(map.num_types);
    map.num_types = count_types(file) - 1;

    diskMap = map;
    diskMap.attrs = SWAP16(diskMap.attrs);
    diskMap.typelist_offs = SWAP16(diskMap.typelist_offs);
    diskMap.namelist_offs = SWAP16(diskMap.namelist_offs);
    diskMap.num_types = SWAP16(diskMap.num_types);

    offset = header.map_offs;
    if (offset + resourceMapSize > Memory_GetHandleSize(handle)) {
        resource_error = 0xff39;
        return;
    }
    memcpy(handle->data + offset, &diskMap, offsetof(ResMap, num_types) + sizeof(diskMap.num_types));

    data_offs = name_offs = 0;
    reflist_offs = 0;
    typelist_offs = 0;

    for (type = file->types; type != NULL; type = type->next) {
        typeEntry.type = type->type;
        typeEntry.resourceCountMinusOne = count_entries(type) - 1;
        typeEntry.referenceListOffset = typeListSize + reflist_offs + 2;
        diskTypeEntry = typeEntry;
        diskTypeEntry.type = SWAP32(diskTypeEntry.type);
        diskTypeEntry.resourceCountMinusOne = SWAP16(diskTypeEntry.resourceCountMinusOne);
        diskTypeEntry.referenceListOffset = SWAP16(diskTypeEntry.referenceListOffset);

        offset = map.typelist_offs + header.map_offs + 2 + typelist_offs;
        if (offset + typeEntrySize > Memory_GetHandleSize(handle)) {
            resource_error = 0xff39;
            return;
        }
        memcpy(handle->data + offset, &diskTypeEntry, sizeof(diskTypeEntry));

        for (entry = type->entries; entry != NULL; entry = entry->next) {
            if (reflist_offs >= map.namelist_offs) {
                CLIO_ReportAssertionFailure("reflist_offs < dmap.namelist_offs", "Resources.c", 943);
            }
            reference.id = entry->id;
            reference.nameOffset = entry->name ? name_offs : -1;
            reference.attributes = entry->attrs & 0xfd;
            reference.dataOffset[0] = (data_offs >> 16) & 0xff;
            reference.dataOffset[1] = (data_offs >> 8) & 0xff;
            reference.dataOffset[2] = data_offs & 0xff;
            reference.hand = 0;
            diskReference = reference;
            diskReference.id = SWAP16(diskReference.id);
            diskReference.nameOffset = SWAP16(diskReference.nameOffset);

            offset = map.typelist_offs + header.map_offs + 2 + typeListSize + reflist_offs;
            if (offset + referenceEntrySize > Memory_GetHandleSize(handle)) {
                resource_error = 0xff39;
                return;
            }
            memcpy(handle->data + offset, &diskReference, sizeof(diskReference));
            reflist_offs += sizeof(reference);

            if (entry->name != NULL) {
                if (header.map_offs < header.data_offs &&
                    name_offs + map.namelist_offs + header.map_offs >= header.data_offs) {
                    CLIO_ReportAssertionFailure("name_offs + dmap.namelist_offs + dhdr.map_offs < dhdr.data_offs",
                                                "Resources.c", 963);
                }
                offset = header.map_offs + map.namelist_offs + reference.nameOffset;
                length = *entry->name + 1;
                if (offset + length > Memory_GetHandleSize(handle)) {
                    resource_error = 0xff39;
                    return;
                }
                memcpy(handle->data + offset, entry->name, length);
                name_offs += *entry->name + 1;
            }

            if (data_offs >= header.data_len) {
                CLIO_ReportAssertionFailure("data_offs < dhdr.data_len", "Resources.c", 970);
            }
            if (entry->hand == NULL) {
                CLIO_ReportAssertionFailure("mrle->hand!=NULL", "Resources.c", 971);
            }
            if (header.map_offs > header.data_offs && data_offs + header.data_offs >= header.map_offs) {
                CLIO_ReportAssertionFailure("data_offs + dhdr.data_offs < dhdr.map_offs", "Resources.c", 973);
            }

            fn_00413a00(entry->hand);
            dataHeader.length = Memory_GetHandleSize(entry->hand);
            diskDataHeader = dataHeader;
            diskDataHeader.length = SWAP32(diskDataHeader.length);
            offset = header.data_offs + data_offs;
            if (offset + dataHeaderSize > Memory_GetHandleSize(handle)) {
                resource_error = 0xff39;
                return;
            }
            memcpy(handle->data + offset, &diskDataHeader, sizeof(diskDataHeader));
            data_offs += sizeof(dataHeader);
            length = dataHeader.length;
            offset = header.data_offs + data_offs;
            if (offset + length > Memory_GetHandleSize(handle)) {
                resource_error = 0xff39;
                return;
            }
            memcpy(handle->data + offset, entry->hand->data, length);
            data_offs += dataHeader.length;
            fn_00413a50(entry->hand);
        }

        typelist_offs += sizeof(typeEntry);
    }

    size = Memory_GetHandleSize(handle);
    resource_error = Files_SetPosition(refnum, 1, 0);
    if (resource_error != 0) {
        return;
    }
    resource_error = Files_SetSize(refnum, size);
    if (resource_error != 0) {
        return;
    }
    resource_error = Files_Write(refnum, &size, handle->data);
    if (resource_error != 0) {
        return;
    }
    fn_00413a50(handle);
    Memory_FreeHandle(handle);
}

void write_default_binary_record(SInt16 file)
{
    struct StorageHandle *buffer;
    struct BinaryRecordPrefix prefix;
    char defaults[240];
    struct BinaryRecordTrailer trailer;
    SInt32 size;
    int capacity;
    const int prefixSize = sizeof(prefix);
    const int defaultsSize = sizeof(defaults);
    const int trailerSize = sizeof(trailer.prefix) + sizeof(trailer.reserved) + sizeof(trailer.values);
    const int recordSize = sizeof(prefix) + sizeof(defaults) + trailerSize;
    prefix.words[1] = 65536;
    prefix.words[3] = 503316480;
    prefix.words[0] = 503382016;
    prefix.words[2] = 0;
    memset(defaults, 0, sizeof(defaults));
    memset(defaults + 70, 0, 4);
    memset(defaults + 66, 0, 4);
    memcpy(&trailer.prefix, &prefix, sizeof(prefix));
    trailer.reserved = 0;
    trailer.values[0] = 0;
    trailer.values[1] = 0;
    trailer.values[2] = 7168;
    trailer.values[3] = 7168;
    trailer.values[4] = 65535;
    buffer = (struct StorageHandle *)Memory_NewHandle(recordSize);
    if (buffer == NULL) {
        resource_error = Memory_GetError();
        return;
    }
    fn_00413a00(buffer);
    memset(buffer->data, 0, recordSize);
    capacity = Memory_GetHandleSize(buffer);
    if (capacity < prefixSize) {
        resource_error = -199;
        return;
    }
    memcpy(buffer->data, &prefix, sizeof(prefix));
    capacity = Memory_GetHandleSize(buffer);
    if (capacity < prefixSize + defaultsSize) {
        resource_error = -199;
        return;
    }
    memcpy((buffer->data + sizeof(prefix)), defaults, sizeof(defaults));
    capacity = Memory_GetHandleSize(buffer);
    if (capacity < recordSize) {
        resource_error = -199;
        return;
    }
    memcpy((buffer->data + sizeof(prefix) + sizeof(defaults)), &trailer, trailerSize);
    size = Memory_GetHandleSize(buffer);
    resource_error = Files_SetPosition(file, 1, 0);
    if (resource_error != 0) {
        return;
    }
    resource_error = Files_SetSize(file, size);
    if (resource_error != 0) {
        return;
    }
    resource_error = Files_Write(file, &size, buffer->data);
    if (resource_error != 0) {
        return;
    }
    fn_00413a50(buffer);
    Memory_FreeHandle(buffer);
}

ResType *find_res_type(ResFile *list, int key)
{
    ResType *entry;

    entry = list->types;
    while (entry != NULL) {
        if (entry->type == key) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

ResEntry *find_res_entry(ResFile *file, int type, short id)
{
    ResType *record = find_res_type(file, type);
    ResEntry *entry;

    if (record) {
        entry = record->entries;
        while (entry) {
            if (entry->id == id)
                return entry;
            entry = entry->next;
        }
    }
    return NULL;
}

ResEntry *find_res_entry_in_files(int lookupArg, short lookupKind)
{
    ResEntry *result;
    struct ResFile *node;
    ResFile *key;

    node = find_resfile_by_refnum(current_resfile_refnum);
    while (node != NULL) {
        key = node;
        result = find_res_entry(key, lookupArg, lookupKind);
        if (result != NULL) {
            return result;
        }
        node = node->next;
    }
    return NULL;
}

void append_identifier_list_node(SInt32 a, OSSpec *b)
{
    IdentifierListNode *node = (IdentifierListNode *)malloc(sizeof(IdentifierListNode));
    if (node != NULL) {
        node->fileSpec = *b;
        node->id = a;
        node->next = NULL;
        if (identifier_list_tail != NULL) {
            identifier_list_tail->next = node;
        } else {
            identifier_list = node;
        }
        identifier_list_tail = node;
    }
}

void Resources_RemoveIdentifier(unsigned int key)
{
    struct IdentifierListNode *entry;
    struct IdentifierListNode *previous;
    struct IdentifierListNode *next;

    entry = identifier_list;
    previous = NULL;
    while (entry && entry->id != key) {
        previous = entry;
        entry = entry->next;
    }
    if (entry) {
        if (previous) {
            next = entry->next;
            previous->next = next;
            if (!previous->next)
                identifier_list_tail = previous;
        } else {
            next = entry->next;
            identifier_list = next;
            if (!next)
                identifier_list_tail = NULL;
        }
        free(entry);
    }
}

IdentifierListNode *Resources_FindIdentifierById(int id)
{
    IdentifierListNode *node;

    node = identifier_list;
    if (node != NULL) {
        do {
            if (node->id == id) {
                return node;
            }
            node = node->next;
        } while (node != NULL);
    }
    return NULL;
}

/* Linked list entry indexed by a 32-bit key. */
Boolean Resources_FindIdentifier(OSSpec *key, SInt32 *value)
{
    IdentifierListNode *entry;

    for (entry = identifier_list; entry != NULL; entry = entry->next) {
        if (OS_EqualSpec(&entry->fileSpec, key) != 0) {
            *value = entry->id;
            return 1;
        }
    }
    return 0;
}

int Resources_SetFileTimes(unsigned int fileHandle, UInt32 modificationTime, UInt32 creationTime)
{
    ResFile *file;
    UInt32 times[2];
    DWORD length;
    int error;

    if (Resources_FindIdentifierById(fileHandle) == NULL) {
        times[0] = SWAP32(creationTime);
        times[1] = SWAP32(modificationTime);
        length = sizeof(times);
        error = OS_Seek(fileHandle, 1, 0x52);
        if (error == 0) {
            error = OS_Write(fileHandle, times, &length);
            if (error == 0 && length == sizeof(times))
                error = 0;
        }
        return error;
    }

    file = find_resfile_by_refnum(OS_RefToMac(fileHandle));
    if (file == NULL)
        return 2;
    if ((file->attrs & 0x80) != 0)
        return 0xc;
    StoreBigEndian32(&file->data.modificationTime[0], modificationTime);
    StoreBigEndian32(&file->data.creationTime[0], creationTime);
    file->attrs |= 0x20;
    return 0;
}

SInt32 fn_004083b0(int handle, UInt32 *secondValue, UInt32 *firstValue)
{
    UInt32 values[2];
    SInt32 byteCount;
    DWORD result;
    ResFile *record;
    signed char *bytes;

    if (Resources_FindIdentifierById(handle) == NULL) {
        byteCount = 8;
        result = OS_Seek(handle, 1, 0x52);
        if (result == 0) {
            result = OS_Read(handle, values, &byteCount);
            if (result == 0 && byteCount == 8) {
                result = 0;
                values[0] = SWAP32(values[0]);
                values[1] = SWAP32(values[1]);
                *firstValue = values[0];
                *secondValue = values[1];
            }
        }
        return result;
    }

    record = find_resfile_by_refnum(OS_RefToMac(handle));
    if (record == NULL) {
        return 2;
    }
    bytes = (signed char *)record;
    *secondValue = (bytes[0x4e] << 24) | (bytes[0x4f] << 16) | (bytes[0x50] << 8) | bytes[0x51];
    *firstValue = (bytes[0x4a] << 24) | (bytes[0x4b] << 16) | (bytes[0x4c] << 8) | bytes[0x4d];
    return 0;
}

void fn_00408510(struct OSSpec *resourceSpec)
{
    fn_00411780(resourceSpec);
}

#pragma options align = mac68k

short __stdcall open_resource_fork(void *fileSpec, char mode, short *refNum)
{
    struct {
        union {
            SInt32 first;
            Type type;
        } u;
        unsigned char name[0x42];
    } resourceFile;
    char path[sizeof(OSSpec)];
    union {
        OSSpec spec;
        char path[sizeof(OSSpec)];
    } resourceSpec;
    Boolean create = (mode != 1);
    int error;
    short result;

    error = MacSpecs_MakeOSSpec(fileSpec, path);
    if (error != 0)
        return OS_OSErrorToMacError(error);

    error = MacSpecs_MakeResourceForkSpec(path, &resourceSpec.spec, create);
    if (error != 0)
        return OS_OSErrorToMacError(error);

    error = MacSpecs_MakeCWFileSpecFromString(resourceSpec.path, (CWFileSpec *)&resourceFile);
    if (error != 0)
        return OS_OSErrorToMacError(error);

    if (OS_Status(&resourceSpec.spec) != 0 && create)
        Files_CallWithFileSpecFromPath(resourceFile.u.first, resourceFile.u.type.size, resourceFile.name, 0x43574945,
                                       0x72737263);

    result = Files_OpenFileFromPath(resourceFile.u.first, resourceFile.u.type.size, resourceFile.name, mode, refNum);
    if (result == 0)
        append_identifier_list_node(short_predecessor(*refNum), &resourceSpec.spec);

    return result;
}

#pragma options align = reset

SInt16 __stdcall open_resource_file(void *param1, SInt8 param2)
{
    SInt32 kind;
    SInt16 local;
    SInt32 local2;
    SInt8 c;

    kind = param2;
    if (kind != 1)
        Files_CreateFile(param1, 0x43574945, 0x54455854, -1);
    if (kind == 2)
        c = 3;
    else
        c = param2;
    resource_error = open_resource_fork(param1, c, &local);
    if (resource_error == 0) {
        Files_GetSize(local, &local2);
        if (local2 == 0 && kind != 1)
            write_default_binary_record(local);
        read_resource_file(local, param2, NULL, 0);
        if (resource_error != 0) {
            Files_Close(local);
            close_resource_file(local);
            local = -1;
        }
        current_resfile_refnum = local;
    } else {
        local = -1;
    }
    return local;
}

unsigned char **Resources_GetHand(SInt32 first, SInt16 second)
{
    ResEntry *(*lookup)(int, short) = find_res_entry_in_files;
    ResEntry *argument = lookup(first, second);
    unsigned int value;
    if (argument) {
        resource_error = 0;
        return (unsigned char **)argument->hand;
    }
    value = data_00588525 ? -192U : 0;
    resource_error = (short)value;
    return NULL;
}

void __stdcall Resources_ClearError(void *handle)
{
    resource_error = 0;
    return;
}

void __stdcall close_resource_file(short param)
{
    if (find_resfile_by_refnum(param) != NULL && param != -1) {
        write_resource_file(param);
        free_res_file(param);
        Files_Close(param);
    }
    if (resfile_list != NULL)
        current_resfile_refnum = resfile_list->refnum;
    else
        current_resfile_refnum = 0;
}

static char lbl_00541248[] = "Could not find resource fork for ref = %d\n";
static char lbl_00541274[] = "Fork attributes = %04X\n";
static char lbl_0054128c[] = "Creator = '%4.4s';  Type = '%4.4s'\n";
static char lbl_005412b0[] = "Types:\n";
static char lbl_005412b8[] = "'%c%c%c%c':\n";
static char lbl_005412c8[] = "!!! RefList type '%c%c%c%c' does not match TypeList type !!!\n";
static char lbl_00541308[] = "<none>";
static char lbl_00541310[] = "\tID = %d '%s'\n";
static char lbl_00541320[] = "\tAttributes: ";
static char lbl_00541330[] = "SysHeap ";
static char lbl_0054133c[] = "Purgeable ";
static char lbl_00541348[] = "Locked ";
static char lbl_00541350[] = "Protected ";
static char lbl_0054135c[] = "Preload ";
static char lbl_00541368[] = "Changed ";
static char lbl_00541374[] = "\n";
static char lbl_00541378[] = "Contents:";
static char lbl_00541384[] = "\n%08X: ";
static char lbl_0054138c[] = "%02X ";
static char lbl_00541394[] = "   ";
static char lbl_00541398[] = " %16.16s";
