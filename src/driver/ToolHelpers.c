#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/ToolHelpers.h"
#include "driver/ParserErrors.h"
#include "driver/ParserHelpers.h"
#include "compiler/win32.h"
#include "compiler/CPrep.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/ParserFace.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/ParserHelpers-cc.h"
#include "driver/Projects.h"
#include "driver/Targets.h"
#include "driver/Utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "driver/CLDropinCallbacks_V10.h"

static inline short *driverStatus(void)
{
    return &pCmdLine.state;
}

static inline int *pendingCount(void)
{
    return &data_00587e18;
}

static inline int *activeCount(void)
{
    return &data_00587e14;
}

static inline short *driverFlags(void)
{
    return &pCmdLine.toDisk;
}

static inline short *driverOptions(void)
{
    return &pCmdLine.stages;
}

static inline int shouldReportError(unsigned char reportError)
{
    return reportError != 0;
}

static Boolean data_0054a0f8 = 0;

unsigned int fn_0040d012(unsigned int reportError)
{
    if (targets_value_null_or_zero != 0) {
        *driverStatus() = 1;
        ToolHelpers_cc_PrintVersion(1);
        return 1;
    }
    if (tool_checks_passed != 0) {
        if (*activeCount() == 0) {
            if (data_00587e1c == 0) {
                if (shouldReportError(reportError)) {
                    fn_0040ecb1(0x46);
                    return 0;
                }
            } else {
                if (shouldReportError(reportError))
                    *driverStatus() = 1;
                return 1;
            }
        } else if (*pendingCount() > 0) {
            fn_0040ecb1(0x45);
            return 0;
        }
    }
    if (*driverStatus() == 0 || (*activeCount() > 0 && *driverStatus() == 1))
        *driverStatus() = 3;
    if (data_0054a0f8 == 0) {
        *driverOptions() = 2;
        if (*driverStatus() == 2)
            *driverFlags() |= 2;
    }
    if (*driverStatus() == 3 && (*driverOptions() & 2) == 0)
        *driverStatus() = 2;
    return 1;
}

int fn_0040d0eb(void)
{
    if (pCmdLine.verbose) {
        pCmdLine.verbose++;
    } else {
        pCmdLine.verbose = 2;
    }
    fn_0040ba99(pluginPrivateContext);
    return 1;
}

int parse_stage_settings(int unused1, unsigned char *opt, int unused2, int flags)
{
    unsigned char *cursor = opt;
    Boolean enabled;
    Boolean negated;
    if (flags & 8)
        negated = 1;
    else
        negated = 0;
    enabled = negated;
    enabled ^= 1;
    while (*cursor) {
        if (*cursor == '+')
            enabled = !negated;
        else if (*cursor == '-')
            enabled = negated;
        else if (*cursor == '|') {
            enabled = negated;
            enabled ^= 1;
        } else {
            unsigned short stage = (cursor[0] << 8) | cursor[1];
            data_0054a0f8 = 1;
            switch (stage) {
                case 'Cg':
                    if (enabled)
                        pCmdLine.stages |= 2;
                    else
                        pCmdLine.stages &= ~2;
                    data_0054a0b8 = 1;
                    break;
                case 'Ds':
                    if (enabled)
                        pCmdLine.stages |= 4;
                    else
                        pCmdLine.stages &= ~4;
                    data_0054a0b8 = 3;
                    break;
                case 'Pp':
                    if (enabled)
                        pCmdLine.stages |= 1;
                    else
                        pCmdLine.stages &= ~1;
                    data_0054a0b8 = 2;
                    break;
                case 'Dp':
                    if (enabled)
                        pCmdLine.stages |= 8;
                    else
                        pCmdLine.stages &= ~8;
                    break;
                default:
                    Targets_ForwardVarArgsAndLongjmp("Bad stage settings in %s (%c%c)\n", opt, cursor[0], cursor[1]);
                    break;
            }
            cursor++;
        }
        cursor++;
    }
    return 1;
}

static char lbl_0054a120[] = "wt";
