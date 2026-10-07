#ifndef COMPILER_FUNCLEVELASMPPC_H
#define COMPILER_FUNCLEVELASMPPC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void FuncLevelAsmPPC_GenerateFunction(Object *func);
extern void fn_004e6e30(void);
extern void FuncLevelAsmPPC_AllocateLocals(void);
extern UInt8 data_005884f4;
extern SInt32 data_005871a0;
extern UInt16 function_header_index;
extern UInt8 func_errors;

#ifdef __cplusplus
}
#endif

#endif
