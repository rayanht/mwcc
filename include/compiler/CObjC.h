#ifndef COMPILER_COBJC_H
#define COMPILER_COBJC_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif
struct ObjCProtocolList {
    struct ObjCProtocolList *next; /* 0x00: create_protocol_list clears the emitted list link with memclrw */
    unsigned int count;            /* 0x04: create_protocol_list writes the endian-converted protocol count */
    struct Object
        *protocols[1]; /* 0x08: create_protocol_list sizes protocol slots and emits CObjC_GetProtocolInfo relocations */
};
struct CRec {
    struct CRec *next;
    struct HashNameNode *name;
    struct ObjectList *bases;
    struct MethRec *methods;
    struct Object *
        info; /* 0x10: CObjC_GetProtocolInfo caches the emitted protocol data object, falls back to fn_00509c40 and returns Object * */
};
#pragma options align = mac68k

#pragma options align = reset
#pragma pack(push, 1)
struct IvarEntry {
    SInt32 name;
    SInt32 typeEncoding;
    UInt32 offset;
};
#pragma pack(pop)
#pragma options align = mac68k
struct MessageArgument {
    struct MessageArgument *next;
    struct HashNameNode *name;
    struct ENode *expression;
};
#pragma options align = reset
struct MethRec {
    struct MethRec *next;
    struct Object *function;
    struct TypeFunc *ftype;
    struct HashEntry *selector;
    struct Type *rtype;
    UInt32 rqual;
    struct ObjCParameterNode *args;
    UInt8 isvararg;
    UInt8 isinst;
    UInt8 defined;
    UInt8
        alignmentPadding; /* 0x1f: CObjC_NewMemberNode clears the 0x20-byte record; no method operation accesses this tail padding */
};
struct ObjCDefinition {
    struct ObjCDefinition *next;
    union {
        struct Object *object;
        struct CRec *category;
    } identity;
    int value;
};
#pragma options align = mac68k

#pragma options align = reset
struct ObjCInfo {
    struct Object *classobject;
    struct Object *metaclassobject;
    struct Object *
        auxiliaryObject; /* 0x08: serialize_objc_info writes auxiliaryObject with write_object and relocates its reference. */
    struct MethRec *methods;
    struct ObjectList *protocols;
    struct CRec *vars;
};
struct ObjCParameterNode {
    struct ObjCParameterNode *next;
    struct HashNameNode *selectorName;
    struct HashNameNode *name;
    struct Type *type;
    UInt32 qual;
};
struct OLinkList {
    struct OLinkList *next; /* 0x00: CObjC_GetProtocolInfo links relocations */
    struct Object *obj;     /* 0x04: CObjC_GetProtocolInfo references class, name and method metadata objects */
    SInt32 offset;          /* 0x08: CObjC_GetProtocolInfo selects offsets 0, 4, 8, 12, 16 in protocol data */
    SInt32 addend;          /* 0x0c: CObjC_GetProtocolInfo initializes relocation addends to zero */
};
typedef struct ObjCMethodEntry {
    SInt32 selector;       /* 0x00: create_method_list_object selector relocation */
    SInt32 encoding;       /* 0x04: create_method_list_object type encoding relocation */
    SInt32 implementation; /* 0x08: create_method_list_object function relocation */
} ObjCMethodEntry;
typedef struct ObjCMethodList {
    SInt32 next;                /* 0x00: create_method_list_object memclrw clears header */
    UInt32 count;               /* 0x04: create_method_list_object endian-converts method count */
    ObjCMethodEntry methods[1]; /* 0x08: create_method_list_object iterates emitted entries */
} ObjCMethodList;
extern ENode *CObjC_ParseStringConstant(void);
extern ENode *CObjC_ParseEncodeExpression(void);
extern ENode *CObjC_MakeMessageSend(ENode *receiver, TypeClass *obj, MessageArgument *arguments, ENodeList *extraArgs,
                                    UInt8 mode, Boolean flag);
extern Boolean match_message_arguments(register MethRec *info, MessageArgument *b, Boolean flag);
extern void CObjC_ParseIdentifierList(void);
extern void CObjC_ParseProtocol(void);
extern void fn_00505cb0(void);
extern void fn_00505cc0(void);
extern void parse_class_interface_or_implementation(void);
extern void parse_ivars(TypeClass *classType, char checkExisting);
extern void parse_category_methods_and_check_defined(TypeClass *theclass);
extern void emit_classobject_and_metaclassobject(TypeClass *cls);
extern Object *CObjC_GetProtocolInfo(CRec *p);
extern Object *create_protocol_method_list(CRec *cls, char *nm, SInt16 val, UInt8 kind);
extern Object *create_protocol_list(ObjectList *entries, char *nameArg);
extern Object *create_method_list_object(TypeClass *owner, CRec *category, MethRec *methods, UInt8 *namePrefix,
                                         char *nameSuffix, UInt16 qualifiers, char methodKind);
extern Object *create_ivar_list(TypeClass *cls);
extern void CObjC_005074f0(Type *type, UInt32 qual, Boolean flag);
extern void parse_method_definition(TypeClass *object, CRec *kind, MethRec **methods);
extern void parse_category(TypeClass *a0);
extern Type *CObjC_ParseProtocolList(Type *type);
extern void encode_class(TypeClass *cls, Boolean flag);
extern Type *CObjC_ParseIdType(void);
extern void CObjC_005082b0(TypeStruct *classInfo);
extern void copy_ivars_to_struct(TypeStruct *type, TypeClass *cls);
extern ObjectList *parse_protocol_list(void);
extern TypeFunc *get_method_ftype(MethRec *spec);
extern HashNameNode *CObjC_00508810(TypeClass *obj, CRec *ns, MethRec *info);
extern MethRec *fn_00508940(MethRec *methods, MethRec *method, char check_types, char allow_missing);
extern MethRec *parse_method_declaration(char useGlobalAllocation);
extern void CObjC_ConvertKeywordToIdentifier(void);
extern TypeClass *find_or_create_objc_class(HashNameNode *name);
extern Boolean CObjC_IsIdOrSelType(Type *ty);
extern Type *CObjC_GetIdType(Boolean required);
extern void emit_method_type_encoding(MethRec *p, int b);
extern ENode *CObjC_ParseMessageExpression(void);
extern void create_category_definition(TypeClass *a0, CRec *pb);
extern Boolean CObjC_IsIdCompatiblePointerPair(Type *t1, Type *t2);
extern struct TypeClass *data_00587140;
extern struct HashNameNode *this_self_name;
extern struct Type *class_pointer_type;
extern struct Type *id_type;
extern UInt8 data_00588507;
extern struct NameSpace *cscope_root;
extern CRec *data_00588064;
struct CRec;

#ifdef __cplusplus
}
#endif

#endif
