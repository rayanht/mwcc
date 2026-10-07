#ifndef DRIVER_CLSTATICPLUGINS_H
#define DRIVER_CLSTATICPLUGINS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int fn_004053d0(void);
extern int fn_004053f0(void);
extern unsigned int CLStaticPlugins_SetIdentifiers(SInt32 *architectureIdentifier, SInt32 *abiIdentifier);
extern unsigned int fn_004053a0(SInt32 *pluginType, SInt32 *pluginSubtype);
extern void fn_004053c0(SInt32 *pluginID);

#ifdef __cplusplus
}
#endif

#endif
