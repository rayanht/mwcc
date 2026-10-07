#ifndef DRIVER_ARGUMENTS_H
#define DRIVER_ARGUMENTS_H

#include "compiler/common.h"
#include "driver/Targets.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char data_0054aa78;
extern void append_coalesced_argument(short kind, char *text);
extern void initialize_arguments(unsigned int value, char **otherValue);
extern void skip_whitespace_and_comments(void);
extern Boolean initialize_file_token_cursor(char *name);
extern unsigned int fn_0040f1df(void);
extern char *get_next_token(void);
extern char *get_next_arg(Boolean expand);
extern unsigned char has_more_tokens_or_args(void);
extern void tokenize_arguments(void);
extern void Targets_ParseArguments(int argc, char **argv);
extern void Targets_FreeTokenText(void);
extern void fn_0040f960(void);
extern unsigned int fn_0040f969(void);
extern TokenText *Targets_AdvanceArgument(void);
extern int Targets_IsValueNullOrZero(void);
extern int Targets_IncrementGlobalOnNonzeroResult(void);
extern TokenText *Targets_DecrementCountAndGetTokenText(void);
extern char *Targets_GetTokenTextDescription(TokenText *token);
extern char *Targets_CopyTokenText(TokenText *arg, char *buffer, int maxlen, Boolean warn);
extern void grow_ptr_list(PtrList *list);
extern void append_text_to_ptrlist_item(struct PtrList *list, char *text);
extern void Targets_InitPtrList(PtrList *state);
extern void fn_0040fbe1(PtrList *list, short kind, char *text);
extern void terminate_ptr_list(PtrList *list);
extern void Targets_InitIntegerSequenceResult(PtrList *source, IntegerSequenceResult *result);

#ifdef __cplusplus
}
#endif

#endif
