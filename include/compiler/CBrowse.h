#ifndef COMPILER_CBROWSE_H
#define COMPILER_CBROWSE_H

#include "compiler/common.h"
#include "compiler/CompilerTools.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
union BrowseObjectBuffer {
    GList buffer;
    struct {
        struct Object ***data;
        SInt32 size;
        SInt32 hndlsize;
        SInt32 growsize;
    } objects;
};
#pragma options align = reset
#pragma options align = mac68k
struct BrowseStreamHeader {
    unsigned int magic;
    unsigned int version;
    unsigned short language;
    unsigned short valueA;
    unsigned int valueC;
    unsigned char reserved[60];
};
#pragma options align = reset
extern void write_template_function_browse_record(TemplateFunction *info);
extern void CBrowse_RecordClassLocation(struct TypeClass *type, CPrepFileInfo *location, int first_line, int last_line);
extern void write_identifier_range_record(Macro *source, CPrepFileInfo *info, int first, int last);
extern void CBrowse_WriteObjectBrowseInfo(Object *object, CPrepFileInfo *metadata, CPrepFileInfo *endMetadata,
                                          SInt32 start, SInt32 end);
extern void CBrowse_ForwardObjectFileRange(Object *arg0, CPrepFileInfo *b, CPrepFileInfo *c, SInt32 n, SInt32 m);
extern void write_function_browse_record(Object *obj, SInt32 fileNumber, SInt32 scopeNumber, SInt32 startLine,
                                         SInt32 endLine);
extern void CBrowse_WriteRelatedRecord(NameSpace *nameSpace, HashNameNode *name, CPrepFileInfo *record,
                                       CPrepFileInfo *relatedRecord, SInt32 first, SInt32 last);
extern void CBrowse_RecordNameRange(NameSpace *nameSpace, HashNameNode *hn, CPrepFileInfo *startRecord,
                                    CPrepFileInfo *endRecord, SInt32 start, SInt32 end);
extern void CBrowse_WriteStructMember(StructMember *param0, SInt32 param1, SInt32 param2);
extern void CBrowse_BuildTypeStructBrowseInfo(DeclInfo *obj, TypeStruct *info, GList *out);
extern void CBrowse_RecordDataObject(Object *obj, SInt32 param2, SInt32 param3);
extern void CBrowse_RecordFunction(Object *obj, SInt32 start, SInt32 end);
extern void CBrowse_WriteObjMemberVar(ObjMemberVar *rec, SInt32 start, SInt32 end);
extern void CBrowse_GenerateClassRecord(DeclInfo *record, GList *out);
extern void write_text_or_name_id(GList *output, char *text, int index);
extern void CBrowse_FreeLists(struct CPrepCU *cu);
extern void CBrowse_FlushAndRestoreMemberList(SInt32 value, GList *state);
extern void CBrowse_RestoreScope(SInt32 statementOffset, GList *savedScope);
extern void CBrowse_WriteNameLineRange(NameSpace *names, HashNameNode *name, CPrepFileInfo *file,
                                       CPrepFileInfo *endFile, int firstLine, int lastLine);
extern void CBrowse_StoreBrowseData(CPrepCU *arguments);
extern void CBrowse_InitBrowseData(CPrepCU *classes);
extern UInt8 data_005884f5;

#ifdef __cplusplus
}
#endif

#endif
