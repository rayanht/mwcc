#ifndef COMPILER_CPREP_H
#define COMPILER_CPREP_H

#include "compiler/common.h"
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
struct BufferedToken {
    short token;
    short value_kind;
    struct PFile *tokenfile;
    int tokenoffset;
    int tokenline;
    union {
        CInt64 integer; /* 0x10: CPrepTokenizer_GetNextToken saves/restores token_integer when token == -1 */
        struct {
            int first;
            int second;
        } words; /* 0x10: CPrepTokenizer_GetNextToken, token == -3 (name), -4/-5 (string length) */
        struct {
            char *data;
            int length;
        } string;       /* 0x10: CPrepTokenizer_GetNextToken, token == -4 or -5 */
        Float floating; /* 0x10: CPrepTokenizer_GetNextToken, token == -2, saves/restores token_float */
    } value;
};
#pragma options align = mac68k
struct CNameRef {
    SInt32 a;
    SInt32 b;
    SInt16 c;
};
#pragma options align = reset
#pragma options align = mac68k
struct COpts {
    UInt8 nativeByteOrder;
    UInt8 pad01[1];
    SInt16 processor;
    SInt8 processorModel; /* 0x04: SetProcessorModel stores model; Scheduler selects target scheduling model */
    UInt8 instructionSchedulingMode; /* 0x05: CodeGen selects Scheduler_Schedule when mode is 2 */
    unsigned char cOptimizerDumpEnabled;
    char emitExtraAssemblyData; /* 0x07: PCodeAssembly_ShouldEmitExtraData */
    Boolean f08;
    char
        debugOptions; /* 0x09: TargetPanels_eabi_ppc_LoadCompilerOptions clears this, then loads CompilerOptions.debugOptions for enabled, non-operand debugging. */
    Boolean ppcUnrollSpeculative;
    UInt8 pad0b[1];
    SInt16 ppcUnrollInstructionsLimit;
    SInt16 ppcUnrollFactorLimit;
    UInt8 altivecModel;
    UInt8 altivecVrsave; /* 0x11: CodeGen parses altivec_vrsave; StackFrameEABI gates VRSAVE frame handling */
    UInt8 codeAlignment; /* 0x12: SetCodeAlignment; BE_symbol assigns default symbol alignment */
    char emitSerializedAssemblyFormat; /* 0x13: PCodeAssembly_ShouldEmitSerializedFormat */
    char f14;
    UInt8 pad15[1];
    SInt32 smallDataLimit; /* 0x16: ObjGen_PPC_EABI_SetObjectSection tests initialized data size */
    int smallBSSLimit;     /* 0x1a: ObjGen_PPC_EABI_SetObjectSection tests uninitialized data size */
    UInt8 f1e;
    UInt8 reuseSectionSymbols; /* 0x1f: BE_symbol_004913b0 gates reuse of section symbolLink data */
    UInt8 f20;
    UInt8 pad21[1];
    UInt8 useRegisterSaveHelpers; /* 0x22: StackFrameEABI selects useSaveHelper for register saves */
    char debugEnabled;
    Boolean operandsDebug;
    UInt8 f25;
    UInt8 f26;
    char f27;
    SInt8 rel109Offset; /* 0x28: CodeGen parses rel109_offset; ObjGen_PPC_EABI adds it to relocation offset */
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
    UInt8 f4e;
    Boolean emitMainFileObject; /* 0x4f: ObjGen_PPC_EABI_FinalizeOutputBuffers gates create_main_file_object */
    UInt8
        returnStructsInMemory; /* 0x50: Type_RequiresMemoryReturn forces memory return even for aggregates of size at most 8 */
    UInt8 pad51[3];
    char *f54;
    SInt16 inlineLimit;
    UInt8 cplusplus;
    UInt8 f5b;
    UInt8 f5c;
    SInt8 f5d;
    char f5e;
    char f5f;
    Boolean f60;
    char trigraphs;
    UInt8 f62;
    UInt8 f63;
    UInt8 rejectZeroLengthArrayMembers;
    UInt8 f65;
    UInt8 f66;
    Boolean f67;
    Boolean f68;
    UInt8 f69;
    char rttiEnabled;
    UInt8 f6b;
    UInt8 pad6c[2];
    UInt8 unsignedChar;
    char f6f;
    UInt8 f70;
    UInt8 f71;
    UInt8 f72;
    UInt8 f73;
    UInt8 f74;
    UInt8 f75;
    UInt8 pad76[1];
    UInt8 f77;
    UInt8 f78;
    UInt8 pad79[1];
    UInt8 f7a;
    char f7b;
    char f7c;
    char f7d;
    UInt8 f7e;
    char f7f;
    UInt8 f80;
    char f81;
    char f82;
    UInt8 f83;
    UInt8 f84;
    Boolean f85;
    UInt8 f86;
    UInt8 f87;
    UInt8 pad88[1];
    UInt8 f89;
    volatile char f8a;
    char f8b;
    UInt8 f8c;
    UInt8 pad8d[1];
    UInt8 f8e;
    Boolean f8f;
    UInt8 f90;
    Boolean f91;
    Boolean f92;
    Boolean f93;
    UInt8 f94;
    UInt8 pad95[1];
    UInt8 f96;
    UInt8 f97;
    UInt8 pad98[4];
    char f9c;
    char f9d;
    UInt8 f9e;
    Boolean f9f;
    UInt8 fa0;
    Boolean fa1;
    char fa2;
    char fa3;
    UInt8 fa4;
    Boolean fa5;
    char fa6;
    Boolean fa7;
    UInt8 fa8;
    SInt8 fa9;
    Boolean faa;
    Boolean fab;
    Boolean fac;
    signed char structalignment;
    UInt8 arrayAlignment;
    SInt8 faf;
    UInt8 fb0;
    UInt8 fb1;
    UInt8 fb2;
    UInt8 fb3;
    UInt8 padb4[1];
    UInt8 disableInlining;
    UInt8 fb6;
    char
        peepholeOptimizationEnabled; /* 0xb7: CodeGen gates Peephole_MergeAdjacentBlocks and Peephole_VisitBlocksWithMultipleInstructions */
    Boolean irOptimizationEnabled;
    unsigned char fb9;
    UInt8 fba;
    UInt8 fbb;
    UInt8 fbc;
    UInt8 fbd;
    UInt8 fbe;
    char fbf;
    char fc0;
    SInt8 deleteDeadInstructions;
    UInt8 uniformSpillBlockWeight;
    UInt8 fc3;
    UInt8
        irCommonSubexpressionElimination; /* 0xc4: IRO_Optimizer gates IRO_CommonSubs and available-expression computation */
    UInt8 fc5;
    UInt8 irCopyPropagation; /* 0xc6: IRO_CopyPropagationSetting */
    UInt8 irEliminateUnused; /* 0xc7: IRO_Optimizer passes eliminateUnused to IRO_UseDef */
    UInt8 fc8;
    UInt8 fc9;
    UInt8 irRemoveUnreachable; /* 0xca: IRO_Optimizer gates IRO_RemoveUnreachable */
    UInt8 fcb;
    UInt8 padcc[1];
    UInt8 irLoopUnrolling; /* 0xcd: IRO_Optimizer gates IRO_LoopUnroller */
    Boolean fce;
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
    struct PFile *file;
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
struct PFile {
    CWFileSpec header; /* 0x00: fn_004401b0 copies the resolved file specification */
    char *textstart;
    SInt32 textlength;
    SInt32 linenumber;
    SInt32 pos;
    Boolean hasprepline;
    UInt8 fileIDAlignment; /* 0x57: fn_004401b0 clears whole PFile; unused byte aligns the following SInt16 fileID */
    SInt16 fileID;
    Boolean recordbrowseinfo;
    UInt8 unusedBytes[3]; /* 0x5b: fn_004401b0 clears these bytes with the whole PFile; no member reads or writes */
    Boolean isDefault;
};
#pragma options align = reset
#pragma options align = mac68k
struct PragmaNode {
    struct PragmaNode *next;
    COpts rec;
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
    struct PFile *name;
    HashNameNode *value;
    SInt32 auxiliaryValue;
};
#pragma options align = reset
struct PrepTokenBuffer {
    int count;
    struct SavedPrepToken *tokens;
};
extern VarInfo *CPrep_AllocateVarInfo(void);
extern PFile *CPrep_GetPFile(void);
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
extern SInt32 CPrep_UpdateTokenLine(FOI *foi);
extern void CPrep_PopFile(void);
extern BufferedToken *CPrep_GetLastBufferedToken(void);
extern int CPrep_0043f860(char *p);
extern void CPrep_ResetBufferedTokenPosition(void);
extern void fn_0043f3b0(short code);
extern NameSpaceList *CPrep_ReportError(short token);
extern void CPrep_RemoveBufferedTokens(int *entryCount, SInt32 *firstIndex);
extern void CPrep_InsertTokenBuffer(PrepTokenBuffer *arg, SInt32 *result);
extern void CPrep_BufferTokensThroughSemicolon(PrepTokenBuffer *buffer, void (*processToken)(struct BufferedToken *));
extern void CPrep_SaveFunctionBodyTokens(PrepTokenBuffer *result, void (*tokenCallback)(BufferedToken *), int option);
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
extern void fn_0043f1f0(FOI *name);
extern void CPrep_SaveAndSetOption(unsigned int index, UInt8 value);
extern void undefine_macro(void);
extern void fn_0043e8f0(void);
extern void parse_else_directive(void);
extern void apply_pragma_object_flags(unsigned int flags);
extern UInt8 CPrep_ExpandMacro(void);
extern Macro *lookup_expandable_macro(void);
extern void CPrep_GetFOI(FOI *location, BufferedToken *record);
extern void CPrep_GetPosition(PFile **position, SInt32 *offset);
extern UInt8 *expand_macro(Macro *m);
extern int scan_braced_tokens(void (*callback)(BufferedToken *), int tokenCount);
extern struct CNameRef evaluate_binary_expression_value(struct CNameRef *init, SInt16 minprec);
extern struct CNameRef evaluate_unary_expression_value(void);
extern void CPrep_GetBrowseFilePosition(PFile **file, SInt32 *ppos);
extern void CPrep_ParseDirective(void);
extern int parse_endif_directive(void);
extern PFile *DAT_005875f8;
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
extern struct BufferedToken *bufferedTokenPosition;
extern UInt8 *token_start;
extern struct PFile *currentPFile;
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
extern struct BufferedToken *buffered_token_buffer_end;
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
extern HashNameNode *fn_00441850(PFile *a0, SInt32 *a1);
struct PFile;
struct ObjectCallbackContext;
struct ObjectCallbackContext;

#ifdef __cplusplus
}
#endif

#endif
