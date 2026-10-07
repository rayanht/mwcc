#define CERROR_FILE "unknown.c"
#pragma scheduling off
#include "compiler/common.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/Memory.h"
#include "driver/Option.h"
#include "driver/ParserHelpers-cc.h"
#include "driver/ParserHelpers.h"
#include "driver/Projects.h"
#include "driver/StringUtils.h"
#include "driver/TargetWarningHelpers-ppc-cc.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers-cc.h"
#include "driver/ToolHelpers.h"
unsigned int fn_00405670(void)
{
    data_00537aa2 = 1U;
    data_0058851d = 0U;
    output_path = 0U;
    directive_storage = 0U;
    data_00537d38 = 1U;
    return 1U;
}

int fn_004056a0(void)
{
    char *includes;
    char *includesEnd;

    if (!DAT_00537762) {
        DAT_00537762 = 3;
    }
    if ((0 < data_00587e10) && (data_00537d38 != '\0')) {
        includes = ParserHelpers_GetFirstEnvironmentVariable("MWCEABIPPCIncludes", 1, &includesEnd);
        if ((includes != NULL) && (ParserHelpers_ParsePathList(includes, ';', ':', 1, includesEnd, 1, -1, 0) == 0)) {
            return 0;
        }
    }
    return 1;
}

int fn_00405710(void)
{
    if (ParserHelpers_cc_EmitPragmas((Pragma *)&data_0054a388) == 0 ||
        ParserHelpers_cc_EmitPragmas(data_0054a690) == 0) {
        return 0;
    }
    if (directive_storage != NULL) {
        if (data_0053776c != 0) {
            ToolHelpers_cc_CallValuePairCallback(data_00540b68, directive_storage);
        }
        ToolHelpers_cc_PassVirtualFileValuePair(data_00540b68, &directive_storage);
        c2pstrcpy(&data_00540add, data_00540b68);
    } else {
        data_00540add = 0;
    }
    fn_0040d822();
    if (fn_0040d012(1) == 0) {
        return 0;
    }
    if (data_00537a67 != 0 && data_00537b24 == 0) {
        Targets_DispatchVariadicMessage(0x1c,
                                        "'-use_lmw_stmw on' or '-opt functions' only applies to big-endian machines");
    }
    if (output_path != 0) {
        fn_0040fbe1(data_005876fc, 1, NULL);
        fn_0040fbe1(data_005876fc, 2, "-o");
        fn_0040fbe1(data_005876fc, 1, NULL);
        fn_0040fbe1(data_005876fc, 2, &output_path);
        fn_0040fbe1(data_005876fc, 1, NULL);
    }
    data_00537a76 = data_0053776b;
    data_00537a77 = data_00537d40;
    return 1;
}

#pragma scheduling reset
int fn_00405840(void)
{
    return Targets_SetTool(&data_00540bf8);
}
