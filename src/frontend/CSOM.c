#define CERROR_FILE "CSOM.c"
#include "compiler/common.h"
#include "compiler/CSOM.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "driver/COSToolsCLT.h"
#include <stdio.h>

#include <string.h>

struct CSOMRefNode *somReferences;

static FuncArg lbl_005646a0 = {NULL, NULL, NULL, TYPE(&void_ptr), 0, 0, 0};
static TypeFunc data_005646b8 = {TYPEFUNC, 0, &lbl_005646a0, NULL, TYPE(&void_ptr), 0, 0};

static struct HashNameNode *data_00581c48;
static struct HashNameNode *space_name;
static struct HashNameNode *spaces_name;
static struct HashNameNode *csom_blank_name;

static inline Object *FlushData(void)
{
    Object *data;
    COS_LockHandle(data_00583548.data);
    data = CInit_DeclareString(*data_00583548.data, data_00583548.size, 0, 0);
    COS_UnlockHandle(data_00583548.data);
    return data;
}

void CSOM_Init(char flag)
{
    if (flag == '\0') {
        somReferences = NULL;
    }
    data_00581c48 = GetHashNameNode("somInit");
    space_name = GetHashNameNode("somUninit");
    spaces_name = GetHashNameNode("Environment");
    csom_blank_name = GetHashNameNode("__somself");
}

void CSOM_GenerateRefNodeCode(void)
{
    struct CSOMRefNode *entry;

    if (cprep_cu[0xe0] != 1) {
        for (entry = somReferences; entry != NULL; entry = entry->next) {
            switch (entry->kind) {
                case 0:
                    CodeGen_EmitLoadAndBranchFunction(entry->object, data_00588260,
                                                      entry->theclass->sominfo->classDataObject, entry->id);
                    break;
                case 1:
                    CodeGen_EmitLoadAndBranchFunction(entry->object, som_ref_node_rtfunc,
                                                      entry->theclass->sominfo->classDataObject, entry->id);
                    break;
                case 2:
                    CodeGen_EmitLoadAndBranchFunction(entry->object, som_ref_node_runtime_object,
                                                      entry->theclass->sominfo->classDataObject, entry->id);
                    break;
                default:
                    CError_FATAL(132);
                    break;
            }
        }
    }
}

static HashNameNode *CSOM_NameTranslate(HashNameNode *name)
{
    if (name == constructor_name)
        return data_00581c48;
    if (name == destructor_name)
        return space_name;
    return name;
}

static Type *CSOM_FindClassType(HashNameNode *name)
{
    Type *type;

    if (!(type = CScope_GetTagType(cscope_current, name))) {
        fn_0043f3e0(281, name->name);
        return &stvoid;
    }
    return type;
}

static SInt32 CSOM_GetTokenTableIndex(Object *method)
{
    UInt16 index;

    CError_ASSERT(173, method->type->type == TYPEFUNC && (TYPE_METHOD(method->type)->flags & FUNC_METHOD));
    index = TYPE_METHOD(method->type)->vtbl_index;
    return index * 4 + 0x18;
}

UInt8 encode_som_type(UInt8 *p, Type *ty, Boolean flag)
{
    int result;
    if (ty->size > 4)
        p[1] |= 8;

    switch (*(SInt8 *)ty) {
        case TYPEVOID:
            if (flag)
                return (result = 7);
            break;
        case TYPEINT:
        case TYPEENUM:
            if (is_unsigned(ty)) {
                switch (ty->size) {
                    case 1:
                        p[1] |= 1;
                        return (result = 0);
                    case 2:
                        p[1] |= 1;
                        return (result = 2);
                    case 4:
                        return (result = 4);
                    case 8:
                        return (result = 6);
                }
            } else {
                switch (ty->size) {
                    case 1:
                        p[1] |= 1;
                        return (result = 1);
                    case 2:
                        p[1] |= 1;
                        return (result = 3);
                    case 4:
                        return (result = 5);
                    case 8:
                        return (result = 6);
                }
            }
            break;
        case TYPEFLOAT:
            p[1] |= 4;
            switch (ty->size) {
                case 4:
                    p[1] |= 2;
                    return (result = 8);
                case 8:
                    return (result = 9);
                case 12:
                case 16:
                    return (result = 10);
            }
            break;
        case TYPEPOINTER:
            return (result = 12);
        case TYPESTRUCT:
        case TYPECLASS:
            if (flag) {
                if (ty->size <= 2) {
                    p[1] |= 1;
                    return (result = 11);
                }
                if (ty->size <= 4)
                    return (result = 14);
                return (result = 15);
            }
            break;
    }
    CError_ReportError(ERR_ILLEGAL_SOM_FUNCTION_PARAMETERS_RETURN_TYPE);
    return (result = 5);
}

void encode_member_function_types(TypeMemberFunc *t, Boolean flag)
{
    FuncArg *arg;
    UInt8 buf[3];
    UInt8 acc;
    Boolean odd;

    buf[2] = encode_som_type(buf, t->functype, 1);
    buf[1] = 0;
    buf[0] = 0;

    for (arg = t->args; arg != NULL; arg = arg->next) {
        if (arg == &elipsis || arg == &oldstyle || ++buf[0] == 0) {
            CError_ReportError((SInt32)0x111);
            break;
        }
        encode_som_type(buf, arg->type, 0);
    }

    if (flag) {
        if ((arg = t->args) != NULL) {
            if (t->is_static == 0)
                arg = arg->next;
            if (arg != NULL && CMach_GetFunctionResultClass((TypeFunc *)t))
                arg = arg->next;
        }

        AppendGListByte(&data_00583548, buf[0]);
        AppendGListByte(&data_00583548, (buf[1] << 4) | buf[2]);
        if (buf[1] != 0) {
            odd = 0;
            acc = 0;
            while (arg != NULL) {
                acc = (acc << 4) | encode_som_type(buf, arg->type, 0);
                if (odd) {
                    AppendGListByte(&data_00583548, acc);
                    odd = 0;
                    acc = 0;
                } else {
                    odd = 1;
                }
                arg = arg->next;
            }
            if (odd)
                AppendGListByte(&data_00583548, acc << 4);
        }
    }
}

void CSOM_EncodeMemberFunctionTypes(TypeMemberFunc *function)
{
    encode_member_function_types(function, 0);
}

static Object *CSOM_MakeObject(char *name, char *suffix, SInt32 size)
{
    Object *object;

    object = CParser_NewCompilerDefDataObject();
    object->name = CParser_NameConcat(name, suffix);
    object->type = CDecl_NewStructType(size, 4);
    CScope_AddObject(object->nspace, object->name, (ObjBase *)object);
    return object;
}

void CSOM_InitSOMInfo(TypeClass *type)
{
    ClassList *base;
    SOMInfo *info;

    for (base = type->bases; base != NULL; base = base->next) {
        if (base->base->sominfo == NULL) {
            CError_ReportError(ERR_SOM_CLASSES_ONLY_INHERIT_FROM_OTHER);
            break;
        }
    }
    if (type->sominfo == NULL) {
        info = (SOMInfo *)galloc(22);
        memclrw(info, 22);
        type->sominfo = info;
        info->classDataObject = CSOM_MakeObject(type->classname->name, "ClassData", 0x1c);
        info->classDataObject->flags |= OBJECT_EXPORT;
    }
}

static Boolean CSOM_IsReleaseOrderMethod(Object *object)
{
    if (object->type->type == TYPEFUNC && (TYPE_FUNC(object->type)->flags & 0x20) == 0 &&
        !TYPE_METHOD(object->type)->is_static && (!(object->qual & Q_INLINE) || object->datatype == DVFUNC))
        return 1;
    return 0;
}

Object **build_vtbl_index_table(TypeClass *theclass, SInt32 *count)
{
    int maxIndex = 0;
    Object *object;
    Object *methodObject;
    Boolean eligible;
    Boolean eligibleAgain;
    Object **table;
    int tableSize;
    CScopeObjectIterator scope;
    TypeMemberFunc *method;

    CScope_InitObjectIterator(&scope, theclass->nspace);
    for (;;) {
        object = CScope_NextObjectIteratorObject(&scope);
        if (!object)
            break;
        if (object->type->type != TYPEFUNC || (((TypeMemberFunc *)object->type)->flags & FUNC_METHOD) == 0)
            continue;
        eligible = CSOM_IsReleaseOrderMethod(object);
        if (eligible) {
            if (((TypeMemberFunc *)object->type)->vtbl_index <= maxIndex)
                continue;
            maxIndex = ((TypeMemberFunc *)object->type)->vtbl_index;
            continue;
        }
        ((TypeMemberFunc *)object->type)->vtbl_index = 0;
    }
    tableSize = maxIndex + 1;
    *count = tableSize;
    table = (Object **)lalloc(tableSize * sizeof(*table));
    memclrw(table, tableSize * sizeof(*table));
    CScope_InitObjectIterator(&scope, theclass->nspace);
    for (;;) {
        methodObject = CScope_NextObjectIteratorObject(&scope);
        if (!methodObject)
            break;
        eligibleAgain = CSOM_IsReleaseOrderMethod(methodObject);
        if (!eligibleAgain)
            continue;
        method = (TypeMemberFunc *)methodObject->type;
        table[method->vtbl_index] = methodObject;
    }
    return table;
}

void CSOM_CompleteClass(TypeClass *tclass)
{
    NameSpaceObjectList *next;
    int methodIndex;
    Object *method;
    SOMInfoEntry *entry;
    NameSpaceObjectList *objects;
    Type *requiredType;
    SOMInfoEntry *baseEntry;
    long index;
    Object *otherMethod;
    Object *object;
    HashNameNode *methodName;
    Object *firstMethod;
    Object *checkedMethod;
    FuncArg *args;
    VClassList *base;
    Object **numberedEntries;
    CScopeObjectIterator iterator;
    SInt32 count;

    if (tclass->sominfo->methodNameList) {
        CScope_InitObjectIterator(&iterator, tclass->nspace);
        for (;;) {
            method = CScope_NextObjectIteratorObject(&iterator);
            if (method == NULL)
                break;
            if (!CSOM_IsReleaseOrderMethod(method))
                continue;
            methodName = CSOM_NameTranslate(method->name);
            for (entry = tclass->sominfo->methodNameList, methodIndex = 0; entry != NULL;
                 entry = entry->next, methodIndex++) {
                if (entry->name == methodName) {
                    entry->kind = 1;
                    TYPE_METHOD(method->type)->vtbl_index = methodIndex;
                    break;
                }
            }
            if (entry == NULL)
                CError_ReportError(ERR_INTRODUCED_METHOD_NOT_SPECIFIED_RELEASE_ORDER, method);
        }
        for (entry = tclass->sominfo->methodNameList; entry != NULL; entry = entry->next) {
            if (entry->kind == 0) {
                for (base = tclass->vbases; base != NULL; base = base->next) {
                    for (baseEntry = base->base->sominfo->methodNameList; baseEntry != NULL;
                         baseEntry = baseEntry->next) {
                        if (entry->name == baseEntry->name && baseEntry->kind == 1) {
                            entry->kind = 2;
                            break;
                        }
                    }
                }
            }
        }
    } else {
        numberedEntries = build_vtbl_index_table(tclass, &count);
        for (index = methodIndex = 0; index < count; index++) {
            if ((method = numberedEntries[index]) != NULL) {
                if (methodIndex == 0 && copts.extended_errorcheck != 0)
                    CError_Warning(ERR_SOM_CLASS_NO_RELEASE_ORDER_LIST);
                TYPE_METHOD(method->type)->vtbl_index = methodIndex++;
            }
        }
    }
    CScope_InitObjectIterator(&iterator, tclass->nspace);
    for (;;) {
        objects = CScope_NextObjectIteratorObjectList(&iterator);
        if (objects == NULL)
            break;
        if (objects->object->otype == OT_OBJECT && (next = objects->next) != NULL && next->object->otype == OT_OBJECT) {
            for (; objects != NULL; objects = objects->next) {
                if ((object = (Object *)objects->object)->otype == OT_OBJECT &&
                    ((object->qual & Q_INLINE) == 0 || object->datatype == DVFUNC))
                    CError_ReportError(ERR_ILLEGAL_SOM_FUNCTION_OVERLOAD, object);
            }
        }
    }
    CScope_InitObjectIterator(&iterator, tclass->nspace);
    for (;;) {
        firstMethod = CScope_NextObjectIteratorObject(&iterator);
        if (firstMethod == NULL)
            break;
        if ((firstMethod->qual & Q_INLINE) != 0)
            continue;
        CError_ASSERT(529, firstMethod->type->type == TYPEFUNC);
        ((TypeFunc *)firstMethod->type)->flags |= 4;
        tclass->action = 1;
        for (;;) {
            otherMethod = CScope_NextObjectIteratorObject(&iterator);
            if (otherMethod == NULL)
                break;
            if (otherMethod->type->type == TYPEFUNC)
                ((TypeFunc *)otherMethod->type)->flags &= ~4;
        }
        break;
    }
    if (tclass->sominfo->omitEnvironmentParameter == 0) {
        requiredType = CSOM_FindClassType(spaces_name);
        CScope_InitObjectIterator(&iterator, tclass->nspace);
        for (;;) {
            checkedMethod = CScope_NextObjectIteratorObject(&iterator);
            if (checkedMethod == NULL)
                break;
            if (checkedMethod->type->type != TYPEFUNC || TYPE_METHOD(checkedMethod->type)->is_static != 0 ||
                (TYPE_METHOD(checkedMethod->type)->flags & 32) != 0)
                continue;
            if ((args = TYPE_METHOD(checkedMethod->type)->args) != NULL && (args = args->next) != NULL &&
                args->type->type == TYPEPOINTER && TYPE_POINTER(args->type)->target == requiredType)
                continue;
            CError_ReportError(ERR_NEW_SOM_CALLSTYLE_METHOD_MUST_EXPLICIT, checkedMethod);
        }
    }
    if (tclass->action == 0)
        CError_ReportError(ERR_SOM_CLASS_MUST_ONE_NON_INLINE);
}

struct SOMVTable *find_or_add_base(SOMClassBuildState *list, TypeClass *unused, TypeClass *id, UInt16 *index)
{
    struct SOMVTable *p;
    struct SOMVTable *node;
    SInt16 count;

    p = list->bases;
    count = 0;
    while (p != NULL) {
        if (p->base == id) {
            if (index != NULL)
                *index = count;
            return p;
        }
        p = p->next;
        count++;
    }

    if (list->bases != NULL) {
        count = 1;
        p = list->bases;
        while (p->next != NULL) {
            p = p->next;
            count++;
        }
        p->next = (struct SOMVTable *)lalloc(sizeof(struct SOMVTable));
        memclrw(p->next, sizeof(struct SOMVTable));
        node = p->next;
    } else {
        count = 0;
        node = (struct SOMVTable *)lalloc(sizeof(struct SOMVTable));
        memclrw(node, sizeof(struct SOMVTable));
        list->bases = node;
    }
    node->base = id;
    if (index != NULL)
        *index = count;
    return node;
}

void build_class_vtables_and_members(SOMClassBuildState *layout, TypeClass *cls)
{
    ClassList *baseClass;
    SOMVTable *vtable;
    Object *method;
    VClassList *virtualBase;
    Object *candidate;
    CScopeObjectIterator classScope;
    CScopeObjectIterator baseScope;
    SOMEntry *newEntry;
    SOMEntry **entryLink;
    SOMSlot *overrideSlot;
    SInt32 entryIndex;
    SOMInfoEntry *infoEntry;
    SOMInfoEntry *baseEntry;
    SInt32 baseIndex;
    Object **methods;
    SInt32 methodCount;
    SInt32 methodIndex;
    Object *object;
    HashNameNode *key;

    for (baseClass = cls->bases; baseClass != NULL; baseClass = baseClass->next) {
        vtable = find_or_add_base(layout, cls, baseClass->base, NULL);
        vtable->isDirectBase = 1;
        layout->directBaseCount++;
    }

    if (cls->sominfo->baseClass != NULL && cls->sominfo->baseClass->sominfo != NULL) {
        vtable = find_or_add_base(layout, cls, cls->sominfo->baseClass, NULL);
        vtable->isImplicitBase = 1;
        layout->implicitBaseCount++;
    }

    CScope_InitObjectIterator(&classScope, cls->nspace);
    for (;;) {
        method = CScope_NextObjectIteratorObject(&classScope);
        if (method == NULL)
            break;
        if (method->type->type != TYPEFUNC)
            continue;
        if ((TYPE_FUNC(method->type)->flags & 0x20) == 0)
            continue;
        for (virtualBase = cls->vbases; virtualBase != NULL; virtualBase = virtualBase->next) {
            CScope_InitObjectIterator(&baseScope, virtualBase->base->nspace);
            for (;;) {
                candidate = CScope_NextObjectIteratorObject(&baseScope);
                if (candidate == NULL)
                    break;
                if (candidate->type->type != TYPEFUNC)
                    continue;
                if (method->name != candidate->name)
                    continue;
                if (candidate->datatype != DVFUNC)
                    continue;
                if ((TYPE_FUNC(candidate->type)->flags & 0x20) != 0)
                    continue;
                if (CClass_GetOverrideKind(TYPE_FUNC(method->type), TYPE_FUNC(candidate->type), 0) == 0)
                    continue;
                vtable = find_or_add_base(layout, cls, virtualBase->base, NULL);
                if ((overrideSlot = vtable->slots) != NULL) {
                    overrideSlot = (SOMSlot *)lalloc(sizeof(*overrideSlot));
                    memclrw(overrideSlot, sizeof(*overrideSlot));
                    overrideSlot->next = vtable->slots;
                    vtable->slots = overrideSlot;
                } else {
                    overrideSlot = (SOMSlot *)lalloc(sizeof(*overrideSlot));
                    memclrw(overrideSlot, sizeof(*overrideSlot));
                    vtable->slots = overrideSlot;
                    layout->overrideBaseCount++;
                }
                overrideSlot->overrideMethod = method;
                overrideSlot->baseMethod = candidate;
                break;
            }
        }
        layout->overrideMethodCount++;
    }

    entryLink = &layout->members;
    if (cls->sominfo->methodNameList != NULL) {
        for (infoEntry = cls->sominfo->methodNameList, entryIndex = 0; infoEntry != NULL;
             infoEntry = infoEntry->next, entryIndex++) {
            newEntry = (SOMEntry *)lalloc(offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
            memclrw(newEntry, offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
            *entryLink = newEntry;
            entryLink = &newEntry->next;
            newEntry->key = infoEntry->name;
            newEntry->kind = infoEntry->kind;
            switch (infoEntry->kind) {
                case 2:
                    for (virtualBase = cls->vbases; virtualBase != NULL; virtualBase = virtualBase->next) {
                        baseEntry = virtualBase->base->sominfo->methodNameList;
                        baseIndex = 0;
                        while (baseEntry != NULL) {
                            if (infoEntry->name == baseEntry->name && baseEntry->kind == 1) {
                                find_or_add_base(layout, cls, virtualBase->base, &newEntry->u.vt.offset);
                                newEntry->u.vt.index = (UInt16)baseIndex;
                                break;
                            }
                            baseEntry = baseEntry->next;
                            baseIndex++;
                        }
                        if (baseEntry != NULL)
                            break;
                    }
                    layout->inheritedMemberCount++;
                    break;
                case 1:
                    CScope_InitObjectIterator(&classScope, cls->nspace);
                    for (;;) {
                        candidate = CScope_NextObjectIteratorObject(&classScope);
                        if (candidate == NULL)
                            break;
                        if (candidate->type->type != TYPEFUNC)
                            continue;
                        key = CSOM_NameTranslate(candidate->name);
                        if (key != infoEntry->name)
                            continue;
                        CError_ASSERT(733, TYPE_METHOD(candidate->type)->vtbl_index == entryIndex);
                        newEntry->u.object = candidate;
                        break;
                    }
                    CError_ASSERT(737, candidate != NULL);
                    break;
                default:
                    break;
            }
            layout->memberCount++;
        }
    } else {
        methods = build_vtbl_index_table(cls, &methodCount);
        for (methodIndex = 0; methodIndex < methodCount; methodIndex++) {
            if ((object = methods[methodIndex]) != NULL) {
                newEntry = (SOMEntry *)lalloc(offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
                memclrw(newEntry, offsetof(SOMEntry, flag) + sizeof(newEntry->flag));
                *entryLink = newEntry;
                entryLink = &newEntry->next;
                newEntry->u.object = object;
                newEntry->key = object->name;
                newEntry->kind = 1;
                layout->memberCount++;
            }
        }
    }
}

void create_ancestor_object(SOMClassBuildState *info, TypeClass *cls)
{
    SOMVTable *base;
    Object *obj;
    OLinkList *head;
    SInt32 count;

    if (info->bases != NULL) {
        obj = CSOM_MakeObject(cls->classname->name, "ClassAncestors", 4);
        obj->sclass = TK_STATIC;

        head = NULL;
        count = 0;
        for (base = info->bases; base != NULL; base = base->next) {
            OLinkList *n = (OLinkList *)lalloc(0x10);
            n->next = head;
            head = n;
            n->obj = base->base->sominfo->classDataObject;
            n->offset = count;
            n->addend = 0;
            count += 4;
        }
        {
            char *buf;
            memclrw(buf = (char *)lalloc(count), count);
            obj->type->size = count;
            CInit_DeclareData(obj, buf, head, obj->type->size);
        }
        info->ancestorObject = obj;
    }
}

void create_override_methods_object(SOMClassBuildState *cls, TypeClass *func)
{
    Object *obj;
    SInt32 size;
    SOMSlot *e;
    SOMVTable *b;
    OLinkList *list;
    SInt32 offset;
    OLinkList *op;
    ENode *array;

    if (cls->overrideMethodCount) {
        size = cls->overrideMethodCount * 4;
        obj = CSOM_MakeObject(func->classname->name, "OverrideProcs", size);
        obj->sclass = TK_STATIC;

        list = NULL;
        offset = 0;
        for (b = cls->bases; b != NULL; b = b->next) {
            if (b->slots != NULL) {
                for (e = b->slots; e != NULL; e = e->next) {
                    op = (OLinkList *)lalloc(0x10);
                    op->next = list;
                    list = op;
                    op->obj = e->overrideMethod;
                    op->offset = offset;
                    op->addend = 0;
                    offset += 4;
                }
            }
        }
        memclrw(array = lalloc(size), size);
        CInit_DeclareData(obj, array, list, obj->type->size);
        cls->overrideMethodsObject = obj;
    }
}

Object *build_base_method_vtbl_index_object(SOMClassBuildState *groups)
{
    SOMVTable *group;
    int groupIndex;
    SOMSlot *method;
    Object *object;
    unsigned int count;
    Object *result;

    data_00583548.size = 0;
    groupIndex = 0;
    group = groups->bases;
    while (group != NULL) {
        if (group->slots != NULL) {
            AppendGListWord(&data_00583548, CTool_EndianConvertWord16(groupIndex));
            method = group->slots;
            count = 0;
            while (method != NULL) {
                method = method->next;
                count++;
            }
            AppendGListWord(&data_00583548, CTool_EndianConvertWord16(count));
            method = group->slots;
            while (method != NULL) {
                object = method->baseMethod;
                if (!(object->type->type == TYPEFUNC && (((TypeMemberFunc *)object->type)->flags & FUNC_METHOD)))
                    CError_FATAL(173);
                AppendGListWord(&data_00583548,
                                CTool_EndianConvertWord16(((TypeMemberFunc *)object->type)->vtbl_index));
                method = method->next;
            }
        }
        group = group->next;
        groupIndex++;
    }
    COS_LockHandle(data_00583548.data);
    result = CInit_DeclareString(*data_00583548.data, data_00583548.size, 0, 0);
    COS_UnlockHandle(data_00583548.data);
    return result;
}

static void *MakeWords(SOMClassBuildState *rec)
{
    SOMEntry *p;
    SOMEntry *ps;
    int i;

    data_00583548.size = 0;
    p = ps = (SOMEntry *)rec->members;
    i = 0;
    if (ps) {
        do {
            if (p->kind == 2) {
                AppendGListWord(&data_00583548, CTool_EndianConvertWord16(p->u.vt.offset));
                AppendGListWord(&data_00583548, CTool_EndianConvertWord16(p->u.vt.index));
                AppendGListWord(&data_00583548, CTool_EndianConvertWord16(i));
            }
            i++;
        } while ((p = p->next) != NULL);
    }
    return FlushData();
}

static void MakeRegistrationFunction(SOMClassBuildState *state, TypeClass *cls)
{
    TypeFunc *type;
    Object *object;

    type = galloc(sizeof(TypeFunc));
    memclrw(type, sizeof(TypeFunc));
    type->type = TYPEFUNC;
    type->functype = &stvoid;
    object = CParser_NewCompilerDefFunctionObject();
    object->type = (Type *)type;
    object->sclass = TK_STATIC;
    object->name = CParser_NameConcat(cls->classname->name, "DLLD");
    if (CScope_GetLocalObject(cscope_root, object->name) != NULL)
        CError_ReportError(ERR_OBJECT_REDEFINED, object);
    state->registrationFunction = object;
    CFunc_GenerateDummyFunction(object);
}

void create_special_functions_object(SOMClassBuildState *info, TypeClass *cls)
{
    CScopeObjectIterator search;
    char initialData[16];
    OLinkList *relocations;
    int dataSize;
    Object *object;
    Object *newOperator;
    Object *deleteOperator;
    OLinkList *relocation;

    newOperator = deleteOperator = NULL;
    CScope_InitObjectIterator(&search, cls->nspace);
    for (;;) {
        object = CScope_NextObjectIteratorObject(&search);
        if (object == NULL)
            break;
        if (object->type->type == TYPEFUNC) {
            if (object->name == CMangler_OperatorName(TK_NEW)) {
                newOperator = object;
                info->hasNewOperator = 1;
            } else if (object->name == CMangler_OperatorName(TK_DELETE)) {
                deleteOperator = object;
                info->hasDeleteOperator = 1;
            }
        }
    }
    if (newOperator != NULL || deleteOperator != NULL) {
        object = CSOM_MakeObject(cls->classname->name, "SpecialProcs", 4);
        object->sclass = TK_STATIC;
        relocations = NULL;
        dataSize = 0;
        if (newOperator != NULL) {
            relocation = (OLinkList *)lalloc(sizeof(OLinkList));
            relocation->next = relocations;
            relocations = relocation;
            relocation->obj = newOperator;
            relocation->offset = dataSize;
            relocation->addend = 0;
            dataSize += 4;
        }
        if (deleteOperator != NULL) {
            relocation = (OLinkList *)lalloc(sizeof(OLinkList));
            relocation->next = relocations;
            relocations = relocation;
            relocation->obj = deleteOperator;
            relocation->offset = dataSize;
            relocation->addend = 0;
            dataSize += 4;
        }
        memclrw(initialData, sizeof(initialData));
        object->type->size = dataSize;
        CInit_DeclareData(object, initialData, relocations, object->type->size);
        info->specialFunctionsObject = object;
    }
}

static Object *MakeBases(SOMClassBuildState *record)
{
    SOMVTable *entry;
    SInt32 *values;
    SInt32 size;
    int index;

    size = (record->directBaseCount + record->implicitBaseCount) * (2 * sizeof(*values));
    values = lalloc(size);
    for (entry = record->bases, index = 0; entry != NULL; entry = entry->next) {
        if (entry->isImplicitBase != 0 || entry->isDirectBase != 0) {
            values[index++] = CTool_EndianConvertWord32(entry->base->sominfo->descriptorValue0);
            values[index++] = CTool_EndianConvertWord32(entry->base->sominfo->descriptorValue1);
        }
    }
    return CInit_DeclareString((char *)values, size, 0, 0);
}

static void CSOM_SetNibble(UInt8 *bits, int i, int value)
{
    bits += i >> 1;
    if (i & 1)
        *bits = (*bits & 0xf0) | (value & 0x0f);
    else
        *bits = (*bits & 0x0f) | (value << 4);
}

Object *emit_som_kind_nibbles(SOMClassBuildState *info)
{
    SOMEntry *n;
    int size;
    UInt8 *bits;
    unsigned int i;

    size = (info->memberCount + 1) / 2;
    memclrw(bits = lalloc(size), size);
    for (n = info->members, i = 0; n != NULL; n = n->next, i++) {
        switch (n->kind) {
            case 0:
            case 2:
                CSOM_SetNibble(bits, i, 3);
                break;
            case 1:
                CSOM_SetNibble(bits, i, 0);
                break;
            default:
                CError_FATAL(1048);
                break;
        }
    }
    return CInit_DeclareString((char *)bits, size, 0, 0);
}

static void *MakeOverrides(SOMClassBuildState *rec)
{
    SOMEntry *p;
    SOMEntry *ps;

    data_00583548.size = 0;
    p = ps = (SOMEntry *)rec->members;
    if (ps) {
        do {
            if (p->kind == 1)
                encode_member_function_types(TYPE_METHOD(p->u.object->type), 1);
        } while ((p = p->next) != NULL);
    }
    return FlushData();
}

static Object *MakeNames(SOMClassBuildState *record)
{
    SOMEntry *entry;

    data_00583548.size = 0;
    for (entry = record->members; entry != NULL; entry = entry->next) {
        if (entry->key != NULL)
            AppendGListID(&data_00583548, CSOM_NameTranslate(entry->key)->name);
    }
    return FlushData();
}

void build_descriptor_output(SOMClassBuildState *desc, TypeClass *cls, struct SOMDescriptorOutput *out)
{
    desc->descriptorValues[0] = cls->sominfo->descriptorValue0;
    desc->descriptorValues[1] = cls->sominfo->descriptorValue1;
    desc->descriptorFlags = 1;
    if (desc->hasNewOperator)
        desc->descriptorFlags |= 0x100;
    if (desc->hasDeleteOperator)
        desc->descriptorFlags |= 0x200;

    switch (cls->align) {
        case 1:
            desc->alignmentKind = 0;
            break;
        case 2:
            desc->alignmentKind = 1;
            break;
        case 4:
            desc->alignmentKind = 2;
            break;
        case 8:
            desc->alignmentKind = 3;
            break;
        default:
            desc->alignmentKind = 4;
    }

    desc->descriptorAttribute5 = 0;

    memclrw(out, sizeof(*out));

    out->descriptorValues[0] = CTool_EndianConvertWord32(desc->descriptorValues[0]);
    out->descriptorValues[1] = CTool_EndianConvertWord32(desc->descriptorValues[1]);
    out->descriptorFlags = CTool_EndianConvertWord32(desc->descriptorFlags);
    out->alignmentKind = CTool_EndianConvertWord16(desc->alignmentKind);
    out->memberCount = CTool_EndianConvertWord16(desc->memberCount);
    out->directBaseCount = CTool_EndianConvertWord16(desc->directBaseCount);
    out->implicitBaseCount = CTool_EndianConvertWord16(desc->implicitBaseCount);
    out->overrideBaseCount = CTool_EndianConvertWord16(desc->overrideBaseCount);
    out->inheritedMemberCount = CTool_EndianConvertWord16(desc->inheritedMemberCount);
    out->descriptorAttribute5 = CTool_EndianConvertWord16(desc->descriptorAttribute5);
}

void make_class_descriptor(SOMClassBuildState *record, TypeClass *classType)
{
    SOMClassDescriptor descriptor;
    struct SOMDescriptorOutput descriptorText;
    Object *descriptorObject;
    OLinkList *head;
    OLinkList *node;

    descriptorObject = CSOM_MakeObject(classType->classname->name, "SCI", sizeof(descriptor));
    descriptorObject->sclass = TK_STATIC;
    memclrw(&descriptor, sizeof(descriptor));
    descriptor.value0 = CTool_EndianConvertWord32(0x46);

    head = NULL, node = lalloc(sizeof(*node));
    node->next = head;
    node->obj = classType->sominfo->classDataObject;
    head = node;
    node->offset = 4;
    node->addend = 0;
    if (record->overrideMethodsObject != NULL) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = record->overrideMethodsObject;
        node->offset = 8;
        node->addend = 0;
    }
    if (record->ancestorObject != NULL) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = record->ancestorObject;
        node->offset = 0xc;
        node->addend = 0;
    }
    if (record->registrationFunction != NULL) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = record->registrationFunction;
        node->offset = 0x10;
        node->addend = 0;
    }
    if (record->specialFunctionsObject != NULL) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = record->specialFunctionsObject;
        node->offset = 0x14;
        node->addend = 0;
    }

    build_descriptor_output(record, classType, &descriptorText);
    node = lalloc(sizeof(*node));
    node->next = head;
    head = node;
    node->obj = CInit_DeclareString((char *)&descriptorText, sizeof(descriptorText), 0, 0);
    node->offset = 0x34;
    node->addend = 0;

    node = lalloc(sizeof(*node));
    node->next = head;
    head = node;
    node->obj = CInit_DeclareString(classType->classname->name, strlen(classType->classname->name) + 1, 0, 0);
    node->offset = 0x38;
    node->addend = 0;

    descriptor.classSize = CTool_EndianConvertWord32(classType->size);

    node = lalloc(sizeof(*node));
    node->next = head;
    head = node;
    node->obj = MakeBases(record);
    node->offset = 0x40;
    node->addend = 0;

    if (record->members != NULL) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = emit_som_kind_nibbles(record);
        node->offset = 0x44;
        node->addend = 0;

        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = MakeOverrides(record);
        node->offset = 0x48;
        node->addend = 0;

        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = MakeNames(record);
        node->offset = 0x4c;
        node->addend = 0;
    }

    if (record->overrideMethodsObject != NULL) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = build_base_method_vtbl_index_object(record);
        node->offset = 0x50;
        node->addend = 0;
    }
    if (record->inheritedMemberCount != 0) {
        node = lalloc(sizeof(*node));
        node->next = head;
        head = node;
        node->obj = MakeWords(record);
        node->offset = 0x54;
        node->addend = 0;
    }

    descriptor.value88 = 0;
    CInit_DeclareData(descriptorObject, &descriptor, head, descriptorObject->type->size);
    record->object = descriptorObject;
}

void initialize_class_data_object(SOMClassBuildState *methods, TypeClass *tclass)
{
    OLinkList *list;
    SInt32 offset;
    OLinkList *init;
    SOMEntry *m;
    ENode *buf;

    list = NULL;
    offset = 0x18;
    for (m = (SOMEntry *)methods->members; m; m = m->next) {
        if (m->kind == 1) {
            init = (OLinkList *)(lalloc(sizeof(OLinkList)));
            init->next = list;
            list = init;
            init->obj = m->u.object;
            init->offset = offset;
            init->addend = 0;
        }
        offset += 4;
    }

    memclrw(buf = lalloc(offset), offset);

    init = (OLinkList *)(lalloc(sizeof(OLinkList)));
    init->next = list;
    init->obj = methods->object;
    init->offset = 4;
    init->addend = 0;

    tclass->sominfo->classDataObject->type->size = offset;
    CInit_DeclareData(tclass->sominfo->classDataObject, buf, init, tclass->sominfo->classDataObject->type->size);
}

void CSOM_BuildClass(TypeClass *func)
{
    SOMClassBuildState state;

    memclrw(&state, sizeof(state));
    build_class_vtables_and_members(&state, func);
    create_ancestor_object(&state, func);
    create_override_methods_object(&state, func);
    MakeRegistrationFunction(&state, func);
    create_special_functions_object(&state, func);
    make_class_descriptor(&state, func);
    initialize_class_data_object(&state, func);
}

static TypeClass *GetOwner(void)
{
    TypeClass *tclass;

    if ((tclass = cscope_current->theclass) != NULL && tclass->sominfo != NULL)
        return tclass;
    CError_ReportError(ERR_ILLEGAL_USE_PRAGMA_OUTSIDE_SOM_CLASS);
    return NULL;
}

void CSOM_ParseMethodNameList(void)
{
    char bare;
    short token;
    TypeClass *theclass;
    SOMInfoEntry **tail;
    SOMInfoEntry *entry;
    short delimiter;
    SOMInfoEntry *newEntry;
    struct SOMPragmaNames names;
    int keywordLength;
    keywordLength = sizeof("list");
    if (!(theclass = GetOwner())) {
        return;
    }
    token = CPrep_ExpectEndLine(0);
    if (token != 40) {
        if (token != -3) {
            CPrep_ReportError(114);
            return;
        }
        if (memcmp(data_00587fa0->name, "list", keywordLength) == 0) {
            token = CPrep_ExpectEndLine(0);
            if (token != -3) {
                CPrep_ReportError(107);
                return;
            }
        }
        bare = 1;
    } else {
        bare = 0;
        token = CPrep_ExpectEndLine(0);
    }
    names.head = NULL;
    if (bare != 0 || token != 41) {
        tail = &names.head;
        for (;;) {
            if (token != -3) {
                CPrep_ReportError(107);
                return;
            }
            for (entry = names.head; entry != NULL; entry = entry->next) {
                if (entry->name == data_00587fa0) {
                    CError_ReportError(ERR_IDENTIFIER_REDECLARED, data_00587fa0->name);
                    return;
                }
            }
            newEntry = (SOMInfoEntry *)galloc(10);
            *tail = newEntry;
            newEntry->next = NULL;
            tail = &newEntry->next;
            newEntry->name = data_00587fa0;
            newEntry->kind = 0;
            if (bare != 0) {
                delimiter = CPrep_ExpectEndLine(1);
                if (delimiter == 0) {
                    break;
                }
            } else {
                delimiter = CPrep_ExpectEndLine(0);
                if (delimiter == 41) {
                    break;
                }
            }
            if (delimiter != 44) {
                CPrep_ReportError(116);
                return;
            }
            token = CPrep_ExpectEndLine(bare);
        }
    }
    theclass->sominfo->methodNameList = names.head;
}

void CSOM_ParseDescriptorValues(void)
{
    Type *theclass;
    if (CPrep_ExpectEndLine(0) != '(') {
        CPrep_ReportError(114);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(107);
        return;
    }
    theclass = CScope_GetTagType(cscope_current, data_00587fa0);
    if (!theclass || theclass->type != TYPECLASS || !TYPE_CLASS(theclass)->sominfo) {
        fn_0043f3e0(276, data_00587fa0->name);
        return;
    }
    if (CPrep_ExpectEndLine(0) != ',') {
        CPrep_ReportError(116);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -1) {
        CPrep_ReportError(186);
        return;
    }
    TYPE_CLASS(theclass)->sominfo->descriptorValue0 = intconst_lo;
    if (CPrep_ExpectEndLine(0) != ',') {
        CPrep_ReportError(116);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -1) {
        CPrep_ReportError(186);
        return;
    }
    TYPE_CLASS(theclass)->sominfo->descriptorValue1 = intconst_lo;
    if (CPrep_ExpectEndLine(0) != ')') {
        CPrep_ReportError(115);
        return;
    }
}

void CSOM_ParseBaseClass(void)
{
    Type *cls;
    Type *base;

    if (CPrep_ExpectEndLine(0) != 0x28) {
        CPrep_ReportError(0x72);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(0x6b);
        return;
    }
    cls = CScope_GetTagType(cscope_current, data_00587fa0);
    if (cls == NULL || !IS_TYPE_CLASS(cls) || TYPE_CLASS(cls)->sominfo == NULL) {
        fn_0043f3e0(0x114, data_00587fa0->name);
        return;
    }
    if (CPrep_ExpectEndLine(0) != 0x2c) {
        CPrep_ReportError(0x74);
        return;
    }
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(0x6b);
        return;
    }
    base = CScope_GetTagType(cscope_current, data_00587fa0);
    if (base == NULL || !IS_TYPE_CLASS(base) || TYPE_CLASS(base)->sominfo == NULL) {
        fn_0043f3e0(0x114, data_00587fa0->name);
        return;
    }
    TYPE_CLASS(cls)->sominfo->baseClass = TYPE_CLASS(base);
    if (CPrep_ExpectEndLine(0) != 0x29) {
        CPrep_ReportError(0x73);
        return;
    }
}

void set_owner_target_flag(void)
{
    TypeClass *owner;
    if (!(owner = GetOwner()))
        return;
    if (CPrep_ExpectEndLine(0) != -3) {
        CPrep_ReportError(0x6b);
        return;
    }
    if (!memcmp(data_00587fa0->name, "IDL", 4)) {
        owner->sominfo->omitEnvironmentParameter = 0;
        return;
    }
    if (!memcmp(data_00587fa0->name, "OIDL", 5)) {
        owner->sominfo->omitEnvironmentParameter = 1;
        return;
    }
    CPrep_ReportError(0xba);
}

void CSOM_PrependTheClassArg(TypeFunc *function)
{
    Type *type;
    TypeFunc *func;
    FuncArg *arg;
    HashNameNode *name;
    func = function;
    arg = CParser_NewFuncArg();
    arg->name = GetHashNameNode("__theclass");
    arg->type = CDecl_NewPointerType(CSOM_FindClassType(GetHashNameNode("SOMClass")));
    arg->next = func->args;
    func->args = arg;
}

Object *fn_004e45b0(char *name, char *signature)
{
    ObjectList *list;
    Object *obj;
    FuncArg *arg;

    list = CScope_GetLocalObject(cscope_root, GetHashNameNode(name));
    if (list != NULL && (obj = list->object)->otype == OT_OBJECT) {
        if (obj->type->type == TYPEFUNC && *signature++ == 'p' && TYPE_FUNC(obj->type)->functype->type == TYPEPOINTER) {
            for (arg = TYPE_FUNC(obj->type)->args; arg != NULL; arg = arg->next) {
                switch (*signature++) {
                    case 'p':
                        if (arg->type->type != TYPEPOINTER)
                            break;
                        continue;
                    case 'i':
                        if (arg->type != (Type *)&stsignedint)
                            break;
                        continue;
                    case 'I':
                        if (&arg->type->type != &stunsignedint.type)
                            break;
                        continue;
                    case 'l':
                        if (arg->type != (Type *)&stsignedlong)
                            break;
                        continue;
                    case 'L':
                        if (arg->type != (Type *)&stunsignedlong)
                            break;
                        continue;
                    default:
                        break;
                }
                break;
            }
            if (arg == NULL && *signature == '\0')
                return obj;
        }
        CError_ReportError(ERR_SOM_RUNTIME_FUNCTION_UNEXPECTED_TYPE, name);
    } else {
        CError_ReportError(ERR_SOM_RUNTIME_FUNCTION_NOT_DEFINED_SHOULD, name);
    }
    return NULL;
}

static ENode *CSOM_MakeConditional(ENode *before, ENode *cond, ENode *expr, ENode *after)
{
    ENode *node;

    node = lalloc(sizeof(ENode));
    node->type = ECOND;
    node->cost = 0;
    node->flags = 0;
    node->rtype = &stvoid;
    node->data.cond.cond = cond;
    node->data.cond.expr1 = expr;
    node->data.cond.expr2 = nullnode();
    node->data.cond.expr2->rtype = &stvoid;
    if (before != NULL)
        node = makediadicnode(before, node, ECOMMA);
    if (after != NULL) {
        node = makediadicnode(node, after, ECOMMA);
        node->rtype = after->rtype;
    }
    return node;
}

ENode *CSOM_BuildNewObjectInstance(TypeClass *cls)
{
    Object *obj;
    ENode *monadic;
    ENode *temp;
    ENode *callnode;
    ENode *call2;

    if (tk == '(') {
        tk = CPrepTokenizer_GetNextToken();
        if (tk == ')') {
            tk = CPrepTokenizer_GetNextToken();
        } else {
            CError_ReportError(ERR_NO_PARAMETERS_ALLOWED_SOM_CLASS_CONSTRUCTORS);
        }
    }

    if (copts.SOMCheckEnvironment == 0 || copts.SOMCallOptimization == 0) {
        obj = fn_004e45b0("somNewObjectInstance", "ppll");
        if (obj == NULL)
            return nullnode();
    } else {
        obj = data_00588060;
    }

    callnode = funccallexpr(obj, create_objectrefnode(cls->sominfo->classDataObject),
                            intconstnode((Type *)&stunsignedlong, cls->sominfo->descriptorValue0),
                            intconstnode((Type *)&stunsignedlong, cls->sominfo->descriptorValue1), NULL);
    callnode->rtype = CDecl_NewPointerType((Type *)cls);

    if (copts.SOMCheckEnvironment != 0 && copts.SOMCallOptimization == 0) {
        temp = CExpr_GetETEMPCopy(callnode);
        call2 = funccallexpr(data_005876c0, nullnode(), NULL, NULL, NULL);
        monadic = makemonadicnode(callnode, ELOGNOT);
        monadic->rtype = CParser_GetBoolType();
        callnode = CSOM_MakeConditional(NULL, monadic, call2, temp);
    }
    return callnode;
}

ENode *CSOM_CallReleaseObjectReference(TypeClass *unused, ENode *argument)
{
    Object *object;

    object = fn_004e45b0("somReleaseObjectReference", "pp");
    if (object != NULL) {
        return funccallexpr(object, argument, NULL, NULL, NULL);
    }
    return nullnode();
}

void fn_004e4390(Object *obj)
{
    TypeClass *type;
    Statement *node;
    Object *method;

    method = fn_004e45b0("somReleaseObjectReference", "pp");
    if (method != NULL) {
        type = TYPE_CLASS(obj->type);
        obj->type = CDecl_NewPointerType((Type *)type);
        TYPE_POINTER(obj->type)->qual = Q_REFERENCE;
        node = CFunc_AppendStatement(EINDIRECT);
        node->expr = makediadicnode(create_objectnode2(obj), CSOM_BuildNewObjectInstance(type), EASS);
        CExcept_RegisterDeleteObject(node, obj, method);
    }
}

void find_method_vtbl_class_and_offset(TypeClass *cls, Object *method, TypeClass **outcls, SInt32 *outofs)
{
    struct CScopeObjectIterator state;
    VClassList *vbase;
    Object *found;

    if ((TYPE_METHOD(method->type)->flags & 0x20) == 0) {
        CScope_InitObjectIterator(&state, cls->nspace);
        for (;;) {
            found = CScope_NextObjectIteratorObject(&state);
            if (found == NULL)
                break;
            if (found == method) {
                *outcls = cls;
                *outofs = CSOM_GetTokenTableIndex(found);
                return;
            }
        }
        for (vbase = cls->vbases; vbase != NULL; vbase = vbase->next) {
            CScope_InitObjectIterator(&state, vbase->base->nspace);
            for (;;) {
                found = CScope_NextObjectIteratorObject(&state);
                if (found == NULL)
                    break;
                if (found == method) {
                    *outcls = vbase->base;
                    *outofs = CSOM_GetTokenTableIndex(found);
                    return;
                }
            }
        }
    } else {
        for (vbase = cls->vbases; vbase != NULL; vbase = vbase->next) {
            CScope_InitObjectIterator(&state, vbase->base->nspace);
            for (;;) {
                found = CScope_NextObjectIteratorObject(&state);
                if (found == NULL)
                    break;
                if (found->name == method->name) {
                    if (found->type->type == TYPEFUNC && found->datatype == DVFUNC &&
                        (TYPE_METHOD(found->type)->flags & 0x20) == 0 &&
                        CClass_GetOverrideKind(TYPE_FUNC(method->type), TYPE_FUNC(found->type), 0)) {
                        *outcls = vbase->base;
                        *outofs = CSOM_GetTokenTableIndex(found);
                        return;
                    }
                    break;
                }
            }
        }
    }
    CError_FATAL(1731);
}

static ENode *CSOM_CallClassDataMethod(TypeClass *tclass, ENode *expr)
{
    ENode *method;
    Object object;

    method = create_objectrefnode(tclass->sominfo->classDataObject);
    method = makediadicnode(method, intconstnode((Type *)&stsignedlong, 8), EADD);
    method->rtype = CDecl_NewPointerType(TYPE(&data_005646b8));
    method = makemonadicnode(method, EINDIRECT);
    memclrw(&object, sizeof(Object));
    object.otype = OT_OBJECT;
    object.name = unnamed_name;
    object.datatype = DFUNC;
    object.type = TYPE(&data_005646b8);
    expr = funccallexpr(&object, expr, NULL, NULL, NULL);
    CError_ASSERT(1761, expr->type == EFUNCCALL);
    expr->data.funccall.funcref = method;
    return expr;
}

ENode *CSOM_GetOrCreateLocalObjectNode(TypeClass *value)
{
    Object *object;
    ObjectList *node;

    for (node = locals; node != NULL; node = node->next) {
        if (node->object->name == csom_blank_name) {
            object = node->object;
            return create_objectnode(object);
        }
    }
    object = CParser_NewLocalDataObject(NULL, 1);
    object->name = csom_blank_name;
    object->type = (Type *)CDecl_NewPointerType((Type *)value);
    CFunc_SetupLocalVarInfo(object);
    return create_objectnode(object);
}

void CSOM_GenerateSomselfAssignment(TypeClass *tclass, Statement *stmt)
{
    HashNameNode *name;
    ObjectList *ivar;
    ENode *somself;
    Statement *s;
    ENode *call;

    name = GetHashNameNode("__somself");
    for (ivar = locals; ivar; ivar = ivar->next) {
        if (ivar->object->name == name) {
            somself = CClass_CreateThisSelfExpr();
            CError_ASSERT(1811, somself != NULL);
            call = CSOM_CallClassDataMethod(tclass, somself);
            s = CFunc_InsertAfterStatement(4, stmt);
            s->expr = makediadicnode(create_objectnode(ivar->object), call, EASS);
            break;
        }
    }
}

ENode *CSOM_AppendPointerArgCall(ENode *node, ENodeList *spec)
{
    Type *resultType = node->rtype;
    ENodeList *pointerArg;
    ENode *pointerExpr;
    ENode *resultExpr;

    CError_ASSERT(1842, (pointerArg = node->data.funccall.args) != NULL);
    CError_ASSERT(1845, pointerArg != spec || (pointerArg = pointerArg->next) != NULL);
    pointerArg = pointerArg->next;
    CError_ASSERT(1847, pointerArg != NULL);
    CError_ASSERT(1850, pointerArg != spec || (pointerArg = pointerArg->next) != NULL);
    CError_ASSERT(1852, pointerArg->node->rtype->type == TYPEPOINTER);

    if (node->data.funccall.functype->functype->type != TYPEVOID) {
        if (spec != NULL) {
            if (spec->node->type == ETEMP) {
                if (spec->node->data.temp.uniqueid == 0)
                    spec->node->data.temp.uniqueid = CParser_GetUniqueID();
                resultExpr = lalloc(sizeof(ENode));
                *resultExpr = *spec->node;
                resultExpr->data.temp.needs_dtor = 0;
            } else {
                resultExpr = CExpr_GetETEMPCopy(spec->node);
            }
        } else {
            resultExpr = CExpr_GetETEMPCopy(node);
        }
    } else {
        resultExpr = NULL;
    }

    if (pointerArg->node->type != EOBJREF) {
        if (pointerArg->node->type == EINDIRECT && pointerArg->node->data.monadic->type == EOBJREF &&
            pointerArg->node->data.monadic->data.objref->datatype == DLOCAL) {
            pointerExpr = lalloc(sizeof(ENode));
            *pointerExpr = *pointerArg->node;
        } else {
            pointerExpr = CExpr_GetETEMPCopy(pointerArg->node);
        }
    } else {
        pointerExpr = lalloc(sizeof(ENode));
        *pointerExpr = *pointerArg->node;
    }

    if (copts.SOMCallOptimization != 0) {
        ENode *appendCall;
        appendCall = funccallexpr(data_00588278, pointerExpr, NULL, NULL, NULL);
        node = makediadicnode(node, appendCall, ECOMMA);
        if (resultExpr != NULL)
            node = makediadicnode(node, resultExpr, ECOMMA);
    } else {
        ENode *pointerValue;
        ENode *appendCall;

        pointerValue = lalloc(sizeof(ENode));
        *pointerValue = *pointerExpr;
        pointerValue = makemonadicnode(pointerValue, EINDIRECT);
        pointerValue->rtype = (Type *)&stsignedlong;
        appendCall = funccallexpr(data_00588278, pointerExpr, NULL, NULL, NULL);
        node = CSOM_MakeConditional(node, pointerValue, appendCall, resultExpr);
    }

    node->rtype = resultType;
    return node;
}

Boolean fn_004e3cd0(Type *ftype)
{
    SInt32 integerRegisters = 8;
    SInt32 floatRegisters = 13;
    FuncArg *arg;

    if (CMach_GetFunctionResultClass((TypeFunc *)ftype))
        integerRegisters--;

    arg = TYPE_FUNC(ftype)->args;
    while (arg != NULL) {
        if (arg == &elipsis || arg == &oldstyle)
            return 0;
        switch ((SInt8)arg->type->type) {
            case TYPEINT:
            case TYPEENUM:
            case TYPEPOINTER:
                if (--integerRegisters < 0)
                    return 0;
                break;
            case TYPEFLOAT:
                if (--floatRegisters < 0)
                    return 0;
                break;
            default:
                return 0;
        }
        arg = arg->next;
    }
    return 1;
}

static char *CSOM_CopyName(char *dst, const char *src)
{
    char c;

    while ((c = *src++) != 0)
        *dst++ = c;
    return dst;
}

ENode *create_glue_objectrefnode(TypeClass *cls, SInt32 id, Object *obj)
{
    struct CSOMRefNode *ref;
    char *cursor;
    char *buffer;
    Boolean memoryReturn;
    UInt32 length;
    char work[256];
    char number[16];
    Object *function;
    ENode *node;

    ref = somReferences;
    while (ref != NULL) {
        if (ref->theclass == cls && ref->id == id)
            break;
        ref = ref->next;
    }
    if (ref == NULL) {
        memoryReturn = CMach_GetFunctionResultClass((TypeFunc *)obj->type);
        length = strlen(cls->sominfo->classDataObject->name->name) + 32;
        if (length > sizeof(work))
            buffer = (char *)lalloc(length);
        else
            buffer = work;
        cursor = CSOM_CopyName(buffer, "__glue_");
        if (cls->sominfo->omitEnvironmentParameter == 0) {
            if (memoryReturn == 0)
                *cursor = '4';
            else
                *cursor = '5';
        } else {
            *cursor = '_';
        }
        *++cursor = '_';
        cursor++;
        sprintf(number, "%ld", (long)strlen(cls->sominfo->classDataObject->name->name));
        cursor = CSOM_CopyName(cursor, number);
        cursor = CSOM_CopyName(cursor, cls->sominfo->classDataObject->name->name);
        *cursor = '_';
        cursor++;
        sprintf(number, "%ld", id);
        cursor = CSOM_CopyName(cursor, number);
        *cursor = 0;

        function = CParser_NewCompilerDefFunctionObject();
        function->nspace = cscope_root;
        function->name = GetHashNameNode(buffer);
        function->u.func.linkname = function->name;
        function->type = obj->type;
        function->qual = obj->qual | Q_IMPLICIT_WEAK;
        function->flags = 0x10;
        CScope_AddObject(function->nspace, function->name, (ObjBase *)function);

        ref = (struct CSOMRefNode *)galloc(0x12);
        ref->next = somReferences;
        ref->object = function;
        ref->theclass = cls;
        ref->id = id;
        somReferences = ref;
        if (cls->sominfo->omitEnvironmentParameter == 0) {
            if (memoryReturn == 0)
                ref->kind = 0;
            else
                ref->kind = 1;
        } else {
            ref->kind = 2;
        }
    }
    node = create_objectrefnode(ref->object);
    node->rtype = CDecl_NewPointerType(obj->type);
    return node;
}

ENode *CSOM_CreateMemberAccessExpr(BClassList *classList, ObjMemberVar *request, ENode *expr)
{
    ENode *access;

    if (expr == NULL && (cscope_currentfunc == NULL || cscope_currentclass == NULL || cscope_is_member_func == 0 ||
                         (expr = CClass_CreateThisSelfExpr()) == NULL)) {
        CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
        return NULL;
    }
    CError_ASSERT(2069, expr->type == EINDIRECT);
    expr = expr->data.monadic;
    if (classList->next == NULL && cscope_currentclass == TYPE_CLASS(classList->type) && expr->type == EOBJREF &&
        expr->data.objref->name == this_arg_name) {
        access = CSOM_GetOrCreateLocalObjectNode(cscope_currentclass);
    } else {
        CClass_CheckBaseAccess(classList, request->access);
        if (request->has_path != 0)
            classList = ((ObjMemberVarPath *)request)->path;
        while (classList->next != NULL)
            classList = classList->next;
        access = CSOM_CallClassDataMethod(TYPE_CLASS(classList->type), expr);
    }
    access = makemonadicnode(access, EINDIRECT);
    access->rtype = classList->type;
    return CClass_AccessMember(access, request->type, request->qual, request->offset);
}

ENode *CSOM_MakeMethodReference(BClassList *classPath, Object *method, Boolean parentResolve)
{
    TypeClass *originalClass;
    TypeClass *targetClass;
    TypeClass *methodClass;
    SInt32 methodOffset;
    SInt32 parentIndex;
    ENode *methodRef;
    ENode *result;
    Object *resolveFunction;

    CError_ASSERT(2107, classPath != NULL);
    originalClass = (TypeClass *)classPath->type;
    if (classPath->next != NULL)
        classPath = classPath->next;
    targetClass = (TypeClass *)classPath->type;
    if (parentResolve) {
        parentIndex = 0;
        if (originalClass != targetClass) {
            ClassList *base;
            for (base = originalClass->bases; base != NULL; base = base->next) {
                parentIndex++;
                if (base->base == targetClass)
                    break;
            }
            if (base == NULL)
                CError_ReportError(ERR_SOM_CLASS_ACCESS_QUALIFICATION_ONLY_ALLOWED);
        }
        find_method_vtbl_class_and_offset(targetClass, method, &methodClass, &methodOffset);
        {
            ENode *classDataRef = create_objectrefnode(methodClass->sominfo->classDataObject);
            methodRef = makediadicnode(classDataRef, intconstnode((Type *)&stsignedlong, methodOffset), EADD);
            methodRef = makemonadicnode(methodRef, EINDIRECT);
        }
        methodRef->rtype = CDecl_NewPointerType(method->type);
        resolveFunction = fn_004e45b0("somParentNumResolve", "ppip");
        if (resolveFunction == NULL)
            return nullnode();
        result = funccallexpr(resolveFunction, create_objectrefnode(originalClass->sominfo->classDataObject),
                              intconstnode((Type *)&stsignedint, parentIndex), methodRef, NULL);
        result->rtype = methodRef->rtype;
        if (copts.SOMCheckEnvironment != 0 && methodClass->sominfo->omitEnvironmentParameter == 0)
            result->flags |= 0x10;
    } else {
        find_method_vtbl_class_and_offset(targetClass, method, &methodClass, &methodOffset);
        if (copts.SOMCallOptimization != 0 && fn_004e3cd0(method->type) != 0)
            return create_glue_objectrefnode(methodClass, methodOffset, method);
        {
            ENode *classDataRef = create_objectrefnode(methodClass->sominfo->classDataObject);
            result = makediadicnode(classDataRef, intconstnode((Type *)&stsignedlong, methodOffset), EADD);
            result = makemonadicnode(result, EINDIRECT);
        }
        result->rtype = CDecl_NewPointerType(method->type);
        if (copts.SOMCheckEnvironment != 0 && methodClass->sominfo->omitEnvironmentParameter == 0)
            result->flags |= 0x10;
    }
    return result;
}
