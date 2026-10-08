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
extern union PreprocessedText {
    GList list;
    struct StorageHandle *handle;
} data_00586da8;

#ifdef __cplusplus
}
#endif

#endif
