#ifndef DRIVER_CLSTATICMAIN_H
#define DRIVER_CLSTATICMAIN_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int main(int argc, char **argv);
extern char *build_time;
extern char *build_date;

#ifdef __cplusplus
}
#endif

#endif
