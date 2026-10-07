#define CERROR_FILE "CLDropinCallbacks_V10.c"
#pragma bool off

#include "compiler/common.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "compiler/types.h"
#include "compiler/win32.h"
#include "compiler/CExpr2.h"
#include "compiler/CPrep.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLBrowser.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#include "driver/CLDependencies.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLLicenses.h"
#include "driver/CLLoadAndCache.h"
#include "driver/CLMain.h"
#include "driver/CLOverlays.h"
#include "driver/CLPlugins.h"
#include "driver/CLPrefs.h"
#include "driver/CLSegs.h"
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
#include "driver/StringUtils.h"
extern "C" {

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "mwcc/Plugins.h"
/* Declarations gathered from the merged files. */

/* Header, descriptor and state buffers for the drop-in callback interface. */

/* Stored record and its exported representation. */

/* Scratch results from the conversion and lookup helpers. */

/* Six-byte conversion result. */

#include "driver/CLFileOps.h"

#include "driver/CLFileOps.h"

#include "driver/CLFileOps.h"
}

extern "C" {
static inline void SetFileDependencyCode(struct ExportedRecord *fileInfo, short dependencyCode)
{
    fileInfo->code = dependencyCode;
}

int __stdcall get_file_info(int unused, int key, int unusedFlags, struct ExportedRecord *result)
{
    union TimestampRecord {
        struct StoredRecord record;
        DropinFileRecord file;
    } *record;
    struct PackedConversionResult conversion;
    short browserCode;
    char textBuffer[260];
    if (DAT_00541b28 > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBGetFileInfo");
    }
    record = (union TimestampRecord *)CLFiles_FindFileByIndex(&default_target->files, key);
    if (record == 0) {
        return 9;
    }
    memset(result, 0, sizeof(*result));
    MacSpecs_MakeCWFileSpecFromString(record->file.inputPath.directory.path, &result->fileReference);
    OS_TimeToMac(record->file.sourceFileTime, &result->convertedValue);
    CLOverlays_ConvertSecondsToTimestamp(result->convertedValue, &conversion);
    CLOverlays_ConvertTimestampTo1904EpochSeconds(conversion.value, &result->convertedValue);
    result->shortValue = record->file.lookupPathIndex;
    result->optionB = record->file.configurationReceivedWithoutCapability;
    result->optionC = record->file.configurationReceivedWithCapability;
    result->optionD = record->file.requiresLink;
    result->optionE = record->record.optionE;
    result->optionF = record->record.optionF;
    result->optionG = record->record.optionG;
    result->optionH = 0;
    result->optionA = record->record.optionA;
    OS_TimeToMac(record->file.callbackFileTime, &result->secondTimestamp);
    if (record->file.selectedPlugin != 0) {
        strncpy(&result->string, CLPlugins_GetName(record->file.selectedPlugin), 31);
        result->stringEnd = 0;
    } else {
        result->string = 0;
    }
    SetFileDependencyCode(result, record->file.dependencyOption);
    result->flag = record->file.dependencyStatusNegative;
    result->fileType = record->file.fileType;
    result->secondValue = record->record.secondValue;
    if (data_00541c0a != 0) {
        if (CLBrowser_LookupValue(&data_00587570,
                                  OS_SpecToString(&record->file.inputPath, textBuffer, sizeof(textBuffer)),
                                  &browserCode) != 0) {
            result->flag = 1;
            SetFileDependencyCode(result, browserCode);
        } else {
            result->flag = 0;
            SetFileDependencyCode(result, browserCode);
        }
    } else {
        result->flag = 0;
        SetFileDependencyCode(result, 0);
    }
    result->finalValue = 0;
    result->optionI = 0;
    return 0;
}

/* Output record populated by the lookup. */

Boolean lookup_file(DropinRequest *unused, char *key, DropinFileCallback *output, OSSpec *argument, Boolean *found)
{
    char *value;
    ChainRecord *result;
    value = CLProj_GetFileName(key);
    result = CLFiles_FindChainRecord(default_target->fileLookup, value);
    if (result) {
        if (output->suppressFileReferenceLookup == 0) {
            CLLoadAndCache_CopyStorageHandleData((struct StorageHandle *)result->object,
                                                 (void **)&output->fileReference, &output->referenceValue);
            *found = 1;
        }
        output->referenceKind = 1;
        output->lookupResult = 0;
        output->callbackState = 0;
        output->lookupFailed = 0;
        output->output.fileData.file.directoryId = 0;
        output->output.fileData.file.volumeRef = output->output.fileData.file.directoryId;
        c2pstrcpy(output->output.fileData.file.name, value);
        OS_MakeFileSpec(value, argument);
        if (data_00541d0e != 0)
            CLIO_FormatAndDispatchText("%s\n", value);
        return 1;
    }
    return 0;
}

Boolean lookup_dependency_file(DropinRequest *state, char *request, DropinFileCallback *flags, OSSpec *fileSpec,
                               Boolean *dependencyFound)
{
    Boolean lookupEnabled;
    int fileKey;
    struct DropinFileRecord *file;
    int browserStatus;
    SInt32 dependencyIndex;
    short lookupResult;
    char path[260];
    lookupEnabled = flags->enableDependencyLookup != 0 || data_00541b42 != 0;
    if (CLDependencies_FindFile(&default_target->dependencyTable, request, lookupEnabled, fileSpec, &dependencyIndex) !=
        0) {
        if (state->signature == 1131375984) {
            if (flags->fileKey < 0) {
                fileKey = state->fileKey;
            } else {
                fileKey = flags->fileKey;
            }
            file = CLFiles_FindFileByIndex(&default_target->files, fileKey);
            if (file != 0) {
                CLDependencies_InsertDependencyIfAbsent(&file->dependencies, dependencyIndex, 0, 0,
                                                        (int)flags->searchOption, &flags->callbackState);
            }
            if (data_00541c0a != 0) {
                browserStatus = CLBrowser_FindOrAddLookupEntry(
                    &data_00587570, OS_SpecToString(fileSpec, path, sizeof(path)), &lookupResult);
                if (browserStatus == 0) {
                    return 2;
                }
                if (browserStatus < 0) {
                    flags->lookupFailed = 1;
                    flags->lookupResult = lookupResult;
                } else {
                    flags->lookupFailed = 0;
                    flags->lookupResult = lookupResult;
                }
            } else {
                flags->lookupFailed = 0;
                flags->lookupResult = 0;
            }
        }
        MacSpecs_MakeCWFileSpecFromString(fileSpec->directory.path, &flags->output);
        return 1;
    }
    return 0;
}

Boolean insert_dependency_from_path(DropinRequest *descriptor, char *argument, DropinFileCallback *state,
                                    OSSpec *context, Boolean *changed)
{
    CWPluginPrivateContext *acceptedDescriptor;
    struct DropinFileRecord *entry;
    int flag;
    unsigned char callbackFlag;
    char *callbackState;
    if (data_00541d0d != 0) {
        if (descriptor->signature == 1131375984 || descriptor->signature == 1281977963) {
            acceptedDescriptor = (CWPluginPrivateContext *)descriptor;
        } else {
            return 0;
        }
        entry = CLFiles_FindFileByIndex(&default_target->files, acceptedDescriptor->requestData.fileIndex);
        if (entry == 0) {
            CLIO_ReportAssertionFailure("file != NULL", "CLDropinCallbacks_V10.cpp", 496);
        }
        CLProj_MakeOSSpecFromPath(entry->inputPath.directory.path, argument, 1, context);
        flag = state->enableDependencyLookup != 0 || data_00541b42 != 0;
        callbackFlag = !flag;
        callbackState = &state->callbackState;
        CLDependencies_InsertDependencyIfAbsent(&entry->dependencies, -1, context, callbackFlag,
                                                (int)state->searchOption, callbackState);
        state->lookupResult = 0;
        state->lookupFailed = 0;
        if (state->suppressFileReferenceLookup == 0) {
            state->fileReference = xstrdup("");
            state->referenceValue = 0;
            state->referenceKind = 1;
            *changed = 1;
        } else {
            state->fileReference = 0;
            state->referenceValue = 0;
            state->referenceKind = 0;
        }
        MacSpecs_MakeCWFileSpecFromString(context->directory.path, (CWFileSpec *)&state->output);
        return 1;
    }
    return 0;
}

SInt32 __stdcall CLDropinCallbacks_V10_FindAndLoadFile(DropinRequest *dropin, char *inputPath,
                                                       DropinFileCallback *parameters)
{
    OSSpec callbackPath;
    Boolean handled;
    char resolvedPath[260];
    char message[64];

    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBFindAndLoadFile");

    fn_004151d0(8);
    if (fn_004151f0())
        return 1;

    if (data_00541e10 != 0) {
        strcpy(resolvedPath, inputPath);
    } else if (OS_CanonPath(inputPath, resolvedPath) != 0) {
        return 3;
    }

    handled = 0;
    if (!lookup_file(dropin, resolvedPath, parameters, &callbackPath, &handled) &&
        !lookup_dependency_file(dropin, resolvedPath, parameters, &callbackPath, &handled) &&
        !insert_dependency_from_path(dropin, resolvedPath, parameters, &callbackPath, &handled))
        return 8;

    if (data_00541d0e != 0)
        CLIO_FormatAndDispatchText("%s\n",
                                   CLProj_MakeRelativePath(&callbackPath, 0, data_005880e0, sizeof(resolvedPath)));

    if (dropin->signature == 0x436f6d70) {
        SInt16 verbosity;
        if ((verbosity = DAT_00541b28) > 2) {
            const char *detail, *format;
            sprintf(message, " (browse fileID %d)", parameters->lookupResult);
            detail = parameters->lookupFailed != 0 ? message : "";
            format = parameters->callbackState != 0 ? "Included" : "Including";
            CLErrors_ForwardMessage(0x15, format, OS_SpecToString(&callbackPath, data_005880e0, sizeof(resolvedPath)),
                                    detail);
        } else if (verbosity > 1) {
            if (parameters->callbackState == 0)
                CLErrors_ForwardMessage(0x15, "Including",
                                        OS_SpecToString(&callbackPath, data_005880e0, sizeof(resolvedPath)), "");
        }
    } else {
        parameters->lookupResult = 0;
        parameters->lookupFailed = 0;
        parameters->callbackState = 0;
    }

    if (handled == 0) {
        if (parameters->suppressFileReferenceLookup == 0)
            return CLDropinCallbacks_V10_GetFileText(dropin, &parameters->output, (void **)&parameters->fileReference,
                                                     &parameters->referenceValue, &parameters->referenceKind);
        parameters->fileReference = 0;
        parameters->referenceValue = 0;
        parameters->referenceKind = 0;
        return 0;
    }
    return 0;
}

/* File reference supplied by the callback client. */

int __stdcall CLDropinCallbacks_V10_GetFileText(void *context, CWFileSpec *file, void **result1, unsigned int *result2,
                                                SInt16 *status)
{
    StorageHandle *object = 0;
    UInt8 flag;
    OSSpec path;
    char fileName[0x40];
    int error;
    ChainRecord *lookup;

    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBGetFileText");
    MacSpecs_MakeOSSpec(file, path.directory.path);
    error = CLLoadAndCache_GetFileText(&path, &object, &flag);
    if (error != 0) {
        p2cstrcpy(fileName, file->fileData.file.name);
        if (file->fileData.file.volumeRef != 0 || file->fileData.file.directoryId != 0 ||
            (lookup = CLFiles_FindChainRecord(default_target->fileLookup, fileName)) == NULL) {
            CLErrors_ReportOSError(0x5d, error, OS_SpecToString(&path, data_005880e0, 0x104));
            return 8;
        }
        CLLoadAndCache_CopyStorageHandleData((struct StorageHandle *)lookup->object, result1, result2);
        *status = 1;
        OS_MakeFileSpec(fileName, &path);
    } else {
        if (object != 0) {
            CLLoadAndCache_CopyStorageHandleData(object, result1, result2);
            CLBrowser_ReleaseBuffer(object);
        }
        if (object != 0) {
            if (flag == 0)
                *status = 1;
            else
                *status = 2;
        } else {
            *status = 0;
        }
    }
    return 0;
}

static inline void CLDropinCallbacks_V10_00425560_run1(CallbackPathEntry *p0, struct AccessPathEntry *p1)
{
    p0->hasChildren = (char)(p1->children != 0);
    if (p1->children != 0) {
        p0->childCount = count_access_paths_recursive(p1->children);
        if (p0->childFiles != 0) {
            free(p0->childFiles);
        }
        p0->childFiles = (CWFileSpec *)xmalloc(0, p0->childCount * 70);
        copy_access_paths_to_file_specs_checked(p0->childFiles, p1->children, p0->childCount);
    } else {
        p0->childCount = 0;
        p0->childFiles = 0;
    }
}

/* Cached callback entries and their counts. */

/* Callback request fields; intervening fields are not used here. */

/* Cached callback path entry. */

/* Callback request layout. */

/* Cached callback state. */

/* Callback operation parameters. */

/* Callback input: optional name, data, and mode byte. */

static inline void SetCallbackStorageHandle(DropinRequest *request, StorageHandle *data, StorageHandle **storage)
{
    CLDropinCallbacks_V10_SetStorageHandle((unsigned int)request, (unsigned int)data, storage);
}

unsigned int __stdcall CLDropinCallbacks_V10_FreeMemory(struct DropinRequest *request, void *memory)
{
    unsigned int result = 0U;
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBReleaseFileText");
    if (memory != NULL)
        free(memory);
    else
        result = 3U;
    return (short)result;
}

/* Callback records carry a 16-bit value at offset 32. */

unsigned int __stdcall lookup_callback_record(unsigned int unused, unsigned int key, CallbackRecord *record)
{
    struct PayloadWithValue *found;
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetSegmentInfo");

    if (default_target->linkage != 1U)
        return 4U;

    found = CLSegs_GetValue(&default_target->lookupPaths, key);
    if (!found)
        return 513U;

    strcpy(record->name, found->name);
    record->value = found->value;
    return 0U;
}

/* Text and values returned by the overlay callback. */

unsigned int __stdcall get_overlay_group_info(unsigned int callback, int index, struct CallbackOverlayRecord *record)
{
    struct CLOverlayEntry *entry;
    unsigned int result;

    if (3 < DAT_00541b28) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBGetOverlay1GroupInfo");
    }
    entry = CLOverlays_GetGroupByIndex(&default_target->overlays, index);
    if (default_target->linkage != 2) {
        return 4;
    }
    if (entry != 0) {
        strcpy(record->text, entry->name);
        record->values[0] = entry->values.first;
        record->values[1] = entry->values.second;
        result = CLOverlays_CountOverlays(entry);
        record->result = result;
        return 0;
    }
    return 3;
}

unsigned int __stdcall call_overlays_and_translate_status(unsigned int callbackContext, unsigned int groupIndex,
                                                          unsigned int allocationIndex, unsigned int valueIndex,
                                                          unsigned int *result)
{
    int allocationValue;

    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetOverlay1FileInfo");
    if (default_target->linkage != 2)
        return 4;
    allocationValue =
        CLOverlays_GetAllocationValueByGroupIndex(&default_target->overlays, groupIndex, allocationIndex, valueIndex);
    if (allocationValue >= 0) {
        *result = allocationValue;
        return 0;
    }
    return 3;
}

/* Callback result storage supplied by the caller. */

/* Shared callback data with a payload at offset 28. */

unsigned int __stdcall lookup_overlay_allocation(unsigned int unused, unsigned int groupIndex,
                                                 unsigned int allocationIndex, struct DropinResultStorage *result)
{
    struct OverlayAllocation *allocation;
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetOverlay1Info");
    if (default_target->linkage != 2U)
        return 4U;
    allocation = CLOverlays_GetAllocationByGroupIndex(&default_target->overlays, groupIndex, allocationIndex);
    if (allocation) {
        strcpy(result->name, allocation->name);
        result->value = CLOverlays_GetValueCount(allocation);
        return 0U;
    }
    return 3U;
}

/* Callback records whose fields have no compiler-header counterpart. */

int __stdcall report_message(struct DiagnosticContext *context, struct DiagnosticLocation *location, char *message,
                             char *detail, short kind, int argument)
{
    int messageHasNewline;
    char lastMessageChar;
    int detailHasNewline;
    char lastDetailChar;
    int diagnosticKind;
    DiagnosticSourcePosition record;
    /* The format of a message, then of a message and its detail, by which of them end with a newline. */
    static char *data_0054cbdc[2][4] = {{"%\n", "%", "%\n", "%"}, {"%\n%\n", "%%\n", "%\n%", "%%"}};

    if (DAT_00541b28 > 4) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBReportMessage");
    }
    if (message == 0) {
        message = "";
        messageHasNewline = 0;
    } else {
        lastMessageChar = ((char *)(strlen(message) + (int)message - 1))[0];
        messageHasNewline = lastMessageChar == 13 || lastMessageChar == 10;
    }
    if (fn_004151f0()) {
        return 1;
    }
    if (+kind == 2) {
        diagnosticKind = 3;
    } else if (+kind == 1) {
        diagnosticKind = 2;
    } else {
        diagnosticKind = 1;
    }
    if (location == 0) {
        if (detail != 0 && *detail != 0) {
            lastDetailChar = ((char *)(strlen(detail) + (int)detail - 1))[0];
            detailHasNewline = 1;
            if (lastDetailChar != 13 && lastDetailChar != 10) {
                detailHasNewline = 0;
            }
            CLIO_ReportDiagnostic(context->target->identifier, 0, argument, diagnosticKind,
                                  data_0054cbdc[1][(detailHasNewline << 1) + messageHasNewline], message, detail);
        } else {
            CLIO_ReportDiagnostic(context->target->identifier, 0, argument, diagnosticKind,
                                  data_0054cbdc[0][messageHasNewline], message);
        }
    } else {
        MacSpecs_MakeOSSpec((CWFileSpec *)&context->sourceData, record.primaryFile.directory.path);
        MacSpecs_MakeOSSpec(&location->file, record.file.directory.path);
        record.sourceLine = detail;
        record.line = location->line;
        record.column = location->column;
        record.length = location->length;
        record.selectionOffset = location->selectionOffset;
        record.selectionLength = location->selectionLength;
        if (record.column < 0) {
            record.column = 0;
        }
        if (record.length < 0) {
            record.length = 0;
        }
        if (record.selectionOffset < 0) {
            record.selectionOffset = 0;
        }
        if (record.selectionLength < 0) {
            record.selectionLength = 0;
        }
        CLIO_ReportDiagnostic(context->target->identifier, &record, argument, diagnosticKind,
                              data_0054cbdc[0][messageHasNewline], message);
    }
    return 0;
}

int __stdcall emit_alert_messages(DropinContext *ctx, char *message1, char *message2, char *message3, char *message4)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBAlert");
    if (fn_004151f0())
        return 1;

    if (message4 != 0)
        CLIO_ReportDiagnostic(ctx->record->diagnosticType, 0, 0, 4, "%\n%\n%\n%\n", message1 ? message1 : "",
                              message2 ? message2 : "", message3 ? message3 : "", message4);
    else if (message3 != 0)
        CLIO_ReportDiagnostic(ctx->record->diagnosticType, 0, 0, 4, "%\n%\n%\n", message1 ? message1 : "",
                              message2 ? message2 : "", message3);
    else if (message2 != 0)
        CLIO_ReportDiagnostic(ctx->record->diagnosticType, 0, 0, 4, "%\n%\n", message1 ? message1 : "", message2);
    else
        CLIO_ReportDiagnostic(ctx->record->diagnosticType, 0, 0, 4, "%\n", message1 ? message1 : "");
    return 0;
}

unsigned int __stdcall report_message_detail(CWPluginPrivateContext *callback, char *message, char *detail)
{
    {
        short verbosity = DAT_00541b28;
        if (verbosity > 4) {
            CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBShowStatus");
        }
    }
    if (fn_004151f0()) {
        return 1;
    }
    {
        short verbosity = DAT_00541b28;
        if (verbosity > 1) {
            if ((!message || !*message) && (!detail || !*detail)) {
                return 0;
            }
            if (detail && *detail) {
                CLIO_ReportDiagnostic((Plugin *)*(unsigned int *)callback->shellContext, 0, 0, 5, "%\n%\n",
                                      (message ? message : ""), (detail ? detail : ""));
            } else {
                CLIO_ReportDiagnostic((Plugin *)*(unsigned int *)callback->shellContext, 0, 0, 5, "%\n",
                                      (message ? message : ""));
            }
        }
    }
    return 0;
}

unsigned int __stdcall fn_00424540(unsigned int argument)
{
    if (DAT_00541b28 > 4) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBUserBreak");
    }
    fn_004151d0(8);
    if (fn_004151f0() != 0) {
        return 1;
    }
    return 0;
}

unsigned int __stdcall copy_named_destination_to_temporary(unsigned int context, char *name, unsigned int *result)
{
    unsigned int local;
    NameTableEntry *handle;
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetNamedPreferences");
    handle = CLPrefs_FindNameTableEntry(name);
    if (handle != 0) {
        if (DAT_00541b28 > 2)
            CLErrors_ForwardMessage(0x53U, name);
        CLDropinCallbacks_V10_SetStorageHandle(context, *result, &local);
        local = (unsigned int)CLPrefs_CopyDestinationToTemporary(handle);
        CLDropinCallbacks_V10_StoreValue(context, local, result);
        return 0U;
    }
    CLErrors_EmitDiagnostic(0x5bU, name);
    *result = 0;
    return 2U;
}

unsigned int __stdcall report_store_plugin_data_not_implemented(unsigned int pluginContext, unsigned int dataKey,
                                                                unsigned int pluginData, unsigned int dataSize)
{
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBStorePluginData");
    CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 1261, "UCBStorePluginData not implemented");
    return 2U;
}

unsigned int __stdcall fn_00424660(unsigned int context, unsigned int plugin, unsigned int data, unsigned int size)
{
    if (DAT_00541b28 > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetPluginData");
    }
    CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 1280, "UCBGetPluginData not implemented");
    return 2U;
}

/* 0x541b28, word compare */
/* cdecl, 2 args */
/* stdcall, 2 args */
/* stdcall, 1 arg */
/* stdcall, 2 args */
/* 0x587c84, pointer */
/* cdecl, 2 args */
/* cdecl, 1 arg */
/* stdcall, 2 args */
/* stdcall, 3 args */

int __stdcall set_mod_date(int callbackContext, char *name, SInt32 *modificationDate, int reserved)
{
    SInt32 macTime;
    OSSpec fileSpec;
    _FILETIME fileTime;
    SInt32 fileIndex;
    DropinFileRecord *file;

    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBSetModDate");

    MacSpecs_MakeOSSpec((CWFileSpec *)name, fileSpec.directory.path);

    macTime = *modificationDate;
    if (macTime == 0)
        OS_GetTime(&fileTime);
    else
        OS_MacToTime(macTime, &fileTime);

    for (fileIndex = 0; fileIndex < CLFiles_GetIndex(&default_target->files); fileIndex++) {
        file = (DropinFileRecord *)CLFiles_FindFileByIndex(&default_target->files, fileIndex);
        if (OS_EqualSpec(&file->inputPath, &fileSpec)) {
            file->sourceFileTime = fileTime;
        } else if (OS_EqualSpec(&file->outputPath, &fileSpec)) {
            file->callbackFileTime = fileTime;
        }
    }

    if (OS_SetFileTime(&fileSpec, 0, &fileTime) == 0)
        return 0;
    return 2;
}

__stdcall SInt32 add_project_entry(DropinRequest *context, CWFileSpec *file, UInt8 ignored, FileOpenOptions *args,
                                   UInt32 *objectId)
{
    DropinFileRecord *fileRecord;
    char *extensionStart;
    SInt32 failed;
    SInt32 index;
    struct CLOverlayEntry *overlayGroup;
    struct OverlayAllocation *overlay;
    Plugin *plugin;
    ObjFlagsData *objectFlags;
    OSSpec sourcePath;
    UInt32 fileType;
    char extension[16];
    unsigned int flags;
    char filename[260];
    SInt32 result;
    struct CLTarget *target;

    if (DAT_00541b28 > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBAddProjectEntry");
    }
    MacSpecs_MakeOSSpec(file, sourcePath.directory.path);
    if (OS_IsDir(sourcePath.directory.path) != 0) {
        return 3;
    }
    MsDos_CopyStringToBuffer(sourcePath.name, filename, sizeof(filename));
    extensionStart = filename + strlen(filename) - 1;
    while (extensionStart > filename && *extensionStart != '.') {
        extensionStart--;
    }
    if (extensionStart <= filename) {
        extensionStart = ".";
    }
    strncpy(extension, extensionStart, sizeof(extension) - 1);
    extension[sizeof(extension) - 1] = 0;
    CLProj_MakeRelativePath(&sourcePath, 0, filename, sizeof(filename));
    if (DAT_00541b28 > 2) {
        CLErrors_ForwardMessage(0x4d, " to project", filename);
    }
    if (MacFileTypes_GetFileType(&sourcePath, &fileType) != 0) {
        fileType = 0x54455854;
    }
    plugin = CLPlugins_FindTargetPluginBySelectorOptionName(0, 0x2a2a2a2a, 0x2a2a2a2a, 0x2a2a2a2a, fileType, extension,
                                                            0x2a2a2a2a);
    if (plugin != 0 && default_target->linker != 0 &&
        CLPlugins_MatchTarget(plugin, default_target->cpu, default_target->os, 0) == 0) {
        CLErrors_EmitDiagnostic(0x4c, filename);
    }
    if (plugin == 0) {
        plugin =
            CLPlugins_FindMatchingTargetPlugin(0, default_target->cpu, default_target->os, plugin_type, data_005871d0);
    }
    if (plugin != 0) {
        if (CLPlugins_FindFileMapValue(plugin, fileType, extension, &flags) == 0) {
            flags = 0;
            if (ignored == 0 && data_00541c08 == 0 && data_00541d0c != 1) {
                CLErrors_ForwardMessageArguments(0x49, "file", filename);
            }
            if (ignored != 0) {
                flags = 0x10000000;
            }
        } else if (ignored == 0 && (flags & 0x10000000) != 0 && data_00541c08 == 0) {
            if ((data_00541b1c.payload[0] == 0 || (data_00541b1c.payload[0] & 6) != 0) && data_00541d0c != 1) {
                CLErrors_ForwardMessageArguments(0x1c, filename);
            } else {
                flags &= 0xefffffff;
            }
        }
        if (data_00541c08 != 0 && ignored == 0) {
            flags &= 0xefffffff;
        }
        if (DAT_00587324 != 0) {
            CLIO_FormatAndDispatchText("Using plugin '%s' for '%s'\n", CLPlugins_GetName(plugin), filename);
            CLIO_FormatAndDispatchText("[flags: %s, %s, %s, %s]\n",
                                       (flags & 0x80000000) ? "precompile" : "don't precompile",
                                       (flags & 0x40000000) ? "launchable" : "not launchable",
                                       (flags & 0x20000000) ? "resource file" : "not resource file",
                                       (flags & 0x10000000) ? "ignored" : "used");
        }
    } else {
        CLErrors_EmitDiagnostic(0x4a, filename);
        flags = 0x10000000;
    }
    fileRecord = CLFiles_AllocDropinFileRecord();
    if (fileRecord == 0) {
        return 7;
    }
    target = default_target;
    initialize_four_words(&fileRecord->dependencies, &target->dependencyTable);
    index = args->lookupPathIndex;
    if (index == -1) {
        target = default_target;
        index = CLSegs_GetCount(&target->lookupPaths) - 1;
    }
    fileRecord->lookupPathIndex = (SInt16)index;
    strcpy(fileRecord->inputName, filename);
    fileRecord->inputPath = sourcePath;
    OS_GetFileTime(&fileRecord->inputPath, 0, &fileRecord->sourceFileTime);
    OS_GetTime(&fileRecord->callbackFileTime);
    fileRecord->outputName[0] = 0;
    memset(&fileRecord->outputPath, 0, sizeof(fileRecord->outputPath));
    fileRecord->kind = 0;
    fileRecord->temporaryOutputMask = 0;
    fileRecord->validatedOutputMask = fileRecord->temporaryOutputMask;
    fileRecord->outputMask = fileRecord->validatedOutputMask;
    if (plugin != 0 && CLPlugins_GetType(plugin) != 0x436f6d70) {
        fileRecord->selectedPlugin = 0;
    } else {
        fileRecord->selectedPlugin = plugin;
    }
    fileRecord->fileType = fileType;
    fileRecord->fileFlags = flags;
    if (fileRecord->selectedPlugin != 0) {
        fileRecord->compilerCapabilities = CLPlugins_GetPluginDesc(fileRecord->selectedPlugin)->flags;
    } else {
        fileRecord->compilerCapabilities = 0;
    }
    fileRecord->fileOpenFlag2 = args->flag2;
    fileRecord->fileOpenFlag3 = args->flag3;
    fileRecord->fileOpenFlag1 = args->flag1;
    fileRecord->requiresLink = (flags & 0x20000000) != 0;
    if (fileRecord->selectedPlugin != 0) {
        objectFlags = (ObjFlagsData *)CLPlugins_GetObjectFlags(fileRecord->selectedPlugin);
        fileRecord->compilerFlags = objectFlags->compilerFlags;
    } else {
        fileRecord->compilerFlags = 0;
    }
    fileRecord->inputArgumentMask = 0;
    if ((fileRecord->fileFlags & 0x10000000) == 0) {
        if (fileRecord->selectedPlugin != 0) {
            fileRecord->inputArgumentMask |= 1;
            if ((fileRecord->compilerFlags & 0x80000000) == 0) {
                fileRecord->inputArgumentMask |= 2;
            }
        } else if (plugin != 0) {
            fileRecord->inputArgumentMask |= 2;
        }
    }
    if (fileRecord->requiresLink != 0) {
        fileRecord->inputArgumentMask |= 2;
    }
    fileRecord->outputArgumentMask = fileRecord->inputArgumentMask & 1;
    if ((fileRecord->inputArgumentMask & 1) != 0 && (fileRecord->fileFlags & 0x80000000) == 0 && data_00541d0c != 1 &&
        (fileRecord->compilerFlags & 0x80000000) != 0) {
        fileRecord->outputArgumentMask |= 2;
    }
    *objectId = 0;
    index = args->fileIndex;
    if (index != -1) {
        failed = !CLFiles_InsertIndexedListLinkAtFirstIndex(&default_target->files, &fileRecord->listEntry);
    } else {
        failed = !CLFiles_InsertIndexedListLink(&default_target->files, &fileRecord->listEntry, index);
    }
    if (failed) {
        return 2;
    }
    *objectId = fileRecord->listEntry.index;
    context->addedFileCount++;
    if (default_target->linkage == 2) {
        index = args->overlayIndex;
        if (index == -1) {
            index = CLOverlays_CountGroups(&default_target->overlays) - 1;
        }
        overlayGroup = CLOverlays_GetGroupByIndex(&default_target->overlays, index);
        if (overlayGroup != 0) {
            index = args->overlayTableIndex;
            if (index == -1) {
                index = CLOverlays_CountOverlays(overlayGroup) - 1;
            }
            overlay = CLOverlays_GetOverlayAtIndex(overlayGroup, index);
            if (overlay != 0) {
                if (CLOverlays_AppendEntry(overlay, fileRecord->listEntry.index, &result) == 0) {
                    return 2;
                }
            } else {
                return 2;
            }
        } else {
            return 2;
        }
    }
    return 0;
}

int __stdcall create_new_text_document(DropinRequest *request, DropinCallbackData *document)
{
    DropinFileRecord *file;
    ChainRecord *entry;
    int result;
    SInt32 outputSize;
    SInt32 stdoutSize;
    SInt32 resourceSize;
    StorageHandle *stdoutStorage;
    StorageHandle *resourceStorage;
    CWFileSpec fileSpec;
    FileOpenOptions options;
    UInt32 status;
    unsigned char pascalName[256];

    if (DAT_00541b28 > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBCreateNewTextDocument");
    }
    if (request->signature == 1131375984 || request->signature == 1281977963) {
        if (document->name == 0 || memcmp(document->name, ">stdout", 8) != 0) {
            file = CLFiles_FindFileByIndex(&default_target->files, request->fileKey);
            if (file == 0) {
                return 9;
            }
            if (file->outputStorage != 0) {
                outputSize = Memory_GetHandleSize(file->outputStorage);
                if (CLIO_WriteStorageToStdout(file->outputStorage, outputSize, 1) == 0) {
                    return 2;
                }
                Memory_FreeHandle(file->outputStorage);
            }
            SetCallbackStorageHandle(request, document->storage, &file->outputStorage);
            OS_GetTime(&file->callbackFileTime);
            return 0;
        } else {
            SetCallbackStorageHandle(request, document->storage, &stdoutStorage);
            stdoutSize = Memory_GetHandleSize(stdoutStorage);
            if (CLIO_WriteStorageToStdout(stdoutStorage, stdoutSize, 0) == 0) {
                return 2;
            }
            return 0;
        }
    }
    if (request->signature == 1348563571) {
        SetCallbackStorageHandle(request, document->storage, &resourceStorage);
        if (document->addToProject != 0) {
            if (document->name == 0) {
                return 3;
            }
            entry = CLFiles_CreateChainRecord(document->name, resourceStorage);
            if (entry == 0) {
                return 7;
            }
            if (CLFiles_AppendChainRecord(&default_target->fileLookup, entry) == 0) {
                return 2;
            }
            options.auxiliaryValue = 0;
            options.fileIndex = options.lookupPathIndex = options.overlayIndex = options.overlayTableIndex = -1;
            options.flag1 = options.flag2 = options.flag3 = 0;
            c2pstrcpy(pascalName, document->name);
            Files_MakeFileSpecFromPath(0, 0, pascalName, &fileSpec);
            result = add_project_entry(request, &fileSpec, 1, &options, &status);
            return result;
        }
        resourceSize = Memory_GetHandleSize(resourceStorage);
        if (CLIO_WriteStorageToStdout(resourceStorage, resourceSize, document->name != 0) == 0) {
            return 2;
        }
        return 0;
    }
    CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 1699, "Cannot deal with unexpected document");
    return 4;
}

unsigned int __stdcall allocate_memory(unsigned int unused0, unsigned int value, unsigned int unused2,
                                       unsigned int *result)
{
    unsigned int status;
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBAllocateMemory");
    *result = (unsigned int)xmalloc(0U, value);
    if (*result == 0U)
        status = 2U;
    else
        status = 0U;
    return (unsigned short)status;
}

unsigned int __stdcall free_callback_argument(unsigned int unused1, unsigned int callbackArgument, unsigned int unused2)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBFreeMemory");
    if (callbackArgument) {
        free((char *)callbackArgument);
        return 0;
    }
    return 2;
}

unsigned int __stdcall fn_004252a0(unsigned int unused, unsigned int size, unsigned int reserved, void *result)
{
    unsigned int handle;
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBAllocMemHandle");
    handle = Memory_NewHandle(size);
    if (!handle)
        return 7U;
    CLDropinCallbacks_V10_StoreValue(unused, handle, result);
    return 0U;
}

unsigned int __stdcall fn_004252f0(unsigned int callback, int callbackIndex)
{
    StorageHandle *callbackData;

    if (4 < DAT_00541b28) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBFreeMemHandle");
    }
    CLDropinCallbacks_V10_SetStorageHandle(callback, callbackIndex, &callbackData);
    Memory_FreeHandle(callbackData);
    return 0;
}

int __stdcall fn_00425340(unsigned int argument1, unsigned int argument2, unsigned int *result)
{
    StorageHandle *value;
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetMemHandleSize");
    CLDropinCallbacks_V10_SetStorageHandle(argument1, argument2, &value);
    *result = Memory_GetHandleSize(value);
    return 0;
}

unsigned int __stdcall resize_mem_handle(unsigned int callback, int argument, unsigned int size)
{
    unsigned short status;
    unsigned short result;
    struct StorageHandle *handle;

    if (4 < DAT_00541b28) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBResizeMemHandle");
    }
    CLDropinCallbacks_V10_SetStorageHandle(callback, argument, &handle);
    Memory_ResizeStorageHandle(handle, size);
    status = Memory_GetError();
    if (status == 0) {
        result = 0;
    } else {
        result = 7;
    }
    return result;
}

unsigned int __stdcall get_storage_handle_data(unsigned int unused0, struct StorageHandle *args, unsigned int unused2,
                                               char **destination)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBLockMemHandle");
    fn_00413a00(args);
    *destination = args->data;
    return 0;
}

unsigned int __stdcall fn_00425430(unsigned int unused, unsigned int handle)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBUnlockMemHandle");
    fn_00413a50((StorageHandle *)handle);
    return 0;
}

unsigned int __stdcall copy_command_line_target(unsigned int unused, unsigned int value, unsigned int kind)
{
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBGetTargetName");
    strncpy((char *)value, "command-line target", (short)kind);
    return 0;
}

unsigned int __stdcall log_callback_string(unsigned int argument)
{
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBPreDialog");
    return 0;
}

unsigned int __stdcall log_callback_above_threshold(unsigned int argument)
{
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBPostDialog");
    return 0;
}

unsigned int __stdcall fn_004254e0(unsigned int action, unsigned int fileReference)
{
    if (DAT_00541b28 > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBPreFileAction");
    }
    CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 1907, "UCBPreFileAction not implemented");
    return 2U;
}

unsigned int __stdcall fn_00425520(unsigned int file, unsigned int action)
{
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBPostFileAction");
    CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 1921, "UCBPostFileAction not implemented");
    return 2U;
}

int __stdcall cache_access_path_list(CWPluginPrivateContext *request)
{
    struct CallbackCache *cache;
    CallbackPathEntry *entry0;
    struct AccessPathEntry *source0;
    char *dirty;
    struct AccessPathEntry *source1;
    int index;
    CallbackPathEntry *entry1;
    short version;
    char buffer0[324];
    char buffer1[324];
    version = DAT_00541b28;
    dirty = (char *)request->shellContext;
    if (version > 3) {
        CLIO_FormatAndDispatchText("Callback: %s\n", "UCBCacheAccessPathList");
    }
    if ((cache = (struct CallbackCache *)request->callbackCache) == 0) {
        cache = (request->callbackCache = (struct CallbackCache *)calloc(18, 1));
    }
    if (dirty[4] != 0 || cache->entries0 == 0) {
        cache->count0 = CLAccessPaths_GetCount(&default_target->systemPaths);
        cache->entries0 = (struct CallbackPathEntry *)xrealloc("access paths", cache->entries0, cache->count0 * 80);
        for (index = 0; index < cache->count0; index = index + 1) {
            entry0 = cache->entries0 + index;
            source0 = CLAccessPaths_GetEntry(&default_target->systemPaths, index);
            if (source0 == 0) {
                CLIO_ReportAssertionFailure("path", "CLDropinCallbacks_V10.cpp", 1954);
            }
            CLProj_MakeOSSpecFromPath(source0->path, 0, 0, (struct OSSpec *)buffer0);
            MacSpecs_MakeCWFileSpecFromString(buffer0, &entry0->file);
            CLDropinCallbacks_V10_00425560_run1(entry0, source0);
        }
        dirty[4] = 0;
    }
    if (dirty[5] != 0 || cache->entries1 == 0) {
        cache->count1 = CLAccessPaths_GetCount(&default_target->userPaths);
        cache->entries1 = (struct CallbackPathEntry *)xrealloc("access paths", cache->entries1, cache->count1 * 80);
        for (index = 0; index < cache->count1; index = index + 1) {
            entry1 = cache->entries1 + index;
            source1 = CLAccessPaths_GetEntry(&default_target->userPaths, index);
            if (source1 == 0) {
                CLIO_ReportAssertionFailure("path", "CLDropinCallbacks_V10.cpp", 1988);
            }
            CLProj_MakeOSSpecFromPath(source1->path, 0, 0, (struct OSSpec *)buffer1);
            MacSpecs_MakeCWFileSpecFromString(buffer1, &entry1->file);
            CLDropinCallbacks_V10_00425560_run1(entry1, source1);
        }
        dirty[5] = 0;
    }
    return 0;
}

unsigned int __stdcall CLDropinCallbacks_V10_StoreValue(unsigned int unused, unsigned int value, void *resultAddress)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBSecretAttachHandle");
    *(unsigned int *)resultAddress = value;
    return 0;
}

unsigned int __stdcall CLDropinCallbacks_V10_SetStorageHandle(unsigned int unused, unsigned int value,
                                                              void *resultAddress)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBSecretDetachHandle");
    if (!value || !resultAddress) {
        *(StorageHandle **)resultAddress = 0;
        return 3;
    }
    *(StorageHandle **)resultAddress = (StorageHandle *)value;
    return 0;
}

unsigned int __stdcall copy_value_to_result(unsigned int unused, unsigned int value, unsigned int *result)
{
    if (DAT_00541b28 > 4)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBSecretPeekHandle");

    if (!value || !result) {
        *result = 0;
        return 3;
    }

    *result = value;
    return 0;
}

unsigned int __stdcall request_license(unsigned int unused0, unsigned int request, unsigned int options,
                                       unsigned int flags, unsigned int unused4, unsigned int *licenseResult)
{
    char message[4096];
    int license;
    unsigned int cookieKind;

    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBCheckoutLicense");
    cookieKind = flags & 1U;
    if (cookieKind == 0U && licenseResult == 0)
        return 3U;
    if (flags != 0U && (flags & 1U) == 0U)
        CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 2104, "Unknown license flags");
    if (licenseResult != 0)
        *licenseResult = 0U;
    license = CLLicenses_RequestLicense(request, options, flags & 1U, message);
    if (license != 0) {
        if (licenseResult != 0)
            *licenseResult = license;
        return 0U;
    }
    CLErrors_EmitDiagnostic(87U, message);
    return 516U;
}

unsigned int __stdcall forward_nonzero_value(unsigned int unused, unsigned int value)
{
    if (DAT_00541b28 > 3)
        CLIO_FormatAndDispatchText("Callback: %s\n", (unsigned char *)"UCBCheckinLicense");
    if (value != 0)
        CLLicenses_DeleteLicense(value);
    return 0;
}

unsigned int __stdcall fn_00425a00(unsigned int clientContext, unsigned int basePath, unsigned int relativePath,
                                   unsigned int resolvedPath)
{
    CLErrors_ReportInternalError("CLDropinCallbacks_V10.cpp", 2196, "UCBResolveRelativePath not implemented");
    return 2U;
}
}

static char lbl_0054d038[] = "UCBMacOSErrToCWResult";

/* The callbacks a plug-in calls back into the driver through. */
static void *data_0054d050[43] = {
    (void *)get_file_info,
    (void *)CLDropinCallbacks_V10_FindAndLoadFile,
    (void *)CLDropinCallbacks_V10_GetFileText,
    (void *)CLDropinCallbacks_V10_FreeMemory,
    (void *)lookup_callback_record,
    (void *)get_overlay_group_info,
    (void *)lookup_overlay_allocation,
    (void *)call_overlays_and_translate_status,
    (void *)report_message,
    (void *)emit_alert_messages,
    (void *)report_message_detail,
    (void *)fn_00424540,
    (void *)copy_named_destination_to_temporary,
    (void *)report_store_plugin_data_not_implemented,
    (void *)fn_00424660,
    (void *)set_mod_date,
    (void *)add_project_entry,
    (void *)create_new_text_document,
    (void *)allocate_memory,
    (void *)free_callback_argument,
    (void *)fn_004252a0,
    (void *)fn_004252f0,
    (void *)fn_00425340,
    (void *)resize_mem_handle,
    (void *)get_storage_handle_data,
    (void *)fn_00425430,
    (void *)CLDropinCallbacks_V10_StoreValue,
    (void *)CLDropinCallbacks_V10_SetStorageHandle,
    (void *)copy_value_to_result,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (void *)copy_command_line_target,
    (void *)cache_access_path_list,
    (void *)log_callback_string,
    (void *)log_callback_above_threshold,
    (void *)fn_004254e0,
    (void *)fn_00425520,
    (void *)request_license,
    (void *)forward_nonzero_value,
    (void *)fn_00425a00,
};

static unsigned char data_0054d0fc[12] = {3, 0, 3, 0, 0, 0, 0, 0, 10, 0, 0, 0};

PluginA::PluginA(UInt32 code, int size)
{
    if (size <= 0)
        size = 0x108;
    memset(this, 0, size);
    sig = 0x43574945;
    this->code = code;
    fb0 = data_0054d0fc;
    f104 = data_0054d050;
}
