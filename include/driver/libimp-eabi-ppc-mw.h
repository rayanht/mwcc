#ifndef DRIVER_LIBIMP_EABI_PPC_MW_H
#define DRIVER_LIBIMP_EABI_PPC_MW_H

#include "compiler/common.h"
#include "driver/cc-eabi-ppc-mw.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int __stdcall get_stored_name_and_length(char **a0, int *a1);
extern unsigned int __stdcall fn_0040bfc0(unsigned char *volatile *a0);
extern unsigned int __stdcall get_global_value(unsigned int *a0);
extern unsigned int __stdcall fn_0040bff0(unsigned int *a0);
extern unsigned int __stdcall fn_0040c010(struct ListLink *link);
extern unsigned int __stdcall set_next_from_global(struct ListLink *node);
extern unsigned int __stdcall get_buffer(unsigned char **a0);
extern int __stdcall set_list_link_next(struct ListLink *link);
extern unsigned int __stdcall fn_0040c050(struct ListLink *link);
extern int fn_0040c060(void);
extern unsigned int fn_0040c070(void);

#ifdef __cplusplus
}
#endif

#endif
