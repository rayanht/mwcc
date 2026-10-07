#ifndef COMPILER_INTRINSICS_H
#define COMPILER_INTRINSICS_H

#include "compiler/common.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IntrinsicBinaryEntry {
    Type *result;
    Type *leftType;
    Type *rightType;
    UInt16 code;
};
#pragma options align = mac68k
struct IntrinsicOperation {
    int operation;
};
#pragma options align = reset
struct IntrinsicTripleEntry {
    Type *result;
    Type *type1;
    Type *type2;
    Type *type3;
    UInt16 code;
};
struct IntrinsicTypeEntry {
    struct Type *
        result; /* 0x00: find_unary_intrinsic_code tests the result type as the alternative-table terminator, like find_binary_intrinsic_code. */
    struct Type *type; /* 0x04: find_unary_intrinsic_code matches this operand type against expression->rtype. */
    UInt16 code;       /* 0x08: find_unary_intrinsic_code returns the matched instruction code. */
};
struct IntrinsicVariant {
    Type *
        resultType; /* 0x00: find_matching_op_result returns the result type; generate_unary_vector_intrinsic and Intrinsics_00486db0 test the table terminator */
    Type *type;
    SInt16 op1;
    SInt16 op2;
    SInt16 op3;
    SInt16 op4;
};
struct MangleEntry {
    SInt32 result;
    struct Type *type;
    SInt32 extra;
};
#pragma options align = mac68k
struct OpEntry {
    struct Type *result;      /* 0x00: find_matching_op_result returns the Intrinsics_MakeAltivecCall call->rtype */
    struct Type *operandType; /* 0x04: find_matching_op_result compares against the argument node->rtype */
    void *a;
    void *b;
};
#pragma options align = reset
struct SimpleEntry {
    Type *result;
    UInt16 code;
};
extern unsigned char Intrinsics_InitRegistrations(unsigned char active);
extern void Intrinsics_GenerateIntrinsicCall(ENode *node, short requestedReg, Operand *result);
extern ENode *Intrinsics_MakeAltivecCall(Object *descriptor, ENodeList *args);
extern void fn_00486ad0(ENode *node, short unused, Operand *result, short target, unsigned short kind);
extern void emit_two_gpr_immediate_instruction(ENode *p1, ENode *p2, ENode *p3, SInt16 op);
extern void emit_three_vr_instruction(ENode *e1, ENode *e2, ENode *e3, SInt16 dstreg, Operand *dst, SInt16 op);
extern void Intrinsics_RegisterIntrinsics(void);
extern UInt16 find_intrinsic_triple_code(UInt16 id, ENode *unused, ENode *e1, ENode *e2, ENode *e3);
extern void Intrinsics_00486db0(UInt16 tok, ENode *unused, ENode *node, SInt16 reg, Operand *out);
extern UInt16 find_binary_intrinsic_code(UInt16 id, ENode *unused, ENode *left, ENode *right);
extern UInt16 find_unary_intrinsic_code(UInt16 id, ENode *unused, ENode *expression);
extern void generate_unary_vector_intrinsic(UInt16 tok, ENode *unused, ENode *node, SInt16 reg, Operand *out);
extern void emit_record_form_condition(ENode *left, ENode *right, short unused, Operand *result, int opcode,
                                       unsigned short kind);
extern void emit_instruction_with_vr_result(ENode *expression, ENode *left, ENode *right, short opcode, Operand *result,
                                            int instruction);
extern Type *find_matching_op_result(UInt16 token, ENodeList *args, HashNameNode *name);
extern unsigned char is_same_type_or_signedint_compatible(struct Type *firstType, struct Type *secondType);
extern void emit_rlwnm(ENode *p1, ENode *p2, ENode *p3, ENode *p4, short p5, Operand *p6);
extern void emit_rlwimi(ENode *p1, ENode *p2, ENode *p3, ENode *p4, ENode *p5, short unused, Operand *p6);
extern void emit_operation_from_nodes(short operation, ENode *leftNode, ENode *rightNode);
extern void emit_three_gpr_instruction(SInt16 code, ENode *a, ENode *b, ENode *c);
extern void emit_two_operand_gpr_instruction(SInt16 ins, ENode *e1, ENode *e2, SInt16 reg, Operand *dst);
extern char Intrinsics_IsRegisteredObject(ObjBase *a0);
extern Type *match_intrinsic_triple(UInt16 id, ENodeList *args, HashNameNode *name);
extern Type *check_binary_intrinsic_args(UInt16 op, ENodeList *args, HashNameNode *opname);
extern SInt32 select_altivec_mangle_result(UInt16 code, ENodeList *arg2, HashNameNode *nm);
extern short intrinsic_opcodes[];
extern struct Object *data_00587fc0;
extern SInt16 gUsedVirtualRegistersVR;
union IntrinsicTableEntry {
    void *table;
    struct SimpleEntry *simple;
    struct IntrinsicOperation *operation;
    struct IntrinsicVariant *variant;
    struct IntrinsicBinaryEntry *
        binary; /* 0x00: find_binary_intrinsic_code and check_binary_intrinsic_args select binary intrinsic alternatives */
    struct IntrinsicTypeEntry *unary;
    struct IntrinsicTripleEntry *triple;
    struct OpEntry *op;
};
extern TypeIntegral stsignedint;
extern unsigned int Intrinsics_IsMonadicObjrefTypeFuncFlag200Set(ENode *expression);
extern TypeIntegral stdouble;
extern TypeIntegral stfloat;

#ifdef __cplusplus
}
#endif

#endif
