#ifndef DRIVER_COSTOOLSCLT_H
#define DRIVER_COSTOOLSCLT_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void copy_pstring(UInt8 *destination, UInt8 *source);
extern void *fn_00443110(SInt32 a0);
extern void *CompilerTools_AllocateMemoryIfEnabled(SIZE_T size);
extern void fn_00443160(void *a0);
extern Boolean fn_00443170(struct StorageHandle *a0, UInt32 a1);
extern void fn_00443190(void *a0);
extern void fn_004431a0(void *a0);
extern void fn_004431b0(void *a0);
extern UInt32 CompilerTools_GetScaledTicks(void);
extern void CompilerTools_GetResourceCString(char *buffer, SInt16 id, SInt16 arg);
extern unsigned char CompilerTools_IsByteInDBCSCharacter(unsigned char *a0, unsigned char *a1);
extern int fn_00443200(CWFileSpec *record, short *output, unsigned int argument4, unsigned int argument5);
extern short fn_00443250(CWFileSpec *h, short *a1);
extern SInt16 CompilerTools_GetFileType(CWFileSpec *arguments, UInt32 *output);
extern short CompilerTools_GetFileSize(short a0, SInt32 *a1);
extern SInt16 CompilerTools_ReadFile(SInt16 first, void *second, SInt32 third);
extern SInt16 CompilerTools_Write(SInt16 first, void *second, SInt32 third);
extern short fn_004432f0(short value, long *result);
extern SInt16 CompilerTools_SetFilePosition(SInt16 a0, SInt32 a1);
extern void CompilerTools_CloseFile(short a0);
extern void CompilerTools_MakeCWFileSpecFromPString(void *result, unsigned char *name);
extern void CompilerTools_GetPFileFields(CWFileSpec *record, unsigned short *tag, SInt32 *value, void *data);
extern void resolve_file_name_to_pascal_string(short category, int recordId, void *inputName);
extern void CompilerTools_ResolveFileNameToCString(void *destination, PFile *record, SInt32 *result);
extern unsigned int CompilerTools_GetTicks(void);

#ifdef __cplusplus
}
#endif

#endif
