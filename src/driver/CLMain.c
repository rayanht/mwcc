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
#include "driver/Generic.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/Resources.h"
#include "driver/StringExtras.h"
#include "driver/cc-eabi-ppc-mw.h"
#include <string.h>
#include <setjmp.h>

Project mainProj;
Project *gProj = &mainProj;
PCmdLine optsCmdLine = {0x1002};
PCmdLineEnvir optsEnvir = {0x1000};
PCmdLineCompiler optsCompiler = {0x1004};
PCmdLineLinker optsLinker = {0x1002};
CLState clState;

/* The driver is itself a plugin ('cldr'): its descriptions are its getters' data. */

unsigned int __stdcall return_zero(unsigned int unused)
{
    return 0U;
}

static PluginDesc driver_flags = {2, 'cldr', 7, 0, 0, 11};

unsigned int __stdcall get_data_and_size(unsigned char **data, unsigned int *size)
{
    *data = (unsigned char *)&driver_flags;
    *size = 18U;
    return 0U;
}

static const char *driver_name = "Command-Line Driver";

unsigned int __stdcall copy_global_value_to_address(unsigned int destinationAddress)
{
    *(const char **)destinationAddress = driver_name;
    return 0;
}

static const char *driver_display_name = "Command-Line Driver";

unsigned int __stdcall copy_global_value(unsigned int valueAddress)
{
    *(const char **)valueAddress = driver_display_name;
    return 0;
}

static const char *panel_names[4];
static PluginDirectoryList panel_list = {1, 4, (char **)panel_names};

int __stdcall fn_0040a730(char **panelData)
{
    if (clState.plugintype == 'Comp') {
        panel_names[0] = "CmdLine Panel";
        panel_names[1] = "CmdLine Compiler Panel";
        panel_names[2] = "CmdLine Linker Panel";
        panel_list.count = 3;
    } else {
        panel_names[0] = "CmdLine Panel";
        panel_names[1] = "CmdLine Linker Panel";
        panel_list.count = 2;
    }
    *panelData = (char *)&panel_list;
    return 0;
}

static UInt32 target_cpu = '****';
static UInt32 target_os = '****';
static TargetInfo target_list = {1, 1, &target_cpu, 1, &target_os};

static PluginVersion driver_version = {3, 0, 0, 0};

unsigned int __stdcall fn_0040a7a0(struct ListLink *link)
{
    link->next = (struct ListLink *)&driver_version;
    return 0U;
}

static FileSignature file_signatures[2] = {{'Brws', "DubL", 4, 0}, {'MMPr', "looc", 4, 0}};
static FileSignatureList file_signature_list = {2, file_signatures};

unsigned int __stdcall set_listlink_next_to_global(struct ListLink *node)
{
    node->next = (struct ListLink *)&file_signature_list;
    return 0U;
}

static void *driver_callbacks[9] = {(void *)return_zero,
                                    (void *)get_data_and_size,
                                    (void *)copy_global_value,
                                    (void *)copy_global_value_to_address,
                                    (void *)fn_0040a730,
                                    NULL,
                                    NULL,
                                    (void *)fn_0040a7a0,
                                    (void *)set_listlink_next_to_global};

unsigned int fn_0040a7c0(void)
{
    ClientGlue_AddPlugin((PluginRequiredInputRecord *)driver_callbacks);
}

/* The "Command-line strings" (12000) the driver's messages index. */
static char *command_line_strings[107] = {
    "Could not get current working directory",
    "Cannot find my executable '%s'",
    "Could not initialize plugin '%s'",
    "The %s '%s' requires functionality not present in the command-line driver.",
    "The command-line parser does not support these panels:",
    "\t%s\n",
    "Compiling function: '%s'",
    "Could not write file '%s'",
    "Could not write file '%s' in directory '%s'",
    "Write error on output (errno=%ld)",
    "Current working directory is too long",
    "Unknown filetype '%c%c%c%c', defaulting to '%s'",
    "%s:\ttype %s",
    "Storing output for '%s' in '%s'",
    "Writing text file '%s'",
    "Writing %sobject file '%s'",
    "Writing browse data '%s'",
    "Could not write %s '%s' (error %ld)",
    "Deleting temporary file '%s'",
    "Could not resolve alias for '%s' (error %ld)",
    "%s:\t'%s'%s",
    "File '%s' has browse fileID %d",
    "Can't locate directory '%s'",
    "  %8.2f seconds to %s %s%s%s",
    "  %8d lines compiled",
    "  %8d %s code\n  %8d %s init'd data\n  %8d %s uninit'd data",
    "  %8d total %s code\n  %8d total %s init'd data\n  %8d total %s uninit'd data",
    "File '%s' is not compilable source, target object data, or command file; ignoring",
    "All specified files were ignored",
    "Compiling: '%s'",
    "Compiling: '%s' with '%s'",
    "Precompiling: '%s'",
    "Precompiling: '%s' with '%s'",
    "Preprocessing: '%s'",
    "Preprocessing: '%s' with '%s'",
    "Finding dependencies: '%s'",
    "Finding dependencies: '%s' with '%s'",
    "Importing: '%s'",
    "Importing: '%s' with '%s'",
    "Linking project",
    "Linking project with '%s'",
    "Pre-linking project",
    "Pre-linking project with '%s'",
    "Post-linking project",
    "Post-linking project with '%s'",
    "Disassembling: '%s'",
    "Disassembling: '%s' with '%s'",
    "Syntax checking: '%s'",
    "Syntax checking: '%s' with '%s'",
    "Getting target info from '%s'",
    "Initializing '%s'",
    "Terminating '%s'",
    "'%s' cannot preprocess, skipping '%s'",
    "'%s' cannot precompile, skipping '%s'",
    "'%s' cannot generate code, skipping '%s'",
    "'%s' has no object code to disassemble",
    "'%s' cannot disassemble, skipping '%s'",
    "Neither '%s' nor '%s' can disassemble, skipping '%s'",
    "No precompiled header name given, '%s' assumed",
    "Precompile target '%s' given on command line; source-specified name '%s' ignored",
    "Writing precompiled %s file '%s'",
    "Reading precompiled %s file '%s'",
    "Cannot %s memory for %s",
    "Files/directories must have length <= %ld characters;\n'%s' not accepted",
    "Guessed linker name '%s' from compiler name '%s'",
    "Can't find %s '%s' in path",
    "Calling %s '%s'",
    "Can't execute %s '%s' (%s)",
    "%s '%s' returned with exit code %d",
    "Too many errors printed, aborting program",
    "Too many errors printed, suppressing errors for current file",
    "Too many warnings printed, suppressing warnings for current file",
    "No %s mapping matches '%s' (unrecognized file contents or filename extension); treating as source text",
    "No plugin or target matches the file '%s', ignoring",
    "File '%s' cannot be handled by this tool, ignoring",
    "File '%s' does not match the active target",
    "Adding%s:\t'%s'",
    "Creating new overlay '%s' in group '%s'",
    "Creating new overlay group '%s' at addr %08X:%08X",
    "File '%s' cannot be added to overlay group; define an overlay in the group first",
    "Cannot create virtual export file '%s' (from '-export name,...')",
    "Too many %s defined; at most %d%s is allowed",
    "Loading preference panel '%s'",
    "%s:",
    "\t%s%s",
    "Already defined %s search path; '%s' added after other paths",
    "License check failed: %s",
    "The plugin '%s' (version '%s') cannot be used:\n%s",
    "Plugin '%s' has already been registered",
    "Preferences for '%s' have already been registered",
    "Preferences for '%s' not found",
    "Some preferences needed by the plugin '%s' have not been registered",
    "Could not load file '%s'",
    "Could not find change current working directory to '%s'",
    "Out of memory",
    "The tool did not produce any output while %s the file '%s'",
    "The filename '%s' is invalid",
    "The %slinker for this target was not found",
    "%s\n%s (OS error %d)",
    "%s\n%s (error %d)",
    "Note:  '%s' did not generate any browse information \nfor '%s'; no browser output generated",
    "Source and specified output for the file '%s' are identical; no output will be generated",
    "More than one output filename specified for '%s'; ignoring '%s'",
    "Pref panel data for '%s' is out-of-date or invalid",
    "Changing primary user access path to '%s'",
    "",
    NULL};

unsigned int fn_0040a7d0(void)
{
    return ClientGlue_AddResourceStrings("Command-line strings", 12000, command_line_strings);
}

Boolean invoke_if_requested(Boolean pre, char *argument)
{
    if (pre) {
        CLIO_ExchangeClearGlobal();
        return 1;
    }
    return 0;
}

Boolean latch_flag(Boolean pre, char *argument)
{
    if (pre) {
        clState.pluginDebug = 1;
        return 1;
    }
    return clState.pluginDebug;
}

Boolean is_enabled_or_global_nonzero(Boolean pre, char *argument)
{
    if (pre) {
        return 1;
    }
    return license_path != NULL;
}

/* The options the driver takes before its parser sees the command line; the first is named at run time. */
static struct DriverCommandLineOption driver_options[3] = {
    {"", NULL, invoke_if_requested},
    {"--use-license-file", &license_path, is_enabled_or_global_nonzero},
    {"--plugin-debug", NULL, latch_flag},
};

void consume_driver_command_line_options(int *argc, char ***argv)
{
    int optionIndex;
    struct DriverCommandLineOption *option;

    if (*argc <= 1) {
        return;
    }
    do {
        optionIndex = 0;
        option = driver_options;
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
    entry = driver_options;
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

static int did_init = 0;

int CLMain_Initialize(int argc, char **argv)
{
    static char secret[8];
    char buf[260];

    OS_InitProgram(&argc, &argv);
    memset(&clState, 0, sizeof(clState));
    driver_options[0].name = secret;
    secret[7] = 0;
    secret[4] = 'b';
    secret[1] = '-';
    secret[5] = 'u';
    secret[2] = 'd';
    secret[0] = '-';
    secret[3] = 'e';
    secret[6] = 'g';
    consume_driver_command_line_options(&argc, &argv);
    clState.argc = argc;
    clState.argv = argv;

    CLProj_CopyStringBounded(buf, *argv, strlen(*argv), sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    if (strlen(buf) < 4 || CLIO_CompareStringsIgnoreCase(buf + strlen(buf) - 4, ".exe") != 0)
        CLProj_AppendString(buf, ".exe", sizeof(buf));

    CLIO_InitializeStreamBuffering();
    fn_004098d0();
    CLProj_InitializeCWD(gProj);
    if (fn_0040a7d0() == 0)
        CLErrors_FatalError("Could not initialize resource strings");
    if (fn_0040a7c0() == 0)
        CLErrors_FatalError("Could not initialize built-in plugins");
    if (OS_IsFullPath(*argv) == 0)
        clState.programName = *argv;
    else
        clState.programName = OS_GetFileNamePtr(*argv);

    if (OS_FindProgram(buf, clState.programSpec.path.s) != 0)
        CLErrors_EmitDiagnostic(2, buf);
    Resources_OpenResourceFile(&clState.programSpec);
    fn_00417750();
    did_init = 1;
    return 0;
}

unsigned int CLMain_FreePlugins(unsigned int result)
{
    if (did_init != 0) {
        CLPlugins_FreePlugins();
        CLLicenses_ReleaseLicenses();
        CLProj_FreeTargets(gProj);
        fn_00414e20();
        did_init = 0;
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
    parser =
        CLPlugins_SelectPluginByRequestsAndValues(NULL, clState.parserplugin, plugin_request_count, requests, env->cpu,
                                                  env->os, unique_plugin_name_count, unique_plugin_names);
    if (parser == NULL)
        CLErrors_FatalError("Could not find a command-line parser!");

    info.argc = clState.argc;
    info.argv = clState.argv;
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

    if (clState.pluginDebug != 0) {
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

    data_0054bf48(NULL);
    if (optsCompiler.outMakefile[0] != '\0') {
        error = OS_MakeFileSpec(optsCompiler.outMakefile, &clState.makefileSpec);
        if (error > 0) {
            CLErrors_ReportOSError(8, error, optsCompiler.outMakefile);
            return 1;
        }
        OS_Delete(&clState.makefileSpec);
    }
    if (optsCompiler.sbmPath[0] != '\0') {
        error = OS_MakePathSpec(NULL, optsCompiler.sbmPath, &clState.sbmPathSpec);
        if (error > 0) {
            CLErrors_ReportOSError(0x17, error, optsCompiler.sbmPath);
            return 1;
        }
    } else {
        OS_GetCWD(&clState.sbmPathSpec);
    }
    return 0;
}

SInt32 fn_0040aed0(void)
{
    SInt32 start;
    SInt32 end;
    SInt32 r;

    if (+optsCmdLine.state == 0 || +optsCmdLine.state == 1)
        return 0;
    start = CLFileOps_GetScaledTicks();
    r = CLFileOps_CompileProject();
    if (r != 0)
        return r;
    if (optsCmdLine.state == 3) {
        r = CLFileOps_LinkProject();
        if (r != 0)
            return r;
    }
    end = CLFileOps_GetScaledTicks();
    if (optsCmdLine.timeWorking != 0)
        CLErrors_ForwardMessage(0x18, (end - start) / 60.0, "resolve", "", "project", "");
    return 0;
}

int fn_0040af70(PCmdLine *message, PCmdLine *result)
{
    if (message->version == 0x1002) {
        *result = *message;
        return 1;
    }
    if (message->version == 0x1001) {
        CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *result = *message;
        result->noCmdLineWarnings = 0;
        return 1;
    }
    if (message->version == 0x1000) {
        CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *result = *message;
        result->stderr2stdout = 1;
        return 1;
    }
    CLErrors_EmitDiagnostic(0x68, "CmdLine Panel");
    return 0;
}

unsigned int copy_environment_code_record(PCmdLineEnvir *record, PCmdLineEnvir *result)
{
    if (record->version == 4096U) {
        *result = *record;
        return 1U;
    }
    CLErrors_EmitDiagnostic(104U, "CmdLine Environment");
    return 0U;
}

int convert_command_line_panel_settings(PCmdLineCompiler *source, PCmdLineCompiler *destination)
{
    if (source->version == 0x1004) {
        *destination = *source;
        return 1;
    }
    if (source->version == 0x1003) {
        if (clState.pluginDebug)
            CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->canonicalIncludes = 1;
        return 1;
    }
    if (source->version == 0x1002) {
        if (clState.pluginDebug)
            CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->sbmState = 0;
        destination->sbmPath[0] = 0;
        return 1;
    }
    if (source->version == 0x1001) {
        if (clState.pluginDebug)
            CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->ignoreMissingFiles = 0;
        destination->printHeaderNames = 0;
        destination->sbmState = 0;
        destination->sbmPath[0] = 0;
        return 1;
    }
    if (source->version == 0x1000) {
        CLErrors_ForwardMessageArguments(0x68, "CmdLine Panel");
        *destination = *source;
        destination->forcePrecompile = 0;
        destination->ignoreMissingFiles = 0;
        destination->printHeaderNames = 0;
        destination->sbmState = 0;
        destination->sbmPath[0] = 0;
        return 1;
    }
    CLErrors_EmitDiagnostic(0x68, "CmdLine Compiler Panel");
    return 0;
}

int convert_linker_panel_settings(PCmdLineLinker *input, PCmdLineLinker *output)
{
    if (input->version == 0x1002) {
        *output = *input;
        return 1;
    }
    if (input->version == 0x1001) {
        *output = *input;
        output->callLinker = 1;
        return 1;
    }
    if (input->version == 0x1000) {
        *output = *input;
        output->callPreLinker = 1;
        output->callPostLinker = 1;
        output->keepLinkerOutput = 1;
        output->callLinker = 1;
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
            if (context && !((int (*)(void *, void *))fn_0040af70)((PCmdLine *)handle->data, &optsCmdLine))
                return 0;
        } else {
            CLErrors_EmitDiagnostic(91, "CmdLine Panel");
            return 0;
        }
    }
    if (!context || !ClientGlue_CompareLowercaseStrings(context, "CmdLine Environment")) {
        entry = CLPrefs_FindNameTableEntry("CmdLine Environment");
        if (entry && (handle = CLPrefs_CopyDestinationToTemporary(entry)) != NULL) {
            if (context &&
                !((int (*)(void *, void *))copy_environment_code_record)((PCmdLineEnvir *)handle->data, &optsEnvir))
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
                               (PCmdLineCompiler *)handle->data, &optsCompiler))
                return 0;
        } else {
            CLErrors_EmitDiagnostic(91, "CmdLine Compiler Panel");
            return 0;
        }
    }
    if (!context || !ClientGlue_CompareLowercaseStrings(context, "CmdLine Linker Panel")) {
        entry = CLPrefs_FindNameTableEntry("CmdLine Linker Panel");
        if (entry && (handle = CLPrefs_CopyDestinationToTemporary(entry)) != NULL) {
            if (context &&
                !((int (*)(void *, void *))convert_linker_panel_settings)((PCmdLineLinker *)handle->data, &optsLinker))
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
    entry = create_data_block("CmdLine Environment", &optsEnvir, 8U);
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
    default_target = CLTarg_CreateTarget("default", clState.cpu, clState.os, clState.language);
    CLTarg_AppendEntry(&gProj->targets, default_target);

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

    if (clState.plugintype == 0x4c696e6b && default_target->preLinker == NULL && default_target->linker == NULL &&
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
