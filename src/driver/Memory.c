#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/Memory.h"
#include "driver/CLFileOps.h"
#include "driver/Generic.h"
#include <stdlib.h>

static unsigned short memory_error;
OSHandle *Memory_GetSizeAddress(void *allocation)
{
    OSHandle *buffer = (OSHandle *)allocation;
    return (OSHandle *)&buffer->size;
}

unsigned int set_storage_handle_data(StorageHandle *handle, char *data)
{
    handle->data = data;
    return (unsigned int)data;
}

StorageHandle *Memory_CreateStorageHandle(OSHandle *input)
{
    StorageHandle *record;
    record = malloc(sizeof(*record));
    if (!record) {
        memory_error = 65428U;
        return NULL;
    }
    record->buffer = *input;
    OS_InvalidateHandle(input);
    record->data = (char *)OS_LockHandle(&record->buffer);
    OS_UnlockHandle(&record->buffer);
    return record;
}

void Memory_ExtractMemBuffer(void *input, OSHandle *result)
{
    OSHandle *buffer;
    char *data;
    OSHandle *destination;
    StorageHandle *handle;
    StorageHandle *argument;
    handle = input;
    destination = result;
    argument = handle;
    buffer = Memory_GetSizeAddress(argument);
    data = buffer->addr;
    buffer = (OSHandle *)buffer->size;
    destination->addr = data;
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
    OSHandle recovery;
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
        HGLOBAL result = OS_LockHandle(&data->buffer);
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
        OS_UnlockHandle(&record->buffer);
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
    OSHandle *buffer;
    memory_error = OS_OSErrorToMacError(OS_ResizeHandle(buffer = &handle->buffer, size));
    set_storage_handle_data(handle, OS_LockHandle(buffer));
    OS_UnlockHandle(buffer);
}

unsigned short __stdcall Memory_AppendStorageHandle(const void *data, StorageHandle *handle, int size)
{
    int result;
    OSHandle *buffer;
    result = OS_OSErrorToMacError(OS_AppendHandle(buffer = &handle->buffer, data, size));
    set_storage_handle_data(handle, OS_LockHandle(buffer));
    OS_UnlockHandle(buffer);
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
