#include "compiler/common.h"
#include "driver/CWCompilerLinkerPrivate.h"
#include "driver/CLPluginRequests.h"
#include "driver/CWPluginsPrivate.h"
#include <string.h>

CWPluginPrivateContext *validate_context_signature(CWPluginPrivateContext *record)
{
    if (record && ((long)record->contextSignature == 'Comp' || (long)record->contextSignature == 'Link')) {
        return record;
    }
    return NULL;
}

int __stdcall CWCompilerLinkerPrivate_GetEnabled(int key, unsigned char *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature((CWPluginPrivateContext *)key);
    if (record == NULL) {
        return 4;
    }
    *value = record->enabled;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetActive(int selector, unsigned char *value)
{
    CWPluginPrivateContext *state;

    if (value == NULL) {
        return 3;
    }
    state = validate_context_signature((CWPluginPrivateContext *)selector);
    if (state == NULL) {
        return 4;
    }
    *value = state->active;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetOperation(int key, unsigned char *value)
{
    CWPluginPrivateContext *result;

    if (value == NULL) {
        return 3;
    }
    result = validate_context_signature((CWPluginPrivateContext *)key);
    if (result == NULL) {
        return 4;
    }
    *value = result->operation;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetSetting(CWPluginPrivateContext *key, unsigned char *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature(key);
    if (record == NULL) {
        return 4;
    }
    *value = record->setting;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetReserved15d(int handle, unsigned char *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature((CWPluginPrivateContext *)handle);
    if (record == NULL) {
        return 4;
    }
    *value = record->reserved15d;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetDependencyState(CWPluginPrivateContext *context, BrowseOptions *state)
{
    CWPluginPrivateContext *validatedContext;
    if (state == NULL) {
        return 3;
    }
    validatedContext = validate_context_signature(context);
    if (validatedContext == NULL) {
        return 4;
    }
    *state = validatedContext->dependencyState;
    return 0;
}

SInt32 __stdcall CWCompilerLinkerPrivate_GetTargetSettings(CWPluginPrivateContext *context, TgtRec *target)
{
    CWPluginPrivateContext *plugin;

    if (target == NULL) {
        return 3;
    }
    plugin = validate_context_signature(context);
    if (plugin == NULL) {
        return 4;
    }
    if (plugin->apiVersion >= 10) {
        *target = *plugin->targetSettings;
    } else if (plugin->apiVersion >= 8) {
        target->head = plugin->targetSettings->head;
    } else {
        memset(target, 0, sizeof(*target));
        if (plugin->firstFile.name[0] != 0) {
            target->head.tag = 1;
        } else {
            target->head.tag = 0;
        }
        target->head.firstFile = plugin->firstFile;
        target->head.secondFile = plugin->secondFile;
        target->head.thirdFile = plugin->firstFile;
        target->head.linkage = plugin->linkage;
        target->head.firstByte = plugin->firstByte;
        target->head.secondByte = plugin->secondByte;
        target->head.thirdCode = plugin->thirdCode;
        target->head.fourthCode = plugin->fourthCode;
    }
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetFileIndex(CWPluginPrivateContext *key, unsigned int *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature(key);
    if (record == NULL) {
        return 4;
    }
    *value = record->requestData.fileIndex;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetDependencyOption(int lookupKey, unsigned short *value)
{
    CWPluginPrivateContext *record;

    if (value == NULL) {
        return 3;
    }
    record = validate_context_signature((CWPluginPrivateContext *)lookupKey);
    if (record == NULL) {
        return 4;
    }
    *value = record->dependencyOption;
    return 0;
}

int __stdcall CWCompilerLinkerPrivate_GetContextPayload(CWPluginPrivateContext *handle, CWFileSpec *destination)
{
    CWPluginPrivateContext *storage;
    if (destination == NULL)
        return 3;
    storage = (CWPluginPrivateContext *)validate_context_signature(handle);
    if (storage == NULL)
        return 4;
    *destination = storage->contextData.payload;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_GetResultValues(CWPluginPrivateContext *handle, UInt32 *first_value,
                                                               UInt32 *second_value)
{
    struct Value {
        unsigned short words[2];
    };
    struct ResultValues {
        unsigned char reserved[338];
        struct Value first;
        struct Value second;
    };
    CWPluginPrivateContext *record;
    struct ResultValues *values;

    if (first_value == NULL) {
        return 3;
    }
    if (second_value == NULL) {
        return 3;
    }
    record = validate_context_signature(handle);
    if (record == NULL) {
        return 4;
    }
    values = (struct ResultValues *)record;
    *(struct Value *)first_value = values->first;
    *(struct Value *)second_value = values->second;
    return 0;
}

unsigned int __stdcall CWCompilerLinkerPrivate_CallCompilerCallbackWithValue(CWPluginPrivateContext *object_id,
                                                                             unsigned int argument, long *value)
{
    CWPluginPrivateContext *object;

    if (value == NULL) {
        return 3;
    }
    object = validate_context_signature(object_id);
    if (object == NULL) {
        return 4;
    }
    return ((int(__stdcall **)(CWPluginPrivateContext *, unsigned int, long *))object->compilerCallbacks)[2](
        object, argument, value);
}

SInt32 __stdcall CWCompilerLinkerPrivate_CallCompilerCallback(void *objectId, SInt32 argument)
{
    CWPluginPrivateContext *object;

    object = validate_context_signature(objectId);
    if (object == NULL) {
        return 4;
    }
    return object->compilerCallbacks->callback(object, argument);
}

unsigned int __stdcall call_compiler_callback(CWPluginPrivateContext *object_id, void *argument, unsigned int result)
{
    CWPluginPrivateContext *object;
    if (argument == NULL) {
        return 3;
    }
    if (result == 0) {
        return 3;
    }
    object = validate_context_signature(object_id);
    if (object == NULL) {
        return 4;
    }
    return (*(unsigned int(__stdcall **)(CWPluginPrivateContext *, void *, unsigned int))object->compilerCallbacks)(
        object, argument, result);
}

unsigned int __stdcall CWCompilerLinkerPrivate_InvokeCompilerCallback(CWPluginPrivateContext *instance_id,
                                                                      struct FileProcessingInfo *argument,
                                                                      const char *value)
{
    CWPluginPrivateContext *instance;

    if (argument == NULL) {
        return 3;
    }
    instance = (CWPluginPrivateContext *)validate_context_signature(instance_id);
    if (instance == NULL) {
        return 4;
    }
    return instance->compilerCallbacks->invoke(instance, argument, value);
}
