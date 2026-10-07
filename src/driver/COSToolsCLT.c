#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/COSToolsCLT.h"
#include "compiler/CPrep.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/TextUtils.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>

void copy_pstring(UInt8 *destination, UInt8 *source)
{
    int remaining;

    for (remaining = *source; (short)remaining >= 0; remaining--) {
        *destination++ = *source++;
    }
}

void *COS_NewHandle(SInt32 size)
{
    return (struct StorageHandle *)Memory_NewHandle(size);
}

void *COS_NewOSHandle(SIZE_T size)
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

void COS_FreeHandle(void *handle)
{
    Memory_FreeHandle(handle);
}

Boolean COS_ResizeHandle(struct StorageHandle *handle, UInt32 size)
{
    Memory_ResizeStorageHandle(handle, size);
    return fn_00419620() == 0;
}

void COS_LockHandle(void *handle)
{
    fn_00413a00(handle);
}

void COS_LockHandleHi(void *handle)
{
    fn_00413a40((struct StorageHandle *)handle);
}

void COS_UnlockHandle(void *entry)
{
    fn_00413a50(entry);
}

UInt32 COS_GetTicks(void)
{
    return CLFileOps_GetScaledTicks();
}

void COS_GetString(char *buffer, SInt16 id, SInt16 arg)
{
    CLIO_GetResourceString((unsigned char *)buffer, id, arg);
    CLIO_ConvertPascalToCString(buffer);
}

unsigned char COS_IsMultiByte(unsigned char *textStart, unsigned char *bytePosition)
{
    return MacSpecs_IsByteInDBCSCharacter(textStart, bytePosition);
}

int COS_FileNew(CWFileSpec *record, short *output, unsigned int argument4, unsigned int argument5)
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

short COS_FileOpen(CWFileSpec *fileSpec, short *fileRef)
{
    return Files_OpenFileByPath(fileSpec->fileData.file.volumeRef, fileSpec->fileData.file.directoryId,
                                fileSpec->fileData.file.name, 1, fileRef);
}

SInt16 COS_FileGetType(CWFileSpec *arguments, UInt32 *output)
{
    FileIdentifierInfo resultData;
    SInt16 result;

    result =
        Files_GetFileIdentifierInfoFromPath(arguments->fileData.file.volumeRef, arguments->fileData.file.directoryId,
                                            arguments->fileData.file.name, &resultData);
    *output = resultData.type;
    return result;
}

short COS_FileGetSize(short fileRef, SInt32 *size)
{
    return Files_GetSize(fileRef, size);
}

SInt16 COS_FileRead(SInt16 first, void *second, SInt32 third)
{
    return Files_Read(first, &third, second);
}

SInt16 COS_FileWrite(SInt16 first, void *second, SInt32 third)
{
    return Files_Write(first, &third, second);
}

short COS_FileGetPos(short value, long *result)
{
    return Files_Tell(value, result);
}

SInt16 COS_FileSetPos(SInt16 refNum, SInt32 position)
{
    return Files_SetPosition(refNum, 1, position);
}

void COS_FileClose(short handleIndex)
{
    Files_Close(handleIndex);
}

void COS_FileSetFSSpec(void *result, unsigned char *name)
{
    OSSpec spec;
    char path[256];

    memcpy(path, name + 1, *name);
    path[*name] = 0;
    OS_MakeSpec(path, &spec, NULL);
    OS_OSSpec_To_FSSpec(&spec, (CWFileSpec *)result);
}

void COS_FileGetFSSpecInfo(CWFileSpec *record, unsigned short *tag, SInt32 *value, void *data)
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

void resolve_file_name_to_pascal_string(short category, int recordId, void *inputName)
{
    CWFileSpec record;
    OSSpec resolvedName;

    record.fileData.file.volumeRef = category;
    record.fileData.file.directoryId = recordId;
    copy_pstring(record.fileData.file.name, inputName);
    if (MacSpecs_MakeOSSpec(&record, &resolvedName) == 0) {
        OS_SpecToString(&resolvedName, inputName, 260);
        CLIO_ConvertToPascalString(inputName);
    }
}

void COS_FileGetPathName(void *destination, CPrepFileInfo *record, SInt32 *result)
{
    RecordQuery query;
    if (result) {
        query.name = record->textfile.fileData.file.name;
        query.kind = record->textfile.fileData.file.volumeRef;
        query.value = record->textfile.fileData.file.directoryId;
        query.flags = 0;
        if (Files_UpdateRecordQuery(&query) == 0)
            *result = query.result;
        else
            *result = 0;
    }
    copy_pstring(destination, record->textfile.fileData.file.name);
    resolve_file_name_to_pascal_string(record->textfile.fileData.file.volumeRef,
                                       record->textfile.fileData.file.directoryId, destination);
    CLIO_ConvertPascalToCString(destination);
}

unsigned int CompilerTools_GetTicks(void)
{
    return OS_GetMilliseconds() * 0x3cu / 1000;
}
