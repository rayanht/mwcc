#ifndef COMPILER_TOKENS_H
#define COMPILER_TOKENS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct FileOffsetInfo {
    CPrepFileInfo *file;
    SInt32 tokenline;
    Boolean is_inline;
};
#pragma options align = reset

union TData {
    HashNameNode *tkidentifier;
    CInt64 tkintconst;
    Float tkfloatconst;
    struct {
        char *data;
        SInt32 size;
    } tkstring;
};

struct TStreamElement {
    SInt16 tokentype;
    SInt16 subtype;
    CPrepFileInfo *tokenfile;
    SInt32 tokenoffset;
    SInt32 tokenline;
    TData data;
};

struct TokenStream {
    SInt32 tokens;
    TStreamElement *firsttoken;
};

#ifdef __cplusplus
}
#endif

#endif
