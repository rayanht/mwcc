#ifndef DRIVER_CLMAIN_H
#define DRIVER_CLMAIN_H

#include "compiler/common.h"
#include "driver/Memory.h"
#include "driver/PrefPanels.h"
#include "driver/MsDos.h"
#include "driver/CLProj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PCmdLine optsCmdLine;
extern PCmdLineEnvir optsEnvir;
extern PCmdLineCompiler optsCompiler;
extern PCmdLineLinker optsLinker;

/* The driver's state: its arguments, the target and plugin it was built for, the counts of reported diagnostics and the
 * specs of its outputs. */
struct CLState {
    int argc;
    char **argv;
    SInt32 cpu;
    SInt32 os;
    SInt32 plugintype;
    SInt32 language;
    SInt32 parserplugin;
    OSSpec programSpec;
    char *programName;
    short countWarnings;
    short countErrors;
    Boolean pluginDebug;
    char userBreak;
    char withholdWarnings;
    char withholdErrors;
    OSSpec makefileSpec;
    CLTargetDirectory sbmPathSpec;
    MemBuffer browseTableHandle;
};
extern CLState clState;

extern char **argv;
struct CommandParseInfo {
    SInt32 argc;
    char **argv;
    SInt32 value2;
};
extern int CLMain_Initialize(int argc, char **argv);
extern int parse_command_line(void);
extern SInt32 unique_plugin_name_count;
extern SInt32 plugin_request_count;
extern char **unique_plugin_names;
extern struct ToolArgumentSet *file_argument_sets;
extern struct ToolArgumentSet *tool_argument_sets;
extern struct PluginRequest *plugin_requests;
extern unsigned int __stdcall copy_global_value_to_address(unsigned int a0);
extern unsigned int __stdcall copy_global_value(unsigned int a0);
extern int __stdcall fn_0040a730(char **panelData);
extern unsigned int __stdcall fn_0040a7a0(struct ListLink *link);
extern unsigned int __stdcall set_listlink_next_to_global(struct ListLink *node);
extern Boolean latch_flag(Boolean pre, char *argument);
extern Boolean is_enabled_or_global_nonzero(Boolean pre, char *argument);
extern unsigned int fn_0040a7c0(void);
extern unsigned int fn_0040a7d0(void);
extern Boolean invoke_if_requested(Boolean pre, char *argument);
extern void consume_driver_command_line_options(int *argc, char ***argv);
extern void CLMain_AppendEnabledCommandLineOptions(int *first, char ***second);
extern unsigned int CLMain_FreePlugins(unsigned int result);
extern int delete_file_and_make_path_spec(void);
extern SInt32 fn_0040aed0(void);
extern int fn_0040af70(PCmdLine *message, PCmdLine *result);
extern unsigned int copy_environment_code_record(PCmdLineEnvir *record, PCmdLineEnvir *result);
extern int convert_command_line_panel_settings(PCmdLineCompiler *source, PCmdLineCompiler *destination);
extern int convert_linker_panel_settings(PCmdLineLinker *input, PCmdLineLinker *output);
extern unsigned int create_cmdline_data_blocks(void);
extern int create_default_target(void);
extern unsigned int check_cmdline_entries(char *context);
/* An option the driver takes itself: its handler is called with pre set when it is seen, and without to ask whether
 * to pass it on. */
struct DriverCommandLineOption {
    char *name;
    char **value;
    Boolean (*handler)(Boolean pre, char *argument);
};
extern Project mainProj;
extern Project *gProj;
extern unsigned int __stdcall return_zero(unsigned int unused);
extern unsigned int __stdcall get_data_and_size(unsigned char **data, unsigned int *size);
extern unsigned int CLMain_InitializeAndParseCommandLine(void);
extern char input_name[];
extern char output_name[];

#ifdef __cplusplus
}
#endif

#endif
