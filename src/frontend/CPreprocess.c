#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CPreprocess.h"
#include "compiler/CError.h"
#include "compiler/CInit.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include <string.h>

#include <stdio.h>

static inline UInt8 CPreprocess_UseC99Keywords(void)
{
    return copts.f90;
}

static inline char CPreprocess_SuppressLineBreaks(void)
{
    return copts.f8b;
}

static inline void CPreprocess_FinishOutput(void)
{
    AppendGListByte(&DAT_00586da8.list, '\0');
    fn_00443170(DAT_00586da8.handle, DAT_00586da8.list.size);
}

void CPreprocess_OutputPreprocessedText(void)
{
    UInt8 *tokenStart;
    short token;

    if (InitGList(&DAT_00586da8.list, 10000) != 0)
        CError_LongJump();
    data_00588470 = 0;
    DAT_00588523 = '\0';
    token = CPrepTokenizer_GetNextToken();
    while (token != 0) {
        if (data_00588470 != 0) {
            if (!CPreprocess_SuppressLineBreaks())
                CompilerTools_AppendGListData(&DAT_00586da8.list, "\r\n", 2);
        } else if (DAT_00588523 != '\0') {
            AppendGListByte(&DAT_00586da8.list, ' ');
        }
        switch (token) {
            case '{':
                AppendGListByte(&DAT_00586da8.list, token);
                break;
            case '}':
                AppendGListByte(&DAT_00586da8.list, token);
                break;
            case -2:
            case -1:
                if (macro_expansion_depth > 0)
                    tokenStart = token_start;
                else
                    tokenStart = (UInt8 *)PTR_00587fb0 + bufferedTokenPosition[-1].tokenoffset;
                CompilerTools_AppendGListData(&DAT_00586da8.list, tokenStart, currentTextPosition - tokenStart);
                break;
            case -3:
                CompilerTools_AppendGListData(&DAT_00586da8.list, data_00587fa0->name, strlen(data_00587fa0->name));
                break;
            case 0x100:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "auto", 4);
                break;
            case 0x101:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_register_0056335c, sizeof(s_register_0056335c) - 1);
                break;
            case 0x102:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_static_00563368, sizeof(s_static_00563368) - 1);
                break;
            case 0x103:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_extern_00563370, sizeof(s_extern_00563370) - 1);
                break;
            case 0x104:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_typedef_00563378, sizeof(s_typedef_00563378) - 1);
                break;
            case 0x105:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_inline_00563380, sizeof(s_inline_00563380) - 1);
                break;
            case 0x106:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "void", 4);
                break;
            case 0x107:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "char", 4);
                break;
            case 0x108:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_short_00563398, sizeof(s_short_00563398) - 1);
                break;
            case 0x109:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "int", 3);
                break;
            case 0x10a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "long", 4);
                break;
            case 0x10b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_float_005633ac, sizeof(s_float_005633ac) - 1);
                break;
            case 0x10c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_double_005633b4, sizeof(s_double_005633b4) - 1);
                break;
            case 0x10d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_signed_005633bc, sizeof(s_signed_005633bc) - 1);
                break;
            case 0x10e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_unsigned_005633c4, sizeof(s_unsigned_005633c4) - 1);
                break;
            case 0x10f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_struct_005633d0, sizeof(s_struct_005633d0) - 1);
                break;
            case 0x110:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_union_005633d8, sizeof(s_union_005633d8) - 1);
                break;
            case 0x111:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "enum", 4);
                break;
            case 0x112:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_class_005633e8, sizeof(s_class_005633e8) - 1);
                break;
            case 0x121:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_const_005633f0, sizeof(s_const_005633f0) - 1);
                break;
            case 0x122:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_volatile_005633f8, sizeof(s_volatile_005633f8) - 1);
                break;
            case 0x123:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_pascal_00563404, sizeof(s_pascal_00563404) - 1);
                break;
            case 0x129:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___far_0056340c, sizeof(s___far_0056340c) - 1);
                break;
            case 0x12c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_oneway_00563414, sizeof(s_oneway_00563414) - 1);
                break;
            case 0x12d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "in", 2);
                break;
            case 0x12f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "out", 3);
                break;
            case 0x12e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_inout_00563424, sizeof(s_inout_00563424) - 1);
                break;
            case 0x130:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_bycopy_0056342c, sizeof(s_bycopy_0056342c) - 1);
                break;
            case 0x131:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_byref_00563434, sizeof(s_byref_00563434) - 1);
                break;
            case 0x136:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "asm", 3);
                break;
            case 0x137:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "case", 4);
                break;
            case 0x138:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_default_00563448, sizeof(s_default_00563448) - 1);
                break;
            case 0x139:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "if", 2);
                break;
            case 0x13a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "else", 4);
                break;
            case 0x13b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_switch_0056345c, sizeof(s_switch_0056345c) - 1);
                break;
            case 0x13c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_while_00563464, sizeof(s_while_00563464) - 1);
                break;
            case 0x13d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "do", 2);
                break;
            case 0x13e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "for", 3);
                break;
            case 0x13f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "goto", 4);
                break;
            case 0x140:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_continue_0056347c, sizeof(s_continue_0056347c) - 1);
                break;
            case 0x141:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_break_00563488, sizeof(s_break_00563488) - 1);
                break;
            case 0x142:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_return_00563490, sizeof(s_return_00563490) - 1);
                break;
            case 0x143:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_sizeof_00563498, sizeof(s_sizeof_00563498) - 1);
                break;
            case 0x144:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_catch_005634a0, sizeof(s_catch_005634a0) - 1);
                break;
            case 0x145:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_delete_005634a8, sizeof(s_delete_005634a8) - 1);
                break;
            case 0x146:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_friend_005634b0, sizeof(s_friend_005634b0) - 1);
                break;
            case 0x147:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "new", 3);
                break;
            case 0x148:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_operator_005634bc, sizeof(s_operator_005634bc) - 1);
                break;
            case 0x149:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_private_005634c8, sizeof(s_private_005634c8) - 1);
                break;
            case 0x14a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_protected_005634d0,
                                              sizeof(s_protected_005634d0) - 1);
                break;
            case 0x14b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_public_005634dc, sizeof(s_public_005634dc) - 1);
                break;
            case 0x14c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_template_005634e4, sizeof(s_template_005634e4) - 1);
                break;
            case 0x14d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "this", 4);
                break;
            case 0x14e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_throw_005634f8, sizeof(s_throw_005634f8) - 1);
                break;
            case 0x14f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "try", 3);
                break;
            case 0x150:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_virtual_00563504, sizeof(s_virtual_00563504) - 1);
                break;
            case 0x151:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_inherited_0056350c,
                                              sizeof(s_inherited_0056350c) - 1);
                break;
            case 0x152:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_const_cast_00563518,
                                              sizeof(s_const_cast_00563518) - 1);
                break;
            case 0x153:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_dynamic_cast_00563524,
                                              sizeof(s_dynamic_cast_00563524) - 1);
                break;
            case 0x12a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_explicit_00563534, sizeof(s_explicit_00563534) - 1);
                break;
            case 0x12b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_mutable_00563540, sizeof(s_mutable_00563540) - 1);
                break;
            case 0x154:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_namespace_00563548,
                                              sizeof(s_namespace_00563548) - 1);
                break;
            case 0x155:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_reinterpret_cast_00563554,
                                              sizeof(s_reinterpret_cast_00563554) - 1);
                break;
            case 0x156:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_static_cast_00563568,
                                              sizeof(s_static_cast_00563568) - 1);
                break;
            case 0x157:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_using_00563574, sizeof(s_using_00563574) - 1);
                break;
            case 0x11d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_wchar_t_0056357c, sizeof(s_wchar_t_0056357c) - 1);
                break;
            case 0x120:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_typename_00563584, sizeof(s_typename_00563584) - 1);
                break;
            case 0x158:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "true", 4);
                break;
            case 0x159:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_false_00563598, sizeof(s_false_00563598) - 1);
                break;
            case 0x15a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_typeid_005635a0, sizeof(s_typeid_005635a0) - 1);
                break;
            case 0x15b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_export_005635a8, sizeof(s_export_005635a8) - 1);
                break;
            case 0x125:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___stdcall_005635b0,
                                              sizeof(s___stdcall_005635b0) - 1);
                break;
            case 0x126:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___cdecl_005635bc, sizeof(s___cdecl_005635bc) - 1);
                break;
            case 0x127:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___fastcall_005635c4,
                                              sizeof(s___fastcall_005635c4) - 1);
                break;
            case 0x128:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___floatcall_005635d0,
                                              sizeof(s___floatcall_005635d0) - 1);
                break;
            case 0x124:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___declspec_005635dc,
                                              sizeof(s___declspec_005635dc) - 1);
                break;
            case 0x15c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "*=", 2);
                break;
            case 0x15d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "/=", 2);
                break;
            case 0x15e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "%=", 2);
                break;
            case 0x15f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "+=", 2);
                break;
            case 0x160:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "-=", 2);
                break;
            case 0x161:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "<<=", 3);
                break;
            case 0x162:
                CompilerTools_AppendGListData(&DAT_00586da8.list, ">>=", 3);
                break;
            case 0x163:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "&=", 2);
                break;
            case 0x164:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "^=", 2);
                break;
            case 0x165:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "|=", 2);
                break;
            case 0x166:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "||", 2);
                break;
            case 0x167:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "&&", 2);
                break;
            case 0x168:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "==", 2);
                break;
            case 0x169:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "!=", 2);
                break;
            case 0x16a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "<=", 2);
                break;
            case 0x16b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, ">=", 2);
                break;
            case 0x16c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "<<", 2);
                break;
            case 0x16d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, ">>", 2);
                break;
            case 0x16e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "++", 2);
                break;
            case 0x16f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "--", 2);
                break;
            case 0x170:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "->", 2);
                break;
            case 0x171:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "...", 3);
                break;
            case 0x172:
                CompilerTools_AppendGListData(&DAT_00586da8.list, ".*", 2);
                break;
            case 0x173:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "->*", 3);
                break;
            case 0x174:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "::", 2);
                break;
            case 0x175:
                CompilerTools_AppendGListData(&DAT_00586da8.list, data_0056364c, sizeof(data_0056364c) - 1);
                break;
            case 0x176:
                CompilerTools_AppendGListData(&DAT_00586da8.list, data_00563658, sizeof(data_00563658) - 1);
                break;
            case 0x177:
                CompilerTools_AppendGListData(&DAT_00586da8.list, protocol_string, sizeof(protocol_string) - 1);
                break;
            case 0x178:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "@end", 4);
                break;
            case 0x179:
                CompilerTools_AppendGListData(&DAT_00586da8.list, private_directive, sizeof(private_directive) - 1);
                break;
            case 0x17a:
                CompilerTools_AppendGListData(&DAT_00586da8.list, objc_protected_keyword,
                                              sizeof(objc_protected_keyword) - 1);
                break;
            case 0x17b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, at_public, sizeof(at_public) - 1);
                break;
            case 0x17c:
                CompilerTools_AppendGListData(&DAT_00586da8.list, at_class_string, sizeof(at_class_string) - 1);
                break;
            case 0x17d:
                CompilerTools_AppendGListData(&DAT_00586da8.list, selector_string, sizeof(selector_string) - 1);
                break;
            case 0x17e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, data_005636b0, sizeof(data_005636b0) - 1);
                break;
            case 0x17f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, defs_string, sizeof(defs_string) - 1);
                break;
            case 0x180:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "self", 4);
                break;
            case 0x181:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s_super_005636c8, sizeof(s_super_005636c8) - 1);
                break;
            case 0x11c:
                if (copts.cplusplus == '\0' && CPreprocess_UseC99Keywords())
                    CompilerTools_AppendGListData(&DAT_00586da8.list, s__Bool_005636d0, sizeof(s__Bool_005636d0) - 1);
                else
                    CompilerTools_AppendGListData(&DAT_00586da8.list, "bool", 4);
                break;
            case 0x184:
                if (CPreprocess_UseC99Keywords())
                    CompilerTools_AppendGListData(&DAT_00586da8.list, s_restrict_005636e0,
                                                  sizeof(s_restrict_005636e0) - 1);
                else
                    CompilerTools_AppendGListData(&DAT_00586da8.list, s___restrict_005636ec,
                                                  sizeof(s___restrict_005636ec) - 1);
                break;
            case 0x11b:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___vector_005636f8, sizeof(s___vector_005636f8) - 1);
                break;
            case 0x185:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___attribute___00563704,
                                              sizeof(s___attribute___00563704) - 1);
                break;
            case 0x186:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s___uuidof_00563714, sizeof(s___uuidof_00563714) - 1);
                break;
            case 0x11e:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s__Complex_00563720, sizeof(s__Complex_00563720) - 1);
                break;
            case 0x11f:
                CompilerTools_AppendGListData(&DAT_00586da8.list, s__Imaginary_0056372c,
                                              sizeof(s__Imaginary_0056372c) - 1);
                break;
            case -4:
                if (DAT_005882de != 0) {
                    CompilerTools_AppendGListData(&DAT_00586da8.list, "\"\\p", 3);
                    append_escaped_text(string_token_data + 1, token_value_kind_or_string_length - 1);
                } else {
                    AppendGListByte(&DAT_00586da8.list, '"');
                    append_escaped_text(string_token_data, token_value_kind_or_string_length - 1);
                }
                AppendGListByte(&DAT_00586da8.list, '"');
                break;
            case -5:
                CompilerTools_AppendGListData(&DAT_00586da8.list, "L\"", 2);
                output_escaped_wide_chars(string_token_data, token_value_kind_or_string_length / stwchar.size - 1);
                AppendGListByte(&DAT_00586da8.list, '"');
                break;
            default:
                if (' ' <= token && token <= 0xff)
                    AppendGListByte(&DAT_00586da8.list, token);
                else
                    CError_Internal("CPreprocess.c", 0x218);
                break;
        }
        CPrep_ResetBufferedTokenPosition();
        data_00588470 = 0;
        DAT_00588523 = '\0';
        token = CPrepTokenizer_GetNextToken();
    }
    CPreprocess_FinishOutput();
}
void append_escaped_text(const char *text, UInt16 n)
{
    const UInt8 *p = (const UInt8 *)text;
    while (n--) {
        if (*p < 0x20) {
            AppendGListByte(&DAT_00586da8.list, '\\');
            switch (*p) {
                case '\a':
                    AppendGListByte(&DAT_00586da8.list, 'a');
                    break;
                case '\b':
                    AppendGListByte(&DAT_00586da8.list, 'b');
                    break;
                case '\f':
                    AppendGListByte(&DAT_00586da8.list, 'f');
                    break;
                case '\n':
                    AppendGListByte(&DAT_00586da8.list, 'n');
                    break;
                case '\r':
                    AppendGListByte(&DAT_00586da8.list, 'r');
                    break;
                case '\t':
                    AppendGListByte(&DAT_00586da8.list, 't');
                    break;
                case '\v':
                    AppendGListByte(&DAT_00586da8.list, 'v');
                    break;
                default:
                    if (*p >= 8)
                        AppendGListByte(&DAT_00586da8.list, *p / 8 + '0');
                    AppendGListByte(&DAT_00586da8.list, (*p % 8) + '0');
                    break;
            }
        } else {
            switch (*p) {
                case '"':
                case '\\':
                    AppendGListByte(&DAT_00586da8.list, '\\');
                    break;
            }
            AppendGListByte(&DAT_00586da8.list, *p);
        }
        p++;
    }
}
// "unknown.c"

static void outchar(char ch)
{
    AppendGListByte(&DAT_00586da8.list, ch);
}

void output_escaped_wide_chars(char *data, SInt16 count)
{
    UInt16 *p = (UInt16 *)data;
    SInt32 j, i;

    while (count--) {
        if (*p < 0x20) {
            outchar('\\');
            switch (*p) {
                case 7:
                    outchar('a');
                    break;
                case 8:
                    outchar('b');
                    break;
                case 27:
                    outchar('e');
                    break;
                case 12:
                    outchar('f');
                    break;
                case 10:
                    outchar('n');
                    break;
                case 13:
                    outchar('r');
                    break;
                case 9:
                    outchar('t');
                    break;
                case 11:
                    outchar('v');
                    break;
                default:
                    if (*p >= 8)
                        outchar(*p / 8 + '0');
                    outchar(*p % 8 + '0');
                    break;
            }
        } else if (*p > 0xff) {
            outchar('\\');
            outchar('x');
            j = 0x1000;
            i = 0;
            do {
                outchar("0123456789ABCDEF"[(*p / j) % 16]);
                i++;
                j /= 16;
            } while (i < 4);
        } else {
            switch (*p) {
                case '"':
                case '\\':
                    outchar('\\');
                    break;
            }
            outchar((char)*p);
        }
        p++;
    }
}
// "CPrec.c"

static inline char CPreprocess_ShouldEmitLineDirectives(void)
{
    return copts.f8b;
}

static inline void CPreprocess_AppendLineBreak(void)
{
    if (CPreprocess_ShouldEmitLineDirectives() && DAT_00586da8.handle != NULL && current_file_index >= 0)
        CompilerTools_AppendGListData(&DAT_00586da8.list, "\r\n", 2);
}

void CPreprocess_EmitLineDirective(void)
{
    char buffer[512];
    SInt32 fileValue;
    NameBuf fileName;
    UInt16 fileTag;
    int length;

    if (DAT_00586da8.handle != NULL && current_file_index >= 0) {
        if (data_00588470 != 0 && DAT_00586da8.list.size > 0)
            CompilerTools_AppendGListString(&DAT_00586da8.list, "\r\n");

        if (CPreprocess_ShouldEmitLineDirectives())
            length = sprintf(buffer, "#line %ld\t\"", DAT_00587ef0);
        else
            length = sprintf(buffer, "/* #line %ld\t\"", DAT_00587ef0);
        CompilerTools_AppendGListData(&DAT_00586da8.list, buffer, length);

        if (copts.f8c != 0) {
            CompilerTools_ResolveFileNameToCString(buffer, currentPFile, &fileValue);
            CompilerTools_AppendGListData(&DAT_00586da8.list, buffer, strlen(buffer));
        } else {
            CompilerTools_GetPFileFields(&currentPFile->header, &fileTag, &fileValue, &fileName.len);
            CompilerTools_AppendGListData(&DAT_00586da8.list, fileName.name, fileName.len);
        }

        length = sprintf(buffer, "\"\t/* stack depth %ld */", current_file_index);
        CompilerTools_AppendGListData(&DAT_00586da8.list, buffer, length);

        if (CPreprocess_ShouldEmitLineDirectives())
            CPreprocess_AppendLineBreak();

        data_00588470 = 1;
    }
}

void fn_004d6ed0(void)
{
    if (copts.f8b != '\0' && DAT_00586da8.handle != NULL && current_file_index >= 0) {
        CompilerTools_AppendGListData(&DAT_00586da8.list, "\r\n", 2);
    }
}
