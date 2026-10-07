#define CERROR_FILE "SFPE_PPC_EABI.c"
#include "compiler/common.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CMangler.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CompilerTools.h"
#include "compiler/DWARF.h"
#include "compiler/ELF_Endian.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopOptimization.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/PPCError.h"
#include "compiler/Registers.h"
#include "compiler/SFPE_PPC_EABI.h"
#include "compiler/StackFrameEABI.h"
#include "compiler/Switch.h"
#include "driver/Files.h"

static int assembly_buffer_offset;
typedef void (*ExpressionGenerator)(ENode *, short, short, Operand *);
static PCodeInstruction *first_instr(struct PCodeLabel *q)
{
    PCodeBlock *n;
    for (n = q->target.block; n->instruction_count == 0; n = n->next)
        ;
    return n->instructions;
}

static inline int PCodeAssembly_ShouldEmitExtraData(void)
{
    return copts.emitExtraAssemblyData != 0;
}

static inline int PCodeAssembly_ShouldOptimizeBranches(void)
{
    return copts.peepholeOptimizationEnabled != 0;
}

static inline int PCodeAssembly_ShouldEmitSerializedFormat(void)
{
    return copts.emitSerializedAssemblyFormat != 0;
}

static inline int PCodeAssembly_ShouldEmitDebugInfo(void)
{
    return copts.filesyminfo != 0;
}

int PCodeAssembly_EmitFunction(Object *object, struct PCodeAssemblyEntry *symbolEntries)
{
    PCodeBlock *block;
    PCodeInstruction *instruction;
    GList *buffer;
    ObjGenSection *output;
    int size;
    int extraDataSize;
    UInt8 *extraData;
    WeirdOperand instructionRelocation;
    SInt32 extraSize;
    int offset;
    struct PCodeAssemblyEntry *entry;

    size = PCode_SetCodeOffsets();
    if (size <= 0) {
        PPCError_ReportError(177, object->name->name);
    }
    if (PCodeAssembly_ShouldOptimizeBranches()) {
        size = optimize_branches(size);
    }
    if (size > 32766) {
        expand_out_of_range_conditional_branches();
        size = PCode_SetCodeOffsets();
    }
    if (PCodeAssembly_ShouldEmitDebugInfo()) {
        ObjGen_PPC_EABI_SetObjectSectionIndex(object);
    }
    extraSize = 0;
    if (PCodeAssembly_ShouldEmitExtraData()) {
        extraData = StackFrameEABI_004aabb0(size, COptimizer_GetFunctionObject(object)->name, &extraSize, object);
    }
    if (object->section == 0) {
        object->section = 1;
    }
    extraDataSize = extraSize;
    output = fn_004892a0(object, size + extraSize);
    buffer = ObjGen_PPC_EABI_GetSectionBuffer(output);
    assembly_buffer_offset = buffer->size;
    AppendGListNoData(buffer, size + extraDataSize);
    if (PCodeAssembly_ShouldEmitDebugInfo()) {
        DWARF_CreateBlockNode(object, size + extraSize, assembly_buffer_offset, output);
        ObjGen_PPC_EABI_00488ee0(function_tokenoffset, 0);
    }
    ObjGen_PPC_EABI_ClearSectionSymbolLinkValues();
    if (symbolEntries != NULL) {
        entry = symbolEntries;
        while (entry != NULL) {
            ObjGen_PPC_EABI_SetSymbolOffset(entry->object, entry->block->code_offset);
            entry = entry->next;
        }
    }
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        if (PCodeAssembly_ShouldEmitDebugInfo() && block->line != -1L) {
            ObjGen_PPC_EABI_00488ee0(block->line, block->code_offset);
        }
        instruction = block->instructions;
        offset = block->code_offset;
        while (instruction != NULL) {
            *(int *)(*buffer->data + assembly_buffer_offset + offset) =
                encode_assembly_instruction(instruction, offset, &instructionRelocation);
            if ((instruction->flags & 0x01000000) != 0) {
                ObjGen_PPC_EABI_AppendOutputEntry(output, offset);
            }
            if (instructionRelocation.type != -1) {
                fn_004889b0(output, offset, instructionRelocation.object, instructionRelocation.type,
                            instructionRelocation.addend);
            }
            instruction = instruction->next;
            offset += 4;
        }
    }
    if (PCodeAssembly_ShouldEmitSerializedFormat()) {
        ObjGen_PPC_EABI_EmitSerializedFormat(object, size);
    }
    if (PCodeAssembly_ShouldEmitExtraData()) {
        memcpy(*buffer->data + assembly_buffer_offset + size, extraData, extraDataSize);
    }
    if (PCodeAssembly_ShouldEmitDebugInfo()) {
        ObjGen_PPC_EABI_RestoreFunctionState();
    }
    if (PCodeAssembly_ShouldEmitExtraData()) {
        size += extraSize;
        return size;
    }
    return size;
}

int optimize_branches(int arg)
{
    long prev;
    int removed, changed;
    PCodeBlock *n;
    struct PCodeLabel *q, *qq;
    PCodeBlock *blk;
    PCodeInstruction **pp;
    PCodeInstruction *fd;
    SInt16 k, op;

    do {
        changed = removed = 0;
        for (blk = gPCodeBlocks; blk; blk = blk->next) {
            PCodeInstruction *p;
            if (blk->instruction_count == 0)
                continue;
            if (((p = blk->reverse_instructions)->flags & fIsBranch) == 0)
                continue;
            pp = (PCodeInstruction **)blk->code_offset + (blk->instruction_count - 1);
            if ((op = p->opcode) == 0 && p->operandData.operands[0].kind == PCOp_LABEL) {
                fd = first_instr(q = p->operandData.operands[0].value.label);
                if ((PCodeInstruction **)q->target.block->code_offset == pp + 1) {
                    PCode_UnlinkInstruction(p);
                    changed = removed = 1;
                } else {
                    if (fd->opcode == PC_B) {
                        if (fd->operandData.operands[0].kind == PCOp_LABEL &&
                            fd->operandData.operands[0].value.label != p->operandData.operands[0].value.label) {
                            p->operandData.operands[0].value.label = fd->operandData.operands[0].value.label;
                            changed = 1;
                        }
                    } else if (fd->opcode == PC_BLR) {
                        p->opcode = PC_BLR;
                        changed = 1;
                    }
                }
            } else if ((op == 5 || op == 8) && p->operandData.operands[2].kind == PCOp_LABEL) {
                PCodeBlock *n;
                struct PCodeLabel *q;
                PCodeBlock *ref = p->block;
                fd = first_instr(qq = q = p->operandData.operands[2].value.label);
                if ((PCodeInstruction **)qq->target.block->code_offset == (pp + 1)) {
                    PCode_UnlinkInstruction(p);
                    changed = removed = 1;
                } else {
                    if ((k = fd->opcode) == 0) {
                        if (fd->operandData.operands[0].kind == PCOp_LABEL &&
                            fd->operandData.operands[0].value.label != q) {
                            p->operandData.operands[2].value.label = fd->operandData.operands[0].value.label;
                            changed = 1;
                        }
                    } else if (k == 0x11) {
                        if (op == 5)
                            p->opcode = PC_BTLR;
                        else
                            p->opcode = PC_BFLR;
                        p->operand_count = 2;
                        changed = 1;
                    } else if (k == 0x12) {
                        if (op == 5)
                            p->opcode = PC_BTCTR;
                        else
                            p->opcode = PC_BFCTR;
                        p->operand_count = 2;
                        changed = 1;
                    } else {
                        PCodeBlockLink *r;
                        PCodeBlock *b;
                        if ((b = ref->next) && (fd = b->instructions) != NULL && fd->opcode == PC_BLR &&
                            (PCodeInstruction **)qq->target.block->code_offset == (pp + 2)) {
                            PCodeBlockLink *r;
                            r = b->predecessors;
                            if (r != NULL && r->payload.block == ref && r->next == NULL) {
                                if (op == 5)
                                    p->opcode = PC_BFLR;
                                else
                                    p->opcode = PC_BTLR;
                                p->operand_count = 2;
                                PCode_UnlinkInstruction(ref->next->instructions);
                                changed = removed = 1;
                            }
                        } else if (b && (fd = b->instructions) != NULL && fd->opcode == PC_BCTR &&
                                   (PCodeInstruction **)qq->target.block->code_offset == (pp + 2)) {
                            PCodeBlockLink *r;
                            r = b->predecessors;
                            if (r != NULL && r->payload.block == ref && r->next == NULL) {
                                if (op == 5)
                                    p->opcode = PC_BFCTR;
                                else
                                    p->opcode = PC_BTCTR;
                                p->operand_count = 2;
                                PCode_UnlinkInstruction(ref->next->instructions);
                                changed = removed = 1;
                            }
                        }
                    }
                }
            } else if (op == 2 && p->operandData.operands[3].kind == PCOp_LABEL &&
                       (p->flags & (fSideEffects | fLink)) == 0) {
                PCodeBlock *n;
                struct PCodeLabel *q;
                PCodeBlock *ref = p->block;
                fd = first_instr(qq = q = p->operandData.operands[3].value.label);
                if ((PCodeInstruction **)qq->target.block->code_offset == (pp + 1)) {
                    PCode_UnlinkInstruction(p);
                    changed = removed = 1;
                } else {
                    if ((k = fd->opcode) == 0) {
                        if (fd->operandData.operands[0].kind == PCOp_LABEL &&
                            fd->operandData.operands[0].value.label != q) {
                            p->operandData.operands[3].value.label = fd->operandData.operands[0].value.label;
                            changed = 1;
                        }
                    } else if (k == 0x11) {
                        p->opcode = PC_BCLR;
                        p->operand_count = 3;
                        changed = 1;
                    } else if (k == 0x12) {
                        p->opcode = PC_BCCTR;
                        p->operand_count = 3;
                        changed = 1;
                    } else {
                        UInt32 x;
                        PCodeBlock *b;
                        PCodeBlockLink *r;
                        if ((b = ref->next) && (fd = b->instructions) != NULL && fd->opcode == PC_BLR &&
                            (PCodeInstruction **)qq->target.block->code_offset == (pp + 2)) {
                            PCodeBlockLink *r;
                            x = p->operandData.operands[0].value.unsigned_value & 0x1e;
                            r = b->predecessors;
                            if (r != NULL && r->payload.block == ref && r->next == NULL) {
                                if ((x & 0x1e) == 4)
                                    p->operandData.operands[0].value.unsigned_value = x | 0xc;
                                else if ((x & 0x1e) == 0xc)
                                    p->operandData.operands[0].value.unsigned_value = x & 0x17;
                                p->opcode = PC_BCLR;
                                p->operand_count = 3;
                                PCode_UnlinkInstruction(ref->next->instructions);
                                changed = removed = 1;
                            }
                        } else if (b && (fd = b->instructions) != NULL && fd->opcode == PC_BCTR &&
                                   (PCodeInstruction **)qq->target.block->code_offset == (pp + 2)) {
                            PCodeBlockLink *r;
                            x = p->operandData.operands[0].value.unsigned_value & 0x1e;
                            r = b->predecessors;
                            if (r && r->payload.block == ref && r->next == NULL) {
                                if ((x & 0x1e) == 4)
                                    p->operandData.operands[0].value.unsigned_value = x | 0xc;
                                else if ((x & 0x1e) == 0xc)
                                    p->operandData.operands[0].value.unsigned_value = x & 0x17;
                                p->opcode = PC_BCCTR;
                                p->operand_count = 3;
                                PCode_UnlinkInstruction(ref->next->instructions);
                                changed = removed = 1;
                            }
                        }
                    }
                }
            }
        }
        if (removed)
            arg = PCode_SetCodeOffsets();
        if (!changed)
            return arg;
    } while (1);
}

void expand_out_of_range_conditional_branches(void)
{
    PCodeBlock *entry;
    PCodeLabel *target;
    PCodeLabel *conditionalTarget;
    PCodeLabel *simpleTarget;
    PCodeLabel *compareTarget;
    PCodeBlock *scan;
    int offset;
    short words;
    int branchOffset;
    int displacement;

    offset = 0;
    for (scan = gPCodeBlocks; scan != NULL; scan = scan->next) {
        scan->code_offset = offset;
        if ((words = scan->instruction_count) != 0) {
            offset += words << 2;
            if (scan->reverse_instructions->opcode == PC_BT || scan->reverse_instructions->opcode == PC_BF)
                offset += 4;
        }
    }
    for (entry = gPCodeBlocks; entry != NULL; entry = entry->next) {
        if (entry->instruction_count != 0 && (entry->reverse_instructions->flags & fIsBranch) != 0) {
            switch (entry->reverse_instructions->opcode) {
                case PC_BT:
                case PC_BF:
                    branchOffset = ((entry->instruction_count - 1) << 2) + entry->code_offset;
                    if (entry->reverse_instructions->operandData.operands[2].kind != PCOp_LABEL)
                        break;
                    target = entry->reverse_instructions->operandData.operands[2].value.label;
                    if (entry->reverse_instructions->operandData.operands[2].value.label->target.block->code_offset -
                            branchOffset ==
                        (short)(entry->reverse_instructions->operandData.operands[2]
                                    .value.label->target.block->code_offset -
                                branchOffset))
                        break;
                    entry->reverse_instructions->opcode = entry->reverse_instructions->opcode == PC_BT ? 8 : 5;
                    entry->reverse_instructions->operandData.operands[2].value.label = entry->next->labels;
                    PCode_AppendInstruction(entry, PCodeUtilities_CreateInstruction(0, target));
                    break;
                case PC_BC:
                    branchOffset = ((entry->instruction_count - 1) << 2) + entry->code_offset;
                    if (entry->reverse_instructions->operandData.operands[3].kind != PCOp_LABEL)
                        break;
                    conditionalTarget = entry->reverse_instructions->operandData.operands[3].value.label;
                    if (entry->reverse_instructions->operandData.operands[3].value.label->target.block->code_offset -
                            branchOffset ==
                        (short)(entry->reverse_instructions->operandData.operands[3]
                                    .value.label->target.block->code_offset -
                                branchOffset))
                        break;
                    switch (entry->reverse_instructions->operandData.operands[0].value.signed_value & 30) {
                        case 0:
                        case 2:
                        case 8:
                        case 10:
                            entry->reverse_instructions->operandData.operands[0].value.signed_value ^= 11;
                            entry->reverse_instructions->operandData.operands[3].value.label = entry->next->labels;
                            break;
                        case 16:
                        case 18:
                            entry->reverse_instructions->operandData.operands[0].value.signed_value ^= 3;
                            entry->reverse_instructions->operandData.operands[3].value.label = entry->next->labels;
                            break;
                        case 4:
                        case 12:
                            entry->reverse_instructions->operandData.operands[0].value.signed_value ^= 9;
                            entry->reverse_instructions->operandData.operands[3].value.label = entry->next->labels;
                            break;
                        case 20:
                            PCode_UnlinkInstruction(entry->reverse_instructions);
                            break;
                        default:
                            CError_Internal("PCodeAssembly.c", 2189);
                    }
                    PCode_AppendInstruction(entry, PCodeUtilities_CreateInstruction(0, conditionalTarget));
                    break;
                case PC_BDNZ:
                case PC_BDZ:
                    branchOffset = ((entry->instruction_count - 1) << 2) + entry->code_offset;
                    if (entry->reverse_instructions->operandData.operands[0].kind != PCOp_LABEL)
                        break;
                    simpleTarget = entry->reverse_instructions->operandData.operands[0].value.label;
                    displacement =
                        entry->reverse_instructions->operandData.operands[0].value.label->target.block->code_offset -
                        branchOffset;
                    if (displacement == (short)displacement)
                        break;
                    switch (entry->reverse_instructions->opcode) {
                        case PC_BDZ:
                            entry->reverse_instructions->opcode = PC_BDNZ;
                            break;
                        case PC_BDNZ:
                            entry->reverse_instructions->opcode = PC_BDZ;
                            break;
                        default:
                            CError_Internal("PCodeAssembly.c", 2210);
                    }
                    entry->reverse_instructions->operandData.operands[0].value.label = entry->next->labels;
                    PCode_AppendInstruction(entry, PCodeUtilities_CreateInstruction(0, simpleTarget));
                    break;
                case PC_BDNZT:
                case PC_BDNZF:
                case PC_BDZT:
                case PC_BDZF:
                    branchOffset = ((entry->instruction_count - 1) << 2) + entry->code_offset;
                    if (entry->reverse_instructions->operandData.operands[2].kind == PCOp_LABEL) {
                        compareTarget = entry->reverse_instructions->operandData.operands[2].value.label;
                        if (entry->reverse_instructions->operandData.operands[2]
                                    .value.label->target.block->code_offset -
                                branchOffset !=
                            (short)(entry->reverse_instructions->operandData.operands[2]
                                        .value.label->target.block->code_offset -
                                    branchOffset)) {
                            switch (entry->reverse_instructions->opcode) {
                                case PC_BDNZT:
                                    entry->reverse_instructions->opcode = PC_BDZF;
                                    break;
                                case PC_BDNZF:
                                    entry->reverse_instructions->opcode = PC_BDZT;
                                    break;
                                case PC_BDZT:
                                    entry->reverse_instructions->opcode = PC_BDNZF;
                                    break;
                                case PC_BDZF:
                                    entry->reverse_instructions->opcode = PC_BDNZT;
                                    break;
                                default:
                                    CError_Internal("PCodeAssembly.c", 2240);
                            }
                            entry->reverse_instructions->operandData.operands[2].value.label = entry->next->labels;
                            PCode_AppendInstruction(entry, PCodeUtilities_CreateInstruction(0, compareTarget));
                        }
                    }
                    break;
                default:
                    continue;
            }
        }
    }
}

#define CError_FATAL(line) CError_Internal("PCodeAssembly.c", line)

UInt32 encode_assembly_instruction(PCodeInstruction *instr, UInt32 offset, WeirdOperand *relocation)
{
    UInt32 bits;
    SInt32 value;
    Object *object;
    Type *type;

    bits = gPCodeOpcodeDescriptors[instr->opcode].encoding;
    relocation->type = -1;
    relocation->addend = 0;

    switch (instr->opcode) {
        case PC_BL: {
            int absoluteBranch = instr->flags & fAbsolute;
            if (instr->operandData.assemblyOperands[0].kind == 5) {
                bits |= instr->operandData.assemblyOperands[0].data.mem.offset & 0x3FFFFFC;
                relocation->type = 2;
                relocation->object = instr->operandData.assemblyOperands[0].data.mem.obj;
                if (absoluteBranch == 0) {
                    relocation->type = 2;
                } else {
                    relocation->type = 10;
                    bits |= 2;
                }
            } else if (instr->operandData.assemblyOperands[0].kind == 4) {
                bits |= instr->operandData.assemblyOperands[0].data.imm.value & 0x3FFFFFC;
                if (absoluteBranch)
                    bits |= 2;
            } else {
                bits |= (instr->operandData.assemblyOperands[0].data.label.label->target.block->code_offset - offset) &
                        0x3FFFFFC;
                if (absoluteBranch)
                    CError_FATAL(119);
            }
            break;
        }

        case PC_B: {
            int absoluteBranch = instr->flags & fAbsolute;
            if (instr->operandData.assemblyOperands[0].kind == 5) {
                bits |= instr->operandData.assemblyOperands[0].data.mem.offset & 0x3FFFFFC;
                relocation->object = instr->operandData.assemblyOperands[0].data.mem.obj;
                if (absoluteBranch == 0) {
                    relocation->type = 2;
                } else {
                    relocation->type = 10;
                    bits |= 2;
                }
            } else if (instr->operandData.assemblyOperands[0].kind == 4) {
                bits |= instr->operandData.assemblyOperands[0].data.imm.value & 0x3FFFFFC;
                if (absoluteBranch)
                    bits |= 2;
            } else {
                bits |= (instr->operandData.assemblyOperands[0].data.label.label->target.block->code_offset - offset) &
                        0x3FFFFFC;
                if (absoluteBranch)
                    CError_FATAL(160);
            }
            if (instr->flags & fLink)
                bits |= 1;
            break;
        }

        case PC_BDNZ:
        case PC_BDZ: {
            int absoluteBranch = instr->flags & fAbsolute;
            if (instr->operandData.assemblyOperands[0].kind == 5) {
                bits |= instr->operandData.assemblyOperands[0].data.mem.offset & 0xFFFC;
                relocation->object = instr->operandData.assemblyOperands[0].data.mem.obj;
                if (absoluteBranch == 0) {
                    relocation->type = 8;
                } else {
                    relocation->type = 10;
                    bits |= 2;
                }
            } else {
                if (instr->operandData.assemblyOperands[0].kind == 4)
                    value = instr->operandData.assemblyOperands[0].data.imm.value;
                else
                    value = instr->operandData.assemblyOperands[0].data.label.label->target.block->code_offset - offset;
                bits |= (UInt16)value;
                if (value < 0) {
                    if (instr->flags & 0x200000)
                        bits |= 0x200000;
                } else {
                    if (instr->flags & 0x100000)
                        bits |= 0x200000;
                }
            }
            if (instr->flags & fLink)
                bits |= 1;
            break;
        }

        case PC_BC: {
            int absoluteBranch = instr->flags & fAbsolute;
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 31) << 21;
            bits |= ((instr->operandData.assemblyOperands[1].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[2].data.imm.value) &
                     31)
                    << 16;
            if (instr->operandData.assemblyOperands[3].kind == 5) {
                bits |= instr->operandData.assemblyOperands[3].data.mem.offset & 0xFFFC;
                relocation->object = instr->operandData.assemblyOperands[3].data.mem.obj;
                if (absoluteBranch == 0) {
                    relocation->type = 8;
                } else {
                    relocation->type = 10;
                    bits |= 2;
                }
            } else {
                if (instr->operandData.assemblyOperands[3].kind == 4)
                    value = instr->operandData.assemblyOperands[3].data.imm.value;
                else
                    value = instr->operandData.assemblyOperands[3].data.label.label->target.block->code_offset - offset;
                bits |= (UInt16)value;
                if (value < 0) {
                    if (instr->flags & 0x200000)
                        bits |= 0x200000;
                } else {
                    if (instr->flags & 0x100000)
                        bits |= 0x200000;
                }
            }
            if (instr->flags & fLink)
                bits |= 1;
            break;
        }

        case PC_BT:
        case PC_BF:
        case PC_BDNZT:
        case PC_BDNZF:
        case PC_BDZT:
        case PC_BDZF: {
            int absoluteBranch = instr->flags & fAbsolute;
            bits |= ((instr->operandData.assemblyOperands[0].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[1].data.imm.value) &
                     31)
                    << 16;
            if (instr->operandData.assemblyOperands[2].kind == 5) {
                bits |= instr->operandData.assemblyOperands[2].data.mem.offset & 0xFFFC;
                relocation->object = instr->operandData.assemblyOperands[2].data.mem.obj;
                if (absoluteBranch == 0) {
                    relocation->type = 8;
                } else {
                    relocation->type = 10;
                    bits |= 2;
                }
            } else {
                if (instr->operandData.assemblyOperands[2].kind == 4)
                    value = instr->operandData.assemblyOperands[2].data.imm.value;
                else
                    value = instr->operandData.assemblyOperands[2].data.label.label->target.block->code_offset - offset;
                bits |= (UInt16)value;
                if (value < 0) {
                    if (instr->flags & 0x200000)
                        bits |= 0x200000;
                } else {
                    if (instr->flags & 0x100000)
                        bits |= 0x200000;
                }
                if (absoluteBranch)
                    CError_FATAL(328);
            }
            if (instr->flags & fLink)
                bits |= 1;
            break;
        }

        case PC_BTLR:
        case PC_BTCTR:
        case PC_BFLR:
        case PC_BFCTR:
            bits |= ((instr->operandData.assemblyOperands[0].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[1].data.imm.value) &
                     31)
                    << 16;
            if (instr->flags & fLink)
                bits |= 1;
            if (instr->flags & 0x100000)
                bits |= 0x200000;
            break;

        case PC_BCLR:
        case PC_BCCTR:
            bits |= instr->operandData.assemblyOperands[0].data.imm.value << 21;
            bits |= ((instr->operandData.assemblyOperands[1].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[2].data.imm.value) &
                     31)
                    << 16;
        case PC_BLR:
        case PC_BCTR:
        case PC_BCTRL:
        case PC_BLRL:
            if (instr->flags & fLink)
                bits |= 1;
            if (instr->flags & 0x100000)
                bits |= 0x200000;
            break;

        case PC_CRAND:
        case PC_CRANDC:
        case PC_CREQV:
        case PC_CRNAND:
        case PC_CRNOR:
        case PC_CROR:
        case PC_CRORC:
        case PC_CRXOR:
            bits |= ((instr->operandData.assemblyOperands[0].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[1].data.imm.value) &
                     31)
                    << 21;
            bits |= ((instr->operandData.assemblyOperands[2].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[3].data.imm.value) &
                     31)
                    << 16;
            bits |= ((instr->operandData.assemblyOperands[4].data.reg.reg * 4 +
                      instr->operandData.assemblyOperands[5].data.imm.value) &
                     31)
                    << 11;
            break;

        case PC_MCRF:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 18;
            break;

        case PC_LBZ:
        case PC_LBZU:
        case PC_LHZ:
        case PC_LHZU:
        case PC_LHA:
        case PC_LHAU:
        case PC_LWZ:
        case PC_LWZU:
        case PC_LMW:
        case PC_STB:
        case PC_STBU:
        case PC_STH:
        case PC_STHU:
        case PC_STW:
        case PC_STWU:
        case PC_STMW:
        case PC_LFS:
        case PC_LFSU:
        case PC_LFD:
        case PC_LFDU:
        case PC_STFS:
        case PC_STFSU:
        case PC_STFD:
        case PC_STFDU:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            if (instr->operandData.assemblyOperands[2].kind == 5) {
                object = instr->operandData.assemblyOperands[2].data.mem.obj;
                value = instr->operandData.assemblyOperands[2].data.mem.offset;
                switch (object->datatype) {
                    case DLOCAL:
                        if (Registers_GetInfo(object)->in_param_area) {
                            type = object->type;
                            value += fn_004a9f70(instr->flags & fCallerSPRelative);
                            if (type->type == TYPESTRUCT) {
                                if (TYPE_STRUCT(type)->align == 16) {
                                    value += 15;
                                    value &= ~15;
                                }
                            } else if (type->type == TYPECLASS) {
                                if (TYPE_CLASS(type)->align == 16) {
                                    value += 15;
                                    value &= ~15;
                                }
                            }
                        } else {
                            value += fn_004a9f90();
                        }
                        value += object->u.var.uid;
                        break;
                    case DDATA:
                        if (instr->operandData.assemblyOperands[2].arg == 10 || BE_symbol_004913b0(object)) {
                            if (instr->operandData.assemblyOperands[2].arg == 6)
                                relocation->type = 5;
                            else if (instr->operandData.assemblyOperands[2].arg == 2)
                                relocation->type = 3;
                            else if (instr->operandData.assemblyOperands[2].arg == 3)
                                relocation->type = 4;
                            else if (instr->operandData.assemblyOperands[2].arg == 10)
                                relocation->type = 14;
                            else
                                CError_FATAL(487);
                            relocation->object = object;
                            relocation->addend = value;
                            value = 0;
                        }
                        break;
                    default:
                        CError_FATAL(510);
                }
                bits |= (UInt16)value;
            } else if (instr->operandData.assemblyOperands[2].kind == 7) {
                if (instr->operandData.assemblyOperands[2].arg == 0)
                    value = value;
                value = instr->operandData.assemblyOperands[2].data.labeldiff.labelA->target.block->code_offset -
                        instr->operandData.assemblyOperands[2].data.labeldiff.labelB->target.block->code_offset +
                        instr->operandData.assemblyOperands[2].data.labeldiff.offset;
                if (instr->operandData.assemblyOperands[2].arg == 1)
                    value = -value;
                if (value > 0x7FFF || value < -0x8000)
                    PPCError_ReportError(109);
                bits |= (UInt16)value;
            } else {
                bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            }
            break;

        case PC_LBZX:
        case PC_LBZUX:
        case PC_LHZX:
        case PC_LHZUX:
        case PC_LHAX:
        case PC_LHAUX:
        case PC_LHBRX:
        case PC_LWZX:
        case PC_LWZUX:
        case PC_LWBRX:
        case PC_STBX:
        case PC_STBUX:
        case PC_STHX:
        case PC_STHUX:
        case PC_STHBRX:
        case PC_STWX:
        case PC_STWUX:
        case PC_STWBRX:
        case PC_LFSX:
        case PC_LFSUX:
        case PC_LFDX:
        case PC_LFDUX:
        case PC_STFSX:
        case PC_STFSUX:
        case PC_STFDX:
        case PC_STFDUX:
        case PC_LWARX:
        case PC_LSWX:
        case PC_STFIWX:
        case PC_STSWX:
        case PC_STWCX:
        case PC_ECIWX:
        case PC_ECOWX:
        case PC_DCREAD:
        case PC_TLBSX:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_DCBF:
        case PC_DCBST:
        case PC_DCBT:
        case PC_DCBTST:
        case PC_DCBZ:
        case PC_DCBI:
        case PC_ICBI:
        case PC_DCCCI:
        case PC_ICBT:
        case PC_ICCCI:
        case PC_ICREAD:
        case PC_DCBA:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            break;

        case PC_ADD:
        case PC_ADDC:
        case PC_ADDE:
        case PC_DIVW:
        case PC_DIVWU:
        case PC_MULHW:
        case PC_MULHWU:
        case PC_MULLW:
        case PC_SUBF:
        case PC_SUBFC:
        case PC_SUBFE:
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
        case PC_ADDME:
        case PC_ADDZE:
        case PC_NEG:
        case PC_SUBFME:
        case PC_SUBFZE:
        case PC_MFROM:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            if (instr->flags & fOverflow)
                bits |= 0x400;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_ADDI:
        case PC_ADDIC:
        case PC_ADDICR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            if (instr->operandData.assemblyOperands[2].kind == 5) {
                object = instr->operandData.assemblyOperands[2].data.mem.obj;
                value = instr->operandData.assemblyOperands[2].data.mem.offset;
                switch (object->datatype) {
                    case DLOCAL:
                        if (Registers_GetInfo(object)->in_param_area)
                            value += fn_004a9f70(instr->flags & fCallerSPRelative);
                        else
                            value += fn_004a9f90();
                        value += object->u.var.uid;
                        break;
                    case DDATA:
                    case DFUNC:
                    case DVFUNC:
                        if (instr->operandData.assemblyOperands[2].arg == 6)
                            relocation->type = 5;
                        else if (instr->operandData.assemblyOperands[2].arg == 2)
                            relocation->type = 3;
                        else if (instr->operandData.assemblyOperands[2].arg == 3)
                            relocation->type = 4;
                        else if (instr->operandData.assemblyOperands[2].arg == 10)
                            relocation->type = 14;
                        else
                            CError_FATAL(700);
                        relocation->object = object;
                        relocation->addend = value;
                        value = 0;
                        break;
                    default:
                        CError_FATAL(719);
                }
                bits |= (UInt16)value;
            } else if (instr->operandData.assemblyOperands[2].kind == 7) {
                if (instr->operandData.assemblyOperands[2].arg == 0)
                    value = value;
                value = instr->operandData.assemblyOperands[2].data.labeldiff.labelA->target.block->code_offset -
                        instr->operandData.assemblyOperands[2].data.labeldiff.labelB->target.block->code_offset +
                        instr->operandData.assemblyOperands[2].data.labeldiff.offset;
                if (instr->operandData.assemblyOperands[2].arg == 1)
                    value = -value;
                if (value > 0x7FFF || value < -0x8000)
                    PPCError_ReportError(109);
                bits |= (UInt16)value;
            } else {
                bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            }
            break;

        case PC_ADDIS:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            if (instr->operandData.assemblyOperands[2].kind == 5) {
                value = instr->operandData.assemblyOperands[2].data.mem.offset;
                switch (instr->operandData.assemblyOperands[2].arg) {
                    case 8:
                        relocation->type = 7;
                        break;
                    case 7:
                        relocation->type = 6;
                        break;
                    case 11:
                        relocation->type = 15;
                        break;
                    case 2:
                        relocation->type = 3;
                        break;
                    default:
                        CError_FATAL(774);
                }
                relocation->object = instr->operandData.assemblyOperands[2].data.mem.obj;
                relocation->addend = value;
            } else if (instr->operandData.assemblyOperands[2].kind == 7) {
                if (instr->operandData.assemblyOperands[2].arg == 0)
                    value = value;
                value = instr->operandData.assemblyOperands[2].data.labeldiff.labelA->target.block->code_offset -
                        instr->operandData.assemblyOperands[2].data.labeldiff.labelB->target.block->code_offset +
                        instr->operandData.assemblyOperands[2].data.labeldiff.offset;
                if (instr->operandData.assemblyOperands[2].arg == 1)
                    value = -value;
                if (value > 0x7FFF || value < -0x8000)
                    PPCError_ReportError(109);
                bits |= (UInt16)value;
            } else {
                bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            }
            break;

        case PC_MULLI:
        case PC_SUBFIC:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            break;

        case PC_LI:
        case PC_LIS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            if (instr->operandData.assemblyOperands[1].kind == 5) {
                value = instr->operandData.assemblyOperands[1].data.mem.offset;
                switch (instr->operandData.assemblyOperands[1].arg) {
                    case 8:
                        relocation->type = 7;
                        break;
                    case 7:
                        relocation->type = 6;
                        break;
                    case 6:
                        relocation->type = 5;
                        break;
                    case 2:
                        relocation->type = 3;
                        break;
                    case 3:
                        relocation->type = 4;
                        break;
                    case 11:
                        relocation->type = 15;
                        break;
                    case 10:
                        relocation->type = 14;
                        break;
                    default:
                        CError_FATAL(859);
                }
                relocation->object = instr->operandData.assemblyOperands[1].data.mem.obj;
                relocation->addend = value;
            } else {
                bits |= (UInt16)instr->operandData.assemblyOperands[1].data.imm.value;
            }
            break;

        case PC_ANDI:
        case PC_ANDIS:
        case PC_ORI:
        case PC_ORIS:
        case PC_XORI:
        case PC_XORIS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            if (instr->operandData.assemblyOperands[2].kind == 5) {
                object = instr->operandData.assemblyOperands[2].data.mem.obj;
                value = instr->operandData.assemblyOperands[2].data.mem.offset;
                switch (object->datatype) {
                    case DLOCAL:
                        value += Registers_GetInfo(object)->in_param_area
                                     ? fn_004a9f70(instr->flags & fCallerSPRelative)
                                     : fn_004a9f90();
                        value += object->u.var.uid;
                        break;
                    case DDATA:
                    case DFUNC:
                    case DVFUNC:
                        if (instr->operandData.assemblyOperands[2].arg == 6)
                            relocation->type = 5;
                        else if (instr->operandData.assemblyOperands[2].arg == 7)
                            relocation->type = 6;
                        else if (instr->operandData.assemblyOperands[2].arg == 8)
                            relocation->type = 7;
                        else if (instr->operandData.assemblyOperands[2].arg == 2)
                            relocation->type = 3;
                        else if (instr->operandData.assemblyOperands[2].arg == 3)
                            relocation->type = 4;
                        else if (instr->operandData.assemblyOperands[2].arg == 10)
                            relocation->type = 14;
                        else if (instr->operandData.assemblyOperands[2].arg == 11)
                            relocation->type = 15;
                        else
                            CError_FATAL(931);
                        relocation->object = object;
                        relocation->addend = value;
                        value = 0;
                        break;
                    default:
                        CError_FATAL(957);
                }
                bits |= (UInt16)value;
            } else if (instr->operandData.assemblyOperands[2].kind == 7) {
                if (instr->operandData.assemblyOperands[2].arg == 0)
                    value = value;
                value = instr->operandData.assemblyOperands[2].data.labeldiff.labelA->target.block->code_offset -
                        instr->operandData.assemblyOperands[2].data.labeldiff.labelB->target.block->code_offset +
                        instr->operandData.assemblyOperands[2].data.labeldiff.offset;
                if (instr->operandData.assemblyOperands[2].arg == 1)
                    value = -value;
                if (value > 0x7FFF || value < -0x8000)
                    PPCError_ReportError(109);
                bits |= (UInt16)value;
            } else {
                bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            }
            break;

        case PC_AND:
        case PC_OR:
        case PC_XOR:
        case PC_NAND:
        case PC_NOR:
        case PC_EQV:
        case PC_ANDC:
        case PC_ORC:
        case PC_SLW:
        case PC_SRW:
        case PC_SRAW:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_EXTSB:
        case PC_EXTSH:
        case PC_CNTLZW:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_MR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_NOT:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_SRAWI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 31) << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_RLWINM:
        case PC_RLWIMI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 31) << 11;
            bits |= (instr->operandData.assemblyOperands[3].data.imm.value & 31) << 6;
            bits |= (instr->operandData.assemblyOperands[4].data.imm.value & 31) << 1;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_RLWNM:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            bits |= (instr->operandData.assemblyOperands[3].data.imm.value & 31) << 6;
            bits |= (instr->operandData.assemblyOperands[4].data.imm.value & 31) << 1;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_CMP:
        case PC_CMPL:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            break;

        case PC_CMPI:
        case PC_CMPLI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            break;

        case PC_MTXER:
        case PC_MTCTR:
        case PC_MTLR:
        case PC_MTMSR:
        case PC_MFMSR:
        case PC_MFXER:
        case PC_MFCTR:
        case PC_MFLR:
        case PC_MFCR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            break;

        case PC_MFFS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_MTCRF:
            bits |= instr->operandData.assemblyOperands[0].data.imm.value << 12;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            break;

        case PC_MTFSF:
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 0xFF) << 17;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_FMR:
        case PC_FABS:
        case PC_FNEG:
        case PC_FNABS:
        case PC_FRES:
        case PC_FRSQRTE:
        case PC_FRSP:
        case PC_FCTIW:
        case PC_FCTIWZ:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_FADD:
        case PC_FADDS:
        case PC_FSUB:
        case PC_FSUBS:
        case PC_FDIV:
        case PC_FDIVS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_FMADD:
        case PC_FMADDS:
        case PC_FMSUB:
        case PC_FMSUBS:
        case PC_FNMADD:
        case PC_FNMADDS:
        case PC_FNMSUB:
        case PC_FNMSUBS:
        case PC_FSEL:
            bits |= instr->operandData.assemblyOperands[3].data.reg.reg << 11;
        case PC_FMUL:
        case PC_FMULS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 6;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_FCMPU:
        case PC_FCMPO:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            break;

        case PC_MTSPR:
            if (instr->operandData.assemblyOperands[0].kind == 2) {
                switch (instr->operandData.assemblyOperands[0].data.reg.reg) {
                    case 0:
                        break;
                    case 2:
                        bits |= 0x80000;
                        break;
                    case 1:
                        bits |= 0x90000;
                        break;
                    default:
                        CError_FATAL(1261);
                }
            } else if (instr->operandData.assemblyOperands[0].kind == 4) {
                bits |= ((instr->operandData.assemblyOperands[0].data.imm.value & 0x1F) << 16) +
                        ((instr->operandData.assemblyOperands[0].data.imm.value & 0x3E0) << 6);
            } else {
                CError_FATAL(1266);
            }
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            break;

        case PC_MTDCR:
            if (instr->operandData.assemblyOperands[0].kind == 4)
                bits |= ((instr->operandData.assemblyOperands[0].data.imm.value & 0x1F) << 16) +
                        ((instr->operandData.assemblyOperands[0].data.imm.value & 0x3E0) << 6);
            else
                CError_FATAL(1275);
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            break;

        case PC_MFSPR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            if (instr->operandData.assemblyOperands[1].kind == 2) {
                switch (instr->operandData.assemblyOperands[1].data.reg.reg) {
                    case 0:
                        break;
                    case 2:
                        bits |= 0x80000;
                        break;
                    case 1:
                        bits |= 0x90000;
                        break;
                    default:
                        CError_FATAL(1299);
                }
            } else if (instr->operandData.assemblyOperands[1].kind == 4) {
                bits |= ((instr->operandData.assemblyOperands[1].data.imm.value & 0x1F) << 16) +
                        ((instr->operandData.assemblyOperands[1].data.imm.value & 0x3E0) << 6);
            } else {
                CError_FATAL(1304);
            }
            break;

        case PC_MFDCR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            if (instr->operandData.assemblyOperands[1].kind == 4)
                bits |= ((instr->operandData.assemblyOperands[1].data.imm.value & 0x1F) << 16) +
                        ((instr->operandData.assemblyOperands[1].data.imm.value & 0x3E0) << 6);
            else
                CError_FATAL(1313);
            break;

        case PC_LSWI:
        case PC_STSWI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 31) << 11;
            break;

        case PC_MCRFS:
            bits |= (instr->operandData.assemblyOperands[1].data.imm.value & 7) << 18;
        case PC_MCRXR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            break;

        case PC_MFTB:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= ((instr->operandData.assemblyOperands[1].data.imm.value & 0x1F) << 16) +
                    ((instr->operandData.assemblyOperands[1].data.imm.value & 0x3E0) << 6);
            break;

        case PC_MTSR:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 15) << 16;
            break;

        case PC_MFSR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= (instr->operandData.assemblyOperands[1].data.imm.value & 15) << 16;
            break;

        case PC_MFSRIN:
        case PC_MTSRIN:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            break;

        case PC_MTFSB0:
        case PC_MTFSB1:
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 31) << 21;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_MTFSFI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            bits |= (instr->operandData.assemblyOperands[1].data.imm.value & 15) << 12;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_FSQRT:
        case PC_FSQRTS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_TLBIE:
        case PC_TLBLD:
        case PC_TLBLI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 11;
            break;

        case PC_TW:
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 31) << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            break;

        case PC_TWI:
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 31) << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            break;

        case PC_OPWORD:
            if (instr->operandData.assemblyOperands[0].kind == 7) {
                if (instr->operandData.assemblyOperands[0].arg == 0)
                    value = value;
                value = instr->operandData.assemblyOperands[0].data.labeldiff.labelA->target.block->code_offset -
                        instr->operandData.assemblyOperands[0].data.labeldiff.labelB->target.block->code_offset +
                        instr->operandData.assemblyOperands[0].data.labeldiff.offset;
                if (instr->operandData.assemblyOperands[0].arg == 1)
                    value = -value;
                bits = value;
            } else {
                bits = instr->operandData.assemblyOperands[0].data.imm.value;
            }
            break;

        case PC_MASKG:
        case PC_MASKIR:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_LSCBX:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_DIV:
        case PC_DIVS:
        case PC_DOZ:
        case PC_MUL:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fOverflow)
                bits |= 0x400;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_NABS:
        case PC_ABS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            if (instr->flags & fOverflow)
                bits |= 0x400;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_CLCS:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_DOZI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= (UInt16)instr->operandData.assemblyOperands[2].data.imm.value;
            break;

        case PC_RLMI:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            bits |= (instr->operandData.assemblyOperands[3].data.imm.value & 31) << 6;
            bits |= (instr->operandData.assemblyOperands[4].data.imm.value & 31) << 1;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_SLE:
        case PC_SLEQ:
        case PC_SLLQ:
        case PC_SLQ:
        case PC_SRAQ:
        case PC_SRE:
        case PC_SREA:
        case PC_SREQ:
        case PC_SRLQ:
        case PC_SRQ:
        case PC_RRIB:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_SLIQ:
        case PC_SLLIQ:
        case PC_SRAIQ:
        case PC_SRIQ:
        case PC_SRLIQ:
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 31) << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_TLBRE:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 1) << 11;
            break;

        case PC_TLBWE:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 1) << 11;
            break;

        case PC_WRTEE:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            break;

        case PC_WRTEEI:
            bits |= instr->operandData.assemblyOperands[0].data.imm.value << 15;
            break;

        case PC_DSTT:
        case PC_DSTSTT:
            bits |= 0x2000000;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 3) << 21;
            break;

        case PC_DST:
        case PC_DSTST:
            bits |= (instr->operandData.assemblyOperands[3].data.imm.value & 1) << 25;
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 3) << 21;
            break;

        case PC_DSSALL:
            bits |= 0x2000000;
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 3) << 21;
            break;

        case PC_DSS:
            bits |= (instr->operandData.assemblyOperands[1].data.imm.value & 1) << 25;
            bits |= (instr->operandData.assemblyOperands[0].data.imm.value & 3) << 21;
            break;

        case PC_LVEBX:
        case PC_LVEHX:
        case PC_LVEWX:
        case PC_LVSL:
        case PC_LVSR:
        case PC_LVX:
        case PC_LVXL:
        case PC_STVEBX:
        case PC_STVEHX:
        case PC_STVEWX:
        case PC_STVX:
        case PC_STVXL:
        case PC_VADDCUW:
        case PC_VADDFP:
        case PC_VADDSBS:
        case PC_VADDSHS:
        case PC_VADDSWS:
        case PC_VADDUBM:
        case PC_VADDUBS:
        case PC_VADDUHM:
        case PC_VADDUHS:
        case PC_VADDUWM:
        case PC_VADDUWS:
        case PC_VAND:
        case PC_VANDC:
        case PC_VAVGSB:
        case PC_VAVGSH:
        case PC_VAVGSW:
        case PC_VAVGUB:
        case PC_VAVGUH:
        case PC_VAVGUW:
        case PC_VMAXFP:
        case PC_VMAXSB:
        case PC_VMAXSH:
        case PC_VMAXSW:
        case PC_VMAXUB:
        case PC_VMAXUH:
        case PC_VMAXUW:
        case PC_VMINFP:
        case PC_VMINSB:
        case PC_VMINSH:
        case PC_VMINSW:
        case PC_VMINUB:
        case PC_VMINUH:
        case PC_VMINUW:
        case PC_VMRGHB:
        case PC_VMRGHH:
        case PC_VMRGHW:
        case PC_VMRGLB:
        case PC_VMRGLH:
        case PC_VMRGLW:
        case PC_VMULESB:
        case PC_VMULESH:
        case PC_VMULEUB:
        case PC_VMULEUH:
        case PC_VMULOSB:
        case PC_VMULOSH:
        case PC_VMULOUB:
        case PC_VMULOUH:
        case PC_VNOR:
        case PC_VOR:
        case PC_VPKPX:
        case PC_VPKSHSS:
        case PC_VPKSHUS:
        case PC_VPKSWSS:
        case PC_VPKSWUS:
        case PC_VPKUHUM:
        case PC_VPKUHUS:
        case PC_VPKUWUM:
        case PC_VPKUWUS:
        case PC_VRLB:
        case PC_VRLH:
        case PC_VRLW:
        case PC_VSL:
        case PC_VSLB:
        case PC_VSLH:
        case PC_VSLO:
        case PC_VSLW:
        case PC_VSR:
        case PC_VSRAB:
        case PC_VSRAH:
        case PC_VSRAW:
        case PC_VSRB:
        case PC_VSRH:
        case PC_VSRO:
        case PC_VSRW:
        case PC_VSUBCUW:
        case PC_VSUBFP:
        case PC_VSUBSBS:
        case PC_VSUBSHS:
        case PC_VSUBSWS:
        case PC_VSUBUBM:
        case PC_VSUBUBS:
        case PC_VSUBUHM:
        case PC_VSUBUHS:
        case PC_VSUBUWM:
        case PC_VSUBUWS:
        case PC_VSUMSWS:
        case PC_VSUM2SWS:
        case PC_VSUM4SBS:
        case PC_VSUM4SHS:
        case PC_VSUM4UBS:
        case PC_VXOR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            break;

        case PC_VCFSX:
        case PC_VCFUX:
        case PC_VCTSXS:
        case PC_VCTUXS:
        case PC_VSPLTB:
        case PC_VSPLTH:
        case PC_VSPLTW:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            bits |= (instr->operandData.assemblyOperands[2].data.imm.value & 31) << 16;
            break;

        case PC_VEXPTEFP:
        case PC_VLOGEFP:
        case PC_VREFP:
        case PC_VRFIM:
        case PC_VRFIN:
        case PC_VRFIP:
        case PC_VRFIZ:
        case PC_VRSQRTEFP:
        case PC_VUPKHPX:
        case PC_VUPKHSB:
        case PC_VUPKHSH:
        case PC_VUPKLPX:
        case PC_VUPKLSB:
        case PC_VUPKLSH:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            break;

        case PC_VCMPBFP:
        case PC_VCMPEQFP:
        case PC_VCMPEQUB:
        case PC_VCMPEQUH:
        case PC_VCMPEQUW:
        case PC_VCMPGEFP:
        case PC_VCMPGTFP:
        case PC_VCMPGTSB:
        case PC_VCMPGTSH:
        case PC_VCMPGTSW:
        case PC_VCMPGTUB:
        case PC_VCMPGTUH:
        case PC_VCMPGTUW:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 0x400;
            break;

        case PC_VSPLTISB:
        case PC_VSPLTISH:
        case PC_VSPLTISW:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= (instr->operandData.assemblyOperands[1].data.imm.value & 31) << 16;
            break;

        case PC_VMHADDSHS:
        case PC_VMHRADDSHS:
        case PC_VMLADDUHM:
        case PC_VMSUMMBM:
        case PC_VMSUMSHM:
        case PC_VMSUMSHS:
        case PC_VMSUMUBM:
        case PC_VMSUMUHM:
        case PC_VMSUMUHS:
        case PC_VPERM:
        case PC_VSEL:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            bits |= instr->operandData.assemblyOperands[3].data.reg.reg << 6;
            break;

        case PC_VMADDFP:
        case PC_VNMSUBFP:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 6;
            bits |= instr->operandData.assemblyOperands[3].data.reg.reg << 11;
            break;

        case PC_VSLDOI:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            bits |= (instr->operandData.assemblyOperands[3].data.imm.value & 15) << 6;
            break;

        case PC_VMR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            break;

        case PC_VMRP:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            break;

        case PC_MFVSCR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            break;

        case PC_MTVSCR:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 11;
            break;

        case PC_PSQ_L:
        case PC_PSQ_LU:
        case PC_PSQ_ST:
        case PC_PSQ_STU:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[3].data.imm.value << 15;
            bits |= instr->operandData.assemblyOperands[4].data.imm.value << 12;
            bits |= instr->operandData.assemblyOperands[2].data.imm.value & 0xFFF;
            break;

        case PC_PSQ_LX:
        case PC_PSQ_LUX:
        case PC_PSQ_STX:
        case PC_PSQ_STUX:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            bits |= instr->operandData.assemblyOperands[3].data.imm.value << 10;
            bits |= instr->operandData.assemblyOperands[4].data.imm.value << 7;
            break;

        case PC_DCBZ_L:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            break;

        case PC_PS_ADD:
        case PC_PS_SUB:
        case PC_PS_DIV:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_PS_MADD:
        case PC_PS_MSUB:
        case PC_PS_NMADD:
        case PC_PS_NMSUB:
        case PC_PS_SEL:
        case PC_PS_SUM0:
        case PC_PS_SUM1:
        case PC_PS_MADDS0:
        case PC_PS_MADDS1:
            bits |= instr->operandData.assemblyOperands[3].data.reg.reg << 11;
        case PC_PS_MUL:
        case PC_PS_MULS0:
        case PC_PS_MULS1:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 6;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_PS_RES:
        case PC_PS_ABS:
        case PC_PS_NABS:
        case PC_PS_NEG:
        case PC_PS_MR:
        case PC_PS_RSQRTE:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_PS_CMPU0:
        case PC_PS_CMPO0:
        case PC_PS_CMPU1:
        case PC_PS_CMPO1:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 23;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            break;

        case PC_PS_MERGE00:
        case PC_PS_MERGE01:
        case PC_PS_MERGE10:
        case PC_PS_MERGE11:
            bits |= instr->operandData.assemblyOperands[0].data.reg.reg << 21;
            bits |= instr->operandData.assemblyOperands[1].data.reg.reg << 16;
            bits |= instr->operandData.assemblyOperands[2].data.reg.reg << 11;
            if (instr->flags & fRecordBit)
                bits |= 1;
            break;

        case PC_EIEIO:
        case PC_ISYNC:
        case PC_SYNC:
        case PC_RFI:
        case PC_NOP:
        case PC_SC:
        case PC_TLBIA:
        case PC_TLBSYNC:
        case PC_TRAP:
        case PC_DSA:
        case PC_ESA:
        case PC_RFCI:
            break;

        default:
            CError_FATAL(2068);
    }

    return CTool_EndianConvertWord32(bits);
}
