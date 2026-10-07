#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/ToolHelpers-cc.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLIO.h"
#include "driver/CLMain.h"
#include "driver/CLProj.h"
#include "driver/CLTarg.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/Files.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/StringUtils.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers.h"
#include <stdio.h>
#include <setjmp.h>
static void emit_header(void *p)
{
    HPrintF(p, "C language warning options:\n");
}

void ToolHelpers_cc_CallValuePairCallback(char *key, struct StorageHandle *value)
{
    long result;

    if (value != NULL) {
        if (key != NULL) {
            Targets_FormatAndForwardMessage(0x47, key);
        }
        fn_0041bcb0(pluginPrivateContext, value, &result);
        CWParserPluginsPrivate_CallValuePairCallback(pluginPrivateContext, key, result);
    }
}

char *format_version(unsigned int version, char *buf)
{
    char *p = buf;
    unsigned char minor;
    unsigned char build;
    p += sprintf(p, "%u.%u", (version >> 24) & 0xff, (version >> 16) & 0xff);
    minor = (version >> 8) & 0xff;
    build = version & 0xff;
    if (minor)
        p += sprintf(p, ".%u", minor);
    if (build)
        p += sprintf(p, " build %u", build);
    return buf;
}

static inline char *initZero(void)
{
    char *z = NULL;
    return z;
}

void ToolHelpers_cc_PrintVersion(char includeValue)
{
    char *compilerVersion;
    char *parserVersion;
    struct StorageHandle *output;
    int index;
    char *matchingVersion;
    char *alternateVersion;
    char parserBuffer[16], matchingBuffer[16], alternateBuffer[16], compilerBuffer[16];
    unsigned int parserValue1, parserValue2;
    compilerVersion = NULL;
    matchingVersion = NULL;
    alternateVersion = NULL;
    parserVersion = initZero();
    if (data_00587e22 == 0) {
        output = (struct StorageHandle *)Memory_NewHandle(0);
        if (output == NULL) {
            Targets_ForwardVarArgsAndLongjmp(data_0054a870);
            longjmp(plugin_request_jmp_buf, 7);
        }
        for (index = 0; index < num_panels; index++) {
            if (data_00587cf0[index].type == 1668047986) {
                compilerVersion = format_version(data_00587cf0[index].version, compilerBuffer);
            } else if (data_00587cf0[index].type == 1348563571) {
                parserVersion = format_version(data_00587cf0[index].version, parserBuffer);
            } else if (driverTool[0] == data_00587cf0[index].type) {
                if (matchingVersion == NULL && driverTool[1] == data_00587cf0[index].creator) {
                    matchingVersion = format_version(data_00587cf0[index].version, matchingBuffer);
                } else {
                    alternateVersion = format_version(data_00587cf0[index].version, alternateBuffer);
                }
            }
        }
        CWParserPluginsPrivate_GetParserValues(pluginPrivateContext, &parserValue1, &parserValue2);
        HPrintF(output, data_0054a880);
        if (matchingVersion == NULL) {
            if (alternateVersion == NULL)
                alternateVersion = data_0054a884;
            matchingVersion = alternateVersion;
        }
        {
            DriverTool *tool;
            HPrintF(output,
                    "%s.\nCopyright (c)%s Metrowerks, Inc.\nAll rights reserved.\nVersion %s\nRuntime Built: %s %s\n",
                    tool->toolInfo, (tool = (DriverTool *)driverTool)->copyright, matchingVersion, parserValue1,
                    parserValue2);
        }
        HPrintF(output, data_0054a880);
        if (includeValue != 0) {
            HPrintF(output, please_enter_format, CLProj_GetFileName(*cmdline_environment->argv), *data_00587eec);
        }
        ToolHelpers_cc_CallValuePairCallback(NULL, output);
        Memory_FreeHandle(output);
        data_00587e22 = 1;
    }
}

int ToolHelpers_cc_GetNumFiles(void)
{
    long count;

    CWPluginsPrivate_GetNumFiles(pluginPrivateContext, &count);
    return count;
}

void ToolHelpers_cc_SetFileOutputName(int a, short b, char *s)
{
    int r;
    if (s && *s) {
        if ((r = CWParserPluginsPrivate_CallParserTextCallback(pluginPrivateContext, a, b ? b : 1, s)) != 0) {
            DAT_00543380 = "CWParserSetFileOutputName";
            longjmp(plugin_request_jmp_buf, r);
        }
    }
}

SInt32 ToolHelpers_cc_AddProjectEntry(OSSpec *path, SInt16 mode, char *name, Boolean flag, SInt32 fileId)
{
    SInt32 status;
    SInt32 result;
    CWFileSpec fileSpec[1];
    FileOpenOptions options;
    long *link;
    int error;

    error = MacSpecs_MakeCWFileSpecFromString((char *)path, fileSpec);
    if (error != 0) {
        Targets_ReportOperatingSystemError(0x2c, error, CLProj_MakeRelativePath(path, NULL, data_005880e0, 0x104));
        result = 0;
    } else {
        if (fileId == -2)
            options.fileIndex = 0;
        else if (fileId == -1) {
            link = &options.fileIndex;
            CWPluginsPrivate_GetNumFiles(pluginPrivateContext, link);
        } else if (fileId == 0)
            options.fileIndex = -1;
        else
            options.fileIndex = fileId;

        options.lookupPathIndex = data_00587e04;
        options.overlayIndex = data_00587e08;
        options.overlayTableIndex = data_00587e0c;
        options.auxiliaryValue = 0;
        options.flag1 = data_00587e28;
        options.flag2 = data_00587e27;
        options.flag3 = data_00587e26;

        status = CWPluginsPrivate_OpenFile(pluginPrivateContext, fileSpec, !flag, &options, &fileId);
        if (status != 0) {
            DAT_00543380 = "CWAddProjectEntry";
            longjmp(plugin_request_jmp_buf, status);
        }

        data_00587e28 = data_00587e27 = data_00587e26 = 0;

        ToolHelpers_cc_SetFileOutputName(fileId, mode, name);
        result = 1;
    }
    return result;
}

int ToolHelpers_cc_AddAccessPath(char *spec, char use_first, int value, unsigned char option)
{
    int status;
    int result;
    int selected_value;
    unsigned int query_status;
    char option_b;
    unsigned int update_status;
    FileOperationInfo info;
    char path_buffer[324];
    struct QueryValues values;
    CLProj_MakeOSSpecFromPath(spec, NULL, 0, (OSSpec *)path_buffer);
    status = MacSpecs_MakeCWFileSpecFromString(path_buffer, &info.file.fileReference);
    if (status != 0) {
        Targets_ReportOperatingSystemError(45, status, fn_00412340(path_buffer, data_005880e0, 260));
        result = 0;
    } else {
        if (value == -2) {
            selected_value = 0;
            info.file.selection.value = selected_value;
        } else if ((unsigned int)(value + 1) <= 1) {
            info.file.selection.value = -1;
        } else if (value == -1) {
            query_status =
                ((unsigned int(__stdcall *)(CWPluginPrivateContext *, struct QueryValues *))get_opcode_descriptor)(
                    pluginPrivateContext, &values);
            if (query_status != 0) {
                DAT_00543380 = "CWGetAccessPathListInfo";
                longjmp(plugin_request_jmp_buf, query_status);
            }
            if (use_first != 0) {
                selected_value = values.firstValue;
            } else {
                selected_value = values.secondValue;
            }
            info.file.selection.value = selected_value;
        } else {
            info.file.selection.value = value;
        }
        if (use_first != 0) {
            option_b = 0;
        } else {
            option_b = 1;
        }
        info.useSecondValue = option_b;
        info.option_a = option;
        update_status = CWParserPluginsPrivate_CallFileOperationCallback(pluginPrivateContext, &info);
        if (update_status != 0) {
            DAT_00543380 = "CWParserAddAccessPath";
            longjmp(plugin_request_jmp_buf, update_status);
        }
        result = 1;
    }
    return result;
}

void ToolHelpers_cc_PassVirtualFileValuePair(char *fileData, struct StorageHandle **virtualFile)
{
    long fileInfo;
    int error;

    if (*virtualFile != NULL) {
        fn_0041bcb0(pluginPrivateContext, *virtualFile, &fileInfo);
        error = CWParserPluginsPrivate_PassValuePair(pluginPrivateContext, fileData, fileInfo);
        if (error != 0) {
            DAT_00543380 = "CWParserCreateVirtualFile";
            longjmp(plugin_request_jmp_buf, error);
        }
        Memory_FreeHandle(*virtualFile);
        *virtualFile = NULL;
    }
}

void ToolHelpers_cc_CallFileInfoForDirectory(OSSpec *input)
{
    CWFileSpec body;
    union RecoveryPathFrame frame;
    int result;
    frame.copy = *(OSPathBuffer *)input->directory.path;
    MacSpecs_MakeCWFileSpecFromString((char *)&frame.copy, &body);
    result = CWParserPluginsPrivate_CallFileInfo(pluginPrivateContext, &body);
    if (result != 0U) {
        DAT_00543380 = "CWParserSetOutputFileDirectory";
        longjmp(plugin_request_jmp_buf, result);
    }
}
