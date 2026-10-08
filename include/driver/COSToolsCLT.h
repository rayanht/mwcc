#ifndef DRIVER_COSTOOLSCLT_H
#define DRIVER_COSTOOLSCLT_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void copy_pstring(UInt8 *destination, UInt8 *source);
extern void *COS_NewHandle(SInt32 size);
extern void *COS_NewOSHandle(SIZE_T size);
extern void COS_FreeHandle(void *handle);
extern Boolean COS_ResizeHandle(struct StorageHandle *handle, UInt32 size);
extern void COS_LockHandle(void *handle);
extern void COS_LockHandleHi(void *handle);
extern void COS_UnlockHandle(void *entry);
extern UInt32 COS_GetTicks(void);
extern void COS_DateString(UInt32 seconds, SInt16 form, char *text);
extern void COS_TimeString(UInt32 seconds, Boolean wantSeconds, char *text);
extern void COS_GetString(char *buffer, SInt16 id, SInt16 arg);
extern unsigned char COS_IsMultiByte(unsigned char *textStart, unsigned char *bytePosition);
extern int COS_FileNew(CWFileSpec *record, short *output, unsigned int argument4, unsigned int argument5);
extern short COS_FileOpen(CWFileSpec *fileSpec, short *fileRef);
extern SInt16 COS_FileGetType(CWFileSpec *arguments, UInt32 *output);
extern short COS_FileGetSize(short fileRef, SInt32 *size);
extern SInt16 COS_FileRead(SInt16 first, void *second, SInt32 third);
extern SInt16 COS_FileWrite(SInt16 first, void *second, SInt32 third);
extern short COS_FileGetPos(short value, long *result);
extern SInt16 COS_FileSetPos(SInt16 refNum, SInt32 position);
extern void COS_FileClose(short handleIndex);
extern void COS_FileSetFSSpec(void *result, unsigned char *name);
extern void COS_FileGetFSSpecInfo(CWFileSpec *record, unsigned short *tag, SInt32 *value, void *data);
extern void resolve_file_name_to_pascal_string(short category, int recordId, void *inputName);
extern void COS_FileGetPathName(void *destination, CPrepFileInfo *record, SInt32 *result);
extern unsigned int CompilerTools_GetTicks(void);

#ifdef __cplusplus
}
#endif

#endif
