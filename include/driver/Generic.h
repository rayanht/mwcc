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
extern int WildCardMatch(char *pattern, char *str);
extern OSSpec *__stdcall OS_MatchPath(char *name);
extern char *__stdcall OS_GetFileNamePtr(char *path);
extern int __stdcall OS_MakeSpec2(char *directory, char *filename, OSSpec *result);
extern unsigned int __stdcall OS_MakeSpecWithPath(OSPathSpec *basePath, char *path, UInt8 useSpecialPath,
                                                  OSSpec *destination);
extern unsigned int __stdcall OS_NameSpecChangeExtension(OSNameSpec *file, const char *extensionAddress,
                                                         unsigned char append);
extern unsigned int __stdcall OS_NameSpecSetExtension(OSNameSpec *file, const char *extensionAddress);
extern char *__stdcall OS_SpecToStringRelative(OSSpec *source, OSPathSpec *base, char *destination, int capacity);
extern DWORD __stdcall OS_FindFileInPath(char *name, const char *searchPath, OSSpec *result);
extern int __stdcall OS_FindProgram(char *name, void *param2);
extern unsigned int __stdcall OS_CopyHandle(OSHandle *a, OSHandle *b);
extern DWORD __stdcall OS_AppendHandle(void *handle, const void *source, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
