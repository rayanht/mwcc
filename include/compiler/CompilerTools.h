#ifndef COMPILER_COMPILERTOOLS_H
#define COMPILER_COMPILERTOOLS_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct GList {
    char **data;
    SInt32 size;
    SInt32 hndlsize;
    SInt32 growsize;
};
#pragma pack(pop)
struct Pool {
    struct PoolNode *head;
    SInt32 overhead;
    struct PoolNode *cur;
    char *ptr;
    SInt32 free;
};
struct PoolNode {
    struct PoolNode *next; /* 0x00: select_or_allocate_pool_node links pool nodes */
    struct PoolNode *
        *block;   /* 0x04: select_or_allocate_pool_node dereferences the allocation handle; releaseheaps frees it */
    SInt32 size;  /* 0x08: select_or_allocate_pool_node stores allocation size */
    SInt32 avail; /* 0x0c: select_or_allocate_pool_node tracks remaining bytes */
};
typedef struct {
    unsigned char byte0; /* 0x00: AppendGListLong copies the first native-order value byte */
    unsigned char byte1; /* 0x01: AppendGListLong copies the second native-order value byte */
    unsigned char byte2; /* 0x02: AppendGListLong copies the third native-order value byte */
    unsigned char byte3; /* 0x03: AppendGListLong copies the fourth native-order value byte */
} NativeLongBytes;

union UInt32ByteSwapStorage {
    UInt32 word;
    UInt8 bytes[4];
};
extern char data_00587958;
struct GList;
extern void CompilerTools_ResetPool(void);
extern void CompilerTools_ResetPoolAvail(void);
extern void freelheap(void);
extern void *galloc(SInt32 size);
extern void format_string(char *buf, int size, char *fmt, char *ap);
extern unsigned char CompilerTools_ReportDiagnostic(SInt32 code, ...);
extern SInt16 data_005511b4;
extern SInt32 data_00588228;
extern void CompilerTools_ReportLimitedDiagnostic(SInt32 code, ...);
extern SInt16 limited_diagnostic_limit;
extern SInt32 limited_diagnostic_count;
extern void CompilerTools_FormatMessageAndLongjmp(int arg1, int arg2);
extern void CompilerTools_DispatchMessageBufferByMode(int a0);
extern char message_buffer[];
extern void fn_004431b0(void *a0);
extern short CTool_EndianConvertInPlaceWord16Ptr(short *a0);
extern void CTool_EndianConvertWord64(CInt64 ci, char *result);
extern UInt32 CTool_EndianConvertMem(void *a0, short a1);
extern unsigned int CTool_EndianConvertWord32(unsigned int a0);
extern SInt16 getbit(UInt32 value);
extern void memclrw(void *a0, unsigned int a1);
extern void CompilerTools_DecrementPositiveCounter(void);
extern void fn_00441f10(void);
extern int CTool_TotalHeapSize(void);
extern short CHash(const char *str);
extern void CompilerTools_ConvertCStringToPString(unsigned char *text);
extern void copy_pstring(UInt8 *destination, UInt8 *source);
extern void fn_00443160(void *a0);
extern Boolean fn_00443170(struct StorageHandle *a0, UInt32 a1);
extern void fn_00443190(void *a0);
extern void fn_004431a0(void *a0);
extern void CompilerTools_GetResourceCString(char *buffer, SInt16 id, SInt16 arg);
extern unsigned int CTool_EndianConvertInPlaceWord32Ptr(unsigned int *p);
extern UInt16 CTool_EndianConvertWord16(UInt16 a0);
extern void CToLowercase(char *src, char *dst);
extern char *ScanDec(char *src, SInt32 *value, Boolean *flag);
extern void *CompilerTools_AllocatePoolMemory(UInt32 requestedSize);
extern void *CompilerTools_AllocateBlock(SInt32 size);
extern void *CompilerTools_AllocatePool(unsigned int size);
extern void CompilerTools_ClearPoolBlocks(void);
extern void releaseheaps(void);
extern SInt16 initheaps(void (*param)());
extern SInt16 CompilerTools_InitHeaps(void (*param)());
extern int select_or_allocate_pool_node(Pool *pool, SInt32 size);
extern void InitNameHash(void);
extern HashNameNode *GetHashNameNode(const char *text);
extern HashNameNode *GetHashNameNodeExport(const char *text);
extern void CompilerTools_AppendGListString(GList *buf, const char *str);
extern void AppendGListName(GList *buf, const char *str);
extern void AppendGListTargetEndianLong(GList *buf, UInt32 l);
extern void AppendGListLong(GList *buffer, SInt32 value);
extern void AppendGListTargetEndianWord(GList *buf, UInt16 w);
extern void AppendGListWord(GList *buffer, SInt16 value);
extern void AppendGListByte(GList *buffer, SInt8 value);
extern void AppendGListNoData(GList *buffer, SInt32 additionalLength);
extern void *CompilerTools_AppendGListData(GList *buffer, const void *source, SInt32 count);
extern void ShrinkGList(GList *list);
extern void fn_00442c00(GList *entry);
extern void FreeGList(GList *storage);
extern SInt16 InitGList(GList *allocation, SInt32 size);
extern void CompilerGetCString(short value, char *destination);
extern void *fn_00443110(SInt32 a0);
extern void *CompilerTools_AllocateMemoryIfEnabled(SIZE_T size);
extern UInt32 CompilerTools_GetScaledTicks(void);
extern void (*DAT_00587708)(void);
extern struct Pool galloc_pool;
extern struct Pool data_0057fd84;
extern struct Pool block_pool;
extern struct Pool data_0057fdac;
extern struct Pool heap_pool;
extern void (*data_0057fdd4)();
extern SInt16 data_0057fdd8;
extern int next_name_id;
extern struct HashNameNode **data_00587f88;
extern int fn_00443200(CWFileSpec *record, short *output, unsigned int argument4, unsigned int argument5);
extern unsigned char CompilerTools_IsByteInDBCSCharacter(unsigned char *a0, unsigned char *a1);
extern short fn_00443250(CWFileSpec *h, short *a1);
extern SInt16 CompilerTools_GetFileType(CWFileSpec *arguments, UInt32 *output);
extern short CompilerTools_GetFileSize(short a0, SInt32 *a1);
extern SInt16 CompilerTools_ReadFile(SInt16 first, void *second, SInt32 third);
extern SInt16 CompilerTools_Write(SInt16 first, void *second, SInt32 third);
extern SInt16 CompilerTools_SetFilePosition(SInt16 a0, SInt32 a1);
extern void CompilerTools_CloseFile(short a0);
extern void CompilerTools_MakeCWFileSpecFromPString(void *result, unsigned char *name);
extern void CompilerTools_GetPFileFields(CWFileSpec *record, unsigned short *tag, SInt32 *value, void *data);
extern void resolve_file_name_to_pascal_string(short category, int recordId, void *inputName);
extern void CompilerTools_ResolveFileNameToCString(void *destination, PFile *record, SInt32 *result);
extern unsigned int CompilerTools_GetTicks(void);
extern short fn_004432f0(short value, long *result);

#ifdef __cplusplus
}
#endif

#endif
