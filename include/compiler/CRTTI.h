#ifndef COMPILER_CRTTI_H
#define COMPILER_CRTTI_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DataReference {
    struct DataReference *next;
    Object *target;
    SInt32 offset;
    SInt32 value0c;
};
#pragma options align = mac68k
struct RTTIBaseRecord {
    SInt32 typeReference;
    SInt32 offset;
};
struct RTTIBaseGroup {
    RTTIBaseRecord base;
    SInt32 count;
};
#pragma options align = reset
struct RData {
    struct RData *next;
    Object *key;
    SInt32 vtableOffset;
    SInt32 zero;
};
struct RTTIOffsetEntry {
    SInt32 typeReference;
    SInt32 offset;
};
struct RTTIVTableOffsetNode {
    struct RTTIVTableOffsetNode *next;
    SInt32 vtableOffset;
};
struct RecBaseEntry {
    struct RecBaseEntry *next;
    struct Type *base;
    SInt32 offset;
};
struct VBasePath {
    struct VBasePath *next;
    struct TypeClass *theclass;
    struct RecBaseEntry *children;
    SInt32 offset;
    SInt16 count;
    Boolean isPrivate;
    Boolean isAmbiguous;
};
extern void build_rtti_offset_table(TypeClass *rootClass, TypeClass *cls, Object *key, unsigned char *offsetTable,
                                    int objectOffset, int vtableOffset);
extern ENode *CRTTI_ParseConstCast(void);
extern ENode *CRTTI_ReinterpretCast(void);
extern ENode *CRTTI_ParseExplicitTypecast(void);
extern ENode *explicit_typecast(register ENode *expr, register Type *type, UInt32 flags, char mode);
extern Boolean types_equal(Type *t1, Type *t2);
extern void check_constness_casted_away(Type *p, int b, Type *q, int d);
extern ENode *CRTTI_ParseTypeid(void);
extern RData *CRTTI_BuildRTTIOffsetTable(TypeClass *p1, unsigned char *p2, RData *p3);
extern Object *get_or_create_type_object(Type *type, SInt32 arg2);
extern void collect_public_bases(TypeClass *object, VBasePath *baseList, TypeClass *typeClass, SInt32 offset,
                                 Boolean recurse);
extern struct VBasePath *build_vbase_path_list(TypeClass *from, TypeClass *cls, struct VBasePath *list, SInt32 off,
                                               Boolean flag);
extern void CRTTI_OrBitVector(UInt32 *destination, const UInt32 *source, unsigned int bitCount);
extern void CRTTI_FillWords(UInt32 *words, int bitCount, unsigned int value);
extern ENode *CRTTI_ParseDynamicCast(void);
extern ENode *parse_cast_type_and_expression(DeclInfo *typeSpec);
extern void *create_rtti_base_records(TypeClass *param);
extern void CRTTI_IntersectBitVectors(UInt32 *dst, UInt32 *src, SInt32 nbits);

#ifdef __cplusplus
}
#endif

#endif
