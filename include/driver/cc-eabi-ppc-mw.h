#ifndef DRIVER_CC_EABI_PPC_MW_H
#define DRIVER_CC_EABI_PPC_MW_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

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
extern char data_005434a8[];
extern void *__stdcall set_next_to_global(struct ListNodeLink *node);
extern unsigned int __stdcall copy_global_to_value(unsigned int *a0);
extern unsigned int data_005434e8;
extern unsigned int __stdcall get_stored_value(unsigned int *a0);
extern unsigned int data_00543500;
extern int __stdcall fn_0040be10(signed char **a0);
extern signed char data_00543694[];
extern unsigned int __stdcall fn_0040be20(struct ListLink *link);
extern unsigned int __stdcall fn_0040be30(struct ListLink *link);
extern void *__stdcall set_listnode_next_to_global(struct ListNode *node);
extern int __stdcall set_link_next_to_global(struct ListLink *link);
extern int __stdcall get_global_name_and_length(char **a0, int *a1);
extern char global_name[];
extern unsigned int __stdcall set_data_pointer(unsigned int objectAddress);
extern unsigned int __stdcall fn_0040be90(unsigned int objectAddress);
extern unsigned int __stdcall fn_0040bea0(struct ListLink *link);
extern unsigned int __stdcall get_data_pointer(unsigned char **a0);
extern unsigned char data_00543840[];
extern unsigned int __stdcall set_shared_data(struct SharedDataHeader *header);
extern unsigned char data_005438f0[];
extern unsigned int fn_0040bf10(void);
extern char *data_00545f68[];
extern char *data_00546598[];
extern char *data_00547a34[];
extern char *data_00549c44[];
extern int __stdcall get_stored_name_and_length(char **a0, int *a1);
extern char data_00549eb0[];
extern unsigned int __stdcall fn_0040bfc0(unsigned char *volatile *a0);
extern unsigned char data_00549ecc[];
extern unsigned int __stdcall get_global_value(unsigned int *a0);
extern unsigned int data_00549ef4;
extern unsigned int __stdcall fn_0040bff0(unsigned int *a0);
extern unsigned int data_00549ef8;
extern unsigned int __stdcall fn_0040c010(struct ListLink *link);
extern unsigned int __stdcall set_next_from_global(struct ListLink *node);
extern unsigned int __stdcall get_buffer(unsigned char **a0);
extern unsigned char data_00549fd4[];
extern int __stdcall set_list_link_next(struct ListLink *link);
extern unsigned int __stdcall fn_0040c050(struct ListLink *link);
extern unsigned char data_0054a014[];
extern unsigned int fn_0040c070(void);
extern unsigned int fn_0040bed0(void);
extern int fn_0040c060(void);
extern unsigned char PTR_fn_0054a05c[];
extern unsigned char PTR_fn_0054a080[];
extern unsigned char data_00543738[];
extern unsigned char data_005437bc[];
extern unsigned char data_005438f8[];
extern unsigned char data_0054391c[];
extern ListLink data_00543774;
extern ListLink data_0054a008[];
extern struct ListLink data_00543830;
extern struct ListLink data_00543700;
extern ListLink data_005436f8;
extern struct ListLink data_00549fcc[1];
extern struct ListLink data_00549fc4;
extern struct ListNode data_00543730;
extern struct ListNodeLink data_005434c4;

#ifdef __cplusplus
}
#endif

#endif
