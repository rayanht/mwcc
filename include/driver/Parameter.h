#ifndef DRIVER_PARAMETER_H
#define DRIVER_PARAMETER_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct OnOff {
    signed char which; /* 0x00: Parameter_DispatchByWhich selects the parameter help handler */
    char flags;        /* 0x01: parse_parameter_value reads PARAM_T parameter flags */
    char *name; /* 0x02: get_on_off_option_info and get_option_val_count_state return the on/off parameter name */
    struct PARAM_T *next; /* 0x06: Parameter_CheckParameters follows the parameter chain */
    unsigned char *var;   /* 0x0a: get_on_off_option_info and get_option_val_count_state read the on/off value */
};
#pragma pack(pop)
struct BytePair {
    UInt8 bytes[2]; /* 0x00: is_mask_entry_unchanged loads the two destination bytes for its 16-bit comparison */
};
struct ByteQuad {
    UInt8 bytes
        [4]; /* 0x00: set_file_code copies file-type bytes; is_mask_entry_unchanged loads the four destination bytes for its 32-bit comparison */
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
struct ConstRec {
    PARAM_T base; /* 0x00: store_constrec_val constant parameter prefix */
    UInt8 kind;   /* 0x0a: store_constrec_val selects constant width */
    union {
        UInt8 b;   /* 0x0b: store_constrec_val, kind == 1 */
        UInt16 w;  /* 0x0b: store_constrec_val, kind == 2 */
        UInt32 dw; /* 0x0b: store_constrec_val, kind != 1 && kind != 2 */
    } val;         /* 0x0b: store_constrec_val width-selected constant value */
    UInt8 *dest;   /* 0x0f: store_constrec_val writes constant bytes here */
};
#pragma pack(pop)
#pragma options align = mac68k
struct DumpTextRecord {
    SInt16 kind;
    char *firstLabel;
    char *secondLabel;
    struct PARAM_T *firstInput;
    char *firstDetail;
    struct PARAM_T *secondInput;
    char *secondDetail;
};
#pragma options align = reset
#pragma pack(push, 2)
struct FloatParameterCallback {
    PARAM_T base; /* 0x00: dispatch_param_by_which dispatches the parameter prefix in Parameter.c */
    void (*handler)(void *ctx, float a, float b, float c); /* 0x0a: invoke_float_parameter_callback calls the handler */
    float arg; /* 0x0e: invoke_float_parameter_callback passes the stored callback argument */
};
#pragma pack(pop)
#pragma options align = packed
struct HANDLER_T {
    PARAM_T base;
    int (*func)(char *name, char *value);
};
#pragma options align = reset
#pragma pack(push, 1)
union MaskEntryBits {
    unsigned char b;  /* 0x00: apply_mask_entry and is_mask_entry_unchanged select when entry->width == 1 */
    unsigned short w; /* 0x00: apply_mask_entry and is_mask_entry_unchanged select when entry->width == 2 */
    unsigned long
        l; /* 0x00: apply_mask_entry and is_mask_entry_unchanged select when entry->width != 1 && entry->width != 2 */
};
struct MaskEntry {
    unsigned char unk[10];
    unsigned char width;       /* 0x0a: apply_mask_entry and is_mask_entry_unchanged select both bit-value variants */
    union MaskEntryBits value; /* 0x0b: apply_mask_entry ORs replacement bits into destination */
    union MaskEntryBits mask;  /* 0x0f: apply_mask_entry clears masked destination bits */
    unsigned char *addr;       /* 0x13: apply_mask_entry reads and writes destination bytes */
};
#pragma pack(pop)
#pragma pack(push, 1)
struct NumParm {
    signed char
        which; /* 0x00: Parameter_DispatchByWhich and dispatch_param_by_which select the numeric parameter handler through PARAM_T */
    char flags;           /* 0x01: parse_parameter_value reads the PARAM_T parameter flags */
    char *name;           /* 0x02: format_num_parm supplies the numeric value name */
    struct PARAM_T *next; /* 0x06: Parameter_CheckParameters follows the PARAM_T parameter chain */
    unsigned char size;   /* 0x0a: format_num_parm and fn_00429110 select numeric width */
    char clamp;           /* 0x0b: fn_00429110 permits clamping instead of rejecting out-of-range values */
    unsigned long lo;     /* 0x0c: format_num_parm and fn_00429110 minimum value */
    unsigned long hi;     /* 0x10: format_num_parm and fn_00429110 maximum value */
    unsigned char *var;   /* 0x14: format_num_parm reads and fn_00429110 writes numeric destination bytes */
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PARAM_Conditional {
    PARAM_T base;
    PARAM_T *iftrue;
    PARAM_T *unused;
    PARAM_T *iffalse;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct STRING_T {
    PARAM_T base;    /* 0x00: validate_id_arg reads base.which to select identifier characters */
    SInt16 maxlen;   /* 0x0a: copy_idparm_arg bounds the string copy and reports excessive length */
    Boolean pstring; /* 0x0c: copy_idparm_arg selects Pascal string conversion */
    char *str;       /* 0x0d: copy_idparm_arg copies the argument into this string */
};
#pragma pack(pop)
#pragma pack(push, 1)
struct SETSTRING_T {
    PARAM_T base; /* 0x00: set_string parameter descriptor */
    char *value;  /* 0x0a: set_string source string */
    char pstring; /* 0x0e: set_string selects Pascal versus C string copying */
    char *var;    /* 0x0f: set_string copies into this string buffer; compare_setstring_value reads it */
};
#pragma pack(pop)
#pragma pack(push, 1)
struct FILEPATH_T {
    PARAM_T base;
    char fflags;
    char *defaultstr;
    char *filename; /* 0x0f: set_filepath clears and copies the output path string */
    int maxlen;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct FTYPE_T {
    PARAM_T base;
    UInt32 *fileCode; /* 0x0a: set_file_code stores the four-character file code parsed from arg */
    Boolean iscreator;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct Setting {
    signed char which; /* 0x00: Parameter_DispatchByWhich selects the parameter-kind help handler */
    signed char flags; /* 0x01: format_setting_help tests required, absent and optional value flags */
    char *name;        /* 0x02: format_setting_help and get_setting_name_value read the parameter name */
    PARAM_T *next;     /* 0x06: PARAM_T parameter-chain prefix used by Parameter_CheckParameters */
    char pad0a[4];     /* 0x0a: no observed access in Parameter.c */
    char *valuename;   /* 0x0e: format_setting_help prints the value label */
    int value;         /* 0x12: get_setting_name_value returns the setting value */
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
extern void format_num_parm(NumParm *parm, char **name, char **help, char **value);
extern unsigned int fn_0042910d(void);
extern int fn_00429110(struct NumParm *record, char *cursor);
extern void format_filecode_option(FTYPE_T *opt, char **name, int *flags, char **value);
extern unsigned int fn_00429462(void);
extern int set_file_code(FTYPE_T *opt, char *arg);
extern void format_string_name_help_value(STRING_T *parm, char **name, char **help, char **value);
extern unsigned int fn_004295ca(void);
extern int copy_idparm_arg(struct STRING_T *parm, char *arg, int x);
extern void get_string_name_help_value(STRING_T *parm, char **name, char **help, char **value);
extern unsigned int fn_004296d1(void);
extern void get_option_val_count_state(register struct OnOff *p, register char **out_val, register SInt32 *out_count,
                                       register char **out_state);
extern unsigned int fn_004297bf(void);
extern int parse_on_off(OnOff *opt, char *arg, int flags);
extern void get_on_off_option_info(OnOff *opt, char **name, int *flags, char **value);
extern unsigned int fn_00429896(void);
extern int set_on_off(OnOff *opt, char *arg, int flags);
extern void get_filepath_name_flags_value(FILEPATH_T *opt, char **name, int *flags, int *value);
extern unsigned int fn_0042994d(void);
extern void zero_unsigned_int_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int is_mask_entry_unchanged(MaskEntry *p);
extern int apply_mask_entry(MaskEntry *p, int unused, int flags);
extern void clear_unsigned_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int is_dest_unchanged_by_val_xor(struct ConstRec *p);
extern int xor_const_dest(struct ConstRec *p);
extern void zero_unsigned_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int const_matches_dest(struct ConstRec *p);
extern Boolean store_constrec_val(ConstRec *rec, void *unused, UInt32 flags);
extern void zero_outputs(int a0, unsigned int *a1, unsigned int *a2, unsigned int *a3);
extern int compare_setstring_value(SETSTRING_T *parm);
extern int set_string(struct SETSTRING_T *arguments);
extern void get_setting_name_value(Setting *opt, char **name, int *value, int *flags);
extern unsigned int fn_0042a192(void);
extern void invoke_float_parameter_callback(FloatParameterCallback *rec, float a, float b);
extern void format_setting_help(Setting *opt, char **help, int *a, int *b);
extern unsigned int fn_0042a263(void);
extern int fn_0042a266(HANDLER_T *h, char *arg, int x);
extern void fn_0042a312(DumpTextRecord *record, char **firstOutput, char **secondOutput, int *status);
extern unsigned int get_unsigned_zero(void);
extern int evaluate_conditional_branch(struct PARAM_Conditional *expr, char *a, int b);
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
