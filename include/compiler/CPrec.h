#ifndef COMPILER_CPREC_H
#define COMPILER_CPREC_H

#include "compiler/common.h"
#include "compiler/CInline.h"
#include "compiler/CPrep.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct CPrecWrittenEntry {
    struct CPrecWrittenEntry *next; /* 0x00: CPrec_Register and write_pointer_type link hash buckets */
    void *
        object; /* 0x04: CPrec_FindAddrPatch uses the original object address as a heterogeneous key; write_pointer_type uses TypePointer */
    void *
        image_position; /* 0x08: CPrec_NewAddrPatch records the image address; write_pointer_type stores the serialized position */
};
#pragma pack(pop)
union BVWord {
    SInt32 l;
    UInt8 b[4];
};
union CPrecBytes {
    SInt32 value;
    UInt8 bytes[4];
};
struct CPrecElem {
    SInt32 object;
    SInt32 image_position;
    struct SerializedBucketEntry *list;
};
#pragma options align = mac68k
struct CPrecHeader {
    SInt32 magic;
    UInt16 version;
    UInt8 b06;
    UInt8 b07;
    UInt8 kind;
    UInt8 flag;
    UInt8 cplusplus; /* 0x0a: CPrec.c saves copts.cplusplus */
    UInt8 pad0b[0x1d];
    UInt32 fileSize;
    UInt32 compressedSize;
    UInt32 dataOffset;
    SInt32 relocationCount;
    SInt32 relocationSize;
    SInt32 relocationOffset;
    SInt32 objectPatchSize;
    SInt32 objectPatchOffset;
    SInt32 sourcePatchSize;
    SInt32 sourcePatchOffset;
    SInt32 nameCount;
    SInt32 usingsOffset;
    SInt32 classExtensionOffset;
    SInt32 somReferencesOffset;
    SInt32 pendingBuffersOffset;
    SInt32 uniqueID; /* 0x64: CParser_GetUniqueID; CPrec.c restores via fn_004905c0 */
    SInt32 pendingObjectClassesOffset;
    SInt32 classPointerTypeOffset; /* 0x6c: CPrec.c restores class_pointer_type from precompiled_header_base */
    SInt32 idTypeOffset;           /* 0x70: CPrec.c restores id_type from precompiled_header_base */
    SInt32 selTypeOffset;          /* 0x74: CPrec.c restores sel_type from precompiled_header_base */
    SInt32 selectorHashOffset;
    SInt32 classTypeEntriesOffset;
    SInt32 objcRecordsOffset;
    SInt32 selectorReferenceCount; /* 0x84: CPrec.c saves and restores selector_reference_count */
    SInt32
        objcState; /* 0x88: write_precompiled_file saves data_00587f6c, CPrec.c restores it; CObjCModern_ResetGlobals clears this Objective-C state */
    SInt32 objcStringConstantCount; /* 0x8c: CPrec.c saves and restores objc_string_constant_count */
    SInt32 pendingFunctionsOffset;
    SInt32 pendingInlineWorkOffset;
    SInt32 templateFunctionsOffset;
    UInt8 pad9c[0x4c];
    SInt32 hashNameOffsets[0x800];
    SInt32 macroOffsets[0x800];
    SInt32 namespaceNameOffsets[0x400];
};
#pragma options align = reset
union CPrecKey {
    UInt32 v;
    UInt8 b[4];
};
#pragma options align = mac68k
struct CPrecNode {
    struct CPrecNode *next; /* 0x00: CInline_DispatchNextDeferredNode pops pending_prec_nodes */
    struct Object *obj;     /* 0x04: CInline_DispatchNextDeferredNode dispatches the deferred function */
    union {
        struct {
            struct TemplClass
                *classTemplate; /* 0x08: CInline_DispatchNextDeferredNode kind == 1 calls CTemplateNew_CompileObject */
            struct TemplClassInst
                *context; /* 0x0c: CInline_DispatchNextDeferredNode kind == 1 calls CTemplateNew_CompileObject */
            struct TemplateSourceRecordTyped
                *source; /* 0x10: CInline_DispatchNextDeferredNode kind == 1 calls CTemplateNew_CompileObject */
        } k1;
        struct {
            struct TemplateFunction *
                definition; /* 0x08: CInline_DispatchNextDeferredNode kind == 2 calls CTemplateNew_InstantiateFunction */
            struct TemplFuncInstance *
                specialization; /* 0x0c: CInline_DispatchNextDeferredNode kind == 2 calls CTemplateNew_InstantiateFunction */
        } k2;
        struct {
            FileOffsetInfo location; /* 0x08: CInline_AddFunctionPrecNode saves key (kind == 0) */
            TokenStream tokenBuffer; /* 0x12: CInline_AddFunctionPrecNode saves pair (kind == 0) */
            struct TypeClass *
                contextClass; /* 0x1a: CInline_AddFunctionPrecNode saves value; CInline_0050ebf0_inline2 traverses class parents (kind == 0) */
        } k0;
    } u;
    UInt8 kind; /* 0x1e: CInline_DispatchNextDeferredNode selects the payload variant */
    UInt8 pad1f;
};
#pragma options align = reset
#pragma options align = mac68k
union CPrecPtrU {
    UInt8 *address; /* 0x00: build_global_pointer_entries records heterogeneous global addresses for relocation */
};
#pragma options align = reset

#pragma options align = mac68k
struct CPrecSub {
    struct CPrecSub *next;
    struct Object *object;
    struct CTStateElem *templateArguments;
};
#pragma options align = reset
union LongBytes {
    SInt32 l;
    UInt8 b[4];
};
struct ObjectOffsetEntry {
    struct ObjectOffsetEntry *next; /* 0x00: serialize_membervars links entries into written_entry_buckets */
    ObjMemberVar *
        object; /* 0x04: serialize_membervars stores object and object->next; fn_004da6c0_inline2 looks up the next member variable */
    long offset; /* 0x08: serialize_membervars stores and returns the serialized member-variable offset */
};
/* An object whose initialisation is deferred (CException_AddPendingBuffer; the list at pending_buffers): a copy of its
   initial bytes and the relocations to apply to them. Saved into precompiled headers by serialize_pending_buffers. */
struct PendingBuffer {
    struct PendingBuffer *next; /* 0x00: CException_AddPendingBuffer links pending_buffers */
    struct Object *owner; /* 0x04: CException_AddPendingBuffer sets owner; serialize_pending_buffers writes object */
    char *
        buffer; /* 0x08: CException_AddPendingBuffer copies owner->type->size bytes; serialize_pending_buffers reads char data */
    struct RelocationList *value; /* 0x0c: CException_AddPendingBuffer copies relocation list */
    int entryValue; /* 0x10: CException_AddPendingBuffer sets entryValue; CPrec passes it to fn_004ceab0 */
};

struct PrecTypeEntry {
    struct PrecTypeEntry *next;
    struct TypeClass *type;
};

struct SavedPrepTokenList {
    struct SavedPrepTokenList *next; /* 0x00: append_saved_prep_tokens links saved_prep_tokens */
    TStreamElement *offset; /* 0x04: append_saved_prep_tokens stores bp, the appended TStreamElement array position */
    SInt32 count;           /* 0x08: append_saved_prep_tokens stores n, the token count */
};
#pragma options align = mac68k
struct SelectorMethod {
    struct SelectorMethod *next;
    struct MethRec *method;
};
#pragma options align = reset
struct SerializedBucketEntry {
    struct SerializedBucketEntry *next;
    unsigned int offset;
};
#pragma options align = mac68k
struct TemplateFunction {
    struct TemplateFunction *next;
    struct TemplateFunction *original;
    struct HashNameNode *name;
    struct TemplateParameterRecord *
        params; /* 0x0c: parse_function_template_declaration compares template parameters with CTemplTool_EqualParams */
    TokenStream stream;
    TStreamElement deftoken;
    struct Object *tfunc;
    struct TemplFuncInstance *instances;
    struct CPrepFileInfo
        *srcfile; /* 0x38: write_template_function_browse_record reads source fileID and recordbrowseinfo */
    SInt32 startoffset;
    SInt32 endoffset;
};
#pragma options align = reset
struct SelectorMethod;
struct SelectorMethod;
extern void CPrec_LoadPrecompiledHeader(short file, UInt8 *buffer);
extern void restore_macro_lists(void);
extern void patch_buffered_token_locations(void);
extern UInt8 *apply_object_patches(void);
extern void apply_relocations(void);
extern void decompress_precompiled_header(void);
extern int CPrec_WritePrecompiledFile(void);
extern SInt16 write_precompiled_file(void);
extern SInt32 write_serialized_bucket_offsets(void);
extern short encode_zero_runs(char *a0, int a1);
extern UInt32 serialize_cprec_nodes(CPrecNode *info);
extern PendingBuffer *serialize_pending_buffers(PendingBuffer *item);
extern void serialize_namespace_usings_and_hash(void);
extern SInt32 get_namespace_patch(NameSpace *nspace);
extern SInt32 write_namespace_name(NameSpaceName *p, Boolean flag);
extern SInt32 dispatch_obj_by_otype(ObjBase *obj);
extern SInt32 serialize_membervars(ObjMemberVar *object);
extern struct ObjType *append_objtype_image(ObjType *x);
extern ObjType *write_objtype(ObjType *x);
extern SInt32 write_enum_const(ObjEnumConst *p);
extern UInt32 write_prec_recs(struct IStmtRec *recs, SInt16 count);
extern SInt32 write_enode(ENode *node);
extern SInt32 serialize_cpsi_list(struct CException *x);
extern UInt32 write_typeclass(TypeClass *node);
extern TemplateFunction *write_template_function(struct TemplateFunction *x);
extern unsigned int write_prec_input_record(struct TemplateDeclarationData *record);
extern UInt32 serialize_prec_records(struct TemplateSourceRecordTyped *node);
extern SInt32 serialize_pre_nodes(struct TemplateParameterRecord *node);
extern unsigned int serialize_objc_info(struct ObjCInfo *record);
extern CRec *write_crec_list(CRec *x);
extern ObjectList *write_object_list(ObjectList *x);
extern struct CRec *write_crec(struct CRec *x);
extern MethRec *write_methrec(MethRec *x);
extern HashEntry *write_hash_entry(HashEntry *x);
extern SelectorMethod *write_selector_methods(SelectorMethod *x);
extern SInt32 write_som_info(SOMInfo *entry);
extern unsigned int write_vtable(VTable *record);
extern TypeMemberPointer *append_member_pointer_type(TypeMemberPointer *tmemp);
extern TypeFunc *copy_type_func(TypeFunc *tfunc);
extern FuncArg *write_func_args(FuncArg *arg, Boolean naming);
extern TypeStruct *append_type_struct(TypeStruct *tstruct);
extern TypeBitfield *serialize_type_bitfield(TypeBitfield *tbitfield);
extern TypeEnum *append_type_enum(TypeEnum *tenum);
extern SInt32 write_pointer_type(TypePointer *ptr);
extern int hash_pointer_type(TypePointer *type);
extern SInt32 write_bclass_list(BClassList *x);
extern void serialize_macros(void);
extern void append_hash_names(void);
extern unsigned int align_to_four_byte_boundary(void);
extern void fn_004e0010(void *data, UInt32 size);
extern void patch_hash_name_reference(unsigned int value, HashNameNode *record);
extern void write_serialized_buckets(void);
extern unsigned int serialize_pending_object_class_list(struct PendingObjectClass *entry);
extern unsigned int write_csomrefnode_list(struct CSOMRefNode *record);
extern unsigned int serialize_reference_type_entries(unsigned int *entries, short count);
extern unsigned int write_precompiled_expression_record(struct InlineSwitchData *record);
extern unsigned int write_member_func_ref(MemberFuncRef *entry);
extern unsigned int serialize_reference_entries(struct TemplPartialSpec *record);
extern unsigned int write_prec_type_entries(struct PrecTypeEntry *entry);
extern unsigned int serialize_entry_list(struct CFriend *record);
extern unsigned int write_vclasslist(VClassList *record);
extern unsigned int write_except_spec_list(ExceptSpecList *record);
extern UInt32 write_object(Object *obj);
extern int patch_object_reference(SInt32 location, HashNameNode *object);
extern int add_serialized_bucket_entry(SInt32 offset, SInt32 listIndex);
extern SInt32 serialize_template_class_declarations(TemplateClassDeclaration *list);
extern SInt32 serialize_cprec_rec(struct CInlineInfo *p);
extern SInt32 write_namespace_object_list(NameSpaceObjectList *x);
extern SInt32 serialize_ct_state_elems(CTStateElem *p);
extern SInt32 write_objc_parameter_nodes(ObjCParameterNode *p);
extern ClassList *write_class_list(ClassList *x);
extern int write_templdep(TypeTemplDep *node);
extern TStreamElement *append_saved_prep_tokens(TStreamElement *recs, SInt32 n);
extern int write_type(Type *type);
extern SInt16 serialize_precompiled_data(Boolean writePositions);
extern void build_global_pointer_entries(void);
extern void add_written_type_entry(Type *key, int value);
extern CPrecWrittenEntry *fn_004e0680(void *key);
union CPrecInputPointer {
    UInt8 *bytes;
    SInt32 *words;
};
extern struct TemplateFunction *templateFunctions;
extern struct PendingFunction *pending_functions;
extern SInt32 data_00587f6c;
extern int objc_string_constant_count;
extern struct Type *sel_type;
extern char *data_00587e84;

extern unsigned int CException_HashType(Type *a0);
extern void CException_AddPendingBuffer(Object *owner, const void *buffer, RelocationList *value, int entryValue);
extern RelocationList *copy_relocation_list(RelocationList *p);
extern void CExcept_Terminate(void);
extern void CException_ResetPrecompiledState(UInt8 c);

#ifdef __cplusplus
}
#endif

#endif
