#ifndef COMPILER_INLINEASMPPC_H
#define COMPILER_INLINEASMPPC_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum PCodeOperandFlags { PCodeOperand_Use = 0x01, PCodeOperand_Definition = 0x02, PCodeOperand_LastUse = 0x04 };
enum Kind { K0, K1, K2, K3, K4, K5 };

#pragma pack(push, 2)
struct PCodeOpcodeDescriptor {
    const char *mnemonic;
    const char *operand_format;
    unsigned char operand_count;
    unsigned char rank;
    unsigned short flags;
    SInt32 encoding;
};
#pragma pack(pop)
#pragma options align = mac68k
struct AsmOperand {
    struct HashNameNode *name;
    struct CLabel *label;
    struct Object *object;
    struct Type *value;
    int register_number;
    char is_register;
};
#pragma options align = reset
#pragma options align = mac68k
struct EncodedOperand {
    char kind;
    char negative;
    union {
        SInt32 value;
        struct CLabel *label;
    } data;
    union {
        struct Object *object;
        struct CLabel *label;
    } target;
    union {
        int value;
        short kind;
        struct {
            unsigned short flags;
            unsigned short register_class;
        } reg;
    } modifier;
};
#pragma options align = reset
#pragma options align = mac68k
struct InlineAsmExpression {
    UInt8 type;
    UInt8 flags;
    SInt16 islocal;
    SInt32 value;
    struct Object *object_label;
    struct Object *object;
    struct Object *second_object;
    struct CLabel *label;
    struct CLabel *second_label;
};
#pragma options align = reset
#pragma pack(push, 1)
/* create_function_asm_directive allocates 0x10 for directives; parse_asm_instruction_operands allocates the 0x08 prefix plus operands. */
struct ParsedAsmInstruction {
    /* create_function_asm_directive: lalloc(0x10) allocation; parse_asm_instruction_operands allocates offsetof(ParsedAsmInstruction, data) + operand_count * sizeof(EncodedOperand). */
    unsigned int opcode;
    unsigned char specialFlags;
    unsigned char branch_flags;
    short operand_count;
    union {
        EncodedOperand operands[1];
        struct {
            struct Object *object;
            SInt32 size;
        } directive;
    } data;
};
#pragma pack(pop)

extern void InlineAsmPPC_00462d70(Statement *stmt, AsmOut *out);
extern SInt32 InlineAsmPPC_004631f0(ParsedAsmInstruction *operand);
extern void InlineAsmPPC_GenerateAsmInstruction(Statement *o);
extern void InlineAsmPPC_ParseInstruction(void);
extern void InlineAsmPPC_ParseDirectiveIdentifier(void);
extern void InlineAsmPPC_ParseDirective(int directive);
extern SInt32 InlineAsmPPC_ClassifyIdentifier(Boolean flag);
extern void *create_function_asm_directive(HashNameNode *name, Boolean arg);
extern void InlineAsmPPC_Init(char mode);
extern void InlineAsmPPC_Initialize(void);
extern const char *InlineAsmPPC_GetOpcodeMnemonic(struct ParsedAsmInstruction *instruction);
extern void encode_expression_operand(EncodedOperand *operand, int minimum, int maximum, char negative);
extern void parse_branch_operand(struct ParsedAsmInstruction *stmt, EncodedOperand *out, Boolean wide, Boolean absolute,
                                 Boolean link);
extern SInt32 data_005652f8;
extern unsigned int data_00587128;
extern SInt32 asm_instruction_count;
extern char inlineAsmMode;
extern PCodeInstruction *create_pcode_asm_instruction(ParsedAsmInstruction *ia, SInt32 argcount, UInt8 flag);
extern UInt8 inlineAsmPPCEnabled;
extern struct PCodeOpcodeDescriptor gPCodeOpcodeDescriptors[];
extern struct ParsedAsmInstruction *parse_asm_instruction_operands(struct AsmOperandPattern *pattern);
extern void parse_register_operand(EncodedOperand *out, int regClass, Boolean arg3);
extern Object *get_struct_or_class_pointer_object(void);
extern int fn_00469d40(void *arg);
extern void parse_expression(InlineAsmExpression *op, int x);
extern void parse_binary_expression_tail(InlineAsmExpression *result, int parseMode);
extern void report_register_error(unsigned int value, unsigned char kind);
extern void report_binary_expression_error(HashNameNode *name1, HashNameNode *name2, SInt16 kind);
extern EncodedOperand *parse_displacement_operand(EncodedOperand *p, struct ParsedAsmInstruction *ctx);
extern void parse_expression_operand(EncodedOperand *dest, struct ParsedAsmInstruction *instruction,
                                     Boolean allowAddress);
extern int check_register_value_range(void);
extern int parse_range_checked_expression(struct EncodedOperand *out, SInt32 lo, SInt32 hi);
extern void evaluate_inline_asm_binary_expression(InlineAsmExpression *left, short op, InlineAsmExpression *right);
extern void parse_unary_expression(InlineAsmExpression *expr, int flag);
extern int parse_constant_in_range(int lowerBound, int upperBound);
extern void InlineAsmPPC_ReplaceObjectReferenceArguments(struct Statement *a, Object *b, ENode *c);
extern Statement *InlineAsmPPC_CopyStatement(Statement *stmt);

#ifdef __cplusplus
}
#endif

#endif
