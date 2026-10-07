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

static inline Boolean CScope_ResolveLookupContext(NameResult *result, Object *def)
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
        p->object = list->object;
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

static void CScope_NSIteratorInit(CScopeNSIterator *iterator, NameSpace *nspace, NameResult *result)
{
    memclrw(result, 0x22);
    if (nspace->usings && !result->is_qualified) {
        iterator->nspace = NULL;
        iterator->lookup = build_namespace_scope_rec(nspace);
    } else {
        iterator->nspace = nspace;
        iterator->lookup = NULL;
    }
    iterator->result = result;
}

static Boolean CScope_NSIteratorNext(CScopeNSIterator *iterator)
{
    if (iterator->lookup)
        return (iterator->lookup = iterator->lookup->next) != NULL;
    if ((iterator->nspace = iterator->nspace->parent)) {
        if (iterator->nspace->usings && !iterator->result->is_qualified) {
            iterator->lookup = build_namespace_scope_rec(iterator->nspace);
            iterator->nspace = NULL;
        }
        return 1;
    }
    return 0;
}

static NameSpaceObjectList *CScope_NSIteratorFind(CScopeNSIterator *iterator, HashNameNode *name)
{
    NameSpaceObjectList *list;

    if (iterator->lookup) {
        if (iterator->lookup->namespaces)
            return CScope_0049a000(iterator->lookup, name, NULL);
        if (iterator->lookup->nspace->theclass)
            return NULL;
        if ((list = CScope_FindName(iterator->lookup->nspace, name)))
            return list;
    } else {
        if (iterator->nspace->theclass)
            return NULL;
        if ((list = CScope_FindName(iterator->nspace, name)))
            return list;
    }
    return NULL;
}

#undef CERROR_FILE

static Boolean NextNameSpace(CScopeNSIterator *ctx)
{
    if (ctx->lookup != NULL) {
        ctx->lookup = ctx->lookup->next;
        return ctx->lookup != NULL;
    }
    ctx->nspace = ctx->nspace->parent;
    if (ctx->nspace != NULL) {
        if (ctx->nspace->usings != NULL && !ctx->result->is_qualified) {
            ctx->lookup = build_namespace_scope_rec(ctx->nspace);
            ctx->nspace = NULL;
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

static inline NameSpaceLookupList *CScope_FindUsingScope(NameSpaceLookupList *scope, NameSpaceList *used)
{
    NameSpace *ancestor;
    for (; scope; scope = scope->next) {
        for (ancestor = used->nspace; ancestor; ancestor = ancestor->parent)
            if (scope->nspace == ancestor)
                return scope;
    }
    CError_FATAL(835);
    return scope;
}

static Boolean ScSeen(NameSpaceLookupList *base, NameSpaceList *item)
{
    NameSpace *ns = item->nspace;
    NameSpaceLookupList *n;
    NameSpaceList *it;

    for (n = base; n != NULL; n = n->next) {
        if (n->nspace == ns)
            return 1;
        for (it = n->namespaces; it != NULL; it = it->next)
            if (it->nspace == ns)
                return 1;
    }
    return 0;
}

static void ScAdd(NameSpaceLookupList *node, NameSpaceList *item)
{
    NameSpaceList *it;

    it = CompilerTools_AllocatePool(sizeof(NameSpaceList));
    it->nspace = item->nspace;
    it->next = node->next->namespaces;
    node->next->namespaces = it;
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

void CScope_Setup(void)
{
    cscope_current = cscope_root = CScope_NewHashNameSpace(NULL);
    cscope_currentclass = NULL;
    cscope_currentfunc = NULL;
    cscope_is_member_func = 0;
}

void CScope_Cleanup(void)
{
}

void CScope_GetScope(CScopeSave *save)
{
    save->current = cscope_current;
    save->currentclass = cscope_currentclass;
    save->currentfunc = cscope_currentfunc;
    save->is_member_func = cscope_is_member_func;
}

void CScope_SetNameSpaceScope(NameSpace *nspace, CScopeSave *save)
{
    save->current = cscope_current;
    save->currentclass = cscope_currentclass;
    save->currentfunc = cscope_currentfunc;
    save->is_member_func = cscope_is_member_func;

    cscope_current = nspace;
    cscope_currentclass = nspace->theclass;
    cscope_currentfunc = NULL;
    cscope_is_member_func = 0;
}

void CScope_SetClassScope(TypeClass *cls, CScopeSave *save)
{
    save->current = cscope_current;
    save->currentclass = cscope_currentclass;
    save->currentfunc = cscope_currentfunc;
    save->is_member_func = cscope_is_member_func;

    cscope_current = cls->nspace;
    cscope_currentclass = cls;
    cscope_currentfunc = NULL;
    cscope_is_member_func = 0;
}

void CScope_SetClassDefScope(TypeClass *cls, CScopeSave *save)
{
    save->current = cscope_current;
    save->currentclass = cscope_currentclass;
    save->currentfunc = cscope_currentfunc;
    save->is_member_func = cscope_is_member_func;

    cscope_current = cls->nspace;
    cscope_currentclass = cls;
}

/* Enters FUNCTION's scope, saving the current one in SAVED. */
void CScope_SetFunctionScope(Object *function, CScopeSave *saved)
{
    saved->current = cscope_current;
    saved->currentclass = cscope_currentclass;
    saved->currentfunc = cscope_currentfunc;
    saved->is_member_func = cscope_is_member_func;
    cscope_currentfunc = function;
    cscope_currentclass = NULL;
    cscope_is_member_func = FALSE;
    if ((((TypeMemberFunc *)function->type)->flags & FUNC_METHOD) != 0) {
        cscope_currentclass = ((TypeMemberFunc *)function->type)->theclass;
        cscope_current = cscope_currentclass->nspace;
        cscope_is_member_func = !((TypeMemberFunc *)function->type)->is_static;
    } else {
        cscope_current = function->nspace;
    }
}

/* Enters member function FUNCTION of THECLASS (static when IS_STATIC), saving the current scope in SAVE. */
void CScope_SetMethodScope(Object *cls, TypeClass *ns, unsigned char flag, CScopeSave *save)
{
    save->current = cscope_current;
    save->currentclass = cscope_currentclass;
    save->currentfunc = cscope_currentfunc;
    save->is_member_func = cscope_is_member_func;
    cscope_currentfunc = cls;
    cscope_currentclass = ns;
    cscope_current = ns->nspace;
    cscope_is_member_func = !flag;
}

void CScope_RestoreScope(CScopeSave *save)
{
    cscope_current = save->current;
    cscope_currentclass = save->currentclass;
    cscope_currentfunc = save->currentfunc;
    cscope_is_member_func = save->is_member_func;
}

/* 0x491250, one pointer arg, Boolean result */
/* 0x55e480, "NameResult.c" file name */

/* Global describing a hashed namespace / object table. Offsets verified from
 * the disassembly: bucket array pointer at 0x10, is_hash flag byte at 0x18. */

Boolean CScope_IsEmptySymTable(void)
{
    SInt32 i;
    NameSpaceObjectList *ol;
    NameSpaceName *nsn;

    if (!cscope_root->is_hash)
        CError_FATAL(232);

    for (i = 0; i < 0x400; i++) {
        for (nsn = cscope_root->data.hash[i]; nsn != NULL; nsn = nsn->next) {
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
                acc = collect_type_namespaces(acc, ((TypeMemberPointer *)type)->ty1);
                type = ((TypeMemberPointer *)type)->ty2;
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
    return cscope_root;
}

NameSpace *CScope_FindGlobalNS(NameSpace *scope)

{
    while (scope != NULL) {
        if (((scope->name != NULL) && (scope->theclass == NULL)) && (scope->is_global != 0)) {
            return scope;
        }
        scope = scope->parent;
    }
    return cscope_root;
}

Boolean CScope_IsStdNameSpace(NameSpace *nspace)
{
    return nspace != NULL && nspace->is_global && nspace->parent == cscope_root && nspace->name != NULL &&
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
    object->nspace = cscope_root;
    CScope_AddObject(cscope_root, object->name, (ObjBase *)object);
}

struct NameSpaceLookupList *build_namespace_scope_rec(NameSpace *nspace)
{
    NameSpaceLookupList *rec;
    NameSpaceList head;
    NameSpaceList *scope;
    NameSpaceList *u;
    NameSpaceList *p;
    NameSpaceList *used;
    NameSpaceLookupList *r;

    rec = CompilerTools_AllocatePool(12);
    memclrw(rec, 12);
    rec->nspace = nspace;
    if (nspace->parent)
        rec->next = build_namespace_scope_rec(nspace->parent);
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
            for (p = r->namespaces; p; p = p->next)
                if (p->nspace == used->nspace)
                    break;
            if (!p) {
                p = CompilerTools_AllocatePool(8);
                p->nspace = used->nspace;
                p->next = r->namespaces;
                r->namespaces = p;
            }
        }
    }
    return rec;
}

NameSpaceLookupList *build_usings_scope_list(NameSpace *ns)
{
    NameSpaceLookupList *scope;
    NameSpaceLookupList root;
    NameSpaceList *namespaceEntry;
    NameSpaceList *usingEntry;

    root.nspace = ns;
    root.next = NULL;
    root.namespaces = NULL;
    for (scope = &root; scope != NULL; scope = scope->next) {
        if (scope->nspace != NULL) {
            for (usingEntry = scope->nspace->usings; usingEntry != NULL; usingEntry = usingEntry->next) {
                if (!ScSeen(&root, usingEntry)) {
                    if (scope->next == NULL) {
                        scope->next = CompilerTools_AllocatePool(sizeof(NameSpaceLookupList));
                        scope->next->nspace = NULL;
                        scope->next->namespaces = NULL;
                        scope->next->next = NULL;
                    }
                    ScAdd(scope, usingEntry);
                }
            }
        }
        for (namespaceEntry = scope->namespaces; namespaceEntry != NULL; namespaceEntry = namespaceEntry->next) {
            for (usingEntry = namespaceEntry->nspace->usings; usingEntry != NULL; usingEntry = usingEntry->next) {
                if (!ScSeen(&root, usingEntry)) {
                    if (scope->next == NULL) {
                        scope->next = CompilerTools_AllocatePool(sizeof(NameSpaceLookupList));
                        scope->next->nspace = NULL;
                        scope->next->namespaces = NULL;
                        scope->next->next = NULL;
                    }
                    ScAdd(scope, usingEntry);
                }
            }
        }
    }
    return root.next;
}

#undef CERROR_FILE

NameSpace *get_object_list_nspace(ObjectList *objects, Boolean *flag)
{
    Type *type;
    for (;;) {
        if (objects == NULL)
            return NULL;
        switch (objects->object->otype) {
            case OT_TYPE:
                type = ((ObjType *)objects->object)->type;
                break;
            case OT_TYPETAG:
                type = ((ObjType *)objects->object)->type;
                break;
            case OT_NAMESPACE:
                return ((ObjNameSpace *)objects->object)->nspace;
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
BClassList *find_class_member_path(NameResult *result, TypeClass *tclass, SInt32 offset)
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
                        result->type = ((ObjType *)list->object)->type;
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

Boolean find_and_append_class_member_path(NameResult *scope, NameSpace *target, HashNameNode *mode, Boolean flag)
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

NameSpaceObjectList *CScope_0049a000(NameSpaceLookupList *context, HashNameNode *name, NameSpace **outscope)
{
    NameSpaceObjectList *matches;
    NameSpace *scope;
    NameSpaceObjectList *candidate;
    Boolean copied;
    NameSpaceList *namespaceEntry;
    NameSpaceObjectList *result;
    long index;

    if ((scope = context->nspace) != NULL) {
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
    for (namespaceEntry = context->namespaces; namespaceEntry != NULL; namespaceEntry = namespaceEntry->next) {
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

NameSpaceObjectList *find_scope_object_list(CScopeNSIterator *ctx, HashNameNode *key)
{
    NameSpaceObjectList *result;
    NameSpace *scope;
    NameSpaceLookupList *scopeRecord;

    if ((scopeRecord = ctx->lookup) != NULL) {
        NameSpace *namespace;
        if (scopeRecord->namespaces != NULL)
            return CScope_0049a000(scopeRecord, key, NULL);
        if ((namespace = scopeRecord->nspace)->theclass != NULL) {
            if (find_and_append_class_member_path(ctx->result, namespace, key, 0) != 0) {
                result = ctx->result->objects;
                ctx->result->objects = NULL;
                return result;
            }
            return NULL;
        }
        if ((result = ListSearch(namespace, key)) != NULL)
            return result;
    } else {
        if ((scope = ctx->nspace)->theclass != NULL) {
            if (find_and_append_class_member_path(ctx->result, scope, key, 0) != 0) {
                result = ctx->result->objects;
                ctx->result->objects = NULL;
                return result;
            }
            return NULL;
        }
        if ((result = ListSearch(scope, key)) != NULL)
            return result;
    }
    return NULL;
}

Boolean set_parse_result_from_objects(NameResult *result, NameSpaceObjectList *objects, HashNameNode *name)
{
    if (objects->next == NULL || objects->next->object->otype == OT_TYPETAG) {
        switch (objects->object->otype) {
            case OT_NAMESPACE:
                CError_ReportError(ERR_ILLEGAL_USE_NAMESPACE_NAME);
                return 0;
            case OT_TYPE:
                result->type = ((ObjType *)objects->object)->type;
                result->qual = ((ObjType *)objects->object)->qual;
                result->object = objects->object;
                result->name = name;
                result->is_type = 1;
                break;
            case OT_TYPETAG:
                result->type = ((ObjType *)objects->object)->type;
                result->qual = 0;
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

UInt8 CScope_FindQualifiedClassMember(NameResult *holder, TypeClass *type, HashNameNode *name)
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
        if (success && holder->type == NULL) {
            return 1;
        }
        CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE, name->name);
    }
    return 0;
}

NameSpace *find_name_nspace(NameResult *result, NameSpace *nspace, HashNameNode *name)
{
    Boolean local;
    NameSpace *found;
    NameSpaceList *using;
    NameSpaceLookupList *scope;
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
        for (scope = build_usings_scope_list(nspace); scope != NULL; scope = scope->next) {
            for (using = scope->namespaces; using != NULL; using = using->next) {
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

NameSpaceObjectList *find_namespace_object(NameResult *state, NameSpace *nspace, HashNameNode *name,
                                           NameSpace **foundSpace)
{
    NameSpaceObjectList *result;
    NameSpaceObjectList *lookupResult;
    NameSpaceLookupList *usingSpace;
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
            usingSpace = usingSpace->next;
        }
    }
    return NULL;
}

Boolean find_type_name_in_scope(NameResult *out, NameSpace *scope, HashNameNode *name)
{
    NameSpaceObjectList *s;
    NameSpaceObjectList *p;
    NameSpaceList *ol;
    NameSpaceLookupList *nl;
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
                out->type = ((ObjType *)p->object)->type;
                return 1;
            }
            p = p->next;
        }
    }

    if (scope->usings != NULL) {
        offset = 0;
        for (nl = build_usings_scope_list(scope); nl != NULL; nl = nl->next) {
            for (ol = nl->namespaces; ol != NULL; ol = ol->next) {
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
                out->type = (Type *)offset;
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
    CScopeNSIterator ctx;
    NameResult state;

    memclrw(&state, sizeof(NameResult));
    if (nspace->usings != NULL && !state.is_qualified) {
        ctx.nspace = NULL;
        ctx.lookup = build_namespace_scope_rec(nspace);
    } else {
        ctx.nspace = nspace;
        ctx.lookup = NULL;
    }
    ctx.result = &state;
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
        if (ctx.lookup != NULL) {
            ctx.lookup = ctx.lookup->next;
            more = (ctx.lookup != NULL);
        } else {
            ctx.nspace = ctx.nspace->parent;
            if (ctx.nspace != NULL) {
                if (ctx.nspace->usings != NULL && !ctx.result->is_qualified) {
                    ctx.lookup = build_namespace_scope_rec(ctx.nspace);
                    ctx.nspace = NULL;
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
    CScopeNSIterator ctx;
    NameResult state;

    memclrw(&state, sizeof(NameResult));
    if (nspace->usings != NULL && !state.is_qualified) {
        ctx.nspace = NULL;
        ctx.lookup = build_namespace_scope_rec(nspace);
    } else {
        ctx.nspace = nspace;
        ctx.lookup = NULL;
    }
    ctx.result = &state;
    do {
        for (objects = find_scope_object_list(&ctx, name); objects != NULL; objects = objects->next) {
            if (objects->object->otype == OT_TYPETAG)
                return ((ObjType *)objects->object)->type;
        }
    } while (NextNameSpace(&ctx));
    return NULL;
}

#undef CERROR_FILE

Boolean parse_qualified_templdep_type(NameResult *context, Type *qualifier, Boolean allowToken328)
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
            context->type = (Type *)node;
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
            context->type = (Type *)node;
            return 1;
        } else {
            break;
        }
    }
    CPrep_SetBufferedTokenPosition(&savedState);
    context->type = qualifier;
    return 1;
}

Boolean parse_name_in_namespace(NameResult *scope, NameSpace *ns)
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
                            CScope_GetType(cscope_current, data_00587fa0, NULL) == (Type *)ns->theclass) {
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
        if (scope->type != NULL && scope->type->type == TYPECLASS &&
            CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x3c) {
            typeClass = (TemplClass *)scope->type;
            if (typeClass->theclass.flags & CLASS_IS_TEMPL_INST) {
                typeClass = ((TemplClassInst *)typeClass)->templ;
            } else if ((typeClass->theclass.flags & CLASS_IS_TEMPL) == 0) {
                return 1;
            }
            tk = CPrepTokenizer_GetNextToken();
            scope->type = CTemplTool_GetSelfRefTemplate(typeClass);
            if (scope->type->type == TYPECLASS && CPrepTokenizer_GetNextTokenAndRestorePosition() == 0x174) {
                CPrepTokenizer_GetNextToken();
                tk = CPrepTokenizer_GetNextToken();
                scope->is_qualified = 1;
                ns = ((TypeClass *)scope->type)->nspace;
                scope->type = NULL;
                scope->object = NULL;
                continue;
            }
        }
        return 1;
    }
}

Boolean CScope_ParseExprName(NameResult *scope)
{
    Boolean moreScopes;
    CScopeNSIterator cursor;
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
        base = cscope_current;
        if (base->usings != NULL && scope->is_qualified == 0) {
            cursor.nspace = NULL;
            cursor.lookup = build_namespace_scope_rec(base);
        } else {
            cursor.nspace = base;
            cursor.lookup = NULL;
        }
        cursor.result = scope;
        do {
            entry = find_scope_object_list(&cursor, name);
            if (entry != NULL && entry->object->otype != OT_TYPETAG) {
                NameSpace *which;
                if (cursor.lookup != NULL)
                    which = cursor.lookup->nspace;
                else
                    which = cursor.nspace;
                scope->nspace = which;
                return set_parse_result_from_objects(scope, entry, name);
            }
            if (cursor.lookup != NULL) {
                cursor.lookup = cursor.lookup->next;
                moreScopes = (cursor.lookup != NULL);
            } else {
                cursor.nspace = cursor.nspace->parent;
                if (cursor.nspace != NULL) {
                    if (cursor.nspace->usings != NULL && cursor.result->is_qualified == 0) {
                        cursor.lookup = build_namespace_scope_rec(cursor.nspace);
                        cursor.nspace = NULL;
                    }
                    moreScopes = 1;
                } else {
                    moreScopes = 0;
                }
            }
        } while (moreScopes);
        scope->nspace = cscope_current;
        scope->name = name;
        return 1;
    }

    if ((tk == TK_COLON_COLON || tk == TK_IDENTIFIER) && CScope_ParseQualifiedScope(scope, 1)) {
        if (scope->type != NULL)
            return 1;
        if (scope->nspace == NULL)
            CError_FATAL(2185);
    } else {
        memclrw(scope, sizeof(*scope));
        scope->nspace = cscope_current;
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
                        CScope_GetType(cscope_current, data_00587fa0, NULL) == (Type *)scope->nspace->theclass) {
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
        cursor.nspace = NULL;
        cursor.lookup = build_namespace_scope_rec(nspace);
    } else {
        cursor.nspace = nspace;
        cursor.lookup = NULL;
    }
    cursor.result = scope;
    do {
        entry = find_scope_object_list(&cursor, name);
        if (entry != NULL) {
            if (cursor.lookup != NULL)
                found = cursor.lookup->nspace;
            else
                found = cursor.nspace;
            scope->nspace = found;
            return set_parse_result_from_objects(scope, entry, name);
        }
        if (cursor.lookup != NULL) {
            cursor.lookup = cursor.lookup->next;
            moreScopes = (cursor.lookup != NULL);
        } else {
            cursor.nspace = cursor.nspace->parent;
            if (cursor.nspace != NULL) {
                if (cursor.nspace->usings != NULL && cursor.result->is_qualified == 0) {
                    cursor.lookup = build_namespace_scope_rec(cursor.nspace);
                    cursor.nspace = NULL;
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
    scope->nspace = cscope_current;
    scope->name = name;
    return 1;
}

#undef CERROR_FILE

/* 0x490660, returns Boolean in al */
/* 0x4986e0, returns Boolean in al */
/* 0x5882d8, word accesses */
/* 0x58427a, byte accesses */
/* 0x5884f8, byte accesses */

Boolean CScope_ParseDeclName(NameResult *lookup)
{
    HashNameNode *name;
    CScopeNSIterator iterator;
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
            currentScope = cscope_current;
            if (currentScope->usings != NULL && lookup->is_qualified == 0) {
                iterator.nspace = NULL;
                iterator.lookup = build_namespace_scope_rec(currentScope);
            } else {
                iterator.nspace = currentScope;
                iterator.lookup = NULL;
            }
            iterator.result = lookup;
            do {
                result = find_scope_object_list(&iterator, name);
                if (result != NULL && (copts.cplusplus != 0 || result->object->otype != OT_TYPETAG)) {
                    lookup->nspace = iterator.lookup != NULL ? iterator.lookup->nspace : iterator.nspace;
                    return set_parse_result_from_objects(lookup, result, name);
                }
                if (iterator.lookup != NULL) {
                    iterator.lookup = iterator.lookup->next;
                    more = iterator.lookup != NULL;
                } else {
                    iterator.nspace = iterator.nspace->parent;
                    if (iterator.nspace != NULL) {
                        if (iterator.nspace->usings != NULL && iterator.result->is_qualified == 0) {
                            iterator.lookup = build_namespace_scope_rec(iterator.nspace);
                            iterator.nspace = NULL;
                        }
                        more = 1;
                    } else {
                        more = 0;
                    }
                }
            } while (more);
            lookup->nspace = cscope_current;
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
    if (lookup->type != NULL)
        return 1;
    if ((scope = lookup->nspace) == NULL)
        CError_FATAL(2305);
    switch (tk) {
        case TK_OPERATOR: {
            NameSpace *savedScope = cscope_current;
            TypeClass *savedObject = cscope_currentclass;
            Object *savedOffset = cscope_currentfunc;
            UInt8 savedFlag = cscope_is_member_func;

            cscope_current = scope;
            cscope_currentclass = scope->theclass;
            cscope_currentfunc = NULL;
            cscope_is_member_func = 0;
            if (!CParser_00490660(NULL, 1)) {
                cscope_current = savedScope;
                cscope_currentclass = savedObject;
                cscope_currentfunc = savedOffset;
                cscope_is_member_func = savedFlag;
                return 0;
            }
            cscope_current = savedScope;
            cscope_currentclass = savedObject;
            cscope_currentfunc = savedOffset;
            cscope_is_member_func = savedFlag;
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
        iterator.nspace = NULL;
        iterator.lookup = build_namespace_scope_rec(scope);
    } else {
        iterator.nspace = scope;
        iterator.lookup = NULL;
    }
    iterator.result = lookup;
    do {
        result = find_scope_object_list(&iterator, name);
        if (result != NULL) {
            lookup->nspace = iterator.lookup != NULL ? iterator.lookup->nspace : iterator.nspace;
            return set_parse_result_from_objects(lookup, result, name);
        }
        if (iterator.lookup != NULL) {
            iterator.lookup = iterator.lookup->next;
            more = iterator.lookup != NULL;
        } else {
            iterator.nspace = iterator.nspace->parent;
            if (iterator.nspace != NULL) {
                if (iterator.nspace->usings != NULL && iterator.result->is_qualified == 0) {
                    iterator.lookup = build_namespace_scope_rec(iterator.nspace);
                    iterator.nspace = NULL;
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

Boolean CScope_ParseQualifiedScope(NameResult *result, SInt32 flag)
{
    Type *classType;
    HashNameNode *name;
    NameSpace *found;
    SInt32 tokenValue;
    CScopeNSIterator iterator;
    SInt16 token;
    NameSpaceObjectList *objects;
    Type *objectType;
    Type *templateType;
    TemplClass *templateClass;
    Boolean hasNext;

    memclrw(result, sizeof(*result));
    found = NULL;
    if (tk == TK_COLON_COLON) {
        result->nspace = found = cscope_root;
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
            ns = cscope_current;
        if (ns->usings != NULL && result->is_qualified == 0) {
            iterator.nspace = NULL;
            iterator.lookup = build_namespace_scope_rec(ns);
        } else {
            iterator.nspace = ns;
            iterator.lookup = NULL;
        }
    }
    iterator.result = result;
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
                        result->type = classType;
                        return 1;
                    }
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    return 0;
                }
                if (token == 0x3c) {
                    if (TCE(classType)->theclass.flags & CLASS_IS_TEMPL_INST) {
                        classType = (Type *)TCE(classType)->templ;
                    } else if ((TCE(classType)->theclass.flags & CLASS_IS_TEMPL) == 0) {
                        result->type = classType;
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
                            result->type = templateType;
                            return 1;
                        }
                        return parse_qualified_templdep_type(result, templateType, flag);
                    }
                    if (templateType->type != TYPECLASS)
                        return 0;
                    result->nspace = found = TCE(templateType)->theclass.nspace;
                    if (CPrepTokenizer_GetNextTokenAndRestorePosition() != 0x174) {
                        result->type = templateType;
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
                        result->type = objectType;
                        return 1;
                    }
                    CError_ReportError(ERR_DECLARATION_SYNTAX_ERROR);
                    return 0;
                }
                tk = CPrepTokenizer_GetNextToken();
                if (tk == '<') {
                    result->type = objectType;
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
        if (iterator.lookup != NULL) {
            iterator.lookup = iterator.lookup->next;
            hasNext = (iterator.lookup != NULL);
        } else {
            iterator.nspace = iterator.nspace->parent;
            if (iterator.nspace != NULL) {
                if (iterator.nspace->usings != NULL && iterator.result->is_qualified == 0) {
                    iterator.lookup = build_namespace_scope_rec(iterator.nspace);
                    iterator.nspace = NULL;
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

Boolean CScope_ParseElaborateName(NameResult *result)
{
    HashNameNode *name;
    NameSpaceObjectList *objects;
    CScopeNSIterator lookup;
    Boolean more;
    NameSpace *currentScope;
    NameSpace *parsedScope;

    if (copts.cplusplus == 0) {
        memclrw(result, sizeof(*result));
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        currentScope = cscope_current;
        name = data_00587fa0;
        if (currentScope->usings != NULL && result->is_qualified == 0) {
            lookup.nspace = NULL;
            lookup.lookup = build_namespace_scope_rec(currentScope);
        } else {
            lookup.nspace = currentScope;
            lookup.lookup = NULL;
        }
        lookup.result = result;
        do {
            for (objects = find_scope_object_list(&lookup, name); objects != NULL; objects = objects->next) {
                if (objects->object->otype == OT_TYPETAG) {
                    result->nspace = lookup.lookup != NULL ? lookup.lookup->nspace : lookup.nspace;
                    return set_parse_result_from_objects(result, objects, name);
                }
            }
            if (lookup.lookup != NULL) {
                lookup.lookup = lookup.lookup->next;
                more = lookup.lookup != NULL;
            } else {
                lookup.nspace = lookup.nspace->parent;
                if (lookup.nspace != NULL) {
                    if (lookup.nspace->usings != NULL && lookup.result->is_qualified == 0) {
                        lookup.lookup = build_namespace_scope_rec(lookup.nspace);
                        lookup.nspace = NULL;
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
        result->nspace = cscope_current;
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            return 0;
        }
        name = data_00587fa0;
    } else {
        if (result->type != NULL)
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
        lookup.nspace = NULL;
        lookup.lookup = build_namespace_scope_rec(parsedScope);
    } else {
        lookup.nspace = parsedScope;
        lookup.lookup = NULL;
    }
    lookup.result = result;
    do {
        for (objects = find_scope_object_list(&lookup, name); objects != NULL; objects = objects->next) {
            if (objects->object->otype == OT_TYPETAG || objects->object->otype == OT_TYPE) {
                result->nspace = lookup.lookup != NULL ? lookup.lookup->nspace : lookup.nspace;
                return set_parse_result_from_objects(result, objects, name);
            }
        }
        if (lookup.lookup != NULL) {
            lookup.lookup = lookup.lookup->next;
            more = lookup.lookup != NULL;
        } else {
            lookup.nspace = lookup.nspace->parent;
            if (lookup.nspace != NULL) {
                if (lookup.nspace->usings != NULL && lookup.result->is_qualified == 0) {
                    lookup.lookup = build_namespace_scope_rec(lookup.nspace);
                    lookup.nspace = NULL;
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

Boolean CScope_FindObject(NameSpace *nspace, NameResult *result, HashNameNode *name)
{
    NameSpaceObjectList *list;
    CScopeNSIterator iterator;

    CScope_NSIteratorInit(&iterator, nspace, result);
    do {
        for (list = CScope_NSIteratorFind(&iterator, name); list; list = list->next) {
            if (copts.cplusplus || list->object->otype != OT_TYPETAG) {
                result->nspace = iterator.lookup ? iterator.lookup->nspace : iterator.nspace;
                return set_parse_result_from_objects(result, list, name);
            }
        }
    } while (CScope_NSIteratorNext(&iterator));
    return 0;
}

#undef CERROR_FILE

#define CERROR_FILE "NameResult.c"

NameSpaceObjectList *CScope_FindObjectList(NameResult *result, HashNameNode *name)
{
    NameSpace *namespace;
    CScopeNSIterator state;
    NameSpaceObjectList *entry;
    Boolean more;

    memclrw(result, sizeof(*result));
    namespace = (NameSpace *)cscope_current;
    if (namespace->usings != NULL && result->is_qualified == 0) {
        state.nspace = NULL;
        state.lookup = build_namespace_scope_rec(namespace);
    } else {
        state.nspace = namespace;
        state.lookup = NULL;
    }
    state.result = result;
    do {
        for (entry = find_scope_object_list(&state, name); entry != NULL; entry = entry->next) {
            if (copts.cplusplus || entry->object->otype != OT_TYPETAG) {
                result->nspace = state.lookup ? state.lookup->nspace : state.nspace;
                return entry;
            }
        }
        if (state.lookup != NULL) {
            state.lookup = state.lookup->next;
            more = (state.lookup != NULL);
        } else {
            state.nspace = state.nspace->parent;
            if (state.nspace != NULL) {
                if (state.nspace->usings != NULL && state.result->is_qualified == 0) {
                    state.lookup = build_namespace_scope_rec(state.nspace);
                    state.nspace = NULL;
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
    CScopeNSIterator lookup;
    NameResult result;
    NameSpace *ns;

    memclrw(&result, sizeof(result));
    ns = cscope_current;
    if (ns->usings != NULL && result.is_qualified == 0) {
        lookup.nspace = NULL;
        lookup.lookup = build_namespace_scope_rec(ns);
    } else {
        lookup.nspace = ns;
        lookup.lookup = NULL;
    }
    lookup.result = &result;
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
        if (lookup.lookup != NULL) {
            lookup.lookup = lookup.lookup->next;
            more = (lookup.lookup != NULL);
        } else {
            lookup.nspace = lookup.nspace->parent;
            if (lookup.nspace != NULL) {
                if (lookup.nspace->usings != NULL && lookup.result->is_qualified == 0) {
                    lookup.lookup = build_namespace_scope_rec(lookup.nspace);
                    lookup.nspace = NULL;
                }
                more = 1;
            } else {
                more = 0;
            }
        }
    } while (more);
    return 0;
}

Boolean CScope_FindClassMemberObject(TypeClass *tclass, NameResult *result, HashNameNode *name)
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

int CScope_InitObjectIterator(CScopeObjectIterator *save, NameSpace *obj)
{
    memclrw(save, sizeof(*save));
    save->nspace = obj;
    if (save->nspace->is_hash == 0)
        save->nextname = obj->data.list;
    else
        save->nextname = *obj->data.hash;
}

#undef CERROR_FILE

Object *CScope_NextObjectIteratorObject(CScopeObjectIterator *s)
{
    while (1) {
        if (s->currlist != NULL) {
            do {
                ObjBase *obj = s->currlist->object;
                if (obj->otype == OT_OBJECT) {
                    s->currlist = s->currlist->next;
                    return (Object *)obj;
                }
                s->currlist = s->currlist->next;
            } while (s->currlist != NULL);
        }
        if (s->nextname != NULL) {
            s->currlist = &s->nextname->first;
            s->nextname = s->nextname->next;
            continue;
        }
        if (s->nspace->is_hash == 0 || ++s->hashindex >= 0x400)
            return NULL;
        s->nextname = s->nspace->data.hash[s->hashindex];
    }
}

#undef CERROR_FILE

/* Hash-table owner/namespace record as laid out in this build: the hash
 * bucket array lives at 0x10 and the "is hashed" byte flag at 0x18. */

NameSpaceObjectList *CScope_NextObjectIteratorObjectList(CScopeObjectIterator *state)
{
    NameSpaceName *entry;

    for (;;) {
        if ((entry = state->nextname) != NULL) {
            state->nextname = entry->next;
            return &entry->first;
        }
        if (state->nspace->is_hash == 0 || ++state->hashindex >= 0x400)
            return NULL;
        state->nextname = state->nspace->data.hash[state->hashindex];
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

Boolean CScope_FindTypeName(NameSpace *nspace, HashNameNode *name, NameResult *result)
{
    CScopeNSIterator state;
    NameSpaceObjectList *item;
    Boolean found;

    memclrw(result, sizeof(*result));
    if (nspace->usings != NULL && result->is_qualified == 0) {
        state.nspace = NULL;
        state.lookup = build_namespace_scope_rec(nspace);
    } else {
        state.nspace = nspace;
        state.lookup = NULL;
    }
    state.result = result;
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
        if (state.lookup != NULL) {
            state.lookup = state.lookup->next;
            found = state.lookup != NULL;
        } else {
            state.nspace = state.nspace->parent;
            if (state.nspace != NULL) {
                if (state.nspace->usings != NULL && state.result->is_qualified == 0) {
                    state.lookup = build_namespace_scope_rec(state.nspace);
                    state.nspace = NULL;
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
            if (l->object->otype == OT_OBJECT && l->object->datatype == DALIAS) {
                newlist = CScope_CopyList((ObjectList *)list);
                l = newlist;
                pp = &l;
                while ((p = *pp) != NULL) {
                    if (p->object->otype == OT_OBJECT && p->object->datatype == DALIAS)
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

Boolean CScope_ParseMemberName(TypeClass *ctx, NameResult *node, Boolean flag)
{
    Boolean result;
    if (tk == TK_COLON_COLON) {
    qualified_name:
        if (!CScope_ParseExprName(node))
            return 0;
        if (node->type != NULL && node->type->type == TYPETEMPLATE && ((TypeTemplDep *)node->type)->dtype == 1) {
            if (flag)
                return 1;
            CError_ReportError(ERR_ILLEGAL_USE_TEMPLATE_ARGUMENT_DEPENDENT_TYPE,
                               ((TypeTemplDep *)node->type)->u.qual.name->name);
            node->type = NULL;
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
    NameResult result;
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
    NameResult info;

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
        if (info.type != NULL && info.type->type == TYPETEMPLATE && ((TypeTemplDep *)info.type)->dtype == 1) {
            CError_ASSERT(3390, isVirtual);
            if (consumedToken) {
                ObjType *record = galloc(10);
                memclrw(record, 10);
                record->otype = OT_TYPE;
                record->access = flag;
                record->type = (Type *)info.type;
                CScope_AddObject(nspace, ((TypeTemplDep *)info.type)->u.qual.name, (ObjBase *)record);
            } else {
                CTemplateClass_AppendFuncDeclaration((TemplClass *)nspace->theclass, ((TypeTemplDep *)info.type), flag);
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
        NameSpace *savedNamespace = cscope_current;
        cscope_current = nspace;
        if (!CScope_ParseExprName(&info) || info.is_qualified == 0) {
            cscope_current = savedNamespace;
            CError_ReportError(ERR_ILLEGAL_ACCESS_USING_DECLARATION);
            return;
        }
        cscope_current = savedNamespace;
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
    NameResult lookupState;
    CScopeNSIterator search;
    NameSpaceObjectList *result;
    ObjNameSpace *object;
    Boolean found;

    memclrw(&lookupState, sizeof(lookupState));
    if (tk == TK_COLON_COLON) {
        nameSpace = cscope_root;
        lookupState.is_qualified = 1;
        tk = CPrepTokenizer_GetNextToken();
    }
    for (;;) {
        if (tk != TK_IDENTIFIER) {
            CError_ReportError(ERR_IDENTIFIER_EXPECTED);
            break;
        }
        if (nameSpace->usings != NULL && lookupState.is_qualified == 0) {
            search.nspace = NULL;
            search.lookup = build_namespace_scope_rec(nameSpace);
        } else {
            search.nspace = nameSpace;
            search.lookup = NULL;
        }
        search.result = &lookupState;
        do {
            result = find_scope_object_list(&search, data_00587fa0);
            if (result != NULL && result->object->otype == OT_NAMESPACE) {
                object = (ObjNameSpace *)result->object;
                nameSpace = object->nspace;
                break;
            }
            if (search.lookup != NULL) {
                search.lookup = search.lookup->next;
                found = (search.lookup != NULL);
            } else {
                search.nspace = search.nspace->parent;
                if (search.nspace != NULL) {
                    if (search.nspace->usings != NULL && search.result->is_qualified == 0) {
                        search.lookup = build_namespace_scope_rec(search.nspace);
                        search.nspace = NULL;
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

    if (!(list = CScope_FindName(cscope_current, name))) {
        tk = CPrepTokenizer_GetNextToken();
        objns = (ObjNameSpace *)galloc(sizeof(ObjNameSpace));
        memclrw(objns, sizeof(ObjNameSpace));
        objns->otype = OT_NAMESPACE;
        objns->access = ACCESSPUBLIC;
        objns->nspace = parse_namespace_name(cscope_current);
        CScope_AddObject(cscope_current, name, (ObjBase *)objns);
    } else if (list->object->otype != OT_NAMESPACE) {
        CError_ReportError(ERR_ILLEGAL_NAMESPACE);
        tk = CPrepTokenizer_GetNextToken();
        (void)parse_namespace_name(cscope_current);
    } else {
        tk = CPrepTokenizer_GetNextToken();
        if (parse_namespace_name(cscope_current) != ((ObjNameSpace *)list->object)->nspace)
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
