#define CERROR_FILE "CABI.c"
#include "compiler/common.h"
#include "compiler/CABI.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"

struct HashNameNode *vtable_name;
struct Object *newh_func;

static void *trans_vtboffsets;
static Object *CABI_ThisArg(void);

enum { OVERRIDE_VIRTUAL = 1 };

enum { ST_EXPRESSION_0050b120 = 4 };

typedef Statement *(*TransConstructorCallback)(Statement *stmt, TypeClass *tclass, TypeClass *base, SInt32 offset,
                                               Boolean flag);

typedef struct Node Node;

static SInt32 CABI_FindNVBase(TypeClass *tclass, TypeClass *baseclass, SInt32 offset);

static inline Statement *destroy_array(Statement *expr, ObjMemberVar *member, TypeClass *cls, Type *type,
                                       Object *classobj)
{
    ENode *base;
    ENode *ref;
    Statement *node;
    SInt32 offset;
    ref = create_objectrefnode(classobj);
    ref->flags |= ENODE_FLAG_80;
    node = CFunc_InsertAfterStatement(4, expr);
    offset = member->offset;
    base = CABI_MakeThisExpr(cls, offset);
    node->expr = funccallexpr(data_0058717c, base, ref, intconstnode((Type *)&stsignedlong, type->size),
                              intconstnode((Type *)&stsignedlong, member->type->size / type->size));
    return node;
}

/* Entries and insertion state used while laying out a class vtable. */
static inline char use_vtable_size_without_vbases(void)
{
    return copts.vbase_abi_v2;
}

static inline char base_layout_mode(void)
{
    return copts.vbase_abi_v2;
}

Type *CABI_GetSizeTType(void)
{
    return (Type *)&stunsignedlong;
}

Type *CABI_GetPtrDiffTType(void)
{
    return (Type *)&stsignedlong;
}

SInt16 CABI_ComputeAlignmentPadding(Type *data, SInt32 mask)
{
    SInt16 value;
    SInt32 alignment;

    value = CMachine_GetTypeAlignment(data);
    alignment = value;
    if (alignment <= 1) {
        return 0;
    }
    return (alignment - (mask & (alignment - 1U))) & (alignment - 1U);
}

void CABI_ReverseBitField(TypeBitfield *tbitfield)
{
    UInt32 bits;

    switch (tbitfield->bitfieldtype->size) {
        case 1:
            bits = 8;
            break;
        case 2:
            bits = 16;
            break;
        case 4:
            bits = 32;
            break;
        case 8:
            bits = 64;
            break;
        default:
            CError_FATAL(168);
    }
    tbitfield->offset = bits - tbitfield->offset - tbitfield->bitlength;
}

static SInt32 CABI_BaseSize(TypeClass *base)
{
    SInt32 size = base->size;
    if (copts.vbase_abi_v2) {
        if (base->vbases)
            return base->vbases->offset;
    }
    return size;
}

void layout_nonvirtual_bases(void *abiContext, TypeClass *derivedClass)
{
    TypeClass *base;
    ClassList *baseEntry;
    SInt32 offset;
    VClassList *virtualBase;
    Boolean previousBaseWasEmpty;

    previousBaseWasEmpty = 0;
    offset = derivedClass->size;
    for (baseEntry = derivedClass->bases; baseEntry != NULL; baseEntry = baseEntry->next) {
        if (!baseEntry->is_virtual) {
            base = baseEntry->base;
            if (!(base->flags & 0x1000)) {
                baseEntry->offset = CMach_MemberAlignValue(TYPE(base), offset) + offset;
                if (base_layout_mode()) {
                    offset = baseEntry->offset + CABI_BaseSize(base);
                } else {
                    offset = baseEntry->offset + base->size;
                    for (virtualBase = base->vbases; virtualBase != NULL; virtualBase = virtualBase->next)
                        offset -= virtualBase->base->size;
                }
                previousBaseWasEmpty = 0;
            } else {
                if (previousBaseWasEmpty)
                    offset++;
                previousBaseWasEmpty = 1;
                baseEntry->offset = offset;
            }
        }
    }
    derivedClass->size = offset;
}

static void layout_virtual_base_pointers(ClassLayout *layout, TypeClass *tclass)
{
    ClassList *base;
    SInt32 size = tclass->size;

    for (base = tclass->bases; base; base = base->next) {
        if (base->is_virtual) {
            base->offset = CMach_MemberAlignValue(TYPE(&void_ptr), size) + size;
            size = base->offset + void_ptr.size;
        }
    }
    tclass->size = size;
}

static SInt32 find_union_member_offset(TypeClass *tclass, HashNameNode *name)
{
    ObjMemberVar *member;

    if (!name)
        return 0;
    member = tclass->ivars;
    for (;;) {
        if (member->name == name)
            return member->offset;
        member = member->next;
        if (!member)
            CError_FATAL(358);
    }
}

void layout_class_ivars(ClassLayout *member, TypeClass *type)
{
    SInt32 unionOffset;
    Boolean removeUnnamed;
    Boolean firstUnionMember;
    SInt32 maximumSize;
    TypeClass *unionType;
    SInt32 bitfieldBits;
    SInt32 offset;
    ObjMemberVar **link;
    ObjMemberVar *mem;
    ObjMemberVar *node;
    SInt32 initialSize;
    HashNameNode *name;
    TypeBitfield *bitfield;
    SInt32 size;

    removeUnnamed = 0;
    initialSize = maximumSize = type->size;
    CMach_StructLayoutInitOffset(initialSize);
    unionType = NULL;
    for (mem = type->ivars; mem != NULL; mem = mem->next) {
        if (mem->anonunion == 0) {
            if ((mem->offset & 0x80000000) == 0) {
                if (type->mode == 1)
                    CMach_StructLayoutInitOffset(initialSize);
                if (mem->type->type == TYPEBITFIELD)
                    mem->offset = CMach_StructLayoutBitfield(TYPE_BITFIELD(mem->type), mem->qual);
                else
                    mem->offset = CMach_StructLayoutGetOffset(mem->type, mem->qual);
                if (type->mode == 1 && (size = CMach_StructLayoutGetCurSize()) > maximumSize)
                    maximumSize = size;
                unionType = NULL;
            } else {
                if (unionType == NULL)
                    CError_FATAL(408);
                mem->offset = unionOffset + find_union_member_offset(unionType, mem->name);
                if (firstUnionMember == 0)
                    mem->anonunion = 1;
                firstUnionMember = 0;
            }
            if (mem->name == unnamed_name || mem->name == NULL)
                removeUnnamed = 1;
        } else {
            if (mem->type->type != TYPECLASS)
                CError_FATAL(418);
            if (type->mode == 1)
                CMach_StructLayoutInitOffset(initialSize);
            unionOffset = CMach_StructLayoutGetOffset(mem->type, mem->qual);
            unionType = (TypeClass *)mem->type;
            firstUnionMember = 1;
            if (type->mode == 1 && (offset = CMach_StructLayoutGetCurSize()) > maximumSize)
                maximumSize = offset;
            removeUnnamed = 1;
        }
        if (member->vtable_ivar == mem)
            type->vtable->offset = mem->offset;
    }
    if (removeUnnamed) {
        link = &type->ivars;
        while ((node = *link) != NULL) {
            if (node->name == NULL || node->name == unnamed_name)
                *link = node->next;
            else
                link = &node->next;
        }
    }
    if (type->mode == 1)
        type->size = maximumSize;
    else
        type->size = CMach_StructLayoutGetCurSize();
    if (copts.reverse_bitfields != 0) {
        for (mem = type->ivars; mem != NULL; mem = mem->next) {
            if (mem->type->type != TYPEBITFIELD)
                continue;
            CABI_ReverseBitField(TYPE_BITFIELD(mem->type));
        }
    }
}

static void layout_virtual_bases(ClassLayout *layout, TypeClass *tclass)
{
    VClassList *base;
    SInt32 size = tclass->size;

    for (base = tclass->vbases; base; base = base->next) {
        base->offset = CMach_MemberAlignValue(TYPE(base->base), size) + size;
        size = base->offset + CABI_BaseSize(base->base);
        if (base->has_override)
            size += CMach_MemberAlignValue(TYPE(&stunsignedlong), size) + stunsignedlong.size;
    }
    tclass->size = size;
}

static char inherit_vtable_member(TypeClass *classType)
{
    VTable *baseVtable;
    ClassList *base;

    for (base = classType->bases; base != NULL; base = base->next) {
        if (base->is_virtual == 0 && (baseVtable = base->base->vtable) != NULL && base->offset == 0) {
            classType->vtable->offset = baseVtable->offset;
            return 1;
        }
    }
    return 0;
}

Object *CABI_FindZeroVirtualBaseMember(TypeClass *scope, Object *key)
{
    NameSpaceObjectList *entry;
    ClassList *node = scope->bases;
    Object *object;
    Object *result;

    while (node != NULL) {
        if (node->is_virtual == '\0' && node->offset == 0 && node->voffset == 0 && node->base->vtable != NULL) {
            for (entry = CScope_FindName(node->base->nspace, key->name); entry != NULL; entry = entry->next) {
                if ((object = (Object *)entry->object)->otype == OT_OBJECT && object->datatype == DVFUNC &&
                    CClass_GetOverrideKind(TYPE_FUNC(object->type), TYPE_FUNC(key->type), '\0') == '\x01') {
                    return object;
                }
            }
            {
                result = CABI_FindZeroVirtualBaseMember(node->base, key);
                if (result != NULL) {
                    return result;
                }
            }
        }
        node = node->next;
    }
    return NULL;
}

void CABI_AddVTable(TypeClass *tclass)
{
    tclass->vtable = galloc(sizeof(VTable));
    memclrw(tclass->vtable, sizeof(VTable));
}

SInt32 CABI_GetVTableOffset(TypeClass *tclass)
{
    return 0;
}

int get_vtable_size_without_vbases(TypeClass *cl)
{
    VClassList *vb;
    SInt32 result = cl->vtable->size;

    for (vb = cl->vbases; vb; vb = vb->next) {
        if (vb->base->vtable)
            result = result - get_vtable_size_without_vbases(vb->base);
    }
    return result;
}

static void CABI_ApplyClassFlags(Object *obj, UInt8 flags, Boolean is_inline)
{
    if (flags & CLASS_EFLAGS_INTERNAL)
        obj->flags |= OBJECT_INTERNAL;
    if (flags & CLASS_EFLAGS_IMPORT)
        obj->flags |= OBJECT_IMPORT;
    if (flags & CLASS_EFLAGS_EXPORT)
        obj->flags |= OBJECT_EXPORT;
}

void layout_vtable(ClassLayout *layout, TypeClass *classType)
{
    int size;
    char hasVtableMember;
    ObjMemberVar *member;
    int index;
    ClassList *base;
    VClassList *vbase;
    Object *method;
    TypeMemberFunc *methodType;
    Object *overridden;
    VClassList *virtualBase;
    Object *vtableObject;
    unsigned char eflags;
    int methodIndex;

    size = 0;
    if (classType->vtable == NULL) {
        CABI_AddVTable(classType);
        layout->firstVirtualSlot = layout->lex_order_count - 1;
    }
    hasVtableMember = inherit_vtable_member(classType);
    if (hasVtableMember == 0) {
        member = (ObjMemberVar *)galloc(sizeof(ObjMemberVar));
        memclrw(member, sizeof(ObjMemberVar));
        member->otype = OT_MEMBERVAR;
        member->access = ACCESSPUBLIC;
        member->name = vtable_name;
        member->type = (Type *)&void_ptr;
        layout->vtable_ivar = member;
        index = layout->firstVirtualSlot;
        for (;;) {
            if (index < 0) {
                member->next = classType->ivars;
                classType->ivars = member;
                break;
            }
            if (layout->objlist[index] == NULL) {
                CError_FATAL(662);
            }
            if (layout->objlist[index]->otype == OT_MEMBERVAR) {
                member->next = OBJ_MEMBER_VAR(layout->objlist[index])->next;
                OBJ_MEMBER_VAR(layout->objlist[index])->next = member;
                break;
            }
            --index;
        }
        if ((classType->flags & (8192 | CLASS_SINGLE_OBJECT)) != 0) {
            size = void_ptr.size;
        } else {
            size = 8;
        }
    } else {
        layout->vtable_ivar = NULL;
    }
    for (base = classType->bases; base != NULL; base = base->next) {
        if (base->base->vtable != NULL && base->is_virtual == 0) {
            base->voffset = size;
            if (use_vtable_size_without_vbases() != 0) {
                size += get_vtable_size_without_vbases(base->base);
            } else {
                size += base->base->vtable->size;
                for (vbase = base->base->vbases; vbase != NULL; vbase = vbase->next) {
                    if (vbase->base->vtable != NULL) {
                        size -= vbase->base->vtable->size;
                    }
                }
            }
        }
    }
    for (methodIndex = 0; methodIndex < layout->lex_order_count; ++methodIndex) {
        if ((method = OBJECT(layout->objlist[methodIndex])) == NULL) {
            CError_FATAL(710);
        }
        if (method->otype == OT_OBJECT && method->datatype == DVFUNC) {
            methodType = (TypeMemberFunc *)method->type;
            overridden = CABI_FindZeroVirtualBaseMember(classType, method);
            if (overridden != NULL) {
                methodType->vtbl_index = ((TypeMemberFunc *)overridden->type)->vtbl_index;
            } else {
                methodType->vtbl_index = size;
                size += 4;
            }
        }
    }
    for (virtualBase = classType->vbases; virtualBase != NULL; virtualBase = virtualBase->next) {
        if (virtualBase->base->vtable != NULL) {
            virtualBase->voffset = size;
            if (use_vtable_size_without_vbases() != 0) {
                size += get_vtable_size_without_vbases(virtualBase->base);
            } else {
                size += virtualBase->base->vtable->size;
            }
        }
    }
    vtableObject = CParser_NewCompilerDefDataObject();
    CABI_ApplyClassFlags(vtableObject, classType->eflags, 0);
    vtableObject->name = CMangler_VTableName(classType);
    vtableObject->type = CDecl_NewStructType(size, 4);
    vtableObject->nspace = classType->nspace;
    switch ((signed char)classType->action) {
        case 0:
            vtableObject->sclass = TK_STATIC;
            vtableObject->qual |= Q_IMPLICIT_WEAK;
            break;
    }
    CParser_UpdateObject(vtableObject, NULL);
    classType->vtable->object = vtableObject;
    classType->vtable->owner = classType;
    classType->vtable->size = size;
}

int CABI_LayoutClass(struct ClassLayout *members, TypeClass *type)
{
    int baseSize;
    int size;
    int virtualSize;
    ClassList *base;
    int baseClassSize;
    VClassList *vbase;
    SInt32 alignment;
    short padding;
    TypeClass *baseClass;
    char useVirtualOffset;
    int alignmentMask;
    char savedMode;
    savedMode = copts.structalignment;
    type->size = 0;
    if (type->sominfo == NULL) {
        if (type->bases != NULL) {
            layout_nonvirtual_bases(members, type);
        }
        if ((type->flags & CLASS_HAS_VBASES) != 0) {
            layout_virtual_base_pointers(members, type);
        }
        if (members->has_vtable != 0) {
            layout_vtable(members, type);
        }
        layout_class_ivars(members, type);
        if ((type->flags & CLASS_HAS_VBASES) != 0) {
            layout_virtual_bases(members, type);
        }
    } else {
        copts.structalignment = 2;
        layout_class_ivars(members, type);
    }
    type->align = CMach_GetClassAlign(type);
    if (type->size == 0) {
        type->size = 1;
        type->flags |= 4096;
    } else {
        type->size += CABI_ComputeAlignmentPadding(TYPE(type), type->size);
    }
    type->flags |= CLASS_COMPLETED;
    copts.structalignment = savedMode;
    return;
}

void CABI_MakeDefaultArgConstructor(TypeClass *theclass, Object *function)
{
    DefArgCtorInfo *defaults;
    ENodeList *callArgs;
    FuncArg *arg;
    FuncArg *formalArgs;
    unsigned char classFlags;
    char savedState;
    CScopeSave savedScope;
    Statement body;
    Statement statement;
    if (anyerrors != 0 || function->access == ACCESSNONE) {
        return;
    }
    CError_ASSERT(857, (defaults = function->u.func.defargdata) != 0);
    CABI_ApplyClassFlags(function, theclass->eflags, 0);
    CScope_SetFunctionScope(function, &savedScope);
    CFunc_FuncGenSetup(&body, function);
    savedState = copts.filesyminfo;
    copts.filesyminfo = 0;
    CFunc_SetupNewFuncArgs(function, ((TypeMemberFunc *)function->type)->args);
    if ((theclass->flags & CLASS_HAS_VBASES) != 0) {
        arguments->next->object->name = unnamed_name;
    }
    body.next = &statement;
    memclrw(&statement, sizeof(statement));
    statement.type = ST_RETURN;
    statement.expr = (ENode *)lalloc(sizeof(ENode));
    statement.expr->type = EFUNCCALL;
    statement.expr->cost = 200;
    statement.expr->flags = 0;
    statement.expr->rtype = (Type *)&void_ptr;
    statement.expr->data.funccall.funcref = CExpr_MakeObjRefNode(defaults->default_func, 0);
    statement.expr->data.funccall.functype = (TypeFunc *)defaults->default_func->type;
    formalArgs = ((TypeMemberFunc *)defaults->default_func->type)->args;
    statement.expr->data.funccall.args = (ENodeList *)lalloc(sizeof(ENodeList));
    callArgs = statement.expr->data.funccall.args;
    callArgs->node = ((ENode * (*)(Object *)) create_objectnode)(arguments->object);
    if ((theclass->flags & CLASS_HAS_VBASES) != 0) {
        formalArgs = formalArgs->next;
        callArgs = callArgs->next = (ENodeList *)lalloc(sizeof(ENodeList));
        callArgs->node = ((ENode * (*)(Object *)) create_objectnode)(arguments->next->object);
    }
    arg = formalArgs->next;
    callArgs = callArgs->next = (ENodeList *)lalloc(sizeof(ENodeList));
    callArgs->node = fn_00513040(defaults->default_arg, 0);
    while ((arg = arg->next) != NULL && arg->dexpr != NULL) {
        callArgs = callArgs->next = (ENodeList *)lalloc(sizeof(ENodeList));
        callArgs->node = fn_00513040(arg->dexpr, 0);
    }
    callArgs->next = NULL;
    CFunc_CodeCleanup(&body);
    CFunc_Gen(&body, function, 0);
    CScope_RestoreScope(&savedScope);
    copts.filesyminfo = savedState;
    function->u.func.defargdata = NULL;
}

static Object *CABI_ThisArg(void)
{
    CError_ASSERT(922, arguments && IS_TYPE_POINTER_ONLY(arguments->object->type));
    return arguments->object;
}

ENode *CABI_MakeThisExpr(TypeClass *typeClass, int count)
{
    ENode *type;
    if (typeClass != NULL) {
        if (typeClass->sominfo == NULL) {
            type = create_objectnode(CABI_ThisArg());
            type->rtype = (Type *)&void_ptr;
            if ((typeClass->flags & CLASS_HANDLEOBJECT) != 0) {
                type = makemonadicnode(type, 4);
            }
        } else {
            type = CSOM_GetOrCreateLocalObjectNode(typeClass);
        }
    } else {
        type = create_objectnode(CABI_ThisArg());
        type->rtype = (Type *)&void_ptr;
    }
    if (count != 0) {
        type = makediadicnode(type, intconstnode((Type *)&stunsignedlong, count), 15);
    }
    return type;
}

static inline Object *CABI_FlagArg(void)
{
    CError_ASSERT(967, arguments && arguments->next && arguments->next->object->type->type == TYPEINT);
    return (Object *)arguments->next->object;
}

static ENode *CABI_MakeFlagExpr(void)
{
    Object *arg;

    arg = CABI_FlagArg();
    return create_objectnode(arg);
}

static ENode *CABI_SourceArg(TypeClass *tclass, Boolean flag)
{
    ObjectList *list;

    CError_ASSERT(991, list = arguments);
    CError_ASSERT(992, list = list->next);
    if (flag && (tclass->flags & CLASS_HAS_VBASES))
        CError_ASSERT(993, list = list->next);
    CError_ASSERT(994, IS_TYPE_POINTER_ONLY(list->object->type));
    return create_objectnode(list->object);
}

ENode *build_vbase_ptr_initializers(ENode *expr, TypeClass *func, TypeClass *cls, TypeClass *vbase, SInt32 offset)
{
    ClassList *list;
    VToff *p;
    SInt32 off;
    ENode *n;

    for (list = cls->bases; list; list = list->next) {
        if (list->base == vbase && list->is_virtual) {
            off = offset + list->offset;
            for (p = trans_vtboffsets; p; p = p->next)
                if (off == p->off)
                    break;
            if (!p) {
                p = (VToff *)lalloc(8);
                p->off = off;
                p->next = trans_vtboffsets;
                trans_vtboffsets = p;
                expr = makediadicnode(makemonadicnode(CABI_MakeThisExpr(NULL, off), EINDIRECT), expr, EASS);
            }
        }
        if (!list->is_virtual)
            off = offset + list->offset;
        else
            off = CClass_VirtualBaseOffset(func, list->base);
        expr = build_vbase_ptr_initializers(expr, func, list->base, vbase, off);
    }
    return expr;
}

static Statement *CABI_InitVBasePtrs(Statement *stmt, TypeClass *tclass)
{
    VClassList *vbase;
    ENode *expr;

    vbase = tclass->vbases;
    trans_vtboffsets = NULL;
    for (; vbase; vbase = vbase->next) {
        expr = CABI_MakeThisExpr(NULL, vbase->offset);
        expr = build_vbase_ptr_initializers(expr, tclass, tclass, vbase->base, 0);
        stmt = CFunc_InsertAfterStatement(ST_EXPRESSION, stmt);
        stmt->expr = (struct ENode *)expr;
    }
    return stmt;
}

BaseOffsetPath *find_shortest_virtual_base_offset_path(TypeClass *tclass, TypeClass *base)
{
    ClassList *directBase;
    BaseOffsetPath *path;
    int length;
    ClassList *candidateBase;
    BaseOffsetPath *bestPath;
    BaseOffsetPath *step;
    ClassList *bases;
    BaseOffsetPath *rest;
    BaseOffsetPath *result;
    short bestLength;
    directBase = tclass->bases;
    while (directBase != NULL) {
        if (directBase->base == base && directBase->is_virtual != 0) {
            result = (BaseOffsetPath *)lalloc(8);
            result->next = NULL;
            result->offset = directBase->offset;
            return result;
        }
        directBase = directBase->next;
    }
    bestPath = NULL;
    candidateBase = (bases = tclass->bases);
    if (bases != NULL) {
        do {
            path = find_shortest_virtual_base_offset_path(candidateBase->base, base);
            if (path != NULL) {
                length = 1;
                step = (rest = path->next);
                if (rest != NULL) {
                    do {
                        step = step->next;
                        length = length + 1;
                    } while (step != NULL);
                }
                if (candidateBase->is_virtual != 0) {
                    length = length + 1;
                }
                if (bestPath == NULL || (short)length < bestLength) {
                    if (candidateBase->is_virtual != 0) {
                        bestPath = (BaseOffsetPath *)lalloc(8);
                        bestPath->next = path;
                        bestPath->offset = candidateBase->offset;
                    } else {
                        bestPath = path;
                        path->offset += candidateBase->offset;
                    }
                    bestLength = length;
                }
            }
            candidateBase = candidateBase->next;
        } while (candidateBase != NULL);
    }
    return bestPath;
}

static ENode *make_virtual_base_pointer(TypeClass *tclass, TypeClass *base)
{
    BaseOffsetPath *path;
    ENode *expr;

    path = find_shortest_virtual_base_offset_path(tclass, base);
    CError_ASSERT(1118, path);
    expr = makemonadicnode(CABI_MakeThisExpr(NULL, path->offset), EINDIRECT);
    while ((path = path->next)) {
        if (path->offset)
            expr = makediadicnode(expr, intconstnode(TYPE(&stunsignedlong), path->offset), EADD);
        expr = makemonadicnode(expr, EINDIRECT);
    }
    return expr;
}

static SInt32 CABI_FindNVBase(TypeClass *tclass, TypeClass *baseclass, SInt32 offset)
{
    ClassList *base;
    SInt32 tmp;

    if (tclass == baseclass)
        return offset;
    for (base = tclass->bases; base; base = base->next) {
        if (!base->is_virtual && (tmp = CABI_FindNVBase(base->base, baseclass, offset + base->offset)) >= 0)
            return tmp;
    }
    return -1;
}

SInt32 CABI_GetCtorOffsetOffset(TypeClass *tclass, TypeClass *baseclass)
{
    SInt32 basesize;
    SInt32 size;
    char savealign;

    size = CABI_BaseSize(tclass);
    if (baseclass) {
        basesize = CABI_FindNVBase(tclass, baseclass, 0);
        CError_ASSERT(1169, basesize >= 0);
        size -= basesize;
    }
    savealign = copts.structalignment;
    if (tclass->eflags & CLASS_EFLAGS_F0)
        copts.structalignment = ((tclass->eflags & CLASS_EFLAGS_F0) >> 4) - 1;
    size += CMach_MemberAlignValue(TYPE(&stunsignedlong), size);
    copts.structalignment = savealign;
    return size;
}

Statement *assign_vbase_ctor_offsets(Statement *list, TypeClass *cls)
{
    VClassList *vb;
    ENode *obj;
    SInt32 vbaseoffset;
    SInt32 value;
    SInt32 ctoroffset;
    Object *thisnode;
    ENode *expr;

    thisnode = NULL;
    for (vb = cls->vbases; vb != NULL; vb = vb->next) {
        if (vb->has_override) {
            if (thisnode == NULL)
                thisnode = create_temp_object((Type *)&void_ptr);
            vbaseoffset = CClass_VirtualBaseOffset(cls, vb->base);
            ctoroffset = CABI_GetCtorOffsetOffset(vb->base, NULL);
            CError_ASSERT(1206, obj = make_virtual_base_pointer(cls, vb->base));
            expr = makediadicnode(create_objectnode(thisnode), obj, EASS);
            list = CFunc_InsertAfterStatement(4, list);
            list->expr = expr;

            expr = makediadicnode(create_objectnode(thisnode), intconstnode((Type *)&stunsignedlong, ctoroffset), EADD);
            expr = makemonadicnode(expr, EINDIRECT);
            expr->rtype = (Type *)&stunsignedlong;
            expr = makediadicnode(expr, makediadicnode(CABI_MakeThisExpr(NULL, vbaseoffset), create_objectnode(thisnode), ESUB), EASS);
            list = CFunc_InsertAfterStatement(4, list);
            list->expr = expr;
        }
    }
    return list;
}

Statement *assign_vtable_pointers(Statement *result, Object *obj, TypeClass *cls, TypeClass *base, SInt32 offset,
                                  SInt32 voffset)
{
    ENode *objref;
    ENode *node;
    ENode *name;
    ClassList *b;
    VtOffEntry *entry;
    SInt32 key;
    SInt32 noff;
    SInt32 nvoff;

    if (((VTable *)base->vtable)->owner == base) {
        key = offset + ((VTable *)base->vtable)->offset;
        entry = trans_vtboffsets;
        while (entry != NULL) {
            if (key == entry->value)
                break;
            entry = entry->next;
        }
        if (entry == NULL) {
            entry = lalloc(8);
            entry->value = key;
            entry->next = trans_vtboffsets;
            trans_vtboffsets = entry;

            objref = create_objectrefnode(obj);
            objref->rtype = (Type *)&void_ptr;
            voffset += CABI_GetVTableOffset(base);
            if (voffset != 0)
                objref = makediadicnode(objref, intconstnode((Type *)&stunsignedlong, voffset), EADD);

            node = CABI_MakeThisExpr(cls, 0);

            name = CClass_AdjustBasePointer(node, base_path_depth, 0);

            if (((VTable *)base->vtable)->offset != 0 && canadd(name, ((VTable *)base->vtable)->offset) == 0) {
                name =
                    makediadicnode(name, intconstnode((Type *)&stunsignedlong, ((VTable *)base->vtable)->offset), EADD);
                optimizecomm(name);
            }

            result = CFunc_InsertAfterStatement(4, result);
            result->expr = makediadicnode(makemonadicnode(name, EINDIRECT), objref, EASS);
        }
    }

    for (b = base->bases; b != NULL; b = b->next) {
        if (b->base->vtable != NULL) {
            base_path[base_path_depth] = b;
            base_path_depth++;
            if (b->is_virtual) {
                noff = CClass_VirtualBaseOffset(cls, b->base);
                nvoff = CClass_VirtualBaseVTableOffset(cls, b->base);
            } else {
                noff = offset + b->offset;
                nvoff = voffset + b->voffset;
            }
            result = assign_vtable_pointers(result, obj, cls, b->base, noff, nvoff);
            base_path_depth--;
        }
    }
    return result;
}

static Boolean CABI_IsOperatorNew(Object *obj)
{
    return obj->otype == OT_OBJECT && obj->type->type == TYPEFUNC && ((TypeFunc *)obj->type)->args &&
           ((TypeFunc *)obj->type)->args->type == CABI_GetSizeTType() &&
           ((TypeFunc *)obj->type)->args->next == NULL;
}

Object *CABI_ConstructorCallsNew(TypeClass *tclass)
{
    HashNameNode *name;
    NameResult result;
    NameSpaceObjectList *list;

    if (tclass->sominfo == NULL && (tclass->flags & CLASS_HANDLEOBJECT) != 0) {
        if (CScope_FindClassMemberObject(tclass, &result, CMangler_OperatorName(0x147))) {
            if (result.object != NULL) {
                if (CABI_IsOperatorNew(OBJECT(result.object)))
                    return OBJECT(result.object);
            } else {
                for (list = result.objects; list != NULL; list = list->next) {
                    if (CABI_IsOperatorNew(OBJECT(list->object)))
                        return OBJECT(list->object);
                }
            }
        }
        return newh_func;
    }
    return NULL;
}

static inline void CABI_RegisterVBaseDtor(Statement *stmt, VClassList *vbase)
{
    Object *dtor;

    if ((dtor = CClass_Destructor(vbase->base)))
        CExcept_RegisterMember(stmt, CABI_ThisArg(), vbase->offset, dtor, CABI_FlagArg(), 0);
}

void CABI_InsertConstructorInitialization(Object *obj, Statement *stmt, TypeClass *tclass,
                                          Statement *(*callback)(Statement *, TypeClass *, TypeClass *, SInt32,
                                                                 Boolean),
                                          Boolean has_try)
{
    Object *function;
    Object *constructor;
    Object *destructor;
    Statement *current;
    CLabel *label;
    ENode *expr;
    VClassList *virtualBase;
    ClassList *base;
    ObjMemberVar *member;
    CtorChain *initializer;
    ENode *args;
    ENode *constructorRef;
    ENode *destructorRef;
    Type *type;
    VTable *vtable;

    current = stmt;
    if ((function = CABI_ConstructorCallsNew(tclass))) {
        label = newlabel();
        current = CFunc_InsertAfterStatement(ST_IFGOTO, stmt);
        current->expr = CABI_MakeThisExpr(NULL, 0);
        current->label = label;
        expr = funccallexpr(function, intconstnode((Type *)&stunsignedlong, tclass->size), NULL, NULL, NULL);
        expr = makediadicnode(CABI_MakeThisExpr(NULL, 0), expr, EASS);
        current = CFunc_InsertAfterStatement(ST_IFGOTO, current);
        current->expr = (ENode *)expr;
        current->label = label;
        current = CFunc_InsertAfterStatement(ST_RETURN, current);
        current->expr = NULL;
        current = CFunc_InsertAfterStatement(ST_LABEL, current);
        current->label = label;
        label->stmt = current;
    }

    if (has_try) {
        for (current = stmt;; current = current->next) {
            CError_ASSERT(1393, current);
            if (current->type == ST_BEGINCATCH)
                break;
        }
    }

    if (!tclass->sominfo) {
        if (tclass->flags & CLASS_HAS_VBASES) {
            label = newlabel();
            current = CFunc_InsertAfterStatement(ST_IFGOTO, current);
            expr = create_objectnode(CABI_FlagArg());
            current->expr = CExpr_New_EEQU_Node(expr, intconstnode((Type *)&stsignedshort, 0));
            current->label = label;
            current = CABI_InitVBasePtrs(current, tclass);

            for (virtualBase = tclass->vbases; virtualBase; virtualBase = virtualBase->next) {
                if (!callback) {
                    for (initializer = ctor_chain; initializer; initializer = initializer->next) {
                        if (initializer->what == INIT_VBASE && initializer->u.vbase == virtualBase)
                            break;
                    }
                    if (initializer) {
                        current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                        current->expr = initializer->objexpr;
                    } else if (CClass_Constructor(virtualBase->base)) {
                        if ((function = (constructor = CClass_DefaultConstructor(virtualBase->base)))) {
                            args = NULL;
                            if (virtualBase->base->flags & CLASS_HAS_VBASES)
                                args = intconstnode((Type *)&stsignedshort, 0);
                            current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                            current->expr =
                                funccallexpr(function, CABI_MakeThisExpr(NULL, virtualBase->offset), args, NULL, NULL);
                        } else {
                            CError_ReportError(ERR_CANNOT_CONSTRUCT_BASE_CLASS, virtualBase->base->classname->name);
                        }
                    }
                } else {
                    current = callback(current, tclass, virtualBase->base, virtualBase->offset, 1);
                }
                CABI_RegisterVBaseDtor(current, virtualBase);
            }

            current = CFunc_InsertAfterStatement(ST_LABEL, current);
            current->label = label;
            label->stmt = current;
        }

        for (base = tclass->bases; base; base = base->next) {
            if (!base->is_virtual) {
                if (!callback) {
                    for (initializer = ctor_chain; initializer; initializer = initializer->next) {
                        if (initializer->what == INIT_BASE && initializer->u.base == base)
                            break;
                    }
                    if (initializer) {
                        current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                        current->expr = initializer->objexpr;
                    } else if (CClass_Constructor(base->base)) {
                        if ((function = (constructor = CClass_DefaultConstructor(base->base)))) {
                            args = NULL;
                            if (base->base->flags & CLASS_HAS_VBASES)
                                args = intconstnode((Type *)&stsignedshort, 0);
                            current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                            current->expr =
                                funccallexpr(function, CABI_MakeThisExpr(NULL, base->offset), args, NULL, NULL);
                        } else {
                            CError_ReportError(ERR_CANNOT_CONSTRUCT_BASE_CLASS, base->base->classname->name);
                        }
                    }
                } else {
                    current = callback(current, tclass, base->base, base->offset, 1);
                }
                if ((function = (Object *)(destructor = CClass_Destructor((TypeClass *)base->base))))
                    CExcept_RegisterMember(current, CABI_ThisArg(), base->offset, function, NULL, 0);
            }
        }

        if ((vtable = tclass->vtable) && vtable->object && vtable->owner == tclass) {
            base_path_depth = 0;
            trans_vtboffsets = NULL;
            current = assign_vtable_pointers(current, tclass->vtable->object, tclass, tclass, 0, 0);
        }
    }

    if (!tclass->sominfo && (tclass->flags & CLASS_SOM_INIT))
        current = assign_vbase_ctor_offsets(current, tclass);

    if (!callback) {
        for (member = tclass->ivars; member; member = member->next) {
            for (initializer = ctor_chain; initializer; initializer = initializer->next) {
                if (initializer->what == INIT_MEMBER && initializer->u.membervar == member)
                    break;
            }
            if (initializer) {
                current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                current->expr = initializer->objexpr;
                switch ((SInt8)(type = member->type)->type) {
                    case TYPEARRAY:
                        do {
                            type = ((TypePointer *)type)->target;
                        } while (type->type == TYPEARRAY);
                        if (type->type == TYPECLASS &&
                            (function = (destructor = CClass_Destructor((TypeClass *)type)))) {
                            CError_ASSERT(1529, type->size);
                            CExcept_RegisterMemberArray(current, CABI_ThisArg(), member->offset, function,
                                                        member->type->size / type->size, type->size);
                        }
                        break;
                    case TYPECLASS:
                        if ((function = (Object *)(destructor = CClass_Destructor((TypeClass *)type))))
                            CExcept_RegisterMember(current, CABI_ThisArg(), member->offset, function, NULL, 1);
                        break;
                }
            } else {
                switch ((SInt8)(type = member->type)->type) {
                    case TYPEARRAY:
                        do {
                            type = ((TypePointer *)type)->target;
                        } while (type->type == TYPEARRAY);
                        if (type->type == TYPECLASS && CClass_Constructor((TypeClass *)type)) {
                            if ((function = (constructor = CClass_DefaultConstructor((TypeClass *)type)))) {
                                CError_ASSERT(1558, type->size);
                                constructorRef = create_objectrefnode(function);
                                if ((function = (Object *)(destructor = CClass_Destructor((TypeClass *)type))))
                                    destructorRef = create_objectrefnode(function);
                                else
                                    destructorRef = nullnode();
                                current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                                current->expr = CExpr_FuncCallSix(
                                    class_array_initializer, CABI_MakeThisExpr(tclass, member->offset), constructorRef,
                                    destructorRef, intconstnode((Type *)&stsignedlong, type->size),
                                    intconstnode((Type *)&stsignedlong, member->type->size / type->size), NULL);
                                if (function)
                                    CExcept_RegisterMemberArray(current, CABI_ThisArg(), member->offset, function,
                                                                member->type->size / type->size, type->size);
                            } else {
                                CError_ReportError(ERR_CANNOT_CONSTRUCT_DIRECT_MEMBER, member->name->name);
                            }
                        }
                        break;
                    case TYPECLASS:
                        if (CClass_Constructor((TypeClass *)type)) {
                            if ((function = (constructor = CClass_DefaultConstructor((TypeClass *)type)))) {
                                args = NULL;
                                if (((TypeClass *)type)->flags & CLASS_HAS_VBASES)
                                    args = intconstnode((Type *)&stsignedshort, 1);
                                current = CFunc_InsertAfterStatement(ST_EXPRESSION, current);
                                current->expr =
                                    funccallexpr(function, CABI_MakeThisExpr(tclass, member->offset), args, NULL, NULL);
                            } else {
                                CError_ReportError(ERR_CANNOT_CONSTRUCT_DIRECT_MEMBER, member->name->name);
                            }
                            if ((function = (Object *)(destructor = CClass_Destructor((TypeClass *)type))))
                                CExcept_RegisterMember(current, CABI_ThisArg(), member->offset, function, NULL, 1);
                        }
                        break;
                }
            }
        }
    } else {
        current = callback(current, tclass, NULL, 0, 1);
    }

    if (!tclass->sominfo) {
        for (current = stmt->next; current; current = current->next) {
            if (current->type == ST_RETURN) {
                CError_ASSERT(1619, !current->expr);
                current->expr = CABI_MakeThisExpr(NULL, 0);
            }
        }
    }
}

void CABI_GenerateClassFunction(TypeClass *cl, Object *func)
{
    CScopeSave save;
    Statement fg;
    Statement stmt;
    UInt8 ef;
    UInt8 savesym;

    if (anyerrors != 0 || func->access == ACCESSNONE)
        return;

    CABI_ApplyClassFlags(func, cl->eflags, 0);
    CScope_SetFunctionScope(func, &save);
    CFunc_FuncGenSetup(&fg, func);

    savesym = copts.filesyminfo;
    copts.filesyminfo = 0;
    CFunc_SetupNewFuncArgs(func, TYPE_FUNC(func->type)->args);
    ctor_chain = NULL;

    if (cl->flags & CLASS_HAS_VBASES) {
        arguments->next->object->name = CParser_GetUniqueName();
    }

    fg.next = &stmt;
    memclrw(&stmt, sizeof(stmt));
    stmt.type = ST_RETURN;
    CABI_InsertConstructorInitialization(func, &fg, cl, NULL, 0);
    CFunc_CodeCleanup(&fg);
    CFunc_Gen(&fg, func, 0);
    CScope_RestoreScope(&save);
    copts.filesyminfo = savesym;
}

static ENode *make_class_assignment(TypeClass *tclass, ENode *this_expr, ENode *src)
{
    ENode *expr;
    Object *func;

    if (!(func = CClass_AssignmentOperator(tclass))) {
        expr = makemonadicnode(this_expr, EINDIRECT);
        expr->rtype = TYPE(tclass);
        return makediadicnode(expr, src, EASS);
    } else {
        return funccallexpr(func, this_expr, getnodeaddress(src, 0), NULL, NULL);
    }
}

OffsetEntry *fn_0050bf30(OffsetEntry *list, Type *type, SInt32 offset, Boolean flag)
{
    OffsetEntry *e;
    SInt32 end;

    if (type->type == TYPEBITFIELD)
        type = TYPE_BITFIELD(type)->bitfieldtype;
    end = offset + type->size;

    if (flag) {
        for (e = list; e; e = e->next) {
            if (e->flag) {
                if (e->start <= offset && e->end >= end)
                    return list;
                if (e->start >= offset && e->end <= end) {
                    e->type = type;
                    e->start = offset;
                    e->end = end;
                    for (e = e->next; e; e = e->next) {
                        if (e->start >= offset && e->end <= end)
                            e->end = e->start;
                    }
                    return list;
                }
            }
        }
    }

    if (list) {
        for (e = list; e->next; e = e->next)
            ;
        e->next = lalloc(sizeof(OffsetEntry));
        e = e->next;
    } else {
        list = e = lalloc(sizeof(OffsetEntry));
    }
    e->next = NULL;
    e->type = type;
    e->start = offset;
    e->end = end;
    e->flag = flag;
    return list;
}

Statement *make_baseclass_and_ivars_copy_statements(Statement *stmt, TypeClass *tclass, TypeClass *baseclass,
                                                    SInt32 offset, Boolean flag)
{
    ENode *expr;
    ENode *src;
    ENode *this_expr;
    ENodeList *args;
    Object *func;
    Object *dtor;
    ObjMemberVar *ivar;
    OffsetEntry *regions;
    Type *type;
    SInt32 i;
    SInt32 count;
    SInt32 off;

    if (baseclass) {
        if (baseclass->flags & 0x1000) {
            if ((flag && !CClass_CopyConstructor(baseclass)) || (!flag && !CClass_AssignmentOperator(baseclass)))
                return stmt;
        }

        src = CABI_SourceArg(tclass, flag);
        src->rtype = TYPE(baseclass);
        src->data.monadic = CClass_DirectBasePointerCast(src->data.monadic, tclass, baseclass);

        stmt = CFunc_InsertAfterStatement(ST_EXPRESSION_0050b120, stmt);
        if (flag) {
            args = lalloc(sizeof(ENodeList));
            args->next = NULL;
            args->node = src;
            stmt->expr = CExpr_ConstructObject(TYPE(baseclass), CABI_MakeThisExpr(NULL, offset), args, 1, 0, 0, 0, 1);
        } else {
            this_expr = CClass_DirectBasePointerCast(CABI_MakeThisExpr(NULL, 0), tclass, baseclass);
            expr = make_class_assignment(baseclass, this_expr, src);
            stmt->expr = expr;
        }
    } else {
        for (ivar = tclass->ivars, regions = NULL; ivar; ivar = ivar->next) {
            if (ivar->name == vtable_name)
                continue;

            switch ((SInt8)(type = ivar->type)->type) {
                case TYPEARRAY:
                    while (type->type == TYPEARRAY)
                        type = TYPE_POINTER(type)->target;
                    if (type->type != TYPECLASS) {
                        regions = fn_0050bf30(regions, ivar->type, ivar->offset, 1);
                        break;
                    }
                case TYPECLASS:
                    if (flag) {
                        if (CClass_CopyConstructor(TYPE_CLASS(type)) || CClass_CopyConstructor(TYPE_CLASS(type))) {
                            regions = fn_0050bf30(regions, ivar->type, ivar->offset, 0);
                            break;
                        }
                    } else {
                        if (CClass_AssignmentOperator(TYPE_CLASS(type))) {
                            regions = fn_0050bf30(regions, ivar->type, ivar->offset, 0);
                            break;
                        }
                    }
                default:
                    regions = fn_0050bf30(regions, ivar->type, ivar->offset, 1);
                    break;
            }
        }

        for (; regions; regions = regions->next) {
            if (regions->start >= regions->end)
                continue;

            type = regions->type;
            src = CABI_SourceArg(tclass, flag);
            src->rtype = type;
            if (!canadd(src->data.monadic, regions->start)) {
                src->data.monadic =
                    makediadicnode(src->data.monadic, intconstnode(TYPE(&stunsignedlong), regions->start), EADD);
                optimizecomm(src->data.monadic);
            }

            if (!regions->flag) {
                if (type->type == TYPEARRAY) {
                    while (type->type == TYPEARRAY)
                        type = TYPE_POINTER(type)->target;
                    CError_ASSERT(1875, type->type == TYPECLASS);
                    if (!type->size)
                        continue;

                    count = regions->type->size / type->size;
                    for (i = 0, off = regions->start; i < count; i++, off += type->size) {
                        src = CABI_SourceArg(tclass, flag);
                        src->rtype = type;
                        if (!canadd(src->data.monadic, off)) {
                            src->data.monadic =
                                makediadicnode(src->data.monadic, intconstnode(TYPE(&stunsignedlong), off), EADD);
                            optimizecomm(src->data.monadic);
                        }

                        stmt = CFunc_InsertAfterStatement(ST_EXPRESSION_0050b120, stmt);
                        if (flag) {
                            args = lalloc(sizeof(ENodeList));
                            memclrw(args, sizeof(ENodeList));
                            args->node = src;
                            stmt->expr =
                                CExpr_ConstructObject(type, CABI_MakeThisExpr(tclass, off), args, 1, 1, 0, 1, 1);
                        } else {
                            expr = make_class_assignment(TYPE_CLASS(type), CABI_MakeThisExpr(tclass, off), src);
                            stmt->expr = expr;
                        }
                    }

                    if (flag && (dtor = CClass_Destructor(TYPE_CLASS(type))))
                        CExcept_RegisterMemberArray(stmt, CABI_ThisArg(), regions->start, dtor, count, type->size);
                    continue;
                }

                CError_ASSERT(1909, type->type == TYPECLASS);
                stmt = CFunc_InsertAfterStatement(ST_EXPRESSION_0050b120, stmt);
                if (flag) {
                    args = lalloc(sizeof(ENodeList));
                    memclrw(args, sizeof(ENodeList));
                    args->node = src;
                    stmt->expr =
                        CExpr_ConstructObject(type, CABI_MakeThisExpr(tclass, regions->start), args, 1, 1, 0, 1, 1);
                    if ((dtor = CClass_Destructor(TYPE_CLASS(type))))
                        CExcept_RegisterMember(stmt, CABI_ThisArg(), regions->start, dtor, NULL, 1);
                    continue;
                }

                expr = make_class_assignment(TYPE_CLASS(type), CABI_MakeThisExpr(tclass, regions->start), src);
            } else {
                if (type->type == TYPEARRAY) {
                    if (type->size > 1 && ((regions->start & 1) || (type->size & 1))) {
                        stmt = CFunc_InsertAfterStatement(ST_EXPRESSION_0050b120, stmt);
                        stmt->expr =
                            funccallexpr(data_005870d8, CABI_MakeThisExpr(tclass, regions->start),
                                         getnodeaddress(src, 0), intconstnode(TYPE(&stunsignedlong), type->size), NULL);
                        continue;
                    }
                    type = CDecl_NewStructType(type->size, 4);
                    src->rtype = type;
                }
                this_expr = makemonadicnode(CABI_MakeThisExpr(tclass, regions->start), EINDIRECT);
                this_expr->rtype = type;
                stmt = CFunc_InsertAfterStatement(ST_EXPRESSION_0050b120, stmt);
                expr = makediadicnode(this_expr, src, EASS);
            }
            stmt->expr = expr;
        }
    }

    return stmt;
}

void CABI_GenClassFunction(TypeClass *tclass, Object *function)
{
    CScopeSave scopeSave;
    Statement body;
    Statement returnStatement;
    UInt8 savedFileSymInfo;
    UInt8 classFlags;

    if (anyerrors || function->access == ACCESSNONE)
        return;

    CABI_ApplyClassFlags(function, tclass->eflags, 0);
    CScope_SetFunctionScope(function, &scopeSave);
    CFunc_FuncGenSetup(&body, function);

    savedFileSymInfo = copts.filesyminfo;
    copts.filesyminfo = 0;
    CFunc_SetupNewFuncArgs(function, ((TypeFunc *)function->type)->args);
    ctor_chain = NULL;

    if (tclass->flags & CLASS_HAS_VBASES) {
        HashNameNode *name = CParser_GetUniqueName();
        arguments->next->object->name = name;
    }

    body.next = &returnStatement;
    memclrw(&returnStatement, sizeof(Statement));
    returnStatement.type = ST_RETURN;

    CABI_InsertConstructorInitialization(function, &body, tclass, make_baseclass_and_ivars_copy_statements, 0);
    CFunc_CodeCleanup(&body);
    CFunc_Gen(&body, function, 0);
    CScope_RestoreScope(&scopeSave);
    copts.filesyminfo = savedFileSymInfo;
}

void CABI_MakeDefaultConstructor(TypeClass *cls, Object *func)
{
    CScopeSave save;
    Statement stmt;
    UInt8 saved;
    Statement *acc;
    VClassList *vb;
    ClassList *cb;
    ENode *node;
    Statement *ret;

    if (anyerrors != 0 || func->access == ACCESSNONE)
        return;
    CABI_ApplyClassFlags(func, cls->eflags, 0);

    CScope_SetFunctionScope(func, &save);
    CFunc_FuncGenSetup(&stmt, func);
    saved = copts.filesyminfo;
    copts.filesyminfo = 0;
    CFunc_SetupNewFuncArgs(func, TYPE_FUNC(func->type)->args);

    acc = curstmt;
    for (vb = cls->vbases; vb != NULL; vb = vb->next)
        acc = make_baseclass_and_ivars_copy_statements(acc, cls, vb->base, vb->offset, 0);

    for (cb = cls->bases; cb != NULL; cb = cb->next) {
        if (!cb->is_virtual)
            acc = make_baseclass_and_ivars_copy_statements(acc, cls, cb->base, cb->offset, 0);
    }

    acc = make_baseclass_and_ivars_copy_statements(acc, cls, NULL, 0, 0);

    ret = CFunc_InsertAfterStatement(8, acc);

    node = CABI_MakeThisExpr(NULL, 0);
    ret->expr = node;

    CFunc_CodeCleanup(&stmt);
    CFunc_Gen(&stmt, func, 0);
    CScope_RestoreScope(&save);
    copts.filesyminfo = saved;
}

Statement *destroy_members(Statement *expr, ObjMemberVar *member, TypeClass *cls)
{
    Type *type;
    ENode *base;
    Object *dtor;
    SInt32 offset;
    for (; member != NULL; member = member->next) {
        type = member->type;
        if (type->type == TYPEARRAY) {
            while (type->type == TYPEARRAY)
                type = TPTR_TARGET(type);
            if (type->type == TYPECLASS) {
                if ((dtor = CClass_Destructor((TypeClass *)type)) != NULL) {
                    expr = destroy_members(expr, member->next, cls);
                    dtor = CABI_GetDestructorObject(dtor, 1);
                    return destroy_array(expr, member, cls, type, dtor);
                }
            }
        } else if (type->type == TYPECLASS) {
            if ((dtor = CClass_Destructor((TypeClass *)type)) != NULL) {
                expr = destroy_members(expr, member->next, cls);
                expr = CFunc_InsertAfterStatement(4, expr);
                offset = member->offset;
                base = CABI_MakeThisExpr(cls, offset);
                expr->expr = CABI_DestroyObject(dtor, base, 1, 1, 0);
                return expr;
            }
        }
    }
    return expr;
}

Statement *destroy_nonvirtual_bases(Statement *acc, ClassList *list)
{
    Object *dtor;
    SInt32 count;
    SInt32 i;
    ClassList *p;
    SInt32 offset;
    ENode *node;

    p = list;
    count = 0;
    while (p != NULL) {
        p = p->next;
        count++;
    }
    for (; count > 0; count--) {
        i = 1;
        p = list;
        while (i < count) {
            i++;
            p = p->next;
        }
        if (!p->is_virtual && (dtor = CClass_Destructor(p->base)) != NULL) {
            acc = CFunc_InsertAfterStatement(4, acc);
            offset = p->offset;
            acc->expr = CABI_DestroyObject(dtor, CABI_MakeThisExpr(NULL, offset), 0, 1, 0);
        }
    }
    return acc;
}

Statement *build_base_destruction_statements(Statement *stmt, VClassList *bl)
{
    Object *dtor;

    while (bl != NULL) {
        if ((dtor = CClass_Destructor(bl->base)) != NULL) {
            stmt = build_base_destruction_statements(stmt, bl->next);
            stmt = CFunc_InsertAfterStatement(EINDIRECT, stmt);
            stmt->expr = CABI_DestroyObject(dtor, CABI_MakeThisExpr(NULL, bl->offset), 0, 1, 0);
            break;
        }
        bl = bl->next;
    }
    return stmt;
}

void CABI_TransDestructor(Object *destructor, Object *completeDestructor, Statement *stmt, TypeClass *tclass, int mode)
{
    Statement *current;
    CLabel *exitLabel;
    ENode *node;
    Object *deleteFunction;
    Statement *next;
    Statement *conditional;
    CLabel *label;
    Boolean destroyBases;
    Boolean handleDelete;
    Boolean destroyVirtualBases;
    Boolean destroyMembers;
    FuncArg *deleteArgs;

    if (tclass->sominfo != NULL) {
        handleDelete = destroyBases = destroyVirtualBases = 0;
        destroyMembers = 1;
    } else {
        handleDelete = destroyBases = destroyMembers = destroyVirtualBases = 1;
    }

    label = newlabel();

    current = stmt;
    if (current != NULL) {
        do {
            if (current->type == ST_RETURN) {
                CError_ASSERT(2297, current->expr == 0);
                current->type = ST_GOTO;
                current->label = label;
            }
            if ((next = current->next) != NULL && next->type == ST_RETURN && next->next == NULL) {
                CError_ASSERT(2302, next->expr == 0);
                current->next = NULL;
                break;
            }
            current = next;
        } while (next != NULL);
    }

    current = stmt;
    if (handleDelete) {
        exitLabel = newlabel();
        current = CFunc_InsertAfterStatement(7, stmt);
        CError_ASSERT(922, arguments != 0 && arguments->object->type->type == TYPEPOINTER);
        node = create_objectnode(arguments->object);
        node->rtype = (Type *)&void_ptr;
        current->expr = node;
        current->label = exitLabel;
    }

    if (destroyBases && tclass->vtable != NULL && ((VTable *)tclass->vtable)->object != NULL &&
        ((VTable *)tclass->vtable)->owner == tclass) {
        base_path_depth = 0;
        trans_vtboffsets = NULL;
        current = assign_vtable_pointers(current, ((VTable *)tclass->vtable)->object, tclass, tclass, 0, 0);
    }

    if (tclass->sominfo == NULL && (tclass->flags & CLASS_SOM_INIT) != 0) {
        assign_vbase_ctor_offsets(current, tclass);
    }

    next = stmt;
    while (next->next != NULL)
        next = next->next;
    current = CFunc_InsertAfterStatement(ST_LABEL, next);
    current->label = label;
    current->dobjstack = NULL;
    label->stmt = current;

    if (destroyMembers && (tclass->flags & CLASS_HANDLEOBJECT) == 0) {
        current = destroy_members(current, tclass->ivars, tclass);
    }

    if (destroyBases && tclass->bases != NULL) {
        current = destroy_nonvirtual_bases(current, tclass->bases);
    }

    if (destroyVirtualBases && (tclass->flags & CLASS_HAS_VBASES) != 0) {
        label = newlabel();
        current = CFunc_InsertAfterStatement(7, current);
        CError_ASSERT(967, arguments != 0 && arguments->next != 0 && arguments->next->object->type->type == TYPEINT);
        node = create_objectnode(arguments->next->object);
        current->expr = node;
        current->label = label;
        current = build_base_destruction_statements(current, tclass->vbases);
        current = CFunc_InsertAfterStatement(ST_LABEL, current);
        current->label = label;
        label->stmt = current;
    }

    if (handleDelete) {
        conditional = CFunc_InsertAfterStatement(ST_IFGOTO, current);
        CError_ASSERT(967, arguments != 0 && arguments->next != 0 && arguments->next->object->type->type == TYPEINT);
        node = create_objectnode(arguments->next->object);
        node = CExpr_New_ELESSEQU_Node(node, intconstnode((Type *)&stsignedshort, 0));
        conditional->expr = node;
        conditional->label = exitLabel;
        current = CFunc_InsertAfterStatement(ST_EXPRESSION, conditional);
        deleteFunction = CParser_FindDeallocationObject((Type *)tclass, 0, 0);
        if ((deleteArgs = ((TypeFunc *)deleteFunction->type)->args) != NULL && deleteArgs->next != NULL) {
            CError_ASSERT(922, arguments != 0 && arguments->object->type->type == TYPEPOINTER);
            node = create_objectnode(arguments->object);
            node->rtype = (Type *)&void_ptr;
            current->expr =
                funccallexpr(deleteFunction, node, intconstnode((Type *)&stunsignedlong, tclass->size), NULL, NULL);
        } else {
            CError_ASSERT(922, arguments != 0 && arguments->object->type->type == TYPEPOINTER);
            node = create_objectnode(arguments->object);
            node->rtype = (Type *)&void_ptr;
            current->expr = funccallexpr(deleteFunction, node, NULL, NULL, NULL);
        }
        current = CFunc_InsertAfterStatement(ST_LABEL, current);
        current->label = exitLabel;
        exitLabel->stmt = current;
    }

    current = CFunc_InsertAfterStatement(ST_RETURN, current);
    if (tclass->sominfo != NULL) {
        current->expr = NULL;
    } else {
        CError_ASSERT(922, arguments != 0 && arguments->object->type->type == TYPEPOINTER);
        node = create_objectnode(arguments->object);
        node->rtype = (Type *)&void_ptr;
        current->expr = node;
    }
}

void CABI_MakeDefaultDestructor(TypeClass *tclass, Object *func)
{
    Boolean savedebuginfo;
    CScopeSave savedscope;
    Statement firststmt;
    Statement returnstmt;

    if (anyerrors || func->access == ACCESSNONE)
        return;

    CABI_ApplyClassFlags(func, tclass->eflags, 0);
    CScope_SetFunctionScope(func, &savedscope);
    CFunc_FuncGenSetup(&firststmt, func);
    savedebuginfo = copts.filesyminfo;
    copts.filesyminfo = 0;
    CFunc_SetupNewFuncArgs(func, TYPE_FUNC(func->type)->args);

    firststmt.next = &returnstmt;
    memclrw(&returnstmt, sizeof(Statement));
    returnstmt.type = ST_RETURN;

    CFunc_CodeCleanup(&firststmt);
    CABI_TransDestructor(func, func, &firststmt, tclass, 0);
    CFunc_Gen(&firststmt, func, 0);
    CScope_RestoreScope(&savedscope);
    copts.filesyminfo = savedebuginfo;
}

Object *CABI_GetDestructorObject(Object *obj, UInt8 mode)
{
    return obj;
}

ENode *CABI_DestroyObject(Object *dtor, ENode *objexpr, UInt8 mode, Boolean flag1, Boolean flag2)
{
    ENode *expr;
    ENodeList *list;
    short val;

    switch (mode) {
        case 2:
        case 3:
            if (flag2)
                val = 1;
            else
                val = -1;
            break;
        case 1:
            val = -1;
            break;
        case 0:
            val = 0;
            break;
        default:
            CError_FATAL(2751);
    }

    expr = lalloc(sizeof(ENode));
    expr->type = EFUNCCALL;
    expr->cost = 200;
    expr->flags = 0;
    expr->rtype = &stvoid;
    expr->data.funccall.funcref = create_objectrefnode(dtor);
    if (flag1)
        expr->data.funccall.funcref->flags |= ENODE_FLAG_80;
    expr->data.funccall.functype = TYPE_FUNC(dtor->type);
    dtor->flags |= OBJECT_USED;

    list = lalloc(sizeof(ENodeList));
    list->node = objexpr;
    expr->data.funccall.args = list;
    list->next = lalloc(sizeof(ENodeList));
    list = list->next;
    list->next = NULL;
    list->node = intconstnode(TYPE(&stsignedshort), val);
    return expr;
}
