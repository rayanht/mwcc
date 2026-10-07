#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLPrefs.h"
#include "compiler/CPrep.h"
#include "driver/AssertionFailure.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLPlugins.h"
#include "driver/CLTarg.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include <string.h>

/* Source and destination handles for a preference data copy. */
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#pragma auto_inline off
#include <ctype.h>

static NameTableEntry *name_table_entries;

#pragma auto_inline reset

StorageHandle *CLPrefs_CopyDestinationToTemporary(NameTableEntry *preferences)
{
    NameTableEntry *copy;
    unsigned int size;
    copy = preferences;
    size = Memory_GetHandleSize(copy->handles.storage.destination);
    if (copy->handles.storage.temporary == NULL) {
        copy->handles.storage.temporary = (StorageHandle *)Memory_NewHandle(size);
    } else {
        Memory_ResizeStorageHandle(copy->handles.storage.temporary, size);
    }
    if (Memory_GetError() != 0) {
        CLIO_ReportAssertionFailure("MemError()==noErr", "CLPrefs.c", 60);
    }
    fn_00413a00(copy->handles.storage.destination);
    fn_00413a00(copy->handles.storage.temporary);
    memcpy(copy->handles.storage.temporary->data, copy->handles.storage.destination->data, size);
    fn_00413a50(copy->handles.storage.temporary);
    fn_00413a50(copy->handles.storage.destination);
    return copy->handles.storage.temporary;
}

int CLPrefs_CopyStorage(NameTableEntry *storage, StorageHandle *source)
{
    unsigned int size = Memory_GetHandleSize(source);
    if (source != storage->handles.storage.temporary) {
        if (storage->handles.storage.temporary == NULL) {
            storage->handles.storage.temporary = (StorageHandle *)Memory_NewHandle(size);
        } else {
            Memory_ResizeStorageHandle(storage->handles.storage.temporary, size);
        }
        if (Memory_GetError() != 0) {
            return 0;
        }
        fn_00413a00(storage->handles.storage.temporary);
        fn_00413a00(source);
        memcpy(storage->handles.storage.temporary->data, source->data, size);
        fn_00413a50(source);
        fn_00413a50(storage->handles.storage.temporary);
    }
    fn_00413a00(storage->handles.storage.destination);
    fn_00413a00(storage->handles.storage.temporary);
    memcpy(storage->handles.storage.destination->data, storage->handles.storage.temporary->data, size);
    fn_00413a50(storage->handles.storage.temporary);
    fn_00413a50(storage->handles.storage.destination);
    return 1;
}

Boolean CLPrefs_AddPrefPanel(NameTableEntry *entry)
{
    NameTableEntry **link;

    link = &name_table_entries;
    while (*link != NULL) {
        if (strcmp((*link)->name, entry->name) == 0) {
            CLErrors_EmitDiagnostic(0x5a, entry->name);
            return 0;
        }
        link = &(*link)->next;
    }
    if (DAT_00587324 != '\0') {
        CLIO_FormatAndDispatchText("Defining/adding pref panel '%s'\n", entry->name);
    }
    *link = entry;
    return 1;
}

NameTableEntry *CLPrefs_FindNameTableEntry(char *name)
{
    NameTableEntry *entry = name_table_entries;
    while (entry != NULL) {
        if (CLIO_CompareStringsIgnoreCase(entry->name, name) == 0) {
            break;
        }
        entry = entry->next;
    }
    return entry;
}

unsigned int CLPrefs_ConvertLoneLFToCR(StorageHandle *buffer)
{
    char *cursor;
    char *end;
    int length;
    fn_00413a00(buffer);
    end = buffer->data;
    length = Memory_GetHandleSize(buffer);
    cursor = end;
    while (cursor < end + length) {
        if (*cursor == '\r') {
            if (cursor[1] == '\n')
                cursor += 2;
            else
                cursor++;
        } else {
            if (*cursor == '\n')
                *cursor = '\r';
            cursor++;
        }
    }
    fn_00413a50(buffer);
    return 0;
}

#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset
