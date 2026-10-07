#define CERROR_FILE "CLPlugins.c"
#include "compiler/common.h"
#include "driver/CLPlugins.h"
#include "compiler/CPrep.h"
#include "driver/AssertionFailure.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLPrefs.h"
#include "driver/CLTarg.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ClientGlue.h"
#include "driver/Files.h"
#include "driver/MacFileTypes.h"
#include "driver/MacSpecs.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/CLMain.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef short(__stdcall *code_short)(void *);

typedef short(__stdcall *cb_t)(char **);

typedef unsigned short(__stdcall *pfn_t)(void *);

typedef SInt16(__stdcall *CLPluginFunc)(PluginRequest *, SInt32, SInt32, Boolean *);

typedef struct PlugAux PlugAux;

typedef short(__stdcall *PluginInputCallback)(int, short *, unsigned int, int, int);
typedef short(__stdcall *PluginResultCallback)(unsigned int *);

char *CLPlugins_GetName(Plugin *plugin)
{
    short result;
    char *name;

    if (plugin == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0x36);
    }
    if (plugin->callbacks->getName != NULL) {
        result = plugin->callbacks->getName(&name);
        if (result == 0) {
            return name;
        }
    }
    return "(no name found)";
}

UInt8 *get_plugin_result(Plugin *plugin)
{
    static UInt8 data_00541480[4] = {0};
    short status;
    UInt8 *result;

    if (plugin == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0x42);
    }
    if (plugin->callbacks->getResult != NULL) {
        status = plugin->callbacks->getResult((void **)&result);
        if (status == 0) {
            return result;
        }
    }
    return data_00541480;
}

PluginDesc *CLPlugins_GetPluginDesc(Plugin *provider)
{
    static PluginDesc plugin_desc;
    unsigned int *data;
    unsigned int size;

    if (provider == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0x56);
    }
    if (provider->callbacks->getData != NULL) {
        if (provider->callbacks->getData(&data, &size) == 0) {
            memset(&plugin_desc, 0, sizeof(plugin_desc));
            memcpy(&plugin_desc, data, size);
            return &plugin_desc;
        }
    }
    return NULL;
}

unsigned int CLPlugins_GetType(Plugin *type)
{
    PluginDesc *resolvedType;
    if (!type)
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 102U);
    resolvedType = CLPlugins_GetPluginDesc(type);
    if (resolvedType)
        return resolvedType->type;
    return 1313820229U;
}

TargetInfo *get_target_info(Plugin *entry)
{
    static TargetInfo target_info = {1};
    short status;
    unsigned char *result;

    if (entry == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 115);
    }
    if (((Plugin *)entry)->cl_cb == NULL) {
        CLIO_ReportAssertionFailure("pl->cl_cb != NULL", "CLPlugins.c", 116);
    }
    if (((Plugin *)entry)->cl_cb->getTargetInfo != NULL) {
        status = ((Plugin *)entry)->cl_cb->getTargetInfo(&result);
        if (status == 0) {
            return (TargetInfo *)result;
        }
    }
    return &target_info;
}

void *get_plugin_directory_list(Plugin *plugin)
{
    static struct PluginDirectoryList plugin_directory_list = {1, 0, NULL};
    struct PluginDirectoryList *directoryList;
    SInt16 status;
    if (plugin == NULL)
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0x8a);
    if (plugin->callbacks->getDirectoryList != NULL) {
        status = plugin->callbacks->getDirectoryList(&directoryList);
        if (status == 0)
            return directoryList;
    }
    return &plugin_directory_list;
}

FileMapInfo *get_file_map(Plugin *context)
{
    static FileMapInfo file_map = {1};
    FileMapInfo *result;

    if (context == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0x9e);
    }
    if (context->cl_cb == NULL) {
        CLIO_ReportAssertionFailure("pl->cl_cb != NULL", "CLPlugins.c", 0x9f);
    }
    if (context->cl_cb->getFileMap != NULL) {
        if ((*context->cl_cb->getFileMap)(&result) == 0) {
            return result;
        }
    }
    return &file_map;
}

unsigned int get_callback_result(void *input)
{
    short status;
    unsigned int result;
    PluginResultCallback **callbacks = input;

    if (callbacks == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0xb3);
    }
    if ((*callbacks)[8] != NULL) {
        status = (*callbacks)[8](&result);
        if (status == 0) {
            return result;
        }
    }
    return 0;
}

const CWObjectFlags *CLPlugins_GetObjectFlags(Plugin *plugin)
{
    static CWObjectFlags object_flags = {2, 0, "", "", "", "", "", ""};
    const CWObjectFlags *flags;
    PluginDesc *info;

    if (plugin == NULL) {
        CLIO_ReportAssertionFailure("pl", "CLPlugins.c", 0xbf);
    }
    if (plugin->cl_cb == NULL) {
        CLIO_ReportAssertionFailure("pl->cl_cb != NULL", "CLPlugins.c", 0xc0);
    }
    if (plugin->cl_cb->getObjectFlags != NULL) {
        if (plugin->cl_cb->getObjectFlags(&flags) == 0) {
            return flags;
        }
    }
    info = CLPlugins_GetPluginDesc(plugin);
    if (info->type == 0x436f6d70) {
        return NULL;
    }
    return &object_flags;
}

Boolean plugin_name_matches(Plugin *plugin, char *name)
{
    char *pluginName;
    int comparison;

    pluginName = CLPlugins_GetName(plugin);
    comparison = strcmp(pluginName, name);
    return comparison == 0;
}

char CLPlugins_MatchTarget(Plugin *plugin, int firstIdentifier, int secondIdentifier, int flag)
{
    TargetInfo *lists;
    short firstCount;
    int firstIndex;

    lists = get_target_info(plugin);
    firstCount = lists->ncpu;
    for (firstIndex = 0; firstIndex < firstCount; firstIndex = firstIndex + 1) {
        if (firstIdentifier == 0x2a2a2a2a || firstIdentifier == lists->cpus[firstIndex] ||
            (lists->cpus[firstIndex] == 0x2a2a2a2a && (char)flag == '\0')) {
            short secondCount = lists->nos;
            int secondIndex;
            for (secondIndex = 0; secondIndex < secondCount; secondIndex = secondIndex + 1) {
                if (secondIdentifier == 0x2a2a2a2a || secondIdentifier == lists->oses[secondIndex] ||
                    (lists->oses[secondIndex] == 0x2a2a2a2a && (char)flag == '\0')) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

char file_map_matches(FileMap *reference, int value, char *name, char flag)
{
    if (!(((reference->ext[0] != '\0' || flag != '\0') && (name[0] != '\0' || flag != '\0')) ||
          reference->type != value) ||
        (((reference->type == 0 && flag == '\0') || (value == 0 && flag == '\0')) &&
         CLIO_CompareStringsIgnoreCase(reference->ext, name) == 0) ||
        (CLIO_CompareStringsIgnoreCase(reference->ext, name) == 0 && reference->type == value))
        return 1;
    else
        return 0;
}

char plugin_file_map_matches(Plugin *entry, int value, char *text, char option)
{
    FileMapInfo *list = get_file_map(entry);
    int index = 0;
    while (index < list->count) {
        if (file_map_matches(&list->maps[index], value, text, option) != 0) {
            return 1;
        }
        index++;
    }
    return 0;
}

Boolean matches_plugin_type_lang(Plugin *entry, int type, int language, int flags)
{
    PluginDesc *desc = CLPlugins_GetPluginDesc(entry);
    if (desc->type == type || type == 0x2a2a2a2aU) {
        if (desc->lang == language || language == 0x2a2a2a2a || (desc->lang == 0x2a2a2a2a && (Boolean)flags == 0))
            return 1;
    }
    return 0;
}
Boolean call_query_callback(Plugin *p, PluginRequest *a, SInt32 b, SInt32 c)
{
    CLPluginFunc f;
    Boolean result;

    if (p->pr_cb == NULL) {
        CLIO_ReportAssertionFailure("pl->pr_cb != NULL", "CLPlugins.c", 0x143);
    }
    if ((f = *(CLPluginFunc *)p->pr_cb) != NULL) {
        if (f(a, b, c, &result) == 0) {
            return result;
        }
    }
    return 0;
}

UInt8 query_plugin(Plugin *plugin, unsigned int queryArgument, char **queryKind)
{
    short status;
    UInt8 result;

    if (plugin->pr_cb == NULL) {
        CLIO_ReportAssertionFailure("pl->pr_cb != NULL", "CLPlugins.c", 0x14f);
    }
    if (plugin->pr_cb->query != NULL) {
        status = plugin->pr_cb->query(queryArgument, queryKind, &result);
        if (status == 0) {
            return result;
        }
    }
    return 0;
}

char *format_plugin_version(Plugin *plugin, char *buffer)
{
    static char data_0057d90a[18];
    UInt8 *version;
    char *cursor;

    version = get_plugin_result(plugin);
    if (buffer == (char *)0x0) {
        buffer = data_0057d90a;
    }
    if (version[0] | version[1] | version[2] | version[3] != 0) {
        cursor = buffer;
        cursor += sprintf(cursor, "%u", version[0]);
        cursor += sprintf(cursor, ".%u", version[1]);
        if (version[2] != 0) {
            cursor += sprintf(cursor, ".%u", version[2]);
        }
        if (version[3] != 0) {
            sprintf(cursor, " build %u", version[3]);
        }
        return buffer;
    }
    return "(unknown)";
}

/* Callback table with five preceding entries. */
/* Plugin value and callback table. */

UInt8 CLPlugins_WriteObjectFile(Plugin *plugin, CWFileSpec *context, CWFileSpec *input, unsigned int objectFlags,
                                int option, int objectHandle)
{
    int validInput;
    int validContext;
    UInt8 result;
    short callbackResult;
    OSHandle *objectBuffer;
    struct OSSpec outputSpec;

    if (plugin->cl_cb == NULL) {
        CLIO_ReportAssertionFailure("pl->cl_cb != NULL", "CLPlugins.c", 0x173);
    }
    validInput = 0;
    validContext = 0;
    if (objectHandle != 0 && context != NULL) {
        validContext = 1;
    }
    if (validContext != 0 && input != NULL) {
        validInput = 1;
    }
    if (!validInput) {
        CLIO_ReportAssertionFailure("data != NULL && srcfss != NULL && outfss != NULL", "CLPlugins.c", 0x174);
    }
    if (plugin->cl_cb->writeObjectFile != NULL) {
        callbackResult = (*plugin->cl_cb->writeObjectFile)(context, input, objectFlags, option, objectHandle);
        return callbackResult == 0;
    }
    MacSpecs_MakeOSSpec(input, &outputSpec);
    objectBuffer = Memory_GetSizeAddress((struct StorageHandle *)objectHandle);
    result = fn_00415090(&outputSpec, objectFlags, option, objectBuffer);
    return result;
}

UInt8 CLPlugins_FindFileMapValue(Plugin *plugin, int mode, char *name, unsigned int *result)
{
    char matched;
    FileMapInfo *list;
    int index;
    int selected;

    list = get_file_map(plugin);
    selected = -1;
    for (index = 0; index < list->count; ++index) {
        matched = file_map_matches(&list->maps[index], mode, name, 0);
        if (matched != 0) {
            selected = index;
            matched = file_map_matches(&list->maps[index], mode, name, 1);
            if (matched != 0)
                break;
        }
    }
    if (selected < 0) {
        *result = 0;
        return 0;
    }
    *result = list->maps[selected].value;
    return 1;
}
Boolean validate_plugin(Plugin *plug, const char **errmsg)
{
    /* the size of each version of DropInFlags */
    static SInt32 dropin_flags_size[3] = {0, 16, 18};
    PluginDesc *flags;
    unsigned int size;

    *errmsg = "";

    if (plug->callbacks->getData == NULL) {
        *errmsg = "GetDropInFlags callback not found";
        return 0;
    }
    if (plug->callbacks->getData((unsigned int **)&flags, &size) != 0) {
        *errmsg = "GetDropInFlags callback failed";
        return 0;
    }
    if (flags->type != 'Comp' && flags->type != 'Link' && flags->type != 'Pars' &&
        flags->type != (unsigned int)'cldr') {
        *errmsg = "The plugin type is not supported by this driver";
        return 0;
    }
    if (flags->api1 > 10) {
        *errmsg = "The plugin's earliest compatible API version is too new for this driver";
        return 0;
    }
    if (flags->api1 > 1 && flags->api2 < 10 && clState.pluginDebug) {
        CLIO_WriteFormattedText("%s's newest compatible API version is probably too old for this driver\n",
                                CLPlugins_GetName(plug));
    }
    if (size != dropin_flags_size[flags->descriptorVersion]) {
        *errmsg = "The plugin's DropInFlags has an unexpected size";
        return 0;
    }
    if ((flags->flags & 1) == 0 && plug->callbacks->entry == NULL) {
        *errmsg = "The plugin has no entry point";
        return 0;
    }
    if ((flags->flags & 1) != 0 && plug->callbacks->entry != NULL) {
        *errmsg = "The executable tool stub has an entry point";
        return 0;
    }
    if (plug->cl_cb != NULL) {
        const CWObjectFlags *objectFlags;

        if (plug->cl_cb->getObjectFlags == NULL && flags->type == 'Comp') {
            *errmsg = "GetObjectFlags callback not found in compiler plugin";
            return 0;
        }
        objectFlags = CLPlugins_GetObjectFlags(plug);
        if (objectFlags->version < 2 || (objectFlags->flags & 0x7fffffff) != 0) {
            *errmsg = "The object flags data is out-of-date or invalid";
            return 0;
        }
    }
    return 1;
}

Boolean fn_00409700(Plugin *plugin)
{
    Boolean hasMissingDirectory;
    unsigned short status;
    NameTableEntry *directory;
    int index;
    PluginDirectoryList *directories;

    hasMissingDirectory = 0;
    if (plugin->callbacks->getDirectoryList != NULL &&
        (status = plugin->callbacks->getDirectoryList(&directories), status == 0)) {
        for (index = 0; index < directories->count; index = index + 1) {
            directory = CLPrefs_FindNameTableEntry(directories->panels[index]);
            if (directory == NULL) {
                CLErrors_EmitDiagnostic(0x5b, directories->panels[index]);
                hasMissingDirectory = 1;
            }
        }
    }
    return (Boolean)(!hasMissingDirectory);
}

Plugin *CLPlugins_CreatePluginDataCopy(PluginRequiredInputRecord *record36, PluginOptionalData *record24,
                                       PluginQueryTable *record8)
{
    PluginQueryTable *copy8;
    PluginQueryTable *source8;
    Plugin *copy;
    source8 = record8;
    copy = xmalloc(0U, 20U);
    if (!copy)
        return NULL;
    if (!record36)
        return NULL;
    copy->callbacks = xmalloc(0U, 36U);
    if (!copy->callbacks)
        return NULL;
    *(PluginRequiredInputRecord *)copy->callbacks = *record36;
    if (record24) {
        copy->cl_cb = xmalloc(0U, 24U);
        if (!copy->cl_cb)
            return NULL;
        *(PluginOptionalData *)copy->cl_cb = *record24;
    } else {
        copy->cl_cb = NULL;
    }
    if (source8) {
        copy->pr_cb = xmalloc(0U, 8U);
        if (!copy->pr_cb)
            return NULL;
        copy8 = copy->pr_cb;
        *copy8 = *source8;
    } else {
        copy->pr_cb = NULL;
    }
    copy->object = 0U;
    copy->next = 0U;
    return copy;
}

void free_plugin(Plugin *allocations)
{
    if (allocations != NULL) {
        if (allocations->callbacks != NULL) {
            free(allocations->callbacks);
        }
        if (allocations->cl_cb != NULL) {
            free(allocations->cl_cb);
        }
        if (allocations->pr_cb != NULL) {
            free(allocations->pr_cb);
        }
        free(allocations);
    }
}

Boolean fn_004098a0(Plugin *input)
{
    if (!fn_00409700(input)) {
        CLErrors_EmitDiagnostic(0x5c, CLPlugins_GetName(input));
        return 0;
    }
    return 1;
}

static Plugin *data_0057d91c;

void fn_004098d0(void)
{
    data_0057d91c = NULL;
    return;
}

void CLPlugins_FreePlugins(void)
{
    Plugin *p;

    p = data_0057d91c;
    while (p != NULL) {
        Plugin *next;
        next = p->next;
        fn_00417440((Plugin *)p, '\0');
        free_plugin(p);
        p = next;
    }
}

int CLPlugins_AddPlugin(void *pluginHandle)
{
    Plugin *plugin = pluginHandle;
    char *name;
    const char *local;
    PluginDesc *desc;
    SInt32 lang;
    TargetInfo *ti;
    FileMapInfo *fm;
    PluginDirectoryList *pi;
    Plugin **slot;
    SInt16 i;

    name = CLPlugins_GetName(plugin);
    if (!validate_plugin(plugin, &local)) {
        CLErrors_EmitDiagnostic(0x58, name, format_plugin_version(plugin, NULL), local);
        return 0;
    }
    slot = &data_0057d91c;
    while (*slot != NULL) {
        if (plugin_name_matches(*slot, name)) {
            CLErrors_EmitDiagnostic(0x59, name);
            return 0;
        }
        slot = &(*slot)->next;
    }
    *slot = plugin;

    desc = CLPlugins_GetPluginDesc(plugin);
    if (((desc->flags & 1) == 0) && !(Boolean)fn_00417440(plugin, 1)) {
        CLErrors_EmitDiagnostic(3, name);
        return 0;
    }
    if (desc->type == 0x436f6d70 && (desc->flags & 0x17c000)) {
        CLErrors_EmitDiagnostic(4, "compiler", name);
    }
    if (desc->type == 0x4c696e6b && (desc->flags & 0x5fe0000)) {
        CLErrors_EmitDiagnostic(4, "linker", name);
    }

    if (clState.pluginDebug) {
        lang = desc->lang;
        if (lang == 0)
            lang = 0x2d2d2d2d;
        CLIO_FormatAndDispatchText("Added plugin '%s', version '%s'\n", name, format_plugin_version(plugin, NULL));
        CLIO_FormatAndDispatchText("Type: '%c%c%c%c';  Lang: '%c%c%c%c';  API range: %d-%d\n",
                                   (desc->type & 0xff000000) >> 24, (desc->type & 0xff0000) >> 16,
                                   (desc->type & 0xff00) >> 8, desc->type & 0xff, (lang & 0xff000000) >> 24,
                                   (lang & 0xff0000) >> 16, (lang & 0xff00) >> 8, lang & 0xff, desc->api1, desc->api2);

        if (plugin->cl_cb != NULL) {
            ti = get_target_info(plugin);
            CLIO_FormatAndDispatchText("Target CPUs: ");
            for (i = 0; i < ti->ncpu; i++) {
                UInt32 v = ti->cpus[i];
                CLIO_FormatAndDispatchText("'%c%c%c%c', ", (v & 0xff000000) >> 24, (v & 0xff0000) >> 16,
                                           (v & 0xff00) >> 8, v & 0xff);
            }
            CLIO_FormatAndDispatchText("\nTarget OSes: ");
            for (i = 0; i < ti->nos; i++) {
                UInt32 v = ti->oses[i];
                CLIO_FormatAndDispatchText("'%c%c%c%c', ", (v & 0xff000000) >> 24, (v & 0xff0000) >> 16,
                                           (v & 0xff00) >> 8, v & 0xff);
            }
            CLIO_FormatAndDispatchText("\n");
            fm = get_file_map(plugin);
            CLIO_FormatAndDispatchText("File mappings:\n");
            for (i = 0; i < fm->count; i++) {
                CLIO_FormatAndDispatchText("\tFile type: '%c%c%c%c'  Extension: '%s'\n",
                                           (fm->maps[i].type & 0xff000000) >> 24, (fm->maps[i].type & 0xff0000) >> 16,
                                           (fm->maps[i].type & 0xff00) >> 8, fm->maps[i].type & 0xff, fm->maps[i].ext);
            }
        }
        pi = get_plugin_directory_list(plugin);
        CLIO_FormatAndDispatchText("Pref panels needed:\n");
        for (i = 0; i < pi->count; i++) {
            CLIO_FormatAndDispatchText("\t'%s'\n", pi->panels[i]);
        }
        CLIO_FormatAndDispatchText("Dropin flags:\n");
        if (desc->flags & 1) {
            CLIO_FormatAndDispatchText("\texecutable tool,\n");
        }
        if (desc->type == 0x436f6d70) {
            if (desc->flags & 0x80000000)
                CLIO_FormatAndDispatchText("\tgenerates code,\n");
            if (desc->flags & 0x40000000)
                CLIO_FormatAndDispatchText("\tgenerates resources, \n");
            if (desc->flags & 0x20000000)
                CLIO_FormatAndDispatchText("\tcan preprocess, \n");
            if (desc->flags & 0x10000000)
                CLIO_FormatAndDispatchText("\tcan precompile, \n");
            if (desc->flags & 0x08000000)
                CLIO_FormatAndDispatchText("\tis Pascal, \n");
            if (desc->flags & 0x04000000)
                CLIO_FormatAndDispatchText("\tcan import, \n");
            if (desc->flags & 0x02000000)
                CLIO_FormatAndDispatchText("\tcan disassemble, \n");
            if (desc->flags & 0x00800000)
                CLIO_FormatAndDispatchText("\tallow duplicate filenames, \n");
            if (desc->flags & 0x00400000)
                CLIO_FormatAndDispatchText("\tallow multiple targets, \n");
            if (desc->flags & 0x00100000)
                CLIO_FormatAndDispatchText("\tuses target storage, \n");
            if (desc->flags & 0x00080000)
                CLIO_FormatAndDispatchText("\temits own browser symbols, \n");
            if (desc->flags & 0x00040000)
                CLIO_FormatAndDispatchText("\tshould be always reloaded, \n");
            if (desc->flags & 0x00020000)
                CLIO_FormatAndDispatchText("\trequires project build started msg, \n");
            if (desc->flags & 0x00010000)
                CLIO_FormatAndDispatchText("\trequires target build started msg, \n");
            if (desc->flags & 0x00008000)
                CLIO_FormatAndDispatchText("\trequires subproject build started msg, \n");
            if (desc->flags & 0x00004000)
                CLIO_FormatAndDispatchText("\trequires file list build started msg, \n");
        }
        if (desc->type == 0x4c696e6b) {
            if (desc->flags & 0x80000000)
                CLIO_FormatAndDispatchText("\tcan't disassemble, \n");
            if (desc->flags & 0x40000000)
                CLIO_FormatAndDispatchText("\tis a post-linker, \n");
            if (desc->flags & 0x20000000)
                CLIO_FormatAndDispatchText("\tallow duplicate filenames, \n");
            if (desc->flags & 0x10000000)
                CLIO_FormatAndDispatchText("\tallow multiple targets, \n");
            if (desc->flags & 0x08000000)
                CLIO_FormatAndDispatchText("\tis a pre-linker, \n");
            if (desc->flags & 0x04000000)
                CLIO_FormatAndDispatchText("\tuses target storage, \n");
            if (desc->flags & 0x02000000)
                CLIO_FormatAndDispatchText("\tsupports unmangling, \n");
            if (desc->flags & 0x01000000)
                CLIO_FormatAndDispatchText("\tis Magic Cap linker, \n");
            if (desc->flags & 0x00800000)
                CLIO_FormatAndDispatchText("\tshould be always reloaded, \n");
            if (desc->flags & 0x00400000)
                CLIO_FormatAndDispatchText("\trequires project build started msg, \n");
            if (desc->flags & 0x00200000)
                CLIO_FormatAndDispatchText("\trequires target build started msg, \n");
            if (desc->flags & 0x00100000)
                CLIO_FormatAndDispatchText("\trequires subproject build started msg, \n");
            if (desc->flags & 0x00080000)
                CLIO_FormatAndDispatchText("\trequires file list build started msg, \n");
            if (desc->flags & 0x00040000)
                CLIO_FormatAndDispatchText("\trequires target link started msg, \n");
            if (desc->flags & 0x00020000)
                CLIO_FormatAndDispatchText("\twants pre-run request, \n");
        }
        CLIO_FormatAndDispatchText("\n");
    }
    return 1;
}

Plugin *CLPlugins_FindMatchingTargetPlugin(Plugin *node, SInt32 a, SInt32 b, SInt32 c, SInt32 d)
{
    Plugin *obj = node ? node : data_0057d91c;
    Plugin *result = NULL;

    while (obj != NULL) {
        if (obj->cl_cb != NULL && CLPlugins_MatchTarget(obj, a, b, 0) && matches_plugin_type_lang(obj, c, d, 0)) {
            result = obj;
            if (CLPlugins_MatchTarget(obj, a, b, 1) && matches_plugin_type_lang(obj, c, d, 1))
                break;
        }
        obj = obj->next;
    }
    return result;
}

Plugin *CLPlugins_FindTargetPluginBySelectorOptionName(Plugin *first, int selector, int optionKind, int optionValue,
                                                       int nameKind, char *name, int selectorValue)
{
    Plugin *candidate;
    Plugin *match;
    char nameMatch;

    candidate = first != NULL ? first : data_0057d91c;
    match = NULL;
    while (candidate != NULL) {
        if ((matches_plugin_type_lang(candidate, selector, selectorValue, '\x01') != '\0') &&
            (candidate->cl_cb != NULL) && (CLPlugins_MatchTarget(candidate, optionKind, optionValue, '\0') != '\0') &&
            ((nameMatch = plugin_file_map_matches(candidate, nameKind, name, '\0')) != 0)) {
            match = candidate;
            if ((CLPlugins_MatchTarget(candidate, optionKind, optionValue, '\x01') != '\0') &&
                ((nameMatch = plugin_file_map_matches(candidate, nameKind, name, '\x01')) != 0)) {
                break;
            }
        }
        candidate = candidate->next;
    }
    return match;
}
Plugin *CLPlugins_FindLinkerPlugin(Plugin *start, int kind, int variant)
{
    Plugin *plugin;
    Plugin *candidate;
    PluginDesc *capabilities;

    plugin = start != NULL ? start : data_0057d91c;
    candidate = NULL;
    while (plugin != NULL) {
        if (matches_plugin_type_lang(plugin, 0x4c696e6b, 0x2a2a2a2a, 1) != '\0' &&
            ((capabilities = CLPlugins_GetPluginDesc(plugin), (capabilities->flags & 0x48000000) == 0)) &&
            CLPlugins_MatchTarget(plugin, kind, variant, '\0') != '\0') {
            candidate = plugin;
            if (CLPlugins_MatchTarget(plugin, kind, variant, '\x01') != '\0') {
                break;
            }
        }
        plugin = plugin->next;
    }
    return candidate;
}

Plugin *CLPlugins_FindLinkPluginForTarget(Plugin *plugins, unsigned int kind, unsigned int subtype)
{
    Plugin *plugin = plugins ? plugins : data_0057d91c;
    Plugin *result = NULL;
    while (plugin != NULL) {
        if (matches_plugin_type_lang(plugin, 0x4c696e6bU, 0x2a2a2a2aU, 1U)) {
            if ((CLPlugins_GetPluginDesc(plugin)->flags & 0x8000000U) != 0U) {
                if (CLPlugins_MatchTarget(plugin, kind, subtype, 0U)) {
                    result = plugin;
                    if (CLPlugins_MatchTarget(plugin, kind, subtype, 1U))
                        break;
                }
            }
        }
        plugin = plugin->next;
    }
    return result;
}

Plugin *CLPlugins_FindMatchingLinkPlugin(Plugin *plugins, int kind, int selector)
{
    Plugin *plugin = plugins ? plugins : data_0057d91c;
    Plugin *match = NULL;
    while (plugin != NULL) {
        if (matches_plugin_type_lang(plugin, 0x4c696e6b, 0x2a2a2a2a, 1) != '\0') {
            if ((CLPlugins_GetPluginDesc(plugin)->flags & 0x40000000) != 0) {
                if (CLPlugins_MatchTarget(plugin, kind, selector, 0) != '\0') {
                    match = plugin;
                    if (CLPlugins_MatchTarget(plugin, kind, selector, 1) != '\0')
                        break;
                }
            }
        }
        plugin = plugin->next;
    }
    return match;
}

Plugin *CLPlugins_SelectPluginByRequestsAndValues(Plugin *plugins, int selector, int request_count,
                                                  PluginRequest *requests, int option4, int option5, int value_count,
                                                  char **values)
{
    int request_index;
    UInt8 *results;
    int value_index;
    char **value;
    int failed_index;
    char matched;
    char all_succeeded;
    Plugin *plugin;
    Plugin *selected;
    plugin = plugins != NULL ? plugins : data_0057d91c;
    selected = NULL;
    results = malloc(value_count);
    while (plugin != NULL) {
        if (matches_plugin_type_lang(plugin, 1348563571, selector, 1) != 0) {
            request_index = 0;
            matched = 0;
            for (; request_index < request_count; request_index = request_index + 1) {
                if (requests[request_index].tag.signed_kind != 1348563571 &&
                    requests[request_index].tag.kind != 1668047986 && requests[request_index].flag_10 == 0 &&
                    call_query_callback(plugin, &requests[request_index], option4, option5) != 0) {
                    matched = 1;
                }
            }
            if (matched != 0) {
                selected = plugin;
                value_index = 0;
                all_succeeded = 1;
                if (value_count > 0) {
                    value = values;
                    do {
                        results[value_index] = query_plugin(plugin, 1, value);
                        value = value + 1;
                        all_succeeded &= results[value_index];
                        value_index = value_index + 1;
                    } while (value_index < value_count);
                }
                if (all_succeeded != 0) {
                    break;
                }
            }
        }
        plugin = plugin->next;
    }
    if (selected != NULL && all_succeeded == 0) {
        CLErrors_ForwardMessage(5);
        failed_index = 0;
        for (; failed_index < value_count; failed_index = failed_index + 1) {
            if (results[failed_index] == 0) {
                CLErrors_ForwardMessage(6, values[failed_index]);
            }
        }
    }
    free(results);
    return selected;
}

int CLPlugins_BuildPluginRequests(Plugin *list, SInt32 *count, PluginRequest **out)
{
    Plugin *node;
    PluginDesc *info;
    UInt8 *name;
    SInt32 itemCount;
    SInt32 index;

    node = list ? list : data_0057d91c;
    itemCount = 0;
    while (node != NULL) {
        itemCount++;
        node = node->next;
    }
    *count = itemCount;
    *out = xmalloc(NULL, itemCount * sizeof(PluginRequest));
    if (*out == NULL)
        return 0;

    node = list ? list : data_0057d91c;
    index = 0;
    while (node != NULL) {
        info = (PluginDesc *)CLPlugins_GetPluginDesc(node);
        name = get_plugin_result(node);
        if (info == NULL)
            CLIO_ReportAssertionFailure("df != NULL", "CLPlugins.c", 0x40d);
        if (name == NULL)
            CLIO_ReportAssertionFailure("vi != NULL", "CLPlugins.c", 0x40e);
        (*out)[index].tag.signed_kind = info->type;
        (*out)[index].secondCode = info->lang;
        (*out)[index].flags = info->flags;
        (*out)[index].name = (name[0] << 24) | (name[1] << 16) | (name[2] << 8) | name[3];
        (*out)[index].flag_10 = (info->flags & 1) != 0;
        index++;
        node = node->next;
    }
    return 1;
}

static inline SInt32 CountPluginNames(Plugin *list)
{
    Plugin *p;
    PluginDirectoryList *s;
    SInt32 total;

    p = list ? list : data_0057d91c;
    total = 0;
    for (; p != NULL; p = p->next) {
        s = get_plugin_directory_list(p);
        total += s->count;
    }
    return total;
}

int CLPlugins_GetUniquePluginNames(Plugin *nameList, SInt32 *nameCount, char ***nameArray)
{
    char **array;
    PluginDirectoryList *resolvedList;
    Plugin *list;
    SInt32 count;
    SInt32 nameIndex;
    SInt32 existingIndex;

    array = (char **)malloc(CountPluginNames(nameList) << 2);
    if (array == NULL)
        return 0;

    count = 0;
    list = nameList ? nameList : data_0057d91c;

    for (; list != NULL; list = list->next) {
        resolvedList = get_plugin_directory_list(list);
        for (nameIndex = 0; nameIndex < resolvedList->count; nameIndex++) {
            existingIndex = 0;
            if (0 < count) {
                do {
                    if (CLIO_CompareStringsIgnoreCase(array[existingIndex], resolvedList->panels[nameIndex]) == 0)
                        break;
                    existingIndex++;
                } while (existingIndex < count);
            }
            if (existingIndex >= count) {
                array[count] = resolvedList->panels[nameIndex];
                count++;
            }
        }
    }

    *nameArray = xmalloc(NULL, count << 2);
    if (*nameArray == NULL)
        return 0;
    *nameArray = (char **)realloc(array, count << 2);
    *nameCount = count;
    return 1;
}

int fn_0040a610(Plugin *plugin, SInt32 refCon)
{
    unsigned int fileTypesTable;

    fileTypesTable = get_callback_result(plugin);
    if (fileTypesTable == 0) {
        return 1;
    }
    CLPluginRequests_AppendMacFileTypesTable(NULL, fileTypesTable);
    return 1;
}

int CLPlugins_DispatchArgumentToPlugins(Plugin *node, SInt32 argument, SInt32 firstIdentifier, SInt32 secondIdentifier)
{
    if (node == NULL) {
        node = data_0057d91c;
    }
    while (node != NULL) {
        if ((node->cl_cb == NULL) || (CLPlugins_MatchTarget(node, firstIdentifier, secondIdentifier, 0) != 0)) {
            fn_0040a610(node, argument);
        }
        node = node->next;
    }
    return 1;
}

short CLPlugins_CallEntry(Plugin *dispatch, CWPluginPrivateContext *argument)
{
    if (dispatch->callbacks->entry != NULL) {
        return dispatch->callbacks->entry((unsigned int)argument);
    }
    return 2;
}
