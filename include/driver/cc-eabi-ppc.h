#ifndef DRIVER_CC_EABI_PPC_H
#define DRIVER_CC_EABI_PPC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LanguageSettings {
    unsigned char options[14];
};
struct SymbolSettings {
    unsigned char preferenceData[2]; /* 0x00: initialize_copts copies the opaque prefix from settingsHandle.symbols. */
    unsigned char deleteDeadInstructions; /* 0x02: initialize_copts copies this to copts.deleteDeadInstructions. */
    unsigned char
        spillBlockWeightMode; /* 0x03: initialize_copts enables copts.uniformSpillBlockWeight when this is 1. */
    unsigned char
        trailingPreferenceData[8]; /* 0x04: initialize_copts copies the opaque tail from settingsHandle.symbols. */
};
extern void cc_eabi_ppc_ReportCompilingFunction(char *name);
extern int __stdcall dispatch_compiler_plugin_request(CWPluginPrivateContext *input);
extern void initialize_copts(CPrepCU *source);
extern signed char initialize_compiler_plugin_cu(unsigned int input);
struct DriverSettings {
    unsigned char b[9];
    char name[32];
    unsigned char options[15];
    SInt16 inlineLimit; /* 0x38: cc-eabi-ppc.c copies this into copts.inlineLimit */
    unsigned char tail[4];
};
extern Boolean DAT_0054c3d0;
extern int data_00588258;
extern struct CPrepCU compiler_plugin_cu;

#ifdef __cplusplus
}
#endif

#endif
