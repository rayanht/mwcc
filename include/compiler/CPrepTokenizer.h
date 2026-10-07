#ifndef COMPILER_CPREPTOKENIZER_H
#define COMPILER_CPREPTOKENIZER_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct NamePiece {
    struct NamePiece *next;
    char *src;
    SInt32 off;
    SInt32 len;
};
#pragma options align = reset
extern SInt16 CPrepTokenizer_GetToken(void);
extern SInt16 CPrepTokenizer_ScanToken(void);
extern char *string_token_data;
extern char *PTR_00587fb0;
extern struct HashNameNode *data_00587fa0;
extern UInt8 *textend;
extern short CPrepTokenizer_GetNextToken(void);
extern int classify_identifier_token(void);
extern int check_null_terminator(void);
extern unsigned int mark_previous_text_character(unsigned int result);
extern unsigned int clear_global_and_return_argument(unsigned int a0);
extern void concatenate_string_tokens(char strip_terminator);
extern int check_illegal_token(void);
extern short CPrepTokenizer_PeekNextToken(void);
extern short CPrepTokenizer_GetNextTokenAndRestorePosition(void);
extern UInt16 DAT_005882de;
extern UInt8 concatenating_string_tokens;
extern int classify_identifier_or_xor_token(void);
extern int classify_w_keyword(void);
extern unsigned int tokenize_v_keyword(void);
extern int classify_u_keyword(void);
extern SInt32 scan_t_keyword(void);
extern unsigned int tokenize_s_keyword(void);
extern SInt32 scan_r_keyword(void);
extern SInt32 scan_identifier(void);
extern SInt32 scan_o_keyword(void);
extern SInt32 scan_n_keyword(void);
extern int classify_mutable_or_identifier(void);
extern int classify_identifier_name_token(void);
extern SInt32 scan_i_keyword(void);
extern int expand_macro_or_classify_identifier(void);
extern unsigned int classify_f_keyword(void);
extern int recognize_e_keyword(void);
extern int scan_d_keyword(void);
extern SInt32 fn_00493ea0(void);
extern SInt32 scan_b_keyword(void);
extern int scan_identifier_or_keyword_a(void);
extern int scan_identifier_and_expand_macro(void);
extern short scan_div_assign(short ch);
extern short scan_xor_operator(short ch);
extern short scan_greater_token(short ch);
extern short scan_less_token(short ch);
extern unsigned int read_escaped_character(void);
extern short fn_004961c0(short suffix, int unused);
extern UInt8 *CPrepTokenizer_ScanIdentifier(UInt8 *cursor);
extern SInt16 CPrepTokenizer_PeekChar(void);
extern short CPrepTokenizer_NextChar(void);
extern short CPrepTokenizer_ScanChar(void);
extern unsigned int return_unsigned_minus_six(void);
extern short scan_quoted_literal(short ch);
extern int scan_numeric_literal(void);
extern int scan_dot_token(SInt16 c);
extern unsigned int parse_zero_prefixed_number(SInt16 c);
extern unsigned int tokenize_and(volatile unsigned short token);
extern unsigned int scan_bitor(volatile unsigned short token);
extern unsigned int scan_minus_token(volatile unsigned short token);
extern unsigned int scan_plus_token(volatile unsigned short token);
extern unsigned int scan_not_or_logical_ne(void);
extern unsigned int tokenize_percent(void);
extern SInt32 scan_at_token(void);
extern unsigned int tokenize_equals(void);
extern unsigned int tokenize_colon(void);
extern unsigned int scan_star_or_mult_assign(void);
extern unsigned int parse_directive_or_return_hash(void);
extern SInt32 scan_string_literal(short ch);
extern SInt32 scan_character_constant(short ch);
extern int fn_00495640(void);
extern void CPrepTokenizer_SkipToEndOfLine(void);
extern Boolean CPrepTokenizer_SkipQuotedLiteral(UInt8 *p, SInt16 quote);
extern unsigned char *skip_line(unsigned char *p);
extern void CPrepTokenizer_SkipToDelimiter(SInt16 delimiter);
extern void CPrepTokenizer_SkipToChar(short c);
extern SInt16 parse_float_suffix(short c);
extern short fn_00496610(char report);
extern UInt8 *CPrepTokenizer_ScanIdentifier(UInt8 *cursor);
extern SInt16 CPrepTokenizer_PeekChar(void);
extern short CPrepTokenizer_NextChar(void);
extern short CPrepTokenizer_ScanChar(void);
extern struct Float token_float;
extern SInt32 token_value_kind_or_string_length;
extern struct StorageHandle *string_literal_storage;
extern CInt64 token_integer;
extern Boolean data_00587010;
extern SInt32 string_literal_buffer_size;
extern char *lookahead_position;
extern UInt8 *currentTextPosition;
extern UInt8 data_005884fc;
extern UInt8 data_00588515;
extern unsigned char data_0058851a;
extern unsigned char data_00588524;

#ifdef __cplusplus
}
#endif

#endif
