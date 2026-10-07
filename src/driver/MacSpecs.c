#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/MacSpecs.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/CFunc.h"
#include "compiler/IroLoop.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/StringUtils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int next_entry_index = 3;
static unsigned int next_entry_id = 2;

static struct NameRegistryEntry *spec_name_registry;
static struct MacSpecEntry **mac_spec_entries[256];
static unsigned int directory_count;
static char DAT_0057e818[64];
static char DAT_0057e858[520];
static char file_name_buffer[256];

int store_mac_spec_entry(MacSpecEntry *entry)
{
    unsigned int index = entry->index;
    unsigned int directory = index;
    unsigned int slot = index & 0xff;

    directory >>= 8;
    if (directory >= directory_count) {
        do {
            if (directory >= 0x100) {
                fprintf(stderr, "Fatal error:  too many directories referenced, out of memory\n");
                return 0;
            }
            mac_spec_entries[directory] = calloc(sizeof(*mac_spec_entries[directory]), 0x100);
            if (mac_spec_entries[directory] == NULL) {
                return 0;
            }
            ++directory_count;
        } while (directory >= directory_count);
    }
    mac_spec_entries[directory][slot] = entry;
    return 1;
}

struct NameRegistryEntry *find_or_create_name_registry_entry(struct NameRegistryEntry **entries, char *name)
{
    NameRegistryEntry *entry;

    for (; *entries != NULL; entries = &(*entries)->next) {
        if (OS_EqualPath(name, (*entries)->root.name) != 0) {
            return *entries;
        }
    }
    if (next_entry_id >= 0x100) {
        return NULL;
    }
    entry = (NameRegistryEntry *)malloc(sizeof(NameRegistryEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->next = NULL;
    entry->id = next_entry_id;
    next_entry_id = next_entry_id + 1;
    entry->root.name = (char *)malloc(strlen(name) + 1);
    if (entry->root.name == NULL) {
        return NULL;
    }
    strcpy(entry->root.name, name);
    entry->root.index = 2;
    entry->root.parent.registry = entry;
    entry->root.children = NULL;
    entry->root.next = NULL;
    *entries = entry;
    return entry;
}

struct MacSpecEntry *lookup_dir_id(unsigned int dirID)
{
    unsigned int index = dirID >> 8;
    unsigned int entryIndex = dirID & 0xff;
    if (dirID == 2U)
        CLIO_ReportAssertionFailure("dirID != 2", "MacSpecs.c", 166U);
    if (index >= directory_count)
        return NULL;
    return mac_spec_entries[index][entryIndex];
}

/* A named entry and the table that owns its entry chain. */
MacSpecEntry *find_or_create_child_entry(MacSpecEntry *table, char *name)
{
    MacSpecEntry **link;
    int result;
    MacSpecEntry *entry;

    for (link = &table->children; *link != NULL; link = &(*link)->next) {
        result = OS_EqualPath(name, (*link)->name);
        if (result != 0) {
            return *link;
        }
    }
    if (next_entry_index >= 0x10000) {
        return NULL;
    }
    result = strlen(name) + 1;
    entry = (MacSpecEntry *)malloc(sizeof(MacSpecEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->name = (char *)malloc(result);
    if (entry->name == NULL) {
        return NULL;
    }
    strcpy(entry->name, name);
    entry->children = NULL;
    entry->next = NULL;
    entry->index = next_entry_index;
    next_entry_index = next_entry_index + 1;
    result = store_mac_spec_entry(entry);
    if (result == 0) {
        return NULL;
    }
    entry->parent.entry = table;
    *link = entry;
    return entry;
}

int find_or_create_spec_entry(OSPathSpec *spec, unsigned int *typePtr, unsigned int *offsetPtr)
{
    char str[0x104];
    char name[0x40];
    char buf[0x104];
    char *end;
    char *p;
    NameRegistryEntry *node;
    MacSpecEntry *rec;
    char *tok;

    if (!OS_PathSpecToString(spec, buf, 0x104))
        return 0;
    end = OS_GetDirPtr(buf);
    strncpy(name, buf, end - buf);
    name[end - buf] = '\0';
    strcpy(str, end);
    node = find_or_create_name_registry_entry(&spec_name_registry, name);
    if (node == NULL)
        return 0;
    *typePtr = node->id;
    rec = &node->root;
    p = str + 1;
    if (str[0] != '\\')
        CLIO_ReportAssertionFailure("*pb == OS_PATHSEP", "MacSpecs.c", 0x108);
    while (*p != '\0') {
        tok = p;
        while (*p != '\0' && *p != '\\')
            p++;
        *p = '\0';
        rec = find_or_create_child_entry(rec, tok);
        p++;
        if (rec == NULL)
            return 0;
    }
    *offsetPtr = rec->index;
    return 1;
}

void lookup_spec_and_advance_parent(int *id, int *kind, unsigned int **result)
{
    NameRegistryEntry *node;

    if (*id == 1) {
        *result = NULL;
    } else if (*kind == 2U) {
        node = spec_name_registry;
        while (node != NULL && node->id != *id) {
            node = node->next;
        }
        if (node != NULL) {
            *result = (unsigned int *)&node->root;
            *id = 1;
            *kind = 1;
        } else {
            *result = NULL;
        }
    } else {
        *result = (unsigned int *)lookup_dir_id(*kind);
        if (*result != NULL) {
            MacSpecEntry *entry = (MacSpecEntry *)*result;
            MacSpecEntry *parent = entry->parent.entry;
            *kind = parent->index;
        }
    }
}

int find_or_create_spec_entry_negated(OSPathSpec *input, unsigned int *firstResult, unsigned int *secondResult)
{
    if (find_or_create_spec_entry(input, firstResult, secondResult) != 0) {
        *firstResult = -*firstResult;
        return 1;
    }
    *firstResult = 0;
    *secondResult = 0;
    return 0;
}

int build_name_and_backslash_path(int a, int b, void *buffer1, void *buffer2)
{
    int n;
    unsigned int *arr[256];
    unsigned int *val;
    char *p;
    char *out1 = buffer1;
    char *out2 = buffer2;

    a = -a;
    n = 0;
    do {
        lookup_spec_and_advance_parent(&a, &b, &val);
        if (val != NULL) {
            arr[n] = val;
            n++;
        }
    } while (val != NULL);

    if (n != 0) {
        strcpy(out1, (char *)*arr[--n]);
    } else {
        *out1 = 0;
        *out2 = 0;
        return 0;
    }

    *out2 = '\\';
    p = out2 + 1;
    while (n--) {
        strcpy(p, (char *)*arr[n]);
        p += strlen(p);
        *p++ = '\\';
    }
    *p = 0;
    return 1;
}

int __stdcall parse_value_and_offset(OSPathSpec *text, unsigned short *value, unsigned int *offset)
{
    unsigned int parsedValue;
    unsigned int parsedOffset;

    if (find_or_create_spec_entry_negated(text, &parsedValue, &parsedOffset)) {
        *value = parsedValue;
        *offset = parsedOffset;
        return 0;
    }
    *value = 0;
    *offset = 0;
    return 3;
}

int __stdcall OS_OSSpec_To_FSSpec(OSSpec *input, CWFileSpec *output)
{
    UInt16 volumeRef;
    unsigned int directoryId;
    int status;

    status = parse_value_and_offset(&input->path, &volumeRef, &directoryId);
    output->vRefNum = volumeRef;
    output->parID = directoryId;
    if (status != 0) {
        return status;
    }
    if (OS_NameSpecToString(&input->name, file_name_buffer, 0x40) == NULL) {
        return 0x6f;
    }
    c2pstrcpy(output->name, file_name_buffer);
    return 0;
}

DWORD __stdcall fn_00413670(short kind, int value, OSPathSpec *path)
{
    unsigned int directoryLength;
    unsigned int nameLength;
    DWORD result;

    if (kind == 0 || value == 0) {
        result = OS_GetCWD(path);
        if (result != 0) {
            return result;
        }
    } else {
        if (build_name_and_backslash_path(kind, value, DAT_0057e818, DAT_0057e858) == 0) {
            return 3;
        }
        directoryLength = strlen(DAT_0057e818);
        nameLength = strlen(DAT_0057e858);
        if ((int)(directoryLength + nameLength) < 0x104) {
            memcpy(path->s, DAT_0057e818, directoryLength);
            memcpy(path->s + directoryLength, DAT_0057e858, 1 + nameLength);
        }
    }
    return 0;
}

/* Unused lookup request declaration removed: no accesses or allocations. */
int __stdcall MacSpecs_MakeOSSpec(CWFileSpec *record, OSSpec *spec)
{
    int result = fn_00413670(record->vRefNum, record->parID, &spec->path);
    if (result != 0) {
        return result;
    }
    p2cstrcpy(file_name_buffer, record->name);
    return OS_MakeNameSpec(file_name_buffer, &spec->name);
}

int __stdcall MacSpecs_MakeResourceForkSpec(OSSpec *source, OSSpec *destination, char retryOnError)
{
    char pathBuffer[0x104];
    DWORD error;

    OS_PathSpecToString(&source->path, pathBuffer, 0x104);

    error = OS_MakeSpec2(pathBuffer, "RESOURCE.FRK", destination);
    if (error != 0)
        return error;

    error = OS_Status(destination);
    if (error != 0) {
        if (retryOnError != 0) {
            error = OS_Mkdir(destination);
            if (error != 0)
                return error;
            SetFileAttributesA(OS_SpecToString(destination, data_005880e0, 0x104), 2);
        } else {
            return error;
        }
        error = OS_MakeSpec2(pathBuffer, "RESOURCE.FRK", destination);
        if (error != 0)
            return error;
    } else {
        if (OS_IsFile(destination) != 0)
            return 0x10b;
    }

    error = OS_MakeNameSpec(OS_NameSpecToString(&source->name, data_005880e0, 0x104), &destination->name);
    if (error != 0)
        return error;
    return 0;
}
