#ifndef DRIVER_TEXTUTILS_H
#define DRIVER_TEXTUTILS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct StringListHeader {
    unsigned char countHigh; /* 0x00: CLIO_GetResourceString reads the high byte of the big-endian STR# string count */
    unsigned char countLow;  /* 0x01: CLIO_GetResourceString reads the low byte of the STR# string count */
};
extern char *CLIO_ConvertToPascalString(char *string);
extern char *__stdcall CLIO_ConvertPascalToCString(char *p);
extern void __stdcall CLIO_GetResourceString(unsigned char *output, short resourceID, short stringIndex);
extern void __stdcall CLIO_GetResourceCString(char *a0, int a1, int a2);

#ifdef __cplusplus
}
#endif

#endif
