#define CERROR_FILE "unknown.c"
#pragma bool off

#include "compiler/common.h"
#include "driver/CLPluginRequests.h"
#include "driver/AssertionFailure.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLLicenses.h"
#include "driver/CLMain.h"
#include "driver/CLOverlays.h"
#include "driver/CLPlugins.h"
#include "driver/CLTarg.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacFileTypes.h"
#include "driver/MacSpecs.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
extern "C" {

#include "mwcc/Plugins.h"

#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

extern "C" {
void initialize_plugin_request(Plugin *owner, int phase)
{
    PluginOutputItem *item;
    struct CLTarget *target;
    OSSpec text;
    OSPathSpec name;

    item = (PluginOutputItem *)owner->object->shellContext;
    item->owner = owner;
    item->initializationFlag = 1;
    item->initializationFlagCopy = item->initializationFlag;
    owner->object->request = phase;
    owner->object->apiVersion = 11;
    owner->object->pluginStorage = 0;
    OS_MakeFileSpec("Project.mcp", &text);
    OS_OSSpec_To_FSSpec(&text, &owner->object->sourcefile);
    if (default_target != 0) {
        target = default_target;
        OS_MakeSpecWithPath(&target->outputDirectory, 0, 0, &text);
        OS_OSSpec_To_FSSpec(&text, &owner->object->targetfile);
    } else {
        OS_GetCWD(&name);
        OS_MakeSpecWithPath(&name, 0, 0, &text);
        OS_OSSpec_To_FSSpec(&text, &owner->object->targetfile);
    }
    owner->object->shellSignature = 0x43574945;
    owner->object->contextSignature = (void *)CLPlugins_GetPluginDesc(owner)->type;
    if (default_target != 0) {
        target = default_target;
        owner->object->numFiles = CLFiles_GetIndex(&target->files);
    } else {
        owner->object->numFiles = 0;
    }
    if (default_target != 0) {
        target = default_target;
        owner->object->numOverlayGroups = CLOverlays_CountGroups(&target->overlays);
    } else {
        owner->object->numOverlayGroups = 0;
    }
    owner->object->callbackOSError = owner->object->value_ae = 0;
    owner->object->callbackCache = 0;
}

Boolean CLPluginRequests_ParseCommandLine(Plugin *func, struct CLTarget *context, struct CommandParseInfo *value1,
                                          SInt32 value2, SInt32 value3, SInt32 value4, void *value5, SInt32 value6,
                                          char **value7, struct ToolArgumentSet *value8, ToolArgumentSet *value9,
                                          void *value10, void *value11)
{
    CWPluginPrivateContext *state;
    SInt32 contextValue;
    Boolean selectedValue;

    initialize_plugin_request(func, 1);
    state = (CWPluginPrivateContext *)func->object;
    state->contextData.generation.value10 = value10;
    state->contextData.generation.value11 = value11;
    state->requestData.commandParseInfo = value1;
    state->contextData.generation.value2 = value2;
    state->contextData.generation.value3 = value3;
    state->contextData.generation.value4 = value4;
    state->contextData.generation.value5 = value5;
    state->contextData.generation.value6 = value6;
    state->contextData.generation.value7 = value7;
    state->contextData.generation.toolArguments = value8;
    state->contextData.generation.argumentRecord = value9;
    if (MemUtils_CallPluginEntry(func) != 0)
        return 0;
    if (!CLPluginRequests_InitializeTargetSettings(context, context->linker, context->linkerFlags))
        return 0;
    contextValue = context->settings->head.linkage;
    if (contextValue == 1)
        selectedValue = 1;
    else if (contextValue == 2)
        selectedValue = 2;
    else
        selectedValue = 0;
    context->linkage = selectedValue;
    if (!fn_004098a0(func))
        return 0;
    initialize_plugin_request(func, 2);
    state->requestData.commandParseInfo = value1;
    state->contextData.generation.value2 = value2;
    state->contextData.generation.value3 = value3;
    state->contextData.generation.value4 = value4;
    state->contextData.generation.value5 = value5;
    state->contextData.generation.value6 = value6;
    state->contextData.generation.value7 = value7;
    state->contextData.generation.toolArguments = value8;
    state->contextData.generation.argumentRecord = value9;
    return MemUtils_CallPluginEntry(func) == 0;
}

Boolean CLPluginRequests_SetupFileRequest(Plugin *job, DropinFileRecord *input, short flags)
{
    int operationFlags;
    int mode;
    mode = operationFlags = flags;
    mode &= 2;
    Boolean enabled;
    SInt16 result;
    CWPluginPrivateContext *ctx;
    if (mode) {
        unsigned char setting;
        if ((setting = optsCompiler.forcePrecompile) == 1)
            enabled = 1;
        else if (setting == 2)
            enabled = 0;
        else
            enabled = (input->fileFlags & 0x80000000) != 0;
    } else
        enabled = 0;
    if (optsCmdLine.verbose) {
        char *messageArgument = CLPlugins_GetName(job);
        int extra = optsCmdLine.verbose > 1;
        if (operationFlags & 1)
            CLErrors_ForwardMessage(extra + 34, input->inputName, messageArgument);
        else if (mode) {
            if (enabled)
                CLErrors_ForwardMessage(extra + 32, input->inputName, messageArgument);
            else
                CLErrors_ForwardMessage(extra + 30, input->inputName, messageArgument);
        } else if (operationFlags & 8)
            CLErrors_ForwardMessage(extra + 36, input->inputName, messageArgument);
        else if (operationFlags & 4)
            CLErrors_ForwardMessage(extra + 46, input->inputName, messageArgument);
        else if (!flags)
            CLErrors_ForwardMessage(extra + 48, input->inputName, messageArgument);
    }
    if (!fn_004098a0(job))
        return 0;
    initialize_plugin_request(job, 0);
    ctx = job->object;
    *ctx->targetSettings = *default_target->settings;
    OS_OSSpec_To_FSSpec(&input->inputPath, &ctx->contextData.payload);
    if (UCBGetFileText(ctx, &ctx->contextData.payload, &ctx->callbackValue, &ctx->callbackFlags, &result))
        return 0;
    ctx->requestData.fileIndex = input->listEntry.index;
    ctx->setting = optsCmdLine.debugInfo;
    ctx->dependencyStatusNegative = input->dependencyStatusNegative;
    ctx->dependencyOption = input->dependencyOption;
    memcpy(&ctx->dependencyState, &input->dependencyState, sizeof(ctx->dependencyState));
    if (operationFlags & 9) {
        if (operationFlags & 1)
            ctx->operation = 1;
        else
            ctx->operation = 2;
    } else
        ctx->operation = 0;
    if (mode && enabled) {
        ctx->enabled = 1;
        ctx->active = 1;
    } else {
        ctx->enabled = 0;
        ctx->active = 0;
    }
    if (!(operationFlags & 11)) {
        ctx->operation = 2;
        ctx->enabled = 2;
    }
    return MemUtils_CallPluginEntry(job) == 0;
}

Boolean CLPluginRequests_InitializeTargetSettings(CLTarget *input, Plugin *plugin, UInt32 flags)
{
    OSSpec path;
    CWFileSpec file;
    SInt32 result;
    CLTarget *requestInput = (CLTarget *)input;

    if (plugin != 0 && (flags & 1) == 0) {
        CWPluginPrivateContext *pluginData;

        if (optsCmdLine.verbose > 1) {
            char *pluginValue = CLPlugins_GetName(plugin);
            CLErrors_ForwardMessage(0x32, pluginValue);
        }
        if (CLPlugins_GetType(plugin) != 0x4c696e6b) {
            CLIO_ReportAssertionFailure("Plugin_GetPluginType(linker) == CWDROPINLINKERTYPE", "CLPluginRequests.cpp",
                                        0x135);
        }
        if (!fn_004098a0(plugin)) {
            return 0;
        }
        initialize_plugin_request(plugin, 2);
        pluginData = plugin->object;
        *pluginData->targetSettings = *(struct TgtRec *)requestInput->settings;
        result = MemUtils_CallPluginEntry(plugin);
        *(struct TgtRec *)requestInput->settings = *pluginData->targetSettings;
    } else {
        OS_MakeFileSpec("(unknown file)", &path);
        OS_OSSpec_To_FSSpec(&path, &file);
        ((struct TgtRec *)requestInput->settings)->head.tag = 1;
        ((struct TgtRec *)requestInput->settings)->head.firstFile = file;
        ((struct TgtRec *)requestInput->settings)->head.secondFile = file;
        ((struct TgtRec *)requestInput->settings)->head.thirdFile = file;
        ((struct TgtRec *)requestInput->settings)->head.linkage = 0;
        ((struct TgtRec *)requestInput->settings)->head.firstByte = 0;
        ((struct TgtRec *)requestInput->settings)->head.secondByte = 0;
        ((struct TgtRec *)requestInput->settings)->head.platformCodes[0] = requestInput->cpu;
        ((struct TgtRec *)requestInput->settings)->head.platformCodes[1] = requestInput->os;
        ((struct TgtRec *)requestInput->settings)->head.platformCodes[2] = 0x43574945;
        ((struct TgtRec *)requestInput->settings)->head.platformCodes[3] = 0x4150504c;
        ((struct TgtRec *)requestInput->settings)->head.thirdCode = 0x4d574442;
        ((struct TgtRec *)requestInput->settings)->head.fourthCode = 0x3f3f3f3f;
        result = 0;
    }
    return result == 0;
}

Boolean CLPluginRequests_UpdateTargetSettings(Plugin *record, UInt32 flags, struct TgtRec *snapshot)
{
    if (optsCmdLine.verbose != 0) {
        char *diagnosticArg = CLPlugins_GetName(record);
        SInt32 diagnosticVariant = (optsCmdLine.verbose > 1);
        if (flags & 0x40000000)
            CLErrors_ForwardMessage(diagnosticVariant + 0x2c, diagnosticArg);
        else if (flags & 0x8000000)
            CLErrors_ForwardMessage(diagnosticVariant + 0x2a, diagnosticArg);
        else
            CLErrors_ForwardMessage(diagnosticVariant + 0x28, diagnosticArg);
    }
    if (fn_004098a0(record) == 0)
        return 0;
    initialize_plugin_request(record, 0);
    {
        CWPluginPrivateContext *storage = record->object;
        SInt32 result;
        *storage->targetSettings = *snapshot;
        result = MemUtils_CallPluginEntry(record);
        *snapshot = *storage->targetSettings;
        return (result == 0);
    }
}
}

UInt8 CLPluginRequests_CallPluginForFile(Plugin *plugin, DropinFileRecord *context)
{
    Plugin *selectedPlugin;
    int result;

    if (plugin == 0) {
        selectedPlugin = context->selectedPlugin;
    } else {
        selectedPlugin = plugin;
    }
    if (optsCmdLine.verbose != 0) {
        CLErrors_ForwardMessage((1 < optsCmdLine.verbose) + 0x2e, context->inputName, CLPlugins_GetName(plugin));
    }
    if ((UInt8)fn_004098a0(selectedPlugin) == 0) {
        return 0;
    }
    if (plugin == 0) {
        initialize_plugin_request(selectedPlugin, 2);
        selectedPlugin->object->requestData.fileIndex = context->listEntry.index;
        result = MemUtils_CallPluginEntry(selectedPlugin);
    } else {
        initialize_plugin_request(selectedPlugin, 1);
        selectedPlugin->object->requestData.fileIndex = context->listEntry.index;
        result = MemUtils_CallPluginEntry(selectedPlugin);
    }
    return (result == 0);
}

extern "C" int fn_00417440(Plugin *plugin, Boolean initialize)
{
    int result = 2;
    if (plugin != NULL) {
        if (clState.pluginDebug != 0) {
            UInt8 message;
            if (initialize)
                message = 0x33;
            else
                message = 0x34;
            CLErrors_ForwardMessage(message, CLPlugins_GetName(plugin));
        }
        if (initialize) {
            UInt32 pluginType = CLPlugins_GetPluginDesc(plugin)->type;
            if (pluginType == 'cldr') {
                plugin->object = (CWPluginPrivateContext *)new PluginA(pluginType, 0);
            } else if (pluginType == 'Pars') {
                plugin->object = (CWPluginPrivateContext *)new PluginB;
            } else if (pluginType == 'Comp' || pluginType == 'Link') {
                plugin->object = (CWPluginPrivateContext *)new PluginC;
            } else {
                plugin->object = NULL;
            }
            plugin->object->shellContext = operator new(6);
        }
        if (plugin->object != NULL) {
            SInt8 request;
            if (initialize)
                request = -2;
            else
                request = -1;
            initialize_plugin_request(plugin, request);
            result = MemUtils_CallPluginEntry(plugin);
        } else {
            result = 0;
        }
        if (!initialize)
            release_negative_license_values();
    }
    return result == 0;
}

extern "C" {

void CLPluginRequests_AppendMacFileTypesTable(struct MacFileTypeNode **firstArgument, SInt32 secondArgument)
{
    MacFileTypes_AppendTable(firstArgument, secondArgument);
}
}
