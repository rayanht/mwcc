#ifndef COMPILER_CPREPROCESS_H
#define COMPILER_CPREPROCESS_H

#include "compiler/common.h"
#include "compiler/CompilerTools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NameBuf {
    UInt8 len;
    char name[256];
};
extern void CPreprocess_OutputPreprocessedText(void);
extern void append_escaped_text(const char *text, UInt16 n);
extern void output_escaped_wide_chars(char *data, SInt16 count);
extern void CPreprocess_EmitLineDirective(void);
extern void fn_004d6ed0(void);
extern char data_0056364c[11];
extern char data_00563658[16];
extern char protocol_string[10];
extern char private_directive[9];
extern char objc_protected_keyword[11];
extern char at_public[8];
extern char at_class_string[7];
extern char selector_string[10];
extern char data_005636b0[8];
extern char defs_string[6];
extern union {
    GList list;
    struct StorageHandle *handle;
} DAT_00586da8;
extern char s__Bool_005636d0[6];
extern char s__Complex_00563720[9];
extern char s__Imaginary_0056372c[11];
extern char s___attribute___00563704[14];
extern char s___cdecl_005635bc[8];
extern char s___declspec_005635dc[11];
extern char s___far_0056340c[6];
extern char s___fastcall_005635c4[11];
extern char s___floatcall_005635d0[12];
extern char s___restrict_005636ec[11];
extern char s___stdcall_005635b0[10];
extern char s___uuidof_00563714[9];
extern char s___vector_005636f8[9];
extern char s_break_00563488[6];
extern char s_bycopy_0056342c[7];
extern char s_byref_00563434[6];
extern char s_catch_005634a0[6];
extern char s_class_005633e8[6];
extern char s_const_005633f0[6];
extern char s_const_cast_00563518[11];
extern char s_continue_0056347c[9];
extern char s_default_00563448[8];
extern char s_delete_005634a8[7];
extern char s_double_005633b4[7];
extern char s_dynamic_cast_00563524[13];
extern char s_explicit_00563534[9];
extern char s_export_005635a8[7];
extern char s_extern_00563370[7];
extern char s_false_00563598[6];
extern char s_float_005633ac[6];
extern char s_friend_005634b0[7];
extern char s_inherited_0056350c[10];
extern char s_inline_00563380[7];
extern char s_inout_00563424[6];
extern char s_mutable_00563540[8];
extern char s_namespace_00563548[10];
extern char s_oneway_00563414[7];
extern char s_operator_005634bc[9];
extern char s_pascal_00563404[7];
extern char s_private_005634c8[8];
extern char s_protected_005634d0[10];
extern char s_public_005634dc[7];
extern char s_register_0056335c[9];
extern char s_reinterpret_cast_00563554[17];
extern char s_restrict_005636e0[9];
extern char s_return_00563490[7];
extern char s_short_00563398[6];
extern char s_signed_005633bc[7];
extern char s_sizeof_00563498[7];
extern char s_static_00563368[7];
extern char s_static_cast_00563568[12];
extern char s_struct_005633d0[7];
extern char s_super_005636c8[6];
extern char s_switch_0056345c[7];
extern char s_template_005634e4[9];
extern char s_throw_005634f8[6];
extern char s_typedef_00563378[8];
extern char s_typeid_005635a0[7];
extern char s_typename_00563584[9];
extern char s_union_005633d8[6];
extern char s_unsigned_005633c4[9];
extern char s_using_00563574[6];
extern char s_virtual_00563504[8];
extern char s_volatile_005633f8[9];
extern char s_wchar_t_0056357c[8];
extern char s_while_00563464[6];

#ifdef __cplusplus
}
#endif

#endif
