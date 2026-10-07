#ifndef DRIVER_OPTIMIZERHELPERS_H
#define DRIVER_OPTIMIZERHELPERS_H

#include "compiler/common.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"

#ifdef __cplusplus
extern "C" {
#endif

union OptFlag {
    int i;
    Boolean b;
};
extern int fn_0040d8c0(short arg1, int arg2, int arg3, int arg4);
extern int parse_optimizer_settings(SInt32 option, unsigned char *options, int unused, UInt32 flags);
extern Boolean data_00540b26;
extern char data_00540b27;
extern int report_optimizer_options(void);
extern Pragma data_0054a388[8];

#ifdef __cplusplus
}
#endif

#endif
