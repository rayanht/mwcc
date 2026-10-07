#ifndef DRIVER_WARNINGHELPERS_H
#define DRIVER_WARNINGHELPERS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int parse_warning_settings(int option, char *settings, int argument, int flags);
extern char data_00537770;
extern char data_00537771;
extern char data_0053777a;
extern unsigned int print_command_line_warning_options(void);

#ifdef __cplusplus
}
#endif

#endif
