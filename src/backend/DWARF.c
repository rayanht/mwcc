#define CERROR_FILE "DWARF.c"
#include "compiler/common.h"
#include "compiler/DWARF.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CABI.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMachine.h"
#include "compiler/CMangler.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
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
#include "compiler/Registers.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/Switch.h"
#include "driver/Files.h"

#include <string.h>

#include "compiler/Objects.h"

/* The compiler walks a type graph, marking each type record it visits.
 * The record at argument 2 holds the type at offset 0, a "visited" marker
 * at offset 0x18 and a lazily created debug-information entry at 0x1a. */

#pragma options align = mac68k
static SInt32 data_00580fb0[50];
static struct ObjGenRelocation *data_00581078[50];
static UInt8 data_00581140;
static struct DwarfNode *dwarf_node_head;
static struct DwarfNode *currentDwarfNode;
static struct DwarfNode *currentDwarfScope;
static UInt8 data_0058114e;
#pragma options align = reset

static SInt32 data_00560498 = -1;
static SInt32 *dwarf_depth_ptr = &data_00560498;
static SInt32 *dwarf_entry_offsets = data_00580fb0;
static struct ObjGenRelocation **data_005604a4 = data_00581078;
static UInt8 *data_005604a8 = &data_00581140;
static SInt32 data_005604ac = -1;

typedef struct DObj DObj;

typedef struct DwarfSym DwarfSym;

void DWARF_AddPendingObject(Object *object)
{
    PendingObject *previous;
    PendingObject *entry;

    if (!copts.filesyminfo)
        return;
    if (object->name->name[0] == '@')
        return;
    if (object->datatype != DDATA)
        return;

    entry = pending_objects;
    previous = entry;
    if (entry) {
        do {
            if (entry->object == object) {
                if (previous != entry) {
                    previous->next = entry->next;
                    entry->next = pending_objects;
                    pending_objects = entry;
                }
                return;
            }
            previous = entry;
            entry = entry->next;
        } while (entry);
    }
    previous = galloc(sizeof(PendingObject));
    previous->object = object;
    previous->next = pending_objects;
    pending_objects = previous;
    if (!previous->object->debugInfo.entry)
        DWARF_CreateObjectDebugEntry(previous->object);
}

void setup_return_operand(Object *func)
{
    Type *rtype = TYPE_FUNC(func->type)->functype;

    if (rtype->type == TYPEVOID) {
        return_operand.kind = 0;
    } else if (rtype->type == TYPEINT || rtype->type == TYPEENUM || rtype->type == TYPEPOINTER ||
               (rtype->type == TYPEMEMBERPOINTER && rtype->size == 4)) {
        if ((rtype->type == TYPEINT || rtype->type == TYPEENUM) && rtype->size == 8) {
            return_operand.kind = 8;
            return_operand.metadata.type = rtype;
            return_operand.operand.registers.first = return_gpr_first;
            return_operand.operand.registers.second = returnRegHi;
        } else {
            return_operand.kind = 1;
            return_operand.operand.registers.offset = 0;
            return_operand.operand.registers.first = 3;
            return_operand.metadata.type = rtype;
        }
    } else if (rtype->type == TYPEFLOAT) {
        Boolean gpr;
        if ((gpr = copts.operandsDebug) && rtype->type == TYPEFLOAT && rtype->size == 4) {
            return_operand.kind = 1;
            return_operand.operand.registers.offset = 0;
            return_operand.operand.registers.first = 3;
            return_operand.metadata.type = rtype;
        } else if (gpr && rtype->type == TYPEFLOAT && rtype->size != 4) {
            return_operand.kind = 8;
            return_operand.metadata.type = rtype;
            return_operand.operand.registers.first = return_gpr_first;
            return_operand.operand.registers.second = returnRegHi;
        } else {
            return_operand.kind = 2;
            return_operand.operand.registers.first = 1;
            return_operand.metadata.type = rtype;
        }
    } else if ((rtype->type == TYPESTRUCT || rtype->type == TYPECLASS) && !Type_RequiresMemoryReturn(rtype)) {
        return_operand.kind = 8;
        return_operand.metadata.type = rtype;
        return_operand.operand.registers.first = return_gpr_first;
        return_operand.operand.registers.second = returnRegHi;
    } else {
        return_operand.kind = 10;
        return_operand.metadata.type = rtype;
        return_operand.operand.registers.first = 3;
    }
}

void DWARF_SetSectionAndState(ObjGenSection *section, ObjGenSection *state)
{
    dwarf_section = (ObjGenSection *)section;
    if (copts.f26 == '\0') {
        dwarf_info_buffer = ObjGen_PPC_EABI_GetSectionBuffer(section);
    }
    DAT_00587698 = (ObjGenSection *)state;
}

void DWARF_Init(void)
{
    DwarfNode *node;
    copts.f26 = 1;
    data_0058114e = 0;
    init_dwarf_state();
    node = (DwarfNode *)galloc(38);
    dwarf_node_head = node;
    memclrw(node, 38);
    currentDwarfNode = dwarf_node_head;
    node = dwarf_node_head;
    node->kind = 0xffff;
}

void init_dwarf_state(void)
{
    DwarfFunctionState *scope;

    data_00587ea8 = galloc(sizeof(DwarfStateList));
    dwarf_state_list_tail = data_00587ea8;
    memset(data_00587ea8, 0, sizeof(DwarfStateList));

    scope = galloc(sizeof(*scope));
    data_00587168 = scope;
    currentDwarfFunctionState = scope;
    memset(scope, 0, sizeof(*scope));

    data_00587ea8->state = currentDwarfFunctionState;

    if (copts.f26) {
        dwarf_depth_ptr = &currentDwarfFunctionState->depth;
        dwarf_entry_offsets = currentDwarfFunctionState->offsets;
        data_005604a4 = currentDwarfFunctionState->entries;
        data_005604a8 = &currentDwarfFunctionState->pending;
    } else {
        dwarf_depth_ptr = &data_00560498;
        dwarf_entry_offsets = data_00580fb0;
        data_005604a4 = data_00581078;
        data_005604a8 = &data_00581140;
    }

    *dwarf_depth_ptr = -1;
    *data_005604a8 = 0;
    memset(dwinfo_buckets, 0, sizeof(dwinfo_buckets));

    pending_objects = NULL;
    dwarf_node_head = currentDwarfNode = currentDwarfScope = NULL;

    data_005604ac = -1;
    current_block_node = NULL;
}

void insert_type_nodes_recursive(DWInfo *a, DWInfo *b)
{
    Type *t = b->type;

    b->marked = 1;
    switch ((SInt8)t->type) {
        case TYPETEMPLATE:
            CError_FATAL(2211);
            break;

        case TYPEBITFIELD: {
            DWInfo *e;
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            e = find_or_create_dwinfo(TYPE_BITFIELD(t)->bitfieldtype);
            if (e->marked == 0)
                insert_type_nodes_recursive(a, e);
            break;
        }

        case TYPEENUM:
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            break;

        case TYPESTRUCT: {
            StructMember *m;
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            for (m = TYPE_STRUCT(t)->members; m != NULL; m = m->next) {
                DWInfo *e = find_or_create_dwinfo(m->type);
                if (e->marked == 0)
                    insert_type_nodes_recursive(b, e);
            }
            break;
        }

        case TYPEPOINTER: {
            DWInfo *e;
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            e = find_or_create_dwinfo(TPTR_TARGET(t));
            if (e->marked == 0)
                insert_type_nodes_recursive(b, e);
            break;
        }

        case TYPEARRAY: {
            DWInfo *e;
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            e = find_or_create_dwinfo(TPTR_TARGET(t));
            if (e->marked == 0)
                insert_type_nodes_recursive(b, e);
            break;
        }

        case TYPECLASS: {
            ScopeSearch save;
            ObjMemberVar *iv;
            ClassList *cb;
            VClassList *vb;

            if (b->typeNode == NULL)
                insert_type_node_before(a, b);

            if (TYPE_CLASS(t)->vbases != NULL)
                for (vb = TYPE_CLASS(t)->vbases; vb != NULL; vb = vb->next) {
                    DWInfo *e = find_or_create_dwinfo((Type *)vb->base);
                    if (e->marked == 0)
                        insert_type_nodes_recursive(b, e);
                }
            if (TYPE_CLASS(t)->bases != NULL)
                for (cb = TYPE_CLASS(t)->bases; cb != NULL; cb = cb->next) {
                    DWInfo *e = find_or_create_dwinfo((Type *)cb->base);
                    if (e->marked == 0)
                        insert_type_nodes_recursive(b, e);
                }
            for (iv = TYPE_CLASS(t)->ivars; iv != NULL; iv = iv->next) {
                DWInfo *e = find_or_create_dwinfo(iv->type);
                if (e->marked == 0)
                    insert_type_nodes_recursive(b, e);
            }

            CScope_InitScopeSearch(&save, TYPE_CLASS(t)->nspace);
            for (;;) {
                Object *obj = CScope_NextObject(&save);
                DWInfo *e;

                if (obj == NULL)
                    break;
                if (obj->type == NULL)
                    continue;
                if (obj->type->type == TYPEFUNC && (TYPE_FUNC(obj->type)->flags & 0x400) != 0) {
                    if (obj->u.templateFunction->instances == NULL)
                        continue;
                    e = find_or_create_dwinfo(obj->u.templateFunction->instances->object->type);
                } else {
                    e = find_or_create_dwinfo(obj->type);
                }
                if (e->marked == 0)
                    insert_type_nodes_recursive(b, e);
            }
            break;
        }

        case TYPEMEMBERPOINTER:
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            b->typeNode->kind = 0x1f;
            if (TYPE_MEMBER_POINTER(t)->ty1 != NULL) {
                DWInfo *e = find_or_create_dwinfo(TYPE_MEMBER_POINTER(t)->ty1);
                if (e->marked == 0)
                    insert_type_nodes_recursive(b, e);
            }
            if (TYPE_MEMBER_POINTER(t)->ty2 != NULL) {
                DWInfo *e = find_or_create_dwinfo(TYPE_MEMBER_POINTER(t)->ty2);
                if (e->marked == 0)
                    insert_type_nodes_recursive(b, e);
            }
            break;

        case TYPEFUNC: {
            FuncArg *arg;
            if (b->typeNode == NULL)
                insert_type_node_before(a, b);
            b->typeNode->kind = 0x15;
            if (TYPE_FUNC(t)->functype != NULL) {
                DWInfo *e = find_or_create_dwinfo(TYPE_FUNC(t)->functype);
                if (e->marked == 0)
                    insert_type_nodes_recursive(b, e);
            }
            for (arg = TYPE_FUNC(t)->args; arg != NULL; arg = arg->next) {
                if (arg->type != NULL) {
                    DWInfo *e = find_or_create_dwinfo(arg->type);
                    if (e->marked == 0)
                        insert_type_nodes_recursive(b, e);
                }
            }
            break;
        }

        case TYPELABEL:
            break;
    }
}

void create_type_node(DWInfo *typeLink)
{
    DwarfNode *node;
    UInt16 kind;
    DwarfNode *current;
    DwarfNode *newNode;

    kind = 0;
    switch ((SInt8)typeLink->type->type) {
        case TYPEENUM:
        case TYPESTRUCT:
        case TYPECLASS:
        case TYPEMEMBERPOINTER:
        case TYPEARRAY:
            kind = 0x13;
            break;
        case TYPEFUNC:
            kind = 0xffff;
            break;
        case TYPEPOINTER:
            kind = 0x4081;
            break;
        case TYPETEMPLATE:
            CError_FATAL(2175);
            break;
        default:
            return;
    }
    if (current_block_node == NULL) {
        currentDwarfNode->next = (DwarfNode *)galloc(sizeof(DwarfNode));
        memclrw(currentDwarfNode->next, sizeof(DwarfNode));
        currentDwarfNode->next->prev = currentDwarfNode;
        currentDwarfNode = currentDwarfNode->next;
        currentDwarfNode->scope = currentDwarfFunctionState;
        node = currentDwarfNode;
        node->scope = data_00587168;
    } else {
        current = current_block_node;
        CError_ASSERT(2106, current != NULL);
        newNode = (DwarfNode *)galloc(sizeof(DwarfNode));
        memclrw(newNode, sizeof(DwarfNode));
        newNode->next = current;
        newNode->prev = current->prev;
        CError_ASSERT(2110, newNode->prev != NULL);
        current->prev = newNode;
        newNode->prev->next = newNode;
        newNode->scope = data_00587168;
        node = newNode;
    }
    node->kind = kind;
    node->type = typeLink;
    {
        DWInfo *info = node->type;
        info->typeNode = node;
    }
}

void insert_type_node_before(DWInfo *before, DWInfo *info)
{
    DwarfNode *node;
    UInt16 kind;
    Type *type;
    DwarfNode *list;
    DWInfo *owner;

    type = info->type;
    if (before == NULL)
        CError_FATAL(2124);
    switch ((SInt8)type->type) {
        case TYPEENUM:
        case TYPESTRUCT:
        case TYPECLASS:
        case TYPEMEMBERPOINTER:
        case TYPEARRAY:
            kind = 0x13;
            break;
        case TYPEFUNC:
            kind = 0xffff;
            break;
        case TYPEPOINTER:
            kind = 0x4081;
            break;
        case TYPETEMPLATE:
            CError_FATAL(2141);
            break;
        default:
            return;
    }
    list = before->typeNode;
    if (list == NULL)
        CError_FATAL(2106);
    node = galloc(sizeof(*node));
    memclrw(node, sizeof(*node));
    node->next = list;
    node->prev = list->prev;
    if (node->prev == NULL)
        CError_FATAL(2110);
    list->prev = node;
    node->prev->next = node;
    node->scope = data_00587168;
    node->kind = kind;
    node->type = info;
    owner = (DWInfo *)node->type;
    owner->typeNode = node;
}

static void SetScope(DwarfFunctionState *obj)
{
    currentDwarfFunctionState = obj;
    if (copts.f26 != 0) {
        dwarf_info_buffer = &obj->info;
        dwarf_depth_ptr = &obj->depth;
        dwarf_entry_offsets = obj->offsets;
        data_005604a4 = obj->entries;
        data_005604a8 = &obj->pending;
    }
    dwarf_lines = &obj->lines;
    section_buffer = ObjGen_PPC_EABI_GetSectionBuffer(obj->section);
}

static void InitOffsets(int offset)
{
    currentDwarfFunctionState->offset = 0;
    currentDwarfFunctionState->lineSectionOffset = offset;
}

static void WritePadding(int amount)
{
    AppendGListLong(dwarf_info_buffer, amount);
    amount -= 4;
    if (amount >= 2) {
        AppendGListWord(dwarf_info_buffer, 0);
        amount -= 2;
    }
    if (amount == 1)
        AppendGListByte(dwarf_info_buffer, 0);
}

static long ReadPosition(void)
{
    return *(long *)*dwarf_lines->data;
}

static void WritePosition(long pos)
{
    *(long *)*dwarf_lines->data = pos;
}

static inline UInt8 DWARF_FullDebugEnabled(void)
{
    return copts.f26;
}

static inline void DWARF_ActivateFunctionState(struct ObjGenSection *section)
{
    DwarfFunctionState *functionState = section->debugState;
    currentDwarfFunctionState = functionState;
    if (DWARF_FullDebugEnabled() != 0) {
        dwarf_info_buffer = &functionState->info;
        dwarf_depth_ptr = &functionState->depth;
        dwarf_entry_offsets = functionState->offsets;
        data_005604a4 = functionState->entries;
        data_005604a8 = &functionState->pending;
    }
    dwarf_lines = &functionState->lines;
    section_buffer = ObjGen_PPC_EABI_GetSectionBuffer(functionState->section);
}

static inline UInt8 dwarf_uses_zero_addends(void)
{
    return copts.fd5;
}

static inline char *dwarfFileName(char *path, Boolean fullPath)
{
    SInt32 i;
    if (fullPath == 0) {
        for (i = (SInt32)strlen(path); i >= 0; i--) {
            if (path[i] == '\\' || path[i] == '/')
                return &path[i + 1];
        }
    }
    return path;
}

static inline UInt8 dwarfZeroRelocationAddends(void)
{
    return copts.fd5;
}

static inline ObjGenSection *dwarfLineSection(void)
{
    return DAT_00587698;
}

static inline UInt8 dwarf_zero_addend_mode(void)
{
    return copts.fd5;
}

#pragma opt_propagation off

static inline UInt8 NextQual(UInt16 *q)
{
    UInt32 v = (UInt16)*q;

    if (v & Q_REFERENCE) {
        *q -= Q_REFERENCE;
        return 2;
    }
    if (v & Q_CONST) {
        *q -= Q_CONST;
        return 3;
    }
    if (v & Q_INLINE_DATA) {
        return 3;
    }
    if (v & Q_VOLATILE) {
        *q -= Q_VOLATILE;
        return 4;
    }
    return 0;
}

#pragma opt_propagation reset

void DWARF_CreateObjectDebugEntry(Object *object)
{
    DWInfo *typeEntry;
    DwarfNode *entry;

    if (copts.filesyminfo == 0)
        return;
    if (object->name->name[0] == '@')
        return;
    if (object->debugInfo.entry != NULL)
        return;

    typeEntry = find_or_create_dwinfo(object->type);
    if (typeEntry->typeNode == NULL)
        create_type_node(typeEntry);

    currentDwarfNode->next = (DwarfNode *)galloc(sizeof(DwarfNode));
    memclrw(currentDwarfNode->next, sizeof(DwarfNode));
    currentDwarfNode->next->prev = currentDwarfNode;
    currentDwarfNode = currentDwarfNode->next;
    currentDwarfNode->scope = currentDwarfFunctionState;

    entry = currentDwarfNode;
    object->debugInfo.entry = entry;
    if (object->dwarfLinks.pendingEntry != NULL) {
        CError_Internal(CERROR_FILE, 0x7f5);
        object->dwarfLinks.pendingEntry->u.sym.replacement = entry;
    }

    entry->kind = 7;
    entry->type = typeEntry;
    entry->u.block.object = object;
    entry->u.block.codeSize = 0;
    entry->u.block.codeOffset = 0;
}

void emit_variable_entry(struct DwarfSym *sym, Object *obj, DWInfo *arg)
{
    SInt32 size;
    SInt32 offset;
    DwarfLocationOperand local;
    DwarfFixup *fixup;
    SInt32 length;
    char *name;

    offset = dwarf_info_buffer->size;
    fixup = sym->fixups;
    while (fixup != NULL) {
        if (copts.fd5 != 0) {
            BE_elf_SetRelocationValue(fixup->reference, offset);
        } else {
            *(SInt32 *)(*dwarf_info_buffer->data + fixup->offset) = offset;
        }
        fixup = fixup->next;
    }
    sym->fixups = fixup;
    sym->offset = offset;

    get_type_dwarf_ref(arg, obj->qual, 1);
    size = emit_entry_header(CParser_HasInternalLinkage(obj) ? 12 : 7);
    AppendGListWord(dwarf_info_buffer, 0x38);
    size += 2;
    name = obj->name->name;
    length = 0;
    while (name[length] != 0) {
        AppendGListByte(dwarf_info_buffer, name[length]);
        length++;
    }
    AppendGListByte(dwarf_info_buffer, 0);
    length++;
    size += length;
    if (copts.cplusplus != 0) {
        if (obj->name->name != COptimizer_GetFunctionObject(obj)->name) {
            AppendGListWord(dwarf_info_buffer, 0x2008);
            size += 2;
            name = COptimizer_GetFunctionObject(obj)->name;
            length = 0;
            while (name[length] != 0) {
                AppendGListByte(dwarf_info_buffer, name[length]);
                length++;
            }
            AppendGListByte(dwarf_info_buffer, 0);
            length++;
            size += length;
        }
    }
    local.kind = 7;
    local.operand.value = 0;
    local.operand.reference.reference = obj;
    size += emit_dwarf_ref(arg);
    size += emit_location_attribute(&local, 0x23, 0);
    *(SInt32 *)(*dwarf_info_buffer->data + offset) = size;
}

void DWARF_AddVar(Object *record, SInt32 offset)
{
    VarInfo *registerInfo = record->u.var.info;
    DWInfo *debugType = find_or_create_dwinfo(record->type);
    DwarfNode *node;

    if (debugType->typeNode == NULL)
        create_type_node(debugType);

    if (currentDwarfScope != NULL) {
        DwarfNode *previous = currentDwarfScope->next;
        currentDwarfScope->next = (DwarfNode *)galloc(sizeof(DwarfNode));
        memclrw(currentDwarfScope->next, sizeof(DwarfNode));
        currentDwarfScope->next->prev = currentDwarfScope;
        currentDwarfScope = currentDwarfScope->next;
        currentDwarfScope->scope = currentDwarfFunctionState;
        currentDwarfScope->next = previous;
        if (previous == NULL)
            currentDwarfNode = currentDwarfScope;
        node = currentDwarfScope;
    } else {
        currentDwarfNode->next = (DwarfNode *)galloc(sizeof(DwarfNode));
        memclrw(currentDwarfNode->next, sizeof(DwarfNode));
        currentDwarfNode->next->prev = currentDwarfNode;
        currentDwarfNode = currentDwarfNode->next;
        currentDwarfNode->scope = currentDwarfFunctionState;
        node = currentDwarfNode;
    }

    node->kind = 5;
    node->type = debugType;
    node->u.var.offset = record->u.var.uid + offset;
    node->u.var.flags = record->qual;
    node->u.var.reg = registerInfo->reg;
    if (registerInfo->reg != 0) {
        if ((copts.operandsDebug && record->type->type == TYPEFLOAT && record->type->size != 4) ||
            ((record->type->type == TYPEINT || record->type->type == TYPEENUM) && record->type->size == 8))
            node->u.var.reg2 = registerInfo->regHi;
    }
    node->u.var.name = record->name;
}

void DWARF_004ad260(DWInfo *ptype, union DwarfNodePayload *info)
{
    Type *type = ptype->type;
    SInt32 length;
    UInt16 reg;
    UInt16 secondReg;
    SInt32 entryOffset;
    SInt32 stackOffset;
    HashNameNode *name;
    SInt32 nameLength;
    DwarfLocationOperand location;
    DwarfLocationOperand registerPair;

    stackOffset = info->var.offset;
    reg = info->var.reg;
    secondReg = info->var.reg2;
    name = info->var.name;
    entryOffset = dwarf_info_buffer->size;

    get_type_dwarf_ref(ptype, info->var.flags, 1);
    length = emit_entry_header(5);
    AppendGListWord(dwarf_info_buffer, 0x38);
    length += 2;
    for (nameLength = 0; name->name[nameLength] != 0; nameLength++)
        AppendGListByte(dwarf_info_buffer, name->name[nameLength]);
    AppendGListByte(dwarf_info_buffer, 0);
    nameLength++;
    length += nameLength;
    registerPair.kind = 0;

    if (reg != 0) {
        UInt8 typeCode;
        UInt8 useGPR;
        SInt32 structKind;
        if ((typeCode = type->type) == TYPEFLOAT) {
            if ((useGPR = copts.operandsDebug) && typeCode == TYPEFLOAT) {
                if (useGPR && typeCode == TYPEFLOAT && type->size != 4) {
                    location.kind = 1;
                    location.operand.registers.first = secondReg;
                    registerPair.operand.registers.first = reg;
                    location.operand.registers.offset = 0;
                    registerPair.kind = 8;
                    registerPair.operand.registers.second = secondReg;
                } else {
                    location.kind = 1;
                    location.operand.registers.first = reg;
                    location.operand.registers.offset = 0;
                }
            } else {
                location.kind = 2;
                location.operand.registers.first = reg;
            }
        } else if (typeCode == TYPESTRUCT && (structKind = TYPE_STRUCT(type)->stype) >= 4 && structKind <= 0xe) {
            location.kind = 4;
            location.operand.registers.first = reg;
        } else if ((typeCode == TYPEINT || typeCode == TYPEENUM) && type->size == 8) {
            location.kind = 1;
            location.operand.registers.first = secondReg;
            registerPair.operand.registers.first = reg;
            location.operand.registers.offset = 0;
            registerPair.kind = 8;
            registerPair.operand.registers.second = secondReg;
        } else {
            location.kind = 1;
            location.operand.registers.first = reg;
            location.operand.registers.offset = 0;
        }
    } else {
        location.kind = 5;
        location.operand.value = stackOffset;
        location.operand.indirect.reg = stack_base_reg;
    }

    length += emit_dwarf_ref(ptype);
    if (registerPair.kind == 8)
        length += emit_location_attribute(&registerPair, 0x2343, 0);
    length += emit_location_attribute(&location, 0x23, 0);
    *(SInt32 *)(*dwarf_info_buffer->data + entryOffset) = length;
}

void DWARF_AddLocalVariable(Object *parameter, int offset)
{
    VarInfo *registers;
    DWInfo *type;
    DwarfNode *location;

    registers = parameter->u.var.info;
    if (parameter->datatype != DLOCAL)
        return;
    type = find_or_create_dwinfo(parameter->type);
    if (type->typeNode == NULL)
        create_type_node(type);
    currentDwarfNode->next = (DwarfNode *)galloc(sizeof(DwarfNode));
    memclrw(currentDwarfNode->next, sizeof(DwarfNode));
    currentDwarfNode->next->prev = currentDwarfNode;
    currentDwarfNode = currentDwarfNode->next;
    currentDwarfNode->scope = currentDwarfFunctionState;
    location = currentDwarfNode;
    currentDwarfNode->kind = 0xc;
    location->type = type;
    location->u.var.offset = parameter->u.var.uid + offset;
    location->u.var.flags = parameter->qual;
    location->u.var.reg = registers->reg;
    if (registers->reg != 0) {
        if ((copts.operandsDebug != 0 && parameter->type->type == TYPEFLOAT && parameter->type->size != 4) ||
            ((parameter->type->type == TYPEINT || parameter->type->type == TYPEENUM) && parameter->type->size == 8))
            location->u.var.reg2 = registers->regHi;
    }
    location->u.var.name = parameter->name;
}

void DWARF_004ad570(DWInfo *ptype, union DwarfNodePayload *info)
{
    Type *type = ptype->type;
    SInt32 len;
    UInt16 reg;
    UInt16 secondReg;
    SInt32 entryOffset;
    SInt32 variableOffset;
    HashNameNode *name;
    SInt32 nameLength;
    DwarfLocationOperand location;
    DwarfLocationOperand registerPair;

    variableOffset = info->var.offset;
    reg = info->var.reg;
    secondReg = info->var.reg2;
    name = info->var.name;
    entryOffset = dwarf_info_buffer->size;

    get_type_dwarf_ref(ptype, info->var.flags, 1);
    len = emit_entry_header(12);
    AppendGListWord(dwarf_info_buffer, 0x38);
    len += 2;
    for (nameLength = 0; name->name[nameLength] != 0; nameLength++)
        AppendGListByte(dwarf_info_buffer, name->name[nameLength]);
    AppendGListByte(dwarf_info_buffer, 0);
    nameLength++;
    len += nameLength;
    registerPair.kind = 0;

    if (reg != 0) {
        UInt8 typecode;
        UInt8 useGpr;
        SInt32 structureKind;
        if ((typecode = type->type) == TYPEFLOAT) {
            if ((useGpr = copts.operandsDebug) && typecode == TYPEFLOAT) {
                if (useGpr && typecode == TYPEFLOAT && type->size != 4) {
                    location.kind = 1;
                    location.operand.registers.first = secondReg;
                    registerPair.operand.registers.first = reg;
                    location.operand.registers.offset = 0;
                    registerPair.kind = 8;
                    registerPair.operand.registers.second = secondReg;
                } else {
                    location.kind = 1;
                    location.operand.registers.first = reg;
                    location.operand.registers.offset = 0;
                }
            } else {
                location.kind = 2;
                location.operand.registers.first = reg;
            }
        } else if (typecode == TYPESTRUCT && (structureKind = TYPE_STRUCT(type)->stype) >= 4 && structureKind <= 0xe) {
            location.kind = 4;
            location.operand.registers.first = reg;
        } else if ((typecode == TYPEINT || typecode == TYPEENUM) && type->size == 8) {
            location.kind = 1;
            location.operand.registers.first = secondReg;
            registerPair.operand.registers.first = reg;
            location.operand.registers.offset = 0;
            registerPair.kind = 8;
            registerPair.operand.registers.second = secondReg;
        } else {
            location.kind = 1;
            location.operand.registers.first = reg;
            location.operand.registers.offset = 0;
        }
    } else {
        location.kind = 5;
        location.operand.value = variableOffset;
        location.operand.indirect.reg = stack_base_reg;
    }

    len += emit_dwarf_ref(ptype);
    if (registerPair.kind == 8)
        len += emit_location_attribute(&registerPair, 0x2343, 0);
    len += emit_location_attribute(&location, 0x23, 0);
    *(SInt32 *)(*dwarf_info_buffer->data + entryOffset) = len;
}

SInt32 emit_dwarf_ref(DWInfo *info)
{
    DwarfRef *ref = &info->rec;
    volatile DwarfRef *tagRef = ref;
    QualNode *qualifier;
    SInt32 size;

    qualifier = ref->qualifiers;
    switch (ref->tag) {
        case 0x55:
            AppendGListWord(dwarf_info_buffer, tagRef->tag);
            size = 2;
            AppendGListWord(dwarf_info_buffer, ref->num);
            size += 2;
            break;
        case 0x83: {
            DWInfo *typeInfo;
            SInt32 lengthOffset;
            SInt32 qualifierCount;
            ObjGenRelocation *relocation;

            AppendGListWord(dwarf_info_buffer, tagRef->tag);
            size = 2;
            lengthOffset = dwarf_info_buffer->size;
            qualifierCount = 0;
            AppendGListWord(dwarf_info_buffer, 0);
            size += 2;
            while (qualifier != NULL) {
                AppendGListByte(dwarf_info_buffer, qualifier->type);
                qualifier = qualifier->next;
                qualifierCount++;
                size++;
            }
            relocation = BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarf_section, ref->num,
                                              currentDwarfFunctionState->offset);
            typeInfo = ref->typeInfo;
            if (!typeInfo->rec.valid) {
                DwarfFixup *fixup;
                CError_ASSERT(1728, typeInfo->typeNode != NULL);
                fixup = (DwarfFixup *)galloc(sizeof(DwarfFixup));
                fixup->reference = relocation;
                fixup->offset = dwarf_info_buffer->size;
                fixup->next = typeInfo->typeNode->u.fixups;
                typeInfo->typeNode->u.fixups = fixup;
            }
            AppendGListLong(dwarf_info_buffer, copts.fd5 ? 0 : ref->num);
            size += 4;
            *(UInt16 *)(*dwarf_info_buffer->data + lengthOffset) = qualifierCount + 4;
            break;
        }
        case 0x63: {
            SInt32 lengthOffset;
            SInt32 qualifierCount;

            AppendGListWord(dwarf_info_buffer, tagRef->tag);
            size = 2;
            lengthOffset = dwarf_info_buffer->size;
            qualifierCount = 0;
            AppendGListWord(dwarf_info_buffer, 0);
            size += 2;
            while (qualifier != NULL) {
                AppendGListByte(dwarf_info_buffer, qualifier->type);
                qualifier = qualifier->next;
                qualifierCount++;
                size++;
            }
            AppendGListWord(dwarf_info_buffer, ref->num);
            size += 2;
            *(UInt16 *)(*dwarf_info_buffer->data + lengthOffset) = qualifierCount + 2;
            break;
        }
        case 0x72:
        case 0x1d2: {
            ObjGenRelocation *relocation;
            DWInfo *typeInfo = info;

            AppendGListWord(dwarf_info_buffer, tagRef->tag);
            size = 2;
            relocation = BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarf_section, ref->num,
                                              currentDwarfFunctionState->offset);
            if (!ref->valid) {
                DwarfFixup *fixup;
                CError_ASSERT(1758, typeInfo->typeNode != NULL);
                fixup = (DwarfFixup *)galloc(sizeof(DwarfFixup));
                fixup->reference = relocation;
                fixup->offset = dwarf_info_buffer->size;
                fixup->next = typeInfo->typeNode->u.fixups;
                typeInfo->typeNode->u.fixups = fixup;
            }
            AppendGListLong(dwarf_info_buffer, copts.fd5 ? 0 : ref->num);
            size += 4;
            break;
        }
        default:
            CError_FATAL(1768);
            break;
    }
    return size;
}

void fn_004ada90(void)
{
    return;
}

#pragma sym on

void DWARF_WriteDebugInfo(void)
{
    DWInfo *entryData;
    DwarfStateList *scopeLink;
    DwarfStateList *outputScope;
    DwarfNode *node;
    DwarfNode *entry;
    SInt32 offset;
    SInt32 length;
    int padding;
    unsigned short tag;

    for (currentDwarfNode = dwarf_node_head; (entry = currentDwarfNode->next) != NULL;
         currentDwarfNode = currentDwarfNode->next) {
        if (entry->kind == 0x13 || entry->kind == 0x4081) {
            entryData = (DWInfo *)entry->type;
            if (entryData->marked == 0) {
                insert_type_nodes_recursive(NULL, entryData);
            }
        }
    }

    offset = 0;
    for (scopeLink = data_00587ea8; scopeLink != NULL; scopeLink = scopeLink->next) {
        SetScope(scopeLink->state);
        InitOffsets(offset);
        AppendGListLong(dwarf_lines, 0);
        length = ReadPosition();
        AppendGListWord(dwarf_lines, -1);
        AppendGListLong(dwarf_lines, section_buffer->size);
        WritePosition(length + 10);
        offset += dwarf_lines->size;
    }

    offset = 0;
    for (scopeLink = data_00587ea8; scopeLink != NULL;) {
        SetScope(scopeLink->state);
        if (copts.f26 != 0) {
            currentDwarfFunctionState->offset = offset;
        }
        if (copts.f26 != 0 || scopeLink->state == data_00587168) {
            for (currentDwarfNode = dwarf_node_head; (node = currentDwarfNode->next) != NULL;) {
                if (copts.f26 != 0 && node->scope != currentDwarfFunctionState) {
                    currentDwarfNode = node;
                    continue;
                } else {
                    tag = node->kind;
                    entryData = (DWInfo *)node->type;
                    switch (tag) {
                        case 0x11:
                            emit_compile_unit(currentDwarfFunctionState->section, node->u.regByte);
                            break;
                        case 0x4080:
                            if (copts.fd4 != 0 && *data_005604a8 != 0) {
                                if (copts.fd5 != 0) {
                                    BE_elf_SetRelocationValue(data_005604a4[*dwarf_depth_ptr],
                                                              dwarf_info_buffer->size +
                                                                  currentDwarfFunctionState->offset);
                                } else {
                                    *(int *)(*dwarf_info_buffer->data + dwarf_entry_offsets[*dwarf_depth_ptr]) =
                                        dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                                }
                                AppendGListLong(dwarf_info_buffer, 4);
                            }
                            *dwarf_depth_ptr -= 1;
                            if (copts.f26 == 0) {
                                SetScope(data_00587168);
                            }
                            break;
                        case 0x15:
                            emit_function_type(&node->u.fixups, entryData);
                            break;
                        case 0x1f:
                            emit_member_pointer_type(&node->u.fixups, entryData);
                            break;
                        case 7:
                            emit_variable_entry(&node->u.sym, node->u.sym.object, node->type);
                            break;
                        case 5:
                            DWARF_004ad260(entryData, &node->u);
                            break;
                        case 0xc:
                            DWARF_004ad570(entryData, &node->u);
                            break;
                        case 6:
                        case 0x14:
                            if (copts.f26 == 0) {
                                SetScope(node->scope);
                            }
                            emit_function_entry(node->u.block.object, node->u.block.codeSize, node->u.block.codeOffset,
                                                node->u.block.returnOperand, node->u.block.pendingObjects);
                            break;
                        case 0x13:
                            if (entryData->marked != 0) {
                                get_type_dwarf_ref(entryData, node->u.var.flags, 0);
                            }
                            break;
                        case 0x4081:
                        case 0xffff:
                            break;
                        default:
                            CError_Internal(CERROR_FILE, 0x66d);
                            break;
                    }
                }
                currentDwarfNode = currentDwarfNode->next;
            }
            if (copts.fd4 != 0) {
                if ((int)(entry = (DwarfNode *)*dwarf_depth_ptr) >= 0 && data_005604a4[(int)entry] != NULL) {
                    if (copts.fd5 != 0) {
                        BE_elf_SetRelocationValue(data_005604a4[(int)entry],
                                                  dwarf_info_buffer->size + currentDwarfFunctionState->offset);
                    } else {
                        *(int *)(*dwarf_info_buffer->data + dwarf_entry_offsets[(int)entry]) =
                            dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                    }
                    AppendGListLong(dwarf_info_buffer, 4);
                }
                AppendGListLong(dwarf_info_buffer, 4);
            }
            *dwarf_depth_ptr -= 1, padding = 4 - (dwarf_info_buffer->size % 4);
            if (padding < 4) {
                WritePadding(padding + 4);
            }
            if (copts.fd5 != 0) {
                BE_elf_SetRelocationValue(currentDwarfFunctionState->pendingReference,
                                          dwarf_info_buffer->size + currentDwarfFunctionState->offset);
            } else {
                *(int *)(*dwarf_info_buffer->data + currentDwarfFunctionState->pendingPosition) =
                    dwarf_info_buffer->size + currentDwarfFunctionState->offset;
            }
        }
        scopeLink = scopeLink->next;
        offset += dwarf_info_buffer->size;
    }

    for (outputScope = data_00587ea8; outputScope != NULL; outputScope = outputScope->next) {
        if (outputScope->state != NULL) {
            if (outputScope->state->info.size != 0) {
                BE_elf_AppendGList(&dwarf_info_section->buffer, &outputScope->state->info);
                FreeGList(&outputScope->state->info);
            }
            if (outputScope->state->lines.size != 0) {
                BE_elf_AppendGList(&dwarf_line_section->buffer, &outputScope->state->lines);
                FreeGList(&outputScope->state->lines);
            }
        }
    }
}

#pragma sym reset

unsigned int DWARF_RestoreFunctionState(void)
{
    DwarfFunctionState *A;
    GList *r;

    currentDwarfNode->next = (DwarfNode *)galloc(38U);
    memclrw(currentDwarfNode->next, 38U);
    currentDwarfNode->next->prev = currentDwarfNode;
    currentDwarfNode = currentDwarfNode->next;
    currentDwarfNode->scope = currentDwarfFunctionState;
    currentDwarfNode->kind = 0x4080U;
    currentDwarfScope = 0U;
    A = data_00587168;
    currentDwarfFunctionState = A;
    if (copts.f26 != 0U) {
        dwarf_info_buffer = &A->info;
        dwarf_depth_ptr = &A->depth;
        dwarf_entry_offsets = A->offsets;
        data_005604a4 = A->entries;
        data_005604a8 = &A->pending;
    }
    dwarf_lines = &A->lines;
    r = ObjGen_PPC_EABI_GetSectionBuffer(A->section);
    section_buffer = r;
    if (--data_005604ac == -1)
        current_block_node = 0U;
    return (unsigned int)r;
}

int DWARF_CreateBlockNode(Object *object, SInt32 codeSize, SInt32 codeOffset, ObjGenSection *section)
{
    DwarfNode *node;
    DWInfo *debugType;

    DWARF_SetupFunctionState(section);
    setup_return_operand(object);
    currentDwarfNode->next = galloc(sizeof(DwarfNode));
    memclrw(currentDwarfNode->next, sizeof(DwarfNode));
    currentDwarfNode->next->prev = currentDwarfNode;
    currentDwarfNode = currentDwarfNode->next;
    currentDwarfNode->scope = currentDwarfFunctionState;
    node = currentDwarfNode;
    if (++data_005604ac == 0)
        current_block_node = node;
    node->kind = 0x14;
    node->u.block.object = object;
    node->u.block.codeSize = codeSize;
    node->u.block.codeOffset = codeOffset;
    node->u.block.pendingObjects = pending_objects;
    pending_objects = NULL;
    node->u.block.returnOperand = galloc(sizeof(return_operand));
    *(DwarfLocationOperand *)node->u.block.returnOperand = return_operand;

    if (copts.cplusplus && (TYPE_METHOD(object->type)->flags & FUNC_METHOD)) {
        debugType = find_or_create_dwinfo((Type *)TYPE_METHOD(object->type)->theclass);
        if (debugType->typeNode == NULL)
            create_type_node(debugType);
    }
    if (TYPE_FUNC(object->type)->functype != NULL) {
        debugType = find_or_create_dwinfo(TYPE_FUNC(object->type)->functype);
        if (debugType->typeNode == NULL)
            create_type_node(debugType);
    }
    if (!copts.f26 && data_0058114e) {
        data_0058114e = 0;
        AppendGListLong(dwarf_lines, 4);
        AppendGListLong(dwarf_lines, 0);
        *(SInt32 *)*dwarf_lines->data = *(SInt32 *)*dwarf_lines->data + 4;
    }
    currentDwarfScope = node;
    return 1;
}

void DWARF_SetupFunctionState(struct ObjGenSection *functionSection)
{
    if (functionSection->debugState == NULL) {
        currentDwarfFunctionState = galloc(sizeof(DwarfFunctionState));
        memset(currentDwarfFunctionState, 0, sizeof(DwarfFunctionState));
        currentDwarfFunctionState->depth = -1;
        currentDwarfFunctionState->section = functionSection;
        if (InitGList(&currentDwarfFunctionState->lines, 1000) != 0)
            CError_LongJump();
        if (InitGList(&currentDwarfFunctionState->info, 5000) != 0)
            CError_LongJump();
        functionSection->debugState = currentDwarfFunctionState;
        dwarf_state_list_tail->next = galloc(sizeof(DwarfStateList));
        dwarf_state_list_tail = dwarf_state_list_tail->next;
        memset(dwarf_state_list_tail, 0, sizeof(DwarfStateList));
        dwarf_state_list_tail->state = currentDwarfFunctionState;
        if (DWARF_FullDebugEnabled() != 0) {
            DWARF_SetupSectionDebugState(functionSection);
        } else {
            DWARF_ActivateFunctionState(functionSection);
            data_0058114e = 1;
        }
    } else {
        DWARF_ActivateFunctionState(functionSection);
    }
}

SInt32 emit_function_entry(Object *func, SInt32 code_size, SInt32 code_offset,
                           DwarfLocationOperand *return_type_location, PendingObject *variables)
{
    DwarfRef class_reference;
    SInt32 entry_size;
    SInt32 start_offset;

    start_offset = dwarf_info_buffer->size;
    if (CParser_HasInternalLinkage(func))
        entry_size = emit_entry_header(0x14);
    else
        entry_size = emit_entry_header(6);

    AppendGListWord(dwarf_info_buffer, 0x38);
    entry_size += 2;
    {
        char *name = func->name->name;
        SInt32 length;
        for (length = 0; name[length] != 0; length++)
            AppendGListByte(dwarf_info_buffer, name[length]);
        AppendGListByte(dwarf_info_buffer, 0);
        entry_size += length + 1;
    }

    if (copts.cplusplus != 0) {
        if (func->name->name != COptimizer_GetFunctionObject(func)->name) {
            char *name;
            SInt32 length;
            AppendGListWord(dwarf_info_buffer, 0x2008);
            entry_size += 2;
            name = COptimizer_GetFunctionObject(func)->name;
            for (length = 0; name[length] != 0; length++)
                AppendGListByte(dwarf_info_buffer, name[length]);
            AppendGListByte(dwarf_info_buffer, 0);
            entry_size += length + 1;
        }
    }

    if (copts.cplusplus != 0 && (TYPE_METHOD(func->type)->flags & FUNC_METHOD) != 0) {
        DwarfFixup *fixup;
        ObjGenRelocation *reference;
        DWInfo *symbol;
        AppendGListWord(dwarf_info_buffer, 0x142);
        symbol = find_or_create_dwinfo((Type *)TYPE_METHOD(func->type)->theclass);
        class_reference = get_type_dwarf_ref(symbol, 0, 1);
        reference = BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarf_section,
                                         class_reference.num, currentDwarfFunctionState->offset);
        if (class_reference.valid == 0) {
            CError_ASSERT(1337, symbol->typeNode != 0);
            fixup = galloc(sizeof(DwarfFixup));
            fixup->reference = reference;
            fixup->offset = dwarf_info_buffer->size;
            fixup->next = symbol->typeNode->u.fixups;
            symbol->typeNode->u.fixups = fixup;
        }
        AppendGListLong(dwarf_info_buffer, dwarf_uses_zero_addends() ? 0 : class_reference.num);
        entry_size += 6;
    }

    if (return_type_location->kind != 0) {
        DWInfo *symbol;
        symbol = find_or_create_dwinfo(return_type_location->metadata.type);
        get_type_dwarf_ref(symbol, 0, 1);
        if (return_type_location->kind == 5 || return_type_location->kind == 0xa)
            entry_size += emit_location_attribute(return_type_location, 0x2a3, 1);
        entry_size += emit_dwarf_ref(symbol);
    }

    AppendGListWord(dwarf_info_buffer, 0x111);
    BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, func, NULL, 0, currentDwarfFunctionState->offset);
    AppendGListLong(dwarf_info_buffer, dwarf_uses_zero_addends() ? 0 : code_offset);

    AppendGListWord(dwarf_info_buffer, 0x121);
    BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, currentDwarfFunctionState->section,
                         code_offset + code_size, currentDwarfFunctionState->offset);
    AppendGListLong(dwarf_info_buffer, dwarf_uses_zero_addends() ? 0 : code_offset + code_size);
    entry_size += 0xc;

    if (copts.f26 == 0) {
        if (currentDwarfFunctionState != data_00587168) {
            AppendGListWord(dwarf_info_buffer, 0x106);
            BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, DAT_00587698,
                                 currentDwarfFunctionState->lineSectionOffset, currentDwarfFunctionState->offset);
            AppendGListLong(dwarf_info_buffer,
                            dwarf_uses_zero_addends() ? 0 : currentDwarfFunctionState->lineSectionOffset);
            entry_size += 6;
        }
        if (currentDwarfFunctionState->lineBaseRelocated == 0) {
            currentDwarfFunctionState->lineBaseRelocated = 1;
            BE_elf_AddRelocation(DAT_00587698, 4, NULL, currentDwarfFunctionState->section, 0,
                                 currentDwarfFunctionState->lineSectionOffset);
        }
    }

    {
        PendingObject *variable = variables;
        while (variable != NULL) {
            Object *variable_object = variable->object;
            if (variable_object->debugInfo.symbol != NULL) {
                DwarfSymbol *symbol;
                ObjGenRelocation *reference;
                DwarfFixup *fixup;
                AppendGListWord(dwarf_info_buffer, 0x2022);
                variable_object = variable->object;
                symbol = variable_object->debugInfo.symbol;
                reference = BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarf_section,
                                                 symbol->u.offset, currentDwarfFunctionState->offset);
                if (symbol->u.offset == 0) {
                    fixup = galloc(sizeof(DwarfFixup));
                    fixup->reference = reference;
                    fixup->offset = dwarf_info_buffer->size;
                    fixup->next = symbol->references;
                    symbol->references = fixup;
                }
                AppendGListLong(dwarf_info_buffer, dwarf_uses_zero_addends() ? 0 : symbol->u.offset);
                entry_size += 6;
            } else {
                char *name;
                SInt32 length;
                CError_FATAL(1394);
                AppendGListWord(dwarf_info_buffer, 0x2038);
                entry_size += 2;
                name = COptimizer_GetFunctionObject(variable->object)->name;
                for (length = 0; name[length] != 0; length++)
                    AppendGListByte(dwarf_info_buffer, name[length]);
                AppendGListByte(dwarf_info_buffer, 0);
                entry_size += length + 1;
            }
            variable = variable->next;
        }
    }

    *(SInt32 *)(*dwarf_info_buffer->data + start_offset) = entry_size;

    CError_ASSERT(1401, ++*dwarf_depth_ptr < 0x32);
    dwarf_entry_offsets[*dwarf_depth_ptr] = 0;
    *data_005604a8 = 0;
    return start_offset;
}

void emit_member_pointer_type(DwarfFixup **references, DWInfo *type)
{
    TypeMemberPointer *entry;
    DwarfFixup *reference;
    SInt32 offset;
    DwarfRef saved;
    DWInfo *record;
    SInt32 size;

    entry = (TypeMemberPointer *)type->type;
    offset = dwarf_info_buffer->size;
    reference = *references;
    while (reference != NULL) {
        if (copts.fd5 != 0) {
            BE_elf_SetRelocationValue(reference->reference, offset);
        } else {
            *(SInt32 *)(*dwarf_info_buffer->data + reference->offset) = offset;
        }
        reference = reference->next;
    }
    *references = reference;
    get_type_dwarf_ref(type, 0, 0);
    size = emit_entry_header(0x1f);
    if (entry->ty1 != NULL) {
        record = find_or_create_dwinfo(entry->ty1);
        get_type_dwarf_ref(record, 0, 1);
        size += emit_dwarf_ref(record);
    }
    if (entry->ty2 != NULL) {
        record = find_or_create_dwinfo(entry->ty2);
        get_type_dwarf_ref(record, 0, 1);
        saved = record->rec;
        record->rec.tag = 0x1d2;
        size += emit_dwarf_ref(record);
        record->rec = saved;
    }
    *(SInt32 *)(*dwarf_info_buffer->data + offset) = size;
}

void emit_function_type(DwarfFixup **fixups, DWInfo *function)
{
    TypeFunc *type;
    DwarfFixup *fixup;
    FuncArg *argument;
    DwarfNode *savedNext;
    DWInfo *typeRecord;
    SInt32 size;
    SInt32 offset;

    type = TYPE_FUNC(function->type);
    offset = dwarf_info_buffer->size;
    for (fixup = *fixups; fixup != NULL; fixup = fixup->next) {
        if (copts.fd5 != 0) {
            BE_elf_SetRelocationValue(fixup->reference, offset);
        } else {
            *(SInt32 *)(*dwarf_info_buffer->data + fixup->offset) = offset;
        }
    }
    *fixups = fixup;
    get_type_dwarf_ref(function, 0, 0);
    size = emit_entry_header(0x15);
    if (type->functype != NULL) {
        typeRecord = find_or_create_dwinfo(type->functype);
        get_type_dwarf_ref(typeRecord, 0, 1);
        size += emit_dwarf_ref(typeRecord);
    }
    CError_ASSERT(1230, function->typeNode != NULL);
    currentDwarfScope = function->typeNode;
    *data_005604a8 = 0;
    argument = type->args;
    if (argument != NULL && copts.cplusplus != 0 && (type->flags & FUNC_METHOD) != 0)
        argument = argument->next;
    *(SInt32 *)(*dwarf_info_buffer->data + offset) = size;
    CError_ASSERT(1239, ++*dwarf_depth_ptr < 0x32);
    dwarf_entry_offsets[*dwarf_depth_ptr] = 0;
    for (; argument != NULL; argument = argument->next) {
        if (argument->type != NULL) {
            currentDwarfNode = currentDwarfScope;
            savedNext = currentDwarfScope->next;
            currentDwarfScope->next = (DwarfNode *)galloc(sizeof(DwarfNode));
            memclrw(currentDwarfScope->next, sizeof(DwarfNode));
            currentDwarfScope->next->prev = currentDwarfScope;
            currentDwarfScope = currentDwarfScope->next;
            currentDwarfScope->scope = currentDwarfFunctionState;
            currentDwarfScope->next = savedNext;
            if (savedNext == NULL)
                currentDwarfNode = currentDwarfScope;
            offset = dwarf_info_buffer->size;
            size = emit_entry_header(5);
            typeRecord = find_or_create_dwinfo(argument->type);
            get_type_dwarf_ref(typeRecord, 0, 1);
            size += emit_dwarf_ref(typeRecord);
            *(SInt32 *)(*dwarf_info_buffer->data + offset) = size;
        }
    }
    currentDwarfScope = NULL;
    if (copts.fd4 != 0 && *data_005604a8 != 0) {
        if (copts.fd5 != 0) {
            BE_elf_SetRelocationValue(data_005604a4[*dwarf_depth_ptr],
                                      dwarf_info_buffer->size + currentDwarfFunctionState->offset);
        } else {
            *(SInt32 *)(*dwarf_info_buffer->data + dwarf_entry_offsets[*dwarf_depth_ptr]) =
                dwarf_info_buffer->size + currentDwarfFunctionState->offset;
        }
        AppendGListLong(dwarf_info_buffer, 4);
    }
    --*dwarf_depth_ptr;
    *data_005604a8 = 0;
}

void DWARF_ReplaceTrailingLongWordLong(unsigned int firstValue, unsigned int secondValue)
{
    int savedCount;

    savedCount = *(int *)*dwarf_lines->data;
    dwarf_lines->size -= 10;
    *(int *)*dwarf_lines->data = savedCount - 10;
    savedCount = *(int *)*dwarf_lines->data;
    AppendGListLong(dwarf_lines, firstValue);
    AppendGListWord(dwarf_lines, 0xffffffff);
    AppendGListLong(dwarf_lines, secondValue);
    *(int *)*dwarf_lines->data = savedCount + 10;
}

void DWARF_AppendLongWordLong(unsigned int firstValue, unsigned int secondValue)
{
    int outputPosition;

    outputPosition = *(int *)*dwarf_lines->data;
    AppendGListLong(dwarf_lines, firstValue);
    AppendGListWord(dwarf_lines, 0xffffffff);
    AppendGListLong(dwarf_lines, secondValue);
    *(int *)*dwarf_lines->data = outputPosition + 10;
}

#define DWARF_PRODUCER "MW EABI PPC C-Compiler"

void emit_compile_unit(struct ObjGenSection *section, UInt8 language)
{
    SInt32 recordOffset;
    UInt32 languageValue;
    char sourcePath[1024];
    SInt32 recordSize;
    char *sourceName;
    SInt32 nameLength;

    languageValue = language;
    recordOffset = dwarf_info_buffer->size;
    recordSize = emit_entry_header(0x11);
    AppendGListWord(dwarf_info_buffer, 600);
    recordSize += 2;
    for (nameLength = 0; DWARF_PRODUCER[nameLength] != 0; nameLength++)
        AppendGListByte(dwarf_info_buffer, DWARF_PRODUCER[nameLength]);
    AppendGListByte(dwarf_info_buffer, 0);
    recordSize += nameLength + 1;
    AppendGListWord(dwarf_info_buffer, 0x38);
    recordSize += 2;
    sourceName = ObjGen_PPC_EABI_GetSectionName(section);
    strncpy(sourcePath, sourceName, sizeof(sourcePath));
    sourceName = dwarfFileName(sourcePath, copts.fd6);
    for (nameLength = 0; sourceName[nameLength] != 0; nameLength++)
        AppendGListByte(dwarf_info_buffer, sourceName[nameLength]);
    AppendGListByte(dwarf_info_buffer, 0);
    recordSize += nameLength + 1;
    AppendGListWord(dwarf_info_buffer, 0x136);
    CompilerTools_AppendGListData(dwarf_info_buffer, &languageValue, sizeof(languageValue));
    AppendGListWord(dwarf_info_buffer, 0x111);
    BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, section, 0, currentDwarfFunctionState->offset);
    AppendGListLong(dwarf_info_buffer, 0);
    AppendGListWord(dwarf_info_buffer, 0x121);
    BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, section, section_buffer->size,
                         currentDwarfFunctionState->offset);
    AppendGListLong(dwarf_info_buffer, dwarfZeroRelocationAddends() ? 0 : section_buffer->size);
    AppendGListWord(dwarf_info_buffer, 0x106);
    BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarfLineSection(),
                         currentDwarfFunctionState->lineSectionOffset, currentDwarfFunctionState->offset);
    AppendGListLong(dwarf_info_buffer, dwarfZeroRelocationAddends() ? 0 : currentDwarfFunctionState->lineSectionOffset);
    recordSize += 0x18;
    *(SInt32 *)(*dwarf_info_buffer->data + recordOffset) = recordSize;
    BE_elf_AddRelocation(dwarfLineSection(), 4, NULL, section, 0, currentDwarfFunctionState->lineSectionOffset);
    if (copts.f26 == 0)
        currentDwarfFunctionState->lineBaseRelocated = 1;
    if (++(*dwarf_depth_ptr) >= 50)
        CError_Internal(CERROR_FILE, 0x49b);
    dwarf_entry_offsets[*dwarf_depth_ptr] = 0;
}

void DWARF_SetupSectionDebugState(struct ObjGenSection *section)
{
    DwarfNode *record;
    DwarfFunctionState *state;
    DwarfNode *entry;
    UInt8 language;
    if (section->debugState == NULL) {
        section->debugState = data_00587168;
        section->debugState->section = section;
        if (InitGList(&section->debugState->lines, 1000))
            CError_LongJump();
        if (InitGList(&section->debugState->info, 5000))
            CError_LongJump();
    }
    state = section->debugState;
    currentDwarfFunctionState = state;
    if (copts.f26) {
        dwarf_info_buffer = &state->info;
        dwarf_depth_ptr = &state->depth;
        dwarf_entry_offsets = state->offsets;
        data_005604a4 = state->entries;
        data_005604a8 = &state->pending;
    }
    dwarf_lines = &state->lines;
    section_buffer = ObjGen_PPC_EABI_GetSectionBuffer(state->section);
    record = galloc(sizeof(*record));
    memclrw(record, sizeof(*record));
    dwarf_node_head->prev = record;
    record->next = dwarf_node_head;
    dwarf_node_head = record;
    record->kind = 0xffff;
    dwarf_node_head->next->scope = currentDwarfFunctionState;
    entry = dwarf_node_head->next;
    entry->kind = 0x11;
    if (copts.cplusplus)
        language = 4;
    else
        language = 1;
    entry->u.regByte = language;
    AppendGListLong(dwarf_lines, 4);
    AppendGListLong(dwarf_lines, 0);
    {
        SInt32 *position = (SInt32 *)*dwarf_lines->data;
        *position = *position + 4;
    }
}

#define S dwarf_info_buffer

SInt32 emit_entry_header(SInt16 value)
{
    SInt32 entryIndex;
    ObjGenRelocation *reference;
    SInt32 patchOffset;

    *data_005604a8 = 1;
    entryIndex = *dwarf_depth_ptr;
    if (entryIndex >= 0 && (patchOffset = dwarf_entry_offsets[entryIndex]) != 0) {
        if (copts.fd5 != 0) {
            BE_elf_SetRelocationValue(data_005604a4[entryIndex],
                                      dwarf_info_buffer->size + currentDwarfFunctionState->offset);
        } else {
            *(SInt32 *)(*dwarf_info_buffer->data + patchOffset) =
                dwarf_info_buffer->size + currentDwarfFunctionState->offset;
        }
    }
    AppendGListLong(dwarf_info_buffer, 0);
    AppendGListWord(dwarf_info_buffer, value);
    AppendGListWord(dwarf_info_buffer, 0x12);
    entryIndex = *dwarf_depth_ptr;
    if (entryIndex >= 0) {
        dwarf_entry_offsets[entryIndex] = dwarf_info_buffer->size;
        reference = BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarf_section, 0,
                                         currentDwarfFunctionState->offset);
        data_005604a4[*dwarf_depth_ptr] = reference;
    } else {
        reference = BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, NULL, dwarf_section, 0,
                                         currentDwarfFunctionState->offset);
        currentDwarfFunctionState->pendingReference = reference;
        currentDwarfFunctionState->pendingPosition = dwarf_info_buffer->size;
    }
    AppendGListLong(dwarf_info_buffer, 0);
    return 12;
}

int emit_location_attribute(DwarfLocationOperand *location, UInt16 attribute, unsigned char shouldDereference)
{
    int attributeSize;
    int expressionSize;
    SInt32 expressionSizeOffset;

    if (location->kind == 0)
        return 0;

    AppendGListWord(dwarf_info_buffer, attribute);
    attributeSize = 2;
    expressionSizeOffset = dwarf_info_buffer->size;
    AppendGListNoData(dwarf_info_buffer, attributeSize);
    attributeSize += 2;
    expressionSize = 0;

    switch (location->kind) {
        case 1:
            AppendGListByte(dwarf_info_buffer, 1);
            AppendGListLong(dwarf_info_buffer, location->operand.registers.first);
            expressionSize += 5;
            if (location->operand.registers.offset != 0) {
                AppendGListByte(dwarf_info_buffer, 4);
                AppendGListLong(dwarf_info_buffer, location->operand.registers.offset);
                AppendGListByte(dwarf_info_buffer, 7);
                expressionSize += 6;
            }
            break;
        case 8:
            AppendGListByte(dwarf_info_buffer, location->operand.registers.second + 0x50);
            AppendGListByte(dwarf_info_buffer, -0x6d);
            AppendGListByte(dwarf_info_buffer, 4);
            AppendGListByte(dwarf_info_buffer, location->operand.registers.first + 0x50);
            AppendGListByte(dwarf_info_buffer, -0x6d);
            AppendGListByte(dwarf_info_buffer, 4);
            expressionSize += 6;
            break;
        case 2:
            AppendGListByte(dwarf_info_buffer, 1);
            AppendGListLong(dwarf_info_buffer, location->operand.registers.first + 0x20);
            expressionSize += 5;
            break;
        case 3:
            AppendGListByte(dwarf_info_buffer, -0x7f);
            AppendGListLong(dwarf_info_buffer, location->operand.registers.first);
            expressionSize += 5;
            break;
        case 4:
            AppendGListByte(dwarf_info_buffer, 1);
            AppendGListLong(dwarf_info_buffer, location->operand.registers.first + 0x464);
            expressionSize += 5;
            break;
        case 7:
            AppendGListByte(dwarf_info_buffer, 3);
            BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, location->operand.reference.reference, NULL,
                                 location->operand.reference.value, currentDwarfFunctionState->offset);
            AppendGListLong(dwarf_info_buffer, dwarf_zero_addend_mode() ? 0 : location->operand.reference.value);
            expressionSize += 5;
            break;
        case 5:
            AppendGListByte(dwarf_info_buffer, 2);
            AppendGListLong(dwarf_info_buffer, location->operand.indirect.reg);
            AppendGListByte(dwarf_info_buffer, 4);
            AppendGListLong(dwarf_info_buffer, location->operand.indirect.value);
            AppendGListByte(dwarf_info_buffer, 7);
            expressionSize += 11;
            if (shouldDereference != 0) {
                AppendGListByte(dwarf_info_buffer, 6);
                expressionSize += 1;
            }
            break;
        case 11:
            AppendGListByte(dwarf_info_buffer, 4);
            AppendGListLong(dwarf_info_buffer, location->metadata.value);
            AppendGListByte(dwarf_info_buffer, 7);
            AppendGListByte(dwarf_info_buffer, 6);
            AppendGListByte(dwarf_info_buffer, 4);
            AppendGListLong(dwarf_info_buffer, location->operand.value);
            AppendGListByte(dwarf_info_buffer, 7);
            AppendGListByte(dwarf_info_buffer, 6);
            expressionSize += 14;
            break;
        case 6:
            AppendGListByte(dwarf_info_buffer, 3);
            BE_elf_AddRelocation(dwarf_section, dwarf_info_buffer->size, location->operand.pair.first, NULL,
                                 location->operand.pair.second, currentDwarfFunctionState->offset);
            AppendGListLong(dwarf_info_buffer, dwarf_zero_addend_mode() ? 0 : location->operand.pair.second);
            expressionSize += 5;
            break;
        case 9:
            AppendGListByte(dwarf_info_buffer, 4);
            AppendGListLong(dwarf_info_buffer, location->operand.value);
            AppendGListByte(dwarf_info_buffer, 7);
            expressionSize += 6;
            break;
        case 10:
            AppendGListByte(dwarf_info_buffer, 2);
            AppendGListLong(dwarf_info_buffer, location->operand.registers.first);
            expressionSize += 5;
            break;
        default:
            CError_Internal(CERROR_FILE, 1011);
            break;
    }

    *(UInt16 *)(*dwarf_info_buffer->data + expressionSizeOffset) = expressionSize;
    attributeSize += expressionSize;
    return attributeSize;
}

DwarfRef get_type_dwarf_ref(DWInfo *info, UInt16 a, Boolean b)
{
    Type *t;

    if (info->rec.valid != 0) {
        return info->rec;
    }
    t = info->type;
    info->rec.qualifiers = NULL;
    info->rec.num = 0;
    info->rec.typeInfo = NULL;
    if (t->type != TYPEENUM && (UInt8)(t->type - 1) <= 2) {
        info->rec.tag = 0x55;
        info->rec.num = get_integral_type_code(t);
        info->rec.valid = 1;
    } else if (t->type == TYPEVOID) {
        info->rec.tag = 0x55;
        info->rec.num = 0x14;
        info->rec.valid = 1;
    } else {
        set_type_dwarf_ref(t, a, b, &info->rec);
    }
    return info->rec;
}

struct DWInfo *find_or_create_dwinfo(struct Type *type)
{
    unsigned short low, high, middleLow, middleHigh;
    int hash;
    int bucketIndex;
    DWInfo *record;
    DWInfo *bucket;
    DWInfo *newRecord;

    low = (UInt32)type & 511;
    high = ((UInt32)type >> 23) & 511;
    middleLow = ((UInt32)type >> 9) & 127;
    middleHigh = ((UInt32)type >> 16) & 127;
    bucketIndex = hash = low ^ high ^ (middleLow ^ middleHigh);
    record = bucket = dwinfo_buckets[bucketIndex];
    if (bucket) {
        do {
            if (record->type == type)
                return record;
            record = record->next;
        } while (record);
    }
    newRecord = galloc(sizeof(DWInfo));
    memclrw(newRecord, sizeof(DWInfo));
    newRecord->next = dwinfo_buckets[hash];
    dwinfo_buckets[hash] = newRecord;
    newRecord->type = type;
    return newRecord;
}

void set_type_dwarf_ref(Type *p, UInt16 x, Boolean flag, DwarfRef *out)
{
    switch ((SInt8)p->type) {
        case TYPEENUM:
            out->tag = 0x72;
            if (!flag) {
                out->num = dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                out->valid = 1;
                {
                    emit_enum_type(p);
                }
            }
            return;
        case TYPESTRUCT:
            out->tag = 0x72;
            if (!flag) {
                out->num = dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                out->valid = 1;
                {
                    TypeStruct *structure = (TypeStruct *)p;
                    DWARF_004b0a80(structure);
                }
            }
            return;
        case TYPEPOINTER:
            *out = create_type_dwarf_ref(p, x, flag);
            return;
        case TYPEARRAY:
            out->tag = 0x72;
            if (!flag) {
                out->num = dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                out->valid = 1;
                emit_array_type(p);
            }
            return;
        case TYPECLASS:
            out->tag = 0x72;
            if (!flag) {
                out->num = dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                out->valid = 1;
                {
                    TypeClass *classType = (TypeClass *)p;
                    emit_class_dwarf(classType);
                }
            }
            return;
        case TYPEFUNC:
            out->tag = 0x72;
            if (!flag) {
                out->num = dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                out->valid = 1;
            }
            return;
        case TYPEBITFIELD:
            out->tag = 0x55;
            out->num = 0x14;
            out->valid = 1;
            return;
        case TYPEMEMBERPOINTER:
            out->tag = 0x72;
            if (!flag) {
                out->num = dwarf_info_buffer->size + currentDwarfFunctionState->offset;
                out->valid = 1;
            }
            return;
        default:
            CError_FATAL(840);
    }
}

#pragma opt_propagation off

DwarfRef create_type_dwarf_ref(Type *type, UInt16 qual, Boolean isArgument)
{
    UInt8 pointerKind;
    UInt8 qualifierKind;
    UInt8 typeKind;
    DwarfRef result;
    DwarfRef typeRef;
    QualNode *tail;
    QualNode *head;
    UInt16 pointerQual;

    head = (QualNode *)galloc(6);
    result.qualifiers = tail = head;
    head->next = NULL;
    result.valid = 0;

    while (type->type == TYPEPOINTER) {
        pointerQual = TYPE_POINTER(type)->qual;
        if ((UInt16)TYPE_POINTER(type)->qual & Q_REFERENCE) {
            pointerQual -= Q_REFERENCE;
            pointerKind = 2;
        } else {
            pointerKind = 1;
        }
        while ((qualifierKind = NextQual(&pointerQual)) != 0) {
            tail->type = qualifierKind;
            tail->next = (QualNode *)galloc(6);
            tail = tail->next;
        }
        tail->type = pointerKind;
        type = TYPE_POINTER(type)->target;
        if (type->type == TYPEPOINTER) {
            tail->next = (QualNode *)galloc(6);
            tail = tail->next;
        }
    }
    tail->next = NULL;

    typeKind = type->type;
    if (typeKind != TYPEENUM && (UInt8)(typeKind - TYPEINT) <= 2) {
        result.tag = 0x63;
        while ((qualifierKind = NextQual(&qual)) != 0) {
            tail->next = (QualNode *)galloc(6);
            tail = tail->next;
            tail->type = qualifierKind;
            tail->next = NULL;
        }
        result.num = get_integral_type_code(type);
        return result;
    }
    if (typeKind == TYPEVOID) {
        result.tag = 0x55;
        result.num = 0xd;
        return result;
    }
    result.tag = 0x83;
    while ((qualifierKind = NextQual(&qual)) != 0) {
        tail->next = (QualNode *)galloc(6);
        tail = tail->next;
        tail->type = qualifierKind;
        tail->next = NULL;
    }
    {
        DWInfo *info = find_or_create_dwinfo(type);
        typeRef = get_type_dwarf_ref(info, 0, isArgument);
        result.typeInfo = info;
        result.num = typeRef.num;
    }
    return result;
}

#pragma opt_propagation reset

UInt16 get_integral_type_code(Type *type)
{
    switch (((TypeIntegral *)type)->integral) {
        case IT_BOOL:
            return 3U;
        case IT_CHAR:
            return 1U;
        case IT_SCHAR:
            return 2U;
        case IT_UCHAR:
            return 3U;
        case IT_SHORT:
            return 5U;
        case IT_USHORT:
            return 6U;
        case IT_INT:
            return 7U;
        case IT_UINT:
            return 9U;
        case IT_LONG:
            return 10U;
        case IT_ULONG:
            return 12U;
        case IT_LONGLONG:
            return 32776U;
        case IT_ULONGLONG:
            return 33288U;
        case IT_FLOAT:
            return 14U;
        case IT_SHORTDOUBLE:
            return 15U;
        case IT_DOUBLE:
            return 15U;
        case IT_LONGDOUBLE:
            return 15U;
        case IT_WCHAR_T:
        default:
            return 0U;
    }
}

void emit_array_type(Type *type)
{
    SInt32 entryOffset;
    DWInfo *elementInfo;
    DwarfFixup *fixup;
    DWInfo *arrayInfo;
    SInt16 attributeSize;
    SInt32 blockLengthOffset;
    SInt32 upperBound;
    SInt32 entrySize;

    entryOffset = dwarf_info_buffer->size;
    elementInfo = find_or_create_dwinfo(TPTR_TARGET(type));
    arrayInfo = find_or_create_dwinfo(type);

    if (arrayInfo->typeNode == NULL)
        CError_FATAL(629);

    fixup = arrayInfo->typeNode->u.fixups;
    while (fixup != NULL) {
        if (copts.fd5 != 0)
            BE_elf_SetRelocationValue(fixup->reference, entryOffset);
        else
            *(SInt32 *)(*dwarf_info_buffer->data + fixup->offset) = entryOffset;
        fixup = fixup->next;
    }
    arrayInfo->typeNode->u.fixups = fixup;

    get_type_dwarf_ref(elementInfo, 0, 1);
    entrySize = emit_entry_header(1);
    AppendGListWord(dwarf_info_buffer, 0xa3);
    entrySize += 2;
    blockLengthOffset = dwarf_info_buffer->size;
    AppendGListWord(dwarf_info_buffer, 0);
    attributeSize = 2;
    AppendGListByte(dwarf_info_buffer, 0);
    AppendGListWord(dwarf_info_buffer, 10);
    AppendGListLong(dwarf_info_buffer, 0);

    if (TPTR_TARGET(type)->size == 0) {
        upperBound = 0;
    } else if (type->size == TPTR_TARGET(type)->size) {
        upperBound = 0;
    } else {
        upperBound = type->size / TPTR_TARGET(type)->size - 1;
    }
    AppendGListLong(dwarf_info_buffer, upperBound);
    AppendGListByte(dwarf_info_buffer, 8);

    attributeSize += 12;
    attributeSize += emit_dwarf_ref(elementInfo);

    *(UInt16 *)(*dwarf_info_buffer->data + blockLengthOffset) = attributeSize - 2;
    entrySize += attributeSize;
    *(SInt32 *)(*dwarf_info_buffer->data + entryOffset) = entrySize;
}

static SInt32 EmitName(char *s)
{
    SInt32 i = 0;
    for (; s[i] != 0; i++)
        AppendGListByte(dwarf_info_buffer, s[i]);
    AppendGListByte(dwarf_info_buffer, 0);
    return i + 1;
}

static inline void PatchDwarfLength(SInt32 offset, SInt32 length)
{
    *(SInt32 *)(*dwarf_info_buffer->data + offset) = length;
}

static inline void patch_dwarf_sibling(void)
{
    if (copts.fd5 != 0)
        BE_elf_SetRelocationValue(data_005604a4[*dwarf_depth_ptr],
                                  dwarf_info_buffer->size + currentDwarfFunctionState->offset);
    else
        *(SInt32 *)(*dwarf_info_buffer->data + dwarf_entry_offsets[*dwarf_depth_ptr]) =
            dwarf_info_buffer->size + currentDwarfFunctionState->offset;
}

void emit_class_dwarf(TypeClass *cls)
{
    SInt32 recordPos;
    SInt32 memberPos;
    ObjMemberVar *member;
    DWInfo *typeInfo;
    DwarfFixup *fixup;
    char *name;
    SInt32 length;
    SInt32 tag;
    SInt32 classMode;
    SInt8 mode;
    SInt32 memberLength;
    char *memberName;
    DwarfLocationOperand location;
    ScopeSearch search;
    Boolean hasChildren;
    Object *entry;
    hasChildren = 0;
    recordPos = dwarf_info_buffer->size;
    typeInfo = find_or_create_dwinfo((Type *)cls);
    if (typeInfo->typeNode == NULL)
        CError_FATAL(408);
    for (fixup = typeInfo->typeNode->u.fixups; fixup != NULL; fixup = fixup->next) {
        if (copts.fd5 != 0)
            BE_elf_SetRelocationValue(fixup->reference, recordPos);
        else
            PatchDwarfLength(fixup->offset, recordPos);
    }
    typeInfo->typeNode->u.fixups = fixup;
    mode = cls->mode;
    classMode = mode;
    if (classMode == 2)
        tag = 2;
    else if (classMode == 0)
        tag = 0x13;
    else if (classMode == 1)
        tag = 0x17;
    else
        CError_FATAL(418);
    length = emit_entry_header(tag);
    if (cls->classname != NULL) {
        AppendGListWord(dwarf_info_buffer, 0x38);
        length += 2;
        name = cls->classname->name;
        length += EmitName(name);
    }
    AppendGListWord(dwarf_info_buffer, 0xb6);
    AppendGListLong(dwarf_info_buffer, cls->size);
    length += 6;
    PatchDwarfLength(recordPos, length);
    CError_ASSERT(430, ++*dwarf_depth_ptr < 0x32);
    dwarf_entry_offsets[*dwarf_depth_ptr] = 0;
    if (cls->bases != NULL) {
        ClassList *base;
        for (base = cls->bases; base != NULL; base = base->next) {
            DWInfo *baseType;
            hasChildren = 1;
            memberPos = dwarf_info_buffer->size;
            memberLength = emit_entry_header(0x1c);
            AppendGListWord(dwarf_info_buffer, 0x38);
            memberLength += 2;
            memberName = base->base->classname->name;
            memberLength += EmitName(memberName);
            baseType = find_or_create_dwinfo((Type *)base->base);
            get_type_dwarf_ref(baseType, 0, 1);
            memberLength += emit_dwarf_ref(baseType);
            switch (base->access) {
                case ACCESSPUBLIC:
                    AppendGListWord(dwarf_info_buffer, 0x288);
                    memberLength += 2;
                    memberLength += EmitName("");
                    break;
                case ACCESSPRIVATE:
                    AppendGListWord(dwarf_info_buffer, 0x248);
                    memberLength += 2;
                    memberLength += EmitName("");
                    break;
                case ACCESSPROTECTED:
                    AppendGListWord(dwarf_info_buffer, 0x268);
                    memberLength += 2;
                    memberLength += EmitName("");
                    break;
            }
            if (base->is_virtual != 0) {
                AppendGListWord(dwarf_info_buffer, 0x308);
                memberLength += 2;
                memberLength += EmitName("");
            }
            location.kind = 9;
            location.operand.value = base->offset;
            memberLength += emit_location_attribute(&location, 0x23, 0);
            PatchDwarfLength(memberPos, memberLength);
        }
    }
    for (member = cls->ivars; member != NULL; member = member->next) {
        TypeBitfield *bitfield;
        hasChildren = 1;
        if (member->type->type == TYPEBITFIELD) {
            bitfield = (TypeBitfield *)member->type;
            typeInfo = find_or_create_dwinfo(bitfield->bitfieldtype);
        } else {
            typeInfo = find_or_create_dwinfo(member->type);
            bitfield = NULL;
        }
        get_type_dwarf_ref(typeInfo, member->qual, 1);
        memberPos = dwarf_info_buffer->size;
        memberLength = emit_entry_header(0xd);
        AppendGListWord(dwarf_info_buffer, 0x38);
        memberLength += 2;
        memberName = member->name->name;
        memberLength += EmitName(memberName);
        memberLength += emit_dwarf_ref(typeInfo);
        switch (member->access) {
            case ACCESSPUBLIC:
                AppendGListWord(dwarf_info_buffer, 0x288);
                memberLength += 2;
                memberLength += EmitName("");
                break;
            case ACCESSPRIVATE:
                AppendGListWord(dwarf_info_buffer, 0x248);
                memberLength += 2;
                memberLength += EmitName("");
                break;
            case ACCESSPROTECTED:
                AppendGListWord(dwarf_info_buffer, 0x268);
                memberLength += 2;
                memberLength += EmitName("");
                break;
        }
        if (bitfield != NULL) {
            AppendGListWord(dwarf_info_buffer, 0xc5);
            AppendGListWord(dwarf_info_buffer, bitfield->offset);
            AppendGListWord(dwarf_info_buffer, 0xd6);
            AppendGListLong(dwarf_info_buffer, bitfield->bitlength);
            memberLength += 0xa;
        }
        location.kind = 9;
        location.operand.value = member->offset;
        memberLength += emit_location_attribute(&location, 0x23, 0);
        PatchDwarfLength(memberPos, memberLength);
    }
    CScope_InitScopeSearch(&search, cls->nspace);
    for (;;) {
        if (!(entry = CScope_NextObject(&search)))
            break;
        if (entry->datatype == DFUNC || entry->datatype == DVFUNC) {
            if (entry->type->type == TYPEFUNC && (TYPE_FUNC(entry->type)->flags & 0x400) != 0) {
                if (entry->u.templateFunction->instances == NULL)
                    continue;
                typeInfo = find_or_create_dwinfo(entry->u.templateFunction->instances->object->type);
            } else {
                typeInfo = find_or_create_dwinfo(entry->type);
            }
            hasChildren = 1;
            memberPos = dwarf_info_buffer->size;
            memberLength = emit_entry_header(6);
            AppendGListWord(dwarf_info_buffer, 0x38);
            memberLength += 2;
            memberLength += EmitName(entry->name->name);
            name = COptimizer_GetFunctionObject(entry)->name;
            if (entry->name->name != name) {
                AppendGListWord(dwarf_info_buffer, 0x2008);
                memberLength += 2;
                memberLength += EmitName(name);
            }
            switch (entry->access) {
                case ACCESSPUBLIC:
                    AppendGListWord(dwarf_info_buffer, 0x288);
                    memberLength += 2;
                    memberLength += EmitName("");
                    break;
                case ACCESSPRIVATE:
                    AppendGListWord(dwarf_info_buffer, 0x248);
                    memberLength += 2;
                    memberLength += EmitName("");
                    break;
                case ACCESSPROTECTED:
                    AppendGListWord(dwarf_info_buffer, 0x268);
                    memberLength += 2;
                    memberLength += EmitName("");
                    break;
            }
            if (entry->datatype == DVFUNC) {
                location.kind = 0xb;
                location.operand.value = ((TypeMemberFunc *)entry->type)->vtbl_index;
                location.metadata.value = cls->vtable->offset;
                memberLength += emit_location_attribute(&location, (cls->flags & CLASS_ABSTRACT) ? 0x293 : 0x303, 0);
            }
            get_type_dwarf_ref(typeInfo, entry->qual, 1);
            memberLength += emit_dwarf_ref(typeInfo);
            PatchDwarfLength(memberPos, memberLength);
        } else if (entry->datatype == DDATA) {
            hasChildren = 1;
            memberPos = dwarf_info_buffer->size;
            memberLength = emit_entry_header(0x16);
            AppendGListWord(dwarf_info_buffer, 0x38);
            memberLength += 2;
            memberName = entry->name->name;
            memberLength += EmitName(memberName);
            typeInfo = find_or_create_dwinfo(entry->type);
            get_type_dwarf_ref(typeInfo, entry->qual, 1);
            memberLength += emit_dwarf_ref(typeInfo);
            PatchDwarfLength(memberPos, memberLength);
        }
    }
    if (copts.fd4 != 0 && hasChildren) {
        patch_dwarf_sibling();
        AppendGListLong(dwarf_info_buffer, 4);
    }
    --*dwarf_depth_ptr;
    if (!hasChildren)
        patch_dwarf_sibling();
}

void DWARF_004b0a80(TypeStruct *type)
{
    SInt32 entryOffset;
    SInt32 length;
    SInt32 nameLength;
    SInt32 tag;
    TypeBitfield *bitfield;
    DWInfo *memberInfo;
    StructMember *member;
    DWInfo *info;
    Type *memberType;
    DwarfFixup *fixup, *fixupHead;
    Boolean hasMembers;
    DwarfLocationOperand location;
    TypeBitfield reversedBitfield;
    char *name;
    GList *buffer = dwarf_info_buffer;
    hasMembers = 0;
    entryOffset = buffer->size;
    info = find_or_create_dwinfo((Type *)type);
    CError_ASSERT(289, info->typeNode != NULL);
    fixup = fixupHead = info->typeNode->u.fixups;
    if (fixupHead != NULL)
        do {
            if (copts.fd5)
                BE_elf_SetRelocationValue(fixup->reference, entryOffset);
            else
                *(SInt32 *)(*dwarf_info_buffer->data + fixup->offset) = entryOffset;
            fixup = fixup->next;
        } while (fixup != NULL);
    info->typeNode->u.fixups = fixup;
    switch (type->stype) {
        case 0:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            tag = 0x13;
            break;
        case 1:
            tag = 0x17;
            break;
        default:
            CError_FATAL(314);
    }
    length = emit_entry_header(tag);
    if (type->name != NULL) {
        AppendGListWord(dwarf_info_buffer, 0x38);
        length += 2;
        name = type->name->name;
        for (nameLength = 0; name[nameLength] != 0; nameLength++)
            AppendGListByte(dwarf_info_buffer, name[nameLength]);
        AppendGListByte(dwarf_info_buffer, 0);
        length += nameLength + 1;
    }
    AppendGListWord(dwarf_info_buffer, 0xb6);
    AppendGListLong(dwarf_info_buffer, type->size);
    *(SInt32 *)(*dwarf_info_buffer->data + entryOffset) = 6 + length;
    if (++*dwarf_depth_ptr >= 0x32)
        CError_FATAL(325);
    dwarf_entry_offsets[*dwarf_depth_ptr] = 0;
    member = type->members;
    if (member != NULL)
        do {
            hasMembers = 1;
            if ((memberType = member->type)->type == TYPEBITFIELD) {
                bitfield = (TypeBitfield *)memberType;
                memberInfo = find_or_create_dwinfo(bitfield->bitfieldtype);
                if (copts.nativeByteOrder) {
                    reversedBitfield = *bitfield;
                    bitfield = &reversedBitfield;
                    CABI_ReverseBitField(&reversedBitfield);
                }
            } else {
                memberInfo = find_or_create_dwinfo(memberType);
                bitfield = NULL;
            }
            get_type_dwarf_ref(memberInfo, member->qual, 1);
            entryOffset = dwarf_info_buffer->size;
            length = emit_entry_header(0xd);
            AppendGListWord(dwarf_info_buffer, 0x38);
            length += 2;
            name = member->name->name;
            for (nameLength = 0; name[nameLength] != 0; nameLength++)
                AppendGListByte(dwarf_info_buffer, name[nameLength]);
            AppendGListByte(dwarf_info_buffer, 0);
            length += nameLength + 1;
            length += emit_dwarf_ref(memberInfo);
            if (bitfield != NULL) {
                AppendGListWord(dwarf_info_buffer, 0xc5);
                AppendGListWord(dwarf_info_buffer, bitfield->offset);
                AppendGListWord(dwarf_info_buffer, 0xd6);
                AppendGListLong(dwarf_info_buffer, bitfield->bitlength);
                AppendGListWord(dwarf_info_buffer, 0xb6);
                AppendGListLong(dwarf_info_buffer, bitfield->bitfieldtype->size);
                length += 0x10;
            }
            location.kind = 9;
            location.operand.value = member->offset;
            length += emit_location_attribute(&location, 0x23, 0);
            *(SInt32 *)(*dwarf_info_buffer->data + entryOffset) = length;
        } while ((member = member->next) != NULL);
    if (copts.fd4 && hasMembers) {
        if (copts.fd5)
            BE_elf_SetRelocationValue(data_005604a4[*dwarf_depth_ptr],
                                      dwarf_info_buffer->size + currentDwarfFunctionState->offset);
        else
            *(SInt32 *)(*dwarf_info_buffer->data + dwarf_entry_offsets[*dwarf_depth_ptr]) =
                dwarf_info_buffer->size + currentDwarfFunctionState->offset;
        AppendGListLong(dwarf_info_buffer, 4);
    }
    --*dwarf_depth_ptr;
    if (!hasMembers) {
        if (copts.fd5)
            BE_elf_SetRelocationValue(data_005604a4[*dwarf_depth_ptr],
                                      dwarf_info_buffer->size + currentDwarfFunctionState->offset);
        else
            *(SInt32 *)(*dwarf_info_buffer->data + dwarf_entry_offsets[*dwarf_depth_ptr]) =
                dwarf_info_buffer->size + currentDwarfFunctionState->offset;
    }
}

static SInt32 DW_String(char *name)
{
    SInt32 i;
    char c;
    char *str = name;

    i = 0;
    while ((c = str[i]) != 0) {
        AppendGListByte(dwarf_info_buffer, c);
        i++;
    }
    AppendGListByte(dwarf_info_buffer, 0);
    return i + 1;
}

static inline void patch_dwarf_info_length(SInt32 offset, SInt32 length)
{
    *(SInt32 *)(*dwarf_info_buffer->data + offset) = length;
}

void emit_enum_type(Type *type)
{
    SInt32 constantsSize;
    SInt32 entrySize;
    SInt32 startOffset;
    SInt32 constantsOffset;
    DWInfo *entry;
    DwarfFixup *fixup;
    ObjEnumConst *constant;

    startOffset = dwarf_info_buffer->size;
    entry = find_or_create_dwinfo(type);
    CError_ASSERT(246, entry->typeNode != NULL);
    fixup = entry->typeNode->u.fixups;
    if (fixup != NULL) {
        do {
            if (copts.fd5) {
                BE_elf_SetRelocationValue(fixup->reference, startOffset);
            } else {
                patch_dwarf_info_length(fixup->offset, startOffset);
            }
            fixup = fixup->next;
        } while (fixup != NULL);
    }
    entry->typeNode->u.fixups = fixup;
    entrySize = emit_entry_header(4);
    if (TYPE_ENUM(type)->enumname != NULL) {
        AppendGListWord(dwarf_info_buffer, 0x38);
        entrySize += 2;
        entrySize += DW_String(TYPE_ENUM(type)->enumname->name);
    }
    AppendGListWord(dwarf_info_buffer, 0xb6);
    AppendGListLong(dwarf_info_buffer, type->size);
    AppendGListWord(dwarf_info_buffer, 0xf4);
    constantsOffset = dwarf_info_buffer->size;
    constantsSize = 0;
    AppendGListLong(dwarf_info_buffer, 0);
    entrySize += 0xc;
    constant = TYPE_ENUM(type)->enumlist;
    if (constant != NULL) {
        do {
            SInt32 nameSize;
            AppendGListLong(dwarf_info_buffer, constant->val.lo);
            constantsSize += 4;
            entrySize += 4;
            nameSize = DW_String(constant->name->name);
            constantsSize += nameSize;
            constant = constant->next;
            entrySize += nameSize;
        } while (constant != NULL);
    }
    patch_dwarf_info_length(constantsOffset, constantsSize);
    patch_dwarf_info_length(startOffset, entrySize);
}
