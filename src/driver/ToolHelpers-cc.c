#include "compiler/common.h"
#include "driver/ToolHelpers-cc.h"
#include "driver/CLIO.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/MacSpecs.h"
#include "driver/Option.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/ParserHelpers.h"
#include "driver/Projects.h"
#include <string.h>
#include "driver/CLDropinCallbacks_V10.h"
int set_output_path(char *name, int unused, char *path)
{
    OSSpec spec1;
    Boolean isdir1;
    OSSpec spec2;
    Boolean isdir2;
    CWFileSpec info;
    long count;
    ExportedRecord tinfo;
    int err;
    if (!path)
        path = name;
    if (pCmdLine.state == 3 || (pCmdLine.state == 0 && pTool->tool == 'Link')) {
        if (data_0058851d) {
            fn_0040ecb1(0x29, path);
            return 0;
        }
        data_0058851d = 1U;
        if (pTool->tool == 'Comp') {
            strncpy(&output_path, path, 0x100);
            return 1;
        }
        if ((err = OS_MakeSpec(path, &spec2, &isdir2)) != 0) {
            Targets_ReportOperatingSystemError(0x40, err, path);
            return 0;
        }
        if (isdir2)
            OS_NameSpecToString(&spec2.name, &output_path, 0x104);
        ToolHelpers_cc_CallFileInfoForDirectory(&spec2);
        return 1;
    }
    if ((err = OS_MakeSpec(path, &spec1, &isdir1)) != 0) {
        Targets_ReportOperatingSystemError(0x40, err, path);
        return 0;
    }
    if (!err && !isdir1) {
        if (output_path_set) {
            fn_0040ecb1(0x3b, path);
            return 0;
        }
        output_path_set = 1;
        OS_OSSpec_To_FSSpec(&spec1, &info);
        if ((err = CWParserPluginsPrivate_CallFileInfo(pluginPrivateContext, &info)) != 0) {
            data_00543380 = "CWParserSetOutputFileDirectory";
            longjmp(plugin_request_jmp_buf, err);
        }
        return 1;
    } else {
        if (data_00587d04[0]) {
            fn_0040ecb1(0x29, path);
            return 0;
        }
        strncpy(data_00587d04, path, 0x100);
        if (pCmdLine.stages == 8)
            return 1;
        if (data_00588530 > 1)
            return 1;
        CWPluginsPrivate_GetNumFiles(pluginPrivateContext, &count);
        while (count-- > 0) {
            if (!CWPluginsPrivate_InvokeExportedRecordCallback(pluginPrivateContext, count, 0, &tinfo) &&
                tinfo.fileType == 0x54455854) {
                data_00588530 = 1;
                break;
            }
        }
        if (!data_00588530) {
            data_00588530 = 2;
            return 1;
        }
        ToolHelpers_cc_SetFileOutputName(count, data_0054a0b8, data_00587d04);
        data_00587d04[0] = 0;
        return 1;
    }
}

int log_linker_option(const char *option)
{
    Targets_ForwardVarArgsAndLongjmp("Calling linker option '%s'\n", option);
    return 0;
}

static char lbl_0054a2c4[] = "";
static char lbl_0054a2c8[] = "Calling linker settings option '%s'='%s'\n";

void fn_0040d822(void)
{
    if (data_00587d04[0]) {
        int n = ToolHelpers_cc_GetNumFiles();
        if (pCmdLine.stages == 8)
            strcpy(pCmdLineCompiler.outMakefile, data_00587d04);
        else if (data_00588530 == 2) {
            if (data_00587e10 > 0 || data_00587e14 > 0)
                fn_0040ecb1(0x29, data_00587d04);
            else
                fn_0040ecb1(0x2a, data_00587d04);
        } else
            ToolHelpers_cc_SetFileOutputName(n - 1, data_0054a0b8, data_00587d04);
        data_00587d04[0] = 0;
    }
    if (output_path_set) {
        pCmdLineCompiler.relPathInOutputDir = 0;
    }
}
