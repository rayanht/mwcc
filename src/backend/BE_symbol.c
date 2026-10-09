#define CERROR_FILE "BE_symbol.c"
#include "compiler/common.h"
#include "version.h"
#include "compiler/BE_symbol.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/CError.h"
#include "compiler/CExpr2.h"
#include "compiler/CMangler.h"
#include "compiler/CParser.h"
#include "compiler/CodeGen.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/TOC.h"
#include <string.h>

struct BE_SymNode *be_symbol_list;
struct BE_SymNode *symbol_tail;
struct ObjGenSection *data_005884aa;

enum { FuncType = 0x102 };
static inline short output_type(BE_SymNode *p)
{
    return p->kind;
}

BE_SymNode *BE_symbol_ResetSymbolTail(void)
{
    return symbol_tail = be_symbol_list;
}

BE_SymNode *BE_symbol_AdvanceSymbolTail(void)
{
    BE_SymNode *entry;
    if (symbol_tail == NULL)
        CError_FATAL(57);
    entry = symbol_tail;
    symbol_tail = entry->next;
    return symbol_tail;
}

BE_SymNode *BE_symbol_DefineObjectSymbol(Object *object, int offset, ObjGenSection *value)
{
    BE_SymNode *symbol;
    int alignment;
    char kind;
    char linkageKind;

    linkageKind = 0;
    if (object->type->type == TYPEFUNC) {
        return BE_symbol_SetupObjectSymbol(object, offset, value);
    }
    if ((object->qual & Q_IMPLICIT_WEAK) != 0) {
        kind = 2;
        linkageKind = 13;
    } else if ((object->qual & Q_WEAK) != 0) {
        kind = 2;
        linkageKind = 14;
    } else if (CParser_HasInternalLinkage(object) != 0) {
        kind = 0;
    } else {
        kind = 1;
    }
    symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
    kind <<= 4;
    kind++;
    symbol->symbolKind = kind;
    symbol->size = offset;
    symbol->attributes = 0;
    symbol->sectionData.section = value;
    if ((object->qual & Q_ALIGNED_MASK) != 0) {
        alignment = object->qual & Q_ALIGNED_MASK;
        switch (alignment) {
            case 0x2000000:
                alignment = 1;
                break;
            case 0x4000000:
                alignment = 2;
                break;
            case 0x6000000:
                alignment = 4;
                break;
            case 0x8000000:
                alignment = 8;
                break;
            case 0xa000000:
                alignment = 0x10;
                break;
            case 0xc000000:
                alignment = 0x20;
                break;
            case 0x10000000:
                alignment = 0x40;
                break;
            case 0x12000000:
                alignment = 0x80;
                break;
            case 0x14000000:
                alignment = 0x100;
                break;
            case 0x16000000:
                alignment = 0x200;
                break;
            case 0x18000000:
                alignment = 0x400;
                break;
            case 0x1a000000:
                alignment = 0x800;
                break;
            case 0x1c000000:
                alignment = 0x1000;
                break;
            case 0x1e000000:
                alignment = 0x2000;
                break;
            default:
                CError_FATAL(175);
        }
        symbol->alignment = alignment;
    } else {
        symbol->alignment = StackFrameEABI_GetTypeAlignment(object->type);
    }
    symbol->linkageKind = linkageKind;
    symbol->linkageFlags = 0;
    if (copts.force_active != 0) {
        symbol->linkageFlags |= 8;
    }
    return symbol;
}

BE_SymNode *BE_symbol_SetupObjectSymbol(Object *object, int size, ObjGenSection *section)
{
    unsigned char linkageKind;
    unsigned char binding;
    BE_SymNode *symbol;
#if VERSION > VERSION_GC_1_0
    long alignment;
#endif
    linkageKind = 0;
    if (CParser_HasInternalLinkage(object))
        binding = 0;
    else
        binding = 1;
    if (object->qual & Q_IMPLICIT_WEAK) {
        binding = 2;
        linkageKind = 13;
    } else if (object->qual & Q_WEAK) {
        binding = 2;
        linkageKind = 14;
    }
    symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(object);
    symbol->size = size;
    symbol->attributes = 0;
    symbol->sectionData.section = section;
#if VERSION > VERSION_GC_1_0
    if (object->qual & Q_ALIGNED_MASK) {
        alignment = object->qual & Q_ALIGNED_MASK;
        switch (alignment) {
            case 0x02000000:
                alignment = 1;
                break;
            case 0x04000000:
                alignment = 2;
                break;
            case 0x06000000:
                alignment = 4;
                break;
            case 0x08000000:
                alignment = 8;
                break;
            case 0x0a000000:
                alignment = 16;
                break;
            case 0x0c000000:
                alignment = 32;
                break;
            case 0x10000000:
                alignment = 64;
                break;
            case 0x12000000:
                alignment = 128;
                break;
            case 0x14000000:
                alignment = 256;
                break;
            case 0x16000000:
                alignment = 512;
                break;
            case 0x18000000:
                alignment = 1024;
                break;
            case 0x1a000000:
                alignment = 2048;
                break;
            case 0x1c000000:
                alignment = 4096;
                break;
            case 0x1e000000:
                alignment = 8192;
                break;
            default:
                CError_FATAL(273);
        }
        symbol->alignment = alignment;
    } else
#endif
        symbol->alignment = copts.codeAlignment;
    symbol->linkageKind = linkageKind;
    symbol->linkageFlags = 0;
    if (copts.force_active)
        symbol->linkageFlags |= 8;
    binding <<= 4;
    binding += 2;
    symbol->symbolKind = binding;
    return symbol;
}

BE_SymNode *fn_004918f0(Object *symbol, struct ObjGenSection *value)
{
    char flags;
    BE_SymNode *record;
    if (CParser_HasInternalLinkage(symbol))
        flags = 0;
    else
        flags = 1;
    record = BE_symbol_GetOrCreateFunctionObjectSymbol(symbol);
    flags <<= 4;
    record->symbolKind = flags;
    record->size = 0;
    record->attributes = 0;
    record->sectionData.section = value;
    record->alignment = 4;
    record->linkageKind = 0;
    record->linkageFlags = 0;
    if (copts.force_active != 0) {
        record->linkageFlags |= 8;
    }
    return record;
}

BE_SymNode *BE_symbol_CreateSymNode(void *value)
{
    BE_SymNode *record;

    record = galloc(50U);
    memset(record, 0, 50);
    record->nameData.hashName = value;
    if (be_symbol_list != NULL)
        symbol_tail->next = record;
    else
        be_symbol_list = record;
    symbol_tail = record;
    return record;
}

static BE_SymNode *find_output(void *name, short type)
{
    BE_SymNode *p;
    if ((p = be_symbol_list) != NULL)
        do {
            if (name == p->nameData.hashName && (output_type(p) == FuncType) == (type == FuncType))
                return p;
        } while ((p = p->next) != NULL);
    return NULL;
}

static struct BE_SymNode *data_0055da80 = NULL;

void BE_symbol_Init(void)
{
#if VERSION == VERSION_GC_1_0
    fn_004b4ce0(&be_symbol_list);
#else
    memset(&be_symbol_list, 0, 12U);
#endif
    data_0055da80 = NULL;
}

struct BE_SymNode *BE_symbol_GetSymbolOrderTail(void)
{
    return symbol_order_tail;
}

BE_SymNode *BE_symbol_GetOrCreateFunctionObjectSymbol(Object *object)
{
    HashNameNode *name;
    SInt16 kind;
    UInt8 flag;
    BE_SymNode *found;
    BE_SymNode *node;

    name = COptimizer_GetFunctionObject(object);
    kind = CParser_HasInternalLinkage(object) ? 0x102 : 0x103;

    found = find_output(name, kind);

    if (found)
        return found;

    if (kind == 0x102)
        flag = 0;
    else
        flag = 1;
    node = BE_symbol_CreateSymNode(name);
    node->symbolKind = flag << 4;
    node->kind = kind;
    node->sectionData.section = data_005884aa;
    return node;
}

BE_SymNode *BE_symbol_GetSectionSym(struct ObjGenSection *ctx)
{
    BE_SymNode *node;
    BE_SymNode *head;
    BE_SymNode *tail;

    if (ctx->sym != NULL)
        return ctx->sym;
    node = (BE_SymNode *)galloc(sizeof(BE_SymNode));
    memset(node, 0, sizeof(BE_SymNode));
    node->nameData.hashName = GetHashNameNodeExport(ctx->name);
    node->kind = 0x102;
    if (data_0055da80 == NULL)
        data_0055da80 = be_symbol_list;
    tail = (head = data_0055da80)->next;
    head->next = node;
    head = node;
    node->next = tail;
    data_0055da80 = head;
    ctx->sym = node;
    return node;
}

unsigned int BE_symbol_CreateSectionSymbol(ObjGenSection *input)
{
    char *buffer;
    HashNameNode *name;
    BE_SymNode *symbol;

    buffer = (char *)galloc(strlen(input->name) + 5);
    strcpy(buffer, "..");
    strcat(buffer, input->name);
    strcat(buffer, ".0");
    name = GetHashNameNode(buffer);
    symbol = BE_symbol_CreateSymNode(name);
    symbol->symbolKind = 0;
    symbol->attributes = 0;
    symbol->sectionData.section = input;
    symbol->alignment = 1;
    symbol->kind = 258;
    symbol->flags |= 64;
    return (unsigned int)symbol;
}

unsigned int BE_symbol_GetOffset(BE_SymNode *symbol)
{
    return symbol->offset;
}

Object *BE_symbol_GetFunctionSymbolLinkData(Object *func)
{
    BE_SymNode *node;
    ObjGenSymbolLink *value;

    node = BE_symbol_GetOrCreateFunctionObjectSymbol(func);

    if ((func->qual & Q_TENTATIVE) && node->size == 0)
        ObjGen_PPC_EABI_EmitObjectWithDebugEntry(func, NULL, NULL, func->type->size);

    value = node->sectionData.section->symbolLink;
    if (!(func->qual & Q_IMPLICIT_WEAK) && !(func->qual & Q_WEAK) && value != NULL) {
        if (value->object != NULL)
            return value->object;
    }
    return NULL;
}

Boolean fn_004913b0(Object *obj)
{
    struct BE_SymNode *symbol;
    ObjGenSection *data;

    symbol = BE_symbol_GetOrCreateFunctionObjectSymbol(obj);
    data = symbol->sectionData.section;
    CError_ASSERT(VERSION > VERSION_GC_1_0 ? 499 : 441, data != NULL);

    if (copts.usedatapool != 0 && data->symbolLink != NULL && data->symbolLink->symbol != NULL &&
        (obj->qual & Q_IMPLICIT_WEAK) == 0 && (obj->qual & Q_WEAK) == 0 && !PCodeUtilities_Require(obj) &&
        TOC_HasObjectReferenceWithoutExpression(data->symbolLink->object)) {
        Boolean oldFlag = data->symbolLink->value;
        data->symbolLink->value = 1;
        return (oldFlag == 0);
    }
    return 1;
}
