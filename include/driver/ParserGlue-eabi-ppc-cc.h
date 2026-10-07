#ifndef DRIVER_PARSERGLUE_EABI_PPC_CC_H
#define DRIVER_PARSERGLUE_EABI_PPC_CC_H

#include "compiler/common.h"
#include "driver/Targets.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Pragma {
    char *value;  /* 0x00: PragmaHasSetting reads the setting byte; ParserHelpers_cc_EmitPragmas checks PR_UNSET */
    char *pragma; /* 0x04: ParserHelpers_cc_EmitPragmas emits the pragma name and tests the table terminator */
    int flags;    /* 0x08: ParserHelpers_cc_EmitPragmas selects normal or reversed on/off settings */
};
extern unsigned int fn_00405670(void);
extern UInt8 data_00537aa2;
extern char data_00537d38;
extern int fn_004056a0(void);
extern signed short DAT_00537762;
extern int fn_00405710(void);
extern char data_0053776b;
extern char data_00537a67;
extern char data_00537a76;
extern char data_00537a77;
extern char data_00537b24;
extern char data_00537d40;
extern unsigned char data_00540add;
extern char *data_00540b68;
extern unsigned char data_0054a388[];
extern char output_path;
extern struct PtrList data_005876fc[];
extern struct StorageHandle *directive_storage;
extern int fn_00405840(void);
extern int data_00540bf8;

#ifdef __cplusplus
}
#endif

#endif
