#include "compiler/common.h"
#include "driver/ParserHelpers-cc.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/Option.h"
#include "driver/Parameter.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/ParserHelpers.h"
#include "driver/Projects.h"
#include "driver/Targets.h"
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "driver/CLDropinCallbacks_V10.h"
#define OPTION_ASSERT(cond, line) ((cond) ? (void)0 : CLIO_ReportAssertionFailure(#cond, "ParserHelpers-cc.c", line))
#define PR_UNSET 0
int fn_0040d283(int unused, char *first, char *second)
{
    ParserHelpers_AppendText(&directive_storage, first);
    if (second != NULL) {
        ParserHelpers_AppendText(&directive_storage, second);
    }
    return 1;
}

int append_define_directive(char *name, char *value)
{
    char buf[0x400];
    if (driverTool[1] == 0x632b2b20 || driverTool[1] == 0x41736d20)
        sprintf(buf, "#define %s %s\n", name, value ? value : "1");
    else if (driverTool[1] == 0x70617363)
        sprintf(buf, "{$definec %s %s}\n", name, value ? value : "1");
    else {
        sprintf(buf, "Option '-D|d' is not supported with this plugin");
        fn_0040ecb1(0x1c, buf);
        return 0;
    }
    ParserHelpers_AppendText(&directive_storage, buf);
    return 1;
}

int append_undef_directive(char *option, int unused, char *symbol)
{
    char buf[300];
    if (driverTool[1] == 0x632b2b20 || driverTool[1] == 0x41736d20)
        sprintf(buf, "#undef %s\n", symbol);
    else if (driverTool[1] == 0x70617363)
        sprintf(buf, "{$undefc %s}\n", symbol);
    else {
        sprintf(buf, "Option -%s is not supported with this plugin", option);
        fn_0040ecb1(0x1c, buf);
        return 0;
    }
    ParserHelpers_AppendText(&directive_storage, buf);
    return 1;
}

int append_include_directive(char *option, void *handle, char *filename)
{
    char buf[300];
    struct StorageHandle **storage;
    if (!handle)
        storage = &directive_storage;
    else
        storage = (struct StorageHandle **)handle;
    if (*filename) {
        if (driverTool[1] == 0x632b2b20 || driverTool[1] == 0x41736d20)
            sprintf(buf, "#include \"%s\"\n", filename);
        else if (driverTool[1] == 0x70617363)
            sprintf(buf, "{$I+}\n{$I %s}\n{$I-}\n", filename);
        else {
            sprintf(buf, "Option -%s is not supported with this plugin", option);
            fn_0040ecb1(0x1c, buf);
            return 0;
        }
        ParserHelpers_AppendText(storage, buf);
    }
    return 1;
}

static inline int PragmaHasSetting(const Pragma *pragma, char setting)
{
    const char *value = pragma->value;
    return *value == setting;
}

int ParserHelpers_cc_EmitPragmas(Pragma *pragmas)
{
    char buf[300];
    for (; pragmas->pragma; pragmas++) {
        if (pragmas->flags == 0 || pragmas->flags == 1) {
            const char *value = NULL;
            Boolean reverse = pragmas->flags == 1;
            char on = !reverse ? (char)1 : (char)2;
            char off = !reverse ? (char)2 : (char)1;
            if (PragmaHasSetting(pragmas, on))
                value = "on";
            else if (PragmaHasSetting(pragmas, off))
                value = "off";
            else if (PragmaHasSetting(pragmas, 3))
                value = "auto";
            else if (PragmaHasSetting(pragmas, 4))
                value = "reset";
            else
                OPTION_ASSERT(*((char *)pragmas->value) == PR_UNSET, 181);
            if (value) {
                sprintf(buf, "#pragma %s %s\n", pragmas->pragma, value);
                ParserHelpers_AppendText(&directive_storage, buf);
            }
        } else {
            OPTION_ASSERT(!"Can't handle pragma", 190);
        }
    }
    return 1;
}
