#ifndef DRIVER_PROJECTS_H
#define DRIVER_PROJECTS_H

#include <setjmp.h>
#include "compiler/common.h"
#include "driver/CLAccessPaths.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct QueryValues {
    int firstValue;
    int secondValue;
    char reserved[4];
};
#pragma pack(pop)
union RecoveryPathFrame {
    OSPathBuffer copy;
    unsigned char bytes[324];
};
extern jmp_buf plugin_request_jmp_buf;
extern struct CWPluginPrivateContext *pluginPrivateContext;
extern void ToolHelpers_cc_CallValuePairCallback(char *key, struct StorageHandle *value);
extern char *format_version(unsigned int version, char *buf);
extern void ToolHelpers_cc_PrintVersion(char includeValue);
extern char data_00587e22;
extern int ToolHelpers_cc_GetNumFiles(void);
extern void ToolHelpers_cc_SetFileOutputName(int a, short b, char *s);
extern SInt32 ToolHelpers_cc_AddProjectEntry(OSSpec *path, SInt16 mode, char *name, Boolean flag, SInt32 fileId);
extern UInt8 data_00587e26;
extern UInt8 data_00587e27;
extern UInt8 data_00587e28;
extern int ToolHelpers_cc_AddAccessPath(char *spec, char use_first, int value, unsigned char option);
extern void ToolHelpers_cc_PassVirtualFileValuePair(char *fileData, struct StorageHandle **virtualFile);
extern void ToolHelpers_cc_GetOutputFileDirectory(CWFileSpec *directory);
extern void ToolHelpers_cc_CallFileInfoForDirectory(OSSpec *input);
extern void ToolHelpers_cc_AddOverlay1Group(char *name, void *address, SInt32 *groupNumber);
extern void ToolHelpers_cc_AddOverlay1(char *name, SInt32 groupNumber, SInt32 *overlayNumber);
extern void ToolHelpers_cc_AddSegment(char *name, short attributes, SInt32 *segmentNumber);
extern void ToolHelpers_cc_ChangeSegment(SInt32 segmentNumber, char *name, short attributes);
extern int data_00587e04;
extern int data_00587e08;
extern int data_00587e0c;

#ifdef __cplusplus
}
#endif

#endif
