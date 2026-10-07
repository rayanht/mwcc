#ifndef COMPILER_INSTRSELECTION_H
#define COMPILER_INSTRSELECTION_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* What an Operand holds (Operand.kind), as Operands.c and InstrSelection.c treat each: a general, floating-point or vector
 * register, an address register plus a displacement or an index register, a register pair, an immediate, a
 * condition-register field, a symbol, memory at one of those. Numbers: kind is a byte. */
#define OpndType_GPR 0
#define OpndType_GPR_ImmOffset 1
#define OpndType_GPR_Indexed 2
#define OpndType_GPRPair 3
#define OpndType_Immediate 4
#define OpndType_FPR 5
#define OpndType_VR 6
#define OpndType_CRField 7
#define OpndType_Symbol 8
#define OpndType_IndirectGPR_ImmOffset 9
#define OpndType_IndirectGPR_Indexed 10
#define OpndType_IndirectSymbol 11

#pragma pack(push, 2)
struct Operand {
    unsigned char kind;
    unsigned char unknown_01;
    short reg;
    short regHi;
    short secondary_reg;
    short displacement;
    unsigned int flags;
    int immediate;
    struct Object *object;
};
#pragma pack(pop)
struct ConstInfo2 {
    SInt32 multiplier;
    SInt32 shift;
};
#pragma options align = mac68k
struct DeferredDispatch {
    unsigned char header[10];
    unsigned char *input;
    struct DispatchResult *result;
};
#pragma options align = reset
struct DispatchResult {
    unsigned char bytes[22];
};
/* Magic-number parameters for unsigned division by a constant. */
struct DivisionParameters {
    int multiplier;
    int addIndicator;
    int shift;
};
struct Operand;
struct Operand;
struct FunctionCallFrame {
    struct FunctionCallFrame *next;
    TypeFunc *functionType;
    Operand operand;
};
extern void emit_postinc_postdec_gpr_pair(ENode *expr, short outputReg, short outputRegHi, Operand *output);
extern int InstrSelection_GetMaskRange(UInt32 mask, SInt16 *first, SInt16 *last);
extern void emit_gpr_comparison(short secondaryReg, ENode *left, ENode *right, Operand *result);
extern void generate_comparison_gpr(ENode *expr, Operand *output, short requestedReg);
extern void InstrSelection_SelectComparison(ENode *n, void *p);
extern unsigned int swap_kind_pairs(unsigned int kind);
extern void generate_condition_branches(ENode *expr, PCodeLabel *trueLabel, PCodeLabel *falseLabel,
                                        PCodeLabel *fallthroughLabel);
extern void generate_comparison(ENode *e, SInt32 a2, SInt32 a3, Operand *out);
extern void InstrSelection_EmitUnaryFPRInstruction(SInt16 opcode, ENode *node, SInt16 reg, Operand *res);
extern void emit_gpr_immediate_operation(SInt16 opcode, ENode *expr, SInt32 value, SInt16 outputReg, Operand *output);
extern unsigned int fn_004b5ce0(void);
extern void generate_conditional_expression(ENode *node, SInt16 outputReg, SInt16 outputReg2, Operand *output);
extern void fn_004b6530(void);
extern void report_fatal_error(void);
extern unsigned char fn_004b4aa0(unsigned char kind);
extern unsigned int generate_intrinsic_or_function_call(ENode *value, unsigned int operand, unsigned int unused,
                                                        Operand *target);
extern void load_float_constant(ENode *a0, unsigned int a1, unsigned int a2, Operand *a3);
extern void InstrSelection_EmitAddImmediate(SInt16 a0, SInt16 a1, int a2);
extern int is_contiguous_mask(unsigned int mask, short *firstBit, short *lastBit);
extern void emit_gpr_immediate_instruction(short opcode, ENode *expr, short value, short outputReg, Operand *output);
extern void make_objref_operand(ENode *node, unsigned int unused1, unsigned int unused2, Operand *result);
extern void select_or(ENode *expr, short outputReg, short outputRegHi, Operand *output);
extern void gen_xor(ENode *expr, short outputReg, short outputRegHi, Operand *output);
extern void select_right_shift(ENode *expr, short outputReg, short outputRegHi, Operand *output);
extern void generate_left_shift(ENode *expr, short outputReg, short outputRegHi, Operand *output);
extern void emit_add(ENode *node, SInt16 a, SInt16 b, Operand *c);
extern void generate_modulo(ENode *node, short outputReg, short outputRegHi, Operand *result);
extern void generate_division(ENode *node, short reg, short flags, Operand *result);
extern void select_monadic_operand(ENode *enode, SInt16 unused2, SInt16 unused3, Operand *out);
extern void generate_gpr_pair_shift(ENode *node, short outputReg, short outputRegHi, Operand *result);
extern void emit_gpr_pair_and(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result);
extern void generate_gpr_pair_assignment(ENode *node, SInt16 requestedReg, SInt16 requestedRegHi, Operand *out);
extern void emit_gpr_pair_multiply(ENode *expr, SInt16 p2, SInt16 p3, Operand *out);
extern void emit_gpr_pair_or(ENode *node, SInt16 reg1, SInt16 reg2, Operand *out);
extern void emit_gpr_pair_subtraction(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result);
extern void emit_gpr_pair_add(ENode *node, short a, short b, Operand *out);
extern void gen_xor_reg_pair(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result);
extern void emit_cmpli_with_addis(SInt16 p1, ENode *node, SInt32 p3, Operand *res);
extern void InstrSelection_004b37b0(short comparison, ENode *input, short sense, Operand *result);
extern void emit_binary_fpr_instruction(short op, ENode *n1, ENode *n2, SInt16 reg, Operand *dst);
extern void emit_conditional_funccall(ENode *e, SInt32 a2, SInt32 a3, struct Operand *out);
extern void emit_multiply(ENode *node, SInt16 dstreg, SInt16 src, Operand *result);
extern void generate_assignment(ENode *node, SInt16 requestedRegister, SInt16 flags, Operand *out);
extern void force_monadic_operand_register(ENode *expr, short outputReg, short outputRegHi, Operand *output);
extern void emit_negation(ENode *node, SInt16 reg1, SInt16 reg2, Operand *out);
extern void compute_unsigned_division_parameters(unsigned int divisor, struct DivisionParameters *parameters);
extern void InstrSelection_GenerateLongLongComparison(ENode *expr, Operand *result, SInt32 flag);
extern int InstrSelection_MatchPostIncDecRegister(ENode *node, Operand *info, SInt32 *size);
extern void generate_postinc_postdec(ENode *expr, SInt32 r1, SInt32 r2, Operand *res);
extern void compute_signed_division_multiplier_shift(SInt32 d, ConstInfo2 *mag);
extern void InstrSelection_EmitBinaryGPRInstruction(short opcode, ENode *left, ENode *right, short outputReg,
                                                    Operand *output);
extern void generate_gpr_pair_division_or_modulo(ENode *node, SInt16 arg2, SInt16 arg3, Operand *result);
extern void emit_fpr_comparison(SInt16 comparison, ENode *left, ENode *right, Operand *result);
extern void emit_bitwise_not(ENode *node, SInt16 r1, SInt16 r2, Operand *dst);
extern void InstrSelection_EmitUnaryGPRInstruction(SInt16 param1, ENode *param2, SInt16 param3, Operand *param4);
extern void generate_gpr_pair_type_conversion(ENode *node, SInt16 reg1, SInt16 reg2, Operand *result);
extern void InstrSelection_EmitThreeOperandFPRInstruction(SInt16 op, ENode *n2, ENode *n3, ENode *n4, SInt16 param5,
                                                          Operand *out);
extern void generate_boolean_expression(ENode *expression, int unused1, int unused2, Operand *result);
extern void make_intval_operand(ENode *node, SInt16 reg1, SInt16 reg2, Operand *op);
extern void select_diadic_left_then_right(ENode *node, SInt32 a, SInt32 b, Operand *ctx);
extern void emit_bitwise_and(ENode *node, SInt16 arg2, SInt16 arg3, Operand *out);
extern void select_subtraction(ENode *input, int resultReg, int secondaryReg, Operand *flags);
extern void select_indirect_operand(ENode *expression, int targetReg, int flags, Operand *result);
extern void emit_vector128_constant(ENode *node, short requestedRegister, short unused, Operand *result);
extern void get_function_type_operand(ENode *lookup, short unused1, short unused2, Operand *result);
extern void generate_type_conversion(ENode *node, short outputReg, short outputRegHi, Operand *result);
extern void get_objaccess_cached_value(ENode *node, UInt32 argument2, UInt32 argument3, Operand *result,
                                       UInt32 argument5);
extern void (*data_00560648[])(void *, short, short, void *);
extern Float float_one;
extern struct FunctionCallFrame *function_call_frames;
extern short gUsedVirtualRegistersFPR;
extern SInt16 gUsedVirtualRegistersGPR;
extern void get_dispatch_result(struct DeferredDispatch *dispatch, unsigned int argument2, unsigned int argument3,
                                struct DispatchResult *output);
extern void InstrSelection_EmitSwitchTables(Object *a0);
extern struct ObjectList *switch_tables;
struct PCodeLabel;
struct PCodeLabel;
extern char vector128_patterns[];
extern MWVector128 alternate_vector_patterns[];
extern TypeIntegral stunsignedlonglong;

#ifdef __cplusplus
}
#endif

#endif
