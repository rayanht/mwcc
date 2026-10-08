#ifndef DRIVER_CLTOOLEXEC_H
#define DRIVER_CLTOOLEXEC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SInt32 argc;
struct ToolCommandLine {
    int argc;
    char **argv;
    char **envp;
};
extern void CLToolExec_AppendArgument(int *count, char ***storage, const char *value);
extern int append_command_line_arguments(int count, char **strings, int *stored_count, char ***stored_strings);
extern int free_items(char **items);
extern int build_tool_command_line(int flags, DropinFileRecord *tool, struct ToolCommandLine *arguments);
extern unsigned int CLToolExec_SetTemporaryOutputMask(void);
extern unsigned int CLToolExec_DeleteTemporaryOutputs(void);
extern int CLToolExec_ExecuteLinker(Plugin *tool, UInt32 flags, DropinFileRecord *argument, char *inputPath,
                                    char *outputPath);

#ifdef __cplusplus
}
#endif

#endif
