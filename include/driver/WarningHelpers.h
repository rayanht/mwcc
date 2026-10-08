#ifndef DRIVER_WARNINGHELPERS_H
#define DRIVER_WARNINGHELPERS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int parse_warning_settings(int option, char *settings, int argument, int flags);
extern unsigned int print_command_line_warning_options(void);

#ifdef __cplusplus
}
#endif

#endif
