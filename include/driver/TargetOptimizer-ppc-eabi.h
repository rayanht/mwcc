#ifndef DRIVER_TARGETOPTIMIZER_PPC_EABI_H
#define DRIVER_TARGETOPTIMIZER_PPC_EABI_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/win32.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OperationRecord {
    OSSpec fileSpec;
    struct OSHandle buffer;
    unsigned char loaded;
    unsigned char dirty;
    unsigned char writeBack;
};
extern void fn_00420700(void);
extern unsigned int fn_00420710(OperationRecord *context);
extern DWORD write_file_buffer(struct OperationRecord *file);
extern unsigned int __stdcall TargetOptimizer_ppc_eabi_InitOperationRecord(OSSpec *source, OSHandle *argument,
                                                                           unsigned char flag, OperationRecord *state);
extern int __stdcall TargetOptimizer_ppc_eabi_GetMemBufferPtrAndSize(unsigned char *state, char **firstResult,
                                                                     int *secondResult);
extern unsigned int __stdcall TargetOptimizer_ppc_eabi_UnloadOperationRecord(struct OperationRecord *record);
extern int TargetOptimizer_ppc_eabi_SetOption(short option, char enabled);
extern unsigned int TargetOptimizer_ppc_eabi_ReportScheduling(struct StorageHandle *argument);

#ifdef __cplusplus
}
#endif

#endif
