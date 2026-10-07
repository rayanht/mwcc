#ifndef DRIVER_CLPLUGINREQUESTS_H
#define DRIVER_CLPLUGINREQUESTS_H

#include <setjmp.h>
#include "compiler/common.h"
#include "driver/Files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PluginOutputItem {
    struct Plugin *owner;
    UInt8 initializationFlagCopy;
    UInt8 initializationFlag;
};
#pragma options align = mac68k
struct TgtHead {
    UInt16 tag;
    CWFileSpec firstFile;
    CWFileSpec secondFile;
    CWFileSpec thirdFile;
    SInt16 linkage;
    UInt8 firstByte;
    UInt8 secondByte;
    UInt32 platformCodes[4];
    UInt32 thirdCode;
    UInt32 fourthCode;
};
#pragma options align = reset
#pragma options align = mac68k
struct TgtRec {
    TgtHead head;
    char trailingData[0x46];
};
#pragma options align = reset
#pragma options align = mac68k
struct ToolArgumentSet {
    int count;
    char **arguments;
    char **additional_arguments;
};
#pragma options align = reset
extern Boolean CLPluginRequests_InitializeTargetSettings(CLTarget *input, Plugin *plugin, UInt32 flags);
extern UInt8 CLPluginRequests_CallPluginForFile(Plugin *plugin, DropinFileRecord *context);
extern void CLPluginRequests_AppendMacFileTypesTable(struct MacFileTypeNode **firstArgument, SInt32 secondArgument);
extern void initialize_plugin_request(Plugin *owner, int phase);
extern Boolean CLPluginRequests_ParseCommandLine(Plugin *func, struct CLTarget *context,
                                                 struct CommandParseInfo *value1, SInt32 value2, SInt32 value3,
                                                 SInt32 value4, void *value5, SInt32 value6, char **value7,
                                                 struct ToolArgumentSet *value8, struct ToolArgumentSet *value9,
                                                 void *value10, void *value11);
extern Boolean CLPluginRequests_SetupFileRequest(Plugin *job, DropinFileRecord *input, short flags);
extern Boolean CLPluginRequests_UpdateTargetSettings(Plugin *record, UInt32 flags, struct TgtRec *snapshot);
extern int fn_00417440(Plugin *obj, Boolean flag);

#ifdef __cplusplus
}
#endif

#endif
