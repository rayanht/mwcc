#ifndef COMPILER_INLINEASM_H
#define COMPILER_INLINEASM_H

#include <setjmp.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
union SerializedValue {
    struct CLabel *reference;
    struct Object *object;
    int number;
};
#pragma options align = reset
extern char *format_inlineasm_instruction(ENode *info);
extern SInt32 evaluate_binary_expression(SInt32 left);
extern SInt32 InlineAsm_ParseStructOrClassMemberOffset(Type *obj);
extern SInt32 InlineAsm_ParseMemberOffset(Type *type);
extern Boolean InlineAsm_ResolveOperandNameDefault(HashNameNode *name, struct AsmOperand *result);
extern CLabel *InlineAsm_CreateLabel(HashNameNode *name);
extern void InlineAsm_Error(short code);
extern void InlineAsm_LongJump(void);
extern int scan_unary_expression(void);
extern SInt32 InlineAsm_ParseMemberArrayOffset(Type *type);
extern void InlineAsm_ParseAsmStatement(void);
extern void InlineAsm_ParseAsmLines(short value);
extern void parse_asm_lines(volatile SInt16 endToken, int parseOption);
extern void parse_label(void);
extern unsigned int scan_expression(void);
extern unsigned char InlineAsm_ResolveOperandName(HashNameNode *name, struct AsmOperand *operand, char allow_kind2);
extern void *InlineAsm_GetOperandLabel(Statement *owner);
extern Object *InlineAsm_GetObjectByIndex(struct ParsedAsmInstruction *t, SInt32 index, SInt32 *offset);
extern CLabel *InlineAsm_FindOperandLabel(Statement *object);
extern void InlineAsm_CopyAndRemapParsedAsmInstruction(Statement *owner, Statement *links,
                                                       struct ParsedAsmInstruction **result, UInt32 *resultSize);
extern void InlineAsm_RecordObjectUses(Statement *record);
extern void InlineAsm_CopyInstructionAndResolveOperands(Statement *output, struct CLabel **references, char flag,
                                                        ParsedAsmInstruction *source, SInt32 size);
extern jmp_buf inlineAsmJmpBuf;
extern SInt32 DAT_00587f18;
extern jmp_buf data_00583a68;

#ifdef __cplusplus
}
#endif

#endif
