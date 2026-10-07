#ifndef DRIVER_CLDROPINCALLBACKS_V10_H
#define DRIVER_CLDROPINCALLBACKS_V10_H

#include "compiler/common.h"
#include "driver/Files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DropinCallbackData {
    const char *name; /* 0x00: create_new_text_document selects stdout or names a project entry */
    struct StorageHandle
        *storage; /* 0x04: create_new_text_document; CLDropinCallbacks_V10_SetStorageHandle forwards this handle */
    char
        addToProject; /* 0x08: create_new_text_document creates and appends a chain record and calls add_project_entry when set */
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
    char alignmentPadding[1]; /* 0x47: count_access_paths_recursive */
    int childCount;
    CWFileSpec *childFiles;
};
struct CallbackRecord {
    char name[32];        /* 0x00: lookup_callback_record copies the segment name from CLSegs_GetValue. */
    unsigned short value; /* 0x20: lookup_callback_record copies found->value from CLSegs_GetValue. */
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
    int selectionOffset;   /* 0x4E: report_message copies to record.selectionOffset */
    short selectionLength; /* 0x52: report_message copies to record.selectionLength */
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
    char unusedByte
        [1]; /* 0x07: lookup_file and lookup_dependency_file leave this byte unused between suppressFileReferenceLookup and fileReference. */
    char *
        fileReference; /* 0x08: lookup_file copies file bytes; insert_dependency_from_path sets xstrdup(""); UCBLookUpUnit interprets precompiled bytes when referenceKind == 2. */
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
    int fileType;    /* 0x7a: ParserHelpers-cc.c reads the file-info callback result and tests 'TEXT'. */
    int secondValue; /* 0x7e: get_file_info exports the second value. */
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
extern int __stdcall get_file_info(int unused, int key, int unusedFlags, struct ExportedRecord *result);
extern Boolean lookup_file(DropinRequest *unused, char *key, DropinFileCallback *output, OSSpec *argument,
                           Boolean *found);
extern Boolean lookup_dependency_file(DropinRequest *state, char *request, DropinFileCallback *flags, OSSpec *text,
                                      Boolean *arg5);
extern Boolean insert_dependency_from_path(DropinRequest *descriptor, char *argument, DropinFileCallback *state,
                                           OSSpec *context, Boolean *changed);
extern SInt32 __stdcall CLDropinCallbacks_V10_FindAndLoadFile(DropinRequest *dropin, char *inputPath,
                                                              DropinFileCallback *parameters);
extern int __stdcall CLDropinCallbacks_V10_GetFileText(void *context, CWFileSpec *file, void **result1,
                                                       unsigned int *result2, SInt16 *status);
extern int __stdcall report_message(struct DiagnosticContext *context, struct DiagnosticLocation *location,
                                    char *message, char *detail, short kind, int argument);
extern int __stdcall emit_alert_messages(DropinContext *ctx, char *message1, char *message2, char *message3,
                                         char *message4);
extern int __stdcall set_mod_date(int a1, char *name, SInt32 *tp, int a4);
extern __stdcall SInt32 add_project_entry(DropinRequest *context, CWFileSpec *file, UInt8 flag,
                                          struct FileOpenOptions *args, UInt32 *objectId);
extern int __stdcall create_new_text_document(DropinRequest *request, struct DropinCallbackData *descriptor);
extern unsigned int __stdcall fn_00425a00(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3);
extern unsigned int __stdcall report_store_plugin_data_not_implemented(unsigned int arg0, unsigned int arg1,
                                                                       unsigned int arg2, unsigned int arg3);
extern unsigned int __stdcall fn_00424660(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3);
extern unsigned int __stdcall free_callback_argument(unsigned int unused1, unsigned int callbackArgument,
                                                     unsigned int unused2);
extern unsigned int __stdcall fn_004252a0(unsigned int a0, unsigned int a1, unsigned int a2, void *a3);
extern unsigned int __stdcall fn_00425430(unsigned int unused, unsigned int value);
extern unsigned int __stdcall log_callback_string(unsigned int argument);
extern unsigned int __stdcall log_callback_above_threshold(unsigned int argument);
extern unsigned int __stdcall fn_004254e0(unsigned int a0, unsigned int a1);
extern unsigned int __stdcall fn_00425520(unsigned int a0, unsigned int a1);
extern unsigned int __stdcall CLDropinCallbacks_V10_StoreValue(unsigned int unused, unsigned int value,
                                                               void *resultAddress);
extern unsigned int __stdcall CLDropinCallbacks_V10_SetStorageHandle(unsigned int unused, unsigned int value,
                                                                     void *resultAddress);
extern unsigned int __stdcall copy_value_to_result(unsigned int unused, unsigned int value, unsigned int *result);
extern unsigned int __stdcall forward_nonzero_value(unsigned int unused, unsigned int value);
extern unsigned int __stdcall CLDropinCallbacks_V10_FreeMemory(struct DropinRequest *a0, void *a1);
extern unsigned int __stdcall allocate_memory(unsigned int unused0, unsigned int value, unsigned int unused2,
                                              unsigned int *result);
extern unsigned int __stdcall copy_command_line_target(unsigned int unused, unsigned int value, unsigned int kind);
extern unsigned int __stdcall get_storage_handle_data(unsigned int unused0, struct StorageHandle *args,
                                                      unsigned int unused2, char **destination);
extern unsigned int __stdcall fn_00424540(unsigned int argument);
extern unsigned int __stdcall get_overlay_group_info(unsigned int callback, int index,
                                                     struct CallbackOverlayRecord *record);
extern unsigned int __stdcall lookup_overlay_allocation(unsigned int unused, unsigned int arg1, unsigned int arg2,
                                                        struct DropinResultStorage *result);
extern unsigned int __stdcall fn_004252f0(unsigned int callback, int callbackIndex);
extern int __stdcall fn_00425340(unsigned int argument1, unsigned int argument2, unsigned int *result);
extern unsigned int __stdcall call_overlays_and_translate_status(unsigned int unused, unsigned int arg1,
                                                                 unsigned int arg2, unsigned int arg3,
                                                                 unsigned int *result);
extern unsigned int __stdcall copy_named_destination_to_temporary(unsigned int context, char *name,
                                                                  unsigned int *result);
extern unsigned int __stdcall resize_mem_handle(unsigned int callback, int argument, unsigned int size);
extern unsigned int __stdcall request_license(unsigned int unused0, unsigned int arg1, unsigned int arg2,
                                              unsigned int flags, unsigned int unused4, unsigned int *result);
extern int __stdcall cache_access_path_list(CWPluginPrivateContext *request);
extern unsigned int __stdcall lookup_callback_record(unsigned int unused, unsigned int key, CallbackRecord *record);
extern unsigned int __stdcall report_message_detail(CWPluginPrivateContext *callback, char *message, char *detail);
extern short DAT_00541b28;
extern char data_00541b42;
extern UInt8 data_00541c08;
extern UInt8 data_00541d0c;
extern char data_00541d0d;
extern UInt8 data_00541d0e;
extern char data_00541c0a;
extern SInt32 data_005871d0;
extern MemBuffer data_00587570;
extern struct MessageRecord data_00541b1c;

#ifdef __cplusplus
}
#endif

#endif
