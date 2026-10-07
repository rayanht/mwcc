#include "compiler/common.h"
#include "driver/cc-eabi-ppc-mw.h"
#include "compiler/InlineAsmPPC.h"
#include "driver/ClientGlue.h"
#pragma scheduling off
int __stdcall get_name_and_length(char **name, int *length)
{
    *name = data_005434a8;
    *length = 18;
    return 0;
}
#pragma scheduling reset

#pragma scheduling off
void *__stdcall set_next_to_global(struct ListNodeLink *node)
{
    node->next = &data_005434c4;
    return NULL;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int __stdcall copy_global_to_value(unsigned int *value)
{
    *value = data_005434e8;
    return 0U;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int __stdcall get_stored_value(unsigned int *value)
{
    *value = data_00543500;
    return 0;
}
#pragma scheduling reset
#pragma scheduling off

static void helper(signed char **p)
{
    *p = data_00543694;
}

#pragma scheduling reset
#pragma scheduling off
int __stdcall fn_0040be10(signed char **arguments)
{
    helper(arguments);
    return 0;
}
#pragma scheduling reset
#pragma scheduling off

unsigned int __stdcall fn_0040be30(struct ListLink *link)
{
    link->next = &data_00543700;
    return 0;
}
#pragma scheduling reset
#pragma scheduling off
int __stdcall set_link_next_to_global(struct ListLink *link)
{
    link->next = &data_00543774;
    return 0;
}
#pragma scheduling reset
#pragma scheduling off

void *__stdcall set_listnode_next_to_global(struct ListNode *node)
{
    node->next = &data_00543730;
    return NULL;
}
unsigned int __stdcall fn_0040be20(struct ListLink *link)
{
    link->next = &data_005436f8;
    return 0U;
}

#pragma scheduling reset
#pragma scheduling off
int __stdcall get_global_name_and_length(char **name, int *length)
{
    *name = global_name;
    *length = 18;
    return 0;
}
#pragma scheduling reset

unsigned int __stdcall set_data_pointer(unsigned int objectAddress)
{
    struct DataPointerObject *object;
    unsigned char *data;
    object = (struct DataPointerObject *)objectAddress;
    object->data = (data = data_005437e8);
    object = NULL;
    return (unsigned int)object;
}

unsigned int __stdcall fn_0040be90(unsigned int objectAddress)
{
    struct TableObject *object;
    unsigned char *table;
    object = (struct TableObject *)objectAddress;
    object->table = (table = data_005437f0);
    object = NULL;
    return (unsigned int)object;
}
#pragma optimization_level 2
unsigned int __stdcall fn_0040bea0(struct ListLink *link)
{
    link->next = &data_00543830;
    return 0U;
}

#pragma optimization_level reset
#pragma scheduling off
unsigned int __stdcall get_data_pointer(unsigned char **output)
{
    *output = data_00543840;
    return 0;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int __stdcall set_shared_data(struct SharedDataHeader *header)
{
    header->data = data_005438f0;
    return 0;
}
#pragma scheduling reset

unsigned int fn_0040bed0(void)
{
    unsigned int success = 0U;
    if (ClientGlue_CreateAndAddPlugin(data_00543738, data_005437bc) &&
        ClientGlue_CreateAndAddPlugin(data_005438f8, data_0054391c))
        success = 1U;
    return success;
}

#pragma scheduling off
unsigned int fn_0040bf10(void)
{
    unsigned int success;
    unsigned int thirdSucceeded;
    unsigned int secondSucceeded;
    success = 0U, thirdSucceeded = 0U, secondSucceeded = 0U;
    if (ClientGlue_AddResourceStrings("Compiler Errors", 10000, data_00545f68) != 0U) {
        if (ClientGlue_AddResourceStrings("Compiler Strings", 10100, data_00546598) != 0U) {
            secondSucceeded = 1U;
        }
    }
    if (secondSucceeded != 0U) {
        if (ClientGlue_AddResourceStrings("PPC Compiler Errors", 10001, data_00547a34) != 0U) {
            thirdSucceeded = 1U;
        }
    }
    if (thirdSucceeded != 0U) {
        if (ClientGlue_AddResourceStrings("Linker Errors", 11001, data_00549c44) != 0U) {
            success = 1U;
        }
    }
    return success;
}

#pragma scheduling reset
#pragma scheduling off
int __stdcall get_stored_name_and_length(char **name, int *length)
{
    *name = data_00549eb0;
    *length = 18;
    return 0;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int __stdcall fn_0040bfc0(unsigned char *volatile *buffer)
{
    *buffer = data_00549ecc;
    return 0U;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int __stdcall get_global_value(unsigned int *value)
{
    *value = data_00549ef4;
    return 0U;
}
#pragma scheduling reset

#pragma scheduling off
unsigned int __stdcall fn_0040bff0(unsigned int *value)
{
    *value = data_00549ef8;
    return 0U;
}
#pragma scheduling reset

#pragma optimization_level 2
int __stdcall set_list_link_next(struct ListLink *link)
{
    link->next = data_0054a008;
    return 0;
}

#pragma optimization_level reset
#pragma scheduling off
unsigned int __stdcall fn_0040c050(struct ListLink *link)
{
    link->next = (struct ListLink *)data_0054a014;
    return 0U;
}
#pragma scheduling reset

#pragma optimization_level 2
unsigned int __stdcall set_next_from_global(struct ListLink *node)
{
    node->next = data_00549fcc;
    return 0U;
}
#pragma optimization_level reset

#pragma scheduling off
unsigned int __stdcall get_buffer(unsigned char **buffer)
{
    *buffer = data_00549fd4;
    return 0;
}
#pragma scheduling reset

#pragma optimization_level 2
unsigned int __stdcall fn_0040c010(struct ListLink *link)
{
    link->next = &data_00549fc4;
    return 0U;
}
#pragma optimization_level reset

int fn_0040c060(void)

{
    return ClientGlue_CreateAndAddPlugin(&PTR_fn_0054a05c, &PTR_fn_0054a080);
}

unsigned int fn_0040c070(void)
{
    return 1U;
}
