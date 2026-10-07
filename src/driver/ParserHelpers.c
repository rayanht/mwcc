#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/ParserHelpers.h"
#include "compiler/win32.h"
#include "compiler/CPrep.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/Generic.h"
#include "driver/MacSpecs.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/ParserHelpers-cc.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers-cc.h"
#include "driver/ToolHelpers.h"
#include "driver/Utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "driver/CLDropinCallbacks_V10.h"

short data_0054a0b8 = 0;

#define va_start(ap, parm) ap = (char *)&parm + ((((long)((char *)&parm + 2) - (long)&parm) + 3) / 4 * 4)
int get_spec_from_signature_callback(const char *path, OSSpec *spec)
{
    DropinFileCallback l;
    l.enableDependencyLookup = 1;
    l.searchOption = 0;
    l.suppressFileReferenceLookup = 1;
    l.fileKey = -1;
    if (CWPluginsPrivate_CallSignatureCallback(pluginPrivateContext, path, &l) == 0) {
        MacSpecs_MakeOSSpec(&l.output, (char *)spec);
        return 1;
    }
    return 0;
}

char *ParserHelpers_GetFirstEnvironmentVariable(char *list, char verbose, char **out)
{
    char *p = list;
    char *last = list;
    char *r;
    while (*p) {
        if ((r = getenv(p)) != NULL) {
            if (out)
                *out = p;
            return r;
        }
        last = p;
        p += strlen(p) + 1;
    }
    if (verbose)
        Targets_ReportMessage(0x34, last);
    *out = NULL;
    return NULL;
}

Boolean match_extension_pattern(char *pattern, char *name)
{
    char *base = CLProj_GetFileName(name);
    char *ext = strrchr(base, '.');
    char *p;
    if (!ext)
        return 0;
    if (!pattern)
        return 1;
    p = ext;
    while (*pattern) {
        if (*pattern == '|' && *p == 0)
            return 1;
        if (/* (stand-in: the original caller passes a char through its own prototype) */ ((int (*)(char))to_lowercase)(
                *pattern) ==
            /* (stand-in: the original caller passes a char through its own prototype) */ ((int (*)(char))to_lowercase)(
                *p)) {
            pattern++;
            p++;
        } else {
            while (*pattern && *pattern != '|')
                pattern++;
            if (*pattern)
                pattern++;
            p = ext;
        }
    }
    return *pattern == 0 && *p == 0;
}

int fn_0040c9e7(char *name, int recursive, char *override)
{
    char spec[260];
    UInt32 err;
    char *path = override ? override : name;
    if (!path)
        return 0;
    if (!*path)
        return 1;
    if (strlen(path) >= 0x104) {
        fn_0040ecb1(0xd, path + strlen(path) - 32, 0x104);
        return 0;
    }
    err = OS_MakePathSpec(NULL, path, spec);
    if (err == 2 || err == 3) {
        Targets_ReportMessage(0x2d, path);
        return 1;
    }
    if (err) {
        Targets_ReportOperatingSystemError(0x2d, err, path);
        return 1;
    }
    if (!ToolHelpers_cc_AddAccessPath(spec, data_0058851b, 0, recursive ? 1 : 0))
        return 0;
    return 1;
}

void report_environment_variable_message(void (*report)(char *msg, va_list ap), char *where, int id, ...)
{
    char buf[0x400];
    va_list ap;
    if (where && *where)
        sprintf(buf, "In environment variable '%s':\n", where);
    else
        buf[0] = 0;
    va_start(ap, id);
    Targets_GetResourceCString(id, buf + strlen(buf));
    report(buf, ap);
}

int ParserHelpers_ParsePathList(char *path, char separator, char alternateSeparator, int mode, char *name, char option,
                                int flags, unsigned char toggle)
{
    int result;
    char *end;
    unsigned char plus;
    char component[260];
    char resolved[260];
    plus = *path == '+';
    if (plus) {
        ++path;
    }
    if ((separator == ':' || alternateSeparator == ':') && Utils_IsAlpha(*path) != 0 && path[1] == ':' &&
        OS_MakePathSpec(NULL, path, resolved) == 0) {
        result = ToolHelpers_cc_AddAccessPath(resolved, option, flags, plus ^ toggle);
    } else {
        if (strchr(path, separator) == NULL) {
            separator = alternateSeparator;
        }
        while (*path != 0) {
            end = component;
            for (; *path != 0 && *path != separator && end + 1 < component + 260; ++end) {
                *end = *path;
                ++path;
            }
            *end = 0;
            if (end + 1 >= component + 260) {
                report_environment_variable_message(Targets_FormatAndDispatchMessage, mode == 1 ? name : NULL, 9,
                                                    component, component + strlen(component) - 16, 260);
                return 0;
            }
            if (OS_MakePathSpec(NULL, component, resolved) != 0) {
                report_environment_variable_message(Targets_ReportFormattedMessage, mode == 1 ? name : NULL, 45,
                                                    component);
            } else {
                ToolHelpers_cc_AddAccessPath(resolved, option, flags, plus ^ toggle);
            }
            if (*path != 0) {
                ++path;
            }
            plus = *path == '+';
            if (plus) {
                ++path;
            }
        }
        result = 1;
    }
    return result;
}

static char match_path_buffer[0x104];

int fn_0040cd55(char *name, char *filter, char *override)
{
    OSSpec spec;
    Boolean isdir;
    char *path = override ? override : name;
    int err;
    OSSpec *match;
    if (!path)
        return 0;
    if (!*path)
        return 1;
    data_00587e14++;
    err = make_osspec_from_path(path, &spec, &isdir);
    if (!err)
        err = OS_Status(&spec);
    if (!err && !isdir) {
        fn_0040ecb1(0x2f, path);
        data_00587e18++;
        return 0;
    }
    if (err && err != 2 && !strpbrk(path, "*?")) {
        Targets_ReportOperatingSystemError(0x2c, err, path);
        data_00587e18++;
        return 0;
    }
    if (err && (match = CLProj_FindNextMatchingEntry(path))) {
        do {
            if (filter &&
                !match_extension_pattern(filter, MsDos_CopyStringToBuffer(match->name, match_path_buffer, 0x104)))
                Targets_ReportMessage(0x4c, CLProj_MakeRelativePath(match, NULL, match_path_buffer, 0x104), filter);
            if (!ToolHelpers_cc_AddProjectEntry(match, data_0054a0b8, data_00587d04, 1, -1)) {
                data_00587e18++;
                return 0;
            }
            data_00587d04[0] = 0;
        } while ((match = CLProj_FindNextMatchingEntry(NULL)));
        return 1;
    }
    if (err && data_00587e20) {
        if (get_spec_from_signature_callback(path, &spec))
            err = 0;
        else
            err = 2;
    }
    if (err) {
        fn_0040ecb1(0x2c, path);
        data_00587e18++;
        return 0;
    }
    if (filter && !match_extension_pattern(filter, path))
        Targets_ReportMessage(0x4c, path, filter);
    if (!ToolHelpers_cc_AddProjectEntry(&spec, data_0054a0b8, data_00587d04, 1, -1)) {
        data_00587e18++;
        return 0;
    }
    data_00587d04[0] = 0;
    return 1;
}

static char lbl_0054a0e0[] = ".lib|.a";
static char lbl_0054a0e8[] = "lib%s%*.*s";
static char lbl_0054a0f4[] = "";

void ParserHelpers_AppendText(struct StorageHandle **hp, char *text)
{
    unsigned int len;
    if (!*hp) {
        *hp = (struct StorageHandle *)Memory_NewHandle(strlen(text) + 1);
        if (*hp) {
            fn_00413a00(*hp);
            memcpy((*hp)->data, text, strlen(text) + 1);
            fn_00413a50(*hp);
        } else
            longjmp(plugin_request_jmp_buf, 7);
    } else {
        len = Memory_GetHandleSize(*hp) - 1;
        Memory_ResizeStorageHandle(*hp, len + strlen(text) + 1);
        if (!Memory_GetError()) {
            fn_00413a00(*hp);
            memcpy((*hp)->data + len, text, strlen(text) + 1);
            fn_00413a50(*hp);
        } else
            longjmp(plugin_request_jmp_buf, 7);
    }
}

int fn_0040cff6(void)
{
    ToolHelpers_cc_PrintVersion(0);
    return 1;
}

int fn_0040d002(void)
{
    fn_0040ba99(pluginPrivateContext);
    return 1;
}

static inline short *driverStatus(void)
{
    return &DAT_00537762;
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
    return &data_00537766;
}

static inline short *driverOptions(void)
{
    return &data_00537764;
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
    if (data_0053776c > 0) {
        data_0053776c++;
    } else {
        data_0053776c = 2;
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
                        data_00537764 |= 2;
                    else
                        data_00537764 &= ~2;
                    data_0054a0b8 = 1;
                    break;
                case 'Ds':
                    if (enabled)
                        data_00537764 |= 4;
                    else
                        data_00537764 &= ~4;
                    data_0054a0b8 = 3;
                    break;
                case 'Pp':
                    if (enabled)
                        data_00537764 |= 1;
                    else
                        data_00537764 &= ~1;
                    data_0054a0b8 = 2;
                    break;
                case 'Dp':
                    if (enabled)
                        data_00537764 |= 8;
                    else
                        data_00537764 &= ~8;
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
static int lbl_0054a124 = 0;

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
