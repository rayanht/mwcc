#ifndef DRIVER_CLDROPINCALLBACKS_V10_H
#define DRIVER_CLDROPINCALLBACKS_V10_H

#include "compiler/common.h"
#include "driver/Files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DropinCallbackData {
    const char *name;
    struct StorageHandle *storage;
    char addToProject;
    UInt8 unk09[19];
    unsigned char payload[12];
    unsigned int kind;
};

struct CallbackCache {
    int count0;
    struct CallbackPathEntry *entries0;
    int count1;
    struct CallbackPathEntry *entries1;
};
struct CallbackOverlayRecord {
    char text[256];
    unsigned int values[2];
    int result;
};
struct CallbackPathEntry {
    CWFileSpec file;
    char hasChildren;
    char alignmentPadding[1];
    int childCount;
    CWFileSpec *childFiles;
};
struct CallbackRecord {
    char name[32];
    unsigned short value;
};
struct DiagnosticContext {
    char reserved0[8];
    struct DiagnosticTarget *target;
    char reserved12[256];
    char sourceData;
};
#pragma options align = mac68k
struct DiagnosticLocation {
    CWFileSpec file;
    int line;
    short column;
    short length;
    int selectionOffset;
    short selectionLength;
};
#pragma options align = reset
#pragma options align = mac68k
struct DiagnosticTarget {
    struct Plugin *identifier;
};
#pragma options align = reset
#pragma pack(push, 1)
struct DropinFileCallback {
    char enableDependencyLookup;
    signed char searchOption;
    long fileKey;
    char suppressFileReferenceLookup;
    char unusedByte[1];
    char *fileReference;
    unsigned int referenceValue;
    short referenceKind;
    short lookupResult;
    CWFileSpec output;
    char callbackState;
    char lookupFailed;
};
#pragma pack(pop)
struct DropinLookupResult {
    short code;
};
struct DropinRequest {
    unsigned int header[2];
    struct DropinRequestData *data;
    UInt8 unk0c[148];
    int signature;
    SInt32 addedFileCount;
    UInt8 unka8[96];
    int fileKey;
};
#pragma pack(push, 1)
struct DropinRequestData {
    struct Plugin *reference;
};
#pragma pack(pop)
struct DropinResultStorage {
    char name[256];
    unsigned int value;
};
#pragma options align = mac68k
struct ExportedRecord {
    CWFileSpec fileReference;
    int convertedValue;
    short shortValue;
    char optionB, optionC, optionD, optionE, optionF, optionA;
    int secondTimestamp;
    char string;
    char reservedString[30];
    char stringEnd;
    short code;
    char flag, optionH;
    int fileType;
    int secondValue;
    char optionI, optionG;
    int finalValue;
};
#pragma options align = reset
#pragma pack(push, 1)
struct PackedConversionResult {
    int value;
    char reserved[2];
};
#pragma pack(pop)
#pragma options align = mac68k
struct StoredRecord {
    char reservedPrefix[8];
    short shortValue;
    char reservedAlignment[2];
    long long firstWideValue;
    long long secondWideValue;
    char reservedBeforeText[514];
    char text;
    char reservedAfterText[653];
    int stringHandle;
    char reservedBeforeOptions[45];
    char optionA, optionB, optionC, optionD, optionE, optionF, optionG;
    char reservedBeforeFlag[16];
    char flag;
    char reservedBeforeCode[1];
    short code;
    char reservedBeforeValues[32];
    int firstValue, secondValue;
};
#pragma options align = reset
extern int __stdcall UCBGetFileInfo(int unused, int key, int unusedFlags, struct ExportedRecord *result);
extern Boolean lookup_file(DropinRequest *unused, char *key, DropinFileCallback *output, OSSpec *argument,
                           Boolean *found);
extern Boolean lookup_dependency_file(DropinRequest *state, char *request, DropinFileCallback *flags, OSSpec *text,
                                      Boolean *arg5);
extern Boolean insert_dependency_from_path(DropinRequest *descriptor, char *argument, DropinFileCallback *state,
                                           OSSpec *context, Boolean *changed);
extern SInt32 __stdcall UCBFindAndLoadFile(DropinRequest *dropin, char *inputPath, DropinFileCallback *parameters);
extern int __stdcall UCBGetFileText(void *context, CWFileSpec *file, void **result1, unsigned int *result2,
                                    SInt16 *status);
extern int __stdcall UCBReportMessage(struct DiagnosticContext *context, struct DiagnosticLocation *location,
                                      char *message, char *detail, short kind, int argument);
extern int __stdcall UCBAlert(DropinContext *ctx, char *message1, char *message2, char *message3, char *message4);
extern int __stdcall UCBSetModDate(int callbackContext, char *name, SInt32 *modificationDate, int reserved);
extern __stdcall SInt32 UCBAddProjectEntry(DropinRequest *context, CWFileSpec *file, UInt8 flag,
                                           struct FileOpenOptions *args, UInt32 *objectId);
extern int __stdcall UCBCreateNewTextDocument(DropinRequest *request, struct DropinCallbackData *descriptor);
extern unsigned int __stdcall UCBResolveRelativePath(unsigned int clientContext, unsigned int basePath,
                                                     unsigned int relativePath, unsigned int resolvedPath);
extern unsigned int __stdcall UCBStorePluginData(unsigned int arg0, unsigned int arg1, unsigned int arg2,
                                                 unsigned int arg3);
extern unsigned int __stdcall UCBGetPluginData(unsigned int context, unsigned int plugin, unsigned int data,
                                               unsigned int size);
extern unsigned int __stdcall UCBFreeMemory(unsigned int unused1, unsigned int callbackArgument, unsigned int unused2);
extern unsigned int __stdcall UCBAllocMemHandle(unsigned int unused, unsigned int size, unsigned int reserved,
                                                void *result);
extern unsigned int __stdcall UCBUnlockMemHandle(unsigned int unused, unsigned int value);
extern unsigned int __stdcall UCBPreDialog(unsigned int argument);
extern unsigned int __stdcall UCBPostDialog(unsigned int argument);
extern unsigned int __stdcall UCBPreFileAction(unsigned int action, unsigned int fileReference);
extern unsigned int __stdcall UCBPostFileAction(unsigned int file, unsigned int action);
extern unsigned int __stdcall UCBSecretAttachHandle(unsigned int unused, unsigned int value, void *resultAddress);
extern unsigned int __stdcall UCBSecretDetachHandle(unsigned int unused, unsigned int value, void *resultAddress);
extern unsigned int __stdcall UCBSecretPeekHandle(unsigned int unused, unsigned int value, unsigned int *result);
extern unsigned int __stdcall UCBCheckinLicense(unsigned int unused, unsigned int value);
extern unsigned int __stdcall UCBReleaseFileText(struct DropinRequest *request, void *memory);
extern unsigned int __stdcall UCBAllocateMemory(unsigned int unused0, unsigned int value, unsigned int unused2,
                                                unsigned int *result);
extern unsigned int __stdcall UCBGetTargetName(unsigned int unused, unsigned int value, unsigned int kind);
extern unsigned int __stdcall UCBLockMemHandle(unsigned int unused0, struct StorageHandle *args, unsigned int unused2,
                                               char **destination);
extern unsigned int __stdcall UCBUserBreak(unsigned int argument);
extern unsigned int __stdcall UCBGetOverlay1GroupInfo(unsigned int callback, int index,
                                                      struct CallbackOverlayRecord *record);
extern unsigned int __stdcall UCBGetOverlay1Info(unsigned int unused, unsigned int arg1, unsigned int arg2,
                                                 struct DropinResultStorage *result);
extern unsigned int __stdcall UCBFreeMemHandle(unsigned int callback, int callbackIndex);
extern int __stdcall UCBGetMemHandleSize(unsigned int argument1, unsigned int argument2, unsigned int *result);
extern unsigned int __stdcall UCBGetOverlay1FileInfo(unsigned int unused, unsigned int arg1, unsigned int arg2,
                                                     unsigned int arg3, unsigned int *result);
extern unsigned int __stdcall UCBGetNamedPreferences(unsigned int context, char *name, unsigned int *result);
extern unsigned int __stdcall UCBResizeMemHandle(unsigned int callback, int argument, unsigned int size);
extern unsigned int __stdcall UCBCheckoutLicense(unsigned int unused0, unsigned int arg1, unsigned int arg2,
                                                 unsigned int flags, unsigned int unused4, unsigned int *result);
extern int __stdcall UCBCacheAccessPathList(CWPluginPrivateContext *request);
extern unsigned int __stdcall UCBGetSegmentInfo(unsigned int unused, unsigned int key, CallbackRecord *record);
extern unsigned int __stdcall UCBShowStatus(CWPluginPrivateContext *callback, char *message, char *detail);

#ifdef __cplusplus
}
#endif

#endif
