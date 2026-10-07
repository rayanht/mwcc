#include "compiler/common.h"
#include "driver/ParserFace.h"
#include "compiler/CPrep.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/Arguments.h"
#include "driver/CLMain.h"
#include "driver/CLPlugins.h"
#include "driver/CLToolExec.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ClientGlue.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/Files.h"
#include "driver/Help.h"
#include "driver/Memory.h"
#include "driver/Option.h"
#include "driver/Parameter.h"
#include "driver/ParserErrors.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/ParserHelpers.h"
#include "driver/Projects.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers.h"
#include "driver/cc-eabi-ppc-mw.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#define B3(x) (((x) & 0xff000000) >> 24)
#define B2(x) (((x) & 0xff0000) >> 16)
#define B1(x) (((x) & 0xff00) >> 8)
#define B0(x) ((x) & 0xff)

const char *DAT_00543380 = NULL;

static void *xmalloc(const char *what, int size)
{
    char *buffer;
    char message[80];

    buffer = malloc(size);
    if (buffer == NULL) {
        sprintf(message, "Out of memory when allocating %d bytes%s%s", size, what ? " for " : "", what ? what : "");
        CWPluginsPrivate_CallCallback9(pluginPrivateContext, message, NULL, NULL, 0);
        longjmp(plugin_request_jmp_buf, 7);
        return NULL;
    }
    return buffer;
}

void *ToolHelpers_ResizeBuffer(const char *what, void *buffer, int size)
{
    char *resizedBuffer;
    char message[80];

    resizedBuffer = realloc(buffer, size);
    if (resizedBuffer == NULL) {
        sprintf(message, "Out of memory when resizing buffer to %d bytes%s%s", size, what ? " for " : "",
                what ? what : "");
        CWPluginsPrivate_CallCallback9(pluginPrivateContext, message, NULL, NULL, 0);
        longjmp(plugin_request_jmp_buf, 7);
        return NULL;
    }
    return resizedBuffer;
}

int initialize_cmdline_environment(struct CWPluginPrivateContext *context)
{
    EnvInfo **h;
    EnvInfo info;
    int err;
    int i;
    memset(&pluginPrivateContext, 0, 0x194);
    pluginPrivateContext = context;
    if ((err = DropInCompilerLinkerPrivate_CallArgumentValue(context, "CmdLine Environment", &h)) != 0)
        return err;
    info = **h;
    data_00587e1f = info.parserEnvironmentFlag;
    data_00587ce6 = info.height;
    help_width = info.width;
    if ((err = CWParserPluginsPrivate_GetEnvironment(context, &cmdline_environment)) != 0)
        return err;
    if ((err = CWParserPluginsPrivate_GetOutputValues(context, data_00587cfc, data_00587d00)) != 0)
        return err;
    if ((err = CWParserPluginsPrivate_GetLookupValues(context, &lookup_value_count, &lookup_names)) != 0)
        return err;
    if ((err = CWParserPluginsPrivate_GetPanels(context, &num_panels, &data_00587cf0)) != 0)
        return err;
    data_00587e23 = 0;
    for (i = 0; i < num_panels; i++)
        if (data_00587cf0[i].enabled)
            data_00587e23 = 1;
    return 0;
}

int register_option_lists(struct CWPluginPrivateContext *context)
{
    if (driverTool == NULL) {
        return 2;
    }
    Targets_RegisterOptionLists();
    return 0;
}

int run_tool_checks(struct CWPluginPrivateContext *context)
{
    int r;
    unsigned short result;
    DriverTool *tool;
    tool_checks_passed = 1;
    data_00587e04 = 1;
    data_00587e0c = data_00587e08 = 0;
    Targets_InitPtrList(data_005876fc);
    Targets_InitPtrList(data_00587688);
    Targets_InitPtrList(&data_00588044);
    tool = (DriverTool *)driverTool;
    if (tool->precheck) {
        DriverTool *calltool = (DriverTool *)driverTool;
        tool_checks_passed &= calltool->precheck();
    }
    Targets_ParseArguments(cmdline_environment->argc, cmdline_environment->argv);
    targets_value_null_or_zero = Targets_IsValueNullOrZero();
    tool_checks_passed &= Option_ParseOptionList(Option_GetOptionList(), 1) && !data_00587e1d;
    fn_0040f960();
    if ((r = fn_0040ba99(context)) != 0)
        return r;
    tool = (DriverTool *)driverTool;
    if (tool->check && tool_checks_passed) {
        DriverTool *calltool = (DriverTool *)driverTool;
        tool_checks_passed &= calltool->check();
    }
    if ((r = fn_0040ba99(context)) != 0)
        return r;
    if (data_00587e1e && tool_checks_passed)
        tool_checks_passed &= Option_ShowHelp();
    fn_0040f960();
    if (tool_checks_passed != 0)
        tool_checks_passed &= Option_ParseOptionList(Option_GetOptionList(), 0) && !data_00587e1d;
    tool = (DriverTool *)driverTool;
    if (tool->postcheck && tool_checks_passed) {
        DriverTool *calltool = (DriverTool *)driverTool;
        tool_checks_passed &= calltool->postcheck();
    }
    Targets_FreeTokenText();
    result = (tool_checks_passed && !data_00587e1d) ? 0 : 2;
    return result;
}

void **copy_resource_by_name(char *name)
{
    int index;
    struct StorageHandle *handle;
    Resource *resources;
    for (index = 0; index < driverTool[10]; index++) {
        if (!ClientGlue_CompareLowercaseStrings(name, (resources = (Resource *)driverTool[11])[index].name)) {
            handle = (struct StorageHandle *)Memory_NewHandle((resources = (Resource *)driverTool[11])[index].size);
            if (!handle)
                return NULL;
            fn_00413a00(handle);
            memcpy(handle->data, (resources = (Resource *)driverTool[11])[index].data,
                   (resources = (Resource *)driverTool[11])[index].size);
            fn_00413a50((void **)handle);
            return (void **)handle;
        }
    }
    return NULL;
}

int fn_0040ba99(struct CWPluginPrivateContext *context)
{
    int index;
    int result;
    struct StorageHandle *preferences;
    for (index = 0; index < lookup_value_count; index++) {
        char *name = lookup_names[index];
        preferences = (struct StorageHandle *)copy_resource_by_name(name);
        if (preferences != NULL) {
            result = fn_0041bfd0(pluginPrivateContext, name, (void **)preferences);
            if (result != 0) {
                fn_0040ecb1(0x44, name);
                return result;
            }
        }
    }
    return 0;
}

int set_enabled_link_parser_entries(struct CWPluginPrivateContext *context)
{
    int err;
    int i;
    IntegerSequenceResult data;
    if ((err = fn_0040ba99(context)) != 0)
        return err;
    for (i = 0; i < num_panels; i++) {
        if (data_00587cf0[i].type == 0x4c696e6b && data_00587cf0[i].enabled) {
            if (data_00587cf0[i].flags & 0x8000000)
                Targets_InitIntegerSequenceResult((PtrList *)data_00587688, &data);
            else if (data_00587cf0[i].flags & 0x40000000)
                Targets_InitIntegerSequenceResult((PtrList *)&data_00588044, &data);
            else
                Targets_InitIntegerSequenceResult((PtrList *)data_005876fc, &data);
            if ((err = CWParserPluginsPrivate_SetParserEntry(pluginPrivateContext, i, &data)) != 0)
                return err;
        } else if (data_00587cf0[i].enabled) {
            unsigned long type = data_00587cf0[i].type;
            unsigned long creator = data_00587cf0[i].creator;
            fprintf(stderr, "*** No support for %c%c%c%c/%c%c%c%c tool\n", B3(type), B2(type), B1(type), B0(type),
                    B3(creator), B2(creator), B1(creator), B0(creator));
        }
    }
    return 0;
}

static PluginDesc data_00543430 = {2, 'Pars', 7, 0, 'Seep', 11};
static const char *data_00543458 = "Command-Line Parser";
static const char *data_0054345c = "Command-Line Parser";
static PluginDirectoryList data_00543460 = {1, 0, NULL};
static UInt32 lbl_00543468 = '****';
static UInt32 lbl_0054346c = '****';
#pragma options align = mac68k
static struct {
    SInt16 version;
    SInt16 cpuCount;
    UInt32 *cpus;
    SInt16 osCount;
    UInt32 *oss;
} lbl_00543470 = {1, 1, &lbl_00543468, 1, &lbl_0054346c};
#pragma options align = reset
static UInt8 data_00543480[4] = {1, 1, 0, 0};

int __stdcall get_data_pointer_and_constant(unsigned char **dataPointer, int *constant)
{
    *dataPointer = (unsigned char *)&data_00543430;
    *constant = 18;
    return 0;
}

unsigned int __stdcall set_next_to_head(struct ListLink *entry)
{
    struct ListLink *head;
    head = (struct ListLink *)data_00543458;
    entry->next = head;
    return 0U;
}

unsigned int __stdcall set_link_next_from_global(struct ListLink *link)
{
    struct ListLink *next;
    next = (struct ListLink *)data_0054345c;
    link->next = next;
    return 0U;
}

unsigned int __stdcall fn_0040bc70(struct ListLink *link)
{
    link->next = (struct ListLink *)&data_00543460;
    return 0;
}

int __stdcall set_list_link_next_to_global(struct ListLink *link)
{
    link->next = (struct ListLink *)data_00543480;
    return 0;
}

int __stdcall match_tool(int *pair, int b, int c, unsigned char *out)
{
    *out = Targets_MatchTool(pair[0], pair[1], b, c);
    return 0;
}

unsigned int __stdcall store_boolean_result(int argument0, char **argument1, unsigned char *result)
{
    Boolean value;
    value = Targets_MatchCommandLineOptions(argument0, argument1);
    *result = (unsigned char)value;
    return 0U;
}

int __stdcall dispatch_plugin_request(struct CWPluginPrivateContext *context)
{
    int result = 0;
    long request;
    CWPluginsPrivate_GetRequest(context, &request);
    if (!(result = _Setjmp(plugin_request_jmp_buf))) {
        switch (request) {
            case -2:
                result = 0;
                break;
            case 1:
                if (!(result = initialize_cmdline_environment(context)))
                    result = fn_0040ba99(context);
                break;
            case 2:
                if (!(result = initialize_cmdline_environment(context)))
                    if (!(result = register_option_lists(context)))
                        if (!(result = run_tool_checks(context)))
                            result = set_enabled_link_parser_entries(context);
                break;
            case -1:
                break;
        }
    } else if (DAT_00543380 && result != 1)
        fprintf(stderr, "Unexpected error in %s [%d]\n", DAT_00543380, result);
    CWPluginsPrivate_ReturnArgument(context, result);
    return result;
}
