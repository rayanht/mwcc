#define CERROR_FILE "CBrowse.c"
#include "compiler/common.h"
#include "compiler/CBrowse.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "compiler/Unmangle.h"
#include "driver/CLPluginRequests.h"
#include "driver/COSToolsCLT.h"
#include "driver/CWParserPluginsPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/DropInCompilerLinkerPrivate.h"
#include "driver/Files.h"
#include "driver/Memory.h"
#include <string.h>

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

static inline PFile *browse_source(TemplateFunction *info)
{
    return info->srcfile;
}

void write_template_function_browse_record(TemplateFunction *info)
{
    SInt16 sourceCount;
    SInt32 startFileId, endFileId;
    SInt32 startOffset, endOffset;
    char *name;
    SInt32 nameId;

    if (info->srcfile != NULL && browse_source(info)->recordbrowseinfo == 0)
        CError_FATAL(977);

    if (info->srcfile != NULL && browse_source(info)->fileID != 0 && info->startoffset > 0 &&
        info->endoffset >= info->startoffset) {
        startOffset = info->startoffset;
        endOffset = info->endoffset;
        nameId = info->name->id;
        name = info->name->name;
        sourceCount = browse_source(info)->fileID;
        endFileId = sourceCount;
        startFileId = sourceCount;
        CError_ASSERT(582, name != 0);
        AppendGListByte(&data_00581ba8.buffer, 7);
        AppendGListWord(&data_00581ba8.buffer, startFileId);
        AppendGListWord(&data_00581ba8.buffer, endFileId);
        AppendGListLong(&data_00581ba8.buffer, startOffset - 1);
        AppendGListLong(&data_00581ba8.buffer, endOffset - 1);
        AppendGListLong(&data_00581ba8.buffer, 0);
        write_text_or_name_id(&data_00581ba8.buffer, name, nameId);
        AppendGListWord(&data_00581ba8.buffer, 0);
        AppendGListByte(&data_00581ba8.buffer, 1);
    }
}

void CBrowse_RecordClassLocation(struct TypeClass *type, PFile *location, int first_line, int last_line)
{
    short file_id;
    int file_start;
    int file_end;
    char *name;
    char *name_check;
    int name_value;
    BROWSE_ASSERT(location != 0 && location->recordbrowseinfo == 0, CError_Internal("CBrowse.c", 963));
    if (location != NULL) {
        if ((file_id = location->fileID) != 0 && first_line > 0 && last_line >= first_line) {
            name_value = type->classname->id;
            name = name_check = type->classname->name;
            file_end = file_start = (int)file_id;
            BROWSE_ASSERT(name_check == 0, CError_Internal("CBrowse.c", 582));
            AppendGListByte(&data_00581ba8.buffer, 7);
            AppendGListWord(&data_00581ba8.buffer, file_start);
            AppendGListWord(&data_00581ba8.buffer, file_end);
            AppendGListLong(&data_00581ba8.buffer, first_line - 1);
            AppendGListLong(&data_00581ba8.buffer, last_line - 1);
            AppendGListLong(&data_00581ba8.buffer, 0);
            write_text_or_name_id(&data_00581ba8.buffer, name, name_value);
            AppendGListWord(&data_00581ba8.buffer, 0);
            AppendGListByte(&data_00581ba8.buffer, 0);
        }
    }
}

void write_identifier_range_record(Macro *source, PFile *info, int first, int last)
{
    short identifier;
    int firstIdentifier;
    int secondIdentifier;
    char *payload;
    char *checkedPayload;
    int recordValue;
    BROWSE_ASSERT(info != 0 && (info->recordbrowseinfo == 0 || source->flag != 0), CError_Internal("CBrowse.c", 949));
    if (info != NULL) {
        if ((identifier = info->fileID) != 0 && first > 0 && last >= first) {
            recordValue = source->name->id;
            payload = checkedPayload = source->name->name;
            secondIdentifier = firstIdentifier = (int)identifier;
            BROWSE_ASSERT(checkedPayload == 0, CError_Internal("CBrowse.c", 582));
            AppendGListByte(&data_00581ba8.buffer, 3);
            AppendGListWord(&data_00581ba8.buffer, firstIdentifier);
            AppendGListWord(&data_00581ba8.buffer, secondIdentifier);
            AppendGListLong(&data_00581ba8.buffer, first - 1);
            AppendGListLong(&data_00581ba8.buffer, last - 1);
            AppendGListLong(&data_00581ba8.buffer, 0);
            write_text_or_name_id(&data_00581ba8.buffer, payload, recordValue);
            AppendGListWord(&data_00581ba8.buffer, 0);
        }
    }
}

/* The assert file name. */
/* 0x563344, "CBrowse.c" */

static inline void browse_write(char *pq, SInt32 id1, char *alt, SInt32 id2, int n2, int n3, SInt32 a4, SInt32 a5,
                                Boolean b)
{
    CError_ASSERT(582, pq != NULL);
    AppendGListByte(&data_00581ba8.buffer, b ? (UInt8)6 : (UInt8)1);
    AppendGListWord(&data_00581ba8.buffer, n2);
    AppendGListWord(&data_00581ba8.buffer, n3);
    AppendGListLong(&data_00581ba8.buffer, a4 - 1);
    AppendGListLong(&data_00581ba8.buffer, a5 - 1);
    AppendGListLong(&data_00581ba8.buffer, 0);
    write_text_or_name_id(&data_00581ba8.buffer, pq, id1);
    if (!(alt == NULL || alt == pq)) {
        write_text_or_name_id(&data_00581ba8.buffer, alt, id2);
    } else {
        AppendGListWord(&data_00581ba8.buffer, 0);
    }
}

void CBrowse_WriteObjectBrowseInfo(Object *object, PFile *metadata, PFile *endMetadata, SInt32 start, SInt32 end)
{
    char *alternateName = NULL;
    Boolean isFunction;
    SInt32 flags = 0;
    HashNameNode *linkname;
    SInt32 linkId;
    SInt32 nameId;
    char *name;
    SInt32 startValue;
    SInt32 endValue;

    isFunction = is_const_object(object);
    CError_ASSERT(910, metadata != NULL && metadata->recordbrowseinfo != 0);
    CError_ASSERT(911, object != NULL);
    if (tk == ';')
        end++;
    if (endMetadata != NULL && endMetadata->fileID != 0 && start > 0 && end >= start) {
        linkname = COptimizer_GetFunctionObject(object);
        if (object->name != linkname)
            alternateName = linkname->name;
        linkId = linkname->id;
        nameId = object->name->id;
        name = object->name->name;
        browse_write(name, nameId, alternateName, linkId, startValue = metadata->fileID, endValue = endMetadata->fileID,
                     start, end, isFunction);
        if (!isFunction) {
            if (object->sclass == TK_STATIC)
                flags |= 2;
            AppendGListLong(&data_00581ba8.buffer, flags);
        }
    }
}

void CBrowse_ForwardObjectFileRange(Object *object, PFile *browseFile, PFile *sourceFile, SInt32 startOffset,
                                    SInt32 endOffset)
{
    if (browseFile == NULL || browseFile->recordbrowseinfo == 0)
        CError_Internal("CBrowse.c", 0x378);
    if (sourceFile != NULL && sourceFile->fileID != 0 && startOffset > 0 && endOffset + 1 >= startOffset)
        write_function_browse_record(object, browseFile->fileID, sourceFile->fileID, startOffset, endOffset + 1);
}

void write_function_browse_record(Object *obj, SInt32 fileNumber, SInt32 scopeNumber, SInt32 startLine, SInt32 endLine)
{
    SInt32 flags;
    SInt32 functionId;
    char *displayName;
    char *className;
    char *alias;
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
    functionName = COptimizer_GetFunctionObject(obj);
    if (obj->name != functionName) {
        alias = functionName->name;
        if (*alias == '.')
            alias++;
        else
            aliasNameId = functionName->id;
    }
    CError_ASSERT(582, displayName != NULL);
    AppendGListByte(&data_00581ba8.buffer, 0);
    AppendGListWord(&data_00581ba8.buffer, fileNumber);
    AppendGListWord(&data_00581ba8.buffer, scopeNumber);
    AppendGListLong(&data_00581ba8.buffer, startLine - 1);
    AppendGListLong(&data_00581ba8.buffer, endLine - 1);
    AppendGListLong(&data_00581ba8.buffer, 0);
    write_text_or_name_id(&data_00581ba8.buffer, displayName, nameId);
    if (alias != NULL && alias != displayName)
        write_text_or_name_id(&data_00581ba8.buffer, alias, aliasNameId);
    else
        AppendGListWord(&data_00581ba8.buffer, 0);
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

void CBrowse_WriteRelatedRecord(NameSpace *nameSpace, HashNameNode *name, PFile *record, PFile *relatedRecord,
                                SInt32 first, SInt32 last)
{
    char *qualifiedName;
    SInt32 nameLength;
    SInt32 relatedValue;
    SInt32 recordValue;

    CError_ASSERT(644, record != NULL && record->recordbrowseinfo != 0);
    if (tk == ',')
        last++;
    if (relatedRecord != NULL && relatedRecord->fileID != 0 && first > 0 && last >= first) {
        qualifiedName = CError_GetQualifiedName(nameSpace, name);
        nameLength = name->id;
        relatedValue = relatedRecord->fileID;
        recordValue = record->fileID;
        CError_ASSERT(582, name->name);
        AppendGListByte(&data_00581ba8.buffer, 6);
        AppendGListWord(&data_00581ba8.buffer, recordValue);
        AppendGListWord(&data_00581ba8.buffer, relatedValue);
        AppendGListLong(&data_00581ba8.buffer, first - 1);
        AppendGListLong(&data_00581ba8.buffer, last - 1);
        AppendGListLong(&data_00581ba8.buffer, 0);
        write_text_or_name_id(&data_00581ba8.buffer, name->name, nameLength);
        if (qualifiedName != NULL && qualifiedName != name->name)
            write_text_or_name_id(&data_00581ba8.buffer, qualifiedName, -1);
        else
            AppendGListWord(&data_00581ba8.buffer, 0);
    }
}

void CBrowse_RecordNameRange(NameSpace *nameSpace, HashNameNode *hn, PFile *startRecord, PFile *endRecord, SInt32 start,
                             SInt32 end)
{
    char *qualifiedName;
    SInt32 nameID;
    SInt32 endIndex;
    SInt32 startIndex;
    if (startRecord == NULL || startRecord->recordbrowseinfo == 0) {
        CError_Internal("CBrowse.c", 0x276);
    }
    if (endRecord != NULL && endRecord->fileID != 0 && start > 0 && end >= start) {
        qualifiedName = CError_GetQualifiedName(nameSpace, hn);
        nameID = hn->id;
        endIndex = endRecord->fileID;
        startIndex = startRecord->fileID;
        if (hn->name == NULL) {
            CError_Internal("CBrowse.c", 0x246);
        }
        AppendGListByte(&data_00581ba8.buffer, 4);
        AppendGListWord(&data_00581ba8.buffer, startIndex);
        AppendGListWord(&data_00581ba8.buffer, endIndex);
        AppendGListLong(&data_00581ba8.buffer, start - 1);
        AppendGListLong(&data_00581ba8.buffer, end - 1);
        AppendGListLong(&data_00581ba8.buffer, 0);
        write_text_or_name_id(&data_00581ba8.buffer, hn->name, nameID);
        if (qualifiedName != NULL && qualifiedName != hn->name) {
            write_text_or_name_id(&data_00581ba8.buffer, qualifiedName, -1);
        } else {
            AppendGListWord(&data_00581ba8.buffer, 0);
        }
    }
}

static inline void writeBrowseLine(GList *stream, unsigned int line)
{
    AppendGListLong(stream, line);
}

static inline void reportBrowseError(unsigned int line)
{
    CError_Internal("CBrowse.c", line);
}

static inline void writeBrowseRecordKind(GList *stream, SInt8 kind)
{
    AppendGListByte(stream, kind);
}

void CBrowse_WriteNameLineRange(NameSpace *names, HashNameNode *name, PFile *file, PFile *endFile, int firstLine,
                                int lastLine)
{
    char *qualifiedName;
    int nameId;
    int endFileId;
    int fileId;

    if (!file || !file->recordbrowseinfo)
        reportBrowseError(616);

    if (endFile && endFile->fileID && firstLine > 0 && lastLine >= firstLine) {
        qualifiedName = CError_GetQualifiedName(names, name);
        nameId = name->id;
        endFileId = endFile->fileID;
        fileId = file->fileID;
        if (!name->name)
            reportBrowseError(582);

        writeBrowseRecordKind(&data_00581ba8.buffer, 5);
        AppendGListWord(&data_00581ba8.buffer, fileId);
        AppendGListWord(&data_00581ba8.buffer, endFileId);
        writeBrowseLine(&data_00581ba8.buffer, firstLine - 1);
        writeBrowseLine(&data_00581ba8.buffer, lastLine - 1);
        writeBrowseLine(&data_00581ba8.buffer, 0);
        write_text_or_name_id(&data_00581ba8.buffer, name->name, nameId);
        if (qualifiedName && qualifiedName != name->name)
            write_text_or_name_id(&data_00581ba8.buffer, qualifiedName, -1);
        else
            AppendGListWord(&data_00581ba8.buffer, 0);
    }
}

void CBrowse_FlushAndRestoreMemberList(SInt32 value, GList *state)
{
    unsigned int offset;
    if (state == NULL) {
        CError_Internal("CBrowse.c", 556U);
    }
    if (browse_member_list.data != NULL) {
        if (value > 0 && browse_member_list.size > 0) {
            memcpy(*browse_member_list.data + 9, &value, sizeof(value));
            offset = data_00581ba8.buffer.size;
            AppendGListNoData(&data_00581ba8.buffer, browse_member_list.size);
            memcpy(*data_00581ba8.buffer.data + offset, *browse_member_list.data, browse_member_list.size);
            AppendGListByte(&data_00581ba8.buffer, -1);
        }
        FreeGList(&browse_member_list);
    }
    browse_member_list = *state;
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
        CompilerTools_AppendGListData(&browse_member_list, param0->name->name, len + 1);
    }
}

void CBrowse_BuildTypeStructBrowseInfo(DeclInfo *obj, TypeStruct *info, GList *out)
{
    HashNameNode *name;

    CError_ASSERT(478, obj != NULL && out != NULL);
    *out = browse_member_list;
    if (!(obj->browseFile != NULL && obj->browseFile->fileID != 0 && obj->browseFile->recordbrowseinfo != 0 &&
          obj->sourceFile != NULL && obj->sourceFile->fileID != 0 && obj->sourceLine > 0)) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    if ((name = info->name) == NULL || CParser_IsNullOrAtOrDollarPrefixedName(name) != 0) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    InitGList(&browse_member_list, 0x4000);
    AppendGListByte(&browse_member_list, 2);
    AppendGListWord(&browse_member_list, obj->browseFile->fileID);
    AppendGListWord(&browse_member_list, obj->sourceFile->fileID);
    AppendGListLong(&browse_member_list, obj->sourceLine - 1);
    CError_ASSERT(519, browse_member_list.size == 9);
    AppendGListLong(&browse_member_list, obj->sourceLine - 1);
    AppendGListLong(&browse_member_list, 0);
    write_text_or_name_id(&browse_member_list, name->name, name->id);
    AppendGListWord(&browse_member_list, 0);
    AppendGListLong(&browse_member_list, 0);
    AppendGListByte(&browse_member_list, 0);
}

void CBrowse_RestoreScope(SInt32 statementOffset, GList *savedScope)
{
    UInt32 outputOffset;

    if (savedScope == NULL)
        CError_Internal("CBrowse.c", 451U);
    if (browse_member_list.data != NULL) {
        if (browse_member_list.size > 0) {
            if (tk == ';')
                statementOffset++;
            memcpy(*browse_member_list.data + 9, &statementOffset, sizeof(statementOffset));
            outputOffset = data_00581ba8.buffer.size;
            AppendGListNoData(&data_00581ba8.buffer, browse_member_list.size);
            memcpy((*data_00581ba8.buffer.data + outputOffset), *browse_member_list.data, browse_member_list.size);
            AppendGListByte(&data_00581ba8.buffer, -1);
        }
        FreeGList(&browse_member_list);
    }
    browse_member_list = *savedScope;
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
        CompilerTools_AppendGListData(&browse_member_list, obj->name->name, len + 1);
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
        CompilerTools_AppendGListData(&browse_member_list, rec->name->name, len + 1);
    }
}

void CBrowse_GenerateClassRecord(DeclInfo *record, GList *out)
{
    HashNameNode *name;
    Type *baseType;
    TypeClassExt800 *base;
    SInt32 baseNameID;
    ClassList *baseList;
    char *baseName;
    char *className;
    SInt32 baseCount;

    CError_ASSERT(225, record && record->dtype && out);
    *out = browse_member_list;
    if (!(record->browseFile && record->browseFile->fileID && record->browseFile->recordbrowseinfo &&
          record->sourceFile && record->sourceFile->fileID && record->sourceLine > 0)) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    if (CParser_IsNullOrAtOrDollarPrefixedName(TYPE_CLASS(record->dtype)->classname) != 0) {
        memclrw(&browse_member_list, sizeof(browse_member_list));
        return;
    }
    InitGList(&browse_member_list, 0x4000);
    AppendGListByte(&browse_member_list, 2);
    AppendGListWord(&browse_member_list, record->browseFile->fileID);
    AppendGListWord(&browse_member_list, record->sourceFile->fileID);
    AppendGListLong(&browse_member_list, record->sourceLine - 1);
    CError_ASSERT(268, browse_member_list.size == 9);
    AppendGListLong(&browse_member_list, record->sourceLine - 1);
    AppendGListLong(&browse_member_list, 0);
    name = TYPE_CLASS(record->dtype)->classname;
    write_text_or_name_id(&browse_member_list, name->name, name->id);
    fn_004c2ac0(record->dtype, 0);
    AppendGListByte(&data_00583548, 0);
    className = CompilerTools_AllocatePool(data_00583548.size + 1);
    strcpy(className, *data_00583548.data);
    while (*className != 0 && *className >= '0' && *className <= '9')
        className++;
    if (strcmp(TYPE_CLASS(record->dtype)->classname->name, className) != 0)
        write_text_or_name_id(&browse_member_list, className, -1);
    else
        AppendGListWord(&browse_member_list, 0);
    AppendGListLong(&browse_member_list, 0);
    baseCount = 0;
    for (baseList = TYPE_CLASS(record->dtype)->bases; baseList != NULL; baseList = baseList->next)
        baseCount++;
    AppendGListByte(&browse_member_list, baseCount);
    for (baseList = TYPE_CLASS(record->dtype)->bases; baseList != NULL; baseList = baseList->next) {
        AppendGListByte(&browse_member_list, data_00563340[baseList->access]);
        AppendGListByte(&browse_member_list, baseList->is_virtual);
        base = (TypeClassExt800 *)baseList->base;
        if ((base->base.flags & CLASS_IS_TEMPL_INST) && base->suppressImplicitInstantiation == 0)
            baseType = base->classTemplate;
        else
            baseType = TYPE(base);
        fn_004c2ac0(baseType, 0);
        AppendGListByte(&data_00583548, 0);
        baseName = CompilerTools_AllocatePool(data_00583548.size + 1);
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
            CompilerTools_AppendGListData(output, text, length + 1U);
    }
}

void CBrowse_FreeLists(struct CPrepCU *cu)
{
    FreeGList(&data_00581ba8.buffer);
    FreeGList(&browse_member_list);
    FreeGList(&browse_function_buffer.buffer);
}

void CBrowse_StoreBrowseData(CPrepCU *arguments)
{
    Object **objects;
    int count;
    int index;
    long result;

    CError_ASSERT(149, arguments != NULL);

    if (data_00581ba8.buffer.size >= 0x4cU) {
        fn_004431a0(browse_function_buffer.buffer.data);
        count = browse_function_buffer.objects.size / sizeof(Object *);
        objects = *browse_function_buffer.objects.data;
        for (index = 0; index < count; index++, objects++) {
            if ((*objects)->type->type == TYPEFUNC && (TYPE_FUNC((*objects)->type)->flags & FUNC_DEFINED) == 0)
                write_function_browse_record(*objects, 0, 0, -1, -1);
        }
        fn_004431b0(browse_function_buffer.buffer.data);
        AppendGListByte(&data_00581ba8.buffer, -1);
        fn_00443170(data_00581ba8.handle, data_00581ba8.buffer.size);
        if (fn_0041bcb0(arguments->context, data_00581ba8.handle, &result) == 0) {
            arguments->browseData = result;
            data_00581ba8.buffer.data = NULL;
        }
    }
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
    CompilerTools_AppendGListData(&data_00581ba8.buffer, &header, 76U);
}
