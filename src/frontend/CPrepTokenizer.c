#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CInt64.h"
#include "compiler/CMachine.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "version.h"
#include "driver/COSToolsCLT.h"
#include "driver/Memory.h"
/* Buffered lexical item and its associated value. */
#include <string.h>

CInt64 token_integer;
struct Float token_float;
Boolean data_00587010;
SInt32 string_literal_buffer_size;
char *lookahead_position;
UInt8 *currentTextPosition;
struct HashNameNode *data_00587fa0;
char *data_00587fb0;
UInt8 *textend;
SInt32 token_value_kind_or_string_length;
char *string_token_data;
struct StorageHandle *string_literal_storage;
UInt16 data_005882de;
UInt8 data_005884fc;
UInt8 data_00588515;
unsigned char data_0058851a;
UInt8 concatenating_string_tokens;
unsigned char data_00588524;

static UInt8 data_00580dd0;

typedef int (*TokenScanner)(short);

#define KW(s, n) (memcmp(s, (char *)(data_00587fa0->name), n) == 0)

static inline struct TStreamElement *CurrentBufferedToken(void)
{
    return bufferedTokenPosition;
}

static inline Float *BufferedTokenFloat(struct TStreamElement *token)
{
    return &token->data.tkfloatconst;
}

static inline int cmpw(const void *a, const void *b, unsigned int n)
{
    return memcmp(a, b, n);
}

static inline void check_extended_cpp_keyword(void)
{
    if (copts.ecplusplus)
        fn_0043f3b0(0x153);
}

static inline void CPrepTokenizer_CheckCppExtension(void)
{
    if (copts.ecplusplus)
        fn_0043f3b0(0x153);
}

static inline UInt8 *nextcharposition(void)
{
    return (UInt8 *)lookahead_position;
}

static inline int IsWide(void)
{
    int r = 0;
    if (copts.wchar_type && copts.cplusplus)
        r = 1;
    return r;
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

static unsigned char find_start(unsigned char *s)
{
    char *lineStart = (char *)s;
    if (macro_expansion_depth == 0) {
        while (*lineStart != '\r' && lineStart > data_00587fb0)
            lineStart--;
    } else {
        while (*lineStart != '\r' && lineStart > macro_text_start)
            lineStart--;
    }
    return COS_IsMultiByte((unsigned char *)lineStart, s);
}

static inline UInt8 CPrepTokenizer_ObjectiveCEnabled(void)
{
    return copts.objective_c;
}

static inline void CPrepTokenizer_SkipLineSplices(unsigned char **position)
{
    if (copts.multibyteaware != 0) {
        while (**position == '\\' && find_start(*position) == 0 && (*position)[1] == '\r') {
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

static inline char multibyte_characters_enabled(void)
{
    return copts.multibyteaware;
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
                data_00588523 = 1;
                continue;
            case 47:
                currentTextPosition = cursor;
                cursor++;
                if (copts.multibyteaware != 0) {
                    while (*cursor == '\\' && find_start(cursor) == 0 && cursor[1] == '\r') {
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
                        if (copts.ANSIstrict == 0 || copts.cplusplus != 0 || copts.c9x != 0) {
                            data_00588523 = 1;
                            commentEnd = skip_line(cursor + 1);
                            lookahead_position = (char *)commentEnd;
                            return data_0058851a;
                        }
                    }
                    if (next == '*') {
                        cursor++;
                        data_00588523 = 1;
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
                                if (copts.multibyteaware != 0) {
                                    while (*cursor == '\\' && find_start(cursor) == 0 && cursor[1] == '\r') {
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
                    if (!copts.multibyteaware || !find_start((unsigned char *)((char *)cursor - 1))) {
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
                    while (*cursor == '\\' && find_start(cursor) == 0 && cursor[1] == '\r') {
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
                    if (copts.ANSIstrict == 0 || copts.cplusplus != 0) {
                        data_00588523 = 1;
                        currentTextPosition = skip_line(cursor + 1);
                        return data_0058851a;
                    }
                }
                if (*cursor == '*') {
                    cursor++;
                    data_00588523 = 1;
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
                                while (*cursor == '\\' && find_start(cursor) == 0 && cursor[1] == '\r') {
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
                    if (*cursor == '\r' &&
                        (!multibyte_characters_enabled() || !find_start((unsigned char *)((char *)cursor - 1)))) {
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
                if (*cursor == '\r' &&
                    (copts.multibyteaware == 0 || find_start((unsigned char *)((char *)cursor - 1)) == 0)) {
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

short CPrepTokenizer_PeekChar(void)
{
    UInt8 *save = currentTextPosition;
    short c = CPrepTokenizer_NextChar();
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = save;
    return c;
}

static short peek_char_quietly(void)
{
    UInt8 *save = currentTextPosition;
    short c;
    data_00580dd0 = 0;
    c = fn_00496610(0);
    lookahead_position = (char *)currentTextPosition;
    currentTextPosition = save;
    return c;
}

static void accept_peeked_char(void)
{
    currentTextPosition = (UInt8 *)lookahead_position;
    if (data_00580dd0)
        CPrep_IncrementCountersAndUpdateTextOffset();
}

UInt8 *CPrepTokenizer_ScanIdentifier(UInt8 *cursor)
{
    UInt8 character;
    UInt8 lineStatus;
    int length;
    char identifier[256];

    length = 0;
    while (1) {
        for (;;) {
            if (data_00586fd0[*cursor] == '\0')
                break;
            if (length < 0xff) {
                identifier[length] = *cursor;
                ++length;
            }
            ++cursor;
        }
        character = *cursor;
        if (copts.multibyteaware != '\0') {
            if (character != '\\')
                break;
            lineStatus = find_start(cursor);
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

short fn_004961c0(short suffix, int radix)
{
    Boolean isLong;
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
            suffix = CPrepTokenizer_PeekChar();
            if (suffix != 'l' && suffix != 'L')
                break;
            isLong = 1;
            suffix = CPrep_ReadLookaheadCharacter();
            if (copts.longlong == 0)
                break;
            currentTextPosition = (UInt8 *)lookahead_position;
            suffix = CPrepTokenizer_PeekChar();
            if (suffix != 'l' && suffix != 'L')
                break;
            isLongLong = 1;
            suffix = CPrep_ReadLookaheadCharacter();
            break;
        case 'L':
        case 'l':
            isLong = 1;
            currentTextPosition = (UInt8 *)lookahead_position;
            suffix = CPrepTokenizer_PeekChar();
            if (suffix == 'u' || suffix == 'U') {
                isUnsigned = 1;
                suffix = CPrep_ReadLookaheadCharacter();
                break;
            }
            if (copts.longlong == 0 || (suffix != 'l' && suffix != 'L'))
                break;
            isLongLong = 1;
            suffix = CPrep_ReadLookaheadCharacter();
            break;
        case 'I':
        case 'i':
            if (copts.cpp_extensions != 0 && copts.ANSIstrict == 0 && copts.longlong != 0) {
                currentTextPosition = (UInt8 *)lookahead_position;
                suffix = CPrepTokenizer_PeekChar();
                if (suffix == '6') {
                    currentTextPosition = (UInt8 *)lookahead_position;
                    suffix = CPrepTokenizer_PeekChar();
                    if (suffix == '4') {
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
        if (copts.longlong != 0) {
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
            if (copts.ANSIstrict)
                break;
            currentTextPosition = (UInt8 *)lookahead_position;
            nextCharacter = *currentTextPosition;
            suffix = nextCharacter;
            floatType = 15;
            if (stdouble.size != stlongdouble.size)
                token_float = CMachine_RoundFloatToType(TYPE(&stdouble), token_float);
            break;

        default:
            if (copts.float_constants) {
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

static short hex_digit_value(short c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

unsigned int read_escaped_character(void)
{
    short count;
    long value;
    int mask;
    short ch;
    short escape;
    char overflow;
    short digit;

    mask = 255;
    if (data_00588515 != 0)
        mask = 65535;
    data_005884fc = 0;
    ch = fn_00496610(1);
    if (ch == '\\' && (copts.multibyteaware == 0 || !find_start(currentTextPosition - 1))) {
        data_005884fc = 1;
        switch (escape = fn_00496610(1)) {
            case 'a':
                return 7;
            case 'b':
                return 8;
            case 't':
                return 9;
            case 'v':
                return 11;
            case 'f':
                return 12;
            case 'n':
                if (copts.mpwc_newline != 0)
                    return 13;
                return 10;
            case 'r':
                if (copts.mpwc_newline != 0)
                    return 10;
                return 13;
            case 'e':
                return 27;
            case 'x':
                value = hex_digit_value(fn_00496610(1));
                if (value == -1) {
                    CPrep_ReportError(100);
                    return ' ';
                }
                overflow = 0;
                while (hex_digit_value(digit = peek_char_quietly()) != -1) {
                    value = (value << 4) + hex_digit_value(digit);
                    accept_peeked_char();
                    if (value & ~mask)
                        overflow = 1;
                }
                if (overflow)
                    CError_ReportError(ERR_ILLEGAL_CHARACTER_CONSTANT);
                break;
            default:
                if (escape >= '0' && escape <= '7') {
                    overflow = 0;
                    count = 1;
                    value = escape - '0';
                    while (count < 3 && (digit = peek_char_quietly()) >= '0' && digit <= '7') {
                        value = (value << 3) + digit - '0';
                        accept_peeked_char();
                        if (value & ~mask)
                            overflow = 1;
                        count++;
                    }
                    if (overflow)
                        CError_ReportError(ERR_ILLEGAL_CHARACTER_CONSTANT);
                } else {
                    value = escape;
                }
        }
    } else {
        value = ch;
    }
    if (mask == 255)
        return (signed char)(value & mask);
    return value & mask;
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

unsigned char *skip_line(unsigned char *p)
{
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
                if (copts.multibyteaware != 0) {
                    if (find_start((unsigned char *)((char *)p - 1)))
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

Boolean CPrepTokenizer_SkipQuotedLiteral(UInt8 *p, SInt16 quote)
{
    UInt8 c;
    UInt8 *q;
#if VERSION > VERSION_GC_1_0
    UInt8 *f;
#endif

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
                if (copts.multibyteaware != 0) {
#if VERSION == VERSION_GC_1_0
                    if (find_start((unsigned char *)((char *)p - 1)))
                        break;
#else
                    f = q = p - 1;
                    if (macro_expansion_depth == 0) {
                        while (*f != '\r' && f > (UInt8 *)data_00587fb0)
                            f--;
                    } else {
                        while (*f != '\r' && (char *)f > macro_text_start)
                            f--;
                    }
                    if (COS_IsMultiByte(f, q))
                        break;
#endif
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

void CPrepTokenizer_SkipToEndOfLine(void)
{
    unsigned short ch;
    unsigned char *cursor;
#if VERSION > VERSION_GC_1_0
    char *spliceLineStart;
#endif

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
                    if (copts.ANSIstrict == 0 || copts.cplusplus != 0) {
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
                data_00588523 = 1;
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
                if (copts.multibyteaware != 0) {
#if VERSION == VERSION_GC_1_0
                    if (find_start((unsigned char *)((char *)cursor - 1)))
                        break;
#else
                    spliceLineStart = (char *)cursor - 1;
                    if (macro_expansion_depth == 0) {
                        while (*spliceLineStart != '\r' && spliceLineStart > data_00587fb0)
                            --spliceLineStart;
                    } else {
                        while (*spliceLineStart != '\r' && spliceLineStart > macro_text_start)
                            --spliceLineStart;
                    }
                    if (COS_IsMultiByte((unsigned char *)spliceLineStart, (unsigned char *)((char *)cursor - 1)) != 0)
                        break;
#endif
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

unsigned int return_unsigned_minus_six(void)
{
    return 4294967290U;
}

int fn_00495640(void)
{
    fn_0043e8f0();
    if (data_0058850d != '\0') {
        return -7;
    }
    return 0;
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
        if (copts.unsigned_char)
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
    data_005882de = 0;
    length = 0;
    if (peek_char_quietly() == '\\') {
        savedPosition = currentTextPosition;
        accept_peeked_char();
        if (peek_char_quietly() == 'p') {
            accept_peeked_char();
            data_005882de = 1;
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
            if (!COS_ResizeHandle(string_literal_storage, string_literal_buffer_size))
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
    if (data_005882de != 0) {
        if (length > 0xff) {
            CPrep_ReportError(0x6a);
            length = 0xff;
        }
        *string_literal_storage->data = length;
        length++;
    } else {
        if (length + 1 >= string_literal_buffer_size) {
            string_literal_buffer_size += 0x100;
            if (!COS_ResizeHandle(string_literal_storage, string_literal_buffer_size))
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

unsigned int parse_directive_or_return_hash(void)
{
    if (data_00588524 != '\0') {
        CPrep_ParseDirective();
        return 0;
    }
    return 0x23;
}

unsigned int scan_star_or_mult_assign(void)
{
    data_00588524 = 0;
    if (CPrepTokenizer_PeekChar() == '=') {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_MULT_ASSIGN;
    }
    return '*';
}

#if VERSION == VERSION_GC_1_0
unsigned int tokenize_colon(void)
{
    unsigned char token;

    data_00588524 = 0;
    if (copts.cplusplus != 0) {
        token = CPrepTokenizer_PeekChar();
        if (token == ':') {
            currentTextPosition = (unsigned char *)lookahead_position;
            return TK_COLON_COLON;
        }
        if (token == '>' && data_0058850f == 0) {
            currentTextPosition = (unsigned char *)lookahead_position;
            return ']';
        }
    }
    return ':';
}
#else
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
        if (token == '>' && data_0058850f == 0) {
            currentTextPosition = (unsigned char *)lookahead_position;
            return ']';
        }
    }
    return ':';
}
#endif

short scan_less_token(short ch)
{
    data_00588524 = 0;
    if ((ch = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (UInt8 *const)lookahead_position;
        return TK_LESS_EQUAL;
    }
    if (ch == '<') {
        currentTextPosition = (UInt8 *const)lookahead_position;
        if (CPrepTokenizer_PeekChar() == '=') {
            currentTextPosition = (UInt8 *const)lookahead_position;
            return TK_SHL_ASSIGN;
        }
        return TK_SHL;
    }
    if (ch == '%' && copts.cplusplus && !data_0058850f) {
        currentTextPosition = (UInt8 *const)lookahead_position;
        return '{';
    }
    if (ch == ':' && copts.cplusplus && !data_0058850f) {
        currentTextPosition = (UInt8 *const)lookahead_position;
        return '[';
    }
    return '<';
}

unsigned int tokenize_equals(void)
{
    data_00588524 = 0;
    if (CPrepTokenizer_PeekChar() == '=') {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_LOGICAL_EQ;
    }
    return '=';
}

short scan_greater_token(short ch)
{
    data_00588524 = 0;
    if ((ch = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_GREATER_EQUAL;
    }
    if (ch == '>') {
        currentTextPosition = (UInt8 *)lookahead_position;
        if (CPrepTokenizer_PeekChar() == '=') {
            currentTextPosition = (UInt8 *)lookahead_position;
            return TK_SHR_ASSIGN;
        }
        return TK_SHR;
    }
    return '>';
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

#if VERSION == VERSION_GC_1_0
unsigned int tokenize_percent(void)
{
    unsigned char token;

    data_00588524 = 0;
    token = CPrepTokenizer_PeekChar();
    if (token == '=') {
        currentTextPosition = ReadParserState(lookahead_position);
        return TK_MOD_ASSIGN;
    }
    if (token == '>' && copts.cplusplus != 0 && data_0058850f == 0) {
        currentTextPosition = ReadParserState(lookahead_position);
        return '}';
    }
    return '%';
}
#else
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
    if (token == 62 && copts.cplusplus != 0 && data_0058850f == 0) {
        currentTextPosition = ReadParserState(lookahead_position);
        return '}';
    }
    return '%';
}
#endif

unsigned int scan_not_or_logical_ne(void)
{
    data_00588524 = 0;
    if (CPrepTokenizer_PeekChar() == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_LOGICAL_NE;
    }
    return TK_NOT;
}

#if VERSION == VERSION_GC_1_0
unsigned int scan_plus_token(unsigned short token)
{
    data_00588524 = 0;
    if ((token = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_ADD_ASSIGN;
    }
    if (token == '+') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_INCREMENT;
    }
    return '+';
}
#else
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
#endif

#if VERSION == VERSION_GC_1_0
unsigned int scan_minus_token(unsigned short token)
{
    data_00588524 = 0;
    if ((token = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_SUB_ASSIGN;
    }
    if (token == '-') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_DECREMENT;
    }
    if (token == '>') {
        currentTextPosition = (UInt8 *)lookahead_position;
        if (copts.cplusplus != 0 && CPrepTokenizer_PeekChar() == '*') {
            currentTextPosition = (UInt8 *)lookahead_position;
            return TK_ARROW_STAR;
        }
        return TK_ARROW;
    }
    return '-';
}
#else
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
#endif

#if VERSION == VERSION_GC_1_0
unsigned int scan_bitor(unsigned short token)
{
    data_00588524 = 0;
    if ((token = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_OR_ASSIGN;
    }
    if (token == '|') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_LOGICAL_OR;
    }
    return TK_BITOR;
}
#else
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
#endif

#if VERSION == VERSION_GC_1_0
unsigned int tokenize_and(unsigned short token)
{
    data_00588524 = 0;
    if ((token = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_AND_ASSIGN;
    }
    if (token == '&') {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_LOGICAL_AND;
    }
    return TK_BITAND;
}
#else
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
#endif

short scan_xor_operator(short ch)
{
    data_00588524 = 0;
    if ((ch = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (UInt8 *)lookahead_position;
        return TK_XOR_ASSIGN;
    }
    return '^';
}

short scan_div_assign(short ch)
{
    data_00588524 = 0;
    if ((ch = CPrepTokenizer_PeekChar()) == '=') {
        currentTextPosition = (unsigned char *)lookahead_position;
        return TK_DIV_ASSIGN;
    }
    return '/';
}

unsigned int parse_zero_prefixed_number(SInt16 c)
{
    char *p;
    Boolean flag;
    char fflag;

    data_00588524 = 0;
    p = (char *)currentTextPosition - 1;
    c = CPrepTokenizer_NextChar();
    if (c == 'x' || c == 'X') {
        currentTextPosition = (UInt8 *)CInt64_ScanHexString(&token_integer, (char *)currentTextPosition, &flag);
    } else if (copts.ANSIstrict == 0 && (c == 'b' || c == 'B')) {
        currentTextPosition = (UInt8 *)CInt64_ScanBinString(&token_integer, (char *)currentTextPosition, &flag);
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
                token_value_kind_or_string_length = parse_float_suffix(CPrepTokenizer_PeekChar());
                return -2;
        }
        currentTextPosition = (UInt8 *)CInt64_ScanOctString(&token_integer, p, &flag);
    }
    if (flag) {
        CPrep_ReportError(0x9a);
        token_integer = cint64_zero;
    }
    token_value_kind_or_string_length = fn_004961c0(CPrepTokenizer_PeekChar(), 1);
    return -1;
}

int scan_dot_token(SInt16 c)
{
    char flag;
    UInt8 *numberStart;

    data_00588524 = 0;
    numberStart = currentTextPosition - 1;
    c = CPrepTokenizer_NextChar();
    if (c >= '0' && c <= '9') {
        currentTextPosition = CMach_FloatScan((char *)numberStart, &token_float, &flag);
        if (flag != 0)
            CPrep_ReportError(0x9a);
        token_value_kind_or_string_length = parse_float_suffix(CPrepTokenizer_PeekChar());
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

int scan_numeric_literal(short c)
{
    char *start;
    Boolean integerOverflow;
    char floatOverflow;

    data_00588524 = 0;
    start = (char *)(currentTextPosition - 1);
    currentTextPosition = (UInt8 *)CInt64_ScanDecString(&token_integer, start, &integerOverflow);
    c = CPrepTokenizer_PeekChar();
    if (c == '.' || c == 'e' || c == 'E') {
        currentTextPosition = CMach_FloatScan(start, &token_float, &floatOverflow);
        if (floatOverflow)
            CPrep_ReportError(0x9a);
        token_value_kind_or_string_length = parse_float_suffix(CPrepTokenizer_PeekChar());
        return -2;
    } else {
        if (integerOverflow) {
            CPrep_ReportError(0x9a);
            token_integer = cint64_zero;
        }
        token_value_kind_or_string_length = fn_004961c0(CPrepTokenizer_PeekChar(), 0);
        return -1;
    }
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

short scan_quoted_literal(short ch)
{
    SInt32 r;

    data_00588524 = 0;
    if ((ch = CPrepTokenizer_PeekChar()) == '\'') {
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

int scan_identifier_or_keyword_a(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0)
        return 0;
    if (memcmp("auto", data_00587fa0->name, 5) == 0)
        return TK_AUTO;
    if (copts.cplusplus != 0 || copts.onlystdkeywords == 0) {
        if (memcmp("asm", data_00587fa0->name, 4) == 0)
            return TK_ASM;
    }
    if (copts.cplusplus != 0 && data_005884fd == 0 && data_0058850f == 0) {
        if (memcmp("and", data_00587fa0->name, 4) == 0)
            return TK_LOGICAL_AND;
        if (memcmp("and_eq", data_00587fa0->name, 7) == 0)
            return TK_AND_ASSIGN;
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
        if (copts.booltruefalse != 0) {
            if (memcmp("bool", data_00587fa0->name, 5) == 0)
                return TK_BOOL;
        }
        if (data_005884fd == 0 && data_0058850f == 0) {
            if (memcmp("bitand", data_00587fa0->name, 7) == 0)
                return TK_BITAND;
            if (memcmp("bitor", data_00587fa0->name, 6) == 0)
                return TK_BITOR;
        }
    }
    if (copts.objective_c != 0) {
        if (memcmp("bycopy", data_00587fa0->name, 7) == 0)
            return TK_BYCOPY;
        if (memcmp("byref", data_00587fa0->name, 6) == 0)
            return TK_BYREF;
    }
    return TK_IDENTIFIER;
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
        if (!data_005884fd && !data_0058850f) {
            if (memcmp("compl", data_00587fa0->name, sizeof("compl")) == 0)
                return TK_COMPL;
        }
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
            if (copts.ecplusplus != '\0') {
                fn_0043f3b0(0x153);
            }
            return TK_DYNAMIC_CAST;
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
        if (memcmp("export", data_00587fa0->name, 7) == 0 && copts.ecplusplus == 0)
            return TK_EXPORT;
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
        if (copts.booltruefalse != 0) {
            if (memcmp("false", data_00587fa0->name, 6) == 0)
                return TK_FALSE;
        }
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
    if (copts.cplusplus || copts.c9x) {
        if (KW("inline", 7))
            return TK_INLINE;
    } else if (!copts.onlystdkeywords) {
        if (KW("inline", 7))
            return TK_INLINE;
    }
    if (copts.objective_c != 0) {
        if (KW("in", 3))
            return TK_IN;
        if (KW("inout", 6))
            return TK_INOUT;
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

int classify_mutable_or_identifier(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != '\0' && CPrep_ExpandMacro() != '\0') {
        return 0;
    }
    if (copts.cplusplus != '\0') {
        if (memcmp("mutable", data_00587fa0->name, 8) == 0) {
            if (copts.ecplusplus != '\0') {
                fn_0043f3b0(0x153);
            }
            return TK_MUTABLE;
        }
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
            if (copts.ecplusplus)
                fn_0043f3b0(0x153);
            return TK_NAMESPACE;
        }
        if (data_005884fd == 0 && data_0058850f == 0) {
            if (KW("not", 4))
                return TK_NOT;
            if (KW("not_eq", 7))
                return TK_LOGICAL_NE;
        }
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
        if (data_005884fd == 0 && data_0058850f == 0) {
            if (memcmp("or", data_00587fa0->name, 3) == 0)
                return TK_LOGICAL_OR;
            if (memcmp("or_eq", data_00587fa0->name, 6) == 0)
                return TK_OR_ASSIGN;
        }
    }
    if (copts.objective_c != 0) {
        if (memcmp("out", data_00587fa0->name, 4) == 0)
            return TK_OUT;
        if (memcmp("oneway", data_00587fa0->name, 7) == 0)
            return TK_ONEWAY;
    }
    return TK_IDENTIFIER;
}

SInt32 scan_identifier(void)
{
    data_00588524 = 0;
    currentTextPosition = CPrepTokenizer_ScanIdentifier(currentTextPosition - 1);
    if (macrocheck != 0 && CPrep_ExpandMacro() != 0)
        return 0;
    if (copts.onlystdkeywords == 0) {
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
            if (copts.ecplusplus)
                fn_0043f3b0(0x153);
            return TK_REINTERPRET_CAST;
        }
    }
    if (copts.c9x != 0) {
        if (KW("restrict", 9))
            return TK_RESTRICT;
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
            if (copts.ecplusplus) {
                fn_0043f3b0(0x153);
            }
            return TK_STATIC_CAST;
        }
    }
    return TK_IDENTIFIER;
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
        if (copts.booltruefalse != 0 && KW("true", 5))
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
            if (copts.ecplusplus)
                fn_0043f3b0(0x153);
            return TK_USING;
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
    if ((copts.cplusplus != '\0') && (copts.wchar_type != '\0')) {
        if (cmpw("wchar_t", data_00587fa0->name, 8) == 0) {
            return TK_WCHAR_T;
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
    if (((copts.cplusplus != '\0') && (data_005884fd == '\0')) && (data_0058850f == '\0')) {
        if (memcmp("xor", data_00587fa0->name, 4) == 0) {
            return '^';
        }
        if (memcmp("xor_eq", data_00587fa0->name, 7) == 0) {
            return TK_XOR_ASSIGN;
        }
    }
    return TK_IDENTIFIER;
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
    if (copts.c9x != '\0') {
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
    if (copts.altivec_model != '\0') {
        if (memcmp("__vector", data_00587fa0->name, 9) == 0) {
            return TK_UU_VECTOR;
        }
    }
    return TK_IDENTIFIER;
}

unsigned int clear_global_and_return_argument(unsigned int argument)
{
    data_00588524 = 0U;
    return argument;
}

int check_illegal_token(void)
{
    if (macro_expansion_depth == 0) {
        CError_ReportError(ERR_ILLEGAL_TOKEN);
    }
    return 0;
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

int check_null_terminator(void)
{
    if (*currentTextPosition == '\0') {
        return 0;
    }
    return -6;
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

void concatenate_string_tokens(char strip_terminator)
{
    NamePiece *pieces;
    SInt16 token;
    NamePiece *piece;
    NamePiece *cursor;
    struct TStreamElement *record;
    char *buffer;
    char *original_buffer;
    SInt32 original_length;
    UInt8 original_flag;
    SInt32 combined_length;

    original_buffer = string_token_data;
    original_length = token_value_kind_or_string_length;
    original_flag = data_005882de;
    concatenating_string_tokens = 1;
    pieces = NULL;
    combined_length = token_value_kind_or_string_length;

    for (;;) {
        token = CPrepTokenizer_PeekNextToken();

        if (token != -4 && token != -5) {
            data_005882de = original_flag;
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
                record->subtype = data_005882de;
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

        piece = (NamePiece *)lalloc(sizeof(NamePiece));
        piece->next = pieces;
        piece->src = string_token_data;
        piece->len = token_value_kind_or_string_length - 1;
        piece->off = combined_length - 1;
        pieces = piece;
        if (data_005882de != 0) {
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
                    data_005882de = CurrentBufferedToken()->subtype;
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
            source_offset = macro_stack[0].pos - data_00587fb0;
            bufferedTokenPosition->tokenoffset = source_offset;
            token_start = currentTextPosition;
        } else {
            source_offset = (char *)currentTextPosition - data_00587fb0;
            bufferedTokenPosition->tokenoffset = source_offset;
        }
        CurrentBufferedToken()->tokenline = data_00587ef0;
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
                            CurrentBufferedToken()->subtype = data_005882de;
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
