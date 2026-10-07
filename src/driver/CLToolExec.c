#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLToolExec.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "driver/AssertionFailure.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLDependencies.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLPlugins.h"
#include "driver/CLSegs.h"
#include "driver/CLTarg.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
#include "driver/MemUtils.h"
#include "driver/MsDos.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Tool selection entry; fields not used here are retained as opaque bytes. */

/* Stored command arguments and optional additional argument vector. */

/* Tool execution record; only paths and flag words are used here. */

/* Storage for a dynamically resized array of words. */

/* Flag words in the records returned by the tool list. */

void CLToolExec_AppendArgument(int *count, char ***storage, const char *value)
{
    *storage = xrealloc("command-line arguments", *storage, (*count + 2U) << 2);
    (*storage)[(*count)++] = xstrdup(value);
}

int append_command_line_arguments(int count, char **strings, int *stored_count, char ***stored_strings)
{
    int i;
    int total;

    total = stored_count ? *stored_count : 0;

    *stored_strings = xrealloc("command-line arguments", *stored_strings, (total + count + 1) * sizeof(char *));

    if (strings != NULL) {
        for (i = 0; i < count; i++) {
            (*stored_strings)[total] = xstrdup(strings[i]);
            total++;
        }
    }

    (*stored_strings)[total] = NULL;

    if (stored_count != NULL)
        *stored_count = total;

    return 1;
}

int free_items(char **items)
{
    int index;

    for (index = 0; items[index] != NULL; index = index + 1) {
        free(items[index]);
    }
    free(items);
    return 1;
}

int build_tool_command_line(int flags, DropinFileRecord *tool, struct ToolCommandLine *arguments)
{
    int index;
    struct ToolArgumentSet *argument_set;
    char path[324];
    index = 0;
    if (plugin_request_count > 0) {
        do {
            if (plugin_requests[index].tag.signed_kind == 1281977963 &&
                (flags & 1207959552) == (plugin_requests[index].flags & 1207959552)) {
                break;
            }
            index = index + 1;
        } while (index < plugin_request_count);
    }
    if (index >= plugin_request_count) {
        CLIO_ReportAssertionFailure("x < numPlugins", "CLToolExec.c", 87);
    }
    arguments->argc = 1;
    arguments->argv = xmalloc("command-line arguments", 8);
    CLMain_AppendEnabledCommandLineOptions(&arguments->argc, &arguments->argv);
    argument_set = &tool_argument_sets[index];
    append_command_line_arguments(argument_set->count - 1, argument_set->arguments + 1, &arguments->argc,
                                  &arguments->argv);
    index = 0;
    if (argument_set->additional_arguments != NULL) {
        while (argument_set->additional_arguments[index] != NULL) {
            index = index + 1;
        }
    }
    arguments->envp = NULL;
    append_command_line_arguments(index, argument_set->additional_arguments, NULL, &arguments->envp);
    if (tool == NULL && (flags & 1073741824) == 0) {
        for (index = 0; index < CLFiles_GetIndex(&default_target->files); index = index + 1) {
            tool = CLFiles_FindFileByIndex(&default_target->files, index);
            if ((tool->inputArgumentMask & 2) != 0) {
                CLToolExec_AppendArgument(&arguments->argc, &arguments->argv, tool->inputName);
            }
            if ((tool->outputArgumentMask & 2) != 0) {
                if (CLFileOps_SetupOutputPath(tool, 2) != 0 && fn_00419e90(tool) != 0) {
                    CLToolExec_AppendArgument(&arguments->argc, &arguments->argv,
                                              OS_SpecToString((OSSpec *)&tool->outputPath, data_005880e0, 260));
                } else {
                    return 0;
                }
            }
        }
    } else if (tool != NULL) {
        arguments->argv = xrealloc("command-line arguments", arguments->argv, (arguments->argc + 1) << 2);
        if ((tool->inputArgumentMask & 2) != 0) {
            CLToolExec_AppendArgument(&arguments->argc, &arguments->argv, tool->inputName);
        }
        if ((tool->outputArgumentMask & 2) != 0 && CLFileOps_SetupOutputPath(tool, 2) != 0 && fn_00419e90(tool) != 0) {
            CLToolExec_AppendArgument(&arguments->argc, &arguments->argv,
                                      OS_SpecToString((OSSpec *)&tool->outputPath, data_005880e0, 260));
        }
    } else {
        if ((flags & 1073741824) != 0) {
            MacSpecs_MakeOSSpec(&default_target->settings->head.firstFile, path);
            CLToolExec_AppendArgument(&arguments->argc, &arguments->argv,
                                      OS_SpecToString((OSSpec *)path, data_005880e0, 260));
        }
    }
    arguments->argv[arguments->argc] = NULL;
    return 1;
}

unsigned int CLToolExec_SetTemporaryOutputMask(void)
{
    DropinFileRecord *record;
    int index = 0;
    while (index < CLFiles_GetIndex(&default_target->files)) {
        record = CLFiles_FindFileByIndex(&default_target->files, index);
        if ((record->outputArgumentMask & 2) != 0 && (optsCmdLine.toDisk & 2) == 0 && (record->outputMask & 2) == 0)
            record->temporaryOutputMask |= 2;
        ++index;
    }
    return 1;
}

/* Entry in the tool execution list; only the path and flag words are known. */
unsigned int CLToolExec_DeleteTemporaryOutputs(void)
{
    SInt32 index;
    DropinFileRecord *entry;
    index = 0;
    while (index < CLFiles_GetIndex(&default_target->files)) {
        entry = CLFiles_FindFileByIndex(&default_target->files, index);
        if ((entry->outputArgumentMask & 2) && (entry->temporaryOutputMask & 2)) {
            if (optsCmdLine.verbose > 1) {
                CLErrors_ForwardMessage(19U, OS_SpecToString(&entry->outputPath, data_005880e0, 260U));
            }
            OS_Delete(&entry->outputPath);
            entry->temporaryOutputMask &= ~2;
        }
        ++index;
    }
    return 1U;
}

int CLToolExec_ExecuteLinker(Plugin *tool, UInt32 flags, DropinFileRecord *argument, char *inputPath, char *outputPath)
{
    char *cursor;
    char *message;
    int result;
    int index;
    char *pluginName;
    char *toolName;
    char fallbackName[64];
    OSSpec toolPath;
    ToolCommandLine command;
    UInt32 status;

    if ((flags & 1) == 0)
        CLIO_ReportAssertionFailure("dropinflags & isExecutableTool", "CLToolExec.c", 0xf7);

    command.envp = NULL;
    command.argc = 0;
    command.argv = command.envp;

    if (flags & 0x8000000)
        message = "pre-linker";
    else if (flags & 0x40000000)
        message = "post-linker";
    else
        message = "linker";

    pluginName = CLPlugins_GetName(tool);
    toolName = pluginName;
    if ((flags & 0x48000000) == 0 && optsCompiler.linkerName[0] != 0)
        toolName = optsCompiler.linkerName;

    if (CLFileOps_FindExecutable(toolName, &toolPath) != 0) {
        strcpy(fallbackName, clState.programName);
        for (cursor = fallbackName; *cursor != 0; cursor++)
            *cursor = (char)tolower(*cursor);

        if ((cursor = strstr(fallbackName, "cc")) != NULL || (cursor = strstr(fallbackName, "pas")) != NULL ||
            (cursor = strstr(fallbackName, "asm")) != NULL) {
            if (*cursor != 'c') {
                memmove(cursor + 2, cursor + 3, strlen(cursor) - 3);
                cursor[strlen(cursor) - 1] = '\0';
            }
            cursor[0] = 'l';
            cursor[1] = 'd';
        } else {
            cursor = strstr(fallbackName, "mwc");
            if (cursor != NULL) {
                char *suffix = cursor + 2;
                memmove(suffix + 3, cursor + 2, strlen(cursor + 2));
                cursor += 2;
                memcpy(cursor, "Link", 4);
            }
        }

        if (CLFileOps_FindExecutable(fallbackName, &toolPath) == 0) {
            char *resolvedName = fallbackName;
            if (optsCmdLine.verbose != 0)
                CLErrors_ForwardMessage(0x41, resolvedName, clState.programName);
        } else {
            CLErrors_EmitDiagnostic(0x42, message, toolName);
            return 0;
        }
    }

    result = 1;
    build_tool_command_line(flags, argument, &command);
    cursor = OS_SpecToString(&toolPath, data_005880e0, 0x104);
    command.argv[0] = xstrdup(cursor);
    command.argv[command.argc] = NULL;

    if (optsCmdLine.verbose != 0 || optsCmdLine.dryRun != 0) {
        CLIO_FormatAndDispatchText("Command line:\n");
        for (index = 0; index < command.argc && command.argv[index] != NULL; index++) {
            if (strchr(command.argv[index], ' ') != NULL)
                CLIO_FormatAndDispatchText("\"%s\" ", command.argv[index]);
            else
                CLIO_FormatAndDispatchText("%s ", command.argv[index]);
        }
        CLIO_FormatAndDispatchText("\n");
    }

    fflush(stdout);
    fflush(stderr);

    if (optsCmdLine.verbose != 0) {
        cursor = OS_SpecToString(&toolPath, data_005880e0, 0x104);
        CLErrors_ForwardMessage(0x43, message, cursor);
    }

    if (optsCmdLine.dryRun == 0) {
        int executionError = OS_Execute(&toolPath, command.argv, command.envp, inputPath, outputPath, &status);
        if (executionError != 0) {
            char *errorDetail = OS_GetErrText(executionError);
            CLErrors_EmitDiagnostic(0x44, message, command.argv[0], errorDetail);
            result = 0;
        } else if (status != 0) {
            CLErrors_EmitDiagnostic(0x45, message, command.argv[0], status);
            result = 0;
        }
    }

    free_items(command.argv);
    free_items(command.envp);
    return result;
}
