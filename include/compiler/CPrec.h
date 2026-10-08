#ifndef COMPILER_CPREC_H
#define COMPILER_CPREC_H

#include "compiler/common.h"
#include "compiler/templates.h"
#include "compiler/CInline.h"
#include "compiler/CPrep.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct CPrecWrittenEntry {
    struct CPrecWrittenEntry *next;
    void *object;
    void *image_position;
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
    UInt8 cplusplus;
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
    SInt32 uniqueID;
    SInt32 pendingObjectClassesOffset;
    SInt32 classPointerTypeOffset;
    SInt32 idTypeOffset;
    SInt32 selTypeOffset;
    SInt32 selectorHashOffset;
    SInt32 classTypeEntriesOffset;
    SInt32 objcRecordsOffset;
    SInt32 selectorReferenceCount;
    SInt32 objcState;
    SInt32 objcStringConstantCount;
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
    struct CPrecNode *next;
    struct Object *obj;
    union {
        struct {
            struct TemplClass *classTemplate;
            struct TemplClassInst *context;
            struct TemplateMember *source;
        } k1;
        struct {
            struct TemplateFunction *definition;
            struct TemplFuncInstance *specialization;
        } k2;
        struct {
            FileOffsetInfo location;
            TokenStream tokenBuffer;
            struct TypeClass *contextClass;
        } k0;
    } u;
    UInt8 kind;
    UInt8 pad1f;
};
#pragma options align = reset
#pragma options align = mac68k
union CPrecPtrU {
    UInt8 *address;
};
#pragma options align = reset

#pragma options align = mac68k
struct CPrecSub {
    struct CPrecSub *next;
    struct Object *object;
    struct TemplArg *templateArguments;
};
#pragma options align = reset
union LongBytes {
    SInt32 l;
    UInt8 b[4];
};
struct ObjectOffsetEntry {
    struct ObjectOffsetEntry *next;
    ObjMemberVar *object;
    long offset;
};
/* An object whose initialisation is deferred (CException_AddPendingBuffer; the list at pending_buffers): a copy of its
   initial bytes and the relocations to apply to them. Saved into precompiled headers by serialize_pending_buffers. */
struct PendingBuffer {
    struct PendingBuffer *next;
    struct Object *owner;
    char *buffer;
    struct OLinkList *value;
    int entryValue;
};

struct PrecTypeEntry {
    struct PrecTypeEntry *next;
    struct TypeClass *type;
};

struct SavedPrepTokenList {
    struct SavedPrepTokenList *next;
    TStreamElement *offset;
    SInt32 count;
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
extern void CPrec_LoadPrecompiledHeader(short file, UInt8 *buffer);
extern void restore_macro_lists(void);
extern void patch_buffered_token_locations(void);
extern UInt8 *apply_object_patches(void);
extern void apply_relocations(void);
extern void decompress_precompiled_header(void);
extern int CPrec_WritePrecompiledFile(void);
extern SInt16 write_precompiled_file(void);
extern SInt32 write_serialized_bucket_offsets(void);
extern short encode_zero_runs(char *data, int size);
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
extern SInt32 serialize_cpsi_list(struct ExceptionAction *x);
extern UInt32 write_typeclass(TypeClass *node);
extern TemplateFunction *write_template_function(struct TemplateFunction *x);
extern unsigned int write_prec_input_record(struct TemplateFriend *record);
extern UInt32 serialize_prec_records(struct TemplateMember *node);
extern SInt32 serialize_pre_nodes(struct TemplParam *node);
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
extern unsigned int serialize_pending_object_class_list(struct CallbackAction *entry);
extern unsigned int write_csomrefnode_list(struct CSOMRefNode *record);
extern unsigned int serialize_reference_type_entries(unsigned int *entries, short count);
extern unsigned int write_precompiled_expression_record(struct InlineSwitchData *record);
extern unsigned int write_member_func_ref(EMemberInfo *entry);
extern unsigned int serialize_reference_entries(struct TemplPartialSpec *record);
extern unsigned int write_prec_type_entries(struct PrecTypeEntry *entry);
extern unsigned int serialize_entry_list(struct ClassFriend *record);
extern unsigned int write_vclasslist(VClassList *record);
extern unsigned int write_except_spec_list(ExceptSpecList *record);
extern UInt32 write_object(Object *obj);
extern int patch_object_reference(SInt32 location, HashNameNode *object);
extern int add_serialized_bucket_entry(SInt32 offset, SInt32 listIndex);
extern SInt32 serialize_template_class_declarations(TemplateAction *list);
extern SInt32 serialize_cprec_rec(struct CInlineInfo *p);
extern SInt32 write_namespace_object_list(NameSpaceObjectList *x);
extern SInt32 serialize_ct_state_elems(TemplArg *p);
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

extern unsigned int CException_HashType(Type *type);
extern void CException_AddPendingBuffer(Object *owner, const void *buffer, OLinkList *value, int entryValue);
extern OLinkList *copy_relocation_list(OLinkList *p);
extern void fn_004e0970(void);
extern void CException_ResetPrecompiledState(UInt8 c);

#ifdef __cplusplus
}
#endif

#endif
