#ifndef COMPILER_WIN32_H
#define COMPILER_WIN32_H

#include "compiler/common.h"
#include "compiler/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOL;
typedef unsigned long DWORD;
struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};
typedef struct _FILETIME _FILETIME, *P_FILETIME;
typedef struct _FILETIME *LPFILETIME;
typedef DWORD *LPDWORD;
struct HINSTANCE__ {
    int unused;
};
typedef struct HINSTANCE__ HINSTANCE__, *PHINSTANCE__;
typedef unsigned char BYTE;
typedef void *HANDLE;
typedef HANDLE HGLOBAL;
typedef unsigned short WORD;
typedef struct HINSTANCE__ *HINSTANCE;
typedef void *LPCVOID;
typedef void *LPVOID;
struct HRSRC__ {
    int unused;
};
typedef struct HRSRC__ HRSRC__, *PHRSRC__;
typedef struct HRSRC__ *HRSRC;
typedef HINSTANCE HMODULE;
typedef BYTE *LPBYTE;
typedef unsigned int UINT;
typedef short SHORT;
struct _COORD {
    SHORT X;
    SHORT Y;
};
typedef struct _COORD _COORD, *P_COORD;
struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
};
typedef struct _SMALL_RECT _SMALL_RECT, *P_SMALL_RECT;
typedef struct _COORD COORD;
typedef struct _SMALL_RECT SMALL_RECT;
struct _CONSOLE_SCREEN_BUFFER_INFO {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
};
typedef struct _CONSOLE_SCREEN_BUFFER_INFO _CONSOLE_SCREEN_BUFFER_INFO, *P_CONSOLE_SCREEN_BUFFER_INFO;
typedef struct _CONSOLE_SCREEN_BUFFER_INFO *PCONSOLE_SCREEN_BUFFER_INFO;
typedef BOOL(__stdcall *PHANDLER_ROUTINE)(DWORD);
typedef unsigned long ULONG_PTR;
typedef void *PVOID;
struct _struct_519 {
    DWORD Offset;
    DWORD OffsetHigh;
};
union _union_518 {
    struct _struct_519 s;
    PVOID Pointer;
};
struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union _union_518 u;
    HANDLE hEvent;
};
typedef struct _OVERLAPPED _OVERLAPPED, *P_OVERLAPPED;
typedef union _union_518 _union_518, *P_union_518;
typedef struct _struct_519 _struct_519, *P_struct_519;
struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
};
typedef struct _SECURITY_ATTRIBUTES _SECURITY_ATTRIBUTES, *P_SECURITY_ATTRIBUTES;
struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
};
typedef struct _SYSTEMTIME _SYSTEMTIME, *P_SYSTEMTIME;
typedef long LONG;
typedef wchar_t WCHAR;
typedef char CHAR;
typedef struct _FILETIME FILETIME;
struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[260];
    CHAR cAlternateFileName[14];
};
typedef struct _WIN32_FIND_DATAA _WIN32_FIND_DATAA, *P_WIN32_FIND_DATAA;
typedef struct _OVERLAPPED *LPOVERLAPPED;
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
typedef CHAR *LPSTR;
struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
};
typedef struct _STARTUPINFOA _STARTUPINFOA, *P_STARTUPINFOA;
typedef struct _WIN32_FIND_DATAA *LPWIN32_FIND_DATAA;
typedef struct _STARTUPINFOA *LPSTARTUPINFOA;
typedef struct _RTL_CRITICAL_SECTION_DEBUG *PRTL_CRITICAL_SECTION_DEBUG;
struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
};
typedef struct _RTL_CRITICAL_SECTION _RTL_CRITICAL_SECTION, *P_RTL_CRITICAL_SECTION;
typedef struct _RTL_CRITICAL_SECTION *PRTL_CRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION LPCRITICAL_SECTION;
struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
};
typedef struct _LIST_ENTRY LIST_ENTRY;
struct _RTL_CRITICAL_SECTION_DEBUG {
    WORD Type;
    WORD CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Flags;
    WORD CreatorBackTraceIndexHigh;
    WORD SpareWORD;
};
typedef struct _RTL_CRITICAL_SECTION_DEBUG _RTL_CRITICAL_SECTION_DEBUG, *P_RTL_CRITICAL_SECTION_DEBUG;
typedef struct _LIST_ENTRY _LIST_ENTRY, *P_LIST_ENTRY;
typedef struct _SYSTEMTIME *LPSYSTEMTIME;
typedef CHAR *LPCSTR;
typedef LONG *PLONG;
typedef CHAR *LPCH;
typedef char *va_list;
typedef ULONG_PTR SIZE_T;
typedef enum SectionFlags {
    IMAGE_SCN_TYPE_NO_PAD = 8,
    IMAGE_SCN_RESERVED_0001 = 16,
    IMAGE_SCN_CNT_CODE = 32,
    IMAGE_SCN_CNT_INITIALIZED_DATA = 64,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA = 128,
    IMAGE_SCN_LNK_OTHER = 256,
    IMAGE_SCN_LNK_INFO = 512,
    IMAGE_SCN_RESERVED_0040 = 1024,
    IMAGE_SCN_LNK_REMOVE = 2048,
    IMAGE_SCN_LNK_COMDAT = 4096,
    IMAGE_SCN_GPREL = 32768,
    IMAGE_SCN_MEM_16BIT = 131072,
    IMAGE_SCN_MEM_PURGEABLE = 131072,
    IMAGE_SCN_MEM_LOCKED = 262144,
    IMAGE_SCN_MEM_PRELOAD = 524288,
    IMAGE_SCN_ALIGN_1BYTES = 1048576,
    IMAGE_SCN_ALIGN_2BYTES = 2097152,
    IMAGE_SCN_ALIGN_4BYTES = 3145728,
    IMAGE_SCN_ALIGN_8BYTES = 4194304,
    IMAGE_SCN_ALIGN_16BYTES = 5242880,
    IMAGE_SCN_ALIGN_32BYTES = 6291456,
    IMAGE_SCN_ALIGN_64BYTES = 7340032,
    IMAGE_SCN_ALIGN_128BYTES = 8388608,
    IMAGE_SCN_ALIGN_256BYTES = 9437184,
    IMAGE_SCN_ALIGN_512BYTES = 10485760,
    IMAGE_SCN_ALIGN_1024BYTES = 11534336,
    IMAGE_SCN_ALIGN_2048BYTES = 12582912,
    IMAGE_SCN_ALIGN_4096BYTES = 13631488,
    IMAGE_SCN_ALIGN_8192BYTES = 14680064,
    IMAGE_SCN_LNK_NRELOC_OVFL = 16777216,
    IMAGE_SCN_MEM_DISCARDABLE = 33554432,
    IMAGE_SCN_MEM_NOT_CACHED = 67108864,
    IMAGE_SCN_MEM_NOT_PAGED = 134217728,
    IMAGE_SCN_MEM_SHARED = 268435456,
    IMAGE_SCN_MEM_EXECUTE = 536870912,
    IMAGE_SCN_MEM_READ = 1073741824,
    IMAGE_SCN_MEM_WRITE = 2147483648
} SectionFlags;
/* Win32 CreateProcess output, passed as &pi by OS_Execute. */
struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
};
struct _TIME_ZONE_INFORMATION;
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) BOOL __stdcall CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
__declspec(dllimport) HANDLE __stdcall CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                                                   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                   DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                                   HANDLE hTemplateFile);
__declspec(dllimport) BOOL __stdcall CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                                                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                                    LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                                                    DWORD dwCreationFlags, LPVOID lpEnvironment,
                                                    LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo,
                                                    struct _PROCESS_INFORMATION *lpProcessInformation);
__declspec(dllimport) void __stdcall DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
__declspec(dllimport) BOOL __stdcall DeleteFileA(LPCSTR lpFileName);
__declspec(dllimport) void __stdcall EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
__declspec(dllimport) void __stdcall ExitProcess(UINT uExitCode);
__declspec(dllimport) BOOL __stdcall FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);
__declspec(dllimport) BOOL __stdcall FindClose(HANDLE hFindFile);
__declspec(dllimport) HANDLE __stdcall FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
__declspec(dllimport) BOOL __stdcall FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
__declspec(dllimport) HRSRC __stdcall FindResourceA(HMODULE hModule, LPCSTR lpName, LPCSTR lpType);
__declspec(dllimport) DWORD __stdcall FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                                                     DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize,
                                                     va_list *Arguments);
__declspec(dllimport) BOOL __stdcall FreeEnvironmentStringsA(LPSTR);
__declspec(dllimport) LPSTR __stdcall GetCommandLineA(void);
__declspec(dllimport) BOOL __stdcall GetConsoleScreenBufferInfo(HANDLE hConsoleOutput,
                                                                PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo);
__declspec(dllimport) DWORD __stdcall GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);
__declspec(dllimport) LPSTR __stdcall GetEnvironmentStrings(void);
__declspec(dllimport) BOOL __stdcall GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);
__declspec(dllimport) DWORD __stdcall GetFileAttributesA(LPCSTR lpFileName);
__declspec(dllimport) DWORD __stdcall GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
__declspec(dllimport) BOOL __stdcall GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime,
                                                 LPFILETIME lpLastWriteTime);
__declspec(dllimport) DWORD __stdcall GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer,
                                                       LPSTR *lpFilePart);
__declspec(dllimport) DWORD __stdcall GetLastError(void);
__declspec(dllimport) void __stdcall GetLocalTime(LPSYSTEMTIME lpSystemTime);
__declspec(dllimport) HMODULE __stdcall GetModuleHandleA(LPCSTR lpModuleName);
__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
__declspec(dllimport) UINT __stdcall GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize);
__declspec(dllimport) void __stdcall GetSystemTime(LPSYSTEMTIME lpSystemTime);
__declspec(dllimport) DWORD __stdcall GetTickCount(void);
__declspec(dllimport) DWORD __stdcall GetTimeZoneInformation(struct _TIME_ZONE_INFORMATION *lpTimeZoneInformation);
__declspec(dllimport) UINT __stdcall GetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize);
__declspec(dllimport) HGLOBAL __stdcall GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
__declspec(dllimport) UINT __stdcall GlobalFlags(HGLOBAL hMem);
__declspec(dllimport) HGLOBAL __stdcall GlobalFree(HGLOBAL hMem);
__declspec(dllimport) HGLOBAL __stdcall GlobalReAlloc(HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags);
__declspec(dllimport) void __stdcall InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
__declspec(dllimport) BOOL __stdcall IsDBCSLeadByte(BYTE TestChar);
__declspec(dllimport) void __stdcall LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
__declspec(dllimport) HGLOBAL __stdcall LoadResource(HMODULE hModule, HRSRC hResInfo);
__declspec(dllimport) LPVOID __stdcall LockResource(HGLOBAL hResData);
__declspec(dllimport) BOOL __stdcall ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                              LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
__declspec(dllimport) BOOL __stdcall RemoveDirectoryA(LPCSTR lpPathName);
__declspec(dllimport) BOOL __stdcall SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);
__declspec(dllimport) BOOL __stdcall SetEndOfFile(HANDLE hFile);
__declspec(dllimport) BOOL __stdcall SetFileAttributesA(LPCSTR lpFileName, DWORD dwFileAttributes);
__declspec(dllimport) DWORD __stdcall SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh,
                                                     DWORD dwMoveMethod);
__declspec(dllimport) BOOL __stdcall SetFileTime(HANDLE hFile, const FILETIME *lpCreationTime,
                                                 const FILETIME *lpLastAccessTime, const FILETIME *lpLastWriteTime);
__declspec(dllimport) BOOL __stdcall SetStdHandle(DWORD nStdHandle, HANDLE hHandle);
__declspec(dllimport) DWORD __stdcall SizeofResource(HMODULE hModule, HRSRC hResInfo);
__declspec(dllimport) BOOL __stdcall SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime);
__declspec(dllimport) DWORD __stdcall TlsAlloc(void);
__declspec(dllimport) BOOL __stdcall TlsFree(DWORD dwTlsIndex);
__declspec(dllimport) LPVOID __stdcall TlsGetValue(DWORD dwTlsIndex);
__declspec(dllimport) BOOL __stdcall TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);
__declspec(dllimport) DWORD __stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                                               LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef struct _SYSTEMTIME SYSTEMTIME;

#ifdef __cplusplus
}
#endif

#endif
