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
struct GList;
extern void CompilerTools_ResetPool(void);
extern void CompilerTools_ResetPoolAvail(void);
extern void freelheap(void);
extern void *galloc(SInt32 size);
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
extern void (*DAT_00587708)(void);
extern int next_name_id;
extern struct HashNameNode **data_00587f88;

#ifdef __cplusplus
}
#endif

#endif
