#define CERROR_FILE "CPrec.c"
#include "compiler/common.h"
#include "compiler/CPrec.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CABI.h"
#include "compiler/CClass.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInit.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "driver/COSToolsCLT.h"
#include "driver/CWCompilerLinkerPrivate.h"
#include "driver/CWPluginsPrivate.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>
#include <stdio.h>

/* 1.0 calls the image-writing helpers; later versions declare them inline. */
#if VERSION == VERSION_GC_1_0
#define CPREC_INLINE
#else
#define CPREC_INLINE inline
#endif

struct TemplateFunction *templateFunctions;
struct PendingFunction *pending_functions;
char *data_00587e84;
SInt32 data_00587f6c;
int objc_string_constant_count;
struct Type *sel_type;

/* Calls deferred until the precompiled data has been loaded. */

typedef enum { PRECFLAG_OFF = 0, PRECFLAG_ON = 1 } PrecFlag;

#pragma options align = mac68k
static SInt16 precompiled_file;
static GList precompiled_buffer;
static struct CPrecHeader *prec_header;
static struct CPrecWrittenEntry **written_entry_buckets;
static struct SerializedBucketEntry *serialized_bucket_entries;
static struct CPrecElem *serialized_buckets;
static CPrecWrittenEntry **data_00581c02;
static struct PendingBuffer *pending_buffers;
static struct SavedPrepTokenList *saved_prep_tokens;
static SInt32 *global_pointer_entries;
static SInt32 serialized_bucket_count;
static SInt32 prec_position;
static SInt32 flushed_size;
static UInt8 *data_00581c1e;
static UInt8 *precompiled_header_base;
static SInt16 data_00581c26;
static UInt8 data_00581c28;
static SInt32 data_00581c2a;
#pragma options align = reset

#define CPrec_Pad()                                                                                                    \
    do {                                                                                                               \
        if (data_00581c28 != 0) {                                                                                      \
            while ((prec_position & 3) != 0) {                                                                         \
                AppendGListByte(&precompiled_buffer, 0);                                                               \
                prec_position += 1;                                                                                    \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)
#define written_entry_buckets ((CPrecWrittenEntry **)written_entry_buckets)
enum { CPrecSerializedEntryBytes = 10 };

static CPrecWrittenEntry *CPrec_FindRecord(const void *key);
static inline UInt32 CPrec_HashPtr(const void *key);
static inline void append_data_and_count_bytes(void *data, SInt32 len);
static inline void *CPrec_AppendAlign_004de900(void);
static inline void insert_written_entry_by_key(void *key, void *value);
static inline SInt32 hash_value(SInt32 value);
static inline void append_precompiled_data(TypeBitfield *data, SInt32 len);
static inline void *pad_precompiled_buffer(void);
static void prepend_written_entry_to_hash_bucket(TypeBitfield *key, TypeBitfield *value);
static inline SInt32 hash_value_bytes(SInt32 value);
static inline CPrecWrittenEntry *findentry(void *key);
static inline UInt32 align_counter_with_zero_padding(void);
static inline UInt32 hashptr(void *p);
static inline void PrecompAppend(ENodeList *data, SInt32 size);
static inline SInt32 PrecompBegin(void);
static inline UInt32 CPrec_004daf90_Align(void);
static inline void patch_name_reference(UInt32 off, HashNameNode *nm);
static inline void append_data_and_count_size(void *d, SInt32 size);
static inline SInt32 append_alignment_padding(void);
static inline CPrecWrittenEntry *find_written_entry(Object *x);
static inline UInt32 CPrec_HashNew(void *p);
static inline UInt32 hash_pointer(void *pv);
static CPREC_INLINE ObjNameSpace *CPrec_GetObjNameSpacePatch(ObjNameSpace *obj);
static CPREC_INLINE NameSpaceList *CPrec_GetNSUsingPatch(NameSpaceList *u);
static inline void putlong(SInt32 value);
static CPREC_INLINE SInt16 CPrec_Flush(void);
static CPREC_INLINE SInt16 checkflush(void);

static inline SInt32 CPrec_AlignedPosition(void)
{
    CPrec_Pad();
    return prec_position;
}

static inline SInt32 CPrec_Hash(SInt32 value)
{
    CPrecBytes b;
    b.value = value;
    return (b.bytes[0] + value + b.bytes[1] + b.bytes[2] + b.bytes[3]) & 0x3fff;
}

static inline CPrecWrittenEntry *CPrec_FindAddrPatch(void *key)
{
    CPrecWrittenEntry *n;
    for (n = written_entry_buckets[CPrec_Hash((SInt32)key)]; n != NULL; n = n->next)
        if (n->object == key)
            return n;
    return NULL;
}

static inline void CPrec_NewAddrPatch(void *key, void *value)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;

    bucket = &written_entry_buckets[CPrec_Hash((SInt32)key)];
    n = lalloc(sizeof(CPrecWrittenEntry));
    n->object = key;
    n->image_position = value;
    n->next = *bucket;
    *bucket = n;
}

static inline void *CPrec_AppendAlign(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* CPrec_NamePatch body */
static inline void CPrec_NamePatch(void *dst, HashNameNode *name)
{
    name->id = 1;
    patch_object_reference((SInt32)dst, name);
}

static inline unsigned hash_inner(void *p)
{
    union {
        void *p;
        unsigned char b[4];
    } u;
    u.p = p;
    return ((unsigned)p + u.b[0] + u.b[1] + u.b[2] + u.b[3]) & 0x3fff;
}

static inline unsigned hash(void *p)
{
    return hash_inner(p);
}

static inline CPrecWrittenEntry *find(void *p)
{
    CPrecWrittenEntry *n;
    for (n = written_entry_buckets[hash(p)]; n; n = n->next)
        if (n->object == p)
            return n;
    return NULL;
}

static inline SInt32 alignbuf(void)
{
    if (data_00581c28)
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    return prec_position;
}

static inline void addpatch(void *p, UInt32 off)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;
    bucket = &written_entry_buckets[hash(p)];
    n = lalloc(12);
    n->object = p;
    n->image_position = (void *)off;
    n->next = *bucket;
    *bucket = n;
}

static inline void append(void *p, int size)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, p, size);
    prec_position += size;
}

static inline int fn_004da6c0_part1(int p)
{
    union {
        int w;
        unsigned char b[4];
    } u;
    u.w = p;
    return u.b[0] + p + u.b[1] + u.b[2] + u.b[3] & 16383;
}

static inline ObjectOffsetEntry *fn_004da6c0_inline1(void *a0)
{
    int v1s;
    ObjectOffsetEntry *v1;
    v1 = (ObjectOffsetEntry *)(v1s = ((int *)written_entry_buckets)[fn_004da6c0_part1((int)a0)]);
    if (v1s != 0) {
        do {
            if ((int)v1->object == (int)a0) {
                return v1;
            }
            v1 = (ObjectOffsetEntry *)v1->next;
        } while ((int)v1 != 0);
    }
    return NULL;
}

static inline ObjectOffsetEntry *fn_004da6c0_inline2(ObjMemberVar *a0)
{
    int v6;
    int v7s;
    ObjectOffsetEntry *v7;
    v6 = (int)a0->next;
    v7 = (ObjectOffsetEntry *)(v7s = ((int *)written_entry_buckets)[fn_004da6c0_part1((int)a0->next)]);
    if (v7s != 0) {
        do {
            if ((int)v7->object == v6) {
                return v7;
            }
            v7 = (ObjectOffsetEntry *)v7->next;
        } while ((int)v7 != 0);
    }
    return NULL;
}

static inline int *fn_004da6c0_bucket(int key)
{
    return (int *)&written_entry_buckets[fn_004da6c0_part1(key)];
}

static inline int get_next_offset(long v2, int *saved)
{
    long t5;
    t5 = prec_position;
    *saved = t5;
    add_serialized_bucket_entry((SInt32)(v2 + 4), (SInt32)(t5));
    return t5;
}

static inline UInt32 hash_pointer_bytes(void *p)
{
    union {
        void *p;
        UInt8 b[4];
    } u;
    u.p = p;
    return ((UInt32)p + u.b[0] + u.b[1] + u.b[2] + u.b[3]) & 0x3fff;
}

static inline CPrecWrittenEntry *find_written_entry_by_object(void *p)
{
    CPrecWrittenEntry *n;
    for (n = written_entry_buckets[hash_pointer_bytes(p)]; n; n = n->next)
        if (n->object == p)
            return n;
    return NULL;
}

static inline SInt32 align_buffer_to_four_bytes(void)
{
    if (data_00581c28)
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    return prec_position;
}

static inline void add(void *p, UInt32 value)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;
    bucket = &written_entry_buckets[hash_pointer_bytes(p)];
    n = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    n->object = p;
    n->image_position = (void *)value;
    n->next = *bucket;
    *bucket = n;
}

static inline void append_enum_const(ObjEnumConst *p)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, p, 22);
    prec_position += 22;
}

/* memclrw */
/* CPrec_NamePatch body */

static inline CPrecWrittenEntry *CPrec_FindAddrPatch_004deb20(void *key)
{
    CPrecWrittenEntry *n;
    for (n = written_entry_buckets[CPrec_Hash((SInt32)key)]; n != NULL; n = n->next)
        if (n->object == key)
            return n;
    return NULL;
}

static inline void prepend_written_entry(void *key, int value)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;
    bucket = &written_entry_buckets[CPrec_Hash((SInt32)key)];
    n = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    n->object = key;
    n->image_position = (void *)value;
    n->next = *bucket;
    *bucket = n;
}

static inline void *append_four_byte_alignment_padding(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void append_precompiled_buffer_data(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static inline void set_name_id_and_forward(void *dst, HashNameNode *name)
{
    name->id = 1;
    (void)patch_object_reference((SInt32)(dst), (void *)(name));
}

static inline SInt32 CPrec_GetTypePatch(Type *type)
{
    CPrecWrittenEntry *e;

    if ((e = fn_004e0680(type)))
        return (SInt32)e->image_position;
    switch ((SInt8)type->type) {
        case TYPEARRAY:
        case TYPEPOINTER: {
            TypePointer *pointerType = (TypePointer *)type;
            return write_pointer_type(pointerType);
        }
        case TYPEENUM: {
            TypeEnum *enumType = (TypeEnum *)type;
            TypeEnum *(*patchEnum)(TypeEnum *) = append_type_enum;
            TypeEnum *patch = patchEnum(enumType);
            return (SInt32)patch;
        }
        case TYPEBITFIELD: {
            TypeBitfield *bitfieldType = (TypeBitfield *)type;
            TypeBitfield *patch = (TypeBitfield *)serialize_type_bitfield(bitfieldType);
            return (SInt32)patch;
        }
        case TYPESTRUCT:
            return (SInt32)append_type_struct((TypeStruct *)type);
        case TYPEFUNC: {
            TypeFunc *functionType = (TypeFunc *)type;
            TypeFunc *functionPatch = copy_type_func(functionType);
            return (SInt32)functionPatch;
        }
        case TYPEMEMBERPOINTER:
            return (SInt32)append_member_pointer_type((TypeMemberPointer *)type);
        case TYPETEMPLATE: {
            TypeTemplDep *templateType = (TypeTemplDep *)type;
            return write_templdep(templateType);
        }
        case TYPECLASS: {
            TypeClass *classType = (TypeClass *)type;
            return write_typeclass(classType);
        }
        case TYPEVOID:
        case TYPEINT:
        case TYPEFLOAT:
        case TYPELABEL:
        case TYPEOBJCID:
        case TYPETEMPLDEPEXPR:
        default:
            CError_FATAL(2691);
            return 0;
    }
}

static inline SInt32 AppendAlign(void)
{
    if (data_00581c28)
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    return prec_position;
}

static inline void NewAddrPatch(TypePointer *ptr, SInt32 value)
{
    union {
        TypePointer *ptr;
        UInt8 bytes[4];
    } u;
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *p;
    u.ptr = ptr;
    bucket = (CPrecWrittenEntry *
                  *)&written_entry_buckets[((UInt32)ptr + u.bytes[0] + u.bytes[1] + u.bytes[2] + u.bytes[3]) & 0x3fff];
    p = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    p->object = ptr;
    p->image_position = (void *)value;
    p->next = *bucket;
    *bucket = p;
}

static inline void AppendData(void *ptr, int size)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, ptr, size);
    prec_position += size;
}

void CException_ResetPrecompiledState(UInt8 c)
{
    precompiled_file = 0;
    precompiled_buffer.data = NULL;
    prec_header = NULL;
    pending_buffers = NULL;
    data_00581c26 = 0;
    return;
}

void fn_004e0970(void)
{
    if (precompiled_file != 0) {
        COS_FileClose(precompiled_file);
        precompiled_file = 0;
    }
    if (precompiled_buffer.data != NULL) {
        FreeGList(&precompiled_buffer);
    }
}

OLinkList *copy_relocation_list(OLinkList *p)
{
    OLinkList *n;
    if (p == NULL)
        return NULL;
    n = galloc(sizeof(OLinkList));
    *n = *p;
    n->next = copy_relocation_list(n->next);
    return n;
}

void CException_AddPendingBuffer(Object *owner, const void *buffer, OLinkList *value, int entryValue)
{
    struct PendingBuffer *entry;

    if (owner->sclass != TK_STATIC && (owner->qual & (Q_IMPLICIT_WEAK | Q_WEAK)) == 0)
        CError_ReportError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);

    entry = (struct PendingBuffer *)galloc(sizeof(struct PendingBuffer));
    entry->owner = owner;
    entry->entryValue = entryValue;
    entry->next = pending_buffers;
    pending_buffers = entry;

    if (buffer != NULL) {
        entry->buffer = galloc(owner->type->size);
        memcpy(entry->buffer, buffer, owner->type->size);
    } else {
        entry->buffer = NULL;
    }

    entry->value = copy_relocation_list(value);
}

static CPREC_INLINE void CPrec_SetupPtrHash(void)
{
    written_entry_buckets = lalloc(0x4000 * sizeof(CPrecWrittenEntry *));
    memclrw(written_entry_buckets, 0x4000 * sizeof(CPrecWrittenEntry *));
}

static CPREC_INLINE void CPrec_SetupSelectorHash(void)
{
    data_00581c02 = lalloc(0x400 * sizeof(CPrecWrittenEntry *));
    memclrw(data_00581c02, 0x400 * sizeof(CPrecWrittenEntry *));
}

unsigned int CException_HashType(Type *type)
{
    union {
        unsigned int value;
        unsigned char bytes[4];
    } address;

    address.value = (unsigned int)type;
    return (address.bytes[0] + (unsigned int)type + address.bytes[1] + address.bytes[2] + address.bytes[3]) & 0x3fffU;
}

CPrecWrittenEntry *fn_004e0680(void *key)
{
    CPrecWrittenEntry *entry;

    for (entry = written_entry_buckets[CException_HashType(key)]; entry != NULL; entry = entry->next) {
        if (entry->object == key)
            return entry;
    }
    return NULL;
}

void add_written_type_entry(void *key, void *imagePosition)
{
    CPrecWrittenEntry **slot;
    CPrecWrittenEntry *entry;

    slot = &written_entry_buckets[CException_HashType(key)];
    entry = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    entry->object = key;
    entry->image_position = imagePosition;
    entry->next = *slot;
    *slot = entry;
}

void build_global_pointer_entries(void)
{
    int index, count;
    char counting = 1;
    CPrecPtrU *entries;
    index = count = 0;
    for (;;) {
        if (!counting) {
            entries[index].address = (UInt8 *)cscope_root;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stvoid;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stbool;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stchar;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stsignedchar;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stunsignedchar;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stwchar;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stsignedshort;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stunsignedshort;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stsignedint;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stunsignedint;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stsignedlong;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stunsignedlong;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stsignedlonglong;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stunsignedlonglong;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stfloat;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stshortdouble;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stdouble;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stlongdouble;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&elipsis;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&oldstyle;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&type_placeholder;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&data_0055d5c0;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&stvoid;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&void_ptr;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&data_0055d5e8;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)&exception_temp_object_type;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)newh_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00587ed0;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_005870d8;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)typeid_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)dynamic_cast_object;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)cast_member_pointer_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)rt_memberpointercompare;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)memberpointercompare_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_0058769c;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00587fd0;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00587678;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00588260;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)som_ref_node_rtfunc;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)som_ref_node_runtime_object;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00588278;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00588060;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_005876c0;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00587f80;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)member_function_pointer_call_rtfunc;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)class_array_initializer;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)array_allocation_runtime_function;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_0058717c;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)destructor_aware_call_rtfunc;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)destructor_aware_call_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)destructor_registration_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)throw_func;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_005882a4;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_005875a0;
            index++;
        } else
            count++;
        if (!counting) {
            entries[index].address = (UInt8 *)data_00587654;
            index++;
        } else
            count++;
        if (!counting)
            break;
        entries = lalloc(count * sizeof(*entries));
        global_pointer_entries = (SInt32 *)entries;
        serialized_bucket_count = count;
        counting = 0;
    }
}

static CPREC_INLINE void CPrec_SetupBuiltIns(void)
{
    SInt32 i;

    build_global_pointer_entries();
    serialized_buckets = lalloc(serialized_bucket_count * sizeof(struct CPrecElem));
    memclrw(serialized_buckets, serialized_bucket_count * sizeof(struct CPrecElem));
    for (i = 0; i < serialized_bucket_count; i++) {
        serialized_buckets[i].object = global_pointer_entries[i];
        serialized_buckets[i].image_position = ~i;
        add_written_type_entry((void *)serialized_buckets[i].object, (void *)serialized_buckets[i].image_position);
    }
}

int add_serialized_bucket_entry(SInt32 offset, SInt32 listIndex)
{
    SerializedBucketEntry *node;
    SInt32 listId;

    if (data_00581c28 != 0) {
        node = (SerializedBucketEntry *)lalloc(sizeof(*node));
        node->offset = offset;
        CError_ASSERT(484, (node->offset & 0x80000001) == 0);
        if (listIndex < 0) {
            listIndex = ~listIndex;
            CError_ASSERT(490, listIndex < serialized_bucket_count);
            node->next = serialized_buckets[listIndex].list;
            listId = 0;
            serialized_buckets[listIndex].list = node;
        } else {
            node->next = serialized_bucket_entries;
            serialized_bucket_entries = node;
            listId = listIndex;
        }
        offset -= flushed_size;
        CError_ASSERT(502, offset >= 0 && offset <= precompiled_buffer.size);
        *(SInt32 *)((char *)*precompiled_buffer.data + offset) = listId;
    }
}

int patch_object_reference(SInt32 location, HashNameNode *object)
{
    if (data_00581c28) {
        CPrecWrittenEntry *entry;
        SerializedBucketEntry *reference;
        SInt32 index;

        CError_ASSERT(520, entry = fn_004e0680(object));
        reference = (SerializedBucketEntry *)lalloc(sizeof(SerializedBucketEntry));
        reference->offset = location;
        reference->next = serialized_bucket_entries;
        serialized_bucket_entries = reference;
        CError_ASSERT(525, (reference->offset & 0x80000001) == 0);
        index = location - flushed_size;
        CError_ASSERT(529, index >= 0 && index <= precompiled_buffer.size);
        {
            void **slot = (void **)(*precompiled_buffer.data + index);
            *slot = entry->image_position;
        }
    }
}

void patch_hash_name_reference(void *dst, HashNameNode *record)
{
    record->id = 1;
    patch_object_reference((SInt32)dst, record);
}

void *align_to_four_byte_boundary(void)
{
    if (data_00581c28 != '\0') {
        for (; (prec_position & 3) != 0; prec_position += 1) {
            AppendGListByte(&precompiled_buffer, 0);
        }
    }
    return (void *)prec_position;
}

static CPREC_INLINE void put(UInt8 c)
{
    if (data_00581c28 != 0)
        AppendGListByte(&precompiled_buffer, c);
    prec_position++;
}

static CPREC_INLINE void CPrec_AppendWord16(SInt16 v)
{
    if (data_00581c28)
        AppendGListWord(&precompiled_buffer, v);
    prec_position += 2;
}

static CPREC_INLINE void CPrec_AppendPointer(void *p)
{
    if (data_00581c28)
        AppendGListLong(&precompiled_buffer, (SInt32)p);
    prec_position += 4;
}

static CPREC_INLINE void CPrec_AppendWord32(SInt32 v)
{
    if (data_00581c28)
        AppendGListLong(&precompiled_buffer, v);
    prec_position += 4;
}

static CPREC_INLINE void CPrec_AppendPointerPatch(void *ptr)
{
    CPrecWrittenEntry *entry;
    SerializedBucketEntry *patch;

    if (ptr != NULL) {
        CError_ASSERT(621, entry = fn_004e0680(ptr));
        if (data_00581c28) {
            patch = (SerializedBucketEntry *)lalloc(sizeof(SerializedBucketEntry));
            patch->offset = prec_position;
            patch->next = serialized_bucket_entries;
            serialized_bucket_entries = patch;
            CError_ASSERT(628, (patch->offset & 0x80000001) == 0);
        }
        CPrec_AppendWord32((SInt32)entry->image_position);
    } else {
        CPrec_AppendWord32(0);
    }
}

static CPREC_INLINE void CPrec_AppendNamePatch(HashNameNode *name)
{
    if (name != NULL) {
        CPrec_AppendPointerPatch(name);
        name->id = 1;
    }
}

static CPREC_INLINE void CPrec_AppendString(const char *s)
{
    SInt32 len = strlen(s) + 1;
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, (void *)s, len);
    prec_position += len;
}

void fn_004e0010(void *data, UInt32 size)
{
    if (data_00581c28 != 0) {
        AppendGListData(&precompiled_buffer, data, size);
    }
    prec_position += size;
}

static CPREC_INLINE void CPrec_RawMemPatch(void *source, void *data, SInt32 len)
{
    void *offset = align_to_four_byte_boundary();
    fn_004e0010(data, len);
    add_serialized_bucket_entry((SInt32)source, (SInt32)offset);
}

void append_hash_names(void)
{
    HashNameNode *name;
    int i;
    HashNameNode *p;
    HashNameNode *next;

    if (data_00581c28) {
        i = 0;
        do {
            name = data_00587f88[i];
            while (name && name->id == 0)
                name = name->next;
            if (name) {
                prec_header->hashNameOffsets[i] = (SInt32)(p = align_to_four_byte_boundary());
                while (1) {
                    add_written_type_entry(name, p);
                    CPrec_AppendWord32(0);
                    CPrec_AppendPointer(NULL);
                    CPrec_AppendWord16(name->hashval);
                    CPrec_AppendString(name->name);
                    name = name->next;
                    while (name && name->id == 0)
                        name = name->next;
                    if (!name)
                        break;
                    add_serialized_bucket_entry((SInt32)(&p->next), (SInt32)(next = align_to_four_byte_boundary()));
                    p = next;
                }
            }
        } while (++i < 0x800);
    } else {
        i = 0;
        do {
            if ((name = data_00587f88[i])) {
                p = align_to_four_byte_boundary();
                while (1) {
                    add_written_type_entry(name, p);
                    CPrec_AppendWord32(0);
                    CPrec_AppendPointer(NULL);
                    CPrec_AppendWord16(name->hashval);
                    CPrec_AppendString(name->name);
                    name = name->next;
                    if (!name)
                        break;
                    add_serialized_bucket_entry((SInt32)(&p->next), (SInt32)(next = align_to_four_byte_boundary()));
                    p = next;
                }
            }
        } while (++i < 0x800);
    }
}

void serialize_macros(void)
{
    SInt32 j;
    Macro *p;
    SInt32 i;
    void *pos;
    void *start;

    i = 0;
    do {
        for (p = macro_buckets[i]; p != NULL; p = p->next) {
            if (p->text != NULL) {
                add_written_type_entry(p->text, (void *)prec_position);
                CPrec_AppendString(p->text);
            }
        }
    } while (++i < 0x800);

    i = 0;
    do {
        if ((p = macro_buckets[i]) == NULL)
            continue;
        start = align_to_four_byte_boundary();
        if (data_00581c28)
            prec_header->macroOffsets[i] = (SInt32)start;
        for (;;) {
            CPrec_AppendWord32(0);
            CPrec_AppendNamePatch(p->name);
            CPrec_AppendPointerPatch(p->text);
            CPrec_AppendWord16(p->nargs);
            put(p->flag);
            put(p->isExpanding);
            for (j = 1; j < (p->nargs & 0x7fff); j++)
                CPrec_AppendNamePatch(p->args[j - 1]);
            p = p->next;
            if (p == NULL)
                break;
            add_serialized_bucket_entry((SInt32)start, (SInt32)(pos = align_to_four_byte_boundary()));
            start = pos;
        }
    } while (++i < 0x800);
}

SInt32 write_bclass_list(BClassList *classes)
{
    CPrecWrittenEntry *entry;
    BClassList *first;
    BClassList *current;
    BClassList *next;

    if ((entry = fn_004e0680(classes)))
        return (SInt32)entry->image_position;

    add_written_type_entry(classes, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(classes, sizeof(BClassList));
        add_serialized_bucket_entry((SInt32)&current->type, write_type(classes->type));
        if (!classes->next)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        classes = classes->next;
    }
    return (SInt32)first;
}

int hash_pointer_type(TypePointer *type)
{
    Type *base;
    int hash;
    FuncArg *arg;
    Type *returnType;
    TypePointer *pointerType;
    hash = type->qual;
    base = type->target;
    for (;;) {
        switch ((signed char)base->type) {
            case TYPECLASS: {
                const TypeClass *classType = (const TypeClass *)base;
                const HashNameNode *className;
                if ((className = classType->classname) != NULL)
                    hash += className->hashval;
                break;
            }
            case TYPEENUM:
                if (((TypeEnum *)base)->enumname != NULL)
                    hash += ((TypeEnum *)base)->enumname->hashval;
                hash += TYPEENUM;
                base = ((TypeEnum *)base)->enumtype;
            case TYPEINT:
            case TYPEFLOAT:
                hash += ((TypeIntegral *)base)->integral;
                break;
            case TYPEPOINTER:
                pointerType = (TypePointer *)base;
                hash += pointerType->qual;
                base = pointerType->target;
                continue;
            case TYPEARRAY:
                hash += base->size;
                base = ((TypePointer *)base)->target;
                continue;
            case TYPEFUNC:
                returnType = ((TypeFunc *)base)->functype;
                hash += (signed char)returnType->type;
                hash += returnType->size;
                arg = ((TypeFunc *)base)->args;
                if (arg != NULL) {
                    do {
                        if (arg->type != NULL) {
                            hash += (signed char)arg->type->type;
                            hash += arg->type->size;
                        }
                        arg = arg->next;
                    } while (arg != NULL);
                }
                break;
            default:
                break;
        }
        break;
    }
    hash += (signed char)base->type + base->size;
    return ((hash >> 8) + ((hash >> 24) + hash + (hash >> 16))) & 1023;
}

#if VERSION == VERSION_GC_1_0
SInt32 write_pointer_type(TypePointer *ptr)
{
    CPrecWrittenEntry *entry;
    TypePointer *type;
    TypePointer *pos;
    int hash;
    TypePointer *writtenType;

    if (ptr->qual & Q_IS_OBJC_ID) {
        add_written_type_entry(ptr, pos = align_to_four_byte_boundary());
        fn_004e0010(ptr, 20);
        if (ptr->protocols[0])
            add_serialized_bucket_entry((SInt32)&pos->protocols[0], (SInt32)write_object_list(ptr->protocols[0]));
    } else {
        if (!copts.faster_pch_gen && data_00581c28 && ptr->size > 0) {
            hash = hash_pointer_type(ptr);
            for (entry = (CPrecWrittenEntry *)data_00581c02[hash]; entry; entry = entry->next) {
                type = ptr;
                writtenType = (TypePointer *)entry->object;
                do {
                    if (type->type != writtenType->type || type->size != writtenType->size ||
                        type->qual != writtenType->qual)
                        break;
                    type = (TypePointer *)type->target;
                    writtenType = (TypePointer *)writtenType->target;
                    if (type->type != TYPEPOINTER || type->type != TYPEARRAY) {
                        if (type == writtenType)
                            return (SInt32)entry->image_position;
                        break;
                    }
                } while (1);
            }
            add_written_type_entry(ptr, pos = align_to_four_byte_boundary());
            entry = (CPrecWrittenEntry *)lalloc(sizeof(*entry));
            entry->object = ptr;
            entry->image_position = pos;
            entry->next = (CPrecWrittenEntry *)data_00581c02[hash];
            data_00581c02[hash] = (CPrecWrittenEntry *)entry;
        } else {
            add_written_type_entry(ptr, pos = align_to_four_byte_boundary());
        }
        fn_004e0010(ptr, 14);
    }
    add_serialized_bucket_entry((SInt32)&pos->target, write_type(ptr->target));
    return (SInt32)pos;
}
#else
SInt32 write_pointer_type(TypePointer *ptr)
{
    SInt32 pos;
    int hash;
    CPrecWrittenEntry *entry;

    if (ptr->qual & Q_IS_OBJC_ID) {
        NewAddrPatch(ptr, pos = AppendAlign());
        AppendData(ptr, 20);
        if (ptr->protocols[0])
            add_serialized_bucket_entry(pos + 14, (SInt32)write_object_list(ptr->protocols[0]));
    } else {
        if (!copts.faster_pch_gen && data_00581c28 && ptr->size > 0) {
            hash = hash_pointer_type(ptr);
            for (entry = (CPrecWrittenEntry *)data_00581c02[hash]; entry; entry = entry->next) {
                TypePointer *type = ptr, *writtenType = (TypePointer *)entry->object;
                do {
                    if (type->type != writtenType->type || type->size != writtenType->size ||
                        type->qual != writtenType->qual)
                        break;
                    type = (TypePointer *)type->target;
                    writtenType = (TypePointer *)writtenType->target;
                    if (type->type != TYPEPOINTER || type->type != TYPEARRAY) {
                        if (type == writtenType)
                            return (SInt32)entry->image_position;
                        break;
                    }
                } while (1);
            }
            NewAddrPatch(ptr, pos = AppendAlign());
            entry = (CPrecWrittenEntry *)lalloc(sizeof(*entry));
            entry->object = ptr;
            entry->image_position = (char *)pos;
            entry->next = (CPrecWrittenEntry *)data_00581c02[hash];
            data_00581c02[hash] = (CPrecWrittenEntry *)entry;
        } else {
            NewAddrPatch(ptr, pos = AppendAlign());
        }
        AppendData(ptr, 14);
    }
    add_serialized_bucket_entry(pos + 6, write_type(ptr->target));
    return pos;
}
#endif

TypeEnum *append_type_enum(TypeEnum *tenum)
{
    TypeEnum *p;

    add_written_type_entry(tenum, p = align_to_four_byte_boundary());
    fn_004e0010(tenum, sizeof(TypeEnum));
    if (tenum->nspace)
        add_serialized_bucket_entry((SInt32)&p->nspace, get_namespace_patch(tenum->nspace));
    if (tenum->enumlist)
        add_serialized_bucket_entry((SInt32)&p->enumlist, write_enum_const(tenum->enumlist));
    add_serialized_bucket_entry((SInt32)&p->enumtype, write_type(tenum->enumtype));
    if (tenum->enumname)
        patch_hash_name_reference(&p->enumname, tenum->enumname);
    return p;
}

/* CPrec_GetTypePatch */
/* CPrec_NewPointerPatch */

int serialize_type_bitfield(TypeBitfield *bitfield)
{
    TypeBitfield *p;

    add_written_type_entry(bitfield, p = align_to_four_byte_boundary());
    fn_004e0010(bitfield, sizeof(TypeBitfield));
    add_serialized_bucket_entry((SInt32)&p->bitfieldtype, write_type(bitfield->bitfieldtype));
    return (int)p;
}

TypeStruct *append_type_struct(TypeStruct *tstruct)
{
    StructMember *member;
    TypeStruct *p;
    StructMember *current;
    StructMember *next;

    add_written_type_entry(tstruct, p = align_to_four_byte_boundary());
    fn_004e0010(tstruct, 0x12);
    if (tstruct->name)
        patch_hash_name_reference(&p->name, tstruct->name);
    if ((member = tstruct->members)) {
        add_serialized_bucket_entry((SInt32)(&p->members), (SInt32)(current = align_to_four_byte_boundary()));
        while (1) {
            fn_004e0010(member, sizeof(StructMember));
            add_serialized_bucket_entry((SInt32)(&current->type), write_type(member->type));
            patch_hash_name_reference(&current->name, member->name);
            if (!member->next)
                break;
            add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = align_to_four_byte_boundary()));
            current = next;
            member = member->next;
        }
    }
    return p;
}

unsigned int write_except_spec_list(ExceptSpecList *record)
{
    ExceptSpecList *first;
    ExceptSpecList *current;
    ExceptSpecList *next;

    first = current = align_to_four_byte_boundary();
    while (record != NULL) {
        fn_004e0010(record, sizeof(ExceptSpecList));
        if (record->type != NULL)
            add_serialized_bucket_entry((SInt32)&current->type, write_type(record->type));
        if (record->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        record = record->next;
    }
    return (unsigned int)first;
}

#if VERSION == VERSION_GC_1_0
FuncArg *write_func_args(FuncArg *arg, Boolean naming)
{
#if VERSION > VERSION_GC_1_0
    SInt32 position;
    SInt32 nextPosition;
#endif
    CPrecWrittenEntry *entry;
    FuncArg *first;
    FuncArg *current;
    FuncArg *next;

    if ((entry = fn_004e0680(arg)))
        return entry->image_position;

    first = current = align_to_four_byte_boundary();
    while (1) {
        if (!naming)
            arg->name = NULL;
        fn_004e0010(arg, sizeof(FuncArg));
        if (naming && arg->name)
            patch_hash_name_reference(&current->name, arg->name);
        if (arg->dexpr)
            add_serialized_bucket_entry((SInt32)&current->dexpr, write_enode((ENode *)arg->dexpr));
        if (arg->type)
            add_serialized_bucket_entry((SInt32)&current->type, write_type(arg->type));
        else
            CError_FATAL(1142);
        if (!arg->next)
            break;
        if ((entry = fn_004e0680(arg->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        next = align_to_four_byte_boundary();
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)next);
        current = next;
        arg = arg->next;
        add_written_type_entry(arg, next);
#else
        position = nextPosition = (SInt32)(next = align_to_four_byte_boundary());
        add_serialized_bucket_entry((SInt32)&current->next, nextPosition);
        current = next;
        arg = arg->next;
        add_written_type_entry(arg, (void *)position);
#endif
    }
    return first;
}
#else
FuncArg *write_func_args(FuncArg *arg, Boolean naming)
{
    CPrecWrittenEntry *entry;
    SInt32 position;
    SInt32 nextPosition;
    FuncArg *current;
    UInt32 first;
    FuncArg *next;

    if ((entry = CPrec_FindAddrPatch_004deb20(arg)))
        return entry->image_position;

    first = (UInt32)(current = append_four_byte_alignment_padding());
    while (1) {
        if (!naming)
            arg->name = NULL;
        append_precompiled_buffer_data(arg, sizeof(*arg));
        if (naming && arg->name)
            set_name_id_and_forward(&current->name, arg->name);
        if (arg->dexpr)
            (void)add_serialized_bucket_entry((SInt32)&current->dexpr, write_enode((ENode *)arg->dexpr));
        if (arg->type) {
            add_serialized_bucket_entry((SInt32)&current->type, (SInt32)write_type(arg->type));
        } else
            CError_FATAL(1142);
        if (!arg->next)
            break;
        if ((entry = CPrec_FindAddrPatch_004deb20(arg->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        position = nextPosition = (SInt32)(next = append_four_byte_alignment_padding());
        add_serialized_bucket_entry((SInt32)&current->next, nextPosition);
        current = next;
        arg = arg->next;
        prepend_written_entry(arg, position);
    }
    return (FuncArg *)first;
}
#endif

TypeFunc *copy_type_func(TypeFunc *tfunc)
{
    TypeFunc *p;

    add_written_type_entry(tfunc, p = align_to_four_byte_boundary());
    fn_004e0010(tfunc, (tfunc->flags & FUNC_METHOD) ? sizeof(TypeMemberFunc) : sizeof(TypeFunc));
    add_serialized_bucket_entry((SInt32)&p->functype, write_type(tfunc->functype));
    if (tfunc->args)
        add_serialized_bucket_entry((SInt32)&p->args, (SInt32)write_func_args(tfunc->args, (tfunc->flags & 0x8000400) != 0));
    if (tfunc->exspecs)
        add_serialized_bucket_entry((SInt32)&p->exspecs, write_except_spec_list(tfunc->exspecs));
    if (tfunc->flags & FUNC_METHOD)
        add_serialized_bucket_entry((SInt32)&TYPE_METHOD(p)->theclass, write_type((Type *)TYPE_METHOD(tfunc)->theclass));
    return p;
}

TypeMemberPointer *append_member_pointer_type(TypeMemberPointer *tmemp)
{
    TypeMemberPointer *p;

    add_written_type_entry(tmemp, p = align_to_four_byte_boundary());
    fn_004e0010(tmemp, 0x12);
    add_serialized_bucket_entry((SInt32)(&p->ty1), write_type(tmemp->ty1));
    add_serialized_bucket_entry((SInt32)(&p->ty2), write_type(tmemp->ty2));
    return p;
}

/* hash-table / emitter base */
/* tracing enabled? */
/* running byte counter */

int write_templdep(TypeTemplDep *node)
{
    TypeTemplDep *current;

    add_written_type_entry(node, current = align_to_four_byte_boundary());
    fn_004e0010(node, sizeof(TypeTemplDep));
    switch (node->dtype) {
        case 0:
            break;
        case 1:
            add_serialized_bucket_entry((SInt32)&current->u.qual.type, write_templdep(node->u.qual.type));
            patch_hash_name_reference(&current->u.qual.name, node->u.qual.name);
            break;
        case 2:
            add_serialized_bucket_entry((SInt32)&current->u.templ.templ, write_type(TYPE(node->u.templ.templ)));
            add_serialized_bucket_entry((SInt32)&current->u.templ.args, serialize_ct_state_elems(node->u.templ.args));
            break;
        case 3:
            add_serialized_bucket_entry((SInt32)&current->u.array.type, write_type(node->u.array.type));
            add_serialized_bucket_entry((SInt32)&current->u.array.index, write_enode(node->u.array.index));
            break;
        case 4:
            add_serialized_bucket_entry((SInt32)&current->u.qualtempl.type, write_templdep(node->u.qualtempl.type));
            add_serialized_bucket_entry((SInt32)&current->u.qualtempl.args, serialize_ct_state_elems(node->u.qualtempl.args));
            break;
        case 5:
            add_serialized_bucket_entry((SInt32)&current->u.bitfield.type, write_type(node->u.bitfield.type));
            add_serialized_bucket_entry((SInt32)&current->u.bitfield.size, write_enode(node->u.bitfield.size));
            break;
        default:
            CError_FATAL(1270);
    }
    return (int)current;
}

ClassList *write_class_list(ClassList *x)
{
    CPrecWrittenEntry *e;
    ClassList *first;
    ClassList *current;
    ClassList *next;

    if ((e = fn_004e0680(x)))
        return e->image_position;
    add_written_type_entry(x, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(x, 0x12);
        add_serialized_bucket_entry((SInt32)(&current->base), write_type((Type *)x->base));
        if (!x->next)
            break;
        if ((e = fn_004e0680(x->next))) {
            add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(e->image_position));
            break;
        }
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        x = x->next;
    }
    return first;
}

unsigned int write_vclasslist(VClassList *record)
{
    VClassList *first;
    VClassList *current;
    VClassList *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        fn_004e0010(record, sizeof(VClassList));
        add_serialized_bucket_entry((SInt32)&current->base, write_type((Type *)record->base));
        if (record->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        record = record->next;
    }
    return (unsigned int)first;
}

/* Serialized entries occupy ten bytes, independent of host padding. */

unsigned int serialize_entry_list(ClassFriend *record)
{
    ClassFriend *first;
    ClassFriend *current;
    ClassFriend *next;

    first = current = align_to_four_byte_boundary();
    for (;; record = record->next) {
        fn_004e0010(record, CPrecSerializedEntryBytes);
        if (record->isclass != 0)
            add_serialized_bucket_entry((SInt32)&current->u, write_type(TYPE(record->u.theclass)));
        else
            add_serialized_bucket_entry((SInt32)&current->u, write_object(record->u.obj));
        if (record->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
    }
    return (unsigned int)first;
}

unsigned int write_prec_type_entries(struct PrecTypeEntry *entry)
{
    struct PrecTypeEntry *first;
    struct PrecTypeEntry *current;
    struct PrecTypeEntry *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        fn_004e0010(entry, sizeof(struct PrecTypeEntry));
        add_serialized_bucket_entry((SInt32)&current->type, write_type((Type *)entry->type));
        if (entry->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        entry = entry->next;
    }
    return (unsigned int)first;
}

unsigned int write_vtable(VTable *record)
{
    VTable *p;

    p = align_to_four_byte_boundary();
    fn_004e0010(record, sizeof(VTable));
    if (record->object != NULL)
        add_serialized_bucket_entry((SInt32)&p->object, write_object(record->object));
    if (record->owner != NULL)
        add_serialized_bucket_entry((SInt32)&p->owner, write_type((Type *)record->owner));
    return (unsigned int)p;
}

static CPREC_INLINE SOMInfoEntry *write_som_method_names(SOMInfoEntry *node)
{
    SOMInfoEntry *first;
    SOMInfoEntry *current;
    SOMInfoEntry *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        fn_004e0010(node, 0xa);
        patch_hash_name_reference(&current->name, node->name);
        if (node->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        node = node->next;
    }
    return first;
}

SInt32 write_som_info(SOMInfo *entry)
{
    SOMInfo *p;

    p = align_to_four_byte_boundary();
    fn_004e0010(entry, 0x16);
    if (entry->baseClass != NULL)
        add_serialized_bucket_entry((SInt32)&p->baseClass, write_type((Type *)entry->baseClass));
    if (entry->classDataObject != NULL)
        add_serialized_bucket_entry((SInt32)&p->classDataObject, write_object(entry->classDataObject));
    if (entry->methodNameList != NULL)
        add_serialized_bucket_entry((SInt32)&p->methodNameList, (SInt32)write_som_method_names(entry->methodNameList));
    return (SInt32)p;
}

SInt32 write_objc_parameter_nodes(ObjCParameterNode *p)
{
    ObjCParameterNode *first;
    ObjCParameterNode *current;
    ObjCParameterNode *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        fn_004e0010(p, 0x20);
        if (p->selectorName)
            patch_hash_name_reference(&current->selectorName, p->selectorName);
        if (p->name)
            patch_hash_name_reference(&current->name, p->name);
        if (p->type)
            add_serialized_bucket_entry((SInt32)&current->type, write_type(p->type));
        if (!p->next)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        p = p->next;
    }
    return (SInt32)first;
}

SelectorMethod *write_selector_methods(SelectorMethod *method)
{
#if VERSION > VERSION_GC_1_0
    SelectorMethod *imagePosition;
    SelectorMethod *serializedNext;
#endif
    CPrecWrittenEntry *entry;
    SelectorMethod *first;
    SelectorMethod *current;
    SelectorMethod *next;

    if ((entry = fn_004e0680(method)))
        return entry->image_position;
    add_written_type_entry(method, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(method, sizeof(SelectorMethod));
        if (method->method)
            add_serialized_bucket_entry((SInt32)&current->method, (SInt32)write_methrec(method->method));
        if (!method->next)
            break;
        if ((entry = fn_004e0680(method->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        method = method->next;
        current = next;
        add_written_type_entry(method, next);
#else
        imagePosition = next = align_to_four_byte_boundary();
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(serializedNext = next));
        method = method->next;
        current = next;
        add_written_type_entry(method, imagePosition);
#endif
    }
    return first;
}

HashEntry *write_hash_entry(HashEntry *x)
{
#if VERSION > VERSION_GC_1_0
    HashEntry *value;
    HashEntry *tmp;
#endif
    CPrecWrittenEntry *entry;
    HashEntry *first;
    HashEntry *current;
    HashEntry *next;

    if ((entry = fn_004e0680(x)))
        return entry->image_position;
    add_written_type_entry(x, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(x, 0x10);
        if (x->obj)
            add_serialized_bucket_entry((SInt32)(&current->obj), (SInt32)(write_object(x->obj)));
        patch_hash_name_reference(&current->name, x->name);
        if (x->methods)
            add_serialized_bucket_entry((SInt32)(&current->methods), (SInt32)(write_selector_methods(x->methods)));
        if (!x->next)
            break;
        if ((entry = fn_004e0680(x->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        x = x->next;
        current = next;
        add_written_type_entry(x, next);
#else
        value = tmp = next = align_to_four_byte_boundary();
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)tmp);
        current = next;
        x = x->next;
        add_written_type_entry(x, value);
#endif
    }
    return first;
}

MethRec *write_methrec(MethRec *method)
{
#if VERSION > VERSION_GC_1_0
#endif
    CPrecWrittenEntry *entry;
    MethRec *first;
    MethRec *current;
    MethRec *next;

    if ((entry = fn_004e0680(method)))
        return entry->image_position;
    add_written_type_entry(method, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(method, sizeof(MethRec));
        if (method->function)
            add_serialized_bucket_entry((SInt32)&current->function, write_object(method->function));
        if (method->ftype)
            add_serialized_bucket_entry((SInt32)&current->ftype, write_type((Type *)method->ftype));
        if (method->selector)
            add_serialized_bucket_entry((SInt32)&current->selector, (SInt32)write_hash_entry(method->selector));
        if (method->rtype)
            add_serialized_bucket_entry((SInt32)&current->rtype, write_type(method->rtype));
        if (method->args)
            add_serialized_bucket_entry((SInt32)&current->args, write_objc_parameter_nodes(method->args));
        if (!method->next)
            break;
        if ((entry = fn_004e0680(method->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        method = method->next;
        current = next;
        add_written_type_entry(method, next);
#else
        {
            MethRec *tmp;
            MethRec *saved;
            MethRec *held;
            next = align_to_four_byte_boundary();
            held = saved = tmp = next;
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)saved);
            method = method->next;
            current = next;
            add_written_type_entry(method, held);
        }
#endif
    }
    return first;
}

struct CRec *write_crec(struct CRec *record)
{
#if VERSION > VERSION_GC_1_0
    union {
        CRec *ptr;
        SInt32 value;
    } saved;
    SInt32 position;
#endif
    CPrecWrittenEntry *entry;
    CRec *first;
    CRec *current;
    CRec *next;

    if ((entry = fn_004e0680(record)))
        return entry->image_position;
    add_written_type_entry(record, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(record, sizeof(CRec));
        patch_hash_name_reference(&current->name, record->name);
        if (record->bases)
            add_serialized_bucket_entry((SInt32)&current->bases, (SInt32)write_object_list(record->bases));
        if (record->methods)
            add_serialized_bucket_entry((SInt32)&current->methods, (SInt32)write_methrec(record->methods));
        if (record->info)
            add_serialized_bucket_entry((SInt32)&current->info, (SInt32)write_object(record->info));
        if (!record->next)
            break;
        if ((entry = fn_004e0680(record->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        record = record->next;
        current = next;
        add_written_type_entry(record, next);
#else
        next = align_to_four_byte_boundary();
        saved.value = position = (SInt32)next;
        add_serialized_bucket_entry((SInt32)&current->next, position);
        record = record->next;
        current = next;
        add_written_type_entry(record, (CRec *)saved.value);
        saved.value = 0;
#endif
    }
    return first;
}

ObjectList *write_object_list(ObjectList *x)
{
    CPrecWrittenEntry *e;
    ObjectList *first;
    ObjectList *current;
    ObjectList *next;

    if ((e = fn_004e0680(x)))
        return e->image_position;
    add_written_type_entry(x, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(x, sizeof(ObjectList));
        add_serialized_bucket_entry((SInt32)(&current->object), (SInt32)(write_crec((CRec *)x->object)));
        if (!x->next)
            break;
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        x = x->next;
    }
    return first;
}

CRec *write_crec_list(CRec *record)
{
#if VERSION > VERSION_GC_1_0
    CRec *value;
    CRec *tmp;
#endif
    CPrecWrittenEntry *entry;
    CRec *first;
    CRec *current;
    CRec *next;

    if ((entry = fn_004e0680(record)))
        return entry->image_position;
    add_written_type_entry(record, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(record, 0x10);
        patch_hash_name_reference(&current->name, record->name);
        if (record->bases)
            add_serialized_bucket_entry((SInt32)&current->bases, (SInt32)write_object_list(record->bases));
        if (record->methods)
            add_serialized_bucket_entry((SInt32)&current->methods, (SInt32)write_methrec(record->methods));
        if (!record->next)
            break;
        if ((entry = fn_004e0680(record->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        record = record->next;
        current = next;
        add_written_type_entry(record, next);
#else
        value = tmp = next = align_to_four_byte_boundary();
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)tmp);
        current = next;
        record = record->next;
        add_written_type_entry(record, value);
#endif
    }
    return first;
}

unsigned int serialize_objc_info(struct ObjCInfo *info)
{
    struct ObjCInfo *p;

    p = align_to_four_byte_boundary();
    fn_004e0010(info, 0x1a);
    if (info->classobject != NULL)
        add_serialized_bucket_entry((SInt32)&p->classobject, write_object(info->classobject));
    if (info->metaclassobject != NULL)
        add_serialized_bucket_entry((SInt32)&p->metaclassobject, write_object(info->metaclassobject));
    if (info->auxiliaryObject != NULL)
        add_serialized_bucket_entry((SInt32)&p->auxiliaryObject, write_object(info->auxiliaryObject));
    if (info->methods != NULL)
        add_serialized_bucket_entry((SInt32)&p->methods, (SInt32)write_methrec(info->methods));
    if (info->protocols != NULL)
        add_serialized_bucket_entry((SInt32)&p->protocols, (SInt32)write_object_list(info->protocols));
    if (info->vars != NULL)
        add_serialized_bucket_entry((SInt32)&p->vars, (SInt32)write_crec_list(info->vars));
    return (unsigned int)p;
}

SInt32 serialize_ct_state_elems(TemplArg *element)
{
    TemplArg *first;
    TemplArg *current;
    TemplArg *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        fn_004e0010(element, sizeof(TemplArg));
        if (element->pid.type) {
            if (element->data.typeparam.type)
                add_serialized_bucket_entry((SInt32)&current->data, write_type(element->data.typeparam.type));
        } else {
            if (element->data.paramdecl.expr)
                add_serialized_bucket_entry((SInt32)&current->data, write_enode(element->data.paramdecl.expr));
        }
        if (element->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        element = element->next;
    }
    return (SInt32)first;
}

SInt32 serialize_pre_nodes(TemplParam *node)
{
    TemplParam *first;
    TemplParam *current;
    TemplParam *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        fn_004e0010(node, sizeof(TemplParam));
        if (node->name != NULL)
            patch_hash_name_reference(&current->name, node->name);
        if (node->pid.type != 0) {
            if (node->data.typeparam.type != NULL)
                add_serialized_bucket_entry((SInt32)&current->data.typeparam.type, write_type(node->data.typeparam.type));
        } else {
            add_serialized_bucket_entry((SInt32)&current->data.paramdecl.type, write_type(node->data.paramdecl.type));
            if (node->data.paramdecl.defaultarg != NULL)
                add_serialized_bucket_entry((SInt32)&current->data.paramdecl.defaultarg,
                                            write_enode(node->data.paramdecl.defaultarg));
        }
        if (node->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        node = node->next;
    }
    return (SInt32)first;
}

TStreamElement *append_saved_prep_tokens(TStreamElement *recs, SInt32 n)
{
    TStreamElement *rp;
    TStreamElement *first;
    TStreamElement *bp;
    TStreamElement tmp;
    SavedPrepTokenList *node;
    SInt32 i;

    for (rp = recs, i = 0; i < n; i++, rp++) {
        tmp = *rp;
        memclrw(rp, sizeof(TStreamElement));
        switch (rp->tokentype = tmp.tokentype) {
            case -3:
                rp->data.tkstring.data = tmp.data.tkstring.data;
                break;
            case -1:
                rp->subtype = tmp.subtype;
                rp->data = tmp.data;
                break;
            case -2:
                rp->subtype = tmp.subtype;
                rp->data = tmp.data;
                break;
            case -5:
            case -4:
                rp->subtype = tmp.subtype;
                rp->data = tmp.data;
                break;
        }
    }

    first = bp = align_to_four_byte_boundary();
    fn_004e0010(recs, n * sizeof(TStreamElement));
    if (data_00581c28) {
        node = lalloc(sizeof(SavedPrepTokenList));
        node->offset = bp;
        node->count = n;
        node->next = saved_prep_tokens;
        saved_prep_tokens = node;
    }

    for (rp = recs, i = 0; i < n; i++, rp++, bp++) {
        switch (rp->tokentype) {
            case -3:
                patch_hash_name_reference(&bp->data.tkstring.data, (HashNameNode *)rp->data.tkstring.data);
                break;
            case -5:
            case -4:
                CPrec_RawMemPatch(&bp->data.tkstring.data, rp->data.tkstring.data, rp->data.tkstring.size);
                break;
            case -7:
            case -2:
            case -1:
                break;
            default:
                CError_ASSERT(1974, rp->tokentype >= 0);
                break;
        }
    }

    return first;
}

static CPREC_INLINE TemplFuncInstance *write_templ_func_instances(TemplFuncInstance *inst)
{
    TemplFuncInstance *first;
    TemplFuncInstance *current;
    TemplFuncInstance *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(inst, sizeof(TemplFuncInstance));
        add_serialized_bucket_entry((SInt32)&current->object, write_object(inst->object));
        add_serialized_bucket_entry((SInt32)&current->args, serialize_ct_state_elems(inst->args));
        if (!inst->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        inst = inst->next;
    }
    return first;
}

UInt32 serialize_prec_records(TemplateMember *record)
{
    TemplateMember *first;
    TemplateMember *current;
    TemplateMember *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        memclrw(&record->fileoffset.file,
                offsetof(TemplateMember, stream.tokens) - offsetof(TemplateMember, fileoffset.file));
        record->srcfile = NULL;
        record->startoffset = NULL;
        record->endoffset = NULL;
        fn_004e0010(record, 0x2a);
        if (record->params != NULL)
            add_serialized_bucket_entry((SInt32)&current->params, serialize_pre_nodes(record->params));
        add_serialized_bucket_entry((SInt32)&current->object, write_object(record->object));
        if (record->stream.firsttoken != NULL)
            add_serialized_bucket_entry((SInt32)&current->stream.firsttoken,
                                        (SInt32)append_saved_prep_tokens(record->stream.firsttoken, record->stream.tokens));
        if (record->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        record = record->next;
    }
    return (UInt32)first;
}

unsigned int serialize_reference_entries(TemplPartialSpec *record)
{
    TemplPartialSpec *first;
    TemplPartialSpec *current;
    TemplPartialSpec *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(record, sizeof(TemplPartialSpec));
        if (record->templ != NULL)
            add_serialized_bucket_entry((SInt32)&current->templ, write_type((Type *)record->templ));
        if (record->args != NULL)
            add_serialized_bucket_entry((SInt32)&current->args, serialize_ct_state_elems(record->args));
        if (record->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        record = record->next;
    }
    return (unsigned int)first;
}

unsigned int write_prec_input_record(TemplateFriend *record)
{
    TemplateFriend *p;

    memclrw(&record->fileoffset, sizeof(record->fileoffset));
    p = align_to_four_byte_boundary();
    fn_004e0010(record, sizeof(TemplateFriend));
    if (record->decl.thetype != NULL)
        add_serialized_bucket_entry((SInt32)&p->decl.thetype, write_type(record->decl.thetype));
    if (record->decl.nspace != NULL)
        add_serialized_bucket_entry((SInt32)&p->decl.nspace, get_namespace_patch(record->decl.nspace));
    if (record->decl.name != NULL)
        patch_hash_name_reference(&p->decl.name, record->decl.name);
    if (record->decl.expltargs != NULL)
        add_serialized_bucket_entry((SInt32)&p->decl.expltargs, serialize_ct_state_elems(record->decl.expltargs));
    if (record->stream.firsttoken != NULL)
        add_serialized_bucket_entry((SInt32)&p->stream.firsttoken,
                                    (SInt32)append_saved_prep_tokens(record->stream.firsttoken, record->stream.tokens));
    return (unsigned int)p;
}

SInt32 serialize_template_class_declarations(TemplateAction *list)
{
    TemplateAction *first;
    TemplateAction *current;
    TemplateAction *next;

    first = current = align_to_four_byte_boundary();
    for (;;) {
        memclrw(&list->source_ref, sizeof(list->source_ref));
        fn_004e0010(list, sizeof(TemplateAction));
        switch (list->type) {
            case TAT_NESTEDCLASS:
                add_serialized_bucket_entry((SInt32)&current->u.tclasstype, write_type(TYPE(list->u.tclasstype)));
                break;
            case TAT_ENUMTYPE:
                add_serialized_bucket_entry((SInt32)&current->u.enumtype, write_type(TYPE(list->u.enumtype)));
                break;
            case TAT_FRIEND:
                add_serialized_bucket_entry((SInt32)&current->u.tfriend, write_prec_input_record(list->u.tfriend));
                break;
            case TAT_ENUMERATOR:
                add_serialized_bucket_entry((SInt32)&current->u.enumerator.objenumconst,
                                            write_enum_const(list->u.enumerator.objenumconst));
                if (list->u.enumerator.initexpr)
                    add_serialized_bucket_entry((SInt32)&current->u.enumerator.initexpr,
                                                write_enode(list->u.enumerator.initexpr));
                break;
            case TAT_BASE:
                add_serialized_bucket_entry((SInt32)&current->u.base.type, write_type(list->u.base.type));
                if (list->u.base.insert_after)
                    add_serialized_bucket_entry((SInt32)&current->u.base.insert_after,
                                                (SInt32)write_class_list(list->u.base.insert_after));
                break;
            case TAT_OBJECTINIT:
                add_serialized_bucket_entry((SInt32)&current->u.objectinit.object, write_object(list->u.objectinit.object));
                add_serialized_bucket_entry((SInt32)&current->u.objectinit.initexpr,
                                            write_enode(list->u.objectinit.initexpr));
                break;
            case TAT_USINGDECL:
                add_serialized_bucket_entry((SInt32)&current->u.usingdecl.type, write_templdep(list->u.usingdecl.type));
                break;
            case TAT_OBJECTDEF:
                add_serialized_bucket_entry((SInt32)&current->u.refobj, dispatch_obj_by_otype(list->u.refobj));
                break;
            default:
                CError_FATAL(2313);
                break;
        }
        if (list->next == NULL)
            break;
        add_serialized_bucket_entry((SInt32)current, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        list = list->next;
    }
    return (SInt32)first;
}

TemplateFunction *write_template_function(TemplateFunction *function)
{
#if VERSION == VERSION_GC_1_0
    CPrecWrittenEntry *entry;
    TemplateFunction *first;
    TemplateFunction *current;
    TemplateFunction *next;
#else
    CPrecWrittenEntry *entry;
    TemplFuncInstance *firstObject;
    TemplateFunction *nextImage;
    TemplFuncInstance *currentObject;
    TemplFuncInstance *object;
    TemplateFunction *next;
    TemplateFunction *current;
    TemplateFunction *first;
#endif

    if ((entry = fn_004e0680(function)))
        return entry->image_position;
    add_written_type_entry(function, first = current = align_to_four_byte_boundary());
    while (1) {
        memclrw(&function->deftoken, sizeof(function->deftoken));
        function->srcfile = NULL;
        function->startoffset = 0;
        function->endoffset = 0;
        fn_004e0010(function, sizeof(TemplateFunction));
        if (function->original)
            add_serialized_bucket_entry((SInt32)&current->original, (SInt32)write_template_function(function->original));
        patch_hash_name_reference(&current->name, function->name);
        if (function->params)
            add_serialized_bucket_entry((SInt32)&current->params, serialize_pre_nodes(function->params));
        if (function->stream.firsttoken)
            add_serialized_bucket_entry((SInt32)&current->stream.firsttoken,
                                        (SInt32)append_saved_prep_tokens(function->stream.firsttoken, function->stream.tokens));
        add_serialized_bucket_entry((SInt32)&current->tfunc, write_object(function->tfunc));
#if VERSION == VERSION_GC_1_0
        if (function->instances)
            add_serialized_bucket_entry((SInt32)&current->instances, (SInt32)write_templ_func_instances(function->instances));
#else
        if ((object = function->instances)) {
            TemplFuncInstance *nextObject;
            firstObject = currentObject = align_to_four_byte_boundary();
            while (1) {
                fn_004e0010(object, sizeof(TemplFuncInstance));
                add_serialized_bucket_entry((SInt32)&currentObject->object, write_object(object->object));
                add_serialized_bucket_entry((SInt32)&currentObject->args, serialize_ct_state_elems(object->args));
                if (!object->next)
                    break;
                add_serialized_bucket_entry((SInt32)&currentObject->next, (SInt32)(nextObject = align_to_four_byte_boundary()));
                currentObject = nextObject;
                object = object->next;
            }
            add_serialized_bucket_entry((SInt32)&current->instances, (SInt32)firstObject);
        }
#endif
        if (!function->next)
            break;
        if ((entry = fn_004e0680(function->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        function = function->next;
        add_written_type_entry(function, next);
#else
        {
            TemplateFunction *image;
            add_serialized_bucket_entry((SInt32)&current->next,
                                        (SInt32)(nextImage = next = align_to_four_byte_boundary(), image = next));
            current = next;
            function = function->next;
            add_written_type_entry(function, nextImage);
        }
#endif
    }
    return first;
}

UInt32 write_typeclass(TypeClass *node)
{
    CPrecWrittenEntry *entry;
    TypeClass *first;
    TypeClass *current;
    TypeClass *next;
    TypeClass *nextInst;
    Boolean hasNextInst;
    Boolean hasNextTempl;

    first = current = align_to_four_byte_boundary();
    while (1) {
        hasNextTempl = hasNextInst = 0;
        add_written_type_entry(node, current);
        if (node->flags & CLASS_IS_TEMPL) {
            fn_004e0010(node, sizeof(TemplClass));
            if (TEMPL_CLASS(node)->next)
                hasNextTempl = 1;
            if (TEMPL_CLASS(node)->templ_parent)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->templ_parent,
                                            write_type(TYPE(TEMPL_CLASS(node)->templ_parent)));
            if (TEMPL_CLASS(node)->inst_parent)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->inst_parent,
                                            write_type(TYPE(TEMPL_CLASS(node)->inst_parent)));
            if (TEMPL_CLASS(node)->templ__params)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->templ__params,
                                            serialize_pre_nodes(TEMPL_CLASS(node)->templ__params));
            if (TEMPL_CLASS(node)->members)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->members,
                                            serialize_prec_records(TEMPL_CLASS(node)->members));
            if (TEMPL_CLASS(node)->instances)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->instances,
                                            write_type(TYPE(TEMPL_CLASS(node)->instances)));
            if (TEMPL_CLASS(node)->pspec_owner)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->pspec_owner,
                                            write_type(TYPE(TEMPL_CLASS(node)->pspec_owner)));
            if (TEMPL_CLASS(node)->pspecs)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->pspecs,
                                            serialize_reference_entries(TEMPL_CLASS(node)->pspecs));
            if (TEMPL_CLASS(node)->actions)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->actions,
                                            serialize_template_class_declarations(TEMPL_CLASS(node)->actions));
        } else if (node->flags & CLASS_IS_TEMPL_INST) {
            fn_004e0010(node, sizeof(TemplClassInst));
            if (TEMPL_CLASS_INST(node)->next)
                hasNextInst = 1;
            if (TEMPL_CLASS_INST(node)->parent)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS_INST(current)->parent,
                                            write_type(TYPE(TEMPL_CLASS_INST(node)->parent)));
            if (TEMPL_CLASS_INST(node)->templ)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS_INST(current)->templ,
                                            write_type(TYPE(TEMPL_CLASS_INST(node)->templ)));
            if (TEMPL_CLASS_INST(node)->inst_args)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS_INST(current)->inst_args,
                                            serialize_ct_state_elems(TEMPL_CLASS_INST(node)->inst_args));
            if (TEMPL_CLASS_INST(node)->oargs)
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS_INST(current)->oargs,
                                            serialize_ct_state_elems(TEMPL_CLASS_INST(node)->oargs));
        } else {
            fn_004e0010(node, sizeof(TypeClass));
        }
        if (node->nspace)
            add_serialized_bucket_entry((SInt32)&current->nspace, get_namespace_patch(node->nspace));
        if (node->classname)
            patch_hash_name_reference(&current->classname, node->classname);
        if (node->bases)
            add_serialized_bucket_entry((SInt32)&current->bases, (SInt32)write_class_list(node->bases));
        if (node->vbases)
            add_serialized_bucket_entry((SInt32)&current->vbases, write_vclasslist(node->vbases));
        if (node->ivars)
            add_serialized_bucket_entry((SInt32)&current->ivars, serialize_membervars(node->ivars));
        if (node->friends)
            add_serialized_bucket_entry((SInt32)&current->friends, serialize_entry_list(node->friends));
        if (node->vtable)
            add_serialized_bucket_entry((SInt32)&current->vtable, write_vtable(node->vtable));
        if (node->sominfo)
            add_serialized_bucket_entry((SInt32)&current->sominfo, write_som_info(node->sominfo));
        if (node->objcinfo)
            add_serialized_bucket_entry((SInt32)&current->objcinfo, serialize_objc_info(node->objcinfo));
        if (hasNextTempl) {
            if (!(entry = fn_004e0680(TEMPL_CLASS(node)->next))) {
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->next, (SInt32)(next = align_to_four_byte_boundary()));
                current = next;
                node = TYPE_CLASS(TEMPL_CLASS(node)->next);
                continue;
            }
            add_serialized_bucket_entry((SInt32)&TEMPL_CLASS(current)->next, (SInt32)entry->image_position);
        }
        if (hasNextInst) {
            if (!(entry = fn_004e0680(TEMPL_CLASS_INST(node)->next))) {
                add_serialized_bucket_entry((SInt32)&TEMPL_CLASS_INST(current)->next, (SInt32)(nextInst = align_to_four_byte_boundary()));
                current = nextInst;
                node = TYPE_CLASS(TEMPL_CLASS_INST(node)->next);
                continue;
            }
            add_serialized_bucket_entry((SInt32)&TEMPL_CLASS_INST(current)->next, (SInt32)entry->image_position);
        }
        break;
    }
    return (UInt32)first;
}

int write_type(Type *type)
{
    CPrecWrittenEntry *entry;

    if ((entry = fn_004e0680(type)))
        return (int)entry->image_position;
    switch ((signed char)type->type) {
        case TYPEPOINTER:
        case TYPEARRAY:
            return write_pointer_type((TypePointer *)type);
        case TYPEENUM:
            return (int)append_type_enum((TypeEnum *)type);
        case TYPEBITFIELD:
            return serialize_type_bitfield((TypeBitfield *)type);
        case TYPESTRUCT:
            return (int)append_type_struct((TypeStruct *)type);
        case TYPEFUNC:
            return (int)copy_type_func((TypeFunc *)type);
        case TYPEMEMBERPOINTER:
            return (int)append_member_pointer_type((TypeMemberPointer *)type);
        case TYPETEMPLATE:
            return write_templdep((TypeTemplDep *)type);
        case TYPECLASS:
            return write_typeclass((TypeClass *)type);
        case TYPEVOID:
        case TYPETEMPLDEPEXPR:
        default:
            CError_FATAL(2691);
            return 0;
    }
}

SInt32 serialize_cpsi_list(ExceptionAction *item)
{
    ExceptionAction *first;
    ExceptionAction *current;
    int i;
    ExceptionAction *next;
    Object **objects;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(item, 0x1e);
        switch (item->kind) {
            case EAT_DESTROYLOCAL:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_DESTROYLOCALCOND:
                add_serialized_bucket_entry((SInt32)&current->data.local_cond.dtor,
                                            write_object(item->data.local_cond.dtor));
                break;
            case EAT_DESTROYLOCALOFFSET:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_DESTROYLOCALPOINTER:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_DESTROYLOCALARRAY:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case 6:
                break;
            case EAT_DESTROYMEMBER:
            case EAT_DESTROYBASE:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_DESTROYMEMBERCOND:
                add_serialized_bucket_entry((SInt32)&current->data.local_cond.dtor,
                                            write_object(item->data.local_cond.dtor));
                break;
            case EAT_DESTROYMEMBERARRAY:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_DELETEPOINTER:
            case EAT_DELETELOCALPOINTER:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_DELETEPOINTERCOND:
                add_serialized_bucket_entry((SInt32)&current->data.local.dtor, write_object(item->data.local.dtor));
                break;
            case EAT_CATCHBLOCK:
                if (item->data.catch_block.typeInfo) {
                    add_serialized_bucket_entry((SInt32)&current->data.catch_block.typeInfo,
                                                write_object(item->data.catch_block.typeInfo));
                    add_serialized_bucket_entry((SInt32)&current->data.catch_block.exceptionType,
                                                (SInt32)write_type(item->data.catch_block.exceptionType));
                }
                break;
            case EAT_ACTIVECATCHBLOCK:
                break;
            case EAT_SPECIFICATION:
                if (item->data.specification.ids) {
                    add_serialized_bucket_entry((SInt32)&current->data.specification.ids,
                                                (SInt32)(objects = align_to_four_byte_boundary()));
                    fn_004e0010(item->data.specification.ids,
                                              item->data.specification.count * sizeof(Object *));
                    for (i = 0; i < item->data.specification.count; i++)
                        add_serialized_bucket_entry((SInt32)&objects[(UInt32)i],
                                                    write_object(item->data.specification.ids[i]));
                }
                break;
            case EAT_TERMINATE:
                break;
            default:
                CError_FATAL(2800);
                break;
        }
        if (!item->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        item = item->next;
    }
    return (SInt32)first;
}

static CPREC_INLINE ENodeList *write_enode_list(ENodeList *list)
{
    ENodeList *first;
    ENodeList *current;
    ENodeList *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(list, sizeof(ENodeList));
        add_serialized_bucket_entry((SInt32)&current->node, write_enode(list->node));
        if (!list->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        list = list->next;
    }
    return first;
}

unsigned int write_member_func_ref(EMemberInfo *entry)
{
    EMemberInfo *current;

    CError_FATAL(2848);
    current = align_to_four_byte_boundary();
    fn_004e0010(entry, sizeof(EMemberInfo));
    if (entry->path)
        add_serialized_bucket_entry((SInt32)&current->path, write_bclass_list(entry->path));
    if (entry->expr)
        add_serialized_bucket_entry((SInt32)&current->expr, write_enode(entry->expr));
    if (entry->templargs)
        CError_FATAL(2863);
    add_serialized_bucket_entry((SInt32)&current->list, write_namespace_object_list(entry->list));
    return (unsigned int)current;
}

SInt32 write_enode(ENode *node)
{
    ENode *current;

    if (node->type == ETEMPLDEP && node->data.templdep.subtype == TDE_SOURCEREF)
        node->data.objlist.templargs = NULL;

    current = align_to_four_byte_boundary();
    fn_004e0010(node, sizeof(ENode));
    add_serialized_bucket_entry((SInt32)&current->rtype, write_type(node->rtype));

    switch (node->type) {
        case EPOSTINC:
        case EPOSTDEC:
        case EPREINC:
        case EPREDEC:
        case EINDIRECT:
        case EMONMIN:
        case EBINNOT:
        case ELOGNOT:
        case EFORCELOAD:
        case ETYPCON:
        case EBITFIELD:
            add_serialized_bucket_entry((SInt32)&current->data.monadic, write_enode(node->data.monadic));
            break;
        case EMUL:
        case EMULV:
        case EDIV:
        case EMODULO:
        case EADDV:
        case ESUBV:
        case EADD:
        case ESUB:
        case ESHL:
        case ESHR:
        case ELESS:
        case EGREATER:
        case ELESSEQU:
        case EGREATEREQU:
        case EEQU:
        case ENOTEQU:
        case EAND:
        case EXOR:
        case EOR:
        case ELAND:
        case ELOR:
        case EASS:
        case EMULASS:
        case EDIVASS:
        case EMODASS:
        case EADDASS:
        case ESUBASS:
        case ESHLASS:
        case ESHRASS:
        case EANDASS:
        case EXORASS:
        case EORASS:
        case ECOMMA:
        case EPMODULO:
        case EROTL:
        case EROTR:
        case EBCLR:
        case EBTST:
        case EBSET:
            add_serialized_bucket_entry((SInt32)&current->data.diadic.left, write_enode(node->data.diadic.left));
            add_serialized_bucket_entry((SInt32)&current->data.diadic.right, write_enode(node->data.diadic.right));
            break;
        case ECOND:
            add_serialized_bucket_entry((SInt32)&current->data.cond.cond, write_enode(node->data.cond.cond));
            add_serialized_bucket_entry((SInt32)&current->data.cond.expr1, write_enode(node->data.cond.expr1));
            add_serialized_bucket_entry((SInt32)&current->data.cond.expr2, write_enode(node->data.cond.expr2));
            break;
        case ESTRINGCONST:
            CPrec_RawMemPatch(&current->data.string.data, node->data.string.data, node->data.string.size);
            break;
        case EOBJREF:
            add_serialized_bucket_entry((SInt32)&current->data.objref, write_object(node->data.objref));
            break;
        case EOBJLIST:
            add_serialized_bucket_entry((SInt32)&current->data.objlist.list,
                                        write_namespace_object_list(node->data.objlist.list));
            CError_ASSERT(2938, node->data.objlist.templargs == 0);
            if (node->data.objlist.name)
                patch_hash_name_reference(&current->data.objlist.name, node->data.objlist.name);
            break;
        case EMFPOINTER:
            add_serialized_bucket_entry((SInt32)&current->data.diadic.left, write_enode(node->data.diadic.left));
            add_serialized_bucket_entry((SInt32)&current->data.diadic.right, write_enode(node->data.diadic.right));
            break;
        case ENULLCHECK:
            add_serialized_bucket_entry((SInt32)&current->data.diadic.left, write_enode(node->data.diadic.left));
            add_serialized_bucket_entry((SInt32)&current->data.diadic.right, write_enode(node->data.diadic.right));
            break;
        case ETEMP:
            add_serialized_bucket_entry((SInt32)&current->data.temp.type, write_type(node->data.temp.type));
            break;
        case EFUNCCALL:
        case EFUNCCALLP:
            add_serialized_bucket_entry((SInt32)&current->data.funccall.funcref, write_enode(node->data.funccall.funcref));
            add_serialized_bucket_entry((SInt32)&current->data.funccall.functype, write_type(TYPE(node->data.funccall.functype)));
            if (node->data.funccall.args)
                add_serialized_bucket_entry((SInt32)&current->data.funccall.args, (SInt32)write_enode_list(node->data.funccall.args));
            break;
        case EMEMBER:
            add_serialized_bucket_entry((SInt32)&current->data.emember, write_member_func_ref(node->data.emember));
            break;
        case ETEMPLDEP:
            switch (node->data.templdep.subtype) {
                case TDE_SIZEOF:
                    add_serialized_bucket_entry((SInt32)&current->data.templdep.u.typeexpr.type, write_type(node->data.templdep.u.typeexpr.type));
                    break;
                case TDE_CAST:
                    if (node->data.templdep.u.cast.args)
                        add_serialized_bucket_entry((SInt32)&current->data.templdep.u.cast.args,
                                                    (SInt32)write_enode_list(node->data.templdep.u.cast.args));
                    add_serialized_bucket_entry((SInt32)&current->data.templdep.u.cast.type, write_type(node->data.templdep.u.cast.type));
                    break;
                case TDE_QUALNAME:
                    patch_hash_name_reference(&current->data.templdep.u.qual.name, node->data.templdep.u.qual.name);
                    add_serialized_bucket_entry((SInt32)&current->data.templdep.u.qual.type,
                                                write_type(TYPE(node->data.templdep.u.qual.type)));
                    break;
                case TDE_OBJ:
                    add_serialized_bucket_entry((SInt32)&current->data.templdep.u.obj, write_object(node->data.templdep.u.obj));
                    break;
                case TDE_SOURCEREF:
                    add_serialized_bucket_entry((SInt32)&current->data.templdep.u.sourceref.expr, write_enode(node->data.templdep.u.sourceref.expr));
                    break;
                case 0:
                    break;
                default:
                    CError_FATAL(3025);
                    break;
            }
            break;
        case EINTCONST:
        case EFLOATCONST:
        case EPRECOMP:
        case EARGOBJ:
        case ELOCOBJ:
        case ELABEL:
        case EINSTRUCTION:
        case EVECTOR128CONST:
            break;
        default:
            CError_FATAL(3031);
            break;
    }
    return (SInt32)current;
}

#pragma auto_inline off
unsigned int write_precompiled_expression_record(InlineSwitchData *record)
{
    InlineSwitchData *current;

    current = align_to_four_byte_boundary();
    fn_004e0010(record, sizeof(InlineSwitchData) + (record->caseCount - 1) * sizeof(CIBEntry));
    add_serialized_bucket_entry((SInt32)&current->expression, write_enode(record->expression));
    add_serialized_bucket_entry((SInt32)&current->valueType, write_type(record->valueType));
    return (unsigned int)current;
}
#pragma auto_inline reset

#if VERSION == VERSION_GC_1_0
static void *write_asm_objects(ParsedAsmInstruction *data, SInt32 size)
{
    void *start;
    SInt32 i;
    SInt32 offset;
    Object *object;

    start = align_to_four_byte_boundary();
    fn_004e0010(data, size);
    i = 0;
    while (1) {
        if (!(object = InlineAsm_GetObjectByIndex(data, i, &offset)))
            break;
        add_serialized_bucket_entry((SInt32)start + offset, write_object(object));
        i++;
    }
    return start;
}
#endif

#if VERSION == VERSION_GC_1_0
UInt32 write_prec_recs(IStmtRec *recs, SInt16 count)
{
    IStmtRec *first;
    IStmtRec *current;
    SInt16 i;

    first = current = align_to_four_byte_boundary();
    fn_004e0010(recs, count * sizeof(IStmtRec));
    for (i = 0; i < count; i++, current++, recs++) {
        if (recs->exceptionActions)
            add_serialized_bucket_entry((SInt32)&current->exceptionActions, serialize_cpsi_list(recs->exceptionActions));
        switch (recs->type) {
            case 1:
            case 2:
            case 3:
                break;
            case 4:
            case 8:
            case 12:
            case 13:
            case 14:
                if (recs->data.operand)
                    add_serialized_bucket_entry((SInt32)&current->data.operand, write_enode(recs->data.operand));
                break;
            case 6:
            case 7:
                add_serialized_bucket_entry((SInt32)&current->data.operand, write_enode(recs->data.operand));
                break;
            case 5:
                add_serialized_bucket_entry((SInt32)&current->data.switchInfo,
                                            write_precompiled_expression_record(recs->data.switchInfo));
                break;
            case 16:
                add_serialized_bucket_entry((SInt32)&current->data.assembly,
                                            (SInt32)write_asm_objects(recs->data.assembly, recs->secondaryOperand.assemblyData));
                break;
            default:
                CError_FATAL(3141);
                break;
        }
    }
    return (UInt32)first;
}
#else
UInt32 write_prec_recs(IStmtRec *recs, SInt16 count)
{
    UInt32 recordOffset;
    SInt16 index;
    UInt32 start;
    SInt32 entryOffset;

    start = recordOffset = (UInt32)align_to_four_byte_boundary();
    fn_004e0010(recs, count * sizeof(IStmtRec));
    index = 0;
    if (count > 0) {
        do {
            if (recs->exceptionActions != NULL) {
                add_serialized_bucket_entry(recordOffset + 8, serialize_cpsi_list(recs->exceptionActions));
            }
            switch (recs->type) {
                case 1:
                case 2:
                case 3:
                    break;
                case 4:
                case 8:
                case 12:
                case 13:
                case 14:
                    if (recs->data.operand != NULL) {
                        add_serialized_bucket_entry(recordOffset + 12, write_enode(recs->data.operand));
                    }
                    break;
                case 6:
                case 7:
                    add_serialized_bucket_entry(recordOffset + 12, write_enode(recs->data.operand));
                    break;
                case 5:
                    add_serialized_bucket_entry(recordOffset + 12,
                                                write_precompiled_expression_record(recs->data.switchInfo));
                    break;
                case 16: {
                    UInt32 tableSize = recs->secondaryOperand.assemblyData;
                    ParsedAsmInstruction *table = recs->data.assembly;
                    SInt32 entryIndex;
                    UInt32 tableOffset;
                    Object *object;

                    tableOffset = (UInt32)align_to_four_byte_boundary();
                    fn_004e0010(table, tableSize);
                    entryIndex = 0;
                    for (;;) {
                        object = InlineAsm_GetObjectByIndex(table, entryIndex, &entryOffset);
                        if (object == NULL) {
                            break;
                        }
                        add_serialized_bucket_entry(tableOffset + entryOffset, write_object(object));
                        entryIndex++;
                    }
                    add_serialized_bucket_entry(recordOffset + 12, tableOffset);
                    break;
                }
                default:
                    CError_FATAL(3141);
                    break;
            }
            index++;
            recordOffset += sizeof(*recs);
            recs++;
        } while (index < count);
    }
    return start;
}
#endif

unsigned int serialize_reference_type_entries(CInlineVar *vars, short count)
{
    CInlineVar *first;
    CInlineVar *current;
    short i;

    first = current = align_to_four_byte_boundary();
    fn_004e0010(vars, count * sizeof(CInlineVar));
    for (i = 0; i < count; i++, current++, vars++) {
        patch_hash_name_reference(&current->name, vars->name);
        add_serialized_bucket_entry((SInt32)&current->type, write_type(vars->type));
    }
    return (unsigned int)first;
}

SInt32 serialize_cprec_rec(CInlineInfo *record)
{
    CInlineInfo *current;

    memclrw(&record->fileinfo, sizeof(record->fileinfo));
    record->f1c = 0;
    record->tokenoffset = 0;
    record->tokenline = 0;
    current = align_to_four_byte_boundary();
    fn_004e0010(record, sizeof(CInlineInfo));
    if (record->nargs)
        add_serialized_bucket_entry((SInt32)&current->arginfo, serialize_reference_type_entries(record->arginfo, record->nargs));
    if (record->nlocals)
        add_serialized_bucket_entry((SInt32)&current->localinfo, serialize_reference_type_entries(record->localinfo, record->nlocals));
    if (record->nstmts)
        add_serialized_bucket_entry((SInt32)&current->stmtinfo, write_prec_recs(record->stmtinfo, record->nstmts));
    return (SInt32)current;
}

#if VERSION == VERSION_GC_1_0
SInt32 write_enum_const(ObjEnumConst *enumConst)
{
    CPrecWrittenEntry *entry;
    ObjEnumConst *first;
    ObjEnumConst *current;
    ObjEnumConst *next;

    if ((entry = fn_004e0680(enumConst)))
        return (SInt32)entry->image_position;
    add_written_type_entry(enumConst, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(enumConst, sizeof(ObjEnumConst));
        if (data_00581c28) {
            if (enumConst->access == 255)
                CError_FATAL(3228);
            enumConst->access = 255;
        }
        patch_hash_name_reference(&current->name, enumConst->name);
        add_serialized_bucket_entry((SInt32)&current->type, write_type(enumConst->type));
        if (!enumConst->next)
            break;
        if ((entry = fn_004e0680(enumConst->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        enumConst = enumConst->next;
        add_written_type_entry(enumConst, next);
    }
    return (SInt32)first;
}
#else
SInt32 write_enum_const(ObjEnumConst *enumConst)
{
    HashNameNode *name;
    CPrecWrittenEntry *entry;
    SInt32 result, pos, next, value;

    if ((entry = find_written_entry_by_object(enumConst)) != NULL)
        return (SInt32)entry->image_position;
    add(enumConst, result = pos = align_buffer_to_four_bytes());
    for (;;) {
        append_enum_const(enumConst);
        if (data_00581c28) {
            if (enumConst->access == 255)
                CError_FATAL(3228);
            enumConst->access = 255;
        }
        name = enumConst->name;
        name->id = 1;
        patch_object_reference(pos + 6, name);
        add_serialized_bucket_entry(pos + 10, write_type(enumConst->type));
        if (!enumConst->next)
            break;
        if ((entry = find_written_entry_by_object(enumConst->next)) != NULL) {
            add_serialized_bucket_entry(pos + 2, (SInt32)entry->image_position);
            break;
        }
        next = align_buffer_to_four_bytes();
        value = next;
        add_serialized_bucket_entry(pos + 2, next);
        enumConst = enumConst->next;
        pos = next;
        add(enumConst, value);
    }
    return result;
}
#endif

#pragma auto_inline off
ObjType *write_objtype(ObjType *x)
{
    CPrecWrittenEntry *entry;
    ObjType *current;

    if ((entry = fn_004e0680(x)))
        return entry->image_position;
    add_written_type_entry(x, current = align_to_four_byte_boundary());
    fn_004e0010(x, sizeof(ObjType));
    add_serialized_bucket_entry((SInt32)&current->type, write_type(x->type));
    return current;
}
#pragma auto_inline reset

#pragma auto_inline off
ObjType *append_objtype_image(ObjType *x)
{
    CPrecWrittenEntry *entry;
    ObjType *current;

    if ((entry = fn_004e0680(x)))
        return entry->image_position;
    add_written_type_entry(x, current = align_to_four_byte_boundary());
    fn_004e0010(x, 6);
    add_serialized_bucket_entry((SInt32)&current->type, write_type(x->type));
    return current;
}
#pragma auto_inline reset

#if VERSION == VERSION_GC_1_0
static CPREC_INLINE ObjNameSpace *CPrec_GetObjNameSpacePatch(ObjNameSpace *obj)
{
    CPrecWrittenEntry *entry;
    ObjNameSpace *current;

    if ((entry = fn_004e0680(obj)))
        return entry->image_position;
    add_written_type_entry(obj, current = align_to_four_byte_boundary());
    fn_004e0010(obj, sizeof(ObjNameSpace));
    add_serialized_bucket_entry((SInt32)&current->nspace, get_namespace_patch(obj->nspace));
    return current;
}
#else
static CPREC_INLINE ObjNameSpace *CPrec_GetObjNameSpacePatch(ObjNameSpace *obj)
{
    CPrecWrittenEntry *e;
    ObjNameSpace *p;

    if ((e = CPrec_FindAddrPatch(obj)))
        return e->image_position;
    CPrec_NewAddrPatch(obj, p = CPrec_AppendAlign());
    CPrec_AppendData(obj, 6);
    add_serialized_bucket_entry((SInt32)(&p->nspace), get_namespace_patch(obj->nspace));
    return p;
}
#endif

#if VERSION == VERSION_GC_1_0
SInt32 serialize_membervars(ObjMemberVar *object)
{
    CPrecWrittenEntry *entry;
    ObjMemberVar *first;
    ObjMemberVar *current;
    ObjMemberVar *next;

    if ((entry = fn_004e0680(object)))
        return (SInt32)entry->image_position;
    add_written_type_entry(object, first = current = align_to_four_byte_boundary());
    while (1) {
        if (object->has_path) {
            fn_004e0010(object, sizeof(ObjMemberVarPath));
            if (((ObjMemberVarPath *)object)->path)
                add_serialized_bucket_entry((SInt32)&((ObjMemberVarPath *)current)->path,
                                            write_bclass_list(((ObjMemberVarPath *)object)->path));
        } else {
            fn_004e0010(object, sizeof(ObjMemberVar));
        }
        if (object->name)
            patch_hash_name_reference(&current->name, object->name);
        add_serialized_bucket_entry((SInt32)&current->type, write_type(object->type));
        if (!object->next)
            break;
        if ((entry = fn_004e0680(object->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        object = object->next;
        add_written_type_entry(object, next);
    }
    return (SInt32)first;
}
#else
SInt32 serialize_membervars(ObjMemberVar *object)
{
    SInt32 offset;
    ObjectOffsetEntry *entry;
    HashNameNode *reference;
    ObjectOffsetEntry *nextEntry;
    ObjectOffsetEntry **bucket;
    ObjectOffsetEntry *newEntry;
    ObjectOffsetEntry *nextNewEntry;
    SInt32 nextOffset;
    int allocatedOffset;
    SInt32 firstOffset;
    unsigned long initialOffset;
    if ((entry = fn_004da6c0_inline1(object)) != NULL) {
        return entry->offset;
    }
    initialOffset = 0;
    if (data_00581c28 != 0) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    initialOffset = firstOffset = offset = prec_position;
    bucket = (ObjectOffsetEntry **)fn_004da6c0_bucket((int)object);
    newEntry = lalloc(sizeof(ObjectOffsetEntry));
    newEntry->object = object;
    newEntry->offset = initialOffset;
    newEntry->next = *bucket;
    *bucket = newEntry;
    for (;;) {
        if (object->has_path != 0) {
            if (data_00581c28 != 0) {
                AppendGListData(&precompiled_buffer, object, sizeof(ObjMemberVar) + sizeof(BClassList *));
            }
            prec_position += sizeof(ObjMemberVar) + sizeof(BClassList *);
            if (((ObjMemberVarPath *)object)->path != NULL) {
                add_serialized_bucket_entry(offset + 24, write_bclass_list(((ObjMemberVarPath *)object)->path));
            }
        } else {
            if (data_00581c28 != 0) {
                AppendGListData(&precompiled_buffer, object, sizeof(ObjMemberVar));
            }
            prec_position += sizeof(ObjMemberVar);
        }
        if ((reference = object->name) != NULL) {
            reference->id = 1;
            patch_object_reference(offset + 8, reference);
        }
        add_serialized_bucket_entry(offset + 12, write_type(object->type));
        if (object->next == NULL) {
            break;
        }
        if ((nextEntry = fn_004da6c0_inline2(object)) != NULL) {
            add_serialized_bucket_entry(offset + 4, nextEntry->offset);
            break;
        }
        if (data_00581c28 != 0) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        nextOffset = get_next_offset(offset, &allocatedOffset);
        object = object->next;
        offset = nextOffset;
        bucket = (ObjectOffsetEntry **)fn_004da6c0_bucket((int)object);
        nextNewEntry = lalloc(sizeof(ObjectOffsetEntry));
        nextNewEntry->object = object;
        nextNewEntry->offset = allocatedOffset;
        nextNewEntry->next = *bucket;
        *bucket = nextNewEntry;
    }
    return firstOffset;
}
#endif

static CPREC_INLINE DefArgCtorInfo *write_defarg_info(DefArgCtorInfo *defarg)
{
    DefArgCtorInfo *current;

    current = align_to_four_byte_boundary();
    fn_004e0010(defarg, sizeof(DefArgCtorInfo));
    add_serialized_bucket_entry((SInt32)&current->default_func, write_object(defarg->default_func));
    add_serialized_bucket_entry((SInt32)&current->default_arg, write_enode(defarg->default_arg));
    return current;
}

static CPREC_INLINE InlineXRef *write_inline_xrefs(InlineXRef *xref)
{
    InlineXRef *first;
    InlineXRef *current;
    InlineXRef *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(xref, sizeof(InlineXRef) + (xref->numxrefs - 1) * sizeof(xref->xref[0]));
        add_serialized_bucket_entry((SInt32)&current->object, write_object(xref->object));
        if (!xref->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        xref = xref->next;
    }
    return first;
}

UInt32 write_object(Object *obj)
{
    CPrecWrittenEntry *entry;
    Object *current;

    if (fn_0041b910(compiler_plugin_cu.context))
        CError_Longjmp();
    if ((entry = fn_004e0680(obj)))
        return (UInt32)entry->image_position;
    add_written_type_entry(obj, current = align_to_four_byte_boundary());
    obj->dwarfLinks.toc = NULL;
    obj->debugInfo.symbol = NULL;
    if ((obj->qual & Q_IS_TEMPLATED) && obj->datatype != DALIAS) {
        fn_004e0010(obj, sizeof(ObjectTemplated));
        add_serialized_bucket_entry((SInt32)&((ObjectTemplated *)current)->parent,
                                    write_object(((ObjectTemplated *)obj)->parent));
    } else {
        fn_004e0010(obj, sizeof(Object));
    }
    if (obj->nspace)
        add_serialized_bucket_entry((SInt32)&current->nspace, get_namespace_patch(obj->nspace));
    if (obj->name)
        patch_hash_name_reference(&current->name, obj->name);
    add_serialized_bucket_entry((SInt32)&current->type, write_type(obj->type));
    switch (obj->datatype) {
        case DLOCAL:
            CError_FATAL(3459);
            break;
        case DFUNC:
        case DVFUNC:
            if (IS_TYPE_FUNC(obj->type) && (TYPE_FUNC(obj->type)->flags & 0x400))
                add_serialized_bucket_entry((SInt32)&current->u.templateFunction,
                                            (SInt32)write_template_function(obj->u.templateFunction));
            else if ((obj->qual & Q_INLINE) && obj->u.func.u)
                add_serialized_bucket_entry((SInt32)&current->u.func.u, serialize_cprec_rec(obj->u.func.u));
            if (obj->u.func.defargdata)
                add_serialized_bucket_entry((SInt32)&current->u.func.defargdata,
                                            (SInt32)write_defarg_info(obj->u.func.defargdata));
            if (obj->u.func.linkname)
                patch_hash_name_reference(&current->u.func.linkname, obj->u.func.linkname);
            break;
        case DDATA:
            if (obj->u.data.info)
                CError_FATAL(3494);
            if (obj->qual & Q_INLINE_DATA) {
                switch ((SInt8)obj->type->type) {
                    case TYPEFLOAT:
                        CPrec_RawMemPatch(&current->u.data.u.string, obj->u.data.u.string, 8);
                        break;
                    case TYPEINT:
                    case TYPEENUM:
                    case TYPEPOINTER:
                        break;
                    default:
                        CError_FATAL(3510);
                }
            }
            if (obj->u.data.linkname)
                patch_hash_name_reference(&current->u.data.linkname, obj->u.data.linkname);
            break;
        case DINLINEFUNC:
            CPrec_RawMemPatch(&current->u.ifunc.data, obj->u.ifunc.data, obj->u.ifunc.size);
            if (obj->u.ifunc.xrefs)
                add_serialized_bucket_entry((SInt32)&current->u.ifunc.xrefs, (SInt32)write_inline_xrefs(obj->u.ifunc.xrefs));
            break;
        case DALIAS:
            add_serialized_bucket_entry((SInt32)&current->u.alias.object, write_object(obj->u.alias.object));
            if (obj->u.alias.member)
                add_serialized_bucket_entry((SInt32)&current->u.alias.member, write_bclass_list(obj->u.alias.member));
            break;
        case DABSOLUTE:
            break;
        default:
            CError_FATAL(3549);
    }
    if (data_00581c28)
        obj->datatype = 0xff;
    return (UInt32)current;
}

SInt32 dispatch_obj_by_otype(ObjBase *obj)
{
    switch (obj->otype) {
        default:
            CError_FATAL(3566);
        case OT_ENUMCONST:
            return write_enum_const((ObjEnumConst *)obj);
        case OT_TYPE:
            write_objtype((ObjType *)obj);
            return;
        case OT_TYPETAG:
            append_objtype_image((ObjType *)obj);
            return;
        case OT_NAMESPACE:
            return (SInt32)CPrec_GetObjNameSpacePatch((ObjNameSpace *)obj);
        case OT_MEMBERVAR:
            return serialize_membervars((ObjMemberVar *)obj);
        case OT_OBJECT:
            return write_object((Object *)obj);
    }
}

SInt32 write_namespace_object_list(NameSpaceObjectList *list)
{
    CPrecWrittenEntry *entry;
    NameSpaceObjectList *first;
    NameSpaceObjectList *current;
    NameSpaceObjectList *next;
#if VERSION > VERSION_GC_1_0
    NameSpaceObjectList *nextImage;
#endif

    if ((entry = fn_004e0680(list)))
        return (SInt32)entry->image_position;
    add_written_type_entry(list, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(list, sizeof(NameSpaceObjectList));
        add_serialized_bucket_entry((SInt32)&current->object, dispatch_obj_by_otype(list->object));
        if (!list->next)
            break;
        if ((entry = fn_004e0680(list->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
#if VERSION == VERSION_GC_1_0
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        list = list->next;
        add_written_type_entry(list, next);
#else
        {
            NameSpaceObjectList *image;
            add_serialized_bucket_entry((SInt32)&current->next,
                                        (SInt32)(nextImage = next = align_to_four_byte_boundary(), image = next));
            current = next;
            list = list->next;
            add_written_type_entry(list, nextImage);
        }
#endif
    }
    return (SInt32)first;
}

#if VERSION == VERSION_GC_1_0
SInt32 write_namespace_name(NameSpaceName *namespaceName, Boolean allowFlush)
{
    CPrecWrittenEntry *entry;
    NameSpaceName *first;
    NameSpaceName *current;
    NameSpaceName *next;

    if ((entry = fn_004e0680(namespaceName)))
        return (SInt32)entry->image_position;
    add_written_type_entry(namespaceName, first = current = align_to_four_byte_boundary());
    while (1) {
        fn_004e0010(namespaceName, sizeof(NameSpaceName));
        patch_hash_name_reference(&current->name, namespaceName->name);
        add_serialized_bucket_entry((SInt32)&current->first.object, dispatch_obj_by_otype(namespaceName->first.object));
        if (namespaceName->first.next)
            add_serialized_bucket_entry((SInt32)&current->first.next, write_namespace_object_list(namespaceName->first.next));
        if (!namespaceName->next)
            break;
        if ((entry = fn_004e0680(namespaceName->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        namespaceName = namespaceName->next;
        add_written_type_entry(namespaceName, next);
        if (allowFlush && data_00581c28 && checkflush())
            break;
    }
    return (SInt32)first;
}
#else
SInt32 write_namespace_name(NameSpaceName *namespaceName, Boolean allowFlush)
{
    CPrecWrittenEntry *entry;
    SInt32 currentPosition, firstPosition, nextPosition, patchPosition;
    HashNameNode *name;

    if ((entry = find(namespaceName)) != NULL)
        return (SInt32)entry->image_position;
    firstPosition = currentPosition = alignbuf();
    addpatch(namespaceName, patchPosition = currentPosition);
    for (;;) {
        append(namespaceName, sizeof(NameSpaceName));
        name = namespaceName->name;
        name->id = 1;
        patch_object_reference(currentPosition + 4, name);
        add_serialized_bucket_entry(currentPosition + 12, dispatch_obj_by_otype(namespaceName->first.object));
        if (namespaceName->first.next)
            add_serialized_bucket_entry(currentPosition + 8, write_namespace_object_list(namespaceName->first.next));
        if (!namespaceName->next)
            break;
        if ((entry = find(namespaceName->next)) != NULL) {
            add_serialized_bucket_entry(currentPosition, (SInt32)entry->image_position);
            break;
        }
        patchPosition = nextPosition = alignbuf();
        add_serialized_bucket_entry(currentPosition, patchPosition = nextPosition);
        namespaceName = namespaceName->next;
        currentPosition = nextPosition;
        addpatch(namespaceName, patchPosition);
        if (allowFlush && data_00581c28 && checkflush())
            break;
    }
    return firstPosition;
}
#endif

static CPREC_INLINE NameSpaceList *CPrec_GetNSUsingPatch(NameSpaceList *list)
{
    NameSpaceList *first;
    NameSpaceList *current;
    NameSpaceList *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(list, sizeof(NameSpaceList));
        add_serialized_bucket_entry((SInt32)&current->nspace, get_namespace_patch(list->nspace));
        if (!list->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        list = list->next;
    }
    return first;
}

#if VERSION == VERSION_GC_1_0
SInt32 get_namespace_patch(NameSpace *nspace)
{
    CPrecWrittenEntry *entry;
    NameSpace *current;
    SInt32 i;
    NameSpaceName **hashtable;

    if ((entry = fn_004e0680(nspace)))
        return (SInt32)entry->image_position;
    add_written_type_entry(nspace, current = align_to_four_byte_boundary());
    fn_004e0010(nspace, sizeof(NameSpace));
    if (nspace->parent)
        add_serialized_bucket_entry((SInt32)&current->parent, get_namespace_patch(nspace->parent));
    if (nspace->name)
        patch_hash_name_reference(&current->name, nspace->name);
    if (nspace->usings)
        add_serialized_bucket_entry((SInt32)&current->usings, (SInt32)CPrec_GetNSUsingPatch(nspace->usings));
    if (nspace->theclass)
        add_serialized_bucket_entry((SInt32)&current->theclass, write_type(TYPE(nspace->theclass)));
    if (nspace->is_hash) {
        add_serialized_bucket_entry((SInt32)&current->data.hash, (SInt32)(hashtable = align_to_four_byte_boundary()));
        fn_004e0010(nspace->data.hash, 0x400 * sizeof(NameSpaceName *));
        for (i = 0; i < 0x400; i++) {
            if (nspace->data.hash[i])
                add_serialized_bucket_entry((SInt32)&hashtable[(UInt32)i], write_namespace_name(nspace->data.hash[i], 0));
        }
    } else if (nspace->data.list) {
        add_serialized_bucket_entry((SInt32)&current->data.list, write_namespace_name(nspace->data.list, 0));
    }
    return (SInt32)current;
}
#else
SInt32 get_namespace_patch(NameSpace *nspace)
{
    NameSpace *image;
    CPrecWrittenEntry *entry;
    NameSpaceName **hashtable;
    SInt32 i;

    if ((entry = CPrec_FindAddrPatch(nspace)))
        return (SInt32)entry->image_position;
    CPrec_NewAddrPatch(nspace, image = CPrec_AppendAlign());
    CPrec_AppendData(nspace, sizeof(NameSpace));
    if (nspace->parent)
        add_serialized_bucket_entry((SInt32)&image->parent, get_namespace_patch(nspace->parent));
    if (nspace->name)
        CPrec_NamePatch(&image->name, nspace->name);
    if (nspace->usings)
        add_serialized_bucket_entry((SInt32)&image->usings, (SInt32)CPrec_GetNSUsingPatch(nspace->usings));
    if (nspace->theclass)
        add_serialized_bucket_entry((SInt32)&image->theclass, write_type((Type *)nspace->theclass));
    if (nspace->is_hash) {
        hashtable = CPrec_AppendAlign();
        add_serialized_bucket_entry((SInt32)&image->data.hash, (SInt32)hashtable);
        CPrec_AppendData(nspace->data.hash, 0x400 * sizeof(*hashtable));
        for (i = 0; i < 0x400; i++) {
            if (nspace->data.hash[i])
                add_serialized_bucket_entry((SInt32)&hashtable[(UInt32)i],
                                            write_namespace_name(nspace->data.hash[i], 0));
        }
    } else if (nspace->data.list) {
        add_serialized_bucket_entry((SInt32)&image->data.list, write_namespace_name(nspace->data.list, 0));
    }
    return (SInt32)image;
}
#endif

void serialize_namespace_usings_and_hash(void)
{
    SInt32 i;
    SInt32 offset;

    CError_ASSERT(3777, cscope_root->is_hash);
    if (cscope_root->usings) {
        offset = (SInt32)CPrec_GetNSUsingPatch(cscope_root->usings);
        if (data_00581c28)
            prec_header->usingsOffset = offset;
    }
    if (data_00581c28)
        prec_header->nameCount = cscope_root->names;
    for (i = 0; i < 0x400; i++) {
        if (cscope_root->data.hash[i]) {
            offset = write_namespace_name(cscope_root->data.hash[i], 1);
            if (data_00581c28) {
                if (data_00581c26)
                    return;
                prec_header->namespaceNameOffsets[i] = offset;
            }
        }
    }
}

unsigned int write_csomrefnode_list(struct CSOMRefNode *node)
{
    struct CSOMRefNode *first;
    struct CSOMRefNode *current;
    struct CSOMRefNode *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(node, sizeof(struct CSOMRefNode));
        add_serialized_bucket_entry((SInt32)&current->object, write_object(node->object));
        add_serialized_bucket_entry((SInt32)&current->theclass, write_type(TYPE(node->theclass)));
        if (!node->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        node = node->next;
    }
    return (unsigned int)first;
}

static CPREC_INLINE OLinkList *write_olink_list(OLinkList *link)
{
    OLinkList *first;
    OLinkList *current;
    OLinkList *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(link, sizeof(OLinkList));
        add_serialized_bucket_entry((SInt32)&current->obj, write_object(link->obj));
        if (!link->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        link = link->next;
    }
    return first;
}

PendingBuffer *serialize_pending_buffers(PendingBuffer *item)
{
    PendingBuffer *first;
    PendingBuffer *current;
    PendingBuffer *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(item, sizeof(PendingBuffer));
        add_serialized_bucket_entry((SInt32)&current->owner, write_object(item->owner));
        if (item->buffer)
            CPrec_RawMemPatch(&current->buffer, item->buffer, item->owner->type->size);
        if (item->value)
            add_serialized_bucket_entry((SInt32)&current->value, (SInt32)write_olink_list(item->value));
        if (!item->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        item = item->next;
    }
    return first;
}

unsigned int serialize_pending_object_class_list(struct CallbackAction *action)
{
    struct CallbackAction *first;
    struct CallbackAction *current;
    struct CallbackAction *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(action, sizeof(struct CallbackAction));
        add_serialized_bucket_entry((SInt32)&current->obj, write_object(action->obj));
        add_serialized_bucket_entry((SInt32)&current->tclass, write_type(TYPE(action->tclass)));
        if (!action->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        action = action->next;
    }
    return (unsigned int)first;
}

#if VERSION == VERSION_GC_1_0
static CPREC_INLINE HashEntry **CPrec_WriteTable(HashEntry **table)
{
    HashEntry **first;
    HashEntry **current;
    UInt32 i;

    first = current = align_to_four_byte_boundary();
    fn_004e0010(table, 0x400 * sizeof(HashEntry *));
    for (i = 0; i < 0x400; i++, current++) {
        if (table[i])
            add_serialized_bucket_entry((SInt32)current, (SInt32)write_hash_entry(table[i]));
    }
    return first;
}
#else
static inline SInt32 CPrec_WriteTable(void *array)
{
    HashEntry *value;
    SInt32 wpos;
    SInt32 dst;
    UInt32 index;

    CPrec_Pad();
    wpos = dst = prec_position;
    if (data_00581c28 != 0)
        AppendGListData(&precompiled_buffer, array, 0x1000);
    prec_position += 0x1000;
    for (index = 0; index < 0x400; index++, dst += 4) {
        if ((value = ((HashEntry **)array)[index]) != NULL)
            add_serialized_bucket_entry(dst, (SInt32)write_hash_entry(value));
    }
    return wpos;
}
#endif

#if VERSION == VERSION_GC_1_0
static CPREC_INLINE PendingFunction *CPrec_WriteList(PendingFunction *pending)
{
    PendingFunction *first;
    PendingFunction *current;
    PendingFunction *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        fn_004e0010(pending, sizeof(PendingFunction));
        add_serialized_bucket_entry((SInt32)&current->cls, write_object(pending->cls));
        add_serialized_bucket_entry((SInt32)&current->func, write_enode((ENode *)pending->func));
        if (!pending->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        pending = pending->next;
    }
    return first;
}
#else
static inline SInt32 CPrec_WriteList(PendingFunction *node)
{
    SInt32 wpos;
    SInt32 current;
    SInt32 next;

    wpos = current = CPrec_AlignedPosition();
    for (;;) {
        if (data_00581c28 != 0)
            AppendGListData(&precompiled_buffer, node, 0xc);
        prec_position += 0xc;
        add_serialized_bucket_entry(current + 8, write_object(node->cls));
        add_serialized_bucket_entry(current + 4, write_enode((ENode *)node->func));
        if (node->next == NULL)
            break;
        add_serialized_bucket_entry(current, next = CPrec_AlignedPosition());
        current = next;
        node = node->next;
    }
    return wpos;
}
#endif

#if VERSION == VERSION_GC_1_0
UInt32 serialize_cprec_nodes(CPrecNode *node)
{
    CPrecNode *first;
    CPrecNode *current;
    CPrecNode *next;

    first = current = align_to_four_byte_boundary();
    while (1) {
        if (node->kind == 0)
            memclrw(&node->u.k0.location, sizeof(node->u.k0.location));
        fn_004e0010(node, sizeof(CPrecNode));
        add_serialized_bucket_entry((SInt32)&current->obj, write_object(node->obj));
        switch (node->kind) {
            case 0:
                if (node->u.k0.tokenBuffer.firsttoken)
                    add_serialized_bucket_entry((SInt32)&current->u.k0.tokenBuffer.firsttoken,
                                                (SInt32)append_saved_prep_tokens(node->u.k0.tokenBuffer.firsttoken,
                                                                                 node->u.k0.tokenBuffer.tokens));
                if (node->u.k0.contextClass)
                    add_serialized_bucket_entry((SInt32)&current->u.k0.contextClass, write_type(TYPE(node->u.k0.contextClass)));
                break;
            case 1:
                add_serialized_bucket_entry((SInt32)&current->u.k1.classTemplate, write_type(TYPE(node->u.k1.classTemplate)));
                add_serialized_bucket_entry((SInt32)&current->u.k1.context, write_type(TYPE(node->u.k1.context)));
                add_serialized_bucket_entry((SInt32)&current->u.k1.source, serialize_prec_records(node->u.k1.source));
                break;
            case 2:
                add_serialized_bucket_entry((SInt32)&current->u.k2.definition, (SInt32)write_template_function(node->u.k2.definition));
                add_serialized_bucket_entry((SInt32)&current->u.k2.specialization,
                                            (SInt32)write_templ_func_instances(node->u.k2.specialization));
                break;
            case 3:
                break;
            default:
                CError_FATAL(4045);
        }
        if (!node->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = align_to_four_byte_boundary()));
        current = next;
        node = node->next;
    }
    return (UInt32)first;
}
#else
UInt32 serialize_cprec_nodes(CPrecNode *info)
{
    CPrecNode *node = info;
    SInt32 nodeOffset;
    SInt32 result;
    SInt32 specializationOffset;
    SInt32 firstSpecializationOffset;
    TemplFuncInstance *specialization;
    SInt32 nextOffset;

    if (data_00581c28 != 0) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    result = nodeOffset = prec_position;
    for (;;) {
        if (node->kind == 0) {
            memclrw(&node->u.k0.location, sizeof(node->u.k0.location));
        }
        if (data_00581c28 != 0) {
            AppendGListData(&precompiled_buffer, node, sizeof(*node));
        }
        prec_position += sizeof(*node);
        add_serialized_bucket_entry(nodeOffset + 4, write_object(node->obj));
        switch (node->kind) {
            case 0:
                if (node->u.k0.tokenBuffer.firsttoken != NULL) {
                    add_serialized_bucket_entry(nodeOffset + 0x16,
                                                (SInt32)append_saved_prep_tokens(node->u.k0.tokenBuffer.firsttoken,
                                                                                 node->u.k0.tokenBuffer.tokens));
                }
                if (node->u.k0.contextClass != NULL) {
                    add_serialized_bucket_entry(nodeOffset + 0x1a, write_type((Type *)node->u.k0.contextClass));
                }
                break;
            case 1:
                add_serialized_bucket_entry(nodeOffset + 8, write_type((Type *)node->u.k1.classTemplate));
                add_serialized_bucket_entry(nodeOffset + 0xc, write_type((Type *)node->u.k1.context));
                add_serialized_bucket_entry(nodeOffset + 0x10, serialize_prec_records(node->u.k1.source));
                break;
            case 2:
                add_serialized_bucket_entry(nodeOffset + 8, (SInt32)write_template_function(node->u.k2.definition));
                specialization = node->u.k2.specialization;
                if (data_00581c28 != 0) {
                    while ((prec_position & 3) != 0) {
                        AppendGListByte(&precompiled_buffer, 0);
                        prec_position++;
                    }
                }
                firstSpecializationOffset = specializationOffset = prec_position;
                for (;;) {
                    if (data_00581c28 != 0) {
                        AppendGListData(&precompiled_buffer, specialization, sizeof(*specialization));
                    }
                    prec_position += sizeof(*specialization);
                    add_serialized_bucket_entry(specializationOffset + 4, write_object(specialization->object));
                    add_serialized_bucket_entry(specializationOffset + 8,
                                                serialize_ct_state_elems(specialization->args));
                    if (specialization->next == NULL) {
                        break;
                    }
                    if (data_00581c28 != 0) {
                        while ((prec_position & 3) != 0) {
                            AppendGListByte(&precompiled_buffer, 0);
                            prec_position++;
                        }
                    }
                    nextOffset = prec_position;
                    add_serialized_bucket_entry(specializationOffset, nextOffset);
                    specializationOffset = nextOffset;
                    specialization = specialization->next;
                }
                add_serialized_bucket_entry(nodeOffset + 0xc, firstSpecializationOffset);
                break;
            case 3:
                break;
            default:
                CError_FATAL(4045);
        }
        if (node->next == NULL) {
            break;
        }
        if (data_00581c28 != 0) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        specializationOffset = prec_position;
        add_serialized_bucket_entry(nodeOffset, specializationOffset);
        nodeOffset = specializationOffset;
        node = node->next;
    }
    return result;
}
#endif

void write_serialized_buckets(void)
{
    SerializedBucketEntry *entry;
    int count;
    int i;

    i = 0;
    if (0 < serialized_bucket_count) {
        do {
            count = 0;
            for (entry = serialized_buckets[i].list; entry; entry = entry->next)
                count++;
            if (count) {
                CPrec_AppendPointer((void *)count);
                CPrec_AppendPointer((void *)i);
                for (entry = serialized_buckets[i].list; entry; entry = entry->next)
                    CPrec_AppendPointer((void *)entry->offset);
            }
            i++;
        } while (i < serialized_bucket_count);
    }
    CPrec_AppendPointer(NULL);
}

static CPREC_INLINE void write_saved_prep_token_patches(void)
{
    SavedPrepTokenList *list;

    for (list = saved_prep_tokens; list; list = list->next) {
        CPrec_AppendPointer(list->offset);
        CPrec_AppendPointer((void *)list->count);
    }
    CPrec_AppendPointer(NULL);
}

short encode_zero_runs(char *data, int size)
{
    char buffer[2304];
    int outputSize = 0;
    char *cursor = data;
    char *end = data + size;
    int runLength;
    int runStart;
    short result;

    for (;;) {
        do {
            if (cursor < end) {
                if (*cursor == 0) {
                    runLength = 0xe0;
                    while (*cursor == 0 && cursor < end && runLength < 0x100) {
                        runLength++;
                        cursor++;
                    }
                    buffer[outputSize] = runLength - 1;
                    outputSize++;
                } else {
                    runStart = outputSize;
                    outputSize++;
                    runLength = 0;
                    while (cursor < end && runLength < 0xe0) {
                        if (*cursor == 0 && cursor[1] == 0)
                            break;
                        buffer[outputSize] = *cursor;
                        cursor++;
                        outputSize++;
                        runLength++;
                    }
                    buffer[runStart] = runLength - 1;
                }
            }
        } while (cursor < end && outputSize <= 0x800);
        result = COS_FileWrite(precompiled_file, buffer, outputSize);
        if (result)
            return result;
        if (cursor >= end)
            break;
        outputSize = 0;
    }
    return 0;
}

static CPREC_INLINE SInt16 CPrec_Flush(void)
{
    if (data_00581c28) {
        SInt16 result;
        char **buffer;

        align_to_four_byte_boundary();
        flushed_size += precompiled_buffer.size;
        COS_LockHandle(precompiled_buffer.data);
        buffer = precompiled_buffer.data;
        result = encode_zero_runs(*buffer, precompiled_buffer.size);
        COS_UnlockHandle(precompiled_buffer.data);
        precompiled_buffer.size = 0;
        return result;
    }
    return 0;
}

static CPREC_INLINE SInt16 checkflush(void)
{
    SInt16 result;
    SInt32 size;

    if ((size = precompiled_buffer.size) > data_00581c2a)
        data_00581c2a = size;
    if (precompiled_buffer.size > 10000) {
        if ((result = CPrec_Flush()))
            return data_00581c26 = result;
    }
    return 0;
}

static inline void putlong(SInt32 value)
{
    BVWord u;
    u.l = value;
    put(u.b[3]);
    put(u.b[2]);
    put(u.b[1]);
    put(u.b[0]);
}

SInt32 write_serialized_bucket_offsets(void)
{
    SerializedBucketEntry *p;
    SInt32 count;
    SInt32 value;
    SInt32 prev;
    SInt32 delta;

    precompiled_buffer.size = 0;
    p = serialized_bucket_entries;
    count = prev = 0;
    while (p != NULL) {
        CError_ASSERT(4207, (p->offset & 0x80000001) == 0);
        delta = p->offset - prev;
        value = p->offset;
        if (delta >= -128 && delta <= 126) {
            put((UInt8)((delta >> 1) | 0x80));
        } else {
            putlong(value);
        }
        prev = p->offset;
        p = p->next;
        count++;
    }
    return count;
}

/* Returns an error code. */
SInt16 serialize_precompiled_data(Boolean writePositions)
{
    SInt32 blockPosition;

    freelheap();
    CPrec_SetupPtrHash();
    CPrec_SetupSelectorHash();
    serialized_bucket_entries = NULL;
    saved_prep_tokens = NULL;
    prec_position = 0;
    flushed_size = 0;
    data_00581c28 = writePositions;
    data_00581c26 = 0;
    CPrec_SetupBuiltIns();
    CPrec_AppendPointer(NULL);
    append_hash_names();
    {
        SInt16 flushResult;
        if ((flushResult = CPrec_Flush()) != 0)
            return flushResult;
    }

    serialize_macros();
    {
        SInt16 flushResult;
        if ((flushResult = CPrec_Flush()) != 0)
            return flushResult;
    }

    serialize_namespace_usings_and_hash();
    if (writePositions) {
        if (data_00581c26)
            return data_00581c26;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (class_template_list) {
        blockPosition = write_type((Type *)class_template_list);
        if (writePositions)
            prec_header->classExtensionOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (templateFunctions) {
        TemplateFunction *serializedFunctions = write_template_function(templateFunctions);
        if (writePositions)
            prec_header->templateFunctionsOffset = (SInt32)serializedFunctions;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (somReferences) {
        blockPosition = write_csomrefnode_list(somReferences);
        if (writePositions)
            prec_header->somReferencesOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (pending_buffers) {
        PendingBuffer *serializedBuffers = serialize_pending_buffers(pending_buffers);
        if (writePositions)
            prec_header->pendingBuffersOffset = (SInt32)serializedBuffers;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (pending_object_classes) {
        blockPosition = serialize_pending_object_class_list(pending_object_classes);
        if (writePositions)
            prec_header->pendingObjectClassesOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (class_pointer_type) {
        blockPosition = write_type(class_pointer_type);
        if (writePositions)
            prec_header->classPointerTypeOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (id_type) {
        blockPosition = write_type(id_type);
        if (writePositions)
            prec_header->idTypeOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (sel_type) {
        blockPosition = write_type(sel_type);
        if (writePositions)
            prec_header->selTypeOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (selector_hash) {
        blockPosition = (SInt32)CPrec_WriteTable(selector_hash);
        if (writePositions)
            prec_header->selectorHashOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (class_type_entries) {
        blockPosition = write_prec_type_entries(class_type_entries);
        if (writePositions)
            prec_header->classTypeEntriesOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (data_00588064) {
        struct CRec *serializedRecords = write_crec(data_00588064);
        if (writePositions)
            prec_header->objcRecordsOffset = (SInt32)serializedRecords;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (pending_functions) {
        blockPosition = (SInt32)CPrec_WriteList(pending_functions);
        if (writePositions)
            prec_header->pendingFunctionsOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    if (pendingInlineWork) {
        blockPosition = serialize_cprec_nodes(pendingInlineWork);
        if (writePositions)
            prec_header->pendingInlineWorkOffset = blockPosition;
        {
            SInt16 flushResult;
            if ((flushResult = CPrec_Flush()) != 0)
                return flushResult;
        }
    }

    return 0;
}

static CPREC_INLINE SInt16 align_output(SInt16 handle, SInt32 *position)
{
    char padding[8];
    SInt32 count;
    SInt16 result;

    if ((*position & 7) == 0)
        return 0;
    count = 8 - (*position & 7);
    memclrw(padding, 8);
    result = COS_FileWrite(handle, padding, count);
    *position += count;
    return result;
}

SInt16 write_precompiled_file(void)
{
    SInt32 position;
    char message[128];
    HashNameNode *node;
    SavedPrepTokenList *patchNode;
    SInt32 bucket;
    SInt16 error;
    SInt32 value;

    if (InitGList(&precompiled_buffer, 0x40000))
        CError_LongJump();

    CompilerGetCString(10, message);
    fn_0041b8d0(compiler_plugin_cu.context, message, "");
    CPrep_RemoveFlaggedMacros();

    bucket = 0;
    do {
        for (node = data_00587f88[bucket]; node != NULL; node = node->next)
            node->id = 0;
    } while (++bucket < 0x800);

    error = serialize_precompiled_data(0);
    if (error)
        return error;

    CompilerGetCString(11, message);
    fn_0041b8d0(compiler_plugin_cu.context, message, "");

    prec_header = galloc(0x50e8);
    memclrw(prec_header, 0x50e8);
    prec_header->magic = (SInt32)0xbeefface;
    prec_header->version = 0x412;
    prec_header->kind = 2;
    prec_header->flag = copts.check_header_flags;
    prec_header->cplusplus = copts.cplusplus;
    prec_header->uniqueID = CParser_GetUniqueID();
    prec_header->selectorReferenceCount = selector_reference_count;
    prec_header->objcState = data_00587f6c;
    prec_header->objcStringConstantCount = objc_string_constant_count;

    error = COS_FileWrite(precompiled_file, prec_header, 0x50e8);
    if (error)
        return error;

    position = 0x50e8;
    error = serialize_precompiled_data(1);
    if (error)
        return error;

    prec_header->fileSize = prec_position;
    prec_header->dataOffset = position;

    error = COS_FileGetPos(precompiled_file, &position);
    if (error)
        return error;

    prec_header->compressedSize = position - prec_header->dataOffset;
    prec_header->relocationCount = write_serialized_bucket_offsets();
    prec_header->relocationSize = precompiled_buffer.size;
    prec_header->relocationOffset = position;

    if (prec_header->relocationCount != 0) {
        error = COS_FileWrite(precompiled_file, *precompiled_buffer.data, precompiled_buffer.size);
        if (error)
            return error;
        position += precompiled_buffer.size;
    }

    if ((error = align_output(precompiled_file, &position)) != 0)
        return error;

    precompiled_buffer.size = 0;
    write_serialized_buckets();
    prec_header->objectPatchSize = precompiled_buffer.size;
    prec_header->objectPatchOffset = position;

    error = COS_FileWrite(precompiled_file, *precompiled_buffer.data, precompiled_buffer.size);
    if (error)
        return error;

    position += precompiled_buffer.size;

    if (saved_prep_tokens != NULL) {
        precompiled_buffer.size = 0;
        write_saved_prep_token_patches();
        prec_header->sourcePatchSize = precompiled_buffer.size;
        prec_header->sourcePatchOffset = position;
        error = COS_FileWrite(precompiled_file, *precompiled_buffer.data, precompiled_buffer.size);
        if (error)
            return error;
        position += precompiled_buffer.size;
    }

    error = COS_FileSetPos(precompiled_file, 0);
    if (error)
        return error;

    error = COS_FileWrite(precompiled_file, prec_header, 0x50e8);
    if (error)
        return error;

    return 0;
}

int CPrec_WritePrecompiledFile(void)
{
    unsigned int status;
    short messageKind;
    short error;
    struct FileProcessingInfo fileInfo;
    char message[128];
    struct StorageHandle *fileHandle;
    CPrepCU *context;

    fileInfo.info = ((CPrepCU *)cprep_cu)->mainFile;
    context = (CPrepCU *)cprep_cu;
    status = CWCompilerLinkerPrivate_InvokeCompilerCallback(context->context, &fileInfo, data_00587e84);
    if (status != 0) {
        return status;
    }
    messageKind = 3;
    error = COS_FileNew(&fileInfo.info, &precompiled_file, copts.precompiledHeaderCreator,
                        copts.precompiledHeaderFileTypes[0]);
    if (error == 0) {
        messageKind = 4;
        error = write_precompiled_file();
    }
    fn_004e0970();
    if (error != 0) {
        CompilerGetCString(messageKind, message);
        sprintf(error_message_buffer, message, error);
        context = (CPrepCU *)cprep_cu;
        CWPluginsPrivate_InvokeMessageCallback((struct DispatchObject_0041b830 *)context->context, NULL,
                                               error_message_buffer, NULL, 2, 0);
    } else {
        fileHandle = NULL;
        context = (CPrepCU *)cprep_cu;
        CWPluginsPrivate_CallFileProcessingCallback(context->context, &fileInfo, (void **)&fileHandle, 1);
    }
}

static CPREC_INLINE void read_precompiled_header_data_at_offset(SInt32 offset, void *buffer, SInt32 size)
{
    if (COS_FileSetPos(precompiled_file, offset) || COS_FileRead(precompiled_file, buffer, size))
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
}

#pragma opt_lifetimes off
void decompress_precompiled_header(void)
{
    UInt8 *end;
    UInt8 *dst;
    UInt8 *src;
    SInt32 n;
    UInt32 size;
    UInt32 csize;

    if (data_00581c1e == NULL) {
        size = (prec_header->fileSize >> 7) + prec_header->fileSize + 0x40;
        dst = galloc(size);
        precompiled_header_base = dst;
        src = dst + size - prec_header->compressedSize;
        csize = prec_header->compressedSize;
        read_precompiled_header_data_at_offset(prec_header->dataOffset, src, csize);
    } else {
        dst = galloc(prec_header->fileSize);
        precompiled_header_base = dst;
        src = data_00581c1e + prec_header->dataOffset;
    }
    end = src + prec_header->compressedSize;
    while (src < end) {
        if ((n = *src++) >= 0xe0) {
            n -= 0xe0;
            do {
                n--;
                *dst++ = 0;
            } while (n >= 0);
        } else {
            do {
                *dst++ = *src++;
            } while (--n >= 0);
        }
    }
    if (src != end || dst != precompiled_header_base + prec_header->fileSize)
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
}
#pragma opt_lifetimes reset

void apply_relocations(void)
{
    UInt8 *cursor;
    SInt32 remaining;
    SInt32 offset;
    UInt8 *base;
    UInt8 *relocations;
    LongBytes decoded;

    remaining = prec_header->relocationCount;
    if (remaining != 0) {
        if (data_00581c1e == NULL) {
            relocations = lalloc(prec_header->relocationSize);
            read_precompiled_header_data_at_offset(prec_header->relocationOffset, relocations, prec_header->relocationSize);
        } else {
            relocations = data_00581c1e + prec_header->relocationOffset;
        }
        offset = 0;
        cursor = relocations;
        base = precompiled_header_base;
        do {
            if ((*cursor & 0x80) == 0) {
                decoded.b[3] = cursor[0];
                decoded.b[2] = cursor[1];
                decoded.b[1] = cursor[2];
                decoded.b[0] = cursor[3];
                offset = decoded.l;
                cursor += 4;
            } else {
                offset += (SInt8)(*cursor << 1);
                cursor += 1;
            }
            {
                UInt8 **address = (UInt8 **)(base + offset);
                *address += (SInt32)base;
            }
        } while (--remaining > 0);
        freelheap();
        relocations += prec_header->relocationSize;
        if (cursor != relocations)
            CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
    }
}

UInt8 *apply_object_patches(void)
{
    SInt32 count;
    SInt32 value;
    SInt32 *p;
    union CPrecInputPointer input;
    UInt8 *base;

    if (prec_header->objectPatchSize != 0) {
        build_global_pointer_entries();
        if (data_00581c1e == NULL) {
            p = lalloc(prec_header->objectPatchSize);
            read_precompiled_header_data_at_offset(prec_header->objectPatchOffset, p, prec_header->objectPatchSize);
        } else {
            input.bytes = data_00581c1e + prec_header->objectPatchOffset;
            p = input.words;
        }

        base = precompiled_header_base;
        for (;;) {
            if ((count = *p++) == 0)
                break;
            value = global_pointer_entries[*p++];
            do {
                *(SInt32 *)(base + *p++) = value;
            } while (--count);
        }
    }
    return base;
}

void patch_buffered_token_locations(void)
{
    TStreamElement *token;
    SInt32 index;
    UInt32 tokenCount;
    CPrepFileInfo *currentFile;
    SInt32 currentOffset;
    SInt32 *patchCursor;
    union CPrecInputPointer input;
    UInt8 *headerBase;
    SInt32 tokenOffset;

    if (prec_header->sourcePatchSize != 0) {
        CPrep_GetPosition(&currentFile, &currentOffset);

        if (data_00581c1e == NULL) {
            patchCursor = lalloc(prec_header->sourcePatchSize);
            read_precompiled_header_data_at_offset(prec_header->sourcePatchOffset, patchCursor, prec_header->sourcePatchSize);
        } else {
            input.bytes = data_00581c1e + prec_header->sourcePatchOffset;
            patchCursor = input.words;
        }

        headerBase = precompiled_header_base;
        for (;;) {
            if ((tokenOffset = *patchCursor++) == 0)
                break;
            token = (TStreamElement *)(headerBase + tokenOffset);
            tokenCount = (UInt32)*patchCursor++;
            for (index = 0; index < tokenCount; index++) {
                token->tokenfile = currentFile;
                token->tokenoffset = currentOffset;
                token++;
            }
        }
    }
}

void restore_macro_lists(void)
{
    int entry;
    int entry_count;
    Macro **lists;
    long saved_offset;
    Macro *node;
    SInt32 *saved_lists;
    int list_index;
    UInt8 *arena;
    lists = macro_buckets;
    arena = precompiled_header_base;
    saved_lists = prec_header->macroOffsets;
    list_index = 0;
    do {
        node = *lists;
        if (node != NULL) {
            do {
                node->name = GetHashNameNode(node->name->name);
                entry_count = node->nargs & 32767;
                entry = 1;
                while (entry < entry_count) {
                    node->args[entry - 1] = GetHashNameNode(node->args[entry - 1]->name);
                    entry = entry + 1;
                }
                node = node->next;
            } while (node != NULL);
        }
        if ((saved_offset = *saved_lists) != 0) {
            if (*lists != NULL) {
                node = (Macro *)(arena + saved_offset);
                while (node->next != NULL) {
                    node = node->next;
                }
                node->next = *lists;
            }
            *lists = (Macro *)(arena + saved_offset);
        }
        list_index = list_index + 1;
        saved_lists = saved_lists + 1;
        lists = lists + 1;
    } while (list_index < 2048);
}

static CPREC_INLINE void relocate_offset_table(SInt32 *offsets, int count, void **table)
{
    int i;
    UInt8 *base;
    SInt32 offset;

    base = precompiled_header_base;
    for (i = 0; i < count; i++, offsets++, table++) {
        if ((offset = *offsets))
            *table = base + offset;
        else
            *table = NULL;
    }
}

static CPREC_INLINE void restore_namespace_names(void)
{
    CError_ASSERT(4845, cscope_root->is_hash);
    cscope_root->names = prec_header->nameCount;
    relocate_offset_table(prec_header->namespaceNameOffsets, 0x400, (void **)cscope_root->data.hash);
}

static CPREC_INLINE void reset_hash_name_ids(void)
{
    HashNameNode *name;
    int i;

    for (i = 0; i < 0x800; i++) {
        for (name = data_00587f88[i]; name; name = name->next)
            name->id = -1;
    }
}

static CPREC_INLINE void declare_pending_buffers(void)
{
    PendingBuffer *record;

    record = pending_buffers;
    pending_buffers = NULL;
    for (; record; record = record->next)
        CInit_DeclareData(record->owner, record->buffer, record->value, record->entryValue);
}

void CPrec_LoadPrecompiledHeader(short file, UInt8 *buffer)
{
    struct CPrecHeader *header;
    SInt32 offset;
    UInt8 *base;
    SInt32 index;
    SInt32 bucket;
    HashNameNode *name;
    SInt32 *offsets;
    HashNameNode **hashEntries;
    PendingBuffer *record;

    precompiled_file = file;
    data_00581c1e = buffer;
    CPrep_RemoveFlaggedMacros();
    if (!CScope_IsEmptySymTable())
        CError_FatalError(ERR_ILLEGAL_USE_PRECOMPILED_HEADER);

    if (data_00581c1e == NULL) {
        prec_header = galloc(sizeof(struct CPrecHeader));
        header = prec_header;
        read_precompiled_header_data_at_offset(0, header, sizeof(*header));
    } else {
        header = (struct CPrecHeader *)data_00581c1e;
        prec_header = header;
    }

    if (prec_header->magic != 0xbeefface)
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
    if (prec_header->version != 0x412)
        CError_FatalError(ERR_ILLEGAL_PRECOMPILED_HEADER_VERSION);
    if (prec_header->kind != 2)
        CError_FatalError(ERR_ILLEGAL_PRECOMPILED_HEADER_COMPILER_FLAGS_TARGET);

    copts.check_header_flags = prec_header->flag;
    copts.check_header_flags != 0;
    decompress_precompiled_header();
    apply_relocations();
    apply_object_patches();

    relocate_offset_table(prec_header->hashNameOffsets, 0x800, (void **)data_00587f88);
    reset_hash_name_ids();
    restore_macro_lists();
    patch_buffered_token_locations();

    restore_namespace_names();

    if (!prec_header->usingsOffset)
        cscope_root->usings = NULL;
    else
        cscope_root->usings = (struct NameSpaceList *)(precompiled_header_base + prec_header->usingsOffset);

    if (!prec_header->classExtensionOffset)
        class_template_list = NULL;
    else
        class_template_list = (TemplClass *)(precompiled_header_base + prec_header->classExtensionOffset);

    if (!prec_header->templateFunctionsOffset)
        templateFunctions = NULL;
    else
        templateFunctions = (struct TemplateFunction *)(precompiled_header_base + prec_header->templateFunctionsOffset);

    if (!prec_header->somReferencesOffset)
        somReferences = NULL;
    else
        somReferences = (struct CSOMRefNode *)(precompiled_header_base + prec_header->somReferencesOffset);

    if (!prec_header->pendingBuffersOffset)
        pending_buffers = NULL;
    else
        pending_buffers = (PendingBuffer *)(precompiled_header_base + prec_header->pendingBuffersOffset);

    if (!prec_header->pendingObjectClassesOffset)
        pending_object_classes = NULL;
    else
        pending_object_classes =
            (struct CallbackAction *)(precompiled_header_base + prec_header->pendingObjectClassesOffset);

    if (!prec_header->classPointerTypeOffset)
        class_pointer_type = NULL;
    else
        class_pointer_type = (Type *)(precompiled_header_base + prec_header->classPointerTypeOffset);

    if (!prec_header->idTypeOffset)
        id_type = NULL;
    else
        id_type = (Type *)(precompiled_header_base + prec_header->idTypeOffset);

    if (!prec_header->selTypeOffset)
        sel_type = NULL;
    else
        sel_type = (Type *)(precompiled_header_base + prec_header->selTypeOffset);

    if (!prec_header->selectorHashOffset)
        selector_hash = NULL;
    else
        selector_hash = (struct HashEntry **)(precompiled_header_base + prec_header->selectorHashOffset);

    if (!prec_header->classTypeEntriesOffset)
        class_type_entries = NULL;
    else
        class_type_entries = (struct PrecTypeEntry *)(precompiled_header_base + prec_header->classTypeEntriesOffset);

    if (!prec_header->objcRecordsOffset)
        data_00588064 = NULL;
    else
        data_00588064 = (struct CRec *)(precompiled_header_base + prec_header->objcRecordsOffset);

    if (!prec_header->pendingFunctionsOffset)
        pending_functions = NULL;
    else
        pending_functions = (struct PendingFunction *)(precompiled_header_base + prec_header->pendingFunctionsOffset);

    if (!prec_header->pendingInlineWorkOffset)
        pendingInlineWork = NULL;
    else
        pendingInlineWork = (CPrecNode *)(precompiled_header_base + prec_header->pendingInlineWorkOffset);

    CParser_SetUniqueID(prec_header->uniqueID);
    selector_reference_count = prec_header->selectorReferenceCount;
    data_00587f6c = prec_header->objcState;
    objc_string_constant_count = prec_header->objcStringConstantCount;

    precompiled_file = 0;
    fn_004e0970();
    cscope_current = cscope_root;
    if (!CParser_ReInitRuntimeObjects(1))
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
    CPrep_RegisterPredefinedMacros();

    if (cprep_cu[0xe0] != 1)
        declare_pending_buffers();
}
