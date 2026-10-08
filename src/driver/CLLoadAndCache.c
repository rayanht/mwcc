#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLLoadAndCache.h"
#include "driver/CLBrowser.h"
#include "driver/CLIO.h"
#include "driver/CLPrefs.h"
#include "driver/MacFileTypes.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include <string.h>
#pragma auto_inline off

#pragma auto_inline reset

int CLLoadAndCache_GetFileText(OSSpec *path, StorageHandle **outHandle, UInt8 *outFlag)
{
    UInt32 filetype;
    SInt32 handle;
    SInt32 size;
    int flag;
    StorageHandle *buffer;
    DWORD ret;
    StorageHandle *buf;

    *outHandle = CLBrowser_FindCacheEntryBuffer(path, outFlag);
    if (*outHandle != NULL)
        return 0;

    flag = 0;
    ret = MacFileTypes_GetFileType(path, &filetype);
    if (ret == 0 && filetype != 0x54455854)
        flag = 1;
    *outFlag = flag;
    *outHandle = NULL;

    ret = OS_Open(path, 0, &handle);
    if (ret != 0)
        return ret;

    ret = OS_GetSize(handle, &size);
    if (ret != 0)
        return ret;

    buf = (StorageHandle *)Memory_NewHandle(size + 1);
    if (buf == NULL) {
        CLIO_FormatAndDispatchText("\nOut of memory\n");
        longjmp(driver_jmp_buf, 1);
    }

    fn_00413a00(buf);
    ret = OS_Read(handle, buf->data, &size);
    if (ret != 0) {
        Memory_FreeHandle(buf);
        OS_Close(handle);
        return ret;
    }
    OS_Close(handle);
    buf->data[size] = 0;
    fn_00413a50(buf);
    if (*outFlag == 0)
        CLPrefs_ConvertLoneLFToCR(buf);
    buffer = buf;
    CLBrowser_CacheFileText(path, buffer, *outFlag);
    *outHandle = buf;
    return 0;
}

void CLLoadAndCache_CopyStorageHandleData(StorageHandle *sourceHandle, void **bufferOut, unsigned int *sizeOut)
{
    *sizeOut = Memory_GetHandleSize(sourceHandle);
    *bufferOut = xmalloc(NULL, *sizeOut);
    fn_00413a00(sourceHandle);
    memcpy(*bufferOut, sourceHandle->data, *sizeOut);
    *sizeOut -= 1;
    fn_00413a50(sourceHandle);
}
