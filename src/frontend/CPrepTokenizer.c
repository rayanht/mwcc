#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CParser.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroCSE.h"
#include "compiler/IroLoop.h"
#include "compiler/IroTransform.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/COSToolsCLT.h"
#include "driver/Files.h"
#include "driver/Memory.h"
/* Buffered lexical item and its associated value. */
#include <string.h>

static UInt8 data_00580dd0;

typedef int (*TokenScanner)(short);

/* The scanner of each character a token starts with. */
static TokenScanner data_0055da98[256] = {
    (TokenScanner)mark_previous_text_character,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)check_illegal_token,
    (TokenScanner)check_illegal_token,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)fn_00495640,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)check_null_terminator,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)scan_not_or_logical_ne,
    (TokenScanner)scan_string_literal,
    (TokenScanner)parse_directive_or_return_hash,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)tokenize_percent,
    (TokenScanner)tokenize_and,
    (TokenScanner)scan_character_constant,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_star_or_mult_assign,
    (TokenScanner)scan_plus_token,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_minus_token,
    (TokenScanner)scan_dot_token,
    (TokenScanner)scan_div_assign,
    (TokenScanner)parse_zero_prefixed_number,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)scan_numeric_literal,
    (TokenScanner)tokenize_colon,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_less_token,
    (TokenScanner)tokenize_equals,
    (TokenScanner)scan_greater_token,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_at_token,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_quoted_literal,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_xor_operator,
    (TokenScanner)classify_identifier_token,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_identifier_or_keyword_a,
    (TokenScanner)scan_b_keyword,
    (TokenScanner)fn_00493ea0,
    (TokenScanner)scan_d_keyword,
    (TokenScanner)recognize_e_keyword,
    (TokenScanner)classify_f_keyword,
    (TokenScanner)expand_macro_or_classify_identifier,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_i_keyword,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)classify_identifier_name_token,
    (TokenScanner)classify_mutable_or_identifier,
    (TokenScanner)scan_n_keyword,
    (TokenScanner)scan_o_keyword,
    (TokenScanner)scan_identifier,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_r_keyword,
    (TokenScanner)tokenize_s_keyword,
    (TokenScanner)scan_t_keyword,
    (TokenScanner)classify_u_keyword,
    (TokenScanner)tokenize_v_keyword,
    (TokenScanner)classify_w_keyword,
    (TokenScanner)classify_identifier_or_xor_token,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)scan_identifier_and_expand_macro,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)scan_bitor,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)clear_global_and_return_argument,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
    (TokenScanner)return_unsigned_minus_six,
};
#define KW(s, n) (memcmp(s, (char *)(data_00587fa0->name), n) == 0)
SInt16 CPrepTokenizer_GetToken(void)
{
    SInt16 c;
    UInt8 *save;
    c = CPrepTokenizer_ScanChar();
    currentTextPosition = (UInt8 *)lookahead_position;
    data_00588524 = 0;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
        return TK_IDENTIFIER;
    }
    if (c == '.') {
        save = currentTextPosition;
        if (CPrepTokenizer_NextChar() == '.' && CPrepTokenizer_NextChar() == '.')
            return TK_ELLIPSIS;
        currentTextPosition = save;
    }
    return c;
}

SInt16 CPrepTokenizer_ScanToken(void)
{
    SInt16 token;
    SInt16 result;

    for (;;) {
        do {
            token = CPrepTokenizer_ScanChar();
            currentTextPosition = (unsigned char *)lookahead_position;
            result = data_0055da98[token](token);
            if (result != 0)
                return result;
        } while (*currentTextPosition != '\0');

        if (macro_expansion_depth > 0) {
            CPrep_PopMacro();
        } else if (current_file_index > 0) {
            CPrep_PopFile();
        } else {
            return 0;
        }
    }
}

static inline struct TStreamElement *CurrentBufferedToken(void)
{
    return bufferedTokenPosition;
}

static inline Float *BufferedTokenFloat(struct TStreamElement *token)
{
    return &token->data.tkfloatconst;
}

short CPrepTokenizer_GetNextToken(void)
{
    short token;
    short dispatch_index;
    short scanned_token;
    int source_offset;

    for (;;) {
        if (remainingBufferedTokenCount <= 0)
            break;
        if ((token = CurrentBufferedToken()->tokentype) < 0) {
            switch (token) {
                case -1:
                    token_integer = CurrentBufferedToken()->data.tkintconst;
                    token_value_kind_or_string_length = CurrentBufferedToken()->subtype;
                    break;
                case -2:
                    token_float = CurrentBufferedToken()->data.tkfloatconst;
                    token_value_kind_or_string_length = CurrentBufferedToken()->subtype;
                    break;
                case -3:
                    data_00587fa0 = CurrentBufferedToken()->data.tkidentifier;
                    break;
                case -4:
                case -5:
                    string_token_data = CurrentBufferedToken()->data.tkstring.data;
                    token_value_kind_or_string_length = CurrentBufferedToken()->data.tkstring.size;
                    DAT_005882de = CurrentBufferedToken()->subtype;
                    break;
                case -7:
                    data_00588470 = 1;
                    if (data_0058850d == 0) {
                        bufferedTokenPosition++;
                        remainingBufferedTokenCount -= 1;
                        continue;
                    }
                    break;
                default:
                    CError_Internal("CPrepTokenizer.c", 2504);
            }
        }
        bufferedTokenPosition++;
        remainingBufferedTokenCount -= 1;
        return token;
    }

    for (;;) {
        dispatch_index = CPrepTokenizer_ScanChar();
        CurrentBufferedToken()->tokenfile = currentPFile;
        if (macro_expansion_depth > 0) {
            source_offset = macro_stack[0].pos - PTR_00587fb0;
            bufferedTokenPosition->tokenoffset = source_offset;
            token_start = currentTextPosition;
        } else {
            source_offset = (char *)currentTextPosition - PTR_00587fb0;
            bufferedTokenPosition->tokenoffset = source_offset;
        }
        CurrentBufferedToken()->tokenline = DAT_00587ef0;
        currentTextPosition = (UInt8 *)lookahead_position;
        scanned_token = data_0055da98[dispatch_index](dispatch_index);
        do {
            if (scanned_token != 0) {
                if (bufferedTokenPosition >= buffered_token_buffer_end)
                    CPrep_GrowBufferedTokenBuffer(1024);
                CurrentBufferedToken()->tokentype = scanned_token;
                if ((token = CurrentBufferedToken()->tokentype) < 0) {
                    switch (token) {
                        case -1:
                            CurrentBufferedToken()->data.tkintconst = token_integer;
                            CurrentBufferedToken()->subtype = token_value_kind_or_string_length;
                            break;
                        case -2:
                            CurrentBufferedToken()->data.tkfloatconst = token_float;
                            CurrentBufferedToken()->subtype = token_value_kind_or_string_length;
                            break;
                        case -3:
                            CurrentBufferedToken()->data.tkidentifier = data_00587fa0;
                            break;
                        case -4:
                        case -5:
                            CurrentBufferedToken()->data.tkstring.data = string_token_data;
                            CurrentBufferedToken()->data.tkstring.size = token_value_kind_or_string_length;
                            CurrentBufferedToken()->subtype = DAT_005882de;
                            if (concatenating_string_tokens == 0) {
                                bufferedTokenPosition++;
                                concatenate_string_tokens(token == -5);
                                return token;
                            }
                            break;
                        case -6:
                            bufferedTokenPosition++;
                            CError_ReportError(ERR_ILLEGAL_TOKEN);
                            bufferedTokenPosition--;
                            continue;
                        default:
                            break;
                    }
                }
                bufferedTokenPosition++;
                return token;
            }
        } while (0);
        if (*currentTextPosition != 0)
            continue;
        if (macro_expansion_depth > 0) {
            CPrep_PopMacro();
            continue;
        }
        if (macro_expansion_depth > 0 || currentTextPosition >= textend) {
            if (current_file_index > 0) {
                CPrep_PopFile();
                continue;
            }
            return 0;
        }
        bufferedTokenPosition++;
        CError_FatalError(ERR_ILLEGAL_TOKEN);
    }
}

void concatenate_string_tokens(char strip_terminator)
{
    SInt16 token;
    NamePiece *pieces;
    NamePiece *piece;
    NamePiece *cursor;
    struct TStreamElement *record;
    char *buffer;
    char *original_buffer;
    SInt32 original_length;
    UInt8 original_flag;
    SInt32 lexer_state;
    SInt32 combined_length;

    original_buffer = string_token_data;
    original_length = token_value_kind_or_string_length;
    original_flag = DAT_005882de;
    concatenating_string_tokens = 1;
    pieces = NULL;
    combined_length = token_value_kind_or_string_length;

    for (;;) {
        CPrep_GetBufferedTokenPosition(&lexer_state);
        do {
            token = CPrepTokenizer_GetNextToken();
        } while (token == -7);
        CPrep_SetBufferedTokenPosition(&lexer_state);

        if (token != -4 && token != -5) {
            DAT_005882de = original_flag;
            if (pieces != NULL) {
                buffer = galloc(token_value_kind_or_string_length = combined_length);
                string_token_data = buffer;
                memcpy(buffer, original_buffer, original_length);
                for (cursor = pieces; cursor != NULL; cursor = cursor->next) {
                    memcpy(buffer + cursor->off, cursor->src, cursor->len);
                }
                if (original_flag != 0) {
                    if (combined_length > 0x100) {
                        CError_ReportError(ERR_STRING_TOO_LONG);
                        combined_length = 0x100;
                    }
                    buffer[0] = (char)(combined_length - 1);
                } else {
                    buffer[combined_length - 1] = 0;
                    if (strip_terminator != 0) {
                        buffer[combined_length - 2] = 0;
                    }
                }
                record = bufferedTokenPosition - 1;
                record->data.tkstring.data = buffer;
                record->data.tkstring.size = combined_length;
                record->subtype = DAT_005882de;
                concatenating_string_tokens = 0;
                return;
            }
            string_token_data = original_buffer;
            token_value_kind_or_string_length = original_length;
            concatenating_string_tokens = 0;
            return;
        }

        do {
            token = CPrepTokenizer_GetNextToken();
            bufferedTokenPosition--;
            memmove(bufferedTokenPosition, bufferedTokenPosition + 1,
                    remainingBufferedTokenCount * sizeof(struct TStreamElement));
        } while (token == -7);

        piece = (NamePiece *)CompilerTools_AllocatePool(sizeof(NamePiece));
        piece->next = pieces;
        piece->src = string_token_data;
        piece->len = token_value_kind_or_string_length - 1;
        piece->off = combined_length - 1;
        pieces = piece;
        if (DAT_005882de != 0) {
            piece->src++;
        }
        if (original_flag != 0) {
            piece->off++;
        }
        if (token == -5) {
            piece->len--;
        }
        if (strip_terminator != 0) {
            piece->off--;
        }
        combined_length += piece->len;
    }
}

short CPrepTokenizer_PeekNextToken(void)
{
    short result;
    SInt32 saved;
    CPrep_GetBufferedTokenPosition(&saved);
    do {
        result = CPrepTokenizer_GetNextToken();
    } while (result == -7);
    CPrep_SetBufferedTokenPosition(&saved);
    return result;
}

short CPrepTokenizer_GetNextTokenAndRestorePosition(void)
{
    short result;
    SInt32 saved;

    CPrep_GetBufferedTokenPosition(&saved);
    result = CPrepTokenizer_GetNextToken();
    CPrep_SetBufferedTokenPosition(&saved);
    return result;
}

int check_null_terminator(void)
{
    if (*currentTextPosition == '\0') {
        return 0;
    }
    return -6;
}

unsigned int mark_previous_text_character(unsigned int result)
{
    if (macro_expansion_depth > 0 || currentTextPosition >= textend) {
        data_00588524 = 0;
        return result;
    }
    currentTextPosition[-1] = 0xc0;
    return -6;
}

int check_illegal_token(void)
{
    if (macro_expansion_depth == 0) {
        CError_ReportError(ERR_ILLEGAL_TOKEN);
    }
    return 0;
}

unsigned int clear_global_and_return_argument(unsigned int argument)
{
    data_00588524 = 0U;
    return argument;
}

int classify_identifier_token(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if ((macrocheck != '\0') && (CPrep_ExpandMacro() != '\0')) {
        return 0;
    }
    if (memcmp("__stdcall", data_00587fa0->name, 10) == 0) {
        return TK_UU_STDCALL;
    }
    if (memcmp("__cdecl", data_00587fa0->name, 8) == 0) {
        return TK_UU_CDECL;
    }
    if (memcmp("__fastcall", data_00587fa0->name, 11) == 0) {
        return TK_UU_FASTCALL;
    }
    if (memcmp("__floatcall", data_00587fa0->name, 12) == 0) {
        return TK_UU_FLOATCALL;
    }
    if (memcmp("__declspec", data_00587fa0->name, 11) == 0) {
        return TK_UU_DECLSPEC;
    }
    if (memcmp("__asm", data_00587fa0->name, 6) == 0) {
        return TK_ASM;
    }
    if (memcmp("__asm__", data_00587fa0->name, 8) == 0) {
        return TK_ASM;
    }
    if (memcmp("__inline", data_00587fa0->name, 9) == 0) {
        return TK_INLINE;
    }
    if (memcmp("__inline__", data_00587fa0->name, 11) == 0) {
        return TK_INLINE;
    }
    if (memcmp("__restrict", data_00587fa0->name, 11) == 0) {
        return TK_RESTRICT;
    }
    if (memcmp("__attribute__", data_00587fa0->name, 14) == 0) {
        return TK_UU_ATTRIBUTE;
    }
    if (memcmp("__far", data_00587fa0->name, 6) == 0) {
        return TK_UU_FAR;
    }
    if (copts.f90 != '\0') {
        if (memcmp("_Bool", data_00587fa0->name, 6) == 0) {
            return TK_BOOL;
        }
        if (memcmp("_Complex", data_00587fa0->name, 9) == 0) {
            return TK__COMPLEX;
        }
        if (memcmp("_Imaginary", data_00587fa0->name, 11) == 0) {
            return TK__IMAGINARY;
        }
    }
    if (copts.altivecModel != '\0') {
        if (memcmp("__vector", data_00587fa0->name, 9) == 0) {
            return TK_UU_VECTOR;
        }
    }
    return TK_IDENTIFIER;
}

int classify_identifier_or_xor_token(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if ((macrocheck != '\0') && (CPrep_ExpandMacro() != '\0')) {
        return 0;
    }
    if (((copts.cplusplus != '\0') && (data_005884fd == '\0')) && (DAT_0058850f == '\0')) {
        if (memcmp("xor", data_00587fa0->name, 4) == 0) {
            return '^';
        }
        if (memcmp("xor_eq", data_00587fa0->name, 7) == 0) {
            return TK_XOR_ASSIGN;
        }
    }
    return TK_IDENTIFIER;
}

static int cmpw(const void *a, const void *b, unsigned int n)
{
    return memcmp(a, b, n);
}

int classify_w_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if ((macrocheck != '\0') && (CPrep_ExpandMacro() != '\0')) {
        return 0;
    }
    if (cmpw("while", data_00587fa0->name, 6) == 0) {
        return TK_WHILE;
    }
    if ((copts.cplusplus != '\0') && (copts.f7f != '\0')) {
        if (cmpw("wchar_t", data_00587fa0->name, 8) == 0) {
            return TK_WCHAR_T;
        }
    }
    return TK_IDENTIFIER;
}

unsigned int tokenize_v_keyword(void)
{
    const char *text;

    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if ((macrocheck != 0) && (CPrep_ExpandMacro() != 0)) {
        return 0;
    }
    text = data_00587fa0->name;
    if (memcmp("void", text, 5) == 0) {
        return TK_VOID;
    }
    text = data_00587fa0->name;
    if (memcmp("volatile", text, 9) == 0) {
        return TK_VOLATILE;
    }
    if (copts.cplusplus != 0) {
        text = data_00587fa0->name;
        if (memcmp("virtual", text, 8) == 0) {
            return TK_VIRTUAL;
        }
    }
    return TK_IDENTIFIER;
}

int classify_u_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0) {
        return 0;
    }
    if (memcmp("union", data_00587fa0->name, 6) == 0) {
        return TK_UNION;
    }
    if (memcmp("unsigned", data_00587fa0->name, 9) == 0) {
        return TK_UNSIGNED;
    }
    if (copts.cplusplus != 0) {
        if (memcmp("using", data_00587fa0->name, 6) == 0) {
            if (copts.f5b)
                fn_0043f3b0(0x153);
            return TK_USING;
        }
    }
    return TK_IDENTIFIER;
}

static inline void check_extended_cpp_keyword(void)
{
    if (copts.f5b)
        fn_0043f3b0(0x153);
}

SInt32 scan_t_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro())
        return 0;
    if (KW("typedef", 8))
        return TK_TYPEDEF;
    if (copts.cplusplus != 0) {
        if (KW("this", 5))
            return TK_THIS;
        if (copts.f75 != 0 && KW("true", 5))
            return TK_TRUE;
        if (KW("template", 9)) {
            check_extended_cpp_keyword();
            return TK_TEMPLATE;
        }
        if (KW("try", 4)) {
            check_extended_cpp_keyword();
            return TK_TRY;
        }
        if (KW("throw", 6)) {
            check_extended_cpp_keyword();
            return TK_THROW;
        }
        if (KW("typeid", 7)) {
            check_extended_cpp_keyword();
            return TK_TYPEID;
        }
        if (KW("typename", 9)) {
            check_extended_cpp_keyword();
            return TK_TYPENAME;
        }
    }
    return TK_IDENTIFIER;
}

unsigned int tokenize_s_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck && CPrep_ExpandMacro()) {
        return 0;
    }
    if (memcmp("short", data_00587fa0->name, 6) == 0) {
        return TK_SHORT;
    }
    if (memcmp("signed", data_00587fa0->name, 7) == 0) {
        return TK_SIGNED;
    }
    if (memcmp("sizeof", data_00587fa0->name, 7) == 0) {
        return TK_SIZEOF;
    }
    if (memcmp("static", data_00587fa0->name, 7) == 0) {
        return TK_STATIC;
    }
    if (memcmp("struct", data_00587fa0->name, 7) == 0) {
        return TK_STRUCT;
    }
    if (memcmp("switch", data_00587fa0->name, 7) == 0) {
        return TK_SWITCH;
    }
    if (copts.cplusplus) {
        if (memcmp("static_cast", data_00587fa0->name, 12) == 0) {
            if (copts.f5b) {
                fn_0043f3b0(0x153);
            }
            return TK_STATIC_CAST;
        }
    }
    return TK_IDENTIFIER;
}

SInt32 scan_r_keyword(void)
{
    UInt8 *cursor;
    data_00588524 = 0;
    cursor = currentTextPosition - 1;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(cursor);
    if (macrocheck != 0 && CPrep_ExpandMacro())
        return 0;
    if (KW("register", 9))
        return TK_REGISTER;
    if (KW("return", 7))
        return TK_RETURN;
    if (copts.cplusplus != 0) {
        if (KW("reinterpret_cast", 17)) {
            if (copts.f5b)
                fn_0043f3b0(0x153);
            return TK_REINTERPRET_CAST;
        }
    }
    if (copts.f90 != 0) {
        if (KW("restrict", 9))
            return TK_RESTRICT;
    }
    return TK_IDENTIFIER;
}

SInt32 scan_identifier(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0)
        return 0;
    if (copts.f62 == 0) {
        if (memcmp("pascal", data_00587fa0->name, 7) == 0)
            return TK_PASCAL;
    }
    if (copts.cplusplus != 0) {
        if (memcmp("private", data_00587fa0->name, 8) == 0)
            return TK_PRIVATE;
        if (memcmp("protected", data_00587fa0->name, 10) == 0)
            return TK_PROTECTED;
        if (memcmp("public", data_00587fa0->name, 7) == 0)
            return TK_PUBLIC;
    }
    return TK_IDENTIFIER;
}

SInt32 scan_o_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro())
        return 0;
    if (copts.cplusplus != 0) {
        if (memcmp("operator", data_00587fa0->name, 9) == 0)
            return TK_OPERATOR;
        if (data_005884fd == 0 && DAT_0058850f == 0) {
            if (memcmp("or", data_00587fa0->name, 3) == 0)
                return TK_LOGICAL_OR;
            if (memcmp("or_eq", data_00587fa0->name, 6) == 0)
                return TK_OR_ASSIGN;
        }
    }
    if (copts.f5c != 0) {
        if (memcmp("out", data_00587fa0->name, 4) == 0)
            return TK_OUT;
        if (memcmp("oneway", data_00587fa0->name, 7) == 0)
            return TK_ONEWAY;
    }
    return TK_IDENTIFIER;
}

SInt32 scan_n_keyword(void)
{
    UInt8 *cursor;
    data_00588524 = 0;
    cursor = currentTextPosition - 1;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(cursor);
    if (macrocheck != 0 && CPrep_ExpandMacro())
        return 0;
    if (copts.cplusplus != 0) {
        if (KW("new", 4))
            return TK_NEW;
        if (KW("namespace", 10)) {
            if (copts.f5b)
                fn_0043f3b0(0x153);
            return TK_NAMESPACE;
        }
        if (data_005884fd == 0 && DAT_0058850f == 0) {
            if (KW("not", 4))
                return TK_NOT;
            if (KW("not_eq", 7))
                return TK_LOGICAL_NE;
        }
    }
    return TK_IDENTIFIER;
}

int classify_mutable_or_identifier(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != '\0' && CPrep_ExpandMacro() != '\0') {
        return 0;
    }
    if (copts.cplusplus != '\0') {
        if (memcmp("mutable", data_00587fa0->name, 8) == 0) {
            if (copts.f5b != '\0') {
                fn_0043f3b0(0x153);
            }
            return TK_MUTABLE;
        }
    }
    return TK_IDENTIFIER;
}

int classify_identifier_name_token(void)
{
    const char *comparisonData;
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if ((macrocheck != '\0') && (CPrep_ExpandMacro() != '\0')) {
        return 0;
    }
    comparisonData = data_00587fa0->name;
    if (memcmp("long", comparisonData, 5) == 0) {
        return 0x10a;
    }
    return TK_IDENTIFIER;
}

SInt32 scan_i_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro())
        return 0;
    if (KW("if", 3))
        return TK_IF;
    if (KW("int", 4))
        return TK_INT;
    if (copts.cplusplus || copts.f90) {
        if (KW("inline", 7))
            return TK_INLINE;
    } else if (!copts.f62) {
        if (KW("inline", 7))
            return TK_INLINE;
    }
    if (copts.f5c != 0) {
        if (KW("in", 3))
            return TK_IN;
        if (KW("inout", 6))
            return TK_INOUT;
    }
    return TK_IDENTIFIER;
}

int expand_macro_or_classify_identifier(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if ((macrocheck != '\0') && (CPrep_ExpandMacro() != '\0')) {
        return 0;
    }
    if (memcmp("goto", data_00587fa0->name, 5) == 0) {
        return 0x13f;
    }
    return TK_IDENTIFIER;
}

unsigned int classify_f_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0)
        return 0;
    if (memcmp("float", data_00587fa0->name, 6) == 0)
        return TK_FLOAT;
    if (memcmp("for", data_00587fa0->name, 4) == 0)
        return TK_FOR;
    if (copts.cplusplus != 0) {
        if (memcmp("friend", data_00587fa0->name, 7) == 0)
            return TK_FRIEND;
        if (copts.f75 != 0) {
            if (memcmp("false", data_00587fa0->name, 6) == 0)
                return TK_FALSE;
        }
    }
    return TK_IDENTIFIER;
}

int recognize_e_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0)
        return 0;
    if (memcmp("else", data_00587fa0->name, 5) == 0)
        return TK_ELSE;
    if (memcmp("enum", data_00587fa0->name, 5) == 0)
        return TK_ENUM;
    if (memcmp("extern", data_00587fa0->name, 7) == 0)
        return TK_EXTERN;
    if (copts.cplusplus != 0) {
        if (memcmp("explicit", data_00587fa0->name, 9) == 0)
            return TK_EXPLICIT;
        if (memcmp("export", data_00587fa0->name, 7) == 0 && copts.f5b == 0)
            return TK_EXPORT;
    }
    return TK_IDENTIFIER;
}

int scan_d_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != '\0' && CPrep_ExpandMacro() != '\0') {
        return 0;
    }
    if (memcmp("default", data_00587fa0->name, 8) == 0) {
        return TK_DEFAULT;
    }
    if (memcmp("do", data_00587fa0->name, 3) == 0) {
        return 0x13d;
    }
    if (memcmp("double", data_00587fa0->name, 7) == 0) {
        return TK_DOUBLE;
    }
    if (copts.cplusplus != '\0') {
        if (memcmp("delete", data_00587fa0->name, 7) == 0) {
            return TK_DELETE;
        }
        if (memcmp("dynamic_cast", data_00587fa0->name, 13) == 0) {
            if (copts.f5b != '\0') {
                fn_0043f3b0(0x153);
            }
            return TK_DYNAMIC_CAST;
        }
    }
    return TK_IDENTIFIER;
}

static inline void CPrepTokenizer_CheckCppExtension(void)
{
    if (copts.f5b)
        fn_0043f3b0(0x153);
}

SInt32 fn_00493ea0(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck && CPrep_ExpandMacro())
        return 0;
    if (memcmp("case", data_00587fa0->name, sizeof("case")) == 0)
        return TK_CASE;
    if (memcmp("char", data_00587fa0->name, sizeof("char")) == 0)
        return TK_CHAR;
    if (memcmp("const", data_00587fa0->name, sizeof("const")) == 0)
        return TK_CONST;
    if (memcmp("continue", data_00587fa0->name, sizeof("continue")) == 0)
        return TK_CONTINUE;
    if (copts.cplusplus) {
        if (memcmp("const_cast", data_00587fa0->name, sizeof("const_cast")) == 0) {
            CPrepTokenizer_CheckCppExtension();
            return TK_CONST_CAST;
        }
        if (memcmp("catch", data_00587fa0->name, sizeof("catch")) == 0) {
            CPrepTokenizer_CheckCppExtension();
            return TK_CATCH;
        }
        if (memcmp("class", data_00587fa0->name, sizeof("class")) == 0)
            return TK_CLASS;
        if (!data_005884fd && !DAT_0058850f) {
            if (memcmp("compl", data_00587fa0->name, sizeof("compl")) == 0)
                return TK_COMPL;
        }
    }
    return TK_IDENTIFIER;
}

SInt32 scan_b_keyword(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro())
        return 0;
    if (memcmp("break", data_00587fa0->name, 6) == 0)
        return TK_BREAK;
    if (copts.cplusplus != 0) {
        if (copts.f75 != 0) {
            if (memcmp("bool", data_00587fa0->name, 5) == 0)
                return TK_BOOL;
        }
        if (data_005884fd == 0 && DAT_0058850f == 0) {
            if (memcmp("bitand", data_00587fa0->name, 7) == 0)
                return TK_BITAND;
            if (memcmp("bitor", data_00587fa0->name, 6) == 0)
                return TK_BITOR;
        }
    }
    if (copts.f5c != 0) {
        if (memcmp("bycopy", data_00587fa0->name, 7) == 0)
            return TK_BYCOPY;
        if (memcmp("byref", data_00587fa0->name, 6) == 0)
            return TK_BYREF;
    }
    return TK_IDENTIFIER;
}

int scan_identifier_or_keyword_a(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0)
        return 0;
    if (memcmp("auto", data_00587fa0->name, 5) == 0)
        return TK_AUTO;
    if (copts.cplusplus != 0 || copts.f62 == 0) {
        if (memcmp("asm", data_00587fa0->name, 4) == 0)
            return TK_ASM;
    }
    if (copts.cplusplus != 0 && data_005884fd == 0 && DAT_0058850f == 0) {
        if (memcmp("and", data_00587fa0->name, 4) == 0)
            return TK_LOGICAL_AND;
        if (memcmp("and_eq", data_00587fa0->name, 7) == 0)
            return TK_AND_ASSIGN;
    }
    return TK_IDENTIFIER;
}

static UInt8 *nextcharposition(void)
{
    return (UInt8 *)lookahead_position;
}

static short prepnextchar2(void)
{
    UInt8 *start;
    short c;

    start = currentTextPosition;
    c = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = start;
    return c;
}

static int IsWide(void)
{
    int r = 0;
    if (copts.f7f && copts.cplusplus)
        r = 1;
    return r;
}

static UInt16 peek(void)
{
    UInt8 *save = currentTextPosition;
    UInt16 c = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = save;
    return c;
}

static inline UInt8 *lookaheadState(void)
{
    return (unsigned char *)lookahead_position;
}

static inline void ReadLexerPosition(UInt8 **position, void *storage)
{
    *position = storage;
}

static inline void WriteLexerPosition(char **storage, UInt8 *position)
{
    *storage = (char *)position;
}

static inline UInt8 *currentPosition(void)
{
    return currentTextPosition;
}

static inline void setCurrentPosition(UInt8 *position)
{
    currentTextPosition = position;
}

static inline UInt8 *lookaheadPosition(void)
{
    return (unsigned char *)lookahead_position;
}

static inline void setLookaheadPosition(UInt8 *position)
{
    lookahead_position = (char *)position;
}

static inline unsigned char *ReadParserState(char *state)
{
    return (unsigned char *)state;
}

static inline unsigned char *findSpliceLineStart(char *lineStart)
{
    if (macro_expansion_depth == 0) {
        while (*lineStart != 13 && lineStart > PTR_00587fb0)
            --lineStart;
    } else {
        while (*lineStart != 13 && lineStart > macro_text_start)
            --lineStart;
    }
    return (unsigned char *)lineStart;
}

short scan_quoted_literal(short ch)
{
    SInt32 r;

    data_00588524 = 0;
    if ((ch = prepnextchar2()) == '\'') {
        data_00588515 = 1;
        currentTextPosition = nextcharposition();
        r = scan_character_constant(ch);
        data_00588515 = 0;
        token_value_kind_or_string_length = IsWide() ? (UInt8)4 : (UInt8)6;
        return r;
    }
    if (ch == '"') {
        data_00588515 = 1;
        currentTextPosition = nextcharposition();
        r = scan_string_literal(ch);
        data_00588515 = 0;
        return r;
    }
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck && CPrep_ExpandMacro())
        return 0;
    return -3;
}

int scan_identifier_and_expand_macro(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != '\0' && CPrep_ExpandMacro() != '\0') {
        return 0;
    }
    return -3;
}

int scan_numeric_literal(void)
{
    UInt16 c;
    short token;
    short result;
    char *start;
    Boolean integerOverflow;
    char floatOverflow;

    data_00588524 = 0;
    start = (char *)(currentTextPosition - 1);
    currentTextPosition = CExpr2_ParseDecimalCInt64(&token_integer, start, &integerOverflow);
    c = peek();
    if (c == '.' || c == 'e' || c == 'E') {
        currentTextPosition = CMach_FloatScan(start, &token_float, &floatOverflow);
        if (floatOverflow)
            CPrep_ReportError(0x9a);
        token = peek();
        result = parse_float_suffix(token);
        token_value_kind_or_string_length = result;
        return -2;
    } else {
        if (integerOverflow) {
            CPrep_ReportError(0x9a);
            token_integer = cint64_zero;
        }
        token = peek();
        token_value_kind_or_string_length = fn_004961c0(token, 0);
        return -1;
    }
}

int scan_dot_token(SInt16 c)
{
    char flag;
    UInt8 *numberStart;
    short token;
    char *tokenEnd;

    data_00588524 = 0;
    numberStart = currentTextPosition - 1;
    c = CPrepTokenizer_NextChar();
    if (c >= '0' && c <= '9') {
        currentTextPosition = CMach_FloatScan((char *)numberStart, &token_float, &flag);
        if (flag != 0)
            CPrep_ReportError(0x9a);
        numberStart = currentTextPosition;
        c = CPrepTokenizer_NextChar();
        tokenEnd = (char *)currentTextPosition;
        lookahead_position = tokenEnd;
        currentTextPosition = numberStart;
        token = parse_float_suffix(c);
        token_value_kind_or_string_length = token;
        return TK_FLOATCONST;
    }
    if (copts.cplusplus != 0 && c == '*')
        return TK_DOT_STAR;
    if (c == '.') {
        c = CPrepTokenizer_NextChar();
        if (c == '.')
            return TK_ELLIPSIS;
    }
    currentTextPosition = ++numberStart;
    return '.';
}

unsigned int parse_zero_prefixed_number(SInt16 c)
{
    char *p;
    void *cursor;
    Boolean flag;
    char fflag;

    data_00588524 = 0;
    cursor = currentTextPosition;
    p = (char *)cursor - 1;
    c = CPrepTokenizer_NextChar();
    if (c == 'x' || c == 'X') {
        cursor = currentTextPosition;
        cursor = CExpr2_ParseHexInt64(&token_integer, cursor, &flag);
        currentTextPosition = cursor;
    } else if (copts.rejectZeroLengthArrayMembers == 0 && (c == 'b' || c == 'B')) {
        cursor = currentTextPosition;
        cursor = parse_binary_digits(&token_integer, cursor, &flag);
        currentTextPosition = cursor;
    } else {
        while (c >= '0' && c <= '9')
            c = CPrepTokenizer_NextChar();
        switch (c) {
            case '.':
            case 'E':
            case 'e':
                currentTextPosition = CMach_FloatScan(p, &token_float, &fflag);
                if (fflag)
                    CPrep_ReportError(0x9a);
                cursor = currentTextPosition;
                p = cursor;
                c = CPrepTokenizer_NextChar();
                cursor = currentTextPosition;
                lookahead_position = cursor;
                cursor = p;
                currentTextPosition = cursor;
                token_value_kind_or_string_length = parse_float_suffix(c);
                return -2;
            default:
                break;
        }
        cursor = CExpr2_ParseOctalInt64(&token_integer, p, &flag);
        currentTextPosition = cursor;
    }
    if (flag) {
        CPrep_ReportError(0x9a);
        token_integer = cint64_zero;
    }
    cursor = currentTextPosition;
    p = cursor;
    c = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    cursor = p;
    currentTextPosition = cursor;
    token_value_kind_or_string_length = fn_004961c0(c, 1);
    return -1;
}

short scan_div_assign(short ch)
{
    data_00588524 = 0;
    if ((ch = prepnextchar2()) == '=') {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_DIV_ASSIGN;
    }
    return '/';
}

short scan_xor_operator(short ch)
{
    data_00588524 = 0;
    if ((ch = prepnextchar2()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_XOR_ASSIGN;
    }
    return '^';
}

unsigned int tokenize_and(volatile unsigned short token)
{
    short nextToken;
    UInt8 *savedState;

    data_00588524 = 0;
    savedState = currentTextPosition;
    nextToken = CPrepTokenizer_NextChar();
    token = nextToken;
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = savedState;
    if (token == 61) {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_AND_ASSIGN;
    }
    if (nextToken == 38) {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_LOGICAL_AND;
    }
    return TK_BITAND;
}

unsigned int scan_bitor(volatile unsigned short token)
{
    short nextToken;
    UInt8 *savedState;

    data_00588524 = 0;
    savedState = currentTextPosition;
    nextToken = CPrepTokenizer_NextChar();
    token = nextToken;
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = savedState;
    if (token == 61) {
        currentTextPosition = lookaheadState();
        return TK_OR_ASSIGN;
    }
    if (nextToken == 124) {
        currentTextPosition = lookaheadState();
        return TK_LOGICAL_OR;
    }
    return TK_BITOR;
}

unsigned int scan_minus_token(volatile unsigned short token)
{
    unsigned short lookahead;
    UInt8 *savedPosition;
    data_00588524 = 0;
    savedPosition = currentTextPosition;
    lookahead = CPrepTokenizer_NextChar();
    token = lookahead;
    WriteLexerPosition(&lookahead_position, currentTextPosition);
    currentTextPosition = savedPosition;
    if (token == 61) {
        ReadLexerPosition(&currentTextPosition, lookahead_position);
        return TK_SUB_ASSIGN;
    }
    if (lookahead == 45) {
        ReadLexerPosition(&currentTextPosition, lookahead_position);
        return TK_DECREMENT;
    }
    if (lookahead == 62) {
        ReadLexerPosition(&currentTextPosition, lookahead_position);
        if (copts.cplusplus != 0) {
            savedPosition = currentTextPosition;
            lookahead = CPrepTokenizer_NextChar();
            WriteLexerPosition(&lookahead_position, currentTextPosition);
            currentTextPosition = savedPosition;
            if (lookahead == 42) {
                ReadLexerPosition(&currentTextPosition, lookahead_position);
                return TK_ARROW_STAR;
            }
        }
        return TK_ARROW;
    }
    return '-';
}

unsigned int scan_plus_token(unsigned short token)
{
    unsigned short nextToken;
    UInt8 *savedPosition;

    data_00588524 = 0;
    savedPosition = currentPosition();
    nextToken = CPrepTokenizer_NextChar();
    setLookaheadPosition(currentPosition());
    setCurrentPosition(savedPosition);
    if ((token = nextToken) == '=') {
        setCurrentPosition(lookaheadPosition());
        return TK_ADD_ASSIGN;
    }
    if (nextToken == '+') {
        setCurrentPosition(lookaheadPosition());
        return TK_INCREMENT;
    }
    return '+';
}

unsigned int scan_not_or_logical_ne(void)
{
    short token;
    UInt8 *savedPosition;

    data_00588524 = 0;
    savedPosition = currentTextPosition;
    token = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = savedPosition;
    if (token == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_LOGICAL_NE;
    }
    return TK_NOT;
}

unsigned int tokenize_percent(void)
{
    unsigned char token;
    unsigned char *savedState;
    data_00588524 = 0;
    savedState = currentTextPosition;
    token = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = savedState;
    if (token == 61) {
        currentTextPosition = ReadParserState(lookahead_position);
        return TK_MOD_ASSIGN;
    }
    if (token == 62 && copts.cplusplus != 0 && DAT_0058850f == 0) {
        currentTextPosition = ReadParserState(lookahead_position);
        return '}';
    }
    return '%';
}

static inline UInt8 CPrepTokenizer_ObjectiveCEnabled(void)
{
    return copts.f5c;
}

SInt32 scan_at_token(void)
{
    UInt8 *savedTextPosition = currentTextPosition;
    data_00588524 = 0;
    if (CPrepTokenizer_ObjectiveCEnabled() || data_005884fd) {
        Boolean savedIdentifierFlag = data_00587010;
        data_00587010 = 1;
        currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
        data_00587010 = savedIdentifierFlag;
        if (CPrepTokenizer_ObjectiveCEnabled()) {
            if (memcmp("@interface", data_00587fa0->name, 11) == 0)
                return TK_AT_INTERFACE;
            if (memcmp("@implementation", data_00587fa0->name, 16) == 0)
                return TK_AT_IMPLEMENTATION;
            if (memcmp("@protocol", data_00587fa0->name, 10) == 0)
                return TK_AT_PROTOCOL;
            if (memcmp("@end", data_00587fa0->name, 5) == 0)
                return TK_AT_END;
            if (memcmp("@private", data_00587fa0->name, 9) == 0)
                return TK_AT_PRIVATE;
            if (memcmp("@protected", data_00587fa0->name, 11) == 0)
                return TK_AT_PROTECTED;
            if (memcmp("@public", data_00587fa0->name, 8) == 0)
                return TK_AT_PUBLIC;
            if (memcmp("@class", data_00587fa0->name, 7) == 0)
                return TK_AT_CLASS;
            if (memcmp("@selector", data_00587fa0->name, 10) == 0)
                return TK_AT_SELECTOR;
            if (memcmp("@encode", data_00587fa0->name, 8) == 0)
                return TK_AT_ENCODE;
            if (memcmp("@defs", data_00587fa0->name, 6) == 0)
                return TK_AT_DEFS;
        }
        if (data_005884fd)
            return TK_IDENTIFIER;
        currentTextPosition = savedTextPosition;
    }

    return '@';
}

short scan_greater_token(short ch)
{
    data_00588524 = 0;
    if ((ch = prepnextchar2()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_GREATER_EQUAL;
    }
    if (ch == '>') {
        currentTextPosition = (UInt8 *)lookahead_position;
        if (prepnextchar2() == '=') {
            currentTextPosition = (UInt8 *)lookahead_position;
            return TK_SHR_ASSIGN;
        }
        return TK_SHR;
    }
    return '>';
}

unsigned int tokenize_equals(void)
{
    UInt8 *savedState;
    short token;

    data_00588524 = 0;
    savedState = currentTextPosition;
    token = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = savedState;
    if (token == 61) {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_LOGICAL_EQ;
    }
    return '=';
}

short scan_less_token(short ch)
{
    data_00588524 = 0;
    if ((ch = prepnextchar2()) == '=') {
        currentTextPosition = (UInt8 *const)lookahead_position;
        return TK_LESS_EQUAL;
    }
    if (ch == '<') {
        currentTextPosition = (UInt8 *const)lookahead_position;
        if (prepnextchar2() == '=') {
            currentTextPosition = (UInt8 *const)lookahead_position;
            return TK_SHL_ASSIGN;
        }
        return TK_SHL;
    }
    if (ch == '%' && copts.cplusplus && !DAT_0058850f) {
        currentTextPosition = (UInt8 *const)lookahead_position;
        return '{';
    }
    if (ch == ':' && copts.cplusplus && !DAT_0058850f) {
        currentTextPosition = (UInt8 *const)lookahead_position;
        return '[';
    }
    return '<';
}

unsigned int tokenize_colon(void)
{
    unsigned char token;
    UInt8 *savedPosition;

    data_00588524 = 0;
    if (copts.cplusplus != 0) {
        savedPosition = currentTextPosition;
        token = CPrepTokenizer_NextChar();
        lookahead_position = (char *)currentTextPosition;
        currentTextPosition = savedPosition;
        if (token == ':') {
            currentTextPosition = (unsigned char *)lookahead_position;
            return TK_COLON_COLON;
        }
        if (token == '>' && DAT_0058850f == 0) {
            currentTextPosition = (unsigned char *)lookahead_position;
            return ']';
        }
    }
    return ':';
}

unsigned int scan_star_or_mult_assign(void)
{
    short token;
    unsigned char *nextState;
    unsigned char *savedState;

    data_00588524 = 0;
    savedState = currentTextPosition;
    token = CPrepTokenizer_NextChar();
    nextState = currentTextPosition;
    lookahead_position = (char *)nextState;
    currentTextPosition = savedState;
    if (token == 61) {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_MULT_ASSIGN;
    }
    return '*';
}

unsigned int parse_directive_or_return_hash(void)
{
    if (data_00588524 != '\0') {
        CPrep_ParseDirective();
        return 0;
    }
    return 0x23;
}

SInt32 scan_string_literal(short ch)
{
    SInt32 offset;
    SInt32 length;
    UInt8 *savedPosition;
    short nextChar;
    char *output;
    UInt16 *end;
    CInt64 value;

    data_00588524 = 0;
    if (data_0058852a > 0)
        return '"';

    output = string_literal_storage->data;
    DAT_005882de = 0;
    length = 0;
    savedPosition = currentTextPosition;
    data_00580dd0 = 0;
    nextChar = fn_00496610(0);
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = savedPosition;
    if (nextChar == '\\') {
        currentTextPosition = (UInt8 *)lookahead_position;
        if (data_00580dd0 != 0)
            CPrep_IncrementCountersAndUpdateTextOffset();
        data_00580dd0 = 0;
        nextChar = fn_00496610(0);
        lookahead_position = (char *)currentTextPosition;
        if (nextChar == 'p') {
            currentTextPosition = (UInt8 *)lookahead_position;
            if (data_00580dd0 != 0)
                CPrep_IncrementCountersAndUpdateTextOffset();
            DAT_005882de = 1;
            output++;
        } else {
            currentTextPosition = savedPosition;
        }
    }
    while (1) {
        nextChar = read_escaped_character();
        if (nextChar == '"' || nextChar == '\r' || nextChar == 0) {
            if (data_005884fc == 0) {
                if (nextChar == 0 && (macro_expansion_depth > 0 || currentTextPosition >= textend))
                    CPrep_ReportError(0x66);
                else if (nextChar != '"')
                    CPrep_ReportError(0x65);
                break;
            }
        }
        if (length + 2 >= string_literal_buffer_size) {
            offset = output - string_literal_storage->data;
            string_literal_buffer_size += 0x100;
            if (!fn_00443170(string_literal_storage, string_literal_buffer_size))
                CError_LongJump();
            output = string_literal_storage->data + offset;
        }
        if (data_00588515 != 0) {
            CInt64_SetLong(&value, nextChar);
            CMach_InitIntMem((Type *)&stunsignedshort, value, output);
            output += 2;
            length += 2;
        } else {
            *output = nextChar;
            output++;
            length++;
        }
    }
    if (DAT_005882de != 0) {
        if (length > 0xff) {
            CPrep_ReportError(0x6a);
            length = 0xff;
        }
        *string_literal_storage->data = length;
        length++;
    } else {
        if (length + 1 >= string_literal_buffer_size) {
            string_literal_buffer_size += 0x100;
            if (!fn_00443170(string_literal_storage, string_literal_buffer_size))
                CError_LongJump();
        }
        if (data_00588515 != 0) {
            end = (UInt16 *)(string_literal_storage->data + length);
            *end = 0;
            length += 2;
        } else {
            string_literal_storage->data[length] = 0;
            length++;
        }
    }
    token_value_kind_or_string_length = length;
    string_token_data = galloc(length);
    memcpy(string_token_data, string_literal_storage->data, token_value_kind_or_string_length);
    return data_00588515 ? -5 : -4;
}

SInt32 scan_character_constant(short ch)
{
    SInt32 character_count;
    SInt32 value;
    SInt32 character;

    data_00588524 = 0;
    character_count = 0;
    value = 0;
    for (;;) {
        character = read_escaped_character();
        if (character == '\'' && data_005884fc == 0)
            break;
        if (copts.unsignedChar)
            character &= 0xff;
        if (character_count >= 4 || ((character == 0 || character == '\r') && data_005884fc == 0)) {
            CPrep_ReportError(100);
            break;
        }
        if (data_00588515) {
            if (character_count > 0)
                value = (value << 16) | character;
            else
                value = (UInt16)character;
            character_count++;
        } else {
            if (character_count > 0)
                value = (value << 8) | (character & 0xff);
            else
                value = character;
        }
        character_count++;
    }
    if (character_count < 2) {
        if (character_count == 0)
            CPrep_ReportError(100);
        if (copts.cplusplus)
            token_value_kind_or_string_length = 2;
        else
            token_value_kind_or_string_length = 7;
    } else if (character_count == 2) {
        token_value_kind_or_string_length = 8;
    } else {
        token_value_kind_or_string_length = 10;
    }
    switch (character_count) {
        case 1:
            intconst_lo = value;
            token_integer.hi = (value < 0) ? -1 : 0;
            break;
        case 2: {
            SInt32 two_byte_value = value & 0xffff;
            intconst_lo = two_byte_value;
            token_integer.hi = (two_byte_value < 0) ? -1 : 0;
            break;
        }
        case 3: {
            SInt32 three_byte_value = value & 0xffffff;
            intconst_lo = three_byte_value;
            token_integer.hi = (three_byte_value < 0) ? -1 : 0;
            break;
        }
        default:
            intconst_lo = value;
            token_integer.hi = 0;
    }
    return -1;
}

int fn_00495640(void)
{
    fn_0043e8f0();
    if (data_0058850d != '\0') {
        return -7;
    }
    return 0;
}

unsigned int return_unsigned_minus_six(void)
{
    return 4294967290U;
}

static inline void CPrepTokenizer_SkipLineSplices(unsigned char **position)
{
    if (copts.f6f != 0) {
        while (**position == '\\' &&
               CompilerTools_IsByteInDBCSCharacter(findSpliceLineStart((char *)*position), *position) == 0 &&
               (*position)[1] == '\r') {
            CPrep_IncrementCountersAndUpdateTextOffset();
            if ((*position)[2] == '\n')
                *position += 3;
            else
                *position += 2;
        }
    } else {
        while (**position == '\\' && (*position)[1] == '\r') {
            CPrep_IncrementCountersAndUpdateTextOffset();
            if ((*position)[2] == '\n')
                *position += 3;
            else
                *position += 2;
        }
    }
}

void CPrepTokenizer_SkipToEndOfLine(void)
{
    unsigned short ch;
    unsigned char *cursor;
    unsigned char *spliceLineStart;

    cursor = currentTextPosition;
    for (;;) {
        switch (*cursor++) {
            case '\0':
            case '\r':
                currentTextPosition = cursor - 1;
                return;
            case '"':
                if (CPrepTokenizer_SkipQuotedLiteral(cursor, '"') == 0)
                    return;
                cursor = currentTextPosition;
                break;
            case '\'':
                if (CPrepTokenizer_SkipQuotedLiteral(cursor, '\'') == 0)
                    return;
                cursor = currentTextPosition;
                break;
            case '/':
                CPrepTokenizer_SkipLineSplices(&cursor);
                if (*cursor == '/') {
                    if (copts.rejectZeroLengthArrayMembers == 0 || copts.cplusplus != 0) {
                        currentTextPosition = skip_line(cursor + 1);
                        if (data_0058851a == '\r')
                            --currentTextPosition;
                        return;
                    }
                }
                if (*cursor != '*')
                    break;
                currentTextPosition = cursor - 1;
                ++cursor;
                DAT_00588523 = 1;
                for (;;) {
                    if ((ch = *cursor++) == '\0') {
                        if (macro_expansion_depth > 0 || cursor >= textend) {
                            currentTextPosition = cursor - 1;
                            CPrep_ReportError(103);
                            return;
                        }
                        cursor[-1] = ' ';
                    }
                    if (ch == '*') {
                        CPrepTokenizer_SkipLineSplices(&cursor);
                        if (*cursor == '/') {
                            ++cursor;
                            break;
                        }
                    }
                    if (ch == '\r')
                        CPrep_IncrementCountersAndUpdateTextOffset();
                }
                break;
            case '\\':
                if (copts.f6f != 0) {
                    spliceLineStart = findSpliceLineStart((char *)cursor - 1);
                    if (CompilerTools_IsByteInDBCSCharacter(spliceLineStart, (unsigned char *)((char *)cursor - 1)) !=
                        0)
                        break;
                }
                if (*cursor != '\r')
                    break;
                CPrep_IncrementCountersAndUpdateTextOffset();
                if (cursor[1] == '\n')
                    cursor += 2;
                else
                    ++cursor;
                break;
            case '?':
                if (copts.trigraphs == 0)
                    break;
                if (*cursor != '?')
                    break;
                if (cursor[1] != '/')
                    break;
                if (cursor[2] != '\r')
                    break;
                CPrep_IncrementCountersAndUpdateTextOffset();
                cursor += 3;
                break;
        }
    }
}

Boolean CPrepTokenizer_SkipQuotedLiteral(UInt8 *p, SInt16 quote)
{
    UInt8 c;
    UInt8 *q;
    UInt8 *f;

    currentTextPosition = (p - 1);
    for (;;) {
        q = p++;
        c = *q;
        switch (c) {
            case '\0':
            case '\r':
                currentTextPosition = (p - 1);
                return 0;
            case '"':
                if (quote == '"') {
                    currentTextPosition = p;
                    return 1;
                }
                break;
            case '\'':
                if (quote == '\'') {
                    currentTextPosition = p;
                    return 1;
                }
                break;
            case '\\':
                if (copts.f6f != 0) {
                    f = q = p - 1;
                    if (macro_expansion_depth == 0) {
                        while (*f != '\r' && f > (UInt8 *)PTR_00587fb0)
                            f--;
                    } else {
                        while (*f != '\r' && (char *)f > macro_text_start)
                            f--;
                    }
                    if (CompilerTools_IsByteInDBCSCharacter(f, q))
                        break;
                }
                q = p++;
                c = *q;
                switch (c) {
                    case '\0':
                        currentTextPosition = (p - 1);
                        return 0;
                    case '\r':
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (*p == '\n')
                            p++;
                        break;
                }
                break;
            case '?':
                if (copts.trigraphs != 0 && p[0] == '?' && p[1] == '/' && p[2] == '\r') {
                    CPrep_IncrementCountersAndUpdateTextOffset();
                    p += 3;
                }
                break;
        }
    }
}

unsigned char *skip_line(unsigned char *p)
{
    unsigned char *q;
    unsigned char *start;
    unsigned char *back;

    for (;;) {
        switch (*p++) {
            case 0:
                if (macro_expansion_depth > 0 || p >= textend) {
                    data_0058851a = 0;
                    return p - 1;
                }
                p[-1] = ' ';
                break;
            case '\r':
                data_0058851a = '\r';
                return p;
            case '\\':
                if (copts.f6f != 0) {
                    start = p;
                    back = start;
                    q = --back;
                    start = back;
                    if (macro_expansion_depth == 0) {
                        while (*q != '\r' && q > (unsigned char *)PTR_00587fb0)
                            q--;
                    } else {
                        while (*q != '\r' && (char *)q > macro_text_start)
                            q--;
                    }
                    if (CompilerTools_IsByteInDBCSCharacter(q, start))
                        break;
                }
                if (*p == '\r')
                    p++;
                if (*p == '\n')
                    p++;
                break;
        }
    }
}

void CPrepTokenizer_SkipToDelimiter(SInt16 delimiter)
{
    SInt16 character;
    for (;;) {
        character = read_escaped_character();
        if (character == '\r')
            CPrep_IncrementCountersAndUpdateTextOffset();
        if ((character == 0 || character == '\r') && !data_005884fc) {
            if (character == 0) {
                if (macro_expansion_depth > 0 || currentTextPosition >= textend)
                    CPrep_ReportError(102);
                else
                    CPrep_ReportError(100);
            } else if (delimiter == '"' || delimiter == '\'') {
                CPrep_ReportError(112);
            } else {
                CPrep_ReportError(117);
            }
            break;
        }
        if (character == delimiter && !data_005884fc)
            break;
    }
}

void CPrepTokenizer_SkipToChar(short c)
{
    short ch;

    while (1) {
        ch = read_escaped_character();
        if ((ch == 0 || ch == 0xd) && !data_005884fc) {
            if (ch == 0) {
                if (macro_expansion_depth > 0 || currentTextPosition >= textend)
                    CPrep_ReportError(0x66);
                else
                    CPrep_ReportError(0x64);
            } else {
                if (c == 0x22 || c == 0x27)
                    CPrep_ReportError(0x70);
                else
                    CPrep_ReportError(0x75);
            }
            break;
        }
        if (ch == c && !data_005884fc)
            break;
    }
}

static inline short hex(short input)
{
    short c = input;
    if (input >= 48 && c <= 57)
        c = c - 48;
    else if (c >= 97 && c <= 102)
        c = c - 87;
    else if (c >= 65 && c <= 70)
        c = c - 55;
    else
        c = -1;
    return c;
}

static inline short hex2(short c)
{
    if (c >= 48 && c <= 57)
        return c - 48;
    if (c >= 97 && c <= 102)
        return c - 87;
    if (c >= 65 && c <= 70)
        return c - 55;
    return -1;
}

static inline short hexlook(int input)
{
    short c;
    if ((c = input) >= 48 && c <= 57)
        c = c - 48;
    else if (c >= 97 && c <= 102)
        c = c - 87;
    else if (c >= 65 && c <= 70)
        c = c - 55;
    else
        c = -1;
    return c;
}

static inline int complement(int value)
{
    int mask = ~value;
    return mask;
}

static inline unsigned char *cursor_start(void)
{
    return currentTextPosition - 1;
}

static inline char verify(unsigned char *saved)
{
    char *v1 = (char *)saved;
    if (macro_expansion_depth == 0) {
        for (; *v1 != 13 && v1 > PTR_00587fb0; --v1)
            ;
    } else {
        for (; *v1 != 13 && v1 > macro_text_start; --v1)
            ;
    }
    return CompilerTools_IsByteInDBCSCharacter((unsigned char *)v1, saved);
}

static inline short signedpeek(void)
{
    short c;
    unsigned char *saved = currentTextPosition;
    data_00580dd0 = 0;
    c = fn_00496610(0);
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = saved;
    return c;
}

unsigned int read_escaped_character(void)
{
    short count;
    long value;
    int mask;
    short ch;
    short escape;
    int overflowMask;
    mask = 255;
    if (data_00588515 != 0)
        mask = 65535;
    data_005884fc = 0;
    ch = fn_00496610(1);
    if (ch == 92 && (copts.f6f == 0 || !verify(cursor_start()))) {
        data_005884fc = 1;
        switch (escape = fn_00496610(1)) {
            case 97:
                return 7;
            case 98:
                return 8;
            case 116:
                return 9;
            case 118:
                return 11;
            case 102:
                return 12;
            case 110:
                if (copts.f66 != 0)
                    return 13;
                return 10;
            case 114:
                if (copts.f66 != 0)
                    return 10;
                return 13;
            case 101:
                return 27;
            case 120: {
                char overflow;
                short digit;
                value = hex(fn_00496610(1));
                if (value == -1) {
                    CPrep_ReportError(100);
                    return 32;
                }
                overflow = 0;
                overflowMask = complement(mask);
                while (digit = signedpeek(), hexlook((unsigned short)digit) != -1) {
                    value = hex2(digit) + (value << 4), currentTextPosition = (unsigned char *)lookahead_position;
                    if (data_00580dd0 != 0)
                        CPrep_IncrementCountersAndUpdateTextOffset();
                    if (value & overflowMask)
                        overflow = 1;
                }
                if (overflow)
                    CError_ReportError(ERR_ILLEGAL_CHARACTER_CONSTANT);
                break;
            }
            default: {
                char overflow;
                short raw;
                short digit;
                if (escape >= 48 && escape <= 55) {
                    overflow = 0;
                    overflowMask = mask;
                    count = 1;
                    value = escape - 48;
                    overflowMask = ~overflowMask;
                    while (count < 3 && (digit = raw = signedpeek()) >= 48 && raw <= 55) {
                        value = (value << 3) + raw - 48;
                        currentTextPosition = (unsigned char *)lookahead_position;
                        if (data_00580dd0 != 0)
                            CPrep_IncrementCountersAndUpdateTextOffset();
                        if (value & overflowMask)
                            overflow = 1;
                        ++count;
                    }
                    if (overflow)
                        CError_ReportError(ERR_ILLEGAL_CHARACTER_CONSTANT);
                } else
                    value = escape;
            }
        }
    } else
        value = ch;
    if (mask == 255)
        return (signed char)(value & mask);
    return value & mask;
}

SInt16 parse_float_suffix(short suffix)
{
    SInt16 floatType;
    char nextCharacter;

    switch (suffix) {
        case 'F':
        case 'f':
            currentTextPosition = (UInt8 *)lookahead_position;
            nextCharacter = *currentTextPosition;
            suffix = nextCharacter;
            floatType = 13;
            if (stfloat.size != stlongdouble.size)
                token_float = CMachine_RoundFloatToType(TYPE(&stfloat), token_float);
            break;

        case 'L':
        case 'l':
            currentTextPosition = (UInt8 *)lookahead_position;
            nextCharacter = *currentTextPosition;
            suffix = nextCharacter;
            floatType = 16;
            break;

        case 'D':
        case 'd':
            if (copts.rejectZeroLengthArrayMembers)
                break;
            currentTextPosition = (UInt8 *)lookahead_position;
            nextCharacter = *currentTextPosition;
            suffix = nextCharacter;
            floatType = 15;
            if (stdouble.size != stlongdouble.size)
                token_float = CMachine_RoundFloatToType(TYPE(&stdouble), token_float);
            break;

        default:
            if (copts.f91) {
                floatType = 13;
                if (stfloat.size != stlongdouble.size)
                    token_float = CMachine_RoundFloatToType(TYPE(&stfloat), token_float);
            } else {
                floatType = 15;
                if (stdouble.size != stlongdouble.size)
                    token_float = CMachine_RoundFloatToType(TYPE(&stdouble), token_float);
            }
            break;
    }

    if ((suffix >= 'a' && suffix <= 'z') || (suffix >= 'A' && suffix <= 'Z') || suffix == '_' ||
        (suffix >= '0' && suffix <= '9'))
        CPrep_ReportError(0x69);

    return floatType;
}

static inline int needs_unsigned(unsigned int n, char u)
{
    int r = 1;
    if ((n & 0x80000000) == 0 && u == 0)
        r = 0;
    return r;
}

static inline short CPrep_ReadLookaheadCharacter(void)
{
    signed char character;
    currentTextPosition = (UInt8 *)lookahead_position;
    character = *currentTextPosition;
    return character;
}

short fn_004961c0(short suffix, int radix)
{
    Boolean isLong;
    short next;
    SInt32 high;
    UInt32 low;
    int unsignedLongLong;
    unsigned char integerType;
    Boolean isLongLong;
    Boolean isUnsigned;

    isUnsigned = isLong = isLongLong = 0;
    switch (suffix) {
        case 'U':
        case 'u':
            isUnsigned = 1;
            currentTextPosition = (UInt8 *)lookahead_position;
            next = peek();
            suffix = next;
            if (next != 'l' && next != 'L')
                break;
            isLong = 1;
            suffix = CPrep_ReadLookaheadCharacter();
            if (copts.f77 == 0)
                break;
            currentTextPosition = (UInt8 *)lookahead_position;
            next = peek();
            suffix = next;
            if (next != 'l' && next != 'L')
                break;
            isLongLong = 1;
            suffix = CPrep_ReadLookaheadCharacter();
            break;
        case 'L':
        case 'l':
            isLong = 1;
            currentTextPosition = (UInt8 *)lookahead_position;
            next = peek();
            suffix = next;
            if (next == 'u' || next == 'U') {
                isUnsigned = 1;
                suffix = CPrep_ReadLookaheadCharacter();
                break;
            }
            if (copts.f77 == 0 || (next != 'l' && next != 'L'))
                break;
            isLongLong = 1;
            suffix = CPrep_ReadLookaheadCharacter();
            break;
        case 'I':
        case 'i':
            if (copts.f68 != 0 && copts.rejectZeroLengthArrayMembers == 0 && copts.f77 != 0) {
                currentTextPosition = (UInt8 *)lookahead_position;
                next = peek();
                suffix = next;
                if (next == '6') {
                    currentTextPosition = (UInt8 *)lookahead_position;
                    next = peek();
                    suffix = next;
                    if (next == '4') {
                        isLong = isLongLong = 1;
                        suffix = CPrep_ReadLookaheadCharacter();
                    }
                }
            }
    }
    if ((suffix >= 'a' && suffix <= 'z') || (suffix >= 'A' && suffix <= 'Z') || suffix == '_' ||
        (suffix >= '0' && suffix <= '9'))
        CPrep_ReportError(105);

    high = token_integer.hi;
    if (isLongLong != 0 || (isLong == 0 && high != 0)) {
        if (copts.f77 != 0) {
            unsignedLongLong = 1;
            if ((high & 0x80000000) == 0 && isUnsigned == 0)
                unsignedLongLong = 0;
            if (unsignedLongLong != 0)
                integerType = 12;
            else
                integerType = 11;
            return integerType;
        }
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
        token_integer = cint64_zero;
    }
    if (high != 0) {
        CError_ReportError(ERR_NUMBER_OUT_RANGE);
        token_integer = cint64_zero;
    }
    low = token_integer.lo;
    if (isLong != 0) {
        if (needs_unsigned(low, isUnsigned) != 0)
            integerType = 10;
        else
            integerType = 9;
        return integerType;
    }
    if (isUnsigned != 0)
        return 8;
    if (low <= 2147483647)
        integerType = 7;
    else
        integerType = 8;
    return integerType;
}

UInt8 *CPrepTokenizer_ScanIdentifier(UInt8 *cursor)
{
    UInt8 character;
    UInt8 lineStatus;
    UInt8 *lineStart;
    int length;
    char identifier[256];

    length = 0;
    while (1) {
        for (;;) {
            if (DAT_00586fd0[*cursor] == '\0')
                break;
            if (length < 0xff) {
                identifier[length] = *cursor;
                ++length;
            }
            ++cursor;
        }
        character = *cursor;
        if (copts.f6f != '\0') {
            if (character != '\\')
                break;
            lineStart = cursor;
            if (macro_expansion_depth == 0) {
                while ((*lineStart != '\r') && ((char *)lineStart > PTR_00587fb0)) {
                    lineStart = lineStart - 1;
                }
            } else {
                while ((*lineStart != '\r') && ((char *)lineStart > macro_text_start)) {
                    lineStart = lineStart - 1;
                }
            }
            lineStatus = CompilerTools_IsByteInDBCSCharacter(lineStart, cursor);
            if ((lineStatus != '\0') || (cursor[1] != '\r'))
                break;
            if (cursor[2] == '\n') {
                cursor += 3;
            } else {
                cursor += 2;
            }
            CPrep_IncrementCountersAndUpdateTextOffset();
        } else {
            if ((character != '\\') || (cursor[1] != '\r'))
                break;
            if (cursor[2] == '\n') {
                cursor += 3;
            } else {
                cursor += 2;
            }
            CPrep_IncrementCountersAndUpdateTextOffset();
        }
    }
    identifier[length] = 0;
    data_00587fa0 = GetHashNameNode(identifier);
    return cursor;
}

SInt16 CPrepTokenizer_PeekChar(void)
{
    UInt8 *lift_value_0;
    SInt16 lift_call_1;
    UInt8 *lift_value_2;
    lift_value_0 = currentTextPosition;
    lift_call_1 = CPrepTokenizer_NextChar();
    lift_value_2 = currentTextPosition;
    lookahead_position = (char *)lift_value_2;
    currentTextPosition = lift_value_0;
    return lift_call_1;
}

static inline unsigned char CheckEscapedNewline(UInt8 *cursor)
{
    unsigned char *slash;
    unsigned char *lineStart;
    lineStart = slash = cursor - 1;
    if (macro_expansion_depth == 0) {
        while (*lineStart != '\r' && lineStart > (unsigned char *)PTR_00587fb0)
            --lineStart;
    } else {
        while (*lineStart != '\r' && (char *)lineStart > macro_text_start)
            --lineStart;
    }
    return CompilerTools_IsByteInDBCSCharacter(lineStart, slash);
}

short fn_00496610(char report)
{
    short ch;
    UInt8 *cursor;
    cursor = currentTextPosition;
    for (;;) {
        switch (ch = *cursor++) {
            case 0:
                currentTextPosition = cursor - 1;
                return ch;
            case '\\':
            processBackslash:
                if (*cursor == '\r' && (copts.f6f == 0 || CheckEscapedNewline(cursor) == 0)) {
                    if (report != 0)
                        CPrep_IncrementCountersAndUpdateTextOffset();
                    else
                        data_00580dd0 = 1;
                    if (cursor[1] == '\n') {
                        cursor += 2;
                        continue;
                    }
                    ++cursor;
                    continue;
                }
                currentTextPosition = cursor;
                return ch;
            case '?':
                if (copts.trigraphs != 0 && *cursor == '?') {
                    switch (cursor[1]) {
                        case '=':
                            currentTextPosition = cursor + 2;
                            return '#';
                        case '/':
                            ch = '\\';
                            cursor += 2;
                            goto processBackslash;
                        case '\'':
                            currentTextPosition = cursor + 2;
                            return '^';
                        case '(':
                            currentTextPosition = cursor + 2;
                            return '[';
                        case ')':
                            currentTextPosition = cursor + 2;
                            return ']';
                        case '!':
                            currentTextPosition = cursor + 2;
                            return '|';
                        case '<':
                            currentTextPosition = cursor + 2;
                            return '{';
                        case '>':
                            currentTextPosition = cursor + 2;
                            return '}';
                        case '-':
                            currentTextPosition = cursor + 2;
                            return '~';
                    }
                }
                currentTextPosition = cursor;
                return ch;
            default:
                currentTextPosition = cursor;
                return ch;
        }
    }
}

static unsigned char *find_start(unsigned char *s)
{
    char *lineStart = (char *)s;
    if (macro_expansion_depth == 0) {
        while (*lineStart != '\r' && lineStart > PTR_00587fb0)
            lineStart--;
    } else {
        while (*lineStart != '\r' && lineStart > macro_text_start)
            lineStart--;
    }
    return (unsigned char *)lineStart;
}

static inline unsigned char is_multibyte_trail_before(unsigned char *p)
{
    unsigned char *s;
    unsigned char *q;
    q = (s = p - 1);
    if (macro_expansion_depth == 0) {
        while (*q != '\r' && q > (unsigned char *)PTR_00587fb0)
            q--;
    } else {
        while (*q != '\r' && (char *)q > macro_text_start)
            q--;
    }
    return CompilerTools_IsByteInDBCSCharacter(q, s);
}

static inline char multibyte_characters_enabled(void)
{
    return copts.f6f;
}

short CPrepTokenizer_NextChar(void)
{
    short character;
    unsigned char *cursor;

    cursor = currentTextPosition;
    for (;;) {
        switch (character = *cursor++) {
            case 0:
                cursor--;
                currentTextPosition = cursor;
                return character;

            case '/':
                if (multibyte_characters_enabled()) {
                    while (*cursor == '\\' && CompilerTools_IsByteInDBCSCharacter(find_start(cursor), cursor) == 0 &&
                           cursor[1] == '\r') {
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (cursor[2] == '\n')
                            cursor += 3;
                        else
                            cursor += 2;
                    }
                } else {
                    while (*cursor == '\\' && cursor[1] == '\r') {
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (cursor[2] == '\n')
                            cursor += 3;
                        else
                            cursor += 2;
                    }
                }
                if (*cursor == '/') {
                    if (copts.rejectZeroLengthArrayMembers == 0 || copts.cplusplus != 0) {
                        DAT_00588523 = 1;
                        currentTextPosition = skip_line(cursor + 1);
                        return data_0058851a;
                    }
                }
                if (*cursor == '*') {
                    cursor++;
                    DAT_00588523 = 1;
                    for (;;) {
                        if ((character = *cursor++) == 0) {
                            if (macro_expansion_depth > 0 || cursor >= textend) {
                                currentTextPosition = cursor - 1;
                                CPrep_ReportError(0x67);
                                return character;
                            }
                            cursor[-1] = ' ';
                        }
                        if (character == '*') {
                            if (multibyte_characters_enabled()) {
                                while (*cursor == '\\' &&
                                       CompilerTools_IsByteInDBCSCharacter(find_start(cursor), cursor) == 0 &&
                                       cursor[1] == '\r') {
                                    CPrep_IncrementCountersAndUpdateTextOffset();
                                    if (cursor[2] == '\n')
                                        cursor += 3;
                                    else
                                        cursor += 2;
                                }
                            } else {
                                while (*cursor == '\\' && cursor[1] == '\r') {
                                    CPrep_IncrementCountersAndUpdateTextOffset();
                                    if (cursor[2] == '\n')
                                        cursor += 3;
                                    else
                                        cursor += 2;
                                }
                            }
                            if (*cursor == '/')
                                break;
                        }
                        if (character == '\r')
                            CPrep_IncrementCountersAndUpdateTextOffset();
                    }
                    currentTextPosition = cursor + 1;
                    return ' ';
                }
                currentTextPosition = cursor;
                return character;

            case '\\':
                do {
                    if (*cursor == '\r' && (!multibyte_characters_enabled() || !is_multibyte_trail_before(cursor))) {
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (cursor[1] == '\n')
                            cursor += 2;
                        else
                            cursor += 1;
                        break;
                    }
                    currentTextPosition = cursor;
                    return character;

                    case '%':
                        if (*cursor == ':' && copts.cplusplus != 0) {
                            currentTextPosition = cursor + 1;
                            return '#';
                        }
                        currentTextPosition = cursor;
                        return character;

                    case '?':
                        if (copts.trigraphs && *cursor == '?') {
                            switch (cursor[1]) {
                                case '=':
                                    currentTextPosition = cursor + 2;
                                    return '#';
                                case '/':
                                    character = '\\';
                                    cursor += 2;
                                    continue;
                                case '\'':
                                    currentTextPosition = cursor + 2;
                                    return '^';
                                case '(':
                                    currentTextPosition = cursor + 2;
                                    return '[';
                                case ')':
                                    currentTextPosition = cursor + 2;
                                    return ']';
                                case '!':
                                    currentTextPosition = cursor + 2;
                                    return '|';
                                case '<':
                                    currentTextPosition = cursor + 2;
                                    return '{';
                                case '>':
                                    currentTextPosition = cursor + 2;
                                    return '}';
                                case '-':
                                    currentTextPosition = cursor + 2;
                                    return '~';
                            }
                        }
                        currentTextPosition = cursor;
                        return character;
                } while (character == '\\');
                break;

            default:
                currentTextPosition = cursor;
                return character;
        }
    }
}

static inline unsigned char CPrepTokenizer_CheckSplice(unsigned char *cursor)
{
    char *lineStart = (char *)cursor;
    if (macro_expansion_depth == 0) {
        while (*lineStart != '\r' && lineStart > PTR_00587fb0)
            lineStart--;
    } else {
        while (*lineStart != '\r' && lineStart > macro_text_start)
            lineStart--;
    }
    return CompilerTools_IsByteInDBCSCharacter((unsigned char *)lineStart, cursor);
}

static inline unsigned char CPrepTokenizer_IsDBCSTrail(unsigned char *cursor)
{
    unsigned char *backslash = cursor;
    char *lineStart = (char *)--backslash;
    if (macro_expansion_depth == 0) {
        while (*lineStart != '\r' && lineStart > PTR_00587fb0)
            lineStart--;
    } else {
        while (*lineStart != '\r' && lineStart > macro_text_start)
            lineStart--;
    }
    return CompilerTools_IsByteInDBCSCharacter((unsigned char *)lineStart, backslash);
}

short CPrepTokenizer_ScanChar(void)
{
    short ch;
    unsigned char *cursor;
    char *lineStart;
    unsigned char next;
    unsigned char *commentEnd;

    cursor = currentTextPosition;
    for (;;) {
        switch (ch = *cursor) {
            case 0:
                if (macro_expansion_depth > 0 || cursor >= textend) {
                    lookahead_position = (char *)(currentTextPosition = cursor);
                    return ch;
                }
                currentTextPosition = cursor;
                lookahead_position = (char *)&cursor[1];
                return ch;
            case 4:
            case 5:
                cursor++;
                continue;
            case 9:
            case 10:
            case 11:
            case 12:
            case 32:
                cursor++;
                DAT_00588523 = 1;
                continue;
            case 47:
                currentTextPosition = cursor;
                cursor++;
                if (copts.f6f != 0) {
                    while (*cursor == '\\' && CPrepTokenizer_CheckSplice(cursor) == 0 && cursor[1] == '\r') {
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (cursor[2] == '\n')
                            cursor += 3;
                        else
                            cursor += 2;
                    }
                } else {
                    while (*cursor == '\\' && cursor[1] == '\r') {
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (cursor[2] == '\n')
                            cursor += 3;
                        else
                            cursor += 2;
                    }
                }
                if (macro_expansion_depth <= 0) {
                    if ((next = *cursor) == '/') {
                        if (copts.rejectZeroLengthArrayMembers == 0 || copts.cplusplus != 0 || copts.f90 != 0) {
                            DAT_00588523 = 1;
                            commentEnd = skip_line(cursor + 1);
                            lookahead_position = (char *)commentEnd;
                            return data_0058851a;
                        }
                    }
                    if (next == '*') {
                        cursor++;
                        DAT_00588523 = 1;
                        for (;;) {
                            if ((ch = *cursor++) == 0) {
                                if (macro_expansion_depth > 0 || cursor >= textend) {
                                    CPrep_ReportError(103);
                                    lookahead_position = (char *)&cursor[-1];
                                    return ch;
                                }
                                cursor[-1] = ' ';
                            }
                            if (ch == '*') {
                                if (copts.f6f != 0) {
                                    while (*cursor == '\\' && CPrepTokenizer_CheckSplice(cursor) == 0 &&
                                           cursor[1] == '\r') {
                                        CPrep_IncrementCountersAndUpdateTextOffset();
                                        if (cursor[2] == '\n')
                                            cursor += 3;
                                        else
                                            cursor += 2;
                                    }
                                } else {
                                    while (*cursor == '\\' && cursor[1] == '\r') {
                                        CPrep_IncrementCountersAndUpdateTextOffset();
                                        if (cursor[2] == '\n')
                                            cursor += 3;
                                        else
                                            cursor += 2;
                                    }
                                }
                                if (*cursor == '/') {
                                    cursor++;
                                    break;
                                }
                            }
                            if (ch == '\r')
                                CPrep_IncrementCountersAndUpdateTextOffset();
                        }
                        continue;
                    }
                }
                lookahead_position = (char *)cursor;
                return ch;
            case 37:
                currentTextPosition = cursor;
                if (cursor[1] == ':' && copts.cplusplus != 0) {
                    lineStart = (char *)&cursor[1];
                    lookahead_position = lineStart + 1;
                    return '#';
                }
                lookahead_position = (char *)&cursor[1];
                return ch;
            case 92:
                currentTextPosition = cursor;
                cursor++;
            splice:
                if (*cursor == '\r') {
                    if (!copts.f6f || !CPrepTokenizer_IsDBCSTrail(cursor)) {
                        CPrep_IncrementCountersAndUpdateTextOffset();
                        if (cursor[1] == '\n')
                            cursor += 2;
                        else
                            cursor += 1;
                        continue;
                    }
                }
                lookahead_position = (char *)cursor;
                return ch;

            case 63:
                currentTextPosition = cursor;
                if (copts.trigraphs != 0 && cursor[1] == '?') {
                    next = cursor[2];
                    switch (next) {
                        case '=':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '#';
                        case '/':
                            ch = '\\';
                            cursor += 3;
                            goto splice;
                        case '\'':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '^';
                        case '(':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '[';
                        case ')':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return ']';
                        case '!':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '|';
                        case '<':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '{';
                        case '>':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '}';
                        case '-':
                            lineStart = (char *)&cursor[1];
                            lookahead_position = lineStart + 2;
                            return '~';
                        default:
                            break;
                    }
                }
                lookahead_position = (char *)&cursor[1];
                return ch;
            default:
                currentTextPosition = cursor;
                lookahead_position = (char *)&cursor[1];
                return ch;
        }
    }
}
