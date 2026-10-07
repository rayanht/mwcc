#include "compiler/common.h"
#include "driver/Generic.h"
#include "compiler/win32.h"
#include "driver/ClientGlue.h"
#include "driver/MsDos.h"
#include "driver/StringExtras.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static OSSpec data_0057f018;
static DirectorySearch directory_search;
static char matching_entry_directory[0x103];
static char data_0057f36b[0x3f];

int WildCardMatch(char *pattern, char *str)
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

OSSpec *__stdcall OS_MatchPath(char *name)
{
    char entryName[0x43];
    Boolean flag;
    OSSpec path;
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
        if (OS_MakePathSpec(NULL, matching_entry_directory, &path.path) != 0)
            return NULL;
        CLProj_CopyStringBounded(data_0057f36b, fileName, -1, 0x40);
        if (OS_MakeNameSpec(data_0057f36b, &path.name) != 0)
            return NULL;
        if (OS_OpenDir(&path.path, &directory_search) != 0)
            return NULL;
    }
    while ((state = &directory_search, spec = &data_0057f018, OS_ReadDir(state, spec, entryName, &flag)) == 0) {
        if (flag != 0 && WildCardMatch(data_0057f36b, entryName) != 0)
            return &data_0057f018;
    }
    state = &directory_search;
    OS_CloseDir(state);
    return NULL;
}

char *__stdcall OS_GetFileNamePtr(char *path)
{
    char *name;
    name = strrchr(path, '\\');
    if (!name)
        name = path;
    else
        name += 1;
    return name;
}

int __stdcall OS_MakeSpec2(char *directory, char *filename, OSSpec *result)
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
    return OS_MakeSpec(path, output, NULL);
}

unsigned int __stdcall OS_MakeSpecWithPath(OSPathSpec *basePath, char *path, UInt8 useSpecialPath, OSSpec *destination)
{
    int hasSpecialCharacters = 0;
    UInt8 specialCharacters;
    char buffer[0x104];

    if (path != NULL && strpbrk(path, "/\\:") != NULL)
        hasSpecialCharacters = 1;
    if (path == NULL) {
        if (basePath != NULL)
            destination->path = *basePath;
        else
            OS_GetCWD(&destination->path);
        return OS_MakeNameSpec("", &destination->name);
    }
    specialCharacters = hasSpecialCharacters;
    if (!((useSpecialPath && specialCharacters) || OS_IsFullPath(path) != 0)) {
        if (basePath != NULL)
            OS_PathSpecToString(basePath, buffer, 0x104);
        else
            buffer[0] = 0;
        {
            char *appendAt = buffer + 0x104 - strlen(path);
            char *end = buffer + strlen(buffer);
            if (end <= appendAt)
                appendAt = end;
            strcpy(appendAt, path);
        }
        return OS_MakeSpec(buffer, destination, NULL);
    }
    return OS_MakeSpec(path, destination, NULL);
}

unsigned int __stdcall OS_NameSpecChangeExtension(OSNameSpec *file, const char *extensionAddress, unsigned char append)
{
    char buffer[64];
    char *extensionStart;
    OS_NameSpecToString(file, buffer, 260U);
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

unsigned int __stdcall OS_NameSpecSetExtension(OSNameSpec *file, const char *extensionAddress)
{
    char path[64];
    char *suffix;
    OS_NameSpecToString(file, path, 260U);
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

static char lbl_0054bf14[] = "%s%s";

char *__stdcall OS_SpecToStringRelative(OSSpec *source, OSPathSpec *base, char *destination, int capacity)
{
    char *sourceCursor;
    char *baseCursor;
    char *outputCursor;
    char sourcePath[260];
    char basePath[260];
    OSPathSpec currentPath;
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
        OS_GetCWD(&currentPath);
        base = &currentPath;
    }
    if (OS_PathSpecToString(base, basePath, sizeof(basePath)) == NULL) {
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

DWORD __stdcall OS_FindFileInPath(char *name, const char *searchPath, OSSpec *result)
{
    const char *separator;
    DWORD status;
    char directory[260];
    while (searchPath != NULL && *searchPath != 0) {
        separator = strchr((char *)searchPath, ';');
        if (separator == NULL) {
            separator = strpbrk(searchPath, ";,");
        }
        if (separator == NULL) {
            separator = searchPath + strlen(searchPath);
        }
        CLProj_CopyStringBounded(directory, searchPath, separator - searchPath, 0x103);
        status = OS_MakeSpec2(directory, name, result);
        if (status == 0) {
            status = OS_Status(result);
            if (status == 0) {
                return 0;
            }
        }
        searchPath = (*separator != 0) ? separator + 1 : NULL;
    }
    status = OS_MakeFileSpec(name, result);
    if (status == 0) {
        status = OS_Status(result);
        if (status == 0) {
            return 0;
        }
    }
    return status;
}

int __stdcall OS_FindProgram(char *name, void *param2)
{
    char buf[0x104];
    char dir[0x104];
    DWORD r;
    char *p;
    OSSpec *output = (OSSpec *)param2;

    strncpy(buf, name, 0x104);
    buf[0x103] = 0;
    if ((UInt32)strlen(buf) < 4 || ClientGlue_CompareLowercaseStrings(buf + strlen(buf) - 4, ".exe") != 0)
        CLProj_AppendString(buf, ".exe", 0x104);

    if (strchr(buf, '\\') == 0) {
        if (GetSystemDirectoryA(dir, 0x104) != 0) {
            if (OS_MakeSpec2(dir, buf, output) == 0) {
                if ((r = OS_Status(output)) == 0)
                    return r;
            }
        }
        if (GetWindowsDirectoryA(dir, 0x104) != 0) {
            if (OS_MakeSpec2(dir, buf, output) == 0) {
                if ((r = OS_Status(output)) == 0)
                    return r;
            }
        }
        p = getenv("PATH");
        if (OS_FindFileInPath(buf, p, output) == 0)
            return 0;
    }

    r = OS_MakeFileSpec(buf, output);
    if (r == 0)
        r = OS_Status(output);
    return r;
}

unsigned int __stdcall OS_CopyHandle(MemBuffer *source, MemBuffer *destination)
{
    unsigned int err;
    DWORD size;
    HGLOBAL sourceData;
    HGLOBAL destinationData;

    err = OS_GetHandleSize(source, &size);
    if (err == 0) {
        err = OS_NewHandle(size, destination);
        if (err == 0) {
            sourceData = OS_LockHandle(source);
            destinationData = OS_LockHandle(destination);
            memcpy(destinationData, sourceData, size);
            OS_UnlockHandle(source);
            OS_UnlockHandle(destination);
            return 0;
        }
    }
    OS_FreeHandle(destination);
    return err;
}

DWORD __stdcall OS_AppendHandle(void *handle, const void *source, unsigned int size)
{
    DWORD result;
    char *buffer;
    DWORD offset;

    result = OS_GetHandleSize((struct MemBuffer *)handle, &offset);
    if (result == 0) {
        result = OS_ResizeHandle((struct MemBuffer *)handle, offset + size);
        if (result == 0) {
            buffer = (char *)OS_LockHandle((struct MemBuffer *)handle);
            if (buffer != 0) {
                memcpy(buffer + offset, source, size);
                OS_UnlockHandle((struct MemBuffer *)handle);
                return 0;
            }
        }
    }
    return result;
}
