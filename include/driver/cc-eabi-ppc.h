#ifndef DRIVER_CC_EABI_PPC_H
#define DRIVER_CC_EABI_PPC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void cc_eabi_ppc_ReportCompilingFunction(char *name);
extern int __stdcall dispatch_compiler_plugin_request(CWPluginPrivateContext *input);
extern void initialize_copts(CPrepCU *source);
extern signed char initialize_compiler_plugin_cu(unsigned int input);
extern Boolean DAT_0054c3d0;
extern int data_00588258;
extern struct CPrepCU compiler_plugin_cu;

#ifdef __cplusplus
}
#endif

#endif
