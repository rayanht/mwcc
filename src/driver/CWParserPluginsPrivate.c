#include "compiler/common.h"
#include "driver/CWParserPluginsPrivate.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "driver/CLPluginRequests.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/Files.h"
typedef int(__stdcall *ParserTextCallback)(CWPluginPrivateContext *context, int argument2, int argument3, char *text);

typedef int(__stdcall *FileInfoMethod)(CWPluginPrivateContext *, CWFileSpec *);
typedef unsigned int(__stdcall *fnptr_std)(CWPluginPrivateContext *, FileOperationInfo *);

typedef int(__stdcall *AddOverlay1GroupCallback)(CWPluginPrivateContext *, char *, void *, SInt32 *);
typedef int(__stdcall *AddOverlay1Callback)(CWPluginPrivateContext *, char *, SInt32, SInt32 *);
typedef int(__stdcall *AddSegmentCallback)(CWPluginPrivateContext *, char *, short, SInt32 *);
typedef int(__stdcall *SetSegmentCallback)(CWPluginPrivateContext *, SInt32, char *, short);

typedef struct PanelEntry PanelEntry;

CWPluginPrivateContext *validate_parser_context(CWPluginPrivateContext *context)
{
    if (context != NULL && context->contextSignature == (CWPluginContext)'Pars') {
        return context;
    }
    return NULL;
}

unsigned int __stdcall CWParserPluginsPrivate_GetParserValues(CWPluginPrivateContext *query, unsigned int *first_value,
                                                              unsigned int *second_value)
{
    CWPluginPrivateContext *record;

    record = validate_parser_context(query);
    if (record == NULL) {
        return 4;
    }
    if (first_value == NULL) {
        return 3;
    }
    if (second_value == NULL) {
        return 3;
    }
    *first_value = record->contextData.parser.first_value;
    *second_value = record->contextData.parser.second_value;
    return 0;
}

int __stdcall CWParserPluginsPrivate_GetEnvironment(void *input, CommandLineArguments **value_out)
{
    CWPluginPrivateContext *record;

    record = validate_parser_context(input);
    if (record == NULL) {
        return 4;
    }
    if (value_out == NULL) {
        return 3;
    }
    *value_out = record->requestData.environment;
    return 0;
}

int __stdcall CWParserPluginsPrivate_GetOutputValues(struct CWPluginPrivateContext *context, unsigned char *firstValue,
                                                     unsigned char *secondValue)
{
    CWPluginPrivateContext *record;

    record = validate_parser_context(context);
    if (record == NULL) {
        return 4;
    }
    if (firstValue == NULL) {
        return 3;
    }
    if (secondValue == NULL) {
        return 3;
    }
    *(int *)firstValue = record->contextData.parser.outputFirstValue;
    *(void **)secondValue = record->contextData.parser.outputSecondValue;
    return 0;
}

int __stdcall CWParserPluginsPrivate_GetPanels(void *context, int *firstValue, struct PanelEntry **secondValue)
{
    CWPluginPrivateContext *record;

    record = validate_parser_context(context);
    if (record == NULL) {
        return 4;
    }
    if (firstValue == NULL) {
        return 3;
    }
    if (secondValue == NULL) {
        return 3;
    }
    *firstValue = record->contextData.parser.count;
    *secondValue = record->contextData.parser.panels;
    return 0;
}

int __stdcall CWParserPluginsPrivate_GetLookupValues(void *context, int *firstValue, char ***secondValue)
{
    CWPluginPrivateContext *record;

    record = validate_parser_context(context);
    if (record == NULL) {
        return 4;
    }
    if (firstValue == NULL) {
        return 3;
    }
    if (secondValue == NULL) {
        return 3;
    }
    *firstValue = record->contextData.parser.lookupFirstValue;
    *secondValue = record->contextData.parser.lookupSecondValue;
    return 0;
}

int __stdcall CWParserPluginsPrivate_SetParserEntry(void *ctx, int index, IntegerSequenceResult *value)
{
    CWPluginPrivateContext *collection;

    collection = validate_parser_context(ctx);
    if (collection == NULL) {
        return 4;
    }
    if (index < 0 || index >= collection->contextData.parser.count) {
        return 3;
    }
    if (value == NULL) {
        return 3;
    }
    collection->contextData.parser.entries[index] = *value;
    return 0;
}

int __stdcall fn_0041bfd0(void *context, char *name, void **value)
{
    int result;
    CWPluginPrivateContext *object;

    object = validate_parser_context(context);
    if (object == NULL) {
        return 4;
    }
    if (name == NULL) {
        return 3;
    }
    result = ((int(__stdcall *)(CWPluginPrivateContext *, char *,
                                void **))object->contextData.parser.parserCallbacks[1])(object, name, value);
    return result;
}

unsigned int __stdcall CWParserPluginsPrivate_CallFileOperationCallback(CWPluginPrivateContext *objectId,
                                                                        FileOperationInfo *argument)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(objectId);
    if (object == NULL) {
        return 4;
    }
    if (argument == NULL) {
        return 3;
    }
    return ((fnptr_std)object->contextData.parser.parserCallbacks[0])(object, argument);
}

int __stdcall CWParserPluginsPrivate_CallFileInfo(CWPluginPrivateContext *ctx, CWFileSpec *info)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(ctx);
    if (object == NULL) {
        return 4;
    }
    if (info == NULL) {
        return 3;
    }
    return ((FileInfoMethod)object->contextData.parser.parserCallbacks[3])(object, info);
}

int __stdcall CWParserPluginsPrivate_CallParserTextCallback(CWPluginPrivateContext *request, int argument2,
                                                            int argument3, char *text)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(request);
    if (object == NULL) {
        return 4;
    }
    if (text == NULL) {
        return 3;
    }
    return ((ParserTextCallback)object->contextData.parser.parserCallbacks[2])(object, argument2, argument3, text);
}

int __stdcall CWParserPluginsPrivate_PassValuePair(CWPluginPrivateContext *target, char *firstValue,
                                                   unsigned int secondValue)
{
    struct ValuePairState arguments;

    arguments.firstValue = firstValue;
    arguments.secondValue = secondValue;
    arguments.flag = 1;
    CWPluginsPrivate_CallValuePairCallback(target, &arguments);
}

void __stdcall CWParserPluginsPrivate_CallValuePairCallback(CWPluginPrivateContext *target, char *firstValue,
                                                            unsigned int secondValue)
{
    struct ValuePairState state;

    state.firstValue = firstValue;
    state.secondValue = secondValue;
    state.flag = 0;
    CWPluginsPrivate_CallValuePairCallback(target, &state);
}

#pragma auto_inline reset

int __stdcall CWParserPluginsPrivate_AddOverlay1Group(CWPluginPrivateContext *context, char *name, void *address,
                                                      SInt32 *groupNumber)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(context);
    if (object == NULL) {
        return 4;
    }
    if (name == NULL || address == NULL || groupNumber == NULL) {
        return 3;
    }
    return ((AddOverlay1GroupCallback)object->contextData.parser.parserCallbacks[4])(object, name, address,
                                                                                     groupNumber);
}

int __stdcall CWParserPluginsPrivate_AddOverlay1(CWPluginPrivateContext *context, char *name, SInt32 groupNumber,
                                                 SInt32 *overlayNumber)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(context);
    if (object == NULL) {
        return 4;
    }
    if (name == NULL || overlayNumber == NULL) {
        return 3;
    }
    return ((AddOverlay1Callback)object->contextData.parser.parserCallbacks[5])(object, name, groupNumber,
                                                                                overlayNumber);
}

int __stdcall CWParserPluginsPrivate_AddSegment(CWPluginPrivateContext *context, char *name, short attributes,
                                                SInt32 *segmentNumber)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(context);
    if (object == NULL) {
        return 4;
    }
    if (name == NULL || segmentNumber == NULL) {
        return 3;
    }
    return ((AddSegmentCallback)object->contextData.parser.parserCallbacks[6])(object, name, attributes, segmentNumber);
}

int __stdcall CWParserPluginsPrivate_SetSegment(CWPluginPrivateContext *context, SInt32 segmentNumber, char *name,
                                                short attributes)
{
    CWPluginPrivateContext *object;

    object = validate_parser_context(context);
    if (object == NULL) {
        return 4;
    }
    if (name == NULL) {
        return 3;
    }
    return ((SetSegmentCallback)object->contextData.parser.parserCallbacks[7])(object, segmentNumber, name, attributes);
}
