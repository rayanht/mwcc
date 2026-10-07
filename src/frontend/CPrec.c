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
#include "driver/CWPluginsPrivate.h"
#include "driver/cc-eabi-ppc.h"
#include <string.h>
#include <stdio.h>

/* Declarations gathered from the merged files. */
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
        if (COS_FileSetPos(precompiled_file, 0) != 0 || COS_FileRead(precompiled_file, header, sizeof(*header)) != 0)
            CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
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

    offsets = prec_header->hashNameOffsets;
    hashEntries = data_00587f88;
    base = precompiled_header_base;
    index = 0;
    do {
        offset = *offsets;
        if (offset != 0)
            *hashEntries = (HashNameNode *)(base + offset);
        else
            *hashEntries = NULL;
        offsets++;
        hashEntries++;
    } while (++index < 0x800);

    bucket = 0;
    do {
        name = data_00587f88[bucket];
        if (name) {
            do {
                name->id = -1;
                name = name->next;
            } while (name != NULL);
        }
    } while (++bucket < 0x800);

    restore_macro_lists();
    patch_buffered_token_locations();

    CError_ASSERT(4845, cscope_root->is_hash != 0);
    {
        SInt32 namespaceOffset;
        UInt8 *namespaceBase;
        SInt32 namespaceIndex;
        SInt32 *namespaceOffsets;
        NameSpaceName **namespaceEntries;

        cscope_root->names = prec_header->nameCount, namespaceEntries = cscope_root->data.hash;
        namespaceOffsets = prec_header->namespaceNameOffsets;
        namespaceBase = precompiled_header_base;
        namespaceIndex = 0;
        do {
            namespaceOffset = *namespaceOffsets;
            if (namespaceOffset != 0)
                *namespaceEntries = (NameSpaceName *)(namespaceBase + namespaceOffset);
            else
                *namespaceEntries = NULL;
            namespaceOffsets++;
            namespaceEntries++;
        } while (++namespaceIndex < 0x400);
    }

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
    if (precompiled_buffer.data != NULL)
        FreeGList(&precompiled_buffer);
    cscope_current = cscope_root;
    if (!CParser_ReInitRuntimeObjects(1))
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
    CPrep_RegisterPredefinedMacros();

    if (cprep_cu[0xe0] != 1) {
        record = pending_buffers;
        pending_buffers = NULL;
        for (; record != NULL; record = record->next)
            CInit_DeclareData(record->owner, record->buffer, record->value, record->entryValue);
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

static void CPrec_ReadData(SInt32 offset, void *buffer, SInt32 size)
{
    if (COS_FileSetPos(precompiled_file, offset) != 0 || COS_FileRead(precompiled_file, buffer, size) != 0)
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
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
            CPrec_ReadData(prec_header->sourcePatchOffset, patchCursor, prec_header->sourcePatchSize);
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

static void read_precompiled_header_data_at_offset(SInt32 offset, void *buffer, SInt32 size)
{
    if (COS_FileSetPos(precompiled_file, offset) != 0 || COS_FileRead(precompiled_file, buffer, size) != 0)
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
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

static void read_precompiled_header_data(SInt32 offset, void *buffer, SInt32 size)
{
    if (COS_FileSetPos(precompiled_file, offset) != 0 || COS_FileRead(precompiled_file, buffer, size) != 0)
        CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
}

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
            read_precompiled_header_data(prec_header->relocationOffset, relocations, prec_header->relocationSize);
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

void decompress_precompiled_header(void)
{
    UInt8 *end;
    UInt8 *dst;
    UInt8 *src;
    SInt32 n;
    UInt32 size;
    UInt32 csize;

    if (data_00581c1e == NULL) {
        size = prec_header->fileSize;
        size = (size >> 7) + size + 0x40;
        dst = galloc(size);
        precompiled_header_base = dst;
        src = dst + size - prec_header->compressedSize;
        csize = prec_header->compressedSize;
        if (COS_FileSetPos(precompiled_file, prec_header->dataOffset) != 0 ||
            COS_FileRead(precompiled_file, src, csize) != 0)
            CError_FatalError(ERR_ILLEGAL_DATA_PRECOMPILED_HEADER);
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
    status = CPrep_InvokeCompilerCallback(context->context, &fileInfo, data_00587e84);
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
    if (precompiled_file != 0) {
        COS_FileClose(precompiled_file);
        precompiled_file = 0;
    }
    if (precompiled_buffer.data != NULL) {
        FreeGList(&precompiled_buffer);
    }
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

static inline SInt16 align_output(SInt16 handle, SInt32 *position)
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
        for (patchNode = saved_prep_tokens; patchNode != NULL; patchNode = patchNode->next) {
            value = (SInt32)patchNode->offset;
            if (data_00581c28)
                AppendGListLong(&precompiled_buffer, value);
            prec_position += 4;
            value = patchNode->count;
            if (data_00581c28)
                AppendGListLong(&precompiled_buffer, value);
            prec_position += 4;
        }
        if (data_00581c28)
            AppendGListLong(&precompiled_buffer, 0);
        prec_position += 4;
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

#define CPrec_Pad()                                                                                                    \
    do {                                                                                                               \
        if (data_00581c28 != 0) {                                                                                      \
            while ((prec_position & 3) != 0) {                                                                         \
                AppendGListByte(&precompiled_buffer, 0);                                                               \
                prec_position += 1;                                                                                    \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

static inline SInt32 CPrec_AlignedPosition(void)
{
    CPrec_Pad();
    return prec_position;
}

static SInt16 CPrec_Flush(void)
{
    if (data_00581c28 != 0) {
        SInt16 r;
        char **buffer;
        CPrec_Pad();
        flushed_size += precompiled_buffer.size;
        COS_LockHandle(precompiled_buffer.data);
        buffer = precompiled_buffer.data;
        r = encode_zero_runs(*buffer, precompiled_buffer.size);
        COS_UnlockHandle(precompiled_buffer.data);
        precompiled_buffer.size = 0;
        return r;
    }
    return 0;
}

static inline SInt32 CPrec_Hash_4d7df0(SInt32 value)
{
    CPrecBytes b;
    b.value = value;
    return (b.bytes[0] + value + b.bytes[1] + b.bytes[2] + b.bytes[3]) & 0x3fff;
}

static inline void CPrec_Register(SInt32 v, SInt32 w)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *node;
    bucket = &written_entry_buckets[CPrec_Hash_4d7df0(v)];
    node = lalloc(0xc);
    node->object = (void *)v;
    node->image_position = (void *)w;
    node->next = *bucket;
    *bucket = node;
}

static inline void CPrec_SetupBuiltIns(void)
{
    SInt32 i;
    serialized_buckets = lalloc(serialized_bucket_count * 0xc);
    memclrw(serialized_buckets, serialized_bucket_count * 0xc);
    for (i = 0; i < serialized_bucket_count; i++) {
        serialized_buckets[i].object = global_pointer_entries[i];
        serialized_buckets[i].image_position = ~i;
        CPrec_Register(serialized_buckets[i].object, serialized_buckets[i].image_position);
    }
}

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

/* Returns an error code. */
SInt16 serialize_precompiled_data(Boolean writePositions)
{
    SInt32 blockPosition;

    freelheap();
    written_entry_buckets = (CPrecWrittenEntry **)lalloc(0x4000 * sizeof(*written_entry_buckets));
    memclrw(written_entry_buckets, 0x4000 * sizeof(*written_entry_buckets));
    data_00581c02 = (CPrecWrittenEntry **)lalloc(0x400 * sizeof(CPrecWrittenEntry *));
    memclrw(data_00581c02, 0x400 * sizeof(CPrecWrittenEntry *));
    serialized_bucket_entries = NULL;
    saved_prep_tokens = NULL;
    prec_position = 0;
    flushed_size = 0;
    data_00581c28 = writePositions;
    data_00581c26 = 0;
    build_global_pointer_entries();
    CPrec_SetupBuiltIns();

    if (data_00581c28)
        AppendGListLong(&precompiled_buffer, 0);
    prec_position += sizeof(SInt32);
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
        blockPosition = CPrec_WriteTable(selector_hash);
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
        blockPosition = CPrec_WriteList(pending_functions);
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

static void put(UInt8 c)
{
    if (data_00581c28 != 0)
        AppendGListByte(&precompiled_buffer, c);
    prec_position++;
}

static void putlong(SInt32 value)
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

static inline void *PrecItemAlign(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void PrecItemData(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static inline int add_serialized_pointer_entry(SInt32 offset, const void *imagePosition)
{
    return add_serialized_bucket_entry(offset, (SInt32)imagePosition);
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

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

static NameSpaceList *CPrec_GetNSUsingPatch(NameSpaceList *u)
{
    NameSpaceList *first;
    NameSpaceList *current;
    NameSpaceList *next;

    first = current = CPrec_AppendAlign();
    while (1) {
        CPrec_AppendData(u, sizeof(NameSpaceList));
        add_serialized_bucket_entry((SInt32)(&current->nspace), get_namespace_patch(u->nspace));
        if (!u->next)
            break;
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = CPrec_AppendAlign()));
        current = next;
        u = u->next;
    }
    return first;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static ObjNameSpace *CPrec_GetObjNameSpacePatch(ObjNameSpace *obj)
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

#define written_entry_buckets ((CPrecWrittenEntry **)written_entry_buckets)
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

static inline SInt16 flush(void)
{
    SInt16 err;
    if (data_00581c28) {
        alignbuf();
        flushed_size += precompiled_buffer.size;
        COS_LockHandle(precompiled_buffer.data);
        err = encode_zero_runs(*precompiled_buffer.data, precompiled_buffer.size);
        COS_UnlockHandle(precompiled_buffer.data);
        precompiled_buffer.size = 0;
    } else
        err = 0;
    return err;
}

static inline SInt16 checkflush(void)
{
    SInt16 err;
    SInt32 size;
    if ((size = precompiled_buffer.size) > data_00581c2a)
        data_00581c2a = size;
    if (precompiled_buffer.size > 10000) {
        if ((err = flush()) != 0)
            return data_00581c26 = err;
    }
    return 0;
}

/* writing the image */
/* image offset */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_zero_bytes_to_alignment(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void append_prec_data(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static int fn_004da6c0_part1(int p)
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

static UInt32 hash_pointer(void *pv)
{
    CPrecKey u;
    u.v = (UInt32)pv;
    return ((UInt32)u.b[0] + (UInt32)pv + (UInt32)u.b[1] + (UInt32)u.b[2] + (UInt32)u.b[3]) & 0x3fff;
}

static UInt32 CPrec_HashNew(void *p)
{
    return hash_pointer(p);
}

static CPrecWrittenEntry *find_written_entry(Object *x)
{
    CPrecWrittenEntry *e;
    for (e = written_entry_buckets[hash_pointer(x)]; e != NULL; e = e->next) {
        if (e->object == x)
            return e;
    }
    return NULL;
}

static SInt32 append_alignment_padding(void)
{
    SInt32 p;
    if (data_00581c28 != 0) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    p = prec_position;
    return p;
}

static void append_data_and_count_size(void *d, SInt32 size)
{
    if (data_00581c28 != 0)
        AppendGListData(&precompiled_buffer, d, size);
    prec_position += size;
}

static void patch_name_reference(UInt32 off, HashNameNode *nm)
{
    nm->id = 1;
    patch_object_reference(off, nm);
}

static inline void CPrec_Float(Object *obj, UInt32 off)
{
    void *d = obj->u.data.u.string;
    SInt32 cur;
    append_alignment_padding();
    cur = prec_position;
    append_data_and_count_size(d, 8);
    add_serialized_bucket_entry((SInt32)(off + 0x26), (SInt32)(cur));
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_zero_alignment_padding(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData_004da950(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_zero_padding_to_alignment(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void append_data_and_count_length(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
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

static UInt32 CPrec_004daf90_Align(void)
{
    if (data_00581c28 != 0) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return prec_position;
}

/* Align the stream to a 4-byte boundary and return the new position. */
static SInt32 PrecompBegin(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return prec_position;
}

/* Append raw bytes to the stream. */
static void PrecompAppend(ENodeList *data, SInt32 size)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, size);
    prec_position += size;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_glist_alignment_padding(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData_004db910(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static int CPrec_004dbc00_part1(int p)
{
    union {
        int w;
        unsigned char b[4];
    } u;
    u.w = p;
    return u.b[0] + p + u.b[1] + u.b[2] + u.b[3] & 16383;
}

static inline ObjectOffsetEntry *CPrec_004dbc00_inline1(Type *a0, long *out)
{
    ObjectOffsetEntry *node;
    int result;
    int v1s;
    node = (ObjectOffsetEntry *)(long)(v1s = ((int *)written_entry_buckets)[CPrec_004dbc00_part1((int)a0)]);
    if (v1s != 0) {
        do {
            if ((int)node->object == (int)a0) {
                goto found;
            }
            node = (ObjectOffsetEntry *)((long)node->next);
        } while (node);
    }
    node = NULL;
found:
    *out = (long)node;
    return (ObjectOffsetEntry *)node;
}

static inline int CPrec_004dbc00_inline2(Type *p0, int p1)
{
    int v3;
    v3 = align_to_four_byte_boundary();
    add_written_type_entry(p0, v3);
    fn_004e0010(p0, p1);
    return v3;
}

static UInt32 hashptr(void *p)
{
    union {
        void *pp;
        UInt8 b[4];
    } u;
    u.pp = p;
    return (u.b[0] + (UInt32)p + u.b[1] + u.b[2] + u.b[3]) & 0x3fff;
}

static UInt32 align_counter_with_zero_padding(void)
{
    if (data_00581c28 != 0) {
        for (; (prec_position & 3) != 0; prec_position++)
            AppendGListByte(&precompiled_buffer, 0);
    }
    return prec_position;
}

static CPrecWrittenEntry *findentry(void *key)
{
    union {
        void *pp;
        UInt8 b[4];
    } u;
    UInt32 h;
    CPrecWrittenEntry *e;

    u.pp = key;
    h = (u.b[0] + (UInt32)key + u.b[1] + u.b[2] + u.b[3]) & 0x3fff;
    e = ((CPrecWrittenEntry **)(UInt32)written_entry_buckets)[h];
    while (e != NULL) {
        if (e->object == key)
            return e;
        e = e->next;
    }
    e = NULL;
    return e;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* lalloc */
/* memclrw */
/* append byte */
/* append data */
/* CPrec_NamePatch body */

static inline void CPrec_AppendDatum(TData *u, TStreamElement *bp)
{
    SInt32 size;
    void *data;
    void *p;

    size = u->tkstring.size;
    data = u->tkstring.data;
    p = CPrec_AppendAlign();
    CPrec_AppendData(data, size);
    add_serialized_bucket_entry((SInt32)(&bp->data.tkstring.data), (SInt32)(p));
}

static void CPrec_MarkSlot(HashNameNode **dst, HashNameNode *src)
{
    if (src) {
        src->id = 1;
        patch_object_reference((SInt32)(dst), src);
    }
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void insert_written_entry(void *key, void *value)
{
    CPrecWrittenEntry *n;
    CPrecWrittenEntry **bucket;

    bucket = &written_entry_buckets[CPrec_Hash((SInt32)key)];
    n = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    n->object = key;
    n->image_position = value;
    n->next = *bucket;
    *bucket = n;
}

static inline void *pad_to_four_byte_alignment(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void forward_data_and_accumulate_length(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_zero_bytes_to_align_offset(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData_004dd4e0(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static inline void add_written_entry(void *key, void *value)
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

static inline void *align_prec_position(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData_004dd660(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static inline void mark_and_patch_name_reference(void *dst, HashNameNode *name)
{
    name->id = 1;
    patch_object_reference((SInt32)(dst), (void *)(name));
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* lalloc */
/* append byte */
/* append data */

static inline CPrecWrittenEntry *find_object_in_written_entry_bucket(void *key)
{
    CPrecWrittenEntry *n;
    for (n = written_entry_buckets[CPrec_Hash((SInt32)key)]; n != NULL; n = n->next)
        if (n->object == key)
            return n;
    return NULL;
}

static inline void insert_written_entry_by_object(void *key, void *value)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;

    bucket = &written_entry_buckets[CPrec_Hash((SInt32)key)];
    n = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    n->object = key;
    n->image_position = value;
    n->next = *bucket;
    *bucket = n;
}

static inline void *append_glist_alignment_bytes(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void append_data_and_update_length(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *align_to_four(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void forward_data_and_accumulate_len(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static inline int add_serialized_method(MethRec **field, MethRec *position)
{
    return add_serialized_bucket_entry((SInt32)field, (SInt32)position);
}

static inline int add_serialized_selector_link(SelectorMethod **field, SelectorMethod *position)
{
    return add_serialized_bucket_entry((SInt32)field, (SInt32)position);
}

static inline int add_serialized_pointer(const void *field, const void *position)
{
    return add_serialized_bucket_entry((SInt32)field, (SInt32)position);
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *pad_position_to_alignment(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData_004de570(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static SInt32 hash_value_bytes(SInt32 value)
{
    CPrecBytes b;
    b.value = value;
    return (b.bytes[0] + value + b.bytes[1] + b.bytes[2] + b.bytes[3]) & 0x3fff;
}

static void prepend_written_entry_to_hash_bucket(TypeBitfield *key, TypeBitfield *value)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;

    bucket = &written_entry_buckets[hash_value_bytes((SInt32)key)];
    n = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    n->object = key;
    n->image_position = value;
    n->next = *bucket;
    *bucket = n;
}

static void *pad_precompiled_buffer(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static void append_precompiled_data(TypeBitfield *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static SInt32 hash_value(SInt32 value)
{
    CPrecBytes b;
    b.value = value;
    return (b.bytes[0] + value + b.bytes[1] + b.bytes[2] + b.bytes[3]) & 0x3fff;
}

static void insert_written_entry_by_key(void *key, void *value)
{
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *n;

    bucket = &written_entry_buckets[hash_value((SInt32)key)];
    n = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    n->object = key;
    n->image_position = value;
    n->next = *bucket;
    *bucket = n;
}

static void *CPrec_AppendAlign_004de900(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static void append_data_and_count_bytes(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_zero_bytes_to_align_position(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void accumulate_data_length_and_forward_data(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

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

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void CPrec_AppendData_004dee40(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static inline void *append_align(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *align_and_return_offset(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static inline void CPrec_AppendData_004df0c0(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

#ifndef DEF_fn_004dbf80
#endif
#ifndef DEF_fn_004de740
#endif
#ifndef DEF_fn_004de900
#endif
#ifndef DEF_fn_004de9e0
#endif
#ifndef DEF_fn_004dee40
#endif
#ifndef DEF_fn_004deff0
#endif
#ifndef DEF_fn_004df0c0
#endif
#ifndef DEF_fn_004df290
#endif

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
            TypeBitfield *patch = serialize_type_bitfield(bitfieldType);
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

static inline void patch_serialized_field(const void *field, SInt32 index)
{
    add_serialized_bucket_entry((SInt32)field, index);
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

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline SInt32 append_prec_alignment_padding(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return prec_position;
}

static inline void CPrec_AppendData_004df620(void *data, SInt32 len)
{
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, data, len);
    prec_position += len;
}

static UInt32 CPrec_HashPtr(const void *key)
{
    union {
        UInt8 b[4];
        const void *p;
    } u;
    u.p = key;
    return ((UInt32)key + u.b[0] + u.b[1] + u.b[2] + u.b[3]) & 0x3fff;
}

static CPrecWrittenEntry *CPrec_FindRecord(const void *key)
{
    CPrecWrittenEntry *r;

    r = written_entry_buckets[CPrec_HashPtr(key)];
    while (r != NULL) {
        if (r->object == key)
            return r;
        r = r->next;
    }
    return NULL;
}

static inline void CPrec_AddRecord(const void *key, UInt32 pos)
{
    CPrecWrittenEntry *r;
    UInt32 h;
    CPrecWrittenEntry **slot;

    h = CPrec_HashPtr(key);
    slot = &written_entry_buckets[h];
    r = (CPrecWrittenEntry *)lalloc(12);
    r->object = (void *)key;
    r->image_position = (void *)pos;
    r->next = *slot;
    *slot = r;
}

static inline UInt32 CPrec_AppendAlign_004df7a0(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return prec_position;
}

static inline void CPrec_AppendLong(UInt32 v)
{
    if (data_00581c28)
        AppendGListLong(&precompiled_buffer, v);
    prec_position += 4;
}

static inline void CPrec_AppendShort(SInt16 v)
{
    if (data_00581c28)
        AppendGListWord(&precompiled_buffer, v);
    prec_position += 2;
}

static inline void CPrec_AppendByte(UInt8 v)
{
    if (data_00581c28)
        AppendGListByte(&precompiled_buffer, v);
    prec_position += 1;
}

static inline void CPrec_AppendOffset(void *key)
{
    CPrecWrittenEntry *r;
    SerializedBucketEntry *np;

    if (key != NULL) {
        if ((r = CPrec_FindRecord(key)) == NULL)
            CError_FATAL(621);
        if (data_00581c28) {
            np = (SerializedBucketEntry *)lalloc(8);
            np->offset = prec_position;
            np->next = serialized_bucket_entries;
            serialized_bucket_entries = np;
            if ((np->offset & 0x80000001) != 0)
                CError_FATAL(628);
        }
        CPrec_AppendLong((UInt32)r->image_position);
    } else {
        CPrec_AppendLong(0);
    }
}

static inline void CPrec_AppendObj(HashNameNode *key)
{
    if (key != NULL) {
        CPrec_AppendOffset(key);
        key->id = 1;
    }
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

static inline void *append_prec_alignment(void)
{
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    return (void *)prec_position;
}

static void CPrec_AppendWord32(SInt32 v)
{
    if (data_00581c28)
        AppendGListLong(&precompiled_buffer, v);
    prec_position += 4;
}

static void CPrec_AppendWord16(SInt16 v)
{
    if (data_00581c28)
        AppendGListWord(&precompiled_buffer, v);
    prec_position += 2;
}

static void CPrec_AppendString(const char *s)
{
    SInt32 len = strlen(s) + 1;
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, (void *)s, len);
    prec_position += len;
}

void write_serialized_buckets(void)
{
    SerializedBucketEntry *entry;
    unsigned int value;
    int entryCount;
    int bucketIndex;

    bucketIndex = 0;
    if (0 < serialized_bucket_count) {
        do {
            entryCount = 0;
            entry = serialized_buckets[bucketIndex].list;
            while (entry != NULL) {
                entry = entry->next;
                entryCount = entryCount + 1;
            }
            if (entryCount != 0) {
                if (data_00581c28 != '\0') {
                    AppendGListLong(&precompiled_buffer, entryCount);
                }
                prec_position += 4;
                if (data_00581c28 != '\0') {
                    AppendGListLong(&precompiled_buffer, bucketIndex);
                }
                prec_position += 4;
                for (entry = serialized_buckets[bucketIndex].list; entry != NULL; entry = entry->next) {
                    value = entry->offset;
                    if (data_00581c28 != '\0') {
                        AppendGListLong(&precompiled_buffer, value);
                    }
                    prec_position += 4;
                }
            }
            bucketIndex = bucketIndex + 1;
        } while (bucketIndex < serialized_bucket_count);
    }
    if (data_00581c28 != '\0') {
        AppendGListLong(&precompiled_buffer, 0);
    }
    prec_position += 4;
}

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

unsigned int serialize_pending_object_class_list(struct CallbackAction *entry)
{
    SInt32 firstOffset;
    SInt32 valueOffset;
    SInt32 recordOffset;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    recordOffset = prec_position;
    firstOffset = prec_position;
    for (;;) {
        if (data_00581c28 != '\0') {
            AppendGListData(&precompiled_buffer, entry, sizeof(*entry));
        }
        prec_position += sizeof(*entry);
        valueOffset = write_object(entry->obj);
        add_serialized_bucket_entry(recordOffset + 4, valueOffset);
        valueOffset = write_type((Type *)entry->tclass);
        add_serialized_bucket_entry(recordOffset + 8, valueOffset);
        if (entry->next == NULL)
            break;
        if (data_00581c28 != '\0') {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        valueOffset = prec_position;
        add_serialized_bucket_entry(recordOffset, valueOffset);
        recordOffset = valueOffset;
        entry = entry->next;
    }
    return firstOffset;
}

PendingBuffer *serialize_pending_buffers(PendingBuffer *item)
{
    PendingBuffer *first;
    PendingBuffer *current;
    OLinkList *relocation;
    OLinkList *currentRelocation;
    OLinkList *firstRelocation;
    SInt32 size;
    char *buffer;

    first = current = PrecItemAlign();
    while (1) {
        PendingBuffer *next;
        PrecItemData(item, sizeof(PendingBuffer));
        add_serialized_bucket_entry((SInt32)&current->owner, write_object(item->owner));
        if (item->buffer) {
            char *data = item->buffer;
            size = item->owner->type->size;
            buffer = PrecItemAlign();
            PrecItemData(data, size);
            add_serialized_pointer_entry((SInt32)&current->buffer, buffer);
        }
        if ((relocation = item->value)) {
            firstRelocation = currentRelocation = PrecItemAlign();
            while (1) {
                OLinkList *nextRelocation;
                PrecItemData(relocation, sizeof(OLinkList));
                add_serialized_bucket_entry((SInt32)&currentRelocation->obj, write_object(relocation->obj));
                if (!relocation->next)
                    break;
                add_serialized_pointer_entry((SInt32)&currentRelocation->next, nextRelocation = PrecItemAlign());
                currentRelocation = nextRelocation;
                relocation = relocation->next;
            }
            add_serialized_pointer_entry((SInt32)&current->value, firstRelocation);
        }
        if (!item->next)
            break;
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = PrecItemAlign()));
        current = next;
        item = item->next;
    }
    return first;
}

unsigned int write_csomrefnode_list(struct CSOMRefNode *record)
{
    unsigned int listOffset;
    SInt32 offset;
    SInt32 recordOffset;

    if (data_00581c28 != 0) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    recordOffset = prec_position;
    listOffset = prec_position;
    for (;;) {
        if (data_00581c28 != 0) {
            AppendGListData(&precompiled_buffer, record, 0x12);
        }
        prec_position += 0x12;
        offset = write_object(record->object);
        add_serialized_bucket_entry(recordOffset + 4, offset);
        offset = write_type((Type *)record->theclass);
        add_serialized_bucket_entry(recordOffset + 8, offset);
        if (record->next == NULL)
            break;
        if (data_00581c28 != 0) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        offset = prec_position;
        add_serialized_bucket_entry(recordOffset, offset);
        recordOffset = offset;
        record = record->next;
    }
    return listOffset;
}

void serialize_namespace_usings_and_hash(void)
{
    SInt32 usingsOffset;
    SInt32 entryOffset;
    NameSpaceList *usingEntry;
    SInt32 nextOffset;
    SInt32 bucket;
    SInt32 nameOffset;

    CError_ASSERT(3777, cscope_root->is_hash);

    if ((usingEntry = cscope_root->usings) != NULL) {
        if (data_00581c28) {
            while (prec_position & 3) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        entryOffset = prec_position;
        usingsOffset = entryOffset;
        for (;;) {
            if (data_00581c28)
                AppendGListData(&precompiled_buffer, usingEntry, sizeof(*usingEntry));
            prec_position += sizeof(*usingEntry);
            add_serialized_bucket_entry(entryOffset + 4, get_namespace_patch(usingEntry->nspace));
            if (usingEntry->next == NULL)
                break;
            if (data_00581c28) {
                while (prec_position & 3) {
                    AppendGListByte(&precompiled_buffer, 0);
                    prec_position++;
                }
            }
            nextOffset = prec_position;
            add_serialized_bucket_entry(entryOffset, nextOffset);
            entryOffset = nextOffset;
            usingEntry = usingEntry->next;
        }
        if (data_00581c28)
            prec_header->usingsOffset = usingsOffset;
    }
    if (data_00581c28)
        prec_header->nameCount = cscope_root->names;

    for (bucket = 0; bucket < 0x400; bucket++) {
        if (cscope_root->data.hash[bucket] != NULL) {
            nameOffset = write_namespace_name(cscope_root->data.hash[bucket], 1);
            if (data_00581c28) {
                if (data_00581c26 != 0)
                    return;
                prec_header->namespaceNameOffsets[bucket] = nameOffset;
            }
        }
    }
}

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

SInt32 write_namespace_object_list(NameSpaceObjectList *x)
{
    CPrecWrittenEntry *e;
    NameSpaceObjectList *first;
    NameSpaceObjectList *current;
    NameSpaceObjectList *next;
    NameSpaceObjectList *value;
    NameSpaceObjectList *tmp;

    if ((e = CPrec_FindAddrPatch(x)))
        return (SInt32)e->image_position;
    CPrec_NewAddrPatch(x, first = current = append_zero_bytes_to_alignment());
    while (1) {
        append_prec_data(x, 8);
        add_serialized_bucket_entry((SInt32)(&current->object), dispatch_obj_by_otype(x->object));
        if (!x->next)
            break;
        if ((e = CPrec_FindAddrPatch(x->next))) {
            add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(e->image_position));
            break;
        }
        value = next = append_zero_bytes_to_alignment();
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(tmp = next));
        x = x->next;
        current = next;
        CPrec_NewAddrPatch(x, value);
    }
    return (SInt32)first;
}

SInt32 dispatch_obj_by_otype(ObjBase *obj)
{
    switch (obj->otype) {
        default:
            CError_FATAL(3566);
        case OT_ENUMCONST: {
            ObjEnumConst *enumobj = (ObjEnumConst *)obj;
            return write_enum_const(enumobj);
        }
        case OT_TYPE: {
            ObjType *typeobj = (ObjType *)obj;
            write_objtype(typeobj);
            return;
        }
        case OT_TYPETAG: {
            ObjType *typeobj = (ObjType *)obj;
            append_objtype_image(typeobj);
            return;
        }
        case OT_NAMESPACE:
            return (SInt32)CPrec_GetObjNameSpacePatch((ObjNameSpace *)obj);
        case OT_MEMBERVAR: {
            ObjMemberVar *member = (ObjMemberVar *)obj;
            return serialize_membervars(member);
        }
        case OT_OBJECT:
            return write_object((Object *)obj);
    }
}

UInt32 write_object(Object *obj)
{
    CPrecWrittenEntry *entry;
    SInt32 offset;
    HashNameNode *name;

    if (fn_0041b910(compiler_plugin_cu.context) != 0)
        CError_Longjmp();

    if ((entry = find_written_entry(obj)) != NULL)
        return (UInt32)entry->image_position;

    {
        UInt32 value;
        CPrecWrittenEntry **bucket;
        CPrecWrittenEntry *entry;
        value = offset = append_alignment_padding();
        bucket = &written_entry_buckets[CPrec_HashNew(obj)];
        entry = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
        entry->object = obj;
        entry->image_position = (void *)value;
        entry->next = *bucket;
        *bucket = entry;
    }

    obj->dwarfLinks.toc = NULL;
    obj->debugInfo.symbol = NULL;

    if ((obj->qual & Q_IS_TEMPLATED) != 0 && obj->datatype != DALIAS) {
        append_data_and_count_size(obj, 0x3a);
        add_serialized_bucket_entry(offset + 0x36, write_object(((ObjectTemplated *)obj)->parent));
    } else
        append_data_and_count_size(obj, 0x36);

    if (obj->nspace != NULL)
        add_serialized_bucket_entry(offset + 6, get_namespace_patch(obj->nspace));

    if ((name = obj->name) != NULL)
        patch_name_reference(offset + 0xa, name);

    add_serialized_bucket_entry(offset + 0xe, write_type(obj->type));

    switch (obj->datatype) {
        case DLOCAL:
            CError_FATAL(3459);
            break;
        case DFUNC:
        case DVFUNC: {
            Type *type;
            DefArgCtorInfo *defarg;
            if ((type = obj->type)->type == TYPEFUNC && (((TypeFunc *)type)->flags & 0x400) != 0)
                add_serialized_bucket_entry(offset + 0x26, (SInt32)write_template_function(obj->u.templateFunction));
            else if ((obj->qual & Q_INLINE) != 0 && obj->u.func.u != NULL)
                add_serialized_bucket_entry(offset + 0x26, serialize_cprec_rec((CInlineInfo *)obj->u.func.u));
            if ((defarg = obj->u.func.defargdata) != NULL) {
                UInt32 defargOffset = align_to_four_byte_boundary();
                fn_004e0010(defarg, sizeof(*defarg));
                add_serialized_bucket_entry(defargOffset, write_object(defarg->default_func));
                add_serialized_bucket_entry(defargOffset + 4, write_enode(defarg->default_arg));
                add_serialized_bucket_entry(offset + 0x2a, defargOffset);
            }
            if ((name = obj->u.func.linkname) != NULL)
                patch_name_reference(offset + 0x2e, name);
            break;
        }
        case DDATA: {
            if (obj->u.data.info != NULL)
                CError_FATAL(3494);
            if ((obj->qual & Q_INLINE_DATA) != 0) {
                switch ((SInt8)obj->type->type) {
                    case TYPEFLOAT:
                        CPrec_Float(obj, offset);
                        break;
                    case TYPEINT:
                    case TYPEENUM:
                    case TYPEPOINTER:
                        break;
                    default:
                        CError_FATAL(3510);
                }
            }
            if ((name = obj->u.data.linkname) != NULL)
                patch_name_reference(offset + 0x32, name);
            break;
        }
        case DINLINEFUNC: {
            unsigned char *data;
            SInt32 size;
            UInt32 dataOffset;
            SInt32 current, first;
            InlineXRef *xref;
            size = obj->u.ifunc.size;
            data = obj->u.ifunc.data;
            dataOffset = append_alignment_padding();
            append_data_and_count_size(data, size);
            add_serialized_bucket_entry(offset + 0x26, dataOffset);
            if ((xref = obj->u.ifunc.xrefs) != NULL) {
                SInt32 next;
                append_alignment_padding();
                first = current = prec_position;
                for (;;) {
                    UInt32 xrefSize;
                    xrefSize = (UInt32)(xref->numxrefs - 1) * sizeof(xref->xref[0]) + sizeof(*xref);
                    append_data_and_count_size(xref, (SInt32)xrefSize);
                    add_serialized_bucket_entry(current + 4, write_object(xref->object));
                    if (xref->next == NULL)
                        break;
                    next = append_alignment_padding();
                    add_serialized_bucket_entry(current, next);
                    current = next;
                    xref = xref->next;
                }
                add_serialized_bucket_entry(offset + 0x2e, first);
            }
            break;
        }
        case DALIAS:
            add_serialized_bucket_entry(offset + 0x26, write_object(obj->u.alias.object));
            if (obj->u.alias.member != NULL)
                add_serialized_bucket_entry(offset + 0x2a, write_bclass_list(obj->u.alias.member));
            break;
        case DABSOLUTE:
            break;
        default:
            CError_FATAL(3549);
    }

    if (data_00581c28 != 0)
        obj->datatype = 0xff;

    return offset;
}

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

ObjType *append_objtype_image(ObjType *x)
{
    CPrecWrittenEntry *e;
    ObjType *p;

    if ((e = CPrec_FindAddrPatch(x)))
        return e->image_position;
    CPrec_NewAddrPatch(x, p = append_zero_alignment_padding());
    CPrec_AppendData_004da950(x, 6);
    add_serialized_bucket_entry((SInt32)(&p->type), write_type(x->type));
    return p;
}

ObjType *write_objtype(ObjType *x)
{
    CPrecWrittenEntry *e;
    ObjType *p;

    if ((e = CPrec_FindAddrPatch(x)))
        return e->image_position;
    CPrec_NewAddrPatch(x, p = append_zero_padding_to_alignment());
    append_data_and_count_length(x, 10);
    add_serialized_bucket_entry((SInt32)(&p->type), write_type(x->type));
    return p;
}

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

SInt32 serialize_cprec_rec(CInlineInfo *record)
{
    SInt32 base;

    memclrw(&record->fileinfo, 10);
    record->f1c = 0;
    record->tokenoffset = 0;
    record->tokenline = 0;

    if (data_00581c28 != 0) {
        for (; (prec_position & 3) != 0; prec_position++) {
            AppendGListByte(&precompiled_buffer, 0);
        }
    }

    base = prec_position;
    if (data_00581c28 != 0) {
        AppendGListData(&precompiled_buffer, record, sizeof(*record));
    }
    prec_position += sizeof(*record);

    if (record->nargs != 0) {
        add_serialized_bucket_entry(base + 2,
                                    serialize_reference_type_entries((unsigned int *)record->arginfo, record->nargs));
    }
    if (record->nlocals != 0) {
        add_serialized_bucket_entry(
            base + 8, serialize_reference_type_entries((unsigned int *)record->localinfo, record->nlocals));
    }
    if (record->nstmts != 0) {
        add_serialized_bucket_entry(base + 0xe, write_prec_recs((IStmtRec *)record->stmtinfo, record->nstmts));
    }
    return base;
}

unsigned int serialize_reference_type_entries(unsigned int *entries, short count)
{
    unsigned int offset;
    HashNameNode *reference;
    unsigned int start;
    short index;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            ++prec_position;
        }
    }
    start = offset = prec_position;
    if (data_00581c28 != '\0') {
        AppendGListData(&precompiled_buffer, entries, count * 16);
    }
    index = 0;
    prec_position += count * 16;
    if (count > 0) {
        do {
            reference = (HashNameNode *)*entries;
            reference->id = 1;
            patch_object_reference(offset, reference);
            add_serialized_bucket_entry(offset + 4, write_type((Type *)entries[1]));
            ++index;
            offset += 16;
            entries += 4;
        } while (index < count);
    }
    return start;
}

UInt32 write_prec_recs(IStmtRec *recs, SInt16 count)
{
    UInt32 recordOffset;
    SInt16 index;
    UInt32 start;
    SInt32 entryOffset;

    start = recordOffset = CPrec_004daf90_Align();
    if (data_00581c28 != 0) {
        AppendGListData(&precompiled_buffer, recs, count * sizeof(*recs));
    }
    prec_position += count * sizeof(*recs);
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
                    UInt32 tableOffset;
                    SInt32 entryIndex;
                    Object *object;

                    tableOffset = CPrec_004daf90_Align();
                    if (data_00581c28 != 0) {
                        AppendGListData(&precompiled_buffer, table, tableSize);
                    }
                    prec_position += tableSize;
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

unsigned int write_precompiled_expression_record(InlineSwitchData *record)
{
    SInt32 offset;
    SInt32 size;
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    offset = prec_position;
    size = (record->caseCount - 1) * 10 + 22;
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, record, size);
    prec_position += size;
    add_serialized_bucket_entry(offset, write_enode(record->expression));
    add_serialized_bucket_entry(offset + 4, write_type(record->valueType));
    return offset;
}

SInt32 write_enode(ENode *node)
{
    SInt32 listOffset;
    SInt32 firstOffset;
    SInt32 nextOffset;
    ENodeList *list;
    HashNameNode *templateName;
    HashNameNode *name;
    char *stringData;
    SInt32 stringSize;
    SInt32 offset;

    if (node->type == ETEMPLDEP && node->data.templdep.subtype == TDE_SOURCEREF)
        node->data.objlist.templargs = NULL;

    offset = PrecompBegin();
    PrecompAppend((ENodeList *)node, sizeof(*node));
    add_serialized_bucket_entry(offset + 6, write_type(node->rtype));

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
            add_serialized_bucket_entry(offset + 10, write_enode(node->data.monadic));
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
            add_serialized_bucket_entry(offset + 10, write_enode(node->data.diadic.left));
            add_serialized_bucket_entry(offset + 14, write_enode(node->data.diadic.right));
            break;
        case ECOND:
            add_serialized_bucket_entry(offset + 10, write_enode(node->data.cond.cond));
            add_serialized_bucket_entry(offset + 14, write_enode(node->data.cond.expr1));
            add_serialized_bucket_entry(offset + 18, write_enode(node->data.cond.expr2));
            break;
        case ESTRINGCONST:
            stringSize = node->data.string.size;
            stringData = node->data.string.data;
            listOffset = PrecompBegin();
            PrecompAppend((ENodeList *)stringData, stringSize);
            add_serialized_bucket_entry(offset + 14, listOffset);
            break;
        case EOBJREF:
            add_serialized_bucket_entry(offset + 10, write_object(node->data.objref));
            break;
        case EOBJLIST:
            add_serialized_bucket_entry(offset + 10, write_namespace_object_list(node->data.objlist.list));
            CError_ASSERT(2938, node->data.objlist.templargs == 0);
            if ((name = node->data.objlist.name) != NULL) {
                name->id = 1;
                patch_object_reference(offset + 18, name);
            }
            break;
        case EMFPOINTER:
            add_serialized_bucket_entry(offset + 10, write_enode(node->data.diadic.left));
            add_serialized_bucket_entry(offset + 14, write_enode(node->data.diadic.right));
            break;
        case ENULLCHECK:
            add_serialized_bucket_entry(offset + 10, write_enode(node->data.diadic.left));
            add_serialized_bucket_entry(offset + 14, write_enode(node->data.diadic.right));
            break;
        case ETEMP:
            add_serialized_bucket_entry(offset + 10, write_type(node->data.temp.type));
            break;
        case EFUNCCALL:
        case EFUNCCALLP:
            add_serialized_bucket_entry(offset + 10, write_enode(node->data.funccall.funcref));
            add_serialized_bucket_entry(offset + 18, write_type((Type *)node->data.funccall.functype));
            if ((list = node->data.funccall.args) != NULL) {
                listOffset = PrecompBegin();
                firstOffset = listOffset;
                for (;;) {
                    PrecompAppend(list, sizeof(*list));
                    add_serialized_bucket_entry(listOffset + 4, write_enode(list->node));
                    if (list->next == NULL)
                        break;
                    nextOffset = PrecompBegin();
                    add_serialized_bucket_entry(listOffset, nextOffset);
                    listOffset = nextOffset;
                    list = list->next;
                }
                add_serialized_bucket_entry(offset + 14, firstOffset);
            }
            break;
        case EMEMBER:
            add_serialized_bucket_entry(offset + 10, write_member_func_ref(node->data.emember));
            break;
        case ETEMPLDEP:
            switch (node->data.templdep.subtype) {
                case TDE_SIZEOF:
                    add_serialized_bucket_entry(offset + 10, write_type(node->data.templdep.u.typeexpr.type));
                    break;
                case TDE_CAST:
                    if ((list = node->data.templdep.u.cast.args) != NULL) {
                        SInt32 entryOffset, followingOffset, firstEntryOffset;
                        firstEntryOffset = entryOffset = PrecompBegin();
                        for (;;) {
                            PrecompAppend(list, sizeof(*list));
                            add_serialized_bucket_entry(entryOffset + 4, write_enode(list->node));
                            if (list->next == NULL)
                                break;
                            followingOffset = PrecompBegin();
                            add_serialized_bucket_entry(entryOffset, followingOffset);
                            entryOffset = followingOffset;
                            list = list->next;
                        }
                        add_serialized_bucket_entry(offset + 10, firstEntryOffset);
                    }
                    add_serialized_bucket_entry(offset + 14, write_type(node->data.templdep.u.cast.type));
                    break;
                case TDE_QUALNAME:
                    templateName = node->data.templdep.u.qual.name;
                    templateName->id = 1;
                    patch_object_reference(offset + 14, templateName);
                    add_serialized_bucket_entry(offset + 10, write_type(TYPE(node->data.templdep.u.qual.type)));
                    break;
                case TDE_OBJ:
                    add_serialized_bucket_entry(offset + 10, write_object(node->data.templdep.u.obj));
                    break;
                case TDE_SOURCEREF:
                    add_serialized_bucket_entry(offset + 10, write_enode(node->data.templdep.u.sourceref.expr));
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
    return offset;
}

unsigned int write_member_func_ref(EMemberInfo *entry)
{
    SInt32 offset;

    CError_FATAL(2848);
    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    offset = prec_position;
    if (data_00581c28)
        AppendGListData(&precompiled_buffer, entry, sizeof(*entry));
    prec_position += sizeof(*entry);
    if (entry->path)
        add_serialized_bucket_entry(offset, write_bclass_list(entry->path));
    if (entry->expr)
        add_serialized_bucket_entry(offset + 4, write_enode(entry->expr));
    if (entry->templargs)
        CError_FATAL(2863);
    add_serialized_bucket_entry(offset + 8, write_namespace_object_list(entry->list));
    return offset;
}

SInt32 serialize_cpsi_list(ExceptionAction *item)
{
    SInt32 first;
    ExceptionAction *current;
    ExceptionAction *next;
    Object **objects;
    int i;

    first = (SInt32)(current = append_glist_alignment_padding());
    while (1) {
        CPrec_AppendData_004db910(item, 0x1e);
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
                                                (SInt32)(objects = append_glist_alignment_padding()));
                    CPrec_AppendData_004db910(item->data.specification.ids,
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
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(next = append_glist_alignment_padding()));
        current = next;
        item = item->next;
    }
    return first;
}

int write_type(Type *type)
{
    long found;
    CPrecWrittenEntry *entry;
    int offset;
    Type *baseType;
    CPrecWrittenEntry *baseEntry;
    UInt32 baseOffset;
    TypeBitfield *bitfieldPatch;
    int functionOffset;
    CPrecWrittenEntry *newEntry;
    unsigned int hash;
    CPrecWrittenEntry **bucket;
    int memberPointerOffset;
    int cachedOffset;
    int enumOffset;

    entry = (CPrecWrittenEntry *)(long)CPrec_004dbc00_inline1(type, &found);
    if (found != 0)
        return (int)entry->image_position;

    switch ((signed char)type->type) {
        case TYPEPOINTER:
        case TYPEARRAY: {
            TypePointer *pointerType = (TypePointer *)type;
            return write_pointer_type(pointerType);
        }
        case TYPEENUM: {
            enumOffset = CPrec_004dbc00_inline2(type, sizeof(TypeEnum));
            if (((TypeEnum *)type)->nspace != NULL)
                add_serialized_bucket_entry(enumOffset + 6, get_namespace_patch(((TypeEnum *)type)->nspace));
            if (((TypeEnum *)type)->enumlist != NULL)
                add_serialized_bucket_entry(enumOffset + 10, write_enum_const(((TypeEnum *)type)->enumlist));
            add_serialized_bucket_entry(enumOffset + 14, write_type(((TypeEnum *)type)->enumtype));
            if (((TypeEnum *)type)->enumname != NULL)
                patch_hash_name_reference(enumOffset + 18, ((TypeEnum *)type)->enumname);
            return enumOffset;
        }
        case TYPEBITFIELD: {
            cachedOffset = offset = align_to_four_byte_boundary();
            hash = CException_HashType(type);
            bucket = &written_entry_buckets[hash];
            newEntry = (CPrecWrittenEntry *)lalloc(sizeof(*newEntry));
            newEntry->object = type;
            newEntry->image_position = (TypeBitfield *)cachedOffset;
            newEntry->next = *bucket;
            *bucket = newEntry;
            if (data_00581c28 != 0)
                AppendGListData(&precompiled_buffer, type, sizeof(TypeBitfield));
            prec_position += sizeof(TypeBitfield);
            baseType = ((TypeBitfield *)type)->bitfieldtype;
            baseEntry = fn_004e0680(baseType);
            if (baseEntry != NULL) {
                baseOffset = (int)baseEntry->image_position;
            } else {
                switch ((signed char)baseType->type) {
                    case TYPEPOINTER:
                    case TYPEARRAY: {
                        TypePointer *pointerType = (TypePointer *)baseType;
                        baseOffset = write_pointer_type(pointerType);
                        break;
                    }
                    case TYPEENUM: {
                        TypeEnum *enumType = (TypeEnum *)baseType;
                        TypeEnum *enumPatch = append_type_enum(enumType);
                        baseOffset = (int)enumPatch;
                        break;
                    }
                    case TYPEBITFIELD: {
                        TypeBitfield *baseBitfield = (TypeBitfield *)baseType;
                        bitfieldPatch = serialize_type_bitfield(baseBitfield);
                        baseOffset = (int)bitfieldPatch;
                        break;
                    }
                    case TYPESTRUCT:
                        baseOffset = (int)append_type_struct((TypeStruct *)baseType);
                        break;
                    case TYPEFUNC: {
                        TypeFunc *functionType = (TypeFunc *)baseType;
                        TypeFunc *functionPatch = copy_type_func(functionType);
                        baseOffset = (int)functionPatch;
                        break;
                    }
                    case TYPEMEMBERPOINTER:
                        baseOffset = (int)append_member_pointer_type((TypeMemberPointer *)baseType);
                        break;
                    case TYPETEMPLATE: {
                        TypeTemplDep *templateType = (TypeTemplDep *)baseType;
                        baseOffset = write_templdep(templateType);
                        break;
                    }
                    case TYPECLASS: {
                        TypeClass *baseClass = (TypeClass *)baseType;
                        UInt32 classPatch = write_typeclass(baseClass);
                        baseOffset = classPatch;
                        break;
                    }
                    case TYPEVOID:
                    case TYPETEMPLDEPEXPR:
                    default:
                        CError_FATAL(2691);
                        baseOffset = 0;
                }
            }
            add_serialized_bucket_entry(offset + 6, baseOffset);
            return offset;
        }
        case TYPESTRUCT:
            return (int)append_type_struct((TypeStruct *)type);
        case TYPEFUNC: {
            functionOffset = align_to_four_byte_boundary();
            add_written_type_entry(type, functionOffset);
            fn_004e0010(type, (((TypeMemberFunc *)type)->flags & FUNC_METHOD) != 0 ? sizeof(TypeMemberFunc)
                                                                                   : sizeof(TypeFunc));
            add_serialized_bucket_entry(functionOffset + 14, write_type(((TypeMemberFunc *)type)->functype));
            if (((TypeMemberFunc *)type)->args != NULL)
                add_serialized_bucket_entry(
                    functionOffset + 6, (SInt32)write_func_args(((TypeMemberFunc *)type)->args,
                                                                (((TypeMemberFunc *)type)->flags & 134218752) != 0));
            if (((TypeMemberFunc *)type)->exspecs != NULL)
                add_serialized_bucket_entry(functionOffset + 10,
                                            (SInt32)write_except_spec_list(((TypeMemberFunc *)type)->exspecs));
            if ((((TypeMemberFunc *)type)->flags & FUNC_METHOD) != 0)
                add_serialized_bucket_entry(functionOffset + 26,
                                            write_type((Type *)((TypeMemberFunc *)type)->theclass));
            return functionOffset;
        }
        case TYPEMEMBERPOINTER: {
            memberPointerOffset = CPrec_004dbc00_inline2(type, sizeof(TypeMemberPointer));
            add_serialized_bucket_entry(memberPointerOffset + 6, write_type(((TypeMemberPointer *)type)->ty1));
            add_serialized_bucket_entry(memberPointerOffset + 10, write_type(((TypeMemberPointer *)type)->ty2));
            return memberPointerOffset;
        }
        case TYPETEMPLATE: {
            TypeTemplDep *templateType = (TypeTemplDep *)type;
            return write_templdep(templateType);
        }
        case TYPECLASS: {
            TypeClass *classType = (TypeClass *)type;
            UInt32 classPatch = write_typeclass(classType);
            return classPatch;
        }
        case TYPEVOID:
        case TYPETEMPLDEPEXPR:
        default:
            CError_FATAL(2691);
            return 0;
    }
}

UInt32 write_typeclass(TypeClass *node)
{
    UInt32 result;
    UInt32 offset;
    UInt32 nextClassOffset;
    UInt32 nextOtherOffset;
    CPrecWrittenEntry *existingEntry;
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *entry;
    PrecFlag hasNextClass;
    Boolean hasNextOther;
    HashNameNode *referencedNode;

    result = offset = align_counter_with_zero_padding();

    for (;;) {
        hasNextClass = hasNextOther = 0;
        bucket = &written_entry_buckets[hashptr(node)];
        entry = (CPrecWrittenEntry *)lalloc(sizeof(*entry));
        entry->object = node;
        entry->image_position = (UInt8 *)offset;
        entry->next = *bucket;
        *bucket = entry;

        if ((node->flags & CLASS_IS_TEMPL) != 0) {
            if (data_00581c28 != 0)
                AppendGListData(&precompiled_buffer, node, sizeof(TemplClass));
            prec_position += sizeof(TemplClass);
            if (((TemplClass *)node)->next != NULL)
                hasNextClass = 1;
            if (((TemplClass *)node)->templ_parent != NULL)
                add_serialized_bucket_entry(offset + 0x36, write_type((Type *)((TemplClass *)node)->templ_parent));
            if (((TemplClass *)node)->inst_parent != NULL)
                add_serialized_bucket_entry(offset + 0x3a, write_type(TYPE(((TemplClass *)node)->inst_parent)));
            if (((TemplClass *)node)->templ__params != NULL)
                add_serialized_bucket_entry(offset + 0x3e, serialize_pre_nodes(((TemplClass *)node)->templ__params));
            if (((TemplClass *)node)->members != NULL)
                add_serialized_bucket_entry(offset + 0x42, serialize_prec_records(((TemplClass *)node)->members));
            if (((TemplClass *)node)->instances != NULL)
                add_serialized_bucket_entry(offset + 0x46, write_type((Type *)((TemplClass *)node)->instances));
            if (((TemplClass *)node)->pspec_owner != NULL)
                add_serialized_bucket_entry(offset + 0x4a, write_type(TYPE(((TemplClass *)node)->pspec_owner)));
            if (((TemplClass *)node)->pspecs != NULL)
                add_serialized_bucket_entry(offset + 0x4e, serialize_reference_entries(((TemplClass *)node)->pspecs));
            if (((TemplClass *)node)->actions != NULL)
                add_serialized_bucket_entry(offset + 0x52,
                                            serialize_template_class_declarations(((TemplClass *)node)->actions));
        } else if ((node->flags & CLASS_IS_TEMPL_INST) != 0) {
            if (data_00581c28 != 0)
                AppendGListData(&precompiled_buffer, node, sizeof(TemplClassInst));
            prec_position += sizeof(TemplClassInst);
            if (((TemplClassInst *)node)->next != NULL)
                hasNextOther = 1;
            if (((TemplClassInst *)node)->parent != NULL)
                add_serialized_bucket_entry(offset + 0x36, write_type(TYPE(((TemplClassInst *)node)->parent)));
            if (((TemplClassInst *)node)->templ != NULL)
                add_serialized_bucket_entry(offset + 0x3a, write_type(TYPE(((TemplClassInst *)node)->templ)));
            if (((TemplClassInst *)node)->inst_args != NULL)
                add_serialized_bucket_entry(offset + 0x3e,
                                            serialize_ct_state_elems(((TemplClassInst *)node)->inst_args));
            if (((TemplClassInst *)node)->oargs != NULL)
                add_serialized_bucket_entry(offset + 0x42, serialize_ct_state_elems(((TemplClassInst *)node)->oargs));
        } else {
            if (data_00581c28 != 0)
                AppendGListData(&precompiled_buffer, node, sizeof(*node));
            prec_position += sizeof(*node);
        }

        if (node->nspace != NULL)
            add_serialized_bucket_entry(offset + 0x06, get_namespace_patch(node->nspace));
        if (node->classname != NULL) {
            referencedNode = node->classname;
            referencedNode->id = 1;
            patch_object_reference(offset + 0x0a, referencedNode);
        }
        if (node->bases != NULL)
            add_serialized_bucket_entry(offset + 0x0e, (SInt32)write_class_list(node->bases));
        if (node->vbases != NULL)
            add_serialized_bucket_entry(offset + 0x12, write_vclasslist(node->vbases));
        if (node->ivars != NULL)
            add_serialized_bucket_entry(offset + 0x16, serialize_membervars(node->ivars));
        if (node->friends != NULL)
            add_serialized_bucket_entry(offset + 0x1a, serialize_entry_list(node->friends));
        if (node->vtable != NULL)
            add_serialized_bucket_entry(offset + 0x1e, write_vtable(node->vtable));
        if (node->sominfo != NULL)
            add_serialized_bucket_entry(offset + 0x22, write_som_info(node->sominfo));
        if (node->objcinfo != NULL)
            add_serialized_bucket_entry(offset + 0x26, serialize_objc_info(node->objcinfo));

        if (hasNextClass != 0) {
            existingEntry = entry = findentry(((TemplClass *)node)->next);
            if (existingEntry == NULL) {
                add_serialized_bucket_entry(offset + 0x32, nextClassOffset = align_counter_with_zero_padding());
                offset = nextClassOffset;
                node = (TypeClass *)((TemplClass *)node)->next;
                continue;
            }
            add_serialized_bucket_entry(offset + 0x32, (SInt32)entry->image_position);
        }
        if (hasNextOther != 0) {
            existingEntry = entry = findentry(((TemplClassInst *)node)->next);
            if (existingEntry == NULL) {
                add_serialized_bucket_entry(offset + 0x32, nextOtherOffset = align_counter_with_zero_padding());
                offset = nextOtherOffset;
                node = (TypeClass *)((TemplClassInst *)node)->next;
                continue;
            }
            add_serialized_bucket_entry(offset + 0x32, (SInt32)entry->image_position);
        }
        break;
    }
    return result;
}

/* writing the image */
/* image offset */
/* image buffer handle */
/* address patch table */
/* lalloc */
/* append byte */
/* append data */

TemplateFunction *write_template_function(TemplateFunction *function)
{
    CPrecWrittenEntry *entry;
    TemplFuncInstance *firstObject;
    TemplateFunction *nextImage;
    TemplFuncInstance *currentObject;
    TemplFuncInstance *object;
    TemplateFunction *next;
    TemplateFunction *current;
    TemplateFunction *first;

    if ((entry = CPrec_FindAddrPatch(function)))
        return entry->image_position;
    CPrec_NewAddrPatch(function, first = current = CPrec_AppendAlign());
    while (1) {
        memclrw(&function->deftoken, sizeof(function->deftoken));
        function->srcfile = NULL;
        function->startoffset = 0;
        function->endoffset = 0;
        CPrec_AppendData(function, sizeof(*function));
        if (function->original)
            add_serialized_bucket_entry((SInt32)&current->original,
                                        (SInt32)write_template_function(function->original));
        CPrec_NamePatch(&current->name, function->name);
        if (function->params)
            add_serialized_bucket_entry((SInt32)&current->params, serialize_pre_nodes(function->params));
        if (function->stream.firsttoken)
            add_serialized_bucket_entry(
                (SInt32)&current->stream.firsttoken,
                (SInt32)append_saved_prep_tokens(function->stream.firsttoken, function->stream.tokens));
        add_serialized_bucket_entry((SInt32)&current->tfunc, write_object(function->tfunc));
        if ((object = function->instances)) {
            TemplFuncInstance *nextObject;
            firstObject = currentObject = CPrec_AppendAlign();
            while (1) {
                CPrec_AppendData(object, sizeof(*object));
                add_serialized_bucket_entry((SInt32)&currentObject->object, write_object(object->object));
                add_serialized_bucket_entry((SInt32)&currentObject->args, serialize_ct_state_elems(object->args));
                if (!object->next)
                    break;
                add_serialized_bucket_entry((SInt32)&currentObject->next, (SInt32)(nextObject = CPrec_AppendAlign()));
                currentObject = nextObject;
                object = object->next;
            }
            add_serialized_bucket_entry((SInt32)&current->instances, (SInt32)firstObject);
        }
        if (!function->next)
            break;
        if ((entry = CPrec_FindAddrPatch(function->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        {
            TemplateFunction *image;
            add_serialized_bucket_entry((SInt32)&current->next,
                                        (SInt32)(nextImage = next = CPrec_AppendAlign(), image = next));
            current = next;
            function = function->next;
            CPrec_NewAddrPatch(function, nextImage);
        }
    }
    return first;
}

SInt32 serialize_template_class_declarations(TemplateAction *list)
{
    TemplateAction *declaration;
    SInt32 result;
    SInt32 imageOffset;
    SInt32 nextOffset;

    declaration = list;
    if (data_00581c28) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    imageOffset = prec_position;
    result = imageOffset;
    for (;;) {
        memclrw(&declaration->source_ref, sizeof(declaration->source_ref));
        if (data_00581c28)
            AppendGListData(&precompiled_buffer, declaration, sizeof(*declaration));
        prec_position += sizeof(*declaration);
        switch (declaration->type) {
            case TAT_NESTEDCLASS:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.tclasstype),
                                            write_type(TYPE(declaration->u.tclasstype)));
                break;
            case TAT_ENUMTYPE:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.enumtype),
                                            write_type(TYPE(declaration->u.enumtype)));
                break;
            case TAT_FRIEND:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.tfriend),
                                            write_prec_input_record(declaration->u.tfriend));
                break;
            case TAT_ENUMERATOR:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.enumerator.objenumconst),
                                            write_enum_const(declaration->u.enumerator.objenumconst));
                if (declaration->u.enumerator.initexpr)
                    add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.enumerator.initexpr),
                                                write_enode(declaration->u.enumerator.initexpr));
                break;
            case TAT_BASE:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.base.type),
                                            write_type(declaration->u.base.type));
                if (declaration->u.base.insert_after)
                    add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.base.insert_after),
                                                (SInt32)write_class_list(declaration->u.base.insert_after));
                break;
            case TAT_OBJECTINIT:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.objectinit.object),
                                            write_object(declaration->u.objectinit.object));
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.objectinit.initexpr),
                                            write_enode(declaration->u.objectinit.initexpr));
                break;
            case TAT_USINGDECL:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.usingdecl.type),
                                            write_templdep(declaration->u.usingdecl.type));
                break;
            case TAT_OBJECTDEF:
                add_serialized_bucket_entry(imageOffset + offsetof(TemplateAction, u.refobj),
                                            dispatch_obj_by_otype(declaration->u.refobj));
                break;
            default:
                CError_FATAL(2313);
                break;
        }
        if (declaration->next == NULL)
            break;
        if (data_00581c28) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        nextOffset = prec_position;
        add_serialized_bucket_entry(imageOffset, nextOffset);
        imageOffset = nextOffset;
        declaration = declaration->next;
    }
    return result;
}

unsigned int write_prec_input_record(TemplateFriend *record)
{
    SInt32 offset;
    SInt32 reference;

    memclrw(&record->fileoffset, sizeof(record->fileoffset));
    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            ++prec_position;
        }
    }
    offset = prec_position;
    if (data_00581c28 != '\0') {
        AppendGListData(&precompiled_buffer, record, sizeof(*record));
    }
    prec_position += sizeof(*record);
    if (record->decl.thetype != NULL) {
        reference = write_type(record->decl.thetype);
        add_serialized_bucket_entry(offset, reference);
    }
    if (record->decl.nspace != NULL) {
        SInt32 namespaceReference = get_namespace_patch(record->decl.nspace);
        add_serialized_bucket_entry(offset + 8, namespaceReference);
    }
    if (record->decl.name != NULL) {
        HashNameNode *target = record->decl.name;
        target->id = 1;
        patch_object_reference(offset + 0xc, target);
    }
    if (record->decl.expltargs != NULL) {
        reference = serialize_ct_state_elems(record->decl.expltargs);
        add_serialized_bucket_entry(offset + 0x10, reference);
    }
    if (record->stream.firsttoken != NULL) {
        TStreamElement *entries = append_saved_prep_tokens(record->stream.firsttoken, record->stream.tokens);
        add_serialized_bucket_entry(offset + 0x28, (SInt32)entries);
    }
    return offset;
}

unsigned int serialize_reference_entries(TemplPartialSpec *record)
{
    SInt32 startOffset;
    SInt32 targetOffset;
    SInt32 recordOffset;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    recordOffset = prec_position;
    startOffset = prec_position;
    while (1) {
        if (data_00581c28 != '\0') {
            AppendGListData(&precompiled_buffer, record, sizeof(*record));
        }
        prec_position += sizeof(*record);
        if (record->templ != NULL) {
            targetOffset = write_type((Type *)record->templ);
            add_serialized_bucket_entry(recordOffset + 4, targetOffset);
        }
        if (record->args != NULL) {
            TemplArg *args = record->args;
            targetOffset = serialize_ct_state_elems(args);
            add_serialized_bucket_entry(recordOffset + 8, targetOffset);
        }
        if (record->next == NULL)
            break;
        if (data_00581c28 != '\0') {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        targetOffset = prec_position;
        add_serialized_bucket_entry(recordOffset, targetOffset);
        recordOffset = targetOffset;
        record = record->next;
    }
    return startOffset;
}

UInt32 serialize_prec_records(TemplateMember *record)
{
    SInt32 first_offset;
    SInt32 record_offset;
    SInt32 next_offset;

    if (data_00581c28 != 0) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    record_offset = prec_position;
    first_offset = record_offset;
    for (;;) {
        memclrw(&record->fileoffset.file,
                offsetof(TemplateMember, stream.tokens) - offsetof(TemplateMember, fileoffset.file));
        record->srcfile = NULL;
        record->startoffset = NULL;
        record->endoffset = NULL;
        if (data_00581c28 != 0) {
            AppendGListData(&precompiled_buffer, record, 0x2a);
        }
        prec_position += 0x2a;
        if (record->params != NULL) {
            add_serialized_bucket_entry(record_offset + offsetof(TemplateMember, params),
                                        serialize_pre_nodes(record->params));
        }
        add_serialized_bucket_entry(record_offset + offsetof(TemplateMember, object), write_object(record->object));
        if (record->stream.firsttoken != NULL) {
            add_serialized_bucket_entry(
                record_offset + offsetof(TemplateMember, stream.firsttoken),
                (SInt32)append_saved_prep_tokens(record->stream.firsttoken, record->stream.tokens));
        }
        if (record->next == NULL) {
            break;
        }
        if (data_00581c28 != 0) {
            while (prec_position & 3) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        next_offset = prec_position;
        add_serialized_bucket_entry(record_offset, prec_position);
        record_offset = next_offset;
        record = record->next;
    }
    return first_offset;
}

TStreamElement *append_saved_prep_tokens(TStreamElement *recs, SInt32 n)
{
    TStreamElement *rp;
    TStreamElement *first;
    TStreamElement *bp;
    TStreamElement tmp;
    SavedPrepTokenList *node;
    HashNameNode *d;
    SInt32 i;

    for (i = 0, rp = recs; i < n; i++, rp++) {
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

    first = bp = CPrec_AppendAlign();
    CPrec_AppendData(recs, n * sizeof(TStreamElement));
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
                d = (HashNameNode *)rp->data.tkstring.data;
                d->id = 1;
                patch_object_reference((SInt32)&bp->data.tkstring.data, d);
                break;
            case -5:
            case -4:
                CPrec_AppendDatum(&rp->data, bp);
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

SInt32 serialize_pre_nodes(TemplParam *node)
{
    TemplParam *parameter = node;
    SInt32 firstOffset;
    SInt32 nodeOffset;
    SInt32 nextOffset;
    HashNameNode *name;

    if (data_00581c28) {
        while (prec_position & 3) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    nodeOffset = prec_position;
    firstOffset = nodeOffset;
    for (;;) {
        if (data_00581c28) {
            AppendGListData(&precompiled_buffer, parameter, sizeof(*parameter));
        }
        prec_position += sizeof(*parameter);
        if ((name = parameter->name) != NULL) {
            name->id = 1;
            patch_object_reference(nodeOffset + 4, name);
        }
        if (parameter->pid.type != 0) {
            if (parameter->data.typeparam.type != NULL) {
                add_serialized_bucket_entry(nodeOffset + 12, write_type(parameter->data.typeparam.type));
            }
        } else {
            add_serialized_bucket_entry(nodeOffset + 12, write_type(parameter->data.paramdecl.type));
            if (parameter->data.paramdecl.defaultarg != NULL) {
                add_serialized_bucket_entry(nodeOffset + 20, write_enode(parameter->data.paramdecl.defaultarg));
            }
        }
        if (parameter->next == NULL) {
            break;
        }
        if (data_00581c28) {
            while (prec_position & 3) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        nextOffset = prec_position;
        add_serialized_bucket_entry(nodeOffset, nextOffset);
        nodeOffset = nextOffset;
        parameter = parameter->next;
    }
    return firstOffset;
}

SInt32 serialize_ct_state_elems(TemplArg *element)
{
    SInt32 base;
    SInt32 pos;
    SInt32 nextPos;
    SInt32 index;

    if (data_00581c28) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    pos = prec_position;
    base = pos;
    for (;;) {
        if (data_00581c28) {
            AppendGListData(&precompiled_buffer, element, sizeof(*element));
        }
        prec_position += sizeof(*element);
        if (element->pid.type) {
            if (element->data.typeparam.type) {
                index = write_type(element->data.typeparam.type);
                add_serialized_bucket_entry(pos + offsetof(TemplArg, data), index);
            }
        } else {
            if (element->data.paramdecl.expr) {
                index = write_enode(element->data.paramdecl.expr);
                add_serialized_bucket_entry(pos + offsetof(TemplArg, data), index);
            }
        }
        if (element->next == NULL)
            break;
        if (data_00581c28) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        nextPos = prec_position;
        add_serialized_bucket_entry(pos, nextPos);
        pos = nextPos;
        element = element->next;
    }

    return base;
}

unsigned int serialize_objc_info(struct ObjCInfo *info)
{
    SInt32 infoPosition;
    SInt32 objectPosition;
    Object *auxiliaryObject;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    infoPosition = prec_position;
    if (data_00581c28 != '\0') {
        AppendGListData(&precompiled_buffer, info, 0x1a);
    }
    prec_position += 0x1a;
    if (info->classobject != NULL) {
        objectPosition = write_object(info->classobject);
        add_serialized_bucket_entry(infoPosition, objectPosition);
    }
    if (info->metaclassobject != NULL) {
        objectPosition = write_object(info->metaclassobject);
        add_serialized_bucket_entry(infoPosition + offsetof(struct ObjCInfo, metaclassobject), objectPosition);
    }
    if ((auxiliaryObject = info->auxiliaryObject) != NULL) {
        objectPosition = write_object(auxiliaryObject);
        add_serialized_bucket_entry(infoPosition + offsetof(struct ObjCInfo, auxiliaryObject), objectPosition);
    }
    if (info->methods != NULL) {
        MethRec *methods = write_methrec(info->methods);
        add_serialized_bucket_entry(infoPosition + offsetof(struct ObjCInfo, methods), (SInt32)methods);
    }
    if (info->protocols != NULL) {
        ObjectList *protocols = write_object_list(info->protocols);
        add_serialized_bucket_entry(infoPosition + offsetof(struct ObjCInfo, protocols), (SInt32)protocols);
    }
    if (info->vars != NULL) {
        CRec *variables = write_crec_list(info->vars);
        add_serialized_bucket_entry(infoPosition + offsetof(struct ObjCInfo, vars), (SInt32)variables);
    }
    return infoPosition;
}

CRec *write_crec_list(CRec *record)
{
    CPrecWrittenEntry *entry;
    CRec *value;
    CRec *tmp;
    CRec *current;
    CRec *first;
    CRec *next;

    if ((entry = CPrec_FindAddrPatch(record)))
        return (CRec *)entry->image_position;
    insert_written_entry(record, first = current = pad_to_four_byte_alignment());

    while (1) {
        forward_data_and_accumulate_length(record, 0x10);
        CPrec_NamePatch(&current->name, record->name);
        if (record->bases) {
            ObjectList *reference = write_object_list(record->bases);
            add_serialized_bucket_entry((SInt32)&current->bases, (SInt32)reference);
        }
        if (record->methods)
            add_serialized_bucket_entry((SInt32)&current->methods, (SInt32)write_methrec(record->methods));
        if (!record->next)
            break;
        if ((entry = CPrec_FindAddrPatch(record->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)(CRec *)entry->image_position);
            break;
        }
        value = tmp = next = pad_to_four_byte_alignment();
        add_serialized_bucket_entry((SInt32)&current->next, (SInt32)tmp);
        current = next;
        record = record->next;
        insert_written_entry(record, value);
    }
    return first;
}

ObjectList *write_object_list(ObjectList *x)
{
    CPrecWrittenEntry *e;
    ObjectList *first;
    ObjectList *current;
    ObjectList *next;

    if ((e = CPrec_FindAddrPatch(x)))
        return e->image_position;
    CPrec_NewAddrPatch(x, first = current = append_zero_bytes_to_align_offset());
    while (1) {
        CPrec_AppendData_004dd4e0(x, sizeof(ObjectList));
        add_serialized_bucket_entry((SInt32)(&current->object), (SInt32)(write_crec((CRec *)x->object)));
        if (!x->next)
            break;
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = append_zero_bytes_to_align_offset()));
        current = next;
        x = x->next;
    }
    return first;
}

struct CRec *write_crec(struct CRec *record)
{
    CPrecWrittenEntry *entry;
    CRec *first;
    CRec *current;
    CRec *next;
    union {
        CRec *ptr;
        SInt32 value;
    } saved;
    SInt32 position;

    if ((entry = CPrec_FindAddrPatch(record)))
        return entry->image_position;
    add_written_entry(record, first = current = align_prec_position());
    while (1) {
        CPrec_AppendData_004dd660(record, sizeof(CRec));
        mark_and_patch_name_reference(&current->name, record->name);
        if (record->bases)
            add_serialized_bucket_entry((SInt32)&current->bases, (SInt32)write_object_list(record->bases));
        if (record->methods)
            add_serialized_bucket_entry((SInt32)&current->methods, (SInt32)write_methrec(record->methods));
        if (record->info)
            add_serialized_bucket_entry((SInt32)&current->info, (SInt32)write_object(record->info));
        if (!record->next)
            break;
        if ((entry = CPrec_FindAddrPatch(record->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        next = align_prec_position();
        saved.value = position = (SInt32)next;
        add_serialized_bucket_entry((SInt32)&current->next, position);
        record = record->next;
        current = next;
        add_written_entry(record, (CRec *)saved.value);
        saved.value = 0;
    }
    return first;
}

MethRec *write_methrec(MethRec *method)
{
    CPrecWrittenEntry *entry;
    MethRec *first;
    MethRec *current;
    MethRec *position;

    if ((entry = CPrec_FindAddrPatch(method)))
        return entry->image_position;

    position = first = current = CPrec_AppendAlign();
    CPrec_NewAddrPatch(method, position);
    while (1) {
        CPrec_AppendData(method, sizeof(*method));
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
        if ((entry = CPrec_FindAddrPatch(method->next))) {
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)entry->image_position);
            break;
        }
        {
            MethRec *next = CPrec_AppendAlign();
            MethRec *tmp;
            MethRec *saved;
            MethRec *held;
            held = saved = tmp = next;
            add_serialized_bucket_entry((SInt32)&current->next, (SInt32)saved);
            method = method->next;
            current = next;
            CPrec_NewAddrPatch(method, held);
        }
    }
    return first;
}

HashEntry *write_hash_entry(HashEntry *x)
{
    CPrecWrittenEntry *e;
    HashEntry *first;
    HashEntry *current;
    HashEntry *next;
    HashEntry *value;
    HashEntry *tmp;

    if ((e = find_object_in_written_entry_bucket(x)))
        return e->image_position;
    insert_written_entry_by_object(x, first = current = append_glist_alignment_bytes());
    while (1) {
        append_data_and_update_length(x, 0x10);
        if (x->obj)
            add_serialized_bucket_entry((SInt32)(&current->obj), (SInt32)(write_object(x->obj)));
        CPrec_NamePatch(&current->name, x->name);
        if (x->methods)
            add_serialized_bucket_entry((SInt32)(&current->methods), (SInt32)(write_selector_methods(x->methods)));
        if (!x->next)
            break;
        if ((e = find_object_in_written_entry_bucket(x->next))) {
            add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(e->image_position));
            break;
        }
        value = tmp = next = append_glist_alignment_bytes();
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(tmp));
        current = next;
        x = x->next;
        insert_written_entry_by_object(x, value);
    }
    return first;
}

SelectorMethod *write_selector_methods(SelectorMethod *method)
{
    CPrecWrittenEntry *entry;
    SelectorMethod *first;
    SelectorMethod *current;
    SelectorMethod *next;
    SelectorMethod *imagePosition;
    SelectorMethod *serializedNext;

    if ((entry = CPrec_FindAddrPatch(method)))
        return entry->image_position;
    CPrec_NewAddrPatch(method, first = current = align_to_four());
    while (1) {
        forward_data_and_accumulate_len(method, sizeof(*method));
        if (method->method)
            add_serialized_pointer(&current->method, write_methrec(method->method));
        if (!method->next)
            break;
        if ((entry = CPrec_FindAddrPatch(method->next))) {
            add_serialized_pointer(&current->next, entry->image_position);
            break;
        }
        imagePosition = next = align_to_four();
        add_serialized_pointer(&current->next, serializedNext = next);
        method = method->next;
        current = next;
        CPrec_NewAddrPatch(method, imagePosition);
    }
    return first;
}

SInt32 write_objc_parameter_nodes(ObjCParameterNode *p)
{
    SInt32 start;
    SInt32 offset;
    SInt32 next;
    if (data_00581c28) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    offset = prec_position;
    start = prec_position;
    for (;;) {
        if (data_00581c28)
            AppendGListData(&precompiled_buffer, p, 0x20);
        prec_position += 0x20;
        if (p->selectorName) {
            HashNameNode *mark = p->selectorName;
            mark->id = 1;
            patch_object_reference(offset + 4, mark);
        }
        if (p->name) {
            HashNameNode *mark = p->name;
            mark->id = 1;
            patch_object_reference(offset + 8, mark);
        }

        if (p->type)
            add_serialized_bucket_entry(offset + 12, write_type(p->type));
        if (!p->next)
            break;
        if (data_00581c28) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        next = prec_position;
        add_serialized_bucket_entry(offset, prec_position);
        offset = next;
        p = p->next;
    }
    return start;
}

SInt32 write_som_info(SOMInfo *entry)
{
    SInt32 start;
    SInt32 pos;
    SOMInfoEntry *node;
    SInt32 listStart;

    if (data_00581c28) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position++;
        }
    }
    start = prec_position;
    if (data_00581c28) {
        AppendGListData(&precompiled_buffer, entry, 0x16);
    }
    prec_position += 0x16;
    if (entry->baseClass != NULL) {
        add_serialized_bucket_entry(start, write_type((Type *)entry->baseClass));
    }
    if (entry->classDataObject != NULL) {
        add_serialized_bucket_entry(start + 4, write_object(entry->classDataObject));
    }
    if (entry->methodNameList != NULL) {
        node = entry->methodNameList;
        if (data_00581c28) {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position++;
            }
        }
        listStart = pos = prec_position;
        for (;;) {
            if (data_00581c28) {
                AppendGListData(&precompiled_buffer, node, 0xa);
            }
            prec_position += 0xa;
            {
                HashNameNode *reference = node->name;
                reference->id = 1;
                patch_object_reference(pos + 4, reference);
            }
            if (node->next == NULL)
                break;
            if (data_00581c28) {
                while ((prec_position & 3) != 0) {
                    AppendGListByte(&precompiled_buffer, 0);
                    prec_position++;
                }
            }
            {
                SInt32 nextPos = prec_position;
                add_serialized_bucket_entry(pos, nextPos);
                pos = nextPos;
                node = node->next;
            }
        }
        add_serialized_bucket_entry(start + 8, listStart);
    }
    return start;
}

unsigned int write_vtable(VTable *record)
{
    UInt32 recordOffset;
    SInt32 relocationOffset;

    if (data_00581c28 != 0) {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    recordOffset = prec_position;
    if (data_00581c28 != 0) {
        AppendGListData(&precompiled_buffer, record, sizeof(*record));
    }
    prec_position += sizeof(*record);
    if (record->object != NULL) {
        relocationOffset = write_object(record->object);
        add_serialized_bucket_entry(recordOffset, relocationOffset);
    }
    if (record->owner != NULL) {
        relocationOffset = write_type((Type *)record->owner);
        add_serialized_bucket_entry(recordOffset + 4, relocationOffset);
    }
    return recordOffset;
}

unsigned int write_prec_type_entries(struct PrecTypeEntry *entry)
{
    SInt32 firstOffset;
    SInt32 typeOffset;
    SInt32 entryOffset;
    SInt32 nextEntryOffset;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    entryOffset = prec_position;
    firstOffset = prec_position;
    for (;;) {
        if (data_00581c28 != '\0') {
            AppendGListData(&precompiled_buffer, entry, sizeof(*entry));
        }
        prec_position += sizeof(*entry);
        typeOffset = write_type((Type *)entry->type);
        add_serialized_bucket_entry(entryOffset + sizeof(entry->next), typeOffset);
        if (entry->next == NULL)
            break;
        if (data_00581c28 != '\0') {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        nextEntryOffset = prec_position;
        add_serialized_bucket_entry(entryOffset, nextEntryOffset);
        entryOffset = nextEntryOffset;
        entry = entry->next;
    }
    return firstOffset;
}

/* Serialized entries occupy ten bytes, independent of host padding. */
enum { CPrecSerializedEntryBytes = 10 };

unsigned int serialize_entry_list(ClassFriend *record)
{
    SInt32 startOffset;
    SInt32 serializedOffset;
    SInt32 recordOffset;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    recordOffset = prec_position;
    startOffset = prec_position;
    for (;; record = record->next) {
        if (data_00581c28 != '\0') {
            AppendGListData(&precompiled_buffer, record, CPrecSerializedEntryBytes);
        }
        prec_position += CPrecSerializedEntryBytes;
        if (record->isclass != 0) {
            serializedOffset = write_type(TYPE(record->u.theclass));
            add_serialized_bucket_entry(recordOffset + 4, serializedOffset);
        } else {
            serializedOffset = write_object(record->u.obj);
            add_serialized_bucket_entry(recordOffset + 4, serializedOffset);
        }
        if (record->next == NULL)
            break;
        if (data_00581c28 != '\0') {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        serializedOffset = prec_position;
        add_serialized_bucket_entry(recordOffset, serializedOffset);
        recordOffset = serializedOffset;
    }
    return startOffset;
}

unsigned int write_vclasslist(VClassList *record)
{
    SInt32 startOffset;
    SInt32 typeOffset;
    SInt32 recordOffset;

    if (data_00581c28 != '\0') {
        while ((prec_position & 3) != 0) {
            AppendGListByte(&precompiled_buffer, 0);
            prec_position += 1;
        }
    }
    recordOffset = prec_position;
    startOffset = prec_position;
    for (;;) {
        if (data_00581c28 != '\0') {
            AppendGListData(&precompiled_buffer, record, sizeof(*record));
        }
        prec_position += sizeof(*record);
        typeOffset = write_type((Type *)record->base);
        add_serialized_bucket_entry(recordOffset + 4, typeOffset);
        if (record->next == NULL)
            break;
        if (data_00581c28 != '\0') {
            while ((prec_position & 3) != 0) {
                AppendGListByte(&precompiled_buffer, 0);
                prec_position += 1;
            }
        }
        typeOffset = prec_position;
        add_serialized_bucket_entry(recordOffset, typeOffset);
        recordOffset = typeOffset;
        record = record->next;
    }
    return startOffset;
}

ClassList *write_class_list(ClassList *x)
{
    CPrecWrittenEntry *e;
    ClassList *first;
    ClassList *current;
    ClassList *next;

    if ((e = CPrec_FindAddrPatch(x)))
        return e->image_position;
    CPrec_NewAddrPatch(x, first = current = pad_position_to_alignment());
    while (1) {
        CPrec_AppendData_004de570(x, 0x12);
        add_serialized_bucket_entry((SInt32)(&current->base), write_type((Type *)x->base));
        if (!x->next)
            break;
        if ((e = CPrec_FindAddrPatch(x->next))) {
            add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(e->image_position));
            break;
        }
        add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = pad_position_to_alignment()));
        current = next;
        x = x->next;
    }
    return first;
}

/* hash-table / emitter base */
/* tracing enabled? */
/* running byte counter */

int write_templdep(TypeTemplDep *node)
{
    CPrecKey key;
    CPrecWrittenEntry **bucket;
    CPrecWrittenEntry *entry;
    SInt32 position;
    SInt32 imagePosition;
    SInt32 reference;
    HashNameNode *name;

    if (data_00581c28) {
        for (; prec_position & 3; prec_position++) {
            AppendGListByte(&precompiled_buffer, 0);
        }
    }
    position = prec_position;
    imagePosition = prec_position;
    key.v = (UInt32)node;
    bucket = &written_entry_buckets[((UInt32)node + key.b[0] + key.b[1] + key.b[2] + key.b[3]) & 0x3fff];
    entry = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    entry->object = node;
    entry->image_position = (TypeTemplDep *)imagePosition;
    entry->next = *bucket;
    *bucket = entry;
    if (data_00581c28) {
        AppendGListData(&precompiled_buffer, node, sizeof(*node));
    }
    prec_position += sizeof(*node);
    switch (node->dtype) {
        case 0:
            break;
        case 1:
            reference = write_templdep(node->u.qual.type);
            add_serialized_bucket_entry(position + 8, reference);
            name = node->u.qual.name;
            name->id = 1;
            patch_object_reference(position + 12, name);
            break;
        case 2:
            reference = write_type((Type *)node->u.templ.templ);
            add_serialized_bucket_entry(position + 8, reference);
            reference = serialize_ct_state_elems(node->u.templ.args);
            add_serialized_bucket_entry(position + 12, reference);
            break;
        case 3:
            reference = write_type((Type *)node->u.array.type);
            add_serialized_bucket_entry(position + 8, reference);
            reference = write_enode(node->u.array.index);
            add_serialized_bucket_entry(position + 12, reference);
            break;
        case 4:
            reference = write_templdep(node->u.qualtempl.type);
            add_serialized_bucket_entry(position + 8, reference);
            reference = serialize_ct_state_elems(node->u.qualtempl.args);
            add_serialized_bucket_entry(position + 12, reference);
            break;
        case 5:
            reference = write_type((Type *)node->u.bitfield.type);
            add_serialized_bucket_entry(position + 8, reference);
            reference = write_enode(node->u.bitfield.size);
            add_serialized_bucket_entry(position + 12, reference);
            break;
        default:
            CError_FATAL(1270);
    }
    return position;
}

TypeMemberPointer *append_member_pointer_type(TypeMemberPointer *tmemp)
{
    TypeMemberPointer *p;

    insert_written_entry_by_key(tmemp, p = CPrec_AppendAlign_004de900());
    append_data_and_count_bytes(tmemp, 0x12);
    add_serialized_bucket_entry((SInt32)(&p->ty1), write_type(tmemp->ty1));
    add_serialized_bucket_entry((SInt32)(&p->ty2), write_type(tmemp->ty2));
    return p;
}

TypeFunc *copy_type_func(TypeFunc *tfunc)
{
    TypeFunc *copy;

    CPrec_NewAddrPatch(tfunc, copy = append_zero_bytes_to_align_position());
    accumulate_data_length_and_forward_data(tfunc,
                                            (tfunc->flags & FUNC_METHOD) ? sizeof(TypeMemberFunc) : sizeof(TypeFunc));
    add_serialized_bucket_entry((SInt32)&copy->functype, write_type(tfunc->functype));
    if (tfunc->args)
        add_serialized_bucket_entry((SInt32)&copy->args,
                                    (SInt32)write_func_args(tfunc->args, (tfunc->flags & 0x8000400) != 0));
    if (tfunc->exspecs)
        add_serialized_bucket_entry((SInt32)&copy->exspecs, write_except_spec_list(tfunc->exspecs));
    if (tfunc->flags & FUNC_METHOD)
        add_serialized_bucket_entry((SInt32)&TYPE_METHOD(copy)->theclass,
                                    write_type((Type *)TYPE_METHOD(tfunc)->theclass));
    return copy;
}

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

unsigned int write_except_spec_list(ExceptSpecList *record)
{
    SInt32 startOffset;
    SInt32 referenceOffset;
    SInt32 recordOffset;

    if (data_00581c28 != '\0') {
        for (; (prec_position & 3) != 0; prec_position += 1) {
            AppendGListByte(&precompiled_buffer, 0);
        }
    }
    recordOffset = prec_position;
    startOffset = prec_position;
    if (record != NULL) {
        do {
            if (data_00581c28 != '\0') {
                AppendGListData(&precompiled_buffer, record, sizeof(ExceptSpecList));
            }
            prec_position += sizeof(ExceptSpecList);
            if (record->type != NULL) {
                referenceOffset = write_type(record->type);
                add_serialized_bucket_entry(recordOffset + offsetof(ExceptSpecList, type), referenceOffset);
            }
            if (record->next == NULL) {
                break;
            }
            if (data_00581c28 != '\0') {
                for (; (prec_position & 3) != 0; prec_position += 1) {
                    AppendGListByte(&precompiled_buffer, 0);
                }
            }
            referenceOffset = prec_position;
            add_serialized_bucket_entry(recordOffset, prec_position);
            record = record->next;
            recordOffset = referenceOffset;
        } while (record != NULL);
    }
    return startOffset;
}

TypeStruct *append_type_struct(TypeStruct *tstruct)
{
    StructMember *member;
    TypeStruct *p;
    StructMember *current;
    StructMember *next;

    CPrec_NewAddrPatch(tstruct, p = append_align());
    CPrec_AppendData_004dee40(tstruct, 0x12);
    if (tstruct->name)
        CPrec_NamePatch(&p->name, tstruct->name);
    if ((member = tstruct->members)) {
        add_serialized_bucket_entry((SInt32)(&p->members), (SInt32)(current = append_align()));
        while (1) {
            CPrec_AppendData_004dee40(member, sizeof(StructMember));
            add_serialized_bucket_entry((SInt32)(&current->type), write_type(member->type));
            CPrec_NamePatch(&current->name, member->name);
            if (!member->next)
                break;
            add_serialized_bucket_entry((SInt32)(&current->next), (SInt32)(next = append_align()));
            current = next;
            member = member->next;
        }
    }
    return p;
}

/* CPrec_GetTypePatch */
/* CPrec_NewPointerPatch */

TypeBitfield *serialize_type_bitfield(TypeBitfield *bitfield)
{
    TypeBitfield *serialized;
    prepend_written_entry_to_hash_bucket(bitfield, serialized = pad_precompiled_buffer());
    append_precompiled_data(bitfield, sizeof(TypeBitfield));
    {
        int typeIndex = write_type(bitfield->bitfieldtype);
        add_serialized_bucket_entry((SInt32)&serialized->bitfieldtype, typeIndex);
    }
    return serialized;
}

TypeEnum *append_type_enum(TypeEnum *tenum)
{
    TypeEnum *offset;

    CPrec_NewAddrPatch(tenum, offset = align_and_return_offset());
    CPrec_AppendData_004df0c0(tenum, sizeof(*tenum));
    if (tenum->nspace)
        patch_serialized_field(&offset->nspace, get_namespace_patch(tenum->nspace));
    if (tenum->enumlist)
        patch_serialized_field(&offset->enumlist, write_enum_const(tenum->enumlist));
    patch_serialized_field(&offset->enumtype, CPrec_GetTypePatch(tenum->enumtype));
    if (tenum->enumname)
        CPrec_NamePatch(&offset->enumname, tenum->enumname);
    return offset;
}

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

SInt32 write_bclass_list(BClassList *classes)
{
    CPrecWrittenEntry *entry;
    SInt32 firstPosition;
    SInt32 currentPosition;
    SInt32 nextPosition;

    if ((entry = CPrec_FindAddrPatch(classes)))
        return (SInt32)entry->image_position;

    CPrec_NewAddrPatch(classes, (BClassList *)(firstPosition = currentPosition = append_prec_alignment_padding()));
    while (1) {
        CPrec_AppendData_004df620(classes, sizeof(BClassList));
        add_serialized_bucket_entry(currentPosition + 4, write_type(classes->type));
        if (!classes->next)
            break;
        nextPosition = append_prec_alignment_padding();
        add_serialized_bucket_entry(currentPosition, nextPosition);
        currentPosition = nextPosition;
        classes = classes->next;
    }
    return firstPosition;
}

void serialize_macros(void)
{
    UInt32 n;
    Macro *p;
    SInt32 j;
    SInt32 i;
    SInt32 pos;
    SInt32 start;
    char *s;

    i = 0;
    do {
        for (p = macro_buckets[i]; p != NULL; p = p->next) {
            if (p->text != NULL) {
                CPrec_AddRecord(p->text, prec_position);
                s = p->text;
                n = strlen(s) + 1;
                if (data_00581c28)
                    AppendGListData(&precompiled_buffer, s, n);
                prec_position += n;
            }
        }
    } while (++i < 0x800);

    i = 0;
    do {
        if ((p = macro_buckets[i]) == NULL)
            continue;
        pos = CPrec_AppendAlign_004df7a0();
        start = pos;
        if (data_00581c28)
            prec_header->macroOffsets[i] = pos;
        for (;;) {
            CPrec_AppendLong(0);

            CPrec_AppendObj(p->name);
            CPrec_AppendOffset(p->text);

            CPrec_AppendShort(p->nargs);
            CPrec_AppendByte(p->flag);
            CPrec_AppendByte(p->isExpanding);

            for (j = 1; j < (p->nargs & 0x7fff); j++)
                CPrec_AppendObj(p->args[j - 1]);

            p = p->next;
            if (p == NULL)
                break;
            CPrec_AppendAlign_004df7a0();
            pos = prec_position;
            add_serialized_bucket_entry(start, pos);
            start = pos;
        }
    } while (++i < 0x800);
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
                prec_header->hashNameOffsets[i] = (SInt32)(p = append_prec_alignment());
                while (1) {
                    CPrec_NewAddrPatch(name, p);
                    CPrec_AppendWord32(0);
                    CPrec_AppendWord32(0);
                    CPrec_AppendWord16(name->hashval);
                    CPrec_AppendString(name->name);
                    name = name->next;
                    while (name && name->id == 0)
                        name = name->next;
                    if (!name)
                        break;
                    add_serialized_bucket_entry((SInt32)(&p->next), (SInt32)(next = append_prec_alignment()));
                    p = next;
                }
            }
        } while (++i < 0x800);
    } else {
        i = 0;
        do {
            if ((name = data_00587f88[i])) {
                p = append_prec_alignment();
                while (1) {
                    CPrec_NewAddrPatch(name, p);
                    CPrec_AppendWord32(0);
                    CPrec_AppendWord32(0);
                    CPrec_AppendWord16(name->hashval);
                    CPrec_AppendString(name->name);
                    name = name->next;
                    if (!name)
                        break;
                    add_serialized_bucket_entry((SInt32)(&p->next), (SInt32)(next = append_prec_alignment()));
                    p = next;
                }
            }
        } while (++i < 0x800);
    }
}

void fn_004e0010(void *data, UInt32 size)
{
    if (data_00581c28 != 0) {
        AppendGListData(&precompiled_buffer, data, size);
    }
    prec_position += size;
}

unsigned int align_to_four_byte_boundary(void)

{
    if (data_00581c28 != '\0') {
        for (; (prec_position & 3) != 0; prec_position += 1) {
            AppendGListByte(&precompiled_buffer, 0);
        }
    }
    return prec_position;
}

void patch_hash_name_reference(unsigned int value, HashNameNode *record)
{
    record->id = 1;
    patch_object_reference((SInt32)(value), record);
}

static SInt32 hash_cprec_bytes(SInt32 value)
{
    CPrecBytes b;
    b.value = value;
    return (b.bytes[0] + value + b.bytes[1] + b.bytes[2] + b.bytes[3]) & 0x3fff;
}

static CPrecWrittenEntry *CPrec_Find(SInt32 id)
{
    CPrecWrittenEntry *n;
    for (n = ((CPrecWrittenEntry **)written_entry_buckets)[hash_cprec_bytes(id)]; n != NULL; n = n->next)
        if (n->object == (void *)id)
            return n;
    return NULL;
}

int patch_object_reference(SInt32 location, HashNameNode *object)
{
    if (data_00581c28) {
        CPrecWrittenEntry *entry;
        SerializedBucketEntry *reference;
        SInt32 index;

        CError_ASSERT(520, entry = CPrec_Find((SInt32)object));
        reference = (SerializedBucketEntry *)lalloc(sizeof(SerializedBucketEntry));
        reference->offset = location;
        reference->next = serialized_bucket_entries;
        serialized_bucket_entries = reference;
        CError_ASSERT(525, (reference->offset & 0x80000001) == 0);
        index = location - flushed_size;
        CError_ASSERT(529, index >= 0 && index <= precompiled_buffer.size);
        *(void **)(*precompiled_buffer.data + index) = entry->image_position;
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

/* Entry in the hashed value table. */
void add_written_type_entry(Type *key, int imagePosition)
{
    CPrecKey keyBytes;
    CPrecWrittenEntry **slot;
    CPrecWrittenEntry *entry;
    unsigned int hash;

    keyBytes.v = (UInt32)key;
    hash = keyBytes.b[0] + (UInt32)key + keyBytes.b[1] + keyBytes.b[2] + keyBytes.b[3];
    hash &= 16383U;
    slot = &written_entry_buckets[hash];
    entry = (CPrecWrittenEntry *)lalloc(sizeof(CPrecWrittenEntry));
    entry->object = key;
    entry->image_position = (UInt8 *)imagePosition;
    entry->next = *slot;
    *slot = entry;
}

CPrecWrittenEntry *fn_004e0680(void *key)
{
    CPrecKey hashKey;
    unsigned int index;
    CPrecWrittenEntry *entry;

    hashKey.v = (UInt32)key;
    index = hashKey.b[0];
    index += (unsigned int)key;
    index += hashKey.b[1];
    index += hashKey.b[2];
    index += hashKey.b[3];
    index &= 0x3fff;
    entry = written_entry_buckets[index];
    while (entry != NULL) {
        if (entry->object == key)
            return entry;
        entry = entry->next;
    }
    return NULL;
}

unsigned int CException_HashType(Type *type)
{
    union {
        unsigned int value;
        unsigned char bytes[4];
    } address;

    unsigned int hash = (unsigned int)type;
    address.value = hash;
    return (hash + address.bytes[0] + address.bytes[1] + address.bytes[2] + address.bytes[3]) & 0x3fffU;
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

void CException_ResetPrecompiledState(UInt8 c)
{
    precompiled_file = 0;
    precompiled_buffer.data = NULL;
    prec_header = NULL;
    pending_buffers = NULL;
    data_00581c26 = 0;
    return;
}
