#ifndef DRIVER_CWPLUGINSPRIVATE_H
#define DRIVER_CWPLUGINSPRIVATE_H

#include "compiler/common.h"
#include "compiler/CPrep.h"
#include "driver/Files.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The CodeWarrior plugin API's context: the request, the shell's 'CWIE' signature, and the shell's callback table
 * (each callback takes the context first). */
#pragma options align = mac68k
struct CWPluginPrivateContext {
    long request;
    long apiVersion;
    void *shellContext;
    void *pluginStorage;
    CWFileSpec sourcefile;
    CWFileSpec targetfile;
    long shellSignature;
    void *contextSignature;
    long numFiles;
    long numOverlayGroups;
    short callbackOSError;
    short value_ae;
    char padB0[4];
    struct CallbackCache *callbackCache;
    char padB8[0x4c];
    void **callbacks;
    union {
        struct CommandLineArguments *environment;
        int fileIndex;
        struct CommandParseInfo *commandParseInfo;
    } requestData;
    union {
        CWFileSpec payload;
        struct {
            SInt32 value2;
            SInt32 value3;
            void *value10;
            void *value11;
            SInt32 value4;
            void *value5;
            SInt32 value6;
            char **value7;
            struct ToolArgumentSet *toolArguments;
            struct ToolArgumentSet *argumentRecord;
        } generation;
        struct {
            int outputFirstValue;
            void *outputSecondValue;
            unsigned int first_value;
            unsigned int second_value;
            int count;
            struct PanelEntry *panels;
            int lookupFirstValue;
            void *lookupSecondValue;
            struct IntegerSequenceResult *entries;
            unsigned char pad130[4];
            void **parserCallbacks;
            unsigned char pad138[0x1a];
        } parser;
    } contextData;
    void *callbackValue;
    unsigned int callbackFlags;
    unsigned char enabled;
    unsigned char active;
    unsigned char operation;
    unsigned char reserved15d;
    unsigned char setting;
    unsigned char reserved15f;
    short dependencyOption;
    struct BrowseOptions dependencyState;
    char dependencyStatusNegative;
    UInt8 pad173[0x31];
    CWFileSpec firstFile;
    CWFileSpec secondFile;
    UInt16 linkage;
    UInt8 firstByte;
    UInt8 secondByte;
    UInt16 pad234;
    UInt32 thirdCode;
    UInt32 fourthCode;
    UInt32 pad23e[2];
    struct TgtRec *targetSettings;
    unsigned char pad24a[0x1c];
    struct ObjectCallbackContext *compilerCallbacks;
};
typedef struct CWPluginPrivateContext *CWPluginContext;
#pragma options align = reset
struct CachedOpcodeMetadata {
    const char *operand_format;
    unsigned int unknown_04;
    const char *mnemonic;
    unsigned int unknown_0c;
    unsigned char operand_count;
    unsigned char unknown_11;
};
struct FileOpenOptions {
    SInt32 fileIndex;
    SInt32 lookupPathIndex;
    SInt32 overlayIndex;
    SInt32 overlayTableIndex;
    SInt32 auxiliaryValue;
    UInt8 flag1;
    UInt8 flag2;
    UInt8 flag3;
};
struct FileProcessingInfo {
    CWFileSpec info;
    short reserved;
};
extern Boolean is_valid_plugin_context(struct CWPluginPrivateContext *context);
extern UInt8 has_entry_signature_and_kind(CWPluginPrivateContext *entry);
extern unsigned char is_valid_context(CWPluginPrivateContext *record);
extern int __stdcall CWPluginsPrivate_ReturnArgument(CWPluginPrivateContext *a0, int a1);
extern unsigned int __stdcall CWPluginsPrivate_GetRequest(CWPluginPrivateContext *entry, long *result);
extern unsigned int __stdcall CWPluginsPrivate_GetAPIVersion(CWPluginPrivateContext *input, long *result);
extern int __stdcall CWPluginsPrivate_GetSourceFile(CWPluginPrivateContext *p, CWFileSpec *q);
extern int __stdcall CWPluginsPrivate_GetOutputFileDirectory(CWPluginPrivateContext *context, CWFileSpec *directory);
extern unsigned int ensure_callback_cache(struct CWPluginPrivateContext *object);
extern unsigned int __stdcall get_opcode_descriptor(CWPluginPrivateContext *input, PCodeOpcodeDescriptor *descriptor);
extern unsigned int __stdcall CWPluginsPrivate_ValidateAndCallCallback(CWPluginPrivateContext *context, int argument2,
                                                                       int argument3, unsigned int argument4,
                                                                       int argument5);
extern unsigned int __stdcall CWPluginsPrivate_InvokeMessageCallback(void *object, struct MessageContext *argument1,
                                                                     char *argument2, char *argument3, short argument4,
                                                                     unsigned int argument5);
extern unsigned int __stdcall fn_0041b8d0(CWPluginPrivateContext *object, char *argument1, void *argument2);
extern int __stdcall fn_0041b910(struct CWPluginPrivateContext *object);
extern unsigned int __stdcall CWPluginsPrivate_CallArgumentValueCallback(CWPluginPrivateContext *object,
                                                                         const char *argument, int *value);
extern SInt32 __stdcall CWPluginsPrivate_OpenFile(CWPluginPrivateContext *state, CWFileSpec *argument, int value,
                                                  FileOpenOptions *argument4, SInt32 *argument5);
extern unsigned int __stdcall CWPluginsPrivate_CallValuePairCallback(CWPluginPrivateContext *object,
                                                                     struct ValuePairState *argument);
extern unsigned int __stdcall CWPluginsPrivate_CallContextArgumentCallback(CWPluginPrivateContext *object,
                                                                           unsigned int argument);
extern unsigned int __stdcall fn_0041bb50(CWPluginPrivateContext *object, unsigned int argument2,
                                          unsigned int argument3, UInt8 **argument4);
extern unsigned int __stdcall fn_0041bbb0(CWPluginPrivateContext *object, unsigned int argument);
extern unsigned int __stdcall CWPluginsPrivate_InvokeCallback40(CWPluginPrivateContext *context, const char *argument2,
                                                                const char *argument3, unsigned int argument4,
                                                                unsigned int argument5, int *argument6);
extern unsigned int __stdcall fn_0041bc50(CWPluginPrivateContext *object, unsigned int argument);
extern unsigned int __stdcall CWPluginsPrivate_GetNumFiles(CWPluginPrivateContext *state, long *count);
extern int __stdcall CWPluginsPrivate_InvokeExportedRecordCallback(CWPluginPrivateContext *context, int index,
                                                                   int argument, struct ExportedRecord *info);
extern unsigned int __stdcall CWPluginsPrivate_CallSignatureCallback(CWPluginPrivateContext *record,
                                                                     const char *argument2, void *argument3);
extern int __stdcall fn_0041b7f0(CWPluginPrivateContext *object, void *argument);
extern int __stdcall CWPluginsPrivate_CallCallback9(void *object, char *argument2, char *argument3,
                                                    unsigned char *argument4, unsigned int argument5);
extern unsigned int __stdcall CWPluginsPrivate_CallFileProcessingCallback(CWPluginPrivateContext *object,
                                                                          struct FileProcessingInfo *value,
                                                                          void **argument3, unsigned int argument4);
extern unsigned int __stdcall fn_0041bab0(CWPluginPrivateContext *object, unsigned int argument1,
                                          unsigned int argument2, unsigned int *argument3);

#ifdef __cplusplus
}
#endif

#endif
