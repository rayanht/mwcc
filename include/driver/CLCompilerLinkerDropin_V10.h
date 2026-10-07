#ifndef DRIVER_CLCOMPILERLINKERDROPIN_V10_H
#define DRIVER_CLCOMPILERLINKERDROPIN_V10_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct DropinConfiguration {
    struct StorageHandle *primaryReference;
    struct StorageHandle *secondaryReference;
    SInt32 fileType;
    UInt32 values[4];
    UInt8 flag;
    UInt8 reserved1;
    char *path;
    UInt8 reserved2[8];
    UInt16 option;
    CWFileSpec *outputFileSpec;
};
#pragma options align = reset
struct DropinContext {
    char reserved[8];
    struct DropinDiagnosticTarget *record;
};
struct DropinDiagnosticTarget {
    struct Plugin *diagnosticType;
};
#pragma options align = mac68k
struct DropinFileValue {
    int value;
};
#pragma options align = reset
#pragma options align = mac68k
struct DropinResultSlot {
    int value;
};
#pragma options align = reset
#pragma pack(push, 1)
struct InitializationAuxiliaryState {
    UInt8 opaquePrefix[0xd8];
    SInt32 defaultValue1;
    SInt32 defaultValue2;
    UInt8 opaqueSuffix[0x136 - 0xe0];
};
#pragma pack(pop)
extern int __stdcall cache_precompiled_header(unsigned int context, short *callback, int argument);
extern unsigned int __stdcall call_primary_reference_callback(unsigned int argument, unsigned int key, void *extra);
extern int __stdcall store_object_data(int compiler, int dropinId, DropinConfiguration *configuration);
extern unsigned int __stdcall report_begin_sub_compile_not_implemented(unsigned int argument0, unsigned int argument1,
                                                                       unsigned int argument2);
extern unsigned int __stdcall report_end_sub_compile_not_implemented(unsigned int unused);
extern int __stdcall get_precompiled_header_spec(DropinRequest *request, int output, const char *path);
extern unsigned int __stdcall fn_004262a0(unsigned int unused1, unsigned int unused2);
extern unsigned int __stdcall report_unimplemented_resource_file_put(unsigned int a0, unsigned int a1, unsigned int a2,
                                                                     unsigned int a3);
extern int __stdcall lookup_precompiled_unit(struct DropinRequest *request, char *inputName, char mode,
                                             void **outputObject, struct DropinResultSlot *outputValue);
extern unsigned int __stdcall log_callback(unsigned int unused1, unsigned int unused2);
extern int __stdcall store_precompiled_unit(void *obj, char *filename, int arg3, int arg4);
extern unsigned int __stdcall free_allocation(unsigned int unused, unsigned int value);
extern int __stdcall report_alert(DropinContext *context, const char *text, short code);
extern int __stdcall report_os_error_message(struct DropinRequest *request, const char *message, short code);
extern unsigned int __stdcall get_object_file_spec(unsigned int unused, unsigned int key, unsigned int output);
extern unsigned int __stdcall fn_00426da0(unsigned int unused, NameSpaceName *name, unsigned int unused2);
extern int __stdcall fn_00425ef0(unsigned int a0, unsigned int a1);
extern unsigned int __stdcall get_file_output_path(unsigned int a0, unsigned int a1, unsigned int a2);
extern unsigned int __stdcall copy_name_with_p_extension(unsigned int unused, const char *name, char *output);
extern unsigned int fn_00426320(OSSpec *destination, DropinFileRecord *record);
extern unsigned int __stdcall clear_primary_reference_value(unsigned int unused0, unsigned int recordKey,
                                                            unsigned int unused2);

#ifdef __cplusplus
}
#endif

#endif
