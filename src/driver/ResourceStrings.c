#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/ResourceStrings.h"
#include <stdio.h>

static ResourceRegistration resourceRegistrations[16];
static char resource_string_buffer[64];

int ResourceStrings_AddResource(char *resourceData, short resourceId, char **resourceValue)
{
    int i;

    for (i = 0; i < 16 && resourceRegistrations[i].resourceId != 0; ++i) {
        if (resourceId == resourceRegistrations[i].resourceId) {
            fprintf(stderr, "Resource %d is already added!\n", resourceId);
            return 0;
        }
    }
    if (i >= 16) {
        return 0;
    }
    resourceRegistrations[i].data = resourceData;
    resourceRegistrations[i].resourceId = resourceId;
    resourceRegistrations[i].value = resourceValue;
    return 1;
}

char *ResourceStrings_GetString(short id, short index)
{
    int i;
    int j;
    index--;
    for (i = 0; i < 16; i++) {
        if (id == resourceRegistrations[i].resourceId) {
            j = 0;
            if (index < 0) {
                sprintf(resource_string_buffer, "[Illegal string index #%d in list '%s' (%d)]", index,
                        resourceRegistrations[i].data, id);
                return resource_string_buffer;
            }
            while (j <= index) {
                if (resourceRegistrations[i].value[j] == NULL) {
                    sprintf(resource_string_buffer, "[String #%d not found in resource '%s' (%d)]", index + 1,
                            resourceRegistrations[i].data, id);
                    return resource_string_buffer;
                }
                if (j == index)
                    return resourceRegistrations[i].value[j];
                j++;
            }
        }
    }
    return NULL;
}
