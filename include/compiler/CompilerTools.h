#ifndef COMPILER_COMPILERTOOLS_H
#define COMPILER_COMPILERTOOLS_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct GList {
    char **data;
    SInt32 size;
    SInt32 hndlsize;
    SInt32 growsize;
};
#pragma options align = reset
struct HeapMem {
    struct HeapBlock *blocks;
    SInt32 allocsize;
    struct HeapBlock *curblock;
    char *curfreep;
    SInt32 curfree;
};
struct HeapBlock {
    struct HeapBlock *next;
    struct HeapBlock **blockhandle;
    SInt32 blocksize;
    SInt32 blockfree;
};
typedef struct {
    unsigned char byte0;
    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;
} NativeLongBytes;

union UInt32ByteSwapStorage {
    UInt32 word;
    UInt8 bytes[4];
};
extern void freeoheap(void);
extern void freeaheap(void);
extern void freelheap(void);
extern void *galloc(SInt32 size);
extern short CTool_EndianConvertInPlaceWord16Ptr(short *word);
extern void CTool_EndianConvertWord64(CInt64 ci, char *result);
extern UInt32 CTool_EndianConvertMem(void *buffer, short size);
extern unsigned int CTool_EndianConvertWord32(unsigned int value);
extern SInt16 getbit(UInt32 value);
extern void memclrw(void *buffer, unsigned int size);
extern void unlocklheap(void);
extern void locklheap(void);
extern int CTool_TotalHeapSize(void);
extern short CHash(const char *str);
extern void CTool_CtoPstr(unsigned char *text);
extern unsigned int CTool_EndianConvertInPlaceWord32Ptr(unsigned int *p);
extern UInt16 CTool_EndianConvertWord16(UInt16 word);
extern void CToLowercase(char *src, char *dst);
extern char *ScanDec(char *src, SInt32 *value, Boolean *flag);
extern void *oalloc(UInt32 requestedSize);
extern void *aalloc(SInt32 size);
extern void *lalloc(unsigned int size);
extern void releasegheap(void);
extern void releaseheaps(void);
extern SInt16 initheaps(void (*param)());
extern SInt16 CompilerTools_InitHeaps(void (*param)());
extern int select_or_allocate_pool_node(HeapMem *pool, SInt32 size);
extern void InitNameHash(void);
extern HashNameNode *GetHashNameNode(const char *text);
extern HashNameNode *GetHashNameNodeExport(const char *text);
extern void AppendGListName(GList *buf, const char *str);
extern void AppendGListID(GList *buf, const char *str);
extern void AppendGListTargetEndianLong(GList *buf, UInt32 l);
extern void AppendGListLong(GList *buffer, SInt32 value);
extern void AppendGListTargetEndianWord(GList *buf, UInt16 w);
extern void AppendGListWord(GList *buffer, SInt16 value);
extern void AppendGListByte(GList *buffer, SInt8 value);
extern void AppendGListNoData(GList *buffer, SInt32 additionalLength);
extern void *AppendGListData(GList *buffer, const void *source, SInt32 count);
extern void ShrinkGList(GList *list);
extern void LockGList(GList *entry);
extern void FreeGList(GList *storage);
extern SInt16 InitGList(GList *allocation, SInt32 size);
extern void CompilerGetCString(short value, char *destination);
extern void (*data_00587708)(void);
extern int next_name_id;
extern struct HashNameNode **data_00587f88;

#ifdef __cplusplus
}
#endif

#endif
