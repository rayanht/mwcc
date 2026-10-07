#ifndef DRIVER_MSDOS_H
#define DRIVER_MSDOS_H

#include "compiler/common.h"
#include "compiler/win32.h"
#include "driver/CLTarg.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSSpec {
    CLTargetDirectory directory;
    char name[0x40];
};
extern unsigned int __stdcall OS_RefToMac(unsigned int value);
extern DWORD __stdcall OS_Create(OSSpec *spec, const unsigned int *options);
extern DWORD __stdcall OS_Status(OSSpec *spec);
extern unsigned int __stdcall fn_00411290(const void *spec, const unsigned int *type);
extern DWORD __stdcall OS_GetFileTime(OSSpec *path, FILETIME *creationTime, FILETIME *writeTime);
extern DWORD __stdcall OS_SetFileTime(OSSpec *path, FILETIME *creationTimeWords, FILETIME *lastWriteTimeWords);
extern DWORD __stdcall OS_Open(OSSpec *path, UInt8 mode, SInt32 *file);
extern DWORD __stdcall OS_Read(int file, LPVOID buffer, SInt32 *byteCount);
extern DWORD __stdcall OS_Seek(int file, UInt8 origin, LONG distance);
extern DWORD __stdcall OS_Close(SInt32 handle);
extern DWORD __stdcall OS_GetSize(int handle, SInt32 *result);
extern DWORD __stdcall OS_SetSize(int file, LONG position);
extern DWORD __stdcall OS_Delete(OSSpec *fileName);
extern DWORD __stdcall OS_Mkdir(OSSpec *path);
extern DWORD __stdcall fn_00411780(OSSpec *path);
extern void ensure_trailing_backslash(char *path);
extern DWORD __stdcall OS_GetCWD(char *text);
extern DWORD redirect_standard_handle_to_file(HANDLE *previousHandle, DWORD standardHandle, LPCSTR filename);
extern char *__stdcall OS_GetDirPtr(char *path);
extern unsigned int OS_CanonPath(const char *src, char *dst);
extern unsigned int __stdcall MsDos_IsAbsolutePath(char *path);
extern int __stdcall OS_EqualPath(const char *left, const char *right);
extern int __stdcall make_osspec_from_path(const char *path, OSSpec *output, Boolean *is_file);
extern SInt32 __stdcall OS_MakeFileSpec(const char *input, OSSpec *output);
extern __stdcall UInt32 OS_MakePathSpec(char *directory, char *path, char *spec);
extern int __stdcall OS_MakeNameSpec(const char *name, char *destination);
extern char *__stdcall OS_SpecToString(OSSpec *spec, char *destination, int capacity);
extern char *__stdcall fn_00412340(char *source, char *destination, unsigned int capacity);
extern char *__stdcall MsDos_CopyStringToBuffer(char *source, char *buffer, unsigned int capacity);
extern int __stdcall OS_EqualSpec(const struct OSSpec *first, const struct OSSpec *second);
extern int __stdcall OS_IsDir(char *path);
extern int __stdcall OS_IsFile(OSSpec *spec);
extern int __stdcall MsDos_ReturnZero(char *a0);
extern int __stdcall fn_004125b0(const OSSpec *source, OSSpec *destination);
extern DWORD __stdcall OS_OpenDir(char *path, struct DirectorySearch *directory);
extern UInt32 __stdcall OS_ReadDir(DirectorySearch *state, OSSpec *spec, char *filename, Boolean *isdir);
extern DWORD __stdcall OS_CloseDir(DirectorySearch *resource);
extern int OS_GetMilliseconds(void);
extern void __stdcall OS_GetTime(_FILETIME *fileTime);
extern DWORD __stdcall OS_NewHandle(SIZE_T size, MemBuffer *block);
extern UInt32 __stdcall OS_ResizeHandle(MemBuffer *buf, UInt32 newsize);
extern HGLOBAL __stdcall MsDos_GetValidMemBufferPtr(MemBuffer *handle);
extern void __stdcall fn_004129c0(MemBuffer *a0);
extern DWORD __stdcall OS_FreeHandle(MemBuffer *handle);
extern unsigned int __stdcall OS_GetHandleSize(MemBuffer *entry, DWORD *value);
extern void __stdcall OS_InvalidateHandle(MemBuffer *buffer);
extern unsigned char __stdcall OS_ValidHandle(MemBuffer *value);
extern int __stdcall OS_OSErrorToMacError(int errorCode);
extern void __stdcall OS_TimeToMac(FILETIME time, int *result);
extern unsigned int __stdcall OS_EqualPathSpec(const char *a0, const char *a1);
extern unsigned int __stdcall equal_path(const char *a0, const char *a1);
extern DWORD __stdcall OS_Tell(int file, long *position);
extern void __stdcall OS_MacToTime(unsigned long secs, FILETIME *ft);
extern unsigned char __ctype_map[];
extern int __stdcall OS_Write(unsigned int handle, LPCVOID buffer, DWORD *size);
extern int __stdcall OS_Execute(OSSpec *name, char **args, char **arg3, char *in, char *out, UInt32 *exitcode);
extern char *__stdcall OS_GetErrText(DWORD errorCode);
extern unsigned int __stdcall fn_004111c0(int *a0, char ***a1);

extern unsigned int data_0054b770;
extern int short_predecessor(short value);
extern Boolean __stdcall MacSpecs_IsByteInDBCSCharacter(BYTE *a, BYTE *b);
extern DWORD __stdcall MacSpecs_LoadMacResource(char *path, LPVOID *resourceData, DWORD *resourceSize);

#ifdef __cplusplus
}
#endif

#endif
