#ifndef DRIVER_CWCOMPILERLINKERPRIVATE_H
#define DRIVER_CWCOMPILERLINKERPRIVATE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CWPluginPrivateContext *validate_context_signature(CWPluginPrivateContext *record);
extern int __stdcall CWCompilerLinkerPrivate_GetEnabled(int key, unsigned char *value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetActive(int selector, unsigned char *value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetOperation(int key, unsigned char *value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetSetting(CWPluginPrivateContext *key, unsigned char *value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetReserved15d(int handle, unsigned char *value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetDependencyState(CWPluginPrivateContext *context,
                                                                         BrowseOptions *state);
extern SInt32 __stdcall CWCompilerLinkerPrivate_GetTargetSettings(CWPluginPrivateContext *obj, TgtRec *dst);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetFileIndex(CWPluginPrivateContext *key, unsigned int *value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetDependencyOption(int lookupKey, unsigned short *value);
extern int __stdcall CWCompilerLinkerPrivate_GetContextPayload(CWPluginPrivateContext *handle, CWFileSpec *destination);
extern unsigned int __stdcall CWCompilerLinkerPrivate_GetResultValues(CWPluginPrivateContext *handle,
                                                                      UInt32 *first_value, UInt32 *second_value);
extern unsigned int __stdcall CWCompilerLinkerPrivate_CallCompilerCallbackWithValue(CWPluginPrivateContext *object_id,
                                                                                    unsigned int argument, long *value);
extern SInt32 __stdcall CWCompilerLinkerPrivate_CallCompilerCallback(void *objectId, SInt32 argument);
extern unsigned int __stdcall call_compiler_callback(CWPluginPrivateContext *object_id, void *argument,
                                                     unsigned int result);
extern unsigned int __stdcall CWCompilerLinkerPrivate_InvokeCompilerCallback(CWPluginPrivateContext *instance_id,
                                                                             struct FileProcessingInfo *argument,
                                                                             const char *value);

#ifdef __cplusplus
}
#endif

#endif
