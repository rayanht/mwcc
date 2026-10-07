#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/COSToolsCLT.h"
#include "compiler/CPrep.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
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

void *fn_00443110(SInt32 size)
{
    return (struct StorageHandle *)Memory_NewHandle(size);
}

void *CompilerTools_AllocateMemoryIfEnabled(SIZE_T size)
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

void fn_00443160(void *handle)
{
    Memory_FreeHandle(handle);
}

Boolean fn_00443170(struct StorageHandle *handle, UInt32 size)
{
    Memory_ResizeStorageHandle(handle, size);
    return fn_00419620() == 0;
}

void fn_00443190(void *handle)
{
    fn_00413a00(handle);
}

void fn_004431a0(void *handle)
{
    fn_00413a40((struct StorageHandle *)handle);
}

void fn_004431b0(void *entry)
{
    fn_00413a50(entry);
}

UInt32 CompilerTools_GetScaledTicks(void)
{
    return CLFileOps_GetScaledTicks();
}

void CompilerTools_GetResourceCString(char *buffer, SInt16 id, SInt16 arg)
{
    CLIO_GetResourceString((unsigned char *)buffer, id, arg);
    CLIO_ConvertPascalToCString(buffer);
}

unsigned char CompilerTools_IsByteInDBCSCharacter(unsigned char *textStart, unsigned char *bytePosition)
{
    return MacSpecs_IsByteInDBCSCharacter(textStart, bytePosition);
}

int fn_00443200(CWFileSpec *record, short *output, unsigned int argument4, unsigned int argument5)
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

short fn_00443250(CWFileSpec *fileSpec, short *fileRef)
{
    return Files_OpenFileByPath(fileSpec->fileData.file.volumeRef, fileSpec->fileData.file.directoryId,
                                fileSpec->fileData.file.name, 1, fileRef);
}

SInt16 CompilerTools_GetFileType(CWFileSpec *arguments, UInt32 *output)
{
    FileIdentifierInfo resultData;
    SInt16 result;

    result =
        Files_GetFileIdentifierInfoFromPath(arguments->fileData.file.volumeRef, arguments->fileData.file.directoryId,
                                            arguments->fileData.file.name, &resultData);
    *output = resultData.type;
    return result;
}

short CompilerTools_GetFileSize(short fileRef, SInt32 *size)
{
    return Files_GetSize(fileRef, size);
}

SInt16 CompilerTools_ReadFile(SInt16 first, void *second, SInt32 third)
{
    return Files_Read(first, &third, second);
}

SInt16 CompilerTools_Write(SInt16 first, void *second, SInt32 third)
{
    return Files_Write(first, &third, second);
}

short fn_004432f0(short value, long *result)
{
    return Files_Tell(value, result);
}

SInt16 CompilerTools_SetFilePosition(SInt16 refNum, SInt32 position)
{
    return Files_SetPosition(refNum, 1, position);
}

void CompilerTools_CloseFile(short handleIndex)
{
    Files_Close(handleIndex);
}

void CompilerTools_MakeCWFileSpecFromPString(void *result, unsigned char *name)
{
    OSSpec spec;
    char path[256];

    memcpy(path, name + 1, *name);
    path[*name] = 0;
    make_osspec_from_path(path, &spec, NULL);
    MacSpecs_MakeCWFileSpecFromString((char *)&spec, (CWFileSpec *)result);
}

void CompilerTools_GetPFileFields(CWFileSpec *record, unsigned short *tag, SInt32 *value, void *data)
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
    char resolvedName[324];

    record.fileData.file.volumeRef = category;
    record.fileData.file.directoryId = recordId;
    copy_pstring(record.fileData.file.name, inputName);
    if (MacSpecs_MakeOSSpec(&record, resolvedName) == 0) {
        OS_SpecToString((OSSpec *)resolvedName, inputName, 260);
        CLIO_ConvertToPascalString(inputName);
    }
}

void CompilerTools_ResolveFileNameToCString(void *destination, PFile *record, SInt32 *result)
{
    RecordQuery query;
    if (result) {
        query.name = record->header.fileData.file.name;
        query.kind = record->header.fileData.file.volumeRef;
        query.value = record->header.fileData.file.directoryId;
        query.flags = 0;
        if (Files_UpdateRecordQuery(&query) == 0)
            *result = query.result;
        else
            *result = 0;
    }
    copy_pstring(destination, record->header.fileData.file.name);
    resolve_file_name_to_pascal_string(record->header.fileData.file.volumeRef, record->header.fileData.file.directoryId,
                                       destination);
    CLIO_ConvertPascalToCString(destination);
}

unsigned int CompilerTools_GetTicks(void)
{
    return OS_GetMilliseconds() * 0x3cu / 1000;
}
