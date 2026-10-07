#ifndef DRIVER_CC_EABI_PPC_MW_H
#define DRIVER_CC_EABI_PPC_MW_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
typedef struct PluginVersion PluginVersion;
typedef struct FileSignature FileSignature;
typedef struct FileSignatureList FileSignatureList;
struct PluginVersion {
    UInt8 major;
    UInt8 minor;
    UInt8 bugfix;
    UInt8 build;
};
/* A file type a plugin recognizes by the bytes at its start. */
struct FileSignature {
    UInt32 type;
    char *bytes;
    SInt32 length;
    SInt16 offset;
};
struct FileSignatureList {
    SInt16 count;
    struct FileSignature *signatures;
};
#pragma options align = reset
struct DataPointerObject {
    unsigned char *data;
};
struct ListLink {
    struct ListLink *next;
};
struct ListNode {
    struct ListNode *next;
};
struct ListNodeLink {
    struct ListNodeLink *next;
};
struct SharedDataHeader {
    unsigned char *data;
};
struct TableObject {
    unsigned char *table;
};
extern int __stdcall get_name_and_length(char **a0, int *a1);
extern void *__stdcall set_next_to_global(struct ListNodeLink *node);
extern unsigned int __stdcall copy_global_to_value(unsigned int *a0);
extern unsigned int __stdcall get_stored_value(unsigned int *a0);
extern int __stdcall fn_0040be10(signed char **a0);
extern unsigned int __stdcall fn_0040be20(struct ListLink *link);
extern unsigned int __stdcall fn_0040be30(struct ListLink *link);
extern void *__stdcall set_listnode_next_to_global(struct ListNode *node);
extern int __stdcall set_link_next_to_global(struct ListLink *link);
extern int __stdcall get_global_name_and_length(char **a0, int *a1);
extern unsigned int __stdcall set_data_pointer(unsigned int objectAddress);
extern unsigned int __stdcall fn_0040be90(unsigned int objectAddress);
extern unsigned int __stdcall fn_0040bea0(struct ListLink *link);
extern unsigned int __stdcall get_data_pointer(unsigned char **a0);
extern unsigned int __stdcall set_shared_data(struct SharedDataHeader *header);
extern unsigned int fn_0040bf10(void);
extern unsigned int fn_0040bed0(void);

#ifdef __cplusplus
}
#endif

#endif
