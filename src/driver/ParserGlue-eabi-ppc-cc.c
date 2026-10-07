#define CERROR_FILE "unknown.c"
#pragma scheduling off
#include "compiler/common.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/Arguments.h"
#include "driver/Help.h"
#include "driver/Memory.h"
#include "driver/OptimizerHelpers.h"
#include "driver/Option.h"
#include "driver/Parameter.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"
#include "driver/ParserHelpers-cc.h"
#include "driver/ParserHelpers.h"
#include "driver/PrefPanels.h"
#include "driver/Projects.h"
#include "driver/StringUtils.h"
#include "driver/TargetWarningHelpers-ppc-cc.h"
#include "driver/Targets.h"
#include "driver/ToolHelpers-cc.h"
#include "driver/ToolHelpers.h"
#include "driver/WarningHelpers.h"

/* The C/C++ parser's preference panels, the options that set them, and the tool they describe. */

PCmdLine pCmdLine = {0x1002, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};

PCmdLineCompiler pCmdLineCompiler = {0x1001};

PCmdLineLinker pCmdLineLinker = {0x1000};

PBackEnd pBackEnd = {0xb, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0x14};

PLinker pLinker = {5, 0, 0,       0,      0, 0, 0,        0,         0,          0,    0, 0, 0,
                   0, 0, 0x10000, 0x3000, 0, 0, 0x3dfff0, 0x2800000, 0xffe00000, 0x1a, 1, 0, "__start"};

PDisassembler pDisassembler = {3, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1};

PProject pProject = {7, {0}, 0x400, 0x40, 1, 0, 8, 8, 1};

PCLTExtras pCLTExtras = {1};

char useDefaultIncludes = 1;
char unused_00537d3c = 1;
char useFullPaths = 0;

extern OptionList optlstCmdLine_msgstyle_mpw_conflicts;
extern OptionList optlstCmdLine_help_all_conflicts;
extern OptionList optlstCmdLine_help_opt_conflicts;
extern OptionList optlstCmdLine_help_tool_all_conflicts;
extern OptionList optlstCmdLineCompiler_cwd_proj_conflicts;
extern OptionList optlstCmdLineCompiler_noprecompile_conflicts;
extern OptionList optlstFrontEnd_ansi_off_conflicts;
extern OptionList optlstFrontEnd_char_signed_conflicts;
extern OptionList optlstFrontEnd_dialect_c_conflicts;
extern OptionList optlstFrontEnd_enum_min_conflicts;
extern OptionList optlstFrontEnd_msext_on_conflicts;
extern OptionList optlstBackEnd_func_align_4_conflicts;
extern OptionList optlstBackEnd_align_power_conflicts;
extern OptionList optlstProject_proc_401_conflicts;
extern OptionList optlstProject_fp_none_conflicts;
extern OptionList optlstProject_model_absolute_conflicts;
extern OptionList optlstProject_application_conflicts;
extern OptionList optlstProject_big_conflicts;
extern OptionList optlstLinker_sreceol_mac_conflicts;
extern OptionList optlstLinker_m_conflicts;
extern OptionList optlstDisassembler_fmt_x_conflicts;

MASK_T optlstCmdLine_help_usage_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x4000, 0x0, &data_00587ce0};

Option optlstCmdLine_help_usage = {"usage", 0x2101, (PARAM_T *)&optlstCmdLine_help_usage_param0,
                                   NULL,    NULL,   "show usage information"};

MASK_T optlstCmdLine_help_spaces_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x100, 0x0, &data_00587ce0};

Option optlstCmdLine_help_spaces = {"spaces", 0x102101, (PARAM_T *)&optlstCmdLine_help_spaces_param0,
                                    NULL,     NULL,     "insert blank lines between options in printout"};

MASK_T optlstCmdLine_help_all_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x6a, 0x0, &data_00587ce0};

Option optlstCmdLine_help_all = {"all",
                                 0x42101,
                                 (PARAM_T *)&optlstCmdLine_help_all_param0,
                                 NULL,
                                 &optlstCmdLine_help_all_conflicts,
                                 "show all standard options"};

MASK_T optlstCmdLine_help_normal_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x80, 0xe, &data_00587ce0};

Option optlstCmdLine_help_normal = {"normal",
                                    0x142101,
                                    (PARAM_T *)&optlstCmdLine_help_normal_param0,
                                    NULL,
                                    &optlstCmdLine_help_all_conflicts,
                                    "show only standard options"};

MASK_T optlstCmdLine_help_obsolete_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x4, 0x0, &data_00587ce0};

Option optlstCmdLine_help_obsolete = {"obsolete", 0x102101, (PARAM_T *)&optlstCmdLine_help_obsolete_param0,
                                      NULL,       NULL,     "show obsolete options"};

MASK_T optlstCmdLine_help_ignored_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x2, 0x0, &data_00587ce0};

Option optlstCmdLine_help_ignored = {"ignored", 0x102101, (PARAM_T *)&optlstCmdLine_help_ignored_param0,
                                     NULL,      NULL,     "show ignored options"};

MASK_T optlstCmdLine_help_deprecated_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x8, 0x0, &data_00587ce0};

Option optlstCmdLine_help_deprecated = {"deprecated", 0x102101, (PARAM_T *)&optlstCmdLine_help_deprecated_param0,
                                        NULL,         NULL,     "show deprecated options"};

MASK_T optlstCmdLine_help_meaningless_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x20, 0x0, &data_00587ce0};

Option optlstCmdLine_help_meaningless = {"meaningless", 0x102101, (PARAM_T *)&optlstCmdLine_help_meaningless_param0,
                                         NULL,          NULL,     "show options meaningless for this target"};

MASK_T optlstCmdLine_help_compatible_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x40, 0x0, &data_00587ce0};

Option optlstCmdLine_help_compatible = {"compatible", 0x102101, (PARAM_T *)&optlstCmdLine_help_compatible_param0,
                                        NULL,         NULL,     "show compatibility options"};

MASK_T optlstCmdLine_help_secret_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x10, 0x0, &data_00587ce0};

Option optlstCmdLine_help_secret = {"secret", 0x103101, (PARAM_T *)&optlstCmdLine_help_secret_param0,
                                    NULL,     NULL,     "show secret options"};

MASK_T optlstCmdLine_help_opt_param1 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x1, 0x0, &data_00587ce0};

STRING_T optlstCmdLine_help_opt_param0 = {
    {PARAMWHICH_String, 0, "name", (PARAM_T *)&optlstCmdLine_help_opt_param1}, 64, 0, (char *)&data_00587ca0};

Option optlstCmdLine_help_opt = {"opt|option",
                                 0x42101,
                                 (PARAM_T *)&optlstCmdLine_help_opt_param0,
                                 NULL,
                                 &optlstCmdLine_help_opt_conflicts,
                                 "show help for a given option"};

STRING_T optlstCmdLine_help_search_param0 = {{PARAMWHICH_String, 0, "keyword", NULL}, 64, 0, (char *)&data_00587ca0};

Option optlstCmdLine_help_search = {"search",
                                    0x42101,
                                    (PARAM_T *)&optlstCmdLine_help_search_param0,
                                    NULL,
                                    &optlstCmdLine_help_opt_conflicts,
                                    "show help for an option whose name or help contains 'keyword' (case-sensitive)"};

MASK_T optlstCmdLine_help_group_param1 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x8000, 0x0, &data_00587ce0};

STRING_T optlstCmdLine_help_group_param0 = {
    {PARAMWHICH_String, 0, "keyword", (PARAM_T *)&optlstCmdLine_help_group_param1}, 64, 0, (char *)&data_00587ca0};

Option optlstCmdLine_help_group = {"group",
                                   0x42101,
                                   (PARAM_T *)&optlstCmdLine_help_group_param0,
                                   NULL,
                                   &optlstCmdLine_help_opt_conflicts,
                                   "show help for groups whose names contain 'keyword' (case-sensitive)"};

MASK_T optlstCmdLine_help_tool_all_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0xc00, 0x0, &data_00587ce0};

Option optlstCmdLine_help_tool_all = {"all",
                                      0x40101,
                                      (PARAM_T *)&optlstCmdLine_help_tool_all_param0,
                                      NULL,
                                      &optlstCmdLine_help_tool_all_conflicts,
                                      "show all options available in this tool"};

MASK_T optlstCmdLine_help_tool_this_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x400, 0x800, &data_00587ce0};

Option optlstCmdLine_help_tool_this = {"this",
                                       0x40101,
                                       (PARAM_T *)&optlstCmdLine_help_tool_this_param0,
                                       NULL,
                                       &optlstCmdLine_help_tool_all_conflicts,
                                       "show options executed by this tool"};

MASK_T optlstCmdLine_help_tool_other_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x800, 0x400, &data_00587ce0};

Option optlstCmdLine_help_tool_other = {"other|skipped",
                                        0x40101,
                                        (PARAM_T *)&optlstCmdLine_help_tool_other_param0,
                                        NULL,
                                        &optlstCmdLine_help_tool_all_conflicts,
                                        "show options passed to another tool"};

MASK_T optlstCmdLine_help_tool_both_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0xc00, 0x200, &data_00587ce0};

Option optlstCmdLine_help_tool_both = {"both",
                                       0x40101,
                                       (PARAM_T *)&optlstCmdLine_help_tool_both_param0,
                                       NULL,
                                       &optlstCmdLine_help_tool_all_conflicts,
                                       "show options used in all tools"};

Option *optlstCmdLine_help_tool_list[] = {&optlstCmdLine_help_tool_all, &optlstCmdLine_help_tool_this,
                                          &optlstCmdLine_help_tool_other, &optlstCmdLine_help_tool_both, NULL};

OptionList optlstCmdLine_help_tool2 = {NULL, 0x100, optlstCmdLine_help_tool_list};

MASK_T optlstCmdLine_help_tool_param0 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x200, 0x0, &data_00587ce0};

Option optlstCmdLine_help_tool = {"tool",
                                  0x8101,
                                  (PARAM_T *)&optlstCmdLine_help_tool_param0,
                                  &optlstCmdLine_help_tool2,
                                  NULL,
                                  "categorize groups of options by tool"};

Option *optlstCmdLine_help_list[] = {
    &optlstCmdLine_help_usage,      &optlstCmdLine_help_spaces,      &optlstCmdLine_help_all,
    &optlstCmdLine_help_normal,     &optlstCmdLine_help_obsolete,    &optlstCmdLine_help_ignored,
    &optlstCmdLine_help_deprecated, &optlstCmdLine_help_meaningless, &optlstCmdLine_help_compatible,
    &optlstCmdLine_help_secret,     &optlstCmdLine_help_opt,         &optlstCmdLine_help_search,
    &optlstCmdLine_help_group,      &optlstCmdLine_help_tool,        NULL};

OptionList optlstCmdLine_help2 = {NULL, 0x100, optlstCmdLine_help_list};

MASK_T optlstCmdLine_help_param2 = {PARAMWHICH_Mask, 1, NULL, NULL, 4, 0x680, 0x0, &data_00587ce0};

SET_T optlstCmdLine_help_param1 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLine_help_param2, 1, 1,
                                   &data_00587e1c};

SET_T optlstCmdLine_help_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLine_help_param1, 1, 1,
                                   &data_00587e1e};

Option optlstCmdLine_help = {"help||-help",        0x3a101, (PARAM_T *)&optlstCmdLine_help_param0,
                             &optlstCmdLine_help2, NULL,    "display help"};

SET_T optlstCmdLine_version_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &data_00587e1c};

GENERIC_T optlstCmdLine_version_param0 = {PARAMWHICH_Generic,
                                          1,
                                          NULL,
                                          (PARAM_T *)&optlstCmdLine_version_param1,
                                          (int (*)(const char *, void *, const char *, int))fn_0040cff6,
                                          NULL,
                                          NULL};

Option optlstCmdLine_version = {"version", 0x101, (PARAM_T *)&optlstCmdLine_version_param0,
                                NULL,      NULL,  "show version, configuration, and build date"};

SET_T optlstCmdLine_timing_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLine.timeWorking};

Option optlstCmdLine_timing = {"timing", 0x1c1, (PARAM_T *)&optlstCmdLine_timing_param0,
                               NULL,     NULL,  "collect timing statistics"};

GENERIC_T optlstCmdLine_progress_param1 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040cff6, NULL, NULL};

SET_T optlstCmdLine_progress_param0 = {PARAMWHICH_Set,   1, NULL, (PARAM_T *)&optlstCmdLine_progress_param1, 2, 1,
                                       &pCmdLine.verbose};

Option optlstCmdLine_progress = {"progress", 0x1c1, (PARAM_T *)&optlstCmdLine_progress_param0,
                                 NULL,       NULL,  "show progress and version"};

GENERIC_T optlstCmdLine_v_param1 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040cff6, NULL, NULL};

GENERIC_T optlstCmdLine_v_param0 = {PARAMWHICH_Generic,
                                    1,
                                    NULL,
                                    (PARAM_T *)&optlstCmdLine_v_param1,
                                    (int (*)(const char *, void *, const char *, int))fn_0040d0eb,
                                    NULL,
                                    NULL};

Option optlstCmdLine_v = {"v|verbose", 0x1c1, (PARAM_T *)&optlstCmdLine_v_param0,
                          NULL,        NULL,  "verbose information; cumulative; implies ~~progress"};

SET_T optlstCmdLine_search_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &data_00587e20};

Option optlstCmdLine_search = {
    "search",
    0x1c1,
    (PARAM_T *)&optlstCmdLine_search_param0,
    NULL,
    NULL,
    "search access paths for source files specified on the command line; may specify object code and libraries as well; this option provides the IDE's 'access paths' functionality"};

SET_T optlstCmdLine_lines_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLine.showLines};

Option optlstCmdLine_lines = {"lines", 0x11c1, (PARAM_T *)&optlstCmdLine_lines_param0, NULL, NULL, "show line count"};

SET_T optlstCmdLine_wraplines_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pCmdLine.noWrapOutput};

Option optlstCmdLine_wraplines = {"wraplines", 0x1001c1, (PARAM_T *)&optlstCmdLine_wraplines_param0,
                                  NULL,        NULL,     "word wrap messages"};

NUM_T optlstCmdLine_maxerrors_param0 = {PARAMWHICH_Number, 0, "max", NULL, 2, 0, 0, 0, &pCmdLine.maxErrors};

Option optlstCmdLine_maxerrors = {
    "maxerrors", 0x1c0, (PARAM_T *)&optlstCmdLine_maxerrors_param0,
    NULL,        NULL,  "specify maximum number of errors to print, zero means no maximum"};

NUM_T optlstCmdLine_maxwarnings_param0 = {PARAMWHICH_Number, 0, "max", NULL, 2, 0, 0, 0, &pCmdLine.maxWarnings};

Option optlstCmdLine_maxwarnings = {
    "maxwarnings", 0x1c0, (PARAM_T *)&optlstCmdLine_maxwarnings_param0,
    NULL,          NULL,  "specify maximum number of warnings to print, zero means no maximum"};

SET_T optlstCmdLine_msgstyle_mpw_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pCmdLine.msgStyle};

Option optlstCmdLine_msgstyle_mpw = {"mpw",
                                     0x401c1,
                                     (PARAM_T *)&optlstCmdLine_msgstyle_mpw_param0,
                                     NULL,
                                     &optlstCmdLine_msgstyle_mpw_conflicts,
                                     "use MPW message style"};

SET_T optlstCmdLine_msgstyle_std_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0, &pCmdLine.msgStyle};

Option optlstCmdLine_msgstyle_std = {"std",
                                     0x401c1,
                                     (PARAM_T *)&optlstCmdLine_msgstyle_std_param0,
                                     NULL,
                                     &optlstCmdLine_msgstyle_mpw_conflicts,
                                     "use standard message style"};

SET_T optlstCmdLine_msgstyle_gcc_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 1, &pCmdLine.msgStyle};

Option optlstCmdLine_msgstyle_gcc = {"gcc",
                                     0x401c1,
                                     (PARAM_T *)&optlstCmdLine_msgstyle_gcc_param0,
                                     NULL,
                                     &optlstCmdLine_msgstyle_mpw_conflicts,
                                     "use GCC-like message style"};

SET_T optlstCmdLine_msgstyle_IDE_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 3, &pCmdLine.msgStyle};

Option optlstCmdLine_msgstyle_IDE = {"IDE",
                                     0x401c1,
                                     (PARAM_T *)&optlstCmdLine_msgstyle_IDE_param0,
                                     NULL,
                                     &optlstCmdLine_msgstyle_mpw_conflicts,
                                     "use CW IDE-like message style"};

SET_T optlstCmdLine_msgstyle_parseable_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 4, &pCmdLine.msgStyle};

Option optlstCmdLine_msgstyle_parseable = {"parseable",
                                           0x401c1,
                                           (PARAM_T *)&optlstCmdLine_msgstyle_parseable_param0,
                                           NULL,
                                           &optlstCmdLine_msgstyle_mpw_conflicts,
                                           "use context-free machine-parseable message style"};

Option *optlstCmdLine_msgstyle_list[] = {&optlstCmdLine_msgstyle_mpw,       &optlstCmdLine_msgstyle_std,
                                         &optlstCmdLine_msgstyle_gcc,       &optlstCmdLine_msgstyle_IDE,
                                         &optlstCmdLine_msgstyle_parseable, NULL};

OptionList optlstCmdLine_msgstyle2 = {NULL, 0x701, optlstCmdLine_msgstyle_list};

OptionList optlstCmdLine_msgstyle_mpw_conflicts = {NULL, 0x701, optlstCmdLine_msgstyle_list};

Option optlstCmdLine_msgstyle = {
    "msgstyle", 0x81c1, NULL, &optlstCmdLine_msgstyle2, NULL, "set error/warning message style"};

GENERIC_T optlstCmdLine_stderr_param1 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d002, NULL, NULL};

SET_T optlstCmdLine_stderr_param0 = {PARAMWHICH_Set,         1, NULL, (PARAM_T *)&optlstCmdLine_stderr_param1, 1, 0,
                                     &pCmdLine.stderr2stdout};

Option optlstCmdLine_stderr = {
    "stderr", 0x1001c1, (PARAM_T *)&optlstCmdLine_stderr_param0,
    NULL,     NULL,     "use separate stderr and stdout streams; if using ~~nostderr, stderr goes to stdout"};

GENERIC_T optlstCmdLine_opt_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040cd55, NULL, NULL};

Option optlstCmdLine_opt = {"", 0x100, (PARAM_T *)&optlstCmdLine_opt_param0, NULL, NULL, NULL};

Option *optlstCmdLine_list[] = {&optlstCmdLine_help,      &optlstCmdLine_version,
                                &optlstCmdLine_timing,    &optlstCmdLine_progress,
                                &optlstCmdLine_v,         &optlstCmdLine_search,
                                &optlstCmdLine_lines,     &optlstCmdLine_wraplines,
                                &optlstCmdLine_maxerrors, &optlstCmdLine_maxwarnings,
                                &optlstCmdLine_msgstyle,  &optlstCmdLine_stderr,
                                &optlstCmdLine_opt,       NULL};

OptionList optlstCmdLine = {
    "General Command-Line Options\n\tPlease see '~~help usage' for details about the meaning of this help.\010", 0x700,
    optlstCmdLine_list};

Option *optlstCmdLine_help_all_conflicts_list[] = {&optlstCmdLine_help_all, &optlstCmdLine_help_normal, NULL};

OptionList optlstCmdLine_help_all_conflicts = {NULL, 0x0, optlstCmdLine_help_all_conflicts_list};

Option *optlstCmdLine_help_opt_conflicts_list[] = {&optlstCmdLine_help_opt, &optlstCmdLine_help_search,
                                                   &optlstCmdLine_help_group, NULL};

OptionList optlstCmdLine_help_opt_conflicts = {NULL, 0x0, optlstCmdLine_help_opt_conflicts_list};

Option *optlstCmdLine_help_tool_all_conflicts_list[] = {&optlstCmdLine_help_tool_all, &optlstCmdLine_help_tool_this,
                                                        &optlstCmdLine_help_tool_other, &optlstCmdLine_help_tool_both,
                                                        NULL};

OptionList optlstCmdLine_help_tool_all_conflicts = {NULL, 0x0, optlstCmdLine_help_tool_all_conflicts_list};

SET_T optlstCmdLineCompiler_browse_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLineCompiler.browserEnabled};

Option optlstCmdLineCompiler_browse = {"browse", 0x1101, (PARAM_T *)&optlstCmdLineCompiler_browse_param0,
                                       NULL,     NULL,   "record browse information to file named <output file>.b"};

SET_T optlstCmdLineCompiler_c_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pCmdLine.state};

Option optlstCmdLineCompiler_c = {"c",  0x101, (PARAM_T *)&optlstCmdLineCompiler_c_param0,
                                  NULL, NULL,  "compile only, do not link"};

GENERIC_T optlstCmdLineCompiler_codegen_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_stage_settings,
    "Cg",
    NULL};

Option optlstCmdLineCompiler_codegen = {"codegen", 0x100101, (PARAM_T *)&optlstCmdLineCompiler_codegen_param0,
                                        NULL,      NULL,     "generate object code"};

SET_T optlstCmdLineCompiler_convertpaths_param0 = {
    PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pCmdLineCompiler.canonicalIncludes};

Option optlstCmdLineCompiler_convertpaths = {
    "convertpaths",
    0x100101,
    (PARAM_T *)&optlstCmdLineCompiler_convertpaths_param0,
    NULL,
    NULL,
    "interpret #include paths specified in a foreign operating system; i.e., <sys/stat.h> or <:sys:stat.h>; when enabled, '/' and ':' will separate directories and cannot be used in filenames (note: this is not a problem on Win32, since these characters are already disallowed in filenames; it is safe to leave the option 'on')"};

SET_T optlstCmdLineCompiler_cwd_proj_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0, &pCmdLineCompiler.includeSearch};

Option optlstCmdLineCompiler_cwd_proj = {"proj",
                                         0x40100,
                                         (PARAM_T *)&optlstCmdLineCompiler_cwd_proj_param0,
                                         NULL,
                                         &optlstCmdLineCompiler_cwd_proj_conflicts,
                                         "begin search in current working directory"};

SET_T optlstCmdLineCompiler_cwd_source_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 1, &pCmdLineCompiler.includeSearch};

Option optlstCmdLineCompiler_cwd_source = {"source",
                                           0x40100,
                                           (PARAM_T *)&optlstCmdLineCompiler_cwd_source_param0,
                                           NULL,
                                           &optlstCmdLineCompiler_cwd_proj_conflicts,
                                           "begin search in directory of source file"};

SET_T optlstCmdLineCompiler_cwd_explicit_param0 = {
    PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pCmdLineCompiler.includeSearch};

Option optlstCmdLineCompiler_cwd_explicit = {
    "explicit",
    0x40100,
    (PARAM_T *)&optlstCmdLineCompiler_cwd_explicit_param0,
    NULL,
    &optlstCmdLineCompiler_cwd_proj_conflicts,
    "begin search with first (~~I)nclude path; does not implicitly search current working directory"};

Option *optlstCmdLineCompiler_cwd_list[] = {&optlstCmdLineCompiler_cwd_proj, &optlstCmdLineCompiler_cwd_source,
                                            &optlstCmdLineCompiler_cwd_explicit, NULL};

OptionList optlstCmdLineCompiler_cwd2 = {NULL, 0x101, optlstCmdLineCompiler_cwd_list};

OptionList optlstCmdLineCompiler_cwd_proj_conflicts = {NULL, 0x101, optlstCmdLineCompiler_cwd_list};

Option optlstCmdLineCompiler_cwd = {
    "cwd", 0x8100, NULL, &optlstCmdLineCompiler_cwd2, NULL, "specify #include searching semantics"};

SETTING_T optlstCmdLineCompiler_D_param0 = {
    PARAMWHICH_Setting, 6, "name", NULL, (int (*)(const char *, const char *))append_define_directive, "value"};

Option optlstCmdLineCompiler_D = {"D|d|define", 0x106, (PARAM_T *)&optlstCmdLineCompiler_D_param0,
                                  NULL,         NULL,  "define symbol 'name' to 'value' if specified, else '1'"};

SET_T optlstCmdLineCompiler_defaults_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &useDefaultIncludes};

Option optlstCmdLineCompiler_defaults = {"defaults", 0x100141, (PARAM_T *)&optlstCmdLineCompiler_defaults_param0,
                                         NULL,       NULL,     "same as '~~[no]stdinc'"};

GENERIC_T optlstCmdLineCompiler_dis_param1 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              NULL,
                                              (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                              "Ds",
                                              NULL};

SET_T optlstCmdLineCompiler_dis_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_dis_param1, 2, 2,
                                          &pCmdLine.state};

Option optlstCmdLineCompiler_dis = {
    "dis|disassemble", 0x1c1, (PARAM_T *)&optlstCmdLineCompiler_dis_param0, NULL, NULL, "disassemble files to stdout"};

GENERIC_T optlstCmdLineCompiler_E_param1 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                            "Pp",
                                            NULL};

SET_T optlstCmdLineCompiler_E_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_E_param1, 2, 2,
                                        &pCmdLine.state};

Option optlstCmdLineCompiler_E = {"E",  0x105, (PARAM_T *)&optlstCmdLineCompiler_E_param0,
                                  NULL, NULL,  "preprocess source files"};

GENERIC_T optlstCmdLineCompiler_EP_param2 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))fn_0040d283,
                                             "#pragma simple_prepdump on\n",
                                             NULL};

GENERIC_T optlstCmdLineCompiler_EP_param1 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             (PARAM_T *)&optlstCmdLineCompiler_EP_param2,
                                             (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                             "Pp",
                                             NULL};

SET_T optlstCmdLineCompiler_EP_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_EP_param1, 2, 2,
                                         &pCmdLine.state};

Option optlstCmdLineCompiler_EP = {"EP", 0x105, (PARAM_T *)&optlstCmdLineCompiler_EP_param0,
                                   NULL, NULL,  "preprocess and strip out #line directives"};

STRING_T optlstCmdLineCompiler_ext_param0 = {
    {PARAMWHICH_String, 0, "extension", NULL}, 15, 0, pCmdLineCompiler.objFileExt};

Option optlstCmdLineCompiler_ext = {
    "ext",
    0x101,
    (PARAM_T *)&optlstCmdLineCompiler_ext_param0,
    NULL,
    NULL,
    "specify extension for generated object files; with a leading period ('.'), appends extension; without, replaces source file's extension"};

SETSTRING_T optlstCmdLineCompiler_fatext_param0 = {
    {PARAMWHICH_SetString, 1, NULL, NULL}, "eppc.o", 0, pCmdLineCompiler.objFileExt};

Option optlstCmdLineCompiler_fatext = {"fatext", 0x101, (PARAM_T *)&optlstCmdLineCompiler_fatext_param0,
                                       NULL,     NULL,  "use 'eppc.o' as extension for generated object files"};

SET_T optlstCmdLineCompiler_force_compile_param0 = {
    PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLineCompiler.compileIgnored};

Option optlstCmdLineCompiler_force_compile = {"force_compile",
                                              0x101101,
                                              (PARAM_T *)&optlstCmdLineCompiler_force_compile_param0,
                                              NULL,
                                              NULL,
                                              "force compilation of unrecognized files"};

SET_T optlstCmdLineCompiler_i_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pCmdLineCompiler.includeSearch};

SET_T optlstCmdLineCompiler_i_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_i_param1, 1, 1,
                                        &data_0058851b};

Option optlstCmdLineCompiler_i = {
    "i-|I-",
    0x101,
    (PARAM_T *)&optlstCmdLineCompiler_i_param0,
    NULL,
    NULL,
    "delimit groups of user paths (before ~~I-) from system paths (after ~~I-); system paths are searched with '#include <...>' as well as '#include \"...\"'; implies '~~cwd explicit' (so add '-I.' if necessary)"};

GENERIC_T optlstCmdLineCompiler_I_param0 = {
    PARAMWHICH_Generic, 0, "path", NULL, (int (*)(const char *, void *, const char *, int))fn_0040c9e7, NULL, NULL};

Option optlstCmdLineCompiler_I = {
    "I|i",
    0x107,
    (PARAM_T *)&optlstCmdLineCompiler_I_param0,
    NULL,
    NULL,
    "add path to list of #include search paths; initially, all paths are user paths (#include \"...\"); use '~~I-' to switch to system paths (#include <...>)"};

GENERIC_T optlstCmdLineCompiler_ir_param0 = {
    PARAMWHICH_Generic, 0, "path", NULL, (int (*)(const char *, void *, const char *, int))fn_0040c9e7, "", NULL};

Option optlstCmdLineCompiler_ir = {"ir", 0x101, (PARAM_T *)&optlstCmdLineCompiler_ir_param0,
                                   NULL, NULL,  "add a recursive access path to list of #include search paths"};

STRING_T optlstCmdLineCompiler_linkername_param0 = {
    {PARAMWHICH_String, 0, NULL, NULL}, 64, 0, pCmdLineCompiler.linkerName};

Option optlstCmdLineCompiler_linkername = {"linkername", 0x1101, (PARAM_T *)&optlstCmdLineCompiler_linkername_param0,
                                           NULL,         NULL,   "give name of alternate linker"};

GENERIC_T optlstCmdLineCompiler_M_param1 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                            "Dp",
                                            NULL};

SET_T optlstCmdLineCompiler_M_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_M_param1, 2, 2,
                                        &pCmdLine.state};

Option optlstCmdLineCompiler_M = {
    "M",  0x105, (PARAM_T *)&optlstCmdLineCompiler_M_param0,
    NULL, NULL,  "scan source files for dependencies and emit Makefile, do not generate object code"};

GENERIC_T optlstCmdLineCompiler_MM_param2 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                             "Dp",
                                             NULL};

SET_T optlstCmdLineCompiler_MM_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MM_param2, 1, 1, &pCmdLineCompiler.depsOnlyUserFiles};

SET_T optlstCmdLineCompiler_MM_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MM_param1, 2, 2,
                                         &pCmdLine.state};

Option optlstCmdLineCompiler_MM = {"MM", 0x105, (PARAM_T *)&optlstCmdLineCompiler_MM_param0,
                                   NULL, NULL,  "like ~~M, but do not list system include files"};

GENERIC_T optlstCmdLineCompiler_MD_param2 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                             "DpCg",
                                             NULL};

MASK_T optlstCmdLineCompiler_MD_param1 = {
    PARAMWHICH_Mask, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MD_param2, 2, 0xa, 0x0, &pCmdLine.toDisk};

SET_T optlstCmdLineCompiler_MD_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MD_param1, 2, 2,
                                         &pCmdLine.state};

Option optlstCmdLineCompiler_MD = {
    "MD", 0x105, (PARAM_T *)&optlstCmdLineCompiler_MD_param0,
    NULL, NULL,  "like ~~M, but write dependency map to a file and generate object code"};

GENERIC_T optlstCmdLineCompiler_MMD_param3 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              NULL,
                                              (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                              "DpCg",
                                              NULL};

SET_T optlstCmdLineCompiler_MMD_param2 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MMD_param3, 1, 1, &pCmdLineCompiler.depsOnlyUserFiles};

MASK_T optlstCmdLineCompiler_MMD_param1 = {
    PARAMWHICH_Mask, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MMD_param2, 2, 0xa, 0x0, &pCmdLine.toDisk};

SET_T optlstCmdLineCompiler_MMD_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_MMD_param1, 2, 2,
                                          &pCmdLine.state};

Option optlstCmdLineCompiler_MMD = {"MMD", 0x105, (PARAM_T *)&optlstCmdLineCompiler_MMD_param0,
                                    NULL,  NULL,  "like ~~MD, but do not list system include files"};

GENERIC_T optlstCmdLineCompiler_make_param1 = {PARAMWHICH_Generic,
                                               1,
                                               NULL,
                                               NULL,
                                               (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                               "Dp",
                                               NULL};

SET_T optlstCmdLineCompiler_make_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_make_param1, 2, 2,
                                           &pCmdLine.state};

Option optlstCmdLineCompiler_make = {
    "make", 0x101, (PARAM_T *)&optlstCmdLineCompiler_make_param0,
    NULL,   NULL,  "scan source files for dependencies and emit Makefile, do not generate object code"};

SET_T optlstCmdLineCompiler_nofail_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLineCompiler.noFail};

Option optlstCmdLineCompiler_nofail = {"nofail", 0x100, (PARAM_T *)&optlstCmdLineCompiler_nofail_param0,
                                       NULL,     NULL,  "continue working after errors in earlier files"};

SET_T optlstCmdLineCompiler_nolink_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pCmdLine.state};

Option optlstCmdLineCompiler_nolink = {"nolink", 0x101, (PARAM_T *)&optlstCmdLineCompiler_nolink_param0,
                                       NULL,     NULL,  "compile only, do not link"};

GENERIC_T optlstCmdLineCompiler_noprecompile_param1 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d002, NULL, NULL};

SET_T optlstCmdLineCompiler_noprecompile_param0 = {PARAMWHICH_Set,
                                                   1,
                                                   NULL,
                                                   (PARAM_T *)&optlstCmdLineCompiler_noprecompile_param1,
                                                   1,
                                                   2,
                                                   &pCmdLineCompiler.forcePrecompile};

Option optlstCmdLineCompiler_noprecompile = {"noprecompile",
                                             0x40100,
                                             (PARAM_T *)&optlstCmdLineCompiler_noprecompile_param0,
                                             NULL,
                                             &optlstCmdLineCompiler_noprecompile_conflicts,
                                             "do not precompile any files based on the filename extension"};

SET_T optlstCmdLineCompiler_nosyspath_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLineCompiler.noSysPath};

Option optlstCmdLineCompiler_nosyspath = {"nosyspath", 0x101, (PARAM_T *)&optlstCmdLineCompiler_nosyspath_param0,
                                          NULL,        NULL,  "treat #include <...> like #include \"...\""};

GENERIC_T optlstCmdLineCompiler_o_param0 = {PARAMWHICH_Generic,
                                            0,
                                            "file|dir",
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))set_output_path,
                                            NULL,
                                            NULL};

Option optlstCmdLineCompiler_o = {
    "o",  0x100, (PARAM_T *)&optlstCmdLineCompiler_o_param0,
    NULL, NULL,  "specify output filename or directory for object file(s) or text output"};

GENERIC_T optlstCmdLineCompiler_P_param2 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                            "Pp",
                                            NULL};

MASK_T optlstCmdLineCompiler_P_param1 = {
    PARAMWHICH_Mask, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_P_param2, 2, 0x1, 0x0, &pCmdLine.toDisk};

SET_T optlstCmdLineCompiler_P_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_P_param1, 2, 2,
                                        &pCmdLine.state};

Option optlstCmdLineCompiler_P = {"P",  0x105, (PARAM_T *)&optlstCmdLineCompiler_P_param0,
                                  NULL, NULL,  "preprocess and send output to file; do not generate code"};

GENERIC_T optlstCmdLineCompiler_precompile_param3 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d002, NULL, NULL};

SET_T optlstCmdLineCompiler_precompile_param2 = {PARAMWHICH_Set,
                                                 1,
                                                 NULL,
                                                 (PARAM_T *)&optlstCmdLineCompiler_precompile_param3,
                                                 1,
                                                 1,
                                                 &pCmdLineCompiler.forcePrecompile};

GENERIC_T optlstCmdLineCompiler_precompile_param1 = {PARAMWHICH_Generic,
                                                     0,
                                                     "file|dir",
                                                     (PARAM_T *)&optlstCmdLineCompiler_precompile_param2,
                                                     (int (*)(const char *, void *, const char *, int))set_output_path,
                                                     NULL,
                                                     NULL};

SET_T optlstCmdLineCompiler_precompile_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_precompile_param1, 2, 2, &pCmdLine.state};

Option optlstCmdLineCompiler_precompile = {
    "precompile",
    0x40100,
    (PARAM_T *)&optlstCmdLineCompiler_precompile_param0,
    NULL,
    &optlstCmdLineCompiler_noprecompile_conflicts,
    "generate precompiled header from source; write header to 'file' if specified, or put header in 'dir'; if argument is \"\", write header to source-specified location; if neither is defined, header filename is derived from source filename; note: the driver can tell whether to precompile a file based on its extension; '~~precompile file source' then is the same as '~~c ~~o file source'"};

GENERIC_T optlstCmdLineCompiler_preprocess_param1 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_stage_settings,
    "Pp",
    NULL};

SET_T optlstCmdLineCompiler_preprocess_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_preprocess_param1, 2, 2, &pCmdLine.state};

Option optlstCmdLineCompiler_preprocess = {"preprocess", 0x101, (PARAM_T *)&optlstCmdLineCompiler_preprocess_param0,
                                           NULL,         NULL,  "preprocess source files"};

GENERIC_T optlstCmdLineCompiler_prefix_param0 = {
    PARAMWHICH_Generic,
    0,
    "file",
    NULL,
    (int (*)(const char *, void *, const char *, int))append_include_directive,
    NULL,
    NULL};

Option optlstCmdLineCompiler_prefix = {"prefix", 0x100, (PARAM_T *)&optlstCmdLineCompiler_prefix_param0,
                                       NULL,     NULL,  "prefix text file or precompiled header onto all source files"};

GENERIC_T optlstCmdLineCompiler_S_param2 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                            "Ds",
                                            NULL};

MASK_T optlstCmdLineCompiler_S_param1 = {
    PARAMWHICH_Mask, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_S_param2, 2, 0x4, 0x0, &pCmdLine.toDisk};

SET_T optlstCmdLineCompiler_S_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineCompiler_S_param1, 2, 2,
                                        &pCmdLine.state};

Option optlstCmdLineCompiler_S = {"S",  0x1c5, (PARAM_T *)&optlstCmdLineCompiler_S_param0,
                                  NULL, NULL,  "disassemble and send output to file"};

SET_T optlstCmdLineCompiler_stdinc_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &useDefaultIncludes};

Option optlstCmdLineCompiler_stdinc = {
    "stdinc",
    0x100101,
    (PARAM_T *)&optlstCmdLineCompiler_stdinc_param0,
    NULL,
    NULL,
    "use standard system include paths (specified by the environment variable %MWCIncludes%); added after all system '~~I' paths"};

GENERIC_T optlstCmdLineCompiler_U_param0 = {PARAMWHICH_Generic,
                                            0,
                                            "name",
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))append_undef_directive,
                                            NULL,
                                            NULL};

Option optlstCmdLineCompiler_U = {"U|u|undefine", 0x106, (PARAM_T *)&optlstCmdLineCompiler_U_param0,
                                  NULL,           NULL,  "undefine symbol 'name'"};

Option *optlstCmdLineCompiler_list[] = {&optlstCmdLineCompiler_browse,
                                        &optlstCmdLineCompiler_c,
                                        &optlstCmdLineCompiler_codegen,
                                        &optlstCmdLineCompiler_convertpaths,
                                        &optlstCmdLineCompiler_cwd,
                                        &optlstCmdLineCompiler_D,
                                        &optlstCmdLineCompiler_defaults,
                                        &optlstCmdLineCompiler_dis,
                                        &optlstCmdLineCompiler_E,
                                        &optlstCmdLineCompiler_EP,
                                        &optlstCmdLineCompiler_ext,
                                        &optlstCmdLineCompiler_fatext,
                                        &optlstCmdLineCompiler_force_compile,
                                        &optlstCmdLineCompiler_i,
                                        &optlstCmdLineCompiler_I,
                                        &optlstCmdLineCompiler_ir,
                                        &optlstCmdLineCompiler_linkername,
                                        &optlstCmdLineCompiler_M,
                                        &optlstCmdLineCompiler_MM,
                                        &optlstCmdLineCompiler_MD,
                                        &optlstCmdLineCompiler_MMD,
                                        &optlstCmdLineCompiler_make,
                                        &optlstCmdLineCompiler_nofail,
                                        &optlstCmdLineCompiler_nolink,
                                        &optlstCmdLineCompiler_noprecompile,
                                        &optlstCmdLineCompiler_nosyspath,
                                        &optlstCmdLineCompiler_o,
                                        &optlstCmdLineCompiler_P,
                                        &optlstCmdLineCompiler_precompile,
                                        &optlstCmdLineCompiler_preprocess,
                                        &optlstCmdLineCompiler_prefix,
                                        &optlstCmdLineCompiler_S,
                                        &optlstCmdLineCompiler_stdinc,
                                        &optlstCmdLineCompiler_U,
                                        NULL};

OptionList optlstCmdLineCompiler = {"Preprocessing, Precompiling, and Input File Control Options\n", 0x100,
                                    optlstCmdLineCompiler_list};

Option *optlstCmdLineCompiler_noprecompile_conflicts_list[] = {&optlstCmdLineCompiler_noprecompile,
                                                               &optlstCmdLineCompiler_precompile, NULL};

OptionList optlstCmdLineCompiler_noprecompile_conflicts = {NULL, 0x0,
                                                           optlstCmdLineCompiler_noprecompile_conflicts_list};

GENERIC_T optlstCmdLineLinker_dis_param2 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                            "Ds",
                                            NULL};

SET_T optlstCmdLineLinker_dis_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineLinker_dis_param2, 1, 0, NULL};

SET_T optlstCmdLineLinker_dis_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineLinker_dis_param1, 2, 2,
                                        &pCmdLine.state};

Option optlstCmdLineLinker_dis = {"dis|disassemble",
                                  0xc1,
                                  (PARAM_T *)&optlstCmdLineLinker_dis_param0,
                                  NULL,
                                  NULL,
                                  "disassemble object code and do not link; implies '~~nostdlib'"};

GENERIC_T optlstCmdLineLinker_L_param0 = {PARAMWHICH_Generic,
                                          0,
                                          "path",
                                          NULL,
                                          (int (*)(const char *, void *, const char *, int))log_linker_option,
                                          NULL,
                                          NULL};

Option optlstCmdLineLinker_L = {
    "L|l",
    0xc7,
    (PARAM_T *)&optlstCmdLineLinker_L_param0,
    NULL,
    NULL,
    "add library search path; default is to search current working directory and then system directories (see '~~defaults'); search paths have global scope over the command line and are searched in the order given"};

GENERIC_T optlstCmdLineLinker_lr_param0 = {
    PARAMWHICH_Generic, 0,   "path", NULL, (int (*)(const char *, void *, const char *, int))log_linker_option,
    (void *)1,          NULL};

Option optlstCmdLineLinker_lr = {"lr", 0xc1, (PARAM_T *)&optlstCmdLineLinker_lr_param0,
                                 NULL, NULL, "like '~~l', but add recursive library search path"};

GENERIC_T optlstCmdLineLinker_l_param0 = {PARAMWHICH_Generic,
                                          0,
                                          "file",
                                          NULL,
                                          (int (*)(const char *, void *, const char *, int))log_linker_option,
                                          NULL,
                                          NULL};

Option optlstCmdLineLinker_l = {
    "l",
    0xc6,
    (PARAM_T *)&optlstCmdLineLinker_l_param0,
    NULL,
    NULL,
    "add a library by searching access paths for file named lib<file>.<ext> where <ext> is a typical library extension; added before system libraries (see '~~defaults')"};

SET_T optlstCmdLineLinker_defaults_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, NULL};

Option optlstCmdLineLinker_defaults = {"defaults", 0x100041, (PARAM_T *)&optlstCmdLineLinker_defaults_param0,
                                       NULL,       NULL,     "same as ~~[no]stdlib"};

SET_T optlstCmdLineLinker_nofail_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLineCompiler.noFail};

Option optlstCmdLineLinker_nofail = {
    "nofail", 0x40, (PARAM_T *)&optlstCmdLineLinker_nofail_param0,
    NULL,     NULL, "continue importing or disassembling after errors in earlier files"};

SET_T optlstCmdLineLinker_stdlib_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, NULL};

Option optlstCmdLineLinker_stdlib = {
    "stdlib",
    0x100041,
    (PARAM_T *)&optlstCmdLineLinker_stdlib_param0,
    NULL,
    NULL,
    "use system library access paths (specified by %MWLibraries%) and add system libraries (specified by %MWLibraryFiles%)"};

GENERIC_T optlstCmdLineLinker_S_param3 = {PARAMWHICH_Generic,
                                          1,
                                          NULL,
                                          NULL,
                                          (int (*)(const char *, void *, const char *, int))parse_stage_settings,
                                          "Ds",
                                          NULL};

MASK_T optlstCmdLineLinker_S_param2 = {PARAMWHICH_Mask, 1, NULL, (PARAM_T *)&optlstCmdLineLinker_S_param3, 2, 0x4, 0x0,
                                       &pCmdLine.toDisk};

SET_T optlstCmdLineLinker_S_param1 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineLinker_S_param2, 1, 0, NULL};

SET_T optlstCmdLineLinker_S_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstCmdLineLinker_S_param1, 2, 2,
                                      &pCmdLine.state};

Option optlstCmdLineLinker_S = {"S",  0xc5, (PARAM_T *)&optlstCmdLineLinker_S_param0,
                                NULL, NULL, "disassemble and send output to file; do not link; implies '~~nostdlib'"};

Option *optlstCmdLineLinker_list[] = {
    &optlstCmdLineLinker_dis,    &optlstCmdLineLinker_L,        &optlstCmdLineLinker_lr,
    &optlstCmdLineLinker_l,      &optlstCmdLineLinker_defaults, &optlstCmdLineLinker_nofail,
    &optlstCmdLineLinker_stdlib, &optlstCmdLineLinker_S,        NULL};

OptionList optlstCmdLineLinker = {"Command-Line Linker Options\n", 0x200, optlstCmdLineLinker_list};

SET_T optlstFrontEnd_ansi_off_param2 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.ansistrict};

SET_T optlstFrontEnd_ansi_off_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_ansi_off_param2, 1, 0, &pFrontEndC.enumsalwaysint};

SET_T optlstFrontEnd_ansi_off_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_ansi_off_param1, 1, 0, &pFrontEndC.onlystdkeywords};

Option optlstFrontEnd_ansi_off = {"off",
                                  0x40100,
                                  (PARAM_T *)&optlstFrontEnd_ansi_off_param0,
                                  NULL,
                                  &optlstFrontEnd_ansi_off_conflicts,
                                  "same as '~~stdkeywords on', '~~enum min', and '~~strict off'"};

SET_T optlstFrontEnd_ansi_on_param2 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.ansistrict};

SET_T optlstFrontEnd_ansi_on_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_ansi_on_param2, 1, 0, &pFrontEndC.enumsalwaysint};

SET_T optlstFrontEnd_ansi_on_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_ansi_on_param1, 1, 1, &pFrontEndC.onlystdkeywords};

Option optlstFrontEnd_ansi_on = {"on|relaxed",
                                 0x40100,
                                 (PARAM_T *)&optlstFrontEnd_ansi_on_param0,
                                 NULL,
                                 &optlstFrontEnd_ansi_off_conflicts,
                                 "same as '~~stdkeywords off', '~~enum min', and '~~strict on'"};

SET_T optlstFrontEnd_ansi_strict_param2 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.ansistrict};

SET_T optlstFrontEnd_ansi_strict_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_ansi_strict_param2, 1, 1, &pFrontEndC.enumsalwaysint};

SET_T optlstFrontEnd_ansi_strict_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_ansi_strict_param1, 1, 1, &pFrontEndC.onlystdkeywords};

Option optlstFrontEnd_ansi_strict = {"strict",
                                     0x40100,
                                     (PARAM_T *)&optlstFrontEnd_ansi_strict_param0,
                                     NULL,
                                     &optlstFrontEnd_ansi_off_conflicts,
                                     "same as '~~stdkeywords off', '~~enum int', and '~~strict on'"};

Option *optlstFrontEnd_ansi_list[] = {&optlstFrontEnd_ansi_off, &optlstFrontEnd_ansi_on, &optlstFrontEnd_ansi_strict,
                                      NULL};

OptionList optlstFrontEnd_ansi2 = {NULL, 0x101, optlstFrontEnd_ansi_list};

OptionList optlstFrontEnd_ansi_off_conflicts = {NULL, 0x101, optlstFrontEnd_ansi_list};

Option optlstFrontEnd_ansi = {"ansi", 0x8100,
                              NULL,   &optlstFrontEnd_ansi2,
                              NULL,   "specify ANSI conformance options, overriding the given settings"};

ONOFF_T optlstFrontEnd_ARM_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.arm};

Option optlstFrontEnd_ARM = {"ARM", 0x100, (PARAM_T *)&optlstFrontEnd_ARM_param0,
                             NULL,  NULL,  "check code for ARM (Annotated C++ Reference Manual) conformance"};

ONOFF_T optlstFrontEnd_bool_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.booltruefalse};

Option optlstFrontEnd_bool = {"bool", 0x100, (PARAM_T *)&optlstFrontEnd_bool_param0,
                              NULL,   NULL,  "enable C++ 'bool' type, 'true' and 'false' constants"};

SET_T optlstFrontEnd_char_signed_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.unsignedchars};

Option optlstFrontEnd_char_signed = {"signed",
                                     0x40100,
                                     (PARAM_T *)&optlstFrontEnd_char_signed_param0,
                                     NULL,
                                     &optlstFrontEnd_char_signed_conflicts,
                                     "chars are signed"};

SET_T optlstFrontEnd_char_unsigned_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.unsignedchars};

Option optlstFrontEnd_char_unsigned = {"unsigned",
                                       0x40100,
                                       (PARAM_T *)&optlstFrontEnd_char_unsigned_param0,
                                       NULL,
                                       &optlstFrontEnd_char_signed_conflicts,
                                       "chars are unsigned"};

Option *optlstFrontEnd_char_list[] = {&optlstFrontEnd_char_signed, &optlstFrontEnd_char_unsigned, NULL};

OptionList optlstFrontEnd_char2 = {NULL, 0x101, optlstFrontEnd_char_list};

OptionList optlstFrontEnd_char_signed_conflicts = {NULL, 0x101, optlstFrontEnd_char_list};

Option optlstFrontEnd_char = {"char", 0x8100, NULL, &optlstFrontEnd_char2, NULL, "set sign of 'char'"};

ONOFF_T optlstFrontEnd_Cpp_exceptions_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.enableexceptions};

Option optlstFrontEnd_Cpp_exceptions = {"Cpp_exceptions",
                                        0x100,
                                        (PARAM_T *)&optlstFrontEnd_Cpp_exceptions_param0,
                                        NULL,
                                        NULL,
                                        "enable or disable C++ exceptions"};

GENERIC_T optlstFrontEnd_dialect_c_param1 = {
    PARAMWHICH_Generic,        1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d283,
    "#pragma cplusplus off\n", NULL};

SET_T optlstFrontEnd_dialect_c_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_dialect_c_param1, 1, 0, &pFrontEndC.cplusplus};

Option optlstFrontEnd_dialect_c = {"c",
                                   0x40100,
                                   (PARAM_T *)&optlstFrontEnd_dialect_c_param0,
                                   NULL,
                                   &optlstFrontEnd_dialect_c_conflicts,
                                   "treat source as C always"};

SET_T optlstFrontEnd_dialect_c___param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.cplusplus};

Option optlstFrontEnd_dialect_c__ = {"c++",
                                     0x40100,
                                     (PARAM_T *)&optlstFrontEnd_dialect_c___param0,
                                     NULL,
                                     &optlstFrontEnd_dialect_c_conflicts,
                                     "treat source as C++ always"};

SET_T optlstFrontEnd_dialect_ec___param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.cplusplus};

SET_T optlstFrontEnd_dialect_ec___param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_dialect_ec___param1, 1, 1, &pFrontEndC.ecplusplus};

Option optlstFrontEnd_dialect_ec__ = {
    "ec++",
    0x40100,
    (PARAM_T *)&optlstFrontEnd_dialect_ec___param0,
    NULL,
    &optlstFrontEnd_dialect_c_conflicts,
    "generate warnings for use of C++ features outside Embedded C++ subset (implies 'dialect cplus')"};

SET_T optlstFrontEnd_dialect_objc_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.objective_c};

Option optlstFrontEnd_dialect_objc = {"objc",
                                      0x40100,
                                      (PARAM_T *)&optlstFrontEnd_dialect_objc_param0,
                                      NULL,
                                      &optlstFrontEnd_dialect_c_conflicts,
                                      "allow Objective C extensions"};

Option *optlstFrontEnd_dialect_list[] = {&optlstFrontEnd_dialect_c, &optlstFrontEnd_dialect_c__,
                                         &optlstFrontEnd_dialect_ec__, &optlstFrontEnd_dialect_objc, NULL};

OptionList optlstFrontEnd_dialect2 = {NULL, 0x101, optlstFrontEnd_dialect_list};

OptionList optlstFrontEnd_dialect_c_conflicts = {NULL, 0x101, optlstFrontEnd_dialect_list};

Option optlstFrontEnd_dialect = {"dialect|lang",           0x8100, NULL,
                                 &optlstFrontEnd_dialect2, NULL,   "specify source language"};

SET_T optlstFrontEnd_enum_min_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.enumsalwaysint};

Option optlstFrontEnd_enum_min = {"min",
                                  0x40100,
                                  (PARAM_T *)&optlstFrontEnd_enum_min_param0,
                                  NULL,
                                  &optlstFrontEnd_enum_min_conflicts,
                                  "use minimum sized enums"};

SET_T optlstFrontEnd_enum_int_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.enumsalwaysint};

Option optlstFrontEnd_enum_int = {"int",
                                  0x40100,
                                  (PARAM_T *)&optlstFrontEnd_enum_int_param0,
                                  NULL,
                                  &optlstFrontEnd_enum_min_conflicts,
                                  "use int-sized enums"};

Option *optlstFrontEnd_enum_list[] = {&optlstFrontEnd_enum_min, &optlstFrontEnd_enum_int, NULL};

OptionList optlstFrontEnd_enum2 = {NULL, 0x101, optlstFrontEnd_enum_list};

OptionList optlstFrontEnd_enum_min_conflicts = {NULL, 0x101, optlstFrontEnd_enum_list};

Option optlstFrontEnd_enum = {
    "enum", 0x8100, NULL, &optlstFrontEnd_enum2, NULL, "specify word size for enumeration types"};

SET_T optlstFrontEnd_inline_on_param2 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.alwaysinline};

SET_T optlstFrontEnd_inline_on_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_on_param2, 2, 0, &pFrontEndC.inlinelevel};

SET_T optlstFrontEnd_inline_on_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_on_param1, 1, 0, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_on = {"on|smart", 0x100, (PARAM_T *)&optlstFrontEnd_inline_on_param0,
                                   NULL,       NULL,  "turn on inlining for 'inline' functions"};

SET_T optlstFrontEnd_inline_none_param2 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.alwaysinline};

SET_T optlstFrontEnd_inline_none_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_none_param2, 2, 0xffffffff, &pFrontEndC.inlinelevel};

SET_T optlstFrontEnd_inline_none_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_none_param1, 1, 1, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_none = {"none|off", 0x100, (PARAM_T *)&optlstFrontEnd_inline_none_param0,
                                     NULL,       NULL,  "turn off inlining"};

SET_T optlstFrontEnd_inline_auto_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.autoinline};

SET_T optlstFrontEnd_inline_auto_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_auto_param1, 1, 0, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_auto = {
    "auto", 0x100, (PARAM_T *)&optlstFrontEnd_inline_auto_param0,
    NULL,   NULL,  "auto-inline small functions (without 'inline' explicitly specified)"};

SET_T optlstFrontEnd_inline_noauto_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.autoinline};

Option optlstFrontEnd_inline_noauto = {"noauto", 0x100, (PARAM_T *)&optlstFrontEnd_inline_noauto_param0,
                                       NULL,     NULL,  "do not auto-inline"};

SET_T optlstFrontEnd_inline_all_param3 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.autoinline};

SET_T optlstFrontEnd_inline_all_param2 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_all_param3, 1, 0, &pFrontEndC.alwaysinline};

SET_T optlstFrontEnd_inline_all_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_all_param2, 2, 0, &pFrontEndC.inlinelevel};

SET_T optlstFrontEnd_inline_all_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_all_param1, 1, 0, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_all = {"all", 0x100, (PARAM_T *)&optlstFrontEnd_inline_all_param0,
                                    NULL,  NULL,  "turn on aggressive inlining: same as '~~inline on, auto'"};

SET_T optlstFrontEnd_inline_always_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.alwaysinline};

SET_T optlstFrontEnd_inline_always_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_always_param1, 1, 0, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_always = {"always", 0x81100, (PARAM_T *)&optlstFrontEnd_inline_always_param0,
                                       NULL,     NULL,    "always inlining functions is currently unstable"};

SET_T optlstFrontEnd_inline_deferred_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.defer_codegen};

SET_T optlstFrontEnd_inline_deferred_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_deferred_param1, 1, 0, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_deferred = {
    "deferred",
    0x100,
    (PARAM_T *)&optlstFrontEnd_inline_deferred_param0,
    NULL,
    NULL,
    "defer inlining until end of compilation unit; this allows inlining of functions in both directions"};

NUM_T optlstFrontEnd_inline_level_param1 = {PARAMWHICH_Number, 0, "n", NULL, 2, 1, 0, 8, &pFrontEndC.inlinelevel};

SET_T optlstFrontEnd_inline_level_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_inline_level_param1, 1, 0, &pFrontEndC.dontinline};

Option optlstFrontEnd_inline_level = {
    "level||depth", 0x2104, (PARAM_T *)&optlstFrontEnd_inline_level_param0,
    NULL,           NULL,   "inline functions up to 'n' levels deep; level 0 is the same as '~~inline on'"};

Option *optlstFrontEnd_inline_list[] = {
    &optlstFrontEnd_inline_on,       &optlstFrontEnd_inline_none,  &optlstFrontEnd_inline_auto,
    &optlstFrontEnd_inline_noauto,   &optlstFrontEnd_inline_all,   &optlstFrontEnd_inline_always,
    &optlstFrontEnd_inline_deferred, &optlstFrontEnd_inline_level, NULL};

OptionList optlstFrontEnd_inline2 = {NULL, 0x100, optlstFrontEnd_inline_list};

Option optlstFrontEnd_inline = {"inline", 0xa100, NULL, &optlstFrontEnd_inline2, NULL, "specify inline options"};

SET_T optlstFrontEnd_mapcr_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.mpwcnewline};

Option optlstFrontEnd_mapcr = {
    "mapcr",
    0x100100,
    (PARAM_T *)&optlstFrontEnd_mapcr_param0,
    NULL,
    NULL,
    "reverse mapping of '\\n' and '\\r' so that '\\n'==13 and '\\r'==10 (for Macintosh MPW compatability)"};

GENERIC_T optlstFrontEnd_msext_on_param0 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))fn_0040d283,
                                            "#pragma cpp_extensions on\n",
                                            NULL};

Option optlstFrontEnd_msext_on = {
    "on",
    0x40100,
    (PARAM_T *)&optlstFrontEnd_msext_on_param0,
    NULL,
    &optlstFrontEnd_msext_on_conflicts,
    "enable extensions: redefining macros,\rallowing XXX::yyy syntax when declaring method yyy of class XXX,\rallowing extra commas,\rignoring casts to the same type,\rtreating function types with equivalent parameter lists but different return types as equal,\rallowing pointer-to-integer conversions,\rand various syntactical differences"};

GENERIC_T optlstFrontEnd_msext_off_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))fn_0040d283,
                                             "#pragma cpp_extensions off\n",
                                             NULL};

Option optlstFrontEnd_msext_off = {"off",
                                   0x40100,
                                   (PARAM_T *)&optlstFrontEnd_msext_off_param0,
                                   NULL,
                                   &optlstFrontEnd_msext_on_conflicts,
                                   "disable extensions; default on non-x86 targets"};

Option *optlstFrontEnd_msext_list[] = {&optlstFrontEnd_msext_on, &optlstFrontEnd_msext_off, NULL};

OptionList optlstFrontEnd_msext2 = {NULL, 0x101, optlstFrontEnd_msext_list};

OptionList optlstFrontEnd_msext_on_conflicts = {NULL, 0x101, optlstFrontEnd_msext_list};

Option optlstFrontEnd_msext = {
    "msext", 0x8100, NULL, &optlstFrontEnd_msext2, NULL, "[dis]allow Microsoft VC++ extensions"};

SET_T optlstFrontEnd_multibyte_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.multibyteaware};

Option optlstFrontEnd_multibyte = {"multibyte|multibyteaware",
                                   0x100100,
                                   (PARAM_T *)&optlstFrontEnd_multibyte_param0,
                                   NULL,
                                   NULL,
                                   "enable multi-byte character encodings for source text, comments, and strings"};

GENERIC_T optlstFrontEnd_once_param0 = {
    PARAMWHICH_Generic,  1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d283,
    "#pragma once on\n", NULL};

Option optlstFrontEnd_once = {"once", 0x100, (PARAM_T *)&optlstFrontEnd_once_param0,
                              NULL,   NULL,  "prevent header files from being processed more than once"};

GENERIC_T optlstFrontEnd_pragma_param1 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d283, "\n", NULL};

GENERIC_T optlstFrontEnd_pragma_param0 = {PARAMWHICH_Generic,
                                          0,
                                          "...",
                                          (PARAM_T *)&optlstFrontEnd_pragma_param1,
                                          (int (*)(const char *, void *, const char *, int))fn_0040d283,
                                          "#pragma ",
                                          NULL};

Option optlstFrontEnd_pragma = {"pragma", 0x100, (PARAM_T *)&optlstFrontEnd_pragma_param0,
                                NULL,     NULL,  "define a pragma for the compiler such as \"#pragma ...\""};

SET_T optlstFrontEnd_r_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.checkprotos};

Option optlstFrontEnd_r = {"r|requireprotos",   0x100, (PARAM_T *)&optlstFrontEnd_r_param0, NULL, NULL,
                           "require prototypes"};

SET_T optlstFrontEnd_relax_pointers_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.mpwpointerstyle};

Option optlstFrontEnd_relax_pointers = {"relax_pointers",
                                        0x100,
                                        (PARAM_T *)&optlstFrontEnd_relax_pointers_param0,
                                        NULL,
                                        NULL,
                                        "relax pointer type-checking rules"};

ONOFF_T optlstFrontEnd_RTTI_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.useRTTI};

Option optlstFrontEnd_RTTI = {"RTTI", 0x100, (PARAM_T *)&optlstFrontEnd_RTTI_param0,
                              NULL,   NULL,  "select run-time typing information (for C++)"};

SET_T optlstFrontEnd_som_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.direct_to_som};

Option optlstFrontEnd_som = {"som", 0x100, (PARAM_T *)&optlstFrontEnd_som_param0,
                             NULL,  NULL,  "enable Apple's Direct-to-SOM implementation"};

SET_T optlstFrontEnd_som_env_check_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.direct_to_som};

SET_T optlstFrontEnd_som_env_check_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstFrontEnd_som_env_check_param1, 1, 1, &pFrontEndC.som_env_check};

Option optlstFrontEnd_som_env_check = {"som_env_check",
                                       0x100,
                                       (PARAM_T *)&optlstFrontEnd_som_env_check_param0,
                                       NULL,
                                       NULL,
                                       "enables automatic SOM environment and new allocation checking; implies ~~som"};

ONOFF_T optlstFrontEnd_stdkeywords_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.onlystdkeywords};

Option optlstFrontEnd_stdkeywords = {"stdkeywords", 0x100, (PARAM_T *)&optlstFrontEnd_stdkeywords_param0,
                                     NULL,          NULL,  "allow only standard keywords"};

SET_T optlstFrontEnd_str_reuse_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pFrontEndC.dontreusestrings};

Option optlstFrontEnd_str_reuse = {"reuse", 0x100100, (PARAM_T *)&optlstFrontEnd_str_reuse_param0,
                                   NULL,    NULL,     "reuse strings; equivalent strings are the same object"};

SET_T optlstFrontEnd_str_pool_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pFrontEndC.poolstrings};

Option optlstFrontEnd_str_pool = {"pool", 0x100100, (PARAM_T *)&optlstFrontEnd_str_pool_param0,
                                  NULL,   NULL,     "pool strings into a single data object"};

SET_T optlstFrontEnd_str_readonly_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.readonlystrings};

Option optlstFrontEnd_str_readonly = {"readonly", 0x100100, (PARAM_T *)&optlstFrontEnd_str_readonly_param0,
                                      NULL,       NULL,     "make all string constants read-only"};

Option *optlstFrontEnd_str_list[] = {&optlstFrontEnd_str_reuse, &optlstFrontEnd_str_pool, &optlstFrontEnd_str_readonly,
                                     NULL};

OptionList optlstFrontEnd_str2 = {NULL, 0x100, optlstFrontEnd_str_list};

Option optlstFrontEnd_str = {"str|strings",        0x8100, NULL,
                             &optlstFrontEnd_str2, NULL,   "specify string constant options"};

ONOFF_T optlstFrontEnd_strict_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.ansistrict};

Option optlstFrontEnd_strict = {"strict", 0x100, (PARAM_T *)&optlstFrontEnd_strict_param0,
                                NULL,     NULL,  "specify ANSI strictness checking"};

ONOFF_T optlstFrontEnd_trigraphs_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.trigraphs};

Option optlstFrontEnd_trigraphs = {"trigraphs", 0x100, (PARAM_T *)&optlstFrontEnd_trigraphs_param0,
                                   NULL,        NULL,  "enable recognition of trigraphs"};

ONOFF_T optlstFrontEnd_wchar_t_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pFrontEndC.wchar_type};

Option optlstFrontEnd_wchar_t = {"wchar_t", 0x100, (PARAM_T *)&optlstFrontEnd_wchar_t_param0,
                                 NULL,      NULL,  "enable wchar_t as a built-in C++ type"};

Option *optlstFrontEnd_list[] = {&optlstFrontEnd_ansi,
                                 &optlstFrontEnd_ARM,
                                 &optlstFrontEnd_bool,
                                 &optlstFrontEnd_char,
                                 &optlstFrontEnd_Cpp_exceptions,
                                 &optlstFrontEnd_dialect,
                                 &optlstFrontEnd_enum,
                                 &optlstFrontEnd_inline,
                                 &optlstFrontEnd_mapcr,
                                 &optlstFrontEnd_msext,
                                 &optlstFrontEnd_multibyte,
                                 &optlstFrontEnd_once,
                                 &optlstFrontEnd_pragma,
                                 &optlstFrontEnd_r,
                                 &optlstFrontEnd_relax_pointers,
                                 &optlstFrontEnd_RTTI,
                                 &optlstFrontEnd_som,
                                 &optlstFrontEnd_som_env_check,
                                 &optlstFrontEnd_stdkeywords,
                                 &optlstFrontEnd_str,
                                 &optlstFrontEnd_strict,
                                 &optlstFrontEnd_trigraphs,
                                 &optlstFrontEnd_wchar_t,
                                 NULL};

OptionList optlstFrontEnd = {"Front-End C/C++ Language Options\n", 0x100, optlstFrontEnd_list};

SET_T optlstDebugging_g_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &useFullPaths};

SET_T optlstDebugging_g_param0 = {PARAMWHICH_Set,     1, NULL, (PARAM_T *)&optlstDebugging_g_param1, 1, 1,
                                  &pCmdLine.debugInfo};

Option optlstDebugging_g = {"g",  0x145, (PARAM_T *)&optlstDebugging_g_param0,
                            NULL, NULL,  "generate debugging information; same as '~~sym full'"};

SET_T optlstDebugging_sym_off_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pCmdLine.debugInfo};

Option optlstDebugging_sym_off = {"off", 0x141, (PARAM_T *)&optlstDebugging_sym_off_param0,
                                  NULL,  NULL,  "do not generate debugging information"};

SET_T optlstDebugging_sym_on_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pCmdLine.debugInfo};

Option optlstDebugging_sym_on = {"on", 0x2141, (PARAM_T *)&optlstDebugging_sym_on_param0,
                                 NULL, NULL,   "turn on debugging information"};

SET_T optlstDebugging_sym_full_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &useFullPaths};

SET_T optlstDebugging_sym_full_param0 = {PARAMWHICH_Set,     1, NULL, (PARAM_T *)&optlstDebugging_sym_full_param1, 1, 1,
                                         &pCmdLine.debugInfo};

Option optlstDebugging_sym_full = {"full|fullpath",
                                   0x2141,
                                   (PARAM_T *)&optlstDebugging_sym_full_param0,
                                   NULL,
                                   NULL,
                                   "store full paths to source files"};

Option *optlstDebugging_sym_list[] = {&optlstDebugging_sym_off, &optlstDebugging_sym_on, &optlstDebugging_sym_full,
                                      NULL};

OptionList optlstDebugging_sym2 = {NULL, 0x302, optlstDebugging_sym_list};

Option optlstDebugging_sym = {"sym", 0xa141, NULL, &optlstDebugging_sym2, NULL, "specify debugging options"};

Option *optlstDebugging_list[] = {&optlstDebugging_g, &optlstDebugging_sym, NULL};

OptionList optlstDebugging = {"Debugging Control Options\n", 0x300, optlstDebugging_list};

GENERIC_T optlstOptimizer_O_param0 = {PARAMWHICH_Generic,
                                      1,
                                      NULL,
                                      NULL,
                                      (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                      "G2Pe",
                                      NULL};

Option optlstOptimizer_O = {"O", 0x100, (PARAM_T *)&optlstOptimizer_O_param0, NULL, NULL, "same as '~~O2'"};

GENERIC_T optlstOptimizer_O2_0_param0 = {
    PARAMWHICH_Generic, 1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "G0-ShPeMw|",       NULL};

Option optlstOptimizer_O2_0 = {"0", 0x100, (PARAM_T *)&optlstOptimizer_O2_0_param0, NULL, NULL, "same as '~~opt off'"};

GENERIC_T optlstOptimizer_O2_1_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                         "G1",
                                         NULL};

Option optlstOptimizer_O2_1 = {"1",  0x100, (PARAM_T *)&optlstOptimizer_O2_1_param0,
                               NULL, NULL,  "same as '~~opt level=1'"};

GENERIC_T optlstOptimizer_O2_2_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                         "G2Pe",
                                         NULL};

Option optlstOptimizer_O2_2 = {"2",  0x100, (PARAM_T *)&optlstOptimizer_O2_2_param0,
                               NULL, NULL,  "same as '~~opt level=2, peephole'"};

GENERIC_T optlstOptimizer_O2_3_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                         "G3Pe",
                                         NULL};

Option optlstOptimizer_O2_3 = {"3",  0x100, (PARAM_T *)&optlstOptimizer_O2_3_param0,
                               NULL, NULL,  "same as '~~opt level=3, peephole'"};

GENERIC_T optlstOptimizer_O2_4_param0 = {
    PARAMWHICH_Generic, 1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "G4ShPeMw|",        NULL};

Option optlstOptimizer_O2_4 = {"4",  0x100, (PARAM_T *)&optlstOptimizer_O2_4_param0,
                               NULL, NULL,  "same as '~~opt level=4, peephole, schedule, functions'"};

GENERIC_T optlstOptimizer_O2_p_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                         "Gp",
                                         NULL};

Option optlstOptimizer_O2_p = {"p",  0x100, (PARAM_T *)&optlstOptimizer_O2_p_param0,
                               NULL, NULL,  "same as '~~opt speed'"};

GENERIC_T optlstOptimizer_O2_s_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                         "Gs",
                                         NULL};

Option optlstOptimizer_O2_s = {"s",  0x100, (PARAM_T *)&optlstOptimizer_O2_s_param0,
                               NULL, NULL,  "same as '~~opt space'"};

Option *optlstOptimizer_O2_list[] = {
    &optlstOptimizer_O2_0, &optlstOptimizer_O2_1, &optlstOptimizer_O2_2, &optlstOptimizer_O2_3,
    &optlstOptimizer_O2_4, &optlstOptimizer_O2_p, &optlstOptimizer_O2_s, NULL};

OptionList optlstOptimizer_O22 = {NULL, 0x100, optlstOptimizer_O2_list};

Option optlstOptimizer_O2 = {
    "O", 0x8106, NULL, &optlstOptimizer_O22, NULL, "control optimization; you may combine options as in '~~O4,p'"};

GENERIC_T optlstOptimizer_opt_off_param0 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                            "G0",
                                            NULL};

Option optlstOptimizer_opt_off = {"off|none", 0x100, (PARAM_T *)&optlstOptimizer_opt_off_param0,
                                  NULL,       NULL,  "suppress all optimizations; default"};

GENERIC_T optlstOptimizer_opt_on_param0 = {PARAMWHICH_Generic,
                                           1,
                                           NULL,
                                           NULL,
                                           (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                           "G2Pe",
                                           NULL};

Option optlstOptimizer_opt_on = {"on", 0x100, (PARAM_T *)&optlstOptimizer_opt_on_param0,
                                 NULL, NULL,  "same as ~~opt level=2, peephole"};

GENERIC_T optlstOptimizer_opt_all_param0 = {
    PARAMWHICH_Generic, 1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "G4GpShPeMw|",      NULL};

Option optlstOptimizer_opt_all = {"all|full", 0x100, (PARAM_T *)&optlstOptimizer_opt_all_param0,
                                  NULL,       NULL,  "same as ~~opt speed,level=4, peephole, schedule, functions"};

GENERIC_T optlstOptimizer_opt_space_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Gs",
    NULL};

Option optlstOptimizer_opt_space = {"space", 0x100100, (PARAM_T *)&optlstOptimizer_opt_space_param0,
                                    NULL,    NULL,     "optimize for space"};

GENERIC_T optlstOptimizer_opt_speed_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Gp",
    NULL};

Option optlstOptimizer_opt_speed = {"speed", 0x100100, (PARAM_T *)&optlstOptimizer_opt_speed_param0,
                                    NULL,    NULL,     "optimize for speed"};

GENERIC_T optlstOptimizer_opt_l_param1 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))fn_0040d8c0, NULL, NULL};

NUM_T optlstOptimizer_opt_l_param0 = {PARAMWHICH_Number,
                                      0,
                                      "num",
                                      (PARAM_T *)&optlstOptimizer_opt_l_param1,
                                      1,
                                      0,
                                      0,
                                      4,
                                      &pGlobalOptimizer.optimizationlevel};

Option optlstOptimizer_opt_l = {
    "l|level",
    0x20100,
    (PARAM_T *)&optlstOptimizer_opt_l_param0,
    NULL,
    NULL,
    "set optimization level:\rlevel 0: global register allocation only for temporary values\rlevel 1: adds dead code elimination\rlevel 2: adds common subexpression elimination and copy propagation\rlevel 3: adds loop transformations, strength reducation, and loop-invariant code motion\rlevel 4: adds repeated common subexpression elimination and loop-invariant code motion"};

GENERIC_T optlstOptimizer_opt_cse_param0 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                            "Cs",
                                            NULL};

Option optlstOptimizer_opt_cse = {"cse|commonsubs",
                                  0x100100,
                                  (PARAM_T *)&optlstOptimizer_opt_cse_param0,
                                  NULL,
                                  NULL,
                                  "common subexpression elimination"};

GENERIC_T optlstOptimizer_opt_deadcode_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Dc",
    NULL};

Option optlstOptimizer_opt_deadcode = {"deadcode", 0x100100, (PARAM_T *)&optlstOptimizer_opt_deadcode_param0,
                                       NULL,       NULL,     "removal of dead code"};

GENERIC_T optlstOptimizer_opt_deadstore_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Ds",
    NULL};

Option optlstOptimizer_opt_deadstore = {"deadstore", 0x100100, (PARAM_T *)&optlstOptimizer_opt_deadstore_param0,
                                        NULL,        NULL,     "removal of dead assignments"};

GENERIC_T optlstOptimizer_opt_lifetimes_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Lt",
    NULL};

Option optlstOptimizer_opt_lifetimes = {"lifetimes", 0x100100, (PARAM_T *)&optlstOptimizer_opt_lifetimes_param0,
                                        NULL,        NULL,     "computation of variable lifetimes"};

GENERIC_T optlstOptimizer_opt_loop_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                             "Li",
                                             NULL};

Option optlstOptimizer_opt_loop = {
    "loop|loopinvariants",       0x100100, (PARAM_T *)&optlstOptimizer_opt_loop_param0, NULL, NULL,
    "removal of loop invariants"};

GENERIC_T optlstOptimizer_opt_prop_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                             "Pr",
                                             NULL};

Option optlstOptimizer_opt_prop = {"prop|propagation",
                                   0x100100,
                                   (PARAM_T *)&optlstOptimizer_opt_prop_param0,
                                   NULL,
                                   NULL,
                                   "propagation of constant and copy assignments"};

GENERIC_T optlstOptimizer_opt_strength_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Sr",
    NULL};

Option optlstOptimizer_opt_strength = {
    "strength", 0x100100, (PARAM_T *)&optlstOptimizer_opt_strength_param0,
    NULL,       NULL,     "strength reduction; reducing multiplication by an index variable into addition"};

GENERIC_T optlstOptimizer_opt_dead_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                             "DcDs",
                                             NULL};

Option optlstOptimizer_opt_dead = {"dead", 0x100100, (PARAM_T *)&optlstOptimizer_opt_dead_param0,
                                   NULL,   NULL,     "same as ~~opt [no]deadcode and [no]deadstore"};

GENERIC_T optlstOptimizer_opt_peep_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
                                             "Pe",
                                             NULL};

Option optlstOptimizer_opt_peep = {
    "peep|peephole", 0x100100, (PARAM_T *)&optlstOptimizer_opt_peep_param0, NULL, NULL, "peephole optimization"};

GENERIC_T optlstOptimizer_opt_schedule_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Sh",
    NULL};

Option optlstOptimizer_opt_schedule = {"schedule", 0x100100, (PARAM_T *)&optlstOptimizer_opt_schedule_param0,
                                       NULL,       NULL,     NULL};

GENERIC_T optlstOptimizer_opt_functions_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "Mw",
    NULL};

Option optlstOptimizer_opt_functions = {"functions", 0x100100, (PARAM_T *)&optlstOptimizer_opt_functions_param0,
                                        NULL,        NULL,     "function epilogue/prologue"};

GENERIC_T optlstOptimizer_opt_inter_param0 = {
    PARAMWHICH_Generic, 1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "G1PeS1",           NULL};

Option optlstOptimizer_opt_inter = {"inter|local|unroll",
                                    0x1100,
                                    (PARAM_T *)&optlstOptimizer_opt_inter_param0,
                                    NULL,
                                    NULL,
                                    "opt level=1, peep, schedule=601"};

GENERIC_T optlstOptimizer_opt_rep_param0 = {
    PARAMWHICH_Generic, 1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))parse_optimizer_settings,
    "G2PeS1",           NULL};

Option optlstOptimizer_opt_rep = {"rep", 0x1100, (PARAM_T *)&optlstOptimizer_opt_rep_param0,
                                  NULL,  NULL,   "opt level=2, peep, schedule=601"};

GENERIC_T optlstOptimizer_opt_display_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))report_optimizer_options,
    NULL,
    NULL};

Option optlstOptimizer_opt_display = {"display|dump", 0x100, (PARAM_T *)&optlstOptimizer_opt_display_param0,
                                      NULL,           NULL,  "display list of active optimizations"};

Option *optlstOptimizer_opt_list[] = {
    &optlstOptimizer_opt_off,       &optlstOptimizer_opt_on,        &optlstOptimizer_opt_all,
    &optlstOptimizer_opt_space,     &optlstOptimizer_opt_speed,     &optlstOptimizer_opt_l,
    &optlstOptimizer_opt_cse,       &optlstOptimizer_opt_deadcode,  &optlstOptimizer_opt_deadstore,
    &optlstOptimizer_opt_lifetimes, &optlstOptimizer_opt_loop,      &optlstOptimizer_opt_prop,
    &optlstOptimizer_opt_strength,  &optlstOptimizer_opt_dead,      &optlstOptimizer_opt_peep,
    &optlstOptimizer_opt_schedule,  &optlstOptimizer_opt_functions, &optlstOptimizer_opt_inter,
    &optlstOptimizer_opt_rep,       &optlstOptimizer_opt_display,   NULL};

OptionList optlstOptimizer_opt2 = {NULL, 0x102, optlstOptimizer_opt_list};

Option optlstOptimizer_opt = {"opt", 0x8100, NULL, &optlstOptimizer_opt2, NULL, "specify optimization options"};

Option *optlstOptimizer_list[] = {&optlstOptimizer_O, &optlstOptimizer_O2, &optlstOptimizer_opt, NULL};

OptionList optlstOptimizer = {
    "Optimizer Options\n\tNote that all options besides '-opt off|on|all|space|speed|level=...' (marked with 'compatibility') are for backwards compatibility or special needs only; other optimization options may be superceded by use of '~~opt level=xxx'.\010",
    0x100, optlstOptimizer_list};

GENERIC_T optlstWarnings_w_off_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                         "Nw-CwWeIpEdPuUvUaEcPdHvLaIcNiSc",
                                         NULL};

Option optlstWarnings_w_off = {"off||quiet", 0x1c1, (PARAM_T *)&optlstWarnings_w_off_param0,
                               NULL,         NULL,  "turn off all warnings"};

GENERIC_T optlstWarnings_w_on_param0 = {
    PARAMWHICH_Generic,       1,   NULL, NULL, (int (*)(const char *, void *, const char *, int))parse_warning_settings,
    "CwAwIpEdPuUvUaEcPdHvLa", NULL};

Option optlstWarnings_w_on = {"on", 0x1c1, (PARAM_T *)&optlstWarnings_w_on_param0, NULL, NULL, "turn on most warnings"};

GENERIC_T optlstWarnings_w_cmdline_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                             "Cw",
                                             NULL};

Option optlstWarnings_w_cmdline = {"cmdline", 0x1001c1, (PARAM_T *)&optlstWarnings_w_cmdline_param0,
                                   NULL,      NULL,     "command-line driver/parser warnings"};

GENERIC_T optlstWarnings_w_err_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                         "We",
                                         NULL};

Option optlstWarnings_w_err = {"err|error|iserr|iserror", 0x1001c1, (PARAM_T *)&optlstWarnings_w_err_param0, NULL, NULL,
                               "treat warnings as errors"};

GENERIC_T optlstWarnings_w_all_param0 = {PARAMWHICH_Generic,
                                         1,
                                         NULL,
                                         NULL,
                                         (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                         "CwAwIpEdPuUvUaEcPdHvLaIcNiScCp",
                                         NULL};

Option optlstWarnings_w_all = {"all", 0x101, (PARAM_T *)&optlstWarnings_w_all_param0,
                               NULL,  NULL,  "turn on all warnings, require prototypes"};

GENERIC_T optlstWarnings_w_pragmas_param0 = {PARAMWHICH_Generic,
                                             1,
                                             NULL,
                                             NULL,
                                             (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                             "Ip",
                                             NULL};

Option optlstWarnings_w_pragmas = {
    "pragmas|illpragmas", 0x100101, (PARAM_T *)&optlstWarnings_w_pragmas_param0, NULL, NULL, "illegal #pragmas"};

GENERIC_T optlstWarnings_w_empty_param0 = {PARAMWHICH_Generic,
                                           1,
                                           NULL,
                                           NULL,
                                           (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                           "Ed",
                                           NULL};

Option optlstWarnings_w_empty = {"empty|emptydecl",   0x100101, (PARAM_T *)&optlstWarnings_w_empty_param0, NULL, NULL,
                                 "empty declarations"};

GENERIC_T optlstWarnings_w_possible_param0 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              NULL,
                                              (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                              "Pu",
                                              NULL};

Option optlstWarnings_w_possible = {
    "possible|unwanted",        0x100101, (PARAM_T *)&optlstWarnings_w_possible_param0, NULL, NULL,
    "possible unwanted effects"};

GENERIC_T optlstWarnings_w_unusedarg_param0 = {PARAMWHICH_Generic,
                                               1,
                                               NULL,
                                               NULL,
                                               (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                               "Ua",
                                               NULL};

Option optlstWarnings_w_unusedarg = {"unusedarg", 0x100101, (PARAM_T *)&optlstWarnings_w_unusedarg_param0,
                                     NULL,        NULL,     "unused arguments"};

GENERIC_T optlstWarnings_w_unusedvar_param0 = {PARAMWHICH_Generic,
                                               1,
                                               NULL,
                                               NULL,
                                               (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                               "Uv",
                                               NULL};

Option optlstWarnings_w_unusedvar = {"unusedvar", 0x100101, (PARAM_T *)&optlstWarnings_w_unusedvar_param0,
                                     NULL,        NULL,     "unused variables"};

GENERIC_T optlstWarnings_w_unused_param0 = {PARAMWHICH_Generic,
                                            1,
                                            NULL,
                                            NULL,
                                            (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                            "UaUv",
                                            NULL};

Option optlstWarnings_w_unused = {"unused", 0x100101, (PARAM_T *)&optlstWarnings_w_unused_param0,
                                  NULL,     NULL,     "same as ~~w [no]unusedarg,[no]unusedvar"};

GENERIC_T optlstWarnings_w_extracomma_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_warning_settings,
    "Ec",
    NULL};

Option optlstWarnings_w_extracomma = {
    "extracomma|comma", 0x100101, (PARAM_T *)&optlstWarnings_w_extracomma_param0, NULL, NULL, "extra commas"};

GENERIC_T optlstWarnings_w_pedantic_param0 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              NULL,
                                              (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                              "Pd",
                                              NULL};

Option optlstWarnings_w_pedantic = {
    "pedantic|extended", 0x100101, (PARAM_T *)&optlstWarnings_w_pedantic_param0, NULL, NULL, "pedantic error checking"};

GENERIC_T optlstWarnings_w_hidevirtual_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_warning_settings,
    "Hv",
    NULL};

Option optlstWarnings_w_hidevirtual = {
    "hidevirtual|hidden|hiddenvirtual", 0x100101, (PARAM_T *)&optlstWarnings_w_hidevirtual_param0, NULL, NULL,
    "hidden virtual functions"};

GENERIC_T optlstWarnings_w_implicit_param0 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              NULL,
                                              (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                              "Ic",
                                              NULL};

Option optlstWarnings_w_implicit = {
    "implicit|implicitconv",          0x100101, (PARAM_T *)&optlstWarnings_w_implicit_param0, NULL, NULL,
    "implicit arithmetic conversions"};

GENERIC_T optlstWarnings_w_notinlined_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_warning_settings,
    "Ni",
    NULL};

Option optlstWarnings_w_notinlined = {"notinlined", 0x100101, (PARAM_T *)&optlstWarnings_w_notinlined_param0,
                                      NULL,         NULL,     "'inline' functions not inlined"};

GENERIC_T optlstWarnings_w_largeargs_param0 = {PARAMWHICH_Generic,
                                               1,
                                               NULL,
                                               NULL,
                                               (int (*)(const char *, void *, const char *, int))parse_warning_settings,
                                               "La",
                                               NULL};

Option optlstWarnings_w_largeargs = {"largeargs", 0x100101, (PARAM_T *)&optlstWarnings_w_largeargs_param0,
                                     NULL,        NULL,     "passing large arguments to unprototyped functions"};

GENERIC_T optlstWarnings_w_structclass_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))parse_warning_settings,
    "Sc",
    NULL};

Option optlstWarnings_w_structclass = {"structclass", 0x100101, (PARAM_T *)&optlstWarnings_w_structclass_param0,
                                       NULL,          NULL,     "inconsistent use of 'class' and 'struct'"};

GENERIC_T optlstWarnings_w_display_param0 = {
    PARAMWHICH_Generic,
    1,
    NULL,
    NULL,
    (int (*)(const char *, void *, const char *, int))print_command_line_warning_options,
    NULL,
    NULL};

Option optlstWarnings_w_display = {"display|dump", 0x101, (PARAM_T *)&optlstWarnings_w_display_param0,
                                   NULL,           NULL,  "display list of active warnings"};

Option *optlstWarnings_w_list[] = {&optlstWarnings_w_off,       &optlstWarnings_w_on,
                                   &optlstWarnings_w_cmdline,   &optlstWarnings_w_err,
                                   &optlstWarnings_w_all,       &optlstWarnings_w_pragmas,
                                   &optlstWarnings_w_empty,     &optlstWarnings_w_possible,
                                   &optlstWarnings_w_unusedarg, &optlstWarnings_w_unusedvar,
                                   &optlstWarnings_w_unused,    &optlstWarnings_w_extracomma,
                                   &optlstWarnings_w_pedantic,  &optlstWarnings_w_hidevirtual,
                                   &optlstWarnings_w_implicit,  &optlstWarnings_w_notinlined,
                                   &optlstWarnings_w_largeargs, &optlstWarnings_w_structclass,
                                   &optlstWarnings_w_display,   NULL};

OptionList optlstWarnings_w2 = {NULL, 0x102, optlstWarnings_w_list};

Option optlstWarnings_w = {"w|warn|warnings", 0x8101, NULL, &optlstWarnings_w2, NULL, "warning options"};

Option *optlstWarnings_list[] = {&optlstWarnings_w, NULL};

OptionList optlstWarnings = {"C/C++ Warning Options\n", 0x700, optlstWarnings_list};

SET_T optlstBackEnd_align_power_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 2, &pBackEnd.structalignment};

Option optlstBackEnd_align_power = {"power|powerpc",
                                    0x40100,
                                    (PARAM_T *)&optlstBackEnd_align_power_param0,
                                    NULL,
                                    &optlstBackEnd_align_power_conflicts,
                                    "PowerPC alignment"};

SET_T optlstBackEnd_align_mac68k_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pBackEnd.structalignment};

Option optlstBackEnd_align_mac68k = {"mac68k",
                                     0x40100,
                                     (PARAM_T *)&optlstBackEnd_align_mac68k_param0,
                                     NULL,
                                     &optlstBackEnd_align_power_conflicts,
                                     "Macintosh 680x0 alignment"};

SET_T optlstBackEnd_align_mac68k4byte_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.structalignment};

Option optlstBackEnd_align_mac68k4byte = {"mac68k4byte",
                                          0x40100,
                                          (PARAM_T *)&optlstBackEnd_align_mac68k4byte_param0,
                                          NULL,
                                          &optlstBackEnd_align_power_conflicts,
                                          "Mac 680x0 4-byte alignment"};

GENERIC_T optlstBackEnd_align_array_param0 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              NULL,
                                              (int (*)(const char *, void *, const char *, int))fn_0040d283,
                                              "#pragma align_array_members on\n",
                                              NULL};

Option optlstBackEnd_align_array = {
    "array|arraymembers", 0x100, (PARAM_T *)&optlstBackEnd_align_array_param0, NULL, NULL, "align members of arrays"};

GENERIC_T optlstBackEnd_align_1_param0 = {PARAMWHICH_Generic,
                                          1,
                                          NULL,
                                          NULL,
                                          (int (*)(const char *, void *, const char *, int))fn_0040d283,
                                          "#pragma options align=packed\n",
                                          NULL};

Option optlstBackEnd_align_1 = {"1",
                                0x41100,
                                (PARAM_T *)&optlstBackEnd_align_1_param0,
                                NULL,
                                &optlstBackEnd_align_power_conflicts,
                                "byte alignment"};

SET_T optlstBackEnd_align_2_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pBackEnd.structalignment};

Option optlstBackEnd_align_2 = {"2",
                                0x41100,
                                (PARAM_T *)&optlstBackEnd_align_2_param0,
                                NULL,
                                &optlstBackEnd_align_power_conflicts,
                                "short alignment"};

SET_T optlstBackEnd_align_4_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 2, &pBackEnd.structalignment};

Option optlstBackEnd_align_4 = {"4",
                                0x41100,
                                (PARAM_T *)&optlstBackEnd_align_4_param0,
                                NULL,
                                &optlstBackEnd_align_power_conflicts,
                                "long alignment"};

Option *optlstBackEnd_align_list[] = {&optlstBackEnd_align_power,       &optlstBackEnd_align_mac68k,
                                      &optlstBackEnd_align_mac68k4byte, &optlstBackEnd_align_array,
                                      &optlstBackEnd_align_1,           &optlstBackEnd_align_2,
                                      &optlstBackEnd_align_4,           NULL};

OptionList optlstBackEnd_align2 = {NULL, 0x100, optlstBackEnd_align_list};

Option optlstBackEnd_align = {
    "align", 0x8100, NULL, &optlstBackEnd_align2, NULL, "specify structure/array alignment options"};

ONOFF_T optlstBackEnd_common_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pBackEnd.common};

Option optlstBackEnd_common = {"common", 0x100, (PARAM_T *)&optlstBackEnd_common_param0,
                               NULL,     NULL,  "move all uninitialized data into a common section"};

ONOFF_T optlstBackEnd_fp_contract_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pBackEnd.fp_contract};

Option optlstBackEnd_fp_contract = {"fp_contract|maf",
                                    0x100,
                                    (PARAM_T *)&optlstBackEnd_fp_contract_param0,
                                    NULL,
                                    NULL,
                                    "generate fused multiply-add instructions"};

SET_T optlstBackEnd_func_align_4_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pBackEnd.funcalign};

Option optlstBackEnd_func_align_4 = {
    "4",      0x40100, (PARAM_T *)&optlstBackEnd_func_align_4_param0, NULL, &optlstBackEnd_func_align_4_conflicts,
    "4 bytes"};

SET_T optlstBackEnd_func_align_8_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.funcalign};

Option optlstBackEnd_func_align_8 = {
    "8",      0x40100, (PARAM_T *)&optlstBackEnd_func_align_8_param0, NULL, &optlstBackEnd_func_align_4_conflicts,
    "8 bytes"};

SET_T optlstBackEnd_func_align_16_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 2, &pBackEnd.funcalign};

Option optlstBackEnd_func_align_16 = {
    "16",      0x40100, (PARAM_T *)&optlstBackEnd_func_align_16_param0, NULL, &optlstBackEnd_func_align_4_conflicts,
    "16 bytes"};

SET_T optlstBackEnd_func_align_32_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 3, &pBackEnd.funcalign};

Option optlstBackEnd_func_align_32 = {
    "32",      0x40100, (PARAM_T *)&optlstBackEnd_func_align_32_param0, NULL, &optlstBackEnd_func_align_4_conflicts,
    "32 bytes"};

SET_T optlstBackEnd_func_align_64_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 4, &pBackEnd.funcalign};

Option optlstBackEnd_func_align_64 = {
    "64",      0x40100, (PARAM_T *)&optlstBackEnd_func_align_64_param0, NULL, &optlstBackEnd_func_align_4_conflicts,
    "64 bytes"};

SET_T optlstBackEnd_func_align_128_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 5, &pBackEnd.funcalign};

Option optlstBackEnd_func_align_128 = {
    "128",      0x40100, (PARAM_T *)&optlstBackEnd_func_align_128_param0, NULL, &optlstBackEnd_func_align_4_conflicts,
    "128 bytes"};

Option *optlstBackEnd_func_align_list[] = {&optlstBackEnd_func_align_4,
                                           &optlstBackEnd_func_align_8,
                                           &optlstBackEnd_func_align_16,
                                           &optlstBackEnd_func_align_32,
                                           &optlstBackEnd_func_align_64,
                                           &optlstBackEnd_func_align_128,
                                           NULL};

OptionList optlstBackEnd_func_align2 = {NULL, 0x101, optlstBackEnd_func_align_list};

OptionList optlstBackEnd_func_align_4_conflicts = {NULL, 0x101, optlstBackEnd_func_align_list};

Option optlstBackEnd_func_align = {
    "func_align", 0x8100, NULL, &optlstBackEnd_func_align2, NULL, "specify function alignment"};

ONOFF_T optlstBackEnd_pool_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pBackEnd.pooldata};

Option optlstBackEnd_pool = {"pool|pooldata",         0x100, (PARAM_T *)&optlstBackEnd_pool_param0, NULL, NULL,
                             "pool like data objects"};

ONOFF_T optlstBackEnd_profile_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pBackEnd.profiler};

Option optlstBackEnd_profile = {
    "profile", 0x100, (PARAM_T *)&optlstBackEnd_profile_param0,
    NULL,      NULL,  "generate calls to at function entry and exit for use with a profiler"};

SET_T optlstBackEnd_rostr_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.readonlystrings};

Option optlstBackEnd_rostr = {
    "rostr|readonlystrings",          0x100, (PARAM_T *)&optlstBackEnd_rostr_param0, NULL, NULL,
    "make string constants read-only"};

ONOFF_T optlstBackEnd_schedule_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pBackEnd.schedule};

Option optlstBackEnd_schedule = {"schedule", 0x100, (PARAM_T *)&optlstBackEnd_schedule_param0,
                                 NULL,       NULL,  "schedule instructions"};

ONOFF_T optlstBackEnd_use_lmw_stmw_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pBackEnd.use_lmw_stmw};

Option optlstBackEnd_use_lmw_stmw = {
    "use_lmw_stmw", 0x100, (PARAM_T *)&optlstBackEnd_use_lmw_stmw_param0,
    NULL,           NULL,  "use multiple-word load/store instructions for structure copies"};

SET_T optlstBackEnd_vector_on_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.altivec};

Option optlstBackEnd_vector_on = {"on", 0x100, (PARAM_T *)&optlstBackEnd_vector_on_param0,
                                  NULL, NULL,  "turn on support for vector types / codegen"};

SET_T optlstBackEnd_vector_off_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pBackEnd.altivec};

Option optlstBackEnd_vector_off = {"off", 0x100, (PARAM_T *)&optlstBackEnd_vector_off_param0,
                                   NULL,  NULL,  "turn off vectorization"};

SET_T optlstBackEnd_vector_vrsave_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.vrsave};

SET_T optlstBackEnd_vector_vrsave_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstBackEnd_vector_vrsave_param1, 1, 1, &pBackEnd.altivec};

Option optlstBackEnd_vector_vrsave = {"vrsave", 0x100, (PARAM_T *)&optlstBackEnd_vector_vrsave_param0,
                                      NULL,     NULL,  "use VRSAVE prologue/epilogue code, implies '~~vector on'"};

SET_T optlstBackEnd_vector_novrsave_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pBackEnd.vrsave};

Option optlstBackEnd_vector_novrsave = {"novrsave", 0x100, (PARAM_T *)&optlstBackEnd_vector_novrsave_param0,
                                        NULL,       NULL,  "do not use VRSAVE prologue/epilogue code"};

Option optlstBackEnd_vector_toc = {"toc", 0x102900, NULL, NULL, NULL, "store small vector data in TOC"};

Option *optlstBackEnd_vector_list[] = {&optlstBackEnd_vector_on,     &optlstBackEnd_vector_off,
                                       &optlstBackEnd_vector_vrsave, &optlstBackEnd_vector_novrsave,
                                       &optlstBackEnd_vector_toc,    NULL};

OptionList optlstBackEnd_vector2 = {NULL, 0x100, optlstBackEnd_vector_list};

Option optlstBackEnd_vector = {
    "vector", 0x8100, NULL, &optlstBackEnd_vector2, NULL, "specify Altivec vectorization options"};

Option *optlstBackEnd_list[] = {&optlstBackEnd_align,
                                &optlstBackEnd_common,
                                &optlstBackEnd_fp_contract,
                                &optlstBackEnd_func_align,
                                &optlstBackEnd_pool,
                                &optlstBackEnd_profile,
                                &optlstBackEnd_rostr,
                                &optlstBackEnd_schedule,
                                &optlstBackEnd_use_lmw_stmw,
                                &optlstBackEnd_vector,
                                NULL};

OptionList optlstBackEnd = {"Embedded PowerPC Options\n", 0x100, optlstBackEnd_list};

Option *optlstBackEnd_align_power_conflicts_list[] = {&optlstBackEnd_align_power,
                                                      &optlstBackEnd_align_mac68k,
                                                      &optlstBackEnd_align_mac68k4byte,
                                                      &optlstBackEnd_align_1,
                                                      &optlstBackEnd_align_2,
                                                      &optlstBackEnd_align_4,
                                                      NULL};

OptionList optlstBackEnd_align_power_conflicts = {NULL, 0x0, optlstBackEnd_align_power_conflicts_list};

GENERIC_T optlstProject_application_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "Ap", NULL};

Option optlstProject_application = {"application",
                                    0x40041,
                                    (PARAM_T *)&optlstProject_application_param0,
                                    NULL,
                                    &optlstProject_application_conflicts,
                                    "generate an application; same as '~~xm a'"};

GENERIC_T optlstProject_library_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "St", NULL};

Option optlstProject_library = {"library",
                                0x40041,
                                (PARAM_T *)&optlstProject_library_param0,
                                NULL,
                                &optlstProject_application_conflicts,
                                "generate a static library; same as '~~xm l'"};

GENERIC_T optlstProject_partial_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "Pr", NULL};

Option optlstProject_partial = {
    "partial|r", 0x40, (PARAM_T *)&optlstProject_partial_param0,
    NULL,        NULL, "perform a partial link of objects; unresolved symbols are not errors"};

SET_T optlstProject_opt_partial_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pProject.optpartial};

GENERIC_T optlstProject_opt_partial_param0 = {PARAMWHICH_Generic,
                                              1,
                                              NULL,
                                              (PARAM_T *)&optlstProject_opt_partial_param1,
                                              (int (*)(const char *, void *, const char *, int))log_linker_option,
                                              "Pr",
                                              NULL};

Option optlstProject_opt_partial = {
    "opt_partial|r1",
    0x40,
    (PARAM_T *)&optlstProject_opt_partial_param0,
    NULL,
    NULL,
    "perform final work on a partial link; allows use of LCF file and creates tables for static constructors / destructors and C++ exception initialization; implies '~~r'"};

SET_T optlstProject_resolved_partial_param2 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pProject.resolvedpartial};

SET_T optlstProject_resolved_partial_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstProject_resolved_partial_param2, 1, 1, &pProject.optpartial};

GENERIC_T optlstProject_resolved_partial_param0 = {PARAMWHICH_Generic,
                                                   1,
                                                   NULL,
                                                   (PARAM_T *)&optlstProject_resolved_partial_param1,
                                                   (int (*)(const char *, void *, const char *, int))log_linker_option,
                                                   "Pr",
                                                   NULL};

Option optlstProject_resolved_partial = {
    "resolved_partial|r2",
    0x40,
    (PARAM_T *)&optlstProject_resolved_partial_param0,
    NULL,
    NULL,
    "perform resolved final partial link; unresolved symbols are errors; implies '~~r1'"};

SET_T optlstProject_strip_partial_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pProject.strip};

Option optlstProject_strip_partial = {"strip_partial",
                                      0x40,
                                      (PARAM_T *)&optlstProject_strip_partial_param0,
                                      NULL,
                                      NULL,
                                      "perform dead-stripping on partial link; requires '~~r1' or '~~r2'"};

ONOFF_T optlstProject_strip_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pProject.strip};

Option optlstProject_strip = {"strip", 0x1040, (PARAM_T *)&optlstProject_strip_param0,
                              NULL,    NULL,   "perform dead-stripping on partial link; requires '~~r1' or '~~r2'"};

SET_T optlstProject_big_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pProject.bigendian};

Option optlstProject_big = {"big",
                            0x401c0,
                            (PARAM_T *)&optlstProject_big_param0,
                            NULL,
                            &optlstProject_big_conflicts,
                            "generate code and link for a big-endian target"};

SET_T optlstProject_little_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pProject.bigendian};

Option optlstProject_little = {"little",
                               0x401c0,
                               (PARAM_T *)&optlstProject_little_param0,
                               NULL,
                               &optlstProject_big_conflicts,
                               "generate code and link for a little-endian target"};

SET_T optlstProject_proc_401_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0, &pBackEnd.processor};

Option optlstProject_proc_401 = {
    "401", 0x401c0, (PARAM_T *)&optlstProject_proc_401_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_403_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 1, &pBackEnd.processor};

Option optlstProject_proc_403 = {
    "403", 0x401c0, (PARAM_T *)&optlstProject_proc_403_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_505_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pBackEnd.processor};

Option optlstProject_proc_505 = {
    "505", 0x401c0, (PARAM_T *)&optlstProject_proc_505_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_509_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 3, &pBackEnd.processor};

Option optlstProject_proc_509 = {
    "509", 0x401c0, (PARAM_T *)&optlstProject_proc_509_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_555_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 4, &pBackEnd.processor};

Option optlstProject_proc_555 = {
    "555", 0x401c0, (PARAM_T *)&optlstProject_proc_555_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_601_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 5, &pBackEnd.processor};

Option optlstProject_proc_601 = {
    "601", 0x401c0, (PARAM_T *)&optlstProject_proc_601_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_602_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 6, &pBackEnd.processor};

Option optlstProject_proc_602 = {
    "602", 0x401c0, (PARAM_T *)&optlstProject_proc_602_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_603_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 7, &pBackEnd.processor};

Option optlstProject_proc_603 = {
    "603", 0x401c0, (PARAM_T *)&optlstProject_proc_603_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_603e_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 8, &pBackEnd.processor};

Option optlstProject_proc_603e = {
    "603e", 0x401c0, (PARAM_T *)&optlstProject_proc_603e_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_604_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 9, &pBackEnd.processor};

Option optlstProject_proc_604 = {
    "604", 0x401c0, (PARAM_T *)&optlstProject_proc_604_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_604e_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0xa, &pBackEnd.processor};

Option optlstProject_proc_604e = {
    "604e", 0x401c0, (PARAM_T *)&optlstProject_proc_604e_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_740_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0xb, &pBackEnd.processor};

Option optlstProject_proc_740 = {
    "740", 0x401c0, (PARAM_T *)&optlstProject_proc_740_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_750_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0xc, &pBackEnd.processor};

Option optlstProject_proc_750 = {
    "750", 0x401c0, (PARAM_T *)&optlstProject_proc_750_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_801_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0xd, &pBackEnd.processor};

Option optlstProject_proc_801 = {
    "801", 0x401c0, (PARAM_T *)&optlstProject_proc_801_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_821_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0xe, &pBackEnd.processor};

Option optlstProject_proc_821 = {
    "821", 0x401c0, (PARAM_T *)&optlstProject_proc_821_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_823_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0xf, &pBackEnd.processor};

Option optlstProject_proc_823 = {
    "823", 0x401c0, (PARAM_T *)&optlstProject_proc_823_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_850_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x10, &pBackEnd.processor};

Option optlstProject_proc_850 = {
    "850", 0x401c0, (PARAM_T *)&optlstProject_proc_850_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_860_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x11, &pBackEnd.processor};

Option optlstProject_proc_860 = {
    "860", 0x401c0, (PARAM_T *)&optlstProject_proc_860_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_7400_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x15, &pBackEnd.processor};

Option optlstProject_proc_7400 = {
    "7400", 0x401c0, (PARAM_T *)&optlstProject_proc_7400_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_8240_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x12, &pBackEnd.processor};

Option optlstProject_proc_8240 = {
    "8240", 0x401c0, (PARAM_T *)&optlstProject_proc_8240_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_8260_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x13, &pBackEnd.processor};

Option optlstProject_proc_8260 = {
    "8260", 0x401c0, (PARAM_T *)&optlstProject_proc_8260_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_gekko_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x16, &pBackEnd.processor};

Option optlstProject_proc_gekko = {
    "gekko|gecko", 0x401c0, (PARAM_T *)&optlstProject_proc_gekko_param0, NULL, &optlstProject_proc_401_conflicts, ""};

SET_T optlstProject_proc_generic_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 0x14, &pBackEnd.processor};

Option optlstProject_proc_generic = {
    "generic", 0x401c0, (PARAM_T *)&optlstProject_proc_generic_param0, NULL, &optlstProject_proc_401_conflicts, ""};

Option *optlstProject_proc_list[] = {
    &optlstProject_proc_401,  &optlstProject_proc_403,   &optlstProject_proc_505,     &optlstProject_proc_509,
    &optlstProject_proc_555,  &optlstProject_proc_601,   &optlstProject_proc_602,     &optlstProject_proc_603,
    &optlstProject_proc_603e, &optlstProject_proc_604,   &optlstProject_proc_604e,    &optlstProject_proc_740,
    &optlstProject_proc_750,  &optlstProject_proc_801,   &optlstProject_proc_821,     &optlstProject_proc_823,
    &optlstProject_proc_850,  &optlstProject_proc_860,   &optlstProject_proc_7400,    &optlstProject_proc_8240,
    &optlstProject_proc_8260, &optlstProject_proc_gekko, &optlstProject_proc_generic, NULL};

OptionList optlstProject_proc2 = {NULL, 0x701, optlstProject_proc_list};

OptionList optlstProject_proc_401_conflicts = {NULL, 0x701, optlstProject_proc_list};

Option optlstProject_proc = {"proc|processor",     0x81c0, NULL,
                             &optlstProject_proc2, NULL,   "specify processor for scheduling and inline assembler"};

SET_T optlstProject_fp_none_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pBackEnd.fpmode};

Option optlstProject_fp_none = {
    "none|off",         0x401c0, (PARAM_T *)&optlstProject_fp_none_param0, NULL, &optlstProject_fp_none_conflicts,
    "no floating point"};

SET_T optlstProject_fp_soft_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pBackEnd.fpmode};

Option optlstProject_fp_soft = {
    "soft|software",        0x401c0, (PARAM_T *)&optlstProject_fp_soft_param0, NULL, &optlstProject_fp_none_conflicts,
    "software FP emulation"};

SET_T optlstProject_fp_hard_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 2, &pBackEnd.fpmode};

Option optlstProject_fp_hard = {
    "hard|hardware",      0x401c0, (PARAM_T *)&optlstProject_fp_hard_param0, NULL, &optlstProject_fp_none_conflicts,
    "hardware FP codegen"};

SET_T optlstProject_fp_fmadd_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 3, &pBackEnd.fpmode};

Option optlstProject_fp_fmadd = {"fmadd",
                                 0x401c0,
                                 (PARAM_T *)&optlstProject_fp_fmadd_param0,
                                 NULL,
                                 &optlstProject_fp_none_conflicts,
                                 "same as '~~fp hard' and '~~fp_contract'"};

Option *optlstProject_fp_list[] = {&optlstProject_fp_none, &optlstProject_fp_soft, &optlstProject_fp_hard,
                                   &optlstProject_fp_fmadd, NULL};

OptionList optlstProject_fp2 = {NULL, 0x701, optlstProject_fp_list};

OptionList optlstProject_fp_none_conflicts = {NULL, 0x701, optlstProject_fp_list};

Option optlstProject_fp = {
    "fp", 0x81c0, NULL, &optlstProject_fp2, NULL, "specify floating-point code generation options"};

NUM_T optlstProject_sdatathreshold_param0 = {PARAMWHICH_Number, 0, NULL, NULL, 2, 0, 0, 0, &pProject.sdatathreshold};

Option optlstProject_sdatathreshold = {
    "sdatathreshold",
    0x1c0,
    (PARAM_T *)&optlstProject_sdatathreshold_param0,
    NULL,
    NULL,
    "set maximum size in bytes of mutable data objects before they are moved from small data sections"};

NUM_T optlstProject_sdata_param0 = {PARAMWHICH_Number, 0, NULL, NULL, 2, 0, 0, 0, &pProject.sdatathreshold};

Option optlstProject_sdata = {
    "sdata|sdatathreshold",
    0x1c0,
    (PARAM_T *)&optlstProject_sdata_param0,
    NULL,
    NULL,
    "set maximum size in bytes for mutable data objects before being spilled from small data section into data section"};

NUM_T optlstProject_sdata2_param0 = {PARAMWHICH_Number, 0, NULL, NULL, 2, 0, 0, 0, &pProject.sdata2threshold};

Option optlstProject_sdata2 = {
    "sdata2|sdata2threshold",
    0x1c0,
    (PARAM_T *)&optlstProject_sdata2_param0,
    NULL,
    NULL,
    "set maximum size in bytes for constant data objects before being spilled from constant section into data section"};

NUM_T optlstProject_heapsize_param0 = {PARAMWHICH_Number, 0, NULL, NULL, 4, 0, 0, 0, &pProject.heapsize};

Option optlstProject_heapsize = {"heapsize", 0x40, (PARAM_T *)&optlstProject_heapsize_param0,
                                 NULL,       NULL, "set size of data heap in kilobytes"};

NUM_T optlstProject_stacksize_param0 = {PARAMWHICH_Number, 0, NULL, NULL, 4, 0, 0, 0, &pProject.stacksize};

Option optlstProject_stacksize = {"stacksize", 0x40, (PARAM_T *)&optlstProject_stacksize_param0,
                                  NULL,        NULL, "set size of stack in kilobytes"};

SET_T optlstProject_model_absolute_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 1, &pProject.codemodel};

Option optlstProject_model_absolute = {"absolute",
                                       0x401c0,
                                       (PARAM_T *)&optlstProject_model_absolute_param0,
                                       NULL,
                                       &optlstProject_model_absolute_conflicts,
                                       "absolute code and data addressing"};

SET_T optlstProject_model_pic_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 2, &pProject.codemodel};

Option optlstProject_model_pic = {"pic",
                                  0x411c0,
                                  (PARAM_T *)&optlstProject_model_pic_param0,
                                  NULL,
                                  &optlstProject_model_absolute_conflicts,
                                  "position-independent code"};

SET_T optlstProject_model_other_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 2, 3, &pProject.codemodel};

Option optlstProject_model_other = {"other",
                                    0x401c0,
                                    (PARAM_T *)&optlstProject_model_other_param0,
                                    NULL,
                                    &optlstProject_model_absolute_conflicts,
                                    "other code model"};

Option *optlstProject_model_list[] = {&optlstProject_model_absolute, &optlstProject_model_pic,
                                      &optlstProject_model_other, NULL};

OptionList optlstProject_model2 = {NULL, 0x701, optlstProject_model_list};

OptionList optlstProject_model_absolute_conflicts = {NULL, 0x701, optlstProject_model_list};

Option optlstProject_model = {"model", 0x81c0, NULL, &optlstProject_model2, NULL, "specify code model"};

PARAM_T optlstProject_gprel_param0 = {PARAMWHICH_None, 0, NULL, NULL};

Option optlstProject_gprel = {"gprel|gprelative", 0x29c0, (PARAM_T *)&optlstProject_gprel_param0, NULL, NULL, NULL};

GENERIC_T optlstProject_xm_a_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "Ap", NULL};

Option optlstProject_xm_a = {
    "a|application|e|executable", 0x1041, (PARAM_T *)&optlstProject_xm_a_param0, NULL, NULL, "application"};

GENERIC_T optlstProject_xm_l_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "St", NULL};

Option optlstProject_xm_l = {"l|library", 0x1041, (PARAM_T *)&optlstProject_xm_l_param0, NULL, NULL, "static library"};

GENERIC_T optlstProject_xm_p_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "Pr", NULL};

Option optlstProject_xm_p = {"p|partial|partiallink", 0x1041, (PARAM_T *)&optlstProject_xm_p_param0, NULL, NULL,
                             "partial link"};

Option *optlstProject_xm_list[] = {&optlstProject_xm_a, &optlstProject_xm_l, &optlstProject_xm_p, NULL};

OptionList optlstProject_xm2 = {NULL, 0x200, optlstProject_xm_list};

Option optlstProject_xm = {
    "xm", 0x49041, NULL, &optlstProject_xm2, &optlstProject_application_conflicts, "specify project type"};

GENERIC_T optlstProject_xma_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "Ap", NULL};

Option optlstProject_xma = {"xma",
                            0x41041,
                            (PARAM_T *)&optlstProject_xma_param0,
                            NULL,
                            &optlstProject_application_conflicts,
                            "same as '~~xm a'"};

GENERIC_T optlstProject_xml_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "St", NULL};

Option optlstProject_xml = {"xml",
                            0x41041,
                            (PARAM_T *)&optlstProject_xml_param0,
                            NULL,
                            &optlstProject_application_conflicts,
                            "same as '~~xm l'"};

GENERIC_T optlstProject_xmp_param0 = {
    PARAMWHICH_Generic, 1, NULL, NULL, (int (*)(const char *, void *, const char *, int))log_linker_option, "Pr", NULL};

Option optlstProject_xmp = {"xmp",
                            0x41041,
                            (PARAM_T *)&optlstProject_xmp_param0,
                            NULL,
                            &optlstProject_application_conflicts,
                            "same as '~~xm p'"};

Option *optlstProject_list[] = {&optlstProject_application,
                                &optlstProject_library,
                                &optlstProject_partial,
                                &optlstProject_opt_partial,
                                &optlstProject_resolved_partial,
                                &optlstProject_strip_partial,
                                &optlstProject_strip,
                                &optlstProject_big,
                                &optlstProject_little,
                                &optlstProject_proc,
                                &optlstProject_fp,
                                &optlstProject_sdatathreshold,
                                &optlstProject_sdata,
                                &optlstProject_sdata2,
                                &optlstProject_heapsize,
                                &optlstProject_stacksize,
                                &optlstProject_model,
                                &optlstProject_gprel,
                                &optlstProject_xm,
                                &optlstProject_xma,
                                &optlstProject_xml,
                                &optlstProject_xmp,
                                NULL};

OptionList optlstProject = {"Embedded PPC Project Options\n", 0x700, optlstProject_list};

Option *optlstProject_application_conflicts_list[] = {&optlstProject_application,
                                                      &optlstProject_library,
                                                      &optlstProject_xm,
                                                      &optlstProject_xma,
                                                      &optlstProject_xml,
                                                      &optlstProject_xmp,
                                                      NULL};

OptionList optlstProject_application_conflicts = {NULL, 0x0, optlstProject_application_conflicts_list};

Option *optlstProject_big_conflicts_list[] = {&optlstProject_big, &optlstProject_little, NULL};

OptionList optlstProject_big_conflicts = {NULL, 0x0, optlstProject_big_conflicts_list};

STRING_T optlstLinker_m_param1 = {{PARAMWHICH_Sym, 0, NULL, NULL}, 64, 0, pLinker.mainname};

SET_T optlstLinker_m_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinker_m_param1, 1, 1, NULL};

Option optlstLinker_m = {"m|main",
                         0x40040,
                         (PARAM_T *)&optlstLinker_m_param0,
                         NULL,
                         &optlstLinker_m_conflicts,
                         "set main entry point for application or shared library"};

SETSTRING_T optlstLinker_noentry_param1 = {{PARAMWHICH_SetString, 1, NULL, NULL}, "", 0, pLinker.mainname};

SET_T optlstLinker_noentry_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinker_noentry_param1, 1, 1, NULL};

Option optlstLinker_noentry = {
    "noentry|nomain",           0x40040, (PARAM_T *)&optlstLinker_noentry_param0, NULL, &optlstLinker_m_conflicts,
    "do not use an entry point"};

FILEPATH_T optlstLinker_map_param2 = {
    {PARAMWHICH_FilePath, 0, "filename", NULL}, 1, "<outfile>.MAP", pCLTExtras.mapfilename, 255};

IFARG_T optlstLinker_map_param1 = {PARAMWHICH_IfArg,
                                   2,
                                   NULL,
                                   NULL,
                                   (PARAM_T *)&optlstLinker_map_param2,
                                   "set output filename for map file",
                                   NULL,
                                   NULL};

SET_T optlstLinker_map_param0 = {PARAMWHICH_Set,      1, NULL, (PARAM_T *)&optlstLinker_map_param1, 1, 1,
                                 &pLinker.generatemap};

Option optlstLinker_map = {"map", 0x40, (PARAM_T *)&optlstLinker_map_param0, NULL, NULL, "generate link map file"};

SET_T optlstLinker_mapunused_param1 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pLinker.listunused};

SET_T optlstLinker_mapunused_param0 = {PARAMWHICH_Set,      1, NULL, (PARAM_T *)&optlstLinker_mapunused_param1, 1, 1,
                                       &pLinker.generatemap};

Option optlstLinker_mapunused = {"mapunused|unused",
                                 0x100040,
                                 (PARAM_T *)&optlstLinker_mapunused_param0,
                                 NULL,
                                 NULL,
                                 "include list of unused symbols in map file; implies '~~map'"};

GENERIC_T optlstLinker_o_param0 = {PARAMWHICH_Generic,
                                   0,
                                   "outfile",
                                   NULL,
                                   (int (*)(const char *, void *, const char *, int))log_linker_option,
                                   NULL,
                                   NULL};

Option optlstLinker_o = {"o", 0x20040, (PARAM_T *)&optlstLinker_o_param0, NULL, NULL, "specify output filename"};

Option optlstLinker_nosrec = {"nosrec", 0x3840, NULL, NULL, NULL, "option only has a positive form"};

FILEPATH_T optlstLinker_srec_param2 = {
    {PARAMWHICH_FilePath, 0, "filename", NULL}, 1, "<outfile>.mot", pCLTExtras.srecfilename, 255};

IFARG_T optlstLinker_srec_param1 = {PARAMWHICH_IfArg,
                                    2,
                                    NULL,
                                    NULL,
                                    (PARAM_T *)&optlstLinker_srec_param2,
                                    "specify filename for S-record file",
                                    NULL,
                                    NULL};

SET_T optlstLinker_srec_param0 = {PARAMWHICH_Set,       1, NULL, (PARAM_T *)&optlstLinker_srec_param1, 1, 1,
                                  &pLinker.generatesrec};

Option optlstLinker_srec = {"srec", 0x40, (PARAM_T *)&optlstLinker_srec_param0,
                            NULL,   NULL, "generate an S-record file"};

SET_T optlstLinker_sreceol_mac_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pLinker.sreceol};

Option optlstLinker_sreceol_mac = {
    "mac",      0x40040, (PARAM_T *)&optlstLinker_sreceol_mac_param0, NULL, &optlstLinker_sreceol_mac_conflicts,
    "Macintosh"};

SET_T optlstLinker_sreceol_dos_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pLinker.sreceol};

Option optlstLinker_sreceol_dos = {
    "dos", 0x40040, (PARAM_T *)&optlstLinker_sreceol_dos_param0, NULL, &optlstLinker_sreceol_mac_conflicts, "DOS"};

SET_T optlstLinker_sreceol_unix_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 2, &pLinker.sreceol};

Option optlstLinker_sreceol_unix = {
    "unix", 0x40040, (PARAM_T *)&optlstLinker_sreceol_unix_param0, NULL, &optlstLinker_sreceol_mac_conflicts, "Unix"};

Option *optlstLinker_sreceol_list[] = {&optlstLinker_sreceol_mac, &optlstLinker_sreceol_dos, &optlstLinker_sreceol_unix,
                                       NULL};

OptionList optlstLinker_sreceol2 = {NULL, 0x201, optlstLinker_sreceol_list};

OptionList optlstLinker_sreceol_mac_conflicts = {NULL, 0x201, optlstLinker_sreceol_list};

SET_T optlstLinker_sreceol_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pLinker.generatesrec};

Option optlstLinker_sreceol = {"sreceol",
                               0x8040,
                               (PARAM_T *)&optlstLinker_sreceol_param0,
                               &optlstLinker_sreceol2,
                               NULL,
                               "set end-of-line separator for S-record file; implies '~~srec'"};

NUM_T optlstLinker_sreclength_param1 = {PARAMWHICH_Number, 0, "length", NULL, 2, 1, 8, 0xff, &pLinker.sreclength};

SET_T optlstLinker_sreclength_param0 = {PARAMWHICH_Set,       1, NULL, (PARAM_T *)&optlstLinker_sreclength_param1, 1, 1,
                                        &pLinker.generatesrec};

Option optlstLinker_sreclength = {"sreclength", 0x40, (PARAM_T *)&optlstLinker_sreclength_param0,
                                  NULL,         NULL, "specify length of S-records; implies '~~srec'"};

Option *optlstLinker_list[] = {
    &optlstLinker_m,      &optlstLinker_noentry, &optlstLinker_map,     &optlstLinker_mapunused,  &optlstLinker_o,
    &optlstLinker_nosrec, &optlstLinker_srec,    &optlstLinker_sreceol, &optlstLinker_sreclength, NULL};

OptionList optlstLinker = {"ELF Linker Options\n", 0x200, optlstLinker_list};

GENERIC_T optlstLinkerAddresses_lcf_param1 = {
    PARAMWHICH_Generic, 0, "filename", NULL, (int (*)(const char *, void *, const char *, int))fn_0040cd55, NULL, NULL};

SET_T optlstLinkerAddresses_lcf_param0 = {PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_lcf_param1, 1, 1,
                                          &pLinker.uselcf};

Option optlstLinkerAddresses_lcf = {
    "lcf",
    0x20040,
    (PARAM_T *)&optlstLinkerAddresses_lcf_param0,
    NULL,
    NULL,
    "use the linker command file in <filename> for code and data addresses; ~~codeaddr, ~~dataaddr, ~~sdataaddr and ~~sdata2addr are ignored if ~~lcf is present; <filename> must end in '.lcf'"};

NUM_T optlstLinkerAddresses_codeaddr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.codeaddr};

SET_T optlstLinkerAddresses_codeaddr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_codeaddr_param1, 1, 1, &pLinker.hascodeaddr};

Option optlstLinkerAddresses_codeaddr = {"codeaddr", 0x40, (PARAM_T *)&optlstLinkerAddresses_codeaddr_param0,
                                         NULL,       NULL, "set address for code"};

NUM_T optlstLinkerAddresses_dataaddr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.dataaddr};

SET_T optlstLinkerAddresses_dataaddr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_dataaddr_param1, 1, 1, &pLinker.hasdataaddr};

Option optlstLinkerAddresses_dataaddr = {
    "dataaddr",
    0x2040,
    (PARAM_T *)&optlstLinkerAddresses_dataaddr_param0,
    NULL,
    NULL,
    "set address for data; default is to have large data sections follow the code and large const sections"};

NUM_T optlstLinkerAddresses_sdataaddr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.sdataaddr};

SET_T optlstLinkerAddresses_sdataaddr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_sdataaddr_param1, 1, 1, &pLinker.hassdataaddr};

Option optlstLinkerAddresses_sdataaddr = {
    "sdataaddr",
    0x2040,
    (PARAM_T *)&optlstLinkerAddresses_sdataaddr_param0,
    NULL,
    NULL,
    "set address for small data; default is to have .sdata/.sbss sections follow the large data sections"};

NUM_T optlstLinkerAddresses_sdata2addr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.sdata2addr};

SET_T optlstLinkerAddresses_sdata2addr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_sdata2addr_param1, 1, 1, &pLinker.hassdataaddr};

Option optlstLinkerAddresses_sdata2addr = {
    "sdata2addr",
    0x2040,
    (PARAM_T *)&optlstLinkerAddresses_sdata2addr_param0,
    NULL,
    NULL,
    "set address for small constant data; default is to have .sdata2/.sbss2 sections follow the .sbss section"};

NUM_T optlstLinkerAddresses_stackaddr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.stackaddr};

SET_T optlstLinkerAddresses_stackaddr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_stackaddr_param1, 1, 1, &pLinker.hasstackaddr};

Option optlstLinkerAddresses_stackaddr = {"stackaddr", 0x40, (PARAM_T *)&optlstLinkerAddresses_stackaddr_param0,
                                          NULL,        NULL, "the stack"};

NUM_T optlstLinkerAddresses_heapaddr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.heapaddr};

SET_T optlstLinkerAddresses_heapaddr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_heapaddr_param1, 1, 1, &pLinker.hasheapaddr};

Option optlstLinkerAddresses_heapaddr = {
    "heapaddr",
    0x2040,
    (PARAM_T *)&optlstLinkerAddresses_heapaddr_param0,
    NULL,
    NULL,
    "set address for the heap; default is computed by starting with the stack address and subtracting the heapsize and stacksize"};

NUM_T optlstLinkerAddresses_romaddr_param1 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.romaddr};

SET_T optlstLinkerAddresses_romaddr_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstLinkerAddresses_romaddr_param1, 1, 1, &pLinker.hasromaddr};

Option optlstLinkerAddresses_romaddr = {
    "romaddr", 0x2040, (PARAM_T *)&optlstLinkerAddresses_romaddr_param0,
    NULL,      NULL,   "set address for ROM image;default is not to generate a ROM image"};

NUM_T optlstLinkerAddresses_rambuffer_param0 = {PARAMWHICH_Number, 0, "addr", NULL, 4, 0, 0, 0, &pLinker.rambuffer};

Option optlstLinkerAddresses_rambuffer = {
    "rambuffer",
    0x2040,
    (PARAM_T *)&optlstLinkerAddresses_rambuffer_param0,
    NULL,
    NULL,
    "set address for RAM buffer used by ROM images; default is to not generate a ROM image; '~~rambuffer' is ignored if '~~romaddr' is absent"};

Option *optlstLinkerAddresses_list[] = {&optlstLinkerAddresses_lcf,        &optlstLinkerAddresses_codeaddr,
                                        &optlstLinkerAddresses_dataaddr,   &optlstLinkerAddresses_sdataaddr,
                                        &optlstLinkerAddresses_sdata2addr, &optlstLinkerAddresses_stackaddr,
                                        &optlstLinkerAddresses_heapaddr,   &optlstLinkerAddresses_romaddr,
                                        &optlstLinkerAddresses_rambuffer,  NULL};

OptionList optlstLinkerAddresses = {
    "ELF Linker Address Options\n\tIf a linker command file is passed, ~~codeaddr, ~~dataaddr, ~~sdataaddr, and ~~sdata2addr are ignored.\010\tIf a linker command file is not passed, the linker will start the code sections at 0x00010000 and follow contiguously with the large const sections, the large data sections, the small data section, the small const section and then the bss sections.  If you pass one or more of ~~codeaddr, ~~dataaddr, ~~sdataaddr and ~~sdata2addr, the addresses you pass affect the starting addresses of the sections referred to.\010 \tAll addresses are valid, including 0x0.\010",
    0x200, optlstLinkerAddresses_list};

Option *optlstLinker_m_conflicts_list[] = {&optlstLinker_m, &optlstLinker_noentry, NULL};

OptionList optlstLinker_m_conflicts = {NULL, 0x0, optlstLinker_m_conflicts_list};

SET_T optlstDisassembler_fmt_x_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.extended};

Option optlstDisassembler_fmt_x = {"x",
                                   0x1400c0,
                                   (PARAM_T *)&optlstDisassembler_fmt_x_param0,
                                   NULL,
                                   &optlstDisassembler_fmt_x_conflicts,
                                   "[don't] show extended mnemonics"};

Option *optlstDisassembler_fmt_list[] = {&optlstDisassembler_fmt_x, NULL};

OptionList optlstDisassembler_fmt2 = {NULL, 0x601, optlstDisassembler_fmt_list};

OptionList optlstDisassembler_fmt_x_conflicts = {NULL, 0x601, optlstDisassembler_fmt_list};

Option optlstDisassembler_fmt = {
    "fmt|format", 0xc0c0, NULL, &optlstDisassembler_fmt2, NULL, "specify formatting options"};

SET_T optlstDisassembler_show_only_param8 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pDisassembler.showheaders};

SET_T optlstDisassembler_show_only_param7 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param8, 1, 0, &pDisassembler.showdebug};

SET_T optlstDisassembler_show_only_param6 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param7, 1, 0, &pDisassembler.showtables};

SET_T optlstDisassembler_show_only_param5 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param6, 1, 0, &pDisassembler.showdetail};

SET_T optlstDisassembler_show_only_param4 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param5, 1, 0, &pDisassembler.extended};

SET_T optlstDisassembler_show_only_param3 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param4, 1, 1, &pDisassembler.nobinary};

SET_T optlstDisassembler_show_only_param2 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param3, 1, 0, &pDisassembler.showxtables};

SET_T optlstDisassembler_show_only_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param2, 1, 0, &pDisassembler.showcode};

SET_T optlstDisassembler_show_only_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_only_param1, 1, 0, &pDisassembler.showdata};

Option optlstDisassembler_show_only = {"only|none", 0xc0, (PARAM_T *)&optlstDisassembler_show_only_param0,
                                       NULL,        NULL, "as in '~~show none' or, e.g.,\r'~~show only,code,data'"};

SET_T optlstDisassembler_show_all_param8 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showheaders};

SET_T optlstDisassembler_show_all_param7 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param8, 1, 1, &pDisassembler.showdebug};

SET_T optlstDisassembler_show_all_param6 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param7, 1, 1, &pDisassembler.showtables};

SET_T optlstDisassembler_show_all_param5 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param6, 1, 1, &pDisassembler.showdetail};

SET_T optlstDisassembler_show_all_param4 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param5, 1, 1, &pDisassembler.extended};

SET_T optlstDisassembler_show_all_param3 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param4, 1, 0, &pDisassembler.nobinary};

SET_T optlstDisassembler_show_all_param2 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param3, 1, 1, &pDisassembler.showxtables};

SET_T optlstDisassembler_show_all_param1 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param2, 1, 1, &pDisassembler.showcode};

SET_T optlstDisassembler_show_all_param0 = {
    PARAMWHICH_Set, 1, NULL, (PARAM_T *)&optlstDisassembler_show_all_param1, 1, 1, &pDisassembler.showdata};

Option optlstDisassembler_show_all = {"all", 0xc0, (PARAM_T *)&optlstDisassembler_show_all_param0,
                                      NULL,  NULL, "show everything"};

SET_T optlstDisassembler_show_binary_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 0, &pDisassembler.nobinary};

Option optlstDisassembler_show_binary = {
    "binary", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_binary_param0,
    NULL,     NULL,     "show binary information, such as address and opcodes, for object code"};

SET_T optlstDisassembler_show_code_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showcode};

Option optlstDisassembler_show_code = {"code|text", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_code_param0,
                                       NULL,        NULL,     "show .text sections"};

SET_T optlstDisassembler_show_data_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showdata};

Option optlstDisassembler_show_data = {"data", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_data_param0,
                                       NULL,   NULL,     "show data"};

SET_T optlstDisassembler_show_detail_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showdetail};

Option optlstDisassembler_show_detail = {"detail", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_detail_param0,
                                         NULL,     NULL,     "show detailed dump information"};

SET_T optlstDisassembler_show_extended_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.extended};

Option optlstDisassembler_show_extended = {"extended", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_extended_param0,
                                           NULL,       NULL,     "show extended mnemonics"};

SET_T optlstDisassembler_show_exceptions_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showxtables};

Option optlstDisassembler_show_exceptions = {"exceptions|xtab|xtables",
                                             0x1000c0,
                                             (PARAM_T *)&optlstDisassembler_show_exceptions_param0,
                                             NULL,
                                             NULL,
                                             "show exception tables; implies '~~show data'"};

SET_T optlstDisassembler_show_headers_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showheaders};

Option optlstDisassembler_show_headers = {"headers", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_headers_param0,
                                          NULL,      NULL,     "show object headers"};

SET_T optlstDisassembler_show_debug_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showdebug};

Option optlstDisassembler_show_debug = {"debug|dwarf", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_debug_param0,
                                        NULL,          NULL,     "show DWARF information"};

SET_T optlstDisassembler_show_tables_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showtables};

Option optlstDisassembler_show_tables = {"tables", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_tables_param0,
                                         NULL,     NULL,     "show string and symbol tables"};

SET_T optlstDisassembler_show_xtables_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.showxtables};

Option optlstDisassembler_show_xtables = {"xtables", 0x1000c0, (PARAM_T *)&optlstDisassembler_show_xtables_param0,
                                          NULL,      NULL,     "show exception tables"};

Option *optlstDisassembler_show_list[] = {&optlstDisassembler_show_only,
                                          &optlstDisassembler_show_all,
                                          &optlstDisassembler_show_binary,
                                          &optlstDisassembler_show_code,
                                          &optlstDisassembler_show_data,
                                          &optlstDisassembler_show_detail,
                                          &optlstDisassembler_show_extended,
                                          &optlstDisassembler_show_exceptions,
                                          &optlstDisassembler_show_headers,
                                          &optlstDisassembler_show_debug,
                                          &optlstDisassembler_show_tables,
                                          &optlstDisassembler_show_xtables,
                                          NULL};

OptionList optlstDisassembler_show2 = {NULL, 0x600, optlstDisassembler_show_list};

Option optlstDisassembler_show = {"show", 0x80c0, NULL, &optlstDisassembler_show2, NULL, "specify display options"};

SET_T optlstDisassembler_relocate_param0 = {PARAMWHICH_Set, 1, NULL, NULL, 1, 1, &pDisassembler.relocate};

Option optlstDisassembler_relocate = {
    "relocate", 0x1000c0, (PARAM_T *)&optlstDisassembler_relocate_param0,
    NULL,       NULL,     "for DWARF information, relocate addends in .rela.text and .rela.debug"};

ONOFF_T optlstDisassembler_xtables_param0 = {PARAMWHICH_OnOff, 0, NULL, NULL, &pDisassembler.showxtables};

Option optlstDisassembler_xtables = {"xtables", 0x40c0, (PARAM_T *)&optlstDisassembler_xtables_param0,
                                     NULL,      NULL,   "show exception tables"};

Option *optlstDisassembler_list[] = {&optlstDisassembler_fmt, &optlstDisassembler_show, &optlstDisassembler_relocate,
                                     &optlstDisassembler_xtables, NULL};

OptionList optlstDisassembler = {"Embedded PowerPC Disassembler Options\n", 0x600, optlstDisassembler_list};

static char *prefPanels[] = {"PPC EABI CodeGen",     "PPC EABI Linker",       "PPC EABI Disassembler",
                             "PPC EABI Project",     "EPPC Global Optimizer", "C/C++ Compiler",
                             "C/C++ Warnings",       "CmdLine Panel",         "CmdLine Compiler Panel",
                             "CmdLine Linker Panel", "CmdLine Extras EPPC"};

static OptionList *optLists[] = {&optlstCmdLine,   &optlstCmdLineCompiler, &optlstFrontEnd,
                                 &optlstOptimizer, &optlstBackEnd,         &optlstDebugging,
                                 &optlstWarnings,  &optlstProject,         &optlstCmdLineLinker,
                                 &optlstLinker,    &optlstLinkerAddresses, &optlstDisassembler};

static PrefDataPanel stPrefPanels[] = {
    {"CmdLine Panel", (unsigned char *)&pCmdLine, sizeof(PCmdLine)},
    {"CmdLine Compiler Panel", (unsigned char *)&pCmdLineCompiler, sizeof(PCmdLineCompiler)},
    {"CmdLine Linker Panel", (unsigned char *)&pCmdLineLinker, sizeof(PCmdLineLinker)},
    {"C/C++ Compiler", (unsigned char *)&pFrontEndC, sizeof(PFrontEndC)},
    {"C/C++ Warnings", (unsigned char *)&pWarningC, sizeof(PWarningC)},
    {"EPPC Global Optimizer", (unsigned char *)&pGlobalOptimizer, sizeof(PGlobalOptimizer)},
    {"PPC EABI CodeGen", (unsigned char *)&pBackEnd, sizeof(PBackEnd)},
    {"PPC EABI Linker", (unsigned char *)&pLinker, sizeof(PLinker)},
    {"PPC EABI Disassembler", (unsigned char *)&pDisassembler, sizeof(PDisassembler)},
    {"PPC EABI Project", (unsigned char *)&pProject, sizeof(PProject)},
    {"CmdLine Extras EPPC", (unsigned char *)&pCLTExtras, sizeof(PCLTExtras)},
};

PFrontEndC pFrontEndC = {0xc, 0, 0, 0, 0, 0, 0, 0, {0}, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1};

PWarningC pWarningC = {4};

PGlobalOptimizer pGlobalOptimizer = {1, 0, 1};

unsigned int fn_00405670(void)
{
    pLinker.sreceol = 1U;
    data_0058851d = 0U;
    output_path = 0U;
    directive_storage = 0U;
    useDefaultIncludes = 1U;
    return 1U;
}

int fn_004056a0(void)
{
    char *includes;
    char *includesEnd;

    if (!pCmdLine.state) {
        pCmdLine.state = 3;
    }
    if ((0 < data_00587e10) && (useDefaultIncludes != '\0')) {
        includes = ParserHelpers_GetFirstEnvironmentVariable("MWCEABIPPCIncludes\0MWCIncludes", 1, &includesEnd);
        if ((includes != NULL) && (ParserHelpers_ParsePathList(includes, ';', ':', 1, includesEnd, 1, -1, 0) == 0)) {
            return 0;
        }
    }
    return 1;
}

static char *defines_name = "(command-line defines)";

int fn_00405710(void)
{
    if (ParserHelpers_cc_EmitPragmas((Pragma *)&data_0054a388) == 0 ||
        ParserHelpers_cc_EmitPragmas(data_0054a690) == 0) {
        return 0;
    }
    if (directive_storage != NULL) {
        if (pCmdLine.verbose != 0) {
            ToolHelpers_cc_CallValuePairCallback(defines_name, directive_storage);
        }
        ToolHelpers_cc_PassVirtualFileValuePair(defines_name, &directive_storage);
        c2pstrcpy(pFrontEndC.prefixname, defines_name);
    } else {
        pFrontEndC.prefixname[0] = 0;
    }
    fn_0040d822();
    if (fn_0040d012(1) == 0) {
        return 0;
    }
    if (pBackEnd.use_lmw_stmw != 0 && pProject.bigendian == 0) {
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
    pLinker.generatesyminfo = pCmdLine.debugInfo;
    pLinker.fullpaths = useFullPaths;
    return 1;
}

#pragma scheduling reset
static ParserTool parser_tool = {'Comp',
                                 'c++ ',
                                 'ePPC',
                                 'EABI',
                                 11,
                                 prefPanels,
                                 "Metrowerks C/C++ Compiler for Embedded PowerPC",
                                 "1998-2000",
                                 12,
                                 optLists,
                                 11,
                                 stPrefPanels,
                                 (Boolean (*)(void))fn_00405670,
                                 (Boolean (*)(void))fn_004056a0,
                                 (Boolean (*)(void))fn_00405710};

int fn_00405840(void)
{
    return Targets_SetTool(&parser_tool);
}
