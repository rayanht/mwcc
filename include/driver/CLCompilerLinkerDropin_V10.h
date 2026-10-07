#ifndef DRIVER_CLCOMPILERLINKERDROPIN_V10_H
#define DRIVER_CLCOMPILERLINKERDROPIN_V10_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct DropinConfiguration {
    struct StorageHandle *primaryReference;
    struct StorageHandle *secondaryReference;
    SInt32 fileType;
    UInt32 values[4];
    UInt8 flag;
    UInt8 reserved1;
    char *path;
    UInt8 reserved2[8];
    UInt16 option;
    CWFileSpec *outputFileSpec;
};
#pragma options align = reset
struct DropinContext {
    char reserved[8];
    struct DropinDiagnosticTarget *record;
};
struct DropinDiagnosticTarget {
    struct Plugin *diagnosticType;
};
#pragma options align = mac68k
struct DropinFileValue {
    int value;
};
#pragma options align = reset
#pragma options align = mac68k
struct DropinResultSlot {
    int value;
};
#pragma options align = reset
#pragma pack(push, 1)
struct InitializationAuxiliaryState {
    UInt8 opaquePrefix[0xd8];
    SInt32 defaultValue1;
    SInt32 defaultValue2;
    UInt8 opaqueSuffix[0x136 - 0xe0];
};
#pragma pack(pop)
extern int __stdcall UCBCachePrecompiledHeader(unsigned int context, short *callback, int argument);
extern unsigned int __stdcall UCBLoadObjectData(unsigned int argument, unsigned int key, void *extra);
extern int __stdcall UCBStoreObjectData(int compiler, int dropinId, DropinConfiguration *configuration);
extern unsigned int __stdcall UCBBeginSubCompile(unsigned int argument0, unsigned int argument1,
                                                 unsigned int argument2);
extern unsigned int __stdcall UCBEndSubCompile(unsigned int unused);
extern int __stdcall UCBGetPrecompiledHeaderSpec(DropinRequest *request, int output, const char *path);
extern unsigned int __stdcall UCBGetResourceFile(unsigned int unused1, unsigned int unused2);
extern unsigned int __stdcall UCBPutResourceFile(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3);
extern int __stdcall UCBLookUpUnit(struct DropinRequest *request, char *inputName, char mode, void **outputObject,
                                   struct DropinResultSlot *outputValue);
extern unsigned int __stdcall UCBSBMfiles(unsigned int unused1, unsigned int unused2);
extern int __stdcall UCBStoreUnit(void *obj, char *filename, int arg3, int arg4);
extern unsigned int __stdcall UCBReleaseUnit(unsigned int unused, unsigned int value);
extern int __stdcall UCBOSAlert(DropinContext *context, const char *text, short code);
extern int __stdcall UCBOSErrorMessage(struct DropinRequest *request, const char *message, short code);
extern unsigned int __stdcall UCBGetStoredObjectFileSpec(unsigned int unused, unsigned int key, unsigned int output);
extern unsigned int __stdcall UCBGetModifiedFiles(unsigned int unused, NameSpaceName *name, unsigned int unused2);
extern int __stdcall UCBDisplayLines(unsigned int a0, unsigned int a1);
extern unsigned int __stdcall UCBGetSuggestedObjectFileSpec(unsigned int a0, unsigned int a1, unsigned int a2);
extern unsigned int __stdcall UCBUnitNameToFileName(unsigned int unused, const char *name, char *output);
extern unsigned int fn_00426320(OSSpec *destination, DropinFileRecord *record);
extern unsigned int __stdcall UCBFreeObjectData(unsigned int unused0, unsigned int recordKey, unsigned int unused2);

#ifdef __cplusplus
}
#endif

#endif
