#ifndef DRIVER_OPTION_H
#define DRIVER_OPTION_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* plugin option tables of the driver. */
struct Option {
    char *names;
    int avail;
    struct PARAM_T *args;
    struct OptionList *def;
    struct OptionList *group;
    char *help;
};
struct OptionList {
    char *text;
    int flags;
    Option **options;
};
struct OStack {
    char *name; /* 0x00: Option_Push stores context; format_ostack reads name when flags & 4 */
    char *
        value; /* 0x04: push_option_arg supplies duplicated argument; format_ostack reads when flags & 2; fn_0041c1ae frees it */
    short flags; /* 0x08: Option_Push sets stack entry kind and format_ostack tests it */
    short
        alignmentPadding; /* 0x0a: Option_Push writes only name, value and flags; unused trailing storage in oStack. */
};
struct TokenText {
    short kind;
    char *text;
};
struct Triple {
    short a, b, c;
};
extern void fn_0041c1ae(char *a0);
extern void pop_option_stack(void);
extern void fn_0041c1e2(void);
extern int fn_0041c1eb(void);
extern void Option_Push(short flags, void *a, char *b);
extern OStack *Option_PopStack(short flags);
extern void format_ostack(char *str, short flags);
extern void Option_FormatOStackToList(void *list);
extern void Option_ResetOptionLists(void);
extern OptionList *Option_GetOptionList(void);
extern void add_option(Option *option);
extern int Option_RegisterOptionList(OptionList *list);
extern void clear_option_avail_high_bits(OptionList *options);
extern void format_option_list(char *buf, OptionList *list, int flags);
extern int Option_IsAvailable(Option *option, unsigned int mask);
extern unsigned int fn_0041c8ba(void);
extern int fn_0041c8d5(Option *option);
extern unsigned int fn_0041c913(Option *a0);
extern Boolean token_matches_kind(int kind, TokenText *tok);
extern Boolean match_option_kind(int idx, TokenText *s);
extern Boolean fn_0041ca5d(int n, TokenText *x);
extern int match_option_names(char *names, char *arg, int flags, int *result);
extern Option *find_matching_option(OptionList *list, int x, int *result);
extern unsigned int forward_varargs(unsigned int a0, ...);
extern unsigned int Option_ForwardVarArgs(unsigned int a0, ...);
extern int print_option_help(char *filter);
extern unsigned char Option_ShowHelp(void);
extern void push_option(void *a);
extern void push_option_arg(Option *opt, char *arg);
extern unsigned int Option_ParseOptionList(OptionList *holder, unsigned int flags);
extern int Option_ParseDefaultOption(OptionList *options);
extern void Option_FormatMessageWithOptionContext(SInt32 id, char *arg);
extern void Option_ReportError(SInt32 id, char *arg);
extern int show_option_help(char *name);
extern void format_and_dispatch_option_message(SInt32 id, char *arg);
extern void report_option_message(SInt32 id, char *arg);
extern int data_00587594;
extern char data_00587ca0[];
extern int option_list_count;
extern int option_count;
extern int option_capacity;
extern OStack data_00586d20[];
extern int parse_option(Option *option, int flags);
extern int parse_option_list(OptionList *options, UInt32 flags);
extern char option_name[];
extern int data_00587e10;
extern char data_00587e2a;

#ifdef __cplusplus
}
#endif

#endif
