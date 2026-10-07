#ifndef DRIVER_RESOURCES_H
#define DRIVER_RESOURCES_H

#include "compiler/common.h"
#include "driver/MsDos.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct BinaryRecordPrefix {
    int words[4];
};
#pragma pack(pop)
#pragma pack(push, 1)
struct BinaryRecordTrailer {
    struct BinaryRecordPrefix prefix;
    int reserved;
    short values[5];
};
#pragma pack(pop)
struct IdentifierListNode {
    OSSpec fileSpec;
    int id;
    struct IdentifierListNode *next;
};
#pragma pack(push, 1)
struct RefEntry {
    UInt16 id;
    UInt16 nameOffset;
    UInt8 attributes;
    UInt8 dataOffset[3];
    UInt32 hand;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct ResData {
    UInt16 preservedPrefix[0x21];
    UInt8 creationTime[4];
    UInt8 modificationTime[4];
    UInt16 preservedSuffix[0x53];
};
#pragma pack(pop)
struct ResEntry {
    UInt16 id;
    UInt16 resourceTypeAlignment;
    UInt32 resourceType;
    unsigned char *name;
    UInt8 attrs;
    UInt8 handleAlignment[3];
    struct StorageHandle *hand;
    struct ResEntry *next;
};
#pragma pack(push, 2)
struct ResFile {
    short refnum;
    UInt16 attrs;
    struct ResType *types;
    ResData data;
    struct ResFile *next;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct ResHead {
    unsigned int data_offs;
    unsigned int map_offs;
    unsigned int data_len;
    unsigned int map_len;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct ResMap {
    ResHead hdr;
    unsigned int next;
    unsigned short file_ref;
    unsigned short attrs;
    unsigned short typelist_offs;
    unsigned short namelist_offs;
    unsigned short num_types;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct ResType {
    UInt32 type;
    struct ResEntry *entries;
    struct ResType *next;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ResourceDataHeader {
    unsigned int length;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ResourceTypeEntry {
    UInt32 type;
    UInt16 resourceCountMinusOne;
    UInt16 referenceListOffset;
};
#pragma pack(pop)
extern void write_resource_file(short refnum);
extern struct ResEntry *insert_res_entry(struct ResEntry **listAddress, UInt16 key, UInt32 value, UInt8 *name,
                                         UInt8 flag, void **extra);
extern void read_resource_file(short refnum, char readonly, unsigned char *buf, SInt32 size);
extern void write_default_binary_record(SInt16 file);
extern ResEntry *find_res_entry_in_files(int lookupArg, short lookupKind);
extern int Resources_SetFileTimes(unsigned int a, UInt32 b, UInt32 c);
extern short __stdcall open_resource_fork(void *a1, char a2, short *a3);
extern unsigned char fn_00406610(void);
extern struct ResFile *find_resfile_by_refnum(short value);
extern ResFile *find_resfile_with_previous(short value, ResFile **previous);
extern int count_types(ResFile *chain);
extern int count_entries(ResType *list);
extern ResType *find_res_type(ResFile *list, int key);
extern IdentifierListNode *Resources_FindIdentifierById(int id);
extern void __stdcall Resources_ClearError(void *handle);
extern unsigned int Resources_OpenResourceFile(OSSpec *path);
extern struct ResFile *create_resfile(short value0, int value2, int value4, int value8);
extern ResType *append_res_type(ResType **link, unsigned int value1, ResEntry *value2, ResType *next);
extern void free_res_file(short key);
extern unsigned int calculate_resource_sizes(ResFile *root, unsigned int *pa1, unsigned int *pa2, unsigned int *pa3,
                                             unsigned int *pa4);
extern void append_identifier_list_node(SInt32 a, OSSpec *b);
extern void Resources_RemoveIdentifier(unsigned int key);
extern Boolean Resources_FindIdentifier(OSSpec *key, SInt32 *value);
extern void fn_00408510(struct OSSpec *a0);
extern SInt16 __stdcall open_resource_file(void *param1, SInt8 param2);
extern unsigned char **Resources_GetHand(SInt32 first, SInt16 second);
extern void __stdcall close_resource_file(short param);
extern ResEntry *find_res_entry(ResFile *key, int index, short value);
extern SInt32 fn_004083b0(int handle, UInt32 *secondValue, UInt32 *firstValue);
extern UInt8 data_00588525;

#ifdef __cplusplus
}
#endif

#endif
