#define CERROR_FILE "CLCompilerLinkerDropin_V10.c"
#pragma bool off

#include "compiler/common.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/win32.h"
#include "compiler/CError.h"
#include "compiler/CPrep.h"
#include "driver/AssertionFailure.h"
#include "driver/CLBrowser.h"
#include "driver/CLDependencies.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLPlugins.h"
#include "driver/CLSegs.h"
#include "driver/CLTarg.h"
#include "driver/ClientGlue.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/TargetOptimizer-ppc-eabi.h"
extern "C" {

#include <string.h>
#include <stdlib.h>
#include "mwcc/Plugins.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/ErrMgr.h"
/* References, values and path strings supplied to the drop-in callback. */

char reserved[44];

typedef OSSpec DropinPath;

int __stdcall cache_precompiled_header(unsigned int context, short *callback, int argument)
{
    StorageHandle *callbackBuffer;
    OSSpec callbackName;

    if (3 < optsCmdLine.verbose) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBCachePrecompiledHeader");
    }
    MacSpecs_MakeOSSpec((CWFileSpec *)callback, callbackName.directory.path);
    CLDropinCallbacks_V10_SetStorageHandle(context, argument, &callbackBuffer);
    CLBrowser_CacheFileText(&callbackName, callbackBuffer, 1);
    return 0;
}

/* The files list contains DropinFileRecord records. */

unsigned int __stdcall call_primary_reference_callback(unsigned int argument, unsigned int key, void *extra)
{
    DropinFileRecord *record;
    if (optsCmdLine.verbose > (short)(3U)) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBLoadObjectData");
    }
    record = CLFiles_FindFileByIndex(&default_target->files, key);
    if (record == 0) {
        return 9U;
    }
    if (record->objectData != 0U) {
        CLDropinCallbacks_V10_StoreValue(argument, reinterpret_cast<UInt32>(record->objectData), extra);
        return 0U;
    }
    return 2U;
}

int __stdcall store_object_data(int compiler, int dropinId, DropinConfiguration *configuration)
{
    UInt32 primaryValue;
    StorageHandle *secondaryValue;
    DropinFileCallback nameBuffer;
    DropinFileRecord *dropin;
    char *path;

    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBStoreObjectData");
    if (fn_004151f0())
        return 1;
    dropin = CLFiles_FindFileByIndex(&default_target->files, dropinId);
    if (dropin == 0)
        return 9;
    if (configuration->outputFileSpec == 0) {
        CLDropinCallbacks_V10_SetStorageHandle(compiler, reinterpret_cast<UInt32>(configuration->primaryReference),
                                               &primaryValue);
        CLDropinCallbacks_V10_SetStorageHandle(compiler, reinterpret_cast<UInt32>(configuration->secondaryReference),
                                               &secondaryValue);
        dropin->objectData = reinterpret_cast<StorageHandle *>(primaryValue);
        dropin->secondaryReferenceHandle = secondaryValue;
        CLDropinCallbacks_V10_StoreValue(compiler, primaryValue, &configuration->primaryReference);
        CLDropinCallbacks_V10_StoreValue(compiler, reinterpret_cast<UInt32>(secondaryValue),
                                         &configuration->secondaryReference);
    } else {
        if (dropin->kind != 0 && dropin->kind != 2)
            CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 0xeb,
                                         "Cannot store object file spec for '%s'\n", dropin->inputName);
        MacSpecs_MakeOSSpec(configuration->outputFileSpec, dropin->outputPath.directory.path);
        dropin->validatedOutputMask |= 2;
    }
    dropin->codeSize = configuration->values[0];
    dropin->bssSize = configuration->values[1];
    dropin->dataSize = configuration->values[2];
    dropin->reportedTotal = configuration->values[3];
    if (dropin->compilerCapabilities & 0x40000000)
        dropin->configurationReceivedWithCapability = 1;
    else
        dropin->configurationReceivedWithoutCapability = 1;
    dropin->configurationOptionEnabled = configuration->option != 0 && configuration->flag != 0;
    OS_GetTime(&dropin->callbackFileTime);
    SInt32 fileType = configuration->fileType;
    if (fileType != 0)
        dropin->fileType = fileType;
    if (configuration->path != 0) {
        DropinRequest *request = reinterpret_cast<DropinRequest *>(compiler);
        path = static_cast<char *>(configuration->path);
        if (make_osspec_from_path(path, &dropin->inputPath, 0) != 0 || OS_Status(&dropin->inputPath) != 0) {
            nameBuffer.enableDependencyLookup = 1;
            nameBuffer.searchOption = 0;
            nameBuffer.fileKey = -1;
            nameBuffer.suppressFileReferenceLookup = 1;
            if (CLDropinCallbacks_V10_FindAndLoadFile(request, path, &nameBuffer) == 0) {
                MacSpecs_MakeOSSpec(&nameBuffer.output, dropin->inputPath.directory.path);
            } else {
                char *basename = CLProj_GetFileName(path);
                if (basename != path) {
                    nameBuffer.enableDependencyLookup = 1;
                    nameBuffer.searchOption = 0;
                    nameBuffer.fileKey = -1;
                    nameBuffer.suppressFileReferenceLookup = 1;
                    if (CLDropinCallbacks_V10_FindAndLoadFile(request, basename, &nameBuffer) == 0)
                        MacSpecs_MakeOSSpec(&nameBuffer.output, dropin->inputPath.directory.path);
                }
            }
        }
    }
    return 0;
}
unsigned int __stdcall report_begin_sub_compile_not_implemented(unsigned int argument0, unsigned int argument1,
                                                                unsigned int argument2)
{
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBBeginSubCompile");
    CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 367, "UCBBeginSubCompile not implemented");
    return 2U;
}

unsigned int __stdcall report_end_sub_compile_not_implemented(unsigned int unused)
{
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBEndSubCompile");
    CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 379, "UCBEndSubCompile not implemented");
    return 2U;
}

/* Drop-in request and per-file path state. */

/* Settings record supplying the path base. */

/* Common request header preceding the file-specific fields. */

int __stdcall get_precompiled_header_spec(DropinRequest *request, int output, const char *path)
{
    struct DropinRequest *fileRequest;
    DropinFileRecord *file;
    const CWObjectFlags *objectFlags;
    unsigned int error;
    unsigned char useDefault;
    const char *base;
    unsigned int pathError;
    OSSpec resolvedPath;
    char convertedPath[260];

    if (optsCmdLine.verbose > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBGetPrecompiledHeaderSpec");
    }
    if (request->signature == 1131375984 || request->signature == 1281977963) {
        fileRequest = reinterpret_cast<DropinRequest *>(request);
    } else {
        return 4;
    }
    file = CLFiles_FindFileByIndex(&default_target->files, fileRequest->fileKey);
    if (file == 0) {
        CLIO_ReportAssertionFailure("file != NULL", "CLCompilerLinkerDropin_V10.cpp", 415);
    }
    objectFlags = CLPlugins_GetObjectFlags(file->selectedPlugin);
    if (file->outputName[0] == 0) {
        if (path != 0) {
            if (optsCompiler.canonicalIncludes != 0) {
                strcpy(convertedPath, path);
            } else if (OS_CanonPath(path, convertedPath) != 0) {
                return 3;
            }
            error = CLProj_MakeOSSpecFromPath(file->inputPath.directory.path, convertedPath, 0, &resolvedPath);
            if (error != 0) {
                CLErrors_ReportOSError(97, error, convertedPath);
                return 2;
            }
            if (file->kind == 0 || file->kind == 2) {
                file->kind = 2;
                CLProj_MakeRelativePath(&resolvedPath, 0, file->outputName, sizeof(convertedPath));
            }
            if (optsCmdLine.verbose != 0) {
                CLErrors_ForwardMessage(
                    16, "precompiled ",
                    CLProj_MakeRelativePath(&resolvedPath, 0, data_005880e0, sizeof(convertedPath)));
            }
        } else {
            useDefault = !optsCompiler.relPathInOutputDir;
            CLProj_MakeOSSpecFromPath(default_target->outputDirectory.path, file->inputName, useDefault, &resolvedPath);
            if (optsCompiler.pchFileExt[0] != 0) {
                base = optsCompiler.pchFileExt;
            } else {
                base = objectFlags->pchFileExt;
            }
            CLProj_ChangeFileExtension(resolvedPath.name, base);
            if (file->kind == 0 || file->kind == 2) {
                file->kind = 2;
                MsDos_CopyStringToBuffer(resolvedPath.name, file->outputName, sizeof(convertedPath));
            }
            CLErrors_ForwardMessage(59,
                                    CLProj_MakeRelativePath(&resolvedPath, 0, data_005880e0, sizeof(convertedPath)));
        }
        MacSpecs_MakeCWFileSpecFromString(resolvedPath.directory.path, (CWFileSpec *)output);
    } else {
        pathError = CLProj_MakeOSSpecFromPath(default_target->outputDirectory.path, file->outputName, 0, &resolvedPath);
        if (pathError != 0) {
            CLErrors_ReportOSError(97, pathError, file->outputName);
            return 2;
        }
        MacSpecs_MakeCWFileSpecFromString(resolvedPath.directory.path, (CWFileSpec *)output);
        if (path != 0) {
            CLErrors_ForwardMessage(60, CLProj_MakeRelativePath(&resolvedPath, 0, data_005880e0, sizeof(convertedPath)),
                                    path);
        }
    }
    return 0;
}

unsigned int __stdcall fn_004262a0(unsigned int unused1, unsigned int unused2)
{
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetResourceFile");
    CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 509, "UCBGetResourceFile not implemented");
    return 2U;
}

unsigned int __stdcall report_unimplemented_resource_file_put(unsigned int, unsigned int, unsigned int, unsigned int)
{
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBPutResourceFile");
    CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 524, "UCBPutResourceFile not implemented");
    return 2U;
}

/* Per-file state uses DropinFileRecord (get_precompiled_header_spec). */

static void resetcb(DropinFileCallback *p, char fl, int f6v)
{
    p->enableDependencyLookup = 1;
    p->searchOption = fl ? (char)1 : (char)0;
    p->fileKey = -1;
    p->suppressFileReferenceLookup = f6v;
}

/* Private drop-in request, settings, cache entry and result records. */

/* File-list record with cached input and output names and paths. */

#pragma opt_propagation off
int __stdcall lookup_precompiled_unit(struct DropinRequest *request, char *inputName, char mode, void **outputObject,
                                      struct DropinResultSlot *outputValue)
{
    struct DropinRequest *validatedRequest;
    DropinFileRecord *settings;
    CWFileSpec *callbackData;
    unsigned int error;
    int compilerState;
    struct DropinFileRecord *entry;
    DropinFileCallback callback;
    DropinPath inputPath;
    OSSpec outputPath;
    FILETIME fileInfo;
    int fileValue;
    struct DropinFileValue *outputFile;
    char outputName[260];
    if (optsCmdLine.verbose > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBLookUpUnit");
    }
    if (request->signature == 1131375984 || request->signature == 1281977963) {
        validatedRequest = request;
    } else {
        return 4;
    }
    settings = CLFiles_FindFileByIndex(&default_target->files, validatedRequest->fileKey);
    if (settings == 0) {
        CLIO_ReportAssertionFailure("srcfile != NULL", "CLCompilerLinkerDropin_V10.cpp", 586);
    }
    *outputObject = 0;
    outputValue->value = 0;
    compilerState = optsCompiler.sbmState;
    if (compilerState == 1 || compilerState == 3) {
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText("UCBLookUpUnit:  sbmState == sbmRebuild or sbmClean; failing\n");
        }
        return 514;
    }
    resetcb(&callback, mode, 1);
    if (CLDropinCallbacks_V10_FindAndLoadFile(request, inputName, &callback) != 0) {
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText("UCBLookUpUnit:  could not find source '%s'; failing\n", inputName);
        }
        return 8;
    }
    callbackData = &callback.output;
    MacSpecs_MakeOSSpec(callbackData, inputPath.directory.path);
    outputPath = inputPath;
    error = fn_00426320(&outputPath, settings);
    if (error != 0) {
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText("UCBLookUpUnit:  could not make precompiled unit spec for '%s' (%s)\n",
                                       inputName, OS_GetErrText(error));
        }
        return 2;
    }
    if (optsCompiler.sbmPath[0] != 0) {
        outputPath.directory = clState.sbmPathSpec;
        OS_SpecToString(&outputPath, outputName, sizeof(outputName));
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText("UCBLookUpUnit:  Only looking at '%s'\n", outputName);
        }
    } else {
        MsDos_CopyStringToBuffer(outputPath.name, outputName, sizeof(outputName));
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText("UCBLookUpUnit:  searching paths for '%s'\n", outputName);
        }
    }
    resetcb(&callback, mode, 0);
    if (CLDropinCallbacks_V10_FindAndLoadFile(request, outputName, &callback) != 0 || callback.fileReference == 0) {
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText("UCBLookUpUnit:  could not find or load precompiled unit file '%s'; failing\n",
                                       outputName);
        }
        return 8;
    }
    MacSpecs_MakeOSSpec(callbackData, outputPath.directory.path);
    if (callback.referenceKind != 2) {
        CLIO_FormatAndDispatchText("UCBLookUpUnit:  file '%s' does not appear to be precompiled\n",
                                   OS_SpecToString((OSSpec *)&outputPath, data_005880e0, sizeof(outputName)));
        outputFile = (struct DropinFileValue *)callback.fileReference;
        CLDropinCallbacks_V10_FreeMemory(request, outputFile);
        return 2;
    }
    OS_GetFileTime(&inputPath, 0, &fileInfo);
    OS_TimeToMac(fileInfo, &fileValue);
    outputFile = (struct DropinFileValue *)callback.fileReference;
    if (fileValue != outputFile->value) {
        if (clState.pluginDebug != 0) {
            CLIO_FormatAndDispatchText(
                "UCBLookUpUnit:  file '%s' does not have internal timestamp that matches source unit's timestamp (0x%8x != 0x%8x)\n",
                OS_SpecToString((OSSpec *)&outputPath, data_005880e0, sizeof(outputName)), fileValue,
                outputFile->value);
        }
        outputFile = (struct DropinFileValue *)callback.fileReference;
        CLDropinCallbacks_V10_FreeMemory(request, outputFile);
        return 2;
    }
    if (CLFiles_FindDropinFileRecord(&default_target->generatedFiles, &outputPath) == 0) {
        entry = CLFiles_AllocDropinFileRecord();
        entry->inputPath = inputPath;
        OS_SpecToString(&entry->inputPath, entry->inputName, sizeof(outputName));
        entry->outputPath = outputPath;
        OS_SpecToString(&entry->outputPath, entry->outputName, sizeof(outputName));
        entry->selectedPlugin = settings->selectedPlugin;
        entry->kind = 2;
        entry->outputMask = settings->outputMask;
        entry->compilerCapabilities = settings->compilerCapabilities;
        entry->compilerFlags = settings->compilerFlags;
        entry->fileFlags = settings->fileFlags;
        {
            struct CLTarget *project = default_target;
            initialize_four_words(&entry->dependencies, &project->dependencyTable);
        }
        if (CLFiles_InsertIndexedListLinkAtFirstIndex(&default_target->generatedFiles, &entry->listEntry) == 0) {
            return 2;
        }
    }
    if (clState.pluginDebug != 0) {
        CLIO_FormatAndDispatchText("UCBLookUpUnit:  success for '%s'\n", inputName);
    }
    if (optsCmdLine.verbose != 0) {
        CLErrors_ForwardMessage(
            62, "unit symbol table",
            (int)CLProj_MakeRelativePath((OSSpec *)&outputPath, 0, data_005880e0, sizeof(outputName)));
    }
    *outputObject = callback.fileReference;
    outputValue->value = callback.referenceValue;
    return 0;
}
#pragma opt_propagation reset

unsigned int __stdcall log_callback(unsigned int unused1, unsigned int unused2)
{
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBSBMfiles");
    return 0;
}

int __stdcall store_precompiled_unit(void *compilerObject, char *filename, int storageHandle, int reserved)
{
    CWPluginPrivateContext *sourceObject;
    char name[0x104];
    OSSpec path;
    StorageHandle *unit;
    MemBuffer state;
    struct OperationRecord operation;
    unsigned int error;
    DropinFileRecord *sourceFile;
    int compilerState;

    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBStoreUnit");

    if (optsCompiler.canonicalIncludes != 0) {
        strcpy(name, filename);
    } else if (OS_CanonPath(filename, name) != 0) {
        return 3;
    }

    if ((compilerState = optsCompiler.sbmState) == 0 || compilerState == 1) {
        CWPluginPrivateContext &object = *(CWPluginPrivateContext *)compilerObject;
        UInt32 tag;
        if ((tag = (UInt32)object.contextSignature) == 0x436f6d70 || tag == 0x4c696e6b)
            sourceObject = (CWPluginPrivateContext *)compilerObject;
        else
            return 4;

        sourceFile = CLFiles_FindFileByIndex(&default_target->files, sourceObject->requestData.fileIndex);
        if (sourceFile == 0)
            CLIO_ReportAssertionFailure("srcfile != NULL", "CLCompilerLinkerDropin_V10.cpp", 0x312);

        if (optsCompiler.sbmPath[0] != 0) {
            error = CLProj_MakeOSSpecFromPath(clState.sbmPathSpec.path, name, 1, &path);
            if (error != 0) {
                if (clState.pluginDebug != 0)
                    CLIO_FormatAndDispatchText("UCBStoreUnit:  '%s' is a bad unit name (%s)\n", name,
                                               OS_GetErrText(error));
                return 3;
            }
        } else {
            error = OS_MakeFileSpec(name, &path);
            if (error != 0) {
                if (clState.pluginDebug != 0)
                    CLIO_FormatAndDispatchText("UCBStoreUnit:  '%s' is a bad filename (%s)\n", name,
                                               OS_GetErrText(error));
                return 3;
            }
        }

        error = fn_00426320(&path, sourceFile);
        if (error != 0) {
            if (clState.pluginDebug != 0)
                CLIO_FormatAndDispatchText("UCBStoreUnit:  could not make precompiled unit form of '%s' (%s)\n", name,
                                           OS_GetErrText(error));
            return 3;
        }

        if (optsCmdLine.verbose != 0)
            CLErrors_ForwardMessage(0x3d, "unit symbol table", CLProj_MakeRelativePath(&path, 0, data_005880e0, 0x104));

        CLDropinCallbacks_V10_SetStorageHandle((unsigned int)compilerObject, storageHandle, &unit);
        Memory_ExtractMemBuffer(unit, &state);
        error = TargetOptimizer_ppc_eabi_InitOperationRecord(&path, &state, 1, &operation);
        if (error != 0 || (error = TargetOptimizer_ppc_eabi_UnloadOperationRecord(&operation)) != 0) {
            CLErrors_ReportOSError(0x12, error, "precompiled unit",
                                   CLProj_MakeRelativePath(&path, 0, data_005880e0, 0x104));
            return 2;
        }
    }
    return 0;
}
unsigned int __stdcall free_allocation(unsigned int unused, unsigned int allocation)
{
    char *memory = (char *)allocation;
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBReleaseUnit");
    if (!memory)
        return 2U;
    free(memory);
    return 0U;
}

/* Record returned by call_00422a70_2; only these regions are used here. */

/* Record and context passed to the drop-in callback. */

int __stdcall report_alert(DropinContext *context, const char *text, short errorCode)
{
    const char *message;
    char errorMessage[256];

    if (optsCmdLine.verbose > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBOSAlert");
    }
    if (fn_004151f0() != 0) {
        return 1;
    }
    CError_GetErrorString(errorCode, errorMessage);
    if (text != 0) {
        message = text;
    } else {
        message = "";
    }
    CLIO_ReportDiagnostic(context->record->diagnosticType, 0, errorCode, 4, "%\n(%)\n", message, errorMessage);
    return 0;
}

/* Opaque drop-in request header and its referenced data. */

int __stdcall report_os_error_message(struct DropinRequest *request, const char *message, short errorCode)
{
    const char *messageText;
    char errorText[256];

    if (optsCmdLine.verbose > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBOSErrorMessage");
    }
    if (errorCode == 0) {
        return 0;
    }
    if (fn_004151f0() != 0) {
        return 1;
    }

    CError_GetErrorString(errorCode, errorText);
    messageText = message ? message : "";
    CLIO_ReportDiagnostic(request->data->reference, 0, errorCode, 4, "%\n%\n", messageText, errorText);
    return 0;
}

unsigned int __stdcall get_object_file_spec(unsigned int unused, unsigned int key, unsigned int output)
{
    DropinFileRecord *record;
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetStoredObjectFileSpec");

    record = CLFiles_FindFileByIndex(&default_target->files, key);
    if (!record)
        return 9U;
    if ((unsigned short)record->kind != 2U) {
        CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 989, "Lost stored object file spec for '%s'\n",
                                     record->inputName);
        return 2U;
    }
    MacSpecs_MakeCWFileSpecFromString((char *)&record->outputPath, (CWFileSpec *)output);
    return 0U;
}

unsigned int __stdcall fn_00426da0(unsigned int unused, NameSpaceName *name, unsigned int unused2)
{
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetModifiedFiles");
    name->next = 0;
    CLErrors_ReportInternalError("CLCompilerLinkerDropin_V10.cpp", 945, "CWGetModifiedFiles not implemented!\n");
    return 0;
}

int __stdcall fn_00425ef0(unsigned int callbackContext, unsigned int callbackData)
{
    fn_004151d0(12U);
    if (optsCmdLine.verbose > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBDisplayLines");
    if (fn_004151f0() != 0)
        return 1;
    return 0;
}

unsigned int __stdcall get_file_output_path(unsigned int context, unsigned int fileIndex,
                                            unsigned int outputSpecAddress)
{
    struct DropinFileRecord *file;
    if (optsCmdLine.verbose > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetSuggestedObjectFileSpec");
    }
    file = CLFiles_FindFileByIndex(&default_target->files, fileIndex);
    if (file == NULL) {
        return 9;
    }
    if (!CLFileOps_SetupOutputPath(file, 2)) {
        return 2;
    }
    MacSpecs_MakeCWFileSpecFromString((char *)&file->outputPath, (CWFileSpec *)outputSpecAddress);
    return 0;
}

unsigned int __stdcall copy_name_with_p_extension(unsigned int unused, const char *name, char *output)
{
    if (optsCmdLine.verbose > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBUnitNameToFileName");
    }
    strcpy(output, name);
    if (ClientGlue_CompareLowercaseStrings(output + strlen(output) - 2, ".p") != 0) {
        strcat(output, ".p");
    }
    return 0;
}

/* Opaque record carrying the string lookup value. */

/* Packed string lookup result; the preceding data is not used here. */

unsigned int fn_00426320(OSSpec *destination, DropinFileRecord *record)
{
    const char *text;
    unsigned int startsWithDot;
    text = CLPlugins_GetObjectFlags(record->selectedPlugin)->pchFileExt;
    startsWithDot = text ? (text[0] == '.') : 0;
    if (text == 0)
        text = ".sbm";
    return CLProj_SetFileExtension(destination->name, text, startsWithDot);
}

/* Drop-in record with an opaque prefix and a stored value. */

unsigned int __stdcall clear_primary_reference_value(unsigned int unused0, unsigned int recordKey, unsigned int unused2)
{
    DropinFileRecord *record;
    if (optsCmdLine.verbose > (short)3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBFreeObjectData");
    }
    record = CLFiles_FindFileByIndex(&default_target->files, recordKey);
    if (record == 0)
        return 9U;
    if ((unsigned int)record->objectData != 0U) {
        Memory_FreeHandle((StorageHandle *)record->objectData);
        record->objectData = 0U;
        return 0U;
    }
    return 3U;
}
}

extern "C" {
}

PluginC::PluginC() : PluginA(clState.plugintype, sizeof(PluginC))
{
    CLTarget *globals;
    auxiliary = new InitializationAuxiliaryState;
    memset(auxiliary, 0, 0x136);

    if (default_target != 0) {
        globals = default_target;
        auxiliary->defaultValue1 = globals->cpu;
        globals = default_target;
        auxiliary->defaultValue2 = globals->os;
    } else {
        auxiliary->defaultValue1 = auxiliary->defaultValue2 = 0x2a2a2a2a;
    }

    initialValue = 0;
    memset(&initialStorage, 0, 0x46);
    value1 = 0;
    value2 = 0;
    byte1 = byte2 = byte3 = 0;
    byte4 = 0;
    byte5 = 0;
    shortValue = 0;
    memset(&shortStorage, 0, 0x10);
    value3 = 0;
    value4 = 0;
    value5 = 0;
    value6 = 0;
    value7 = 0;
    memset(&trailingStorage, 0, 0xa2);
    sharedValue = "p[B";
}
