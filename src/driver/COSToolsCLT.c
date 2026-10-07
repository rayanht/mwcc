#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/COSToolsCLT.h"
#include "compiler/CPrep.h"
#include "driver/CLFileOps.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
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

    if (data_0054c3d0 != '\0') {
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

    Files_DeleteFileFromPath(record->vRefNum, record->parID, record->name);
    result = Files_CallWithFileSpecFromPath(record->vRefNum, record->parID, record->name, argument4, argument5);
    if ((short)result == 0) {
        Files_OpenFileByPath(record->vRefNum, record->parID, record->name, 3, output);
    }
}

short COS_FileOpen(CWFileSpec *fileSpec, short *fileRef)
{
    return Files_OpenFileByPath(fileSpec->vRefNum, fileSpec->parID, fileSpec->name, 1, fileRef);
}

SInt16 COS_FileGetType(CWFileSpec *arguments, UInt32 *output)
{
    FileIdentifierInfo resultData;
    SInt16 result;

    result = Files_GetFileIdentifierInfoFromPath(arguments->vRefNum, arguments->parID, arguments->name, &resultData);
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
        *tag = record->vRefNum;
    }
    if (value != NULL) {
        *value = record->parID;
    }
    if (data != NULL) {
        copy_pstring(data, record->name);
    }
}

void resolve_file_name_to_pascal_string(short category, int recordId, void *inputName)
{
    CWFileSpec record;
    OSSpec resolvedName;

    record.vRefNum = category;
    record.parID = recordId;
    copy_pstring(record.name, inputName);
    if (MacSpecs_MakeOSSpec(&record, &resolvedName) == 0) {
        OS_SpecToString(&resolvedName, inputName, 260);
        CLIO_ConvertToPascalString(inputName);
    }
}

void COS_FileGetPathName(void *destination, CPrepFileInfo *record, SInt32 *result)
{
    RecordQuery query;
    if (result) {
        query.name = record->textfile.name;
        query.kind = record->textfile.vRefNum;
        query.value = record->textfile.parID;
        query.flags = 0;
        if (Files_UpdateRecordQuery(&query) == 0)
            *result = query.result;
        else
            *result = 0;
    }
    copy_pstring(destination, record->textfile.name);
    resolve_file_name_to_pascal_string(record->textfile.vRefNum, record->textfile.parID, destination);
    CLIO_ConvertPascalToCString(destination);
}

unsigned int CompilerTools_GetTicks(void)
{
    return OS_GetMilliseconds() * 0x3cu / 1000;
}
