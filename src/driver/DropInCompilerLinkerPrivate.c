#include "compiler/common.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/CWPluginsPrivate.h"
/* Object record with an opaque prefix and a validation tag. */

unsigned char has_valid_shell_signature(CWPluginPrivateContext *object)
{
    unsigned char valid = 0;
    if (object != NULL && object->shellSignature == 0x43574945) {
        valid = 1;
    }
    return valid;
}

int __stdcall fn_0041bcb0(CWPluginPrivateContext *object, struct StorageHandle *argument2, long *argument3)
{
    if (has_valid_shell_signature(object) == '\0') {
        return 3;
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *, struct StorageHandle *, long *))object->callbacks[26])(
        object, argument2, argument3);
}

/* Object with an opaque prefix and a dispatch table. */
/* Dispatch table; the earlier entries are not used here. */
/* Opaque instance storage followed by an indexed dispatch table. */
unsigned int __stdcall dispatch_request_callback(CWPluginPrivateContext *context, unsigned int request, void *data)
{
    if (has_valid_shell_signature(context) == 0) {
        return 3;
    }
    return ((unsigned int(__stdcall *)(CWPluginPrivateContext *, unsigned int, void *))context->callbacks[27])(
        context, request, data);
}

int __stdcall DropInCompilerLinkerPrivate_CallArgumentValue(CWPluginPrivateContext *context, const char *argument,
                                                            void *value)
{
    unsigned int result;
    int resolvedArgument;

    if (value == NULL) {
        return 3;
    }
    result = CWPluginsPrivate_CallArgumentValueCallback(context, argument, &resolvedArgument);
    if (result == 0) {
        result = dispatch_request_callback(context, resolvedArgument, value);
    }
    return result;
}
