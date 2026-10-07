#ifndef COMPILER_VALUENUMBERING_H
#define COMPILER_VALUENUMBERING_H

#include "compiler/common.h"
#include "compiler/PCode.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct IndexedValueReference {
    unsigned char kind;
    short index;
};
#pragma options align = reset
struct RegisterValueRecord {
    struct RegisterValueRecord *next;
    struct ValueRegisterOperand *operands;
    struct PCodeInstruction *instruction;
    int related_index;
    int index;
    int input_indices[1];
};
struct RegisterValueState {
    int index;
    struct RegisterValueRecord *value;
};
struct RegisterValueSnapshot {
    struct RegisterValueSnapshot *next;
    PCodeOperand operand;
    int reserved;
    RegisterValueState state;
};
struct ObjectIndexEntry {
    struct ObjectIndexEntry *next;
    struct ObjectIndexEntry *left;
    struct ObjectIndexEntry *right;
    struct Object *object;
    struct RegisterValueState index;
};
struct SavedObjectIndex {
    struct SavedObjectIndex *next;
    char kind;
    char reserved_05[11];
    struct ObjectIndexEntry *entry;
    struct RegisterValueState index;
};

#pragma options align = mac68k
struct ValueDestination {
    char reserved[16];
    struct RegisterValueState value;
};
#pragma options align = reset
struct ValueRegisterOperand {
    struct ValueRegisterOperand *next;
    PCodeOperand operand;
};
#pragma options align = mac68k
struct ValueUpdate {
    struct ValueUpdate
        *next; /* 0x00: copy_register_value_state and invalidate_register_value link snapshots into data_00582c38 */
    PCodeOperand descriptor; /* 0x04: copy_register_value_state saves destination; fn_0051fd70 tests its kind */
    struct ValueDestination
        *destination; /* 0x10: fn_0051fd70 reads only for PCOp_MEMORY; unused for register snapshots */
    struct RegisterValueState
        value; /* 0x14: copy_register_value_state saves destinationRecord for restoration by fn_0051fd70 */
};
#pragma options align = reset
extern void COpt_CopyPropagation(SInt32 mode);
extern SInt32 ValueNumbering_0051f790(RegisterValueRecord *a, PCodeInstruction *b);
extern void ValueNumbering_PerformValueNumbering(int a0);
extern void traverse_single_predecessor_successors(PCodeBlock *node);
extern void fn_0051e720(void);
extern void value_number_block(PCodeBlock *block);
extern void fn_0051ee40(PCodeInstruction *p);
extern void value_number_single_instruction(PCodeInstruction *argument);
extern void value_number_pcode_instruction(PCodeInstruction *value);
extern int find_matching_value_signature(PCodeInstruction *func, PCodeOperand *out);
extern void copy_register_value_state(PCodeOperand *source, PCodeOperand *destination);
extern Boolean compare_register_value_indices(IndexedValueReference *first, IndexedValueReference *second);
extern void initialize_value_states(void);
extern void invalidate_instruction_register_values(PCodeInstruction *blk);
extern void invalidate_register_values(PCodeInstruction *obj);
extern void assign_object_value_index(PCodeInstruction *entry);
extern void value_number_instruction(PCodeInstruction *obj);
extern void invalidate_object_indices(Type *unused, int mode);
extern SInt32 invalidate_register_value(PCodeOperand *sp);
extern void fn_0051ffc0(void);
extern void fn_0051fd70(ValueUpdate *update);
extern void create_register_value_record(PCodeInstruction *instruction);

#ifdef __cplusplus
}
#endif

#endif
