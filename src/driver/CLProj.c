#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLProj.h"
#include "compiler/types.h"
#include "compiler/win32.h"
#include "compiler/CExpr2.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateNew.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLFileOps.h"
#include "driver/CLFiles.h"
#include "driver/CLIO.h"
#include "driver/CLPlugins.h"
#include "driver/CLTarg.h"
#include "driver/Files.h"
#include "driver/MsDos.h"
#include <string.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#pragma auto_inline off
#include <string.h>

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

#pragma auto_inline off

#pragma auto_inline reset

char *__stdcall CLProj_AppendString(char *dest, char *src, int size)
{
    char *end;
    char ch;

    end = dest + strlen(dest);
    while ((ch = *src) != '\0' && (int)(end - dest + 1) < size) {
        *end = ch;
        src++;
        end++;
    }
    *end = '\0';
    return dest;
}

char *__stdcall CLProj_CopyStringBounded(char *destination, const char *source, unsigned int count, int capacity)
{
    char *output;

    for (output = destination; (count-- && *source != '\0' && (int)(output - destination + 1) < capacity);
         output = 1 + output) {
        *output = *source;
        ++source;
    }
    *output = '\0';
    return destination;
}

unsigned char CLProj_InitializeCWD(char *a0)
{
    *(unsigned int *)a0 = 0;
    OS_GetCWD(a0 + 4);
    OS_MakeNameSpec("", a0 + 264);
    return 1;
}

unsigned char CLProj_FreeTargets(void *value)
{
    if (value == NULL) {
        CLIO_ReportAssertionFailure("this != NULL", "CLProj.c", 25U);
    }
    CLTarg_FreeTargets(*(struct CLTarget **)value);
    return 1;
}

int match_wildcard_pattern(char *pattern, char *str)
{
    char c;
    char *last;

    if (*str == 0)
        return 0;

    while (*pattern != 0) {
        if (*pattern == '*') {
            c = *++pattern;
            last = NULL;
            while (*str != 0) {
                if (tolower(*str) == tolower(c))
                    last = str;
                str++;
            }
            if (last != NULL)
                str = last;
            if (tolower(*str) != tolower(c))
                return 0;
        } else if (*pattern == '?' && *str != 0) {
            pattern++;
            str++;
            if (*pattern == 0 && *str != 0)
                return 0;
        } else {
            if (tolower(*pattern) == tolower(*str)) {
                pattern++;
                str++;
            } else
                return 0;
        }
    }

    return *str == 0 || (*str == '\\' && str[1] == 0);
}

OSSpec *__stdcall CLProj_FindNextMatchingEntry(char *name)
{
    char entryName[0x43];
    Boolean flag;
    char path[0x144];
    char *fileName;
    DirectorySearch *state;
    OSSpec *spec;
    if (name != NULL) {
        fileName = strrchr(name, '\\');
        if (fileName == NULL) {
            fileName = name;
            CLProj_CopyStringBounded(matching_entry_directory, ".", -1, 0x104);
        } else {
            ++fileName;
            CLProj_CopyStringBounded(matching_entry_directory, name, fileName - name, 0x104);
        }
        if (OS_MakePathSpec(NULL, matching_entry_directory, path) != 0)
            return NULL;
        CLProj_CopyStringBounded(data_0057f36b, fileName, -1, 0x40);
        if (OS_MakeNameSpec(data_0057f36b, path + 0x104) != 0)
            return NULL;
        if (OS_OpenDir(path, (DirectorySearch *)directory_search) != 0)
            return NULL;
    }
    while ((state = (DirectorySearch *)directory_search, spec = &data_0057f018,
            OS_ReadDir(state, spec, entryName, &flag)) == 0) {
        if (flag != 0 && match_wildcard_pattern(data_0057f36b, entryName) != 0)
            return &data_0057f018;
    }
    state = (DirectorySearch *)directory_search;
    OS_CloseDir(state);
    return NULL;
}

char *__stdcall CLProj_GetFileName(char *path)
{
    char *name;
    name = strrchr(path, '\\');
    if (!name)
        name = path;
    else
        name += 1;
    return name;
}

int __stdcall CLProj_MakeOSSpecFromDirectoryAndFilename(char *directory, char *filename, OSSpec *result)
{
    char path[260];
    int directoryLength;
    int filenameLength;
    char *end;
    OSSpec *output;
    if (directory == NULL)
        directory = "";
    if (filename == NULL)
        filename = "";
    filenameLength = strlen(filename);
    directoryLength = strlen(directory);
    if (259 < filenameLength + directoryLength + 1)
        return 0x6f;
    strncpy(path, directory, directoryLength);
    end = path + directoryLength;
    if (end[-1] != '\\') {
        *end = '\\';
        end++;
    }
    strcpy(end, filename);
    output = result;
    return make_osspec_from_path(path, output, NULL);
}

unsigned int __stdcall CLProj_MakeOSSpecFromPath(char *basePath, char *path, UInt8 useSpecialPath, OSSpec *destination)
{
    int hasSpecialCharacters = 0;
    UInt8 specialCharacters;
    char buffer[0x104];

    if (path != NULL && strpbrk(path, "/\\:") != NULL)
        hasSpecialCharacters = 1;
    if (path == NULL) {
        if (basePath != NULL)
            *(PathTextBuffer *)destination->directory.path = *(PathTextBuffer *)basePath;
        else
            OS_GetCWD(destination->directory.path);
        return OS_MakeNameSpec("", destination->name);
    }
    specialCharacters = hasSpecialCharacters;
    if (!((useSpecialPath && specialCharacters) || MsDos_IsAbsolutePath(path) != 0)) {
        if (basePath != NULL)
            fn_00412340(basePath, buffer, 0x104);
        else
            buffer[0] = 0;
        {
            char *appendAt = buffer + 0x104 - strlen(path);
            char *end = buffer + strlen(buffer);
            if (end <= appendAt)
                appendAt = end;
            strcpy(appendAt, path);
        }
        return make_osspec_from_path(buffer, destination, NULL);
    }
    return make_osspec_from_path(path, destination, NULL);
}

unsigned int __stdcall CLProj_SetFileExtension(void *file, const char *extensionAddress, unsigned char append)
{
    char buffer[64];
    char *extensionStart;
    MsDos_CopyStringToBuffer(file, buffer, 260U);
    if (!append) {
        extensionStart = strrchr(buffer, '.');
        if (extensionStart == NULL)
            extensionStart = buffer + strlen(buffer);
    } else {
        extensionStart = buffer + strlen(buffer);
        if (*extensionAddress != '.') {
            *extensionStart = '.';
            extensionStart++;
            *extensionStart = 0;
        }
    }
    if (strlen(buffer) + strlen(extensionAddress) > sizeof(buffer))
        extensionStart = buffer + sizeof(buffer) - 1 - strlen(extensionAddress);
    strcpy(extensionStart, extensionAddress);
    return OS_MakeNameSpec(buffer, file);
}

unsigned int __stdcall CLProj_ChangeFileExtension(char *file, char *extensionAddress)
{
    char path[64];
    char *suffix;
    MsDos_CopyStringToBuffer(file, path, 260U);
    if (*extensionAddress != '.') {
        suffix = strrchr(path, '.');
        if (!suffix)
            suffix = path + strlen(path);
        if (*extensionAddress) {
            if (strlen(path) + 1 >= sizeof(path)) {
                suffix[-1] = '.';
            } else {
                *suffix++ = '.';
            }
        }
    } else {
        suffix = path + strlen(path);
    }
    if (strlen(path) + strlen(extensionAddress) > sizeof(path)) {
        suffix = path + sizeof(path) - 1 - strlen(extensionAddress);
    }
    strcpy(suffix, extensionAddress);
    return OS_MakeNameSpec(path, file);
}

char *__stdcall CLProj_MakeRelativePath(OSSpec *source, char *base, char *destination, int capacity)
{
    char *sourceCursor;
    char *baseCursor;
    char *outputCursor;
    char sourcePath[260];
    char basePath[260];
    char currentPath[260];
    sourceCursor = sourcePath;
    baseCursor = basePath;
    OS_SpecToString(source, sourcePath, sizeof(sourcePath));
    if (capacity == 0) {
        capacity = sizeof(sourcePath);
    }
    if (destination == NULL) {
        destination = (char *)malloc(capacity);
        if (destination == NULL) {
            return NULL;
        }
    }
    if (base == NULL) {
        OS_GetCWD(currentPath);
        base = currentPath;
    }
    if (fn_00412340(base, basePath, sizeof(basePath)) == NULL) {
        memcpy(destination, sourcePath, capacity - 1);
        destination[capacity - 1] = 0;
        return destination;
    }
    for (; *baseCursor != 0 && tolower(*sourceCursor) == tolower(*baseCursor); baseCursor++) {
        sourceCursor++;
    }
    if ((baseCursor - basePath) < (strlen(sourcePath) >> 1) && strlen(baseCursor) > (strlen(sourcePath) >> 1)) {
        memcpy(destination, sourcePath, capacity - 1);
        destination[capacity - 1] = 0;
        return destination;
    }
    while (baseCursor > basePath) {
        baseCursor--;
        sourceCursor--;
        if (*baseCursor == '\\') {
            break;
        }
    }
    if (baseCursor == basePath) {
        strncpy(destination, sourceCursor, capacity - 1);
        destination[capacity - 1] = 0;
    } else {
        baseCursor++;
        if (*baseCursor != 0) {
            outputCursor = destination;
            for (; *baseCursor != 0; baseCursor++) {
                if (*baseCursor == '\\') {
                    outputCursor += sprintf(outputCursor, "..\\");
                }
            }
            strcpy(outputCursor, (sourceCursor + 1));
        } else {
            strncpy(destination, (sourceCursor + 1), capacity - 1);
            destination[capacity - 1] = 0;
        }
    }
    return destination;
}

static inline int applyClassTypes(DropinFileRecord *request)
{
    TypeClassTemplate *classInfo;
    void *fallbackType;
    struct Type *preferredType;
    classInfo = CLPlugins_GetObjectFlags(request->selectedPlugin);
    preferredType = data_00541bfc;
    if (preferredType == NULL) {
        preferredType = classInfo->relatedClass;
    }
    fallbackType = data_00541bf8;
    if (fallbackType == NULL) {
        fallbackType = classInfo->enclosingTemplate;
    }
    return dispatch_output_storage_by_mask(request, 4, (SInt32)fallbackType, (SInt32)preferredType);
}
