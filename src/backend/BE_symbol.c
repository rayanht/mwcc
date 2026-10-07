#define CERROR_FILE "BE_symbol.c"
#include "compiler/common.h"
#include "compiler/BE_symbol.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CObjC.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
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
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include "compiler/TOC.h"
#include "driver/Files.h"
#include <string.h>

static struct BE_SymNode *data_0055da80 = NULL;
/* Declarations gathered from the merged files. */

/* Back-end symbol records and their associated data. */

Boolean BE_symbol_004913b0(Object *obj)
{
    SInt16 kind;
    HashNameNode *function;
    struct BE_SymNode *symbol;
    ObjGenSection *data;
    Boolean flag;

    function = COptimizer_GetFunctionObject(obj);
    if (CParser_HasInternalLinkage(obj))
        kind = 0x102;
    else
        kind = 0x103;

    symbol = be_symbol_list;
    if (symbol != NULL) {
        for (;;) {
            if (function == symbol->nameData.hashName && ((SInt32)kind == 0x102) == (symbol->kind == 0x102))
                break;
            symbol = symbol->next;
            if (symbol == NULL) {
                symbol = NULL;
                break;
            }
        }
    } else {
        symbol = NULL;
    }
    if (symbol == NULL) {
        if (kind == 0x102)
            flag = 0;
        else
            flag = 1;
        symbol = galloc(sizeof(*symbol));
        memset(symbol, 0, sizeof(*symbol));
        symbol->nameData.hashName = function;
        if (be_symbol_list != NULL)
            symbol_tail->next = symbol;
        else
            be_symbol_list = symbol;
        symbol_tail = symbol;
        symbol->symbolKind = flag << 4;
        symbol->kind = kind;
        symbol->sectionData.section = data_005884aa;
    }

    data = symbol->sectionData.section;
    CError_ASSERT(499, data != NULL);

    if (copts.reuseSectionSymbols != 0 && data->symbolLink != NULL && data->symbolLink->symbol != NULL &&
        (obj->qual & Q_IMPLICIT_WEAK) == 0 && (obj->qual & Q_WEAK) == 0 && !PCodeUtilities_Require(obj) &&
        TOC_HasObjectReferenceWithoutExpression(data->symbolLink->object)) {
        Boolean oldFlag = data->symbolLink->value;
        data->symbolLink->value = 1;
        return (oldFlag == 0);
    }
    return 1;
}

static inline void BE_symbol_FindFunction(HashNameNode *obj, SInt16 kind, BE_SymNode **result)
{
    for (*result = be_symbol_list; *result != NULL; *result = (*result)->next) {
        if (obj == (*result)->nameData.hashName && (((SInt32)kind == 0x102) == ((*result)->kind == 0x102)))
            return;
    }
    *result = NULL;
}

Object *BE_symbol_GetFunctionSymbolLinkData(Object *func)
{
    BE_SymNode *node;
    SInt16 kind;
    HashNameNode *obj;
    Boolean b;
    ObjGenSymbolLink *value;

    obj = COptimizer_GetFunctionObject(func);
    if (CParser_HasInternalLinkage(func))
        kind = 0x102;
    else
        kind = 0x103;

    BE_symbol_FindFunction(obj, kind, &node);

    if (node == NULL) {
        if (kind == 0x102)
            b = 0;
        else
            b = 1;
        node = galloc((SInt32)sizeof(BE_SymNode));
        memset(node, 0, sizeof(BE_SymNode));
        node->nameData.hashName = obj;
        if (be_symbol_list != NULL)
            symbol_tail->next = node;
        else
            be_symbol_list = node;
        symbol_tail = node;
        node->symbolKind = (UInt8)(b << 4);
        node->kind = kind;
        node->sectionData.section = data_005884aa;
    }

    if ((func->qual & Q_TENTATIVE) && node->size == 0)
        ObjGen_PPC_EABI_EmitObjectWithDebugEntry(func, NULL, NULL, func->type->size);

    value = node->sectionData.section->symbolLink;
    if (!(func->qual & Q_IMPLICIT_WEAK) && !(func->qual & Q_WEAK) && value != NULL) {
        if (value->object != NULL)
            return value->object;
    }
    return NULL;
}

unsigned int BE_symbol_GetOffset(BE_SymNode *symbol)
{
    return symbol->offset;
}

/* Input carrying the name used to construct a symbol. */

/* Symbol record linked in creation order. */

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
    symbol = galloc(50);
    memset(symbol, 0, 50);
    symbol->nameData.hashName = name;
    if (be_symbol_list != NULL)
        symbol_tail->next = symbol;
    else
        be_symbol_list = symbol;
    symbol_tail = symbol;
    symbol->symbolKind = 0;
    symbol->attributes = 0;
    symbol->sectionData.section = input;
    symbol->alignment = 1;
    symbol->kind = 258;
    symbol->flags |= 64;
    return (unsigned int)symbol;
}

/* 0x32-byte symbol record: name at 0x00, next at 0x18, flags word at 0x2c. */

/* Owner record: source name string at 0x18, cached symbol at 0x64. */

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

static BE_SymNode *FindFuncNode(HashNameNode *func, int kind)
{
    BE_SymNode *p;
    for (p = be_symbol_list; p != NULL; p = p->next) {
        if (func == (HashNameNode *)p->nameData.hashName && (kind == 0x102) == (p->kind == 0x102))
            return (BE_SymNode *)p;
    }
    return NULL;
}

BE_SymNode *BE_symbol_GetOrCreateFunctionObjectSymbol(Object *arg)
{
    HashNameNode *func;
    SInt16 kind;
    UInt8 flag;
    BE_SymNode *found;
    BE_SymNode *node;

    func = COptimizer_GetFunctionObject(arg);
    if (CParser_HasInternalLinkage(arg))
        kind = 0x102;
    else
        kind = 0x103;

    found = FindFuncNode(func, kind);

    if (found)
        return found;

    if (kind == 0x102)
        flag = 0;
    else
        flag = 1;
    node = galloc(sizeof(BE_SymNode));
    memset(node, 0, sizeof(BE_SymNode));
    node->nameData.hashName = func;
    if (be_symbol_list != NULL)
        symbol_tail->next = node;
    else
        be_symbol_list = node;
    symbol_tail = node;
    node->symbolKind = flag << 4;
    node->kind = kind;
    node->sectionData.section = data_005884aa;
    return node;
}

struct BE_SymNode *BE_symbol_GetSymbolOrderTail(void)
{
    return symbol_order_tail;
}

static BE_SymNode *FindSym(int obj, int kind)
{
    BE_SymNode *p;
    for (p = be_symbol_list; p != NULL; p = p->next) {
        if (obj == (int)p->nameData.hashName && (kind == 258) == (p->kind == 258)) {
            return p;
        }
    }
    return NULL;
}

/* Backend symbol output record. */

static inline void FindBackendSymbol(BE_SymNode **result, int key, int symbolCategory)
{
    BE_SymNode *symbol;
    int category;
    if ((symbol = be_symbol_list) != NULL) {
        category = (short)symbolCategory;
        do {
            if (key == (int)symbol->nameData.hashName && (category == 258) == (symbol->kind == 258)) {
                *result = (BE_SymNode *)symbol;
                return;
            }
            symbol = symbol->next;
        } while (symbol != NULL);
    }
    *result = NULL;
}

enum { FuncType = 0x102 };
static inline short output_type(BE_SymNode *p)
{
    return p->kind;
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

static inline BE_SymNode *new_output(void *name, short type)
{
    BE_SymNode *p;
    unsigned char flag;
    if (type == 0x102)
        flag = 0;
    else
        flag = 1;
    p = galloc(50);
    memset(p, 0, 50);
    p->nameData.hashName = name;
    if (be_symbol_list)
        symbol_tail->next = p;
    else
        be_symbol_list = p;
    symbol_tail = p;
    p->symbolKind = flag << 4;
    p->kind = type;
    p->sectionData.section = data_005884aa;
    return p;
}

static inline BE_SymNode *get_output(short type, void *name)
{
    BE_SymNode *p;
    if (!(p = find_output(name, type)))
        p = new_output(name, type);
    return p;
}

void BE_symbol_Init(void)
{
    memset(&be_symbol_list, 0, 12U);
    data_0055da80 = NULL;
}

/* Prefix of a 50-byte symbol record; the remaining fields are not used here. */

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

/* Backend symbol table entry, linked in insertion order. */

enum { BACKEND_SYMBOL_KIND_258 = 258, BACKEND_SYMBOL_KIND_259 = 259 };

BE_SymNode *BE_symbol_004918f0(Object *symbol, struct ObjGenSection *value)
{
    char flags;
    HashNameNode *symbolKey;
    short kind;
    BE_SymNode *record;
    char kindFlag;
    if (CParser_HasInternalLinkage(symbol) != 0) {
        flags = 0;
    } else {
        flags = 1;
    }
    symbolKey = COptimizer_GetFunctionObject(symbol);
    kind = CParser_HasInternalLinkage(symbol) != 0 ? BACKEND_SYMBOL_KIND_258 : BACKEND_SYMBOL_KIND_259;
    if ((record = FindSym((int)symbolKey, kind)) == NULL) {
        if (kind == BACKEND_SYMBOL_KIND_258) {
            kindFlag = 0;
        } else {
            kindFlag = 1;
        }
        record = galloc(50);
        memset(record, 0, 50);
        record->nameData.hashName = symbolKey;
        if (be_symbol_list != NULL) {
            symbol_tail->next = record;
        } else {
            be_symbol_list = record;
        }
        symbol_tail = record;
        record->symbolKind = (char)(kindFlag << 4);
        record->kind = kind;
        record->sectionData.section = data_005884aa;
    }
    flags <<= 4;
    record->symbolKind = flags;
    record->size = 0;
    record->attributes = 0;
    record->sectionData.section = value;
    record->alignment = 4;
    record->linkageKind = 0;
    record->linkageFlags = 0;
    if (copts.fc0 != 0) {
        record->linkageFlags |= 8;
    }
    return record;
}

BE_SymNode *BE_symbol_SetupObjectSymbol(Object *object, int size, ObjGenSection *section)
{
    unsigned char linkageKind;
    unsigned char binding;
    BE_SymNode *symbol;
    long alignment;
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
    symbol = get_output(CParser_HasInternalLinkage(object) ? (short)0x102 : (short)0x103,
                        COptimizer_GetFunctionObject(object));
    symbol->size = size;
    symbol->attributes = 0;
    symbol->sectionData.section = section;
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
        symbol->alignment = copts.codeAlignment;
    symbol->linkageKind = linkageKind;
    symbol->linkageFlags = 0;
    if (copts.fc0)
        symbol->linkageFlags |= 8;
    binding <<= 4;
    binding += 2;
    symbol->symbolKind = binding;
    return symbol;
}

BE_SymNode *BE_symbol_DefineObjectSymbol(Object *object, int offset, ObjGenSection *value)
{
    char categoryBit;
    BE_SymNode *symbol;
    int symbolCategory;
    int alignment;
    char kind;
    HashNameNode *key;
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
    key = COptimizer_GetFunctionObject(object);
    if (CParser_HasInternalLinkage(object) != 0) {
        symbolCategory = 258;
    } else {
        symbolCategory = 259;
    }
    FindBackendSymbol(&symbol, (int)key, symbolCategory);
    if (symbol == NULL) {
        if ((short)symbolCategory == 258) {
            categoryBit = 0;
        } else {
            categoryBit = 1;
        }
        symbol = galloc(sizeof(*symbol));
        memset(symbol, 0, sizeof(*symbol));
        symbol->nameData.hashName = key;
        if (be_symbol_list != NULL) {
            symbol_tail->next = symbol;
        } else {
            be_symbol_list = symbol;
        }
        symbol_tail = symbol;
        symbol->symbolKind = categoryBit << 4;
        symbol->kind = symbolCategory;
        symbol->sectionData.section = data_005884aa;
    }
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
    if (copts.fc0 != 0) {
        symbol->linkageFlags |= 8;
    }
    return symbol;
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

BE_SymNode *BE_symbol_ResetSymbolTail(void)
{
    return symbol_tail = be_symbol_list;
}
