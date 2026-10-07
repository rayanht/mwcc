#ifndef COMPILER_CPREP_H
#define COMPILER_CPREP_H

#include "compiler/common.h"
#include "compiler/tokens.h"
#include "driver/Files.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 2)
struct IROOptNode {
    struct IROOptNode *next;
    SInt32 code;
    UInt8 flag;
    UInt8 value;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct BrowseOptions {
    SInt8 browseOption; /* 0x00: CPrepCU browseOption view of CPrep_GetDependencyState settings. */
    UInt8 browseEnums;  /* 0x01: CDecl tests this setting while declaring enums. */
    UInt8 browseMacros; /* 0x02: CPrepCU browseMacros view of CPrep_GetDependencyState settings. */
    UInt8 fe9;
    UInt8 pad_ea;
    UInt8 feb;
    UInt8 padec[0x0a];
};
#pragma pack(pop)
#pragma options align = mac68k
struct CNameRef {
    SInt32 a;
    SInt32 b;
    SInt16 c;
};
#pragma options align = reset
#pragma options align = mac68k
struct CompilerLinkerOptions {
    UInt8 littleendian;
    UInt8 pad01[1];
    SInt16 processor;
    SInt8 processorModel; /* 0x04: SetProcessorModel stores model; Scheduler selects target scheduling model */
    UInt8 instructionSchedulingMode; /* 0x05: CodeGen selects Scheduler_Schedule when mode is 2 */
    unsigned char debug_listing;
    char emitExtraAssemblyData; /* 0x07: PCodeAssembly_ShouldEmitExtraData */
    Boolean disable_registers;
    char
        fp_contract; /* 0x09: TargetPanels_eabi_ppc_LoadCompilerOptions clears this, then loads the CodeGen panel's fp_contract unless its fpmode is 0 or 1. */
    Boolean unroll_speculative;
    UInt8 pad0b[1];
    SInt16 unroll_instr_limit;
    SInt16 unroll_factor_limit;
    UInt8 altivec_model;
    UInt8 altivec_vrsave; /* 0x11: CodeGen parses altivec_vrsave; StackFrameEABI gates VRSAVE frame handling */
    UInt8 codeAlignment;  /* 0x12: SetCodeAlignment; BE_symbol assigns default symbol alignment */
    char catssupport;     /* 0x13: PCodeAssembly_ShouldEmitSerializedFormat */
    char forcecatssupport;
    UInt8 pad15[1];
    SInt32 nonconstsmalldatathreshold; /* 0x16: ObjGen_PPC_EABI_SetObjectSection tests initialized data size */
    int constsmalldatathreshold;       /* 0x1a: ObjGen_PPC_EABI_SetObjectSection tests uninitialized data size */
    UInt8 f1e;
    UInt8 usedatapool; /* 0x1f: BE_symbol_004913b0 gates reuse of section symbolLink data */
    UInt8 commonblocks;
    UInt8 pad21[1];
    UInt8 use_lmw_stmw; /* 0x22: StackFrameEABI selects useSaveHelper for register saves */
    char debugEnabled;
    Boolean operandsDebug;
    UInt8 f25;
    UInt8 f26;
    char f27;
    SInt8 rel109_offset; /* 0x28: CodeGen parses rel109_offset; ObjGen_PPC_EABI adds it to relocation offset */
    UInt8 pad29[1];
    struct ObjGenSection
        *textSection; /* 0x2a: ObjGen_PPC_EABI_SetObjectSection selects section for function datatypes */
    struct ObjGenSection *dataSection;      /* 0x2e: ObjGen_PPC_EABI_DefaultDataSectionIndex */
    struct ObjGenSection *bssSection;       /* 0x32: ObjGen_PPC_EABI_DefaultBSSSectionIndex */
    struct ObjGenSection *smallDataSection; /* 0x36: ObjGen_PPC_EABI_SetObjectSection selects initialized small data */
    struct ObjGenSection *smallBSSSection; /* 0x3a: ObjGen_PPC_EABI_SetObjectSection selects uninitialized small data */
    SInt32 sectionHeaderTableSize;
    SInt32 *sectionHeaderTable;
    struct InterruptList *interruptList;
    struct InterruptGenerationRecord *interruptOptions; /* 0x4a: CodeGen_ParsePragma saves active interrupt options */
    UInt8 incompatible_return_small_structs;
    Boolean create_file_object; /* 0x4f: ObjGen_PPC_EABI_FinalizeOutputBuffers gates create_main_file_object */
    UInt8
        incompatible_sfpe_double_params; /* 0x50: Type_RequiresMemoryReturn forces memory return even for aggregates of size at most 8 */
    UInt8 rsqrt;
    UInt8 k63d;
    char *f54;
    SInt16 inlineLimit;
    UInt8 cplusplus;
    UInt8 ecplusplus;
    UInt8 objective_c;
    SInt8 objc_strict;
    char ARM_conform;
    char ARMscoping;
    Boolean checkprotos;
    char trigraphs;
    UInt8 onlystdkeywords;
    UInt8 enumsalwaysint;
    UInt8 ANSIstrict;
    UInt8 mpwc_relax;
    UInt8 mpwc_newline;
    Boolean ignore_oldstyle;
    Boolean cpp_extensions;
    UInt8 pointercast_lvalue;
    char RTTI;
    UInt8 delete_exception;
    UInt8 pad6c;
    UInt8 oldalignment;
    UInt8 unsigned_char;
    char multibyteaware;
    UInt8 auto_inline;
    UInt8 defer_codegen;
    UInt8 direct_to_som;
    UInt8 SOMCheckEnvironment;
    UInt8 SOMCallOptimization;
    UInt8 booltruefalse;
    UInt8 old_enum_mangler;
    UInt8 longlong;
    UInt8 longlong_enums;
    UInt8 no_tfuncinline;
    UInt8 f7a;
    char flat_include;
    char syspath_once;
    char always_import;
    UInt8 simple_class_byval;
    char wchar_type;
    UInt8 vbase_ctor_offset;
    char vbase_abi_v2;
    char def_inherited;
    UInt8 template_patch;
    UInt8 template_friends;
    Boolean faster_pch_gen;
    UInt8 array_new_delete;
    UInt8 dollar_identifiers;
    UInt8 def_inline_tfuncs;
    UInt8 arg_dep_lookup;
    volatile char simple_prepdump;
    char line_prepdump;
    UInt8 fullpath_prepdump;
    UInt8 old_mtemplparser;
    UInt8 suppress_init_code;
    Boolean reverse_bitfields;
    UInt8 c9x;
    Boolean float_constants;
    Boolean no_static_dtors;
    Boolean longlong_prepeval;
    UInt8 const_strings;
    UInt8 dumpir;
    UInt8 experimental;
    UInt8 gcc_extensions;
    UInt8 stdc_fp_contract;
    UInt8 stdc_fenv_access;
    UInt8 stdc_cx_limitedr;
    UInt8 microsoft_EH;
    char warningerrors;
    char extended_errorcheck;
    UInt8 check_header_flags;
    Boolean warn_illpragma;
    UInt8 warn_emptydecl;
    Boolean warn_possunwant;
    char warn_unusedvar;
    char warn_unusedarg;
    UInt8 warn_extracomma;
    Boolean warn_hidevirtual;
    char warn_largeargs;
    Boolean warn_implicitconv;
    UInt8 warn_notinlined;
    SInt8 warn_structclass;
    Boolean warn_padding;
    Boolean warn_no_side_effect;
    Boolean warn_resultnotused;
    signed char structalignment;
    UInt8 alignarraymembers;
    SInt8 dont_reuse_strings;
    UInt8 poolstrings;
    UInt8 explicit_zero_data;
    UInt8 readonly_strings;
    UInt8 exceptions;
    UInt8 padb4[1];
    UInt8 dontinline;
    UInt8 alwaysinline;
    char
        peephole; /* 0xb7: CodeGen gates Peephole_MergeAdjacentBlocks and Peephole_VisitBlocksWithMultipleInstructions */
    Boolean globaloptimizer;
    unsigned char sideeffects;
    UInt8 profile;
    UInt8 cfm_internal;
    UInt8 cfm_import;
    UInt8 cfm_export;
    UInt8 cfm_lib_export;
    char nosyminline;
    char force_active;
    SInt8 deleteDeadInstructions;
    UInt8 optimizesize;
    UInt8 fc3;
    UInt8 commonsubs; /* 0xc4: IRO_Optimizer gates IRO_CommonSubs and available-expression computation */
    UInt8 loopinvariants;
    UInt8 propagation; /* 0xc6: IRO_CopyPropagationSetting */
    UInt8 deadstore;   /* 0xc7: IRO_Optimizer passes eliminateUnused to IRO_UseDef */
    UInt8 strengthreduction;
    UInt8 strengthreductionstrict;
    UInt8 deadcode; /* 0xca: IRO_Optimizer gates IRO_RemoveUnreachable */
    UInt8 lifetimes;
    UInt8 padcc[1];
    UInt8 unrolling; /* 0xcd: IRO_Optimizer gates IRO_LoopUnroller */
    Boolean vectorizeloops;
    UInt8 irSecondOptimizationPass; /* 0xcf: IRO_Optimizer selects two passes rather than one */
    signed char unrollOption;
    SInt8 fd1;
    UInt8 filesyminfo;
    UInt8 fd3;
    char fd4;
    UInt8 fd5;
    Boolean fd6;
    UInt8 padd7[1];
    int precompiledHeaderCreator;
    UInt32 precompiledHeaderFileTypes[2];
    SInt32 fe4;
};
#pragma options align = reset
#pragma options align = mac68k
struct CPrepCU {
    struct CWPluginPrivateContext *context; /* 0x00: dispatch_compiler_plugin_request supplies its plugin context. */
    SInt32 objectData;
    SInt32 browseData;
    UInt8 pad0c[4];
    SInt32 codeSize;
    SInt32 udataSize;
    SInt32 idataSize;
    SInt32 lineCount; /* 0x1c: CPrep_Compile stores line_count. */
    UInt8 pad20[0x14];
    SInt32 objectBuffer;
    SInt32 browseBuffer;
    SInt32 pluginRequest;
    SInt32 apiVersion;
    struct CWFileSpec projectFile;
    SInt32 projectFileCount;
    SInt32 mainFileNumber;
    struct CWFileSpec mainFile;
    UInt32 mainFileOffset;
    UInt32 mainFileLength;
    UInt8 precompiling;
    UInt8 active; /* 0xe1: initialize_compiler_plugin_cu obtains CPrep_GetActive. */
    UInt8 preprocessOnly;
    UInt8 filesyminfo;
    UInt8 useMappedPrecompiledHeaders;
    UInt8 pad_e5;
    struct BrowseOptions
        browseOptions; /* 0xe6: initialize_compiler_plugin_cu obtains the browse settings from CPrep_GetDependencyState. */
    UInt8 compiling;
    UInt8
        mainFileAttributesAlignment; /* 0xf7: initialize_compiler_plugin_cu clears this byte; it aligns the UInt16 mainFileAttributes read by CPrep_GetDependencyOption at 0xf8. */
    UInt16 mainFileAttributes;
    SInt32 platformCode1; /* 0xfa: initialize_compiler_plugin_cu copies head.platformCodes[1]. */
    SInt32 platformCode0; /* 0xfe: initialize_compiler_plugin_cu copies head.platformCodes[0]. */
};
#pragma options align = reset
#pragma options align = mac68k
struct CPrepRec {
    struct CPrepFileInfo *file;
    SInt32 pos;
    SInt16 state;
};
#pragma options align = reset
#pragma options align = mac68k
struct CPrepValue {
    CInt64 value;
    Boolean isUnsigned;
};
#pragma options align = reset
#pragma options align = mac68k
struct CPrep_0043afc0_Entry {
    struct HashNameNode *obj;
    SInt16 align;
};
#pragma options align = reset
struct ObjectCallbackContext {
    char reserved[0x10];
    unsigned int(__stdcall *callback)(struct CWPluginPrivateContext *, unsigned int);
    char reserved14[8];
    unsigned int(__stdcall *invoke)(struct CWPluginPrivateContext *, struct FileProcessingInfo *, const char *);
};
#pragma options align = mac68k
struct IncludeFilenamePointer {
    char *filename;
};
#pragma options align = reset
#pragma options align = mac68k
struct IncludeSearchPolicy {
    char searchLocal; /* 0x00: CPrep.c sets local search for quoted includes and tests it for the macro-key prefix. */
    char unusedBytes[3]; /* 0x01: CPrep.c accesses only searchLocal; these bytes have no reads or writes. */
};
#pragma options align = reset
#pragma options align = mac68k
struct Macro {
    struct Macro *next;
    struct HashNameNode *name;
    char *text;
    UInt16 nargs;
    UInt8 flag;
    UInt8 isExpanding; /* 0x0f: find_expandable_macro rejects active expansions; CPrep_PopState clears it */
    struct HashNameNode *args[1];
};
#pragma options align = reset
#pragma options align = mac68k
struct MacroStack {
    char *pos;
    char *macname;
    struct Macro *macro;
    Boolean macrocheck;
};
#pragma options align = reset
#pragma options align = mac68k
struct OptionEntry {
    char *name;
    SInt16 flags;
};
#pragma options align = reset
#pragma options align = mac68k
struct CPrepFileInfo {
    CWFileSpec textfile; /* 0x00: fn_004401b0 copies the resolved file specification */
    char *textbuffer;
    SInt32 textlength;
    SInt32 linenumber;
    SInt32 pos;
    Boolean hasprepline;
    SInt16 fileID;
    Boolean recordbrowseinfo;
    UInt8 unusedBytes
        [3]; /* 0x5b: fn_004401b0 clears these bytes with the whole CPrepFileInfo; no member reads or writes */
    Boolean isDefault;
};
#pragma options align = reset
#pragma options align = mac68k
struct PragmaNode {
    struct PragmaNode *next;
    CompilerLinkerOptions rec;
};
#pragma options align = reset
#pragma options align = mac68k
struct PragmaSettings {
    unsigned char reserved[226];
    unsigned char mode;
};
#pragma options align = reset
#pragma options align = mac68k
struct PrepNameCacheEntry {
    struct PrepNameCacheEntry *next;
    struct CPrepFileInfo *name;
    HashNameNode *value;
    SInt32 auxiliaryValue;
};
#pragma options align = reset
extern VarInfo *CPrep_AllocateVarInfo(void);
extern CPrepFileInfo *CPrep_GetPFile(void);
extern void parse_elif_directive(void);
extern void parse_ifndef(void);
extern void parse_ifdef_directive(void);
extern void parse_if_directive(void);
extern CNameRef evaluate_conditional_expression_value(void);
extern Boolean is_zero_name_ref(CNameRef *value);
extern void parse_line_directive(void);
extern void CPrep_0043a0a0(char allowInclude);
extern void parse_pragma(void);
extern void concatenate_and_dispatch_string_tokens(void);
extern void parse_structalignment(void);
extern void parse_prep_setting(void);
extern void parse_unroll_pragma(void);
extern void parse_optimization_level_pragma(void);
extern void CPrep_0043b790(void);
extern void parse_align_pragma(void);
extern void parse_inline_limit(void);
extern void parse_pragma_option(int directive);
extern char *expand_builtin_macro(Macro *obj);
extern char *CPrep_GetFileName(char *param1, Boolean param2, Boolean param3);
extern char *expand_macros_in_text(Macro *state, char *text);
extern Boolean evaluate_pragma_option(void);
extern void define_macro(void);
extern SInt16 CPrep_ScanMacroExpandedChar(void);
extern void skip_line_breaks_and_expand_macros(void);
extern unsigned int lookup_available_macro(HashNameNode *name);
extern Macro *find_macro_for_expansion(UInt8 *p);
extern Macro *find_expandable_macro(UInt8 *text);
extern Boolean CPrep_0043ecb0(short ch);
extern void CPrep_PopMacro(void);
extern UInt8 CPrep_Compile(CPrepCU *cu);
extern SInt32 CPrep_UpdateTokenLine(FileOffsetInfo *foi);
extern void CPrep_PopFile(void);
extern TStreamElement *CPrep_GetLastBufferedToken(void);
extern int CPrep_0043f860(char *p);
extern void CPrep_ResetBufferedTokenPosition(void);
extern void fn_0043f3b0(short code);
extern NameSpaceList *CPrep_ReportError(short token);
extern void CPrep_RemoveBufferedTokens(TokenStream *stream, SInt32 *firstIndex);
extern void CPrep_InsertTokenBuffer(TokenStream *arg, SInt32 *result);
extern void CPrep_BufferTokensThroughSemicolon(TokenStream *buffer, void (*processToken)(struct TStreamElement *));
extern void CPrep_SaveFunctionBodyTokens(TokenStream *result, void (*tokenCallback)(TStreamElement *), int option);
extern void CPrep_SetPosition(SInt32 *position);
extern void CPrep_UngetToken(void);
extern void CPrep_IncrementCountersAndUpdateTextOffset(void);
extern void read_pragma_token(void);
extern void fn_0043be10(void);
extern void CPrep_RestoreOption(int key);
extern SInt16 CPrep_ExpectEndLine(char suppressDiagnostic);
extern UInt8 *find_identifier_end_after_optional_paren(UInt8 *text);
extern Macro *find_macro(void);
extern void fn_0043f3e0(unsigned int token, char *name);
extern void fn_004392e0(void);
extern void fn_0043f1f0(FileOffsetInfo *name);
extern void CPrep_SaveAndSetOption(unsigned int index, UInt8 value);
extern void undefine_macro(void);
extern void fn_0043e8f0(void);
extern void parse_else_directive(void);
extern void apply_pragma_object_flags(unsigned int flags);
extern UInt8 CPrep_ExpandMacro(void);
extern Macro *lookup_expandable_macro(void);
extern void CPrep_GetFOI(FileOffsetInfo *location, TStreamElement *record);
extern void CPrep_GetPosition(CPrepFileInfo **position, SInt32 *offset);
extern UInt8 *expand_macro(Macro *m);
extern int scan_braced_tokens(void (*callback)(TStreamElement *), int tokenCount);
extern struct CNameRef evaluate_binary_expression_value(struct CNameRef *init, SInt16 minprec);
extern struct CNameRef evaluate_unary_expression_value(void);
extern void CPrep_GetBrowseFilePosition(CPrepFileInfo **file, SInt32 *ppos);
extern void CPrep_ParseDirective(void);
extern int parse_endif_directive(void);
extern CPrepFileInfo *DAT_005875f8;
extern int DAT_00587ef0;
extern int remainingBufferedTokenCount;
extern UInt8 DAT_0058850f;
extern UInt8 DAT_00588516;
extern UInt8 DAT_00588523;
extern SInt16 macro_expansion_depth;
extern UInt32 intconst_lo;
extern UInt8 f87_enabled;
extern struct Macro **macro_buckets;
extern SInt32 line_count;
extern struct TStreamElement *bufferedTokenPosition;
extern UInt8 *token_start;
extern struct CPrepFileInfo *currentPFile;
extern UInt8 *cprep_cu;
extern char *macro_text_start;
extern short current_file_index;
extern SInt16 data_00588470;
extern Boolean data_005884fd;
extern UInt8 data_0058850d;
extern UInt8 macrocheck;
extern MacroStack macro_stack[];
extern SInt32 CPrep_GetCurrentTextOffset(void);
extern void skip_inactive_if_blocks(void);
extern unsigned char fn_004401b0(unsigned char *name, unsigned char mode, unsigned char skip);
extern void CPrep_SetBufferedTokenPosition(SInt32 *count);
extern void CPrep_GetBufferedTokenPosition(SInt32 *count);
extern void CPrep_GrowBufferedTokenBuffer(SInt32 n);
extern void pop_files_and_release_heaps_and_lists(void);
extern int initialize_preprocessor(void);
extern UInt8 DAT_00586fd0[256];
extern UInt8 data_0058702f;
extern struct TStreamElement *buffered_token_buffer_end;
extern UInt8 data_0058852a;
extern SInt32 __stdcall CPrep_GetTargetSettings(CWPluginPrivateContext *obj, TgtRec *dst);
extern unsigned int __stdcall CPrep_CallCompilerCallbackWithValue(CWPluginPrivateContext *object_id,
                                                                  unsigned int argument, long *value);
extern SInt32 __stdcall CPrep_CallCompilerCallback(void *objectId, SInt32 argument);
extern unsigned int __stdcall call_compiler_callback(CWPluginPrivateContext *object_id, void *argument,
                                                     unsigned int result);
extern unsigned int *CPrep_RemoveFlaggedMacros(void);
extern CWPluginPrivateContext *validate_context_signature(CWPluginPrivateContext *record);
extern int CPrep_AppendStringBounded(char *dst, char *src, int size);
extern void __stdcall CPrep_RegisterPredefinedMacros(void);
extern int __stdcall CPrep_GetEnabled(int key, unsigned char *value);
extern unsigned int __stdcall CPrep_GetActive(int selector, unsigned char *value);
extern unsigned int __stdcall CPrep_GetOperation(int key, unsigned char *value);
extern unsigned int __stdcall CPrep_GetSetting(CWPluginPrivateContext *key, unsigned char *value);
extern unsigned int __stdcall CPrep_GetReserved15d(int handle, unsigned char *value);
extern unsigned int __stdcall CPrep_GetDependencyState(CWPluginPrivateContext *a0, BrowseOptions *a1);
extern unsigned int __stdcall CPrep_GetFileIndex(CWPluginPrivateContext *key, unsigned int *value);
extern unsigned int __stdcall CPrep_GetDependencyOption(int lookupKey, unsigned short *value);
extern int __stdcall CPrep_GetContextPayload(CWPluginPrivateContext *handle, CWFileSpec *destination);
extern unsigned int __stdcall CPrep_GetResultValues(CWPluginPrivateContext *handle, UInt32 *first_value,
                                                    UInt32 *second_value);
extern unsigned int __stdcall CPrep_InvokeCompilerCallback(CWPluginPrivateContext *instance_id,
                                                           struct FileProcessingInfo *argument, const char *value);
extern HashNameNode *fn_00441850(CPrepFileInfo *a0, SInt32 *a1);
struct CPrepFileInfo;
struct ObjectCallbackContext;
struct ObjectCallbackContext;

#ifdef __cplusplus
}
#endif

#endif
