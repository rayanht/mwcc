#ifndef COMPILER_UNMANGLE_H
#define COMPILER_UNMANGLE_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MOutBuf {
    char *ptr;
    unsigned int left;
};
extern void assign_object_register(Object *func, SInt16 register_number);
extern void Unmangle_UnmangleSymbolName(char *name, char *output, int outputSize);
extern void Unmangle_UnmangleName(char *name, char *out, UInt32 size);
extern void unmangle_function_parameters(int *context, MOutBuf *output, char *encoding);
extern char *unmangle_type(int *context, MOutBuf *output, char *type);
extern char *unmangle_vector_type(int *unused, MOutBuf *buf, char *s, void *loc);
extern char *unmangle_indirect_type(int *context, MOutBuf *output, char *input, char *suffix, char marker);
extern char *unmangle_function_type(int *context, MOutBuf *output, char *cursor, char *suffix);
extern char *unmangle_array(int *context, MOutBuf *stream, char *name, char *format);
extern char *unmangle_indirect_declarator(int *context, MOutBuf *output, char *name);
extern char *write_qualifiers(MOutBuf *output, char *qualifiers);
extern char *fn_004e8800(int *a, MOutBuf *out, char *p);
extern char *unmangle_template_arguments(int *ctx, MOutBuf *buf, char *p);
extern char *unmangle_qualified_name(int *context, MOutBuf *output, char *cursor, char append, char alternate);

#ifdef __cplusplus
}
#endif

#endif
