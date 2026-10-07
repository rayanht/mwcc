#ifndef DRIVER_RESOURCESTRINGS_H
#define DRIVER_RESOURCESTRINGS_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ResourceRegistration {
    char *data;
    short resourceId;
    char **value;
};
extern int ResourceStrings_AddResource(char *resourceData, short resourceId, char **resourceValue);
extern char *ResourceStrings_GetString(short id, short index);

#ifdef __cplusplus
}
#endif

#endif
