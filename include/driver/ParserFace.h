#ifndef DRIVER_PARSERFACE_H
#define DRIVER_PARSERFACE_H

#include "compiler/common.h"
#include "driver/Targets.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DriverTool {
    int cpu;
    int os;
    int lang;
    int type;
    int argumentCount;
    char **arguments;
    char *toolInfo;
    char *copyright;
    int optionListCount;
    struct OptionList *
        *optionLists; /* 0x24: Targets_RegisterOptionLists passes each list to Option_RegisterOptionList */
    int resourceCount;
    struct Resource *resources;
    Boolean (*precheck)(void);
    Boolean (*check)(void);
    Boolean (*postcheck)(void);
};
struct EnvInfo {
    short f0;
    short width;
    short height;
    char parserEnvironmentFlag;
    char f7;
};
struct Resource {
    char *name;          /* 0x00: copy_resource_by_name compares the resource name case-insensitively */
    unsigned char *data; /* 0x04: copy_resource_by_name copies size bytes into the new handle with memcpy */
    int size;            /* 0x08: copy_resource_by_name allocates the handle and copies this many bytes */
};
extern int *driverTool;
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
extern unsigned char data_00543430[];
extern unsigned int __stdcall set_next_to_head(struct ListLink *entry);
extern struct ListLink *data_00543458;
extern unsigned int __stdcall set_link_next_from_global(struct ListLink *link);
extern struct ListLink *data_0054345c;
extern unsigned int __stdcall fn_0040bc70(struct ListLink *link);
extern int __stdcall set_list_link_next_to_global(struct ListLink *link);
extern int __stdcall match_tool(int *pair, int b, int c, unsigned char *out);
extern unsigned int __stdcall store_boolean_result(int argument0, char **argument1, unsigned char *result);
extern int __stdcall dispatch_plugin_request(struct CWPluginPrivateContext *context);
extern ListLink data_00543480;
extern ListLink data_00543460;
extern PtrList data_00587688[1];
extern PtrList data_00588044;

#ifdef __cplusplus
}
#endif

#endif
