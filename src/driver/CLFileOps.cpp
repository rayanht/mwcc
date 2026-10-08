#pragma bool off

#include "compiler/common.h"
#include "driver/CLFileOps.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "driver/AssertionFailure.h"
#include "driver/CLBrowser.h"
#include "driver/CLErrors.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLPlugins.h"
#include "driver/CLPrefs.h"
#include "driver/CLToolExec.h"
#include "driver/CLWriteObjectFile.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
extern "C" {

#include "mwcc/Plugins.h"

#include <string.h>


#include <stdio.h>
#include <stdlib.h>

struct CLTarget *default_target;

static SInt16 data_0057f3b0;
#define CERROR_FILE "unknown.c"
}

extern "C" {
}

extern "C" {
static inline int applyClassTypes(DropinFileRecord *request)
{
    const CWObjectFlags *objectFlags;
    UInt32 creator;
    UInt32 type;
    objectFlags = CLPlugins_GetObjectFlags(request->selectedPlugin);
    type = optsCompiler.disFileType ? optsCompiler.disFileType : objectFlags->disFileType;
    creator = optsCompiler.disFileCreator ? optsCompiler.disFileCreator : objectFlags->disFileCreator;
    return dispatch_output_storage_by_mask(request, 4, creator, type);
}

int __stdcall add_access_path(NamespaceOperationContext *context, NamespaceOperationState *state)
{
    struct CLTarget *paths;
    AccessPathEntry *entry;
    OSSpec buffer;
    AccessPaths *paths_list;
    int failed;
    int result;

    result = MacSpecs_MakeOSSpec(&state->file, &buffer);
    if (result != 0) {
        context->errorCode = OS_OSErrorToMacError(result);
        return 3;
    }
    if (optsCmdLine.verbose > 2) {
        CLErrors_ForwardMessage(0x4d, " search path", OS_PathSpecToString(&buffer.path, data_005880e0, 0x104));
    }
    if (state->flag4b == 0) {
        paths = default_target;
        paths_list = &paths->userPaths;
    } else {
        paths = default_target;
        paths_list = &paths->systemPaths;
    }
    entry = CLAccessPaths_CreateAccessPathEntry(&buffer.path);
    if (state->index >= 0)
        failed = !CLAccessPaths_InsertItem(paths_list, state->index, entry);
    else
        failed = !CLAccessPaths_StoreItem(paths_list, entry);
    if (failed)
        return 2;
    if (state->flag4a != 0) {
        CLAccessPaths_InitializeChildren(entry);
        if (fn_004151f0())
            return 1;
    }
    if (state->flag4b == 0)
        context->status[5] = 1;
    else
        context->status[4] = 1;
    return 0;
}

unsigned int (*data_0054bf48)(char *) = NULL;

unsigned int __stdcall add_or_copy_pref_panel_storage(unsigned int unused, char *name, StorageHandle *data)
{
    NameTableEntry *entry;
    unsigned int size;
    entry = CLPrefs_FindNameTableEntry(name);
    if (entry == 0) {
        size = Memory_GetHandleSize(data);
        entry = create_data_block(name, data->data, size);
        if (entry == 0 || CLPrefs_AddPrefPanel(entry) == 0) {
            return 2;
        }
    } else if (CLPrefs_CopyStorage(entry, data) == 0) {
        return 2;
    }
    if (data_0054bf48 != 0) {
        (*data_0054bf48)(name);
    }
    return 0;
}

int __stdcall set_file_output_name_and_kind(int unused, int index, short mode, char *name)
{
    struct CLTarget *context;
    DropinFileRecord *obj;
    IndexedListLink *head;

    if (index < 0)
        return 3;
    context = default_target;
    head = &context->files;
    obj = CLFiles_FindFileByIndex(head, index);
    if (obj == 0)
        return 9;
    if (obj->kind != 0)
        CLErrors_EmitDiagnostic(0x67, obj->inputName, obj->outputName);
    strcpy(obj->outputName, name);
    if ((int)mode == 1)
        obj->kind = 2;
    else if ((int)mode == 2)
        obj->kind = 1;
    else if ((int)mode == 3)
        obj->kind = 4;
    else
        return 3;
    return 0;
}

int __stdcall set_output_directory(CWPluginPrivateContext *record, short *input)
{
    OSSpec workspace;
    int result;
    struct CLTarget *data;

    result = MacSpecs_MakeOSSpec((CWFileSpec *)input, &workspace);
    if (result != 0) {
        record->callbackOSError = OS_OSErrorToMacError(result);
        return 3;
    }
    data = default_target;
    data->outputDirectory = workspace.path;
    record->targetfile = *(CWFileSpec *)input;
    return 0;
}

int __stdcall add_overlay_group(CWPluginPrivateContext *object, char *name, CLOverlayValues *value,
                                unsigned int *result)
{
    CLOverlayValues savedValue;
    int high;
    CLOverlayEntry *entry;
    struct CLTarget *context = default_target;

    if (context->linkage != 2)
        return 4;
    high = value->second;
    savedValue.second = high;
    savedValue.first = value->first;
    entry = CLOverlays_CreateOverlayEntry(name, savedValue);
    if (entry == 0)
        return 2;
    context = default_target;
    if (!CLOverlays_AppendGroup(&context->overlays, entry, result))
        return 2;
    object->numOverlayGroups++;
    if (optsCmdLine.verbose > 2)
        CLErrors_ForwardMessage(0x4f, name, savedValue.second, savedValue.first);
    return 0;
}

__stdcall int append_file_to_overlay(int unused, const char *fileID, int overlayID, unsigned int *result)
{
    struct CLOverlayEntry *entry;
    struct OverlayAllocation *file;
    char success;
    struct CLTarget *ctx = default_target;
    if (ctx->linkage != 2)
        return 4;
    ctx = default_target;
    entry = CLOverlays_GetGroupByIndex(&ctx->overlays, overlayID);
    if (entry == 0)
        return 2;
    file = CLOverlays_CreateOverlayAllocation(fileID);
    if (file == 0)
        return 7;
    success = CLOverlays_AppendOverlay(entry, file, result);
    if (success == 0)
        return 7;
    if (optsCmdLine.verbose > 2)
        CLErrors_ForwardMessage(0x4e, fileID, entry);
    return 0;
}

#define CERROR_FILE "DropinFileRecord.c"

static inline UInt8 CLFileOps_Err1(void)
{
    if (fn_004151f0())
        return 2;
    return 1;
}

static inline UInt8 CLFileOps_Err0(void)
{
    if (fn_004151f0())
        return 2;
    return 0;
}

int __stdcall lookup_path_index(unsigned int context, char *name, UInt16 nameLength, unsigned int *index)
{
    struct PayloadWithValue *nameId;
    Boolean found;
    UInt16 resolvedIndex;
    if (default_target->linkage != 1) {
        return 4;
    }
    nameId = CLSegs_CreatePayloadWithValue(name, nameLength);
    found = CLSegs_AddValue(&default_target->lookupPaths, nameId, &resolvedIndex);
    if (!found) {
        return 2;
    }
    *index = (unsigned int)resolvedIndex;
    return 0;
}

unsigned int __stdcall set_lookup_paths_name_and_value(unsigned int context, unsigned short recordId, char *text,
                                                       unsigned short value)
{
    struct PayloadWithValue *record;
    struct CLTarget *state;

    if (default_target->linkage != 1) {
        return 4;
    }
    state = default_target;
    record = (struct PayloadWithValue *)CLSegs_GetValue(&state->lookupPaths, (&context)[1]);
    if (record == 0) {
        return 0x201;
    }
    strncpy(record->name, text, 32);
    record->name[31] = '\0';
    record->value = value;
    return 0;
}
}

/* The parser's callbacks. */
static void *data_0054bf4c[9] = {
    (void *)add_access_path,
    (void *)add_or_copy_pref_panel_storage,
    (void *)set_file_output_name_and_kind,
    (void *)set_output_directory,
    (void *)add_overlay_group,
    (void *)append_file_to_overlay,
    (void *)lookup_path_index,
    (void *)set_lookup_paths_name_and_value,
    NULL,
};

PluginB::PluginB() : PluginA(0x50617273, -1)
{
    first = 0;
    second = third = 0;
    fourth = 0;
    fifth = 0;
    sixth = 0;
    seventh = 0;
    ninth = 0;
    eighth = 0;
    dataReference = &data_0054bf4c;
}

extern "C" {

unsigned int CLFileOps_GetScaledTicks(void)
{
    unsigned int ticks = OS_GetMilliseconds();
    ticks *= 60;
    return ticks / 1000;
}

unsigned short fn_00419620(void)
{
    return data_0057f3b0;
}

SInt32 dispatch_output_storage_by_mask(DropinFileRecord *state, SInt16 mask, SInt32 argument1, SInt32 argument2)
{
    SInt32 value;
    Boolean result;
    if (state->outputStorage != NULL) {
        value = Memory_GetHandleSize(state->outputStorage);
        if ((SInt32)mask != 8L || (optsCmdLine.toDisk & 8) != 0) {
            if ((state->outputMask & (SInt32)mask) == 0)
                return CLIO_WriteStorageToStdout(state->outputStorage, value, 0);
            if (optsCmdLine.verbose != 0)
                CLErrors_ForwardMessage(0xf, OS_SpecToStringRelative(&state->outputPath, NULL, data_005880e0, 0x104));
            result = CLIO_WriteTextFile(&state->outputPath, state->outputStorage, value, argument1, argument2);
            state->validatedOutputMask |= mask;
            return result;
        }
        if (optsCompiler.outMakefile[0] != 0)
            return CLIO_AppendStorageToFile(&clState.makefileSpec, state->outputStorage, value, argument1, argument2);
        return CLIO_WriteStorageToStdout(state->outputStorage, value, 0);
    }
    return 0;
}

int CLFileOps_SetupOutputPath(DropinFileRecord *obj, SInt16 mask)
{
    char fallbackName[64];
    char pathName[260];
    char suffix[16];
    OSPathSpec directoryPath;
    const char *outputSuffix;
    char *extension;
    UInt32 result;
    char *directory;
    const CWObjectFlags *objectFlags;
    char *name;
    char *outputName;
    Boolean hasOutputName;
    struct CLTarget *context;

    if (optsCmdLine.toDisk & mask)
        obj->outputMask |= mask;

    if (obj->kind == mask) {
        outputName = obj->outputName;
        hasOutputName = (obj->outputName[0] != 0) && ((obj->temporaryOutputMask & mask) == 0);
        if (hasOutputName) {
            obj->outputMask |= mask;
            obj->temporaryOutputMask &= ~mask;
        }
    } else {
        outputName = fallbackName;
        hasOutputName = 0;
    }

    if (((optsCmdLine.toDisk | obj->outputMask | obj->temporaryOutputMask) & mask) == 0)
        obj->temporaryOutputMask |= mask;

    if (!hasOutputName && (((optsCmdLine.toDisk | obj->outputMask | obj->temporaryOutputMask) & mask) != 0)) {
        objectFlags = CLPlugins_GetObjectFlags(obj->selectedPlugin);
        int outputKind = mask;
        if (outputKind == 1) {
            if (optsCompiler.ppFileExt[0] != 0)
                outputSuffix = optsCompiler.ppFileExt;
            else
                outputSuffix = objectFlags->ppFileExt;
        } else if (outputKind == 4) {
            if (optsCompiler.disFileExt[0] != 0)
                outputSuffix = optsCompiler.disFileExt;
            else
                outputSuffix = objectFlags->disFileExt;
        } else if (outputKind == 8) {
            if (optsCompiler.depFileExt[0] != 0)
                outputSuffix = optsCompiler.depFileExt;
            else
                outputSuffix = objectFlags->depFileExt;
        } else if (outputKind == 2) {
            if (optsCompiler.objFileExt[0] != 0)
                outputSuffix = optsCompiler.objFileExt;
            else
                outputSuffix = objectFlags->objFileExt;
            if ((obj->compilerFlags & 0x80000000) == 0 && !hasOutputName)
                outputSuffix = NULL;
        } else {
            outputSuffix = NULL;
        }

        if (outputSuffix != NULL && *outputSuffix != 0) {
            if (*outputSuffix != '.')
                sprintf(suffix, ".%s", outputSuffix);
            else
                strcpy(suffix, outputSuffix);

            if ((obj->temporaryOutputMask & mask) != 0 && (optsCmdLine.stages & 8) == 0) {
                directory = getenv("TEMP");
                if (directory == NULL || *directory == 0 || OS_MakePathSpec(0, directory, &directoryPath) != 0)
                    directory = getenv("TMP");
                if (directory == NULL || *directory == 0 || OS_MakePathSpec(0, directory, &directoryPath) != 0)
                    directory = getenv("TMPDIR");
                if (directory == NULL || *directory == 0 || OS_MakePathSpec(0, directory, &directoryPath) != 0)
                    directory = ".";
                sprintf(pathName, "%s%c%s", directory, '\\', OS_GetFileNamePtr(obj->inputName));
                name = pathName;
            } else {
                if (optsCompiler.relPathInOutputDir == 0)
                    name = OS_GetFileNamePtr(obj->inputName);
                else
                    name = obj->inputName;
            }

            extension = strrchr(OS_GetFileNamePtr(name), '.');
            if (extension == NULL || *outputSuffix == '.')
                extension = name + strlen(name);
            if ((extension - name) + strlen(suffix) >= 64)
                extension = name + 63 - strlen(suffix);
            sprintf(outputName, "%.*s%s", extension - name, name, suffix);
        } else {
            *outputName = 0;
            obj->outputMask &= ~mask;
            obj->temporaryOutputMask &= ~mask;
        }
    }

    if (((obj->outputMask | obj->temporaryOutputMask) & mask) != 0) {
        context = default_target;
        result = OS_MakeSpecWithPath(&context->outputDirectory, outputName, !optsCompiler.relPathInOutputDir,
                                     &obj->outputPath);
        if (result == 0) {
            if (OS_EqualSpec(&obj->inputPath, &obj->outputPath) != 0) {
                if (hasOutputName) {
                    CLErrors_EmitDiagnostic(0x66, outputName);
                    obj->outputMask &= ~mask;
                    obj->temporaryOutputMask &= ~mask;
                    return 0;
                }
                obj->outputMask &= ~mask;
                obj->temporaryOutputMask &= ~mask;
            }
        } else {
            context = default_target;
            CLErrors_ReportOSError(9, result, outputName,
                                   OS_PathSpecToString(&context->outputDirectory, data_005880e0, 0x104));
        }
        return (result == 0);
    }
    return 1;
}

unsigned int setup_file_request(DropinFileRecord *context)
{
    Boolean result;
    Plugin *job = (Plugin *)context->selectedPlugin;
    DropinFileRecord *input = context;
    result = CLPluginRequests_SetupFileRequest(job, input, 0);
    if (result == 0) {
        return 0U;
    }
    return 1U;
}

int setup_preprocessing_output(DropinFileRecord *st)
{
    if ((st->compilerCapabilities & 0x20000000) == 0) {
        CLErrors_ForwardMessageArguments(0x35, CLPlugins_GetName(st->selectedPlugin), st->inputName);
        return 1;
    }
    if (!(Boolean)CLPluginRequests_SetupFileRequest(st->selectedPlugin, st, 1)) {
        return 0;
    }
    if (!CLFileOps_SetupOutputPath(st, 1)) {
        return 0;
    }
    if (st->outputStorage != 0) {
        const CWObjectFlags *objectFlags = CLPlugins_GetObjectFlags(st->selectedPlugin);
        UInt32 type = optsCompiler.ppFileType ? optsCompiler.ppFileType : objectFlags->ppFileType;
        UInt32 creator = optsCompiler.ppFileCreator ? optsCompiler.ppFileCreator : objectFlags->ppFileCreator;
        return dispatch_output_storage_by_mask(st, 1, creator, type);
    }
    CLErrors_EmitDiagnostic(0x60, "preprocessing", st->inputName);
    return 0;
}

unsigned int fn_00419c90(DropinFileRecord *state, unsigned int mode)
{
    OSHandle recovery;
    const CWObjectFlags *objectFlags;
    UInt32 type;
    UInt32 creator;

    objectFlags = CLPlugins_GetObjectFlags(state->selectedPlugin);
    if (!(optsCmdLine.stages & 3) && static_cast<unsigned char>(mode)) {
        if (!CLPluginRequests_SetupFileRequest(state->selectedPlugin, state, 8))
            return 0;
    }
    if (!CLFileOps_SetupOutputPath(state, 2))
        return 0;
    {
        struct CLTarget *pluginState = default_target;
        CLDependencies_WriteDependencies(&pluginState->dependencyTable, state, &recovery);
    }
    state->outputStorage = Memory_CreateStorageHandle(&recovery);
    if (!CLFileOps_SetupOutputPath(state, 8))
        return 0;
    type = optsCompiler.depFileType ? optsCompiler.depFileType : objectFlags->depFileType;
    creator = optsCompiler.depFileCreator ? optsCompiler.depFileCreator : objectFlags->depFileCreator;
    if (dispatch_output_storage_by_mask(state, 8, creator, type))
        return 1;
    return 0;
}

unsigned int fn_00419d80(DropinFileRecord *input)
{
    const CWObjectFlags *objectFlags;
    unsigned int value;
    unsigned int info;
    if ((char)input->dependencyStatusNegative == '\0')
        return 1;
    if (input->secondaryReferenceHandle == 0) {
        CLErrors_EmitDiagnostic(0x65, CLPlugins_GetName(input->selectedPlugin),
                                OS_SpecToStringRelative(&input->inputPath, 0, data_005880e0, 0x104));
        return 0;
    }
    if (((optsCmdLine.toDisk & 2) != 0) && (optsCompiler.browserEnabled != '\0')) {
        objectFlags = CLPlugins_GetObjectFlags(input->selectedPlugin);
        value = optsCompiler.browseFileType ? optsCompiler.browseFileType : objectFlags->brsFileType;
        info = optsCompiler.browseFileCreator ? optsCompiler.browseFileCreator : objectFlags->brsFileCreator;
        value = fn_004286d0(input, info, value);
        if (value == 0)
            return 0;
    }
    return 1;
}

unsigned int write_object_file(DropinFileRecord *record)
{
    const CWObjectFlags *objectFlags;
    SInt32 type;
    SInt32 creator;
    objectFlags = CLPlugins_GetObjectFlags(record->selectedPlugin);
    if (record->objectData == 0U)
        return 1;
    type = optsCompiler.objFileType ? optsCompiler.objFileType : objectFlags->objFileType;
    creator = optsCompiler.objFileCreator ? optsCompiler.objFileCreator : objectFlags->objFileCreator;
    if (CLWriteObjectFile_WriteObjectFile(record, creator, type) == 0U)
        return 0;
    return 1;
}

unsigned int fn_00419e90(DropinFileRecord *state)
{
    unsigned int otherFlags;
    unsigned int flags;
    unsigned int initialFlags;
    initialFlags = (short)state->validatedOutputMask;
    initialFlags &= 2U;
    if (initialFlags != 0U)
        return 1U;
    if (state->objectData == 0U && state->secondaryReferenceHandle == 0U)
        return 1U;
    if (CLFileOps_SetupOutputPath(state, 2)) {
        flags = state->temporaryOutputMask;
        otherFlags = state->outputMask;
        flags |= otherFlags;
        flags &= 2U;
        if (flags == 0U)
            return 1U;
        if (!fn_00419d80(state) || !write_object_file(state))
            return 0U;
        state->validatedOutputMask |= 2U;
        return 1U;
    }
    return 0U;
}

int setup_compile_file_request(DropinFileRecord *file)
{
    SInt32 result = 1;
    SInt16 optionValue;
    char path[260];
    SInt32 status;

    if (!(file->compilerCapabilities & 0x10000000) &&
        (optsCompiler.forcePrecompile == 1 || (file->fileFlags & 0x80000000))) {
        CLErrors_ForwardMessageArguments(0x36, CLPlugins_GetName(file->selectedPlugin), &file->inputName[0]);
        return 1;
    }
    if (optsCompiler.browserEnabled != 0) {
        memset(file->dependencyState, 1, sizeof(file->dependencyState));
        OS_SpecToString(&file->inputPath, path, sizeof(path));
        status = CLBrowser_FindOrAddLookupEntry(&clState.browseTableHandle, path, &optionValue);
        if (status == 0)
            return 0;
        file->dependencyStatusNegative = (status < 0);
        file->dependencyOption = optionValue;
        if (optsCmdLine.verbose > 1)
            CLErrors_ForwardMessage(0x16, &file->inputName[0], file->dependencyOption);
    } else {
        memset(file->dependencyState, 0, sizeof(file->dependencyState));
        file->dependencyStatusNegative = 0;
        file->dependencyStatusNegative = 0;
        file->dependencyOption = 0;
    }
    if ((Boolean)CLPluginRequests_SetupFileRequest(file->selectedPlugin, file, 2)) {
        if ((file->configurationReceivedWithoutCapability == 0 || file->objectData == 0) &&
            (file->compilerCapabilities & 0x80000000) && (file->outputArgumentMask & 2)) {
            CLErrors_EmitDiagnostic(0x60, "compiling", &file->inputName[0]);
            result = 0;
        } else if (optsCmdLine.state == 2 && fn_00419e90(file) == 0) {
            result = 0;
        }
    } else {
        result = 0;
    }
    return result;
}

unsigned int execute_tool_with_output_path(DropinFileRecord *record, Plugin *tool, unsigned int flags)
{
    char outputPath[260];
    int result;
    if (record->outputMask & 4) {
        OS_SpecToString(&record->outputPath, outputPath, sizeof(outputPath));
        result = CLToolExec_ExecuteLinker(tool, flags, record, (optsCmdLine.toDisk & 4) ? 0 : outputPath, 0);
        record->validatedOutputMask |= 4U;
        return result;
    }
    return CLToolExec_ExecuteLinker(tool, flags, record, 0, 0);
}

int disassemble_file(DropinFileRecord *request)
{
    if (request->configurationReceivedWithoutCapability == 0 && request->configurationReceivedWithCapability == 0) {
        CLErrors_EmitDiagnostic(56, request->inputName);
        return 0;
    }
    if (CLFileOps_SetupOutputPath(request, 4) == 0) {
        return 0;
    }
    if ((request->compilerCapabilities & 0x02000000) != 0) {
        if (CLPluginRequests_CallPluginForFile(request->selectedPlugin, request) == 0) {
            return 0;
        }
        if (request->outputStorage != 0) {
            return applyClassTypes(request);
        }
        CLErrors_EmitDiagnostic(96, "disassembling", request->inputName);
        return 0;
    }
    if ((default_target->linkerFlags & 1) != 0 && (default_target->linkerFlags & 0x80000000) == 0) {
        return execute_tool_with_output_path(request, default_target->linker, default_target->linkerFlags);
    }
    if ((default_target->preLinkerFlags & 1) != 0 && (default_target->preLinkerFlags & 0x80000000) == 0) {
        return execute_tool_with_output_path(request, default_target->preLinker, default_target->preLinkerFlags);
    }
    if (default_target->linker != 0 && (default_target->linkerFlags & 0x80000000) == 0) {
        if (CLPluginRequests_CallPluginForFile(default_target->linker, request) != 0) {
            if (request->outputStorage != 0 && Memory_GetHandleSize(request->outputStorage) > 0) {
                return applyClassTypes(request);
            }
            CLErrors_EmitDiagnostic(96, "disassembling", request->inputName);
        }
        return 0;
    }
    if (default_target->preLinker != 0 && (default_target->preLinkerFlags & 0x80000000) == 0) {
        if (CLPluginRequests_CallPluginForFile(default_target->preLinker, request) != 0) {
            if (request->outputStorage != 0 && Memory_GetHandleSize(request->outputStorage) > 0) {
                return applyClassTypes(request);
            }
            CLErrors_EmitDiagnostic(96, "disassembling", request->inputName);
        }
        return 0;
    }
    if (default_target->linker != 0) {
        CLErrors_EmitDiagnostic(58, CLPlugins_GetName(default_target->linker),
                                CLPlugins_GetName(request->selectedPlugin), request->inputName);
    } else {
        CLErrors_EmitDiagnostic(57, CLPlugins_GetName(request->selectedPlugin), request->inputName);
    }
    return 0;
}

int compile_file(DropinFileRecord *file, char *processed)
{
    struct CLTarget *totals;
    SInt32 startTime, endTime;
    int codeWhole, dataWhole, bssWhole;
    char *codeFraction, *dataFraction, *bssFraction;

    totals = default_target;
    *processed = 0;
    if (optsCmdLine.dryRun != 0) {
        *processed = 1;
        return 1;
    }
    if ((file->inputArgumentMask & 1) == 0) {
        if (file->fileFlags & 0x20000000)
            *processed = 1;
        return 1;
    }
    if (file->selectedPlugin == 0)
        CLIO_ReportAssertionFailure("file->compiler", "CLFileOps.c", 0x342);
    *processed = 1;
    startTime = OS_GetMilliseconds();
    CLBrowser_InitMemBuffer(&clState.browseTableHandle);
    CLDependencies_SetAccessPath(&file->inputPath, 1);
    if (optsCmdLine.stages == 0 && setup_file_request(file) == 0)
        return 0;
    if ((optsCmdLine.stages & 1) != 0 && setup_preprocessing_output(file) == 0)
        return 0;
    if ((optsCmdLine.stages & 6) != 0 && setup_compile_file_request(file) == 0)
        return 0;
    if ((optsCmdLine.stages & 8) != 0 && fn_00419c90(file, 1) == 0)
        return 0;
    if ((optsCmdLine.stages & 4) != 0 && disassemble_file(file) == 0)
        return 0;
    if (optsCmdLine.verbose != 0 && (optsCmdLine.stages & 2) != 0) {
        CLErrors_ForwardMessage(0x19, file->reportedTotal);
        totals->total00 += file->reportedTotal;
        set_bytes(file->codeSize, &codeWhole, &codeFraction);
        set_bytes(file->dataSize, &dataWhole, &dataFraction);
        set_bytes(file->bssSize, &bssWhole, &bssFraction);
        CLErrors_ForwardMessage(0x1a, codeWhole, codeFraction, dataWhole, dataFraction, bssWhole, bssFraction);
    }
    totals->count04 += file->codeSize;
    totals->count08 += file->dataSize;
    totals->count0c += file->bssSize;
    CLBrowser_FreeMemBuffer(&clState.browseTableHandle);
    endTime = OS_GetMilliseconds();
    if (optsCmdLine.timeWorking != 0) {
        if (optsCmdLine.verbose != 0) {
            CLErrors_ForwardMessage(0x18, (double)(UInt32)(endTime - startTime) * 0.001, "compile", "", "", "");
        } else {
            CLErrors_ForwardMessage(0x18, (double)(UInt32)(endTime - startTime) * 0.001, "compile", "'",
                                    &file->inputName, "'");
        }
    }
    return 1;
}

void set_bytes(unsigned int byteCount, int *value, char **unitName)
{
    *value = byteCount;
    *unitName = "bytes";
}

int CLFileOps_CompileProject(void)
{
    Boolean changed = 0;
    struct CLTarget *stats;
    struct CLTarget *ops;
    UInt32 startTime;
    SInt32 elapsed;
    char kind;
    int minutes1, minutes2, minutes3;
    char *seconds1, *seconds2, *seconds3;
    SInt32 index;
    SInt32 pathIndex;
    UInt16 subIndex;
    struct AccessPathEntry *path;
    int result;
    SInt32 succeeded;
    SInt32 failed;

    startTime = OS_GetMilliseconds();
    CLBrowser_InitCache();

    if (optsCmdLine.verbose > 1) {
        if (clState.plugintype == 0x436f6d70)
            CLErrors_ForwardMessage(0x54, "User include search paths");
        else
            CLErrors_ForwardMessage(0x54, "Library search paths");
        for (index = 0; index < CLAccessPaths_GetCount(&default_target->systemPaths); index++) {
            path = CLAccessPaths_GetEntry(&default_target->systemPaths, index);
            if (path == 0)
                CLIO_ReportAssertionFailure("path != NULL", "CLFileOps.c", 0x3ca);
            CLErrors_ForwardMessage(0x55, OS_PathSpecToString(path->path, data_005880e0, 0x104),
                                    path->children ? " [r]" : "");
            if (path->children != 0) {
                for (subIndex = 0; subIndex < CLAccessPaths_GetCount(path->children); subIndex++) {
                    struct AccessPathEntry *subPath;
                    subPath = CLAccessPaths_GetEntry(path->children, subIndex);
                    if (subPath == 0)
                        CLIO_ReportAssertionFailure("sub", "CLFileOps.c", 0x3d3);
                    CLErrors_ForwardMessage(0x55, "\t", OS_PathSpecToString(subPath->path, data_005880e0, 0x104));
                }
            }
        }
        if (clState.plugintype == 0x436f6d70)
            CLErrors_ForwardMessage(0x54, "System include search paths");
        for (pathIndex = 0; pathIndex < CLAccessPaths_GetCount(&default_target->userPaths); pathIndex++) {
            path = CLAccessPaths_GetEntry(&default_target->userPaths, pathIndex);
            if (path == 0)
                CLIO_ReportAssertionFailure("path != NULL", "CLFileOps.c", 0x3e0);
            CLErrors_ForwardMessage(0x55, OS_PathSpecToString(path->path, data_005880e0, 0x104),
                                    path->children ? " [r]" : "");
            if (path->children != 0) {
                for (subIndex = 0; subIndex < CLAccessPaths_GetCount(path->children); subIndex++) {
                    struct AccessPathEntry *subPath;
                    subPath = CLAccessPaths_GetEntry(path->children, subIndex);
                    if (subPath == 0)
                        CLIO_ReportAssertionFailure("sub", "CLFileOps.c", 0x3e9);
                    CLErrors_ForwardMessage(0x55, "\t", OS_PathSpecToString(subPath->path, data_005880e0, 0x104));
                }
            }
        }
        CLErrors_ForwardMessage(0x54, "Link order for project");
        for (index = 0; index < CLFiles_GetIndex(&default_target->files); index++) {
            DropinFileRecord *entry = CLFiles_FindFileByIndex(&default_target->files, index);
            CLErrors_ForwardMessage(0x55, entry->inputName, "");
        }
    }

    ops = default_target;
    if (!CLPluginRequests_InitializeTargetSettings(ops, ops->linker, ops->linkerFlags))
        return 1;

    failed = 0;
    succeeded = 0;
    for (index = 0; index < CLFiles_GetIndex(&default_target->files); index++) {
        DropinFileRecord *entry;
        entry = CLFiles_FindFileByIndex(&default_target->files, index);
        if (entry->kind == 0)
            entry->kind = 2;
        result = compile_file(entry, &kind);
        if (result != 0) {
            if (kind)
                succeeded++;
            else
                failed++;
            if (fn_004151f0())
                return 2;
        }
        if (result == 0) {
            if (optsCompiler.noFail != 0) {
                changed = 1;
                clState.countWarnings = 0;
                clState.countErrors = 0;
                clState.withholdWarnings = 0;
                clState.withholdErrors = 0;
            } else {
                return CLFileOps_Err1();
            }
        }
    }

    if (optsCmdLine.stages & 8) {
        for (index = 0; index < CLFiles_GetIndex(&default_target->generatedFiles); index++) {
            DropinFileRecord *entry = CLFiles_FindFileByIndex(&default_target->generatedFiles, index);
            if (fn_00419c90(entry, 0) == 0) {
                if (optsCompiler.noFail != 0) {
                    changed = 1;
                    clState.countWarnings = 0;
                    clState.countErrors = 0;
                    clState.withholdWarnings = 0;
                    clState.withholdErrors = 0;
                } else {
                    return CLFileOps_Err1();
                }
            }
        }
    }

    CLBrowser_FreeCacheEntries();

    stats = default_target;
    if (optsCmdLine.verbose != 0 && succeeded > 1) {
        set_bytes(stats->count04, &minutes1, &seconds1);
        set_bytes(stats->count08, &minutes2, &seconds2);
        set_bytes(stats->count0c, &minutes3, &seconds3);
        CLErrors_ForwardMessage(0x1b, minutes1, seconds1, minutes2, seconds2, minutes3, seconds3);
    }

    elapsed = OS_GetMilliseconds();
    if (optsCmdLine.timeWorking != 0) {
        elapsed = elapsed - startTime;
        CLErrors_ForwardMessage(0x18, elapsed * 0.001, "compile", "", "", "project");
    }

    if (failed > 0 && succeeded == 0) {
        struct CLTarget *finalOps = default_target;
        if (finalOps->preLinker != 0 || (finalOps = default_target)->linker != 0) {
            CLErrors_EmitDiagnostic(0x1d);
            changed = 1;
        }
    }

    if (changed)
        return 1;
    return CLFileOps_Err0();
}

/* Data of the original file that none of its linked code uses. */
static double lbl_0054C0A8 = 0.001;

int CLFileOps_LinkProject(void)
{
    CWFileSpec commandLine;
    OSSpec commandText;
    int startTime;
    int stageStart;
    int currentTime;
    const UInt8 failed = 1;
    const UInt8 cancelled = 2;

    startTime = OS_GetMilliseconds();

    if (CLPluginRequests_InitializeTargetSettings(default_target, default_target->linker,
                                                  default_target->linkerFlags) == 0)
        return 1;

    if (CLToolExec_SetTemporaryOutputMask() == 0)
        return 1;

    if (default_target->preLinker != 0 && optsLinker.callPreLinker != 0 && optsCmdLine.state == 3) {
        stageStart = OS_GetMilliseconds();
        if (default_target->preLinkerFlags & 1) {
            if (CLToolExec_ExecuteLinker(default_target->preLinker, default_target->preLinkerFlags, 0, 0, 0) == 0)
                return 1;
        } else if (optsCmdLine.dryRun == 0) {
            if (CLPluginRequests_UpdateTargetSettings(default_target->preLinker, default_target->preLinkerFlags,
                                                      default_target->settings) == 0) {
                return fn_004151f0() ? cancelled : failed;
            }
        }
        currentTime = OS_GetMilliseconds();
        if (optsCmdLine.timeWorking != 0)
            CLErrors_ForwardMessage(0x18, (double)(currentTime - stageStart) * 0.001, "prelink project", "", "", "");
    }

    if (default_target->linker != 0 && optsLinker.callLinker != 0 && optsCmdLine.state == 3) {
        stageStart = OS_GetMilliseconds();
        if (default_target->linkerFlags & 1) {
            if (CLToolExec_ExecuteLinker(default_target->linker, default_target->linkerFlags, 0, 0, 0) == 0)
                return 1;
        } else if (optsCmdLine.dryRun == 0) {
            if (CLPluginRequests_UpdateTargetSettings(default_target->linker, default_target->linkerFlags,
                                                      default_target->settings) == 0) {
                return fn_004151f0() ? cancelled : failed;
            }
        }
        currentTime = OS_GetMilliseconds();
        if (optsCmdLine.timeWorking != 0)
            CLErrors_ForwardMessage(0x18, (double)(currentTime - stageStart) * 0.001, "link project", "", "", "");
    }

    if (default_target->postLinker != 0 && optsLinker.callPostLinker != 0 && optsCmdLine.state == 3) {
        stageStart = OS_GetMilliseconds();
        if (default_target->postLinkerFlags & 1) {
            if (CLToolExec_ExecuteLinker(default_target->postLinker, default_target->postLinkerFlags, 0, 0, 0) == 0)
                return 1;
        } else if (optsCmdLine.dryRun == 0) {
            if (CLPluginRequests_InitializeTargetSettings(default_target, default_target->linker,
                                                          default_target->linkerFlags) == 0)
                return 1;
            if (default_target->settings->head.tag == 1)
                commandLine = default_target->settings->head.firstFile;
            if (default_target->settings->head.tag != 0) {
                if (CLPluginRequests_UpdateTargetSettings(default_target->postLinker, default_target->postLinkerFlags,
                                                          default_target->settings) == 0) {
                    return fn_004151f0() ? cancelled : failed;
                }
                if (CLPluginRequests_InitializeTargetSettings(default_target, default_target->postLinker,
                                                              default_target->postLinkerFlags) == 0)
                    return 1;
            }
        }
        currentTime = OS_GetMilliseconds();
        if (optsCmdLine.timeWorking != 0)
            CLErrors_ForwardMessage(0x18, (double)(currentTime - stageStart) * 0.001, "postlink project", "", "", "");
        if (!optsLinker.keepLinkerOutput && default_target->settings->head.tag == 1) {
            if (optsCmdLine.verbose > 1) {
                MacSpecs_MakeOSSpec(&commandLine, &commandText);
                CLErrors_ForwardMessage(0x13, OS_SpecToString(&commandText, data_005880e0, 0x104));
            }
            Files_DeleteFile(&commandLine);
        }
    }

    if (CLToolExec_DeleteTemporaryOutputs() == 0)
        return 1;

    currentTime = OS_GetMilliseconds();
    if (optsCmdLine.timeWorking != 0)
        CLErrors_ForwardMessage(0x18, (double)(currentTime - startTime) * 0.001, "finish link stage", "", "", "");

    return 0;
}
}
