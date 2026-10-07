#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CWPluginsPrivate.h"
#include "compiler/CError.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/Files.h"
#include "driver/CLDropinCallbacks_V10.h"
typedef unsigned int __stdcall ContextArgumentCallback(CWPluginPrivateContext *context, unsigned int argument);
typedef unsigned int(__stdcall *ValuePairCallback)(CWPluginPrivateContext *, struct ValuePairState *);
typedef SInt32(__stdcall *DispatchOperation)(CWPluginPrivateContext *, void *, int, void *, void *);
static Boolean ValidateContext(CWPluginContext context)
{
    return context && context->shellSignature == 'CWIE';
}

static Boolean IsInitTermIdle(CWPluginContext context)
{
    return context->request == -2 || context->request == -1 || context->request == -100;
}

#pragma cplusplus on

Boolean is_valid_plugin_context(struct CWPluginPrivateContext *context)
{
    return (context && context->shellSignature == 'CWIE') &&
           !(context->request == -2 || context->request == -1 || context->request == -100);
}

#pragma cplusplus reset

static inline char hasEntrySignature(CWPluginPrivateContext *entry)
{
    char valid = 0;
    if (entry != NULL && entry->shellSignature == 0x43574945)
        valid = 1;
    return valid;
}

static inline char hasBasicEntryKind(CWPluginPrivateContext *entry)
{
    char valid = 1;
    if (entry->request != -2 && entry->request != -1)
        valid = 0;
    return valid;
}

static inline char hasEntryKind(CWPluginPrivateContext *entry)
{
    char valid = 1;
    if (!hasBasicEntryKind(entry) && entry->request != -100)
        valid = 0;
    return valid;
}

UInt8 has_entry_signature_and_kind(CWPluginPrivateContext *entry)
{
    UInt8 result = 0;
    if (hasEntrySignature(entry)) {
        if (hasEntryKind(entry))
            result = 1;
    }
    return result;
}

unsigned char is_valid_context(CWPluginPrivateContext *record)
{
    unsigned char matches = 0;

    if (record != NULL && (unsigned int)record->contextSignature == 0x56435320) {
        matches = 1;
    }
    return matches;
}

unsigned int __stdcall CWPluginsPrivate_GetRequest(CWPluginPrivateContext *entry, long *result)
{
    if (is_valid_plugin_context(entry) == 0 && has_entry_signature_and_kind(entry) == 0) {
        return 3;
    }
    if (result == NULL) {
        return 3;
    }
    *result = entry->request;
    return 0;
}

unsigned int __stdcall CWPluginsPrivate_GetAPIVersion(CWPluginPrivateContext *input, long *result)
{
    Boolean valid;

    valid = is_valid_plugin_context(input);
    if (valid == 0) {
        valid = has_entry_signature_and_kind(input);
        if (valid == 0) {
            return 3;
        }
    }
    if (result == NULL) {
        return 3;
    }
    *result = input->apiVersion;
    return 0;
}

int __stdcall CWPluginsPrivate_GetSourceFile(CWPluginPrivateContext *p, CWFileSpec *q)
{
    CWPluginPrivateContext *record = p;
    if (is_valid_context(record) || !is_valid_plugin_context(p))
        return 3;
    if (q == NULL)
        return 3;
    *q = p->sourcefile;
    return 0;
}

int __stdcall CWPluginsPrivate_GetOutputFileDirectory(CWPluginPrivateContext *context, CWFileSpec *directory)
{
    if (is_valid_context(context) || !is_valid_plugin_context(context))
        return 3;
    if (directory == NULL)
        return 3;
    *directory = context->targetfile;
    return 0;
}

unsigned int __stdcall CWPluginsPrivate_GetNumFiles(CWPluginPrivateContext *state, long *count)
{
    if (is_valid_context(state) || !is_valid_plugin_context(state))
        return 3U;
    if (!count)
        return 3U;
    *count = state->numFiles;
    return 0U;
}

int __stdcall CWPluginsPrivate_InvokeExportedRecordCallback(CWPluginPrivateContext *context, int index, int argument,
                                                            ExportedRecord *info)
{
    if (is_valid_context(context) != 0 || is_valid_plugin_context(context) == 0) {
        return 3;
    }
    if (info == NULL) {
        return 3;
    }
    return ((int(__stdcall *)(CWPluginPrivateContext *, int, int, ExportedRecord *))context->callbacks[0])(
        context, index, argument, info);
}

unsigned int __stdcall CWPluginsPrivate_CallSignatureCallback(CWPluginPrivateContext *context, const char *signature,
                                                              void *value)
{
    typedef unsigned int(__stdcall * SignatureCallback)(CWPluginPrivateContext *, const char *, void *);
    SignatureCallback *callbacks;

    if (is_valid_context(context) || !is_valid_plugin_context(context))
        return 3;
    if (signature == NULL)
        return 3;
    if (value == NULL)
        return 3;

    callbacks = (SignatureCallback *)context->callbacks;
    return callbacks[1](context, signature, value);
}

unsigned int ensure_callback_cache(struct CWPluginPrivateContext *context)
{
    unsigned int result;

    if (context->callbackCache == NULL) {
        result = ((unsigned int(__stdcall *)(struct CWPluginPrivateContext *))context->callbacks[35])(context);
        if (result != 0)
            return result;
        if (context->callbackCache == NULL)
            return 2;
    }
    return 0;
}

unsigned int __stdcall get_opcode_descriptor(CWPluginPrivateContext *input, PCodeOpcodeDescriptor *descriptor)
{
    unsigned int status;

    if (is_valid_plugin_context(input) == 0) {
        return 3;
    }
    if (descriptor == NULL) {
        return 3;
    }
    if (input->apiVersion < 10) {
        return 4;
    }
    status = ensure_callback_cache(input);
    if (status > 0) {
        return status;
    }
    descriptor->mnemonic = ((CachedOpcodeMetadata *)input->callbackCache)->mnemonic;
    descriptor->operand_format = ((CachedOpcodeMetadata *)input->callbackCache)->operand_format;
    descriptor->operand_count = ((CachedOpcodeMetadata *)input->callbackCache)->operand_count;
    descriptor->rank = ((CachedOpcodeMetadata *)input->callbackCache)->unknown_11;
    return 0;
}

unsigned int __stdcall CWPluginsPrivate_ValidateAndCallCallback(CWPluginPrivateContext *context, int argument2,
                                                                int argument3, unsigned int argument4, int argument5)
{
    if (is_valid_plugin_context(context) == '\0') {
        return 3;
    }
    if (argument2 == 0) {
        return 3;
    }
    if (argument3 == 0) {
        return 3;
    }
    if (argument4 == 0) {
        return 3;
    }
    if (argument5 == 0) {
        return 3;
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *, int, int, unsigned int, int))context->callbacks[2])(
        context, argument2, argument3, argument4, argument5);
}

int __stdcall fn_0041b7f0(CWPluginPrivateContext *object, void *argument)
{
    if (is_valid_plugin_context(object) == 0) {
        return 3;
    }
    if (argument == NULL) {
        return 0;
    }
    return ((int(__stdcall **)(CWPluginPrivateContext *, void *))object->callbacks)[3](object, argument);
}

unsigned int __stdcall CWPluginsPrivate_InvokeMessageCallback(void *object, struct MessageContext *argument1,
                                                              char *argument2, char *argument3, unsigned int argument4,
                                                              unsigned int argument5)
{
    if (!is_valid_plugin_context(object)) {
        return 3;
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *, struct MessageContext *, char *, char *, unsigned int,
                                       unsigned int))((CWPluginPrivateContext *)object)
                ->callbacks[8])(object, argument1, argument2, argument3, argument4, argument5);
}

int __stdcall CWPluginsPrivate_CallCallback9(void *object, char *argument2, char *argument3, unsigned char *argument4,
                                             unsigned int argument5)
{
    if (is_valid_plugin_context(object) == '\0') {
        return 3;
    }
    return ((int(__stdcall *)(struct CWPluginPrivateContext *, char *, char *, unsigned char *, unsigned int))(
                (CWPluginPrivateContext *)object)
                ->callbacks[9])(object, argument2, argument3, argument4, argument5);
}

unsigned int __stdcall fn_0041b8d0(CWPluginPrivateContext *object, char *argument1, void *argument2)
{
    Boolean ready;

    ready = is_valid_plugin_context(object);
    if (ready == '\0') {
        return 3;
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *, char *, void *))object->callbacks[10])(
        object, argument1, argument2);
}

int __stdcall fn_0041b910(struct CWPluginPrivateContext *object)
{
    if (is_valid_plugin_context(object) == 0) {
        return 3;
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *))object->callbacks[11])(object);
}

unsigned int __stdcall CWPluginsPrivate_CallArgumentValueCallback(CWPluginPrivateContext *object, const char *argument,
                                                                  int *value)
{
    unsigned int result;
    Boolean available;

    available = is_valid_plugin_context(object);
    if (available == '\0') {
        return 3;
    }
    if (argument == NULL) {
        return 3;
    }
    if (value == NULL) {
        return 3;
    }
    result =
        ((unsigned int(__stdcall *)(int *, const char *, int *))object->callbacks[12])((int *)object, argument, value);
    return result;
}

unsigned int __stdcall CWPluginsPrivate_CallFileProcessingCallback(CWPluginPrivateContext *object,
                                                                   struct FileProcessingInfo *value, void **argument3,
                                                                   unsigned int argument4)
{
    typedef unsigned int(__stdcall * FileProcessingCallback)(CWPluginPrivateContext *, struct FileProcessingInfo *,
                                                             void **, unsigned int);
    if (is_valid_context(object) != 0 || is_valid_plugin_context(object) == 0) {
        return 3;
    }
    if (value == NULL) {
        return 3;
    }
    return ((FileProcessingCallback *)object->callbacks)[0xf](object, value, argument3, argument4);
}

SInt32 __stdcall CWPluginsPrivate_OpenFile(CWPluginPrivateContext *state, CWFileSpec *argument, int value,
                                           FileOpenOptions *argument4, SInt32 *argument5)
{
    unsigned char rejected;
    SInt32 result;

    rejected = is_valid_context(state);
    if ((rejected != '\0') || (is_valid_plugin_context(state) == '\0')) {
        return 3;
    }
    if (argument == NULL) {
        return 3;
    }
    if (state->apiVersion < 8) {
        return 2;
    }
    result = ((DispatchOperation)state->callbacks[16])(state, argument, value, argument4, argument5);
    return result;
}

unsigned int __stdcall CWPluginsPrivate_CallValuePairCallback(CWPluginPrivateContext *object,
                                                              struct ValuePairState *argument)
{
    Boolean available;

    available = is_valid_plugin_context(object);
    if (available == '\0') {
        return 3;
    }
    if (argument == NULL) {
        return 3;
    }
    return ((ValuePairCallback *)object->callbacks)[0x11](object, argument);
}

unsigned int __stdcall fn_0041bab0(CWPluginPrivateContext *object, unsigned int argument1, unsigned int argument2,
                                   unsigned int *argument3)
{
    Boolean accepted;
    accepted = is_valid_plugin_context(object);
    if (accepted == 0) {
        accepted = has_entry_signature_and_kind(object);
        if (accepted == 0) {
            return 3;
        }
    }
    if (argument3 == NULL) {
        return 3;
    }
    return ((unsigned int(__stdcall *)(struct CWPluginPrivateContext *, unsigned int, unsigned int,
                                       unsigned int *))object->callbacks[20])(object, argument1, argument2, argument3);
}

unsigned int __stdcall CWPluginsPrivate_CallContextArgumentCallback(CWPluginPrivateContext *object,
                                                                    unsigned int argument)
{
    ContextArgumentCallback **dispatchTable;

    if (is_valid_plugin_context(object) == '\0' && has_entry_signature_and_kind(object) == '\0') {
        return 3;
    }
    dispatchTable = ((ContextArgumentCallback ***)object)[0x41];
    return dispatchTable[0x15](object, argument);
}

unsigned int __stdcall fn_0041bb50(CWPluginPrivateContext *object, unsigned int argument2, unsigned int argument3,
                                   UInt8 **argument4)
{
    Boolean available;

    available = is_valid_plugin_context(object);
    if (available == 0) {
        available = has_entry_signature_and_kind(object);
        if (available == 0) {
            return 3;
        }
    }
    if (argument4 == NULL) {
        return 3;
    }
    return ((unsigned int(__stdcall *)(int *, unsigned int, unsigned int, int))object->callbacks[24])(
        (int *)object, argument2, argument3, (int)argument4);
}

unsigned int __stdcall fn_0041bbb0(CWPluginPrivateContext *object, unsigned int argument)
{
    if (is_valid_plugin_context(object) == '\0' && has_entry_signature_and_kind(object) == '\0') {
        return 3;
    }
    return ((unsigned int(__stdcall *)(int *, unsigned int))object->callbacks[25])((int *)object, argument);
}

int __stdcall CWPluginsPrivate_ReturnArgument(CWPluginPrivateContext *context, int argument)
{
    return argument;
}

unsigned int __stdcall CWPluginsPrivate_InvokeCallback40(CWPluginPrivateContext *context, const char *argument2,
                                                         const char *argument3, unsigned int argument4,
                                                         unsigned int argument5, int *argument6)
{
    if (is_valid_plugin_context(context) == '\0') {
        if (has_entry_signature_and_kind(context) == '\0') {
            return 3;
        }
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *, const char *, const char *, unsigned int, unsigned int,
                                       int *))context->callbacks[40])(context, argument2, argument3, argument4,
                                                                      argument5, argument6);
}

unsigned int __stdcall fn_0041bc50(CWPluginPrivateContext *object, unsigned int argument)
{
    Boolean accepted;

    accepted = is_valid_plugin_context(object);
    if (accepted == 0) {
        accepted = has_entry_signature_and_kind(object);
        if (accepted == 0) {
            return 3;
        }
    }
    return ((unsigned int(__stdcall *)(int *, unsigned int))object->callbacks[41])((int *)object, argument);
}
