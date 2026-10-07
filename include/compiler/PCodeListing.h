#ifndef COMPILER_PCODELISTING_H
#define COMPILER_PCODELISTING_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void fn_004c4bf0(PCodeInstruction *instruction, char *out);
extern void fn_004c4ba0(void);
extern void fn_004c4bb0(char *arg1, char *arg2);
extern void fn_004c4bc0(const char *format, int register_count);
extern void CodeGen_DumpPCode_004c4bd0(const char *function_name, const char *stage);
extern void fn_004c4be0(void);
extern void format_operand(struct PCodeOperand *n, char *ctx);
struct PCodeBlock;

#ifdef __cplusplus
}
#endif

#endif
