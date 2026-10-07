#ifndef DRIVER_MACFILETYPES_H
#define DRIVER_MACFILETYPES_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct BytePattern {
    int result;
    const char *bytes;
    char length;
    char unknown_09[5];
};
#pragma pack(pop)
struct MacFileTypeNode {
    struct OpcodeDescriptorTable *table;
    struct MacFileTypeNode *next;
};
#pragma pack(push, 1)
struct OpcodeDescriptorTable {
    short count;
    struct BytePattern *patterns;
};
#pragma pack(pop)
extern void __stdcall MacFileTypes_AppendTable(struct MacFileTypeNode **list, SInt32 value);
extern void __stdcall fn_00421af0(OSSpec *value, DWORD input);
extern SInt32 __stdcall MacFileTypes_GetFileType(OSSpec *path, UInt32 *fileType);
extern unsigned int __stdcall fn_00421d30(OSSpec *a0, SInt32 a1, SInt32 a2);
extern unsigned char __stdcall MacFileTypes_MatchBytes(void *bytes, int length, UInt32 *mnemonic);
extern void __stdcall fn_00421a80(int value, unsigned int *result);
extern int(__stdcall *data_00587e70)();

#ifdef __cplusplus
}
#endif

#endif
