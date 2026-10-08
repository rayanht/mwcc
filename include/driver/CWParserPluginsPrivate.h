#ifndef DRIVER_CWPARSERPLUGINSPRIVATE_H
#define DRIVER_CWPARSERPLUGINSPRIVATE_H

#include "compiler/common.h"
#include "driver/Files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CommandLineArguments {
    int argc;
    char **argv;
};
#pragma pack(push, 1)
struct FileOperationInfo {
    union {
        CWFileSpec fileReference;
        struct {
            char reserved[70];
            int value;
        } selection;
    } file;
    char option_a;
    char useSecondValue;
};
#pragma pack(pop)
struct IntegerSequenceResult {
    unsigned int count;
    int *entries;
    unsigned int value;
};
#pragma options align = mac68k
struct PanelEntry {
    UInt32 type;
    UInt32 creator;
    UInt32 flags;
    UInt32 version;
    UInt8 enabled;
    UInt8 alignmentPadding;
};
#pragma options align = reset
struct ValuePairState {
    char *firstValue;
    unsigned int secondValue;
    unsigned char flag;
};
extern CWPluginPrivateContext *validate_parser_context(CWPluginPrivateContext *header);
extern unsigned int __stdcall CWParserPluginsPrivate_GetParserValues(CWPluginPrivateContext *query,
                                                                     unsigned int *first_value,
                                                                     unsigned int *second_value);
extern int __stdcall CWParserPluginsPrivate_GetEnvironment(void *input, CommandLineArguments **value_out);
extern int __stdcall CWParserPluginsPrivate_GetOutputValues(struct CWPluginPrivateContext *context,
                                                            unsigned char *firstValue, unsigned char *secondValue);
extern int __stdcall CWParserPluginsPrivate_GetPanels(void *context, int *firstValue, struct PanelEntry **secondValue);
extern int __stdcall CWParserPluginsPrivate_GetLookupValues(void *context, int *firstValue, char ***secondValue);
extern int __stdcall CWParserPluginsPrivate_SetParserEntry(void *ctx, int index, IntegerSequenceResult *value);
extern int __stdcall fn_0041bfd0(void *context, char *name, void **value);
extern unsigned int __stdcall CWParserPluginsPrivate_CallFileOperationCallback(CWPluginPrivateContext *objectId,
                                                                               FileOperationInfo *argument);
extern int __stdcall CWParserPluginsPrivate_CallFileInfo(CWPluginPrivateContext *ctx, CWFileSpec *info);
extern int __stdcall CWParserPluginsPrivate_CallParserTextCallback(CWPluginPrivateContext *request, int argument2,
                                                                   int argument3, char *text);
extern int __stdcall CWParserPluginsPrivate_PassValuePair(CWPluginPrivateContext *target, char *firstValue,
                                                          unsigned int secondValue);
extern void __stdcall CWParserPluginsPrivate_CallValuePairCallback(CWPluginPrivateContext *target, char *firstValue,
                                                                   unsigned int secondValue);
extern int __stdcall CWParserPluginsPrivate_AddOverlay1Group(CWPluginPrivateContext *context, char *name, void *address,
                                                             SInt32 *groupNumber);
extern int __stdcall CWParserPluginsPrivate_AddOverlay1(CWPluginPrivateContext *context, char *name, SInt32 groupNumber,
                                                        SInt32 *overlayNumber);
extern int __stdcall CWParserPluginsPrivate_AddSegment(CWPluginPrivateContext *context, char *name, short attributes,
                                                       SInt32 *segmentNumber);
extern int __stdcall CWParserPluginsPrivate_SetSegment(CWPluginPrivateContext *context, SInt32 segmentNumber,
                                                       char *name, short attributes);

#ifdef __cplusplus
}
#endif

#endif
