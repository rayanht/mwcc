#include "compiler/common.h"
#include "compiler/CScope.h"
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
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CSOM.h"
#include "compiler/CTemplateClass.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
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
#include "compiler/Objects.h"
#include "compiler/Types.h"
#include <string.h>

static struct TypeClass *class_path_base;
static struct HashNameNode *class_member_name;
static struct TypeClass *found_class;
static struct TemplClass *data_00580de4;
static UInt32 class_path_offset;
static UInt8 data_00580dec;
static SInt8 data_00580ded;

#undef CERROR_FILE
#define CERROR_FILE "CScope.c"
#undef CError_FATAL
#undef CError_ASSERT
#define CError_FATAL(line) CError_Internal("CScope.c", line)
#define CError_ASSERT(line, cond)                                                                                      \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            CError_Internal("CScope.c", line);                                                                         \
    } while (0)

#undef CERROR_FILE

/* The scope separator. The linker stripped this function; its literal stays in the unit's .data, last, as the unit
   generates its functions in reverse order. */
static char *CScope_ScopeSeparator(void)
{
    return "::";
}

static inline Boolean CScope_ResolveLookupContext(CScopeParseResult *result, Object *def)
{
    result->basePath = (BClassList *)CScope_GetClassAccessPath(result->basePath, (TypeClass *)def);
    if (result->basePath == NULL) {
        if (result->name != NULL)
            CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, result->name->name);
        else
            CError_ReportError(ERR_ILLEGAL_CLASS_MEMBER_ACCESS);
        return 0;
    }
    return 1;
}

#undef CERROR_FILE

#define CERROR_FILE "NameSpace.c"

static NameSpaceObjectList *Scope_Find(NameSpace *scope, HashNameNode *name)
{
    NameSpaceName *nn;

    if (!scope->is_hash)
        nn = (NameSpaceName *)scope->data.hash;
    else
        nn = scope->data.hash[name->hashval & 0x3ff];

    while (nn != NULL) {
        if (nn->name == name)
            return &nn->first;
        nn = nn->next;
    }
    return NULL;
}

/* NameSpace record as laid out in this build: the name table lives at 0x10
 * and the hash/list discriminator byte at 0x18. */

static NameSpaceObjectList *ScopeFindName(NameSpace *nspace, HashNameNode *name)
{
    NameSpaceName *np;

    if (!nspace->is_hash)
        np = nspace->data.list;
    else
        np = nspace->data.hash[name->hashval & 0x3FF];
    while (np) {
        if (np->name == name)
            return &np->first;
        np = np->next;
    }
    return NULL;
}

#undef CERROR_FILE

static ObjectList *CScope_CopyList(ObjectList *list)
{
    ObjectList *newlist;
    ObjectList *p;

    p = CompilerTools_AllocatePool(sizeof(ObjectList));
    newlist = p;
    for (;;) {
        p->object.value = list->object.value;
        list = list->next;
        if (list == NULL) {
            p->next = NULL;
            break;
        }
        p->next = CompilerTools_AllocatePool(sizeof(ObjectList));
        p = p->next;
    }
    return newlist;
}

static void CScope_NSIteratorInit(LookupCtx *iterator, NameSpace *nspace, CScopeParseResult *result)
{
    memclrw(result, 0x22);
    if (nspace->usings && !result->is_qualified) {
        iterator->namespaceCursor = NULL;
        iterator->scopeChain = build_namespace_scope_rec(nspace);
    } else {
        iterator->namespaceCursor = nspace;
        iterator->scopeChain = NULL;
    }
    iterator->lookupState = result;
}

static Boolean CScope_NSIteratorNext(LookupCtx *iterator)
{
    if (iterator->scopeChain)
        return (iterator->scopeChain = iterator->scopeChain->outer) != NULL;
    if ((iterator->namespaceCursor = iterator->namespaceCursor->parent)) {
        if (iterator->namespaceCursor->usings && !iterator->lookupState->is_qualified) {
            iterator->scopeChain = build_namespace_scope_rec(iterator->namespaceCursor);
            iterator->namespaceCursor = NULL;
        }
        return 1;
    }
    return 0;
}

static NameSpaceObjectList *CScope_NSIteratorFind(LookupCtx *iterator, HashNameNode *name)
{
    NameSpaceObjectList *list;

    if (iterator->scopeChain) {
        if (iterator->scopeChain->list)
            return CScope_0049a000(iterator->scopeChain, name, NULL);
        if (iterator->scopeChain->ns->theclass)
            return NULL;
        if ((list = CScope_FindName(iterator->scopeChain->ns, name)))
            return list;
    } else {
        if (iterator->namespaceCursor->theclass)
            return NULL;
        if ((list = CScope_FindName(iterator->namespaceCursor, name)))
            return list;
    }
    return NULL;
}

#undef CERROR_FILE

static Boolean NextNameSpace(LookupCtx *ctx)
{
    if (ctx->scopeChain != NULL) {
        ctx->scopeChain = ctx->scopeChain->outer;
        return ctx->scopeChain != NULL;
    }
    ctx->namespaceCursor = ctx->namespaceCursor->parent;
    if (ctx->namespaceCursor != NULL) {
        if (ctx->namespaceCursor->usings != NULL && !ctx->lookupState->is_qualified) {
            ctx->scopeChain = build_namespace_scope_rec(ctx->namespaceCursor);
            ctx->namespaceCursor = NULL;
        }
        return 1;
    }
    return 0;
}

#undef CERROR_FILE

static int lookup(struct NameSpace *a1, HashNameNode *a2)
{
    NameSpaceName *v5;
    if (a1->is_hash == 0) {
        v5 = a1->data.list;
    } else {
        v5 = (NameSpaceName *)((int *)a1->data.hash)[a2->hashval & 1023];
    }
    while ((int)v5 != 0) {
        if (v5->name == a2) {
            return (int)v5 + 8;
        }
        v5 = v5->next;
    }
    return 0;
}

static NameSpaceObjectList *FindInScope(NameSpace *scope, HashNameNode *name)
{
    NameSpaceName *e;
    NameSpace *s;

    s = scope;
    if (scope->is_hash == 0)
        e = s->data.list;
    else
        e = s->data.hash[name->hashval & 0x3ff];
    for (; e != NULL; e = e->next) {
        if (e->name == name)
            return &e->first;
    }
    return NULL;
}

static ObjectList *Scope_FindList(NameSpace *sc, HashNameNode *nm)
{
    NameSpaceName *nn;
    if (sc->is_hash == 0)
        nn = sc->data.list;
    else
        nn = sc->data.hash[nm->hashval & 0x3ff];
    while (nn != NULL) {
        if (nn->name == nm)
            return (ObjectList *)&nn->first;
        nn = nn->next;
    }
    return NULL;
}

#undef CERROR_FILE

static NameSpaceObjectList *ListSearch(NameSpace *scope, HashNameNode *key)
{
    NameSpaceName *n;

    if (scope->is_hash == 0)
        n = (NameSpaceName *)scope->data.hash;
    else
        n = ((NameSpaceName **)scope->data.hash)[key->hashval & 0x3ff];
    for (; n != NULL; n = n->next)
        if (n->name == key)
            return &n->first;
    return NULL;
}

#define OBJ(o) ((Object *)(o))

static inline NameSpaceObjectList *LookupInScope(HashNameNode *name, NameSpace *ns, NameSpaceObjectList **q, long *h)
{
    NameSpaceName *entry;
    if (!ns->is_hash)
        entry = ns->data.list;
    else {
        *h = name->hashval;
        *h &= 0x3ff;
        *h = (long)ns->data.hash[*h];
        entry = (NameSpaceName *)*h;
    }
    for (; entry; entry = entry->next)
        if (entry->name == name) {
            *q = &entry->first;
            return *q;
        }
    *q = NULL;
    return *q;
}

static inline NameSpaceObjectList *FindName(NameSpaceName **q, HashNameNode *name)
{
    goto test;
loop:
    if ((*q)->name == name)
        return &(*q)->first;
    *q = (*q)->next;
test:
    if (*q)
        goto loop;
    return NULL;
}

static inline NameSpaceObjectList *LookupInScope3(HashNameNode *name, NameSpace *ns)
{
    NameSpaceName *p;
    if (!ns->is_hash)
        p = ns->data.list;
    else
        p = ns->data.hash[name->hashval & 0x3ff];
    return FindName(&p, name);
}

static inline NameSpaceObjectList *LookupWrap(HashNameNode *name, NameSpace *ns, NameSpaceObjectList **q, long *h)
{
    return LookupInScope(name, ns, q, h);
}

static Boolean IsFunc(Object *o)
{
    Object *p = o;
    return p->otype == OT_OBJECT && p->type->type == TYPEFUNC;
}

static void AmbiguousError(NameSpace *scope, NameSpaceList *it, HashNameNode *name)
{
    NameSpace *ns = it->nspace;

    if (name != NULL && scope != ns)
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_NAME_FOUND, CError_GetQualifiedHashName(scope, name),
                           CError_GetQualifiedHashName(ns, name));
    else
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
}

static NameSpaceObjectList *CScope_FindMemberName(HashNameNode *name, NameSpace *nspace)
{
    NameSpaceName *entry;

    if (!nspace->is_hash)
        entry = nspace->data.list;
    else
        entry = nspace->data.hash[name->hashval & 1023];
    while (entry) {
        if (entry->name == name)
            return &entry->first;
        entry = entry->next;
    }
    return NULL;
}

static BClassList *CScope_NewPath(TypeClass *tclass, BClassList *next)
{
    BClassList *path = CompilerTools_AllocatePool(8);

    path->next = next;
    path->type = (Type *)tclass;
    return path;
}

static void CScope_AmbigNameError(NameSpace *nspace1, NameSpace *nspace2, HashNameNode *name)
{
    if (name && nspace1 != nspace2)
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_NAME_FOUND, CError_GetQualifiedHashName(nspace1, name),
                           CError_GetQualifiedHashName(nspace2, name));
    else
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
}

/* CScope_AmbigNameError for found_class's namespace, which is also left in *NSPACE1 (a local of the caller). */
static void CScope_AmbigFoundClassError(NameSpace **nspace1, NameSpace *nspace2, HashNameNode *name)
{
    *nspace1 = found_class->nspace;
    if (name && *nspace1 != nspace2)
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_NAME_FOUND, CError_GetQualifiedHashName(*nspace1, name),
                           CError_GetQualifiedHashName(nspace2, name));
    else
        CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
}

#undef CERROR_FILE

/* 0x55e480, filename string */
/* CError_Internal declared in the headers */

static inline ScopeRec *CScope_FindUsingScope(ScopeRec *scope, NameSpaceList *used)
{
    NameSpace *ancestor;
    for (; scope; scope = scope->outer) {
        for (ancestor = used->nspace; ancestor; ancestor = ancestor->parent)
            if (scope->ns == ancestor)
                return scope;
    }
    CError_FATAL(835);
    return scope;
}

static Boolean ScSeen(ScopeRec *base, NameSpaceList *item)
{
    NameSpace *ns = item->nspace;
    ScopeRec *n;
    NameSpaceList *it;

    for (n = base; n != NULL; n = n->outer) {
        if (n->ns == ns)
            return 1;
        for (it = n->list; it != NULL; it = it->next)
            if (it->nspace == ns)
                return 1;
    }
    return 0;
}

static void ScAdd(ScopeRec *node, NameSpaceList *item)
{
    NameSpaceList *it;

    it = CompilerTools_AllocatePool(sizeof(NameSpaceList));
    it->nspace = item->nspace;
    it->next = node->outer->list;
    node->outer->list = it;
}

static inline int CScope_0049b0e0_inline1(NameSpace *v2, HashNameNode *a1)
{
    NameSpaceName *v3;
    int v4;
    if (v2->is_hash == 0) {
        v3 = v2->data.list;
    } else {
        v3 = v2->data.hash[(int)a1->hashval & 1023];
    }
    while ((int)v3 != 0) {
        if (v3->name == a1) {
            return (int)v3 + 8;
        }
        v3 = v3->next;
    }
    return 0;
}

/* Enters FUNCTION's scope, saving the current one in SAVED. */
void CScope_SetFunctionScope(Object *function, CScopeSave *saved)
{
    saved->nspace = currentNameSpace;
    saved->theclass = data_00588040;
    saved->function = data_00588238;
    saved->member_context = data_005884f8;
    data_00588238 = function;
    data_00588040 = NULL;
    data_005884f8 = FALSE;
    if ((((TypeMemberFunc *)function->type)->flags & FUNC_METHOD) != 0) {
        data_00588040 = ((TypeMemberFunc *)function->type)->theclass;
        currentNameSpace = data_00588040->nspace;
        data_005884f8 = !((TypeMemberFunc *)function->type)->is_static;
    } else {
        currentNameSpace = function->nspace;
    }
}

/* Enters member function FUNCTION of THECLASS (static when IS_STATIC), saving the current scope in SAVE. */
void CScope_SetMethodScope(Object *cls, TypeClass *ns, unsigned char flag, CScopeSave *save)
{
    save->nspace = currentNameSpace;
    save->theclass = data_00588040;
    save->function = data_00588238;
    save->member_context = data_005884f8;
    data_00588238 = cls;
    data_00588040 = ns;
    currentNameSpace = ns->nspace;
    data_005884f8 = !flag;
}

void CScope_RestoreScope(CScopeSave *save)
{
    currentNameSpace = save->nspace;
    data_00588040 = save->theclass;
    data_00588238 = save->function;
    data_005884f8 = save->member_context;
}

/* 0x491250, one pointer arg, Boolean result */
/* 0x55e480, "CScopeParseResult.c" file name */

/* Global describing a hashed namespace / object table. Offsets verified from
 * the disassembly: bucket array pointer at 0x10, is_hash flag byte at 0x18. */

Boolean CScope_IsEmptySymTable(void)
{
    SInt32 i;
    NameSpaceObjectList *ol;
    NameSpaceName *nsn;

    if (!registration_context->is_hash)
        CError_FATAL(232);

    for (i = 0; i < 0x400; i++) {
        for (nsn = registration_context->data.hash[i]; nsn != NULL; nsn = nsn->next) {
            for (ol = &nsn->first; ol != NULL; ol = ol->next) {
                if (ol->object->otype != OT_OBJECT || !CParser_IsPublicRuntimeObject(ol->object))
                    return 0;
            }
        }
    }
    return 1;
}

UInt8 CScope_IsInLocalNameSpace(NameSpace *scope)
{
    if (scope != NULL) {
        do {
            if (!scope->is_global && !scope->is_templ) {
                return 1;
            }
            scope = scope->parent;
        } while (scope != NULL);
    }
    return 0;
}

#undef CERROR_FILE

NameSpaceObjectList *CScope_FindName(NameSpace *space, HashNameNode *name)
{
    NameSpaceName *entry;

    if (space->is_hash == 0) {
        entry = space->data.list;
    } else {
        entry = space->data.hash[name->hashval & 0x3ff];
    }

    while (entry != NULL) {
        if (entry->name == name) {
            return &entry->first;
        }
        entry = entry->next;
    }
    return NULL;
}

NameSpaceName *CScope_FindNameSpaceName(NameSpace *nameSpace, HashNameNode *name)
{
    NameSpaceName *node;
    if (nameSpace->is_hash == 0) {
        node = nameSpace->data.list;
    } else {
        node = nameSpace->data.hash[name->hashval & 0x3ff];
    }
    while (node != NULL) {
        if (node->name == name)
            return node;
        node = node->next;
    }
    return NULL;
}

#undef CERROR_FILE

NameSpaceObjectList *CScope_InsertNameSpaceName(NameSpace *nspace, HashNameNode *name)
{
    NameSpaceName *entry;
    if (nspace->is_global)
        entry = (NameSpaceName *)galloc(sizeof(NameSpaceName));
    else
        entry = (NameSpaceName *)CompilerTools_AllocatePool(sizeof(NameSpaceName));
    entry->name = name;
    entry->first.next = NULL;
    entry->first.object = NULL;
    if (nspace->is_hash) {
        NameSpaceName **bucket = &nspace->data.hash[name->hashval & 1023];
        entry->next = *bucket;
        *bucket = entry;
    } else {
        entry->next = nspace->data.list;
        nspace->data.list = entry;
    }
    nspace->names++;
    return &entry->first;
}

NameSpaceObjectList *CScope_InsertName(NameSpace *scope, HashNameNode *name)
{
    NameSpaceName *entry;
    NameSpace *target;
    NameSpaceName *tail;
    target = scope;
    if (target->is_hash != 0) {
        CError_FATAL(369);
    }
    if (target->is_global != 0) {
        entry = (NameSpaceName *)galloc(16U);
    } else {
        entry = (NameSpaceName *)CompilerTools_AllocatePool(16U);
    }
    entry->next = NULL;
    entry->name = name;
    entry->first.next = NULL;
    entry->first.object = NULL;
    if (target->data.list != NULL) {
        tail = target->data.list;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = entry;
    } else {
        target->data.list = entry;
    }
    target->names += 1U;
    return &entry->first;
}

NameSpaceList *fn_0049b300(NameSpaceList *list, NameSpace *nspace)
{
    NameSpaceList *n;
    ClassList *e;

    for (n = list; n != NULL; n = n->next) {
        if (n->nspace == nspace)
            return list;
    }

    n = (NameSpaceList *)CompilerTools_AllocatePool(8);
    n->next = list;
    n->nspace = nspace;
    list = n;

    if (nspace->theclass != NULL) {
        list = fn_0049b300(list, nspace->parent);
        for (e = nspace->theclass->bases; e != NULL; e = e->next) {
            list = fn_0049b300(list, e->base->nspace);
        }
    }
    return list;
}

NameSpaceList *collect_type_namespaces(NameSpaceList *acc, Type *type)
{
    FuncArg *arg;

    for (;;) {
        switch ((SInt8)type->type) {
            case TYPEPOINTER:
            case TYPEARRAY:
                type = ((TypePointer *)type)->target;
                continue;
            case TYPEENUM:
                acc = fn_0049b300(acc, ((TypeEnum *)type)->nspace);
                break;
            case TYPEFUNC:
                for (arg = ((TypeFunc *)type)->args; arg != NULL; arg = arg->next) {
                    if (arg->type != NULL)
                        acc = collect_type_namespaces(acc, arg->type);
                }
                type = ((TypeFunc *)type)->functype;
                continue;
            case TYPEMEMBERPOINTER:
                acc = collect_type_namespaces(acc, ((TypeMemberPointer *)type)->memberType);
                type = ((TypeMemberPointer *)type)->owner.type;
                if (type->type != TYPECLASS)
                    break;
                /* fall through */
            case TYPECLASS:
                acc = fn_0049b300(acc, ((TypeClass *)type)->nspace);
                break;
            case TYPEVOID:
            case TYPEINT:
            case TYPEFLOAT:
            case TYPESTRUCT:
            case TYPEBITFIELD:
            case TYPETEMPLATE:
            case TYPETEMPLDEPEXPR:
                break;
            default:
                CError_FATAL(476);
                break;
        }
        break;
    }
    return acc;
}

NameSpaceObjectList *CScope_ArgumentDependentNameLookup(NameSpaceObjectList *results, HashNameNode *name,
                                                        ENodeList *objects, char excludeMethods)
{
    NameSpaceList *namespaces;
    char matches;
    NameSpaceObjectList *found;
    ENodeList *entry;
    Object *candidate;
    Object *existing;
    NameSpaceObjectList *scan;
    NameSpaceObjectList *link;
    NameSpaceObjectList *member;
    NameSpaceList *scope;

    entry = objects;
    namespaces = NULL;
    if (entry != NULL) {
        do {
            namespaces = collect_type_namespaces(namespaces, entry->node->rtype);
            entry = entry->next;
        } while (entry != NULL);
    }

    for (scope = namespaces; scope != NULL; scope = scope->next) {
        found = (NameSpaceObjectList *)CScope_0049b0e0_inline1(scope->nspace, name);
        for (member = found; member != NULL; member = member->next) {
            if (member->object->otype == OT_OBJECT && ((Object *)member->object)->type->type == TYPEFUNC &&
                (excludeMethods == 0 || (((TypeFunc *)((Object *)member->object)->type)->flags & FUNC_METHOD) == 0)) {
                scan = results;
                while (scan != NULL) {
                    if (scan->object->otype == OT_OBJECT) {
                        existing = (Object *)member->object;
                        candidate = (Object *)scan->object;
                        if (candidate == existing) {
                            matches = 1;
                        } else {
                            matches = candidate->nspace == existing->nspace && candidate->name == existing->name &&
                                      iscpp_typeequal(candidate->type, existing->type) != 0;
                        }
                        if (matches != 0)
                            break;
                    }
                    scan = scan->next;
                }
                if (scan == NULL) {
                    link = (NameSpaceObjectList *)CompilerTools_AllocatePool(sizeof(NameSpaceObjectList));
                    link->object = member->object;
                    link->next = results;
                    results = link;
                }
            }
        }
    }
    return results;
}

NameSpace *CScope_NewHashNameSpace(HashNameNode *name)
{
    NameSpace *nspace;
    NameSpaceName **hash;
    hash = (NameSpaceName **)galloc(4096U);
    memclrw(hash, 4096U);
    nspace = (NameSpace *)galloc(sizeof(NameSpace));
    memclrw(nspace, sizeof(NameSpace));
    nspace->name = name;
    nspace->data.hash = hash;
    nspace->is_hash = 1;
    nspace->is_global = 1;
    return nspace;
}

NameSpace *CScope_NewListNameSpace(HashNameNode *name, Boolean is_global)
{
    NameSpace *ns;

    if (is_global) {
        ns = (NameSpace *)galloc(sizeof(NameSpace));
        memclrw(ns, sizeof(NameSpace));
    } else {
        ns = (NameSpace *)CompilerTools_AllocatePool(sizeof(NameSpace));
        memclrw(ns, sizeof(NameSpace));
    }
    ns->name = name;
    ns->is_hash = 0;
    ns->is_global = is_global;
    return ns;
}

NameSpace *CScope_FindNonClassNonTemplNameSpace(NameSpace *nspace)
{
    while (nspace != NULL) {
        if (nspace->theclass == NULL && nspace->is_templ == '\0') {
            return nspace;
        }
        nspace = nspace->parent;
    }
    return registration_context;
}

NameSpace *CScope_FindGlobalNS(NameSpace *scope)

{
    while (scope != NULL) {
        if (((scope->name != NULL) && (scope->theclass == NULL)) && (scope->is_global != 0)) {
            return scope;
        }
        scope = scope->parent;
    }
    return registration_context;
}

Boolean CScope_IsStdNameSpace(NameSpace *nspace)
{
    return nspace != NULL && nspace->is_global && nspace->parent == registration_context && nspace->name != NULL &&
           !strcmp(nspace->name->name, "std");
}

UInt8 CScope_IsEmptyNameSpace(NameSpace *nameSpace)
{
    if (nameSpace->is_hash != '\0') {
        CError_FATAL(646);
    }
    return nameSpace->data.list == NULL;
}

void CScope_MergeNameSpace(NameSpace *dest, NameSpace *source)
{
    NameSpaceName *last;
    if (dest->is_hash != 0 || source->is_hash != 0) {
        CError_FATAL(660);
    }
    if (dest->data.list != NULL) {
        last = dest->data.list;
        while (last->next != NULL)
            last = last->next;
        last->next = source->data.list;
    } else {
        dest->data.list = source->data.list;
    }
}

void CScope_AddObject(NameSpace *scope, HashNameNode *name, ObjBase *object)
{
    HashNameNode *lookupName;
    char kind;
    NameSpaceName **slot;
    NameSpaceObjectList *current;
    NameSpaceName *entry;
    NameSpaceObjectList *first;
    NameSpaceObjectList *newItem;
    Boolean isFunction;
    Boolean isExistingFunction;
    NameSpaceName *newEntry;

    if (!scope->is_hash)
        entry = scope->data.list;
    else
        entry = scope->data.hash[name->hashval & 1023];
    goto search;
nextEntry:
    if (entry->name != (lookupName = name))
        goto advance;
    first = &entry->first;
    goto found;
advance:
    entry = entry->next;
search:
    if (entry)
        goto nextEntry;
    first = NULL;
found:;
    if ((current = first) == NULL)
        goto addName;
    {
        newItem = scope->is_global ? (NameSpaceObjectList *)galloc(sizeof(NameSpaceObjectList))
                                   : (NameSpaceObjectList *)CompilerTools_AllocatePool(sizeof(NameSpaceObjectList));
        if ((kind = object->otype) == 3 || first->object->otype == OT_NAMESPACE) {
            CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
            return;
        }
        if (kind == 2) {
            for (;;) {
                if (current->object->otype == OT_TYPETAG) {
                    CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
                    return;
                }
                if (copts.cplusplus && current->object->otype == OT_TYPE &&
                    !iscpp_typeequal(((ObjType *)object)->type, ((ObjType *)current->object)->type)) {
                    CError_ReportError(ERR_TYPENAME_REDEFINED);
                    return;
                }
                if (!current->next) {
                    current->next = newItem;
                    newItem->next = NULL;
                    newItem->object = object;
                    return;
                }
                current = current->next;
            }
        }
        if (first->object->otype == OT_TYPETAG) {
            if (copts.cplusplus && kind == 1 &&
                !iscpp_typeequal(((ObjType *)object)->type, ((ObjType *)first->object)->type)) {
                CError_ReportError(ERR_TYPENAME_REDEFINED);
                return;
            }
            if (first->next)
                CError_FATAL(721);
            newItem->object = first->object;
            newItem->next = NULL;
            first->object = object;
            first->next = newItem;
            goto done;
        }
        if (!copts.cplusplus)
            goto illegalOverload;
        isFunction = kind == 5 && ((Object *)object)->type->type == TYPEFUNC;
        if (isFunction)
            goto functions;
    illegalOverload:
        CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
        return;
    functions:
        if ((kind = current->object->otype) != 2)
            goto checkFunction;
        current->next = (NameSpaceObjectList *)galloc(sizeof(NameSpaceObjectList));
        current->next->next = NULL;
        current->next->object = current->object;
        current->object = object;
        return;
    checkFunction:
        isExistingFunction = kind == 5 && ((Object *)current->object)->type->type == TYPEFUNC;
        if (isExistingFunction)
            goto appendFunction;
        CError_ReportError(ERR_ILLEGAL_NAME_OVERLOADING);
        return;
    appendFunction:
        if (current->next != NULL)
            goto nextFunction;
        current->next = (NameSpaceObjectList *)galloc(sizeof(NameSpaceObjectList));
        current->next->next = NULL;
        current->next->object = object;
        return;
    nextFunction:
        current = current->next;
        goto functions;
    }
addName:
    if (scope->theclass && (scope->theclass->flags & (CLASS_IS_TEMPL | CLASS_IS_TEMPL_INST))) {
        CScope_InsertName(scope, name)->object = object;
    } else {
        if (scope->is_global)
            newEntry = (NameSpaceName *)galloc(sizeof(NameSpaceName));
        else
            newEntry = (NameSpaceName *)CompilerTools_AllocatePool(sizeof(NameSpaceName));
        newEntry->name = name;
        newEntry->first.next = NULL;
        newEntry->first.object = NULL;
        if (scope->is_hash) {
            slot = &scope->data.hash[name->hashval & 1023];
            newEntry->next = *slot;
            *slot = newEntry;
        } else {
            newEntry->next = scope->data.list;
            scope->data.list = newEntry;
        }
        scope->names += 1;
        newEntry->first.object = object;
    }
done:
    return;
}

#undef CERROR_FILE

void CScope_AddGlobalObject(Object *object)
{
    object->nspace = registration_context;
    CScope_AddObject(registration_context, object->name, (ObjBase *)object);
}

struct ScopeRec *build_namespace_scope_rec(NameSpace *nspace)
{
    ScopeRec *rec;
    NameSpaceList head;
    NameSpaceList *scope;
    NameSpaceList *u;
    NameSpaceList *p;
    NameSpaceList *used;
    ScopeRec *r;

    rec = CompilerTools_AllocatePool(12);
    memclrw(rec, 12);
    rec->ns = nspace;
    if (nspace->parent)
        rec->outer = build_namespace_scope_rec(nspace->parent);
    if (nspace->usings) {
        head.next = NULL;
        head.nspace = nspace;
        for (scope = &head; scope; scope = scope->next) {
            for (u = scope->nspace->usings; u; u = u->next) {
                for (p = &head; p; p = p->next)
                    if (p->nspace == u->nspace)
                        break;
                if (!p) {
                    p = CompilerTools_AllocatePool(8);
                    p->nspace = u->nspace;
                    p->next = scope->next;
                    scope->next = p;
                }
            }
        }
        for (used = head.next; used; used = used->next) {
            r = CScope_FindUsingScope(rec, used);
            for (p = r->list; p; p = p->next)
                if (p->nspace == used->nspace)
                    break;
            if (!p) {
                p = CompilerTools_AllocatePool(8);
                p->nspace = used->nspace;
                p->next = r->list;
                r->list = p;
            }
        }
    }
    return rec;
}

ScopeRec *build_usings_scope_list(NameSpace *ns)
{
    ScopeRec *scope;
    ScopeRec root;
    NameSpaceList *namespaceEntry;
    NameSpaceList *usingEntry;

    root.ns = ns;
    root.outer = NULL;
    root.list = NULL;
    for (scope = &root; scope != NULL; scope = scope->outer) {
        if (scope->ns != NULL) {
            for (usingEntry = scope->ns->usings; usingEntry != NULL; usingEntry = usingEntry->next) {
                if (!ScSeen(&root, usingEntry)) {
                    if (scope->outer == NULL) {
                        scope->outer = CompilerTools_AllocatePool(sizeof(ScopeRec));
                        scope->outer->ns = NULL;
                        scope->outer->list = NULL;
                        scope->outer->outer = NULL;
                    }
                    ScAdd(scope, usingEntry);
                }
            }
        }
        for (namespaceEntry = scope->list; namespaceEntry != NULL; namespaceEntry = namespaceEntry->next) {
            for (usingEntry = namespaceEntry->nspace->usings; usingEntry != NULL; usingEntry = usingEntry->next) {
                if (!ScSeen(&root, usingEntry)) {
                    if (scope->outer == NULL) {
                        scope->outer = CompilerTools_AllocatePool(sizeof(ScopeRec));
                        scope->outer->ns = NULL;
                        scope->outer->list = NULL;
                        scope->outer->outer = NULL;
                    }
                    ScAdd(scope, usingEntry);
                }
            }
        }
    }
    return root.outer;
}

#undef CERROR_FILE

NameSpace *get_object_list_nspace(ObjectList *objects, Boolean *flag)
{
    Type *type;
    for (;;) {
        if (objects == NULL)
            return NULL;
        switch (objects->object.value->otype) {
            case OT_TYPE:
                type = ((ObjType *)objects->object.value)->type;
                break;
            case OT_TYPETAG:
                type = ((ObjType *)objects->object.value)->type;
                break;
            case OT_NAMESPACE:
                return ((ObjNameSpace *)objects->object.value)->nspace;
            default:
                CError_FATAL(1032);
                break;
            case OT_ENUMCONST:
            case OT_MEMBERVAR:
            case OT_OBJECT:
                objects = objects->next;
                continue;
        }
        if (type->type != TYPECLASS) {
            if (type->type == TYPETEMPLATE)
                return NULL;
            CError_ReportError(ERR_ILLEGAL_NAMESPACE);
            if (flag != NULL)
                *flag = 1;
            return NULL;
        }
        return TYPE_CLASS(type)->nspace;
    }
}

/* The path to the class (TCLASS or one of its bases, OFFSET into the object) that declares the member searched for
   (class_member_name, looked up as data_00580dec says); an ambiguous match is reported. */
BClassList *find_class_member_path(CScopeParseResult *result, TypeClass *tclass, SInt32 offset)
{
    Boolean fail;
    NameSpace *nspace;
    NameSpaceObjectList *list;
    ClassList *base;
    TypeClass *bestClass;
    BClassList *bestBase, *n;
    BClassList *candidate;
    SInt32 thisoffset;
    HashNameNode *name;
    NameSpace *left;
    NameSpace *ns;

    ns = tclass->nspace;
    name = class_member_name;
    if ((list = CScope_FindMemberName(name, ns))) {
        if (found_class) {
            if (CClass_ClassDominates(found_class, tclass))
                return NULL;
            if (CClass_ClassDominates(tclass, found_class))
                found_class = NULL;
        }
        switch (data_00580dec) {
            case 2:
                fail = 0;
                if ((nspace = get_object_list_nspace((ObjectList *)list, &fail))) {
                    if (found_class) {
                        if (found_class != tclass) {
                            CScope_AmbigFoundClassError(&left, tclass->nspace, class_member_name);
                            return NULL;
                        }
                        if (class_path_offset != offset)
                            data_00580ded = 1;
                        return NULL;
                    }
                    found_class = tclass;
                    class_path_offset = offset;
                    result->nspace = nspace;
                    return CScope_NewPath(tclass, NULL);
                }
                if (fail)
                    return NULL;
                break;
            case 0:
                if (found_class) {
                    if (list->object->otype == OT_TYPETAG && result->objects->object->otype == OT_TYPETAG &&
                        ((ObjType *)list->object)->type->type == TYPECLASS &&
                        ((ObjType *)result->objects->object)->type->type == TYPECLASS &&
                        (((TypeClass *)((ObjType *)list->object)->type)->flags & CLASS_IS_TEMPL_INST) &&
                        (((TypeClass *)((ObjType *)result->objects->object)->type)->flags & CLASS_IS_TEMPL_INST) &&
                        ((TemplClassInst *)((ObjType *)list->object)->type)->templ ==
                            ((TemplClassInst *)((ObjType *)result->objects->object)->type)->templ) {
                        data_00580de4 = ((TemplClassInst *)((ObjType *)result->objects->object)->type)->templ;
                    } else {
                        if (found_class != tclass) {
                            CScope_AmbigNameError(found_class->nspace, tclass->nspace, class_member_name);
                            return NULL;
                        }
                        if (class_path_offset != offset) {
                            data_00580ded = 1;
                            return NULL;
                        }
                    }
                }
                found_class = tclass;
                class_path_offset = offset;
                result->objects = list;
                return CScope_NewPath(tclass, NULL);
            case 1:
                for (; list; list = list->next) {
                    if (list->object->otype == OT_TYPETAG) {
                        if (found_class) {
                            if (found_class != tclass) {
                                CScope_AmbigNameError(found_class->nspace, tclass->nspace, class_member_name);
                                return NULL;
                            }
                            if (class_path_offset != offset)
                                data_00580ded = 1;
                            return NULL;
                        }
                        found_class = tclass;
                        class_path_offset = offset;
                        result->type.base = ((ObjType *)list->object)->type;
                        return CScope_NewPath(tclass, NULL);
                    }
                }
                break;
            default:
                CError_FATAL(1182);
        }
    }
    for (base = tclass->bases, bestBase = NULL; base; base = base->next) {
        thisoffset = base->is_virtual ? CClass_FindVBaseOffset(class_path_base, base->base) : offset + base->offset;
        if ((candidate = find_class_member_path(result, base->base, thisoffset))) {
            n = CompilerTools_AllocatePool(8);
            n->next = candidate;
            n->type = (Type *)tclass;
            if (bestBase && bestClass == found_class) {
                if (CClass_IsMoreAccessiblePath(n, bestBase))
                    bestBase = n;
            } else {
                bestClass = found_class;
                bestBase = n;
            }
        }
    }
    return bestBase;
}

Boolean find_and_append_class_member_path(CScopeParseResult *scope, NameSpace *target, HashNameNode *mode, Boolean flag)
{
    BClassList *node;
    BClassList *list;

    class_path_base = target->theclass;
    found_class = NULL;
    data_00580de4 = NULL;
    class_member_name = mode;
    data_00580dec = flag;
    data_00580ded = 0;
    node = find_class_member_path(scope, class_path_base, 0);
    if (node != NULL) {
        if (data_00580de4 != NULL)
            return 1;
        if (scope->basePath != NULL) {
            list = scope->basePath;
            while (list->next != NULL)
                list = list->next;
            if ((TypeClass *)list->type != class_path_base)
                list->next = node;
            else {
                node = node->next;
                list->next = node;
            }
        } else {
            scope->basePath = node;
        }
        if (data_00580ded != 0)
            scope->isambig = 1;
        return 1;
    }
    return 0;
}

BClassList *find_base_class_path(TypeClass *theclass, TypeClass *target, unsigned int offset)
{
    BClassList *node;
    SInt32 baseOffset;
    ClassList *base;
    BClassList *path;
    if (theclass == target) {
        if (found_class != NULL && class_path_offset != offset)
            CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
        node = (BClassList *)CompilerTools_AllocatePool(8);
        node->next = NULL;
        node->type = (Type *)theclass;
        found_class = theclass;
        class_path_offset = offset;
        return node;
    }
    path = NULL;
    for (base = theclass->bases; base; base = base->next) {
        if (base->is_virtual)
            baseOffset = CClass_FindVBaseOffset(class_path_base, base->base);
        else
            baseOffset = offset + base->offset;
        node = find_base_class_path(base->base, target, baseOffset);
        if (node)
            path = node;
    }
    if (path) {
        node = (BClassList *)CompilerTools_AllocatePool(8);
        node->next = path;
        node->type = (Type *)theclass;
        return node;
    }
    return NULL;
}

NameSpaceObjectList *CScope_0049a000(ScopeRec *context, HashNameNode *name, NameSpace **outscope)
{
    NameSpaceObjectList *matches;
    NameSpace *scope;
    NameSpaceObjectList *candidate;
    Boolean copied;
    NameSpaceList *namespaceEntry;
    NameSpaceObjectList *result;
    long index;

    if ((scope = context->ns) != NULL) {
        if ((matches = LookupWrap(name, scope, &matches, &index)) != NULL) {
            result = matches;
        } else {
            result = NULL;
            scope = NULL;
        }
    } else {
        result = NULL;
        scope = NULL;
    }
    (void)index;
    copied = 0;
    for (namespaceEntry = context->list; namespaceEntry != NULL; namespaceEntry = namespaceEntry->next) {
        if ((matches = LookupInScope3(name, namespaceEntry->nspace)) == NULL)
            continue;
        if (result != NULL) {
            NameSpaceObjectList *existing;

            for (candidate = matches; candidate != NULL; candidate = candidate->next) {
                for (existing = result; existing != NULL; existing = existing->next) {
                    if (existing->object == candidate->object)
                        break;
                    if (existing->object->otype != candidate->object->otype)
                        continue;
                    switch (existing->object->otype) {
                        case OT_TYPE:
                            if (((ObjType *)existing->object)->type != ((ObjType *)candidate->object)->type)
                                continue;
                            if (((ObjType *)existing->object)->qual == ((ObjType *)candidate->object)->qual)
                                break;
                            continue;
                        case OT_TYPETAG:
                            if (((ObjType *)existing->object)->type == ((ObjType *)candidate->object)->type)
                                break;
                            continue;
                        case OT_ENUMCONST:
                            if (((ObjEnumConst *)existing->object)->type == ((ObjEnumConst *)candidate->object)->type)
                                break;
                            continue;
                        case OT_OBJECT:
                            if (OBJ(existing->object)->type->type == TYPEFUNC)
                                continue;
                            if (OBJ(candidate->object)->type->type == TYPEFUNC)
                                continue;
                            if (OBJ(existing->object)->datatype == DALIAS) {
                                if (OBJ(candidate->object)->datatype == DALIAS) {
                                    if (OBJ(existing->object)->u.alias.object == OBJ(candidate->object)->u.alias.object)
                                        break;
                                } else {
                                    if (OBJ(existing->object)->u.alias.object == OBJ(candidate->object))
                                        break;
                                }
                            } else {
                                if (OBJ(candidate->object)->datatype == DALIAS &&
                                    OBJ(existing->object) == OBJ(candidate->object)->u.alias.object)
                                    break;
                            }
                            continue;
                        default:
                            continue;
                    }
                    break;
                }
                if (existing != NULL)
                    continue;
                if (!copied) {
                    NameSpaceObjectList *entry;
                    NameSpaceObjectList *tail;
                    NameSpaceObjectList *head;

                    for (entry = result; entry != NULL; entry = entry->next) {
                        Object *object = (Object *)entry->object;

                        if (IsFunc(object))
                            continue;
                        AmbiguousError(scope, namespaceEntry, name);
                    }
                    tail = (NameSpaceObjectList *)CompilerTools_AllocatePool(sizeof(NameSpaceObjectList));
                    head = tail;
                    for (;;) {
                        tail->object = result->object;
                        result = result->next;
                        if (result == NULL) {
                            tail->next = NULL;
                            break;
                        }
                        tail->next = (NameSpaceObjectList *)CompilerTools_AllocatePool(sizeof(NameSpaceObjectList));
                        tail = tail->next;
                    }
                    result = head;
                    copied = 1;
                }
                {
                    Object *object = (Object *)candidate->object;

                    if (IsFunc(object)) {
                        NameSpaceObjectList *newEntry =
                            (NameSpaceObjectList *)CompilerTools_AllocatePool(sizeof(NameSpaceObjectList));
                        newEntry->object = candidate->object;
                        newEntry->next = result;
                        result = newEntry;
                    } else {
                        AmbiguousError(scope, namespaceEntry, name);
                    }
                }
            }
        } else {
            result = matches;
            scope = namespaceEntry->nspace;
        }
    }
    if (outscope != NULL)
        *outscope = scope;
    return result;
}

NameSpaceObjectList *find_scope_object_list(LookupCtx *ctx, HashNameNode *key)
{
    NameSpaceObjectList *result;
    NameSpace *scope;
    ScopeRec *scopeRecord;

    if ((scopeRecord = ctx->scopeChain) != NULL) {
        NameSpace *namespace;
        if (scopeRecord->list != NULL)
            return CScope_0049a000(scopeRecord, key, NULL);
        if ((namespace = scopeRecord->ns)->theclass != NULL) {
            if (find_and_append_class_member_path(ctx->lookupState, namespace, key, 0) != 0) {
                result = ctx->lookupState->objects;
                ctx->lookupState->objects = NULL;
                return result;
            }
            return NULL;
        }
        if ((result = ListSearch(namespace, key)) != NULL)
            return result;
    } else {
        if ((scope = ctx->namespaceCursor)->theclass != NULL) {
            if (find_and_append_class_member_path(ctx->lookupState, scope, key, 0) != 0) {
                result = ctx->lookupState->objects;
                ctx->lookupState->objects = NULL;
                return result;
            }
            return NULL;
        }
        if ((result = ListSearch(scope, key)) != NULL)
            return result;
    }
    return NULL;
}

Boolean set_parse_result_from_objects(CScopeParseResult *result, NameSpaceObjectList *objects, HashNameNode *name)
{
    if (objects->next == NULL || objects->next->object->otype == OT_TYPETAG) {
        switch (objects->object->otype) {
            case OT_NAMESPACE:
                CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                return 0;
            case OT_TYPE:
                result->type.base = ((ObjType *)objects->object)->type;
                result->qualifiers = ((ObjType *)objects->object)->qual;
                result->object = objects->object;
                result->name = name;
                result->is_type = 1;
                break;
            case OT_TYPETAG:
                result->type.base = ((ObjType *)objects->object)->type;
                result->qualifiers = 0;
                result->object = objects->object;
                result->name = name;
                result->is_type = 1;
                break;
            default:
                result->object = objects->object;
        }
    } else {
        result->objects = objects;
    }
    return 1;
}

UInt8 CScope_FindQualifiedClassMember(CScopeParseResult *holder, TypeClass *type, HashNameNode *name)
{
    Boolean success;
    NameSpaceObjectList *objects;

    memclrw(holder, sizeof(*holder));
    CDecl_CompleteType((Type *)type);
    success = find_and_append_class_member_path(holder, type->nspace, name, 0);
    if (success) {
        if ((objects = holder->objects) == NULL) {
            CError_FATAL(1723);
        }
        holder->objects = NULL;
        success = set_parse_result_from_objects(holder, objects, name);
        if (success && holder->type.base == NULL) {
            return 1;
        }
        CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE, name->name);
    }
    return 0;
}

NameSpace *find_name_nspace(CScopeParseResult *result, NameSpace *nspace, HashNameNode *name)
{
    Boolean local;
    NameSpace *found;
    NameSpaceList *using;
    ScopeRec *scope;
    ObjectList *list;

    local = 0;
    if (nspace->theclass != NULL) {
        if (find_and_append_class_member_path(result, nspace, name, 2) != 0) {
            nspace = result->nspace;
            result->nspace = NULL;
            return nspace;
        }
        return NULL;
    }

    if ((list = Scope_FindList(nspace, name)) != NULL && get_object_list_nspace(list, &local) != NULL)
        return nspace;

    if (local == 0 && nspace->usings != NULL) {
        found = NULL;
        for (scope = build_usings_scope_list(nspace); scope != NULL; scope = scope->outer) {
            for (using = scope->list; using != NULL; using = using->next) {
                ObjectList *usingList;
                nspace = using->nspace;
                if ((usingList = Scope_FindList(nspace, name)) != NULL &&
                    (nspace = get_object_list_nspace(usingList, &local)) != NULL) {
                    if (found != NULL && nspace != found)
                        CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                    found = nspace;
                }
                if (local != 0)
                    return NULL;
            }
            if (found != NULL)
                return found;
        }
    }
    return NULL;
}

#undef CERROR_FILE

NameSpaceObjectList *find_namespace_object(CScopeParseResult *state, NameSpace *nspace, HashNameNode *name,
                                           NameSpace **foundSpace)
{
    NameSpaceObjectList *result;
    NameSpaceObjectList *lookupResult;
    ScopeRec *usingSpace;
    NameSpaceObjectList *usingResult;
    NameSpaceObjectList *classResult;
    if (nspace->theclass != NULL) {
        CDecl_CompleteType((Type *)nspace->theclass);
        if (find_and_append_class_member_path(state, nspace, name, 0) != 0) {
            classResult = state->objects;
            state->objects = NULL;
            return classResult;
        }
        return NULL;
    }
    lookupResult = (result = (NameSpaceObjectList *)lookup(nspace, name));
    if (lookupResult != NULL) {
        *foundSpace = nspace;
        return result;
    }
    if (nspace->usings != NULL) {
        usingSpace = build_usings_scope_list(nspace);
        while (usingSpace != NULL) {
            usingResult = CScope_0049a000(usingSpace, name, foundSpace);
            if (usingResult != NULL) {
                return usingResult;
            }
            usingSpace = usingSpace->outer;
        }
    }
    return NULL;
}

Boolean find_type_name_in_scope(CScopeParseResult *out, NameSpace *scope, HashNameNode *name)
{
    NameSpaceObjectList *s;
    NameSpaceObjectList *p;
    NameSpaceList *ol;
    ScopeRec *nl;
    NameSpace *obj;
    SInt32 offset;
    NameSpace *lastobj;

    if (scope->theclass != NULL) {
        CDecl_CompleteType((Type *)scope->theclass);
        if (find_and_append_class_member_path(out, scope, name, 1))
            return 1;
        return 0;
    }

    if ((p = FindInScope(scope, name)) != NULL) {
        while (p != NULL) {
            if (p->object->otype == OT_TYPETAG) {
                out->type.base = ((ObjType *)p->object)->type;
                return 1;
            }
            p = p->next;
        }
    }

    if (scope->usings != NULL) {
        offset = 0;
        for (nl = build_usings_scope_list(scope); nl != NULL; nl = nl->outer) {
            for (ol = nl->list; ol != NULL; ol = ol->next) {
                obj = ol->nspace;
                for (p = FindInScope((NameSpace *)(void *)obj, name); p != NULL; p = p->next) {
                    if (p->object->otype == OT_TYPETAG) {
                        if (offset != 0 && offset != (SInt32)((ObjType *)p->object)->type) {
                            if (!(name == NULL || obj == lastobj)) {
                                CError_ReportError(ERR_AMBIGUOUS_ACCESS_NAME_FOUND,
                                                   CError_GetQualifiedHashName(obj, name),
                                                   CError_GetQualifiedHashName(lastobj, name));
                            } else {
                                CError_ReportError(ERR_AMBIGUOUS_ACCESS_CLASS_STRUCT_UNION_MEMBER);
                            }
                        }
                        offset = (SInt32)((ObjType *)p->object)->type;
                        lastobj = ol->nspace;
                        break;
                    }
                }
            }
            if (offset != 0) {
                out->type.base = (Type *)offset;
                return 1;
            }
        }
    }
    return 0;
}

Type *CScope_GetType(NameSpace *nspace, HashNameNode *name, UInt32 *qual)
{
    NameSpaceObjectList *objects;
    Boolean more;
    LookupCtx ctx;
    CScopeParseResult state;

    memclrw(&state, sizeof(CScopeParseResult));
    if (nspace->usings != NULL && !state.is_qualified) {
        ctx.namespaceCursor = NULL;
        ctx.scopeChain = build_namespace_scope_rec(nspace);
    } else {
        ctx.namespaceCursor = nspace;
        ctx.scopeChain = NULL;
    }
    ctx.lookupState = &state;
    do {
        for (objects = find_scope_object_list(&ctx, name); objects != NULL; objects = objects->next) {
            if (objects->object->otype == OT_TYPETAG) {
                if (qual != NULL)
                    *qual = 0;
                return ((ObjType *)objects->object)->type;
            }
            if (objects->object->otype == OT_TYPE) {
                if (qual != NULL)
                    *qual = ((ObjType *)objects->object)->qual;
                return ((ObjType *)objects->object)->type;
            }
        }
        if (ctx.scopeChain != NULL) {
            ctx.scopeChain = ctx.scopeChain->outer;
            more = (ctx.scopeChain != NULL);
        } else {
            ctx.namespaceCursor = ctx.namespaceCursor->parent;
            if (ctx.namespaceCursor != NULL) {
                if (ctx.namespaceCursor->usings != NULL && !ctx.lookupState->is_qualified) {
                    ctx.scopeChain = build_namespace_scope_rec(ctx.namespaceCursor);
                    ctx.namespaceCursor = NULL;
                }
                more = 1;
            } else {
                more = 0;
            }
        }
    } while (more);
    return NULL;
}

Type *CScope_FindTagType(NameSpace *nspace, HashNameNode *name)
{
    NameSpaceObjectList *objects;
    LookupCtx ctx;
    CScopeParseResult state;

    memclrw(&state, sizeof(CScopeParseResult));
    if (nspace->usings != NULL && !state.is_qualified) {
        ctx.namespaceCursor = NULL;
        ctx.scopeChain = build_namespace_scope_rec(nspace);
    } else {
        ctx.namespaceCursor = nspace;
        ctx.scopeChain = NULL;
    }
    ctx.lookupState = &state;
    do {
        for (objects = find_scope_object_list(&ctx, name); objects != NULL; objects = objects->next) {
            if (objects->object->otype == OT_TYPETAG)
                return ((ObjType *)objects->object)->type;
        }
    } while (NextNameSpace(&ctx));
    return NULL;
}

#undef CERROR_FILE

Boolean parse_qualified_templdep_type(CScopeParseResult *context, Type *qualifier, Boolean allowToken328)
{
    TypeTemplDep *node;
    TypeTemplDep *qualifiedType;
    SInt16 token;
    SInt32 savedState;

    CPrep_GetBufferedTokenPosition(&savedState);
    CError_ASSERT(1967, CPrepTokenizer_GetNextToken() == 0x174);
    for (;;) {
        token = CPrepTokenizer_GetNextToken();
        if (token == 0x148 && allowToken328) {
            if (!CParser_00490660(NULL, 1))
                return 0;
            node = CDecl_NewTemplDepType(1);
            node->u.qual.type = (TypeTemplDep *)qualifier;
            node->u.qual.name = data_00587fa0;
            CPrep_SetBufferedTokenPosition(&savedState);
            CPrepTokenizer_GetNextToken();
            tk = CPrepTokenizer_GetNextToken();
            context->type.base = (Type *)node;
            return 1;
        } else if (token == -3) {
            node = CDecl_NewTemplDepType(1);
            node->u.qual.type = (TypeTemplDep *)qualifier;
            node->u.qual.name = data_00587fa0;
            tk = token;
            CPrep_GetBufferedTokenPosition(&savedState);
            token = CPrepTokenizer_GetNextToken();
            data_00587fa0 = node->u.qual.name;
            if (token == 0x174) {
                qualifier = (Type *)node;
                continue;
            }
            if (token == 0x3c) {
                tk = token;
                qualifiedType = node;
                node = CDecl_NewTemplDepType(4);
                node->u.qualtempl.type = qualifiedType;
                node->u.qualtempl.args = CTemplateNew_ParseTemplateArguments(
                    NULL, 1); /* CTemplateNew_ParseTemplateArguments returns the template argument list. */
                CPrep_GetBufferedTokenPosition(&savedState);
                token = CPrepTokenizer_GetNextToken();
                if (token == 0x174) {
                    qualifier = (Type *)node;
                    continue;
                }
            }
            CPrep_SetBufferedTokenPosition(&savedState);
            context->type.base = (Type *)node;
            return 1;
        } else {
            break;
        }
    }
    CPrep_SetBufferedTokenPosition(&savedState);
    context->type.base = qualifier;
    return 1;
}

Boolean parse_name_in_namespace(CScopeParseResult *scope, NameSpace *ns)
{
    Boolean isDestructor;
    HashNameNode *name;
    NameSpaceObjectList *objects;
    TemplClass *typeClass;

    isDestructor = 0;
    for (;;) {
        switch (tk) {
            case TK_IDENTIFIER:
                name = data_00587fa0;
                if (CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x174) {
                    tk = CPrepTokenizer_GetNextToken();
                    ns = find_name_nspace(scope, ns, name);
                    if (ns == NULL)
                        return 0;
                    scope->is_qualified = 1;
                    tk = CPrepTokenizer_GetNextToken();
                    continue;
                }
                break;
            case TK_OPERATOR:
                if (!CParser_00490660(NULL, 1))
                    return 0;
                CPrep_UngetToken();
                name = data_00587fa0;
                break;
            case TK_COMPL:
                if (ns->theclass != NULL) {
                    tk = CPrepTokenizer_GetNextToken();
                    if (tk == TK_IDENTIFIER) {
                        if (ns->theclass->classname == data_00587fa0 ||
                            CScope_GetType(currentNameSpace, data_00587fa0, NULL) == (Type *)ns->theclass) {
                            name = destructor_name;
                            isDestructor = 1;
                            break;
                        } else {
                            CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                            return 0;
                        }
                    }
                }
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return 0;
            default:
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return 0;
        }
        objects = find_namespace_object(scope, ns, name, &scope->nspace);
        if (objects == NULL || !set_parse_result_from_objects(scope, objects, name)) {
            if (isDestructor) {
                scope->is_destructor = 1;
                return 1;
            }
            if (ns->theclass != NULL && (ns->theclass->flags & CLASS_COMPLETED) == 0)
                CError_ReportError(ERR_ILLEGAL_USE_INCOMPLETE_STRUCT_UNION_CLASS, ns->theclass, 0);
            else
                CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
            return 0;
        }
        if (scope->type.base != NULL && scope->type.base->type == TYPECLASS &&
            CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x3c) {
            typeClass = (TemplClass *)scope->type.base;
            if (typeClass->theclass.flags & CLASS_IS_TEMPL_INST) {
                typeClass = ((TemplClassInst *)typeClass)->templ;
            } else if ((typeClass->theclass.flags & CLASS_IS_TEMPL) == 0) {
                return 1;
            }
            tk = CPrepTokenizer_GetNextToken();
            scope->type.base = CTemplTool_GetSelfRefTemplate(typeClass);
            if (scope->type.base->type == TYPECLASS && CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x174) {
                CPrepTokenizer_GetNextToken();
                tk = CPrepTokenizer_GetNextToken();
                scope->is_qualified = 1;
                ns = ((TypeClass *)scope->type.base)->nspace;
                scope->type.base = NULL;
                scope->object = NULL;
                continue;
            }
        }
        return 1;
    }
}

Boolean CScope_ParseExprName(CScopeParseResult *scope)
{
    Boolean moreScopes;
    LookupCtx cursor;
    NameSpace *base;
    HashNameNode *name;
    NameSpaceObjectList *entry;
    NameSpace *nspace;
    NameSpace *found;

    if (copts.cplusplus == 0) {
        memclrw(scope, sizeof(*scope));
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        name = data_00587fa0;
        base = currentNameSpace;
        if (base->usings != NULL && scope->is_qualified == 0) {
            cursor.namespaceCursor = NULL;
            cursor.scopeChain = build_namespace_scope_rec(base);
        } else {
            cursor.namespaceCursor = base;
            cursor.scopeChain = NULL;
        }
        cursor.lookupState = scope;
        do {
            entry = find_scope_object_list(&cursor, name);
            if (entry != NULL && entry->object->otype != OT_TYPETAG) {
                NameSpace *which;
                if (cursor.scopeChain != NULL)
                    which = cursor.scopeChain->ns;
                else
                    which = cursor.namespaceCursor;
                scope->nspace = which;
                return set_parse_result_from_objects(scope, entry, name);
            }
            if (cursor.scopeChain != NULL) {
                cursor.scopeChain = cursor.scopeChain->outer;
                moreScopes = (cursor.scopeChain != NULL);
            } else {
                cursor.namespaceCursor = cursor.namespaceCursor->parent;
                if (cursor.namespaceCursor != NULL) {
                    if (cursor.namespaceCursor->usings != NULL && cursor.lookupState->is_qualified == 0) {
                        cursor.scopeChain = build_namespace_scope_rec(cursor.namespaceCursor);
                        cursor.namespaceCursor = NULL;
                    }
                    moreScopes = 1;
                } else {
                    moreScopes = 0;
                }
            }
        } while (moreScopes);
        scope->nspace = currentNameSpace;
        scope->name = name;
        return 1;
    }

    if ((tk == TK_COLON_COLON || tk == TK_IDENTIFIER) && CScope_ParseQualifiedScope(scope, 1)) {
        if (scope->type.base != NULL)
            return 1;
        if (scope->nspace == NULL)
            CError_FATAL(2185);
    } else {
        memclrw(scope, sizeof(*scope));
        scope->nspace = currentNameSpace;
    }

    switch (tk) {
        case TK_IDENTIFIER:
            name = data_00587fa0;
            break;
        case TK_OPERATOR:
            if (!CParser_00490660(NULL, 1))
                return 0;
            name = data_00587fa0;
            CPrep_UngetToken();
            break;
        case TK_COMPL:
            if (scope->nspace->theclass != NULL) {
                tk = CPrepTokenizer_GetNextToken();
                if (tk == TK_IDENTIFIER) {
                    if (scope->nspace->theclass->classname == data_00587fa0 ||
                        CScope_GetType(currentNameSpace, data_00587fa0, NULL) == (Type *)scope->nspace->theclass) {
                        if (CClass_Destructor(scope->nspace->theclass) == NULL) {
                            scope->is_destructor = 1;
                            return 1;
                        }
                        name = destructor_name;
                        break;
                    }
                    CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
                    return 0;
                }
            }
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        default:
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
    }

    if (scope->is_qualified != 0) {
        NameSpaceObjectList *result = find_namespace_object(scope, scope->nspace, name, &scope->nspace);
        if (result == NULL) {
            char *diagnosticName = CError_GetQualifiedHashName(scope->nspace, name);
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, diagnosticName);
            return 0;
        }
        return set_parse_result_from_objects(scope, result, name);
    }

    nspace = scope->nspace;
    if (nspace->usings != NULL && scope->is_qualified == 0) {
        cursor.namespaceCursor = NULL;
        cursor.scopeChain = build_namespace_scope_rec(nspace);
    } else {
        cursor.namespaceCursor = nspace;
        cursor.scopeChain = NULL;
    }
    cursor.lookupState = scope;
    do {
        entry = find_scope_object_list(&cursor, name);
        if (entry != NULL) {
            if (cursor.scopeChain != NULL)
                found = cursor.scopeChain->ns;
            else
                found = cursor.namespaceCursor;
            scope->nspace = found;
            return set_parse_result_from_objects(scope, entry, name);
        }
        if (cursor.scopeChain != NULL) {
            cursor.scopeChain = cursor.scopeChain->outer;
            moreScopes = (cursor.scopeChain != NULL);
        } else {
            cursor.namespaceCursor = cursor.namespaceCursor->parent;
            if (cursor.namespaceCursor != NULL) {
                if (cursor.namespaceCursor->usings != NULL && cursor.lookupState->is_qualified == 0) {
                    cursor.scopeChain = build_namespace_scope_rec(cursor.namespaceCursor);
                    cursor.namespaceCursor = NULL;
                }
                moreScopes = 1;
            } else {
                moreScopes = 0;
            }
        }
    } while (moreScopes);

    if (scope->is_qualified != 0) {
        CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
        return 0;
    }
    scope->nspace = currentNameSpace;
    scope->name = name;
    return 1;
}

#undef CERROR_FILE

/* 0x490660, returns Boolean in al */
/* 0x4986e0, returns Boolean in al */
/* 0x5882d8, word accesses */
/* 0x58427a, byte accesses */
/* 0x5884f8, byte accesses */

Boolean CScope_ParseDeclName(CScopeParseResult *lookup)
{
    HashNameNode *name;
    LookupCtx iterator;
    NameSpace *scope;
    NameSpaceObjectList *result;
    Boolean more;
    if (copts.cplusplus == 0) {
    unqualified:
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        {
            NameSpace *currentScope;

            memclrw(lookup, sizeof(*lookup));
            name = data_00587fa0;
            currentScope = currentNameSpace;
            if (currentScope->usings != NULL && lookup->is_qualified == 0) {
                iterator.namespaceCursor = NULL;
                iterator.scopeChain = build_namespace_scope_rec(currentScope);
            } else {
                iterator.namespaceCursor = currentScope;
                iterator.scopeChain = NULL;
            }
            iterator.lookupState = lookup;
            do {
                result = find_scope_object_list(&iterator, name);
                if (result != NULL && (copts.cplusplus != 0 || result->object->otype != OT_TYPETAG)) {
                    lookup->nspace = iterator.scopeChain != NULL ? iterator.scopeChain->ns : iterator.namespaceCursor;
                    return set_parse_result_from_objects(lookup, result, name);
                }
                if (iterator.scopeChain != NULL) {
                    iterator.scopeChain = iterator.scopeChain->outer;
                    more = iterator.scopeChain != NULL;
                } else {
                    iterator.namespaceCursor = iterator.namespaceCursor->parent;
                    if (iterator.namespaceCursor != NULL) {
                        if (iterator.namespaceCursor->usings != NULL && iterator.lookupState->is_qualified == 0) {
                            iterator.scopeChain = build_namespace_scope_rec(iterator.namespaceCursor);
                            iterator.namespaceCursor = NULL;
                        }
                        more = 1;
                    } else {
                        more = 0;
                    }
                }
            } while (more);
            lookup->nspace = currentNameSpace;
            lookup->name = name;
            return 0;
        }
    }
    if (tk != TK_COLON_COLON && tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return 0;
    }
    if (!CScope_ParseQualifiedScope(lookup, 0))
        goto unqualified;
    if (lookup->type.base != NULL)
        return 1;
    if ((scope = lookup->nspace) == NULL)
        CError_FATAL(2305);
    switch (tk) {
        case TK_OPERATOR: {
            NameSpace *savedScope = currentNameSpace;
            TypeClass *savedObject = data_00588040;
            Object *savedOffset = data_00588238;
            UInt8 savedFlag = data_005884f8;

            currentNameSpace = scope;
            data_00588040 = scope->theclass;
            data_00588238 = NULL;
            data_005884f8 = 0;
            if (!CParser_00490660(NULL, 1)) {
                currentNameSpace = savedScope;
                data_00588040 = savedObject;
                data_00588238 = savedOffset;
                data_005884f8 = savedFlag;
                return 0;
            }
            currentNameSpace = savedScope;
            data_00588040 = savedObject;
            data_00588238 = savedOffset;
            data_005884f8 = savedFlag;
            tk = TK_IDENTIFIER;
            name = data_00587fa0;
            CPrep_UngetToken();
            break;
        }
        case TK_IDENTIFIER:
            name = data_00587fa0;
            if (scope->theclass != NULL && scope->theclass->classname == name)
                name = constructor_name;
            break;
        case TK_COMPL:
            if (scope->theclass == NULL) {
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                return 0;
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != TK_IDENTIFIER) {
                CError_ReportError(ERR_IDENTIFIER_EXPECTED);
                return 0;
            }
            if (scope->theclass->classname != data_00587fa0)
                CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
            name = destructor_name;
            break;
        default:
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
    }
    if (lookup->is_qualified != 0) {
        NameSpaceObjectList *objects = find_namespace_object(lookup, lookup->nspace, name, &lookup->nspace);
        if (objects == NULL) {
            CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
            return 0;
        }
        return set_parse_result_from_objects(lookup, objects, name);
    }
    if (scope->usings != NULL && lookup->is_qualified == 0) {
        iterator.namespaceCursor = NULL;
        iterator.scopeChain = build_namespace_scope_rec(scope);
    } else {
        iterator.namespaceCursor = scope;
        iterator.scopeChain = NULL;
    }
    iterator.lookupState = lookup;
    do {
        result = find_scope_object_list(&iterator, name);
        if (result != NULL) {
            lookup->nspace = iterator.scopeChain != NULL ? iterator.scopeChain->ns : iterator.namespaceCursor;
            return set_parse_result_from_objects(lookup, result, name);
        }
        if (iterator.scopeChain != NULL) {
            iterator.scopeChain = iterator.scopeChain->outer;
            more = iterator.scopeChain != NULL;
        } else {
            iterator.namespaceCursor = iterator.namespaceCursor->parent;
            if (iterator.namespaceCursor != NULL) {
                if (iterator.namespaceCursor->usings != NULL && iterator.lookupState->is_qualified == 0) {
                    iterator.scopeChain = build_namespace_scope_rec(iterator.namespaceCursor);
                    iterator.namespaceCursor = NULL;
                }
                more = 1;
            } else {
                more = 0;
            }
        }
    } while (more);
    CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
    return 0;
}

#define TCE(t) ((TemplClassInst *)(t))

Boolean CScope_ParseQualifiedScope(CScopeParseResult *result, SInt32 flag)
{
    Type *classType;
    HashNameNode *name;
    NameSpace *found;
    SInt32 tokenValue;
    LookupCtx iterator;
    SInt16 token;
    NameSpaceObjectList *objects;
    Type *objectType;
    Type *templateType;
    TemplClass *templateClass;
    Boolean hasNext;

    memclrw(result, sizeof(*result));
    found = NULL;
    if (tk == TK_COLON_COLON) {
        result->nspace = found = registration_context;
        result->is_qualified = 1;
        tk = CPrepTokenizer_GetNextToken();
    }
restart:
    if (tk != TK_IDENTIFIER)
        return found != NULL;
    name = data_00587fa0;
    token = CPrepTokenizer_GetNextTokenAndRestorePosition();
    data_00587fa0 = name;
    tokenValue = token;
    if (tokenValue != 0x174 && token != 0x3c)
        return found != NULL;
    {
        NameSpace *ns;
        if (found != NULL)
            ns = found;
        else
            ns = currentNameSpace;
        if (ns->usings != NULL && result->is_qualified == 0) {
            iterator.namespaceCursor = NULL;
            iterator.scopeChain = build_namespace_scope_rec(ns);
        } else {
            iterator.namespaceCursor = ns;
            iterator.scopeChain = NULL;
        }
    }
    iterator.lookupState = result;
    do {
        for (objects = find_scope_object_list(&iterator, name); objects != NULL; objects = objects->next) {
            if (objects->object->otype == OT_NAMESPACE) {
                if (found != NULL && found->theclass != NULL)
                    CError_FATAL(2423);
                result->nspace = found = ((ObjNameSpace *)objects->object)->nspace;
                tk = CPrepTokenizer_GetNextToken();
                if (tk != TK_COLON_COLON) {
                    CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                    return 0;
                }
                result->is_qualified = 1;
                tk = CPrepTokenizer_GetNextToken();
            } else if (objects->object->otype == OT_TYPETAG) {
                classType = ((ObjType *)objects->object)->type;
                if (classType->type != TYPECLASS) {
                    if (token == 0x3c) {
                        result->type.base = classType;
                        return 1;
                    }
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    return 0;
                }
                if (token == 0x3c) {
                    if (TCE(classType)->theclass.flags & CLASS_IS_TEMPL_INST) {
                        classType = (Type *)TCE(classType)->templ;
                    } else if ((TCE(classType)->theclass.flags & CLASS_IS_TEMPL) == 0) {
                        result->type.base = classType;
                        return 1;
                    }
                }
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '<') {
                    if ((TCE(classType)->theclass.flags & CLASS_IS_TEMPL) == 0)
                        CError_FATAL(2467);
                    templateClass = (TemplClass *)classType;
                    templateType = CTemplTool_GetSelfRefTemplate(templateClass);
                    if (templateType->type == TYPETEMPLATE) {
                        if (CPrepTokenizer_GetNextTokenAndRestorePosition() != 0x174) {
                            result->type.base = templateType;
                            return 1;
                        }
                        return parse_qualified_templdep_type(result, templateType, flag);
                    }
                    if (templateType->type != TYPECLASS)
                        return 0;
                    result->nspace = found = TCE(templateType)->theclass.nspace;
                    if (CPrepTokenizer_GetNextTokenAndRestorePosition() != 0x174) {
                        result->type.base = templateType;
                        return 1;
                    }
                    tk = CPrepTokenizer_GetNextToken();
                    CDecl_CompleteType(templateType);
                } else {
                    if (tk != TK_COLON_COLON)
                        CError_FATAL(2490);
                    if ((TCE(classType)->theclass.flags & CLASS_IS_TEMPL) == 0 ||
                        CParser_CheckTemplateClassScope(classType)) {
                        result->nspace = found = TCE(classType)->theclass.nspace;
                    }
                }
                result->is_qualified = 1;
                tk = CPrepTokenizer_GetNextToken();
            } else if (objects->object->otype == OT_TYPE) {
                objectType = ((ObjType *)objects->object)->type;
                if (objectType->type != TYPECLASS) {
                    if (tokenValue == 0x174 && objectType->type == TYPETEMPLATE) {
                        return parse_qualified_templdep_type(result, objectType, flag);
                    }
                    if (token == 0x3c) {
                        result->type.base = objectType;
                        return 1;
                    }
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    return 0;
                }
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '<') {
                    result->type.base = objectType;
                    return 1;
                }
                if (tk != TK_COLON_COLON)
                    CError_FATAL(2525);
                if (objectType->size == 0)
                    CDecl_CompleteType(objectType);
                result->nspace = found = TCE(objectType)->theclass.nspace;
                result->is_qualified = 1;
                tk = CPrepTokenizer_GetNextToken();
            } else {
                if (token == 0x3c)
                    return found != NULL;
                continue;
            }
            goto restart;
        }
        if (iterator.scopeChain != NULL) {
            iterator.scopeChain = iterator.scopeChain->outer;
            hasNext = (iterator.scopeChain != NULL);
        } else {
            iterator.namespaceCursor = iterator.namespaceCursor->parent;
            if (iterator.namespaceCursor != NULL) {
                if (iterator.namespaceCursor->usings != NULL && iterator.lookupState->is_qualified == 0) {
                    iterator.scopeChain = build_namespace_scope_rec(iterator.namespaceCursor);
                    iterator.namespaceCursor = NULL;
                }
                hasNext = 1;
            } else {
                hasNext = 0;
            }
        }
    } while (hasNext);
    CError_ReportError(ERR_UNDEFINED_IDENTIFIER, name->name);
    return 0;
}

Boolean CScope_ParseElaborateName(CScopeParseResult *result)
{
    HashNameNode *name;
    NameSpaceObjectList *objects;
    LookupCtx lookup;
    Boolean more;
    NameSpace *currentScope;
    NameSpace *parsedScope;

    if (copts.cplusplus == 0) {
        memclrw(result, sizeof(*result));
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        currentScope = currentNameSpace;
        name = data_00587fa0;
        if (currentScope->usings != NULL && result->is_qualified == 0) {
            lookup.namespaceCursor = NULL;
            lookup.scopeChain = build_namespace_scope_rec(currentScope);
        } else {
            lookup.namespaceCursor = currentScope;
            lookup.scopeChain = NULL;
        }
        lookup.lookupState = result;
        do {
            for (objects = find_scope_object_list(&lookup, name); objects != NULL; objects = objects->next) {
                if (objects->object->otype == OT_TYPETAG) {
                    result->nspace = lookup.scopeChain != NULL ? lookup.scopeChain->ns : lookup.namespaceCursor;
                    return set_parse_result_from_objects(result, objects, name);
                }
            }
            if (lookup.scopeChain != NULL) {
                lookup.scopeChain = lookup.scopeChain->outer;
                more = lookup.scopeChain != NULL;
            } else {
                lookup.namespaceCursor = lookup.namespaceCursor->parent;
                if (lookup.namespaceCursor != NULL) {
                    if (lookup.namespaceCursor->usings != NULL && lookup.lookupState->is_qualified == 0) {
                        lookup.scopeChain = build_namespace_scope_rec(lookup.namespaceCursor);
                        lookup.namespaceCursor = NULL;
                    }
                    more = 1;
                } else {
                    more = 0;
                }
            }
        } while (more);
        result->name = name;
        return 1;
    }
    if (tk != TK_COLON_COLON && tk != TK_IDENTIFIER) {
        CError_ReportError(ERR_IDENTIFIER_EXPECTED);
        return 0;
    }
    if (CScope_ParseQualifiedScope(result, 0) == 0) {
        result->nspace = currentNameSpace;
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        name = data_00587fa0;
    } else {
        if (result->type.base != NULL)
            return 1;
        if (result->nspace == NULL)
            CError_FATAL(2600);
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        name = data_00587fa0;
        if (result->is_qualified != 0) {
            if (result->nspace->theclass != NULL) {
                if (find_and_append_class_member_path(result, result->nspace, name, 1))
                    return 1;
                return 0;
            }
            return find_type_name_in_scope(result, result->nspace, name);
        }
    }
    parsedScope = result->nspace;
    if (parsedScope->usings != NULL && result->is_qualified == 0) {
        lookup.namespaceCursor = NULL;
        lookup.scopeChain = build_namespace_scope_rec(parsedScope);
    } else {
        lookup.namespaceCursor = parsedScope;
        lookup.scopeChain = NULL;
    }
    lookup.lookupState = result;
    do {
        for (objects = find_scope_object_list(&lookup, name); objects != NULL; objects = objects->next) {
            if (objects->object->otype == OT_TYPETAG || objects->object->otype == OT_TYPE) {
                result->nspace = lookup.scopeChain != NULL ? lookup.scopeChain->ns : lookup.namespaceCursor;
                return set_parse_result_from_objects(result, objects, name);
            }
        }
        if (lookup.scopeChain != NULL) {
            lookup.scopeChain = lookup.scopeChain->outer;
            more = lookup.scopeChain != NULL;
        } else {
            lookup.namespaceCursor = lookup.namespaceCursor->parent;
            if (lookup.namespaceCursor != NULL) {
                if (lookup.namespaceCursor->usings != NULL && lookup.lookupState->is_qualified == 0) {
                    lookup.scopeChain = build_namespace_scope_rec(lookup.namespaceCursor);
                    lookup.namespaceCursor = NULL;
                }
                more = 1;
            } else {
                more = 0;
            }
        }
    } while (more);
    result->name = name;
    return 1;
}

Boolean CScope_FindObject(NameSpace *nspace, CScopeParseResult *result, HashNameNode *name)
{
    NameSpaceObjectList *list;
    LookupCtx iterator;

    CScope_NSIteratorInit(&iterator, nspace, result);
    do {
        for (list = CScope_NSIteratorFind(&iterator, name); list; list = list->next) {
            if (copts.cplusplus || list->object->otype != OT_TYPETAG) {
                result->nspace = iterator.scopeChain ? iterator.scopeChain->ns : iterator.namespaceCursor;
                return set_parse_result_from_objects(result, list, name);
            }
        }
    } while (CScope_NSIteratorNext(&iterator));
    return 0;
}

#undef CERROR_FILE

#define CERROR_FILE "CScopeParseResult.c"

NameSpaceObjectList *CScope_FindObjectList(CScopeParseResult *result, HashNameNode *name)
{
    NameSpace *namespace;
    LookupCtx state;
    NameSpaceObjectList *entry;
    Boolean more;

    memclrw(result, sizeof(*result));
    namespace = (NameSpace *)currentNameSpace;
    if (namespace->usings != NULL && result->is_qualified == 0) {
        state.namespaceCursor = NULL;
        state.scopeChain = build_namespace_scope_rec(namespace);
    } else {
        state.namespaceCursor = namespace;
        state.scopeChain = NULL;
    }
    state.lookupState = result;
    do {
        for (entry = find_scope_object_list(&state, name); entry != NULL; entry = entry->next) {
            if (copts.cplusplus || entry->object->otype != OT_TYPETAG) {
                result->nspace = state.scopeChain ? state.scopeChain->ns : state.namespaceCursor;
                return entry;
            }
        }
        if (state.scopeChain != NULL) {
            state.scopeChain = state.scopeChain->outer;
            more = (state.scopeChain != NULL);
        } else {
            state.namespaceCursor = state.namespaceCursor->parent;
            if (state.namespaceCursor != NULL) {
                if (state.namespaceCursor->usings != NULL && state.lookupState->is_qualified == 0) {
                    state.scopeChain = build_namespace_scope_rec(state.namespaceCursor);
                    state.namespaceCursor = NULL;
                }
                more = 1;
            } else {
                more = 0;
            }
        }
    } while (more);
    return NULL;
}

#undef CERROR_FILE

Boolean CScope_PossibleTypeName(HashNameNode *name)
{
    Boolean more;
    LookupCtx lookup;
    CScopeParseResult result;
    NameSpace *ns;

    memclrw(&result, sizeof(result));
    ns = currentNameSpace;
    if (ns->usings != NULL && result.is_qualified == 0) {
        lookup.namespaceCursor = NULL;
        lookup.scopeChain = build_namespace_scope_rec(ns);
    } else {
        lookup.namespaceCursor = ns;
        lookup.scopeChain = NULL;
    }
    lookup.lookupState = &result;
    do {
        NameSpaceObjectList *objects = find_scope_object_list(&lookup, name);
        if (objects != NULL) {
            switch (objects->object->otype) {
                case OT_TYPE:
                case OT_NAMESPACE:
                    return 1;
                case OT_TYPETAG:
                    if (copts.cplusplus != 0)
                        return 1;
                    break;
                default:
                    return 0;
            }
        }
        if (lookup.scopeChain != NULL) {
            lookup.scopeChain = lookup.scopeChain->outer;
            more = (lookup.scopeChain != NULL);
        } else {
            lookup.namespaceCursor = lookup.namespaceCursor->parent;
            if (lookup.namespaceCursor != NULL) {
                if (lookup.namespaceCursor->usings != NULL && lookup.lookupState->is_qualified == 0) {
                    lookup.scopeChain = build_namespace_scope_rec(lookup.namespaceCursor);
                    lookup.namespaceCursor = NULL;
                }
                more = 1;
            } else {
                more = 0;
            }
        }
    } while (more);
    return 0;
}

Boolean CScope_FindClassMemberObject(TypeClass *tclass, CScopeParseResult *result, HashNameNode *name)
{
    NameSpaceObjectList *objects;
    Boolean success;

    memclrw(result, sizeof(*result));
    success = find_and_append_class_member_path(result, tclass->nspace, name, 0);
    if (success) {
        objects = result->objects;
        result->objects = NULL;
        if (objects != NULL && objects->object->otype == OT_OBJECT) {
            success = set_parse_result_from_objects(result, objects, name);
            return success;
        }
    }
    return 0;
}

#undef CERROR_FILE

int CScope_InitScopeSearch(ScopeSearch *save, NameSpace *obj)
{
    memclrw(save, sizeof(*save));
    save->owner = obj;
    if (save->owner->is_hash == 0)
        save->nextName = obj->data.list;
    else
        save->nextName = *obj->data.hash;
}

#undef CERROR_FILE

Object *CScope_NextObject(ScopeSearch *s)
{
    while (1) {
        if (s->nextObject != NULL) {
            do {
                ObjBase *obj = s->nextObject->object;
                if (obj->otype == OT_OBJECT) {
                    s->nextObject = s->nextObject->next;
                    return (Object *)obj;
                }
                s->nextObject = s->nextObject->next;
            } while (s->nextObject != NULL);
        }
        if (s->nextName != NULL) {
            s->nextObject = &s->nextName->first;
            s->nextName = s->nextName->next;
            continue;
        }
        if (s->owner->is_hash == 0 || ++s->bucketIndex >= 0x400)
            return NULL;
        s->nextName = s->owner->data.hash[s->bucketIndex];
    }
}

#undef CERROR_FILE

/* Hash-table owner/namespace record as laid out in this build: the hash
 * bucket array lives at 0x10 and the "is hashed" byte flag at 0x18. */

NameSpaceObjectList *CScope_NextNameSpaceObjectList(ScopeSearch *state)
{
    NameSpaceName *entry;

    for (;;) {
        if ((entry = state->nextName) != NULL) {
            state->nextName = entry->next;
            return &entry->first;
        }
        if (state->owner->is_hash == 0 || ++state->bucketIndex >= 0x400)
            return NULL;
        state->nextName = state->owner->data.hash[state->bucketIndex];
    }
}

void CScope_DefineTypeTag(NameSpace *ns, HashNameNode *name, Type *type)
{
    ObjType *tag = galloc(6);
    UInt8 access;
    memclrw(tag, 6);
    tag->otype = OT_TYPETAG;
    if (ns->theclass != NULL)
        access = member_access;
    else
        access = 0;
    tag->access = access;
    tag->type = type;
    CScope_AddObject(ns, name, (ObjBase *)tag);
}

Type *CScope_GetTagType(NameSpace *nspace, HashNameNode *name)
{
    NameSpaceObjectList *list;

    for (list = CScope_FindName(nspace, name); list; list = list->next) {
        if (list->object->otype == OT_TYPETAG)
            return ((ObjType *)list->object)->type;
    }
    return NULL;
}

#undef CERROR_FILE

Boolean CScope_FindTypeName(NameSpace *nspace, HashNameNode *name, CScopeParseResult *result)
{
    LookupCtx state;
    NameSpaceObjectList *item;
    Boolean found;

    memclrw(result, sizeof(*result));
    if (nspace->usings != NULL && result->is_qualified == 0) {
        state.namespaceCursor = NULL;
        state.scopeChain = build_namespace_scope_rec(nspace);
    } else {
        state.namespaceCursor = nspace;
        state.scopeChain = NULL;
    }
    state.lookupState = result;
    for (;;) {
        item = find_scope_object_list(&state, name);
        while (item != NULL) {
            switch (item->object->otype) {
                case 1:
                case 2:
                    return set_parse_result_from_objects(result, item, name);
            }
            return 0;
        }
        if (state.scopeChain != NULL) {
            state.scopeChain = state.scopeChain->outer;
            found = state.scopeChain != NULL;
        } else {
            state.namespaceCursor = state.namespaceCursor->parent;
            if (state.namespaceCursor != NULL) {
                if (state.namespaceCursor->usings != NULL && state.lookupState->is_qualified == 0) {
                    state.scopeChain = build_namespace_scope_rec(state.namespaceCursor);
                    state.namespaceCursor = NULL;
                }
                found = 1;
            } else {
                found = 0;
            }
        }
        if (!found)
            break;
    }
    return 0;
}

ObjectList *remove_dalias_objects(NameSpaceObjectList *list)
{
    ObjectList *l;
    ObjectList *newlist;
    ObjectList **pp;
    ObjectList *p;

    l = (ObjectList *)list;
    if (l != NULL) {
        do {
            if (l->object.value->otype == OT_OBJECT && l->object.value->datatype == DALIAS) {
                newlist = CScope_CopyList((ObjectList *)list);
                l = newlist;
                pp = &l;
                while ((p = *pp) != NULL) {
                    if (p->object.value->otype == OT_OBJECT && p->object.value->datatype == DALIAS)
                        *pp = p->next;
                    else
                        pp = &p->next;
                }
                return l;
            }
            l = l->next;
        } while (l != NULL);
    }
    return (ObjectList *)list;
}

ObjectList *CScope_FindObjectListInNameSpace(NameSpace *nspace, HashNameNode *name)
{
    NameSpaceObjectList *nol;
    Object *obj;

    if ((nol = ScopeFindName(nspace, name)) != NULL) {
        obj = (Object *)nol->object;
        switch (obj->otype) {
            case OT_OBJECT:
                return remove_dalias_objects(nol);
            case OT_TYPETAG:
                break;
            default:
                CError_ReportError(ERR_IDENTIFIER_REDECLARED, name->name);
                return NULL;
        }
    }
    return NULL;
}

#undef CERROR_FILE

BClassList *CScope_GetClassAccessPath(BClassList *classes, TypeClass *base)
{
    BClassList *current;
    BClassList *last;
    BClassList *start;
    ClassList *inherited;
    BClassList *entry;
    BClassList *result;
    BClassList *tail;
    TypeClass *type;
    BClassList *next;
    BClassList *path;

    if (classes == NULL)
        return NULL;
    current = classes;
    start = classes;
    for (;;) {
        if ((next = current->next) == NULL) {
            last = (BClassList *)(int)start;
            break;
        }
        for (inherited = TYPE_CLASS(current->type)->bases; inherited; inherited = inherited->next) {
            if (next->type == (Type *)inherited->base)
                break;
        }
        if (inherited == NULL)
            start = next;
        current = next;
    }
    entry = start;
    while (entry != NULL) {
        if (entry->type == (Type *)base)
            return entry;
        entry = entry->next;
    }
    type = TYPE_CLASS(start->type);
    class_path_base = base;
    found_class = NULL;
    if ((path = result = find_base_class_path(base, type, 0)) != NULL) {
        tail = result;
        while (tail != NULL) {
            if (tail->type == last->type) {
                tail->next = start->next;
                return result;
            }
            tail = tail->next;
        }
        CError_FATAL(3053);
    }
    return NULL;
}

#undef CERROR_FILE

Boolean CScope_ParseMemberName(TypeClass *ctx, CScopeParseResult *node, Boolean flag)
{
    Boolean result;
    if (tk == TK_COLON_COLON) {
    qualified_name:
        if (!CScope_ParseExprName(node))
            return 0;
        if (node->type.base != NULL && node->type.base->type == TYPETEMPLATE &&
            ((TypeTemplDep *)node->type.base)->kind == 1) {
            if (flag)
                return 1;
            CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE,
                               ((TypeTemplDep *)node->type.base)->u.qual.name->name);
            node->type.base = NULL;
            return 0;
        }
        if (node->is_destructor)
            return 1;
        node->basePath = CScope_GetClassAccessPath(node->basePath, ctx);
        if (node->basePath == NULL) {
            if (node->name != NULL)
                CError_ReportError(ERR_NOT_STRUCT_UNION_CLASS_MEMBER, node->name->name);
            else
                CError_ReportError(ERR_ILLEGAL_CLASS_MEMBER_ACCESS);
            result = 0;
        } else {
            result = 1;
        }
        return result;
    } else if (tk == TK_IDENTIFIER) {
        HashNameNode *savedName = data_00587fa0;
        SInt16 token = CPrepTokenizer_GetNextTokenAndRestorePosition();
        data_00587fa0 = savedName;
        do {
            switch (token) {
                case 0x174:
                    memclrw(node, sizeof(*node));
                    if (!find_and_append_class_member_path(node, ctx->nspace, savedName, 2))
                        break;
                    continue;
                case 0x3c:
                    if (flag)
                        break;
                    continue;
                default:
                    continue;
            }
            goto qualified_name;
        } while (0);
    }
    memclrw(node, sizeof(*node));
    result = parse_name_in_namespace(node, ctx->nspace);
    return result;
}

void add_using_declaration(BClassList *bases, NameSpace *scope, ObjBase *def, HashNameNode *name, char access)
{
    NameSpaceObjectList *lst;

    if (bases != NULL) {
        if (scope->theclass == NULL)
            CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
        else
            CClass_CheckBaseAccess(bases, def->access);
    }

    if (def->otype == OT_TYPE) {
        if (scope->theclass == NULL) {
            if ((lst = Scope_Find(scope, name)) != NULL) {
                if (lst->object->otype == OT_TYPE && ((ObjType *)def)->type == ((ObjType *)lst->object)->type &&
                    ((ObjType *)def)->qual == ((ObjType *)lst->object)->qual)
                    return;
            }
        }
        {
            ObjType *copy = (ObjType *)galloc(sizeof(ObjType));
            *copy = *(ObjType *)def;
            copy->access = access;
            CScope_AddObject(scope, name, (ObjBase *)copy);
        }
        return;
    }

    if (def->otype == OT_TYPETAG) {
        if (scope->theclass == NULL) {
            if ((lst = Scope_Find(scope, name)) != NULL) {
                if (lst->object->otype == OT_TYPETAG &&
                    ((ObjNameSpace *)def)->nspace == ((ObjNameSpace *)lst->object)->nspace)
                    return;
            }
        }
        {
            ObjNameSpace *copy = (ObjNameSpace *)galloc(sizeof(ObjNameSpace));
            *copy = *(ObjNameSpace *)def;
            copy->access = access;
            CScope_AddObject(scope, name, (ObjBase *)copy);
        }
        return;
    }

    if (def->otype == OT_ENUMCONST) {
        ObjEnumConst *copy = (ObjEnumConst *)galloc(sizeof(ObjEnumConst));
        *copy = *(ObjEnumConst *)def;
        copy->access = access;
        CScope_AddObject(scope, copy->name, (ObjBase *)copy);
        return;
    }

    if (def->otype == OT_MEMBERVAR) {
        if (scope->theclass != NULL) {
            ObjMemberVarPath *copy = galloc(sizeof(ObjMemberVarPath));
            *OBJ_MEMBER_VAR(copy) = *OBJ_MEMBER_VAR(def);
            copy->access = access;
            if ((bases = (BClassList *)CScope_GetClassAccessPath(CClass_GetPathCopy(bases, 1), scope->theclass)) !=
                    NULL &&
                bases->type == (Type *)scope->theclass) {
                copy->has_path = 1;
                copy->path = bases;
            } else {
                CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
            }
            CScope_AddObject(scope, copy->name, (ObjBase *)copy);
        } else {
            CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
        }
        return;
    }

    if (def->otype == OT_OBJECT) {
        if (scope->theclass == NULL) {
            for (lst = Scope_Find(scope, ((Object *)def)->name); lst != NULL; lst = lst->next) {
                if (lst->object->otype == OT_OBJECT) {
                    Object *target = (Object *)lst->object;
                    while (target->datatype == DALIAS)
                        target = target->u.alias.object;
                    if ((Object *)def == target)
                        return;
                }
            }
        }
        {
            Object *copy = (Object *)galloc(sizeof(Object));
            *copy = *(Object *)def;
            copy->access = access;
            copy->datatype = DALIAS;
            copy->u.alias.object = (Object *)def;
            copy->u.alias.member = NULL;
            copy->u.alias.offset = 0;
            if (((TypeMemberFunc *)copy->type)->type == TYPEFUNC &&
                (((TypeMemberFunc *)copy->type)->flags & FUNC_METHOD) && !((TypeMemberFunc *)copy->type)->is_static) {
                if (scope->theclass == NULL ||
                    (copy->u.alias.member = (BClassList *)CScope_GetClassAccessPath(CClass_GetPathCopy(bases, 1),
                                                                                    scope->theclass)) == NULL ||
                    copy->u.alias.member->type != (Type *)scope->theclass) {
                    CError_ReportError(ERR_ILLEGAL_USE_NON_STATIC_MEMBER);
                    copy->u.alias.member = NULL;
                }
            }
            CScope_AddObject(scope, copy->name, (ObjBase *)copy);
        }
        return;
    }

    CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
    return;
}

void CScope_AddClassUsingDeclaration(TypeClass *def, TypeClass *tp, HashNameNode *name, Boolean flag)
{
    CScopeParseResult result;
    Boolean found;
    NameSpaceObjectList *entry;

    memclrw(&result, sizeof(result));
    found = find_and_append_class_member_path(&result, tp->nspace, name, 0);
    if (!found || !CScope_ResolveLookupContext(&result, (Object *)def)) {
        CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE, name->name);
        return;
    }
    if (result.objects != NULL) {
        for (entry = result.objects; entry != NULL; entry = entry->next) {
            switch (entry->object->otype) {
                case OT_ENUMCONST:
                case OT_MEMBERVAR:
                case OT_OBJECT:
                    add_using_declaration(result.basePath, def->nspace, entry->object, result.name, flag);
                    break;
            }
        }
    } else if (result.object != NULL) {
        add_using_declaration(result.basePath, def->nspace, result.object, result.name, flag);
    } else {
        CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE, name->name);
    }
}

void CScope_ParseUsingDeclaration(NameSpace *nspace, AccessType flag, Boolean unused)
{
    Boolean isVirtual;
    Boolean consumedToken;
    CScopeParseResult info;

    if (nspace->theclass != NULL) {
        isVirtual = (nspace->theclass->flags & Q_VIRTUAL) != 0;
        consumedToken = 0;
        if (tk == TK_TYPENAME) {
            if (!isVirtual)
                CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
            consumedToken = 1;
            tk = CPrepTokenizer_GetNextToken();
        }
        if (!CScope_ParseMemberName(nspace->theclass, &info, isVirtual)) {
            CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
            return;
        }
        if (info.type.base != NULL && info.type.base->type == TYPETEMPLATE &&
            ((TypeTemplDep *)info.type.base)->kind == 1) {
            CError_ASSERT(3390, isVirtual);
            if (consumedToken) {
                ObjType *record = galloc(10);
                memclrw(record, 10);
                record->otype = OT_TYPE;
                record->access = flag;
                record->type = (Type *)info.type.base;
                CScope_AddObject(nspace, ((TypeTemplDep *)info.type.base)->u.qual.name, (ObjBase *)record);
            } else {
                CTemplateClass_AppendFuncDeclaration((TemplClass *)nspace->theclass, ((TypeTemplDep *)info.type.base),
                                                     flag);
            }
            tk = CPrepTokenizer_GetNextToken();
            if (tk != ';')
                CError_ReportError(ERR_SEMICOLON_EXPECTED);
            return;
        }
        if (info.is_qualified == 0) {
            CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
            return;
        }
    } else {
        NameSpace *savedNamespace = currentNameSpace;
        currentNameSpace = nspace;
        if (!CScope_ParseExprName(&info) || info.is_qualified == 0) {
            currentNameSpace = savedNamespace;
            CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
            return;
        }
        currentNameSpace = savedNamespace;
    }

    if (info.objects != NULL) {
        NameSpaceObjectList *node;
        for (node = info.objects; node != NULL; node = node->next) {
            if (node->object->otype == OT_OBJECT)
                add_using_declaration(info.basePath, nspace, node->object, info.name, flag);
        }
    } else if (info.object != NULL) {
        add_using_declaration(info.basePath, nspace, info.object, info.name, flag);
    } else {
        CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
    }
    tk = CPrepTokenizer_GetNextToken();
    if (tk != ';')
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
}

NameSpace *parse_namespace_name(NameSpace *nameSpace)
{
    CScopeParseResult lookupState;
    LookupCtx search;
    NameSpaceObjectList *result;
    ObjNameSpace *object;
    Boolean found;

    memclrw(&lookupState, sizeof(lookupState));
    if (tk == TK_COLON_COLON) {
        nameSpace = registration_context;
        lookupState.is_qualified = 1;
        tk = CPrepTokenizer_GetNextToken();
    }
    for (;;) {
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            break;
        }
        if (nameSpace->usings != NULL && lookupState.is_qualified == 0) {
            search.namespaceCursor = NULL;
            search.scopeChain = build_namespace_scope_rec(nameSpace);
        } else {
            search.namespaceCursor = nameSpace;
            search.scopeChain = NULL;
        }
        search.lookupState = &lookupState;
        do {
            result = find_scope_object_list(&search, data_00587fa0);
            if (result != NULL && result->object->otype == OT_NAMESPACE) {
                object = (ObjNameSpace *)result->object;
                nameSpace = object->nspace;
                break;
            }
            if (search.scopeChain != NULL) {
                search.scopeChain = search.scopeChain->outer;
                found = (search.scopeChain != NULL);
            } else {
                search.namespaceCursor = search.namespaceCursor->parent;
                if (search.namespaceCursor != NULL) {
                    if (search.namespaceCursor->usings != NULL && search.lookupState->is_qualified == 0) {
                        search.scopeChain = build_namespace_scope_rec(search.namespaceCursor);
                        search.namespaceCursor = NULL;
                    }
                    found = 1;
                } else {
                    found = 0;
                }
            }
            if (!found) {
                CError_ReportError(ERR_UNDEFINED_IDENTIFIER, data_00587fa0->name);
                break;
            }
        } while (1);
        tk = CPrepTokenizer_GetNextToken();
        if (tk != TK_COLON_COLON) {
            break;
        }
        lookupState.is_qualified = 1;
        tk = CPrepTokenizer_GetNextToken();
    }
    return nameSpace;
}

void CScope_ParseNameSpaceAlias(HashNameNode *name)
{
    NameSpaceObjectList *list;
    ObjNameSpace *objns;

    if (!(list = CScope_FindName(currentNameSpace, name))) {
        tk = CPrepTokenizer_GetNextToken();
        objns = (ObjNameSpace *)galloc(sizeof(ObjNameSpace));
        memclrw(objns, sizeof(ObjNameSpace));
        objns->otype = OT_NAMESPACE;
        objns->access = ACCESSPUBLIC;
        objns->nspace = parse_namespace_name(currentNameSpace);
        CScope_AddObject(currentNameSpace, name, (ObjBase *)objns);
    } else if (list->object->otype != OT_NAMESPACE) {
        CError_ReportError(ERR_ILLEGAL_NAMESPACE);
        tk = CPrepTokenizer_GetNextToken();
        (void)parse_namespace_name(currentNameSpace);
    } else {
        tk = CPrepTokenizer_GetNextToken();
        if (parse_namespace_name(currentNameSpace) != ((ObjNameSpace *)list->object)->nspace)
            CError_ReportError(ERR_IDENTIFIER_REDECLARED, name->name);
    }
    if (tk != ';')
        CError_ReportError(ERR_SEMICOLON_EXPECTED);
}

unsigned int CScope_ParseUsingDirective(NameSpace *container)
{
    NameSpaceList *entry;
    NameSpace *object;

    object = parse_namespace_name(container);
    if (object != container) {
        entry = container->usings;
        while (entry) {
            if (entry->nspace == object)
                break;
            entry = entry->next;
        }
        if (!entry) {
            entry = (NameSpaceList *)galloc(sizeof(NameSpaceList));
            entry->next = container->usings;
            entry->nspace = object;
            container->usings = entry;
        }
    } else {
        entry = CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
    }
    if (tk != ';')
        entry = CError_ReportError(ERR_SEMICOLON_EXPECTED);
    return (unsigned int)entry;
}

#undef CERROR_FILE
