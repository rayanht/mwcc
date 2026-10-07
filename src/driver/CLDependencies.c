#include "compiler/common.h"
#include "driver/CLDependencies.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CExpr.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "driver/AssertionFailure.h"
#include "driver/CLAccessPaths.h"
#include "driver/CLBrowser.h"
#include "driver/CLDropinCallbacks_V10.h"
#include "driver/CLErrors.h"
#include "driver/CLFileOps.h"
#include "driver/CLIO.h"
#include "driver/CLPluginRequests.h"
#include "driver/CLSegs.h"
#include "driver/CLTarg.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/Generic.h"
#include "driver/MemUtils.h"
#include "driver/Memory.h"
#include "driver/MsDos.h"
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include "compiler/ENode.h"
#include "compiler/Types.h"

static struct AccessPathEntry *data_0054d898 = NULL;

/* Declarations gathered from the merged files. */

/* 0x4ec5e0, signature unknown */
/* 0x4ec610, signature unknown */
/* 0x561510, file name string */

#define EMPTYS ("")

unsigned char CLDependencies_InitDeps(Deps *state, CLTarget *target)
{
    AccessPaths *scope;
    data_0054d898 = NULL;
    state->target = target;
    state->count = state->alloccount = 0;
    state->textsize = state->textused = 0;
    state->text = NULL;
    state->records = NULL;
    scope = (AccessPaths *)xmalloc(NULL, sizeof(AccessPaths));
    state->scope = scope;
    CLAccessPaths_Init(state->scope);
    return 1;
}

void CLDependencies_FreeDeps(Deps *block)
{
    AccessPaths *paths;
    if (block->text != NULL) {
        free(block->text);
    }
    if (block->records != NULL) {
        free(block->records);
    }
    paths = block->scope;
    CLAccessPaths_FreeItems(paths);
}

unsigned char get_record_flag(Deps *table, unsigned int index)
{
    return table->records[index].flag;
}

void make_dependency_osspec(Deps *table, int index, char *output)
{
    DepRecord *record = &table->records[index];
    struct AccessPathEntry *path = record->resolvedPath;

    CLProj_MakeOSSpecFromPath(path->path, table->text + record->nameOffset, 0, (OSSpec *)output);
}

unsigned char fn_00427ad0(Deps *table, char flags, char *key, char *argument, SInt32 *index_out, DepRecord **entry_out)
{
    SInt32 index;
    for (index = 0; index < table->count; ++index) {
        *entry_out = &table->records[index];
        if (OS_EqualPath(table->text + (*entry_out)->nameOffset, key)) {
            if (flags != 0 || (*entry_out)->flag != 0) {
                make_dependency_osspec(table, index, argument);
                *index_out = index;
                return 1;
            }
        }
    }
    return 0;
}

Boolean dep_records_equal(Deps *table, SInt32 firstIndex, SInt32 secondIndex)
{
    DepRecord *first;
    DepRecord *second;
    SInt32 result;

    if (firstIndex == secondIndex)
        return 1;
    first = &table->records[firstIndex];
    second = &table->records[secondIndex];
    result = 0;
    if (first->resolvedPath == second->resolvedPath) {
        if (OS_EqualPath(table->text + first->nameOffset, table->text + second->nameOffset))
            result = 1;
    }
    return result;
}

AccessPathEntry *find_or_create_access_path_entry(AccessPaths *scope, char *name)
{
    AccessPathEntry *result = CLAccessPaths_FindPath(scope, name);
    if (result == NULL) {
        result = CLAccessPaths_CreateAccessPathEntry(name);
        CLAccessPaths_StoreItem(scope, result);
    }
    return result;
}

unsigned char find_access_path_entry(AccessPathEntry *entry, char *comparison, AccessPathEntry **result, char *context)
{
    if (CLProj_MakeOSSpecFromPath(entry->path, comparison, 0, (struct OSSpec *)context) == 0) {
        if (OS_IsFile((struct OSSpec *)context) != 0) {
            *result = entry;
            return 1;
        }
    }
    if (entry->children != NULL) {
        if (find_dependency_access_path_entry(entry->children, comparison, result, context) != 0) {
            return 1;
        }
    }
    return 0;
}

Boolean find_dependency_access_path_entry(AccessPaths *dependencies, char *comparison, AccessPathEntry **result,
                                          char *path)
{
    UInt16 index;
    AccessPathEntry *dependency;

    for (index = 0; index < CLAccessPaths_GetCount(dependencies); index++) {
        dependency = CLAccessPaths_GetEntry(dependencies, index);
        if (dependency == NULL)
            CLIO_ReportAssertionFailure("path", "CLDependencies.c", 0x9b);
        if (find_access_path_entry(dependency, comparison, result, path))
            return 1;
    }
    return 0;
}

void append_dep_record(Deps *deps, char *name, UInt8 flag, AccessPathEntry *obj, AccessPathEntry *type,
                       AccessPathEntry *unused, SInt32 *out)
{
    char *str;
    OSSpec buf;
    DepRecord *rec;
    SInt32 len;
    AccessPathEntry *resolvedType;

    if (obj == NULL && type == NULL)
        str = CLProj_GetFileName(name);
    else
        str = name;

    len = strlen(str) + 1;

    if (deps->count >= deps->alloccount) {
        deps->alloccount += 0x10;
        deps->records = xrealloc("include list", deps->records, deps->alloccount * sizeof(DepRecord));
    }

    rec = &deps->records[deps->count];
    rec->nameOffset = deps->textused;
    rec->flag = flag;
    rec->searchPath = obj;

    if (type != NULL) {
        resolvedType = type;
    } else if (obj != NULL) {
        resolvedType = find_or_create_access_path_entry(deps->scope, obj->path);
    } else {
        OS_MakeFileSpec(name, &buf);
        resolvedType = find_or_create_access_path_entry(deps->scope, buf.directory.path);
    }
    rec->resolvedPath = resolvedType;
    rec->contextPath = get_access_path_entry();

    if (deps->textused + len > deps->textsize) {
        deps->textsize += 0x400;
        deps->text = xrealloc("include list", deps->text, deps->textsize);
    }
    strcpy(deps->text + deps->textused, str);
    deps->textused += len;
    *out = deps->count++;
}

unsigned char CLDependencies_FindFile(Deps *dependencies, char *file, char searchFirst, OSSpec *context, SInt32 *index)
{
    unsigned char found;
    AccessPathEntry *lookupText;
    unsigned char alternate;
    char searchMode;
    unsigned char cached;
    DepRecord *record;
    AccessPathEntry *result;
    AccessPathEntry *value;

    *index = -1;
    found = 0;
    cached = fn_00427ad0(dependencies, searchFirst, file, context->directory.path, index, &record);
    if (cached != 0) {
        if (record->contextPath == data_0054d898) {
            found = 1;
        } else if (record->searchPath == NULL) {
            cached = 0;
        }
    }
    if (found == 0) {
        result = NULL;
        value = NULL;
        searchMode = searchFirst;
        if (MsDos_IsAbsolutePath(file) != 0) {
            found = OS_MakeFileSpec(file, context) == 0 && OS_Status(context) == 0;
            value = result = lookupText = NULL;
        } else {
            result = lookupText = get_access_path_entry();
            if (lookupText != NULL) {
                found = find_access_path_entry(result, file, &result, context->directory.path);
                value = NULL;
            }
            if (found == 0 && cached != 0) {
                lookupText = NULL;
                result = record->resolvedPath;
                value = record->searchPath;
                make_dependency_osspec(dependencies, *index, context->directory.path);
                found = 1;
            }
            if (found == 0 && searchFirst != 0) {
                searchMode = 1;
                result = NULL;
                found = find_dependency_access_path_entry(&dependencies->target->systemPaths, file, &value,
                                                          context->directory.path);
            }
            if (found == 0) {
                searchMode = 0;
                result = NULL;
                found = find_dependency_access_path_entry(&dependencies->target->userPaths, file, &value,
                                                          context->directory.path);
            }
        }
        if (found != 0 && *index < 0) {
            alternate = !searchMode;
            append_dep_record(dependencies, file, alternate, value, result, lookupText, index);
        }
    }
    return found;
}

Boolean initialize_four_words(DependencyCollection *value, struct Deps *fourth)
{
    value->count = 0;
    value->capacity = 0;
    value->entries = NULL;
    value->dependencyTable = fourth;
    return 1;
}

SInt32 CLDependencies_SetAccessPath(OSSpec *name, Boolean flag)
{
    char buffer[260];
    char *path;

    switch (data_00541b44) {
        case 2:
            data_0054d898 = NULL;
            return 1;
        case 0:
            if (!flag)
                return 1;
            OS_GetCWD(buffer);
            path = buffer;
            break;
        case 1:
            if (!flag)
                return 1;
            /* fall through */
        case 3:
            if (name != NULL) {
                path = name->directory.path;
            } else {
                OS_GetCWD(buffer);
                path = buffer;
            }
            break;
        default:
            CLErrors_ReportInternalError("CLDependencies.c", 0x197, "Unhandled include file search type (%d)\n",
                                         data_00541b44);
            break;
    }

    data_0054d898 = find_or_create_access_path_entry(default_target->dependencyTable.scope, path);

    if (DAT_00541b28 > 1) {
        CLErrors_ForwardMessage(0x69, fn_00412340(path, data_005880e0, 0x104));
    }
    return 1;
}

AccessPathEntry *get_access_path_entry(void)
{
    return data_0054d898;
}

UInt8 contains_dependency(DependencyCollection *collection, int key)
{
    int index;
    Deps *table;
    index = 0;
    while (index < collection->count) {
        table = &default_target->dependencyTable;
        if (dep_records_equal(table, collection->entries[index], key) != 0) {
            return 1;
        }
        index = index + 1;
    }
    return 0;
}

void append_dependency_entry(DependencyCollection *v, int x, signed char flag)
{
    if (v->count >= v->capacity) {
        v->capacity += 16;
        v->entries = xrealloc("dependency list", v->entries, v->capacity * 4);
    }
    v->entries[v->count++] = x;
}

void CLDependencies_InsertDependencyIfAbsent(DependencyCollection *collection, SInt32 index, OSSpec *name,
                                             unsigned char flags, signed char entryFlag, char *result)
{
    UInt8 matched;
    unsigned int alternateMatch;

    if (index < 0) {
        append_dep_record(&default_target->dependencyTable, OS_SpecToString(name, data_005880e0, 0x104), 0, NULL, NULL,
                          NULL, &index);
    }
    matched = contains_dependency(collection, index);
    if (!matched) {
        if (DAT_00541c0b) {
            alternateMatch = get_record_flag(collection->dependencyTable, index);
        } else {
            alternateMatch = 0;
        }
        if ((unsigned char)alternateMatch == 0) {
            append_dependency_entry(collection, index, entryFlag);
        }
    }
    if (result) {
        *result = matched;
    }
}

char *escape_spaces(char escapeSpaces, char *destination, char *source)
{
    char *result;

    if (!escapeSpaces) {
        return source;
    }
    result = destination;
    while (*source) {
        if (*source == ' ') {
            *destination++ = '\\';
        }
        *destination++ = *source++;
    }
    *destination = '\0';
    return result;
}

void CLDependencies_WriteDependencies(Deps *ctx, DropinFileRecord *file, MemBuffer *stream)
{
    struct Deps *table = ctx;
    struct OSSpec *source;
    SInt32 remaining;
    char line[520];
    char target[260];
    char dependency[260];
    char escaped[324];
    char path[324];
    Boolean hasSpace;
    SInt32 i;
    int *items;
    char *separator;

    do {
        if (OS_NewHandle(0, stream) != 0) {
            break;
        }
        remaining = file->dependencies.count;
        source = &file->outputPath;
        CLProj_MakeRelativePath(source, NULL, target, 0x104);
        if (target[0] != 0) {
            hasSpace = (strchr(target, ' ') != NULL);
            sprintf(line, "%s%s%s%s ", hasSpace ? EMPTYS : EMPTYS, escape_spaces(hasSpace, escaped, target),
                    hasSpace ? EMPTYS : EMPTYS, ":");
            if (CLFileOps_AppendMemBuffer(stream, line, strlen(line)) != 0) {
                break;
            }
            hasSpace = (strchr(file->inputName, ' ') != NULL);
            separator = remaining ? "\\" : EMPTYS;
            sprintf(line, "%s%s%s %s\n", hasSpace ? EMPTYS : EMPTYS, escape_spaces(hasSpace, escaped, file->inputName),
                    hasSpace ? EMPTYS : EMPTYS, separator);
            if (CLFileOps_AppendMemBuffer(stream, line, strlen(line)) != 0) {
                break;
            }
        } else {
            hasSpace = (strchr(file->inputName, ' ') != NULL);
            separator = remaining ? "\\" : EMPTYS;
            sprintf(line, "%s%s%s%s %s\n", hasSpace ? EMPTYS : EMPTYS,
                    escape_spaces(hasSpace, escaped, file->inputName), hasSpace ? EMPTYS : EMPTYS, ":", separator);
            if (CLFileOps_AppendMemBuffer(stream, line, strlen(line)) != 0) {
                break;
            }
        }
        for (i = 0; i < file->dependencies.count; i++) {
            remaining--;
            items = file->dependencies.entries;
            make_dependency_osspec(table, items[i], path);
            OS_SpecToString((OSSpec *)path, dependency, 0x104);
            hasSpace = (strchr(dependency, ' ') != NULL);
            separator = remaining ? "\\" : EMPTYS;
            sprintf(line, "\t%s%s%s %s\n", hasSpace ? EMPTYS : EMPTYS, escape_spaces(hasSpace, escaped, dependency),
                    hasSpace ? EMPTYS : EMPTYS, separator);
            if (CLFileOps_AppendMemBuffer(stream, line, strlen(line)) != 0) {
                goto out_of_memory;
            }
        }
        return;
    } while (0);
out_of_memory:
    CLIO_FormatAndDispatchText("\nOut of memory\n");
    longjmp(driver_jmp_buf, 1);
}
