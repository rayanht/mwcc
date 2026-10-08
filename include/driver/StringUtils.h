#ifndef DRIVER_STRINGUTILS_H
#define DRIVER_STRINGUTILS_H

#include "compiler/common.h"
#include "compiler/win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *mvprintf(char *buffer, unsigned int size, const char *format, va_list args);
extern UInt8 *_pstrcpy(UInt8 *destination, UInt8 *string);
extern unsigned int pstrcmp(UInt8 *left, char *right);
extern int pstrchr(UInt8 *characters, char character);
extern void __stdcall c2pstrcpy(unsigned char *dst, const char *src);
extern void __stdcall p2cstrcpy(char *destination, UInt8 *source);
extern int HPrintF(struct StorageHandle *output, char *format, ...);
extern char *mprintf(char *argument, int kind, char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
