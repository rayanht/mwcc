#ifndef DRIVER_LIBIMP_EABI_PPC_H
#define DRIVER_LIBIMP_EABI_PPC_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/InlineAsmPPC.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ConfigurationBlock116 {
    char reserved_00[5];
    char flag_05;
    char reserved_06[110];
};
struct ConfigurationBlock60 {
    char reserved_00[44];
    char flag_2c;
    char reserved_2d[15];
};
struct Elf32Header {
    unsigned char magic[4];
    unsigned char elfClass;
    unsigned char dataEncoding;
    unsigned char identificationRest[10];
    unsigned short type;
    unsigned short machine;
    unsigned int version;
    unsigned int entry;
    unsigned int program_header_offset;
    unsigned int section_header_offset;
    unsigned int flags;
    unsigned short header_size;
    unsigned short program_header_size;
    unsigned short program_header_count;
    unsigned short section_header_size;
    unsigned short section_header_count;
    unsigned short section_name_index;
};
struct Elf32Section {
    unsigned int name, type, flags, address, file_offset, size;
    unsigned int link, info, alignment, entry_size;
};
struct FileInputNode {
    struct FileInputNode *next; /* 0x00: read_file_into_input_nodes links archive members via last->next */
    UInt8 kind; /* 0x04: read_file_into_input_nodes sets classify_file_header result or archive kind 1 */
    UInt8
        alignmentBytes[3]; /* 0x05: read_file_into_input_nodes zeroes these unused bytes between kind and dataHandle */
    char **dataHandle;     /* 0x08: read_file_into_input_nodes stores the allocated member contents handle */
    char *
        archiveMemberHeader; /* 0x0c: read_file_into_input_nodes reads 60 text bytes and sscanf parses length at 0x30 */
};
struct ReadValue {
    union {
        struct {
            int word0;
            int word1;
        };
        char data[8];
    };
};
extern SInt32 data_0054c4d0;
extern jmp_buf file_input_jmpbuf;
extern void format_message_and_longjmp();
extern void check_ticks_and_longjmp(void);
extern void fn_0041e970(char *a0, void *a1);
extern void fn_0041e990(char *a0, void *a1);
extern UInt8 fn_0041e9b0(char *data);
extern unsigned int DAT_0054c490;
extern char **load_file_data_and_set_archive_signature(CWFileSpec *name, SInt32 *out1, SInt32 *out2, SInt32 *out3);
extern SInt32 section_size_total;
extern SInt32 accumulated_section_size;
extern SInt32 accumulated_section_sizes;
extern char initialize_plugin_context(CWPluginPrivateContext *handle);
extern unsigned int file_index;
extern unsigned char data_0057f51c[];
extern unsigned char data_0057f56f[];
extern unsigned int fn_0041ec20(void);
extern struct CWPluginPrivateContext *plugin_context;
extern long data_0057f48c[12];
extern unsigned int data_0057f4bc;
extern int __stdcall fn_0041ec70(CWPluginPrivateContext *context);
extern SInt32 data_0057f498;
extern SInt32 data_0057f49c;
extern SInt32 data_0057f4a0;
extern int data_0057f4a4;
extern int data_0057f4c0;
extern char data_0058770c[];
extern void fn_0041ede0(char *value);
extern void fn_0041ee00(void *a0, short a1);
extern unsigned char classify_file_header(CWFileSpec *input);
extern signed char file_header_magic_first_byte;
extern signed char data_0054c4d5;
extern char **read_file_into_buffer(CWFileSpec *file, short *error, int extraBytes);
extern FileInputNode *read_file_into_input_nodes(CWFileSpec *name, SInt16 *err);
extern void clear_file_input_data_handles(FileInputNode *args);
extern unsigned char fn_0041f430(Elf32Header *record, char *location, char *name);
extern char data_00588517;
extern void accumulate_section_sizes(FileInputNode *input, char *option1, char *option2);
extern UInt8 nonNativeByteOrder;
extern char *resolve_archive_member_name(char **baseOffset, char *text);
extern void terminate_slash_newline_sequences(char **buffer, int length);
extern void classify_archive_members(FileInputNode *list, char *arg);
extern short dispatch_file_input_by_kind(FileInputNode *record, char *buffer);

#ifdef __cplusplus
}
#endif

#endif
