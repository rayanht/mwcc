#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/MsDos.h"
#include "compiler/win32.h"
#include "compiler/CTemplateNew.h"
#include "driver/AssertionFailure.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLIO.h"
#include "driver/CLTarg.h"
#include "driver/Generic.h"
#include "driver/Memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "driver/MacSpecs.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CPrec.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/DWARF.h"
#include "compiler/IROUseDef.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "driver/StringUtils.h"
typedef DWORD(__stdcall *PFN1)(HANDLE);
typedef DWORD(__stdcall *PFN2)(void);

typedef DWORD(__stdcall *PFN_SetFilePointer)(HANDLE, LONG, PLONG, DWORD);

char *__stdcall OS_GetErrText(DWORD errorCode)
{
    char *lineEnding;

    if (errorCode != 0xfafafafa) {
        FormatMessageA(0x1000, (LPCVOID)0, errorCode, 0, errtext, 0x100, (va_list *)0);
    } else {
        strcpy(errtext, "Unknown process spawning error");
    }
    lineEnding = errtext + strlen(errtext) - 2;
    if ((errtext < lineEnding) && (*lineEnding == '\r') && (lineEnding[1] == '\n')) {
        *lineEnding = '\0';
    }
    return errtext;
}

unsigned int __stdcall fn_004111c0(int *argc, char ***argv)
{
    return 0U;
}

DWORD __stdcall OS_Create(OSSpec *spec, const unsigned int *options)
{
    char *pathResult;
    HANDLE file;

    pathResult = OS_SpecToString(spec, DAT_0057e308, 0x104);
    if (pathResult == (char *)0) {
        return 0x6f;
    }
    file = CreateFileA(DAT_0057e308, 0xc0000000, 1, (LPSECURITY_ATTRIBUTES)0, 2, 0x80, (HANDLE)0);
    if (file == (HANDLE)-1) {
        return GetLastError();
    }
    CloseHandle(file);
    return fn_00411290(spec, options);
}

DWORD __stdcall OS_Status(OSSpec *spec)
{
    char *result;
    DWORD attributes;

    result = OS_SpecToString(spec, DAT_0057e308, 0x104);
    if (result == NULL) {
        return 0x6f;
    }
    attributes = GetFileAttributesA(DAT_0057e308);
    if (attributes == 0xffffffff) {
        attributes = GetLastError();
        return attributes;
    }
    return 0;
}

unsigned int __stdcall fn_00411290(const void *spec, const unsigned int *type)
{
    return 0U;
}

DWORD __stdcall OS_GetFileTime(OSSpec *path, FILETIME *creationTime, FILETIME *writeTime)
{
    DWORD error;
    BOOL succeeded;
    FILETIME lastWriteTime;
    FILETIME fileCreationTime;
    SInt32 file;

    error = OS_Open(path, 0, &file);
    if (error != 0) {
        return error;
    }
    succeeded = GetFileTime((HANDLE)file, &fileCreationTime, NULL, &lastWriteTime);
    if (succeeded == 0) {
        error = GetLastError();
    } else {
        error = 0;
    }
    OS_Close(file);
    if (writeTime != NULL) {
        writeTime->dwLowDateTime = lastWriteTime.dwLowDateTime;
        writeTime->dwHighDateTime = lastWriteTime.dwHighDateTime;
    }
    if (creationTime != NULL) {
        creationTime->dwLowDateTime = fileCreationTime.dwLowDateTime;
        creationTime->dwHighDateTime = fileCreationTime.dwHighDateTime;
    }
    return error;
}

DWORD __stdcall OS_SetFileTime(OSSpec *path, FILETIME *creationTimeWords, FILETIME *lastWriteTimeWords)
{
    DWORD error;
    FILETIME *lastWriteTime;
    BOOL succeeded;
    FILETIME *creationTime;
    SInt32 file;
    FILETIME lastWrite;
    FILETIME creation;

    if (creationTimeWords != NULL) {
        creation.dwLowDateTime = creationTimeWords->dwLowDateTime;
        creation.dwHighDateTime = creationTimeWords->dwHighDateTime;
    }
    if (lastWriteTimeWords != NULL) {
        lastWrite.dwLowDateTime = lastWriteTimeWords->dwLowDateTime;
        lastWrite.dwHighDateTime = lastWriteTimeWords->dwHighDateTime;
    }
    error = OS_Open(path, 1, &file);
    if (error != 0) {
        return error;
    }
    if (lastWriteTimeWords != NULL) {
        lastWriteTime = &lastWrite;
    } else {
        lastWriteTime = NULL;
    }
    if (creationTimeWords != NULL) {
        creationTime = &creation;
    } else {
        creationTime = NULL;
    }
    succeeded = SetFileTime((HANDLE)file, creationTime, NULL, lastWriteTime);
    if (succeeded == 0) {
        error = GetLastError();
    } else {
        error = 0;
    }
    OS_Close(file);
    return error;
}

DWORD __stdcall OS_Open(OSSpec *path, UInt8 mode, SInt32 *file)
{
    DWORD result;
    SInt32 openedFile;
    char *convertedPath;

    convertedPath = OS_SpecToString(path, DAT_0057e308, 0x104);
    if (convertedPath == NULL) {
        return 0x6f;
    }
    openedFile = (SInt32)CreateFileA(DAT_0057e308, open_access_modes[mode], 1, NULL, 3, 0x80, NULL);
    *file = openedFile;
    if ((HANDLE)*file == (HANDLE)0xffffffff) {
        result = GetLastError();
        return result;
    }
    if (mode == 3) {
        result = SetFilePointer((HANDLE)*file, 0, NULL, 2);
        if (result == 0xffffffff) {
            result = GetLastError();
            return result;
        }
    }
    return 0;
}

static inline long FileOffsetDifference(long offset, long base)
{
    return offset - base;
}

int __stdcall OS_Write(unsigned int handle, LPCVOID buffer, DWORD *size)
{
    DWORD position;
    DWORD end;
    DWORD chunkSize;
    DWORD bytesWritten;

    position = SetFilePointer((HANDLE)handle, 0, NULL, 1);
    if (position == -1 || (end = GetFileSize((HANDLE)handle, NULL)) == -1) {
        return GetLastError();
    }
    if (position > end) {
        if (SetFilePointer((HANDLE)handle, end, NULL, 0) == -1) {
            return GetLastError();
        }
        while (position > end) {
            chunkSize = position - end > 32 ? 32 : FileOffsetDifference(position, end);
            if (WriteFile((HANDLE)handle, "", chunkSize, &bytesWritten, NULL) == 0) {
                return GetLastError();
            }
            if (bytesWritten < chunkSize) {
                *size = 0;
                return 0;
            }
            end = end + chunkSize;
        }
    }
    if (WriteFile((HANDLE)handle, buffer, *size, size, NULL) == 0) {
        return GetLastError();
    }
    return 0;
}

DWORD __stdcall OS_Read(int file, LPVOID buffer, SInt32 *byteCount)
{
    DWORD error;
    BOOL success;

    success = ReadFile((HANDLE)file, buffer, *byteCount, (LPDWORD)byteCount, (LPOVERLAPPED)0);
    if (success == 0) {
        error = GetLastError();
        return error;
    }
    return 0;
}

DWORD __stdcall OS_Seek(int file, UInt8 origin, LONG distance)
{
    DWORD result;

    result = SetFilePointer((HANDLE)file, distance, NULL, seek_origins[origin]);
    if (result == 0xffffffff) {
        result = GetLastError();
        return result;
    }
    return 0;
}

DWORD __stdcall OS_Tell(int file, long *position)
{
    *position = SetFilePointer((HANDLE)file, 0, NULL, 1);
    if (*position == (DWORD)-1) {
        return GetLastError();
    }
    return 0;
}

DWORD __stdcall OS_Close(SInt32 handle)
{
    if (CloseHandle((HANDLE)handle) == 0)
        return GetLastError();
    return 0;
}

DWORD __stdcall OS_GetSize(int handle, SInt32 *result)
{
    *result = GetFileSize((HANDLE)handle, NULL);
    if (*result == -1) {
        return GetLastError();
    }
    return 0;
}

DWORD __stdcall OS_SetSize(int file, LONG position)
{
    DWORD originalPosition;
    DWORD newPosition;
    BOOL endOfFileSet;

    originalPosition = SetFilePointer((HANDLE)file, 0, (PLONG)0, 1);
    if (originalPosition == 0xffffffff ||
        (newPosition = SetFilePointer((HANDLE)file, position, (PLONG)0, 0)) == 0xffffffff ||
        (endOfFileSet = SetEndOfFile((HANDLE)file)) == 0 ||
        (originalPosition = SetFilePointer((HANDLE)file, originalPosition, (PLONG)0, 0)) == 0xffffffff) {
        originalPosition = GetLastError();
        return originalPosition;
    }
    return 0;
}

DWORD __stdcall OS_Delete(OSSpec *fileName)
{
    DWORD error;
    BOOL succeeded;
    char *path;

    path = OS_SpecToString(fileName, DAT_0057e308, 0x104);
    if (path == NULL) {
        return 0x6f;
    }
    succeeded = DeleteFileA(DAT_0057e308);
    if (succeeded == 0) {
        error = GetLastError();
        return error;
    }
    return 0;
}

DWORD __stdcall OS_Mkdir(OSSpec *path)
{
    char *convertedPath;
    char *destination = DAT_0057e308;
    BOOL created;
    DWORD error;

    convertedPath = OS_SpecToString(path, destination, 0x104);
    if (convertedPath == (char *)0) {
        return 0x6f;
    }
    created = CreateDirectoryA(DAT_0057e308, (LPSECURITY_ATTRIBUTES)0);
    if (created == 0) {
        error = GetLastError();
        return error;
    }
    return 0;
}

DWORD __stdcall fn_00411780(OSSpec *path)
{
    char *convertedPath;
    BOOL created;
    DWORD error;

    convertedPath = fn_00412340(path->directory.path, DAT_0057e308, 0x104);
    if (convertedPath == NULL) {
        return 0x6f;
    }
    created = RemoveDirectoryA(DAT_0057e308);
    if (created == 0) {
        error = GetLastError();
        return error;
    }
    return 0;
}

void ensure_trailing_backslash(char *path)
{
    path += strlen(path);
    if (path[-1] != '\\') {
        *path = '\\';
        path[1] = '\0';
    }
}

DWORD __stdcall OS_GetCWD(char *text)
{
    DWORD result;

    result = GetCurrentDirectoryA(0x104, text);
    if (result == 0) {
        result = GetLastError();
        return result;
    }
    ensure_trailing_backslash(text);
    return 0;
}

DWORD redirect_standard_handle_to_file(HANDLE *previousHandle, DWORD standardHandle, LPCSTR filename)
{
    HANDLE oldHandle;
    HANDLE fileHandle;
    _SECURITY_ATTRIBUTES securityAttributes;

    securityAttributes.nLength = sizeof(_SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = 1;
    securityAttributes.lpSecurityDescriptor = (LPVOID)0;
    oldHandle = GetStdHandle(standardHandle);
    fileHandle = CreateFileA(filename, 0x40000000, 1, &securityAttributes, 2, 0x80, (HANDLE)0);
    if (fileHandle == (HANDLE)0xffffffff) {
        return GetLastError();
    }
    if (SetStdHandle(standardHandle, fileHandle) == 0) {
        return GetLastError();
    }
    *previousHandle = oldHandle;
    return 0;
}

int __stdcall OS_Execute(OSSpec *name, char **args, char **environment, char *inputFile, char *outputFile,
                         UInt32 *exitcode)
{
    UInt32 commandSize;
    HANDLE savedInput;
    HANDLE savedOutput;
    _STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char *command;
    char **argument;
    UInt32 position;
    BOOL created;

    commandSize = 0;
    for (argument = args; *argument != NULL; argument++)
        commandSize += strlen(*argument) + 3;
    command = malloc(commandSize);
    if (command == NULL)
        return 8;
    for (argument = args, position = 0; *argument != NULL; argument++) {
        if (strchr(*argument, ' ') != NULL) {
            command[position] = '"';
            position++;
            strcpy(command + position, *argument);
            position += strlen(*argument);
            command[position] = '"';
            position++;
        } else if (strchr(*argument, '"') != NULL) {
            char *character;
            for (character = *argument; *character != '\0'; character++, position++) {
                if (*character == '"') {
                    command[position] = '\\';
                    command[position + 1] = '"';
                    position++;
                } else {
                    command[position] = *character;
                }
            }
        } else {
            strcpy(command + position, *argument);
            position += strlen(*argument);
        }
        command[position] = ' ';
        position++;
    }
    command[position] = '\0';
    if (inputFile != NULL) {
        DWORD error = redirect_standard_handle_to_file(&savedInput, -11, inputFile);
        if (error != 0)
            return error;
    }
    if (outputFile != NULL) {
        DWORD error = redirect_standard_handle_to_file(&savedOutput, -12, outputFile);
        if (error != 0) {
            if (inputFile != NULL)
                SetStdHandle(-11, savedInput);
            return error;
        }
    }
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.lpTitle = "Linking";
    if (OS_SpecToString(name, DAT_0057e308, 0x104) == NULL)
        return 0x6f;
    created = CreateProcessA(DAT_0057e308, command, NULL, NULL, 1, 0, NULL, NULL, &startup, &process);
    if (inputFile != NULL)
        SetStdHandle(-11, savedInput);
    if (outputFile != NULL)
        SetStdHandle(-12, savedOutput);
    if (created == 0)
        return GetLastError();
    free(command);
    WaitForSingleObject(process.hProcess, 0xffffffff);
    if (GetExitCodeProcess(process.hProcess, exitcode) != 0) {
        if (*exitcode == 0x103) {
            fprintf(stderr, "?\?? OS_Exec: process still active ?\??\n");
            return 0xfafafafa;
        }
        return 0;
    }
    return GetLastError();
}

char *__stdcall OS_GetDirPtr(char *path)
{
    if (*path == '\\' && path[1] == '\\') {
        char *separator = strchr(path + 2, '\\');
        if (separator == NULL)
            separator = path + strlen(path);
        return separator;
    }
    if ((__ctype_map[(unsigned char)*path] & 0xc0) != 0 && path[1] == ':')
        return path + 2;
    if (*path == '\\')
        return path;
    return path;
}

unsigned int OS_CanonPath(const char *src, char *dst)
{
    const char *read;
    char *write;
    unsigned char ch;
    unsigned char drive;
    unsigned int length;

    length = strlen(src);
    if (length > 259)
        return 111;
    if (dst == NULL)
        dst = (char *)src; /* (canonicalised in place) */

    read = src;
    write = dst;

    if (src[0] == '/' && src[1] == '/' && (__ctype_map[(unsigned char)src[2]] & 0xc0) != 0 &&
        (src[3] == '/' || src[3] == 0)) {
        dst[0] = src[2];
        dst[1] = ':';
        write += 2;
        read += 3;
    } else if ((__ctype_map[(unsigned char)src[0]] & 0xc0) != 0 && src[1] == ':') {
        drive = src[0] & 0xdf;
        dst[0] = drive;
        dst[1] = ':';
        write += 2;
        read += 2;
    }

    while ((ch = *read) != 0) {
        if (ch == '/')
            *write = '\\';
        else
            *write = ch;
        write++;
        read++;
    }
    *write = 0;
    return 0;
}

unsigned int __stdcall MsDos_IsAbsolutePath(char *path)
{
    unsigned int isAbsolute;
    int hasDriveRoot;
    unsigned int hasPrefix;

    isAbsolute = 1;
    hasPrefix = 0;
    if (path[0] == '\\' && path[1] == '\\') {
        hasPrefix = 1;
    }
    if (!hasPrefix) {
        hasDriveRoot = 0;
        hasPrefix = 0;
        if ((__ctype_map[(unsigned char)path[0]] & 0xc0) != 0 && path[1] == ':') {
            hasPrefix = 1;
        }
        if (hasPrefix && path[2] == '\\') {
            hasDriveRoot = 1;
        }
        if (!hasDriveRoot) {
            isAbsolute = 0;
        }
    }
    return isAbsolute;
}

int __stdcall OS_EqualPath(const char *left, const char *right)
{
    int index;
    int equal;

    for (index = 0; left[index] != '\0' && right[index] != '\0' &&
                    ((left[index] >= 'A' && left[index] <= 'Z') ? left[index] + 0x20 : left[index]) ==
                        ((right[index] >= 'A' && right[index] <= 'Z') ? right[index] + 0x20 : right[index]);
         index = index + 1) {
    }
    equal = 0;
    if (left[index] == '\0' && right[index] == '\0') {
        equal = 1;
    }
    return equal;
}

int __stdcall make_osspec_from_path(const char *path, OSSpec *output, Boolean *is_file)
{
    unsigned int error;
    char *last_char;
    const char *basename;
    int attributes;
    int length;
    long name_length;
    char full_path[260];
    char resolved_path[260];
    char *filename;

    error = OS_CanonPath(path, full_path);
    if (error != 0) {
        return error;
    }
    if (GetFullPathNameA(full_path, 260, resolved_path, &filename) == 0) {
        return GetLastError();
    }
    last_char = resolved_path + strlen(resolved_path) - 1;
    basename = strrchr(path, 92);
    if (basename != NULL) {
        basename = basename + 1;
    } else {
        basename = path;
    }
    if (memcmp(basename, "..", 3) != 0) {
        if (memcmp(basename, ".", 2) != 0 && basename[strlen(basename) - 1] == 46 && *last_char != 46) {
            *++last_char = 46;
            *++last_char = 0;
        }
    }
    attributes = GetFileAttributesA(resolved_path);
    if (attributes != -1) {
        if ((attributes & 16) != 0) {
            if (is_file != NULL) {
                *is_file = 0;
            }
            filename = NULL;
        } else {
            if (is_file != NULL) {
                *is_file = 1;
            }
        }
    } else {
        error = GetLastError();
        if (error != 0 && error != 2) {
            return error;
        }
        if (is_file != NULL) {
            *is_file = 1;
        }
    }
    if (filename == NULL) {
        char *end = (filename = last_char + 1);
        if (end[-1] != 92) {
            *end = 92;
            filename += 1;
            *filename = 0;
        }
    }
    length = filename - resolved_path;
    if (length >= 260) {
        output->directory.path[0] = 0;
        return 111;
    }
    memcpy(output->directory.path, resolved_path, length);
    output->directory.path[length] = 0;
    name_length = strlen(filename);
    if (name_length >= 64) {
        output->name[0] = 0;
        return 111;
    }
    memcpy(output->name, filename, name_length + 1);
    return 0;
}

SInt32 __stdcall OS_MakeFileSpec(const char *input, OSSpec *output)
{
    DWORD result;
    Boolean status;
    const char *path = input;

    result = make_osspec_from_path(path, output, &status);
    if (result > 0) {
        return result;
    }
    if (status == '\0') {
        return 5;
    }
    return 0;
}

__stdcall UInt32 OS_MakePathSpec(char *directory, char *path, char *spec)
{
    OSSpec name;
    Boolean flag;
    char buffer[0x144];
    UInt32 directoryLength, pathLength;
    int status;
    pathLength = (path != NULL) ? strlen(path) : 0;
    directoryLength = (directory != NULL) ? strlen(directory) : 0;
    if (directoryLength + pathLength + 2 > 0x144)
        return 0x6f;
    if (directory != NULL) {
        if (directory[0] == '\0') {
            if (path == NULL)
                path = "\\";
            strcpy(buffer, path);
        } else if (directory[1] == '\0') {
            sprintf(buffer, "%s:%s", directory, (path != NULL) ? path : "\\");
        } else if (path != NULL) {
            sprintf(buffer, "%s%s%s", directory, (path[0] == '\\') ? "" : "\\", path);
        } else {
            sprintf(buffer, "%s", directory);
        }
    } else {
        if (path != NULL)
            strcpy(buffer, path);
        else
            strcpy(buffer, ".");
    }
    status = make_osspec_from_path(buffer, &name, &flag);
    strcpy(spec, (char *)&name);
    if (status == 0 && OS_Status(&name) != 0) {
        status = 3;
        flag = 0;
    }
    if (status != 0)
        return status;
    if (flag != 0)
        return 0x10b;
    return 0;
}

int __stdcall OS_MakeNameSpec(const char *name, char *destination)
{
    int length;

    length = strlen(name);
    if (length > 63) {
        return 0x6f;
    }
    if (strchr(name, '\\') != NULL) {
        return 5;
    }
    if (strpbrk(name, "<>:\"/\\|") != NULL) {
        return 0x7b;
    }
    memcpy(destination, name, length + 1);
    return 0;
}

char *__stdcall OS_SpecToString(OSSpec *spec, char *destination, int capacity)
{
    int nameLength;
    int pathLength;

    if (capacity == 0) {
        capacity = sizeof(spec->directory.path);
    }
    if (destination == NULL) {
        destination = (char *)malloc(capacity);
        if (destination == NULL) {
            return NULL;
        }
    }
    pathLength = strlen(spec->directory.path);
    nameLength = strlen(spec->name);
    if (pathLength + nameLength >= capacity) {
        if (pathLength >= capacity) {
            nameLength = 0;
            pathLength = capacity - 1;
        } else {
            nameLength = (capacity - pathLength) - 1;
        }
    }
    memcpy(destination, spec->directory.path, pathLength);
    memcpy(destination + pathLength, spec->name, nameLength);
    destination[pathLength + nameLength] = 0;
    return destination;
}

char *__stdcall fn_00412340(char *source, char *destination, unsigned int capacity)
{
    int length;

    if (capacity == 0) {
        capacity = 0x104;
    }
    if (destination == NULL) {
        destination = (char *)malloc(capacity);
        if (destination == NULL) {
            return NULL;
        }
    }
    length = strlen(source);
    if (length >= (int)capacity) {
        length = capacity - 1;
    }
    memcpy(destination, source, length);
    destination[length] = 0;
    return destination;
}

char *__stdcall MsDos_CopyStringToBuffer(char *source, char *buffer, unsigned int capacity)
{
    int length;

    if (capacity == 0) {
        capacity = 0x40;
    }
    if (buffer == NULL) {
        buffer = malloc(capacity);
        if (buffer == NULL) {
            return NULL;
        }
    }
    length = strlen(source);
    if (length >= (int)capacity) {
        length = capacity - 1;
    }
    memcpy(buffer, source, length);
    buffer[length] = 0;
    return buffer;
}

int __stdcall OS_EqualSpec(const struct OSSpec *first, const struct OSSpec *second)
{
    unsigned int comparisonResult;
    int matches;

    matches = 0;
    comparisonResult = OS_EqualPathSpec(first->directory.path, second->directory.path);
    if (comparisonResult != 0) {
        comparisonResult = equal_path(first->name, second->name);
        if (comparisonResult != 0) {
            matches = 1;
        }
    }
    return matches;
}

unsigned int __stdcall OS_EqualPathSpec(const char *left, const char *right)
{
    return OS_EqualPath(left, right);
}

unsigned int __stdcall equal_path(const char *left, const char *right)
{
    return OS_EqualPath(left, right);
}

int __stdcall OS_IsDir(char *path)
{
    DWORD attributes;
    int length;

    if (OS_SpecToString((OSSpec *)path, DAT_0057e308, 0x104) == NULL)
        return 0x6f;
    length = strlen(DAT_0057e308);
    if (DAT_0057e308[length - 1] == '\\')
        DAT_0057e308[length - 1] = 0;
    attributes = GetFileAttributesA(DAT_0057e308);
    if (attributes == 0xffffffff)
        return 0;
    return (attributes & 0x10) != 0;
}

int __stdcall OS_IsFile(OSSpec *spec)
{
    int length;
    DWORD attributes;
    char *resolvedPath;

    resolvedPath = OS_SpecToString(spec, DAT_0057e308, 0x104);
    if (resolvedPath == NULL) {
        return 0x6f;
    }
    length = strlen(DAT_0057e308);
    if (DAT_0057e308[length - 1] == '\\') {
        DAT_0057e308[length - 1] = 0;
    }
    attributes = GetFileAttributesA(DAT_0057e308);
    if (attributes == 0xffffffff) {
        return 0;
    }
    return (attributes & 0x10) == 0;
}

int __stdcall MsDos_ReturnZero(char *argument)
{
    return 0;
}

int __stdcall fn_004125b0(const OSSpec *source, OSSpec *destination)
{
    const OSSpec *sourceSpec = source;
    *destination = *sourceSpec;
    return 0;
}

DWORD __stdcall OS_OpenDir(char *path, struct DirectorySearch *directory)
{
    LPWIN32_FIND_DATAA findData;
    HANDLE handle;
    DWORD error;
    char pattern[260];

    findData = malloc(0x140);
    directory->findData = findData;
    if (directory->findData == NULL) {
        return 8;
    }
    directory->path = *(OSPathBuffer *)path;
    strcpy(pattern, path);
    strcat(pattern, DAT_0054b80c.pattern);
    handle = FindFirstFileA(pattern, directory->findData);
    directory->handle = handle;
    if (directory->handle == (HANDLE)-1) {
        error = GetLastError();
        return error;
    }
    return 0;
}

UInt32 __stdcall OS_ReadDir(DirectorySearch *state, OSSpec *spec, char *filename, Boolean *isdir)
{
    char name[0x104];
    char path[0x104];
    LPWIN32_FIND_DATAA fd = state->findData;
    SInt32 len;

    for (;;) {
        if (state->handle == (HANDLE)-1)
            return 2;
        if (strlen(fd->cFileName) < 0x40)
            strncpy(name, fd->cFileName, 0x40);
        else
            strncpy(name, fd->cAlternateFileName, 0x40);
        name[0x103] = 0;
        if (!FindNextFileA(state->handle, fd))
            OS_CloseDir(state);
        if (memcmp(name, ".", 2) == 0)
            continue;
        if (memcmp(name, "..", 3) == 0)
            continue;
        if (strlen(state->path.path) + strlen(name) >= 0x104)
            continue;
        len = strlen(state->path.path);
        strncpy(path, state->path.path, 0x103);
        if (len < 0x104) {
            strncpy(path + len, name, 0x103 - len);
            path[0x103] = 0;
        } else {
            return 0x6f;
        }
        strncpy(filename, name, 0x3f);
        filename[0x3f] = 0;
        return make_osspec_from_path(path, spec, isdir);
    }
}

DWORD __stdcall OS_CloseDir(DirectorySearch *resource)
{
    BOOL closed;
    DWORD error;

    if (resource->handle != (HANDLE)0xffffffff) {
        closed = FindClose(resource->handle);
        if (closed == 0) {
            error = GetLastError();
            return error;
        }
        if (resource->findData != NULL) {
            free(resource->findData);
        }
        resource->findData = NULL;
        resource->handle = (HANDLE)0xffffffff;
    }
    return 0;
}

int OS_GetMilliseconds(void)

{
    return GetTickCount();
}

void __stdcall OS_GetTime(_FILETIME *fileTime)
{
    _SYSTEMTIME systemTime;
    DWORD convertedTime[2];

    GetSystemTime(&systemTime);
    SystemTimeToFileTime(&systemTime, (_FILETIME *)convertedTime);
    *fileTime = *(_FILETIME *)convertedTime;
}

DWORD __stdcall OS_NewHandle(SIZE_T size, MemBuffer *block)
{
    block->ptr = GlobalAlloc(0, size);
    block->size = size;
    if (block->ptr == NULL)
        return GetLastError();
    memset(block->ptr, 0, size);
    return 0;
}

UInt32 __stdcall OS_ResizeHandle(MemBuffer *buf, UInt32 newsize)
{
    HGLOBAL newHandle;

    newHandle = GlobalReAlloc(buf->ptr, newsize, 2);
    if (newHandle == NULL) {
        buf->ptr = NULL;
        buf->size = 0;
        return GetLastError();
    }
    if (newsize > buf->size) {
        memset((char *)newHandle + buf->size, 0, newsize - buf->size);
    }
    buf->ptr = newHandle;
    buf->size = newsize;
    return 0;
}

HGLOBAL __stdcall MsDos_GetValidMemBufferPtr(MemBuffer *handle)
{
    UINT flags;

    flags = GlobalFlags(handle->ptr);
    if (flags != 0x8000) {
        return handle->ptr;
    }
    return NULL;
}

void __stdcall fn_004129c0(MemBuffer *buffer)
{
    return;
}

DWORD __stdcall OS_FreeHandle(MemBuffer *handle)
{
    HGLOBAL remainingHandle;
    DWORD error;

    remainingHandle = GlobalFree(handle->ptr);
    if (remainingHandle != NULL) {
        error = GetLastError();
        return error;
    }
    handle->ptr = NULL;
    handle->size = 0;
    return 0;
}

unsigned int __stdcall OS_GetHandleSize(MemBuffer *entry, DWORD *value)
{
    UINT flags;

    flags = GlobalFlags(entry->ptr);
    if (flags != 0x8000) {
        *value = entry->size;
        return 0;
    }
    *value = 0;
    return 8;
}

void __stdcall OS_InvalidateHandle(MemBuffer *buffer)
{
    buffer->ptr = NULL;
    buffer->size = 0;
}

unsigned char __stdcall OS_ValidHandle(MemBuffer *value)
{
    return value != NULL && value->ptr != NULL;
}

int __stdcall OS_OSErrorToMacError(int errorCode)
{
    switch (errorCode) {
        case 0:
            return 0;
        case 2:
            return -43;
        case 3:
        case 0x10b:
            return -120;
        case 4:
            return -42;
        case 5:
        case 0x20:
        case 0x21:
            return -54;
        case 6:
            return -51;
        case 8:
        case 0xe:
        case 0x24:
            return -108;
        case 0xd:
        case 0x10:
        case 0x18:
        case 0x57:
            return -50;
        case 0xf:
            return -35;
        case 0x13:
            return -61;
        case 0x15:
        case 0x17:
        case 0x19:
        case 0x1b:
        case 0x1c:
        case 0x1d:
        case 0x1e:
        case 0x1f:
            return -36;
        case 0x26:
            return -39;
        case 0x27:
            return -34;
        case 0xb7:
            return -48;
    }
    return errorCode | 0xffff8000;
}

void __stdcall OS_TimeToMac(FILETIME time, int *result)
{
    SYSTEMTIME st;
    FILETIME ft;
    SInt32 days;
    SInt32 year;

    ft.dwLowDateTime = time.dwLowDateTime;
    ft.dwHighDateTime = time.dwHighDateTime;
    FileTimeToSystemTime(&ft, &st);

    year = st.wYear - 1904;
    days = year * 365;
    days += (year + 3) / 4 - (year + 4) / 100 + (year - 296) / 400;

    if (st.wYear % 4 == 0 && (st.wYear % 100 != 0 || st.wYear % 400 == 0))
        february_days = 29;
    else
        february_days = 28;

    if (st.wMonth > 12 || st.wMonth == 0)
        st.wMonth = 1;

    while (--st.wMonth != 0)
        days += DAT_0054b80c.monthDays[st.wMonth];

    days += st.wDay - 1;
    *result = days * 86400 + st.wHour * 3600 + st.wMinute * 60 + st.wSecond;
}

void __stdcall OS_MacToTime(unsigned long secs, FILETIME *ft)
{
    SYSTEMTIME st;
    unsigned long remaining = secs;
    unsigned long days, year;
    long dayOfYear, month;
    long gregorianYears;

    memset(&st, 0, sizeof(st));
    st.wSecond = secs % 60;
    remaining /= 60;
    st.wMinute = remaining % 60;
    remaining /= 60;
    st.wHour = remaining % 24;
    remaining /= 24;
    days = remaining;
    dayOfYear = days % 365;
    year = days / 365;
    st.wYear = year + 1904;
    gregorianYears = year - 296;
    dayOfYear -= (year + 3) / 4 - (year + 4) / 100 + gregorianYears / 400;
    if (st.wYear % 4)
        february_days = 28;
    else
        february_days = 29;
    for (month = 0; dayOfYear >= (&february_days)[month - 1]; month++)
        dayOfYear -= (&february_days)[month - 1];
    st.wMonth = month + 1;
    st.wDay = dayOfYear + 1;
    SystemTimeToFileTime(&st, ft);
}

unsigned int __stdcall OS_RefToMac(unsigned int value)
{
    if (value == 4294967295U || (long)value < 0)
        return 0U;
    {
        int ref = value;
        if (ref >= 65535)
            CLIO_ReportAssertionFailure("(long)ref < 0xffff", "MsDos.c", 1560U);
    }
    return value + 1U;
}

#include <stdlib.h>
#include <stdio.h>

int short_predecessor(short value)
{
    if (value != 0) {
        return value - 1;
    }
    return -1;
}

DWORD __stdcall MacSpecs_LoadMacResource(char *path, LPVOID *resourceData, DWORD *resourceSize)
{
    OSSpec *spec = (OSSpec *)path;
    char *convertedPath;
    HMODULE module;
    HRSRC resource;
    HGLOBAL loadedResource;
    LPVOID data;

    convertedPath = OS_SpecToString(spec, DAT_0057e308, 0x104);
    if (convertedPath == NULL) {
        return 0x6f;
    }
    module = GetModuleHandleA(DAT_0057e308);
    if (module == NULL) {
        return GetLastError();
    }
    resource = FindResourceA(module, "#101", "MACRSRC");
    if (resource == NULL) {
        return GetLastError();
    }
    loadedResource = LoadResource(module, resource);
    if (loadedResource == NULL) {
        return GetLastError();
    }
    data = LockResource(loadedResource);
    if (data == NULL) {
        return GetLastError();
    }
    *resourceData = data;
    *resourceSize = SizeofResource(module, resource);
    return 0;
}

Boolean __stdcall MacSpecs_IsByteInDBCSCharacter(BYTE *a, BYTE *b)
{
    BYTE *p = a;
    while (p <= b) {
        Boolean c = IsDBCSLeadByte(*p);
        if (c == 0 && GetLastError() == 0x57)
            return 0;
        if (c != 0) {
            if (p == b || p + 1 == b)
                return 1;
            p += 2;
        } else {
            p += 1;
        }
    }
    return 0;
}

int store_mac_spec_entry(MacSpecEntry *entry)
{
    unsigned int index = entry->index;
    unsigned int directory = index;
    unsigned int slot = index & 0xff;

    directory >>= 8;
    if (directory >= directory_count) {
        do {
            if (directory >= 0x100) {
                fprintf(stderr, "Fatal error:  too many directories referenced, out of memory\n");
                return 0;
            }
            mac_spec_entries[directory] = calloc(sizeof(*mac_spec_entries[directory]), 0x100);
            if (mac_spec_entries[directory] == NULL) {
                return 0;
            }
            ++directory_count;
        } while (directory >= directory_count);
    }
    mac_spec_entries[directory][slot] = entry;
    return 1;
}

struct NameRegistryEntry *find_or_create_name_registry_entry(struct NameRegistryEntry **entries, char *name)
{
    NameRegistryEntry *entry;

    for (; *entries != NULL; entries = &(*entries)->next) {
        if (OS_EqualPath(name, (*entries)->root.name) != 0) {
            return *entries;
        }
    }
    if (next_entry_id >= 0x100) {
        return NULL;
    }
    entry = (NameRegistryEntry *)malloc(sizeof(NameRegistryEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->next = NULL;
    entry->id = next_entry_id;
    next_entry_id = next_entry_id + 1;
    entry->root.name = (char *)malloc(strlen(name) + 1);
    if (entry->root.name == NULL) {
        return NULL;
    }
    strcpy(entry->root.name, name);
    entry->root.index = 2;
    entry->root.parent.registry = entry;
    entry->root.children = NULL;
    entry->root.next = NULL;
    *entries = entry;
    return entry;
}

struct MacSpecEntry *lookup_dir_id(unsigned int dirID)
{
    unsigned int index = dirID >> 8;
    unsigned int entryIndex = dirID & 0xff;
    if (dirID == 2U)
        CLIO_ReportAssertionFailure("dirID != 2", "MacSpecs.c", 166U);
    if (index >= directory_count)
        return NULL;
    return mac_spec_entries[index][entryIndex];
}

/* A named entry and the table that owns its entry chain. */
MacSpecEntry *find_or_create_child_entry(MacSpecEntry *table, char *name)
{
    MacSpecEntry **link;
    int result;
    MacSpecEntry *entry;

    for (link = &table->children; *link != NULL; link = &(*link)->next) {
        result = OS_EqualPath(name, (*link)->name);
        if (result != 0) {
            return *link;
        }
    }
    if (next_entry_index >= 0x10000) {
        return NULL;
    }
    result = strlen(name) + 1;
    entry = (MacSpecEntry *)malloc(sizeof(MacSpecEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->name = (char *)malloc(result);
    if (entry->name == NULL) {
        return NULL;
    }
    strcpy(entry->name, name);
    entry->children = NULL;
    entry->next = NULL;
    entry->index = next_entry_index;
    next_entry_index = next_entry_index + 1;
    result = store_mac_spec_entry(entry);
    if (result == 0) {
        return NULL;
    }
    entry->parent.entry = table;
    *link = entry;
    return entry;
}

int find_or_create_spec_entry(char *spec, unsigned int *typePtr, unsigned int *offsetPtr)
{
    char str[0x104];
    char name[0x40];
    char buf[0x104];
    char *end;
    char *p;
    NameRegistryEntry *node;
    MacSpecEntry *rec;
    char *tok;

    if (!fn_00412340(spec, buf, 0x104))
        return 0;
    end = OS_GetDirPtr(buf);
    strncpy(name, buf, end - buf);
    name[end - buf] = '\0';
    strcpy(str, end);
    node = find_or_create_name_registry_entry(&spec_name_registry, name);
    if (node == NULL)
        return 0;
    *typePtr = node->id;
    rec = &node->root;
    p = str + 1;
    if (str[0] != '\\')
        CLIO_ReportAssertionFailure("*pb == OS_PATHSEP", "MacSpecs.c", 0x108);
    while (*p != '\0') {
        tok = p;
        while (*p != '\0' && *p != '\\')
            p++;
        *p = '\0';
        rec = find_or_create_child_entry(rec, tok);
        p++;
        if (rec == NULL)
            return 0;
    }
    *offsetPtr = rec->index;
    return 1;
}

void lookup_spec_and_advance_parent(int *id, int *kind, unsigned int **result)
{
    NameRegistryEntry *node;

    if (*id == 1) {
        *result = NULL;
    } else if (*kind == 2U) {
        node = spec_name_registry;
        while (node != NULL && node->id != *id) {
            node = node->next;
        }
        if (node != NULL) {
            *result = (unsigned int *)&node->root;
            *id = 1;
            *kind = 1;
        } else {
            *result = NULL;
        }
    } else {
        *result = (unsigned int *)lookup_dir_id(*kind);
        if (*result != NULL) {
            MacSpecEntry *entry = (MacSpecEntry *)*result;
            MacSpecEntry *parent = entry->parent.entry;
            *kind = parent->index;
        }
    }
}

int find_or_create_spec_entry_negated(char *input, unsigned int *firstResult, unsigned int *secondResult)
{
    if (find_or_create_spec_entry(input, firstResult, secondResult) != 0) {
        *firstResult = -*firstResult;
        return 1;
    }
    *firstResult = 0;
    *secondResult = 0;
    return 0;
}

int build_name_and_backslash_path(int a, int b, void *buffer1, void *buffer2)
{
    int n;
    unsigned int *arr[256];
    unsigned int *val;
    char *p;
    char *out1 = buffer1;
    char *out2 = buffer2;

    a = -a;
    n = 0;
    do {
        lookup_spec_and_advance_parent(&a, &b, &val);
        if (val != NULL) {
            arr[n] = val;
            n++;
        }
    } while (val != NULL);

    if (n != 0) {
        strcpy(out1, (char *)*arr[--n]);
    } else {
        *out1 = 0;
        *out2 = 0;
        return 0;
    }

    *out2 = '\\';
    p = out2 + 1;
    while (n--) {
        strcpy(p, (char *)*arr[n]);
        p += strlen(p);
        *p++ = '\\';
    }
    *p = 0;
    return 1;
}

int __stdcall parse_value_and_offset(char *text, unsigned short *value, unsigned int *offset)
{
    unsigned int parsedValue;
    unsigned int parsedOffset;

    if (find_or_create_spec_entry_negated(text, &parsedValue, &parsedOffset)) {
        *value = parsedValue;
        *offset = parsedOffset;
        return 0;
    }
    *value = 0;
    *offset = 0;
    return 3;
}

int __stdcall MacSpecs_MakeCWFileSpecFromString(char *input, CWFileSpec *output)
{
    UInt16 volumeRef;
    unsigned int directoryId;
    int status;

    status = parse_value_and_offset(input, &volumeRef, &directoryId);
    output->fileData.file.volumeRef = volumeRef;
    output->fileData.file.directoryId = directoryId;
    if (status != 0) {
        return status;
    }
    if (MsDos_CopyStringToBuffer(input + 0x104, file_name_buffer, 0x40) == NULL) {
        return 0x6f;
    }
    c2pstrcpy(output->fileData.file.name, file_name_buffer);
    return 0;
}

DWORD __stdcall fn_00413670(short kind, int value, char *path)
{
    unsigned int directoryLength;
    unsigned int nameLength;
    DWORD result;

    if (kind == 0 || value == 0) {
        result = OS_GetCWD(path);
        if (result != 0) {
            return result;
        }
    } else {
        if (build_name_and_backslash_path(kind, value, DAT_0057e818, DAT_0057e858) == 0) {
            return 3;
        }
        directoryLength = strlen(DAT_0057e818);
        nameLength = strlen(DAT_0057e858);
        if ((int)(directoryLength + nameLength) < 0x104) {
            memcpy(path, DAT_0057e818, directoryLength);
            memcpy(path + directoryLength, DAT_0057e858, 1 + nameLength);
        }
    }
    return 0;
}

/* Unused lookup request declaration removed: no accesses or allocations. */
int __stdcall MacSpecs_MakeOSSpec(CWFileSpec *record, char *buffer)
{
    int result = fn_00413670(record->fileData.file.volumeRef, record->fileData.file.directoryId, buffer);
    if (result != 0) {
        return result;
    }
    p2cstrcpy(file_name_buffer, record->fileData.file.name);
    return OS_MakeNameSpec(file_name_buffer, buffer + 0x104);
}

int __stdcall MacSpecs_MakeResourceForkSpec(char *source, OSSpec *destination, char retryOnError)
{
    char pathBuffer[0x104];
    DWORD error;

    fn_00412340(source, pathBuffer, 0x104);

    error = CLProj_MakeOSSpecFromDirectoryAndFilename(pathBuffer, "RESOURCE.FRK", destination);
    if (error != 0)
        return error;

    error = OS_Status(destination);
    if (error != 0) {
        if (retryOnError != 0) {
            error = OS_Mkdir(destination);
            if (error != 0)
                return error;
            SetFileAttributesA(OS_SpecToString(destination, data_005880e0, 0x104), 2);
        } else {
            return error;
        }
        error = CLProj_MakeOSSpecFromDirectoryAndFilename(pathBuffer, "RESOURCE.FRK", destination);
        if (error != 0)
            return error;
    } else {
        if (OS_IsFile(destination) != 0)
            return 0x10b;
    }

    error = OS_MakeNameSpec(MsDos_CopyStringToBuffer(source + 0x104, data_005880e0, 0x104), destination->name);
    if (error != 0)
        return error;
    return 0;
}
