#ifndef DRIVER_TARGETPANELS_EABI_PPC_H
#define DRIVER_TARGETPANELS_EABI_PPC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CompilerSettings {
    unsigned char
        preferenceData[44]; /* 0x00: ReadCompilerSettings copies the opaque prefix of the preference resource */
    unsigned char
        reverseByteOrder; /* 0x2c: TargetPanels_eabi_ppc_LoadCompilerOptions sets copts.nativeByteOrder to its negation */
    unsigned char
        opaquePreferenceByte; /* 0x2d: ReadCompilerSettings copies this opaque resource byte without interpreting it */
    short smallDataLimit;     /* 0x2e: TargetPanels_eabi_ppc_LoadCompilerOptions copies to copts.smallDataLimit */
    short smallBSSLimit;      /* 0x30: TargetPanels_eabi_ppc_LoadCompilerOptions copies to copts.smallBSSLimit */
    unsigned char opaquePreferenceData
        [5]; /* 0x32: ReadCompilerSettings copies these opaque resource bytes without interpreting them */
    unsigned char unk37;
    unsigned char
        trailingPreferenceData[4]; /* 0x38: ReadCompilerSettings copies the opaque tail of the preference resource */
};
struct ExtendedCompilerSettings {
    unsigned char preferenceData[116];
};
struct CompilerOptions {
    short formatVersion;
    unsigned char structAlignment;
    unsigned char unk03;
    unsigned char
        reuseSectionSymbols; /* 0x04: TargetPanels_eabi_ppc_LoadCompilerOptions copies this to copts.reuseSectionSymbols. */
    unsigned char pad05[3];
    unsigned char unk08;
    unsigned char pad09[2];
    unsigned char
        floatingPointSupportEnabled; /* 0x0b: TargetPanels_eabi_ppc_LoadCompilerOptions passes this to SetFloatingPointSupport, selecting copts.f05 = 2 or 0. */
    unsigned char pad0c;
    unsigned char unk0d;
    unsigned char debugMode;
    unsigned char
        useRegisterSaveHelpers; /* 0x0f: TargetPanels_eabi_ppc_LoadCompilerOptions copies this to copts.useRegisterSaveHelpers, which StackFrameEABI uses to select register-save helpers. */
    short processor;
    unsigned char
        powerOfTwoShift; /* 0x12: TargetPanels_eabi_ppc_LoadCompilerOptions computes copts.f12 as 4 << this shift. */
    unsigned char
        debugOptions; /* 0x13: TargetPanels_eabi_ppc_LoadCompilerOptions copies this only for enabled, non-operand debugging. */
    unsigned char builtinStructConversionsEnabled;
    unsigned char
        targetFeatureEnabled; /* 0x15: TargetPanels_eabi_ppc_LoadCompilerOptions passes this to SetTargetFeatureEnabled, selecting copts.f11 = 1 or 0. */
    unsigned char pad16[2];
    unsigned int unk18;
};
extern void TargetPanels_eabi_ppc_LoadCompilerOptions(void);
extern SInt16 data_0057f6a8;
extern void fn_0042c0c0(void);
extern void fn_0042c910(void);

#ifdef __cplusplus
}
#endif

#endif
