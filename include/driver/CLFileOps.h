#ifndef DRIVER_CLFILEOPS_H
#define DRIVER_CLFILEOPS_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/win32.h"
#include "driver/CLDependencies.h"
#include "driver/CLFiles.h"
#include "driver/Files.h"
#include "driver/MsDos.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct DropinFileRecord {
    struct IndexedListLink listEntry;
    SInt16 lookupPathIndex;
    UInt8 pad0A[2];
    _FILETIME sourceFileTime;
    _FILETIME callbackFileTime;
    char inputName[256];
    char outputName[256];
    short kind;
    struct OSSpec inputPath;
    struct OSSpec outputPath;
    short outputMask;
    unsigned short validatedOutputMask;
    short temporaryOutputMask;
    struct Plugin *selectedPlugin;
    int compilerCapabilities;
    int compilerFlags;
    int fileFlags;
    short inputArgumentMask;
    short outputArgumentMask;
    struct StorageHandle *outputStorage;
    struct StorageHandle *objectData;
    struct StorageHandle *secondaryReferenceHandle;
    SInt32 codeSize;
    SInt32 bssSize;
    SInt32 dataSize;
    SInt32 reportedTotal;
    UInt8 configurationOptionEnabled;
    UInt8 pad4dd;
    UInt8 configurationReceivedWithoutCapability;
    char configurationReceivedWithCapability;
    UInt8 requiresLink;
    UInt8 fileOpenFlag2;
    UInt8 fileOpenFlag3;
    UInt8 fileOpenFlag1;
    struct DependencyCollection dependencies;
    UInt8 dependencyStatusNegative;
    UInt8 reserved4f5;
    SInt16 dependencyOption;
    UInt32 dependencyState[8];
    SInt32 fileType;
};
#pragma options align = reset
#pragma pack(push, 1)
struct NamespaceOperationContext {
    UInt8 reserved00[8];
    UInt8 *status;
    UInt8 reserved0c[0xa0];
    SInt16 errorCode;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct NamespaceOperationState {
    CWFileSpec file;
    SInt32 index;
    UInt8 flag4a;
    UInt8 flag4b;
};
#pragma pack(pop)
extern void set_bytes(unsigned int byteCount, int *value, char **unitName);
extern int compile_file(DropinFileRecord *file, char *processed);
extern int CLFileOps_CompileProject(void);
extern int CLFileOps_LinkProject(void);
extern struct CLTarget *default_target;
extern int __stdcall add_access_path(NamespaceOperationContext *context, NamespaceOperationState *state);
extern unsigned int __stdcall add_or_copy_pref_panel_storage(unsigned int unused, char *name, StorageHandle *data);
extern int __stdcall set_file_output_name_and_kind(int unused, int index, short mode, char *name);
extern int __stdcall set_output_directory(CWPluginPrivateContext *record, short *input);
extern int __stdcall add_overlay_group(CWPluginPrivateContext *object, char *name, CLOverlayValues *value,
                                       unsigned int *result);
extern __stdcall int append_file_to_overlay(int unused, const char *fileID, int overlayID, unsigned int *result);
extern int __stdcall lookup_path_index(unsigned int context, char *name, UInt16 nameLength, unsigned int *index);
extern unsigned int __stdcall set_lookup_paths_name_and_value(unsigned int context, unsigned short recordId, char *text,
                                                              unsigned short value);
extern unsigned int CLFileOps_GetScaledTicks(void);
extern unsigned short fn_00419620(void);
extern SInt32 dispatch_output_storage_by_mask(DropinFileRecord *state, SInt16 mask, SInt32 argument1, SInt32 argument2);
extern int CLFileOps_SetupOutputPath(DropinFileRecord *obj, SInt16 mask);
extern unsigned int setup_file_request(DropinFileRecord *context);
extern int setup_preprocessing_output(DropinFileRecord *st);
extern unsigned int fn_00419c90(DropinFileRecord *state, unsigned int mode);
extern unsigned int fn_00419d80(DropinFileRecord *input);
extern unsigned int write_object_file(DropinFileRecord *record);
extern unsigned int fn_00419e90(DropinFileRecord *state);
extern int setup_compile_file_request(DropinFileRecord *file);
extern unsigned int execute_tool_with_output_path(DropinFileRecord *record, Plugin *arg1, unsigned int arg2);
extern int disassemble_file(DropinFileRecord *request);
extern unsigned int (*data_0054bf48)(char *);

#ifdef __cplusplus
}
#endif

#endif
