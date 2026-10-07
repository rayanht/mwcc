#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/Files.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/win32.h"
#include "compiler/CPrep.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLTarg.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/MacFileTypes.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/Resources.h"
#include "driver/StringUtils.h"
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
/* Data of the original file that none of its linked code uses. */
static char lbl_0054B8F8[] = "%*.*s";

int get_file_query_info_and_identifiers(OSSpec *path, FileQueryInfo *info, FileIdentifierInfo *identifiers)
{
    FILETIME firstTimestamp;
    FILETIME secondTimestamp;
    UInt32 fileType;
    UInt32 creator;
    int timestamp;
    SInt32 dataSize;
    SInt32 auxiliarySize;
    OSSpec auxiliaryPath;
    SInt32 handle;
    int identifier;
    char typeBuffer[32];
    SInt32 typeLength;
    DWORD error;
    Boolean alreadyOpen;

    if (info != NULL) {
        OS_GetFileTime(path, &firstTimestamp, &secondTimestamp);
        OS_TimeToMac(firstTimestamp, &timestamp);
        info->firstTimestamp = timestamp;
        OS_TimeToMac(secondTimestamp, &timestamp);
        info->secondTimestamp = timestamp;
    }
    error = OS_Open(path, 0, &handle);
    if (error != 0)
        return error;
    typeLength = 0x20;
    if (OS_Read(handle, typeBuffer, &typeLength) != 0)
        typeLength = 0;
    if (info != NULL) {
        info->attributes = OS_IsDir(path) ? 0x10 : 0;
        OS_GetSize(handle, &dataSize);
        info->dataSize = info->dataAllocatedSize = dataSize;
        info->dataReserved = 0;
    }
    OS_Close(handle);
    MacFileTypes_MatchBytes(typeBuffer, typeLength, &fileType);
    creator = 0x43574945;
    if (fn_00406610() && MacSpecs_MakeResourceForkSpec(path, &auxiliaryPath, 0) == 0 &&
        OS_Status(&auxiliaryPath) == 0) {
        alreadyOpen = Resources_FindIdentifier(&auxiliaryPath, &handle);
        if (alreadyOpen || OS_Open(&auxiliaryPath, 0, &handle) == 0) {
            if (info != NULL) {
                OS_GetSize(handle, &auxiliarySize);
                info->auxiliarySize = info->auxiliaryAllocatedSize = auxiliarySize;
                info->auxiliaryReserved = 0;
            }
            identifier = handle;
            fn_004083b0(identifier, &creator, &fileType);
            if (!alreadyOpen)
                OS_Close(handle);
        }
    } else if (info != NULL) {
        info->auxiliarySize = info->auxiliaryAllocatedSize = 0;
        info->auxiliaryReserved = 0;
    }
    identifiers->type = fileType;
    identifiers->creator = creator;
    identifiers->flags = MsDos_ReturnZero(path) ? 0x8000 : 0;
    identifiers->reserved1 = 0;
    identifiers->reserved2 = 0;
    identifiers->reserved3 = 0;
    return 0;
}

int set_file_and_resource_fork_times(OSSpec *path, DWORD *timeValues, DWORD *metadataValues)
{
    char enabled;
    DWORD result;
    Boolean handleAvailable;
    FILETIME firstTime;
    FILETIME secondTime;
    OSSpec resolvedPath;
    SInt32 handle;

    OS_MacToTime(*timeValues, &firstTime);
    OS_MacToTime(timeValues[1], &secondTime);
    OS_SetFileTime(path, &firstTime, &secondTime);
    fn_00421af0(path, *metadataValues);
    enabled = fn_00406610();
    if (enabled != '\0') {
        result = MacSpecs_MakeResourceForkSpec(path, &resolvedPath, '\0');
        if (result == 0) {
            result = OS_Status(&resolvedPath);
            if (result == 0) {
                handleAvailable = Resources_FindIdentifier(&resolvedPath, &handle);
                if (handleAvailable == '\0') {
                    result = OS_Open(&resolvedPath, 2, &handle);
                    if (result > 0) {
                        return 0;
                    }
                }
                Resources_SetFileTimes(handle, metadataValues[1], *metadataValues);
                if (handleAvailable == '\0') {
                    OS_Close(handle);
                }
            }
        }
    }
    return 0;
}

int __stdcall Files_CallWithFileSpecFromPath(short volume, int directory, UInt8 *path, unsigned int secondArgument,
                                             unsigned int thirdArgument)
{
    int result;
    CWFileSpec fileSpec;

    result = Files_MakeFileSpecFromPath(volume, directory, path, &fileSpec);
    if ((short)result != 0 && (short)result != -43) {
        return result;
    }
    return Files_CreateFile(&fileSpec, secondArgument, thirdArgument, -1);
}

short __stdcall Files_OpenFileFromPath(int input, int options, unsigned char *data, char mode, short *output)
{
    int status;
    CWFileSpec workspace;

    status = Files_MakeFileSpecFromPath(input, options, data, &workspace);
    if ((short)status != 0 && (short)status != -43)
        return status;
    return Files_OpenFile(&workspace, mode, output);
}

short __stdcall Files_OpenFileByPath(unsigned short volumeRef, unsigned int directoryId, UInt8 *name, char mode,
                                     short *handle)
{
    int result;
    CWFileSpec file;
    result = Files_MakeFileSpecFromPath(volumeRef, directoryId, name, &file);
    if (((short)result != 0) && ((short)result != -0x2b)) {
        return result;
    }
    return Files_OpenFile(&file, mode, handle);
}

int __stdcall Files_DeleteFileFromPath(unsigned short kind, int value, UInt8 *data)
{
    DWORD result;
    CWFileSpec buffer;

    result = Files_MakeFileSpecFromPath(kind, value, data, &buffer);
    if (((short)result != 0) && ((short)result != -0x2b)) {
        return result;
    }
    result = Files_DeleteFile(&buffer);
    return result;
}

SInt16 __stdcall Files_GetFileIdentifierInfoFromPath(SInt16 input, SInt32 selector, unsigned char *options,
                                                     FileIdentifierInfo *destination)
{
    CWFileSpec workspace;
    SInt16 status;
    status = Files_MakeFileSpecFromPath(input, selector, options, &workspace);
    if (status != 0 && status != -43) {
        return status;
    }
    return get_file_identifier_info(&workspace, destination);
}

unsigned int __stdcall Files_CreateFile(CWFileSpec *input, unsigned int secondArgument, unsigned int thirdArgument,
                                        int fourthArgument)
{
    union {
        OSSpec spec;
        char bytes[324];
    } path;
    union {
        RecordData data;
        unsigned int arguments[2];
    } recordData;
    unsigned int status;
    int result;

    result = MacSpecs_MakeOSSpec(input, &path.spec);
    if (result != 0) {
        return OS_OSErrorToMacError(result);
    }
    status = OS_Status(&path.spec);
    if (status == 0) {
        return 0xffffffd0;
    }
    status = OS_Create(&path.spec, &data_0054b770);
    if (status != 0) {
        return OS_OSErrorToMacError(status);
    }
    memset(&recordData, 0, sizeof(recordData));
    recordData.arguments[0] = thirdArgument;
    recordData.arguments[1] = secondArgument;
    set_record_identifier(input, &recordData.data);
    return 0;
}

short __stdcall Files_OpenFile(CWFileSpec *file, char mode, short *result)
{
    static unsigned char os_open_modes[5] = {2, 0, 1, 2, 2};
    OSSpec path;
    SInt32 handle;
    unsigned int status;
    DWORD openStatus;

    *result = 0;
    status = MacSpecs_MakeOSSpec(file, &path);
    if (status != 0) {
        return OS_OSErrorToMacError(status);
    }
    if (OS_IsDir(&path) != 0) {
        return 0xffffff88;
    }
    openStatus = OS_Open(&path, os_open_modes[mode], &handle);
    if (openStatus != 0) {
        return OS_OSErrorToMacError(openStatus);
    }
    status = handle;
    status = OS_RefToMac(status);
    *result = (short)status;
    return 0;
}

DWORD __stdcall Files_DeleteFile(CWFileSpec *input)
{
    unsigned int status;
    DWORD result;
    DWORD conversionStatus;
    OSSpec firstBuffer;
    OSSpec secondBuffer;

    status = MacSpecs_MakeOSSpec(input, &firstBuffer);
    if (status != 0) {
        return OS_OSErrorToMacError(status);
    }
    result = OS_Delete(&firstBuffer);
    conversionStatus = MacSpecs_MakeResourceForkSpec(&firstBuffer, &secondBuffer, 0);
    if (conversionStatus == 0) {
        result = OS_Delete(&secondBuffer);
        fn_00408510(&secondBuffer);
    }
    return OS_OSErrorToMacError(result);
}

int __stdcall get_file_identifier_info(CWFileSpec *input, FileIdentifierInfo *result)
{
    int error;
    OSSpec buffer;
    FileIdentifierInfo output;

    error = MacSpecs_MakeOSSpec(input, &buffer);
    if (error)
        return OS_OSErrorToMacError(error);
    error = get_file_query_info_and_identifiers(&buffer, NULL, &output);
    if (error)
        return OS_OSErrorToMacError(error);
    memcpy(result, &output, sizeof(output));
    return 0;
}

void __stdcall set_record_identifier(CWFileSpec *input, RecordData *data)
{
    RecordQuery record;
    UInt8 name[256];

    _pstrcpy(name, input->fileData.file.name);
    record.kind = input->fileData.file.volumeRef;
    record.value = input->fileData.file.directoryId;
    record.name = name;
    record.flags = 0;
    if (Files_UpdateRecordQuery(&record) == 0) {
        record.identifier = *data;
        fn_00414710(&record);
    }
}

SInt16 __stdcall Files_Read(SInt16 file, SInt32 *byteCount, void *buffer)
{
    if (file == 0) {
        return -51;
    }
    return OS_OSErrorToMacError(OS_Read(short_predecessor(file), buffer, byteCount));
}

SInt16 __stdcall Files_Write(SInt16 refNum, SInt32 *size, void *buffer)
{
    if (refNum == 0) {
        return -51;
    }
    return OS_OSErrorToMacError(OS_Write(short_predecessor(refNum), buffer, (DWORD *)size));
}

DWORD __stdcall Files_Close(short handleIndex)
{
    DWORD result;
    struct OSSpec *buffer;
    int handle;
    DWORD status;
    SInt32 value;

    if (handleIndex == 0) {
        return 0xffffffcd;
    }
    handle = short_predecessor(handleIndex);
    buffer = &Resources_FindIdentifierById(handle)->fileSpec;
    if (buffer != NULL) {
        status = OS_GetSize(handle, &value);
        result = OS_OSErrorToMacError(OS_Close(handle));
        if ((status == 0) && (value == 0)) {
            OS_Delete(buffer);
            fn_00408510(buffer);
        }
        Resources_RemoveIdentifier(handle);
    } else {
        result = OS_OSErrorToMacError(OS_Close(handle));
    }
    return result;
}

short __stdcall Files_GetSize(short predecessor, SInt32 *result)
{
    if (predecessor == 0)
        return -51;
    {
        short value = predecessor;
        return OS_OSErrorToMacError(OS_GetSize(short_predecessor(value), result));
    }
}

SInt16 __stdcall Files_SetSize(SInt16 handleId, SInt32 size)
{
    if (handleId == 0) {
        return -51;
    }
    return OS_OSErrorToMacError(OS_SetSize(short_predecessor(handleId), size));
}

DWORD __stdcall Files_Tell(unsigned short identifier, long *result)
{
    if (identifier == 0) {
        return 0xffffffcd;
    }
    return OS_OSErrorToMacError(OS_Tell(short_predecessor(identifier), result));
}

int __stdcall Files_SetPosition(short refNum, short posMode, SInt32 posOff)
{
    int h;
    SInt32 cur;
    SInt32 eof;
    UInt32 pos;
    UInt32 err;
    int mode;

    h = short_predecessor(refNum);
    if (!refNum)
        return -51;
    posMode &= 3;
    if ((mode = posMode) != 0) {
        if ((err = OS_Tell(h, &cur)) != 0)
            return OS_OSErrorToMacError(err);
        if ((err = OS_GetSize(h, &eof)) != 0)
            return OS_OSErrorToMacError(err);
        if (mode == 1)
            pos = posOff;
        else if (mode == 2)
            pos = eof + posOff;
        else if (mode == 3)
            pos = cur + posOff;
        else
            return -50;
        if ((err = OS_Seek(h, 1, pos)) != 0)
            return OS_OSErrorToMacError(err);
        if (pos > eof) {
            OS_Seek(h, 1, cur);
            return -39;
        }
    }
    return 0;
}

short __stdcall Files_UpdateRecordQuery(RecordQuery *record)
{
    RecordQuery updatedRecord;
    CWFileSpec resolvedFile;
    OSSpec fileHandle;
    union {
        FileQueryInfo info;
        struct {
            UInt32 value0;
            UInt32 value1;
            UInt32 value2;
            UInt32 value3;
            UInt32 value4;
            UInt32 value5;
            UInt8 reserved[8];
            UInt8 attributes;
        } values;
    } query;
    FileIdentifierInfo identifier;
    unsigned char *nameSpace;
    int result;

    memcpy(&updatedRecord, record, sizeof(updatedRecord));
    if (updatedRecord.flags != -1)
        nameSpace = updatedRecord.name;
    else
        nameSpace = (unsigned char *)"";
    result = Files_MakeFileSpecFromPath(updatedRecord.kind, updatedRecord.value, nameSpace, &resolvedFile);
    if ((short)result == 0) {
        if (MacSpecs_MakeOSSpec(&resolvedFile, &fileHandle) == 0) {
            get_file_query_info_and_identifiers(&fileHandle, &query.info, &identifier);
            updatedRecord.queryValue0 = query.values.value0;
            updatedRecord.result = query.values.value1;
            updatedRecord.queryValue2 = query.values.value2;
            updatedRecord.queryValue3 = query.values.value3;
            updatedRecord.queryValue4 = query.values.value4;
            updatedRecord.queryValue5 = query.values.value5;
            updatedRecord.attributes = query.values.attributes;
            memcpy(&updatedRecord.identifier, &identifier, sizeof(updatedRecord.identifier));
        } else {
            memset(&updatedRecord, 0, sizeof(updatedRecord));
        }
    }
    updatedRecord.status = result;
    memcpy(record, &updatedRecord, sizeof(updatedRecord));
    return result;
}

static unsigned char lbl_0054B90C[] = {1, 1, 2};

UInt16 __stdcall fn_00414710(RecordQuery *record)
{
    RecordQuery copy;
    CWFileSpec argumentBuffer;
    OSSpec workBuffer;
    DWORD blockWords[9];
    DWORD callBlock[4];
    RecordData *savedBlock;
    UInt16 result;
    int output;

    savedBlock = &copy.identifier;
    memcpy(&copy, record, sizeof(copy));
    result = Files_MakeFileSpecFromPath(copy.kind, copy.value, copy.name, &argumentBuffer);
    if (result == 0) {
        output = MacSpecs_MakeOSSpec(&argumentBuffer, &workBuffer);
        if (output == 0) {
            memcpy(&callBlock, savedBlock, sizeof(callBlock));
            memset(blockWords, 0, sizeof(blockWords));
            blockWords[0] = copy.queryValue0;
            blockWords[1] = copy.result;
            output = set_file_and_resource_fork_times(&workBuffer, blockWords, callBlock);
        }
        result = OS_OSErrorToMacError(output);
    }
    copy.status = result;
    memcpy(record, &copy, sizeof(copy));
    return result;
}

int __stdcall Files_MakeFileSpecFromPath(short volume, int directory, unsigned char *path, CWFileSpec *result)
{
    enum { volumeNameCapacity = 64 };
    int error;
    int index;
    char character;
    char *output;
    OSSpec location;
    char fullPath[260];
    CWFileSpec base;
    char volumeName[volumeNameCapacity];

    result->fileData.file.volumeRef = 0;
    result->fileData.file.directoryId = 0;
    if (volume == 0 && directory == 0) {
        error = OS_GetCWD(&location.path);
        if (error != 0) {
            result->fileData.file.name[0] = 0;
            return OS_OSErrorToMacError(error);
        }
        fn_00412340(&location.path, fullPath, sizeof(fullPath));
    } else {
        if (volume == 0) {
            CLIO_ReportAssertionFailure("vRefNum!=0", "Files.c", 839);
        }
        base.fileData.file.volumeRef = volume;
        base.fileData.file.directoryId = directory == 0 ? 2 : directory;
        base.fileData.file.name[0] = 0;
        error = MacSpecs_MakeOSSpec(&base, &location);
        if (error != 0) {
            result->fileData.file.name[0] = 0;
            return OS_OSErrorToMacError(error);
        }
        fn_00412340(&location.path, fullPath, sizeof(fullPath));
    }
    if (pstrchr(path, ':') != 0) {
        index = 1;
        if (path[1] != ':') {
            while (index <= path[0]) {
                if ((character = path[index]) == ':') {
                    break;
                }
                volumeName[index - 1] = character;
                index++;
                if (index >= volumeNameCapacity) {
                    result->fileData.file.name[0] = 0;
                    return -35;
                }
            }
            volumeName[index - 1] = 0;
            index++;
            if (OS_MakePathSpec(volumeName, NULL, &location.path) == 0) {
                fn_00412340(&location.path, fullPath, sizeof(fullPath));
            } else {
                c2pstrcpy(result->fileData.file.name, volumeName);
                return -35;
            }
        }
        output = fullPath + strlen(fullPath);
        for (; index <= path[0]; index++) {
            if (path[index] == ':') {
                if (index < path[0] && path[index + 1] == ':') {
                    do {
                        output += sprintf(output, "..\\");
                        index++;
                        if (index >= path[0]) {
                            break;
                        }
                    } while (path[index + 1] == ':');
                    continue;
                }
                *output = '\\';
            } else {
                *output = (char)path[index];
            }
            output++;
        }
        *output = 0;
    } else {
        p2cstrcpy(fullPath + strlen(fullPath), path);
    }
    error = make_osspec_from_path(fullPath, &location, NULL);
    if (error == 0) {
        MacSpecs_MakeCWFileSpecFromString(&location, result);
        return OS_OSErrorToMacError(OS_Status(&location));
    }
    result->fileData.file.name[0] = 0;
    return OS_OSErrorToMacError(error);
}
