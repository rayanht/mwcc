#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/Memory.h"
#include "compiler/win32.h"
#include "compiler/CPrep.h"
#include "driver/CLFileOps.h"
#include "driver/Generic.h"
#include "driver/MsDos.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static unsigned short memory_error;
MemBuffer *Memory_GetSizeAddress(void *allocation)
{
    MemBuffer *buffer = (MemBuffer *)allocation;
    return (MemBuffer *)&buffer->size;
}

unsigned int set_storage_handle_data(StorageHandle *handle, char *data)
{
    handle->data = data;
    return (unsigned int)data;
}

StorageHandle *Memory_CreateStorageHandle(MemBuffer *input)
{
    StorageHandle *record;
    record = malloc(sizeof(*record));
    if (!record) {
        memory_error = 65428U;
        return NULL;
    }
    record->buffer = *input;
    OS_InvalidateHandle(input);
    record->data = (char *)MsDos_GetValidMemBufferPtr(&record->buffer);
    fn_004129c0(&record->buffer);
    return record;
}

void Memory_ExtractMemBuffer(void *input, MemBuffer *result)
{
    MemBuffer *buffer;
    char *data;
    MemBuffer *destination;
    StorageHandle *handle;
    StorageHandle *argument;
    handle = input;
    destination = result;
    argument = handle;
    buffer = Memory_GetSizeAddress(argument);
    data = buffer->ptr;
    buffer = (MemBuffer *)buffer->size;
    destination->ptr = data;
    destination->size = (UInt32)buffer;
    argument = handle;
    free(argument);
}

unsigned short Memory_GetError(void)
{
    return memory_error;
}

unsigned int Memory_NewHandle(unsigned int input)
{
    MemBuffer recovery;
    DWORD result = OS_NewHandle(input, &recovery);
    if (result != 0U) {
        memory_error = OS_OSErrorToMacError(result);
        return 0U;
    }
    return (unsigned int)Memory_CreateStorageHandle(&recovery);
}

unsigned int fn_00413990(unsigned int operand, short *value)
{
    unsigned int result;
    *value = 0;
    result = Memory_NewHandle(operand);
    if (result == 0) {
        *value = memory_error;
    }
    return result;
}

void __stdcall Memory_FreeHandle(StorageHandle *record)
{
    DWORD value;
    if (record) {
        value = OS_FreeHandle(&record->buffer);
        value = OS_OSErrorToMacError(value);
        memory_error = value;
        set_storage_handle_data(record, NULL);
    } else {
        memory_error = 65427U;
    }
}

void __stdcall fn_00413a00(struct StorageHandle *data)
{
    if (data != NULL) {
        HGLOBAL result = MsDos_GetValidMemBufferPtr(&data->buffer);
        set_storage_handle_data(data, result);
        memory_error = 0U;
    } else {
        memory_error = 65427U;
    }
}

void __stdcall fn_00413a40(struct StorageHandle *handle)
{
    fn_00413a00(handle);
}

void __stdcall fn_00413a50(void *entry)
{
    StorageHandle *record = entry;
    if (record) {
        memory_error = 0;
        fn_004129c0(&record->buffer);
    } else {
        memory_error = -109;
    }
}

SInt32 __stdcall Memory_GetHandleSize(struct StorageHandle *handle)
{
    int error;
    DWORD value;
    if (handle != NULL) {
        error = OS_OSErrorToMacError(OS_GetHandleSize(&handle->buffer, &value));
        memory_error = error;
    } else {
        value = 0;
        memory_error = 0xff93U;
    }
    return value;
}

void __stdcall Memory_ResizeStorageHandle(StorageHandle *handle, unsigned int size)
{
    MemBuffer *buffer;
    memory_error = OS_OSErrorToMacError(OS_ResizeHandle(buffer = &handle->buffer, size));
    set_storage_handle_data(handle, MsDos_GetValidMemBufferPtr(buffer));
    fn_004129c0(buffer);
}

unsigned short __stdcall Memory_AppendStorageHandle(const void *data, StorageHandle *handle, int size)
{
    int result;
    MemBuffer *buffer;
    result = OS_OSErrorToMacError(CLFileOps_AppendMemBuffer(buffer = &handle->buffer, data, size));
    set_storage_handle_data(handle, MsDos_GetValidMemBufferPtr(buffer));
    fn_004129c0(buffer);
    return result;
}

unsigned short __stdcall fn_00413b50(StorageHandle *source, StorageHandle *destination)
{
    SInt32 size;
    char *data;
    unsigned short result;
    fn_00413a00(source);
    size = Memory_GetHandleSize(source);
    data = source->data;
    result = Memory_AppendStorageHandle(data, destination, size);
    fn_00413a50(source);
    return result;
}
