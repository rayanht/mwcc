#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "compiler/CRTTI.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Switch.h"
#include "driver/Files.h"

/* One entry in the RTTI offset table. */
#include "compiler/ENode.h"
#include "compiler/Types.h"
#include <string.h>

static struct RTTIVTableOffsetNode *rtti_vtable_offset_list;
static struct RData *rtti_offset_table_head;

static inline Boolean IsSameType(Type *a, Type *b)
{
    for (;;) {
        if (a->type != b->type)
            return 0;
        switch ((SInt8)a->type) {
            case TYPEVOID:
                return 1;
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPESTRUCT:
                return a == b;
            case TYPEPOINTER:
                a = TPTR_TARGET(a);
                b = TPTR_TARGET(b);
                continue;
            default:
                return iscpp_typeequal(a, b);
        }
    }
}

static inline Boolean IsZero(CInt64 *v)
{
    return v->hi == 0 && v->lo == 0;
}

void CRTTI_FillWords(UInt32 *words, int bitCount, unsigned int value)
{
    int wordCount;

    for (wordCount = (bitCount + 31) >> 5; wordCount != 0; --wordCount) {
        *words = value;
        ++words;
    }
}

void CRTTI_IntersectBitVectors(UInt32 *dst, UInt32 *src, SInt32 nbits)
{
    int wordCount;
    wordCount = (nbits + 31) >> 5;
    while (wordCount != 0) {
        *dst++ &= *src++;
        wordCount = wordCount - 1;
    }
}

void CRTTI_OrBitVector(UInt32 *destination, const UInt32 *source, unsigned int bitCount)
{
    UInt32 word;
    const UInt32 *sourceWord;
    UInt32 *destinationWord;
    UInt32 wordCount;

    word = bitCount;
    word += 31U;
    destinationWord = destination;
    word = (int)word >> 5;
    sourceWord = source;
    wordCount = word;
    if (word != 0U) {
        do {
            word = *sourceWord;
            *destinationWord |= word;
            ++sourceWord;
            ++destinationWord;
        } while (--wordCount);
    }
}

struct VBasePath *build_vbase_path_list(TypeClass *from, TypeClass *cls, struct VBasePath *list, SInt32 offset,
                                        Boolean isPrivate)
{
    ClassList *base;
    SInt32 baseOffset;
    SInt32 baseIsPrivate;
    VBasePath *path;
    Boolean isAmbiguous;

    if (from != cls) {
        isAmbiguous = 0;
        path = list;
        if (path != NULL) {
            do {
                if (path->theclass == cls) {
                    if (path->offset == offset) {
                        if (!isPrivate) {
                            path->isPrivate = 0;
                        }
                        isAmbiguous = 0;
                    } else {
                        path->isAmbiguous = 1;
                        isAmbiguous = 1;
                    }
                    break;
                }
                path = path->next;
            } while (path != NULL);
        }
        if (path == NULL || isAmbiguous) {
            path = (VBasePath *)CompilerTools_AllocatePool(sizeof(*path));
            memclrw(path, sizeof(*path));
            path->next = list;
            list = path;
            path->theclass = cls;
            path->offset = offset;
            path->isPrivate = isPrivate;
            path->isAmbiguous = isAmbiguous;
        }
    }

    for (base = cls->bases; base != NULL; base = base->next) {
        if (!base->is_virtual) {
            baseOffset = offset + base->offset;
        } else {
            baseOffset = CClass_FindVBaseOffset(from, base->base);
        }
        baseIsPrivate = 1;
        if (!isPrivate && base->access != ACCESSPRIVATE) {
            baseIsPrivate = 0;
        }
        list = build_vbase_path_list(from, base->base, list, baseOffset, baseIsPrivate);
    }
    return list;
}

void collect_public_bases(TypeClass *object, VBasePath *baseList, TypeClass *typeClass, SInt32 offset, Boolean recurse)
{
    RecBaseEntry *entry;
    ClassList *base;

    if (baseList->theclass != typeClass) {
        entry = baseList->children;
        while (entry != NULL) {
            if (entry->base == (Type *)typeClass && entry->offset == offset)
                break;
            entry = entry->next;
        }
        if (entry == NULL) {
            entry = (RecBaseEntry *)CompilerTools_AllocatePool(sizeof(RecBaseEntry));
            entry->next = baseList->children;
            baseList->children = entry;
            entry->base = (Type *)typeClass;
            entry->offset = offset;
            baseList->count++;
        }
    }

    for (base = typeClass->bases; base != NULL; base = base->next) {
        SInt32 baseOffset;

        if (base->access != ACCESSPUBLIC)
            continue;
        if (base->is_virtual) {
            if (!recurse)
                continue;
            baseOffset = CClass_FindVBaseOffset(object, base->base);
        } else {
            baseOffset = offset + base->offset;
        }
        collect_public_bases(object, baseList, base->base, baseOffset, recurse);
    }
}

void *create_rtti_base_records(TypeClass *theclass)
{
    Boolean isPrivate;
    SInt16 publicCount;
    VBasePath *path;
    SInt16 groupCount;
    RTTIBaseRecord *record;
    char *buffer;
    Object *object;
    RecBaseEntry *child;
    DataReference *reference, *references;
    VBasePath *paths;
    SInt32 size;
    SInt16 childCount;
    paths = build_vbase_path_list(theclass, theclass, NULL, 0, 0);
    if (paths == NULL)
        return NULL;
    publicCount = 0;
    groupCount = 0;
    childCount = 0;
    for (path = paths; path != NULL; path = path->next) {
        if (path->isAmbiguous || path->isPrivate) {
            isPrivate = !path->isAmbiguous;
            collect_public_bases(theclass, path, path->theclass, path->offset, isPrivate);
            if (path->count) {
                groupCount++;
                childCount += path->count;
            }
        } else {
            publicCount++;
        }
    }
    if (publicCount == 0 && groupCount == 0)
        return NULL;
    size = (publicCount + childCount) * sizeof(RTTIBaseRecord) + groupCount * sizeof(struct RTTIBaseGroup) +
           sizeof(SInt32);
    buffer = CompilerTools_AllocatePool(size);
    memclrw(buffer, size);
    object = CParser_NewCompilerDefDataObject();
    object->name = CParser_GetUniqueName();
    object->type = CDecl_NewStructType(size, 4);
    references = NULL;
    object->sclass = TK_STATIC;
    record = (RTTIBaseRecord *)buffer;
    if (publicCount != 0) {
        for (path = paths; path != NULL; path = path->next) {
            if (path->isPrivate == 0 && path->isAmbiguous == 0) {
                reference = CompilerTools_AllocatePool(sizeof(DataReference));
                reference->next = references;
                references = reference;
                reference->target = get_or_create_type_object((Type *)path->theclass, 0);
                reference->offset = (char *)record - buffer;
                reference->value0c = 0;
                record->offset = CTool_EndianConvertWord32(path->offset);
                record++;
            }
        }
    }
    if (groupCount != 0) {
        for (path = paths; path != NULL; path = path->next) {
            if (path->count != 0) {
                struct RTTIBaseGroup *group;
                reference = CompilerTools_AllocatePool(sizeof(DataReference));
                reference->next = references;
                references = reference;
                reference->target = get_or_create_type_object((Type *)path->theclass, 0);
                reference->offset = (char *)record - buffer;
                reference->value0c = 0;
                group = (struct RTTIBaseGroup *)(buffer + ((char *)record - buffer));
                group->base.offset = CTool_EndianConvertWord32(path->offset | 0x80000000);
                group->count = CTool_EndianConvertWord32(path->count);
                record = &(group + 1)->base;
                for (child = path->children; child != NULL; child = child->next) {
                    reference = CompilerTools_AllocatePool(sizeof(DataReference));
                    reference->next = references;
                    references = reference;
                    reference->target = get_or_create_type_object(child->base, 0);
                    reference->offset = (char *)record - buffer;
                    reference->value0c = 0;
                    record->offset = CTool_EndianConvertWord32(child->offset);
                    record++;
                }
            }
        }
    }
    fn_004ceab0(object, buffer, references, object->type->size);
    return object;
}

Object *get_or_create_type_object(Type *type, SInt32 flags)
{
    HashNameNode *namehash;
    UInt8 buf[8];
    TypePointer typ;
    Object *obj;
    NameSpaceObjectList *found;
    char *s;
    Object *str;
    Object *classObject;
    DataReference *sub;
    DataReference *rec;
    Object *result;

    do {
        if (type->type == TYPEPOINTER) {
            if ((TYPE_POINTER(type)->qual & Q_CV) != 0) {
                typ = *TYPE_POINTER(type);
                typ.qual &= ~Q_CV;
                type = (Type *)&typ;
            }
        } else {
            flags = 0;
        }

        if (type->type == TYPECLASS && TYPE_CLASS(type)->size == 0) {
            CDecl_CompleteType(type);
            if ((TYPE_CLASS(type)->flags & CLASS_COMPLETED) == 0)
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, type, 0);
        }

        namehash = CMangler_RTTIObjectName(type, flags);

        found = CScope_FindName(cscope_root, namehash);
        if (found != NULL) {
            if ((obj = (Object *)found->object)->otype == OT_OBJECT && obj->datatype == DDATA) {
                result = obj;
                break;
            }
        }

        s = CError_GetTypeString(type, flags, 0);
        str = CInit_DeclareString(s, strlen(s) + 1, 0, 0);

        classObject = NULL;
        if (type->type == TYPECLASS)
            classObject = (Object *)create_rtti_base_records(TYPE_CLASS(type));

        memclrw(buf, sizeof(buf));
        obj = CParser_NewCompilerDefDataObject();
        obj->name = namehash;
        obj->type = CDecl_NewStructType(8, 4);
        obj->sclass = TK_STATIC;

        rec = CompilerTools_AllocatePool(sizeof(DataReference));
        rec->next = NULL;
        rec->target = str;
        rec->offset = 0;
        rec->value0c = 0;
        if (classObject != NULL) {
            sub = CompilerTools_AllocatePool(sizeof(DataReference));
            rec->next = sub;
            rec->next->next = NULL;
            rec->next->target = classObject;
            rec->next->offset = 4;
            rec->next->value0c = 0;
        }

        CScope_AddGlobalObject(obj);
        fn_004ceab0(obj, buf, rec, obj->type->size);
        result = obj;
    } while (0);
    return result;
}

void build_rtti_offset_table(TypeClass *rootClass, TypeClass *cls, Object *key, unsigned char *offsetTable,
                             int objectOffset, int vtableOffset)
{
    RTTIVTableOffsetNode *node;
    RData *record;
    ClassList *base;
    int baseObjectOffset;
    int baseVTableOffset;

    if (cls->vtable->owner == cls) {
        for (node = rtti_vtable_offset_list; node != NULL; node = node->next) {
            if (node->vtableOffset == vtableOffset)
                break;
        }
        if (node == NULL) {
            RTTIOffsetEntry *entry;
            node = (RTTIVTableOffsetNode *)CompilerTools_AllocatePool(sizeof(*node));
            node->next = rtti_vtable_offset_list;
            node->vtableOffset = vtableOffset;
            rtti_vtable_offset_list = node;
            record = (RData *)CompilerTools_AllocatePool(sizeof(*record));
            record->next = rtti_offset_table_head;
            record->key = key;
            record->vtableOffset = vtableOffset;
            record->zero = 0;
            rtti_offset_table_head = record;
            entry = (RTTIOffsetEntry *)(offsetTable + vtableOffset);
            entry->offset = CTool_EndianConvertWord32(-objectOffset);
        } else {
            RTTIOffsetEntry *entry = (RTTIOffsetEntry *)(offsetTable + vtableOffset);
            baseObjectOffset = entry->offset;
            baseVTableOffset = CTool_EndianConvertWord32(-objectOffset);
            if (baseObjectOffset != baseVTableOffset)
                CError_Internal("CRTTI.c", 0x17e);
        }
    }

    for (base = cls->bases; base != NULL; base = base->next) {
        if (base->base->vtable != NULL) {
            if (base->is_virtual) {
                baseObjectOffset = CClass_FindVBaseOffset(rootClass, base->base);
                baseVTableOffset = CClass_VirtualBaseVTableOffset(rootClass, base->base);
            } else {
                baseObjectOffset = objectOffset + base->offset;
                baseVTableOffset = vtableOffset + base->voffset;
            }
            build_rtti_offset_table(rootClass, base->base, key, offsetTable, baseObjectOffset, baseVTableOffset);
        }
    }
}

RData *CRTTI_BuildRTTIOffsetTable(TypeClass *classType, unsigned char *tableName, RData *tableHead)
{
    rtti_vtable_offset_list = NULL;
    rtti_offset_table_head = tableHead;
    build_rtti_offset_table(classType, classType, get_or_create_type_object((Type *)classType, 0), tableName, 0, 0);
    return rtti_offset_table_head;
}

ENode *CRTTI_ParseTypeid(void)
{
    Type *classType;
    Type *type;
    UInt32 qualifiers;
    NameSpace *nspace;
    NameSpaceObjectList *objects;
    ENode *expr;
    HashNameNode *name;
    DeclInfo typeInfo;

    if (copts.RTTI == 0)
        CError_Warning(ERR_RTTI_OPTION_DISABLED);

    name = GetHashNameNode("std");
    objects = CScope_FindName(cscope_root, name);
    if (objects != NULL && objects->object->otype == OT_NAMESPACE)
        nspace = ((ObjNameSpace *)objects->object)->nspace;
    else
        nspace = cscope_root;

    name = GetHashNameNode("type_info");
    {
        Type *foundType = CScope_GetTagType(nspace, name);
        if (foundType != NULL && foundType->type == TYPECLASS && foundType->size != 0)
            classType = foundType;
        else {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, "::std::type_info");
            classType = (Type *)&stchar;
        }
    }

    if ((SInt16)CPrepTokenizer_GetNextToken() != '(') {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return nullnode();
    }

    tk = CPrepTokenizer_GetNextToken();
    if (isdeclaration(1, 1, 1, ')') != 0) {
        memclrw(&typeInfo, sizeof(typeInfo));
        CParser_GetDeclSpecs(&typeInfo, 0);
        CDecl_ParseDeclarator(&typeInfo);
        if (tk != ')')
            CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
        type = typeInfo.thetype;
        qualifiers = typeInfo.qual;
        if (type->type == TYPEPOINTER && (TYPE_POINTER(type)->qual & Q_REFERENCE) != 0)
            type = TYPE_POINTER(type)->target;
    } else {
        expr = s_expression();
        if (tk != ')')
            CError_ReportErrorAndUpdateToken(ERR_RPAREN_EXPECTED);
        else
            tk = CPrepTokenizer_GetNextToken();
        type = expr->rtype;
        qualifiers = expr->flags & ENODE_FLAG_QUALS;
        if (type->type == TYPECLASS && TYPE_CLASS(type)->vtable != NULL) {
            ENode *result =
                funccallexpr(typeid_func, getnodeaddress(expr, 0),
                             intconstnode((Type *)&stsignedlong, TYPE_CLASS(type)->vtable->offset), NULL, NULL);
            result->rtype = (Type *)CDecl_NewPointerType(classType);
            result = makemonadicnode(result, EINDIRECT);
            result->rtype = classType;
            result->flags = ENODE_FLAG_CONST;
            return result;
        }
    }
    {
        ENode *result = create_objectrefnode(get_or_create_type_object(type, qualifiers));
        result = makemonadicnode(result, EINDIRECT);
        result->rtype = classType;
        result->flags = ENODE_FLAG_CONST;
        return result;
    }
}

void check_constness_casted_away(Type *sourceType, int sourceQualifiers, Type *targetType, int targetQualifiers)
{
    UInt8 skipOuterQualifiers = 1;

    if (targetType->type == TYPEPOINTER && (TYPE_POINTER(targetType)->qual & Q_REFERENCE)) {
        skipOuterQualifiers = 0;
        targetType = TPTR_TARGET(targetType);
    }
    for (;;) {
        if (sourceType->type != targetType->type)
            break;
        switch ((SInt8)sourceType->type) {
            case TYPEPOINTER:
                if (!skipOuterQualifiers) {
                    UInt32 sourcePointerQualifiers = TYPE_POINTER(sourceType)->qual;
                    UInt32 targetPointerQualifiers = TYPE_POINTER(targetType)->qual;
                    if (((sourcePointerQualifiers & Q_CONST) && !(targetPointerQualifiers & Q_CONST)) ||
                        ((sourcePointerQualifiers & Q_VOLATILE) && !(targetPointerQualifiers & Q_VOLATILE)))
                        CError_ReportError(ERR_CONSTNESS_CASTED_AWAY);
                }
                sourceType = TPTR_TARGET(sourceType);
                targetType = TPTR_TARGET(targetType);
                skipOuterQualifiers = 0;
                continue;
            case TYPEMEMBERPOINTER:
                if (!skipOuterQualifiers) {
                    UInt32 sourceMemberQualifiers = TYPE_MEMBER_POINTER(sourceType)->qual;
                    UInt32 targetMemberQualifiers = TYPE_MEMBER_POINTER(targetType)->qual;
                    if (((sourceMemberQualifiers & Q_CONST) && !(targetMemberQualifiers & Q_CONST)) ||
                        ((sourceMemberQualifiers & Q_VOLATILE) && !(targetMemberQualifiers & Q_VOLATILE)))
                        CError_ReportError(ERR_CONSTNESS_CASTED_AWAY);
                }
                sourceType = TYPE_MEMBER_POINTER(sourceType)->ty1;
                targetType = TYPE_MEMBER_POINTER(targetType)->ty1;
                skipOuterQualifiers = 0;
                continue;
            default:
                break;
        }
        break;
    }
    if (!skipOuterQualifiers) {
        UInt32 targetCVQualifiers = CParser_GetCVTypeQualifiers(targetType, targetQualifiers);
        UInt32 sourceCVQualifiers = CParser_GetCVTypeQualifiers(sourceType, sourceQualifiers);
        if (((sourceCVQualifiers & Q_CONST) && !(targetCVQualifiers & Q_CONST)) ||
            ((sourceCVQualifiers & Q_VOLATILE) && !(targetCVQualifiers & Q_VOLATILE)))
            CError_ReportError(ERR_CONSTNESS_CASTED_AWAY);
    }
}

ENode *parse_cast_type_and_expression(DeclInfo *typeSpec)
{
    short token;
    ENode *expression;

    token = CPrepTokenizer_GetNextToken();
    if (token != '<') {
        CError_ReportError(ERR_LESS_EXPECTED);
        return NULL;
    }
    tk = CPrepTokenizer_GetNextToken();
    memclrw(typeSpec, sizeof(*typeSpec));
    CParser_GetDeclSpecs(typeSpec, 0);
    CDecl_ParseDeclarator(typeSpec);
    if (typeSpec->name) {
        CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
    }
    if (tk != '>') {
        CError_ReportError(ERR_GREATER_EXPECTED);
        return NULL;
    }
    token = CPrepTokenizer_GetNextToken();
    if (token != '(') {
        CError_ReportError(ERR_LPAREN_EXPECTED);
        return NULL;
    }
    tk = CPrepTokenizer_GetNextToken();
    expression = CExpr_ParseCommaExpression();
    if (typeSpec->thetype->type == TYPEPOINTER) {
        TypePointer *pointerType = (TypePointer *)typeSpec->thetype;
        if (!(pointerType->qual & Q_REFERENCE)) {
            expression = CExpr_GeneratePointerAndRewriteConst(expression);
        }
    } else {
        expression = CExpr_GeneratePointerAndRewriteConst(expression);
    }
    if (tk != ')') {
        CError_ReportError(ERR_RPAREN_EXPECTED);
        return NULL;
    }
    tk = CPrepTokenizer_GetNextToken();
    return expression;
}

Boolean types_equal(Type *left, Type *right)
{
    for (;;) {
        if (left->type != right->type)
            return 0;
        switch ((SInt8)left->type) {
            case TYPEVOID:
                return 1;
            case TYPEINT:
            case TYPEFLOAT:
            case TYPEENUM:
            case TYPESTRUCT:
                return left == right;
            case TYPEPOINTER:
                left = TPTR_TARGET(left);
                right = TPTR_TARGET(right);
                break;
            default:
                return iscpp_typeequal(left, right);
        }
    }
}

ENode *explicit_typecast(register ENode *expr, register Type *type, UInt32 flags, char mode)
{
    Boolean err, set, wrap;

    if (expr->type == ENEWEXCEPTION)
        return oldassignmentpromotion(expr, type, flags & 3, 1);

    set = wrap = err = 0;

    switch ((SInt8)type->type) {
        case TYPEPOINTER:
            if (TYPE_POINTER(type)->qual & Q_REFERENCE) {
                if (IsSameType(TPTR_TARGET(type), expr->rtype))
                    break;
                if (mode != 2)
                    break;
                if (TPTR_TARGET(type)->type == TYPECLASS && expr->rtype->type == TYPECLASS) {
                    if (CClass_FindBasePath((TypeClass *)TPTR_TARGET(type), (TypeClass *)expr->rtype, 0, 1))
                        break;
                    if (CClass_FindBasePath((TypeClass *)expr->rtype, (TypeClass *)TPTR_TARGET(type), 0, 1))
                        break;
                }
            } else if (expr->rtype->type == TYPEPOINTER) {
                if (mode == 3 || IsSameType(type, expr->rtype) || TPTR_TARGET(type)->type == TYPEVOID ||
                    TPTR_TARGET(expr->rtype)->type == TYPEVOID) {
                    set = wrap = 1;
                    break;
                }
                if (mode != 2)
                    break;
                if (TPTR_TARGET(type)->type == TYPECLASS && TPTR_TARGET(expr->rtype)->type == TYPECLASS) {
                    if (CClass_FindBasePath((TypeClass *)TPTR_TARGET(type), (TypeClass *)TPTR_TARGET(expr->rtype), 0,
                                            1))
                        break;
                    if (CClass_FindBasePath((TypeClass *)TPTR_TARGET(expr->rtype), (TypeClass *)TPTR_TARGET(type), 0,
                                            1))
                        break;
                }
            } else {
                if (expr->rtype->type == TYPEENUM)
                    expr->rtype = TYPE_ENUM(expr->rtype)->enumtype;
                if (expr->rtype->type == TYPEINT) {
                    if (expr->type == EINTCONST && IsZero(&expr->data.intval)) {
                        set = 1;
                        break;
                    }
                    if (mode != 2)
                        break;
                }
                if (expr->rtype->type == TYPECLASS && mode == 2)
                    break;
            }
            err = 1;
    }
    if (err) {
        CError_ReportError(ERR_ILLEGAL_EXPLICIT_CONVERSION_FROM, expr->rtype, expr->flags & 3, type, flags);
        return expr;
    }
    if (set) {
        if (wrap && expr->type == EINDIRECT && (copts.pointercast_lvalue != 0 || copts.ANSIstrict == 0))
            expr = makemonadicnode(expr, 0x30);
        expr->rtype = type;
        expr->flags = flags & 3;
        return expr;
    }
    return do_typecast(expr, type, flags);
}

ENode *CRTTI_ParseDynamicCast(void)
{
    ENode *expr;
    Type *targetClass;
    TypeClass *sourceClass;
    TypePointer *targetPointer;
    ENode *result;
    ENode *targetInfo;
    unsigned char isReference;
    char targetKind;
    DeclInfo parsed;
    expr = parse_cast_type_and_expression(&parsed);
    if (expr == NULL)
        return nullnode();
    if (copts.RTTI == 0)
        CError_Warning(ERR_RTTI_OPTION_DISABLED);
    check_constness_casted_away(expr->rtype, expr->flags, parsed.thetype, parsed.qual);
    if (parsed.thetype->type != TYPEPOINTER) {
        CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
        return expr;
    }
    targetPointer = (TypePointer *)parsed.thetype;
    isReference = (targetPointer->qual & Q_REFERENCE) != 0;
    if ((targetKind = (targetClass = targetPointer->target)->type) == TYPECLASS) {
        CDecl_CompleteType(targetClass);
        if ((TYPE_CLASS(targetClass)->flags & CLASS_COMPLETED) == 0) {
            CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, targetClass, 0);
            return expr;
        }
    } else {
        if (targetKind != TYPEVOID) {
            CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            return expr;
        }
        targetClass = NULL;
    }
    if (isReference) {
        if (expr->rtype->type != TYPECLASS) {
            CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            return expr;
        }
        sourceClass = (TypeClass *)expr->rtype;
        if (targetClass != NULL && (sourceClass == TYPE_CLASS(targetClass) ||
                                    CClass_FindBasePath(sourceClass, TYPE_CLASS(targetClass), 0, 1) != 0))
            return do_typecast(expr, parsed.thetype, parsed.qual);
        expr = getnodeaddress(expr, 1);
    } else {
        if (expr->rtype->type != TYPEPOINTER || ((TypePointer *)expr->rtype)->target->type != TYPECLASS) {
            CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            return expr;
        }
        sourceClass = (TypeClass *)((TypePointer *)expr->rtype)->target;
        if (targetClass != NULL && (sourceClass == TYPE_CLASS(targetClass) ||
                                    CClass_FindBasePath(sourceClass, TYPE_CLASS(targetClass), 0, 1) != 0))
            return do_typecast(expr, parsed.thetype, parsed.qual);
    }
    if ((sourceClass->flags & CLASS_COMPLETED) == 0) {
        CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, sourceClass, 0);
        return expr;
    }
    if (sourceClass->vtable == NULL) {
        CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
        return expr;
    }
    if (sourceClass->sominfo != NULL) {
        CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
        return expr;
    }
    if (targetClass != NULL) {
        targetInfo = create_objectrefnode(get_or_create_type_object(targetClass, 0));
        if (TYPE_CLASS(targetClass)->sominfo != NULL) {
            CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            return expr;
        }
    } else {
        targetInfo = nullnode();
    }
    result =
        CExpr_FuncCallSix(dynamic_cast_object, expr, intconstnode((Type *)&stsignedlong, sourceClass->vtable->offset),
                          targetInfo, create_objectrefnode(get_or_create_type_object((Type *)sourceClass, 0)),
                          intconstnode((Type *)&stsignedshort, isReference), NULL);
    if (isReference) {
        result->rtype = CDecl_NewPointerType(targetClass);
        result = makemonadicnode(result, EINDIRECT);
        result->rtype = targetClass;
    } else {
        result->rtype = parsed.thetype;
    }
    result->flags = parsed.qual & ENODE_FLAG_QUALS;
    return result;
}

ENode *CRTTI_ParseExplicitTypecast(void)
{
    struct ENode *expr;
    Type *conversionType;
    DeclInfo conversion;
    expr = parse_cast_type_and_expression(&conversion);
    if (expr == NULL) {
        return nullnode();
    }
    check_constness_casted_away(expr->rtype, expr->flags, conversion.thetype, conversion.qual);
    if (conversion.thetype->type != TYPEVOID) {
        conversionType = conversion.thetype;
        if (conversion.thetype->type == TYPEPOINTER) {
            conversionType = ((TypePointer *)conversionType)->target;
        }
        if ((conversionType->type == TYPECLASS) && (conversionType->size == 0)) {
            CDecl_CompleteType(conversionType);
            if ((((TypeClass *)conversionType)->flags & CLASS_COMPLETED) == 0) {
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, conversionType, 0);
            }
        }
        conversionType = expr->rtype;
        if (conversionType->type == TYPEPOINTER) {
            conversionType = ((TypePointer *)conversionType)->target;
        }
        if ((conversionType->type == TYPECLASS) && (conversionType->size == 0)) {
            CDecl_CompleteType(conversionType);
            if ((((TypeClass *)conversionType)->flags & CLASS_COMPLETED) == 0) {
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, conversionType, 0);
            }
        }
    }
    return explicit_typecast(expr, conversion.thetype, conversion.qual, '\x02');
}

ENode *CRTTI_ReinterpretCast(void)
{
    ENode *node;
    Type *referenceType;
    DeclInfo data;
    SInt8 typeKind;

    node = parse_cast_type_and_expression(&data);
    if (node == NULL)
        return nullnode();
    check_constness_casted_away(node->rtype, node->flags, data.thetype, data.qual);
    if (data.thetype->type == TYPEPOINTER && (TYPE_POINTER(data.thetype)->qual & Q_REFERENCE) != 0) {
        node = CExpr_LValue(node, 1, 1);
        if (node->type != EINDIRECT)
            return node;
        node->data.monadic->rtype = CDecl_NewPointerType(node->rtype);
        node = node->data.monadic;
        referenceType = data.thetype;
        data.thetype = CDecl_NewPointerType(TYPE_POINTER(data.thetype)->target);
        TYPE_POINTER(data.thetype)->qual = data.qual;
        data.qual = 0;
    } else {
        referenceType = NULL;
    }
    typeKind = data.thetype->type;
    switch (typeKind) {
        case TYPEINT:
            typeKind = node->rtype->type;
            switch (typeKind) {
                case TYPEPOINTER:
                case TYPEMEMBERPOINTER:
                    node = do_typecast(node, data.thetype, data.qual);
                    break;
                default:
                    CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            }
            break;
        case TYPEPOINTER:
            typeKind = node->rtype->type;
            switch (typeKind) {
                case TYPEINT:
                    if (referenceType != NULL)
                        data.thetype = referenceType;
                    node = do_typecast(node, data.thetype, data.qual);
                    break;
                case TYPEPOINTER:
                    node = makemonadicnode(node, ETYPCON);
                    node->rtype = data.thetype;
                    node->flags = data.qual & Q_CV;
                    break;
                default:
                    CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            }
            break;
        case TYPEMEMBERPOINTER:
            if (node->rtype->type == TYPEMEMBERPOINTER) {
                if (TYPE_MEMBER_POINTER(data.thetype)->ty1->type == TYPEFUNC) {
                    if (TYPE_MEMBER_POINTER(node->rtype)->ty1->type == TYPEFUNC) {
                        node->rtype = data.thetype;
                        node->flags = data.qual & Q_CV;
                        break;
                    }
                } else {
                    if (TYPE_MEMBER_POINTER(node->rtype)->ty1->type != TYPEFUNC) {
                        node->rtype = data.thetype;
                        node->flags = data.qual & Q_CV;
                        break;
                    }
                }
            }
            node = do_typecast(node, data.thetype, data.qual);
            break;
        default:
            CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
    }
    if (referenceType != NULL && node->rtype->type == TYPEPOINTER) {
        node = makemonadicnode(node, EINDIRECT);
        node->rtype = TYPE_POINTER(data.thetype)->target;
    }
    return node;
}

ENode *CRTTI_ParseConstCast(void)
{
    UInt32 pre;
    DeclInfo conv;
    ENode *result;

    result = parse_cast_type_and_expression(&conv);
    if (result == NULL)
        return nullnode();

    if (conv.thetype->type == TYPEPOINTER) {
        if (TYPE_POINTER(conv.thetype)->qual & Q_REFERENCE) {
            if (!is_typeequal(TPTR_TARGET(conv.thetype), result->rtype))
                CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            if (result->type == EINDIRECT) {
                result->rtype = TPTR_TARGET(conv.thetype);
                result->flags = (UInt16)(conv.qual & Q_CV);
            } else {
                CError_ReportError(ERR_NOT_LVALUE);
            }
        } else {
            if (!is_typeequal(conv.thetype, result->rtype))
                CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
            result = do_typecast(result, conv.thetype, conv.qual);
        }
    } else if (conv.thetype->type == TYPEMEMBERPOINTER) {
        if (!is_typeequal(conv.thetype, result->rtype))
            CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
        result = do_typecast(result, conv.thetype, conv.qual);
    } else {
        CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
    }
    return result;
}
