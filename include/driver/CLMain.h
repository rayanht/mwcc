#ifndef DRIVER_CLMAIN_H
#define DRIVER_CLMAIN_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CodeRecord {
    unsigned short code;
    unsigned short data;
    unsigned int value;
};
struct CommandLinePanelSettings {
    UInt16 version;
    UInt8 reserved[0x1cc - sizeof(UInt16)];
    UInt8 version1001Option;
    UInt8 version1002Option1;
    UInt8 version1002Option2;
    UInt8 version1003Option1;
    UInt8 version1003Option2;
    UInt8 reserved2[0x2d0 - 0x1d1];
    UInt8 version1004Option;
    UInt8 reserved3[1];
};
extern char **argv;
struct CommandParseInfo {
    SInt32 argc;
    char **argv;
    SInt32 value2;
};
struct LinkerPanelSettings {
    unsigned short kind;
    unsigned char flag2;
    unsigned char flag3;
    unsigned char flag4;
    unsigned char flag5;
};
#pragma options align = mac68k
struct MessageRecord {
    unsigned short tag;
    unsigned short reserved;
    short payload[10];
    unsigned char attributes[4];
};
#pragma options align = reset
extern int CLMain_Initialize(int argc, char **argv);
extern char data_0057d930[8];
extern int data_005871bc;
extern int data_005871c0;
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
extern unsigned char latch_flag(unsigned int a0);
extern Boolean is_enabled_or_global_nonzero(char enabled);
extern unsigned int fn_0040a7c0(void);
extern unsigned int fn_0040a7d0(void);
extern unsigned char invoke_if_requested(char shouldInvoke);
extern void consume_driver_command_line_options(int *argc, char ***argv);
extern void CLMain_AppendEnabledCommandLineOptions(int *first, char ***second);
extern unsigned int CLMain_FreePlugins(unsigned int result);
extern int delete_file_and_make_path_spec(void);
extern SInt32 fn_0040aed0(void);
extern int fn_0040af70(struct MessageRecord *message, struct MessageRecord *result);
extern unsigned int copy_environment_code_record(struct CodeRecord *record, struct CodeRecord *result);
extern int convert_command_line_panel_settings(struct CommandLinePanelSettings *source,
                                               struct CommandLinePanelSettings *destination);
extern int convert_linker_panel_settings(LinkerPanelSettings *input, LinkerPanelSettings *output);
extern unsigned int create_cmdline_data_blocks(void);
extern int create_default_target(void);
extern unsigned int check_cmdline_entries(char *context);
extern UInt8 DAT_00541c0c;
extern char DAT_00541e4c[];
extern unsigned short DAT_00541e4e;
extern int DAT_00543148;
extern char *DAT_0057d920;
extern char *DAT_0057d924;
extern char *DAT_0057d928;
extern void *PTR_DAT_00541b18;
extern struct DriverCommandLineOption {
    char *name;
    char **value;
    char (*handler)(int, char *);
} PTR_DAT_00543124[3];
extern struct CommandLinePanelSettings data_00541b40;
extern SInt32 data_00541e44;
extern SInt32 data_00541e48;
extern unsigned char data_00541ee4[];
extern unsigned char data_00542f38[];
extern char *data_00542f3c[];
extern double data_00543248;
extern unsigned int CLMain_InitializeAndParseCommandLine(void);
extern SInt32 data_005871c4;
extern SInt32 data_005871c8;
extern SInt32 data_005871d4;
extern char input_name[];
extern char output_name[];
extern struct CodeRecord cmdline_code_record;
extern struct LinkerPanelSettings linker_panel_settings;
extern ListLink data_00541edc;
extern ListLink data_00541eac;

#ifdef __cplusplus
}
#endif

#endif
