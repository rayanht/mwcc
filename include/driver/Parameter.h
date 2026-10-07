#ifndef DRIVER_PARAMETER_H
#define DRIVER_PARAMETER_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BytePair {
    UInt8 bytes[2]; /* 0x00: is_mask_entry_unchanged loads the two destination bytes for its 16-bit comparison */
};
struct ByteQuad {
    UInt8 bytes
        [4]; /* 0x00: set_file_code copies file-type bytes; is_mask_entry_unchanged loads the four destination bytes for its 32-bit comparison */
};
/* An option's parameter: its kind, how it is given, its name in help, and the parameter that follows it. */
enum {
    PARAMWHICH_None,
    PARAMWHICH_FTypeCreator,
    PARAMWHICH_FilePath,
    PARAMWHICH_Number,
    PARAMWHICH_String,
    PARAMWHICH_Id,
    PARAMWHICH_Sym,
    PARAMWHICH_OnOff,
    PARAMWHICH_OffOn,
    PARAMWHICH_Mask,
    PARAMWHICH_Toggle,
    PARAMWHICH_Set,
    PARAMWHICH_SetString,
    PARAMWHICH_Generic,
    PARAMWHICH_IfArg,
    PARAMWHICH_Setting
};
#pragma pack(push, 1)
struct PARAM_T {
    signed char which;
    char flags;
    char *myname;
    struct PARAM_T *next;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct MASK_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    char size;
    UInt32 ormask;
    UInt32 andmask;
    void *num;
};
struct TOGGLE_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    char size;
    UInt32 mask;
    void *num;
};
struct SET_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    char size;
    UInt32 value;
    void *num;
};
struct NUM_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    char size;
    char fit;
    UInt32 lo;
    UInt32 hi;
    void *num;
};
struct ONOFF_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    unsigned char *var;
};
struct OFFON_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    unsigned char *var;
};
struct GENERIC_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    int (*parse)(const char *opt, void *var, const char *pstr, int flags);
    void *var;
    char *help;
};
struct IFARG_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    PARAM_T *parg;
    char *helpa;
    PARAM_T *pnone;
    char *helpn;
};
struct SETTING_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    int (*parse)(const char *name, const char *value);
    char *valuename;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct STRING_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    SInt16 maxlen;
    Boolean pstring;
    char *str;
};
struct SETSTRING_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    char *value;
    char pstring;
    char *var;
};
struct FILEPATH_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    char fflags;
    char *defaultstr;
    char *filename;
    int maxlen;
};
struct FTYPE_T {
    signed char which;
    char flags;
    char *myname;
    PARAM_T *next;
    UInt32 *fc;
    Boolean iscreator;
};
#pragma pack(pop)
extern int return_true(void);
extern unsigned int return_unsigned_zero(void);
extern void fn_00428f61(unsigned int context, void **firstList, void **secondList, void **thirdList);
extern int parse_parameter_value(PARAM_T *parameter, char **value, UInt32 flags);
extern int Parameter_CheckParameters(PARAM_T *parameter, int incomingflags);
extern Boolean is_non_text_file(char *name, Boolean flag);
extern void Parameter_ForwardVarArgs(int a0, ...);
extern void forward_stack_varargs(SInt32 a0, ...);
extern void Parameter_InitHelpColumn(HelpColumn *buf, short left, short width);
extern char option_parameter_text[4096];
extern int set_filepath(FILEPATH_T *parm, char *arg);
extern void format_num_parm(NUM_T *parm, char **name, char **help, char **value);
extern unsigned int fn_0042910d(void);
extern int fn_00429110(NUM_T *record, char *cursor);
extern void format_filecode_option(FTYPE_T *opt, char **name, int *flags, char **value);
extern unsigned int fn_00429462(void);
extern int set_file_code(FTYPE_T *opt, char *arg);
extern void format_string_name_help_value(STRING_T *parm, char **name, char **help, char **value);
extern unsigned int fn_004295ca(void);
extern int copy_idparm_arg(struct STRING_T *parm, char *arg, int x);
extern void get_string_name_help_value(STRING_T *parm, char **name, char **help, char **value);
extern unsigned int fn_004296d1(void);
extern void get_option_val_count_state(register ONOFF_T *p, register char **out_val, register SInt32 *out_count,
                                       register char **out_state);
extern unsigned int fn_004297bf(void);
extern int parse_on_off(ONOFF_T *opt, char *arg, int flags);
extern void get_on_off_option_info(OFFON_T *opt, char **name, int *flags, char **value);
extern unsigned int fn_00429896(void);
extern int set_on_off(OFFON_T *opt, char *arg, int flags);
extern void get_filepath_name_flags_value(FILEPATH_T *opt, char **name, int *flags, int *value);
extern unsigned int fn_0042994d(void);
extern void zero_unsigned_int_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int is_mask_entry_unchanged(MASK_T *p);
extern int apply_mask_entry(MASK_T *p, int unused, int flags);
extern void clear_unsigned_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int is_dest_unchanged_by_val_xor(TOGGLE_T *p);
extern int xor_const_dest(TOGGLE_T *p);
extern void zero_unsigned_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int const_matches_dest(SET_T *p);
extern Boolean store_constrec_val(SET_T *rec, void *unused, UInt32 flags);
extern void zero_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int compare_setstring_value(SETSTRING_T *parm);
extern int set_string(struct SETSTRING_T *arguments);
extern void get_setting_name_value(GENERIC_T *opt, char **name, int *value, int *flags);
extern unsigned int fn_0042a192(void);
extern void invoke_float_parameter_callback(GENERIC_T *rec, char *pstr, int flags);
extern void format_setting_help(SETTING_T *opt, char **help, int *a, int *b);
extern unsigned int fn_0042a263(void);
extern int fn_0042a266(SETTING_T *h, char *arg, int x);
extern void fn_0042a312(IFARG_T *record, char **firstOutput, char **secondOutput, int *status);
extern unsigned int get_unsigned_zero(void);
extern int evaluate_conditional_branch(IFARG_T *expr, char *a, int b);
extern int dispatch_param_by_which(PARAM_T *param, char *a, int b);
extern void Parameter_DispatchByWhich(PARAM_T *param, int *a, int *b, int *c);
extern unsigned char Parameter_DispatchParam(PARAM_T *param);
extern void push_argument_option(char *argument);
extern void fn_0042a797(void);
extern int validate_id_arg(struct STRING_T *parm, char *arg, int x);

#ifdef __cplusplus
}
#endif

#endif
