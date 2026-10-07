#ifndef DRIVER_CLPLUGINS_H
#define DRIVER_CLPLUGINS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct FileMap {
    UInt32 type;
    char ext[0x20];
    unsigned int value;
};
#pragma options align = reset
#pragma options align = mac68k
struct FileMapInfo {
    UInt16 unk0;
    SInt16 count;
    struct FileMap *maps;
};
#pragma options align = reset
#pragma options align = mac68k
struct PlugAux {
    short(__stdcall *getTargetInfo)(unsigned char **);
    short(__stdcall *getFileMap)(struct FileMapInfo **);
    UInt8 pad[0x08];
    SInt16(__stdcall *getObjectFlags)(const CWObjectFlags **);
    short(__stdcall *writeObjectFile)(CWFileSpec *, CWFileSpec *, unsigned int, int, int);
};
#pragma options align = reset
struct Plugin {
    struct PluginDataCallbacks *callbacks;
    struct PlugAux *cl_cb;
    struct PluginQueryTable *pr_cb;
    struct CWPluginPrivateContext *object;
    struct Plugin *next;
};
#pragma pack(push, 1)
struct PluginDataCallbacks {
    unsigned short(__stdcall *entry)(unsigned int);
    short(__stdcall *getData)(unsigned int **data, unsigned int *size);
    unsigned int unknown08;
    short(__stdcall *getName)(char **);
    unsigned short(__stdcall *getDirectoryList)(struct PluginDirectoryList **);
    unsigned char unknown14[8];
    short(__stdcall *getResult)(void **result);
};
#pragma pack(pop)
#pragma options align = mac68k
struct PluginDesc {
    SInt16 descriptorVersion;
    UInt32 type;
    UInt16 api1;
    UInt32 flags;
    SInt32 lang;
    UInt16 api2;
};
#pragma options align = reset
#pragma pack(push, 1)
struct PluginDirectoryList {
    unsigned short value;
    short count;
    char **panels;
};
#pragma pack(pop)
struct PluginOptionalData {
    unsigned char bytes[24];
};
struct PluginQueryTable {
    unsigned int unknownEntry;
    short(__stdcall *query)(unsigned int argument, char **kind, UInt8 *result);
};
#pragma options align = mac68k
struct PluginRequest {
    union {
        int signed_kind;
        unsigned int kind;
    } tag;
    int secondCode;
    int flags;
    unsigned int name;
    char flag_10;
    char reserved;
};
#pragma options align = reset
#pragma options align = mac68k
struct CWObjectFlags {
    SInt16 version;
    UInt32 flags;
    const char *objFileExt;
    const char *brsFileExt;
    const char *ppFileExt;
    const char *disFileExt;
    const char *depFileExt;
    const char *pchFileExt;
    UInt32 objFileCreator;
    UInt32 objFileType;
    UInt32 brsFileCreator;
    UInt32 brsFileType;
    UInt32 ppFileCreator;
    UInt32 ppFileType;
    UInt32 disFileCreator;
    UInt32 disFileType;
    UInt32 depFileCreator;
    UInt32 depFileType;
};
#pragma options align = reset
#pragma options align = mac68k
struct TargetInfo {
    UInt16 unk0;
    SInt16 ncpu;
    UInt32 *cpus;
    SInt16 nos;
    UInt32 *oses;
};
#pragma options align = reset
extern void *get_plugin_directory_list(Plugin *plugin);
extern const CWObjectFlags *CLPlugins_GetObjectFlags(Plugin *obj);
extern Boolean call_query_callback(Plugin *p, PluginRequest *a, SInt32 b, SInt32 c);
extern Boolean fn_004098a0(Plugin *input);
extern void fn_004098d0(void);
extern char plugin_file_map_matches(Plugin *entry, int value, char *text, char option);
extern void free_plugin(Plugin *allocations);
extern void CLPlugins_FreePlugins(void);
extern int CLPlugins_AddPlugin(void *param);
extern Plugin *CLPlugins_FindLinkerPlugin(Plugin *start, int kind, int variant);
extern Boolean plugin_name_matches(Plugin *plugin, char *name);
extern unsigned int CLPlugins_GetType(Plugin *type);
extern Boolean matches_plugin_type_lang(Plugin *entry, int firstValue, int secondValue, int flag);
extern Plugin *CLPlugins_FindMatchingLinkPlugin(Plugin *plugins, int kind, int selector);
extern UInt8 *get_plugin_result(Plugin *plugin);
extern Plugin *CLPlugins_FindTargetPluginBySelectorOptionName(Plugin *first, int selector, int optionKind,
                                                              int optionValue, int nameKind, char *name,
                                                              int selectorValue);
extern char *CLPlugins_GetName(Plugin *plugin);
extern PluginDesc *CLPlugins_GetPluginDesc(Plugin *provider);
extern TargetInfo *get_target_info(Plugin *entry);
extern UInt8 query_plugin(Plugin *plugin, unsigned int queryArgument, char **queryKind);
extern char *format_plugin_version(Plugin *plugin, char *buffer);
extern Boolean fn_00409700(Plugin *plugin);
extern UInt8 CLPlugins_FindFileMapValue(Plugin *plugin, int mode, char *name, unsigned int *result);
extern struct Plugin *CLPlugins_CreatePluginDataCopy(PluginRequiredInputRecord *record36, PluginOptionalData *record24,
                                                     PluginQueryTable *record8);
extern char CLPlugins_MatchTarget(Plugin *plugin, int firstIdentifier, int secondIdentifier, int flag);
extern char file_map_matches(FileMap *reference, int value, char *name, char flag);
extern Plugin *CLPlugins_FindLinkPluginForTarget(Plugin *plugins, unsigned int kind, unsigned int subtype);
extern UInt8 CLPlugins_WriteObjectFile(Plugin *plugin, CWFileSpec *context, CWFileSpec *input, unsigned int argument,
                                       int option, int handle);
extern Boolean validate_plugin(Plugin *plug, const char **errmsg);
extern int CLPlugins_BuildPluginRequests(Plugin *list, SInt32 *count, PluginRequest **out);
extern unsigned int get_callback_result(void *input);
extern Plugin *CLPlugins_FindMatchingTargetPlugin(Plugin *node, SInt32 a, SInt32 b, SInt32 c, SInt32 d);
extern FileMapInfo *get_file_map(Plugin *context);
extern Plugin *CLPlugins_SelectPluginByRequestsAndValues(Plugin *plugins, int selector, int request_count,
                                                         PluginRequest *requests, int option4, int option5,
                                                         int value_count, char **values);
extern int fn_0040a610(Plugin *input, SInt32 param_2);
extern int CLPlugins_DispatchArgumentToPlugins(Plugin *node, SInt32 argument, SInt32 firstIdentifier,
                                               SInt32 secondIdentifier);
extern int CLPlugins_GetUniquePluginNames(Plugin *nameList, SInt32 *nameCount, char ***nameArray);
extern short CLPlugins_CallEntry(Plugin *dispatch, CWPluginPrivateContext *argument);

#ifdef __cplusplus
}
#endif

#endif
