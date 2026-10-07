#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CPreprocess.h"
#include "compiler/CError.h"
#include "compiler/CInit.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include <string.h>

#include <stdio.h>

static inline UInt8 CPreprocess_UseC99Keywords(void)
{
    return copts.c9x;
}

static inline char CPreprocess_SuppressLineBreaks(void)
{
    return copts.line_prepdump;
}

static inline void CPreprocess_FinishOutput(void)
{
    AppendGListByte(&data_00586da8.list, '\0');
    COS_ResizeHandle(data_00586da8.handle, data_00586da8.list.size);
}

// "unknown.c"

static void outchar(char ch)
{
    AppendGListByte(&data_00586da8.list, ch);
}

// "CPrec.c"

static inline char CPreprocess_ShouldEmitLineDirectives(void)
{
    return copts.line_prepdump;
}

static inline void CPreprocess_AppendLineBreak(void)
{
    if (CPreprocess_ShouldEmitLineDirectives() && data_00586da8.handle != NULL && current_file_index >= 0)
        CompilerTools_AppendGListData(&data_00586da8.list, "\r\n", 2);
}

void fn_004d6ed0(void)
{
    if (copts.line_prepdump != '\0' && data_00586da8.handle != NULL && current_file_index >= 0) {
        CompilerTools_AppendGListData(&data_00586da8.list, "\r\n", 2);
    }
}

void CPreprocess_EmitLineDirective(void)
{
    char buffer[512];
    SInt32 fileValue;
    NameBuf fileName;
    UInt16 fileTag;
    int length;

    if (data_00586da8.handle != NULL && current_file_index >= 0) {
        if (data_00588470 != 0 && data_00586da8.list.size > 0)
            CompilerTools_AppendGListString(&data_00586da8.list, "\r\n");

        if (CPreprocess_ShouldEmitLineDirectives())
            length = sprintf(buffer, "#line %ld\t\"", data_00587ef0);
        else
            length = sprintf(buffer, "/* #line %ld\t\"", data_00587ef0);
        CompilerTools_AppendGListData(&data_00586da8.list, buffer, length);

        if (copts.fullpath_prepdump != 0) {
            COS_FileGetPathName(buffer, currentPFile, &fileValue);
            CompilerTools_AppendGListData(&data_00586da8.list, buffer, strlen(buffer));
        } else {
            COS_FileGetFSSpecInfo(&currentPFile->textfile, &fileTag, &fileValue, &fileName.len);
            CompilerTools_AppendGListData(&data_00586da8.list, fileName.name, fileName.len);
        }

        length = sprintf(buffer, "\"\t/* stack depth %ld */", current_file_index);
        CompilerTools_AppendGListData(&data_00586da8.list, buffer, length);

        if (CPreprocess_ShouldEmitLineDirectives())
            CPreprocess_AppendLineBreak();

        data_00588470 = 1;
    }
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

void append_escaped_text(const char *text, UInt16 n)
{
    const UInt8 *p = (const UInt8 *)text;
    while (n--) {
        if (*p < 0x20) {
            AppendGListByte(&data_00586da8.list, '\\');
            switch (*p) {
                case '\a':
                    AppendGListByte(&data_00586da8.list, 'a');
                    break;
                case '\b':
                    AppendGListByte(&data_00586da8.list, 'b');
                    break;
                case '\f':
                    AppendGListByte(&data_00586da8.list, 'f');
                    break;
                case '\n':
                    AppendGListByte(&data_00586da8.list, 'n');
                    break;
                case '\r':
                    AppendGListByte(&data_00586da8.list, 'r');
                    break;
                case '\t':
                    AppendGListByte(&data_00586da8.list, 't');
                    break;
                case '\v':
                    AppendGListByte(&data_00586da8.list, 'v');
                    break;
                default:
                    if (*p >= 8)
                        AppendGListByte(&data_00586da8.list, *p / 8 + '0');
                    AppendGListByte(&data_00586da8.list, (*p % 8) + '0');
                    break;
            }
        } else {
            switch (*p) {
                case '"':
                case '\\':
                    AppendGListByte(&data_00586da8.list, '\\');
                    break;
            }
            AppendGListByte(&data_00586da8.list, *p);
        }
        p++;
    }
}

void CPreprocess_OutputPreprocessedText(void)
{
    UInt8 *tokenStart;
    short token;

    if (InitGList(&data_00586da8.list, 10000) != 0)
        CError_LongJump();
    data_00588470 = 0;
    data_00588523 = '\0';
    token = CPrepTokenizer_GetNextToken();
    while (token != 0) {
        if (data_00588470 != 0) {
            if (!CPreprocess_SuppressLineBreaks())
                CompilerTools_AppendGListData(&data_00586da8.list, "\r\n", 2);
        } else if (data_00588523 != '\0') {
            AppendGListByte(&data_00586da8.list, ' ');
        }
        switch (token) {
            case '{':
                AppendGListByte(&data_00586da8.list, token);
                break;
            case '}':
                AppendGListByte(&data_00586da8.list, token);
                break;
            case -2:
            case -1:
                if (macro_expansion_depth > 0)
                    tokenStart = token_start;
                else
                    tokenStart = (UInt8 *)data_00587fb0 + bufferedTokenPosition[-1].tokenoffset;
                CompilerTools_AppendGListData(&data_00586da8.list, tokenStart, currentTextPosition - tokenStart);
                break;
            case -3:
                CompilerTools_AppendGListData(&data_00586da8.list, data_00587fa0->name, strlen(data_00587fa0->name));
                break;
            case 0x100:
                CompilerTools_AppendGListData(&data_00586da8.list, "auto", 4);
                break;
            case 0x101:
                CompilerTools_AppendGListData(&data_00586da8.list, "register", 8);
                break;
            case 0x102:
                CompilerTools_AppendGListData(&data_00586da8.list, "static", 6);
                break;
            case 0x103:
                CompilerTools_AppendGListData(&data_00586da8.list, "extern", 6);
                break;
            case 0x104:
                CompilerTools_AppendGListData(&data_00586da8.list, "typedef", 7);
                break;
            case 0x105:
                CompilerTools_AppendGListData(&data_00586da8.list, "inline", 6);
                break;
            case 0x106:
                CompilerTools_AppendGListData(&data_00586da8.list, "void", 4);
                break;
            case 0x107:
                CompilerTools_AppendGListData(&data_00586da8.list, "char", 4);
                break;
            case 0x108:
                CompilerTools_AppendGListData(&data_00586da8.list, "short", 5);
                break;
            case 0x109:
                CompilerTools_AppendGListData(&data_00586da8.list, "int", 3);
                break;
            case 0x10a:
                CompilerTools_AppendGListData(&data_00586da8.list, "long", 4);
                break;
            case 0x10b:
                CompilerTools_AppendGListData(&data_00586da8.list, "float", 5);
                break;
            case 0x10c:
                CompilerTools_AppendGListData(&data_00586da8.list, "double", 6);
                break;
            case 0x10d:
                CompilerTools_AppendGListData(&data_00586da8.list, "signed", 6);
                break;
            case 0x10e:
                CompilerTools_AppendGListData(&data_00586da8.list, "unsigned", 8);
                break;
            case 0x10f:
                CompilerTools_AppendGListData(&data_00586da8.list, "struct", 6);
                break;
            case 0x110:
                CompilerTools_AppendGListData(&data_00586da8.list, "union", 5);
                break;
            case 0x111:
                CompilerTools_AppendGListData(&data_00586da8.list, "enum", 4);
                break;
            case 0x112:
                CompilerTools_AppendGListData(&data_00586da8.list, "class", 5);
                break;
            case 0x121:
                CompilerTools_AppendGListData(&data_00586da8.list, "const", 5);
                break;
            case 0x122:
                CompilerTools_AppendGListData(&data_00586da8.list, "volatile", 8);
                break;
            case 0x123:
                CompilerTools_AppendGListData(&data_00586da8.list, "pascal", 6);
                break;
            case 0x129:
                CompilerTools_AppendGListData(&data_00586da8.list, "__far", 5);
                break;
            case 0x12c:
                CompilerTools_AppendGListData(&data_00586da8.list, "oneway", 6);
                break;
            case 0x12d:
                CompilerTools_AppendGListData(&data_00586da8.list, "in", 2);
                break;
            case 0x12f:
                CompilerTools_AppendGListData(&data_00586da8.list, "out", 3);
                break;
            case 0x12e:
                CompilerTools_AppendGListData(&data_00586da8.list, "inout", 5);
                break;
            case 0x130:
                CompilerTools_AppendGListData(&data_00586da8.list, "bycopy", 6);
                break;
            case 0x131:
                CompilerTools_AppendGListData(&data_00586da8.list, "byref", 5);
                break;
            case 0x136:
                CompilerTools_AppendGListData(&data_00586da8.list, "asm", 3);
                break;
            case 0x137:
                CompilerTools_AppendGListData(&data_00586da8.list, "case", 4);
                break;
            case 0x138:
                CompilerTools_AppendGListData(&data_00586da8.list, "default", 7);
                break;
            case 0x139:
                CompilerTools_AppendGListData(&data_00586da8.list, "if", 2);
                break;
            case 0x13a:
                CompilerTools_AppendGListData(&data_00586da8.list, "else", 4);
                break;
            case 0x13b:
                CompilerTools_AppendGListData(&data_00586da8.list, "switch", 6);
                break;
            case 0x13c:
                CompilerTools_AppendGListData(&data_00586da8.list, "while", 5);
                break;
            case 0x13d:
                CompilerTools_AppendGListData(&data_00586da8.list, "do", 2);
                break;
            case 0x13e:
                CompilerTools_AppendGListData(&data_00586da8.list, "for", 3);
                break;
            case 0x13f:
                CompilerTools_AppendGListData(&data_00586da8.list, "goto", 4);
                break;
            case 0x140:
                CompilerTools_AppendGListData(&data_00586da8.list, "continue", 8);
                break;
            case 0x141:
                CompilerTools_AppendGListData(&data_00586da8.list, "break", 5);
                break;
            case 0x142:
                CompilerTools_AppendGListData(&data_00586da8.list, "return", 6);
                break;
            case 0x143:
                CompilerTools_AppendGListData(&data_00586da8.list, "sizeof", 6);
                break;
            case 0x144:
                CompilerTools_AppendGListData(&data_00586da8.list, "catch", 5);
                break;
            case 0x145:
                CompilerTools_AppendGListData(&data_00586da8.list, "delete", 6);
                break;
            case 0x146:
                CompilerTools_AppendGListData(&data_00586da8.list, "friend", 6);
                break;
            case 0x147:
                CompilerTools_AppendGListData(&data_00586da8.list, "new", 3);
                break;
            case 0x148:
                CompilerTools_AppendGListData(&data_00586da8.list, "operator", 8);
                break;
            case 0x149:
                CompilerTools_AppendGListData(&data_00586da8.list, "private", 7);
                break;
            case 0x14a:
                CompilerTools_AppendGListData(&data_00586da8.list, "protected", 9);
                break;
            case 0x14b:
                CompilerTools_AppendGListData(&data_00586da8.list, "public", 6);
                break;
            case 0x14c:
                CompilerTools_AppendGListData(&data_00586da8.list, "template", 8);
                break;
            case 0x14d:
                CompilerTools_AppendGListData(&data_00586da8.list, "this", 4);
                break;
            case 0x14e:
                CompilerTools_AppendGListData(&data_00586da8.list, "throw", 5);
                break;
            case 0x14f:
                CompilerTools_AppendGListData(&data_00586da8.list, "try", 3);
                break;
            case 0x150:
                CompilerTools_AppendGListData(&data_00586da8.list, "virtual", 7);
                break;
            case 0x151:
                CompilerTools_AppendGListData(&data_00586da8.list, "inherited", 9);
                break;
            case 0x152:
                CompilerTools_AppendGListData(&data_00586da8.list, "const_cast", 10);
                break;
            case 0x153:
                CompilerTools_AppendGListData(&data_00586da8.list, "dynamic_cast", 12);
                break;
            case 0x12a:
                CompilerTools_AppendGListData(&data_00586da8.list, "explicit", 8);
                break;
            case 0x12b:
                CompilerTools_AppendGListData(&data_00586da8.list, "mutable", 7);
                break;
            case 0x154:
                CompilerTools_AppendGListData(&data_00586da8.list, "namespace", 9);
                break;
            case 0x155:
                CompilerTools_AppendGListData(&data_00586da8.list, "reinterpret_cast", 16);
                break;
            case 0x156:
                CompilerTools_AppendGListData(&data_00586da8.list, "static_cast", 11);
                break;
            case 0x157:
                CompilerTools_AppendGListData(&data_00586da8.list, "using", 5);
                break;
            case 0x11d:
                CompilerTools_AppendGListData(&data_00586da8.list, "wchar_t", 7);
                break;
            case 0x120:
                CompilerTools_AppendGListData(&data_00586da8.list, "typename", 8);
                break;
            case 0x158:
                CompilerTools_AppendGListData(&data_00586da8.list, "true", 4);
                break;
            case 0x159:
                CompilerTools_AppendGListData(&data_00586da8.list, "false", 5);
                break;
            case 0x15a:
                CompilerTools_AppendGListData(&data_00586da8.list, "typeid", 6);
                break;
            case 0x15b:
                CompilerTools_AppendGListData(&data_00586da8.list, "export", 6);
                break;
            case 0x125:
                CompilerTools_AppendGListData(&data_00586da8.list, "__stdcall", 9);
                break;
            case 0x126:
                CompilerTools_AppendGListData(&data_00586da8.list, "__cdecl", 7);
                break;
            case 0x127:
                CompilerTools_AppendGListData(&data_00586da8.list, "__fastcall", 10);
                break;
            case 0x128:
                CompilerTools_AppendGListData(&data_00586da8.list, "__floatcall", 11);
                break;
            case 0x124:
                CompilerTools_AppendGListData(&data_00586da8.list, "__declspec", 10);
                break;
            case 0x15c:
                CompilerTools_AppendGListData(&data_00586da8.list, "*=", 2);
                break;
            case 0x15d:
                CompilerTools_AppendGListData(&data_00586da8.list, "/=", 2);
                break;
            case 0x15e:
                CompilerTools_AppendGListData(&data_00586da8.list, "%=", 2);
                break;
            case 0x15f:
                CompilerTools_AppendGListData(&data_00586da8.list, "+=", 2);
                break;
            case 0x160:
                CompilerTools_AppendGListData(&data_00586da8.list, "-=", 2);
                break;
            case 0x161:
                CompilerTools_AppendGListData(&data_00586da8.list, "<<=", 3);
                break;
            case 0x162:
                CompilerTools_AppendGListData(&data_00586da8.list, ">>=", 3);
                break;
            case 0x163:
                CompilerTools_AppendGListData(&data_00586da8.list, "&=", 2);
                break;
            case 0x164:
                CompilerTools_AppendGListData(&data_00586da8.list, "^=", 2);
                break;
            case 0x165:
                CompilerTools_AppendGListData(&data_00586da8.list, "|=", 2);
                break;
            case 0x166:
                CompilerTools_AppendGListData(&data_00586da8.list, "||", 2);
                break;
            case 0x167:
                CompilerTools_AppendGListData(&data_00586da8.list, "&&", 2);
                break;
            case 0x168:
                CompilerTools_AppendGListData(&data_00586da8.list, "==", 2);
                break;
            case 0x169:
                CompilerTools_AppendGListData(&data_00586da8.list, "!=", 2);
                break;
            case 0x16a:
                CompilerTools_AppendGListData(&data_00586da8.list, "<=", 2);
                break;
            case 0x16b:
                CompilerTools_AppendGListData(&data_00586da8.list, ">=", 2);
                break;
            case 0x16c:
                CompilerTools_AppendGListData(&data_00586da8.list, "<<", 2);
                break;
            case 0x16d:
                CompilerTools_AppendGListData(&data_00586da8.list, ">>", 2);
                break;
            case 0x16e:
                CompilerTools_AppendGListData(&data_00586da8.list, "++", 2);
                break;
            case 0x16f:
                CompilerTools_AppendGListData(&data_00586da8.list, "--", 2);
                break;
            case 0x170:
                CompilerTools_AppendGListData(&data_00586da8.list, "->", 2);
                break;
            case 0x171:
                CompilerTools_AppendGListData(&data_00586da8.list, "...", 3);
                break;
            case 0x172:
                CompilerTools_AppendGListData(&data_00586da8.list, ".*", 2);
                break;
            case 0x173:
                CompilerTools_AppendGListData(&data_00586da8.list, "->*", 3);
                break;
            case 0x174:
                CompilerTools_AppendGListData(&data_00586da8.list, "::", 2);
                break;
            case 0x175:
                CompilerTools_AppendGListData(&data_00586da8.list, "@interface", 10);
                break;
            case 0x176:
                CompilerTools_AppendGListData(&data_00586da8.list, "@implementation", 15);
                break;
            case 0x177:
                CompilerTools_AppendGListData(&data_00586da8.list, "@protocol", 9);
                break;
            case 0x178:
                CompilerTools_AppendGListData(&data_00586da8.list, "@end", 4);
                break;
            case 0x179:
                CompilerTools_AppendGListData(&data_00586da8.list, "@private", 8);
                break;
            case 0x17a:
                CompilerTools_AppendGListData(&data_00586da8.list, "@protected", 10);
                break;
            case 0x17b:
                CompilerTools_AppendGListData(&data_00586da8.list, "@public", 7);
                break;
            case 0x17c:
                CompilerTools_AppendGListData(&data_00586da8.list, "@class", 6);
                break;
            case 0x17d:
                CompilerTools_AppendGListData(&data_00586da8.list, "@selector", 9);
                break;
            case 0x17e:
                CompilerTools_AppendGListData(&data_00586da8.list, "@encode", 7);
                break;
            case 0x17f:
                CompilerTools_AppendGListData(&data_00586da8.list, "@defs", 5);
                break;
            case 0x180:
                CompilerTools_AppendGListData(&data_00586da8.list, "self", 4);
                break;
            case 0x181:
                CompilerTools_AppendGListData(&data_00586da8.list, "super", 5);
                break;
            case 0x11c:
                if (copts.cplusplus == '\0' && CPreprocess_UseC99Keywords())
                    CompilerTools_AppendGListData(&data_00586da8.list, "_Bool", 5);
                else
                    CompilerTools_AppendGListData(&data_00586da8.list, "bool", 4);
                break;
            case 0x184:
                if (CPreprocess_UseC99Keywords())
                    CompilerTools_AppendGListData(&data_00586da8.list, "restrict", 8);
                else
                    CompilerTools_AppendGListData(&data_00586da8.list, "__restrict", 10);
                break;
            case 0x11b:
                CompilerTools_AppendGListData(&data_00586da8.list, "__vector", 8);
                break;
            case 0x185:
                CompilerTools_AppendGListData(&data_00586da8.list, "__attribute__", 13);
                break;
            case 0x186:
                CompilerTools_AppendGListData(&data_00586da8.list, "__uuidof", 8);
                break;
            case 0x11e:
                CompilerTools_AppendGListData(&data_00586da8.list, "_Complex", 8);
                break;
            case 0x11f:
                CompilerTools_AppendGListData(&data_00586da8.list, "_Imaginary", 10);
                break;
            case -4:
                if (data_005882de != 0) {
                    CompilerTools_AppendGListData(&data_00586da8.list, "\"\\p", 3);
                    append_escaped_text(string_token_data + 1, token_value_kind_or_string_length - 1);
                } else {
                    AppendGListByte(&data_00586da8.list, '"');
                    append_escaped_text(string_token_data, token_value_kind_or_string_length - 1);
                }
                AppendGListByte(&data_00586da8.list, '"');
                break;
            case -5:
                CompilerTools_AppendGListData(&data_00586da8.list, "L\"", 2);
                output_escaped_wide_chars(string_token_data, token_value_kind_or_string_length / stwchar.size - 1);
                AppendGListByte(&data_00586da8.list, '"');
                break;
            default:
                if (' ' <= token && token <= 0xff)
                    AppendGListByte(&data_00586da8.list, token);
                else
                    CError_Internal("CPreprocess.c", 0x218);
                break;
        }
        CPrep_ResetBufferedTokenPosition();
        data_00588470 = 0;
        data_00588523 = '\0';
        token = CPrepTokenizer_GetNextToken();
    }
    CPreprocess_FinishOutput();
}
