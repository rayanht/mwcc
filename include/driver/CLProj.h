#ifndef DRIVER_CLPROJ_H
#define DRIVER_CLPROJ_H

#include "compiler/common.h"
#include "compiler/win32.h"
#include "driver/CLAccessPaths.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DirectorySearch {
    HANDLE handle;
    LPWIN32_FIND_DATAA findData;
    OSPathBuffer path;
};
struct PathTextBuffer {
    char text[260];
};
extern char data_0054beec[];
extern char this_not_null_string[];
extern char directory_search[];
extern char matching_entry_directory[];
extern char data_0057f36b[];
extern unsigned char CLProj_FreeTargets(void *value);
extern char *__stdcall CLProj_AppendString(char *dest, char *src, int size);
extern char *__stdcall CLProj_CopyStringBounded(char *destination, const char *source, unsigned int count,
                                                int capacity);
extern unsigned char CLProj_InitializeCWD(char *a0);
extern int match_wildcard_pattern(char *pattern, char *str);
extern OSSpec *__stdcall CLProj_FindNextMatchingEntry(char *name);
extern char *__stdcall CLProj_GetFileName(char *path);
extern int __stdcall CLProj_MakeOSSpecFromDirectoryAndFilename(char *directory, char *filename, OSSpec *result);
extern unsigned int __stdcall CLProj_MakeOSSpecFromPath(char *basePath, char *path, UInt8 useSpecialPath,
                                                        OSSpec *destination);
extern unsigned int __stdcall CLProj_SetFileExtension(void *file, const char *extensionAddress, unsigned char append);
extern unsigned int __stdcall CLProj_ChangeFileExtension(char *file, char *extensionAddress);
extern char *__stdcall CLProj_MakeRelativePath(OSSpec *source, char *base, char *destination, int capacity);
extern DWORD __stdcall CLProj_FindFileInSearchPath(char *name, const char *searchPath, OSSpec *result);
extern OSSpec data_0057f018;

#ifdef __cplusplus
}
#endif

#endif
