#ifndef COMPILER_DWARF_H
#define COMPILER_DWARF_H

#include "compiler/common.h"
#include "compiler/CompilerTools.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct DwarfFixup {
    struct DwarfFixup *next;
    struct ObjGenRelocation *reference;
    SInt32 offset;
};
#pragma pack(pop)
#pragma options align = mac68k
struct DwarfFunctionState {
    struct ObjGenSection *section;
    GList info;
    GList lines;
    SInt32 offset;
    SInt32 lineSectionOffset;
    UInt8 lineBaseRelocated;
    UInt8 lineAlignmentPadding;
    struct ObjGenRelocation *pendingReference;
    SInt32 pendingPosition;
    UInt8 pending;
    UInt8 entryAlignmentPadding;
    SInt32 offsets[50];
    struct ObjGenRelocation *entries[50];
    SInt32 depth;
};
#pragma options align = reset
#pragma options align = mac68k
struct DwarfLocationOperand {
    UInt8 kind;
    UInt8 reserved;
    union {
        UInt32 value;
        struct Type *type;
    } metadata;
    union {
        UInt32 value;
        struct {
            UInt8 first;
            UInt8 second;
            UInt32 offset;
        } registers;
        struct {
            UInt32 value;
            UInt8 reg;
        } indirect;
        struct {
            struct Object *first;
            UInt32 second;
        } pair;
        struct {
            UInt32 value;
            UInt16 reserved;
            struct Object *reference;
        } reference;
    } operand;
};
#pragma options align = reset
#pragma pack(push, 1)
struct DwarfSym {
    /* Allocation: DWARF_CreateObjectDebugEntry galloc(sizeof(DwarfNode)), 0x26 bytes; embedded payload, not separately allocated. */
    struct Object *object;
    struct DwarfFixup *fixups;
    SInt32 offset;
    struct DwarfNode *replacement;
};
#pragma pack(pop)
#pragma options align = mac68k
struct DwarfNode {
    struct DwarfNode *next;
    struct DwarfNode *prev;
    struct DWInfo *type;
    struct DwarfFunctionState *scope;
    UInt16 kind;
    union DwarfNodePayload {
        struct {
            UInt16 reg;
            UInt16 reg2;
            SInt16 flags;
            SInt32 offset;
            struct HashNameNode *name;
            UInt32 f20;
            UInt16 f24;
        } var;
        struct {
            struct Object *object;
            SInt32 codeSize;
            SInt32 codeOffset;
            struct DwarfLocationOperand *returnOperand;
            struct PendingObject *pendingObjects;
        } block;
        struct DwarfSym sym;
        UInt8 regByte;
        struct DwarfFixup *fixups;
    } u;
};
#pragma options align = reset
#pragma options align = mac68k
struct DwarfRef {
    UInt16 tag;
    SInt32 num;
    struct QualNode *qualifiers;
    UInt8 valid;
    UInt8 fB;
    struct DWInfo *typeInfo;
};
#pragma options align = reset
/* Type cache entry (find_or_create_dwinfo): the type, its reference and the hash chain link; galloc'd 30 bytes. */
#pragma options align = mac68k
struct DWInfo {
    Type *type;
    DwarfRef rec;
    struct DWInfo *next;
    UInt8 marked;
    UInt8 typeNodeAlignmentPadding;
    struct DwarfNode *typeNode;
};
#pragma options align = reset
/* A list of the functions' DWARF states (data_00587ea8, dwarf_state_list_tail). */
struct DwarfStateList {
    struct DwarfStateList *next;
    struct DwarfFunctionState *state;
};
#pragma pack(push, 2)
struct DwarfSymbol {
    UInt8 header[0x12];
    DwarfFixup *class_references;
    DwarfFixup *references;
    union {
        DwarfSymbol *owner;
        SInt32 offset;
    } u;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct PendingObject {
    struct Object *object;
    struct PendingObject *next;
};
#pragma pack(pop)
/* Linked DWARF type modifiers: pointer, reference, const or volatile. */
struct QualNode {
    struct QualNode *next;
    UInt8 type;
};
extern void DWARF_AddVar(Object *record, SInt32 offset);
extern void DWARF_004ad260(DWInfo *ptype, union DwarfNodePayload *info);
extern void DWARF_004ad570(DWInfo *ptype, union DwarfNodePayload *info);
extern void fn_004ada90(void);
extern void DWARF_WriteDebugInfo(void);
extern int DWARF_CreateBlockNode(Object *object, SInt32 value2, SInt32 value3, ObjGenSection *value4);
extern void DWARF_AppendLongWordLong(unsigned int firstValue, unsigned int secondValue);
extern void DWARF_ReplaceTrailingLongWordLong(unsigned int firstValue, unsigned int secondValue);
extern void emit_compile_unit(struct ObjGenSection *file, UInt8 flag);
extern SInt32 emit_entry_header(SInt16 value);
extern struct DWInfo *find_or_create_dwinfo(struct Type *type);
extern DwarfRef get_type_dwarf_ref(DWInfo *info, UInt16 a, Boolean b);
extern int emit_location_attribute(DwarfLocationOperand *op, UInt16 attribute, unsigned char dereference);
extern UInt16 get_integral_type_code(Type *type);
extern unsigned int DWARF_RestoreFunctionState(void);
extern void emit_class_dwarf(TypeClass *cls);
extern void emit_enum_type(Type *type);
extern void emit_variable_entry(struct DwarfSym *sym, Object *obj, DWInfo *arg);
extern void create_type_node(DWInfo *typeLink);
extern void insert_type_node_before(DWInfo *a, DWInfo *b);
extern void insert_type_nodes_recursive(DWInfo *a, DWInfo *b);
extern void emit_member_pointer_type(DwarfFixup **references, DWInfo *type);
extern void emit_function_type(DwarfFixup **fixups, DWInfo *function);
extern void emit_array_type(Type *ty);
extern void DWARF_004b0a80(TypeStruct *arg);
extern void DWARF_SetupFunctionState(struct ObjGenSection *func);
extern void DWARF_SetupSectionDebugState(struct ObjGenSection *input);
extern SInt32 emit_dwarf_ref(DWInfo *arg);
extern void DWARF_AddLocalVariable(Object *parameter, int offset);
extern void DWARF_CreateObjectDebugEntry(Object *object);
extern void set_type_dwarf_ref(Type *p, UInt16 x, Boolean flag, DwarfRef *out);
extern DwarfRef create_type_dwarf_ref(Type *type, UInt16 qual, Boolean arg4);
extern SInt32 emit_function_entry(Object *func, SInt32 code_size, SInt32 code_offset, DwarfLocationOperand *type_ref,
                                  PendingObject *variables);
extern void DWARF_AddPendingObject(Object *object);
extern void setup_return_operand(Object *func);
extern void DWARF_SetSectionAndState(ObjGenSection *value, ObjGenSection *state);
extern void DWARF_Init(void);
extern void init_dwarf_state(void);
extern struct ObjGenSection *DAT_00587698;
extern struct GList *dwarf_info_buffer;
extern struct DWInfo *dwinfo_buckets[512];
extern struct PendingObject *pending_objects;
extern struct DwarfFunctionState *data_00587168;
extern struct GList *section_buffer;
extern struct DwarfFunctionState *currentDwarfFunctionState;
extern struct ObjGenSection *dwarf_section;
extern struct DwarfStateList *data_00587ea8;
extern struct DwarfNode *current_block_node;
extern struct GList *dwarf_lines;
extern struct DwarfStateList *dwarf_state_list_tail;
extern DwarfLocationOperand return_operand;

#ifdef __cplusplus
}
#endif

#endif
