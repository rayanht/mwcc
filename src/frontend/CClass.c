#define CERROR_FILE "CClass.c"
#include "compiler/common.h"
#include "compiler/CClass.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
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
#include "compiler/CRTTI.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/Exceptions.h"
#include "compiler/InlineAsm.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/Registers.h"
#include "compiler/Switch.h"
#include "driver/Files.h"
#include "compiler/ENode.h"

typedef struct CClassObj CClassObj;

typedef enum OverrideKind { OVERRIDE_NONE, OVERRIDE_1, OVERRIDE_2 } OverrideKind;

typedef enum { OV_NONE, OV_NORMAL, OV_COVARIANT } CovarianceKind;

#pragma options align = mac68k
static TypeClass *current_class;
static PendingThunk *pending_thunks;
static TypeClass *base_path_class;
static SInt32 base_path_offset;
static UInt8 base_path_status;
static UInt8 *data_00581caa;
static SInt32 vtable_size;
static void *rtti_offset_table;
static void *data_00581cb6;
static OverrideClass *virtual_base_layout;
static OverrideClass *root_class_layout;
static SInt32 data_00581cc2;
static Boolean data_00581cc6;
static void *data_00581cc8;
static void *data_00581ccc;
static void *data_00581cd0;
static void *data_00581cd4;
static void *data_00581cd8;
static void *data_00581cdc;
#pragma options align = reset

ENode *CClass_AccessMember(ENode *node, Type *type, UInt32 quals, int value)
{
    Type *savedtype;
    Type *t;
    Type *p;

    savedtype = NULL;
    if (node->rtype->type == TYPECLASS && (TYPE_CLASS(node->rtype)->flags & CLASS_HANDLEOBJECT)) {
        node = makemonadicnode(node, EINDIRECT);
        node->data.monadic->rtype = CDecl_NewPointerType(node->rtype);
    }
    if (type->type == TYPEBITFIELD) {
        savedtype = TYPE_BITFIELD(type)->bitfieldtype;
        narrow_bitfield_type(&type, &value);
    }
    if (value != 0 && canadd(node->data.monadic, value) == 0) {
        node->data.monadic = makediadicnode(node->data.monadic, intconstnode((Type *)&stunsignedlong, value), EADD);
        optimizecomm(node->data.monadic);
    }
    if (savedtype != NULL) {
        if (type->type == TYPEBITFIELD) {
            node->data.monadic = makemonadicnode(node->data.monadic, EBITFIELD);
            node->data.monadic->rtype = type;
            node->rtype = TYPE_BITFIELD(type)->bitfieldtype;
        } else {
            node->rtype = type;
        }
    } else {
        node->rtype = type;
    }
    if (quals & Q_MUTABLE)
        node->flags &= 0xfffe;
    t = node->rtype;
    while (t->type == TYPEARRAY)
        t = TPTR_TARGET(t);
    if (t->type == TYPEPOINTER) {
        if ((node->flags & 3) != 0) {
            node->rtype = copy_pointer_array_type(node->rtype);
            p = node->rtype;
            while (p->type == TYPEARRAY)
                p = TPTR_TARGET(p);
            if (p->type != TYPEPOINTER)
                CError_FATAL(3242);
            TYPE_POINTER(p)->qual |= node->flags;
        }
        node->flags = quals & Q_CV;
    } else {
        node->flags |= quals & Q_CV;
    }
    return node;
}

void narrow_bitfield_type(Type **type, int *displacement)
{
    struct TypeBitfield *bitField;
    Type *baseType;
    struct TypeBitfield *narrowed;
    short byteOffset, lastBit, narrowOffset, wordOffset;
    short firstBit;

    bitField = (struct TypeBitfield *)*type;
    baseType = bitField->bitfieldtype;
    if (bitField->bitlength == 8) {
        switch (bitField->offset) {
            case 0:
                byteOffset = 0;
                break;
            case 8:
                byteOffset = 1;
                break;
            case 16:
                byteOffset = 2;
                break;
            case 24:
                byteOffset = 3;
                break;
            default:
                byteOffset = -1;
        }
        if (byteOffset >= 0) {
            if (baseType->size != 1) {
                if (is_unsigned(bitField->bitfieldtype))
                    *type = (Type *)&stunsignedchar;
                else
                    *type = (Type *)&stsignedchar;
            } else
                *type = baseType;
            *displacement += byteOffset;
            return;
        }
    }
    if (bitField->bitlength == 16) {
        switch (bitField->offset) {
            case 0:
                byteOffset = 0;
                break;
            case 16:
                byteOffset = 2;
                break;
            default:
                byteOffset = -1;
        }
        if (byteOffset >= 0) {
            if (baseType->size != 2) {
                if (is_unsigned(baseType))
                    *type = (Type *)&stunsignedshort;
                else
                    *type = (Type *)&stsignedshort;
            } else
                *type = baseType;
            *displacement += byteOffset;
            return;
        }
    }
    if (bitField->bitlength == 32 && bitField->offset == 0) {
        if (baseType->size != 4) {
            if (is_unsigned(baseType))
                *type = (Type *)&stunsignedlong;
            else
                *type = (Type *)&stsignedlong;
        } else
            *type = baseType;
        return;
    }
    if (bitField->size == 1)
        return;
    firstBit = bitField->offset;
    lastBit = bitField->bitlength + firstBit - 1;
    if (bitField->bitlength < 8 && ((firstBit & 0xFFF8U) == (lastBit & 0xFFF8U))) {
        narrowed = galloc(sizeof(*narrowed));
        narrowOffset = 0;
        *narrowed = *(struct TypeBitfield *)*type;
        *type = (Type *)narrowed;
        if (narrowed->offset >= 8)
            narrowOffset++;
        if (narrowed->offset >= 16)
            narrowOffset = 2;
        if (narrowed->offset >= 24)
            narrowOffset = 3;
        *displacement += narrowOffset;
        narrowed->offset -= (long)narrowOffset << 3;
        narrowed->bitfieldtype = is_unsigned(baseType) ? (Type *)&stunsignedchar : (Type *)&stsignedchar;
        narrowed->size = narrowed->bitfieldtype->size;
        return;
    }
    if (bitField->size == 2)
        return;
    if (bitField->bitlength < 16 && ((firstBit & 0xFFF0U) == (lastBit & 0xFFF0U))) {
        narrowed = galloc(sizeof(*narrowed));
        *narrowed = *(struct TypeBitfield *)*type;
        *type = (Type *)narrowed;
        wordOffset = 0;
        if (narrowed->offset >= 16)
            wordOffset = 2;
        *displacement += wordOffset;
        narrowed->offset -= (long)wordOffset << 3;
        narrowed->bitfieldtype = is_unsigned(baseType) ? (Type *)&stunsignedshort : (Type *)&stsignedshort;
        narrowed->size = narrowed->bitfieldtype->size;
        return;
    }
}

Type *copy_pointer_array_type(Type *type)
{
    TypePointer *copy;

    switch ((SInt8)type->type) {
        case TYPEPOINTER:
        case TYPEARRAY:
            copy = (TypePointer *)galloc(sizeof(TypePointer));
            *copy = *(TypePointer *)type;
            copy->target = copy_pointer_array_type(copy->target);
            return (Type *)copy;
        default:
            return type;
    }
}

#define FT_PTR(o) TYPE_POINTER(((TypeMemberFunc *)(o)->type)->functype)

static TypeMemberFunc *copyMemberFunction(TypeMemberFunc *type)
{
    TypeMemberFunc *copy = (TypeMemberFunc *)galloc(sizeof(TypeMemberFunc));
    *copy = *type;
    return copy;
}

static SInt32 CL_FindOffset(TypeClass *base)
{
    VClassList *p;
    for (p = current_class->vbases; p != NULL; p = p->next)
        if (p->base == base)
            return p->offset;
    CError_FATAL(1182);
    return 0;
}

static SInt32 CL_FindVOffset(TypeClass *base)
{
    VClassList *p;
    for (p = current_class->vbases; p != NULL; p = p->next)
        if (p->base == base)
            return p->voffset;
    CError_FATAL(1199);
    return 0;
}

static SInt32 CClass_GetVBaseOffset(TypeClass *base)
{
    VClassList *v;

    for (v = base_path_class->vbases; v != NULL; v = v->next) {
        if (v->base == base)
            return v->offset;
    }
    CError_FATAL(1182);
    return 0;
}

static inline Boolean CClass_ShouldReplacePath(BClassList *current, BClassList *candidate)
{
    switch (get_path_access(current)) {
        case 0:
            return 0;
        case 3:
            return 1;
        default:
            CError_FATAL(800);
        case 2:
            switch (get_path_access(candidate)) {
                case 0:
                    return 1;
                case 1:
                case 2:
                case 3:
                    return 0;
                default:
                    CError_FATAL(809);
            }
        case 1:
            switch (get_path_access(candidate)) {
                case 0:
                case 2:
                    return 1;
                case 1:
                case 3:
                    return 0;
                default:
                    CError_FATAL(819);
                    return 0;
            }
    }
}

void CClass_CheckEnumAccess(BClassList *bases, ObjBase *object)
{
    UInt8 accessible;
    TypeEnum *enumType;
    if (bases != NULL) {
        bases = deduplicate_and_select_base_path_suffix(bases, NULL);
        if (bases != NULL) {
            accessible = check_base_path_access(bases, object->access);
            if (accessible == '\0') {
                CError_ReportError(ERR_ILLEGAL_ACCESS_PROTECTED_PRIVATE_MEMBER);
            }
            return;
        }
    }
    if (((ObjEnumConst *)object)->access != '\0' &&
        (enumType = (TypeEnum *)((ObjEnumConst *)object)->type)->type == TYPEENUM && enumType->nspace != NULL &&
        enumType->nspace->theclass != NULL) {
        CClass_CheckStaticAccess(NULL, enumType->nspace->theclass, object->access);
    }
}

void CClass_CheckObjectAccess(BClassList *bases, Object *reference)
{
    SInt16 lookup_result;
    Boolean lookup_flags;

    if (reference->nspace != NULL && reference->nspace->theclass != NULL) {
        if (bases == NULL && cscope_currentclass != NULL) {
            bases = CClass_GetBasePath(cscope_currentclass, reference->nspace->theclass, &lookup_result, &lookup_flags);
        }
        CClass_CheckStaticAccess(bases, reference->nspace->theclass, reference->access);
    }
}

void CClass_CheckStaticAccess(BClassList *type, TypeClass *owner, UInt8 access)
{
    ClassFriend *friendEntry;

    if (type != NULL) {
        type = deduplicate_and_select_base_path_suffix(type, owner);
        if (type != NULL && type->next != NULL) {
            if (!check_base_path_access(type, access))
                CError_ReportError(ERR_ILLEGAL_ACCESS_PROTECTED_PRIVATE_MEMBER);
            return;
        }
    }
    switch (access) {
        case 0:
            return;
        case 1:
        case 2:
            if (owner == cscope_currentclass)
                return;
            for (friendEntry = owner->friends; friendEntry != NULL; friendEntry = friendEntry->next) {
                if (friendEntry->isclass != 0) {
                    if (friendEntry->u.theclass == cscope_currentclass)
                        return;
                } else if (friendEntry->u.obj == cscope_currentfunc) {
                    return;
                }
            }
            /* fall through */
        case 3:
            CError_Warning(ERR_ILLEGAL_ACCESS_PROTECTED_PRIVATE_MEMBER);
            return;
        default:
            CError_FATAL(2970);
            return;
    }
}

BClassList *deduplicate_and_select_base_path_suffix(BClassList *p, TypeClass *base)
{
    BClassList *node;
    ClassList *q;
    BClassList *result;

    result = p;
    for (;;) {
        if ((node = p->next) == NULL) {
            if (base == NULL)
                return result;
            if (p->type != (Type *)base)
                result = NULL;
            return result;
        }
        if (p->type != node->type) {
            for (q = TYPE_CLASS(p->type)->bases; q != NULL; q = q->next) {
                if ((Type *)q->base == node->type)
                    break;
            }
            if (q != NULL)
                p = node;
            else {
                result = p = node;
            }
        } else {
            p->next = node->next;
        }
    }
}

void CClass_CheckBaseAccess(BClassList *bases, char access)
{
    Boolean result;

    result = check_base_path_access(bases, access);
    if (result == '\0') {
        CError_ReportError(ERR_ILLEGAL_ACCESS_PROTECTED_PRIVATE_MEMBER);
    }
}

Boolean check_base_path_access(BClassList *cl, UInt8 acc)
{
    TypeClass *cls = TYPE_CLASS(cl->type);
    BClassList *link;
    ClassList *base;
    BClassList *p;
    ClassFriend *friend;
    SInt8 access = acc;
    TypeClass *current;
    BClassList *save;

    if ((link = cl->next) != NULL) {
        if (access != 1) {
            current = cls;
            while (link != NULL) {
                for (base = current->bases; base != NULL; base = base->next) {
                    if ((Type *)base->base == link->type)
                        break;
                }
                if (base == NULL)
                    return 0;
                switch (base->access) {
                    case ACCESSNONE:
                        access = 3;
                        break;
                    case ACCESSPROTECTED:
                        if (access == 0)
                            access = 2;
                        break;
                    case ACCESSPRIVATE:
                        if (access == 1)
                            access = 3;
                        else
                            access = 1;
                        break;
                    case ACCESSPUBLIC:
                        break;
                }
                current = TYPE_CLASS(link->type);
                link = link->next;
            }
        } else {
            access = 3;
        }
    }
    if (access == 0)
        return 1;
    if (access != 3) {
        if (cscope_currentclass == cls)
            return 1;
        if (data_00587140 == cls)
            return 1;
        for (friend = (ClassFriend *)cls->friends; friend != NULL; friend = friend->next) {
            if (friend->isclass != 0) {
                if (friend->u.theclass == cscope_currentclass)
                    return 1;
            } else {
                if (friend->u.obj == cscope_currentfunc)
                    return 1;
            }
        }
    }
    for (p = cl; p->next != NULL; p = p->next) {
        if (check_base_path_access(p->next, acc)) {
            if ((save = p->next->next) != NULL || acc != 0) {
                p->next->next = NULL;
                if (check_base_path_access(cl, 0)) {
                    p->next->next = save;
                    return 1;
                }
                p->next->next = save;
            }
        }
    }
    return 0;
}

ENode *CClass_CreateThisSelfExpr(void)
{
    ENode *expr;
    ENode *result;
    Object *parsedType;

    parsedType = CClass_ThisSelfObject();
    if (!parsedType)
        return NULL;

    expr = create_objectrefnode(parsedType);
    expr->rtype = (Type *)CDecl_NewPointerType((Type *)cscope_currentclass);
    result = makemonadicnode(expr, EINDIRECT);
    result->data.monadic->rtype = (Type *)CDecl_NewPointerType(result->rtype);
    return result;
}

Object *CClass_ThisSelfObject(void)
{
    ObjectList *objects;
    if (cscope_currentfunc != NULL && cscope_currentclass != NULL) {
        if (cscope_currentclass->objcinfo != NULL) {
            objects = (ObjectList *)arguments;
            if (objects != NULL) {
                do {
                    if (objects->object->name == this_self_name)
                        return objects->object;
                    objects = objects->next;
                } while (objects != NULL);
            }
            CError_ReportError(ERR_ILLEGAL_USE_SELF);
        } else {
            if (cscope_is_member_func != 0) {
                objects = (ObjectList *)arguments;
                if (objects != NULL) {
                    do {
                        if (objects->object->name == this_arg_name)
                            return objects->object;
                        objects = objects->next;
                    } while (objects != NULL);
                }
            }
            CError_ReportError(ERR_ILLEGAL_USE_THIS);
        }
    }
    CError_ReportError((unsigned short)(copts.cplusplus != 0 ? 189 : 301));
    return NULL;
}

void CClass_MemberDef(Object *object, TypeClass *cls)
{
    switch ((SInt8)cls->action) {
        case 0:
        case 3:
            return;
        case 1:
            if (object->type->type == TYPEFUNC && (TYPE_FUNC(object->type)->flags & 4)) {
                if (object->qual & Q_INLINE) {
                    if (cls->sominfo != NULL)
                        CError_ReportError(ERR_SOM_CLASS_MUST_ONE_NON_INLINE);
                    cls->action = 0;
                    if (cls->vtable != NULL) {
                        cls->vtable->object->sclass = TK_STATIC;
                        cls->vtable->object->qual |= Q_IMPLICIT_WEAK;
                        if ((cls->vtable->object->flags & OBJECT_FLAGS_2) == 0)
                            CParser_NewCallBackAction(cls->vtable->object, cls);
                        else if (cprep_cu[0xe0] != 1)
                            CParser_NewClassAction(cls);
                    }
                    return;
                }
                if (cprep_cu[0xe0] != 1)
                    CParser_NewClassAction(cls);
            }
            return;
        case 2:
            CError_FATAL(2504);
            return;
        default:
            CError_FATAL(2523);
            return;
    }
}

Object *CClass_CheckPures(TypeClass *type)
{
    return create_root_class_layout(type);
}

void CClass_MakeStaticActionClass(TypeClass *theclass)
{
    if (theclass->vtable != NULL) {
        theclass->vtable->object->sclass = TK_STATIC;
        theclass->vtable->object->qual |= Q_IMPLICIT_WEAK;
        if ((theclass->vtable->object->flags & OBJECT_FLAGS_2) == 0) {
            CParser_NewCallBackAction(theclass->vtable->object, theclass);
        } else if (cprep_cu[0xe0] != 1) {
            CParser_NewClassAction(theclass);
        }
    }
}

void CClass_ClassAction(TypeClass *cls)
{
    SInt32 size;

    if (cls->sominfo != NULL) {
        CSOM_BuildClass(cls);
        return;
    }

    if (cls->vtable != NULL) {
        size = cls->vtable->size;
        vtable_size = size;
        data_00581caa = lalloc(vtable_size);
        memclrw(data_00581caa, vtable_size);

        current_class = cls;
        rtti_offset_table = NULL;
        data_00581cc2 = 0;
        data_00581cc6 = 0;

        build_class_layout(cls);

        memclrw(data_00581caa, vtable_size);

        if (copts.RTTI && (cls->flags & 0x2010) == 0)
            rtti_offset_table = CRTTI_BuildRTTIOffsetTable(cls, data_00581caa, rtti_offset_table);

        CError_ASSERT(2314, cls->vtable->object->type->size == cls->vtable->size);

        CInit_DeclareData(cls->vtable->object, data_00581caa, rtti_offset_table, cls->vtable->size);
    }
}

void CClass_ClassDefaultFuncAction(TypeClass *tclass)
{
    return;
}

void CClass_CheckOverrides(TypeClass *cls)
{
    OverrideClass *layout;
    Object *object;
    ClassList *base;
    VClassList *vbase;
    ObjectList *linked;
    int index;
    CScopeObjectIterator iter;

    index = 0;
    for (base = cls->bases; base != NULL; base = base->next) {
        base->offset = index;
        base->voffset = index;
        index++;
    }
    for (vbase = cls->vbases; vbase != NULL; vbase = vbase->next) {
        vbase->offset = index;
        vbase->voffset = index;
        index++;
    }

    current_class = cls;
    layout = create_class_layout(NULL, cls, 0, 0);
    root_class_layout = layout;
    select_layout_member_overrides(layout);

    if (CClass_004ea020(layout, 0) != NULL) {
        cls->flags |= CLASS_ABSTRACT;
    }
    if (copts.warn_hidevirtual != 0) {
        check_hidden_inherited_virtual_functions(layout, layout);
    }
    if (cls->flags & CLASS_SOM_INIT) {
        mark_vbases_has_override(layout, layout, 0);
    }

    linked = NULL;
    CScope_InitObjectIterator(&iter, cls->nspace);
    for (;;) {
        object = CScope_NextObjectIteratorObject(&iter);
        if (object == NULL) {
            break;
        }
        if (object->datatype == DVFUNC) {
            CError_ASSERT(1997, object->type->type == TYPEFUNC);
            if (TYPE_FUNC(object->type)->flags & 0x20000000) {
                linked = prepend_base_method_copies(linked, object, cls);
            }
        }
    }

    while (linked != NULL) {
        CScope_AddObject(cls->nspace, linked->object->name, (ObjBase *)linked->object);
        linked = linked->next;
    }

    for (base = cls->bases; base != NULL; base = base->next) {
        base->offset = 0;
        base->voffset = 0;
    }
    for (vbase = cls->vbases; vbase != NULL; vbase = vbase->next) {
        vbase->offset = 0;
        vbase->voffset = 0;
    }
}

void check_hidden_inherited_virtual_functions(OverrideClass *layout, OverrideClass *base)
{
    OverrideFunc *b;
    OverrideClassBase *d;
    Object *obj;
    CScopeObjectIterator iter;

    if (layout != base) {
        for (b = (OverrideFunc *)base->members; b != NULL; b = b->next) {
            if (b->selectedClass != layout) {
                CScope_InitObjectIterator(&iter, layout->theclass->nspace);
                for (;;) {
                    obj = CScope_NextObjectIteratorObject(&iter);
                    if (obj == NULL)
                        break;
                    if (obj->name != b->object->name || obj->type->type != TYPEFUNC || obj->datatype == TYPEFUNC ||
                        (((TypeFunc *)obj->type)->flags & 0x20))
                        continue;
                    CError_Warning(ERR_HIDES_INHERITED_VIRTUAL_FUNCTION, obj, b->object);
                    break;
                }
            }
        }
    }
    for (d = (OverrideClassBase *)base->children; d != NULL; d = d->next)
        check_hidden_inherited_virtual_functions(layout, d->layout);
}

void mark_vbases_has_override(OverrideClass *root, OverrideClass *node, unsigned char mark)
{
    OverrideFunc *entry;
    OverrideClassBase *link;
    VClassList *item;

    if (mark != 0) {
        for (entry = (OverrideFunc *)node->members; entry != NULL; entry = entry->next) {
            if (entry->selected == NULL)
                continue;
            virtual_base_layout = NULL;
            if (!contains_base_layout(node, entry->selectedClass))
                CError_FATAL(1880);
            if (virtual_base_layout == NULL)
                continue;
            for (item = root->theclass->vbases; item != NULL; item = item->next) {
                if (item->base == virtual_base_layout->theclass)
                    break;
            }
            if (item == NULL)
                CError_FATAL(1887);
            item->has_override = 1;
        }
    }
    for (link = (OverrideClassBase *)node->children; link != NULL; link = link->next) {
        mark_vbases_has_override(root, link->layout, (mark != 0) || (link->is_virtual != 0));
    }
}

Object *create_root_class_layout(TypeClass *type)
{
    OverrideClass *object;

    current_class = type;
    object = create_class_layout(NULL, type, 0, 0);
    root_class_layout = object;
    select_layout_member_overrides(object);
    return CClass_004ea020(object, 0);
}

void build_class_layout(TypeClass *type)
{
    OverrideClass *object;

    current_class = type;
    object = create_class_layout(NULL, type, 0, 0);
    root_class_layout = object;
    select_layout_member_overrides(object);
    CClass_004ea020(object, '\x01');
    build_virtual_function_entries(object);
}

Object *CClass_004ea020(OverrideClass *record, char report)
{
    Object *result = NULL;
    OverrideFunc *nodeA;
    OverrideClassBase *nodeB;

    for (nodeA = (OverrideFunc *)record->members; nodeA != NULL; nodeA = nodeA->next) {
        if (nodeA->conflict != NULL && report != 0) {
            CError_ReportError(ERR_CLASS_MORE_THAN_ONE_FINAL_OVERRIDER, current_class, 0, nodeA->object,
                               nodeA->selected->object, nodeA->conflict->object);
        }
        if (result == NULL) {
            Object *object;
            if (nodeA->selected != NULL)
                object = nodeA->selected->object;
            else
                object = nodeA->object;
            if (TYPE_FUNC(object->type)->flags & FUNC_PURE)
                result = object;
        }
    }
    for (nodeB = (OverrideClassBase *)record->children; nodeB != NULL; nodeB = nodeB->next) {
        if (result != NULL)
            CClass_004ea020(nodeB->layout, report);
        else
            result = CClass_004ea020(nodeB->layout, report);
    }
    return result;
}

void build_virtual_function_entries(OverrideClass *ctx)
{
    struct OverrideFunc *entry;
    OverrideClassBase *child;
    Object *func;
    SInt32 slot;
    SInt32 diff;
    SInt32 delta;
    SInt32 off;
    VirtualFunctionEntry *node;
    NameSpaceObjectList *found;

    if (ctx->done)
        return;
    for (entry = ctx->members; entry != NULL; entry = entry->next) {
        slot = CABI_GetVTableOffset(ctx->theclass) + (ctx->voffset + TYPE_METHOD(entry->object->type)->vtbl_index);
        CError_ASSERT(1707, slot < vtable_size);
        if (data_00581caa[slot] != 0)
            continue;
        if (entry->selected != NULL) {
            func = entry->selected->object;
            if ((TYPE_METHOD(func->type)->flags & 0x20000000) != 0 &&
                CClass_GetOverrideKind(TYPE_FUNC(entry->object->type), TYPE_FUNC(func->type), 0) == 2) {
                CError_ASSERT(1727,
                              TYPE_METHOD(entry->object->type)->functype->type == TYPEPOINTER &&
                                  TYPE_POINTER(TYPE_METHOD(entry->object->type)->functype)->target->type == TYPECLASS);
                found = CScope_FindName(TYPE_METHOD(func->type)->theclass->nspace,
                                        CMangler_GetCovariantFunctionName(
                                            func, TYPE_POINTER(TYPE_METHOD(entry->object->type)->functype)->target));
                CError_ASSERT(1594, found != NULL && found->next == NULL && found->object->otype == OT_OBJECT);
                func = (Object *)found->object;
            }
            diff = entry->selectedClass->offset - ctx->offset;
            if (diff != 0 && (TYPE_METHOD(func->type)->flags & FUNC_PURE) == 0) {
                virtual_base_layout = NULL;
                CError_ASSERT(1739, contains_base_layout(ctx, entry->selectedClass));
                if (virtual_base_layout != NULL && (current_class->flags & CLASS_SOM_INIT) != 0) {
                    delta = ctx->offset - virtual_base_layout->offset;
                    off = CABI_GetCtorOffsetOffset(virtual_base_layout->theclass, NULL) - delta;
                    CError_ASSERT(1746, off > 0);
                    func = get_or_create_thunk_object(func, diff, 0, off);
                } else {
                    func = get_or_create_thunk_object(func, diff, 0, -1);
                }
            }
        } else {
            func = entry->object;
        }
        if ((TYPE_METHOD(func->type)->flags & FUNC_PURE) == 0) {
            node = (VirtualFunctionEntry *)lalloc(0x10);
            node->next = rtti_offset_table;
            node->func = func;
            node->slot = slot;
            node->x = 0;
            rtti_offset_table = node;
            data_00581caa[slot] = 1;
        }
    }
    ctx->done = 1;
    for (child = (OverrideClassBase *)ctx->children; child != NULL; child = child->next)
        build_virtual_function_entries(child->layout);
}

static SInt32 vbase_offset(TypeClass *tclass, TypeClass *baseclass)
{
    VClassList *vbase;

    for (vbase = tclass->vbases; vbase != NULL; vbase = vbase->next) {
        if (vbase->base == baseclass)
            return vbase->offset;
    }
    CError_FATAL(1182);
    return 0;
}

static inline Object *FindFunctionObject(TypeClass *type)
{
    NameSpaceObjectList *entry;
    Object *object;

    entry = CScope_FindName(type->nspace, destructor_name);
    while (entry) {
        if ((object = (Object *)entry->object)->otype == OT_OBJECT && object->type->type == TYPEFUNC)
            return object;
        entry = entry->next;
    }
    return NULL;
}

static Object *CClass_FindFuncObject(NameSpace *nspace, HashNameNode *name)
{
    NameSpaceObjectList *list;

    for (list = CScope_FindName(nspace, name); list != NULL; list = list->next) {
        if (((Object *)list->object)->otype == OT_OBJECT && ((Object *)list->object)->type->type == TYPEFUNC)
            return (Object *)list->object;
    }
    return NULL;
}

void select_layout_member_overrides(OverrideClass *node)
{
    OverrideFunc *entry;
    OverrideClassBase *child;
    OverrideClass *childNode;
    OverrideFunc *childEntry;
    OverrideClassBase *grandchild;
    OverrideFunc *grandchildEntry;
    OverrideClass *grandchildNode;
    OverrideClassBase *greatGrandchild;
    OverrideClass *greatGrandchildNode;
    OverrideFunc *greatGrandchildEntry;
    OverrideClassBase *descendant;
    OverrideClass *root;

    if (root_class_layout != node) {
        for (entry = node->members; entry != NULL; entry = entry->next) {
            root = root_class_layout;
            select_member_override(root, node, entry);
        }
    }
    for (child = node->children; child != NULL; child = child->next) {
        childNode = child->layout;
        if (root_class_layout != childNode) {
            for (childEntry = childNode->members; childEntry != NULL; childEntry = childEntry->next) {
                root = root_class_layout;
                select_member_override(root, childNode, childEntry);
            }
        }
        for (grandchild = childNode->children; grandchild != NULL; grandchild = grandchild->next) {
            grandchildNode = grandchild->layout;
            if (root_class_layout != grandchildNode) {
                for (grandchildEntry = grandchildNode->members; grandchildEntry != NULL;
                     grandchildEntry = grandchildEntry->next) {
                    root = root_class_layout;
                    select_member_override(root, grandchildNode, grandchildEntry);
                }
            }
            for (greatGrandchild = grandchildNode->children; greatGrandchild != NULL;
                 greatGrandchild = greatGrandchild->next) {
                greatGrandchildNode = greatGrandchild->layout;
                if (root_class_layout != greatGrandchildNode) {
                    for (greatGrandchildEntry = greatGrandchildNode->members; greatGrandchildEntry != NULL;
                         greatGrandchildEntry = greatGrandchildEntry->next) {
                        root = root_class_layout;
                        select_member_override(root, greatGrandchildNode, greatGrandchildEntry);
                    }
                }
                for (descendant = greatGrandchildNode->children; descendant != NULL; descendant = descendant->next) {
                    select_layout_member_overrides(descendant->layout);
                }
            }
        }
    }
    return;
}

void CClass_DefineCovariantFuncs(Object *func, CInlineInfo *inlineInfo)
{
    CClassNode *returnClass;
    Statement *statement;
    Statement body;

    for (returnClass = collect_override_return_class_types(NULL, TYPE_METHOD(func->type)->theclass, func, 1);
         returnClass != NULL; returnClass = returnClass->next) {
        HashNameNode *name = CMangler_GetCovariantFunctionName(func, returnClass->type);
        NameSpaceObjectList *found = CScope_FindName(TYPE_METHOD(func->type)->theclass->nspace, name);
        Object *member;
        Type *returnType;
        NameSpace *functionScope;
        CError_ASSERT(1594, found != NULL && found->next == NULL && OBJ_BASE(found->object)->otype == OT_OBJECT);
        member = (Object *)found->object;
        returnType = CDecl_NewPointerType(returnClass->type);
        functionScope = CFunc_FuncGenSetup(&body, member);
        CInline_ReconstructFunction(member, inlineInfo, &body);
        for (statement = &body; statement != NULL; statement = statement->next) {
            if (statement->type == ST_RETURN && statement->expr != NULL) {
                statement->expr = oldassignmentpromotion(statement->expr, returnType, statement->expr->flags & 3, 0);
            }
        }
        CFunc_Gen(&body, member, 0);
        cscope_current = functionScope->parent;
    }
}

ObjectList *prepend_base_method_copies(ObjectList *objects, Object *method, TypeClass *theclass)
{
    Object *object;
    ObjectList *entry;
    TypeMemberFunc *newtype;
    TypeMemberFunc *functype;
    TypePointer *pointertype;
    CClassNode *base;
    HashNameNode *name;
    TypeClass *baseclass;
    base = collect_override_return_class_types(NULL, theclass, method, 1);
    while (base != NULL) {
        name = CMangler_GetCovariantFunctionName(method, base->type);
        object = (Object *)galloc(54);
        memclrw(object, 54);
        object->otype = OT_OBJECT;
        object->datatype = DFUNC;
        object->section = method->section;
        object->nspace = method->nspace;
        object->name = name;
        baseclass = (TypeClass *)base->type;
        functype = (TypeMemberFunc *)method->type;
        CError_ASSERT(1564, !(functype->type != TYPEFUNC || (functype->flags & FUNC_METHOD) == 0 ||
                              functype->functype->type != TYPEPOINTER));
        pointertype = (TypePointer *)galloc(sizeof(TypePointer));
        *pointertype = *(TypePointer *)functype->functype;
        pointertype->target = (Type *)baseclass;
        newtype = copyMemberFunction(functype);
        newtype->flags &= 0xdfffffdf;
        newtype->functype = (Type *)pointertype;
        object->type = (Type *)newtype;
        object->qual = method->qual & ~Q_INLINE;
        object->sclass = method->sclass;
        object->u.func.linkname = name;
        entry = (ObjectList *)lalloc(sizeof(ObjectList));
        entry->object = object;
        entry->next = objects;
        objects = entry;
        base = base->next;
    }
    return objects;
}

CClassNode *collect_override_return_class_types(CClassNode *types, TypeClass *tclass, Object *method, Boolean skipClass)
{
    CScopeObjectIterator it;
    Object *obj;
    CClassNode *node;
    Type *target;
    ClassList *base;

    if (skipClass == 0) {
        CScope_InitObjectIterator(&it, tclass->nspace);
        for (;;) {
            obj = CScope_NextObjectIteratorObject(&it);
            if (obj == NULL)
                break;
            if (obj->name == method->name && obj->datatype == DVFUNC &&
                CClass_GetOverrideKind(TYPE_FUNC(obj->type), TYPE_FUNC(method->type), 0) == OVERRIDE_2) {
                if (!(FT_PTR(obj)->type == TYPEPOINTER && TPTR_TARGET(FT_PTR(obj))->type == TYPECLASS))
                    CError_FATAL(1526);

                target = TPTR_TARGET(FT_PTR(obj));

                for (node = types; node != NULL; node = node->next) {
                    if (node->type == target)
                        break;
                }
                if (node == NULL) {
                    node = (CClassNode *)lalloc(sizeof(CClassNode));
                    node->type = target;
                    node->next = types;
                    types = node;
                }
            }
        }
    }

    for (base = tclass->bases; base != NULL; base = base->next) {
        if (base->base->vtable != NULL)
            types = collect_override_return_class_types(types, base->base, method, 0);
    }
    return types;
}

void select_member_override(OverrideClass *classRecord, OverrideClass *context, struct OverrideFunc *search)
{
    OverrideFunc *function;
    OverrideClassBase *base;
    UInt8 kind;
    Boolean ok;

    ok = contains_base_layout(context, classRecord);
    if (ok) {
        for (function = classRecord->members; function != NULL; function = function->next) {
            if (search->object->name == function->object->name) {
                kind = CClass_GetOverrideKind(TYPE_FUNC(search->object->type), TYPE_FUNC(function->object->type), 0);
                if (kind) {
                    if (search->selectedClass != NULL) {
                        if (search->selectedClass->theclass == classRecord->theclass ||
                            CClass_ClassDominates(search->selectedClass->theclass, classRecord->theclass))
                            return;
                        if (!CClass_ClassDominates(classRecord->theclass, search->selectedClass->theclass)) {
                            search->conflict = function;
                            return;
                        }
                    }
                    if (classRecord == root_class_layout) {
                        TYPE_METHOD(function->object->type)->flags |= 0x20;
                        if (kind == OVERRIDE_2)
                            TYPE_METHOD(function->object->type)->flags |= 0x20000000;
                    }
                    search->selectedClass = classRecord;
                    search->selected = function;
                    search->conflict = NULL;
                    return;
                }
            }
        }
        for (base = (OverrideClassBase *)classRecord->children; base != NULL; base = base->next) {
            select_member_override(base->layout, context, search);
        }
    }
}

/* Identity values used to match a class search entry. */
Boolean contains_base_layout(OverrideClass *identity, OverrideClass *sub)
{
    OverrideClassBase *node;
    for (node = (OverrideClassBase *)sub->children; node != NULL; node = node->next) {
        if ((node->layout->theclass == identity->theclass && node->layout->offset == identity->offset) ||
            contains_base_layout(identity, node->layout)) {
            if (!virtual_base_layout && node->is_virtual)
                virtual_base_layout = node->layout;
            return 1;
        }
    }
    return 0;
}

OverrideClass *create_class_layout(OverrideClass *root, TypeClass *cls, SInt32 offset, SInt32 voffset)
{
    OverrideClass *layout;
    ClassList *base;
    CScopeObjectIterator scope;
    Object *object;
    OverrideFunc *member;
    OverrideClassBase *derived;

    layout = lalloc((sizeof(*layout) + 1) & ~1);
    memclrw(layout, (sizeof(*layout) + 1) & ~1);
    layout->theclass = cls;
    layout->offset = offset;
    layout->voffset = voffset;
    if (root == NULL)
        root = layout;

    CScope_InitObjectIterator(&scope, cls->nspace);
    for (;;) {
        object = CScope_NextObjectIteratorObject(&scope);
        if (object == NULL)
            break;
        if (object->datatype != DVFUNC)
            continue;
        member = lalloc(sizeof(*member));
        memclrw(member, sizeof(*member));
        member->next = layout->members;
        member->object = object;
        layout->members = member;
    }

    for (base = cls->bases; base != NULL; base = base->next) {
        if (base->base->vtable != NULL || base->base->sominfo != NULL) {
            derived = lalloc((sizeof(*derived) + 1) & ~1);
            memclrw(derived, (sizeof(*derived) + 1) & ~1);
            if (base->is_virtual) {
                SInt32 baseOffset = CL_FindOffset(base->base);
                derived->layout = find_class_layout_by_class_and_offset(root, base->base, baseOffset);
                if (derived->layout == NULL) {
                    SInt32 baseVOffset = CL_FindVOffset(base->base);
                    derived->layout = create_class_layout(root, base->base, baseOffset, baseVOffset);
                }
                derived->is_virtual = 1;
            } else {
                derived->layout = create_class_layout(root, base->base, offset + base->offset, voffset + base->voffset);
                derived->is_virtual = 0;
            }
            derived->next = layout->children;
            layout->children = derived;
        }
    }
    return layout;
}

OverrideClass *find_class_layout_by_class_and_offset(OverrideClass *layout, TypeClass *cls, SInt32 offset)
{
    OverrideClassBase *list;

    if (layout->theclass == cls && layout->offset == offset)
        return layout;
    for (list = layout->children; list; list = list->next) {
        if (layout = find_class_layout_by_class_and_offset(list->layout, cls, offset))
            return layout;
    }
    return NULL;
}

unsigned char CClass_OverridesBaseMember(TypeClass *theclass, HashNameNode *name, Object *target)
{
    unsigned char found = 0;
    NameSpaceObjectList *objects;
    ClassList *base = theclass->bases;

    if (base != NULL) {
        do {
            if (base->base->vtable != NULL) {
                objects = CScope_FindName(base->base->nspace, name);
                while (objects != NULL) {
                    if (objects->object->otype == OT_OBJECT && OBJECT(objects->object)->datatype == DVFUNC &&
                        CClass_GetOverrideKind(TYPE_FUNC(OBJECT(objects->object)->type), TYPE_FUNC(target->type), 1) !=
                            0) {
                        found = 1;
                    }
                    objects = objects->next;
                }
                if (CClass_OverridesBaseMember(base->base, name, target) != 0) {
                    found = 1;
                }
            }
            base = base->next;
        } while (base != NULL);
    }
    return found;
}

unsigned int CClass_VirtualBaseVTableOffset(TypeClass *type, TypeClass *base)
{
    VClassList *classes = type->vbases;

    while (classes) {
        if (classes->base == base)
            return classes->voffset;
        classes = classes->next;
    }

    CError_FATAL(1199);
    return 0;
}

SInt32 CClass_VirtualBaseOffset(TypeClass *cls, TypeClass *base)
{
    VClassList *entry = (VClassList *)cls->vbases;
    while (entry != NULL) {
        if (entry->base == base)
            return entry->offset;
        entry = entry->next;
    }
    CError_FATAL(1182);
    return 0;
}

/* Recursive base-class query. */
Boolean CClass_ClassDominates(TypeClass *cls, TypeClass *base)
{
    ClassList *b;

    for (b = cls->bases; b; b = b->next)
        if (b->base == base || CClass_ClassDominates(b->base, base))
            return 1;
    return 0;
}

int CClass_GetPathOffset(BClassList *cl)
{
    int offset = 0;
    ClassList *found;
    BClassList *next;

    while ((next = cl->next) != NULL) {
        if (cl->type != next->type) {
            found = TYPE_CLASS(cl->type)->bases;
            while (found != NULL) {
                if ((Type *)found->base == next->type) {
                    break;
                }
                found = found->next;
            }
            if (found == NULL) {
                CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
                return offset;
            }
        }
        if (found->is_virtual) {
            return -1;
        }
        offset += found->offset;
        cl = next;
    }
    return offset;
}

ENode *CClass_DirectBasePointerCast(ENode *expr, TypeClass *theclass, TypeClass *base)
{
    ENode *CClass_AdjustBasePointer(ENode * expr, UInt16 count, Boolean flag);
    ClassList *cl;

    base_path_depth = 0;
    base_path_level = 0;
    cl = theclass->bases;
    while (cl) {
        if (cl->base == base) {
            base_path[0] = cl;
            base_path_level = 1;
            break;
        }
        cl = cl->next;
    }
    if (cl == NULL) {
        CError_ASSERT(1093, find_virtual_base_path(theclass, base));
    }
    return CClass_AdjustBasePointer(expr, base_path_level, 0);
}

ENode *CClass_ClassPointerCast(ENode *expr, TypeClass *sourceClass, TypeClass *targetClass, Boolean convertIndirect,
                               Boolean errorflag)
{
    Boolean reverseConversion = 0;

    if ((sourceClass->flags & CLASS_SINGLE_OBJECT) == 0) {
        if (sourceClass != targetClass) {
            if ((sourceClass->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
                CDecl_CompleteType((Type *)sourceClass);
            if ((targetClass->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
                CDecl_CompleteType((Type *)targetClass);
            base_path_level = 0;
            base_path_class = sourceClass;
            base_path_offset = -1;
            base_path_status = 0;
            if (!find_base_path(sourceClass, targetClass, 0, 0, errorflag)) {
                if ((targetClass->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
                    CDecl_CompleteType((Type *)targetClass);
                if ((sourceClass->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
                    CDecl_CompleteType((Type *)sourceClass);
                base_path_level = 0;
                base_path_class = targetClass;
                base_path_offset = -1;
                base_path_status = 0;
                if (find_base_path(targetClass, sourceClass, 0, 0, errorflag))
                    reverseConversion = 1;
                else
                    return expr;
            }
        } else {
            base_path_level = base_path_depth;
        }
        if (!targetClass->sominfo)
            expr = CClass_AdjustBasePointer(expr, base_path_level, reverseConversion);
    }
    if (convertIndirect && expr->type == EINDIRECT && expr->rtype->type == TYPEPOINTER)
        expr = makemonadicnode(expr, ETYPCON);
    return expr;
}

ENode *CClass_AdjustBasePointer(ENode *expr, SInt16 count, Boolean reverse)
{
    SInt16 i = 0;
    if (0 < count) {
        do {
            ClassList *cl = object_groups[i + 0x16];
            if (cl->is_virtual != 0) {
                if (reverse != 0) {
                    CError_ReportError(ERR_ILLEGAL_TYPE_CAST);
                    break;
                }
                if (cl->base->sominfo == NULL) {
                    SInt32 offset = cl->offset;
                    if (offset != 0 && canadd(expr, offset) == 0) {
                        expr = makediadicnode(expr, intconstnode((Type *)&stunsignedlong, offset), EADD);
                        optimizecomm(expr);
                    }
                    expr->rtype = (Type *)CDecl_NewPointerType(CDecl_NewPointerType((Type *)cl->base));
                    expr = makemonadicnode(expr, EINDIRECT);
                    expr->rtype = expr->data.monadic->rtype;
                }
            } else {
                SInt32 offset = cl->offset;
                if (offset != 0) {
                    if (reverse != 0)
                        offset = -offset;
                    if (canadd(expr, offset) == 0) {
                        expr = makediadicnode(expr, intconstnode((Type *)&stunsignedlong, offset), EADD);
                        optimizecomm(expr);
                    }
                }
            }
            i++;
        } while (i < count);
    }
    return expr;
}

TypeClass *CClass_GetQualifiedClass(void)
{
    DeclInfo info;

    memclrw(&info, sizeof info);
    CParser_GetDeclSpecs(&info, 0);
    if (info.thetype->type == TYPECLASS)
        return (TypeClass *)info.thetype;
    return NULL;
}

BClassList *CClass_GetBasePath(TypeClass *type, TypeClass *target, SInt16 *flags, Boolean *status)
{
    BClassList *result;
    BClassList *entry;
    if ((type->flags & (0x800 | CLASS_COMPLETED)) == 0x800)
        CDecl_CompleteType((Type *)type);
    if ((target->flags & (0x800 | CLASS_COMPLETED)) == 0x800)
        CDecl_CompleteType((Type *)target);
    base_path_level = 0;
    base_path_class = type;
    base_path_offset = -1;
    base_path_status = 0;
    result = find_target_base_path(type, target, 0, 1);
    if (result != NULL) {
        *flags = base_path_level;
        *status = base_path_status;
        entry = (BClassList *)lalloc(8);
        entry->next = result;
        entry->type = (Type *)type;
        return entry;
    } else {
        *status = 0;
        return NULL;
    }
}

BClassList *find_target_base_path(TypeClass *cls, TypeClass *target, SInt32 offset, SInt16 level)
{
    BClassList *result;
    BClassList *node;
    SInt32 off;
    BClassList *path;
    ClassList *base;
    Boolean replace;

    for (base = cls->bases, result = NULL; base != NULL; base = base->next) {
        if (base->is_virtual)
            off = CClass_GetVBaseOffset(base->base);
        else
            off = offset + base->offset;

        if (base->base == target) {
            if (base_path_level != 0 && off != base_path_offset) {
                base_path_status = 1;
                return NULL;
            }
            base_path_offset = off;
            base_path_level = level;
            result = lalloc(8);
            result->next = NULL;
            result->type = (Type *)target;
        } else {
            path = find_target_base_path(base->base, target, off, 1 + level);
            if (path == NULL)
                continue;

            node = lalloc(8);
            node->next = path;
            node->type = (Type *)base->base;

            if (result == NULL) {
                result = node;
            } else {
                replace = CClass_ShouldReplacePath(result, node);
                if (replace)
                    result = node;
            }
        }
    }
    return result;
}

unsigned char CClass_IsMoreAccessiblePath(BClassList *path, BClassList *otherPath)
{
    switch (get_path_access(otherPath)) {
        case 0:
            return 0;
        case 3:
            return 1;
        default:
            CError_FATAL(800);
        case 2:
            switch (get_path_access(path)) {
                case 0:
                    return 1;
                case 1:
                case 2:
                case 3:
                    return 0;
                default:
                    CError_FATAL(809);
            }
        case 1:
            switch (get_path_access(path)) {
                case 0:
                case 2:
                    return 1;
                case 1:
                case 3:
                    return 0;
                default:
                    CError_FATAL(819);
            }
            return 0;
    }
}

UInt8 get_path_access(BClassList *path)
{
    TypeClass *classType;
    ClassList *base;
    UInt8 access;

    if (path == NULL) {
        CError_FATAL(752);
    }
    access = 0;
    for (;;) {
        classType = (TypeClass *)path->type;
        path = path->next;
        if (path == NULL) {
            break;
        }
        base = classType->bases;
        while (base != (ClassList *)0) {
            if (base->base == (TypeClass *)path->type)
                break;
            base = base->next;
        }
        if (base == (ClassList *)0) {
            CError_FATAL(761);
        }
        switch (base->access) {
            case ACCESSPUBLIC:
                break;
            case ACCESSPROTECTED:
                if (access == 0)
                    access = 2;
                break;
            case ACCESSPRIVATE:
                if ((access == 0) || (access == 2))
                    access = 1;
                else
                    return 3;
                break;
            case ACCESSNONE:
                return 3;
            default:
                CError_FATAL(782);
        }
    }
    return access;
}

SInt16 CClass_GetBasePathLevel(void)
{
    return base_path_level;
}

UInt8 CClass_FindBasePath(TypeClass *sourceClass, TypeClass *targetClass, char option1, char option2)
{
    UInt8 result;

    if ((sourceClass->flags & (0x800 | CLASS_COMPLETED)) == 0x800) {
        CDecl_CompleteType((Type *)sourceClass);
    }
    if ((targetClass->flags & (0x800 | CLASS_COMPLETED)) == 0x800) {
        CDecl_CompleteType((Type *)targetClass);
    }
    base_path_level = 0;
    base_path_class = sourceClass;
    base_path_offset = -1;
    base_path_status = 0;
    result = find_base_path(sourceClass, targetClass, 0, option1, option2);
    return result;
}

void fn_004eb810(void)
{
    base_path_depth = 0;
    base_path_level = 0;
    return;
}

Boolean find_virtual_base_path(register TypeClass *cls, register TypeClass *target)
{
    register ClassList *cl;

    for (cl = cls->bases; cl != NULL; cl = cl->next) {
        if (cl->base == target && cl->is_virtual && base_path_level == 0) {
            base_path[base_path_depth] = cl;
            base_path_level = 1 + base_path_depth;
            return 1;
        }
    }
    if (base_path_depth < 0x3f) {
        base_path_depth++;
        for (cl = cls->bases; cl != NULL; cl = cl->next) {
            if (base_path_level == 0)
                base_path[base_path_depth - 1] = cl;
            if (find_virtual_base_path(cl->base, target))
                return 1;
        }
        base_path_depth--;
    }
    return 0;
}

Boolean find_base_path(TypeClass *cls, TypeClass *base, SInt32 offset, Boolean check_access, Boolean report_ambiguity)
{
    ClassList *base_entry = cls->bases;
    Boolean found = 0;

    for (; base_entry != NULL; base_entry = base_entry->next) {
        SInt32 base_offset;

        if (base_entry->is_virtual)
            base_offset = vbase_offset(base_path_class, base_entry->base);
        else
            base_offset = offset + base_entry->offset;

        if (base_entry->base == base) {
            if (base_path_level == 0) {
                base_path[base_path_depth] = base_entry;
                base_path_level = base_path_depth + 1;
                base_path_offset = base_offset;
            } else if (base_offset != base_path_offset) {
                if (report_ambiguity)
                    CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                base_path_status = 1;
            }
            found = 1;
        } else if (base_path_depth < 0x3f) {
            base_path_depth++;
            if (base_path_level == 0)
                base_path[base_path_depth - 1] = base_entry;
            if (find_base_path(base_entry->base, base, base_offset, check_access, report_ambiguity)) {
                if (check_access && base_entry->access)
                    CError_ReportError(ERR_ILLEGAL_ACCESS_PROTECTED_PRIVATE_MEMBER);
                found = 1;
            }
            base_path_depth--;
        }
    }
    return found;
}

BClassList *CClass_AppendPath(BClassList *list, BClassList *tail)
{
    BClassList *last;

    if ((last = list) == NULL) {
        return tail;
    }
    while (last->next != NULL) {
        last = last->next;
    }
    while ((tail != NULL) && (tail->type == last->type)) {
        tail = tail->next;
    }
    last->next = tail;
    return list;
}

/* lalloc */
/* galloc */

BClassList *CClass_GetPathCopy(BClassList *list, Boolean global)
{
    BClassList *first;
    BClassList *current;

    if (!list)
        return NULL;

    first = current = global ? galloc(sizeof(BClassList)) : lalloc(sizeof(BClassList));
    current->next = list->next;
    current->type = list->type;
    while ((list = list->next)) {
        current->next = global ? galloc(sizeof(BClassList)) : lalloc(sizeof(BClassList));
        current = current->next;
        current->next = list->next;
        current->type = list->type;
    }
    return first;
}

void fn_004ebae0(TypeClass *type)
{
    Object *found;

    found = FindFunctionObject(type);
    if (found || CClass_CopyConstructor(type) || type->vtable || type->vbases) {
        type->flags |= 0x4000;
        return;
    }
    {
        ClassList *base = type->bases;
        while (base) {
            if (base->base->flags & 0x4000) {
                type->flags |= 0x4000;
                return;
            }
            base = base->next;
        }
    }
    {
        ObjMemberVar *member = type->ivars;
        while (member) {
            Type *memberType = member->type;
            while (memberType->type == TYPEARRAY)
                memberType = ((TypePointer *)memberType)->target;
            if (memberType->type == TYPECLASS && (((TypeClass *)memberType)->flags & 0x4000)) {
                type->flags |= 0x4000;
                return;
            }
            member = member->next;
        }
    }
}

Boolean CClass_ReferenceArgument(TypeClass *cls)
{
    Object *obj;
    SInt32 result;

    if ((cls->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
        CDecl_CompleteType((Type *)cls);

    if (copts.simple_class_byval != 0)
        return (cls->flags & 0x4000) != 0;

    obj = CClass_FindFuncObject(cls->nspace, destructor_name);

    result = 1;
    if (obj == NULL && CClass_CopyConstructor(cls) == NULL)
        result = 0;

    return result;
}

Boolean CClass_HasTypeFuncFlag16384(Object *object)
{
    unsigned int result = 0;
    unsigned int isFunction = 0;

    if (object != NULL && object->type->type == TYPEFUNC)
        isFunction = 1;
    if (isFunction) {
        TypeFunc *function = TYPE_FUNC(object->type);
        unsigned int flags = function->flags;
        if (flags & 16384U)
            result = 1;
    }
    return result;
}

Boolean CClass_IsDestructor(Object *obj)
{
    return obj != NULL && obj->type->type == TYPEFUNC && (TYPE_FUNC(obj->type)->flags & FUNC_IS_DTOR) != 0;
}

/* Finds the member object of the class whose type is a
 * function and whose name is the global DAT_005870f4. */
Object *CClass_Destructor(TypeClass *cls)
{
    NameSpaceObjectList *entry;

    entry = CScope_FindName(cls->nspace, destructor_name);
    while (entry != NULL) {
        if (OBJECT(entry->object)->otype == OT_OBJECT && TYPE(OBJECT(entry->object)->type)->type == TYPEFUNC)
            return OBJECT(entry->object);
        entry = entry->next;
    }
    return NULL;
}

NameSpaceObjectList *CClass_Constructor(TypeClass *type)
{
    NameSpaceObjectList *objects;

    objects = CScope_FindName(type->nspace, constructor_name);
    if (objects != NULL && objects->object->otype == OT_OBJECT && OBJECT(objects->object)->type->type == TYPEFUNC) {
        return objects;
    }
    return NULL;
}

NameSpaceObjectList *CClass_MemberObject(TypeClass *type, HashNameNode *name)
{
    NameSpaceObjectList *objects;

    objects = CScope_FindName(type->nspace, name);
    if ((objects != NULL) && (objects->object->otype == OT_OBJECT)) {
        return objects;
    }
    return NULL;
}

Object *CClass_CopyConstructor(TypeClass *cls)
{
    NameSpaceObjectList *list;

    if (cls->sominfo != NULL)
        return NULL;
    list = CScope_FindName(cls->nspace, constructor_name);
    while (list != NULL) {
        Object *obj;
        if ((obj = (Object *)list->object)->otype == OT_OBJECT && IS_TYPE_FUNC(obj->type)) {
            FuncArg *arg;
            CError_ASSERT(349, (arg = TYPE_FUNC(obj->type)->args) != NULL);
            arg = arg->next;
            if (cls->flags & CLASS_HAS_VBASES) {
                CError_ASSERT(353, arg != NULL);
                arg = arg->next;
            }
            if (arg != NULL && arg != &elipsis && (arg->next == NULL || arg->next->dexpr != NULL) &&
                arg->type->type == TYPEPOINTER && (TYPE_POINTER(arg->type)->qual & Q_REFERENCE) &&
                TPTR_TARGET(arg->type) == (Type *)cls)
                return obj;
        }
        list = list->next;
    }
    return NULL;
}

Object *CClass_AssignmentOperator(TypeClass *theclass)
{
    NameSpaceObjectList *list;
    FuncArg *args;

    list = CScope_FindName(theclass->nspace, assignment_operator_name);
    while (list != NULL) {
        if (list->object->otype == OT_OBJECT && ((Object *)list->object)->type->type == TYPEFUNC &&
            (args = TYPE_FUNC(((Object *)list->object)->type)->args) != NULL && args->next != NULL &&
            args->next->next == NULL) {
            if (args->next->type->type == TYPECLASS && args->next->type == (Type *)theclass)
                return (Object *)list->object;
            if (args->next->type->type == TYPEPOINTER && (TYPE_POINTER(args->next->type)->qual & Q_REFERENCE) &&
                TYPE_POINTER(args->next->type)->target == (Type *)theclass)
                return (Object *)list->object;
        }
        list = list->next;
    }
    return NULL;
}

Object *CClass_DefaultConstructor(TypeClass *cls)
{
    register Object *obj;
    NameSpaceObjectList *member;

    member = CScope_FindName(cls->nspace, constructor_name);
    while (member != NULL) {
        if ((obj = (Object *)member->object)->otype == OT_OBJECT && obj->type->type == TYPEFUNC) {
            if ((cls->flags & CLASS_HAS_VBASES) != 0 && cls->sominfo == NULL) {
                if (TYPE_FUNC(obj->type)->args->next != NULL && TYPE_FUNC(obj->type)->args->next->next == NULL)
                    return obj;
            } else {
                if (TYPE_FUNC(obj->type)->args->next == NULL)
                    return obj;
            }
        }
        member = member->next;
    }
    return NULL;
}

Boolean CClass_IsEmpty(register TypeClass *type)
{
    register ClassList *base;

    if (type->ivars != NULL)
        return 0;
    for (base = type->bases; base != NULL; base = base->next) {
        if (!CClass_IsEmpty(base->base))
            return 0;
    }
    return 1;
}

static Boolean has_qual(UInt32 a, UInt32 b)
{
    return ((b & 1) && (a & 1) == 0) || ((b & 2) && (a & 2) == 0);
}

static Boolean simple_base(TypeClass *a, TypeClass *b)
{
    for (;;) {
        if (a == b)
            return 1;
        if (!a->bases || a->bases->is_virtual)
            return 0;
        a = a->bases->base;
    }
}

static inline CovarianceKind covariance(Type *a, UInt32 aq, Type *b, UInt32 bq, Boolean errorflag)
{
    TypeClass *ca, *cb;
    if (a->type == TYPEPOINTER && b->type == TYPEPOINTER && TYPE_POINTER(a)->qual == TYPE_POINTER(b)->qual &&
        !has_qual(aq, bq) && TPTR_TARGET(a)->type == TYPECLASS && TPTR_TARGET(b)->type == TYPECLASS) {
        ca = TYPE_CLASS(TPTR_TARGET(a));
        cb = TYPE_CLASS(TPTR_TARGET(b));
        if (ca != cb) {
            if ((cb->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
                CDecl_CompleteType((Type *)cb);
            if ((ca->flags & (CLASS_COMPLETED | CLASS_IS_TEMPL_INST)) == 0x800)
                CDecl_CompleteType((Type *)ca);
            base_path_level = 0;
            base_path_class = cb;
            base_path_offset = -1;
            base_path_status = 0;
            if (!find_base_path(cb, ca, 0, errorflag, errorflag))
                return OV_NONE;
        }
        if (!simple_base(cb, ca))
            return OV_COVARIANT;
        return OV_NORMAL;
    }
    return OV_NONE;
}

UInt8 CClass_GetOverrideKind(TypeFunc *a, TypeFunc *b, Boolean errorflag)
{
    if (!a->args || !b->args)
        return OV_NONE;
    if ((a->flags & 0x17000001) != (b->flags & 0x17000001) || !is_arglistsame(a->args->next, b->args->next) ||
        a->args->qual != b->args->qual)
        return OV_NONE;
    if (!iscpp_typeequal(a->functype, b->functype)) {
        CovarianceKind kind = covariance(a->functype, a->qual, b->functype, b->qual, errorflag);
        switch (kind) {
            case OV_NONE:
                if (errorflag)
                    CError_ReportError(ERR_DERIVED_FUNCTION_DIFFERS_FROM_VIRTUAL_BASE);
                return OV_NONE;
            case OV_NORMAL:
                return OV_NORMAL;
            case OV_COVARIANT:
                return OV_COVARIANT;
            default:
                CError_FATAL(239);
        }
    }
    return OV_NORMAL;
}

Object *get_or_create_thunk_object(Object *source, SInt32 firstArgument, SInt32 secondArgument, SInt32 thirdArgument)
{
    PendingThunk *entry;
    Object *object;
    PendingThunk *newEntry;

    CInline_0050f240(source);
    entry = pending_thunks;
    while (entry) {
        if (source == entry->functionObject && firstArgument == entry->b && secondArgument == entry->c &&
            thirdArgument == entry->d)
            return entry->thunkObject;
        entry = entry->next;
    }

    object = CParser_NewCompilerDefFunctionObject();
    object->name = CMangler_ThunkName(source, firstArgument, secondArgument, thirdArgument);
    object->type = (Type *)&data_0055d5e8;
    object->sclass = TK_EXTERN;
    object->qual = Q_IMPLICIT_WEAK;
    object->u.func.linkname = object->name;

    newEntry = galloc(sizeof(*newEntry));
    newEntry->thunkObject = object;
    newEntry->functionObject = source;
    newEntry->b = firstArgument;
    newEntry->c = secondArgument;
    newEntry->d = thirdArgument;
    newEntry->next = pending_thunks;
    pending_thunks = newEntry;
    return object;
}

void CClass_GenThunks(void)
{
    PendingThunk *p;

    for (p = pending_thunks; p != NULL; p = p->next) {
        p->thunkObject->flags |= OBJECT_DEFINED;
        CodeGen_GenThunk(p->thunkObject, p->functionObject, p->b, p->c, p->d);
    }
}

void CClass_Init(void)
{
    pending_thunks = NULL;
    return;
}
