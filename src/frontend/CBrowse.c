#define CERROR_FILE "CBrowse.c"
#include "compiler/common.h"
#include "compiler/CBrowse.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CMangler.h"
#include "compiler/CodeGen.h"
#include "compiler/Unmangle.h"
#include "version.h"
#include "driver/COSToolsCLT.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include <string.h>

UInt8 data_005884f5;

/* The browser's access code of each access type. */
static UInt8 data_00563340[4] = {4, 1, 2, 0};

static union {
    GList buffer;
    struct StorageHandle *handle;
} data_00581ba8;
static GList browse_member_list;
static BrowseObjectBuffer browse_function_buffer;
static SInt32 nextFunctionId;

#define BROWSE_ASSERT(c, s)                                                                                            \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

/* The kind of each record of the browse data. */
typedef enum BrowseType {
    BrowseType_Function,
    BrowseType_Global,
    BrowseType_Class,
    BrowseType_Macro,
    BrowseType_Enum,
    BrowseType_Typedef,
    BrowseType_Constant,
    BrowseType_Template,
    BrowseType_End = 0xFF
} BrowseType;

static void RecordUndefinedMemberFunctions(void);

static inline CPrepFileInfo *browse_source(TemplateFunction *info)
{
    return info->srcfile;
}

void CBrowse_InitBrowseData(CPrepCU *classes)
{
    struct BrowseStreamHeader header;
    if (classes == NULL) {
        CError_Internal("CBrowse.c", 122U);
    }
    classes->browseData = 0;
    InitGList(&data_00581ba8.buffer, 65536);
    InitGList(&browse_function_buffer.buffer, 1024);
    nextFunctionId = 1U;
    data_005884f5 = 0;
    memclrw(&header, 76U);
    header.magic = 0xbeabbaebU;
    header.version = 2U;
    header.valueC = 2U;
    header.language = (copts.cplusplus != 0) ? (unsigned char)2 : (unsigned char)1;
    header.valueA = data_005884f5;
    AppendGListData(&data_00581ba8.buffer, &header, 76U);
}

void CBrowse_StoreBrowseData(CPrepCU *arguments)
{
    long result;

    CError_ASSERT(149, arguments != NULL);

    if (data_00581ba8.buffer.size >= 0x4cU) {
        RecordUndefinedMemberFunctions();
        AppendGListByte(&data_00581ba8.buffer, -1);
        COS_ResizeHandle(data_00581ba8.handle, data_00581ba8.buffer.size);
        if (fn_0041bcb0(arguments->context, data_00581ba8.handle, &result) == 0) {
            arguments->browseData = result;
            data_00581ba8.buffer.data = NULL;
        }
    }
}

void CBrowse_FreeLists(struct CPrepCU *cu)
{
    FreeGList(&data_00581ba8.buffer);
    FreeGList(&browse_member_list);
    FreeGList(&browse_function_buffer.buffer);
}

static void AppendGList(GList *dst, GList *src)
{
    SInt32 offset;

    offset = dst->size;
    AppendGListNoData(dst, src->size);
    memcpy(*dst->data + offset, *src->data, src->size);
}

void write_text_or_name_id(GList *output, char *text, int index)
{
    HashNameNode *entry;
    unsigned int length;

    if (output == NULL || text == NULL || *text == 0)
        CError_Internal("CBrowse.c", 0xbc);

    if (index < 0 && data_005884f5 != 0) {
        for (entry = data_00587f88[CHash(text)]; entry != NULL; entry = entry->next) {
            if (strcmp(text, entry->name) == 0) {
                index = entry->id;
                break;
            }
        }
    }

    if (index >= 0 && data_005884f5 != 0) {
        AppendGListWord(output, -1);
        AppendGListLong(output, index);
    } else {
        length = strlen(text);
        AppendGListWord(output, length);
        if (length != 0)
            AppendGListData(output, text, length + 1U);
    }
}

void CBrowse_GenerateClassRecord(DeclInfo *record, GList *out)
{
    HashNameNode *name;
    Type *baseType;
    TemplClassInst *base;
    SInt32 baseNameID;
    ClassList *baseList;
    char *baseName;
    char *className;
    SInt32 baseCount;

    CError_ASSERT(225, record && record->thetype && out);
    *out = browse_member_list;
    if (!(record->file && record->file->fileID && record->file->recordbrowseinfo && record->file2 &&
          record->file2->fileID && record->sourceoffset > 0)) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    if (CParser_IsNullOrAtOrDollarPrefixedName(TYPE_CLASS(record->thetype)->classname) != 0) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    InitGList(&browse_member_list, 0x4000);
    AppendGListByte(&browse_member_list, 2);
    AppendGListWord(&browse_member_list, record->file->fileID);
    AppendGListWord(&browse_member_list, record->file2->fileID);
    AppendGListLong(&browse_member_list, record->sourceoffset - 1);
    CError_ASSERT(268, browse_member_list.size == 9);
    AppendGListLong(&browse_member_list, record->sourceoffset - 1);
    AppendGListLong(&browse_member_list, 0);
    name = TYPE_CLASS(record->thetype)->classname;
    write_text_or_name_id(&browse_member_list, name->name, name->id);
    fn_004c2ac0(record->thetype, 0);
    AppendGListByte(&data_00583548, 0);
    className = lalloc(data_00583548.size + 1);
    strcpy(className, *data_00583548.data);
    while (*className != 0 && *className >= '0' && *className <= '9')
        className++;
    if (strcmp(TYPE_CLASS(record->thetype)->classname->name, className) != 0)
        write_text_or_name_id(&browse_member_list, className, -1);
    else
        AppendGListWord(&browse_member_list, 0);
    AppendGListLong(&browse_member_list, 0);
    baseCount = 0;
    for (baseList = TYPE_CLASS(record->thetype)->bases; baseList != NULL; baseList = baseList->next)
        baseCount++;
    AppendGListByte(&browse_member_list, baseCount);
    for (baseList = TYPE_CLASS(record->thetype)->bases; baseList != NULL; baseList = baseList->next) {
        AppendGListByte(&browse_member_list, data_00563340[baseList->access]);
        AppendGListByte(&browse_member_list, baseList->is_virtual);
        base = (TemplClassInst *)baseList->base;
        if ((base->theclass.flags & CLASS_IS_TEMPL_INST) && base->is_specialized == 0)
            baseType = TYPE(base->templ);
        else
            baseType = TYPE(base);
        fn_004c2ac0(baseType, 0);
        AppendGListByte(&data_00583548, 0);
        baseName = lalloc(data_00583548.size + 1);
        strcpy(baseName, *data_00583548.data);
        while (*baseName != 0 && *baseName >= '0' && *baseName <= '9')
            baseName++;
        baseNameID = baseList->base->classname->id;
        while (*baseName != 0 && *baseName >= '0' && *baseName <= '9') {
            baseName++;
            baseNameID = -1;
        }
        write_text_or_name_id(&browse_member_list, baseName, baseNameID);
    }
}

void CBrowse_WriteObjMemberVar(ObjMemberVar *rec, SInt32 start, SInt32 end)
{
    SInt16 len;

    if (rec == NULL)
        CError_Internal("CBrowse.c", 0x166);
    if (browse_member_list.data != NULL && start > 0 && end >= start) {
        if (tk == ';')
            end++;
        AppendGListByte(&browse_member_list, 1);
        AppendGListByte(&browse_member_list, data_00563340[rec->access]);
        AppendGListLong(&browse_member_list, 0);
        AppendGListLong(&browse_member_list, start - 1);
        AppendGListLong(&browse_member_list, end - 1);
        len = strlen(rec->name->name);
        AppendGListWord(&browse_member_list, len);
        AppendGListData(&browse_member_list, rec->name->name, len + 1);
    }
}

void CBrowse_RecordFunction(Object *obj, SInt32 start, SInt32 end)
{
    UInt32 flags;
    TypeMemberFunc *func;
    SInt32 id;

    CError_ASSERT(378, obj != NULL);
    if (CParser_IsNullOrAtOrDollarPrefixedName(obj->name))
        return;
    if (browse_member_list.data != NULL && start > 0 && end >= start) {
        flags = 0;
        CError_ASSERT(389, obj->type != NULL && obj->type->type == TYPEFUNC);
        func = TYPE_METHOD(obj->type);
        if (!(func->flags & FUNC_AUTO_GENERATED)) {
            if (obj->datatype == DVFUNC)
                flags |= 0x400;
            if (func->flags & FUNC_PURE)
                flags |= 1;
            if (func->is_static)
                flags |= 2;
            if (func->flags & FUNC_IS_DTOR)
                flags |= 0x800;
            if (func->flags & 0x4000)
                flags |= 0x1000;
            AppendGListByte(&browse_member_list, 0);
            AppendGListByte(&browse_member_list, data_00563340[obj->access]);
            AppendGListLong(&browse_member_list, flags);
            id = func->funcid;
            if (id <= 0) {
                if (!(func->flags & FUNC_DEFINED) || id == -1)
                    AppendGListLong(&browse_function_buffer.buffer, (SInt32)obj);
                id = nextFunctionId++;
                func->funcid = id;
            }
            AppendGListLong(&browse_member_list, id);
            AppendGListLong(&browse_member_list, start - 1);
            AppendGListLong(&browse_member_list, end);
        }
    }
}

void CBrowse_RecordDataObject(Object *obj, SInt32 param2, SInt32 param3)
{
    SInt16 len;

    if (obj == NULL)
        CError_FATAL(433);

    if (browse_member_list.data != NULL && param2 > 0 && param3 >= param2 && obj->datatype == DDATA) {
        if (tk == ';')
            param3++;
        AppendGListByte(&browse_member_list, 1);
        AppendGListByte(&browse_member_list, data_00563340[obj->access]);
        AppendGListLong(&browse_member_list, 2);
        AppendGListLong(&browse_member_list, param2 - 1);
        AppendGListLong(&browse_member_list, param3 - 1);
        len = (SInt16)strlen(obj->name->name);
        AppendGListWord(&browse_member_list, len);
        AppendGListData(&browse_member_list, obj->name->name, len + 1);
    }
}

void CBrowse_RestoreScope(SInt32 statementOffset, GList *savedScope)
{
    if (savedScope == NULL)
        CError_Internal("CBrowse.c", 451U);
    if (browse_member_list.data != NULL) {
        if (browse_member_list.size > 0) {
            if (tk == ';')
                statementOffset++;
            memcpy(*browse_member_list.data + 9, &statementOffset, sizeof(statementOffset));
            AppendGList(&data_00581ba8.buffer, &browse_member_list);
            AppendGListByte(&data_00581ba8.buffer, -1);
        }
        FreeGList(&browse_member_list);
    }
    browse_member_list = *savedScope;
}

void CBrowse_BuildTypeStructBrowseInfo(DeclInfo *obj, TypeStruct *info, GList *out)
{
    HashNameNode *name;

    CError_ASSERT(478, obj != NULL && out != NULL);
    *out = browse_member_list;
    if (!(obj->file != NULL && obj->file->fileID != 0 && obj->file->recordbrowseinfo != 0 && obj->file2 != NULL &&
          obj->file2->fileID != 0 && obj->sourceoffset > 0)) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    if ((name = info->name) == NULL || CParser_IsNullOrAtOrDollarPrefixedName(name) != 0) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    InitGList(&browse_member_list, 0x4000);
    AppendGListByte(&browse_member_list, 2);
    AppendGListWord(&browse_member_list, obj->file->fileID);
    AppendGListWord(&browse_member_list, obj->file2->fileID);
    AppendGListLong(&browse_member_list, obj->sourceoffset - 1);
    CError_ASSERT(519, browse_member_list.size == 9);
    AppendGListLong(&browse_member_list, obj->sourceoffset - 1);
    AppendGListLong(&browse_member_list, 0);
    write_text_or_name_id(&browse_member_list, name->name, name->id);
    AppendGListWord(&browse_member_list, 0);
    AppendGListLong(&browse_member_list, 0);
    AppendGListByte(&browse_member_list, 0);
}

void CBrowse_WriteStructMember(StructMember *param0, SInt32 param1, SInt32 param2)
{
    SInt16 len;

    if (tk == ';')
        param2++;
    if (browse_member_list.data != NULL && param0 != NULL && param1 > 0 && param2 >= param1) {
        AppendGListByte(&browse_member_list, 1);
        AppendGListByte(&browse_member_list, 4);
        AppendGListLong(&browse_member_list, 0);
        AppendGListLong(&browse_member_list, param1 - 1);
        AppendGListLong(&browse_member_list, param2 - 1);
        len = (SInt16)strlen(param0->name->name);
        AppendGListWord(&browse_member_list, len);
        AppendGListData(&browse_member_list, param0->name->name, len + 1);
    }
}

void CBrowse_FlushAndRestoreMemberList(SInt32 value, GList *state)
{
    if (state == NULL) {
        CError_Internal("CBrowse.c", 556U);
    }
    if (browse_member_list.data != NULL) {
        if (value > 0 && browse_member_list.size > 0) {
            memcpy(*browse_member_list.data + 9, &value, sizeof(value));
            AppendGList(&data_00581ba8.buffer, &browse_member_list);
            AppendGListByte(&data_00581ba8.buffer, -1);
        }
        FreeGList(&browse_member_list);
    }
    browse_member_list = *state;
}

static void EmitStandardData(BrowseType browseType, SInt32 fileID1, SInt32 fileID2, SInt32 startOffset,
                             SInt32 endOffset, char *name, SInt32 nameID, char *qualname, SInt32 qualnameID)
{
    CError_ASSERT(582, name);
    AppendGListByte(&data_00581ba8.buffer, browseType);
    AppendGListWord(&data_00581ba8.buffer, fileID1);
    AppendGListWord(&data_00581ba8.buffer, fileID2);
    AppendGListLong(&data_00581ba8.buffer, startOffset - 1);
    AppendGListLong(&data_00581ba8.buffer, endOffset - 1);
    AppendGListLong(&data_00581ba8.buffer, 0);
    write_text_or_name_id(&data_00581ba8.buffer, name, nameID);
    if (qualname != NULL && qualname != name)
        write_text_or_name_id(&data_00581ba8.buffer, qualname, qualnameID);
    else
        AppendGListWord(&data_00581ba8.buffer, 0);
}

void CBrowse_WriteNameLineRange(NameSpace *names, HashNameNode *name, CPrepFileInfo *file, CPrepFileInfo *endFile,
                                int firstLine, int lastLine)
{
    char *qualifiedName;

    CError_ASSERT(616, file != NULL && file->recordbrowseinfo);
    if (endFile && endFile->fileID && firstLine > 0 && lastLine >= firstLine) {
        qualifiedName = CError_GetQualifiedName(names, name);
        EmitStandardData(BrowseType_Typedef, file->fileID, endFile->fileID, firstLine, lastLine, name->name, name->id,
                         qualifiedName, -1);
    }
}

void CBrowse_RecordNameRange(NameSpace *nameSpace, HashNameNode *hn, CPrepFileInfo *startRecord,
                             CPrepFileInfo *endRecord, SInt32 start, SInt32 end)
{
    char *qualifiedName;

    CError_ASSERT(630, startRecord != NULL && startRecord->recordbrowseinfo != 0);
    if (endRecord != NULL && endRecord->fileID != 0 && start > 0 && end >= start) {
        qualifiedName = CError_GetQualifiedName(nameSpace, hn);
        EmitStandardData(BrowseType_Enum, startRecord->fileID, endRecord->fileID, start, end, hn->name, hn->id,
                         qualifiedName, -1);
    }
}

void CBrowse_WriteRelatedRecord(NameSpace *nameSpace, HashNameNode *name, CPrepFileInfo *record,
                                CPrepFileInfo *relatedRecord, SInt32 first, SInt32 last)
{
    char *qualifiedName;

    CError_ASSERT(644, record != NULL && record->recordbrowseinfo != 0);
    if (tk == ',')
        last++;
    if (relatedRecord != NULL && relatedRecord->fileID != 0 && first > 0 && last >= first) {
        qualifiedName = CError_GetQualifiedName(nameSpace, name);
        EmitStandardData(BrowseType_Constant, record->fileID, relatedRecord->fileID, first, last, name->name, name->id,
                         qualifiedName, -1);
    }
}

static HashNameNode *GetLinkName(Object *obj)
{
    return COptimizer_GetFunctionObject(obj);
}

void write_function_browse_record(Object *obj, SInt32 fileNumber, SInt32 scopeNumber, SInt32 startLine, SInt32 endLine)
{
    char *alias;
    SInt32 flags;
    SInt32 functionId;
    char *displayName;
    char *className;
    Boolean needsDemangling;
    TypeMemberFunc *functionType;
    SInt32 aliasNameId;
    char demangledName[0x800];
    char destructorName[0x100];
    HashNameNode *functionName;
    SInt32 nameId;

    CError_ASSERT(738, obj->type != NULL && obj->type->type == TYPEFUNC);
    if (CParser_IsNullOrAtOrDollarPrefixedName(obj->name))
        return;
    functionType = (TypeMemberFunc *)obj->type;
    if ((functionType->flags & (FUNC_AUTO_GENERATED | FUNC_INTRINSIC)) != 0 && (scopeNumber == 0 || startLine < 0))
        return;
    if (obj->name->name[0] != '_' || obj->name->name[1] != '_') {
        displayName = obj->name->name;
        nameId = obj->name->id;
        switch (*displayName) {
            case '.':
                displayName++;
                nameId = -1;
                break;
            case '_':
                switch (displayName[1]) {
                    case '#':
                    case '%':
                    case '@':
                        displayName += 2;
                        nameId = -1;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    } else {
        needsDemangling = 1;
        if (functionType->flags & 0x6000) {
            className = functionType->theclass->classname->name;
            while (*className >= '0' && *className <= '9')
                className++;
            Unmangle_UnmangleName(className, demangledName, sizeof(demangledName));
            displayName = demangledName;
            className = strrchr(demangledName, ':');
            if (className != NULL)
                displayName = className + 1;
            if (functionType->flags & 0x4000) {
                destructorName[0] = '~';
                strncpy(destructorName + 1, displayName, sizeof(destructorName) - 1);
                displayName = destructorName;
            }
            needsDemangling = 0;
        }
        if (needsDemangling) {
            Unmangle_UnmangleSymbolName(obj->name->name, demangledName, sizeof(demangledName));
            displayName = demangledName;
        }
        nameId = -1;
    }
    while (*displayName >= '0' && *displayName <= '9') {
        displayName++;
        nameId = -1;
    }

    alias = NULL;
    aliasNameId = -1;
    functionName = GetLinkName(obj);
    if (obj->name != functionName) {
        alias = functionName->name;
        if (*alias == '.')
            alias++;
        else
            aliasNameId = functionName->id;
    }
    EmitStandardData(BrowseType_Function, fileNumber, scopeNumber, startLine, endLine, displayName, nameId, alias,
                     aliasNameId);
    flags = 0;
    if (obj->qual & Q_INLINE)
        flags |= 0x80;
    if (obj->qual & Q_PASCAL)
        flags |= 0x100;
    if (obj->qual & Q_ASM)
        flags |= 0x200;
    if (obj->sclass == TK_STATIC)
        flags |= 2;
    if (functionType->flags & FUNC_METHOD)
        flags |= 8;
    AppendGListLong(&data_00581ba8.buffer, flags);
    functionId = 0;
    if (functionType->flags & FUNC_METHOD) {
        functionId = functionType->funcid;
        if (functionId <= 0) {
            functionId = nextFunctionId++;
            functionType->funcid = functionId;
        }
    }
    AppendGListLong(&data_00581ba8.buffer, functionId);
}

void CBrowse_ForwardObjectFileRange(Object *object, CPrepFileInfo *browseFile, CPrepFileInfo *sourceFile,
                                    SInt32 startOffset, SInt32 endOffset)
{
    if (browseFile == NULL || browseFile->recordbrowseinfo == 0)
        CError_Internal("CBrowse.c", 0x378);
    if (sourceFile != NULL && sourceFile->fileID != 0 && startOffset > 0 && endOffset + 1 >= startOffset)
        write_function_browse_record(object, browseFile->fileID, sourceFile->fileID, startOffset, endOffset + 1);
}

void CBrowse_WriteObjectBrowseInfo(Object *object, CPrepFileInfo *metadata, CPrepFileInfo *endMetadata, SInt32 start,
                                   SInt32 end)
{
    char *alternateName = NULL;
    Boolean isFunction;
    SInt32 flags = 0;
    HashNameNode *linkname;
    SInt32 linkId;

    isFunction = is_const_object(object);
    CError_ASSERT(910, metadata != NULL && metadata->recordbrowseinfo != 0);
    CError_ASSERT(911, object != NULL);
    if (tk == ';')
        end++;
    if (endMetadata != NULL && endMetadata->fileID != 0 && start > 0 && end >= start) {
        linkname = GetLinkName(object);
        if (object->name != linkname)
            alternateName = linkname->name;
        linkId = linkname->id;
        EmitStandardData(isFunction ? BrowseType_Constant : BrowseType_Global, metadata->fileID, endMetadata->fileID,
                         start, end, object->name->name, object->name->id, alternateName, linkId);
        if (!isFunction) {
            if (object->sclass == TK_STATIC)
                flags |= 2;
            AppendGListLong(&data_00581ba8.buffer, flags);
        }
    }
}

void write_identifier_range_record(Macro *source, CPrepFileInfo *info, int first, int last)
{
    CError_ASSERT(949, info == NULL || (info->recordbrowseinfo != 0 && source->flag == 0));
    if (info != NULL && info->fileID != 0 && first > 0 && last >= first)
        EmitStandardData(BrowseType_Macro, info->fileID, info->fileID, first, last, source->name->name,
                         source->name->id, NULL, -1);
}

void CBrowse_RecordClassLocation(struct TypeClass *type, CPrepFileInfo *location, int first_line, int last_line)
{
    CError_ASSERT(963, location == NULL || location->recordbrowseinfo != 0);
    if (location != NULL && location->fileID != 0 && first_line > 0 && last_line >= first_line) {
        EmitStandardData(BrowseType_Template, location->fileID, location->fileID, first_line, last_line,
                         type->classname->name, type->classname->id, NULL, -1);
        AppendGListByte(&data_00581ba8.buffer, 0);
    }
}

void write_template_function_browse_record(TemplateFunction *info)
{
    CError_ASSERT(977, info->srcfile == NULL || browse_source(info)->recordbrowseinfo != 0);
    if (info->srcfile != NULL && browse_source(info)->fileID != 0 && info->startoffset > 0 &&
        info->endoffset >= info->startoffset) {
        EmitStandardData(BrowseType_Template, browse_source(info)->fileID, browse_source(info)->fileID,
                         info->startoffset, info->endoffset, info->name->name, info->name->id, NULL, -1);
        AppendGListByte(&data_00581ba8.buffer, 1);
    }
}

static void RecordUndefinedMemberFunctions(void)
{
    int index;
    int count;
    Object **objects;

    COS_LockHandleHi(browse_function_buffer.buffer.data);
    count = browse_function_buffer.objects.size / sizeof(Object *);
    objects = *browse_function_buffer.objects.data;
    for (index = 0; index < count; index++, objects++) {
        if ((*objects)->type->type == TYPEFUNC && (TYPE_FUNC((*objects)->type)->flags & FUNC_DEFINED) == 0)
            write_function_browse_record(*objects, 0, 0, -1, -1);
    }
    COS_UnlockHandle(browse_function_buffer.buffer.data);
}
