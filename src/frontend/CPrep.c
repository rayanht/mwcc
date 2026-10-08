#define CERROR_FILE "CPrep.c"
#include "compiler/common.h"
#include "compiler/CPrep.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CBrowse.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CObjC.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CPreprocess.h"
#include "compiler/CodeGen.h"
#include "compiler/DWARF.h"
#include "compiler/FuncLevelAsmPPC.h"
#include "compiler/IrOptimizer.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/Registers.h"
#include "compiler/StackFrameEABI.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLPluginRequests.h"
#include "driver/COSToolsCLT.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Memory.h"
#include <time.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#pragma sym off

static Macro data_0054fd08 = {NULL, NULL, NULL, 0, 1, 0, {NULL}};
static Macro data_0054fd1c = {NULL, NULL, NULL, 0, 2, 0, {NULL}};
static Macro data_0054fd30 = {NULL, NULL, NULL, 0, 3, 0, {NULL}};
static Macro data_0054fd44 = {NULL, NULL, NULL, 0, 4, 0, {NULL}};
static Macro data_0054fd58 = {NULL, NULL, NULL, 0, 5, 0, {NULL}};
static Macro data_0054fd6c = {NULL, NULL, NULL, 0, 6, 0, {NULL}};
static Macro data_0054fd80 = {NULL, NULL, NULL, 0, 7, 0, {NULL}};
static Macro data_0054fd94 = {NULL, NULL, NULL, 0, 8, 0, {NULL}};
static Macro data_0054fda8 = {NULL, NULL, NULL, 0, 9, 0, {NULL}};
static Macro cplusplus_predefined_macro = {NULL, NULL, NULL, 0, 10, 0, {NULL}};
static Macro data_0054fdd0 = {NULL, NULL, NULL, 0, 11, 0, {NULL}};
static Macro data_0054fde4 = {NULL, NULL, NULL, 0, 12, 0, {NULL}};
static Macro data_0054fdf8 = {NULL, NULL, NULL, 0, 13, 0, {NULL}};
static Macro data_0054fe0c = {NULL, NULL, NULL, 0, 14, 0, {NULL}};
static Macro data_0054fe20 = {NULL, NULL, NULL, 0, 15, 0, {NULL}};
static Macro optionalNameMacro = {NULL, NULL, NULL, 0, 19, 0, {NULL}};
static Macro data_0054fe48 = {NULL, NULL, NULL, 0, 16, 0, {NULL}};
static Macro data_0054fe5c = {NULL, NULL, NULL, 0, 17, 0, {NULL}};
static Macro data_0054fe70 = {NULL, NULL, NULL, 0, 18, 0, {NULL}};
static Macro data_0054fe84 = {NULL, NULL, NULL, 0, 20, 0, {NULL}};
static Macro data_0054fe98 = {NULL, NULL, NULL, 0, 21, 0, {NULL}};
static Macro data_0054feac = {NULL, NULL, NULL, 0, 34, 0, {NULL}};
static Macro data_0054fec0 = {NULL, NULL, NULL, 0, 33, 0, {NULL}};
static Macro data_0054fed4 = {NULL, NULL, NULL, 0, 32, 0, {NULL}};

static OptionEntry pragma_options[] = {
    {"little_endian", 0x4000},
    {"longlong", 0x77},
    {"disable_registers", 0x8},
    {"fp_contract", 0x9},
    {"cats", 0x13},
    {"force_cats", 0x14},
    {"pool_data", 0x1F},
    {"use_lmw_stmw", 0x22},
    {"incompatible_return_small_structs", 0x4E},
    {"create_file_object", 0x4F},
    {"incompatible_sfpe_double_params", 0x50},
    {"debug_listing", 0x6},
    {"rsqrt", 0x51},
    {"k63d", 0x52},
    {"3dnow", 0x52},
    {"cplusplus", 0x5A},
    {"ecplusplus", 0x5B},
    {"objective_c", 0x5C},
    {"objc_strict", 0x5D},
    {"ARM_conform", 0x5E},
    {"ARM_scoping", 0x5F},
    {"require_prototypes", 0x60},
    {"trigraphs", 0x61},
    {"only_std_keywords", 0x62},
    {"enumsalwaysint", 0x63},
    {"ANSI_strict", 0x64},
    {"mpwc_relax", 0x65},
    {"mpwc_newline", 0x66},
    {"ignore_oldstyle", 0x67},
    {"cpp_extensions", 0x68},
    {"pointercast_lvalue", 0x69},
    {"RTTI", 0x6A},
    {"delete_exception", 0x6B},
    {"oldalignment", 0x6D},
    {"multibyteaware", 0x6F},
    {"unsigned_char", 0x6E},
    {"auto_inline", 0x70},
    {"defer_codegen", 0x71},
    {"direct_to_som", 0x72},
    {"SOMCheckEnvironment", 0x73},
    {"SOMCallOptimization", 0x74},
    {"bool", 0x75},
    {"old_enum_mangler", 0x76},
    {"longlong_enums", 0x78},
    {"no_tfuncinline", 0x79},
    {"flat_include", 0x7B},
    {"syspath_once", 0x7C},
    {"always_import", 0x7D},
    {"simple_class_byval", 0x7E},
    {"wchar_type", 0x7F},
    {"vbase_ctor_offset", 0x80},
    {"vbase_abi_v2", 0x81},
    {"def_inherited", 0x82},
    {"template_patch", 0x83},
    {"template_friends", 0x84},
    {"faster_pch_gen", 0x85},
    {"array_new_delete", 0x86},
    {"dollar_identifiers", 0x87},
    {"def_inline_tfuncs", 0x88},
    {"arg_dep_lookup", 0x89},
    {"simple_prepdump", 0x8A},
    {"line_prepdump", 0x8B},
    {"fullpath_prepdump", 0x8C},
    {"old_mtemplparser", 0x8D},
    {"suppress_init_code", 0x8E},
    {"reverse_bitfields", 0x8F},
    {"c9x", 0x90},
    {"float_constants", 0x91},
    {"no_static_dtors", 0x92},
    {"longlong_prepeval", 0x93},
    {"const_strings", 0x94},
    {"dumpir", 0x95},
    {"experimental", 0x96},
    {"gcc_extensions", 0x97},
    {"stdc_fp_contract", 0x98},
    {"stdc_fenv_access", 0x99},
    {"stdc_cx_limitedr", 0x9A},
    {"microsoft_exceptions", 0x9B},
    {"microsoft_RTTI", 0x9B},
    {"warning_errors", 0x9C},
    {"extended_errorcheck", 0x9D},
    {"check_header_flags", 0x9E},
    {"warn_illpragma", 0x9F},
    {"warn_emptydecl", 0xA0},
    {"warn_possunwant", 0xA1},
    {"warn_unusedvar", 0xA2},
    {"warn_unusedarg", 0xA3},
    {"warn_extracomma", 0xA4},
    {"warn_hidevirtual", 0xA5},
    {"warn_largeargs", 0xA6},
    {"warn_implicitconv", 0xA7},
    {"warn_notinlined", 0xA8},
    {"warn_structclass", 0xA9},
    {"warn_padding", 0xAA},
    {"warn_no_side_effect", 0xAB},
    {"warn_resultnotused", 0xAC},
    {"align_array_members", 0xAE},
    {"dont_reuse_strings", 0xAF},
    {"pool_strings", 0xB0},
    {"explicit_zero_data", 0xB1},
    {"readonly_strings", 0xB2},
    {"opt_common_subs", 0xC4},
    {"opt_loop_invariants", 0xC5},
    {"opt_propagation", 0xC6},
    {"opt_unroll_loops", 0xCD},
    {"opt_lifetimes", 0xCB},
    {"opt_strength_reduction", 0xC8},
    {"opt_strength_reduction_strict", 0xC9},
    {"opt_dead_code", 0xCA},
    {"opt_dead_assignments", 0xC7},
    {"opt_vectorize_loops", 0xCE},
    {"exceptions", 0xB3},
    {"dont_inline", 0xB5},
    {"always_inline", 0xB6},
    {"optimize_for_size", 0xC2},
    {"peephole", 0xB7},
    {"global_optimizer", 0xB8},
    {"side_effects", 0xB9},
    {"profile", 0xBA},
    {"internal", 0x20BB},
    {"import", 0x20BC},
    {"export", 0x20BD},
    {"lib_export", 0x20BE},
    {"nosyminline", 0xBF},
    {"force_active", 0xC0},
    {"sym", 0xD2},
    {NULL, 0x0},
};

#pragma options align = mac68k
static struct CPrepRec data_0057f6c8[64];
static SInt16 if_depth;
static struct CPrepFileInfo *data_0057f94a[32];
static struct PrepNameCacheEntry *data_0057f9ca;
static UInt32 next_scaled_ticks;
static UInt8 data_0057f9d2;
static UInt8 data_0057f9d3;
static SInt32 data_0057f9d4;
static SInt32 text_offset;
static UInt8 data_0057f9dc;
static UInt8 data_0057f9dd;
static UInt8 data_0057f9de;
static struct CPrep_0043afc0_Entry saved_structalignments[128];
static SInt16 data_0057fce0;
static struct PragmaNode *pragma_list;
static struct IROOptNode *saved_options;
static struct CompilerLinkerOptions *data_0057fcea;
static UInt8 data_0057fcee;
static GList macro_text;
static struct StorageHandle *buffered_token_storage;
static struct TStreamElement *buffered_tokens;
static SInt32 buffered_token_capacity;
static SInt32 data_0057fd0c;
static unsigned char file_cannot_opened_name[64];
static unsigned int total_heap_size;
static struct TStreamElement lastBufferedToken;
static short data_0057fd6c;
#pragma options align = reset

typedef void (*Callback)(TStreamElement *);

typedef enum { CPrep_DidPush, CPrep_DidPop } CPrep_DidFlag;

#define NAMEBUF ((char *)data_00587fa0->name)
#define SETLINE()                                                                                                      \
    do {                                                                                                               \
        data_00588470 = 1;                                                                                             \
        data_00588524 = 1;                                                                                             \
        data_00588523 = 1;                                                                                             \
    } while (0)
#define KEYWORD()                                                                                                      \
    if (memcmp("if", NAMEBUF, 3) == 0)                                                                                 \
        parse_if_directive();                                                                                          \
    else if (memcmp("ifdef", NAMEBUF, 6) == 0)                                                                         \
        parse_ifdef_directive();                                                                                       \
    else if (memcmp("ifndef", NAMEBUF, 7) == 0)                                                                        \
        parse_ifndef();                                                                                                \
    else if (memcmp("elif", NAMEBUF, 5) == 0)                                                                          \
        parse_elif_directive();                                                                                        \
    else if (memcmp("else", NAMEBUF, 5) == 0)                                                                          \
        parse_else_directive();                                                                                        \
    else if (memcmp("endif", NAMEBUF, 6) == 0)                                                                         \
        parse_endif_directive();                                                                                       \
    else {                                                                                                             \
        CPrepTokenizer_SkipToEndOfLine();                                                                              \
        continue;                                                                                                      \
    }

#define REGN(name_, macro_)                                                                                            \
    do {                                                                                                               \
        HashNameNode *name = GetHashNameNode((name_));                                                                 \
        (macro_).name = name;                                                                                          \
        (macro_).next = macro_buckets[name->hashval];                                                                  \
        macro_buckets[(macro_).name->hashval] = &(macro_);                                                             \
        (macro_).isExpanding = 0;                                                                                      \
    } while (0)

#define CPrep_ERROR(line)                                                                                              \
    do {                                                                                                               \
        UInt8 saved_ = data_005884fd;                                                                                  \
        data_005884fd = 0;                                                                                             \
        data_0057f9dc = 1;                                                                                             \
        CError_ReportError(line);                                                                                      \
        data_005884fd = saved_;                                                                                        \
    } while (0)
#define PN ((char *)data_00587fa0->name)
#define EMIT(fn, line)                                                                                                 \
    do {                                                                                                               \
        UInt8 saved = data_005884fd;                                                                                   \
        data_005884fd = 0;                                                                                             \
        data_0057f9dc = 1;                                                                                             \
        fn(line);                                                                                                      \
        data_005884fd = saved;                                                                                         \
    } while (0)
#define PREP_ERR(line)                                                                                                 \
    do {                                                                                                               \
        UInt8 saved = data_005884fd;                                                                                   \
        data_005884fd = 0;                                                                                             \
        data_0057f9dc = 1;                                                                                             \
        CError_Warning(line);                                                                                          \
        data_005884fd = saved;                                                                                         \
    } while (0)

static void LogName(const unsigned char *name);
static void pop_macro_expansion_state(void);
static void pop_macro_state(void);
static void CPrep_PushState(void *obj);
static void CPrep_Fatal(void);
static void CPrep_PopState(void);
static void CPrep_0043afc0_error(SInt32 code);
static void CPrep_ErrorBA(void);
static void CPrep_Error_439930(int line);
static Boolean CNameRef_IsNull(CNameRef *p);
static void CPrep_Push(SInt16 type);
static void CPrep_AddLine(SInt16 kind);
static SInt32 calc_line(CPrepFileInfo *p);
static void report_error(SInt16 code);
static SInt32 check_time(void);
static void CPrep_Error(SInt16 code);

static inline void process_newline(void)
{
    if (macro_expansion_depth == 0) {
        if (data_0058850f != 0)
            fn_004d6ed0();
        data_00587ef0++;
        line_count++;
        if (current_file_index <= 0)
            text_offset = calc_line(data_0057f94a[0]);
    }
    SETLINE();
    if (COS_GetTicks() > next_scaled_ticks) {
        if (check_time() != 0)
            CError_Longjmp();
        next_scaled_ticks = COS_GetTicks() + 5;
    }
}

static inline Type *CPrep_IntegerExpressionType(const CPrepValue *value)
{
    if (copts.longlong_prepeval != 0 || copts.c9x != 0)
        return value->isUnsigned ? (Type *)&stunsignedlonglong : (Type *)&stsignedlonglong;
    return value->isUnsigned ? (Type *)&stunsignedlong : (Type *)&stsignedlong;
}

static inline Type *CPrep_IntegerType(const CPrepValue *value)
{
    if (copts.longlong_prepeval || copts.c9x)
        return value->isUnsigned ? (Type *)&stunsignedlonglong : (Type *)&stsignedlonglong;
    else
        return value->isUnsigned ? (Type *)&stunsignedlong : (Type *)&stsignedlong;
}

#pragma sym reset

static inline void CPrep_ParseListingOption(void)
{
    UInt8 saved;
    if (CPrepTokenizer_ScanToken() == -3) {
        if (memcmp(PN, "on", 3) == 0) {
            data_0057f9d3 = 1;
            return;
        }
        if (memcmp(PN, "off", 4) == 0) {
            data_0057f9d3 = 0;
            return;
        }
    }
    if (copts.warn_illpragma) {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = saved;
    }
}

#pragma sym off

static inline void warn_structalignment(void)
{
    if (copts.warn_illpragma) {
        CPrep_0043afc0_error(0xba);
    }
}

static inline void restorePrepSetting(void)
{
    struct IROOptNode *node = saved_options;
    while (node != NULL) {
        if (node->flag != 0 && node->code == 0xd1) {
            copts.fd1 = node->value;
            node->flag = 0;
            return;
        }
        node = node->next;
    }
    copts.fd1 = data_0057fcea->fd1;
}

static inline void ApplyUnrollOption(void)
{
    IROOptNode *option = saved_options;
    while (option != NULL) {
        if (option->flag != 0 && option->code == 0xd0) {
            copts.unrollOption = option->value;
            option->flag = 0;
            return;
        }
        option = option->next;
    }
    copts.unrollOption = data_0057fcea->unrollOption;
}

static inline void CPrep_RestoreOptimizationLevel(void)
{
    IROOptNode *p = saved_options;
    while (p != NULL) {
        if (p->flag != 0 && p->code == 0xc1) {
            copts.deleteDeadInstructions = p->value;
            p->flag = 0;
            return;
        }
        p = p->next;
    }
    copts.deleteDeadInstructions = data_0057fcea->deleteDeadInstructions;
}

static inline void CPrep_0043bf00_inline1(unsigned int v1)
{
    IROOptNode *v14;
    v14 = saved_options;
    while ((int)v14 != 0) {
        if (v14->flag != 0 && v14->code == (int)v1) {
            ((UInt8 *)&copts)[(int)v1] = v14->value;
            v14->flag = 0;
            return;
        }
        v14 = v14->next;
    }
    ((UInt8 *)&copts)[(int)v1] = ((UInt8 *)data_0057fcea)[v1];
    return;
}

static inline UInt8 *CPrep_OptionAddress(unsigned int index)
{
    return (UInt8 *)&copts + index;
}

static inline Macro **macro_bucket_link(HashNameNode *name)
{
    return &macro_buckets[name->hashval];
}

/* strcmp */
/* memcpy */

static inline void remap_and_report_error(SInt16 code)
{
    Boolean save = data_005884fd;
    data_005884fd = 0;
    if (code == 0x66 && (macro_expansion_depth > 0 || currentTextPosition < textend))
        code = 0x69;
    data_0057f9dc = 1;
    CError_ReportError(code);
    data_005884fd = save;
}

static inline void CPrep_ErrorName(SInt16 code, char *name)
{
    Boolean save = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_ReportError(code, name);
    data_005884fd = save;
}

static inline void CPrep_WarningName(SInt16 code, char *name)
{
    Boolean save = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_Warning(code, name);
    data_005884fd = save;
}

static inline void CPrep_MacroRedefError(char *name, Boolean *flag)
{
    if (!*flag) {
        if (!copts.gcc_extensions) {
            if (copts.cpp_extensions) {
                if (copts.extended_errorcheck)
                    CPrep_WarningName(0x6c, name);
            } else {
                CPrep_ErrorName(0x6c, name);
            }
        }
        *flag = 1;
    }
}

static inline CPrepCU *CPrep_CurrentCompilationUnit(void)
{
    return (CPrepCU *)cprep_cu;
}

#pragma auto_inline off
HashNameNode *fn_00441850(CPrepFileInfo *file, SInt32 *position)
{
    char fileName[256];
    COS_FileGetPathName(fileName, file, position);
    return GetHashNameNode(fileName);
}
#pragma auto_inline reset

int CPrep_AppendStringBounded(char *dst, char *src, int size)
{
    int len = strlen(dst);

    dst += len;
    size -= len;
    if (size < 0)
        return 1;
    while (size) {
        if ((*dst++ = *src++) == '\0')
            break;
        size--;
    }
    if (size == 0 && dst[-1] != '\0') {
        dst[-1] = '\0';
        return 1;
    }
    return 0;
}

unsigned int __stdcall CPrep_InvokeCompilerCallback(CWPluginPrivateContext *instance_id,
                                                    struct FileProcessingInfo *argument, const char *value)
{
    CWPluginPrivateContext *instance;

    if (argument == NULL) {
        return 3;
    }
    instance = (CWPluginPrivateContext *)validate_context_signature(instance_id);
    if (instance == NULL) {
        return 4;
    }
    return instance->compilerCallbacks->invoke(instance, argument, value);
}

#pragma auto_inline off
unsigned int __stdcall call_compiler_callback(CWPluginPrivateContext *object_id, void *argument, unsigned int result)
{
    CWPluginPrivateContext *object;
    if (argument == NULL) {
        return 3;
    }
    if (result == 0) {
        return 3;
    }
    object = validate_context_signature(object_id);
    if (object == NULL) {
        return 4;
    }
    return (*(unsigned int(__stdcall **)(CWPluginPrivateContext *, void *, unsigned int))object->compilerCallbacks)(
        object, argument, result);
}
#pragma auto_inline reset

#pragma auto_inline off
SInt32 __stdcall CPrep_CallCompilerCallback(void *objectId, SInt32 argument)
{
    CWPluginPrivateContext *object;

    object = validate_context_signature(objectId);
    if (object == NULL) {
        return 4;
    }
    return object->compilerCallbacks->callback(object, argument);
}
#pragma auto_inline reset

unsigned int __stdcall CPrep_CallCompilerCallbackWithValue(CWPluginPrivateContext *object_id, unsigned int argument,
                                                           long *value)
{
    CWPluginPrivateContext *object;

    if (value == NULL) {
        return 3;
    }
    object = validate_context_signature(object_id);
    if (object == NULL) {
        return 4;
    }
    return ((int(__stdcall **)(CWPluginPrivateContext *, unsigned int, long *))object->compilerCallbacks)[2](
        object, argument, value);
}

unsigned int __stdcall CPrep_GetResultValues(CWPluginPrivateContext *handle, UInt32 *first_value, UInt32 *second_value)
{
    struct Value {
        unsigned short words[2];
    };
    struct ResultValues {
        unsigned char reserved[338];
        struct Value first;
        struct Value second;
    };
    CWPluginPrivateContext *record;
    struct ResultValues *values;

    if (first_value == NULL) {
        return 3;
    }
    if (second_value == NULL) {
        return 3;
    }
    record = validate_context_signature(handle);
    if (record == NULL) {
        return 4;
    }
    values = (struct ResultValues *)record;
    *(struct Value *)first_value = values->first;
    *(struct Value *)second_value = values->second;
    return 0;
}

int __stdcall CPrep_GetContextPayload(CWPluginPrivateContext *handle, CWFileSpec *destination)
{
    CWPluginPrivateContext *storage;
    if (destination == NULL)
        return 3;
    storage = (CWPluginPrivateContext *)validate_context_signature(handle);
    if (storage == NULL)
        return 4;
    *destination = storage->contextData.payload;
    return 0;
}

unsigned int __stdcall CPrep_GetDependencyOption(int lookupKey, unsigned short *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature((CWPluginPrivateContext *)lookupKey);
    if (record == NULL) {
        return 4;
    }
    *value = record->dependencyOption;
    return 0;
}

unsigned int __stdcall CPrep_GetFileIndex(CWPluginPrivateContext *key, unsigned int *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature(key);
    if (record == NULL) {
        return 4;
    }
    *value = record->requestData.fileIndex;
    return 0;
}

#pragma sym reset

SInt32 __stdcall CPrep_GetTargetSettings(CWPluginPrivateContext *context, TgtRec *target)
{
    CWPluginPrivateContext *plugin;

    if (target == NULL) {
        return 3;
    }
    plugin = validate_context_signature(context);
    if (plugin == NULL) {
        return 4;
    }
    if (plugin->apiVersion >= 10) {
        *target = *plugin->targetSettings;
    } else if (plugin->apiVersion >= 8) {
        target->head = plugin->targetSettings->head;
    } else {
        memset(target, 0, sizeof(*target));
        if (plugin->firstFile.name[0] != 0) {
            target->head.tag = 1;
        } else {
            target->head.tag = 0;
        }
        target->head.firstFile = plugin->firstFile;
        target->head.secondFile = plugin->secondFile;
        target->head.thirdFile = plugin->firstFile;
        target->head.linkage = plugin->linkage;
        target->head.firstByte = plugin->firstByte;
        target->head.secondByte = plugin->secondByte;
        target->head.thirdCode = plugin->thirdCode;
        target->head.fourthCode = plugin->fourthCode;
    }
    return 0;
}

#pragma sym off

unsigned int __stdcall CPrep_GetDependencyState(CWPluginPrivateContext *context, BrowseOptions *state)
{
    CWPluginPrivateContext *validatedContext;
    if (state == NULL) {
        return 3;
    }
    validatedContext = validate_context_signature(context);
    if (validatedContext == NULL) {
        return 4;
    }
    *state = validatedContext->dependencyState;
    return 0;
}

unsigned int __stdcall CPrep_GetReserved15d(int handle, unsigned char *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature((CWPluginPrivateContext *)handle);
    if (record == NULL) {
        return 4;
    }
    *value = record->reserved15d;
    return 0;
}

unsigned int __stdcall CPrep_GetSetting(CWPluginPrivateContext *key, unsigned char *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature(key);
    if (record == NULL) {
        return 4;
    }
    *value = record->setting;
    return 0;
}

unsigned int __stdcall CPrep_GetOperation(int key, unsigned char *value)
{
    CWPluginPrivateContext *result;

    if (value == NULL) {
        return 3;
    }
    result = validate_context_signature((CWPluginPrivateContext *)key);
    if (result == NULL) {
        return 4;
    }
    *value = result->operation;
    return 0;
}

unsigned int __stdcall CPrep_GetActive(int selector, unsigned char *value)
{
    CWPluginPrivateContext *state;

    if (value == NULL) {
        return 3;
    }
    state = validate_context_signature((CWPluginPrivateContext *)selector);
    if (state == NULL) {
        return 4;
    }
    *value = state->active;
    return 0;
}

int __stdcall CPrep_GetEnabled(int key, unsigned char *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature((CWPluginPrivateContext *)key);
    if (record == NULL) {
        return 4;
    }
    *value = record->enabled;
    return 0;
}

#pragma auto_inline off

CWPluginPrivateContext *validate_context_signature(CWPluginPrivateContext *record)
{
    if (record && ((long)record->contextSignature == 'Comp' || (long)record->contextSignature == 'Link')) {
        return record;
    }
    return NULL;
}

#pragma auto_inline reset

void __stdcall CPrep_RegisterPredefinedMacros(void)
{
    const char *optionalName;

    REGN("__FILE__", data_0054fd1c);
    REGN("__LINE__", data_0054fd08);
    REGN("__DATE__", data_0054fd30);
    REGN("__TIME__", data_0054fd44);
    REGN("__STDC__", data_0054fd58);
    REGN("__CASM__", data_0054fd6c);
    REGN("__MC68020__", data_0054fd80);
    REGN("__MC68881__", data_0054fd94);
    REGN("__A5__", data_0054fda8);
    REGN("__cplusplus", cplusplus_predefined_macro);
    REGN("__IEEEdoubles__", data_0054fdd0);
    REGN("__fourbyteints__", data_0054fde4);
    REGN("__MWERKS__", data_0054fdf8);
    REGN("__profile__", data_0054fe48);
    REGN("__option", data_0054fe5c);
    REGN("__SOM_ENABLED__", data_0054fe84);
    REGN("__embedded_cplusplus", data_0054feac);
    REGN("__VEC__", data_0054fec0);
    REGN("__ALTIVEC__", data_0054fed4);
    REGN("powerc", data_0054fe0c);
    REGN("__powerc", data_0054fe20);
    REGN("__POWERPC__", data_0054fe70);

    optionalName = CMach_GetCPU();
    if (optionalName != NULL) {
        REGN((char *)optionalName, optionalNameMacro);
    }
    REGN("__PPC_EABI__", data_0054fe98);
}

unsigned int *CPrep_RemoveFlaggedMacros(void)
{
    Macro **p;
    int i;
    Macro *n;

    for (i = 0; i < 0x800; i++) {
        p = &macro_buckets[i];
        while ((n = *p) != NULL) {
            if (n->flag != 0) {
                *p = n->next;
            } else {
                p = &n->next;
            }
        }
    }
    return (unsigned int *)n;
}

#pragma sym reset

int initialize_preprocessor(void)
{
    SInt32 character;

    data_0057f9d4 = time(NULL);
    text_offset = 0;
    data_0057f9d3 = 0;
    data_0058850d = 0;
    data_0058852a = 0;
    data_0057f9de = 0;
    current_file_index = -1;
    line_count = next_scaled_ticks = 0;
    macrocheck = data_0057fcee = 1;
    string_literal_buffer_size = 0x100;
    macro_expansion_depth = if_depth = 0;
    data_005875f8 = NULL;
    data_0057f9ca = NULL;
    data_0057f9dc = func_errors = anyerrors = 0;
    concatenating_string_tokens = 0;
    data_00587708 = CError_LongJump;
    macro_text.data = NULL;
    data_00586da8.handle = NULL;
    if (InitGList(&macro_text, 10000) != 0) {
        CError_LongJump();
    }
    if ((string_literal_storage = COS_NewHandle(0x100)) == NULL) {
        CError_LongJump();
    }
    if ((buffered_token_storage = COS_NewHandle(1024 * sizeof(*buffered_tokens))) == NULL) {
        CError_LongJump();
    }
    COS_LockHandleHi(buffered_token_storage);
    buffered_tokens = buffered_token_storage->tokens;
    buffered_token_buffer_end = buffered_tokens + 1023;
    bufferedTokenPosition = buffered_tokens;
    buffered_token_capacity = 0x400;
    remainingBufferedTokenCount = 0;
    macro_buckets = (struct Macro **)galloc(2048 * sizeof(*macro_buckets));
    memclrw(macro_buckets, 2048 * sizeof(*macro_buckets));
    CPrep_RegisterPredefinedMacros();
    for (character = 0; 256 > character; character++) {
        data_00586fd0[character] = 0;
    }
    for (character = 'a';; character++) {
        data_00586fd0[character] = 1;
        if (character == 'z')
            break;
    }
    for (character = 'A';; character++) {
        data_00586fd0[character] = 1;
        if (character == 'Z')
            break;
    }
    for (character = '0';; character++) {
        data_00586fd0[character] = 2;
        if (character == '9')
            break;
    }
    data_0058702f = 1;
    f87_enabled = (copts.dollar_identifiers > 0);
    if (copts.filesyminfo) {
        DWARF_Init();
    }
    fn_0048b500();
    return 0;
}

#pragma sym off

void pop_files_and_release_heaps_and_lists(void)
{
    while (current_file_index >= 0) {
        CPrep_PopFile();
    }
    total_heap_size = CTool_TotalHeapSize();
    releaseheaps();
    data_00587708 = NULL;
    FreeGList(&macro_text);
    FreeGList(&data_00586da8.list);
    if (string_literal_storage != NULL) {
        COS_FreeHandle(string_literal_storage);
        string_literal_storage = NULL;
    }
    if (buffered_token_storage != NULL) {
        COS_FreeHandle(buffered_token_storage);
        buffered_token_storage = NULL;
    }
    buffered_tokens = buffered_token_buffer_end = bufferedTokenPosition = NULL;
}

#pragma sym reset

unsigned char fn_004401b0(unsigned char *name, unsigned char mode, unsigned char skip)
{
    CPrepFileInfo node;
    UInt32 type;
    SInt32 offset;
    short id;
    unsigned char resolvedName[258];
    unsigned short volume;
    SInt32 directory;
    DropinFileCallback input;
    char nameBuf[256];
    UInt8 *handle;
    unsigned int mapping;
    int isDefault;

    if (current_file_index >= 31) {
        data_0057f9dc = 1;
        CError_FatalError(ERR_INCLUDE_NESTING_OVERFLOW);
        return 0;
    }
    memclrw(&node, sizeof(node));
    isDefault = 0;
    if (mode == 0)
        isDefault = 1;
    node.isDefault = isDefault;
    if (name != NULL) {
        memclrw(&input, sizeof(input));
        input.enableDependencyLookup = mode;
        input.searchOption = 1;
        input.fileKey = 0xffffffff;
        memcpy(nameBuf, name + 1, name[0]);
        nameBuf[name[0]] = 0;
        if (CWPluginsPrivate_CallSignatureCallback(CPrep_CurrentCompilationUnit()->context, nameBuf, &input) != 0) {
            LogName(name);
            return 0;
        }
        if ((skip != 0 || data_0057f9d3 != 0) && input.callbackState != 0)
            return 1;
        node.textfile = input.output;
        if (input.fileReference != NULL) {
            if ((int)input.referenceKind == 1) {
                node.textbuffer = (char *)input.fileReference;
                node.textlength = input.referenceValue;
                node.fileID = input.lookupResult;
                node.recordbrowseinfo = input.lookupFailed;
            } else if ((int)input.referenceKind == 2) {
                CPrec_LoadPrecompiledHeader(0, (unsigned char *)input.fileReference);
                return 1;
            } else {
                LogName(name);
                return 0;
            }
        } else {
            COS_FileGetFSSpecInfo(&node.textfile, &volume, &directory, resolvedName);
            if (COS_FileOpen(&node.textfile, &id) != 0) {
                LogName(name);
                return 0;
            }
            if (COS_FileGetType(&node.textfile, &type) != 0 || COS_FileGetSize(id, &offset) != 0) {
                COS_FileClose(id);
                LogName(name);
                return 0;
            }
            if (type == copts.precompiledHeaderFileTypes[0]) {
                if (CPrep_CurrentCompilationUnit()->useMappedPrecompiledHeaders != 0) {
                    if (fn_0041bab0(CPrep_CurrentCompilationUnit()->context, offset, 1, &mapping) != 0 &&
                        fn_0041bab0(CPrep_CurrentCompilationUnit()->context, offset, 0, &mapping) != 0) {
                        COS_FileClose(id);
                        CError_LongJump();
                    }
                    fn_0041bb50(CPrep_CurrentCompilationUnit()->context, mapping, 0, &handle);
                    if (COS_FileRead(id, handle, offset) != 0) {
                        COS_FileClose(id);
                        CWPluginsPrivate_CallContextArgumentCallback(CPrep_CurrentCompilationUnit()->context, mapping);
                        LogName(name);
                        return 0;
                    }
                    COS_FileClose(id);
                    call_compiler_callback(CPrep_CurrentCompilationUnit()->context, &node, mapping);
                    CPrec_LoadPrecompiledHeader(0, handle);
                    fn_0041bbb0(CPrep_CurrentCompilationUnit()->context, mapping);
                    return 1;
                }
                CPrec_LoadPrecompiledHeader(id, NULL);
                COS_FileClose(id);
                return 1;
            }
            COS_FileClose(id);
            LogName(name);
            return 0;
        }
    } else {
        if (CPrep_CurrentCompilationUnit()->mainFileOffset == 0) {
            COS_FileGetFSSpecInfo(&CPrep_CurrentCompilationUnit()->mainFile, &volume, &directory, resolvedName);
            LogName(resolvedName);
            CError_DispatchAndLongJump();
            return 0;
        }
        node.textfile = CPrep_CurrentCompilationUnit()->mainFile;
        node.textbuffer = (char *)CPrep_CurrentCompilationUnit()->mainFileOffset;
        node.textlength = CPrep_CurrentCompilationUnit()->mainFileLength;
        node.fileID = CPrep_CurrentCompilationUnit()->mainFileAttributes;
        node.recordbrowseinfo = CPrep_CurrentCompilationUnit()->compiling;
    }
    if (current_file_index >= 0) {
        data_0057f94a[current_file_index]->linenumber = data_00587ef0;
        data_0057f94a[current_file_index]->hasprepline = data_0057f9dd;
        data_0057f94a[current_file_index]->pos =
            currentTextPosition - (UInt8 *)data_0057f94a[current_file_index]->textbuffer;
    }
    currentTextPosition = (UInt8 *)node.textbuffer;
    data_00587ef0 = 1;
    data_00588524 = 1;
    data_0057f94a[++current_file_index] = galloc(sizeof(node));
    *data_0057f94a[current_file_index] = node;
    currentPFile = data_0057f94a[current_file_index];
    data_00587fb0 = currentPFile->textbuffer;
    textend = (UInt8 *)currentPFile->textbuffer + currentPFile->textlength;
    if (data_0058850f != 0 && copts.simple_prepdump == 0)
        CPreprocess_EmitLineDirective();
    return 1;
}

void CPrep_GrowBufferedTokenBuffer(SInt32 n)
{
    SInt32 count = bufferedTokenPosition - buffered_tokens;
    char *tokenData;

    COS_UnlockHandle(buffered_token_storage);
    if (!COS_ResizeHandle(buffered_token_storage, (buffered_token_capacity + n) * 24))
        CError_LongJump();
    COS_LockHandleHi(buffered_token_storage);
    buffered_token_capacity += n;
    tokenData = buffered_token_storage->data;
    buffered_tokens = (struct TStreamElement *)tokenData;
    buffered_token_buffer_end = &buffered_tokens[buffered_token_capacity - 1];
    bufferedTokenPosition = buffered_tokens + count;
}

#pragma sym off

void CPrep_GetBufferedTokenPosition(SInt32 *count)
{
    *count = bufferedTokenPosition - buffered_tokens;
}

void CPrep_SetBufferedTokenPosition(SInt32 *count)
{
    SInt32 value = *count;
    struct TStreamElement *top = bufferedTokenPosition;
    struct TStreamElement *next;
    remainingBufferedTokenCount += (top - buffered_tokens) - value;
    value = *count;
    next = buffered_tokens + value;
    bufferedTokenPosition = next;
}

#pragma sym reset

static void LogName(const unsigned char *name)
{
    short n;
    n = (short)name[0];
    if (n > 63)
        n = 63;
    memcpy(file_cannot_opened_name, (const void *)(name + 1), n);
    file_cannot_opened_name[n] = 0;
    if (currentPFile != NULL) {
        data_0057f9dc = 1;
        CError_ReportError(ERR_FILE_CANNOT_OPENED, file_cannot_opened_name);
    } else {
        CError_DispatchAndLongJump();
    }
}

#pragma sym off

void CPrep_UngetToken(void)
{
    remainingBufferedTokenCount++;
    bufferedTokenPosition--;
    if (bufferedTokenPosition < buffered_tokens)
        CError_FATAL(1237);
}

void CPrep_SetPosition(SInt32 *position)
{
    remainingBufferedTokenCount += (bufferedTokenPosition - buffered_tokens) - (*position - 1);
    bufferedTokenPosition = buffered_tokens + (*position - 1);
    tk = CPrepTokenizer_GetNextToken();
}

int scan_braced_tokens(void (*callback)(TStreamElement *), int tokenCount)
{
    int count = tokenCount + 1;
    int braceDepth = 1;
    short token;
    TStreamElement record;

    for (;;) {
        count++;
        token = CPrepTokenizer_GetNextToken();
        switch (token) {
            case 0:
                return 0;
            case -3:
                if (callback != NULL) {
                    record = bufferedTokenPosition[-1];
                    callback(&record);
                    bufferedTokenPosition[-1] = record;
                    tk = (UInt16)record.tokentype;
                }
                break;
            case '{':
                braceDepth++;
                break;
            case '}':
                braceDepth--;
                if (braceDepth <= 0)
                    return count;
                break;
        }
    }
}

void CPrep_SaveFunctionBodyTokens(TokenStream *result, void (*tokenCallback)(TStreamElement *), int option)
{
    TStreamElement firstToken;
    TStreamElement handlerToken;
    UInt8 savedMode;
    int tokenCount;
    int bodyStart;
    int firstTokenIndex;
    TStreamElement *tokenBuffer;
    UInt16 nextToken;
    Boolean hasHandlers;

    firstTokenIndex = bufferedTokenPosition - buffered_tokens - 1;
    tokenCount = 0;
    bodyStart = 0;
    hasHandlers = FALSE;
    result->tokens = 0;
    result->firsttoken = NULL;
    savedMode = data_0058850d;
    data_0058850d = 1;
    switch (tk) {
        case TK_TRY:
            hasHandlers = TRUE;
        case ':':
            tokenCount = 0;
            tokenCount++;
            for (;;) {
                switch (CPrepTokenizer_GetNextToken()) {
                    case 0x7b:
                        bodyStart = tokenCount;
                        break;
                    case 0:
                    case 0x3b:
                        bodyStart = 0;
                        break;
                    case -3:
                        if (tokenCallback) {
                            firstToken = bufferedTokenPosition[-1];
                            tokenCallback(&firstToken);
                            bufferedTokenPosition[-1] = firstToken;
                            tk = firstToken.tokentype;
                        }
                    default:
                        ++tokenCount;
                        continue;
                }
                break;
            }
            if (bodyStart == 0) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                data_0058850d = savedMode;
                return;
            }
            break;
        case '{':
            break;
        default:
            CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            data_0058850d = savedMode;
            return;
    }
    tokenCount = scan_braced_tokens(tokenCallback, bodyStart);
    if (tokenCount == 0) {
        CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
        data_0058850d = savedMode;
        return;
    }
    if (hasHandlers) {
        for (;;) {
            switch (CPrepTokenizer_GetNextToken()) {
                case -7:
                    tokenCount++;
                    continue;
                case 0x144:
                    tokenCount++;
                    for (;;) {
                        switch (CPrepTokenizer_GetNextToken()) {
                            case 0x7b:
                                bodyStart = tokenCount;
                                break;
                            case 0:
                            case 0x3b:
                                bodyStart = 0;
                                break;
                            case -3:
                                if (tokenCallback) {
                                    handlerToken = bufferedTokenPosition[-1];
                                    tokenCallback(&handlerToken);
                                    bufferedTokenPosition[-1] = handlerToken;
                                    tk = handlerToken.tokentype;
                                }
                            default:
                                tokenCount++;
                                continue;
                        }
                        break;
                    }
                    if ((bodyStart == 0) ||
                        (tokenCount = scan_braced_tokens(tokenCallback, bodyStart), tokenCount == 0)) {
                        CError_ReportError(ERR_CATCH_EXPECTED);
                        data_0058850d = savedMode;
                        return;
                    }
                    nextToken = CPrepTokenizer_PeekNextToken();
                    if (nextToken == 0x144)
                        continue;
                    break;
                default:
                    CError_ReportError(ERR_CATCH_EXPECTED);
                    data_0058850d = savedMode;
                    return;
            }
            break;
        }
    }
    data_0058850d = savedMode;
    result->tokens = tokenCount;
    tokenBuffer = (TStreamElement *)galloc(tokenCount * sizeof(TStreamElement));
    result->firsttoken = tokenBuffer;
    memcpy(result->firsttoken, buffered_tokens + firstTokenIndex, tokenCount * sizeof(TStreamElement));
    return;
}

void CPrep_BufferTokensThroughSemicolon(TokenStream *buffer, void (*processToken)(struct TStreamElement *))
{
    int savedMode;
    SInt32 tokenCount;
    SInt32 firstToken;

    firstToken = (bufferedTokenPosition - buffered_tokens) - 1;
    savedMode = data_0058850d;
    data_0058850d = 1;
    buffer->tokens = 0;
    buffer->firsttoken = NULL;
    tokenCount = 1;
    while (tk != 0 && tk != ';') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == TK_IDENTIFIER && processToken != NULL) {
            processToken(bufferedTokenPosition - 1);
            tk = bufferedTokenPosition[-1].tokentype;
        }
        tokenCount++;
    }
    buffer->tokens = tokenCount;
    buffer->firsttoken = galloc(tokenCount * sizeof(*buffer->firsttoken));
    memcpy(buffer->firsttoken, buffered_tokens + firstToken, tokenCount * sizeof(*buffer->firsttoken));
    data_0058850d = savedMode;
}

void CPrep_InsertTokenBuffer(TokenStream *arg, SInt32 *result)
{
    SInt32 index;
    SInt32 count;
    char *tokenData;

    if (remainingBufferedTokenCount + (index = bufferedTokenPosition - buffered_tokens) + (count = arg->tokens) >=
        buffered_token_capacity) {
        COS_UnlockHandle(buffered_token_storage);
        if (!COS_ResizeHandle(buffered_token_storage, (buffered_token_capacity + count) * sizeof(TStreamElement)))
            CError_LongJump();
        COS_LockHandleHi(buffered_token_storage);
        buffered_token_capacity += count;
        tokenData = buffered_token_storage->data;
        buffered_tokens = (TStreamElement *)tokenData;
        buffered_token_buffer_end = buffered_tokens + (buffered_token_capacity - 1);
        bufferedTokenPosition = buffered_tokens + index;
    }
    if (remainingBufferedTokenCount != 0)
        memmove(bufferedTokenPosition + arg->tokens, bufferedTokenPosition,
                remainingBufferedTokenCount * sizeof(TStreamElement));
    memcpy(bufferedTokenPosition, arg->firsttoken, arg->tokens * sizeof(TStreamElement));
    remainingBufferedTokenCount += arg->tokens;
    *result = bufferedTokenPosition - buffered_tokens;
}

#pragma sym reset

void CPrep_RemoveBufferedTokens(TokenStream *stream, SInt32 *firstIndex)
{
    int remainingCount;
    int index;
    TStreamElement *firstEntry;

    index = *firstIndex;
    remainingCount = bufferedTokenPosition - buffered_tokens;
    firstEntry = buffered_tokens + index;
    remainingCount = remainingCount - index - stream->tokens + remainingBufferedTokenCount;
    if (remainingCount >= 0) {
        if (remainingCount != 0) {
            memmove(firstEntry, firstEntry + stream->tokens, remainingCount * sizeof(*firstEntry));
        }
        bufferedTokenPosition = firstEntry;
        remainingBufferedTokenCount = remainingCount;
    }
}

#pragma sym off

void CPrep_ResetBufferedTokenPosition(void)

{
    if (remainingBufferedTokenCount == 0) {
        bufferedTokenPosition = buffered_tokens;
    }
    return;
}

int fn_0043f860(char *p)
{
    int len;
    short c;

    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
        p++;
        len = 1;
        for (;;) {
            c = *p++;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                len++;
            else
                return len;
        }
    }

    switch (*p) {
        case '*':
            if (p[1] == '=')
                return 2;
            break;
        case '/':
            if (p[1] == '=' || p[1] == '*' || p[1] == '/')
                return 2;
            break;
        case '%':
            if (p[1] == '=')
                return 2;
            break;
        case '+':
            if (p[1] == '=' || p[1] == '+')
                return 2;
            break;
        case '-':
            if (p[1] == '>') {
                if (p[2] == '*')
                    return 3;
                return 2;
            }
            if (p[1] == '=' || p[1] == '-')
                return 2;
            break;
        case '<':
            if (p[1] == '=')
                return 2;
            if (p[1] == '<') {
                if (p[2] == '=')
                    return 3;
                return 2;
            }
            break;
        case '>':
            if (p[1] == '=')
                return 2;
            if (p[1] == '>') {
                if (p[2] == '=')
                    return 3;
                return 2;
            }
            break;
        case '&':
            if (p[1] == '=' || p[1] == '&')
                return 2;
            break;
        case '^':
            if (p[1] == '=')
                return 2;
            break;
        case '|':
            if (p[1] == '=' || p[1] == '|')
                return 2;
            break;
        case '=':
            if (p[1] == '=')
                return 2;
            break;
        case '!':
            if (p[1] == '=')
                return 2;
            break;
        case '.':
            if (p[1] == '.' && p[2] == '.')
                return 3;
            if (p[1] == '*')
                return 2;
            break;
        case ':':
            if (p[1] == ':')
                return 2;
            break;
    }
    return 1;
}

#pragma sym reset

#pragma auto_inline off
TStreamElement *CPrep_GetLastBufferedToken(void)
{
    if (buffered_tokens < bufferedTokenPosition)
        return bufferedTokenPosition - 1;
    return &lastBufferedToken;
}
#pragma auto_inline reset

#pragma sym off

/* Where an error is: the file and offset of TOKEN (else of the current token or the current position), its line and
   column, the source line around it with the error's column marked, and the text just before it. */
void CPrep_GetTokenLocation(TStreamElement *token, CPrepFileInfo **file, SInt32 *position, short *column, SInt32 *line,
                            char *text, short *textpos, short *textcol, char *context, short *contextpos)
{
    SInt32 lineno;
    SInt32 j;
    SInt32 n;
    TStreamElement *tok;
    char *base, *cursor, *start;
    SInt32 offset;
    SInt32 i;
    short c;
    Boolean release;
    short auxshort;
    SInt32 auxlong;
    char *loaded;
    unsigned char name[256];
    CPrepFileInfo *pf;
    char *end;

    if (token && !token->tokenfile)
        token = NULL;
    tok = token;
    if (!tok && buffered_tokens < bufferedTokenPosition)
        tok = bufferedTokenPosition - 1;
    if (tok && !tok->tokenfile)
        CError_FATAL(1714);
    if (data_0057f9dc || !tok) {
        if (!currentPFile)
            CError_DispatchAndLongJump();
        *file = currentPFile;
        if (!macro_expansion_depth) {
            offset = (char *)currentTextPosition - data_00587fb0;
            if (data_0057f9dc && offset > 0)
                --offset;
        } else
            offset = macro_stack[0].pos - data_00587fb0;
        *position = offset;
    } else {
        *file = tok->tokenfile;
        *position = offset = tok->tokenoffset;
    }
    if (!(base = (pf = *file)->textbuffer)) {
        if (CWPluginsPrivate_ValidateAndCallCallback(*(CWPluginPrivateContext **)cprep_cu, (int)pf, (int)&loaded,
                                                     (int)&auxlong, (int)&auxshort)) {
            COS_FileGetFSSpecInfo(&pf->textfile, NULL, NULL, name);
            c = name[0];
            if (c > 63)
                c = 63;
            memcpy(file_cannot_opened_name, name + 1, c);
            file_cannot_opened_name[c] = 0;
            if (currentPFile) {
                data_0057f9dc = 1;
                CError_ReportError(ERR_FILE_CANNOT_OPENED, file_cannot_opened_name);
            } else
                CError_DispatchAndLongJump();
            CError_DispatchAndLongJump();
            base = NULL;
        } else
            base = loaded;
        release = 1;
    } else
        release = 0;
    if (!data_0057f9dd) {
        for (n = 1, j = 0; j < offset; j++)
            if (base[j] == 13)
                n++;
        *line = n;
    } else
        *line = lineno = data_00587ef0;
    c = fn_0043f860(end = base + offset);
    *column = c;
    if (!token && macro_expansion_depth == 1) {
        cursor = (char *)token_start;
        start = macro_text_start;
    } else {
        start = base;
        cursor = end;
    }
    *textpos = 0;
    for (i = 1; i < 80 && cursor - i >= start && cursor[-i] != 13; i++) {
    }
    --i;
    while ((c = cursor[-i]) && (c == 32 || c == 9 || c == 4))
        --i;
    j = 0;
    while ((c = cursor[-i]) != 13 && c && j < 126) {
        if (!i)
            *textpos = j;
        if ((int)c != 4) {
            text[j] = c == 9 ? 32 : c;
            j++;
        }
        --i;
    }
    if (!i) {
        text[j] = 32;
        *textpos = j;
        text[j + 1] = 0;
        *textcol = 1;
    } else {
        text[j] = 0;
        *textcol = fn_0043f860(text + *textpos);
    }
    if (offset > 16) {
        cursor = end - 16;
        *contextpos = 16;
    } else {
        cursor = base;
        *contextpos = offset;
    }
    for (n = 0; n < 31 && *cursor; n++)
        context[n] = *cursor++;
    context[n] = 0;
    if (release)
        fn_0041b7f0(*(CWPluginPrivateContext **)cprep_cu, base);
    data_0057f9dc = 0;
}

#pragma auto_inline off
NameSpaceList *CPrep_ReportError(short token)
{
    Boolean savedState = data_005884fd;
    NameSpaceList *result;

    data_005884fd = 0;
    if (token == 0x66 && (macro_expansion_depth > 0 || currentTextPosition < textend))
        token = 0x69;
    data_0057f9dc = 1;
    result = CError_ReportError(token);
    data_005884fd = savedState;
    return result;
}
#pragma auto_inline reset

void fn_0043f3e0(unsigned int token, char *name)
{
    unsigned char saved;

    saved = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    if ((unsigned short)token == 102 && (macro_expansion_depth > 0 || currentTextPosition < textend)) {
        CError_ReportError(105U);
    } else {
        CError_ReportError((short)token, name);
    }
    data_005884fd = saved;
}

void fn_0043f3b0(short warningCode)
{
    Boolean savedFlag = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_Warning(warningCode);
    data_005884fd = savedFlag;
}

#pragma sym reset

void CPrep_PopFile(void)
{
    CPrepFileInfo *input;

    if (current_file_index < 0)
        return;

    fn_0041b7f0(((struct CPrepCU *)cprep_cu)->context, currentPFile->textbuffer);
    currentPFile->textbuffer = NULL;
    --current_file_index;
    if (current_file_index >= 0) {
        input = data_0057f94a[current_file_index];
        data_00587fb0 = (currentPFile = input)->textbuffer;
        textend = (UInt8 *)((*(CPrepFileInfo *volatile *)&currentPFile)->textbuffer +
                            (*(CPrepFileInfo *volatile *)&currentPFile)->textlength);
        currentTextPosition = (UInt8 *)(data_00587fb0 + input->pos);
        data_00587ef0 = (*(CPrepFileInfo *volatile *)&currentPFile)->linenumber;
        data_0057f9dd = (*(CPrepFileInfo *volatile *)&currentPFile)->hasprepline;
        data_00588524 = 1;
    }
    if (data_0058850f != 0 && copts.simple_prepdump == 0)
        CPreprocess_EmitLineDirective();
}

#pragma sym off

void fn_0043f1f0(FileOffsetInfo *name)
{
    PrepNameCacheEntry *entry;
    PrepNameCacheEntry *newEntry;
    HashNameNode *value;
    CPrepFileInfo *currentName;

    if ((currentName = name->file) == NULL) {
        return;
    }
    if (currentName == (CPrepFileInfo *)data_0057f94a[0]) {
        if (data_005875f8 == NULL) {
            return;
        }
        if (*(cprep_cu + 0xe0) != '\x01') {
            fn_0048b160(NULL, 0, 0);
        }
        data_005875f8 = NULL;
        return;
    }
    if (data_005875f8 == currentName) {
        return;
    }
    for (entry = data_0057f9ca; entry != NULL; entry = entry->next) {
        if (currentName == entry->name) {
            if (*(cprep_cu + 0xe0) != '\x01') {
                fn_0048b160(entry->value, entry->auxiliaryValue, 0);
            }
            data_005875f8 = name->file;
            return;
        }
    }
    newEntry = galloc(sizeof(PrepNameCacheEntry));
    newEntry->next = data_0057f9ca;
    data_0057f9ca = newEntry;
    newEntry->name = name->file;
    value = fn_00441850(newEntry->name, &newEntry->auxiliaryValue);
    newEntry->value = value;
    if (*(cprep_cu + 0xe0) != '\x01') {
        fn_0048b160(newEntry->value, newEntry->auxiliaryValue, 1);
    }
    data_005875f8 = name->file;
}

void CPrep_GetFOI(FileOffsetInfo *location, TStreamElement *record)
{
    if ((record == NULL) || (record->tokenfile == NULL)) {
        if (buffered_tokens < bufferedTokenPosition) {
            location->file = bufferedTokenPosition[-1].tokenfile;
            location->tokenline = bufferedTokenPosition[-1].tokenline;
        } else {
            location->file = (CPrepFileInfo *)data_0057f94a[current_file_index];
            location->tokenline = data_00587ef0;
        }
    } else {
        location->file = record->tokenfile;
        location->tokenline = record->tokenline;
    }
    location->is_inline = 0;
}

SInt32 CPrep_UpdateTokenLine(FileOffsetInfo *foi)
{
    int i;
    SInt32 line;

    if (buffered_tokens < bufferedTokenPosition &&
        bufferedTokenPosition[-1].tokenfile == (struct CPrepFileInfo *)foi->file) {
        line = bufferedTokenPosition[-1].tokenline;
        if (line > foi->tokenline)
            foi->tokenline = line;
    } else if (foi->file == data_0057f94a[current_file_index]) {
        if (data_00587ef0 > foi->tokenline)
            foi->tokenline = data_00587ef0;
    } else {
        for (i = current_file_index - 1; i >= 0; i--) {
            if (foi->file == data_0057f94a[i]) {
                if (data_0057f94a[i]->linenumber > foi->tokenline)
                    foi->tokenline = data_0057f94a[i]->linenumber;
                break;
            }
        }
    }
    return foi->tokenline;
}

void CPrep_GetPosition(CPrepFileInfo **position, SInt32 *offset)
{
    *position = currentPFile;
    if (macro_expansion_depth > 0)
        *offset = macro_stack[0].pos - data_00587fb0;
    else
        *offset = (char *)currentTextPosition - data_00587fb0;
}

#pragma sym reset

UInt8 CPrep_Compile(CPrepCU *cu)
{
    TStreamElement optionData;
    UInt8 result;
    CompilerLinkerOptions *optionSnapshot;
    CPrepCU *currentCU;

    data_0057fce0 = 0x80;
    data_00588515 = 0;
    data_00588516 = 0;
    cprep_cu = (UInt8 *)cu;
    currentPFile = NULL;
    data_00587ef0 = line_count = 0;
    data_0057f9dd = 0;

    if (CPrep_CallCompilerCallback(cu->context, 0) != 0)
        return 0;

    next_scaled_ticks = COS_GetTicks() + 5;
    result = 0;

    copts.delete_exception = 1;
    copts.SOMCallOptimization = 1;
    copts.template_patch = 1;
    copts.template_friends = 1;
    copts.simple_class_byval = 1;
    copts.array_new_delete = 1;
    copts.syspath_once = 1;
    copts.arg_dep_lookup = 1;
    copts.longlong_prepeval = copts.longlong = 1;
    copts.vbase_ctor_offset = 1;
    copts.vbase_abi_v2 = 1;

    if (CompilerTools_InitHeaps(CError_LongJump) != 0) {
        releaseheaps();
        data_00588516 = 1;
        result = 0xff;
    } else {
        if (_Setjmp(error_jmp_buf) == 0) {
            InitNameHash();
            initialize_preprocessor();
            CParser_Setup();
            currentCU = (CPrepCU *)cprep_cu;
            CException_ResetPrecompiledState(currentCU->precompiling);
            CSOM_NoOp();
            fn_0048b3f0();
            CBrowse_InitBrowseData(cu);
            fn_004401b0(NULL, 1, 0);
            if (*copts.f54 != 0)
                fn_004401b0((unsigned char *)copts.f54, 1, 0);

            optionSnapshot = galloc(sizeof(*optionSnapshot));
            data_0057fcea = optionSnapshot;
            *optionSnapshot = copts;

            pragma_list = NULL;
            saved_options = NULL;
            data_0058850f = cu->preprocessOnly;
            if (cu->preprocessOnly != 0)
                CPreprocess_OutputPreprocessedText();
            else
                cparser();

            if (CPrep_CallCompilerCallback(cu->context, line_count) != 0)
                anyerrors = 1;

            if (macro_expansion_depth != 0) {
                data_0057f9dc = 1;
                CError_FatalError(ERR_UNTERMINATED_IF_MACRO);
            } else if (if_depth != 0) {
                data_0057f9dc = 0;
                optionData.tokenfile = data_0057f6c8[if_depth - 1].file;
                optionData.tokenoffset = data_0057f6c8[if_depth - 1].pos;
                if (if_depth != 0)
                    CError_SetBufferedToken(&optionData);
                CError_FatalError(ERR_UNTERMINATED_IF_MACRO);
            }

            if (anyerrors == 0) {
                if (cu->precompiling == 1) {
                    CBrowse_StoreBrowseData(cu);
                    CPrec_WritePrecompiledFile();
                } else if (cu->preprocessOnly == 0) {
                    ObjGen_PPC_EABI_FinalizeOutputBuffers();
                    CBrowse_StoreBrowseData(cu);
                }
                result = 1;
            }

            if (cu->preprocessOnly != 0) {
                currentCU = (CPrepCU *)cprep_cu;
                currentCU->objectBuffer = (SInt32)data_00586da8.handle;
                data_00586da8.handle = NULL;
                currentCU = (CPrepCU *)cprep_cu;
                currentCU->browseBuffer = 0;
            }
        } else {
            CPrep_CallCompilerCallback(cu->context, line_count);
        }

        CParser_Cleanup();
        fn_004e0970();
        fn_004e67a0();
        fn_0048b2e0();
        fn_0048b2d0();
        pop_files_and_release_heaps_and_lists();
        CBrowse_FreeLists(cu);
    }

    cu->lineCount = line_count;
    if (data_00588516 != 0) {
        CompilerGetCString(7, error_message_buffer);
        currentCU = (CPrepCU *)cprep_cu;
        CWPluginsPrivate_InvokeMessageCallback(currentCU->context, NULL, error_message_buffer, NULL, 2, 0);
    }
    return result;
}

void CPrep_PopMacro(void)
{
    macro_expansion_depth--;
    if (macro_expansion_depth == 0 && data_0057fcee != 0)
        freeaheap();
    currentTextPosition = (UInt8 *)macro_stack[macro_expansion_depth].pos;
    macro_text_start = (char *)macro_stack[macro_expansion_depth].macname;
    if (macro_stack[macro_expansion_depth].macro != NULL)
        macro_stack[macro_expansion_depth].macro->isExpanding = 0;
    macrocheck = macro_stack[macro_expansion_depth].macrocheck;
    data_00588523 = 1;
}

#pragma sym off

Boolean fn_0043ecb0(short ch)
{
    char *p;
    short level;
    short c;

    p = (char *)currentTextPosition;
    level = macro_expansion_depth;
    for (;;) {
        switch (c = *p) {
            case 0:
                if (level <= 0)
                    return 0;
                p = macro_stack[--level].pos;
                continue;
            case 13:
                p++;
                continue;
        }
        if (c == ch)
            return 1;
        if (c == ' ' || (c >= 9 && c <= 12)) {
            p++;
            continue;
        }
        return 0;
    }
}

Macro *lookup_expandable_macro(void)
{
    unsigned char *token;
    unsigned int hash;
    HashNameNode *lookupName;
    Macro *entry;
    HashNameNode *name;
    unsigned int savedFlag;
    Macro **table;
    lookupName = name = data_00587fa0;
    table = macro_buckets;
    hash = lookupName->hashval;
    entry = table[hash];
    while (entry) {
        if (entry->name == name) {
            if (entry->flag) {
                if (entry == &data_0054fec0 && !copts.altivec_model)
                    return NULL;
                if (entry == &data_0054fd58 && copts.cplusplus)
                    return NULL;
                if (entry == &cplusplus_predefined_macro && !copts.cplusplus)
                    return NULL;
                if (entry == &data_0054feac && (!copts.cplusplus || !copts.ecplusplus))
                    return NULL;
            }
            if (entry->isExpanding) {
                data_0057f9d2 = 1;
                return NULL;
            }
            if (entry->nargs) {
                savedFlag = macrocheck;
                macrocheck = 0;
                skip_line_breaks_and_expand_macros();
                macrocheck = savedFlag;
                token = currentTextPosition;
                if (*token != '(')
                    return NULL;
            }
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/* Hash-bucket entry: chained by next, back-pointer to the name at +4,
 * a 16-bit kind at +0xc and a byte flag at +0xf. */

Macro *find_expandable_macro(UInt8 *text)
{
    char character;
    HashNameNode *name = data_00587fa0;
    Macro *entry = macro_buckets[name->hashval];

    while (entry != NULL) {
        if (entry->name == name) {
            if (entry->isExpanding) {
                data_0057f9d2 = 1;
                return NULL;
            }
            if (entry->nargs != 0) {
                while ((character = *text) == ' ' || (character >= 9 && character <= 12))
                    text++;
                if (character != '(' && !fn_0043ecb0('('))
                    return NULL;
            }
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

Macro *find_macro_for_expansion(UInt8 *p)
{
    HashNameNode *key = data_00587fa0;
    Macro *node = macro_buckets[key->hashval];

    while (node != NULL) {
        if (node->name == key) {
            if (node->isExpanding != 0) {
                data_0057f9d2 = 1;
                return NULL;
            }
            if (node->nargs != 0) {
                while (*p == ' ' || ((char)*p >= '\t' && (char)*p <= '\f'))
                    p++;
                if (*p != '(')
                    return NULL;
            }
            return node;
        }
        node = node->next;
    }
    return NULL;
}

Macro *find_macro(void)
{
    HashNameNode *key = (HashNameNode *)data_00587fa0;
    Macro *entry;
    for (entry = macro_buckets[key->hashval]; entry; entry = entry->next) {
        if (entry->name == key) {
            if (entry->flag) {
                if (entry == &data_0054fec0 && !copts.altivec_model)
                    return NULL;
                if (entry == &data_0054fd58 && copts.cplusplus)
                    return NULL;
                if (entry == &cplusplus_predefined_macro && !copts.cplusplus)
                    return NULL;
            }
            break;
        }
    }
    return entry;
}

#pragma sym reset

unsigned int lookup_available_macro(HashNameNode *name)
{
    Macro *node;
    node = macro_buckets[name->hashval];
    for (; node != NULL; node = node->next) {
        if (node->name == name) {
            if (node->flag == 0)
                break;
            if (node == &data_0054fec0) {
                if (copts.altivec_model == 0)
                    return 0;
            }
            if (node == &data_0054fd58) {
                if (copts.cplusplus != 0)
                    return 0;
            }
            if (node == &cplusplus_predefined_macro) {
                if (copts.cplusplus == 0)
                    return 0;
            }
            break;
        }
    }
    return (unsigned int)node;
}

#pragma sym off

void CPrep_IncrementCountersAndUpdateTextOffset(void)
{
    if (macro_expansion_depth == 0) {
        if (data_0058850f)
            fn_004d6ed0();
        data_00587ef0++;
        line_count++;
        if (current_file_index <= 0)
            text_offset = (char *)currentTextPosition - data_0057f94a[0]->textbuffer;
    }
}

void fn_0043e8f0(void)
{
    UInt32 ticks;

    if (macro_expansion_depth == 0) {
        if (data_0058850f != 0)
            fn_004d6ed0();
        data_00587ef0 += 1U;
        line_count += 1U;
        if (current_file_index <= 0)
            text_offset = (char *)currentTextPosition - data_0057f94a[0]->textbuffer;
    }
    data_00588470 = 1;
    data_00588524 = 1;
    data_00588523 = 1;
    ticks = COS_GetTicks();
    if (ticks > next_scaled_ticks) {
        struct CPrepCU *compilationUnit = (struct CPrepCU *)cprep_cu;
        if (CPrep_CallCompilerCallback(compilationUnit->context, line_count) != 0)
            CError_Longjmp();
        ticks = COS_GetTicks();
        ticks += 5;
        next_scaled_ticks = ticks;
    }
}

void skip_line_breaks_and_expand_macros(void)
{
    SInt8 ch;
    SInt8 savedMacroState;
    UInt8 *identifierStart;
    Macro *macro;
    UInt8 *expandedText;

    for (;;) {
        ch = CPrepTokenizer_ScanChar();
        switch (ch) {
            case 0:
                if (macro_expansion_depth != 0) {
                    pop_macro_expansion_state();
                    data_00588523 = 1;
                } else {
                    if (macro_expansion_depth > 0 || currentTextPosition >= textend) {
                        if (current_file_index > 0)
                            CPrep_PopFile();
                        else
                            return;
                    } else {
                        currentTextPosition = (UInt8 *)lookahead_position;
                    }
                }
                break;
            case 13:
                if (macro_expansion_depth == 0) {
                    if (data_0058850f != 0)
                        fn_004d6ed0();
                    data_00587ef0++;
                    line_count++;
                    if (current_file_index <= 0)
                        text_offset = (char *)currentTextPosition - data_0057f94a[0]->textbuffer;
                }
                data_00588470 = 1;
                data_00588524 = 1;
                data_00588523 = 1;
                if (COS_GetTicks() > next_scaled_ticks) {
                    struct CPrepCU *compilerUnit = (struct CPrepCU *)cprep_cu;
                    if (CPrep_CallCompilerCallback(compilerUnit->context, line_count))
                        CError_Longjmp();
                    next_scaled_ticks = COS_GetTicks() + 5;
                }
                currentTextPosition = (UInt8 *)lookahead_position;
                break;
            default:
                if (macrocheck != 0 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')) {
                    currentTextPosition = CPrepTokenizer_ScanIdentifier(identifierStart = currentTextPosition);
                    macro = lookup_expandable_macro();
                    if (macro != NULL) {
                        data_0057fcee = macrocheck = 0;
                        savedMacroState = data_00588523;
                        expandedText = expand_macro(macro);
                        macrocheck = 1;
                        if (macro_expansion_depth >= 0x80) {
                            data_0057f9dc = 1;
                            CError_FatalError(ERR_MACROS_TOO_COMPLEX);
                        } else {
                            macro_stack[macro_expansion_depth].pos = (char *)currentTextPosition;
                            macro_stack[macro_expansion_depth].macname = macro_text_start;
                            macro_stack[macro_expansion_depth].macro = NULL;
                            macro_stack[macro_expansion_depth].macrocheck = 1;
                            macro_expansion_depth++;
                        }
                        macro_text_start = (char *)(currentTextPosition = token_start = expandedText);
                        data_0057fcee = 1;
                        macrocheck = 0;
                        data_00588523 = savedMacroState;
                        break;
                    }
                    currentTextPosition = identifierStart;
                }
                return;
        }
    }
}

static void pop_macro_expansion_state(void)
{
    macro_expansion_depth--;
    if (macro_expansion_depth == 0 && data_0057fcee != 0)
        freeaheap();
    currentTextPosition = (UInt8 *)macro_stack[macro_expansion_depth].pos;
    macro_text_start = macro_stack[macro_expansion_depth].macname;
    if (macro_stack[macro_expansion_depth].macro != NULL)
        macro_stack[macro_expansion_depth].macro->isExpanding = 0;
    macrocheck = macro_stack[macro_expansion_depth].macrocheck;
}

SInt16 CPrep_ScanMacroExpandedChar(void)
{
    char c;
    UInt8 *savedptr;
    Macro *e;
    UInt8 savedbyte;
    UInt8 *newptr;
    for (;;) {
        c = CPrepTokenizer_ScanChar();
        switch (c) {
            case 0:
                if (macro_expansion_depth != 0) {
                    macro_expansion_depth--;
                    if (macro_expansion_depth == 0 && data_0057fcee != 0)
                        freeaheap();
                    currentTextPosition = (UInt8 *)macro_stack[macro_expansion_depth].pos;
                    macro_text_start = macro_stack[macro_expansion_depth].macname;
                    if (macro_stack[macro_expansion_depth].macro != NULL)
                        macro_stack[macro_expansion_depth].macro->isExpanding = 0;
                    macrocheck = macro_stack[macro_expansion_depth].macrocheck;
                    data_00588523 = 1;
                    break;
                }
                /* fall through */
            case 13:
                return 0;
            default:
                if (macrocheck != 0 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) {
                    currentTextPosition = CPrepTokenizer_ScanIdentifier(savedptr = currentTextPosition);
                    e = lookup_expandable_macro();
                    if (e != NULL) {
                        data_0057fcee = macrocheck = 0;
                        savedbyte = data_00588523;
                        newptr = expand_macro(e);
                        macrocheck = 1;
                        if (macro_expansion_depth >= 0x80) {
                            data_0057f9dc = 1;
                            CError_FatalError(ERR_MACROS_TOO_COMPLEX);
                        } else {
                            macro_stack[macro_expansion_depth].pos = (char *)currentTextPosition;
                            macro_stack[macro_expansion_depth].macname = macro_text_start;
                            macro_stack[macro_expansion_depth].macro = NULL;
                            macro_stack[macro_expansion_depth].macrocheck = 1;
                            macro_expansion_depth++;
                        }
                        macro_text_start = (char *)(currentTextPosition = token_start = newptr);
                        data_0057fcee = 1;
                        macrocheck = 0;
                        data_00588523 = savedbyte;
                        break;
                    }
                    currentTextPosition = savedptr;
                }
                return 1;
        }
    }
}

#pragma sym reset

void define_macro(void)
{
    Macro *definition;
    Macro *previous;
    Macro *cursor;
    HashNameNode *name;
    HashNameNode **previousArgs;
    UInt8 *start;
    UInt8 *savedPosition;
    int rangeEnd;
    SInt16 token;
    SInt16 ch;
    SInt16 argumentCount;
    SInt16 i;
    Boolean warned;
    Boolean variadic;
    Boolean paste;
    Boolean lastArgument;
    HashNameNode *arguments[64];

    data_00588470 = macrocheck = 0;
    token = CPrepTokenizer_GetToken();
    if (data_00588470) {
        remap_and_report_error(0x70);
        CPrepTokenizer_SkipToEndOfLine();
    }
    if (token != -3) {
        remap_and_report_error(0x6b);
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }

    previous = find_macro();
    name = data_00587fa0;
    warned = 0;
    variadic = 0;

    if (CPrepTokenizer_PeekChar() == '(') {
        argumentCount = 1;
        currentTextPosition = (UInt8 *)lookahead_position;
        previousArgs = previous ? previous->args : NULL;
        do {
            token = CPrepTokenizer_GetToken();
            if (token != -3)
                break;
            if (data_00588470) {
                remap_and_report_error(0x70);
                return;
            }
            if (!strcmp(data_00587fa0->name, "__VA_ARGS__")) {
                CPrep_ErrorName(0x16d, data_00587fa0->name);
                return;
            }
            for (i = 1; i < argumentCount; i++) {
                if (!strcmp(data_00587fa0->name, arguments[i - 1]->name)) {
                    CPrep_ErrorName(0x16d, data_00587fa0->name);
                    return;
                }
            }
            arguments[argumentCount - 1] = data_00587fa0;
            if (previous) {
                if ((previous->nargs & 0x7fff) < argumentCount)
                    CPrep_MacroRedefError(name->name, &warned);
                if (previousArgs[argumentCount - 1] != data_00587fa0 && !copts.cpp_extensions)
                    CPrep_MacroRedefError(name->name, &warned);
            }
            argumentCount++;
            token = CPrepTokenizer_ScanToken();
            if (data_00588470) {
                remap_and_report_error(0x70);
                return;
            }
        } while (token == ',');

        if (data_00588470) {
            remap_and_report_error(0x70);
            return;
        }
        if (token == 0x171) {
            variadic = 1;
            arguments[argumentCount - 1] = GetHashNameNodeExport("__VA_ARGS__");
            argumentCount++;
            token = CPrepTokenizer_ScanToken();
            if (data_00588470) {
                remap_and_report_error(0x70);
                return;
            }
        }
        if (token != ')') {
            remap_and_report_error(0x6d);
            CPrepTokenizer_SkipToEndOfLine();
            return;
        }
        definition = galloc(sizeof(Macro) + (argumentCount - 2) * sizeof(HashNameNode *));
    } else {
        definition = galloc(sizeof(Macro) - sizeof(HashNameNode *));
        argumentCount = 0;
    }

    if (previous && (previous->nargs & 0x7fff) != argumentCount)
        CPrep_MacroRedefError(name->name, &warned);

    definition->name = name;
    definition->nargs = variadic ? (argumentCount | 0x8000) : argumentCount;
    definition->flag = 0;
    definition->isExpanding = 0;
    for (i = 1; i < argumentCount; i++)
        definition->args[i - 1] = arguments[i - 1];

    macro_text.size = 0;
    if (CPrep_ScanMacroExpandedChar()) {
        for (;;) {
            paste = 0;
        pasted:
            lastArgument = 0;
            start = currentTextPosition;
            data_00588523 = 0;
            switch (ch = CPrepTokenizer_NextChar()) {
                case 0:
                    remap_and_report_error(0x66);
                    break;
                case '"':
                case '\'':
                    CPrepTokenizer_SkipToDelimiter(ch);
                    AppendGListData(&macro_text, start, currentTextPosition - start);
                    break;
                case '#':
                    savedPosition = currentTextPosition;
                    if (CPrepTokenizer_NextChar() == '#')
                        remap_and_report_error(0x75);
                    currentTextPosition = savedPosition;
                    if (CPrep_ScanMacroExpandedChar()) {
                        ch = CPrepTokenizer_NextChar();
                        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_') {
                            currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
                            for (i = 1; i < argumentCount; i++) {
                                if (data_00587fa0 == definition->args[i - 1])
                                    break;
                            }
                            if (i < argumentCount) {
                                AppendGListByte(&macro_text, 3);
                                AppendGListByte(&macro_text, i);
                                break;
                            }
                        }
                    }
                    if (copts.ANSIstrict)
                        remap_and_report_error(0x75);
                    AppendGListByte(&macro_text, '#');
                    currentTextPosition = savedPosition;
                    break;
                default:
                    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_') {
                        currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
                        for (i = 1; i < argumentCount; i++) {
                            if (data_00587fa0 == definition->args[i - 1])
                                break;
                        }
                        if (i < argumentCount) {
                            AppendGListByte(&macro_text, paste ? (char)2 : (char)1);
                            AppendGListByte(&macro_text, i);
                            lastArgument = 1;
                        } else {
                            AppendGListName(&macro_text, data_00587fa0->name);
                        }
                    } else {
                        AppendGListByte(&macro_text, ch);
                    }
                    break;
            }

            if (!CPrep_ScanMacroExpandedChar())
                break;
            savedPosition = currentTextPosition;
            if (CPrepTokenizer_NextChar() == '#' && CPrepTokenizer_NextChar() == '#') {
                if (!CPrep_ScanMacroExpandedChar())
                    remap_and_report_error(0x75);
                if (lastArgument)
                    (*macro_text.data)[macro_text.size - 2] = 2;
                paste = 1;
                goto pasted;
            }
            currentTextPosition = savedPosition;
            if (data_00588523)
                AppendGListByte(&macro_text, ' ');
        }
    }

    macrocheck = 1;
    if (macro_text.size > 0) {
        SInt32 length;
        AppendGListByte(&macro_text, 0);
        length = macro_text.size;
        if (length > 0x20000) {
            remap_and_report_error(0x6f);
            return;
        }
        if (previous && (!previous->text || memcmp(*macro_text.data, previous->text, length)))
            CPrep_MacroRedefError(name->name, &warned);
        definition->text = galloc(macro_text.size);
        memcpy(definition->text, *macro_text.data, macro_text.size);
    } else {
        definition->text = NULL;
        if (previous && previous->text)
            CPrep_MacroRedefError(name->name, &warned);
    }

    if (!previous || warned) {
        definition->next = macro_buckets[definition->name->hashval];
        macro_buckets[definition->name->hashval] = definition;
        if (((struct CPrepCU *)cprep_cu)->browseOptions.browseMacros && currentPFile->recordbrowseinfo) {
            rangeEnd = (char *)currentTextPosition - data_00587fb0 + 1;
            write_identifier_range_record(definition, currentPFile, data_0057fd0c, rangeEnd);
        }
        if (warned) {
            cursor = macro_buckets[definition->name->hashval];
            for (;;) {
                if (cursor->next == previous) {
                    cursor->next = cursor->next->next;
                    break;
                }
                CError_ASSERT(2854, cursor = cursor->next);
            }
        }
    }
}

#pragma sym off

void undefine_macro(void)
{
    Macro *entry;
    Boolean savedFlag;
    short token;
    Macro **link;
    HashNameNode *key;

    data_00588470 = 0;
    token = CPrepTokenizer_GetToken();
    if (data_00588470 != 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        data_005884fd = savedFlag;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    if (token != -3) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        data_005884fd = savedFlag;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    key = data_00587fa0;
    link = macro_bucket_link(key);
    entry = *link;
    while (entry != NULL) {
        if (entry->name == key) {
            *link = entry->next;
            macrocheck = 1;
            return;
        }
        link = &entry->next;
        entry = entry->next;
    }
    token = CPrep_ScanMacroExpandedChar();
    if (token != 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = savedFlag;
        CPrepTokenizer_SkipToEndOfLine();
    }
}

Boolean evaluate_pragma_option(void)
{
    OptionEntry *option;
    SInt32 offset;
    Boolean result;

    if (CPrep_ScanMacroExpandedChar() == 0) {
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        return 0;
    }
    if (CPrepTokenizer_ScanToken() != 0x28) {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return 0;
    }
    if (CPrep_ScanMacroExpandedChar() == 0) {
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        return 0;
    }
    if (CPrepTokenizer_GetToken() != -3) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return 0;
    }

    if (memcmp(data_00587fa0->name, "scheduling", 11) == 0) {
        result = copts.instructionSchedulingMode != 0;
    } else if (memcmp(data_00587fa0->name, "floatingpoint", 14) == 0) {
        result = copts.debugEnabled;
    } else if (memcmp(data_00587fa0->name, "sfp_emulation", 14) == 0) {
        result = copts.operandsDebug;
    } else if (memcmp(data_00587fa0->name, "precompile", 11) == 0) {
        result = cprep_cu[0xe0] == 1;
    } else if (memcmp(data_00587fa0->name, "preprocess", 11) == 0) {
        result = cprep_cu[0xe2];
    } else {
        for (option = pragma_options; option->name != NULL; option++) {
            if (strcmp(data_00587fa0->name, option->name) == 0) {
                offset = option->flags & 0x1fff;
                result = ((UInt8 *)&copts)[offset];
                if (offset == 0x75 && copts.cplusplus == 0)
                    result = 0;
                goto option_found;
            }
        }
        result = 0;
    option_found:;
    }

    if (CPrep_ScanMacroExpandedChar() == 0) {
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        return 0;
    }
    if (CPrepTokenizer_ScanToken() != 0x29) {
        CError_ReportError(ERR_RPAREN_EXPECTED);
        return 0;
    }
    return result;
}

UInt8 *find_identifier_end_after_optional_paren(UInt8 *text)
{
    char space, parenSpace;

    while ((space = *text) == ' ' || (9 <= space && space <= 12)) {
        ++text;
    }
    if (space == '(') {
        do {
            ++text;
        } while ((parenSpace = *text) == ' ' || (9 <= parenSpace && parenSpace <= 12));
    }
    if (((char)*text >= 'a' && (char)*text <= 'z') || ((char)*text >= 'A' && (char)*text <= 'Z') || *text == '_') {
        return CPrepTokenizer_ScanIdentifier(text);
    }
    return NULL;
}

char *expand_macros_in_text(Macro *state, char *text)
{
    short character;
    short initialDepth;
    UInt8 *token;
    UInt8 *quotedText;
    UInt8 *remainingText;
    Macro *expansion;
    SInt32 length;
    UInt8 *next;
    char *savedText;
    int split;
    SInt32 size;

    if (macro_expansion_depth >= 0x80) {
        data_0057f9dc = 1;
        CError_FatalError(ERR_MACROS_TOO_COMPLEX);
    } else {
        CPrep_PushState(state);
    }
    macro_text_start = (char *)(token_start = (UInt8 *)text);
    initialDepth = macro_expansion_depth;
    currentTextPosition = (UInt8 *)text;
    macro_text.size = 0;

    goto checkDepth;
    for (;;) {
        switch ((char)*currentTextPosition) {
            case 0:
                break;

            case 4:
                AppendGListByte(&macro_text, 4);
                currentTextPosition++;
                currentTextPosition = CPrepTokenizer_ScanIdentifier((token = currentTextPosition));
                if (data_0057f9de != 0) {
                    if (memcmp(data_00587fa0->name, "defined", 8) == 0) {
                        next = find_identifier_end_after_optional_paren(currentTextPosition);
                        if (next != NULL)
                            currentTextPosition = next;
                    }
                }
                AppendGListData(&macro_text, token, currentTextPosition - token);
                continue;

            case '"':
            case '\'':
                quotedText = currentTextPosition++;
                CPrepTokenizer_SkipToChar((char)*quotedText);
                AppendGListData(&macro_text, quotedText, currentTextPosition - quotedText);
                continue;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':
            case '_':
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
                currentTextPosition = CPrepTokenizer_ScanIdentifier((token = currentTextPosition));
                data_0057f9d2 = 0;
                if (data_0057f9de != 0) {
                    if (memcmp(data_00587fa0->name, "defined", 8) == 0) {
                        next = find_identifier_end_after_optional_paren(currentTextPosition);
                        if (next != NULL) {
                            currentTextPosition = next;
                            AppendGListData(&macro_text, token, next - token);
                            continue;
                        }
                    }
                }
                if (state != NULL)
                    expansion = find_expandable_macro(currentTextPosition);
                else
                    expansion = find_macro_for_expansion(currentTextPosition);
                if (expansion != NULL) {
                    AppendGListByte(&macro_text, 5);
                    length = macro_text.size;
                    if (length > 0x20000) {
                        token_start = currentTextPosition;
                        CPrep_Fatal();
                        return "";
                    }
                    savedText = aalloc(length);
                    memcpy(savedText, *macro_text.data, length);
                    token = expand_macro(expansion);
                    if (macro_expansion_depth == initialDepth) {
                        split = (int)strlen((char *)token) - 1;
                        for (; split > 0; split--) {
                            switch ((char)token[split]) {
                                case ' ':
                                case '"':
                                case '\'':
                                case ';':
                                    break;
                                default:
                                    continue;
                            }
                            break;
                        }
                        if (split > 0) {
                            macro_text.size = 0;
                            AppendGListName(&macro_text, (char *)&token[split + 1]);
                            AppendGListID(&macro_text, (char *)currentTextPosition);
                            currentTextPosition = aalloc(macro_text.size);
                            memcpy(currentTextPosition, *macro_text.data, macro_text.size);
                            macro_text.size = 0;
                            AppendGListData(&macro_text, savedText, length);
                            AppendGListData(&macro_text, token, split + 1);
                        } else {
                            macro_text.size = 0;
                            AppendGListName(&macro_text, (char *)token);
                            AppendGListID(&macro_text, (char *)currentTextPosition);
                            remainingText = aalloc(macro_text.size);
                            memcpy(remainingText, *macro_text.data, macro_text.size);
                            macro_text.size = 0;
                            currentTextPosition = remainingText;
                            AppendGListData(&macro_text, savedText, length);
                        }
                        goto checkDepth;
                    } else {
                        macro_text.size = 0;
                        AppendGListData(&macro_text, savedText, length);
                        AppendGListID(&macro_text, (char *)token);
                        if (macro_expansion_depth < initialDepth)
                            goto allocateResult;
                        macro_text.size--;
                        continue;
                    }
                } else {
                    if (data_0057f9d2 != 0)
                        AppendGListByte(&macro_text, 4);
                    AppendGListData(&macro_text, token, currentTextPosition - token);
                }
                continue;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                AppendGListByte(&macro_text, *currentTextPosition++);
                for (;;) {
                    character = (char)*currentTextPosition;
                    if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                        character == '_' || (character >= '0' && character <= '9')) {
                        AppendGListByte(&macro_text, character);
                        currentTextPosition++;
                    } else {
                        break;
                    }
                }
                continue;

            default:
                AppendGListByte(&macro_text, *currentTextPosition++);
                continue;
        }
        if (macro_expansion_depth >= initialDepth) {
            pop_macro_state();
            data_00588523 = 1;
        }
    checkDepth:
        if (macro_expansion_depth >= initialDepth)
            continue;
        break;
    }

    AppendGListByte(&macro_text, 0);
allocateResult:
    size = macro_text.size;
    if (size > 0x20000) {
        CPrep_Fatal();
        return "";
    }
    savedText = aalloc(size);
    memcpy(savedText, *macro_text.data, size);
    return savedText;
}

static void pop_macro_state(void)
{
    macro_expansion_depth--;
    if (macro_expansion_depth == 0 && data_0057fcee != 0)
        freeaheap();
    currentTextPosition = (UInt8 *)macro_stack[macro_expansion_depth].pos;
    macro_text_start = macro_stack[macro_expansion_depth].macname;
    if (macro_stack[macro_expansion_depth].macro != NULL)
        macro_stack[macro_expansion_depth].macro->isExpanding = 0;
    macrocheck = macro_stack[macro_expansion_depth].macrocheck;
}

static void CPrep_PushState(void *obj)
{
    macro_stack[macro_expansion_depth].pos = (char *)currentTextPosition;
    macro_stack[macro_expansion_depth].macname = macro_text_start;
    macro_stack[macro_expansion_depth].macro = obj;
    if (obj != (void *)0)
        *((char *)obj + 0xf) = 1;
    macro_stack[macro_expansion_depth].macrocheck = macrocheck;
    macro_expansion_depth++;
}

static void CPrep_Fatal(void)
{
    UInt8 save = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_ReportError(ERR_MACROS_TOO_COMPLEX);
    data_005884fd = save;
}

char *CPrep_GetFileName(char *param1, Boolean param2, Boolean param3)
{
    UInt8 buf[256];
    int len;
    char *p;
    int i;

    COS_FileGetFSSpecInfo(&data_0057f94a[param2 ? 0 : current_file_index]->textfile, NULL, NULL, buf);
    len = buf[0];
    if (param1 == NULL)
        param1 = galloc(len + 3);
    p = param1;
    if (param3)
        *p++ = '"';
    i = 0;
    while (i < len) {
        i++;
        *p++ = buf[i];
    }
    if (param3)
        *p++ = '"';
    *p = 0;
    return param1;
}

char *expand_builtin_macro(Macro *macro)
{
    char buffer[256];
    char datePart[32];
    UInt8 filename[256];
    char *cursor;
    int length;
    int index;
    char *text;
    short enabled;
    Boolean result;

    switch (*((UInt8 *)macro + 0xe)) {
        case 1:
            sprintf(buffer, "%ld", data_00587ef0);
        copy_ret:
            {
                unsigned long textLength;
                textLength = strlen(buffer);
                text = aalloc(textLength + 1);
                strcpy(text, buffer);
                return text;
            }

        case 2:
            cursor = buffer;
            COS_FileGetFSSpecInfo(&data_0057f94a[current_file_index]->textfile, NULL, NULL, filename);
            length = filename[0];
            if (cursor == NULL)
                cursor = galloc(length + 3);
            text = cursor;
            *cursor = '"';
            text++;
            index = 0;
            while (index < length) {
                index++;
                *text++ = filename[index];
            }
            *text = '"';
            text[1] = 0;
            goto copy_ret;

        case 3: {
            struct tm *time = localtime(&data_0057f9d4);
            strftime(buffer, 64, "\"%b ", time);
            strftime(datePart, sizeof(datePart), "%d", time);
            if (datePart[0] == '0')
                datePart[0] = ' ';
            strcat(buffer, datePart);
            strftime(datePart, sizeof(datePart), " %Y\"", time);
            strcat(buffer, datePart);
        }
            goto copy_ret;

        case 4:
            strftime(buffer, 64, "\"%H:%M:%S\"", localtime(&data_0057f9d4));
            goto copy_ret;

        case 6:
        case 0xd:
            return "0x2301";

        case 5:
        case 0xe:
        case 0xf:
        case 0x12:
        case 0x15:
            return "1";

        case 7:
        case 8:
        case 9:
            return "0";

        case 0xb:
        case 0xc:
            return "1";

        case 0x13:
            return "1";

        case 0x21:
            return "10205";

        case 0x20:
            return "100000000";

        case 0xa:
            return "199711L";

        case 0x10:
            enabled = copts.profile;
        bool_ret:
            if (enabled)
                return "1";
            return "0";

        case 0x11:
            result = evaluate_pragma_option();
            enabled = result;
            goto bool_ret;

        case 0x14:
            enabled = copts.direct_to_som;
            goto bool_ret;

        case 0x22:
            enabled = copts.ecplusplus;
            goto bool_ret;

        default:
            CError_FATAL(3495);
            return "";
    }
}

UInt8 *expand_macro(Macro *macro)
{
    char *text;
    Boolean variadicArgument;
    SInt16 argument;
    char **arguments;
    char **expandedArguments;
    SInt32 argumentCount;
    UInt8 *savedInput;
    SInt32 parameterCount;
    Boolean variadic;
    UInt16 nargs;
    SInt16 argumentIndex;
    char *result;
    char *argumentText;
    SInt16 character;
    SInt32 length;
    UInt8 *tokenStart;
    SInt16 token;
    SInt16 depth;
    SInt16 firstToken;

    if (macro->flag)
        return (UInt8 *)expand_builtin_macro(macro);

    nargs = macro->nargs & 0x7fff;
    variadic = (macro->nargs & 0x8000) != 0;
    if (nargs != 0) {
        skip_line_breaks_and_expand_macros();
        CError_ASSERT(3520, CPrepTokenizer_NextChar() == '(');

        if (nargs > 1) {
            variadicArgument = variadic && nargs == 2;
            parameterCount = (argumentCount = nargs) - 1;
            length = parameterCount * sizeof(char *);
            arguments = aalloc(length);
            expandedArguments = aalloc(length);
            argument = 1;
            for (; argument < argumentCount;) {
                firstToken = 1;
                depth = 0;
                macro_text.size = 0;
                for (;;) {
                    data_00588523 = 0;
                    for (;;) {
                        token = CPrepTokenizer_NextChar();
                        switch (token) {
                            case 13:
                                if (macro_expansion_depth == 0) {
                                    if (data_0058850f != 0)
                                        fn_004d6ed0();
                                    data_00587ef0++;
                                    line_count++;
                                    if (current_file_index <= 0)
                                        text_offset = currentTextPosition - (UInt8 *)data_0057f94a[0]->textbuffer;
                                }
                                data_00588523 = 1;
                                continue;
                            case 9:
                            case 10:
                            case 11:
                            case 12:
                            case 32:
                                data_00588523 = 1;
                                continue;
                            case 0:
                                if (macro_expansion_depth != 0) {
                                    CPrep_PopState();
                                    data_00588523 = 1;
                                    continue;
                                }
                                CPrep_Error(0x66);
                                goto argumentFinished;
                            case 0x22:
                            case 0x27:
                                tokenStart = currentTextPosition - 1;
                                CPrepTokenizer_SkipToChar(token);
                                AppendGListData(&macro_text, tokenStart, currentTextPosition - tokenStart);
                                goto tokenFinished;
                            case ')':
                                depth--;
                                if (depth < 0) {
                                    argument++;
                                    if (argument < argumentCount) {
                                        token_start = currentTextPosition;
                                        CPrep_Error(0x74);
                                        return (UInt8 *)"";
                                    }
                                    goto argumentFinished;
                                }
                                break;
                            case ',':
                                if (depth <= 0 && !variadicArgument) {
                                    argument++;
                                    if (variadic && argument == parameterCount) {
                                        variadicArgument = 1;
                                    } else if (argument >= argumentCount) {
                                        token_start = currentTextPosition;
                                        CPrep_Error(0x73);
                                        return (UInt8 *)"";
                                    }
                                    goto argumentFinished;
                                }
                                break;
                            case '(':
                                depth++;
                            default:
                                break;
                        }
                        if (data_00588523 != 0 && !firstToken)
                            AppendGListByte(&macro_text, 0x20);
                        AppendGListByte(&macro_text, token);
                        firstToken = 0;
                    tokenFinished:
                        break;
                    }
                }
            argumentFinished:
                expandedArguments[argument - 2] = arguments[argument - 2] = NULL;
                if (macro_text.size > 0) {
                    AppendGListByte(&macro_text, 0);
                    length = macro_text.size;
                    if (length <= 0x20000) {
                        arguments[argument - 2] = aalloc(length);
                        memcpy(arguments[argument - 2], *macro_text.data, length);
                    } else {
                        CPrep_Error(0x6f);
                    }
                }
            }
        } else {
            skip_line_breaks_and_expand_macros();
            if (CPrepTokenizer_NextChar() != ')') {
                token_start = currentTextPosition;
                CPrep_Error(0x73);
            }
        }
    }

    if ((text = macro->text) == NULL)
        return (UInt8 *)"";
    if (nargs <= 1 && !variadic)
        return (UInt8 *)expand_macros_in_text(macro, text);

    macro_text.size = 0;
    savedInput = currentTextPosition;
    while ((token = *text++) != 0) {
        switch (token) {
            case 0x22:
            case 0x27: {
                currentTextPosition = (UInt8 *)text;
                text--;
                result = text;
                CPrepTokenizer_SkipToChar(token);
                text = (char *)currentTextPosition;
                currentTextPosition = savedInput;
                AppendGListData(&macro_text, result, text - result);
                continue;
            }
            case 2:
                if ((argumentText = arguments[*text++ - 1]) != NULL) {
                    for (;;) {
                        switch (*argumentText) {
                            case 0:
                                break;
                            case 4:
                            case 5:
                                argumentText++;
                                continue;
                            default:
                                AppendGListByte(&macro_text, *argumentText++);
                                continue;
                        }
                        break;
                    }
                }
                continue;
            case 3:
                AppendGListByte(&macro_text, 0x22);
                if ((argumentText = arguments[*text++ - 1]) != NULL) {
                    while ((character = *argumentText++) != 0) {
                        switch (character) {
                            case 0x22:
                                AppendGListByte(&macro_text, 0x5c);
                            case 0x27:
                                AppendGListByte(&macro_text, character);
                                while (*argumentText != 0 && *argumentText != character) {
                                    if (*argumentText == 0x22 || *argumentText == 0x5c)
                                        AppendGListByte(&macro_text, 0x5c);
                                    AppendGListByte(&macro_text, *argumentText++);
                                }
                                if (*argumentText == 0x22)
                                    AppendGListByte(&macro_text, 0x5c);
                                if (*argumentText != 0)
                                    AppendGListByte(&macro_text, *argumentText++);
                                break;
                            case 4:
                            case 5:
                                break;
                            default:
                                AppendGListByte(&macro_text, character);
                                break;
                        }
                    }
                }
                AppendGListByte(&macro_text, 0x22);
                continue;
            case 1: {
                char *savedText;
                SInt32 savedLength;
                if ((argumentText = expandedArguments[argumentIndex = *text++ - 1]) == NULL) {
                    if (arguments[argumentIndex] == NULL)
                        continue;
                    savedLength = macro_text.size;
                    savedText = aalloc(savedLength);
                    memcpy(savedText, *macro_text.data, savedLength);
                    expandedArguments[argumentIndex] = expand_macros_in_text(NULL, arguments[argumentIndex]);
                    argumentText = expandedArguments[argumentIndex];
                    macro_text.size = 0;
                    AppendGListData(&macro_text, savedText, savedLength);
                }
                while (*argumentText != 0)
                    AppendGListByte(&macro_text, *argumentText++);
                if (argumentText[-1] == 0x3e && *text == 0x3e)
                    AppendGListByte(&macro_text, 0x20);
                continue;
            }
            default:
                AppendGListByte(&macro_text, token);
        }
    }
    AppendGListByte(&macro_text, 0);
    length = macro_text.size;
    if (length > 0x20000) {
        CPrep_Error(0x6f);
        return (UInt8 *)"";
    }
    result = aalloc(length);
    memcpy(result, *macro_text.data, macro_text.size);
    return (UInt8 *)expand_macros_in_text(macro, result);
}

static void CPrep_PopState(void)
{
    macro_expansion_depth--;
    if (macro_expansion_depth == 0 && data_0057fcee)
        freeaheap();
    currentTextPosition = (void *)macro_stack[macro_expansion_depth].pos;
    macro_text_start = macro_stack[macro_expansion_depth].macname;
    if (macro_stack[macro_expansion_depth].macro)
        macro_stack[macro_expansion_depth].macro->isExpanding = 0;
    macrocheck = macro_stack[macro_expansion_depth].macrocheck;
}

UInt8 CPrep_ExpandMacro(void)
{
    UInt8 *expandedText;
    Macro *macro;
    unsigned char savedExpansionState;

    macro = lookup_expandable_macro();
    if (macro != NULL) {
        data_0057fcee = macrocheck = 0;
        savedExpansionState = data_00588523;
        expandedText = expand_macro(macro);
        macrocheck = 1;
        if (128 <= macro_expansion_depth) {
            data_0057f9dc = 1;
            CError_FatalError(ERR_MACROS_TOO_COMPLEX);
        } else {
            macro_stack[macro_expansion_depth].pos = (char *)currentTextPosition;
            macro_stack[macro_expansion_depth].macname = macro_text_start;
            macro_stack[macro_expansion_depth].macro = NULL;
            macro_stack[macro_expansion_depth].macrocheck = 1;
            ++macro_expansion_depth;
        }
        macro_text_start = (char *)(currentTextPosition = token_start = expandedText);
        data_0057fcee = 1;
        macrocheck = 0;
        data_00588523 = savedExpansionState;
        return 1;
    }
    return 0;
}

SInt16 CPrep_ExpectEndLine(char suppressDiagnostic)
{
    Boolean savedFlag;

    if ((short)CPrep_ScanMacroExpandedChar() != 0) {
        return CPrepTokenizer_ScanToken();
    }
    if (suppressDiagnostic == '\0') {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = savedFlag;
    }
    return 0;
}

void CPrep_SaveAndSetOption(unsigned int index, UInt8 value)
{
    IROOptNode *entry = saved_options;
    for (;;) {
        if (!entry || (entry->flag && entry->code == index)) {
            IROOptNode *new_entry = (IROOptNode *)galloc(sizeof(*new_entry));
            new_entry->next = saved_options;
            saved_options = new_entry;
            entry = new_entry;
            break;
        }
        if (!entry->flag)
            break;
        entry = entry->next;
    }

    entry->value = *CPrep_OptionAddress(index);
    entry->code = index;
    entry->flag = 1;
    *CPrep_OptionAddress(index) = value;
}

/* The option key is a byte offset into CompilerLinkerOptions, not a fixed member index. */
/* The option key is a byte offset into CompilerLinkerOptions, not a fixed member index. */
void CPrep_RestoreOption(int optionOffset)
{
    IROOptNode *entry;

    for (entry = saved_options; entry; entry = entry->next) {
        if (entry->flag && entry->code == optionOffset) {
            unsigned char *options = (unsigned char *)&copts;
            options[optionOffset] = entry->value;
            entry->flag = 0;
            return;
        }
    }
    {
        unsigned char *options = (unsigned char *)&copts;
        options[optionOffset] = *((const unsigned char *)data_0057fcea + optionOffset);
    }
}

void apply_pragma_object_flags(unsigned int flags)
{
    unsigned char saved;
    ObjectList *node;
    for (;;) {
        if (CPrep_ScanMacroExpandedChar() == 0)
            break;
        if (CPrepTokenizer_ScanToken() != -3)
            break;
        node = CScope_GetLocalObject(cscope_root, data_00587fa0);
        if (node == NULL) {
            break;
        } else {
            while (node != NULL) {
                if (node->object->otype == 5) {
                    switch (node->object->datatype) {
                        case DDATA:
                        case DFUNC:
                            node->object->flags |= flags;
                            break;
                        default:
                            saved = data_005884fd;
                            data_005884fd = 0;
                            data_0057f9dc = 1;
                            CError_Warning(ERR_ILLEGAL_PRAGMA);
                            data_005884fd = saved;
                            return;
                    }
                }
                node = node->next;
            }
        }
        if (CPrep_ScanMacroExpandedChar() == 0)
            return;
        if (CPrepTokenizer_ScanToken() != 0x2c)
            break;
        if (CPrep_ScanMacroExpandedChar() == 0)
            break;
    }
    if (copts.warn_illpragma != 0) {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = saved;
    }
}

void parse_pragma_option(int directive)
{
    unsigned int option;
    int alignment;
    char savedFlag;
    char allowSpecial = (directive & 8192) != 0;
    option = directive & 8191;

    if (CPrep_ScanMacroExpandedChar() != 0 && CPrepTokenizer_ScanToken() == -3) {
        if (memcmp(data_00587fa0->name, "on", 3) == 0) {
            CPrep_SaveAndSetOption(option, 1);
            return;
        }
        if (memcmp(data_00587fa0->name, "off", 4) == 0) {
            CPrep_SaveAndSetOption(option, 0);
            return;
        }
        if (memcmp(data_00587fa0->name, "reset", 6) == 0) {
            CPrep_0043bf00_inline1(option);
            return;
        }
        if (allowSpecial && memcmp(data_00587fa0->name, "list", 5) == 0) {
            struct PragmaSettings *settings = (struct PragmaSettings *)cprep_cu;
            if (settings->mode != 0) {
                CPrepTokenizer_SkipToEndOfLine();
                return;
            }
            macrocheck = 1;
            if (option == 187)
                alignment = 16;
            else if (option == 188)
                alignment = 32;
            else if (option == 189)
                alignment = 64;
            else if (option == 190)
                alignment = 96;
            else
                CError_FATAL(3833);
            apply_pragma_object_flags(alignment);
            return;
        }
    }
    if (copts.warn_illpragma != 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = savedFlag;
    }
}

void fn_0043be10(void)
{
    NameSpace *scope;
    ObjectList *entry;
    Object *record;
    SInt16 token;
    unsigned char saved;

    if (((struct PragmaSettings *)cprep_cu)->mode != 0U) {
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    if (cscope_currentfunc != NULL && CPrep_ScanMacroExpandedChar() != 0 && CPrepTokenizer_ScanToken() == 40 &&
        CPrep_ScanMacroExpandedChar() != 0) {
    next_name:
        if (CPrepTokenizer_ScanToken() == -3 && (scope = cscope_current) != NULL) {
            do {
                if (scope->is_global)
                    break;
                entry = CScope_GetLocalObject(scope, data_00587fa0);
                if (entry != NULL) {
                    if ((record = entry->object)->otype == 5U && record->datatype == 1U) {
                        record->flags |= 1U;
                        if (CPrep_ScanMacroExpandedChar() != 0) {
                            token = CPrepTokenizer_ScanToken();
                            if (token == 41)
                                return;
                            if (token == 44 && CPrep_ScanMacroExpandedChar() != 0)
                                goto next_name;
                        }
                        break;
                    }
                }
                scope = scope->parent;
            } while (scope != NULL);
        }
    }
    if (copts.warn_illpragma != 0U) {
        saved = data_005884fd;
        data_005884fd = 0U;
        data_0057f9dc = 1U;
        CError_Warning(186U);
        data_005884fd = saved;
    }
}

void parse_inline_limit(void)
{
    SInt32 limit;

    if (CPrep_ScanMacroExpandedChar() && CPrepTokenizer_ScanToken() == '(' && CPrep_ScanMacroExpandedChar()) {
        switch (CPrepTokenizer_ScanToken()) {
            case -3:
                if (memcmp(data_00587fa0->name, "smart", 6) != 0)
                    PREP_ERR(0xba);
                else
                    copts.inlineLimit = 0;
                break;
            case -1:
                limit = intconst_lo;
                if (limit >= 0 && limit <= 1024) {
                    if (limit == 0)
                        limit = -1;
                    copts.inlineLimit = limit;
                } else {
                    PREP_ERR(0xba);
                }
                break;
            default:
                PREP_ERR(0xba);
                CPrepTokenizer_SkipToEndOfLine();
                return;
        }
        if (CPrep_ScanMacroExpandedChar() == 0 || CPrepTokenizer_ScanToken() != ')')
            PREP_ERR(0x73);
        if (CPrep_ScanMacroExpandedChar()) {
            PREP_ERR(0x71);
            CPrepTokenizer_SkipToEndOfLine();
        }
        return;
    }
    if (copts.warn_illpragma)
        PREP_ERR(0xba);
}

void read_pragma_token(void)
{
    char buffer[256];
    HashNameNode *token;
    short ch;
    short length;
    unsigned char saved;

    if (CPrep_ScanMacroExpandedChar() != 0) {
        length = 0;
        do {
            data_00588523 = 0;
            ch = CPrepTokenizer_ScanChar();
            if (data_00588523 != 0 || ch <= 32)
                break;
            buffer[length++] = ch;
            currentTextPosition = (UInt8 *)lookahead_position;
        } while (length < 255);
        buffer[length] = 0;
        if (length == 0 || length >= 255) {
            saved = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_Warning(ERR_ILLEGAL_PRAGMA);
            data_005884fd = saved;
        }
        token = GetHashNameNode(buffer);
        copts.fe4 = (SInt32)token;
        fn_0048b1e0(token);
        return;
    }
    if (copts.warn_illpragma != 0) {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = saved;
    }
}

void parse_align_pragma(void)
{
    IROOptNode *option;

    if (CPrep_ScanMacroExpandedChar() && CPrepTokenizer_ScanToken() == -3 &&
        memcmp(data_00587fa0->name, "align", 6) == 0 && CPrep_ScanMacroExpandedChar() &&
        CPrepTokenizer_ScanToken() == 0x3d && CPrep_ScanMacroExpandedChar() && CPrepTokenizer_ScanToken() == -3) {
        if (memcmp(data_00587fa0->name, "reset", 6) == 0) {
            for (option = saved_options; option; option = option->next) {
                if (option->flag && option->code == 0xad) {
                    copts.structalignment = option->value;
                    option->flag = 0;
                    goto done;
                }
            }
            copts.structalignment = data_0057fcea->structalignment;
        } else if (memcmp(data_00587fa0->name, "native", 7) == 0) {
            CPrep_SaveAndSetOption(0xad, 2);
        } else if (memcmp(data_00587fa0->name, "mac68k", 7) == 0) {
            CPrep_SaveAndSetOption(0xad, 0);
        } else if (memcmp(data_00587fa0->name, "mac68k4byte", 12) == 0) {
            CPrep_SaveAndSetOption(0xad, 1);
        } else if (memcmp(data_00587fa0->name, "power", 6) == 0) {
            CPrep_SaveAndSetOption(0xad, 2);
        } else if (memcmp(data_00587fa0->name, "packed", 7) == 0) {
            CPrep_SaveAndSetOption(0xad, 8);
        } else {
            goto invalid;
        }
    } else {
    invalid:
        if (copts.warn_illpragma) {
            UInt8 savedSetting = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_Warning(ERR_ILLEGAL_PRAGMA);
            data_005884fd = savedSetting;
        }
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }

done:
    if (CPrep_ScanMacroExpandedChar()) {
        UInt8 savedSetting = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = savedSetting;
        CPrepTokenizer_SkipToEndOfLine();
    }
}

void fn_0043b790(void)
{
    Object *obj;
    UInt8 saved;

    if (cprep_cu[0xe2] != 0) {
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    data_00588470 = 0;
    tk = CPrepTokenizer_GetNextToken();
    obj = CParser_ParseObject();
    if (obj != NULL) {
        if (obj->sclass != TK_EOF && obj->sclass != TK_EXTERN) {
            saved = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_ILLEGAL_STORAGE_CLASS);
            data_005884fd = saved;
        }
        obj->qual |= Q_WEAK;
    } else {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_ILLEGAL_PRAGMA);
        data_005884fd = saved;
    }
    if (data_00588470 != 0) {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        data_005884fd = saved;
    }
    if (tk != ';') {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
        data_005884fd = saved;
    }
}

void parse_optimization_level_pragma(void)
{
    Boolean saved;
    short n;

    if ((SInt16)CPrep_ScanMacroExpandedChar() != 0) {
        n = CPrepTokenizer_ScanToken();
        if (n == -1) {
            SInt32 v = intconst_lo;
            if (v >= 0 && v <= 4) {
                CPrep_SaveAndSetOption(0xc1, v);
                IrOptimizer_SetDeleteDeadInstructionsFlags();
                CodeGen_SetIROptimizationEnabled();
                return;
            }
            saved = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_NUMBER_OUT_RANGE);
            data_005884fd = saved;
            CPrepTokenizer_SkipToEndOfLine();
            return;
        }
        if (n == -3 && memcmp(data_00587fa0->name, "reset", 6) == 0) {
            CPrep_RestoreOptimizationLevel();
            IrOptimizer_SetDeleteDeadInstructionsFlags();
            CodeGen_SetIROptimizationEnabled();
            return;
        }
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_ILLEGAL_TOKEN);
        data_005884fd = saved;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    if (copts.warn_illpragma != 0) {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = saved;
    }
    CPrepTokenizer_SkipToEndOfLine();
}

void parse_unroll_pragma(void)
{
    Boolean saved;
    short token;

    if ((SInt16)CPrep_ScanMacroExpandedChar() != 0) {
        token = CPrepTokenizer_ScanToken();
        if (token == -1) {
            SInt32 value = intconst_lo;
            if (value >= 0 && value <= 0x7f) {
                CPrep_SaveAndSetOption(0xd0, value);
                IrOptimizer_SetDeleteDeadInstructionsFlags();
                CodeGen_SetIROptimizationEnabled();
                return;
            }
            saved = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_NUMBER_OUT_RANGE);
            data_005884fd = saved;
            CPrepTokenizer_SkipToEndOfLine();
            return;
        }
        if (token == -3 && memcmp(data_00587fa0->name, "reset", 6) == 0) {
            ApplyUnrollOption();
            IrOptimizer_SetDeleteDeadInstructionsFlags();
            CodeGen_SetIROptimizationEnabled();
            return;
        }
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_ILLEGAL_TOKEN);
        data_005884fd = saved;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    if (copts.warn_illpragma != 0) {
        saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = saved;
    }
    CPrepTokenizer_SkipToEndOfLine();
}

void parse_prep_setting(void)
{
    SInt16 token;

    token = CPrep_ScanMacroExpandedChar();
    if (token != 0) {
        token = CPrepTokenizer_ScanToken();
        if (token == -1) {
            SInt32 value = intconst_lo;
            if (value >= 0 && value <= 0x7f) {
                CPrep_SaveAndSetOption(0xd1, value);
                IrOptimizer_SetDeleteDeadInstructionsFlags();
                CodeGen_SetIROptimizationEnabled();
                return;
            }
            EMIT(CError_ReportError, 0x9a);
            CPrepTokenizer_SkipToEndOfLine();
            return;
        }
        if (token == -3) {
            if (memcmp(data_00587fa0->name, "reset", 6) == 0) {
                restorePrepSetting();
                IrOptimizer_SetDeleteDeadInstructionsFlags();
                CodeGen_SetIROptimizationEnabled();
                return;
            }
        }
        EMIT(CError_ReportError, 0x69);
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    if (copts.warn_illpragma != 0) {
        EMIT(CError_Warning, 0xba);
    }
    CPrepTokenizer_SkipToEndOfLine();
}

void parse_structalignment(void)
{
    SInt32 token;
    SInt32 index;
    Boolean didPop;
    Boolean didPush;

    if (CPrep_ScanMacroExpandedChar() != 0 && CPrepTokenizer_ScanToken() == '(') {
        macrocheck = 1;
        if (CPrep_ScanMacroExpandedChar() != 0 && (token = CPrepTokenizer_ScanToken()) == ')') {
            copts.structalignment = data_0057fcea->structalignment;
        } else {
            didPush = didPop = 0;
            for (;;) {
                if (token == -3) {
                    if (memcmp(data_00587fa0->name, "push", 5) == 0) {
                        if (data_0057fce0 != 0) {
                            data_0057fce0--;
                            saved_structalignments[data_0057fce0].obj = NULL;
                            didPush = 1;
                            saved_structalignments[data_0057fce0].align = copts.structalignment;
                        } else {
                            CPrep_0043afc0_error(0xba);
                        }
                    } else if (memcmp(data_00587fa0->name, "pop", 4) == 0) {
                        if (data_0057fce0 < 0x80) {
                            copts.structalignment = saved_structalignments[data_0057fce0].align;
                            didPop = 1;
                            data_0057fce0++;
                        } else {
                            CPrep_0043afc0_error(0xba);
                        }
                    } else {
                        if (didPush) {
                            saved_structalignments[data_0057fce0].obj = data_00587fa0;
                        } else if (didPop) {
                            index = data_0057fce0 - 1;
                            for (; 0x80 > index; index++) {
                                if (saved_structalignments[index].obj == data_00587fa0) {
                                    break;
                                }
                            }
                            if (index < 0x80) {
                                copts.structalignment = saved_structalignments[index].align;
                                data_0057fce0 = index + 1;
                            } else {
                                CPrep_0043afc0_error(0xba);
                            }
                        } else {
                            CPrep_0043afc0_error(0xba);
                        }
                        didPush = didPop = 0;
                    }
                } else {
                    if (token == -1) {
                        switch (intconst_lo) {
                            case 0:
                                copts.structalignment = data_0057fcea->structalignment;
                                break;
                            case 1:
                                copts.structalignment = 3;
                                break;
                            case 2:
                                copts.structalignment = 4;
                                break;
                            case 4:
                                copts.structalignment = 5;
                                break;
                            case 8:
                                copts.structalignment = 6;
                                break;
                            case 16:
                                copts.structalignment = 7;
                                break;
                            default:
                                warn_structalignment();
                                break;
                        }
                    }
                    didPush = didPop = 0;
                }
                if (CPrep_ScanMacroExpandedChar() != 0) {
                    token = CPrepTokenizer_ScanToken();
                }
                if (token != ',') {
                    break;
                }
                token = CPrepTokenizer_ScanToken();
            }
        }
        if (token != ')') {
            warn_structalignment();
        }
        macrocheck = 0;
    } else {
        warn_structalignment();
    }
}

static void CPrep_0043afc0_error(SInt32 code)
{
    Boolean saved = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_Warning(code);
    data_005884fd = saved;
}

/* The linker stripped the function that used these literals; they stay in the unit's .data. */
static void CPrep_CommentKindLiterals(const char **literals)
{
    literals[0] = "compiler";
    literals[1] = "exestr";
    literals[2] = "linker";
    literals[3] = "user";
    literals[4] = "lib";
}

void concatenate_and_dispatch_string_tokens(void)
{
    char buffer[128];
    char *end;
    int saved_depth;
    int token;
    struct CPrepCU *unit;

    if (CPrep_ScanMacroExpandedChar() != 0 && CPrepTokenizer_ScanToken() == '(') {
        if (CPrep_ScanMacroExpandedChar() != 0) {
            saved_depth = macro_expansion_depth;
            end = buffer;
            macrocheck = 1;
            buffer[0] = 0;
            token = CPrepTokenizer_ScanToken();
            macrocheck = 0;
            if (token == -4) {
                while (token == -4) {
                    strncpy(end, string_token_data, sizeof(buffer) - (end - buffer));
                    buffer[sizeof(buffer) - 1] = 0;
                    end = buffer + strlen(buffer);
                    macrocheck = 1;
                    if (macro_expansion_depth == saved_depth && CPrep_ScanMacroExpandedChar() == 0)
                        break;
                    macrocheck = 1;
                    token = CPrepTokenizer_ScanToken();
                    macrocheck = 0;
                }
                unit = (struct CPrepCU *)cprep_cu;
                CWPluginsPrivate_InvokeMessageCallback(unit->context, NULL, buffer, NULL, 0, 0x151);
                CPrepTokenizer_SkipToEndOfLine();
                return;
            } else {
                CPrep_ErrorBA();
                return;
            }
        } else {
            if (CPrepTokenizer_ScanToken() != ')') {
                CPrep_ErrorBA();
                return;
            }
        }
    } else {
        CPrep_ErrorBA();
        return;
    }
}

static void CPrep_ErrorBA(void)
{
    if (copts.warn_illpragma) {
        Boolean save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_Warning(ERR_ILLEGAL_PRAGMA);
        data_005884fd = save;
    }
    CPrepTokenizer_SkipToEndOfLine();
}

void parse_pragma(void)
{
    char filename[256];
    char macroName[256];
    PragmaNode *pragma;
    char *src;
    SInt16 remaining;
    UInt8 savedWarning;
    UInt8 savedErrors;
    Macro *macro;
    OptionEntry *option;
    HashNameNode *name;
    HashNameNode *lookupName;
    UInt8 isDefault;
    char *dest;
    int enabled;
    SInt16 length;

    if (!CPrep_ScanMacroExpandedChar())
        return;
    macrocheck = 0;
    if (CPrepTokenizer_GetToken() == -3) {
        if (memcmp(PN, "pack", 5) == 0) {
            parse_structalignment();
        } else if (memcmp(PN, "optimization_level", 19) == 0) {
            parse_optimization_level_pragma();
        } else if (memcmp(PN, "once", 5) == 0) {
            if (CPrep_ScanMacroExpandedChar() != 0) {
                CPrep_ParseListingOption();
            } else {
                COS_FileGetFSSpecInfo(&currentPFile->textfile, NULL, NULL, filename);
                isDefault = currentPFile->isDefault;
                length = (UInt8)filename[0];
                remaining = length;
                if (length > 0xfa)
                    remaining = 0xfa;
                dest = macroName;
                enabled = copts.syspath_once != 0 && isDefault != 0;
                *dest++ = enabled ? 0x24 : (char)0xa4;
                src = filename + 1;
                while (remaining-- > 0)
                    *dest++ = tolower(*src++);
                *dest = 0;
                name = lookupName = GetHashNameNode(macroName);
                if (lookup_available_macro(lookupName) == 0) {
                    macro = galloc(sizeof(Macro));
                    memclrw(macro, sizeof(Macro));
                    macro->name = name;
                    macro->next = macro_buckets[name->hashval];
                    macro_buckets[name->hashval] = macro;
                }
            }
        } else if (memcmp(PN, "unused", 7) == 0) {
            fn_0043be10();
        } else if (memcmp(PN, "inline_depth", 13) == 0) {
            parse_inline_limit();
        } else if (memcmp(PN, "options", 8) == 0) {
            parse_align_pragma();
            macrocheck = 1;
            return;
        } else if (memcmp(PN, "segment", 8) == 0) {
            read_pragma_token();
        } else if (memcmp(PN, "push", 5) == 0) {
            ObjGen_PPC_EABI_BuildSectionHeaderTable();
            pragma = galloc(sizeof(PragmaNode));
            pragma->next = pragma_list;
            pragma_list = pragma;
            pragma->rec = copts;
        } else if (memcmp(PN, "pop", 4) == 0) {
            if (pragma_list != NULL) {
                copts = pragma_list->rec;
                pragma_list = pragma_list->next;
                ObjGen_PPC_EABI_UpdateSectionHeaders();
                fn_004a9c70();
            } else {
                savedErrors = data_005884fd;
                data_005884fd = 0;
                data_0057f9dc = 1;
                CError_ReportError(ERR_PRECEDING_PRAGMA_PUSH_MISSING);
                data_005884fd = savedErrors;
            }
        } else if (memcmp(PN, "parameter", 10) == 0) {
            macrocheck = 1;
            CMach_PragmaParams();
        } else if (memcmp(PN, "overload", 9) == 0) {
            macrocheck = 1;
            fn_0043b790();
        } else if (memcmp(PN, "mark", 5) == 0) {
            CPrepTokenizer_SkipToEndOfLine();
            macrocheck = 1;
            return;
        } else if (memcmp(PN, "precompile_target", 18) == 0) {
            if (CPrep_ScanMacroExpandedChar() == 0) {
                savedErrors = data_005884fd;
                data_005884fd = 0;
                data_0057f9dc = 1;
                CError_ReportError(ERR_UNEXPECTED_END_LINE);
                data_005884fd = savedErrors;
            } else if (CPrepTokenizer_ScanToken() != -4) {
                savedErrors = data_005884fd;
                data_005884fd = 0;
                data_0057f9dc = 1;
                CError_ReportError(ERR_PREPROCESSOR_SYNTAX_ERROR);
                data_005884fd = savedErrors;
                CPrepTokenizer_SkipToEndOfLine();
            } else if (data_005882de != 0) {
                savedErrors = data_005884fd;
                data_005884fd = 0;
                data_0057f9dc = 1;
                CError_ReportError(ERR_ILLEGAL_STRING_CONSTANT);
                data_005884fd = savedErrors;
                CPrepTokenizer_SkipToEndOfLine();
            } else {
                data_00587e84 = string_token_data;
                if (CPrep_ScanMacroExpandedChar() != 0) {
                    savedErrors = data_005884fd;
                    data_005884fd = 0;
                    data_0057f9dc = 1;
                    CError_ReportError(ERR_END_LINE_EXPECTED);
                    data_005884fd = savedErrors;
                    CPrepTokenizer_SkipToEndOfLine();
                }
            }
        } else if (memcmp(PN, "message", 8) == 0) {
            concatenate_and_dispatch_string_tokens();
        } else if (memcmp(PN, "opt_unroll_count", 17) == 0) {
            parse_unroll_pragma();
        } else if (memcmp(PN, "opt_unroll_instr_count", 23) == 0) {
            parse_prep_setting();
        } else if (memcmp(PN, "exception_terminate", 20) == 0) {
            if (!data_0058850f)
                CExcept_Terminate();
        } else if (memcmp(PN, "exception_arrayinit", 20) == 0) {
            if (!data_0058850f)
                CExcept_ArrayInit();
        } else if (memcmp(PN, "exception_magic", 16) == 0) {
            if (!data_0058850f)
                CExcept_Magic();
        } else if (memcmp(PN, "SOMReleaseOrder", 16) == 0) {
            macrocheck = 1;
            CSOM_ParseMethodNameList();
        } else if (memcmp(PN, "SOMClassVersion", 16) == 0) {
            macrocheck = 1;
            CSOM_ParseDescriptorValues();
        } else if (memcmp(PN, "SOMMetaClass", 13) == 0) {
            macrocheck = 1;
            CSOM_ParseBaseClass();
        } else if (memcmp(PN, "SOMCallStyle", 13) == 0) {
            set_owner_target_flag();
        } else {
            for (option = pragma_options; option->name != NULL; option++) {
                if (strcmp(PN, option->name) != 0)
                    continue;
                if ((option->flags & 0x4000) != 0)
                    continue;
                parse_pragma_option(option->flags);
                if (option->flags & 0x8000)
                    fn_004a9c70();
                goto done;
            }
            {
                savedWarning = data_0058850d;
                data_0058850d = 1;
                CodeGen_ParsePragma(data_00587fa0);
                data_0058850d = savedWarning;
                macrocheck = 1;
                return;
            }
        }
    } else {
        if (copts.warn_illpragma) {
            savedErrors = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_Warning(ERR_ILLEGAL_PRAGMA);
            data_005884fd = savedErrors;
        }
        CPrepTokenizer_SkipToEndOfLine();
        macrocheck = 1;
        return;
    }
done:
    if (CPrep_ScanMacroExpandedChar() != 0) {
        savedErrors = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = savedErrors;
        CPrepTokenizer_SkipToEndOfLine();
    }
    f87_enabled = copts.dollar_identifiers != 0;
    macrocheck = 1;
}

#pragma sym reset

void fn_0043a0a0(char allowInclude)
{
    short length;
    short ch;
    short remaining;
    short alternateLength;
    int prefix;
    int alternatePrefix;
    int usePrefix;
    int useAlternatePrefix;
    union {
        int length;
        char lowByte;
    } count;
    long countValue;
    Macro *entry;
    char *nameStart;
    HashNameNode *name;
    HashNameNode *alternateName;
    char *output;
    char savedDiagnostic1;
    char savedDiagnostic2;
    char savedDiagnostic3;
    char savedDiagnostic4;
    IncludeFilenamePointer pointer;
    char *input;
    IncludeSearchPolicy policy;
    short terminator;
    char filename[256];
    char key[256];
    char alternateKey[256];

    if (CPrep_ScanMacroExpandedChar() == 0) {
        savedDiagnostic1 = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        data_005884fd = savedDiagnostic1;
        return;
    }
    terminator = CPrepTokenizer_NextChar();
    if (terminator != '"' && terminator != '<') {
        savedDiagnostic2 = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PREPROCESSOR_SYNTAX_ERROR);
        data_005884fd = savedDiagnostic2;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    policy.searchLocal = 1;
    if (terminator == '<') {
        terminator = '>';
        if (copts.flat_include == 0)
            policy.searchLocal = 0;
    }
    count.length = 0;
    ch = CPrepTokenizer_NextChar();
    while (terminator != ch) {
        if ((short)count.length > 254) {
            savedDiagnostic3 = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_STRING_TOO_LONG);
            data_005884fd = savedDiagnostic3;
            CPrepTokenizer_SkipToEndOfLine();
            return;
        }
        count.length++;
        filename[(short)count.length] = (char)ch;
        ch = CPrepTokenizer_NextChar();
    }
    countValue = count.length;
    (void)countValue;
    filename[0] = countValue;
    pointer.filename = filename;
    if (CPrep_ScanMacroExpandedChar() != 0) {
        savedDiagnostic4 = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = savedDiagnostic4;
        CPrepTokenizer_SkipToEndOfLine();
    }
    length = *(unsigned char *)filename;
    if (length > 250)
        length = 250;
    output = key;
    usePrefix = copts.syspath_once != 0 && (char)!policy.searchLocal;
    if (usePrefix)
        prefix = '$';
    else
        prefix = -92;
    *output = prefix;
    nameStart = filename;
    nameStart = nameStart + 1;
    output++;
    input = nameStart;
    while (length-- > 0) {
        *output = (char)tolower(*input++);
        output++;
    }
    *output = 0;
    if (lookup_available_macro(GetHashNameNode(key)) == 0) {
        if (allowInclude != 0 || copts.always_import != 0) {
            alternateLength = *(unsigned char *)filename;
            if (alternateLength > 250)
                alternateLength = 250;
            output = alternateKey;
            useAlternatePrefix = copts.syspath_once != 0 && (char)!policy.searchLocal;
            if (useAlternatePrefix)
                alternatePrefix = '$';
            else
                alternatePrefix = -92;
            *output = (char)alternatePrefix;
            output++;
            remaining = alternateLength;
            while (remaining-- > 0) {
                *output = (char)tolower(*nameStart++);
                output++;
            }
            *output = 0;
            name = alternateName = GetHashNameNode(alternateKey);
            if (lookup_available_macro(alternateName) == 0) {
                entry = (Macro *)galloc(sizeof(Macro));
                memclrw(entry, sizeof(Macro));
                entry->name = name;
                entry->next = macro_buckets[name->hashval];
                macro_buckets[name->hashval] = entry;
            }
        }
        if (copts.flat_include != 0) {
            for (length = count.length; length > 0; length--) {
                switch (filename[length]) {
                    case '/':
                    case ':':
                    case '\\':
                        pointer.filename = filename;
                        pointer.filename = pointer.filename + length;
                        count.lowByte -= length;
                        countValue = count.length;
                        (void)countValue;
                        filename[length] = countValue;
                        break;
                    default:
                        continue;
                }
                break;
            }
        }
        ((unsigned char (*)(IncludeFilenamePointer, IncludeSearchPolicy, unsigned char))fn_004401b0)(pointer, policy,
                                                                                                     allowInclude);
    }
}

void parse_line_directive(void)
{
    unsigned char filename[256];
    SInt16 ch;
    SInt16 length;
    HashNameNode *oldFile;
    HashNameNode *newFile;

    oldFile = fn_00441850(data_0057f94a[current_file_index], NULL);
    data_0057f9dd = 1;
    if (CPrep_ScanMacroExpandedChar() == 0) {
        Boolean save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        data_005884fd = save;
        return;
    }
    if (CPrepTokenizer_ScanToken() != -1) {
        Boolean save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PREPROCESSOR_SYNTAX_ERROR);
        data_005884fd = save;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    data_00587ef0 = intconst_lo - 1;
    if (CPrep_ScanMacroExpandedChar() == 0)
        return;
    ch = CPrepTokenizer_NextChar();
    if (ch != '"') {
        Boolean save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PREPROCESSOR_SYNTAX_ERROR);
        data_005884fd = save;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    length = 0;
    for (ch = CPrepTokenizer_NextChar(); ch != '"'; ch = CPrepTokenizer_NextChar()) {
        if (ch == 0)
            break;
        if (length > 252) {
            Boolean save = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_STRING_TOO_LONG);
            data_005884fd = save;
            CPrepTokenizer_SkipToEndOfLine();
            return;
        }
        filename[length++] = ch;
    }
    filename[length] = 0;
    CTool_CtoPstr(filename);
    COS_FileSetFSSpec(data_0057f94a[current_file_index], filename);
    if (copts.filesyminfo != 0 && cprep_cu[0xe0] == 0) {
        newFile = fn_00441850(data_0057f94a[current_file_index], NULL);
        if (oldFile != newFile) {
            oldFile = !current_file_index ? NULL : oldFile;
            fn_0048b090(oldFile, newFile);
        }
    }
    if (CPrep_ScanMacroExpandedChar() != 0) {
        Boolean save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = save;
        CPrepTokenizer_SkipToEndOfLine();
    }
}

struct CNameRef evaluate_unary_expression_value(void)
{
    union {
        CPrepValue value;
        CNameRef ref;
    } result;
    if (CPrep_ScanMacroExpandedChar() == 0) {
        CPrep_Error_439930(0x70);
        result.value.value = cint64_zero;
        result.value.isUnsigned = 0;
    } else {
        switch (CPrepTokenizer_ScanToken()) {
            case '-': {
                Type *type;
                result.ref = evaluate_unary_expression_value();
                type = CPrep_IntegerType(&result.value);
                result.value.value = CMach_CalcIntMonadic(type, '-', result.value.value);
                return result.ref;
            }
            case '+':
                result.ref = evaluate_unary_expression_value();
                return result.ref;
            case '!': {
                SInt32 zeroValue;
                result.ref = evaluate_unary_expression_value();
                zeroValue = (Boolean)(result.value.value.hi == 0 && result.value.value.lo == 0);
                result.value.value.lo = zeroValue;
                result.value.value.hi = zeroValue < 0 ? -1 : 0;
                return result.ref;
            }
            case '~': {
                Type *type;
                result.ref = evaluate_unary_expression_value();
                type = CPrep_IntegerType(&result.value);
                result.value.value = CMach_CalcIntMonadic(type, '~', result.value.value);
                return result.ref;
            }
            case '(':
                result.ref = evaluate_conditional_expression_value();
                if (data_0057fd6c != ')') {
                    CPrep_Error_439930(0x73);
                    return result.ref;
                }
                break;
            case 0x158:
                result.value.value = cint64_one;
                result.value.isUnsigned = 1;
                break;
            default:
                CPrep_Error_439930(0x8d);
            case 0x159:
                result.value.value = cint64_zero;
                result.value.isUnsigned = 1;
                break;
            case -1: {
                result.value.value = token_integer;
                result.value.isUnsigned =
                    (token_value_kind_or_string_length == 0xc || token_value_kind_or_string_length == 0xa ||
                     token_value_kind_or_string_length == 8 || token_value_kind_or_string_length == 3);
                break;
            }
            case -3:
                result.value.value = cint64_zero;
                result.value.isUnsigned = 0;
                if (memcmp("defined", data_00587fa0->name, 8) != 0)
                    break;
                macrocheck = 0;
                if (CPrep_ScanMacroExpandedChar() == 0) {
                    CPrep_Error_439930(0x70);
                    break;
                }
                data_0057fd6c = CPrepTokenizer_ScanToken();
                if (data_0057fd6c == '(') {
                    if (CPrep_ScanMacroExpandedChar() == 0) {
                        CPrep_Error_439930(0x70);
                        macrocheck = 1;
                        break;
                    }
                    if (CPrepTokenizer_GetToken() != -3) {
                        CPrep_Error_439930(0x6b);
                        macrocheck = 1;
                        break;
                    }
                    if (find_macro() != NULL)
                        result.value.value = cint64_one;
                    if (CPrep_ScanMacroExpandedChar() == 0) {
                        CPrep_Error_439930(0x70);
                        macrocheck = 1;
                        break;
                    } else if (CPrepTokenizer_ScanToken() != ')') {
                        CPrep_Error_439930(0x73);
                    }
                } else if (data_0057fd6c == -3) {
                    if (find_macro() != NULL)
                        result.value.value = cint64_one;
                } else {
                    CPrep_Error_439930(0x6b);
                }
                macrocheck = 1;
                break;
        }
    }
    if (CPrep_ScanMacroExpandedChar() != 0)
        data_0057fd6c = CPrepTokenizer_ScanToken();
    else
        data_0057fd6c = 0;
    return result.ref;
}

#pragma sym off

static void CPrep_Error_439930(int line)
{
    UInt8 save;
    save = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_ReportError(line);
    data_005884fd = save;
}

struct CNameRef evaluate_binary_expression_value(struct CNameRef *initialOperand, SInt16 precedenceLimit)
{
    union {
        CPrepValue prepValue;
        CNameRef nameRef;
    } leftOperand, rightOperand;
    SInt16 precedence;
    SInt16 operation;
    SInt16 rightPrecedence;
    Boolean savedErrorState;
    Type *type;

    if (initialOperand == NULL) {
        leftOperand.nameRef = evaluate_unary_expression_value();
    } else {
        leftOperand.nameRef = *initialOperand;
    }

    for (;;) {
        precedence = GetPrec(operation = data_0057fd6c);
        if (precedence == 0) {
            return leftOperand.nameRef;
        }
        if (CPrep_ScanMacroExpandedChar() == 0) {
            savedErrorState = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_UNEXPECTED_END_LINE);
            data_005884fd = savedErrorState;
            return leftOperand.nameRef;
        }

        rightOperand.nameRef = evaluate_unary_expression_value();

        for (;;) {
            if (data_0057fd6c == ')' || CPrep_ScanMacroExpandedChar() == 0) {
                if (leftOperand.prepValue.isUnsigned) {
                    rightOperand.prepValue.isUnsigned = 1;
                }
                type = CPrep_IntegerExpressionType(&rightOperand.prepValue);
                leftOperand.prepValue.value =
                    CMach_CalcIntDiadic(type, leftOperand.prepValue.value, operation, rightOperand.prepValue.value);
                return leftOperand.nameRef;
            }

            rightPrecedence = GetPrec(data_0057fd6c);
            if (rightPrecedence == 0) {
                savedErrorState = data_005884fd;
                data_005884fd = 0;
                data_0057f9dc = 1;
                CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                data_005884fd = savedErrorState;
                return leftOperand.nameRef;
            }

            if (precedence >= rightPrecedence) {
                if (leftOperand.prepValue.isUnsigned) {
                    rightOperand.prepValue.isUnsigned = 1;
                }
                type = CPrep_IntegerExpressionType(&rightOperand.prepValue);
                leftOperand.prepValue.value =
                    CMach_CalcIntDiadic(type, leftOperand.prepValue.value, operation, rightOperand.prepValue.value);
                if (GetPrec(rightPrecedence) > precedenceLimit) {
                    break;
                }
                return leftOperand.nameRef;
            }

            rightOperand.nameRef = evaluate_binary_expression_value(&rightOperand.nameRef, precedence);
        }
    }
}

#pragma auto_inline off
Boolean is_zero_name_ref(CNameRef *value)
{
    int isZero;

    isZero = 0;
    if ((value->a == 0) && (value->b == 0)) {
        isZero = 1;
    }
    return isZero;
}
#pragma auto_inline reset

CNameRef evaluate_conditional_expression_value(void)
{
    CNameRef result, trueValue, falseValue;
    CNameRef falseBranch, trueBranch, trueCondition;
    CNameRef nestedFalseValue, nestedTrueValue, falseCondition;
    SInt32 falseConditionTrue, conditionTrue;
    UInt8 savedErrorState;

    result = evaluate_binary_expression_value(NULL, -1);
    if (data_0057fd6c == '?') {
        trueCondition = evaluate_binary_expression_value(NULL, -1);
        if (data_0057fd6c == '?') {
            trueBranch = evaluate_conditional_expression_value();
            if (CPrep_ScanMacroExpandedChar() == 0) {
                CPrep_ReportError(ERR_UNEXPECTED_END_LINE);
                trueValue = trueCondition;
            } else if (data_0057fd6c != ':') {
                CPrep_ReportError(ERR_COLON_EXPECTED);
                trueValue = trueCondition;
            } else if (CPrep_ScanMacroExpandedChar() == 0) {
                CPrep_ReportError(ERR_UNEXPECTED_END_LINE);
                trueValue = trueCondition;
            } else {
                falseBranch = evaluate_conditional_expression_value();
                conditionTrue = !is_zero_name_ref(&trueCondition);
                trueCondition = conditionTrue ? trueBranch : falseBranch;
                trueValue = trueCondition;
            }
        } else {
            trueValue = trueCondition;
        }

        if (CPrep_ScanMacroExpandedChar() == 0) {
            savedErrorState = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_UNEXPECTED_END_LINE);
            data_005884fd = savedErrorState;
            return result;
        }
        if (data_0057fd6c != ':') {
            savedErrorState = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_COLON_EXPECTED);
            data_005884fd = savedErrorState;
            return result;
        }
        if (CPrep_ScanMacroExpandedChar() == 0) {
            savedErrorState = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_UNEXPECTED_END_LINE);
            data_005884fd = savedErrorState;
            return result;
        }

        falseCondition = evaluate_binary_expression_value(NULL, -1);
        if (data_0057fd6c == '?') {
            nestedTrueValue = evaluate_conditional_expression_value();
            if (CPrep_ScanMacroExpandedChar() == 0) {
                CPrep_ReportError(ERR_UNEXPECTED_END_LINE);
                falseValue = falseCondition;
            } else if (data_0057fd6c != ':') {
                CPrep_ReportError(ERR_COLON_EXPECTED);
                falseValue = falseCondition;
            } else if (CPrep_ScanMacroExpandedChar() == 0) {
                CPrep_ReportError(ERR_UNEXPECTED_END_LINE);
                falseValue = falseCondition;
            } else {
                nestedFalseValue = evaluate_conditional_expression_value();
                falseConditionTrue = !is_zero_name_ref(&falseCondition);
                falseCondition = falseConditionTrue ? nestedTrueValue : nestedFalseValue;
                falseValue = falseCondition;
            }
        } else {
            falseValue = falseCondition;
        }

        conditionTrue = !CNameRef_IsNull(&result);
        result = conditionTrue ? trueValue : falseValue;
    }
    return result;
}

static Boolean CNameRef_IsNull(CNameRef *p)
{
    return (p->a == 0 && p->b == 0);
}

void fn_004392e0(void)
{
    CNameRef savedState;
    Boolean savedFlag;

    macrocheck = 1;
    data_00588470 = 0;
    data_0057f9de = 1;
    savedState = evaluate_conditional_expression_value();
    if (data_0057fd6c != 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
        data_005884fd = savedFlag;
    }
    data_0057f9de = 0;
    if (data_00588470 != 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_UNEXPECTED_END_LINE);
        data_005884fd = savedFlag;
    } else if ((short)CPrep_ScanMacroExpandedChar() != 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_END_LINE_EXPECTED);
        data_005884fd = savedFlag;
        CPrepTokenizer_SkipToEndOfLine();
    }
    token_integer.hi = savedState.a;
    intconst_lo = savedState.b;
}

void parse_if_directive(void)
{
    Boolean empty;
    if (if_depth > 0) {
        switch (data_0057f6c8[if_depth - 1].state) {
            case 1:
            case 3:
            case 4:
                CPrepTokenizer_SkipToEndOfLine();
                CPrep_Push(3);
                return;
        }
    }
    fn_004392e0();
    empty = (token_integer.hi == 0 && intconst_lo == 0);
    if (empty) {
        CPrep_Push(1);
        skip_inactive_if_blocks();
    } else {
        CPrep_Push(0);
    }
}

static void CPrep_Push(SInt16 type)
{
    Boolean save;
    if (if_depth >= 0x40) {
        save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_IF_NESTING_OVERFLOW);
        data_005884fd = save;
    } else {
        data_0057f6c8[if_depth].state = type;
        data_0057f6c8[if_depth].file = currentPFile;
        data_0057f6c8[if_depth].pos = (char *)currentTextPosition - data_00587fb0;
        if_depth++;
    }
}

void parse_ifdef_directive(void)
{
    if (if_depth > 0) {
        switch (data_0057f6c8[if_depth - 1].state) {
            case 1:
            case 3:
            case 4:
                CPrepTokenizer_SkipToEndOfLine();
                CPrep_AddLine(3);
                return;
        }
    }
    macrocheck = 0;
    if ((SInt16)CPrep_ScanMacroExpandedChar() == 0) {
        CPrep_ERROR(0x70);
        CPrep_AddLine(1);
        skip_inactive_if_blocks();
        return;
    }
    if (CPrepTokenizer_GetToken() != -3) {
        CPrep_ERROR(0x6b);
        CPrep_AddLine(1);
        skip_inactive_if_blocks();
        return;
    }
    macrocheck = 1;
    if ((SInt16)CPrep_ScanMacroExpandedChar() != 0) {
        CPrep_ERROR(0x71);
        CPrepTokenizer_SkipToEndOfLine();
    }
    if (find_macro() != NULL) {
        CPrep_AddLine(0);
    } else {
        CPrep_AddLine(1);
        skip_inactive_if_blocks();
    }
}

void parse_ifndef(void)
{
    if (if_depth > 0) {
        switch (data_0057f6c8[if_depth - 1].state) {
            case 1:
            case 3:
            case 4:
                CPrepTokenizer_SkipToEndOfLine();
                CPrep_AddLine(3);
                return;
        }
    }
    macrocheck = 0;
    if (CPrep_ScanMacroExpandedChar() == 0) {
        CPrep_ERROR(0x70);
        CPrep_AddLine(1);
        skip_inactive_if_blocks();
        return;
    }
    if (CPrepTokenizer_GetToken() != -3) {
        CPrep_ERROR(0x6b);
        CPrep_AddLine(1);
        skip_inactive_if_blocks();
        return;
    }
    macrocheck = 1;
    if (CPrep_ScanMacroExpandedChar() != 0) {
        CPrep_ERROR(0x71);
        CPrepTokenizer_SkipToEndOfLine();
    }
    if (find_macro() == NULL) {
        CPrep_AddLine(0);
    } else {
        CPrep_AddLine(1);
        skip_inactive_if_blocks();
    }
}

/* Error reporting sequence emitted at each fatal site. */

static void CPrep_AddLine(SInt16 kind)
{
    if (if_depth >= 0x40) {
        CPrep_ERROR(0xd7);
    } else {
        data_0057f6c8[if_depth].state = kind;
        data_0057f6c8[if_depth].file = currentPFile;
        data_0057f6c8[if_depth].pos = (char *)currentTextPosition - data_00587fb0;
        if_depth++;
    }
}

void parse_elif_directive(void)
{
    SInt32 st;

    if (if_depth <= 0 || (st = data_0057f6c8[if_depth - 1].state) == 2 || st == 4) {
        Boolean saved = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PRECEDING_IF_MISSING);
        data_005884fd = saved;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    } else {
        Boolean live;

        switch (st) {
            case 1:
                fn_004392e0();
                live = (token_integer.hi == 0 && intconst_lo == 0);
                if (!live) {
                    data_0057f6c8[if_depth - 1].state = 0;
                }
                return;
            case 3:
                CPrepTokenizer_SkipToEndOfLine();
                return;
            case 0:
                data_0057f6c8[if_depth - 1].state = 3;
                CPrepTokenizer_SkipToEndOfLine();
                skip_inactive_if_blocks();
                return;
            default:
                CError_FATAL(5696);
        }
    }
}

void parse_else_directive(void)
{
    Boolean savedFlag;
    SInt32 st;

    if (((if_depth <= 0) || ((st = data_0057f6c8[if_depth - 1].state) == 2)) || (st == 4)) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PRECEDING_IF_MISSING);
        data_005884fd = savedFlag;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    if (copts.ANSIstrict != '\0') {
        if (CPrep_ScanMacroExpandedChar() != 0) {
            savedFlag = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_END_LINE_EXPECTED);
            data_005884fd = savedFlag;
            CPrepTokenizer_SkipToEndOfLine();
        }
    } else {
        CPrepTokenizer_SkipToEndOfLine();
    }
    switch (data_0057f6c8[if_depth - 1].state) {
        case 1:
            data_0057f6c8[if_depth - 1].state = 2;
            return;
        case 0:
            data_0057f6c8[if_depth - 1].state = 4;
            skip_inactive_if_blocks();
            return;
        case 3:
            data_0057f6c8[if_depth - 1].state = 4;
            return;
        default:
            CError_FATAL(5734);
            return;
    }
}

int parse_endif_directive(void)
{
    unsigned char savedFlag;
    SInt16 result;

    if (if_depth <= 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PRECEDING_IF_MISSING);
        data_005884fd = savedFlag;
        CPrepTokenizer_SkipToEndOfLine();
        return;
    }
    macrocheck = 0;
    if (copts.ANSIstrict != '\0') {
        result = CPrep_ScanMacroExpandedChar();
        if (result != 0) {
            savedFlag = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_ReportError(ERR_END_LINE_EXPECTED);
            data_005884fd = savedFlag;
            CPrepTokenizer_SkipToEndOfLine();
        }
    } else {
        CPrepTokenizer_SkipToEndOfLine();
    }
    macrocheck = 1;
    if (if_depth <= 0) {
        savedFlag = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PREPROCESSOR_SYNTAX_ERROR);
        data_005884fd = savedFlag;
    } else {
        switch (data_0057f6c8[if_depth - 1].state) {
            case 1:
            case 3:
            case 4:
                if_depth--;
                break;
            default:
                if_depth--;
                if (0 < if_depth) {
                    switch (data_0057f6c8[if_depth - 1].state) {
                        case 1:
                        case 3:
                        case 4:
                            skip_inactive_if_blocks();
                            break;
                    }
                }
                break;
        }
    }
}

#pragma sym reset

void skip_inactive_if_blocks(void)
{
    SInt16 tok;
    SInt16 type;
    TStreamElement tmp;
    SInt32 *handle;
    int offset;

    while (if_depth > 0) {
        type = data_0057f6c8[if_depth - 1].state;
        switch (type) {
            case 1:
            case 3:
            case 4:
                for (;;) {
                    tok = CPrepTokenizer_ScanChar(), currentTextPosition = (UInt8 *)lookahead_position;

                    switch (tok) {
                        case 0:
#define EOF_PREFIX()                                                                                                   \
    if (macro_expansion_depth > 0 || currentTextPosition >= textend) {                                                 \
        if (current_file_index > 0) {                                                                                  \
            CPrep_PopFile();                                                                                           \
            continue;                                                                                                  \
        }
                            EOF_PREFIX()
                            data_0057f9dc = 0;
                            tmp.tokenfile = data_0057f6c8[if_depth - 1].file;
                            tmp.tokenoffset = data_0057f6c8[if_depth - 1].pos;
                            CError_SetBufferedToken(&tmp);
                            CError_FatalError(ERR_UNTERMINATED_IF_MACRO);
                            if_depth = 0;
                            return;
                    }
                    else
                    {
                        report_error(0x69);
                        continue;
                    }
                    case 0xd:
                        process_newline();
                        continue;
                    case 0x22:
                        CPrepTokenizer_SkipQuotedLiteral(currentTextPosition, 0x22);
                        continue;
                    case 0x27:
                        CPrepTokenizer_SkipQuotedLiteral(currentTextPosition, 0x22);
                        continue;
                    case 0x23:
                        tok = CPrepTokenizer_ScanChar();
                        currentTextPosition = (UInt8 *)lookahead_position;
                        switch (tok) {
                            case 0:
                                report_error(0x66);
                                break;
                            case 0xd:
                                continue;
                        }
                        currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
                        KEYWORD()
                        break;
                    default:
                        CPrepTokenizer_SkipToEndOfLine();
                        continue;
                }
                break;
        }
        break;
        default:
            return;
    }
}
}

#pragma sym off

static SInt32 calc_line(CPrepFileInfo *p)
{
    return (char *)currentTextPosition - p->textbuffer;
}

static void report_error(SInt16 code)
{
    CPrep_Error(code);
}

static SInt32 check_time(void)
{
    void **p = (void **)cprep_cu;
    return CPrep_CallCompilerCallback(*p, line_count);
}

static void CPrep_Error(SInt16 code)
{
    UInt8 save = data_005884fd;
    data_005884fd = 0;
    if (code == 0x66 && (macro_expansion_depth > 0 || currentTextPosition < textend))
        code = 0x69;
    data_0057f9dc = 1;
    CError_ReportError(code);
    data_005884fd = save;
}

void CPrep_ParseDirective(void)
{
    SInt16 ch;
    Boolean save;

    data_0057fd0c = (char *)currentTextPosition - data_00587fb0;
    ch = CPrepTokenizer_ScanChar();
    currentTextPosition = (UInt8 *)lookahead_position;
    switch (ch) {
        case 0:
            ch = 0x66;
            save = data_005884fd;
            data_005884fd = 0;
            if (macro_expansion_depth > 0 || currentTextPosition < textend)
                ch = 0x69;
            data_0057f9dc = 1;
            CError_ReportError(ch);
            data_005884fd = save;
        case 13:
            return;
    }
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (!strcmp("define", data_00587fa0->name)) {
        define_macro();
        macrocheck = 1;
        return;
    }
    if (!strcmp("undef", data_00587fa0->name)) {
        undefine_macro();
        macrocheck = 1;
        return;
    }
    if (!strcmp("include", data_00587fa0->name)) {
        fn_0043a0a0(0);
        macrocheck = 1;
        return;
    }
    if (!strcmp("line", data_00587fa0->name)) {
        parse_line_directive();
        macrocheck = 1;
        return;
    }
    if (!strcmp("error", data_00587fa0->name)) {
        save = data_005884fd;
        data_005884fd = 0;
        data_0057f9dc = 1;
        CError_ReportError(ERR_PREPROCESSOR_ERROR_DIRECTIVE);
        data_005884fd = save;
        fn_00449d60();
        CPrepTokenizer_SkipToEndOfLine();
        macrocheck = 1;
        return;
    }
    if (!strcmp("pragma", data_00587fa0->name)) {
        parse_pragma();
        macrocheck = 1;
        return;
    }
    if (copts.ANSIstrict == 0) {
        if (!strcmp("warning", data_00587fa0->name)) {
            save = data_005884fd;
            data_005884fd = 0;
            data_0057f9dc = 1;
            CError_Warning(ERR_PREPROCESSOR_WARNING_DIRECTIVE);
            data_005884fd = save;
            CPrepTokenizer_SkipToEndOfLine();
            macrocheck = 1;
            return;
        }
        if (!strcmp(data_00587fa0->name, "ident")) {
            CPrepTokenizer_SkipToEndOfLine();
            macrocheck = 1;
            return;
        }
    }
    if (!strcmp("if", data_00587fa0->name)) {
        macrocheck = 1;
        parse_if_directive();
        return;
    }
    if (!strcmp("ifdef", data_00587fa0->name)) {
        macrocheck = 1;
        parse_ifdef_directive();
        return;
    }
    if (!strcmp("ifndef", data_00587fa0->name)) {
        macrocheck = 1;
        parse_ifndef();
        return;
    }
    if (!strcmp("elif", data_00587fa0->name)) {
        macrocheck = 1;
        parse_elif_directive();
        return;
    }
    if (!strcmp("else", data_00587fa0->name)) {
        macrocheck = 1;
        parse_else_directive();
        return;
    }
    if (!strcmp("endif", data_00587fa0->name)) {
        macrocheck = 1;
        parse_endif_directive();
        return;
    }
    if (copts.objective_c != 0 || copts.ANSIstrict == 0) {
        if (!strcmp("import", data_00587fa0->name)) {
            fn_0043a0a0(1);
            macrocheck = 1;
            return;
        }
    }
    save = data_005884fd;
    data_005884fd = 0;
    data_0057f9dc = 1;
    CError_ReportError(ERR_UNDEFINED_PREPROCESSOR_DIRECTIVE);
    data_005884fd = save;
    CPrepTokenizer_SkipToEndOfLine();
    macrocheck = 1;
    return;
}

#pragma sym reset

SInt32 CPrep_GetCurrentTextOffset(void)
{
    if (buffered_tokens < bufferedTokenPosition) {
        return bufferedTokenPosition[-1].tokenoffset + 1;
    }
    if (macro_expansion_depth != 0) {
        return macro_stack[0].pos - data_0057f94a[current_file_index]->textbuffer;
    }
    return currentTextPosition - (unsigned char *)data_0057f94a[current_file_index]->textbuffer;
}

#pragma sym off

void CPrep_GetBrowseFilePosition(CPrepFileInfo **file, SInt32 *ppos)
{
    CPrepFileInfo *f;
    CPrepFileInfo *fi;

    if (buffered_tokens < bufferedTokenPosition) {
        f = bufferedTokenPosition[-1].tokenfile;
        *ppos = bufferedTokenPosition[-1].tokenoffset + 1;
    } else {
        f = fi = data_0057f94a[current_file_index];
        if (macro_expansion_depth)
            *ppos = macro_stack[0].pos - fi->textbuffer;
        else
            *ppos = currentTextPosition - (UInt8 *)fi->textbuffer;
    }
    if (f && f->fileID > 0 && (f->recordbrowseinfo || template_recordbrowseinfo)) {
        *file = f;
    } else {
        *file = NULL;
        *ppos = 0;
    }
}

CPrepFileInfo *CPrep_GetPFile(void)
{
    return currentPFile;
}

VarInfo *CPrep_AllocateVarInfo(void)
{
    VarInfo *info;
    info = (VarInfo *)lalloc(sizeof(VarInfo));
    memclrw(info, sizeof(VarInfo));
    info->usage = 0;
    info->deftoken = *CPrep_GetLastBufferedToken();
    info->varnumber = next_varnumber;
    next_varnumber++;
    info->noregister = 0;
    info->used = 0;
    info->reg = 0;
    info->regHi = 0;
    info->in_param_area = 0;
    return info;
}

#pragma sym reset
