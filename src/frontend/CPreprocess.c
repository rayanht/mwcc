#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CPreprocess.h"
#include "compiler/CError.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CodeGen.h"
#include "driver/COSToolsCLT.h"
#include <string.h>

#include <stdio.h>

union PreprocessedText data_00586da8;

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
        AppendGListData(&data_00586da8.list, "\r\n", 2);
}

void fn_004d6ed0(void)
{
    if (copts.line_prepdump != '\0' && data_00586da8.handle != NULL && current_file_index >= 0) {
        AppendGListData(&data_00586da8.list, "\r\n", 2);
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
            AppendGListName(&data_00586da8.list, "\r\n");

        if (CPreprocess_ShouldEmitLineDirectives())
            length = sprintf(buffer, "#line %ld\t\"", data_00587ef0);
        else
            length = sprintf(buffer, "/* #line %ld\t\"", data_00587ef0);
        AppendGListData(&data_00586da8.list, buffer, length);

        if (copts.fullpath_prepdump != 0) {
            COS_FileGetPathName(buffer, currentPFile, &fileValue);
            AppendGListData(&data_00586da8.list, buffer, strlen(buffer));
        } else {
            COS_FileGetFSSpecInfo(&currentPFile->textfile, &fileTag, &fileValue, &fileName.len);
            AppendGListData(&data_00586da8.list, fileName.name, fileName.len);
        }

        length = sprintf(buffer, "\"\t/* stack depth %ld */", current_file_index);
        AppendGListData(&data_00586da8.list, buffer, length);

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
                AppendGListData(&data_00586da8.list, "\r\n", 2);
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
                AppendGListData(&data_00586da8.list, tokenStart, currentTextPosition - tokenStart);
                break;
            case -3:
                AppendGListData(&data_00586da8.list, data_00587fa0->name, strlen(data_00587fa0->name));
                break;
            case 0x100:
                AppendGListData(&data_00586da8.list, "auto", 4);
                break;
            case 0x101:
                AppendGListData(&data_00586da8.list, "register", 8);
                break;
            case 0x102:
                AppendGListData(&data_00586da8.list, "static", 6);
                break;
            case 0x103:
                AppendGListData(&data_00586da8.list, "extern", 6);
                break;
            case 0x104:
                AppendGListData(&data_00586da8.list, "typedef", 7);
                break;
            case 0x105:
                AppendGListData(&data_00586da8.list, "inline", 6);
                break;
            case 0x106:
                AppendGListData(&data_00586da8.list, "void", 4);
                break;
            case 0x107:
                AppendGListData(&data_00586da8.list, "char", 4);
                break;
            case 0x108:
                AppendGListData(&data_00586da8.list, "short", 5);
                break;
            case 0x109:
                AppendGListData(&data_00586da8.list, "int", 3);
                break;
            case 0x10a:
                AppendGListData(&data_00586da8.list, "long", 4);
                break;
            case 0x10b:
                AppendGListData(&data_00586da8.list, "float", 5);
                break;
            case 0x10c:
                AppendGListData(&data_00586da8.list, "double", 6);
                break;
            case 0x10d:
                AppendGListData(&data_00586da8.list, "signed", 6);
                break;
            case 0x10e:
                AppendGListData(&data_00586da8.list, "unsigned", 8);
                break;
            case 0x10f:
                AppendGListData(&data_00586da8.list, "struct", 6);
                break;
            case 0x110:
                AppendGListData(&data_00586da8.list, "union", 5);
                break;
            case 0x111:
                AppendGListData(&data_00586da8.list, "enum", 4);
                break;
            case 0x112:
                AppendGListData(&data_00586da8.list, "class", 5);
                break;
            case 0x121:
                AppendGListData(&data_00586da8.list, "const", 5);
                break;
            case 0x122:
                AppendGListData(&data_00586da8.list, "volatile", 8);
                break;
            case 0x123:
                AppendGListData(&data_00586da8.list, "pascal", 6);
                break;
            case 0x129:
                AppendGListData(&data_00586da8.list, "__far", 5);
                break;
            case 0x12c:
                AppendGListData(&data_00586da8.list, "oneway", 6);
                break;
            case 0x12d:
                AppendGListData(&data_00586da8.list, "in", 2);
                break;
            case 0x12f:
                AppendGListData(&data_00586da8.list, "out", 3);
                break;
            case 0x12e:
                AppendGListData(&data_00586da8.list, "inout", 5);
                break;
            case 0x130:
                AppendGListData(&data_00586da8.list, "bycopy", 6);
                break;
            case 0x131:
                AppendGListData(&data_00586da8.list, "byref", 5);
                break;
            case 0x136:
                AppendGListData(&data_00586da8.list, "asm", 3);
                break;
            case 0x137:
                AppendGListData(&data_00586da8.list, "case", 4);
                break;
            case 0x138:
                AppendGListData(&data_00586da8.list, "default", 7);
                break;
            case 0x139:
                AppendGListData(&data_00586da8.list, "if", 2);
                break;
            case 0x13a:
                AppendGListData(&data_00586da8.list, "else", 4);
                break;
            case 0x13b:
                AppendGListData(&data_00586da8.list, "switch", 6);
                break;
            case 0x13c:
                AppendGListData(&data_00586da8.list, "while", 5);
                break;
            case 0x13d:
                AppendGListData(&data_00586da8.list, "do", 2);
                break;
            case 0x13e:
                AppendGListData(&data_00586da8.list, "for", 3);
                break;
            case 0x13f:
                AppendGListData(&data_00586da8.list, "goto", 4);
                break;
            case 0x140:
                AppendGListData(&data_00586da8.list, "continue", 8);
                break;
            case 0x141:
                AppendGListData(&data_00586da8.list, "break", 5);
                break;
            case 0x142:
                AppendGListData(&data_00586da8.list, "return", 6);
                break;
            case 0x143:
                AppendGListData(&data_00586da8.list, "sizeof", 6);
                break;
            case 0x144:
                AppendGListData(&data_00586da8.list, "catch", 5);
                break;
            case 0x145:
                AppendGListData(&data_00586da8.list, "delete", 6);
                break;
            case 0x146:
                AppendGListData(&data_00586da8.list, "friend", 6);
                break;
            case 0x147:
                AppendGListData(&data_00586da8.list, "new", 3);
                break;
            case 0x148:
                AppendGListData(&data_00586da8.list, "operator", 8);
                break;
            case 0x149:
                AppendGListData(&data_00586da8.list, "private", 7);
                break;
            case 0x14a:
                AppendGListData(&data_00586da8.list, "protected", 9);
                break;
            case 0x14b:
                AppendGListData(&data_00586da8.list, "public", 6);
                break;
            case 0x14c:
                AppendGListData(&data_00586da8.list, "template", 8);
                break;
            case 0x14d:
                AppendGListData(&data_00586da8.list, "this", 4);
                break;
            case 0x14e:
                AppendGListData(&data_00586da8.list, "throw", 5);
                break;
            case 0x14f:
                AppendGListData(&data_00586da8.list, "try", 3);
                break;
            case 0x150:
                AppendGListData(&data_00586da8.list, "virtual", 7);
                break;
            case 0x151:
                AppendGListData(&data_00586da8.list, "inherited", 9);
                break;
            case 0x152:
                AppendGListData(&data_00586da8.list, "const_cast", 10);
                break;
            case 0x153:
                AppendGListData(&data_00586da8.list, "dynamic_cast", 12);
                break;
            case 0x12a:
                AppendGListData(&data_00586da8.list, "explicit", 8);
                break;
            case 0x12b:
                AppendGListData(&data_00586da8.list, "mutable", 7);
                break;
            case 0x154:
                AppendGListData(&data_00586da8.list, "namespace", 9);
                break;
            case 0x155:
                AppendGListData(&data_00586da8.list, "reinterpret_cast", 16);
                break;
            case 0x156:
                AppendGListData(&data_00586da8.list, "static_cast", 11);
                break;
            case 0x157:
                AppendGListData(&data_00586da8.list, "using", 5);
                break;
            case 0x11d:
                AppendGListData(&data_00586da8.list, "wchar_t", 7);
                break;
            case 0x120:
                AppendGListData(&data_00586da8.list, "typename", 8);
                break;
            case 0x158:
                AppendGListData(&data_00586da8.list, "true", 4);
                break;
            case 0x159:
                AppendGListData(&data_00586da8.list, "false", 5);
                break;
            case 0x15a:
                AppendGListData(&data_00586da8.list, "typeid", 6);
                break;
            case 0x15b:
                AppendGListData(&data_00586da8.list, "export", 6);
                break;
            case 0x125:
                AppendGListData(&data_00586da8.list, "__stdcall", 9);
                break;
            case 0x126:
                AppendGListData(&data_00586da8.list, "__cdecl", 7);
                break;
            case 0x127:
                AppendGListData(&data_00586da8.list, "__fastcall", 10);
                break;
            case 0x128:
                AppendGListData(&data_00586da8.list, "__floatcall", 11);
                break;
            case 0x124:
                AppendGListData(&data_00586da8.list, "__declspec", 10);
                break;
            case 0x15c:
                AppendGListData(&data_00586da8.list, "*=", 2);
                break;
            case 0x15d:
                AppendGListData(&data_00586da8.list, "/=", 2);
                break;
            case 0x15e:
                AppendGListData(&data_00586da8.list, "%=", 2);
                break;
            case 0x15f:
                AppendGListData(&data_00586da8.list, "+=", 2);
                break;
            case 0x160:
                AppendGListData(&data_00586da8.list, "-=", 2);
                break;
            case 0x161:
                AppendGListData(&data_00586da8.list, "<<=", 3);
                break;
            case 0x162:
                AppendGListData(&data_00586da8.list, ">>=", 3);
                break;
            case 0x163:
                AppendGListData(&data_00586da8.list, "&=", 2);
                break;
            case 0x164:
                AppendGListData(&data_00586da8.list, "^=", 2);
                break;
            case 0x165:
                AppendGListData(&data_00586da8.list, "|=", 2);
                break;
            case 0x166:
                AppendGListData(&data_00586da8.list, "||", 2);
                break;
            case 0x167:
                AppendGListData(&data_00586da8.list, "&&", 2);
                break;
            case 0x168:
                AppendGListData(&data_00586da8.list, "==", 2);
                break;
            case 0x169:
                AppendGListData(&data_00586da8.list, "!=", 2);
                break;
            case 0x16a:
                AppendGListData(&data_00586da8.list, "<=", 2);
                break;
            case 0x16b:
                AppendGListData(&data_00586da8.list, ">=", 2);
                break;
            case 0x16c:
                AppendGListData(&data_00586da8.list, "<<", 2);
                break;
            case 0x16d:
                AppendGListData(&data_00586da8.list, ">>", 2);
                break;
            case 0x16e:
                AppendGListData(&data_00586da8.list, "++", 2);
                break;
            case 0x16f:
                AppendGListData(&data_00586da8.list, "--", 2);
                break;
            case 0x170:
                AppendGListData(&data_00586da8.list, "->", 2);
                break;
            case 0x171:
                AppendGListData(&data_00586da8.list, "...", 3);
                break;
            case 0x172:
                AppendGListData(&data_00586da8.list, ".*", 2);
                break;
            case 0x173:
                AppendGListData(&data_00586da8.list, "->*", 3);
                break;
            case 0x174:
                AppendGListData(&data_00586da8.list, "::", 2);
                break;
            case 0x175:
                AppendGListData(&data_00586da8.list, "@interface", 10);
                break;
            case 0x176:
                AppendGListData(&data_00586da8.list, "@implementation", 15);
                break;
            case 0x177:
                AppendGListData(&data_00586da8.list, "@protocol", 9);
                break;
            case 0x178:
                AppendGListData(&data_00586da8.list, "@end", 4);
                break;
            case 0x179:
                AppendGListData(&data_00586da8.list, "@private", 8);
                break;
            case 0x17a:
                AppendGListData(&data_00586da8.list, "@protected", 10);
                break;
            case 0x17b:
                AppendGListData(&data_00586da8.list, "@public", 7);
                break;
            case 0x17c:
                AppendGListData(&data_00586da8.list, "@class", 6);
                break;
            case 0x17d:
                AppendGListData(&data_00586da8.list, "@selector", 9);
                break;
            case 0x17e:
                AppendGListData(&data_00586da8.list, "@encode", 7);
                break;
            case 0x17f:
                AppendGListData(&data_00586da8.list, "@defs", 5);
                break;
            case 0x180:
                AppendGListData(&data_00586da8.list, "self", 4);
                break;
            case 0x181:
                AppendGListData(&data_00586da8.list, "super", 5);
                break;
            case 0x11c:
                if (copts.cplusplus == '\0' && CPreprocess_UseC99Keywords())
                    AppendGListData(&data_00586da8.list, "_Bool", 5);
                else
                    AppendGListData(&data_00586da8.list, "bool", 4);
                break;
            case 0x184:
                if (CPreprocess_UseC99Keywords())
                    AppendGListData(&data_00586da8.list, "restrict", 8);
                else
                    AppendGListData(&data_00586da8.list, "__restrict", 10);
                break;
            case 0x11b:
                AppendGListData(&data_00586da8.list, "__vector", 8);
                break;
            case 0x185:
                AppendGListData(&data_00586da8.list, "__attribute__", 13);
                break;
            case 0x186:
                AppendGListData(&data_00586da8.list, "__uuidof", 8);
                break;
            case 0x11e:
                AppendGListData(&data_00586da8.list, "_Complex", 8);
                break;
            case 0x11f:
                AppendGListData(&data_00586da8.list, "_Imaginary", 10);
                break;
            case -4:
                if (data_005882de != 0) {
                    AppendGListData(&data_00586da8.list, "\"\\p", 3);
                    append_escaped_text(string_token_data + 1, token_value_kind_or_string_length - 1);
                } else {
                    AppendGListByte(&data_00586da8.list, '"');
                    append_escaped_text(string_token_data, token_value_kind_or_string_length - 1);
                }
                AppendGListByte(&data_00586da8.list, '"');
                break;
            case -5:
                AppendGListData(&data_00586da8.list, "L\"", 2);
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
