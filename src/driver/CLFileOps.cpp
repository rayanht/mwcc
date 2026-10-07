#pragma bool off

#include "compiler/common.h"
#include "driver/CLFileOps.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/win32.h"
#include "compiler/BE_symbol.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Registers.h"
#include "driver/AssertionFailure.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLBrowser.h"
#include "driver/CLDependencies.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLOverlays.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLPlugins.h"
#include "driver/CLPrefs.h"
#include "driver/CLSegs.h"
#include "driver/CLTarg.h"
#include "driver/CLToolExec.h"
#include "driver/CLWriteObjectFile.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ClientGlue.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
extern "C" {

#include "mwcc/Plugins.h"

#include <string.h>

#include <string.h>

#include <stdio.h>
#include <stdlib.h>

static SInt16 data_0057f3b0;
#define CERROR_FILE "unknown.c"
}

extern "C" {
}

extern "C" {
static inline int applyClassTypes(DropinFileRecord *request)
{
    TypeClassTemplate *classInfo;
    void *fallbackType;
    struct Type *preferredType;
    classInfo = (TypeClassTemplate *)CLPlugins_GetObjectFlags(request->selectedPlugin);
    preferredType = data_00541bfc;
    if (preferredType == 0) {
        preferredType = classInfo->relatedClass;
    }
    fallbackType = data_00541bf8;
    if (fallbackType == 0) {
        fallbackType = classInfo->enclosingTemplate;
    }
    return dispatch_output_storage_by_mask(request, 4, (SInt32)fallbackType, (SInt32)preferredType);
}

int __stdcall add_access_path(NamespaceOperationContext *context, NamespaceOperationState *state)
{
    struct CLTarget *paths;
    AccessPathEntry *entry;
    OSSpec buffer;
    AccessPaths *paths_list;
    int failed;
    int result;

    result = MacSpecs_MakeOSSpec(&state->file, buffer.directory.path);
    if (result != 0) {
        context->errorCode = OS_OSErrorToMacError(result);
        return 3;
    }
    if (DAT_00541b28 > 2) {
        CLErrors_ForwardMessage(0x4d, " search path", fn_00412340(buffer.directory.path, data_005880e0, 0x104));
    }
    if (state->flag4b == 0) {
        paths = default_target;
        paths_list = &paths->userPaths;
    } else {
        paths = default_target;
        paths_list = &paths->systemPaths;
    }
    entry = CLAccessPaths_CreateAccessPathEntry(buffer.directory.path);
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

    result = MacSpecs_MakeOSSpec((CWFileSpec *)input, (char *)&workspace);
    if (result != 0) {
        record->callbackOSError = OS_OSErrorToMacError(result);
        return 3;
    }
    data = default_target;
    data->outputDirectory = workspace.directory;
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
    if (DAT_00541b28 > 2)
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
    if (DAT_00541b28 > 2)
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
        if ((SInt32)mask != 8L || (data_00541b22 & 8) != 0) {
            if ((state->outputMask & (SInt32)mask) == 0)
                return CLIO_WriteStorageToStdout(state->outputStorage, value, 0);
            if (DAT_00541b28 != 0)
                CLErrors_ForwardMessage(0xf, CLProj_MakeRelativePath(&state->outputPath, NULL, data_005880e0, 0x104));
            result = CLIO_WriteTextFile(&state->outputPath, state->outputStorage, value, argument1, argument2);
            state->validatedOutputMask |= mask;
            return result;
        }
        if (DAT_00541c0c != 0)
            return CLIO_AppendStorageToFile(&DAT_00587328, state->outputStorage, value, argument1, argument2);
        return CLIO_WriteStorageToStdout(state->outputStorage, value, 0);
    }
    return 0;
}

int CLFileOps_SetupOutputPath(DropinFileRecord *obj, SInt16 mask)
{
    char fallbackName[64];
    char pathName[260];
    char suffix[16];
    char directoryPath[260];
    char *outputSuffix;
    char *extension;
    UInt32 result;
    char *directory;
    OutputSuffixes *suffixes;
    char *name;
    char *outputName;
    Boolean hasOutputName;
    struct CLTarget *context;

    if (data_00541b22 & mask)
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

    if (((data_00541b22 | obj->outputMask | obj->temporaryOutputMask) & mask) == 0)
        obj->temporaryOutputMask |= mask;

    if (!hasOutputName && (((data_00541b22 | obj->outputMask | obj->temporaryOutputMask) & mask) != 0)) {
        suffixes = (OutputSuffixes *)CLPlugins_GetObjectFlags(obj->selectedPlugin);
        int outputKind = mask;
        if (outputKind == 1) {
            if (output_suffix != 0)
                outputSuffix = &output_suffix;
            else
                outputSuffix = suffixes->suffix1;
        } else if (outputKind == 4) {
            if (outputPathSuffix != 0)
                outputSuffix = &outputPathSuffix;
            else
                outputSuffix = suffixes->suffix4;
        } else if (outputKind == 8) {
            if (data_00541bc2 != 0)
                outputSuffix = &data_00541bc2;
            else
                outputSuffix = suffixes->suffix8;
        } else if (outputKind == 2) {
            if (output_suffix_string != 0)
                outputSuffix = &output_suffix_string;
            else
                outputSuffix = suffixes->suffix2;
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

            if ((obj->temporaryOutputMask & mask) != 0 && (data_00541b20 & 8) == 0) {
                directory = getenv("TEMP");
                if (directory == NULL || *directory == 0 || OS_MakePathSpec(0, directory, directoryPath) != 0)
                    directory = getenv("TMP");
                if (directory == NULL || *directory == 0 || OS_MakePathSpec(0, directory, directoryPath) != 0)
                    directory = getenv("TMPDIR");
                if (directory == NULL || *directory == 0 || OS_MakePathSpec(0, directory, directoryPath) != 0)
                    directory = ".";
                sprintf(pathName, "%s%c%s", directory, '\\', CLProj_GetFileName(obj->inputName));
                name = pathName;
            } else {
                if (data_00541c09 == 0)
                    name = CLProj_GetFileName(obj->inputName);
                else
                    name = obj->inputName;
            }

            extension = strrchr(CLProj_GetFileName(name), '.');
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
        result = CLProj_MakeOSSpecFromPath(context->outputDirectory.path, outputName, !data_00541c09, &obj->outputPath);
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
                                   fn_00412340(context->outputDirectory.path, data_005880e0, 0x104));
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
        Object *o = (Object *)CLPlugins_GetObjectFlags(st->selectedPlugin);
        HashNameNode *d = data_00541bf4;
        VarInfo *c;
        if (d == 0) {
            d = o->u.data.linkname;
        }
        c = data_00541bf0;
        if (c == 0) {
            c = o->u.data.info;
        }
        return dispatch_output_storage_by_mask(st, 1, (SInt32)c, (SInt32)d);
    }
    CLErrors_EmitDiagnostic(0x60, "preprocessing", st->inputName);
    return 0;
}

unsigned int fn_00419c90(DropinFileRecord *state, unsigned int mode)
{
    MemBuffer recovery;
    TypeClassTemplate *classType;
    char *argumentOverride;
    char *parameterOption;

    classType = static_cast<TypeClassTemplate *>(CLPlugins_GetObjectFlags(state->selectedPlugin));
    if (!(data_00541b20 & 3) && static_cast<unsigned char>(mode)) {
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
    argumentOverride = data_00541c04;
    argumentOverride =
        argumentOverride ? argumentOverride : reinterpret_cast<char *>(classType->templateArgumentOverrides);
    parameterOption = data_00541c00;
    parameterOption = parameterOption ? parameterOption : reinterpret_cast<char *>(classType->templateParameters);
    if (dispatch_output_storage_by_mask(state, 8, (SInt32)parameterOption, (SInt32)argumentOverride))
        return 1;
    return 0;
}

unsigned int fn_00419d80(DropinFileRecord *input)
{
    struct ObjFlagsData *object;
    unsigned int value;
    unsigned int info;
    if ((char)input->dependencyStatusNegative == '\0')
        return 1;
    if (input->secondaryReferenceHandle == 0) {
        CLErrors_EmitDiagnostic(0x65, CLPlugins_GetName(input->selectedPlugin),
                                CLProj_MakeRelativePath(&input->inputPath, 0, data_005880e0, 0x104));
        return 0;
    }
    if (((data_00541b22 & 2) != 0) && (data_00541c0a != '\0')) {
        object = (ObjFlagsData *)CLPlugins_GetObjectFlags(input->selectedPlugin);
        value = DAT_00541bec;
        if (DAT_00541bec == 0)
            value = object->creator;
        info = DAT_00541be8;
        if (DAT_00541be8 == 0)
            info = object->fileType;
        value = fn_004286d0(input, info, value);
        if (value == 0)
            return 0;
    }
    return 1;
}

unsigned int write_object_file(DropinFileRecord *record)
{
    TypeMemberFunc *member;
    SInt32 funcid;
    SInt32 vtbl_index;
    member = (TypeMemberFunc *)CLPlugins_GetObjectFlags(record->selectedPlugin);
    if (record->objectData == 0U)
        return 1;
    funcid = data_00541be4;
    if (!funcid)
        funcid = member->funcid;
    vtbl_index = data_00541be0;
    if (!vtbl_index)
        vtbl_index = member->vtbl_index;
    if (CLWriteObjectFile_WriteObjectFile(record, vtbl_index, funcid) == 0U)
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

    if (!(file->compilerCapabilities & 0x10000000) && (data_00541d0c == 1 || (file->fileFlags & 0x80000000))) {
        CLErrors_ForwardMessageArguments(0x36, CLPlugins_GetName(file->selectedPlugin), &file->inputName[0]);
        return 1;
    }
    if (data_00541c0a != 0) {
        memset(file->dependencyState, 1, sizeof(file->dependencyState));
        OS_SpecToString(&file->inputPath, path, sizeof(path));
        status = CLBrowser_FindOrAddLookupEntry(&data_00587570, path, &optionValue);
        if (status == 0)
            return 0;
        file->dependencyStatusNegative = (status < 0);
        file->dependencyOption = optionValue;
        if (DAT_00541b28 > 1)
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
        } else if (data_00541b1e == 2 && fn_00419e90(file) == 0) {
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
        result = CLToolExec_ExecuteLinker(tool, flags, record, (data_00541b22 & 4) ? 0 : outputPath, 0);
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
    MemBuffer *timerState;
    int codeWhole, dataWhole, bssWhole;
    char *codeFraction, *dataFraction, *bssFraction;

    totals = default_target;
    *processed = 0;
    if (data_00541b26 != 0) {
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
    timerState = &data_00587570;
    CLBrowser_InitMemBuffer(timerState);
    CLDependencies_SetAccessPath(&file->inputPath, 1);
    if (data_00541b20 == 0 && setup_file_request(file) == 0)
        return 0;
    if ((data_00541b20 & 1) != 0 && setup_preprocessing_output(file) == 0)
        return 0;
    if ((data_00541b20 & 6) != 0 && setup_compile_file_request(file) == 0)
        return 0;
    if ((data_00541b20 & 8) != 0 && fn_00419c90(file, 1) == 0)
        return 0;
    if ((data_00541b20 & 4) != 0 && disassemble_file(file) == 0)
        return 0;
    if (DAT_00541b28 != 0 && (data_00541b20 & 2) != 0) {
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
    CLBrowser_FreeMemBuffer(timerState);
    endTime = OS_GetMilliseconds();
    if (data_00541b2b != 0) {
        if (DAT_00541b28 != 0) {
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

    if (DAT_00541b28 > 1) {
        if (plugin_type == 0x436f6d70)
            CLErrors_ForwardMessage(0x54, "User include search paths");
        else
            CLErrors_ForwardMessage(0x54, "Library search paths");
        for (index = 0; index < CLAccessPaths_GetCount(&default_target->systemPaths); index++) {
            path = CLAccessPaths_GetEntry(&default_target->systemPaths, index);
            if (path == 0)
                CLIO_ReportAssertionFailure("path != NULL", "CLFileOps.c", 0x3ca);
            CLErrors_ForwardMessage(0x55, fn_00412340(path->path, data_005880e0, 0x104), path->children ? " [r]" : "");
            if (path->children != 0) {
                for (subIndex = 0; subIndex < CLAccessPaths_GetCount(path->children); subIndex++) {
                    struct AccessPathEntry *subPath;
                    subPath = CLAccessPaths_GetEntry(path->children, subIndex);
                    if (subPath == 0)
                        CLIO_ReportAssertionFailure("sub", "CLFileOps.c", 0x3d3);
                    CLErrors_ForwardMessage(0x55, "\t", fn_00412340(subPath->path, data_005880e0, 0x104));
                }
            }
        }
        if (plugin_type == 0x436f6d70)
            CLErrors_ForwardMessage(0x54, "System include search paths");
        for (pathIndex = 0; pathIndex < CLAccessPaths_GetCount(&default_target->userPaths); pathIndex++) {
            path = CLAccessPaths_GetEntry(&default_target->userPaths, pathIndex);
            if (path == 0)
                CLIO_ReportAssertionFailure("path != NULL", "CLFileOps.c", 0x3e0);
            CLErrors_ForwardMessage(0x55, fn_00412340(path->path, data_005880e0, 0x104), path->children ? " [r]" : "");
            if (path->children != 0) {
                for (subIndex = 0; subIndex < CLAccessPaths_GetCount(path->children); subIndex++) {
                    struct AccessPathEntry *subPath;
                    subPath = CLAccessPaths_GetEntry(path->children, subIndex);
                    if (subPath == 0)
                        CLIO_ReportAssertionFailure("sub", "CLFileOps.c", 0x3e9);
                    CLErrors_ForwardMessage(0x55, "\t", fn_00412340(subPath->path, data_005880e0, 0x104));
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
            if (data_00541b43 != 0) {
                changed = 1;
                diagnostic_count = 0;
                diagnostic_limit_count = 0;
                data_00587326 = 0;
                diagnosticReported = 0;
            } else {
                return CLFileOps_Err1();
            }
        }
    }

    if (data_00541b20 & 8) {
        for (index = 0; index < CLFiles_GetIndex(&default_target->generatedFiles); index++) {
            DropinFileRecord *entry = CLFiles_FindFileByIndex(&default_target->generatedFiles, index);
            if (fn_00419c90(entry, 0) == 0) {
                if (data_00541b43 != 0) {
                    changed = 1;
                    diagnostic_count = 0;
                    diagnostic_limit_count = 0;
                    data_00587326 = 0;
                    diagnosticReported = 0;
                } else {
                    return CLFileOps_Err1();
                }
            }
        }
    }

    CLBrowser_FreeCacheEntries();

    stats = default_target;
    if (DAT_00541b28 != 0 && succeeded > 1) {
        set_bytes(stats->count04, &minutes1, &seconds1);
        set_bytes(stats->count08, &minutes2, &seconds2);
        set_bytes(stats->count0c, &minutes3, &seconds3);
        CLErrors_ForwardMessage(0x1b, minutes1, seconds1, minutes2, seconds2, minutes3, seconds3);
    }

    elapsed = OS_GetMilliseconds();
    if (data_00541b2b != 0) {
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
    char commandText[324];
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

    if (default_target->preLinker != 0 && data_00541e16 != 0 && data_00541b1e == 3) {
        stageStart = OS_GetMilliseconds();
        if (default_target->preLinkerFlags & 1) {
            if (CLToolExec_ExecuteLinker(default_target->preLinker, default_target->preLinkerFlags, 0, 0, 0) == 0)
                return 1;
        } else if (data_00541b26 == 0) {
            if (CLPluginRequests_UpdateTargetSettings(default_target->preLinker, default_target->preLinkerFlags,
                                                      default_target->settings) == 0) {
                return fn_004151f0() ? cancelled : failed;
            }
        }
        currentTime = OS_GetMilliseconds();
        if (data_00541b2b != 0)
            CLErrors_ForwardMessage(0x18, (double)(currentTime - stageStart) * 0.001, "prelink project", "", "", "");
    }

    if (default_target->linker != 0 && data_00541e19 != 0 && data_00541b1e == 3) {
        stageStart = OS_GetMilliseconds();
        if (default_target->linkerFlags & 1) {
            if (CLToolExec_ExecuteLinker(default_target->linker, default_target->linkerFlags, 0, 0, 0) == 0)
                return 1;
        } else if (data_00541b26 == 0) {
            if (CLPluginRequests_UpdateTargetSettings(default_target->linker, default_target->linkerFlags,
                                                      default_target->settings) == 0) {
                return fn_004151f0() ? cancelled : failed;
            }
        }
        currentTime = OS_GetMilliseconds();
        if (data_00541b2b != 0)
            CLErrors_ForwardMessage(0x18, (double)(currentTime - stageStart) * 0.001, "link project", "", "", "");
    }

    if (default_target->postLinker != 0 && data_00541e17 != 0 && data_00541b1e == 3) {
        stageStart = OS_GetMilliseconds();
        if (default_target->postLinkerFlags & 1) {
            if (CLToolExec_ExecuteLinker(default_target->postLinker, default_target->postLinkerFlags, 0, 0, 0) == 0)
                return 1;
        } else if (data_00541b26 == 0) {
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
        if (data_00541b2b != 0)
            CLErrors_ForwardMessage(0x18, (double)(currentTime - stageStart) * 0.001, "postlink project", "", "", "");
        if (!data_00541e18 && default_target->settings->head.tag == 1) {
            if (DAT_00541b28 > 1) {
                MacSpecs_MakeOSSpec(&commandLine, commandText);
                CLErrors_ForwardMessage(0x13, OS_SpecToString((OSSpec *)commandText, data_005880e0, 0x104));
            }
            Files_DeleteFile(&commandLine);
        }
    }

    if (CLToolExec_DeleteTemporaryOutputs() == 0)
        return 1;

    currentTime = OS_GetMilliseconds();
    if (data_00541b2b != 0)
        CLErrors_ForwardMessage(0x18, (double)(currentTime - startTime) * 0.001, "finish link stage", "", "", "");

    return 0;
}
}
