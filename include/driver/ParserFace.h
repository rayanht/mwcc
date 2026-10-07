#ifndef DRIVER_PARSERFACE_H
#define DRIVER_PARSERFACE_H

#include "compiler/common.h"
#include "driver/Targets.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A command-line tool's parser: the tool it parses for, its preference panels, its option lists and its checks
 * before, between and after the passes over the command line. */
struct ParserTool {
    UInt32 tool;
    UInt32 lang;
    UInt32 cpu;
    UInt32 os;
    int numPrefPanels;
    char **prefPanels;
    char *toolInfo;
    char *copyright;
    int numOptionLists;
    struct OptionList **optionLists;
    int numPrefDataPanels;
    struct PrefDataPanel *prefDataPanels;
    Boolean (*preParse)(void);
    Boolean (*midParse)(void);
    Boolean (*postParse)(void);
};
struct EnvInfo {
    short f0;
    short width;
    short height;
    char parserEnvironmentFlag;
    char f7;
};
/* A preference panel's name and its default data. */
struct PrefDataPanel {
    char *name;
    unsigned char *data;
    int size;
};
extern ParserTool *pTool;
extern struct PanelEntry *data_00587cf0;
extern void *ToolHelpers_ResizeBuffer(const char *what, void *ptr, int size);
extern int initialize_cmdline_environment(struct CWPluginPrivateContext *context);
extern short data_00587ce6;
extern int num_panels;
extern int lookup_value_count;
extern char **lookup_names;
extern unsigned char data_00587cfc[];
extern unsigned char data_00587d00[];
extern char data_00587e1f;
extern int register_option_lists(struct CWPluginPrivateContext *context);
extern int run_tool_checks(struct CWPluginPrivateContext *context);
extern struct CommandLineArguments *cmdline_environment;
extern char data_00587e1d;
extern Boolean data_00587e1e;
extern Boolean tool_checks_passed;
extern void **copy_resource_by_name(char *name);
extern int fn_0040ba99(struct CWPluginPrivateContext *context);
extern int set_enabled_link_parser_entries(struct CWPluginPrivateContext *context);
extern int __stdcall get_data_pointer_and_constant(unsigned char **a0, int *a1);
extern unsigned int __stdcall set_next_to_head(struct ListLink *entry);
extern unsigned int __stdcall set_link_next_from_global(struct ListLink *link);
extern unsigned int __stdcall fn_0040bc70(struct ListLink *link);
extern int __stdcall set_list_link_next_to_global(struct ListLink *link);
extern int __stdcall match_tool(int *pair, int b, int c, unsigned char *out);
extern unsigned int __stdcall store_boolean_result(int argument0, char **argument1, unsigned char *result);
extern int __stdcall dispatch_plugin_request(struct CWPluginPrivateContext *context);
extern PtrList data_00587688[1];
extern PtrList data_00588044;

extern const char *DAT_00543380;

#ifdef __cplusplus
}
#endif

#endif
