#include "compiler/common.h"
#include "driver/libimp-eabi-ppc-mw.h"
#include "driver/CLPlugins.h"
#include "driver/ClientGlue.h"
#include "driver/libimp-eabi-ppc.h"

/* The library importer's static plugin. */

#pragma scheduling off

static PluginDesc data_00549eb0 = {2, 'Comp', 8, 0x80400000, '????', 11};

int __stdcall get_stored_name_and_length(char **name, int *length)
{
    *name = (char *)&data_00549eb0;
    *length = 18;
    return 0;
}

static UInt32 data_00549ec4 = 'ePPC';
static UInt32 data_00549ec8 = 'EABI';
static TargetInfo data_00549ecc = {1, 1, &data_00549ec4, 1, &data_00549ec8};

unsigned int __stdcall fn_0040bfc0(unsigned char *volatile *buffer)
{
    *buffer = (unsigned char *)&data_00549ecc;
    return 0U;
}

static const char *data_00549ef4 = "MW Lib Import PPC EABI";

unsigned int __stdcall get_global_value(unsigned int *value)
{
    *value = (unsigned int)data_00549ef4;
    return 0U;
}

static const char *data_00549ef8 = "MW Lib Import PPC EABI";

unsigned int __stdcall fn_0040bff0(unsigned int *value)
{
    *value = (unsigned int)data_00549ef8;
    return 0U;
}

#pragma scheduling reset

static FileMap data_00549efc[5] = {{0, ".elf", 0}, {0, ".a", 0}, {'ELF ', "", 0}, {'ELF ', ".o", 0}, {'MPLF', "", 0}};
static FileMapInfo data_00549fc4 = {1, 5, data_00549efc};
static PluginDirectoryList data_00549fcc = {1, 0, NULL};

#pragma optimization_level 2

unsigned int __stdcall fn_0040c010(struct ListLink *link)
{
    link->next = (struct ListLink *)&data_00549fc4;
    return 0U;
}

unsigned int __stdcall set_next_from_global(struct ListLink *node)
{
    node->next = (struct ListLink *)&data_00549fcc;
    return 0U;
}

#pragma optimization_level reset

static PluginVersion data_00549fd4 = {2, 3, 3, PLUGIN_BUILD};

#pragma scheduling off

unsigned int __stdcall get_buffer(unsigned char **buffer)
{
    *buffer = (unsigned char *)&data_00549fd4;
    return 0;
}

#pragma scheduling reset

static char data_00549fd8[] = "!<arch>\n";
static char data_00549fe4[] = "\177ELF";
static FileSignature data_00549fec[2] = {{'MPLF', data_00549fd8, 8, 0}, {'ELF ', data_00549fe4, 4, 0}};
static FileSignatureList data_0054a008 = {2, data_00549fec};

#pragma optimization_level 2

int __stdcall set_list_link_next(struct ListLink *link)
{
    link->next = (struct ListLink *)&data_0054a008;
    return 0;
}

#pragma optimization_level reset

static CWObjectFlags data_0054a014 = {2, 0, NULL, NULL, NULL, "s", NULL, NULL, 0, 0, 0, 0, 0, 0, 'CWIE', 'TEXT', 0, 0};

#pragma scheduling off

unsigned int __stdcall fn_0040c050(struct ListLink *link)
{
    link->next = (struct ListLink *)&data_0054a014;
    return 0U;
}

#pragma scheduling reset

static void *PTR_fn_0054a05c[9] = {(void *)fn_0041ec70,
                                   (void *)get_stored_name_and_length,
                                   (void *)fn_0040bff0,
                                   (void *)get_global_value,
                                   (void *)set_next_from_global,
                                   NULL,
                                   NULL,
                                   (void *)get_buffer,
                                   (void *)set_list_link_next};
static void *PTR_fn_0054a080[6] = {(void *)fn_0040bfc0, (void *)fn_0040c010, NULL, NULL, (void *)fn_0040c050, NULL};

int fn_0040c060(void)

{
    return ClientGlue_CreateAndAddPlugin(PTR_fn_0054a05c, PTR_fn_0054a080);
}

unsigned int fn_0040c070(void)
{
    return 1U;
}
