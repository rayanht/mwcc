#ifndef DRIVER_FILES_H
#define DRIVER_FILES_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct FSSpec {
    SInt16 vRefNum;
    SInt32 parID;
    Str63 name;
};
#pragma options align = reset
struct FileIdentifierInfo {
    UInt32 type;
    UInt32 creator;
    UInt16 flags;
    UInt16 reserved1;
    UInt16 reserved2;
    UInt16 reserved3;
};
struct FileQueryInfo {
    UInt32 firstTimestamp;
    UInt32 secondTimestamp;
    UInt32 dataSize;
    UInt32 dataAllocatedSize;
    UInt32 auxiliarySize;
    UInt32 auxiliaryAllocatedSize;
    UInt32 dataReserved;
    UInt32 auxiliaryReserved;
    UInt8 attributes;
};
#pragma options align = mac68k
struct RecordData {
    UInt32 words[4];
};
#pragma options align = reset
#pragma pack(push, 1)
struct RecordQuery {
    UInt8 reserved[0x10];
    SInt16 status;
    UInt8 *name;
    UInt16 kind;
    UInt16 reserved18;
    UInt16 options;
    SInt16 flags;
    UInt8 attributes;
    UInt8 reservedByte;
    RecordData identifier;
    int value;
    UInt16 queryFlags;
    UInt32 queryValue2;
    UInt32 queryValue3;
    UInt16 reservedWord;
    UInt32 queryValue4;
    UInt32 queryValue5;
    UInt32 queryValue0;
    UInt32 result;
};
#pragma pack(pop)
extern int get_file_query_info_and_identifiers(OSSpec *path, FileQueryInfo *info, FileIdentifierInfo *identifiers);
extern int set_file_and_resource_fork_times(OSSpec *path, DWORD *timeValues, DWORD *metadataValues);
extern int __stdcall Files_DeleteFileFromPath(unsigned short kind, int value, UInt8 *data);
extern short __stdcall Files_OpenFile(CWFileSpec *file, char mode, short *result);
extern DWORD __stdcall Files_DeleteFile(CWFileSpec *input);
extern int __stdcall get_file_identifier_info(CWFileSpec *input, FileIdentifierInfo *result);
extern void __stdcall set_record_identifier(CWFileSpec *input, RecordData *data);
extern DWORD __stdcall Files_Tell(unsigned short identifier, long *result);
extern int __stdcall Files_CallWithFileSpecFromPath(short operation, int input, UInt8 *data, unsigned int argument4,
                                                    unsigned int argument5);
extern short __stdcall Files_OpenFileFromPath(int input, int options, unsigned char *data, char mode, short *output);
extern short __stdcall Files_OpenFileByPath(unsigned short volumeRef, unsigned int directoryId, UInt8 *name, char mode,
                                            short *handle);
extern SInt16 __stdcall Files_GetFileIdentifierInfoFromPath(SInt16 input, SInt32 selector, unsigned char *options,
                                                            FileIdentifierInfo *destination);
extern unsigned int __stdcall Files_CreateFile(CWFileSpec *input, unsigned int secondArgument,
                                               unsigned int thirdArgument, int fourthArgument);
extern SInt16 __stdcall Files_Read(SInt16 a0, SInt32 *a1, void *a2);
extern DWORD __stdcall Files_Close(short handleIndex);
extern short __stdcall Files_GetSize(short predecessor, SInt32 *result);
extern SInt16 __stdcall Files_SetSize(SInt16 handleId, SInt32 size);
extern int __stdcall Files_SetPosition(short refNum, short posMode, SInt32 posOff);
extern short __stdcall Files_UpdateRecordQuery(RecordQuery *record);
extern SInt16 __stdcall Files_Write(SInt16 a0, SInt32 *a1, void *a2);
extern UInt16 __stdcall fn_00414710(RecordQuery *record);
extern int __stdcall Files_MakeFileSpecFromPath(short volume, int directory, unsigned char *path, CWFileSpec *result);

#ifdef __cplusplus
}
#endif

#endif
