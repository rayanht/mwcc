#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/Parameter.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "driver/Arguments.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/ClientGlue.h"
#include "driver/Help.h"
#include "driver/MacFileTypes.h"
#include "driver/MsDos.h"
#include "driver/Option.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/StringUtils.h"
#include "driver/Targets.h"
#include "driver/TextUtils.h"
#include "driver/Utils.h"
#include <string.h>

char option_parameter_text[4096];
static char *data_0054da38 = &option_parameter_text[0];
static char *parm_help_buffer = &option_parameter_text[1024];
static char *parm_format_buffer = &option_parameter_text[2048];

typedef enum ParamKind { PK_ZERO = 0, PK_ONE = 1, PK_FOUR = 4, PK_FIVE = 5 } ParamKind;

static inline ByteQuad *fileCodeStorage(FTYPE_T *opt)
{
    return (ByteQuad *)opt->fileCode;
}

static inline ByteQuad *fileCodeValue(unsigned char (*fileCode)[4])
{
    return (ByteQuad *)fileCode;
}

int return_true(void)
{
    return 1;
}

void fn_00428f61(unsigned int context, void **firstList, void **secondList, void **thirdList)
{
    *firstList = NULL;
    *secondList = NULL;
    *thirdList = NULL;
}

unsigned int return_unsigned_zero(void)
{
    return 0U;
}

void format_num_parm(NumParm *parm, char **name, char **help, char **value)
{
    unsigned long v;
    unsigned short w;
    *name = (char *)(v = 0);
    if (parm->name)
        *name = parm->name;
    if (parm->size == 1) {
        unsigned char c;
        if (!*name)
            *name = "byte";
        if (parm->var)
            c = *parm->var;
        v = c;
    } else if (parm->size == 2) {
        if (!*name)
            *name = "short";
        if (parm->var) {
            ((unsigned char *)&w)[0] = parm->var[0];
            ((unsigned char *)&w)[1] = parm->var[1];
        }
        v = w;
    } else if (parm->size == 4) {
        if (!*name)
            *name = "long";
        if (parm->var) {
            ((unsigned char *)&v)[0] = parm->var[0];
            ((unsigned char *)&v)[1] = parm->var[1];
            ((unsigned char *)&v)[2] = parm->var[2];
            ((unsigned char *)&v)[3] = parm->var[3];
        }
    }
    if (parm->size == 4) {
        if (parm->lo != parm->hi)
            sprintf(parm_help_buffer, "range 0x%x - 0x%x", parm->lo, parm->hi);
        *help = parm_help_buffer;
        v <= 0x10000 ? (void)sprintf(parm_format_buffer, "%u", v) : (void)sprintf(parm_format_buffer, "0x%x", v);
    } else {
        if (parm->lo != parm->hi)
            sprintf(parm_help_buffer, "range %u - %u", parm->lo, parm->hi);
        *help = parm_help_buffer;
        sprintf(parm_format_buffer, "%u", v);
    }
    *value = parm_format_buffer;
    if (!parm->var)
        *value = NULL;
    if (parm->lo == parm->hi)
        *help = NULL;
}

unsigned int fn_0042910d(void)
{
    return 0U;
}

#include <string.h>

int fn_00429110(NumParm *record, char *cursor)
{
    UInt32 value = 0;
    UInt32 minimum = 0;
    UInt32 maximum = 0;
    char *text = cursor;

    if (*cursor == '\0') {
        Parameter_ForwardVarArgs(5, "", cursor);
        return 0;
    }

    if ((*cursor == '0' && ((int (*)(char))to_lowercase)(cursor[1]) == 'x') || *cursor == '$') {
        if (*cursor == '$')
            cursor++;
        else
            cursor += 2;
        while (*cursor != '\0') {
            if (Utils_IsHexDigit(*cursor) == 0) {
                Parameter_ForwardVarArgs(5, "hexadecimal ", text);
                return 0;
            }
            if (value > 0x0fffffff) {
                Parameter_ForwardVarArgs(4, text);
                return 0;
            }
            value = (value << 4) |
                    (Utils_IsDigit(*cursor) ? (*cursor - '0') : (((int (*)(char))to_lowercase)(*cursor) - 0x57));
            cursor++;
        }
    } else if (*cursor == '0') {
        cursor++;
        while (*cursor != '\0') {
            if (*cursor < '0' || *cursor >= '8') {
                Parameter_ForwardVarArgs(5, "octal ", text);
                return 0;
            }
            if (value > 0x1fffffff) {
                Parameter_ForwardVarArgs(4, text);
                return 0;
            }
            value = (value << 3) | (*cursor - '0');
            cursor++;
        }
    } else {
        while (*cursor != '\0') {
            if (Utils_IsDigit(*cursor) == 0) {
                Parameter_ForwardVarArgs(5, "decimal ", text);
                return 0;
            }
            if (value > 0x19999999) {
                Parameter_ForwardVarArgs(4, text);
                return 0;
            }
            value = value * 10 + (*cursor - '0');
            cursor++;
        }
    }

    if (record->lo == record->hi) {
        if (record->size == 1) {
            minimum = 0;
            maximum = 0xff;
        } else if (record->size == 2) {
            minimum = 0;
            maximum = 0xffff;
        } else {
            minimum = 0;
            maximum = 0xffffffffUL;
        }
    } else {
        minimum = record->lo;
        maximum = record->hi;
    }

    if (record->clamp == 0) {
        if (value < minimum || value > maximum) {
            Parameter_ForwardVarArgs(6, value, minimum, maximum);
            return 0;
        }
    }

    if (value < minimum) {
        forward_stack_varargs(7, value, minimum, maximum, minimum);
        value = minimum;
    } else if (value > maximum) {
        forward_stack_varargs(7, value, minimum, maximum, maximum);
        value = maximum;
    }

    if (record->size == 1) {
        UInt8 byteValue = value;
        UInt8 *destination = record->var;
        *destination = byteValue;
    }
    if (record->size == 2) {
        union {
            UInt16 value;
            BytePair representation;
        } narrowedValue;
        BytePair destinationBytes, valueBytes;
        narrowedValue.value = value;
        destinationBytes.bytes[0] = ((BytePair *)record->var)->bytes[0];
        destinationBytes.bytes[1] = ((BytePair *)record->var)->bytes[1];
        valueBytes.bytes[0] = ((BytePair *)&narrowedValue)->bytes[0];
        valueBytes.bytes[1] = ((BytePair *)&narrowedValue)->bytes[1];
        destinationBytes = valueBytes;
        ((BytePair *)record->var)->bytes[0] = destinationBytes.bytes[0];
        ((BytePair *)record->var)->bytes[1] = destinationBytes.bytes[1];
    } else if (record->size == 4) {
        union {
            UInt32 value;
            ByteQuad representation;
        } storedValue;
        ByteQuad destinationBytes, valueBytes;
        storedValue.value = value;
        destinationBytes.bytes[0] = ((ByteQuad *)record->var)->bytes[0];
        destinationBytes.bytes[1] = ((ByteQuad *)record->var)->bytes[1];
        destinationBytes.bytes[2] = ((ByteQuad *)record->var)->bytes[2];
        destinationBytes.bytes[3] = ((ByteQuad *)record->var)->bytes[3];
        valueBytes.bytes[0] = ((ByteQuad *)&storedValue)->bytes[0];
        valueBytes.bytes[1] = ((ByteQuad *)&storedValue)->bytes[1];
        valueBytes.bytes[2] = ((ByteQuad *)&storedValue)->bytes[2];
        valueBytes.bytes[3] = ((ByteQuad *)&storedValue)->bytes[3];
        destinationBytes = valueBytes;
        ((ByteQuad *)record->var)->bytes[0] = destinationBytes.bytes[0];
        ((ByteQuad *)record->var)->bytes[1] = destinationBytes.bytes[1];
        ((ByteQuad *)record->var)->bytes[2] = destinationBytes.bytes[2];
        ((ByteQuad *)record->var)->bytes[3] = destinationBytes.bytes[3];
    }

    return 1;
}

void format_filecode_option(FTYPE_T *opt, char **name, int *flags, char **value)
{
    if (opt->base.myname)
        *name = opt->base.myname;
    else if (opt->iscreator)
        *name = "creator";
    else
        *name = "type";
    if (!opt->fileCode)
        *value = NULL;
    else {
        char *p = parm_format_buffer;
        unsigned int v = *opt->fileCode;
        int i = 0;
        *p++ = '\'';
        for (; i < 4; i++) {
            *p++ = v >> 24;
            v <<= 8;
        }
        *p++ = '\'';
        *p = 0;
        *value = parm_format_buffer;
    }
}

unsigned int fn_00429462(void)
{
    return 0U;
}

int set_file_code(FTYPE_T *opt, char *arg)
{
    ByteQuad fileCode = {' ', ' ', ' ', ' '};
    ByteQuad current, value;
    int length = 0;
    char *originalArg = arg;

    while (*arg && length < 4) {
        length++;
        fileCode.bytes[4 - length] = *arg;
        arg++;
    }
    if (*arg) {
        Parameter_ForwardVarArgs(8, originalArg);
        return 0;
    }
    current.bytes[0] = fileCodeStorage(opt)->bytes[0];
    current.bytes[1] = fileCodeStorage(opt)->bytes[1];
    current.bytes[2] = fileCodeStorage(opt)->bytes[2];
    current.bytes[3] = fileCodeStorage(opt)->bytes[3];
    value.bytes[0] = fileCodeValue(&fileCode.bytes)->bytes[0];
    value.bytes[1] = fileCodeValue(&fileCode.bytes)->bytes[1];
    value.bytes[2] = fileCodeValue(&fileCode.bytes)->bytes[2];
    value.bytes[3] = fileCodeValue(&fileCode.bytes)->bytes[3];
    current = value;
    fileCodeStorage(opt)->bytes[0] = current.bytes[0];
    fileCodeStorage(opt)->bytes[1] = current.bytes[1];
    fileCodeStorage(opt)->bytes[2] = current.bytes[2];
    fileCodeStorage(opt)->bytes[3] = current.bytes[3];
    return 1;
}

void format_string_name_help_value(STRING_T *parm, char **name, char **help, char **value)
{
    if (parm->base.myname)
        *name = parm->base.myname;
    else
        *name = "string";
    if (!parm->str) {
        *help = NULL;
        *value = NULL;
    } else {
        sprintf(parm_help_buffer, "maximum length %d chars", parm->maxlen - 1);
        *help = parm_help_buffer;
        if (*parm->str) {
            sprintf(parm_format_buffer, "'%s'", parm->str);
            *value = parm_format_buffer;
        } else
            *value = "none";
    }
}

unsigned int fn_004295ca(void)
{
    return 0U;
}

int copy_idparm_arg(STRING_T *parm, char *arg, int x)
{
    int len = strlen(arg);
    strncpy(parm->str, arg, parm->maxlen - 1);
    if (parm->pstring)
        CLIO_ConvertToPascalString(parm->str);
    if (len > parm->maxlen) {
        Parameter_ForwardVarArgs(9, arg, arg + len - 5, parm->maxlen - 1);
        return 0;
    }
    return 1;
}

void get_string_name_help_value(STRING_T *parm, char **name, char **help, char **value)
{
    if (parm->base.myname)
        *name = parm->base.myname;
    else if (parm->base.which == 5)
        *name = "identifier";
    else
        *name = "symbol";
    if (!parm->str) {
        *value = NULL;
        *help = NULL;
    } else {
        sprintf(parm_help_buffer, "maximum length %d chars", parm->maxlen - 1);
        *help = parm_help_buffer;
        if (*parm->str) {
            sprintf(parm_format_buffer, "'%s'", parm->str);
            *value = parm_format_buffer;
        } else
            *value = "none";
    }
}

unsigned int fn_004296d1(void)
{
    return 0U;
}

int validate_id_arg(STRING_T *parm, char *arg, int x)
{
    char *s;
    const char *extra;
    if (copy_idparm_arg(parm, arg, x)) {
        if (parm->base.which == 5)
            extra = "$_";
        else if (parm->base.which == 6)
            extra = "_.$@?#";
        for (s = arg; *s; s++) {
            if (s == arg && Utils_IsDigit(*s))
                forward_stack_varargs(10, arg);
            if (!Utils_IsAlnum(*s) && !strchr(extra, *s))
                forward_stack_varargs(11, arg, *s);
        }
        return 1;
    }
    return 0;
}

void get_option_val_count_state(register OnOff *p, register char **out_val, register SInt32 *out_count,
                                register char **out_state)
{
    char cc;
    if (p->name != NULL)
        *out_val = p->name;
    else
        *out_val = "on|off";
    *out_count = 0;
    if (p->var == NULL) {
        *out_state = NULL;
    } else {
        if (p->var != NULL)
            cc = *p->var;
        if (cc != 0)
            *out_state = "on";
        else
            *out_state = "off";
    }
}

unsigned int fn_004297bf(void)
{
    return 0U;
}

int parse_on_off(OnOff *opt, char *arg, int flags)
{
    unsigned char on = (flags & 8) == 0;
    if (!ClientGlue_CompareLowercaseStrings(arg, "on"))
        *opt->var = on;
    else if (!ClientGlue_CompareLowercaseStrings(arg, "off"))
        *opt->var = !on;
    else {
        Parameter_ForwardVarArgs(12, arg);
        return 0;
    }
    return 1;
}

void get_on_off_option_info(OnOff *opt, char **name, int *flags, char **value)
{
    if (opt->name)
        *name = opt->name;
    else
        *name = "off|on";
    *flags = 0;
    if (!opt->var)
        *value = NULL;
    else {
        char c;
        if (opt->var)
            c = *opt->var;
        if (!c)
            *value = "on";
        else
            *value = "off";
    }
}

unsigned int fn_00429896(void)
{
    return 0U;
}

int set_on_off(OnOff *opt, char *arg, int flags)
{
    unsigned char on = (flags & 8) == 0;
    if (!ClientGlue_CompareLowercaseStrings(arg, "off"))
        *opt->var = on;
    else if (!ClientGlue_CompareLowercaseStrings(arg, "on"))
        *opt->var = !on;
    else {
        Parameter_ForwardVarArgs(12, arg);
        return 0;
    }
    return 1;
}

void get_filepath_name_flags_value(FILEPATH_T *opt, char **name, int *flags, int *value)
{
    char *unused = parm_help_buffer;
    if (opt->base.myname)
        *name = opt->base.myname;
    else
        *name = "filepath";
    *flags = 0;
    *value = (int)opt->defaultstr;
}

unsigned int fn_0042994d(void)
{
    return 0U;
}

int set_filepath(FILEPATH_T *parm, char *arg)
{
    OSSpec spec;
    char path[0x104];
    SInt32 err;
    int len;
    if (!*arg) {
        *parm->filename = 0;
        return 1;
    }
    if ((err = OS_MakeFileSpec(arg, &spec)) != 0) {
        Parameter_ForwardVarArgs(0x12, arg, OS_GetErrText(err));
        return 0;
    }
    OS_SpecToString(&spec, path, 0x104);
    if (parm->fflags & 1) {
        c2pstrcpy((unsigned char *)parm->filename, path);
        return 1;
    }
    if ((len = strlen(path)) > parm->maxlen) {
        Parameter_ForwardVarArgs(0xd, path + len - 32, path);
        return 0;
    }
    strcpy(parm->filename, path);
    return 1;
}

void zero_unsigned_int_outputs(int unused, unsigned int *firstOutput, unsigned int *secondOutput,
                               unsigned int *thirdOutput)
{
    *secondOutput = 0U;
    *firstOutput = 0U;
    *thirdOutput = 0U;
}

int is_mask_entry_unchanged(MaskEntry *entry)
{
    if (entry->width == 1) {
        unsigned char value = entry->value.b;
        unsigned char mask = entry->mask.b;
        unsigned char current;
        if (entry->addr)
            current = *entry->addr;
        return current == ((current & ~mask) | value);
    } else if (entry->width == 2) {
        unsigned short value = entry->value.w;
        unsigned short mask = entry->mask.w;
        union {
            unsigned short word;
            unsigned char bytes[2];
        } current;
        if (entry->addr) {
            ((BytePair *)current.bytes)->bytes[0] = entry->addr[0];
            ((BytePair *)current.bytes)->bytes[1] = entry->addr[1];
        }
        return current.word == ((current.word & ~mask) | value);
    } else {
        unsigned long value = entry->value.l;
        unsigned long mask = entry->mask.l;
        union {
            unsigned long word;
            unsigned char bytes[4];
        } current;
        if (entry->addr) {
            ((ByteQuad *)current.bytes)->bytes[0] = entry->addr[0];
            ((ByteQuad *)current.bytes)->bytes[1] = entry->addr[1];
            ((ByteQuad *)current.bytes)->bytes[2] = entry->addr[2];
            ((ByteQuad *)current.bytes)->bytes[3] = entry->addr[3];
        }
        return current.word == ((~mask & current.word) | value);
    }
}

int apply_mask_entry(MaskEntry *entry, int unused, int flags)
{
    typedef union {
        unsigned short w;
        unsigned char b[2];
    } WordBytes;
    typedef union {
        unsigned long l;
        unsigned char b[4];
    } LongBytes;
    if (entry->width == 1) {
        unsigned char value = entry->value.b;
        unsigned char mask = entry->mask.b;
        if (flags & 8) {
            unsigned char oldMask = mask;
            mask |= value;
            value = oldMask;
        }
        *entry->addr = (*entry->addr & ~mask) | value;
    } else if (entry->width == 2) {
        unsigned short value = entry->value.w;
        unsigned short mask = entry->mask.w;
        WordBytes current, maskBytes, valueBytes;
        if (flags & 8) {
            unsigned short oldMask = mask;
            mask |= value;
            value = oldMask;
        }
        current.b[0] = entry->addr[0];
        current.b[1] = entry->addr[1];
        maskBytes.b[0] = ((WordBytes *)&mask)->b[0];
        maskBytes.b[1] = ((WordBytes *)&mask)->b[1];
        valueBytes.b[0] = ((WordBytes *)&value)->b[0];
        valueBytes.b[1] = ((WordBytes *)&value)->b[1];
        current.w = (~maskBytes.w & current.w) | valueBytes.w;
        entry->addr[0] = current.b[0];
        entry->addr[1] = current.b[1];
    } else {
        unsigned long value = entry->value.l;
        unsigned long mask = entry->mask.l;
        LongBytes current, maskBytes, valueBytes;
        if (flags & 8) {
            unsigned long oldMask = mask;
            mask |= value;
            value = oldMask;
        }
        current.b[0] = entry->addr[0];
        current.b[1] = entry->addr[1];
        current.b[2] = entry->addr[2];
        current.b[3] = entry->addr[3];
        maskBytes.b[0] = ((LongBytes *)&mask)->b[0];
        maskBytes.b[1] = ((LongBytes *)&mask)->b[1];
        maskBytes.b[2] = ((LongBytes *)&mask)->b[2];
        maskBytes.b[3] = ((LongBytes *)&mask)->b[3];
        valueBytes.b[0] = ((LongBytes *)&value)->b[0];
        valueBytes.b[1] = ((LongBytes *)&value)->b[1];
        valueBytes.b[2] = ((LongBytes *)&value)->b[2];
        valueBytes.b[3] = ((LongBytes *)&value)->b[3];
        current.l = (~maskBytes.l & current.l) | valueBytes.l;
        entry->addr[0] = current.b[0];
        entry->addr[1] = current.b[1];
        entry->addr[2] = current.b[2];
        entry->addr[3] = current.b[3];
    }
    return 1;
}

void clear_unsigned_outputs(int unused, unsigned int *firstOutput, unsigned int *secondOutput,
                            unsigned int *thirdOutput)
{
    *secondOutput = 0U;
    *firstOutput = 0U;
    *thirdOutput = 0U;
}

int is_dest_unchanged_by_val_xor(ConstRec *parameter)
{
    struct ValueBytes {
        unsigned char low;
        unsigned char high;
    };
    if (parameter->kind == 1) {
        unsigned char mask = parameter->val.b;
        unsigned char current;
        if (parameter->dest)
            current = *parameter->dest;
        return current == (current ^ mask);
    } else if (parameter->kind == 2) {
        unsigned short mask = parameter->val.w;
        unsigned short current;
        if (parameter->dest) {
            ((struct ValueBytes *)&current)->low = parameter->dest[0];
            ((struct ValueBytes *)&current)->high = parameter->dest[1];
        }
        return current == (current ^ mask);
    } else {
        unsigned long mask = parameter->val.dw;
        unsigned long current;
        if (parameter->dest) {
            ((struct ValueBytes *)&current)->low = parameter->dest[0];
            ((struct ValueBytes *)&current)->high = parameter->dest[1];
        }
        return current == (current ^ mask);
    }
}

int xor_const_dest(ConstRec *constant)
{
    if (constant->kind == 1) {
        UInt8 mask = (UInt8)constant->val.w;
        constant->dest[0] ^= mask;
    } else if (constant->kind == 2) {
        typedef union {
            SInt16 word;
            UInt8 bytes[2];
        } WordBytes;
        SInt16 value = constant->val.w;
        WordBytes destination, mask;
        destination.bytes[0] = constant->dest[0];
        destination.bytes[1] = constant->dest[1];
        mask.bytes[0] = ((WordBytes *)&value)->bytes[0];
        mask.bytes[1] = ((WordBytes *)&value)->bytes[1];
        destination.word ^= mask.word;
        constant->dest[0] = destination.bytes[0];
        constant->dest[1] = destination.bytes[1];
    } else {
        typedef union {
            SInt32 word;
            UInt8 bytes[4];
        } LongBytes;
        LongBytes destination, mask;
        destination.bytes[0] = constant->dest[0];
        destination.bytes[1] = constant->dest[1];
        destination.bytes[2] = constant->dest[2];
        destination.bytes[3] = constant->dest[3];
        mask.bytes[0] = ((LongBytes *)&constant->val)->bytes[0];
        mask.bytes[1] = ((LongBytes *)&constant->val)->bytes[1];
        mask.bytes[2] = ((LongBytes *)&constant->val)->bytes[2];
        mask.bytes[3] = ((LongBytes *)&constant->val)->bytes[3];
        destination.word ^= mask.word;
        constant->dest[0] = destination.bytes[0];
        constant->dest[1] = destination.bytes[1];
        constant->dest[2] = destination.bytes[2];
        constant->dest[3] = destination.bytes[3];
    }
    return 1;
}

void zero_unsigned_outputs(int mode, unsigned int *firstOutput, unsigned int *secondOutput, unsigned int *thirdOutput)
{
    *secondOutput = 0U;
    *firstOutput = 0U;
    *thirdOutput = 0U;
}

int const_matches_dest(ConstRec *constant)
{
    if (constant->kind == 1) {
        unsigned char value = constant->val.b;
        unsigned char current;
        if (constant->dest)
            current = *constant->dest;
        return value == current;
    } else if (constant->kind == 2) {
        unsigned short value = constant->val.w;
        struct WordBytes {
            unsigned char low, high;
        };
        unsigned short current;
        if (constant->dest) {
            ((struct WordBytes *)&current)->low = constant->dest[0];
            ((struct WordBytes *)&current)->high = constant->dest[1];
        }
        return value == current;
    } else {
        unsigned long value = constant->val.dw;
        struct LongBytes {
            unsigned char first, second, third, fourth;
        };
        unsigned long current;
        if (constant->dest) {
            ((struct LongBytes *)&current)->first = constant->dest[0];
            ((struct LongBytes *)&current)->second = constant->dest[1];
            ((struct LongBytes *)&current)->third = constant->dest[2];
            ((struct LongBytes *)&current)->fourth = constant->dest[3];
        }
        return value == current;
    }
}

Boolean store_constrec_val(ConstRec *rec, void *unused, UInt32 flags)
{
    if (rec->kind == 1) {
        UInt8 value = rec->val.b;
        if ((flags & 8) && value <= 1)
            value = !value;
        rec->dest[0] = value;
    } else if (rec->kind == 2) {
        struct WordBytes {
            UInt8 low, high;
        };
        union WordValue {
            UInt8 bytes[2];
            UInt16 word;
        };
        union WordValue value, output, temporary;
        value.word = rec->val.w;
        if ((flags & 8) && value.word <= 1)
            value.word = !value.word;
        output.bytes[0] = rec->dest[0];
        output.bytes[1] = rec->dest[1];
        temporary.bytes[0] = ((struct WordBytes *)&value)->low;
        temporary.bytes[1] = ((struct WordBytes *)&value)->high;
        output.word = temporary.word;
        rec->dest[0] = output.bytes[0];
        rec->dest[1] = output.bytes[1];
    } else {
        struct LongBytes {
            UInt8 first, second, third, fourth;
        };
        union LongValue {
            UInt8 bytes[4];
            UInt32 word;
        };
        union LongValue value, output, temporary;
        value.word = rec->val.dw;
        if ((flags & 8) && value.word <= 1)
            value.word = !value.word;
        output.bytes[0] = rec->dest[0];
        output.bytes[1] = rec->dest[1];
        output.bytes[2] = rec->dest[2];
        output.bytes[3] = rec->dest[3];
        temporary.bytes[0] = ((struct LongBytes *)&value)->first;
        temporary.bytes[1] = ((struct LongBytes *)&value)->second;
        temporary.bytes[2] = ((struct LongBytes *)&value)->third;
        temporary.bytes[3] = ((struct LongBytes *)&value)->fourth;
        output.word = temporary.word;
        rec->dest[0] = output.bytes[0];
        rec->dest[1] = output.bytes[1];
        rec->dest[2] = output.bytes[2];
        rec->dest[3] = output.bytes[3];
    }
    return 1;
}

void zero_outputs(int selector, unsigned int *firstOutput, unsigned int *secondOutput, unsigned int *thirdOutput)
{
    *secondOutput = 0U;
    *firstOutput = 0U;
    *thirdOutput = 0U;
}

int compare_setstring_value(SETSTRING_T *parm)
{
    if (parm->pstring)
        return !pstrcmp((unsigned char *)parm->var, parm->value);
    else
        return !strcmp((char *)parm->var, parm->value);
}

int set_string(SETSTRING_T *arguments)
{
    SETSTRING_T *parameter = arguments;

    if (parameter->pstring) {
        memcpy(parameter->var, parameter->value, *parameter->value);
    } else {
        strcpy((char *)parameter->var, parameter->value);
    }
    return 1;
}

void get_setting_name_value(Setting *opt, char **name, int *value, int *flags)
{
    if (opt->name)
        *name = opt->name;
    else if ((opt->flags & 3) != 1)
        *name = "xxx";
    else
        *name = NULL;
    *value = opt->value;
    *flags = 0;
}

unsigned int fn_0042a192(void)
{
    return 0U;
}

void invoke_float_parameter_callback(FloatParameterCallback *rec, float a, float b)
{
    rec->handler(&option_name, rec->arg, a, b);
}

void format_setting_help(Setting *opt, char **help, int *a, int *b)
{
    char *p = data_0054da38;
    p += sprintf(p, "%s", opt->name ? opt->name : "var");
    if ((opt->flags & 3) != 1)
        p += sprintf(p, "%s=%s%s", (opt->flags & 2) ? "[" : "", opt->valuename ? opt->valuename : "...",
                     (opt->flags & 2) ? "]" : "");
    *help = data_0054da38;
    *a = 0;
    *b = 0;
}

unsigned int fn_0042a263(void)
{
    return 0U;
}

int fn_0042a266(HANDLER_T *h, char *arg, int x)
{
    char buf[0x100];
    char *value;
    short *tok;
    Boolean hasvalue = 0;
    if (!arg) {
        Parameter_ForwardVarArgs(0x28);
        return 0;
    }
    strncpy(buf, arg, 0x100);
    tok = (short *)fn_0040f969();
    if (tok && *tok == 4) {
        Targets_AdvanceArgument();
        hasvalue = 1;
        if (!parse_parameter_value(&h->base, &value, x))
            return 0;
    } else
        value = NULL;
    if (!value && hasvalue)
        value = "";
    return h->func(buf, value);
}

void fn_0042a312(DumpTextRecord *record, char **firstOutput, char **secondOutput, int *status)
{
    char *firstText = NULL;
    int secondText = 0;
    int firstBody = 0;
    int secondBody = 0;
    int firstSuffix = 0;
    int secondSuffix = 0;
    char firstBuffer[1024];
    char secondBuffer[1024];
    char *firstCursor = firstBuffer;
    char *secondCursor = secondBuffer;
    int written;
    char *lineEnd;

    *firstCursor = 0;
    *secondCursor = 0;
    if (record->firstInput != NULL)
        Parameter_DispatchByWhich(record->firstInput, (int *)&firstText, &firstBody, &firstSuffix);
    if (record->secondInput != NULL)
        Parameter_DispatchByWhich(record->secondInput, &secondText, &secondBody, &secondSuffix);

    if (record->firstDetail != NULL && record->secondDetail != NULL) {
        if (firstSuffix != 0) {
            written = sprintf(secondCursor, "%s (default is %s), else %s", record->firstDetail, firstSuffix,
                              record->secondDetail);
            secondCursor += written;
        } else {
            written = sprintf(secondCursor, "%s, else %s", record->firstDetail, record->secondDetail);
            secondCursor += written;
        }
    } else if (record->firstDetail != NULL) {
        secondCursor += sprintf(secondCursor, "%s", record->firstDetail);
        if (firstSuffix != 0)
            secondCursor += sprintf(secondCursor, "; default is %s", firstSuffix);
    } else if (record->secondDetail != NULL) {
        secondCursor += sprintf(secondCursor, "nothing, else %s", record->secondDetail);
    }

    if (firstText != NULL) {
        if (record->firstLabel != NULL) {
            written = sprintf(firstCursor, "[%s]", record->firstLabel ? record->firstLabel : "param");
            firstCursor += written;
        } else if (firstBody != 0) {
            lineEnd = strchr(firstText, '\n');
            if (lineEnd == NULL)
                lineEnd = firstText + strlen(firstText);
            written = sprintf(firstCursor, "[%.*s]", lineEnd - firstText, firstText);
            firstCursor += written;
        } else {
            written = sprintf(firstCursor, "[%s]", firstText);
            firstCursor += written;
            firstText = NULL;
        }
    }

    if (firstBody != 0 || secondBody != 0) {
        written = sprintf(firstCursor, "\n\t");
        firstCursor += written;
        written = sprintf(secondCursor, "\n\t");
        secondCursor += written;
        if (firstBody != 0) {
            if (firstText != NULL) {
                written = sprintf(firstCursor, "%s", firstText);
                firstCursor += written;
            }
            if (firstBody != 0) {
                written = sprintf(secondCursor, "%s", firstBody);
                secondCursor += written;
            }
            if (secondBody != 0) {
                written = sprintf(firstCursor, "\b\t");
                firstCursor += written;
                written = sprintf(secondCursor, "\b\t");
                secondCursor += written;
            }
        }

        if (secondBody != 0) {
            if (secondText != 0)
                written = sprintf(firstCursor, "%s", secondText);
            else
                written = sprintf(firstCursor, "(if blank)", secondText);
            firstCursor += written;
            if (secondBody != 0) {
                written = sprintf(secondCursor, "%s", secondBody);
                secondCursor += written;
            }
            written = sprintf(firstCursor, "\n");
            firstCursor += written;
            written = sprintf(secondCursor, "\n");
            secondCursor += written;
        }

        written = sprintf(firstCursor, "\b");
        firstCursor += written;
        written = sprintf(secondCursor, "\b");
        secondCursor += written;
    }

    if (firstCursor != firstBuffer) {
        strcpy(data_0054da38, firstBuffer);
        *firstOutput = data_0054da38;
    } else {
        *firstOutput = NULL;
    }
    if (secondCursor != secondBuffer) {
        strcpy(parm_help_buffer, secondBuffer);
        *secondOutput = parm_help_buffer;
    } else {
        *secondOutput = NULL;
    }
    *status = 0;
}

unsigned int get_unsigned_zero(void)
{
    return 0U;
}

int evaluate_conditional_branch(struct PARAM_Conditional *expr, char *a, int b)
{
    if (a)
        return dispatch_param_by_which(expr->iftrue, a, b);
    else if (expr->iffalse)
        return dispatch_param_by_which(expr->iffalse, a, b);
    return 1;
}

static int (*data_0054dbdc[16])(PARAM_T *, int, int) = {
    (int (*)(PARAM_T *, int, int))return_true,
    (int (*)(PARAM_T *, int, int))set_file_code,
    (int (*)(PARAM_T *, int, int))set_filepath,
    (int (*)(PARAM_T *, int, int))fn_00429110,
    (int (*)(PARAM_T *, int, int))copy_idparm_arg,
    (int (*)(PARAM_T *, int, int))validate_id_arg,
    (int (*)(PARAM_T *, int, int))validate_id_arg,
    (int (*)(PARAM_T *, int, int))parse_on_off,
    (int (*)(PARAM_T *, int, int))set_on_off,
    (int (*)(PARAM_T *, int, int))apply_mask_entry,
    (int (*)(PARAM_T *, int, int))xor_const_dest,
    (int (*)(PARAM_T *, int, int))store_constrec_val,
    (int (*)(PARAM_T *, int, int))set_string,
    (int (*)(PARAM_T *, int, int))invoke_float_parameter_callback,
    (int (*)(PARAM_T *, int, int))evaluate_conditional_branch,
    (int (*)(PARAM_T *, int, int))fn_0042a266,
};

int dispatch_param_by_which(PARAM_T *param, char *a, int b)
{
    int result = 0;
    if (!param)
        Targets_ForwardVarArgsAndLongjmp("PARAM_T is NULL");
    if (param->which >= 0 && param->which < 16)
        result = data_0054dbdc[param->which](param, (int)a, b);
    else {
        Targets_ForwardVarArgsAndLongjmp("Unhandled PARAM_T (%d)", param->which);
        result = 0;
    }
    return result;
}

static void (*data_0054dc44[16])(PARAM_T *, int *, int *, int *) = {
    (void (*)(PARAM_T *, int *, int *, int *))fn_00428f61,
    (void (*)(PARAM_T *, int *, int *, int *))format_filecode_option,
    (void (*)(PARAM_T *, int *, int *, int *))get_filepath_name_flags_value,
    (void (*)(PARAM_T *, int *, int *, int *))format_num_parm,
    (void (*)(PARAM_T *, int *, int *, int *))format_string_name_help_value,
    (void (*)(PARAM_T *, int *, int *, int *))get_string_name_help_value,
    (void (*)(PARAM_T *, int *, int *, int *))get_string_name_help_value,
    (void (*)(PARAM_T *, int *, int *, int *))get_option_val_count_state,
    (void (*)(PARAM_T *, int *, int *, int *))get_on_off_option_info,
    (void (*)(PARAM_T *, int *, int *, int *))zero_unsigned_int_outputs,
    (void (*)(PARAM_T *, int *, int *, int *))clear_unsigned_outputs,
    (void (*)(PARAM_T *, int *, int *, int *))zero_unsigned_outputs,
    (void (*)(PARAM_T *, int *, int *, int *))zero_outputs,
    (void (*)(PARAM_T *, int *, int *, int *))get_setting_name_value,
    (void (*)(PARAM_T *, int *, int *, int *))fn_0042a312,
    (void (*)(PARAM_T *, int *, int *, int *))format_setting_help,
};

void Parameter_DispatchByWhich(PARAM_T *param, int *a, int *b, int *c)
{
    *a = 0;
    *b = 0;
    *c = 0;
    if (!param)
        Targets_ForwardVarArgsAndLongjmp("PARAM_T is NULL");
    else if (param->which >= 0 && param->which < 16)
        data_0054dc44[param->which](param, a, b, c);
    else
        Targets_ForwardVarArgsAndLongjmp("Unhandled PARAM_T (%d)", param->which);
}

static int (*data_0054dc84[16])(PARAM_T *) = {
    (int (*)(PARAM_T *))return_unsigned_zero,
    (int (*)(PARAM_T *))fn_00429462,
    (int (*)(PARAM_T *))fn_0042994d,
    (int (*)(PARAM_T *))fn_0042910d,
    (int (*)(PARAM_T *))fn_004295ca,
    (int (*)(PARAM_T *))fn_004296d1,
    (int (*)(PARAM_T *))fn_004296d1,
    (int (*)(PARAM_T *))fn_004297bf,
    (int (*)(PARAM_T *))fn_00429896,
    (int (*)(PARAM_T *))is_mask_entry_unchanged,
    (int (*)(PARAM_T *))is_dest_unchanged_by_val_xor,
    (int (*)(PARAM_T *))const_matches_dest,
    (int (*)(PARAM_T *))compare_setstring_value,
    (int (*)(PARAM_T *))fn_0042a192,
    (int (*)(PARAM_T *))get_unsigned_zero,
    (int (*)(PARAM_T *))fn_0042a263,
};

unsigned char Parameter_DispatchParam(PARAM_T *param)
{
    if (!param)
        Targets_ForwardVarArgsAndLongjmp("PARAM_T is NULL");
    if (param->which >= 0 && param->which < 16)
        return data_0054dc84[param->which](param);
    Targets_ForwardVarArgsAndLongjmp("Unhandled PARAM_T (%d)", param->which);
    {
        int result = 0;
        return result;
    }
}

void push_argument_option(char *argument)
{
    Option_Push(4, argument, NULL);
}

void fn_0042a797(void)
{
    Option_PopStack(4);
}

Boolean is_non_text_file(char *name, Boolean flag)
{
    OSSpec buf;
    UInt32 seg;
    Boolean found;

    if (make_osspec_from_path(name, &buf, &found) == 0 && found != 0 && OS_Status(&buf) == 0 &&
        MacFileTypes_GetFileType(&buf, &seg) == 0 && seg != 0x54455854) {
        if (flag)
            forward_stack_varargs(0x4d, name);
        return 1;
    }
    return 0;
}

static inline void Parameter_EmitValue(char **value, int separator)
{
    fn_0040fbe1(data_005876fc, separator, NULL);
    push_argument_option(*value);
}

int parse_parameter_value(PARAM_T *parameter, char **value, UInt32 flags)
{
    int alternate;
    SInt16 terminator;
    SInt16 separator;
    SInt16 end;
    TokenText *token;

    alternate = (flags & 4) != 0;
    terminator = !alternate ? PK_ONE : PK_FIVE;
    separator = !alternate ? PK_FIVE : PK_FOUR;
    end = !alternate ? PK_ZERO : PK_ONE;

    token = (TokenText *)fn_0040f969();
    if (token == NULL)
        CLIO_ReportAssertionFailure("tok", "Parameter.c", 0x5bb);

    if ((parameter->flags & 3) == 0) {
        if (token->kind == separator || token->kind == 4)
            token = Targets_AdvanceArgument();
        if (token->kind == terminator || token->kind == separator || token->kind == end) {
            Parameter_ForwardVarArgs(0x22);
            return 0;
        }
        if (token->kind != 2) {
            Parameter_ForwardVarArgs(0x39, "parameter", Targets_GetTokenTextDescription(token));
            return 0;
        }
        Targets_CopyTokenText(token, option_parameter_text, 0x1000, 1);
        Targets_AdvanceArgument();
        *value = option_parameter_text;
        if ((parameter->flags & 8) != 0) {
            token = (TokenText *)fn_0040f969();
            if (token->kind == 4)
                token = Targets_AdvanceArgument();
        }
    } else if ((parameter->flags & 1) != 0) {
        *value = NULL;
        if (token->kind == 4 && !alternate) {
            if ((flags & 0x40) == 0) {
                Parameter_ForwardVarArgs(0x25);
                return 0;
            }
            token = Targets_AdvanceArgument();
        }
    } else if ((parameter->flags & 2) != 0) {
        if (token->kind == terminator || token->kind == end) {
            *value = NULL;
        } else if (token->kind == separator) {
            token = Targets_AdvanceArgument();
            if (token->kind == 2) {
                Targets_CopyTokenText(token, option_parameter_text, 0x1000, 1);
                if ((parameter->flags & 0x12) != 0 && is_non_text_file(option_parameter_text, !(flags & 1))) {
                    *value = NULL;
                } else {
                    Targets_AdvanceArgument();
                    *value = option_parameter_text;
                }
            } else {
                *value = NULL;
            }
        } else if (token->kind == 2 && *token->text == 0) {
            token = Targets_AdvanceArgument();
            if (token->kind == separator)
                token = Targets_AdvanceArgument();
            *value = NULL;
        } else if ((flags & 4) == 0 || token->kind == 4) {
            if (token->kind == 4)
                token = Targets_AdvanceArgument();
            if (token->kind == 2) {
                Targets_CopyTokenText(token, option_parameter_text, 0x1000, 1);
                if ((parameter->flags & 0x12) != 0 && is_non_text_file(option_parameter_text, !(flags & 1))) {
                    *value = NULL;
                } else {
                    Targets_AdvanceArgument();
                    *value = option_parameter_text;
                }
            } else {
                *value = NULL;
            }
        } else {
            *value = NULL;
        }
    } else {
        Targets_ForwardVarArgsAndLongjmp("Unknown parameter type");
    }

    if (pTool->tool == 'Comp' && (flags & 2) != 0 && *value != NULL) {
        if ((flags & 0x10) == 0) {
            if (alternate)
                Parameter_EmitValue(value, 4);
            else
                Parameter_EmitValue(value, 5);
        } else {
            push_argument_option(*value);
        }
        Option_FormatOStackToList(&data_005876fc);
        fn_0042a797();
    }
    return 1;
}

int Parameter_CheckParameters(PARAM_T *parameter, int incomingflags)
{
    PARAM_T *param;
    char *result;
    int optional;
    int flags;
    short paramflags;
    param = parameter;
    flags = incomingflags;
    optional = (flags & 4) != 0;
    flags |= 0x10;
    if (param) {
        while (param) {
            if (flags & 0x20)
                flags |= 0x40;
            if (!parse_parameter_value(param, &result, flags))
                return 0;
            if (!(flags & 1)) {
                paramflags = (param->flags & 4) ? 0x20 : 0;
                if (!dispatch_param_by_which(param, result, paramflags | flags))
                    return 0;
            } else if (param->flags & 4) {
                if (!parse_parameter_value(param, &result, flags | 4))
                    return 0;
            }
            if (result)
                flags &= ~0x10;
            flags &= ~0x40;
            param = param->next;
        }
    }
    return 1;
}

void Parameter_ForwardVarArgs(int messageId, ...)
{
    va_list arguments;
    int argumentSize;
    va_list base = (va_list)&messageId;
    va_list nextArgument = (va_list)&messageId;
    nextArgument += sizeof(short);
    argumentSize = nextArgument - base;
    argumentSize += 3;
    arguments = (va_list)&messageId + argumentSize / 4 * 4;
    Option_FormatMessageWithOptionContext(messageId, arguments);
}

void forward_stack_varargs(SInt32 errorId, ...)
{
    char *lastArgument = (char *)&errorId;
    char *arguments = (char *)&errorId;
    int argumentSize;
    arguments += 2;
    argumentSize = arguments - lastArgument;
    argumentSize += 3;
    argumentSize = argumentSize / 4 * 4;
    argumentSize += (unsigned int)&errorId;
    arguments = (char *)argumentSize;
    Option_ReportError(errorId, arguments);
}

void Parameter_InitHelpColumn(HelpColumn *buf, short left, short width)
{
    memset(buf, 0, sizeof(HelpColumn));
    buf->left = left;
    buf->width = width;
}
