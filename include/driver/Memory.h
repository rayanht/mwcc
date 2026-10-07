#ifndef DRIVER_MEMORY_H
#define DRIVER_MEMORY_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MemBuffer {
    char *
        ptr; /* 0x00: OS_NewHandle allocates and clears bytes; OS_ResizeHandle clears the appended bytes; Memory_ExtractMemBuffer reads char *data. */
    UInt32 size; /* 0x04: OS_NewHandle sets the byte count; OS_GetHandleSize returns it. */
};
struct StorageHandle {
    union {
        char *data;
        struct TStreamElement *tokens;
    };
    MemBuffer buffer;
};
struct StorageHandle;
struct StorageHandle;
extern MemBuffer *Memory_GetSizeAddress(void *a0);
extern unsigned int set_storage_handle_data(StorageHandle *a0, char *a1);
extern unsigned short Memory_GetError(void);
extern StorageHandle *Memory_CreateStorageHandle(MemBuffer *input);
extern void Memory_ExtractMemBuffer(void *input, MemBuffer *result);
extern unsigned int Memory_NewHandle(unsigned int input);
extern unsigned int fn_00413990(unsigned int operand, short *value);
extern void __stdcall fn_00413a40(struct StorageHandle *a0);
extern unsigned short __stdcall Memory_AppendStorageHandle(const void *a0, StorageHandle *a1, int a2);
extern unsigned short __stdcall fn_00413b50(StorageHandle *a0, StorageHandle *a1);
extern void __stdcall fn_00413a00(struct StorageHandle *data);
extern void __stdcall Memory_FreeHandle(StorageHandle *record);
extern void __stdcall fn_00413a50(void *entry);
extern SInt32 __stdcall Memory_GetHandleSize(struct StorageHandle *a0);
extern void __stdcall Memory_ResizeStorageHandle(struct StorageHandle *a0, unsigned int a1);

#ifdef __cplusplus
}
#endif

#endif
