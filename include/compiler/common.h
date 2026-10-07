#ifndef COMPILER_COMMON_H
#define COMPILER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Every record of the program, for use through pointers; each is defined in its owner's header. */
struct AddPropagationEntry;
struct AggregateRecord;
struct AsmOperand;
struct BinaryRecordPrefix;
struct BinaryRecordTrailer;
struct BrowseStreamHeader;
struct BrowserFileHeader;
struct ByteBuffer;
struct BytePattern;
struct OverrideFunc;
struct CMBody;
struct CMNode;
struct CObjCNameList;
struct CPrecElem;
struct CPrecHeader;
struct CInlineInfo;
struct CPrepRec;
struct CPrep_0043afc0_Entry;
struct CSOMRefNode;
struct SETSTRING_T;
struct CallbackCache;
struct CallbackOverlayRecord;
struct MachineInfo;
struct TemplateAction;
struct ClassTypeLink;
struct ConfigurationBlock116;
struct CopyPropagationBitSets;
struct DataPointerObject;
struct DeferredDispatch;
struct DiagnosticContext;
struct DiagnosticDetails;
struct DiagnosticLocation;
struct DiagnosticTarget;
struct DispatchObject;
struct DispatchResult;
struct DispatchTable;
struct DropinCallbackData;
struct DropinDiagnosticTarget;
struct DropinFileValue;
struct DropinLookupResult;
struct DropinRequestData;
struct DropinResultSlot;
struct DropinResultStorage;
struct DwarfSym;
struct PCodeBlock;
struct PCodeBlockLink;
struct ExcBase;
struct ExcFlag;
struct ExportedRecord;
struct PCodeOperand;
struct PCodeLabel;
struct ToolArgumentSet;
struct FileProcessingInfo;
struct FlagTextBuffer;
struct FunctionCallFrame;
struct HINSTANCE__;
struct HRSRC__;
struct InlineSwitchData;
struct IStmtRec;
struct InlineIndexReference;
struct InlineSlot;
struct IntrinsicOperation;
struct License;
struct ListNode;
struct ListNodeLink;
struct MemberFunctionPointerData;
struct MemberPointerConstant;
struct MessageContext;
struct IRONode;
struct ObjectCallbackContext;
struct TemplStack;
struct OpcodeDescriptorTable;
struct PackedConversionResult;
struct PanelEntry;
struct ParserPosition;
struct PayloadWithValue;
struct CallbackAction;
struct TemplateAction;
struct PlugAux;
struct PluginDataCallbacks;
struct PragmaSettings;
struct PreThing;
struct PrecTypeEntry;
struct PCodeBlock;
struct QueryValues;
struct ReadValue;
struct RecordBounds;
struct ObjGenSection;
struct PCodeBlock;
struct HashNameNode;
struct RegisterBinding;
struct RegistrationEntry;
struct RegistrationHashEntry;
struct RegistrationTableEntry;
struct SOMDescriptorOutput;
struct SOMPragmaNames;
struct SavedObjectIndex;
struct SecondaryRegistrationEntry;
struct SerializedFormat;
struct SerializedFormatLink;
struct SerializedLocation;
struct SerializedShortHeader;
struct SerializedValueList;
struct SerializedValueList;
struct SerializedWideHeader;
struct SharedDataHeader;
struct SimpleEntry;
struct SpillInstruction;
struct StatementOperand;
struct StoredRecord;
struct TemplArg;
struct TableObject;
struct TemplateAction;
struct TemplateAction;
struct TemplateAction;
struct TemplateAction;
struct TemplateParameterIDEntry;
struct TemplateAction;
struct ToolArgumentSet;
struct ValuePairState;
struct Deps;
struct _OVERLAPPED;
struct _RTL_CRITICAL_SECTION;
struct _RTL_CRITICAL_SECTION_DEBUG;
struct _WIN32_FIND_DATAA;
struct _struct_519;
typedef struct AccessPathEntry AccessPathEntry;
typedef struct Segments Segments;
typedef struct AccessPaths AccessPaths;
typedef struct DropinFileRecord DropinFileRecord;
typedef struct ArgMatch ArgMatch;
typedef struct ArgumentContext ArgumentContext;
typedef struct AsmOp AsmOp;
typedef struct AsmOperandPattern AsmOperandPattern;
typedef struct AsmOut AsmOut;
typedef struct BClassList BClassList;
typedef struct BE_Rec BE_Rec;
typedef struct BE_SymNode BE_SymNode;
typedef struct BaseOffsetPath BaseOffsetPath;
typedef struct BinaryOperatorResult BinaryOperatorResult;
typedef struct BitVector BitVector;
typedef struct BlockOrderEntry BlockOrderEntry;
typedef struct BrowseOptions BrowseOptions;
typedef struct CPrepFileInfo CPrepFileInfo;
typedef struct BrowserCacheEntry BrowserCacheEntry;
typedef struct BufferUpdate BufferUpdate;
typedef struct TStreamElement TStreamElement;
typedef struct FileOpenOptions FileOpenOptions;
typedef struct BytePair BytePair;
typedef struct ByteQuad ByteQuad;
typedef struct CBlockData CBlockData;
typedef struct ClassFriend ClassFriend;
typedef struct DeclInfo DeclInfo;
typedef struct CClassNode CClassNode;
typedef struct OverrideFunc OverrideFunc;
typedef struct OverrideClassBase OverrideClassBase;
typedef struct OverrideClassBase OverrideClassBase;
typedef struct CColoringList CColoringList;
typedef struct CColoringNode CColoringNode;
typedef struct ClassFriend ClassFriend;
typedef struct TemporaryObject TemporaryObject;
typedef struct ExceptionAction ExceptionAction;
typedef struct ClassFriend ClassFriend;
typedef struct CGLink CGLink;
typedef struct CGList CGList;
typedef struct CIBEntry CIBEntry;
typedef struct CIRTypeName CIRTypeName;
typedef struct CInit CInit;
typedef struct InitInfo InitInfo;
typedef struct CInit_ArgumentList CInit_ArgumentList;
typedef struct PooledString PooledString;
typedef struct CInlineInfo CInlineInfo;
typedef struct CInlineVar CInlineVar;
typedef struct CInt64 CInt64;
typedef struct CLBrowserLookupEntry CLBrowserLookupEntry;
typedef struct CLOverlayEntry CLOverlayEntry;
typedef struct CLOverlayValues CLOverlayValues;
typedef struct CLTarget CLTarget;
typedef struct Project Project;
typedef struct CLState CLState;
typedef struct OSPathSpec OSPathSpec;
typedef struct OSNameSpec OSNameSpec;
typedef struct CLabel CLabel;
typedef struct CMDefInfo CMDefInfo;
typedef struct CMRegisterNode CMRegisterNode;
typedef struct CNameNode CNameNode;
typedef struct CNameRef CNameRef;
typedef struct CObjCInfoRec CObjCInfoRec;
typedef struct OLinkList OLinkList;
typedef struct COptBlock COptBlock;
typedef struct COptBlockLink COptBlockLink;
typedef struct COptCSE COptCSE;
typedef struct CompilerLinkerOptions CompilerLinkerOptions;
typedef struct ExceptionAction ExceptionAction;
typedef struct CParseCacheNode CParseCacheNode;
typedef struct CParseRec CParseRec;
typedef struct ParserTryBlock ParserTryBlock;
typedef struct ObjType ObjType;
typedef struct CPrecNode CPrecNode;
typedef struct ClassFriend ClassFriend;
typedef struct SOMInfoEntry SOMInfoEntry;
typedef struct CPrecStoredReference CPrecStoredReference;
typedef struct CPrecSub CPrecSub;
typedef struct CPrecWrittenEntry CPrecWrittenEntry;
typedef struct CPrepCU CPrepCU;
typedef struct CPrepValue CPrepValue;
typedef struct CRec CRec;
typedef struct NameResult NameResult;
typedef struct CScopeSave CScopeSave;
typedef struct TemplArg TemplArg;
typedef struct CWFileSpec CWFileSpec;
typedef struct CWObjectFlags CWObjectFlags;
typedef struct CWPluginPrivateContext CWPluginPrivateContext;
typedef struct CachedOpcodeMetadata CachedOpcodeMetadata;
typedef struct CallbackPathEntry CallbackPathEntry;
typedef struct CallbackRecord CallbackRecord;
typedef struct CaseRange CaseRange;
typedef struct ChainRec ChainRec;
typedef struct ChainRecord ChainRecord;
typedef struct OverrideFunc OverrideFunc;
typedef struct OverrideClassBase OverrideClassBase;
typedef struct OverrideClass OverrideClass;
typedef struct OverrideClassBase OverrideClassBase;
typedef struct ClassLayout ClassLayout;
typedef struct OverrideFunc OverrideFunc;
typedef struct ClassList ClassList;
typedef struct ClassLookupResult ClassLookupResult;
typedef struct ClassNode ClassNode;
typedef struct TemplPartialSpec TemplPartialSpec;
typedef struct ClassTypeUpdate ClassTypeUpdate;
typedef struct CleanNode CleanNode;
typedef struct CWFileSpec CWFileSpec;
typedef struct CmpCtx CmpCtx;
typedef struct CodeBits CodeBits;
typedef struct CMRegisterNode CMRegisterNode;
typedef struct CodeMotionBits CodeMotionBits;
typedef struct CodeMotionCandidate CodeMotionCandidate;
typedef struct CodeMotionEntry CodeMotionEntry;
typedef struct CodeMotionEntryLink CodeMotionEntryLink;
typedef struct CodeMotionListNode CodeMotionListNode;
typedef struct CodeMotionObjectNode CodeMotionObjectNode;
typedef struct CodeMotionRec CodeMotionRec;
typedef struct CodeMotionRef CodeMotionRef;
typedef struct CommandLineArguments CommandLineArguments;
typedef struct PBackEnd PBackEnd;
typedef struct PCLTExtras PCLTExtras;
typedef struct PCmdLine PCmdLine;
typedef struct PCmdLineCompiler PCmdLineCompiler;
typedef struct PCmdLineEnvir PCmdLineEnvir;
typedef struct PDisassembler PDisassembler;
typedef struct PFrontEndC PFrontEndC;
typedef struct PGlobalOptimizer PGlobalOptimizer;
typedef struct CommandParseInfo CommandParseInfo;
typedef struct CompareCase CompareCase;
typedef struct ComparisonValues ComparisonValues;
typedef struct ConstInfo2 ConstInfo2;
typedef struct ConIteratorList ConIteratorList;
typedef struct ConIterator ConIterator;
typedef struct ConversionBlockEntry ConversionBlockEntry;
typedef struct ConversionBlockHeader ConversionBlockHeader;
typedef struct ConversionIterator ConversionIterator;
typedef struct CtorChain CtorChain;
typedef struct DWInfo DWInfo;
typedef struct DataReference DataReference;
typedef struct DeclInfo DeclInfo;
typedef struct DefArgCtorInfo DefArgCtorInfo;
typedef struct TemplateAction TemplateAction;
typedef struct DepRecord DepRecord;
typedef struct DependencyCollection DependencyCollection;
typedef struct DependencyEntry DependencyEntry;
typedef struct Deps Deps;
typedef struct ExceptionAction ExceptionAction;
typedef struct DiagnosticSourcePosition DiagnosticSourcePosition;
typedef struct DirectorySearch DirectorySearch;
typedef struct DivisionParameters DivisionParameters;
typedef struct ParserTool ParserTool;
typedef struct ExportedRecord ExportedRecord;
typedef struct PluginDesc PluginDesc;
typedef struct DropinConfiguration DropinConfiguration;
typedef struct DropinContext DropinContext;
typedef struct DropinFileCallback DropinFileCallback;
typedef struct DropinFileRecord DropinFileRecord;
typedef struct DropinRequest DropinRequest;
typedef struct DstRec DstRec;
typedef struct DwarfFixup DwarfFixup;
typedef struct DwarfFunctionState DwarfFunctionState;
typedef struct DwarfLocationOperand DwarfLocationOperand;
typedef struct DwarfNode DwarfNode;
typedef struct DwarfRef DwarfRef;
typedef struct DwarfStateList DwarfStateList;
typedef struct DwarfSymbol DwarfSymbol;
typedef struct E E;
typedef struct EABIRelocationRecord EABIRelocationRecord;
typedef struct ExceptionAction ExceptionAction;
typedef struct ECacheNode ECacheNode;
typedef struct RTTIBaseRecord RTTIBaseRecord;
typedef struct ENode ENode;
typedef struct ENodeList ENodeList;
typedef struct ERange ERange;
typedef struct ERangeVar ERangeVar;
typedef struct EncodedOperand EncodedOperand;
typedef struct Elf32Header Elf32Header;
typedef struct Elf32Rela Elf32Rela;
typedef struct Elf32Section Elf32Section;
typedef struct ElfHeader ElfHeader;
typedef struct EncodedOperand EncodedOperand;
typedef struct TemplateAction TemplateAction;
typedef struct EnvInfo EnvInfo;
typedef struct EMemberInfo EMemberInfo;
typedef struct PCodeInstruction PCodeInstruction;
typedef struct ExceptSpecList ExceptSpecList;
typedef struct ExceptionAlignmentFill ExceptionAlignmentFill;
typedef struct PCodeInstruction PCodeInstruction;
typedef struct ExceptionByteRecord ExceptionByteRecord;
typedef struct PCodeBlock PCodeBlock;
typedef struct PCodeAssemblyEntry PCodeAssemblyEntry;
typedef struct ExceptionHandlerRecord ExceptionHandlerRecord;
typedef struct PCodeInstruction PCodeInstruction;
typedef struct ExceptionOffsetReference ExceptionOffsetReference;
typedef struct ExceptionOffsetValueReference ExceptionOffsetValueReference;
typedef struct ExceptionOffsetValuesReference ExceptionOffsetValuesReference;
typedef struct ExceptionReferenceTableRecord ExceptionReferenceTableRecord;
typedef struct ExceptionScopeEntry ExceptionScopeEntry;
typedef struct ExceptionShortRecord ExceptionShortRecord;
typedef struct ExceptionTableEntry ExceptionTableEntry;
typedef struct ExceptionTableHeader ExceptionTableHeader;
typedef struct ExceptionTwoOffsetsReference ExceptionTwoOffsetsReference;
typedef struct ExceptionTwoOffsetsValueReference ExceptionTwoOffsetsValueReference;
typedef struct ExceptionTypeReferenceRecord ExceptionTypeReferenceRecord;
typedef struct FILEPATH_T FILEPATH_T;
typedef struct FileOffsetInfo FileOffsetInfo;
typedef struct PCodeOperand PCodeOperand;
typedef struct FTYPE_T FTYPE_T;
typedef struct FileIdentifierInfo FileIdentifierInfo;
typedef struct FileInputNode FileInputNode;
typedef struct FileMap FileMap;
typedef struct FileMapInfo FileMapInfo;
typedef struct FileOpenOptions FileOpenOptions;
typedef struct FileOperationInfo FileOperationInfo;
typedef struct FileQueryInfo FileQueryInfo;
typedef struct Float Float;
typedef struct FloatFormatBuffer FloatFormatBuffer;
typedef struct FloatNode FloatNode;
typedef struct FuncArg FuncArg;
typedef struct MethRec MethRec;
typedef struct GList GList;
typedef struct HashEntry HashEntry;
typedef struct HashNameNode HashNameNode;
typedef struct HelpColumn HelpColumn;
typedef struct IFixup IFixup;
typedef struct SwitchInfo SwitchInfo;
typedef struct IROAddrRecord IROAddrRecord;
typedef struct IRODef IRODef;
typedef struct IROElmList IROElmList;
typedef struct IROExpr IROExpr;
typedef struct IROLinear IROLinear;
typedef struct IROList IROList;
typedef struct IROLoop IROLoop;
typedef struct IROLoopInd IROLoopInd;
typedef struct IRONode IRONode;
typedef struct IROOptNode IROOptNode;
typedef struct IROUse IROUse;
typedef struct IROVarPart IROVarPart;
typedef struct BitVector BitVector;
typedef struct IStmtRec IStmtRec;
typedef struct STRING_T STRING_T;
typedef struct IdentifierListNode IdentifierListNode;
typedef struct IncludeFilenamePointer IncludeFilenamePointer;
typedef struct IncludeSearchPolicy IncludeSearchPolicy;
typedef struct IndexedEntry IndexedEntry;
typedef struct IndexedListLink IndexedListLink;
typedef struct IndexedValueReference IndexedValueReference;
typedef struct InitListItem InitListItem;
typedef struct InitializationAuxiliaryState InitializationAuxiliaryState;
typedef struct InitializerData InitializerData;
typedef struct InitializerEntry InitializerEntry;
typedef struct CInlineInfo CInlineInfo;
typedef struct InlNode InlNode;
typedef struct InlineAsmExpression InlineAsmExpression;
typedef struct InlineAsmRegisterEntry InlineAsmRegisterEntry;
typedef struct InlineMemberPointerTarget InlineMemberPointerTarget;
typedef struct InlineNode InlineNode;
typedef struct InlineObjectEntry InlineObjectEntry;
typedef struct CIBEntry CIBEntry;
typedef struct InlineSwitchData InlineSwitchData;
typedef struct InlineXRef InlineXRef;
typedef struct IntegerSequenceResult IntegerSequenceResult;
typedef struct InterferenceNode InterferenceNode;
typedef struct InterruptGenerationRecord InterruptGenerationRecord;
typedef struct InterruptGenerationRecord InterruptGenerationRecord;
typedef struct InterruptList InterruptList;
typedef struct IntrinsicBinaryEntry IntrinsicBinaryEntry;
typedef struct IntrinsicTripleEntry IntrinsicTripleEntry;
typedef struct IntrinsicTypeEntry IntrinsicTypeEntry;
typedef struct IntrinsicVariant IntrinsicVariant;
typedef struct SimpleEntry SimpleEntry;
typedef struct IROUse IROUse;
typedef struct IrNodeList IrNodeList;
typedef struct ParsedAsmInstruction ParsedAsmInstruction;
typedef struct IvarEntry IvarEntry;
typedef struct TemplateMember TemplateMember;
typedef struct PCmdLineLinker PCmdLineLinker;
typedef struct PLinker PLinker;
typedef struct PProject PProject;
typedef struct PWarningC PWarningC;
typedef struct List12 List12;
typedef struct ListLink ListLink;
typedef struct DropinFileCallback DropinFileCallback;
typedef struct CScopeNSIterator CScopeNSIterator;
typedef struct Loop Loop;
typedef struct LoopCandidate LoopCandidate;
typedef struct LoopVar LoopVar;
typedef struct MOutBuf MOutBuf;
typedef struct MWInfo MWInfo;
typedef struct MacFileTypeNode MacFileTypeNode;
typedef struct MacSpecEntry MacSpecEntry;
typedef struct MachineInfo MachineInfo;
typedef struct MachineOpcodeInfo MachineOpcodeInfo;
typedef struct PipelineStage PipelineStage;
typedef struct CompletionEntry CompletionEntry;
typedef struct Macro Macro;
typedef struct MacroStack MacroStack;
typedef struct MangleEntry MangleEntry;
typedef struct HashNameNode HashNameNode;
typedef struct ObjectList ObjectList;
typedef struct OSHandle OSHandle;
typedef struct MemberCallArguments MemberCallArguments;
typedef struct BigDeclInfo BigDeclInfo;
typedef struct EMemberInfo EMemberInfo;
typedef struct MemberPointerInitializer MemberPointerInitializer;
typedef struct OLinkList OLinkList;
typedef struct ObjMemberVarPath ObjMemberVarPath;
typedef struct MemoNode MemoNode;
typedef struct MessageArgument MessageArgument;
typedef struct MessagePosition MessagePosition;
typedef struct MethRec MethRec;
typedef struct DiagnosticSourcePosition DiagnosticSourcePosition;
typedef struct NameBuf NameBuf;
typedef struct NameEntry NameEntry;
typedef struct NameLookupLink NameLookupLink;
typedef struct NamePiece NamePiece;
typedef struct NameRegistryEntry NameRegistryEntry;
typedef struct NameSpace NameSpace;
typedef struct NameSpaceList NameSpaceList;
typedef struct NameSpaceName NameSpaceName;
typedef struct NameSpaceObjectList NameSpaceObjectList;
typedef struct NameTableEntry NameTableEntry;
typedef struct NamedObjectCacheEntry NamedObjectCacheEntry;
typedef struct NamespaceOperationContext NamespaceOperationContext;
typedef struct NamespaceOperationState NamespaceOperationState;
typedef struct TemplateFriend TemplateFriend;
typedef struct OSPathSpec OSPathSpec;
typedef struct OSSpec OSSpec;
typedef struct OStack OStack;
typedef struct ObjBase ObjBase;
typedef struct ObjCDefinition ObjCDefinition;
typedef struct ObjCDescriptor ObjCDescriptor;
typedef struct ObjCInfo ObjCInfo;
typedef struct ObjCParameterNode ObjCParameterNode;
typedef struct ObjCSymbolTable ObjCSymbolTable;
typedef struct CPrepCU CPrepCU;
typedef struct ObjEnumConst ObjEnumConst;
typedef struct ObjGenRelocation ObjGenRelocation;
typedef struct ObjGenRelocationRequest ObjGenRelocationRequest;
typedef struct ObjGenSection ObjGenSection;
typedef struct ObjGenSymbolLink ObjGenSymbolLink;
typedef struct ObjMemberVar ObjMemberVar;
typedef struct ObjNameSpace ObjNameSpace;
typedef struct ObjType ObjType;
typedef struct ObjcModule ObjcModule;
typedef struct Object Object;
typedef struct ObjectGroup ObjectGroup;
typedef struct ObjectIndexEntry ObjectIndexEntry;
typedef struct ObjectList ObjectList;
typedef struct ObjectOffsetEntry ObjectOffsetEntry;
typedef struct EncodedOperand EncodedOperand;
typedef struct ParsedAsmInstruction ParsedAsmInstruction;
typedef struct OffsetEntry OffsetEntry;
typedef struct OpEntry OpEntry;
typedef struct Operand Operand;
typedef struct OperationRecord OperationRecord;
typedef struct OptimizerOccurrence OptimizerOccurrence;
typedef struct Option Option;
typedef struct OptionEntry OptionEntry;
typedef struct OptionList OptionList;
typedef struct EncodedOperand EncodedOperand;
typedef struct OutputBufferState OutputBufferState;
typedef struct SerializedFormat SerializedFormat;
typedef struct SerializedFormatLink SerializedFormatLink;
typedef struct SerializedValueList SerializedValueList;
typedef struct OverlayAllocation OverlayAllocation;
typedef struct Overlays Overlays;
typedef struct OverrideClassBase OverrideClassBase;
typedef struct DependencyEntry DependencyEntry;
typedef struct PARAM_T PARAM_T;
typedef struct GENERIC_T GENERIC_T;
typedef struct IFARG_T IFARG_T;
typedef struct MASK_T MASK_T;
typedef struct NUM_T NUM_T;
typedef struct ONOFF_T ONOFF_T;
typedef struct SETTING_T SETTING_T;
typedef struct OFFON_T OFFON_T;
typedef struct SET_T SET_T;
typedef struct TOGGLE_T TOGGLE_T;
typedef struct PCodeBlock PCodeBlock;
typedef struct PCodeBlockLink PCodeBlockLink;
typedef struct PCodeBlockLiveness PCodeBlockLiveness;
typedef struct PCodeInstruction PCodeInstruction;
typedef struct PCodeLabel PCodeLabel;
typedef struct PCodeLiveness PCodeLiveness;
typedef struct PCodeOpcodeDescriptor PCodeOpcodeDescriptor;
typedef struct PCodeOperand PCodeOperand;
typedef union TData TData;
typedef struct ParsedAsmInstruction ParsedAsmInstruction;
typedef struct NameSpaceObjectList NameSpaceObjectList;
typedef struct OSPathSpec OSPathSpec;
typedef struct PeepHandler PeepHandler;
typedef struct PendingBuffer PendingBuffer;
typedef struct PendingFunction PendingFunction;
typedef struct PendingObject PendingObject;
typedef struct PendingThunk PendingThunk;
typedef struct Plugin Plugin;
typedef struct Plugin Plugin;
typedef struct PluginDesc PluginDesc;
typedef struct PluginDirectoryList PluginDirectoryList;
typedef struct PluginOptionalData PluginOptionalData;
typedef struct PluginOutputItem PluginOutputItem;
typedef struct PluginQueryTable PluginQueryTable;
typedef struct PluginRequest PluginRequest;
typedef struct PluginRequiredInputRecord PluginRequiredInputRecord;
typedef struct HeapMem HeapMem;
typedef struct HeapBlock HeapBlock;
typedef struct Pragma Pragma;
typedef struct PragmaNode PragmaNode;
typedef struct TemplParam TemplParam;
typedef struct TemplateFriend TemplateFriend;
typedef struct IStmtRec IStmtRec;
typedef struct TemplateMember TemplateMember;
typedef struct TemplPartialSpec TemplPartialSpec;
typedef struct InlineSwitchData InlineSwitchData;
typedef struct PluginDirectoryList PluginDirectoryList;
typedef struct PrepNameCacheEntry PrepNameCacheEntry;
typedef struct TokenStream TokenStream;
typedef struct PtrList PtrList;
typedef struct QualNode QualNode;
typedef struct RData RData;
typedef struct RTTIOffsetEntry RTTIOffsetEntry;
typedef struct RTTIVTableOffsetNode RTTIVTableOffsetNode;
typedef struct PCodeOpcodeDescriptor PCodeOpcodeDescriptor;
typedef struct RecBaseEntry RecBaseEntry;
typedef struct RecordData RecordData;
typedef struct RecordQuery RecordQuery;
typedef struct CMRegisterNode CMRegisterNode;
typedef struct Loop Loop;
typedef struct Loop Loop;
typedef struct CodeMotionCandidate CodeMotionCandidate;
typedef struct RefEntry RefEntry;
typedef struct ReferenceFlags ReferenceFlags;
typedef struct RegisterBlockLiveness RegisterBlockLiveness;
typedef struct RegisterRewriteEntry RegisterRewriteEntry;
typedef struct RegisterValueRecord RegisterValueRecord;
typedef struct RegisterValueSnapshot RegisterValueSnapshot;
typedef struct RegisterValueState RegisterValueState;
typedef struct OLinkList OLinkList;
typedef struct RelocationRecord RelocationRecord;
typedef struct ReplacementCandidate ReplacementCandidate;
typedef struct ResData ResData;
typedef struct ResEntry ResEntry;
typedef struct ResFile ResFile;
typedef struct ResHead ResHead;
typedef struct ResMap ResMap;
typedef struct ResType ResType;
typedef struct TypeBitfield TypeBitfield;
typedef struct PrefDataPanel PrefDataPanel;
typedef struct ResourceDataHeader ResourceDataHeader;
typedef struct ResourceRegistration ResourceRegistration;
typedef struct ResourceTypeEntry ResourceTypeEntry;
typedef struct SClassInfo SClassInfo;
typedef struct SETSTRING_T SETSTRING_T;
typedef struct SOMClassBuildState SOMClassBuildState;
typedef struct SOMClassDescriptor SOMClassDescriptor;
typedef struct SOMEntry SOMEntry;
typedef struct SOMInfo SOMInfo;
typedef struct SOMInfoEntry SOMInfoEntry;
typedef struct SOMSlot SOMSlot;
typedef struct SOMVTable SOMVTable;
typedef struct STRING_T STRING_T;
typedef struct DeclBlock DeclBlock;
typedef struct SavedPrepTokenList SavedPrepTokenList;
typedef struct ValueUpdate ValueUpdate;
typedef struct ScaleTarget ScaleTarget;
typedef struct ScaleTargetGroup ScaleTargetGroup;
typedef struct SchedEntry SchedEntry;
typedef struct NameSpaceLookupList NameSpaceLookupList;
typedef struct CScopeObjectIterator CScopeObjectIterator;
typedef struct Scratch Scratch;
typedef struct SectionAttributeNode SectionAttributeNode;
typedef struct SectionRec SectionRec;
typedef struct SectionSymbolAttributes SectionSymbolAttributes;
typedef struct SelectedNode SelectedNode;
typedef struct SelectorMethod SelectorMethod;
typedef struct SerializedBucketEntry SerializedBucketEntry;
typedef struct EncodedOperand EncodedOperand;
typedef struct PCodeInstruction PCodeInstruction;
typedef struct SOMClassBuildState SOMClassBuildState;
typedef struct SOMEntry SOMEntry;
typedef struct ValueUpdate ValueUpdate;
typedef struct PCodeOperand PCodeOperand;
typedef struct Statement Statement;
typedef struct StatementContext StatementContext;
typedef struct StorageHandle StorageHandle;
typedef struct StrBuf StrBuf;
typedef struct StructMember StructMember;
typedef struct SwitchCase SwitchCase;
typedef struct SwitchInfo SwitchInfo;
typedef struct TB TB;
typedef struct TNode TNode;
typedef struct TOCEntry TOCEntry;
typedef struct TOCNameEntry TOCNameEntry;
typedef struct TOCReferenceEntry TOCReferenceEntry;
typedef struct Statement Statement;
typedef struct ParsedAsmInstruction ParsedAsmInstruction;
typedef struct CWPluginPrivateContext CWPluginPrivateContext;
typedef struct TargetInfo TargetInfo;
typedef struct ParsedAsmInstruction ParsedAsmInstruction;
typedef struct TemplParamID TemplParamID;
typedef struct TemplStack TemplStack;
typedef struct PackedDeclInfo PackedDeclInfo;
typedef struct TemplArg TemplArg;
typedef struct TemplateMember TemplateMember;
typedef struct TemplateAction TemplateAction;
typedef struct TemplateClassMatch TemplateClassMatch;
typedef struct TemplateAction TemplateAction;
typedef struct TypeDeduce TypeDeduce;
typedef struct TemplateFriend TemplateFriend;
typedef struct ObjEnumConst ObjEnumConst;
typedef struct TemplateFunction TemplateFunction;
typedef struct DefAction DefAction;
typedef struct TemplateLookupContext TemplateLookupContext;
typedef struct TemplateMatchItem TemplateMatchItem;
typedef struct DeduceInfo DeduceInfo;
typedef struct ObjectTemplated ObjectTemplated;
typedef struct TemplParam TemplParam;
typedef struct TemplParam TemplParam;
typedef struct TemplateScopeState TemplateScopeState;
typedef struct TemplArg TemplArg;
typedef struct TemplateMember TemplateMember;
typedef struct TemplFuncInstance TemplFuncInstance;
typedef struct TemporaryObject TemporaryObject;
typedef struct TemporaryObjectEntry TemporaryObjectEntry;
typedef struct TgtHead TgtHead;
typedef struct TgtRec TgtRec;
typedef struct TokenText TokenText;
typedef struct ToolCommandLine ToolCommandLine;
typedef struct TemplPartialSpec TemplPartialSpec;
typedef struct TypeClass TypeClass;
typedef struct Triple Triple;
typedef struct Type Type;
typedef struct TypeBitfield TypeBitfield;
typedef struct TypeClass TypeClass;
typedef struct TemplClass TemplClass;
typedef struct TemplClassInst TemplClassInst;
typedef struct TypeEnum TypeEnum;
typedef struct TypeFunc TypeFunc;
typedef struct TypeIntegral TypeIntegral;
typedef struct TypeMemberFunc TypeMemberFunc;
typedef struct TypeMemberPointer TypeMemberPointer;
typedef struct TypePointer TypePointer;
typedef struct TypeStruct TypeStruct;
typedef struct TypeTemplDep TypeTemplDep;
typedef struct VBasePath VBasePath;
typedef struct OverrideClassBase OverrideClassBase;
typedef struct VClassList VClassList;
typedef struct VNode VNode;
typedef struct VTable VTable;
typedef struct VToff VToff;
typedef struct VX VX;
typedef struct ValueDestination ValueDestination;
typedef struct ValueRegisterOperand ValueRegisterOperand;
typedef struct ValueUpdate ValueUpdate;
typedef struct VarInfo VarInfo;
typedef struct VarRecord VarRecord;
typedef struct VectorArrayEntry VectorArrayEntry;
typedef struct VectorArrayUse VectorArrayUse;
typedef struct VirtualFunctionEntry VirtualFunctionEntry;
typedef struct VtOffEntry VtOffEntry;
typedef struct WeirdOperand WeirdOperand;
typedef struct _CONSOLE_SCREEN_BUFFER_INFO _CONSOLE_SCREEN_BUFFER_INFO;
typedef struct _COORD COORD;
typedef struct _FILETIME FILETIME;
typedef struct _FILETIME _FILETIME;
typedef struct _LIST_ENTRY LIST_ENTRY;
typedef struct _PROCESS_INFORMATION PROCESS_INFORMATION;
typedef struct _SECURITY_ATTRIBUTES _SECURITY_ATTRIBUTES;
typedef struct _SMALL_RECT SMALL_RECT;
typedef struct _STARTUPINFOA _STARTUPINFOA;
typedef struct _SYSTEMTIME SYSTEMTIME;
typedef struct _SYSTEMTIME _SYSTEMTIME;
typedef struct _link link;
typedef union BVWord BVWord;
typedef union BrowseObjectBuffer BrowseObjectBuffer;
typedef union CPrecBytes CPrecBytes;
typedef union CPrecKey CPrecKey;
typedef union CPrecPtrU CPrecPtrU;
typedef union ClassTypeHeaderBytes ClassTypeHeaderBytes;
typedef union CodeMotionEntryValue CodeMotionEntryValue;
typedef union ENodeUnion ENodeUnion;
typedef union FloatStorage FloatStorage;
typedef union FunctionTypeBuffer FunctionTypeBuffer;
typedef union InlineOperand InlineOperand;
typedef union LongBytes LongBytes;
typedef union MWVector128 MWVector128;
typedef union OptFlag OptFlag;
typedef union OutputRelocation OutputRelocation;
typedef union PCodeOperandValue PCodeOperandValue;
typedef union SerializedValue SerializedValue;
typedef union U16Bytes U16Bytes;
typedef union UInt32ByteSwapStorage UInt32ByteSwapStorage;
typedef union Val Val;
union MsDosCalendarData;
union _union_518;

#ifndef CERROR_FILE
#define CERROR_FILE __FILE__
#endif
#define CError_ASSERT(line, cond)                                                                                      \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            CError_Internal(CERROR_FILE, line);                                                                        \
    } while (0)
#define CError_FATAL(line) CError_Internal(CERROR_FILE, line)
#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
typedef unsigned int(__stdcall *DispatchCallback)(struct CWPluginPrivateContext *object, char *argument1,
                                                  void *argument2);
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef short SInt16;
typedef unsigned long UInt32;
typedef long SInt32;
typedef unsigned char Boolean;
typedef unsigned int code();
typedef unsigned char undefined;
#ifndef __cplusplus
typedef unsigned char bool;
#endif
typedef unsigned char byte;
typedef unsigned int dword;
/* inferred floating type; verify ABI */
#ifndef __cplusplus
typedef short wchar_t;
#endif
typedef unsigned short word;

typedef enum AccessType { ACCESSPUBLIC, ACCESSPRIVATE, ACCESSPROTECTED, ACCESSNONE } AccessType;
#pragma options align = mac68k
struct TemplParamID {
    UInt16 index;
    UInt8 nindex;
    Boolean type;
};
#pragma options align = reset
/* Object and type qualifiers (Object, DeclInfo and the types' qual). Q_IMPLICIT_WEAK marks what every unit may define
 * (thunks, an inline function's local statics, a static const member initialized in its class): the backend gives it a
 * weak symbol, as it does Q_WEAK. Q_TENTATIVE marks a C tentative definition. Q_ALIGNED_ values are a field, under
 * Q_ALIGNED_MASK. */
enum {
    Q_CONST = 1,
    Q_VOLATILE = 2,
    Q_CV = Q_CONST | Q_VOLATILE,
    Q_ASM = 4,
    Q_PASCAL = 8,
    Q_INLINE = 0x10,
    Q_REFERENCE = 0x20,
    Q_EXPLICIT = 0x40,
    Q_MUTABLE = 0x80,
    Q_VIRTUAL = 0x100,
    Q_FRIEND = 0x200,
    Q_IN = 0x400,
    Q_OUT = 0x800,
    Q_INOUT = 0x1000,
    Q_BYCOPY = 0x2000,
    Q_BYREF = 0x4000,
    Q_ONEWAY = 0x8000,
    Q_INLINE_DATA = 0x10000,
    Q_IMPLICIT_WEAK = 0x20000,
    Q_WEAK = 0x40000,
    Q_MANGLE_NAME = 0x80000,
    Q_IS_OBJC_ID = 0x100000,
    Q_RESTRICT = 0x200000,
    Q_IS_TEMPLATED = 0x400000,
    Q_TENTATIVE = 0x1000000,
    Q_ALIGNED_1 = 0x2000000,
    Q_ALIGNED_2 = 0x4000000,
    Q_ALIGNED_4 = 0x6000000,
    Q_ALIGNED_8 = 0x8000000,
    Q_ALIGNED_16 = 0xA000000,
    Q_ALIGNED_32 = 0xC000000,
    Q_ALIGNED_64 = 0x10000000,
    Q_ALIGNED_128 = 0x12000000,
    Q_ALIGNED_256 = 0x14000000,
    Q_ALIGNED_512 = 0x16000000,
    Q_ALIGNED_1024 = 0x18000000,
    Q_ALIGNED_2048 = 0x1A000000,
    Q_ALIGNED_4096 = 0x1C000000,
    Q_ALIGNED_8192 = 0x1E000000,
    Q_ALIGNED_MASK = 0x1E000000
};
/* An interrupt function: CodeGen sets it under the interrupt options, and StackFrameEABI then adds its interrupt record. */
#define Q_INTERRUPT 0x80000000
#pragma options align = mac68k
struct HashNameNode {
    struct HashNameNode *next;
    SInt32 id;
    SInt16 hashval;
    char name[2];
};
#pragma options align = reset
#pragma options align = mac68k
struct CInt64 {
    SInt32 hi;
    UInt32 lo;
};
#pragma options align = reset
#pragma options align = mac68k
union FloatStorage {
    int words[2];
    double value;
};
#pragma options align = reset
#pragma options align = mac68k
struct Float {
    FloatStorage data;
};
#pragma options align = reset
#pragma options align = mac68k
union MWVector128 {
    UInt8 byteElements[16];  /* 0x00: CMachine_InitVectorMem, TYPESTRUCT stype 4/5/6 */
    UInt16 shortElements[8]; /* 0x00: CMachine_InitVectorMem, TYPESTRUCT stype 7/8/9/14 */
    UInt32 longElements[4];  /* 0x00: CMachine_InitVectorMem, TYPESTRUCT stype 10/11/12; also raw constant comparison */
    float floatElements[4];  /* 0x00: CMachine_InitVectorMem, TYPESTRUCT stype 13 */
};
#pragma options align = reset

#ifdef __cplusplus
}
#endif

#endif
