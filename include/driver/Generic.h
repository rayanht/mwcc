#ifndef DRIVER_GENERIC_H
#define DRIVER_GENERIC_H

#include "compiler/common.h"
#include "driver/OS.h"
#include "compiler/win32.h"
#include "driver/CLAccessPaths.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DirectorySearch {
    HANDLE handle;
    LPWIN32_FIND_DATAA findData;
    OSPathSpec path;
};
extern int match_wildcard_pattern(char *pattern, char *str);
extern OSSpec *__stdcall CLProj_FindNextMatchingEntry(char *name);
extern char *__stdcall CLProj_GetFileName(char *path);
extern int __stdcall CLProj_MakeOSSpecFromDirectoryAndFilename(char *directory, char *filename, OSSpec *result);
extern unsigned int __stdcall CLProj_MakeOSSpecFromPath(OSPathSpec *basePath, char *path, UInt8 useSpecialPath,
                                                        OSSpec *destination);
extern unsigned int __stdcall CLProj_SetFileExtension(OSNameSpec *file, const char *extensionAddress,
                                                      unsigned char append);
extern unsigned int __stdcall CLProj_ChangeFileExtension(OSNameSpec *file, const char *extensionAddress);
extern char *__stdcall CLProj_MakeRelativePath(OSSpec *source, OSPathSpec *base, char *destination, int capacity);
extern DWORD __stdcall CLProj_FindFileInSearchPath(char *name, const char *searchPath, OSSpec *result);
extern int __stdcall CLFileOps_FindExecutable(char *name, void *param2);
extern unsigned int __stdcall CLFileOps_CopyMemBuffer(MemBuffer *a, MemBuffer *b);
extern DWORD __stdcall CLFileOps_AppendMemBuffer(void *handle, const void *source, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
