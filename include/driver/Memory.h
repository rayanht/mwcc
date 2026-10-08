#ifndef DRIVER_MEMORY_H
#define DRIVER_MEMORY_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSHandle {
    char *addr;
    UInt32 size;
};
struct StorageHandle {
    union {
        char *data;
        struct TStreamElement *tokens;
    };
    OSHandle buffer;
};
extern OSHandle *Memory_GetSizeAddress(void *allocation);
extern unsigned int set_storage_handle_data(StorageHandle *handle, char *data);
extern unsigned short Memory_GetError(void);
extern StorageHandle *Memory_CreateStorageHandle(OSHandle *input);
extern void Memory_ExtractMemBuffer(void *input, OSHandle *result);
extern unsigned int Memory_NewHandle(unsigned int input);
extern unsigned int fn_00413990(unsigned int operand, short *value);
extern void __stdcall fn_00413a40(struct StorageHandle *handle);
extern unsigned short __stdcall Memory_AppendStorageHandle(const void *data, StorageHandle *handle, int size);
extern unsigned short __stdcall fn_00413b50(StorageHandle *source, StorageHandle *destination);
extern void __stdcall fn_00413a00(struct StorageHandle *data);
extern void __stdcall Memory_FreeHandle(StorageHandle *record);
extern void __stdcall fn_00413a50(void *entry);
extern SInt32 __stdcall Memory_GetHandleSize(struct StorageHandle *handle);
extern void __stdcall Memory_ResizeStorageHandle(StorageHandle *handle, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
