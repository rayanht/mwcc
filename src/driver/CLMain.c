#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLMain.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/win32.h"
#include "compiler/CPrep.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLBrowser.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#include "driver/CLDependencies.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLLicenses.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLPlugins.h"
#include "driver/CLPrefs.h"
#include "driver/CLProj.h"
#include "driver/CLSegs.h"
#include "driver/CLTarg.h"
#include "driver/CLToolExec.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ClientGlue.h"
#include "driver/Files.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/Resources.h"
#include "driver/cc-eabi-ppc-mw.h"
#include <string.h>
#include <setjmp.h>
unsigned int __stdcall copy_global_value_to_address(unsigned int destinationAddress)
{
    *(SInt32 *)destinationAddress = data_00541e44;
    return 0;
}

unsigned int __stdcall copy_global_value(unsigned int valueAddress)
{
    *(SInt32 *)valueAddress = data_00541e48;
    return 0;
}

int __stdcall fn_0040a730(char **panelData)
{
    if (plugin_type == 0x436f6d70) {
        DAT_0057d920 = "CmdLine Panel";
        DAT_0057d924 = "CmdLine Compiler Panel";
        DAT_0057d928 = "CmdLine Linker Panel";
        DAT_00541e4e = 3;
    } else {
        DAT_0057d920 = "CmdLine Panel";
        DAT_0057d924 = "CmdLine Linker Panel";
        DAT_00541e4e = 2;
    }
    *panelData = DAT_00541e4c;
    return 0;
}

unsigned int __stdcall fn_0040a7a0(struct ListLink *link)
{
    link->next = &data_00541eac;
    return 0U;
}

unsigned int __stdcall set_listlink_next_to_global(struct ListLink *node)
{
    node->next = &data_00541edc;
    return 0U;
}

unsigned int fn_0040a7c0(void)
{
    ClientGlue_AddPlugin((PluginRequiredInputRecord *)data_00541ee4);
}

unsigned int fn_0040a7d0(void)
{
    return ClientGlue_AddResourceStrings("Command-line strings", 12000, data_00542f3c);
}

unsigned char invoke_if_requested(char shouldInvoke)
{
    if (shouldInvoke != '\0') {
        CLIO_ExchangeClearGlobal();
        return 1;
    }
    return 0;
}

unsigned char latch_flag(unsigned int flag)
{
    if ((unsigned char)flag != 0) {
        DAT_00587324 = 1;
        return 1;
    }
    return DAT_00587324;
}

Boolean is_enabled_or_global_nonzero(char enabled)
{
    if (enabled != '\0') {
        return 1;
    }
    return license_path != NULL;
}

void consume_driver_command_line_options(int *argc, char ***argv)
{
    int optionIndex;
    struct DriverCommandLineOption *option;

    if (*argc <= 1) {
        return;
    }
    do {
        optionIndex = 0;
        option = PTR_DAT_00543124;
        do {
            if (strcmp(option->name, (*argv)[1]) == 0) {
                if (option->value != NULL) {
                    *option->value = (*argv)[2];
                    option->handler(1, *option->value);
                    (*argv)[1] = (*argv)[0];
                    --*argc;
                    (*argv)++;
                    (*argv)[1] = (*argv)[0];
                    --*argc;
                    (*argv)++;
                } else {
                    option->handler(1, NULL);
                    (*argv)[1] = (*argv)[0];
                    --*argc;
                    (*argv)++;
                }
                break;
            }
            ++optionIndex;
            ++option;
        } while (optionIndex < 3);
    } while ((*argc > 1) && (optionIndex < 3));
}

void CLMain_AppendEnabledCommandLineOptions(int *first, char ***second)
{
    char enabled;
    int index;
    struct DriverCommandLineOption *entry;

    index = 0;
    entry = PTR_DAT_00543124;
    do {
        enabled = entry->handler(0, NULL);
        if (enabled != '\0') {
            CLToolExec_AppendArgument(first, second, entry->name);
            if (entry->value != NULL) {
                CLToolExec_AppendArgument(first, second, *entry->value);
            }
        }
        index = 1 + index;
        entry = entry + 1;
    } while (index < 3);
}

int CLMain_Initialize(int argc, char **argv)
{
    char buf[260];
    char *path;
    CommandParseInfo *commandLine;

    fn_004111c0(&argc, &argv);
    memset(&data_005871bc, 0,
           sizeof(CommandParseInfo) + 4 * sizeof(SInt32) + 2 * sizeof(OSSpec) + sizeof(program_name) +
               2 * sizeof(short) + 4 * sizeof(Boolean) + sizeof(CLTargetDirectory) + sizeof(MemBuffer));
    PTR_DAT_00543124[0].name = data_0057d930;
    data_0057d930[7] = 0;
    data_0057d930[4] = 'b';
    data_0057d930[1] = '-';
    data_0057d930[5] = 'u';
    data_0057d930[2] = 'd';
    data_0057d930[0] = '-';
    data_0057d930[3] = 'e';
    data_0057d930[6] = 'g';
    consume_driver_command_line_options(&argc, &argv);
    data_005871bc = argc;
    commandLine = (CommandParseInfo *)&data_005871bc;
    commandLine->argv = argv;

    CLProj_CopyStringBounded(buf, *argv, strlen(*argv), sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    if (strlen(buf) < 4 || CLIO_CompareStringsIgnoreCase(buf + strlen(buf) - 4, ".exe") != 0)
        CLProj_AppendString(buf, ".exe", sizeof(buf));

    CLIO_InitializeStreamBuffering();
    fn_004098d0();
    CLProj_InitializeCWD(PTR_DAT_00541b18);
    if (fn_0040a7d0() == 0)
        CLErrors_FatalError("Could not initialize resource strings");
    if (fn_0040a7c0() == 0)
        CLErrors_FatalError("Could not initialize built-in plugins");
    if (MsDos_IsAbsolutePath(*argv) == 0)
        program_name = *argv;
    else
        program_name = CLProj_GetFileName(*argv);

    path = data_005871d8.directory.path;
    if (CLFileOps_FindExecutable(buf, path) != 0)
        CLErrors_EmitDiagnostic(2, buf);
    Resources_OpenResourceFile(path);
    fn_00417750();
    DAT_00543148 = 1;
    return 0;
}

unsigned int CLMain_FreePlugins(unsigned int result)
{
    if (DAT_00543148 != 0) {
        CLPlugins_FreePlugins();
        CLLicenses_ReleaseLicenses();
        CLProj_FreeTargets(PTR_DAT_00541b18);
        fn_00414e20();
        DAT_00543148 = 0;
    }
    return result;
}

int parse_command_line(void)
{
    SInt32 recordIndex;
    PluginRequest *requests;
    CommandParseInfo info;
    Plugin *parser;
    struct CLTarget *env;
    SInt32 argumentIndex, fileIndex;

    if (CLPlugins_BuildPluginRequests(NULL, &plugin_request_count, &plugin_requests) == 0 ||
        CLPlugins_GetUniquePluginNames(NULL, &unique_plugin_name_count, &unique_plugin_names) == 0 ||
        (file_argument_sets = xcalloc(NULL, unique_plugin_name_count * 12)) == NULL ||
        (tool_argument_sets = xcalloc(NULL, plugin_request_count * 12)) == NULL)
        CLErrors_FatalError("Out of memory during init\n");

    env = default_target;
    requests = plugin_requests;
    parser = CLPlugins_SelectPluginByRequestsAndValues(NULL, data_005871d4, plugin_request_count, requests, env->cpu,
                                                       env->os, unique_plugin_name_count, unique_plugin_names);
    if (parser == NULL)
        CLErrors_FatalError("Could not find a command-line parser!");

    info.argc = data_005871bc;
    info.argv = (char **)data_005871c0;
    info.value2 = 0;
    env = default_target;
    if (!CLPluginRequests_ParseCommandLine(parser, env, &info, env->cpu, env->os, plugin_request_count, plugin_requests,
                                           unique_plugin_name_count, unique_plugin_names, tool_argument_sets,
                                           file_argument_sets, &input_name, &output_name)) {
        UInt8 status;
        if (fn_004151f0())
            status = 2;
        else
            status = 1;
        return status;
    }

    if (DAT_00587324 != 0) {
        for (fileIndex = 0; fileIndex < unique_plugin_name_count; fileIndex++) {
            CLIO_WriteFormattedText("Outgoing args for '%s':  ", unique_plugin_names[fileIndex]);
            for (argumentIndex = 0; argumentIndex < file_argument_sets[fileIndex].count; argumentIndex++)
                CLIO_WriteFormattedText("%s ", file_argument_sets[fileIndex].arguments[argumentIndex]);
            CLIO_WriteFormattedText("\r\n");
        }
        recordIndex = 0;
        for (; recordIndex < plugin_request_count; recordIndex++) {
            SInt32 secondCode = plugin_requests[recordIndex].secondCode;
            if (secondCode == 0)
                secondCode = '----';
            CLIO_WriteFormattedText(
                "Outgoing args for '%c%c%c%c/%c%c%c%c':  ", (plugin_requests[recordIndex].tag.kind & 0xFF000000) >> 24,
                (plugin_requests[recordIndex].tag.kind & 0xFF0000) >> 16,
                (plugin_requests[recordIndex].tag.kind & 0xFF00) >> 8, plugin_requests[recordIndex].tag.kind & 0xFF,
                (secondCode & 0xFF000000) >> 24, (secondCode & 0xFF0000) >> 16, (secondCode & 0xFF00) >> 8,
                secondCode & 0xFF);
            for (argumentIndex = 1; argumentIndex < tool_argument_sets[recordIndex].count; argumentIndex++)
                CLIO_WriteFormattedText("%s ", tool_argument_sets[recordIndex].arguments[argumentIndex]);
            CLIO_WriteFormattedText("\r\n");
        }
    }
    return 0;
}

int delete_file_and_make_path_spec(void)
{
    DWORD error;
    UInt8 *firstName;
    OSSpec *resolvedName;
    char *secondName;

    data_0054bf48(NULL);
    if (DAT_00541c0c != '\0') {
        resolvedName = &DAT_00587328;
        firstName = &DAT_00541c0c;
        error = OS_MakeFileSpec((const char *)firstName, resolvedName);
        if (error > 0) {
            CLErrors_ReportOSError(8, error, firstName);
            return 1;
        }
        OS_Delete(resolvedName);
    }
    if (data_00541d10[0] != '\0') {
        secondName = data_00541d10;
        error = OS_MakePathSpec(NULL, secondName, precompiled_unit_directory.path);
        if (error > 0) {
            CLErrors_ReportOSError(0x17, error, secondName);
            return 1;
        }
    } else {
        OS_GetCWD(precompiled_unit_directory.path);
    }
    return 0;
}

SInt32 fn_0040aed0(void)
{
    SInt32 start;
    SInt32 end;
    SInt32 r;

    if (+data_00541b1e == 0 || +data_00541b1e == 1)
        return 0;
    start = CLFileOps_GetScaledTicks();
    r = CLFileOps_CompileProject();
    if (r != 0)
        return r;
    if (data_00541b1e == 3) {
        r = CLFileOps_LinkProject();
        if (r != 0)
            return r;
    }
    end = CLFileOps_GetScaledTicks();
    if (data_00541b2b != 0)
        CLErrors_ForwardMessage(0x18, (double)(end - start) * data_00543248, "resolve", &data_00542f38, "project",
                                &data_00542f38);
    return 0;
}

int fn_0040af70(struct MessageRecord *message, struct MessageRecord *result)
{
    if (message->tag == 0x1002) {
        *result = *message;
        return 1;
    }
    if (message->tag == 0x1001) {
        CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *result = *message;
        result->attributes[2] = 0;
        return 1;
    }
    if (message->tag == 0x1000) {
        CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *result = *message;
        result->attributes[1] = 1;
        return 1;
    }
    CLErrors_EmitDiagnostic(0x68, "CmdLine Panel");
    return 0;
}

unsigned int copy_environment_code_record(struct CodeRecord *record, struct CodeRecord *result)
{
    if (record->code == 4096U) {
        *result = *record;
        return 1U;
    }
    CLErrors_EmitDiagnostic(104U, "CmdLine Environment");
    return 0U;
}

int convert_command_line_panel_settings(struct CommandLinePanelSettings *source,
                                        struct CommandLinePanelSettings *destination)
{
    if (source->version == 0x1004) {
        *destination = *source;
        return 1;
    }
    if (source->version == 0x1003) {
        if (DAT_00587324)
            CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->version1004Option = 1;
        return 1;
    }
    if (source->version == 0x1002) {
        if (DAT_00587324)
            CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->version1003Option1 = 0;
        destination->version1003Option2 = 0;
        return 1;
    }
    if (source->version == 0x1001) {
        if (DAT_00587324)
            CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->version1002Option1 = 0;
        destination->version1002Option2 = 0;
        destination->version1003Option1 = 0;
        destination->version1003Option2 = 0;
        return 1;
    }
    if (source->version == 0x1000) {
        CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->version1001Option = 0;
        destination->version1002Option1 = 0;
        destination->version1002Option2 = 0;
        destination->version1003Option1 = 0;
        destination->version1003Option2 = 0;
        return 1;
    }
    CLErrors_EmitDiagnostic(0x68, "CmdLine Compiler Panel");
    return 0;
}

int convert_linker_panel_settings(LinkerPanelSettings *input, LinkerPanelSettings *output)
{
    if (input->kind == 0x1002) {
        *output = *input;
        return 1;
    }
    if (input->kind == 0x1001) {
        *output = *input;
        output->flag5 = 1;
        return 1;
    }
    if (input->kind == 0x1000) {
        *output = *input;
        output->flag2 = 1;
        output->flag3 = 1;
        output->flag4 = 1;
        output->flag5 = 1;
        return 1;
    }
    CLErrors_EmitDiagnostic(0x68, "CmdLine Linker Panel");
    return 0;
}

unsigned int check_cmdline_entries(char *context)
{
    NameTableEntry *entry;
    StorageHandle *handle;
    if (!context || !ClientGlue_CompareLowercaseStrings(context, "CmdLine Panel")) {
        entry = CLPrefs_FindNameTableEntry("CmdLine Panel");
        if (entry && (handle = CLPrefs_CopyDestinationToTemporary(entry)) != NULL) {
            if (context &&
                !((int (*)(void *, void *))fn_0040af70)((struct MessageRecord *)handle->data, &data_00541b1c))
                return 0;
        } else {
            CLErrors_EmitDiagnostic(91, "CmdLine Panel");
            return 0;
        }
    }
    if (!context || !ClientGlue_CompareLowercaseStrings(context, "CmdLine Environment")) {
        entry = CLPrefs_FindNameTableEntry("CmdLine Environment");
        if (entry && (handle = CLPrefs_CopyDestinationToTemporary(entry)) != NULL) {
            if (context && !((int (*)(void *, void *))copy_environment_code_record)((struct CodeRecord *)handle->data,
                                                                                    &cmdline_code_record))
                return 0;
        } else {
            CLErrors_EmitDiagnostic(91, "CmdLine Environment");
            return 0;
        }
    }
    if (!context || !ClientGlue_CompareLowercaseStrings(context, "CmdLine Compiler Panel")) {
        entry = CLPrefs_FindNameTableEntry("CmdLine Compiler Panel");
        if (entry && (handle = CLPrefs_CopyDestinationToTemporary(entry)) != NULL) {
            if (context && !((int (*)(void *, void *))convert_command_line_panel_settings)(
                               (CommandLinePanelSettings *)handle->data, &data_00541b40))
                return 0;
        } else {
            CLErrors_EmitDiagnostic(91, "CmdLine Compiler Panel");
            return 0;
        }
    }
    if (!context || !ClientGlue_CompareLowercaseStrings(context, "CmdLine Linker Panel")) {
        entry = CLPrefs_FindNameTableEntry("CmdLine Linker Panel");
        if (entry && (handle = CLPrefs_CopyDestinationToTemporary(entry)) != NULL) {
            if (context && !((int (*)(void *, void *))convert_linker_panel_settings)(
                               (LinkerPanelSettings *)handle->data, &linker_panel_settings))
                return 0;
        } else {
            CLErrors_EmitDiagnostic(91, "CmdLine Linker Panel");
            return 0;
        }
    }
    return 1;
}

unsigned int create_cmdline_data_blocks(void)
{
    Boolean result;
    unsigned int combined;
    unsigned int enabled;
    unsigned int prerequisites;
    NameTableEntry *entry;

    data_0054bf48 = check_cmdline_entries;
    entry = create_data_block("CmdLine Environment", &cmdline_code_record, 8U);
    result = CLPrefs_AddPrefPanel(entry);
    combined = result;
    enabled = 0U;
    prerequisites = 0U;
    entry = create_data_block("CmdLine Panel", NULL, 28U);
    if (CLPrefs_AddPrefPanel(entry) != 0) {
        entry = create_data_block("CmdLine Compiler Panel", NULL, 722U);
        if (CLPrefs_AddPrefPanel(entry) != 0)
            prerequisites = 1U;
    }
    if (prerequisites != 0) {
        entry = create_data_block("CmdLine Linker Panel", 0U, 6U);
        if (CLPrefs_AddPrefPanel(entry) != 0)
            enabled = 1U;
    }
    combined |= enabled;
    return combined;
}

int create_default_target(void)
{
    default_target = CLTarg_CreateTarget("default", data_005871c4, data_005871c8, data_005871d0);
    CLTarg_AppendEntry(PTR_DAT_00541b18, default_target);

    CLPlugins_DispatchArgumentToPlugins(NULL, 0, default_target->cpu, default_target->os);

    default_target->linker = CLPlugins_FindLinkerPlugin(NULL, default_target->cpu, default_target->os);
    if (default_target->linker) {
        default_target->linkerFlags = CLPlugins_GetPluginDesc(default_target->linker)->flags;
    } else {
        if (CLPlugins_FindLinkerPlugin(NULL, 0x2a2a2a2a, 0x2a2a2a2a))
            CLErrors_FatalError("A linker is compiled in, but does not match the static target!\n");
        default_target->linkerFlags = 0;
    }

    default_target->preLinker = CLPlugins_FindLinkPluginForTarget(NULL, default_target->cpu, default_target->os);
    if (default_target->preLinker) {
        default_target->preLinkerFlags = CLPlugins_GetPluginDesc(default_target->preLinker)->flags;
    } else {
        if (CLPlugins_FindLinkPluginForTarget(NULL, 0x2a2a2a2a, 0x2a2a2a2a))
            CLErrors_FatalError("A pre-linker is compiled in, but does not match the static target!\n");
        default_target->preLinkerFlags = 0;
    }

    default_target->postLinker = CLPlugins_FindMatchingLinkPlugin(NULL, default_target->cpu, default_target->os);
    if (default_target->postLinker) {
        default_target->postLinkerFlags = CLPlugins_GetPluginDesc(default_target->postLinker)->flags;
    } else {
        if (CLPlugins_FindMatchingLinkPlugin(NULL, 0x2a2a2a2a, 0x2a2a2a2a))
            CLErrors_FatalError("A post-linker is compiled in, but does not match the static target!\n");
        default_target->postLinkerFlags = 0;
    }

    if (plugin_type == 0x4c696e6b && default_target->preLinker == NULL && default_target->linker == NULL &&
        default_target->postLinker == NULL)
        CLErrors_FatalError("The linker plugin was not found!");

    default_target->linkage = 0;
    return 1;
}

unsigned int CLMain_InitializeAndParseCommandLine(void)
{
    unsigned int result;

    result = setjmp(driver_jmp_buf);
    if (!result) {
        if (!create_cmdline_data_blocks())
            CLErrors_FatalError("Could not initialize preferences");
        create_default_target();

        (result = parse_command_line()) || ((result = delete_file_and_make_path_spec()) || (result = fn_0040aed0()));
    } else {
        result = 1U;
    }
    return result;
}

#pragma auto_inline reset
