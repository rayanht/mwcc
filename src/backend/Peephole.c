#define CERROR_FILE "Peephole.c"
#include "compiler/common.h"
#include "compiler/Peephole.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/CExpr2.h"
#include "compiler/CMachine.h"
#include "compiler/CodeGen.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/LoopDetection.h"
#include "compiler/MachineSimulation7400.h"
#include "compiler/PCode.h"
#include "compiler/PCodeUtilities.h"

#include <string.h>

struct PCodeBlock *gReturnBlock;

static struct RegisterBlockLiveness *register_block_liveness;
static struct RegisterBlockLiveness *gRegisterBlockLiveness;
static struct RegisterBlockLiveness *registerBlockLiveness;
static struct RegisterBlockLiveness *data_005813ac;
static struct PeepHandler *CodeGen_PeepholeHandlers_005813b0[466];
static struct PCodeInstruction **CodeGen_ReachingDefTable_00581af8;

typedef int (*PeepholeRuleProc)(PCodeInstruction *, UInt32, UInt32, UInt32, UInt32);
static const int kNumRegs = 32;

static int PCode_LiveAfter(PCodeInstruction *instr);
static int PCode_DefinedBetween(PCodeInstruction *instr, PCodeInstruction *def, PCodeOperand *arg);
static int PCode_UsedBetween(PCodeInstruction *instr, PCodeInstruction *def, PCodeOperand *arg);
static int masks_disjoint(PCodeInstruction *instr, PCodeInstruction *scan, SInt16 reg);
static int canmergemasks(SInt32 b1, SInt32 e1, SInt32 shift, SInt32 b2, SInt32 e2, SInt16 *first, SInt16 *last);
static void AddPeepholeRule(SInt16 opcode, void *handler);

void initialize_register_block_liveness(void)
{
    int use_count, definition_count;
    unsigned long uses0, defs0;
    long uses1, defs1;
    unsigned int uses3, defs3;
    unsigned long uses9, defs9;
    unsigned int bit, copy;
    signed char flags;
    PCodeOperand *operand;
    PCodeInstruction *instruction;
    PCodeInstruction *first_instruction;
    PCodeBlock *block;
    block = gPCodeBlocks;
    while (block != NULL) {
        RegisterBlockLiveness *q;
        RegisterBlockLiveness *p;
        defs0 = 0;
        uses3 = 0;
        defs3 = 0;
        uses9 = 0;
        defs9 = 0;
        uses0 = 0;
        uses1 = 0;
        defs1 = 0;
        first_instruction = block->instructions;
        instruction = first_instruction;
        if (first_instruction)
            do {
                for (operand = instruction->operandData.operands,
                    use_count = definition_count = instruction->operand_count;
                     use_count--; operand++) {
                    if (operand->kind == PCOp_GPR && (flags = operand->flags, flags & PCodeOperand_Use) != 0 &&
                        (defs0 & (bit = 1 << operand->value.reg)) == 0)
                        uses0 = (bit = uses0 | bit);
                    else if (operand->kind == PCOp_FPR && (flags = operand->flags, flags & PCodeOperand_Use) != 0 &&
                             ((copy = (unsigned int)defs1) & (bit = 1 << operand->value.reg)) == 0)
                        uses1 |= bit;
                    else if (operand->kind == PCOp_CRFIELD && (flags = operand->flags, flags & PCodeOperand_Use) != 0 &&
                             ((bit = 1 << operand->value.reg) & defs3) == 0)
                        uses3 = uses3 | bit;
                    else if (operand->kind == PCOp_VR && (flags = operand->flags, flags & PCodeOperand_Use) != 0 &&
                             (defs9 & (bit = 1 << operand->value.reg)) == 0)
                        uses9 |= bit;
                }
                operand = instruction->operandData.operands;
                while (definition_count--) {
                    if (operand->kind == PCOp_GPR && (flags = operand->flags, flags & PCodeOperand_Definition) != 0 &&
                        (uses0 & (bit = 1 << operand->value.reg)) == 0)
                        defs0 |= bit;
                    else if (operand->kind == PCOp_FPR &&
                             (flags = operand->flags, flags & PCodeOperand_Definition) != 0 &&
                             (uses1 & (bit = 1 << operand->value.reg)) == 0)
                        defs1 |= bit;
                    else if (operand->kind == PCOp_CRFIELD &&
                             (flags = operand->flags, flags & PCodeOperand_Definition) != 0 &&
                             ((bit = 1 << operand->value.reg) & uses3) == 0)
                        defs3 = defs3 | bit;
                    else if (operand->kind == PCOp_VR &&
                             (flags = operand->flags, flags & PCodeOperand_Definition) != 0 &&
                             (uses9 & (bit = 1 << operand->value.reg)) == 0)
                        defs9 |= bit;
                    operand++;
                }
                instruction = instruction->next;
            } while (instruction);
        p = &register_block_liveness[block->index];
        p->use = uses0;
        p->def = defs0;
        p->live_in = 2;
        p->live_out = 2;
        p = &gRegisterBlockLiveness[block->index];
        p->use = uses1;
        p->def = defs1;
        p->live_in = 0;
        p->live_out = 0;
        q = &registerBlockLiveness[block->index];
        q->use = uses3;
        q->def = defs3;
        q->live_in = 0;
        q->live_out = 0;
        p = &data_005813ac[block->index];
        p->use = uses9;
        p->def = defs9;
        p->live_in = 0;
        p->live_out = 0;
        block = block->next;
    }
    return;
}

void compute_register_block_liveness(RegisterBlockLiveness *data, UInt32 mask)
{
    PCodeBlockLink *e;
    RegisterBlockLiveness *d;
    UInt32 val;
    UInt32 out;
    SInt32 i;
    SInt32 changed;

    do {
        changed = 0;
        i = gPCodeBlockCount;
        while (i) {
            PCodeBlock *b = gPCodeBlockOrder[--i];
            if (b != NULL) {
                d = &data[b->index];
                val = mask;
                for (e = b->successors; e != NULL; e = e->next)
                    val |= data[e->payload.block->index].live_in;
                d->live_out = val;
                out = d->use | (d->live_out & ~d->def);
                if (out != d->live_in) {
                    d->live_in = out;
                    changed = 1;
                }
            }
        }
    } while (changed);
}

void build_register_block_liveness(Object *object)
{
    Type *returnType = TYPE_FUNC(object->type)->functype;
    int structKind;

    register_block_liveness = (RegisterBlockLiveness *)lalloc(gPCodeBlockCount * sizeof(RegisterBlockLiveness));
    gRegisterBlockLiveness = (RegisterBlockLiveness *)lalloc(gPCodeBlockCount * sizeof(RegisterBlockLiveness));
    registerBlockLiveness = (RegisterBlockLiveness *)lalloc(gPCodeBlockCount * sizeof(RegisterBlockLiveness));
    data_005813ac = (RegisterBlockLiveness *)lalloc(gPCodeBlockCount * sizeof(RegisterBlockLiveness));

    SpillCode_BuildBlockOrder();
    initialize_register_block_liveness();

    if (returnType->type == TYPEINT || returnType->type == TYPEENUM || returnType->type == TYPEPOINTER ||
        (returnType->type == TYPEMEMBERPOINTER && returnType->size == 4)) {
        register_block_liveness[gReturnBlock->index].use |= 8;
        if ((returnType->type == TYPEINT || returnType->type == TYPEENUM) && returnType->size == 8)
            register_block_liveness[gReturnBlock->index].use |= 0x10;
    } else if (returnType->type == TYPEFLOAT) {
        if (copts.operandsDebug && returnType->type == TYPEFLOAT) {
            register_block_liveness[gReturnBlock->index].use |= 8;
            if (returnType->size == 8)
                register_block_liveness[gReturnBlock->index].use |= 0x10;
        } else {
            gRegisterBlockLiveness[gReturnBlock->index].use |= 2;
        }
    } else if (returnType->type == TYPESTRUCT && (structKind = TYPE_STRUCT(returnType)->stype) >= STRUCT_VECTOR_UCHAR &&
               structKind <= STRUCT_VECTOR_PIXEL) {
        data_005813ac[gReturnBlock->index].use |= 4;
    } else if ((returnType->type == TYPESTRUCT || returnType->type == TYPECLASS) &&
               !Type_RequiresMemoryReturn(returnType)) {
        register_block_liveness[gReturnBlock->index].use |= 8;
        if (returnType->size > 4)
            register_block_liveness[gReturnBlock->index].use |= 0x10;
    }

    compute_register_block_liveness(register_block_liveness, 2);
    compute_register_block_liveness(gRegisterBlockLiveness, 0);
    compute_register_block_liveness(registerBlockLiveness, 0);
    compute_register_block_liveness(data_005813ac, 0);
}

void build_reaching_def_table(PCodeBlock *block)
{
    PCodeInstruction *integerDefs[32];
    PCodeInstruction *floatDefs[32];
    PCodeInstruction *vectorDefs[32];
    PCodeInstruction *conditionDefs[8];
    PCodeInstruction *instruction;
    PCodeOperand *operand;
    PCodeInstruction *initialDefinition;
    SInt32 i;
    SInt32 useStart;
    SInt32 operandCount;

    initialDefinition = PCodeUtilities_CreateInstruction(0x8c);
    for (i = 0; i < kNumRegs; i++) {
        vectorDefs[i] = initialDefinition;
        floatDefs[i] = initialDefinition;
        integerDefs[i] = floatDefs[i];
    }
    for (i = 0; i < 8; i++) {
        conditionDefs[i] = initialDefinition;
    }
    operandCount = 0;
    for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
        operandCount += instruction->operand_count;
    }
    if (operandCount != 0) {
        CodeGen_ReachingDefTable_00581af8 =
            (PCodeInstruction **)oalloc(operandCount * sizeof(*CodeGen_ReachingDefTable_00581af8));
        for (i = 0; i < operandCount; i++) {
            CodeGen_ReachingDefTable_00581af8[i] = initialDefinition;
        }
        useStart = 0;
        for (instruction = block->instructions; instruction != NULL; instruction = instruction->next) {
            instruction->useStart = useStart;
            for (i = 0, operand = instruction->operandData.operands; i < instruction->operand_count; i++, operand++) {
                if (operand->kind == PCOp_GPR && (operand->flags & 1)) {
                    CodeGen_ReachingDefTable_00581af8[useStart + i] = integerDefs[operand->value.reg];
                } else if (operand->kind == PCOp_FPR && (operand->flags & 1)) {
                    CodeGen_ReachingDefTable_00581af8[useStart + i] = floatDefs[operand->value.reg];
                } else if (operand->kind == PCOp_VR && (operand->flags & 1)) {
                    CodeGen_ReachingDefTable_00581af8[useStart + i] = vectorDefs[operand->value.reg];
                } else if (operand->kind == PCOp_CRFIELD && (operand->flags & 1)) {
                    CodeGen_ReachingDefTable_00581af8[useStart + i] = conditionDefs[operand->value.reg];
                }
            }
            for (i = 0, operand = instruction->operandData.operands; i < instruction->operand_count; i++, operand++) {
                if (operand->kind == PCOp_GPR && (operand->flags & 2)) {
                    integerDefs[operand->value.reg] = instruction;
                } else if (operand->kind == PCOp_FPR && (operand->flags & 2)) {
                    floatDefs[operand->value.reg] = instruction;
                } else if (operand->kind == PCOp_VR && (operand->flags & 2)) {
                    vectorDefs[operand->value.reg] = instruction;
                } else if (operand->kind == PCOp_CRFIELD && (operand->flags & 2)) {
                    conditionDefs[operand->value.reg] = instruction;
                }
            }
            useStart += instruction->operand_count;
        }
    }
}

int fn_004cc040(PCodeInstruction *instruction, UInt32 gprMask, UInt32 fprMask, UInt32 crFieldMask, UInt32 vectorMask,
                Boolean checkSpecialRegisterZero)
{
    PCodeOperand *operand;
    int remainingOperands;

    if ((instruction->block->flags & 3) != 0)
        return 0;
    if ((instruction->flags & 0x20434) != 0)
        return 0;
    if (instruction->block->predecessors == NULL)
        return 1;

    remainingOperands = instruction->operand_count;
    operand = instruction->operandData.operands;
    while (remainingOperands--) {
        if (operand->kind == PCOp_GPR && (operand->flags & 2) && ((1 << operand->value.reg) & gprMask))
            return 0;
        if (operand->kind == PCOp_FPR && (operand->flags & 2) && ((1 << operand->value.reg) & fprMask))
            return 0;
        if (operand->kind == PCOp_VR && (operand->flags & 2) && ((1 << operand->value.reg) & vectorMask))
            return 0;
        if (operand->kind == PCOp_SPR && (operand->flags & 2) && (operand->value.reg != 0 || checkSpecialRegisterZero))
            return 0;
        if (operand->kind == PCOp_CRFIELD && (operand->flags & 2) && ((1 << operand->value.reg) & crFieldMask))
            return 0;
        operand++;
    }
    return 1;
}

/* Whether an instruction between INSTR and DEF (both exclusive) defines ARG's register. */
static int PCode_DefinedBetween(PCodeInstruction *instr, PCodeInstruction *def, PCodeOperand *arg)
{
    PCodeInstruction *scan;
    PCodeOperand *op;
    UInt32 n;

    for (scan = instr->previous; scan != def; scan = scan->previous) {
        for (op = scan->operandData.operands, n = scan->operand_count; n--; op++) {
            if (op->kind == arg->kind && op->value.reg == arg->value.reg && (op->flags & 2))
                return 1;
        }
    }
    return 0;
}

/* Whether an instruction between INSTR and DEF (both exclusive) uses ARG's register. */
static int PCode_UsedBetween(PCodeInstruction *instr, PCodeInstruction *def, PCodeOperand *arg)
{
    PCodeInstruction *scan;
    PCodeOperand *op;
    UInt32 n;

    for (scan = instr->previous; scan != def; scan = scan->previous) {
        for (op = scan->operandData.operands, n = scan->operand_count; n--; op++) {
            if (op->kind == arg->kind && op->value.reg == arg->value.reg && (op->flags & 1))
                return 1;
        }
    }
    return 0;
}

int has_reg_flag_one_before_flag_two(PCodeInstruction *list, int hash)
{
    PCodeInstruction *node;
    PCodeOperand *e;
    UInt32 i;

    node = list->next;
    if (node != NULL) {
        do {
            e = node->operandData.operands;
            for (i = node->operand_count; i--; e++) {
                if (e->kind == PCOp_SPR && e->value.reg == hash && (e->flags & 1))
                    return 1;
                if (e->kind == PCOp_SPR && e->value.reg == hash && (e->flags & 2))
                    return 0;
            }
            node = node->next;
        } while (node != NULL);
    }
    return 0;
}

static unsigned int RotatedMask(struct PCodeInstruction *pc)
{
    int sh;
    int mb;
    int me;
    unsigned int m;
    sh = pc->operandData.operands[2].value.signed_value;
    mb = pc->operandData.operands[3].value.signed_value;
    me = pc->operandData.operands[4].value.signed_value;
    if (mb <= me)
        m = ((mb > 31) ? 0 : (0xFFFFFFFF >> mb)) & ~(((me + 1) > 31) ? 0 : (0xFFFFFFFF >> (me + 1)));
    else
        m = ((mb > 31) ? 0 : (0xFFFFFFFF >> mb)) | ~(((me + 1) > 31) ? 0 : (0xFFFFFFFF >> (me + 1)));
    return (m >> sh) | (m << (32 - sh));
}
static int canmergemasks(SInt32 b1, SInt32 e1, SInt32 shift, SInt32 b2, SInt32 e2, SInt16 *first, SInt16 *last)
{
    UInt32 mask1;
    UInt32 mask2;

    if (b1 <= e1)
        mask1 = ((b1 > 31) ? 0 : (0xFFFFFFFF >> b1)) & ~(((int)(e1 + 1) > 31) ? 0 : (0xFFFFFFFF >> (e1 + 1)));
    else
        mask1 = ((b1 > 31) ? 0 : (0xFFFFFFFF >> b1)) | ~(((int)(e1 + 1) > 31) ? 0 : (0xFFFFFFFF >> (e1 + 1)));

    if (b2 <= e2)
        mask2 = ((b2 > 31) ? 0 : (0xFFFFFFFF >> b2)) & ~(((int)(e2 + 1) > 31) ? 0 : (0xFFFFFFFF >> (e2 + 1)));
    else
        mask2 = ((b2 > 31) ? 0 : (0xFFFFFFFF >> b2)) | ~(((int)(e2 + 1) > 31) ? 0 : (0xFFFFFFFF >> (e2 + 1)));

    return InstrSelection_GetMaskRange(((mask1 << shift) | (mask1 >> (32 - shift))) & mask2, first, last);
}

int fn_004cbe40(PCodeBlock *node, PCodeInstruction *item, int instructionCount, int branchCount, int targetCount,
                int memoryCount, int specialCount)
{
    PCodeBlockLink *entry;
    SInt32 operandIndex;
    SInt32 opcode;

    if (item != NULL) {
        do {
            instructionCount++;
            if (instructionCount > 0x11)
                return 1;
            opcode = item->opcode;
            switch (opcode) {
                case 0x45:
                case 0x46:
                case 0x47:
                case 0x48:
                case 0x49:
                case 0x4a:
                    if (targetCount == 0)
                        return 1;
                    return 0;
                case 0x77:
                case 0x78:
                case 0x79:
                case 0x7a:
                case 0x7b:
                case 0x7c:
                case 0x7d:
                case 0x7e:
                case 0x7f:
                case 0x80:
                case 0x81:
                case 0x82:
                case 0xc1:
                case 0xc2:
                case 0xc3:
                case 0xc4:
                case 0xc5:
                case 0xc6:
                case 0xc7:
                case 0xc8:
                case 0xc9:
                case 0xca:
                case 0xcb:
                case 0xcc:
                case 0xcd:
                case 0xce:
                case 0xcf:
                case 0xd2:
                case 0xd3:
                case 0xd4:
                case 0xd5:
                case 0xd6:
                case 0xd7:
                case 0xd8:
                case 0xd9:
                case 0xdb:
                case 0xdc:
                case 0xdd:
                    return 1;
                case 0x6e:
                case 0x6f:
                case 0x70:
                case 0x71:
                case 0x72:
                case 0x73:
                case 0x74:
                case 0x75:
                case 0x76:
                    specialCount++;
                    if (specialCount > 1)
                        return 1;
                    break;
                default:
                    break;
            }
            if ((item->flags & (fIsRead | fIsWrite)) != 0) {
                memoryCount++;
                if (memoryCount > 1)
                    return 1;
            } else if ((item->flags & fIsBranch) != 0) {
                branchCount++;
                if (branchCount > 2)
                    return 1;
                for (operandIndex = 0; operandIndex < item->operand_count; operandIndex++) {
                    if (item->operandData.operands[operandIndex].kind == PCOp_CRFIELD) {
                        targetCount++;
                        break;
                    }
                }
            }
            item = item->next;
        } while (item != NULL);
    }
    if (node != NULL && node->successors != NULL) {
        entry = node->successors;
        while (entry != NULL) {
            if (entry->payload.block != NULL &&
                fn_004cbe40(entry->payload.block, entry->payload.block->instructions, instructionCount, branchCount,
                            targetCount, memoryCount, specialCount) == 0)
                return 0;
            entry = entry->next;
        }
    }
    return 1;
}

/* Whether a register written by INSTR is still live after it. */
static int PCode_LiveAfter(PCodeInstruction *instr)
{
    return fn_004cbe40(instr->block, instr->next, 0, 0, 0, 0, 0);
}
unsigned int unlink_equal_reg_instruction(PCodeInstruction *record)
{
    unsigned short second_value = record->operandData.operands[1].value.reg;
    unsigned short first_value = record->operandData.operands[0].value.reg;
    if (first_value == second_value) {
        if (!(record->flags & 128U)) {
            PCode_UnlinkInstruction(record);
            return 1;
        }
    }
    return 0;
}

int unlink_same_reg_instruction(PCodeInstruction *object)
{
    if (object->operandData.operands[0].value.reg == object->operandData.operands[1].value.reg &&
        !(object->flags & fRecordBit)) {
        PCode_UnlinkInstruction(object);
        return 1;
    }
    return 0;
}

int unlink_same_reg_move(PCodeInstruction *instruction)
{
    PCodeInstruction *move = instruction;
    if (move->operandData.operands[0].value.reg == move->operandData.operands[1].value.reg && !(move->flags & 0x80U)) {
        PCode_UnlinkInstruction(move);
        return 1;
    }
    return 0;
}

int unlink_instruction_with_matching_mr_def(PCodeInstruction *p)
{
    PCodeInstruction *def;
    PCodeInstruction **tbl;

    tbl = CodeGen_ReachingDefTable_00581af8;
    def = tbl[p->useStart + 1];

    if (def->opcode == PC_MR && p->operandData.operands[0].value.reg == def->operandData.operands[1].value.reg &&
        !PCode_DefinedBetween(p, def, &p->operandData.operands[0]) && !(p->flags & fRecordBit)) {
        PCode_UnlinkInstruction(p);
        return 1;
    }
    return 0;
}

int unlink_instruction_for_unmatched_fmr(PCodeInstruction *p)
{
    PCodeInstruction *s;
    int idx;
    PCodeInstruction **table;

    idx = p->useStart;
    table = CodeGen_ReachingDefTable_00581af8;
    s = table[idx + 1];
    if (s->opcode == PC_FMR && p->operandData.operands[0].value.reg == s->operandData.operands[1].value.reg &&
        (p->flags & fRecordBit) == 0) {
        if (!PCode_DefinedBetween(p, s, &p->operandData.operands[0])) {
            PCode_UnlinkInstruction(p);
            return 1;
        }
    }
    return 0;
}

int unlink_instruction_matching_vmr_source(PCodeInstruction *p)
{
    PCodeInstruction *def;
    PCodeInstruction **tbl;

    tbl = CodeGen_ReachingDefTable_00581af8;
    def = tbl[p->useStart + 1];

    if (def->opcode == PC_VMR && p->operandData.operands[0].value.reg == def->operandData.operands[1].value.reg &&
        !(p->flags & fRecordBit)) {
        if (!PCode_DefinedBetween(p, def, &p->operandData.operands[0])) {
            PCode_UnlinkInstruction(p);
            return 1;
        }
    }
    return 0;
}

int fn_004cba60(CmpCtx *ctx)
{
    PCodeInstruction *b = ctx->second;
    PCodeInstruction *a = ctx->first;
    int flagB = 0;
    int retB = 0;
    int flagA = 0;
    int retA = 0;
    if (b != NULL) {
        flagB = (((SInt32)b->flags & 3) == 3);
        retB = fn_0052f370(b);
    }
    if (a != NULL) {
        flagA = (((SInt32)a->flags & 3) == 3);
        retA = fn_0052f370(a);
    }
    if (b != NULL) {
        if (a != NULL) {
            if (flagB && !retB) {
                if (flagA) {
                    if (!retA) {
                        ctx->errorCode = 0x18f;
                        return 1;
                    }
                } else {
                    ctx->errorCode = 0x18f;
                    return 1;
                }
            }
        } else {
            if (flagB && !retB) {
                ctx->errorCode = 0x18f;
                return 1;
            }
        }
    } else {
        if (a != NULL && flagA && !retA) {
            ctx->errorCode = 0x18f;
            return 1;
        }
    }
    return 0;
}

int make_record_form_and_unlink_instruction(PCodeInstruction *instruction)
{
    PCodeInstruction *record;
    PCodeOperand operand;

    record = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
    if (instruction->operandData.operands[0].value.reg == 0 &&
        instruction->operandData.operands[2].value.signed_value == 0 && (record->flags & 0x601) == 0x201 &&
        !PCode_UsedBetween(instruction, record, &instruction->operandData.operands[0]) &&
        !PCode_DefinedBetween(instruction, record, &instruction->operandData.operands[0])) {
        if (record->opcode == PC_ADDI) {
            (void)operand;
            operand.kind = PCOp_SPR;
            operand.value.reg = 0;
            operand.flags = 3;
            if (PCode_UsedBetween(instruction, record, &operand))
                return 0;
        }
        PCodeUtilities_MakeRecordForm(record);
        PCode_UnlinkInstruction(instruction);
        return 1;
    }
    return 0;
}

int bypass_extsb_for_low_byte_rotated_mask(PCodeInstruction *instruction, int register_mask)
{
    PCodeInstruction *previous = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];

    if (previous->opcode == PC_EXTSB &&
        ((short)previous->operandData.operands[0].value.reg == instruction->operandData.operands[0].value.reg ||
         (register_mask & (1 << (short)previous->operandData.operands[0].value.reg)) == 0) &&
        (previous->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0 &&
        PCode_DefinedBetween(instruction, previous, &previous->operandData.operands[1]) == 0 &&
        PCode_UsedBetween(instruction, previous, &previous->operandData.operands[0]) == 0 &&
        (RotatedMask(instruction) & 0xFFFFFF00) == 0) {
        instruction->operandData.operands[1].value.reg = previous->operandData.operands[1].value.reg;
        CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
            CodeGen_ReachingDefTable_00581af8[previous->useStart + 1];
        PCode_UnlinkInstruction(previous);
        return 1;
    }
    return 0;
}

int bypass_extsh_for_rotated_mask(struct PCodeInstruction *pattern, int registerMask)
{
    struct PCodeInstruction *instruction = CodeGen_ReachingDefTable_00581af8[pattern->useStart + 1];

    if (instruction->opcode == PC_EXTSH &&
        (instruction->operandData.operands[0].value.reg == pattern->operandData.operands[0].value.reg ||
         ((1 << instruction->operandData.operands[0].value.reg) & registerMask) == 0) &&
        (instruction->flags & fRecordBit) == 0 &&
        PCode_DefinedBetween(pattern, instruction, &instruction->operandData.operands[1]) == 0 &&
        PCode_UsedBetween(pattern, instruction, &instruction->operandData.operands[0]) == 0 &&
        (RotatedMask(pattern) & 0xFFFF0000) == 0) {
        pattern->operandData.operands[1].value.reg = instruction->operandData.operands[1].value.reg;
        CodeGen_ReachingDefTable_00581af8[pattern->useStart + 1] =
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
        PCode_UnlinkInstruction(instruction);
        return 1;
    }
    return 0;
}

int fold_lbz_lbzx_mask(PCodeInstruction *instruction, int register_mask)
{
    PCodeInstruction *producer = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
    if ((producer->opcode == PC_LBZ || producer->opcode == PC_LBZX) &&
        instruction->operandData.operands[2].value.signed_value == 0 &&
        instruction->operandData.operands[3].value.signed_value <= 24 &&
        instruction->operandData.operands[4].value.signed_value == 31 && (instruction->flags & fRecordBit) == 0 &&
        (producer->operandData.operands[0].value.reg == instruction->operandData.operands[0].value.reg ||
         ((1 << producer->operandData.operands[0].value.reg) & register_mask) == 0) &&
        PCode_UsedBetween(instruction, producer, &producer->operandData.operands[0]) == 0 &&
        PCode_DefinedBetween(instruction, producer, &instruction->operandData.operands[0]) == 0 &&
        PCode_UsedBetween(instruction, producer, &instruction->operandData.operands[0]) == 0) {
        producer->operandData.operands[0].value.reg = instruction->operandData.operands[0].value.reg;
        PCode_UnlinkInstruction(instruction);
        return 1;
    }
    return 0;
}

int fold_lhz_lhzx_mask(PCodeInstruction *instruction, int register_mask)
{
    PCodeInstruction *producer = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
    if ((producer->opcode == PC_LHZ || producer->opcode == PC_LHZX) &&
        instruction->operandData.operands[2].value.signed_value == 0 &&
        instruction->operandData.operands[3].value.signed_value <= 16 &&
        instruction->operandData.operands[4].value.signed_value == 31 && (instruction->flags & fRecordBit) == 0 &&
        (producer->operandData.operands[0].value.reg == instruction->operandData.operands[0].value.reg ||
         ((1 << producer->operandData.operands[0].value.reg) & register_mask) == 0) &&
        PCode_UsedBetween(instruction, producer, &producer->operandData.operands[0]) == 0 &&
        PCode_DefinedBetween(instruction, producer, &instruction->operandData.operands[0]) == 0 &&
        PCode_UsedBetween(instruction, producer, &instruction->operandData.operands[0]) == 0) {
        producer->operandData.operands[0].value.reg = instruction->operandData.operands[0].value.reg;
        PCode_UnlinkInstruction(instruction);
        return 1;
    }
    return 0;
}

int fold_reaching_lha_or_extsb(PCodeInstruction *p, UInt32 mask)
{
    PCodeInstruction *q;

    q = CodeGen_ReachingDefTable_00581af8[p->useStart + 1];

    if (q->opcode == PC_LHA && (p->flags & fRecordBit) == 0) {
        if (q->operandData.operands[0].value.reg == p->operandData.operands[0].value.reg ||
            (mask & (1 << q->operandData.operands[0].value.reg)) == 0) {
            if (!PCode_UsedBetween(p, q, &q->operandData.operands[0])) {
                if (!PCode_DefinedBetween(p, q, &p->operandData.operands[0])) {
                    if (!PCode_UsedBetween(p, q, &p->operandData.operands[0])) {
                        q->operandData.operands[0].value.reg = p->operandData.operands[0].value.reg;
                        PCode_UnlinkInstruction(p);
                        return 1;
                    }
                }
            }
        }
    }

    if (q->opcode == PC_EXTSB && (p->flags & fRecordBit) == 0) {
        if (q->operandData.operands[0].value.reg == q->operandData.operands[1].value.reg) {
            if ((mask & (1 << q->operandData.operands[0].value.reg)) == 0) {
                if (!PCode_UsedBetween(p, q, &q->operandData.operands[0])) {
                    p->opcode = PC_EXTSB;
                    PCode_UnlinkInstruction(q);
                    return 1;
                }
            }
        } else {
            if (!PCode_DefinedBetween(p, q, &q->operandData.operands[1])) {
                p->opcode = PC_EXTSB;
                p->operandData.operands[1] = q->operandData.operands[1];
            } else {
                p->opcode = PC_MR;
                p->flags |= fIsMove;
            }
            return 1;
        }
    }
    return 0;
}

int fold_li_operand(PCodeInstruction *instruction, int register_mask)
{
    PCodeInstruction *previous;

    previous = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
    if (previous->opcode == PC_LI && previous->operandData.operands[1].kind == PCOp_MEMORY &&
        instruction->operandData.operands[2].kind == PCOp_IMMEDIATE &&
        instruction->operandData.operands[2].value.signed_value == 0 &&
        previous->operandData.operands[0].value.reg == instruction->operandData.operands[1].value.reg &&
        !PCode_DefinedBetween(instruction, previous, &previous->operandData.operands[0])) {
        instruction->operandData.operands[1].value.reg = 0;
        instruction->operandData.operands[2] = previous->operandData.operands[1];
        CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
            CodeGen_ReachingDefTable_00581af8[previous->useStart + 1];
        if (!PCode_UsedBetween(instruction, previous, &previous->operandData.operands[0]) &&
            (1 << previous->operandData.operands[0].value.reg & register_mask) == 0)
            PCode_UnlinkInstruction(previous);
        return 1;
    }
    return 0;
}

/* An addi whose base register comes from another addi (or an li/la-style definition): the two offsets folded into
   one instruction on the first's base, or the first removed when its result is dead. */
int fold_addi_or_mr_reaching_def(PCodeInstruction *pc, UInt32 mask)
{
    PCodeInstruction *def;
    SInt32 offset, sum;

    def = CodeGen_ReachingDefTable_00581af8[pc->useStart + 1];
    if (def->opcode == PC_ADDI && pc->operandData.operands[2].kind == PCOp_IMMEDIATE) {
        if (!(pc->operandData.operands[0].kind == PCOp_GPR &&
              pc->operandData.operands[0].value.reg == pc->operandData.operands[1].value.reg)) {
            if (pc->operandData.operands[2].value.signed_value == 0 &&
                def->operandData.operands[0].value.reg == def->operandData.operands[1].value.reg &&
                !PCode_UsedBetween(pc, def, &def->operandData.operands[0]) &&
                (!(mask & (1 << def->operandData.operands[0].value.reg)) || PCode_LiveAfter(pc))) {
                if (mask & (1 << def->operandData.operands[0].value.reg)) {
                    pc->opcode++;
                    pc->operandData.operands[1].flags |= 2;
                    pc->operandData.operands[2] = def->operandData.operands[2];
                } else {
                    pc->operandData.operands[2] = def->operandData.operands[2];
                }
                CodeGen_ReachingDefTable_00581af8[pc->useStart + 1] =
                    CodeGen_ReachingDefTable_00581af8[def->useStart + 1];
                PCode_UnlinkInstruction(def);
                return 1;
            }
            offset = pc->operandData.operands[2].value.signed_value;
            sum = 0x1ffff;
            if (def->operandData.operands[2].kind == PCOp_IMMEDIATE)
                sum = def->operandData.operands[2].value.signed_value;
            else if (def->operandData.operands[2].kind == PCOp_MEMORY) {
                if (def->operandData.operands[2].object->datatype == DLOCAL) {
                    sum = def->operandData.operands[2].value.signed_value;
                    sum += def->operandData.operands[2].object->u.var.uid;
                } else if (offset == 0)
                    sum = 0;
                else
                    return 0;
            }
            if (sum + offset != (short)(sum + offset))
                return 0;
            if (!(mask & (1 << def->operandData.operands[0].value.reg))) {
                if (!PCode_UsedBetween(pc, def, &def->operandData.operands[0])) {
                    if (!PCode_DefinedBetween(pc, def, &def->operandData.operands[1])) {
                        pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
                        CodeGen_ReachingDefTable_00581af8[pc->useStart + 1] =
                            CodeGen_ReachingDefTable_00581af8[def->useStart + 1];
                        if (def->operandData.operands[2].kind == PCOp_MEMORY) {
                            pc->operandData.operands[2] = def->operandData.operands[2];
                            pc->operandData.operands[2].value.signed_value += offset;
                        } else
                            pc->operandData.operands[2].value.signed_value = sum + offset;
                        PCode_UnlinkInstruction(def);
                        return 1;
                    }
                }
            }
            if (pc->operandData.operands[1].value.reg != def->operandData.operands[1].value.reg &&
                !PCode_DefinedBetween(pc, def, &def->operandData.operands[1])) {
                if (def->operandData.operands[2].kind == PCOp_MEMORY &&
                    def->operandData.operands[2].object->datatype != DLOCAL)
                    return 0;
                pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
                CodeGen_ReachingDefTable_00581af8[pc->useStart + 1] =
                    CodeGen_ReachingDefTable_00581af8[def->useStart + 1];
                if (def->operandData.operands[2].kind == PCOp_MEMORY) {
                    pc->operandData.operands[2] = def->operandData.operands[2];
                    pc->operandData.operands[2].value.signed_value += offset;
                } else
                    pc->operandData.operands[2].value.signed_value = sum + offset;
                return 1;
            }
        }
    } else if (def->opcode == PC_MR && def->operandData.operands[1].kind == PCOp_GPR &&
               def->operandData.operands[1].value.reg != 0) {
        if (!PCode_DefinedBetween(pc, def, &def->operandData.operands[1])) {
            pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
            CodeGen_ReachingDefTable_00581af8[pc->useStart + 1] = CodeGen_ReachingDefTable_00581af8[def->useStart + 1];
        }
    }
    return 0;
}

int fold_reaching_addi(PCodeInstruction *current)
{
    PCodeInstruction *previous;
    unsigned char kind;
    PCodeInstruction **table;
    unsigned long combined;

    kind = current->operandData.operands[2].kind;
    table = CodeGen_ReachingDefTable_00581af8;
    previous = table[current->useStart + 1];
    if (kind == PCOp_IMMEDIATE && previous->operandData.operands[2].kind == PCOp_IMMEDIATE &&
        previous->opcode == PC_ADDI &&
        previous->operandData.operands[0].value.reg == previous->operandData.operands[1].value.reg &&
        (current->operandData.operands[0].kind != current->operandData.operands[1].kind ||
         current->operandData.operands[0].value.reg != current->operandData.operands[1].value.reg)) {
        if (!PCode_UsedBetween(current, previous, &previous->operandData.operands[0])) {
            if (current->operandData.operands[2].value.signed_value +
                    previous->operandData.operands[2].value.signed_value ==
                (short)(current->operandData.operands[2].value.signed_value +
                        previous->operandData.operands[2].value.signed_value)) {
                if ((long)current->operandData.operands[2].value.signed_value +
                        previous->operandData.operands[2].value.signed_value ==
                    0) {
                    current->opcode--;
                    current->operandData.operands[1].flags &= ~2;
                    current->operandData.operands[2].value.signed_value = 0;
                } else {
                    combined = current->operandData.operands[2].value.signed_value;
                    combined += previous->operandData.operands[2].value.signed_value;
                    current->operandData.operands[2].value.signed_value = combined;
                }
                table = CodeGen_ReachingDefTable_00581af8;
                table[current->useStart + 1] = table[previous->useStart + 1];
                PCode_UnlinkInstruction(previous);
                return 1;
            }
        }
    }
    return 0;
}

int eliminate_matching_addi(PCodeInstruction *instruction, UInt32 registerMask)
{
    PCodeOperand *operand;
    PCodeInstruction *nextInstruction;
    int sourceRegister;
    SInt32 operandsRemaining;

    if (instruction->operandData.operands[2].kind != PCOp_IMMEDIATE)
        return 0;
    sourceRegister = instruction->operandData.operands[1].value.reg;
    if (!PCode_LiveAfter(instruction))
        return 0;
    nextInstruction = instruction->next;
    while (nextInstruction != NULL) {
        operand = nextInstruction->operandData.operands;
        operandsRemaining = nextInstruction->operand_count;
        while (operandsRemaining--) {
            if (operand->kind == PCOp_GPR && operand->value.reg == sourceRegister && ((SInt8)operand->flags & 1))
                return 0;
            if (operand->kind == PCOp_GPR && operand->value.reg == sourceRegister && ((SInt8)operand->flags & 2)) {
                if (nextInstruction->opcode != PC_ADDI)
                    return 0;
                if (nextInstruction->operandData.operands[2].kind != PCOp_IMMEDIATE)
                    return 0;
                if (instruction->operandData.operands[2].value.signed_value ==
                        nextInstruction->operandData.operands[2].value.signed_value &&
                    nextInstruction->operandData.operands[0].value.reg ==
                        nextInstruction->operandData.operands[1].value.reg &&
                    (instruction->operandData.operands[0].kind != instruction->operandData.operands[1].kind ||
                     instruction->operandData.operands[0].value.reg !=
                         instruction->operandData.operands[1].value.reg)) {
                    if ((1 << nextInstruction->operandData.operands[0].value.reg) & registerMask) {
                        instruction->opcode++;
                        instruction->operandData.operands[1].flags |= 2;
                        memcpy(&instruction->operandData.operands[2], &nextInstruction->operandData.operands[2],
                               sizeof(instruction->operandData.operands[2]));
                    } else {
                        memcpy(&instruction->operandData.operands[2], &nextInstruction->operandData.operands[2],
                               sizeof(instruction->operandData.operands[2]));
                    }
                    nextInstruction->opcode = PC_NOP;
                    nextInstruction->operand_count = 0;
                    PCode_UnlinkInstruction(nextInstruction);
                    return 1;
                }
                return 0;
            }
            operand++;
        }
        nextInstruction = nextInstruction->next;
    }
    return 0;
}

SInt32 fold_constant_compare_branch(PCodeInstruction *node)
{
    PCodeInstruction *definition;
    PCodeInstruction *rhs;
    PCodeBlockLink *edge;
    PCodeBlockLink **link;

    if (node->operandData.operands[1].value.signed_value == 2) {
        definition = CodeGen_ReachingDefTable_00581af8[node->useStart];
        if ((definition->opcode == PC_CMPLI || definition->opcode == PC_CMPI) &&
            definition->operandData.operands[0].value.reg == 0) {
            rhs = CodeGen_ReachingDefTable_00581af8[definition->useStart + 1];
            if (rhs->opcode == PC_LI && rhs->operandData.operands[1].kind == PCOp_IMMEDIATE &&
                (node->opcode == PC_BT) == (rhs->operandData.operands[1].value.signed_value ==
                                            definition->operandData.operands[2].value.signed_value)) {
                node->opcode = PC_B;
                node->operand_count = 1;
                node->operandData.operands[0] = node->operandData.operands[2];
                CodeGen_ReachingDefTable_00581af8[node->useStart] =
                    CodeGen_ReachingDefTable_00581af8[node->useStart + 1];

                link = &node->block->successors;
                while ((edge = *link) != NULL) {
                    if (edge->payload.block == node->block->next) {
                        *link = edge->next;
                        break;
                    }
                    link = &edge->next;
                }

                link = &node->block->next->predecessors;
                while ((edge = *link) != NULL) {
                    if (edge->payload.block == node->block) {
                        *link = edge->next;
                        break;
                    }
                    link = &edge->next;
                }
            }
        }
    }
    return 0;
}

SInt32 bypass_cmpli_zero(PCodeInstruction *p)
{
    PCodeInstruction *inst;
    PCodeInstruction *src;

    if (p->operandData.operands[1].value.signed_value == 2) {
        inst = CodeGen_ReachingDefTable_00581af8[p->useStart];
        if (inst->opcode == PC_CMPLI) {
            if (inst->operandData.operands[0].value.reg == 0) {
                if (inst->operandData.operands[2].value.signed_value == 0) {
                    src = CodeGen_ReachingDefTable_00581af8[inst->useStart + 1];
                    if ((src->flags & 0x601) == 0x201) {
                        if (!PCode_UsedBetween(inst, src, &inst->operandData.operands[0])) {
                            if (!PCode_DefinedBetween(inst, src, &inst->operandData.operands[0])) {
                                PCodeUtilities_MakeRecordForm(src);
                                CodeGen_ReachingDefTable_00581af8[p->useStart] =
                                    CodeGen_ReachingDefTable_00581af8[inst->useStart + 1];
                                PCode_UnlinkInstruction(inst);
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int remove_redundant_extsb(PCodeInstruction *self, UInt32 mask)
{
    PCodeInstruction *compare;
    PCodeInstruction *signExtend;

    if (self->operandData.operands[1].value.signed_value == 2) {
        compare = CodeGen_ReachingDefTable_00581af8[self->useStart];
        if ((compare->opcode == PC_CMPI || compare->opcode == PC_CMPLI) &&
            compare->operandData.operands[2].value.signed_value >= 0 &&
            compare->operandData.operands[2].value.signed_value <= 0x7f) {
            signExtend = CodeGen_ReachingDefTable_00581af8[compare->useStart + 1];
            if (signExtend->opcode == PC_EXTSB) {
                if (CodeGen_ReachingDefTable_00581af8[signExtend->useStart + 1]->opcode == PC_LBZ &&
                    !((1 << signExtend->operandData.operands[0].value.reg) & mask)) {
                    if (!PCode_UsedBetween(self, compare, &signExtend->operandData.operands[0]) &&
                        !PCode_UsedBetween(compare, signExtend, &signExtend->operandData.operands[0]) &&
                        !PCode_DefinedBetween(compare, signExtend, &signExtend->operandData.operands[1])) {
                        compare->operandData.operands[1].value.reg = signExtend->operandData.operands[1].value.reg;
                        CodeGen_ReachingDefTable_00581af8[compare->useStart + 1] =
                            CodeGen_ReachingDefTable_00581af8[signExtend->useStart + 1];
                        PCode_UnlinkInstruction(signExtend);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

unsigned int fold_not_into_andc(PCodeInstruction *pc, UInt32 mask)
{
    PCodeInstruction *def;
    PCodeInstruction **table = CodeGen_ReachingDefTable_00581af8;
    PCodeInstruction **source;

    def = table[pc->useStart + 2];
    if (def->opcode == PC_NOT) {
        if ((def->operandData.operands[0].value.reg) == pc->operandData.operands[0].value.reg ||
            !(mask & (1 << (def->operandData.operands[0].value.reg)))) {
            if (!(def->flags & fRecordBit)) {
                if (!PCode_DefinedBetween(pc, def, &def->operandData.operands[1]) &&
                    !PCode_UsedBetween(pc, def, &def->operandData.operands[0])) {
                    pc->operandData.operands[2].value.reg = def->operandData.operands[1].value.reg;
                    table = CodeGen_ReachingDefTable_00581af8;
                    source = CodeGen_ReachingDefTable_00581af8;
                    table[pc->useStart + 2] = source[def->useStart + 1];
                    pc->opcode = PC_ANDC;
                    PCode_UnlinkInstruction(def);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int replace_with_reaching_li(PCodeInstruction *instruction, int register_mask)
{
    PCodeInstruction *previous;

    previous = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
    if (previous->opcode == PC_LI &&
        (previous->operandData.operands[0].value.reg == instruction->operandData.operands[0].value.reg ||
         ((1 << previous->operandData.operands[0].value.reg) & register_mask) == 0) &&
        (instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0) {
        if (!PCode_UsedBetween(instruction, previous, &previous->operandData.operands[0])) {
            instruction->opcode = PC_LI;
            instruction->operandData.operands[1] = previous->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
                CodeGen_ReachingDefTable_00581af8[previous->useStart + 1];
            PCode_UnlinkInstruction(previous);
            return 1;
        }
    }
    return 0;
}

int merge_reaching_def_instruction(PCodeInstruction *instruction, int operand1, int operand2, int operand3,
                                   int registerMask)
{
    PCodeInstruction *next = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
    short opcode;

    if ((unsigned short)((opcode = next->opcode) - 350) <= 2 &&
        (next->operandData.operands[0].value.reg == instruction->operandData.operands[0].value.reg ||
         ((1 << next->operandData.operands[0].value.reg) & registerMask) == 0) &&
        (instruction->flags & fRecordBit) == 0) {
        if (!PCode_UsedBetween(instruction, next, &next->operandData.operands[0])) {
            instruction->opcode = opcode;
            instruction->operandData.operands[1] = next->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
                CodeGen_ReachingDefTable_00581af8[next->useStart + 1];
            PCode_UnlinkInstruction(next);
            return 1;
        }
    }
    return 0;
}

unsigned int retarget_reaching_def_register(PCodeInstruction *instruction, UInt32 unavailableRegisters,
                                            SInt32 registerClass)
{
    PCodeInstruction *reachingDef = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];

    if (reachingDef->flags & fIsRead) {
        if (((1 << reachingDef->operandData.operands[0].value.reg) & unavailableRegisters) == 0) {
            if ((instruction->flags & fRecordBit) == 0) {
                if (!PCode_UsedBetween(instruction, reachingDef, &reachingDef->operandData.operands[0])) {
                    if (!PCode_UsedBetween(instruction, reachingDef, &instruction->operandData.operands[0])) {
                        if (!PCode_DefinedBetween(instruction, reachingDef, &instruction->operandData.operands[0])) {
                            reachingDef->operandData.operands[0].value.reg =
                                instruction->operandData.operands[0].value.reg;
                            PCode_UnlinkInstruction(instruction);
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

unsigned int rewrite_reaching_def_reg(PCodeInstruction *instruction, SInt32 registerClass, UInt32 unavailableRegisters)
{
    PCodeInstruction *definition = CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];

    if (definition->flags & fIsRead) {
        if (((1 << definition->operandData.operands[0].value.reg) & unavailableRegisters) == 0) {
            if ((instruction->flags & fRecordBit) == 0) {
                if (!PCode_UsedBetween(instruction, definition, &definition->operandData.operands[0])) {
                    if (!PCode_UsedBetween(instruction, definition, &instruction->operandData.operands[0])) {
                        if (!PCode_DefinedBetween(instruction, definition, &instruction->operandData.operands[0])) {
                            definition->operandData.operands[0].value.reg =
                                instruction->operandData.operands[0].value.reg;
                            PCode_UnlinkInstruction(instruction);
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int eliminate_redundant_store(PCodeInstruction *in)
{
#if VERSION == VERSION_GC_1_0
    PCodeInstruction *def;
    int isUpdate;
    PCodeInstruction *cursor;
    int isVector;
    int isFloat;
    int storeSize;
    SInt32 loadOffset;
    SInt32 storeOffset;
    int operandSize;
#else
    PCodeInstruction *cursor;
    int isVector;
    int isFloat;
    int storeSize;
    SInt32 loadOffset;
    SInt32 storeOffset;
    int operandSize;
    int isUpdate;
    PCodeInstruction *def;
#endif

    def = CodeGen_ReachingDefTable_00581af8[in->useStart];
    if (in->flags & 0x28400)
        return 0;
    if (def->flags & 0x28400)
        return 0;

    if ((def->flags & fIsRead) && def->operandData.operands[1].kind == PCOp_GPR &&
        def->operandData.operands[1].value.reg == in->operandData.operands[1].value.reg &&
        def->operandData.operands[2].kind == in->operandData.operands[2].kind) {
        if (!PCode_DefinedBetween(in, def, &in->operandData.operands[1])) {
            if (in->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                if (in->operandData.operands[2].value.signed_value != def->operandData.operands[2].value.signed_value)
                    return 0;
            } else if (in->operandData.operands[2].kind == PCOp_MEMORY) {
                if (in->operandData.operands[2].value.signed_value != def->operandData.operands[2].value.signed_value ||
                    in->operandData.operands[2].object != def->operandData.operands[2].object)
                    return 0;
            } else if (in->operandData.operands[2].kind == PCOp_GPR) {
                if (in->operandData.operands[2].value.reg != def->operandData.operands[2].value.reg ||
                    PCode_DefinedBetween(in, def, &in->operandData.operands[2]))
                    return 0;
            } else
                return 0;

            isUpdate = 0;
            isFloat = 0;
            isVector = 0;
            switch (def->opcode) {
                case PC_LBZX:
                    isUpdate = 1;
                case PC_LBZ:
                    operandSize = 1;
                    break;
                case PC_LHZX:
                case PC_LHAX:
                    isUpdate = 1;
                case PC_LHZ:
                case PC_LHA:
                    operandSize = 2;
                    break;
                case PC_LWZX:
                    isUpdate = 1;
                case PC_LWZ:
                    operandSize = 4;
                    break;
                case PC_LFSX:
                    isUpdate = 1;
                case PC_LFS:
                    isFloat = 1;
                    operandSize = 4;
                    break;
                case PC_LFDX:
                    isUpdate = 1;
                case PC_LFD:
                    isFloat = 1;
                    operandSize = 8;
                    break;
                case PC_LVX:
                case PC_LVXL:
                    isUpdate = 1;
                    isVector = 1;
                    operandSize = 16;
                    break;
                default:
                    return 0;
            }
            switch (in->opcode) {
                case PC_STBX:
                    if (!isUpdate)
                        return 0;
                case PC_STB:
                    if (isFloat)
                        return 0;
                    if (operandSize != 1)
                        return 0;
                    break;
                case PC_STHX:
                    if (!isUpdate)
                        return 0;
                case PC_STH:
                    if (isFloat)
                        return 0;
                    if (operandSize != 2)
                        return 0;
                    break;
                case PC_STWX:
                    if (!isUpdate)
                        return 0;
                case PC_STW:
                    if (isFloat)
                        return 0;
                    if (operandSize != 4)
                        return 0;
                    break;
                case PC_STFSX:
                    if (!isUpdate)
                        return 0;
                case PC_STFS:
                    if (!isFloat)
                        return 0;
                    if (operandSize != 4)
                        return 0;
                    break;
                case PC_STFDX:
                    if (!isUpdate)
                        return 0;
                case PC_STFD:
                    if (!isFloat)
                        return 0;
                    if (operandSize != 8)
                        return 0;
                    break;
                case PC_STVX:
                case PC_STVXL:
                    if (!isUpdate)
                        return 0;
                    if (!isVector)
                        return 0;
                    if (operandSize != 16)
                        return 0;
                    break;
                default:
                    return 0;
            }

            for (cursor = in->previous; cursor != NULL && cursor != def; cursor = cursor->previous) {
                if (cursor->flags & fIsWrite) {
                    if (cursor->operandData.operands[1].value.reg != in->operandData.operands[1].value.reg)
                        return 0;
                    if (cursor->operandData.operands[2].kind != def->operandData.operands[2].kind)
                        return 0;
                    if (cursor->operandData.operands[2].kind == PCOp_MEMORY) {
                        if (in->operandData.operands[2].object == cursor->operandData.operands[2].object) {
                            if (in->operandData.operands[2].value.signed_value ==
                                (loadOffset = def->operandData.operands[2].value.signed_value))
                                return 0;
                            storeOffset = cursor->operandData.operands[2].value.signed_value;
                        }
                    } else if (cursor->operandData.operands[2].kind == PCOp_IMMEDIATE) {
                        if (in->operandData.operands[1].value.reg != cursor->operandData.operands[1].value.reg)
                            return 0;
                        if (in->operandData.operands[2].value.signed_value ==
                            (storeOffset = cursor->operandData.operands[2].value.signed_value))
                            return 0;
                        loadOffset = def->operandData.operands[2].value.signed_value;
                    } else
                        return 0;
                    switch (cursor->opcode) {
                        case PC_STB:
                        case PC_STBX:
                            storeSize = 1;
                            break;
                        case PC_STH:
                        case PC_STHX:
                            storeSize = 2;
                            break;
                        case PC_STW:
                        case PC_STWX:
                        case PC_STFS:
                        case PC_STFSX:
                            storeSize = 4;
                            break;
                        case PC_STFD:
                        case PC_STFDX:
                            storeSize = 8;
                            break;
                        case PC_STVX:
                        case PC_STVXL:
                            storeSize = 16;
                            break;
                        default:
                            return 0;
                    }
                    if (loadOffset > storeOffset) {
                        if (storeOffset + storeSize > loadOffset)
                            return 0;
                    } else {
                        if (loadOffset + operandSize > storeOffset)
                            return 0;
                    }
                }
            }
            PCode_UnlinkInstruction(in);
            return 1;
        }
    }
    return 0;
}

int fold_rlwinm_or_mr_reaching_def(PCodeInstruction *pc, UInt32 mask)
{
    SInt16 maskStart, maskEnd;
    PCodeInstruction *def;

    def = CodeGen_ReachingDefTable_00581af8[pc->useStart + 1];
    if (def->opcode == PC_RLWINM && (def->flags & fRecordBit) == 0 &&
        !PCode_DefinedBetween(pc, def, &def->operandData.operands[1]) &&
        (def->operandData.operands[0].value.reg != def->operandData.operands[1].value.reg ||
         ((def->operandData.operands[0].value.reg == pc->operandData.operands[0].value.reg ||
           (mask & (1 << def->operandData.operands[0].value.reg)) == 0) &&
          !PCode_UsedBetween(pc, def, &def->operandData.operands[0]))) &&
        canmergemasks(def->operandData.operands[3].value.signed_value, def->operandData.operands[4].value.signed_value,
                      pc->operandData.operands[2].value.signed_value, pc->operandData.operands[3].value.signed_value,
                      pc->operandData.operands[4].value.signed_value, &maskStart, &maskEnd)) {
        if (pc->opcode == PC_RLWIMI && (maskStart != pc->operandData.operands[3].value.signed_value ||
                                        maskEnd != pc->operandData.operands[4].value.signed_value))
            return 0;
        pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
        CodeGen_ReachingDefTable_00581af8[pc->useStart + 1] = CodeGen_ReachingDefTable_00581af8[def->useStart + 1];
        pc->operandData.operands[2].value.signed_value =
            (pc->operandData.operands[2].value.signed_value + def->operandData.operands[2].value.signed_value) & 0x1F;
        pc->operandData.operands[3].value.signed_value = maskStart;
        pc->operandData.operands[4].value.signed_value = maskEnd;
        if ((def->operandData.operands[0].value.reg == pc->operandData.operands[0].value.reg ||
             (mask & (1 << def->operandData.operands[0].value.reg)) == 0) &&
            !PCode_UsedBetween(pc, def, &def->operandData.operands[0]))
            PCode_UnlinkInstruction(def);
        return 1;
    }

    if (def->opcode == PC_MR && pc->opcode == PC_RLWINM) {
        if (!PCode_DefinedBetween(pc, def, &def->operandData.operands[1])) {
            pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
            CodeGen_ReachingDefTable_00581af8[pc->useStart + 1] = CodeGen_ReachingDefTable_00581af8[def->useStart + 1];
            if ((def->operandData.operands[0].value.reg == pc->operandData.operands[0].value.reg ||
                 (mask & (1 << def->operandData.operands[0].value.reg)) == 0) &&
                (def->flags & fRecordBit) == 0) {
                if (!PCode_UsedBetween(pc, def, &def->operandData.operands[0]))
                    PCode_UnlinkInstruction(def);
            }
            return 1;
        }
    }
    return 0;
}

int combine_mulli(struct PCodeInstruction *state, int register_mask)
{
    PCodeInstruction *instruction;

    instruction = CodeGen_ReachingDefTable_00581af8[state->useStart + 1];
    if (instruction->opcode == PC_MULLI &&
        (instruction->operandData.operands[0].value.reg == state->operandData.operands[0].value.reg ||
         (1 << instruction->operandData.operands[0].value.reg & register_mask) == 0) &&
        (instruction->flags & PCodeInstruction_CloneExtraOperandExcluded) == 0 &&
        !PCode_DefinedBetween(state, instruction, &instruction->operandData.operands[1]) &&
        !PCode_UsedBetween(state, instruction, &instruction->operandData.operands[0]) &&
        state->operandData.operands[2].value.signed_value * instruction->operandData.operands[2].value.signed_value ==
            (short)(state->operandData.operands[2].value.signed_value *
                    instruction->operandData.operands[2].value.signed_value)) {
        state->operandData.operands[1].value.reg = instruction->operandData.operands[1].value.reg;
        CodeGen_ReachingDefTable_00581af8[state->useStart + 1] =
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
        state->operandData.operands[2].value.signed_value =
            state->operandData.operands[2].value.signed_value * instruction->operandData.operands[2].value.signed_value;
        PCode_UnlinkInstruction(instruction);
        return 1;
    }
    return 0;
}

unsigned int combine_addi(PCodeInstruction *pc, UInt32 mask)
{
    PCodeInstruction *def;
    PCodeInstruction **table;
    PCodeInstruction **source;

    table = CodeGen_ReachingDefTable_00581af8;
    def = table[pc->useStart + 1];
    if (def->opcode == PC_ADDI) {
        if ((def->operandData.operands[0].value.reg) == pc->operandData.operands[0].value.reg ||
            !(mask & (1 << (def->operandData.operands[0].value.reg)))) {
            if (!(def->flags & fRecordBit)) {
                if (!PCode_DefinedBetween(pc, def, &def->operandData.operands[1])) {
                    if (!PCode_UsedBetween(pc, def, &def->operandData.operands[0])) {
                        if (pc->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                            def->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                            (pc->operandData.operands[2].value.signed_value +
                             def->operandData.operands[2].value.signed_value) ==
                                (short)(pc->operandData.operands[2].value.signed_value +
                                        def->operandData.operands[2].value.signed_value)) {
                            pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
                            table = CodeGen_ReachingDefTable_00581af8;
                            source = CodeGen_ReachingDefTable_00581af8;
                            table[pc->useStart + 1] = source[def->useStart + 1];
                            pc->operandData.operands[2].value.signed_value +=
                                def->operandData.operands[2].value.signed_value;
                            PCode_UnlinkInstruction(def);
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

unsigned int combine_srawi(PCodeInstruction *pc, UInt32 mask)
{
    PCodeInstruction *def;
    PCodeInstruction **table;
    PCodeInstruction **destination;

    table = CodeGen_ReachingDefTable_00581af8;

    def = table[pc->useStart + 1];
    if (def->opcode == PC_SRAWI) {
        if (def->operandData.operands[0].value.reg == pc->operandData.operands[0].value.reg ||
            !(mask & (1 << def->operandData.operands[0].value.reg))) {
            if (!(def->flags & fRecordBit)) {
                if (!PCode_DefinedBetween(pc, def, &def->operandData.operands[1])) {
                    if (!PCode_UsedBetween(pc, def, &def->operandData.operands[0])) {
                        if (pc->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                            def->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                            pc->operandData.operands[2].value.signed_value +
                                    def->operandData.operands[2].value.signed_value <
                                0x20 &&
                            pc->operandData.operands[2].value.signed_value +
                                    def->operandData.operands[2].value.signed_value >
                                0) {
                            pc->operandData.operands[1].value.reg = def->operandData.operands[1].value.reg;
                            table = CodeGen_ReachingDefTable_00581af8;
                            destination = CodeGen_ReachingDefTable_00581af8;
                            destination[pc->useStart + 1] = table[def->useStart + 1];
                            pc->operandData.operands[2].value.signed_value +=
                                def->operandData.operands[2].value.signed_value;
                            PCode_UnlinkInstruction(def);
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int rewrite_as_addi(PCodeInstruction *record)
{
    PCodeInstruction *link4 = record->previous;
    PCodeInstruction *link0 = record->next;
    unsigned int link4Matches = 0;
    unsigned int link0Matches = 0;
    if (link4 != NULL)
        link4Matches = ((link4->flags & 3) == 1);
    if (link0 != NULL)
        link0Matches = ((link0->flags & 3) == 1);
    if ((record->flags & fRecordBit) == 0 && record->operand_count >= 2 &&
        record->operandData.operands[1].value.reg != 0 && (link4Matches || link0Matches)) {
        record->operand_count = 3;
        record->opcode = PC_ADDI;
        record->operandData.operands[2].kind = PCOp_IMMEDIATE;
        record->operandData.operands[2].value.signed_value = 0;
        record->operandData.operands[2].object = NULL;
    }
    return 0;
}

int has_register_conflict(UInt32 mask, PCodeInstruction *group, PCodeInstruction *operand, PCodeInstruction *other,
                          PCodeInstruction *target)
{
    PCodeOperand *record;
    int remaining;
    PCodeInstruction *link;
    SInt16 operandId;
    SInt16 relatedId;

    if ((mask & (1 << target->operandData.operands[0].value.reg)) != 0 &&
        target->operandData.operands[0].value.reg != group->operandData.operands[0].value.reg &&
        target->operandData.operands[0].value.reg != operand->operandData.operands[2].value.reg)
        return 1;

    for (link = group->block->instructions; link != group->block->reverse_instructions; link = link->next) {
        if (link == operand || link == other || link == target)
            break;
    }

    operandId = operand->operandData.operands[1].value.reg;
    relatedId = target->operandData.operands[1].value.reg;
    while (link != group) {
        record = group->operandData.operands;
        remaining = group->operand_count;
        while (remaining--) {
            if (record->kind == PCOp_GPR &&
                (((record->value.reg == operandId || record->value.reg == relatedId) && (record->flags & 2)) ||
                 (record->value.reg == target->operandData.operands[0].value.reg && (record->flags & 1)))) {
                if (link != operand && link != other && link != target)
                    return 1;
            }
            record++;
        }
        link = link->next;
    }
    return 0;
}

unsigned int fold_to_rlwnm(PCodeInstruction *instruction, unsigned int liveRegisters)
{
    PCodeInstruction *first;
    PCodeInstruction *second;
    int index;
    PCodeInstruction *shift;

    index = instruction->useStart;
    first = CodeGen_ReachingDefTable_00581af8[index + 1];
    second = CodeGen_ReachingDefTable_00581af8[index + 2];

    if ((instruction->flags & fRecordBit) != 0)
        return 0;

    if (((1 << instruction->operandData.operands[1].value.reg) & liveRegisters) != 0 &&
        instruction->operandData.operands[1].value.reg !=
            instruction->operandData.operands[0].value.reg)
        return 0;
    if (((1 << instruction->operandData.operands[2].value.reg) & liveRegisters) != 0 &&
        instruction->operandData.operands[1].value.reg !=
            instruction->operandData.operands[0].value.reg)
        return 0;

    if (first->opcode != 0x6b && first->opcode != 0x6a)
        return 0;
    if (second->opcode != 0x6b && second->opcode != 0x6a)
        return 0;

    if (PCode_UsedBetween(instruction, first, &first->operandData.operands[0]))
        return 0;
    if (PCode_UsedBetween(instruction, second, &second->operandData.operands[0]))
        return 0;

    if (first->opcode == 0x6b && second->opcode == 0x6a &&
        first->operandData.operands[1].value.reg == second->operandData.operands[1].value.reg) {
        shift = CodeGen_ReachingDefTable_00581af8[first->useStart + 2];
        if (shift->opcode == PC_SUBFIC &&
            shift->operandData.operands[1].value.reg == second->operandData.operands[2].value.reg &&
            shift->operandData.operands[2].value.unsigned_value == 0x20) {
            if (has_register_conflict(liveRegisters, (PCodeInstruction *)instruction, second, first, shift) != 0)
                return 0;
            instruction->opcode = PC_RLWNM;
            instruction->operandData.operands[1] = first->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
                CodeGen_ReachingDefTable_00581af8[first->useStart + 1];
            instruction->operandData.operands[2] = second->operandData.operands[2];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 2] =
                CodeGen_ReachingDefTable_00581af8[second->useStart + 2];
            instruction->operandData.operands[3].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[3].value.signed_value = 0;
            instruction->operandData.operands[3].object = NULL;
            instruction->operandData.operands[4].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[4].value.signed_value = 0x1f;
            instruction->operandData.operands[4].object = NULL;
            PCode_UnlinkInstruction(first);
            PCode_UnlinkInstruction(second);
            PCode_UnlinkInstruction(shift);
            return 1;
        }
        shift = CodeGen_ReachingDefTable_00581af8[second->useStart + 2];
        if (shift->opcode == PC_SUBFIC &&
            shift->operandData.operands[1].value.reg == first->operandData.operands[2].value.reg &&
            shift->operandData.operands[2].value.unsigned_value == 0x20) {
            if (has_register_conflict(liveRegisters, (PCodeInstruction *)instruction, second, first, shift) != 0)
                return 0;
            instruction->opcode = PC_RLWNM;
            instruction->operandData.operands[1] = first->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
                CodeGen_ReachingDefTable_00581af8[first->useStart + 1];
            instruction->operandData.operands[2] = second->operandData.operands[2];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 2] =
                CodeGen_ReachingDefTable_00581af8[second->useStart + 2];
            instruction->operandData.operands[3].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[3].value.signed_value = 0;
            instruction->operandData.operands[3].object = NULL;
            instruction->operandData.operands[4].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[4].value.signed_value = 0x1f;
            instruction->operandData.operands[4].object = NULL;
            PCode_UnlinkInstruction(first);
            PCode_UnlinkInstruction(second);
            return 1;
        }
    } else if (first->opcode == 0x6a && second->opcode == 0x6b &&
               first->operandData.operands[1].value.reg == second->operandData.operands[1].value.reg) {
        shift = CodeGen_ReachingDefTable_00581af8[first->useStart + 2];
        if (shift->opcode == PC_SUBFIC &&
            shift->operandData.operands[1].value.reg == second->operandData.operands[2].value.reg &&
            shift->operandData.operands[2].value.unsigned_value == 0x20) {
            if (has_register_conflict(liveRegisters, (PCodeInstruction *)instruction, first, second, shift) != 0)
                return 0;
            instruction->opcode = PC_RLWNM;
            instruction->operandData.operands[1] = first->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
                CodeGen_ReachingDefTable_00581af8[first->useStart + 1];
            instruction->operandData.operands[2] = first->operandData.operands[2];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 2] =
                CodeGen_ReachingDefTable_00581af8[first->useStart + 2];
            instruction->operandData.operands[3].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[3].value.signed_value = 0;
            instruction->operandData.operands[3].object = NULL;
            instruction->operandData.operands[4].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[4].value.signed_value = 0x1f;
            instruction->operandData.operands[4].object = NULL;
            PCode_UnlinkInstruction(first);
            PCode_UnlinkInstruction(second);
            return 1;
        }
        shift = CodeGen_ReachingDefTable_00581af8[second->useStart + 2];
        if (shift->opcode == PC_SUBFIC &&
            shift->operandData.operands[1].value.reg == first->operandData.operands[2].value.reg &&
            shift->operandData.operands[2].value.unsigned_value == 0x20) {
            if (has_register_conflict(liveRegisters, (PCodeInstruction *)instruction, first, second, shift) != 0)
                return 0;
            instruction->opcode = PC_RLWNM;
            instruction->operandData.operands[1] = first->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1] =
                CodeGen_ReachingDefTable_00581af8[first->useStart + 1];
            instruction->operandData.operands[2] = first->operandData.operands[2];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 2] =
                CodeGen_ReachingDefTable_00581af8[first->useStart + 2];
            instruction->operandData.operands[3].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[3].value.signed_value = 0;
            instruction->operandData.operands[3].object = NULL;
            instruction->operandData.operands[4].kind = PCOp_IMMEDIATE;
            instruction->operandData.operands[4].value.signed_value = 0x1f;
            instruction->operandData.operands[4].object = NULL;
            PCode_UnlinkInstruction(first);
            PCode_UnlinkInstruction(second);
            PCode_UnlinkInstruction(shift);
            return 1;
        }
    }
    return 0;
}

SInt32 fold_rlwimi_store_to_stwbrx(PCodeInstruction *node, UInt32 mask)
{
    PCodeInstruction *nodes[4];
    PCodeInstruction *n;
    SInt32 i;
    SInt32 flags;

    flags = 0;

    if (node->opcode == PC_STW && node->operandData.operands[2].kind != PCOp_IMMEDIATE)
        return 0;

    i = 0;
    n = node;
    do {
        if (n->opcode == PC_RLWINM)
            nodes[i] = CodeGen_ReachingDefTable_00581af8[n->useStart + 1];
        else
            nodes[i] = CodeGen_ReachingDefTable_00581af8[n->useStart];
        n = nodes[i];
        if (nodes[0]->operandData.operands[1].value.reg != nodes[i]->operandData.operands[1].value.reg)
            return 0;
        if (i < 3) {
            if (nodes[i]->opcode != PC_RLWIMI)
                return 0;
        } else {
            if (nodes[i]->opcode != PC_RLWINM)
                return 0;
        }
        if (nodes[i]->operandData.operands[2].value.signed_value == 8) {
            if (nodes[i]->operandData.operands[3].value.signed_value == 0x18 &&
                nodes[i]->operandData.operands[4].value.signed_value == 0x1f) {
                if (flags & 1)
                    return 0;
                flags |= 1;
            } else if (nodes[i]->operandData.operands[3].value.signed_value == 8 &&
                       nodes[i]->operandData.operands[4].value.signed_value == 0xf) {
                if (flags & 4)
                    return 0;
                flags |= 4;
            } else
                return 0;
        } else if (nodes[i]->operandData.operands[2].value.signed_value == 0x18) {
            if (nodes[i]->operandData.operands[3].value.signed_value == 0 &&
                nodes[i]->operandData.operands[4].value.signed_value == 7) {
                if (flags & 8)
                    return 0;
                flags |= 8;
            } else if (nodes[i]->operandData.operands[3].value.signed_value == 0x10 &&
                       nodes[i]->operandData.operands[4].value.signed_value == 0x17) {
                if (flags & 2)
                    return 0;
                flags |= 2;
            } else
                return 0;
        } else
            return 0;
        i++;
    } while (i < 4);

    if (PCode_DefinedBetween(node, nodes[3], &nodes[0]->operandData.operands[1]))
        return 0;

    if (node->opcode == PC_STWX) {
        node->opcode = PC_STWBRX;
        node->operandData.operands[0] = nodes[0]->operandData.operands[1];
        CodeGen_ReachingDefTable_00581af8[node->useStart] = CodeGen_ReachingDefTable_00581af8[nodes[3]->useStart + 1];
        return 1;
    }

    if (node->opcode == PC_STW) {
        SInt32 k = node->operandData.operands[2].value.signed_value;
        if (k != 0 && ((1 << nodes[0]->operandData.operands[0].value.reg) & mask))
            return 0;
        node->opcode = PC_STWBRX;
        node->operandData.operands[0] = nodes[0]->operandData.operands[1];
        CodeGen_ReachingDefTable_00581af8[node->useStart] = CodeGen_ReachingDefTable_00581af8[nodes[3]->useStart + 1];
        if (k != 0) {
            nodes[0]->opcode = PC_ADDI;
            nodes[0]->operand_count = 3;
            nodes[0]->operandData.operands[1].kind = PCOp_GPR;
            nodes[0]->operandData.operands[1].kind = PCOp_GPR;
            nodes[0]->operandData.operands[1].value.reg = node->operandData.operands[1].value.reg;
            nodes[0]->operandData.operands[1].flags = (SInt8)node->operandData.operands[1].flags;
            nodes[0]->operandData.operands[2].kind = PCOp_IMMEDIATE;
            nodes[0]->operandData.operands[2].kind = PCOp_IMMEDIATE;
            nodes[0]->operandData.operands[2].value.signed_value = k;
            node->operandData.operands[1].kind = PCOp_GPR;
            node->operandData.operands[1].value.reg = 0;
            node->operandData.operands[1].flags = 0;
            node->operandData.operands[2].kind = PCOp_GPR;
            node->operandData.operands[2].value.reg = nodes[0]->operandData.operands[0].value.reg;
            node->operandData.operands[2].flags = (SInt8)nodes[0]->operandData.operands[0].flags;
            CodeGen_ReachingDefTable_00581af8[node->useStart + 2] = nodes[0];
            PCode_UnlinkInstruction(nodes[1]);
            PCode_UnlinkInstruction(nodes[2]);
            PCode_UnlinkInstruction(nodes[3]);
        } else {
            node->operandData.operands[2] = node->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[node->useStart + 2] =
                CodeGen_ReachingDefTable_00581af8[node->useStart + 1];
            node->operandData.operands[1].kind = PCOp_GPR;
            node->operandData.operands[1].value.reg = 0;
            node->operandData.operands[1].flags = 0;
        }
        return 1;
    } else
        return 0;
}

int fold_rlwimi_rlwinm_to_sthbrx(PCodeInstruction *instruction, int mask)
{
    PCodeInstruction *nodes[2];
    PCodeInstruction *n;
    SInt32 i;
    SInt32 flags;

    flags = 0;

    if (instruction->opcode == PC_STH && instruction->operandData.operands[2].kind != PCOp_IMMEDIATE)
        return 0;

    i = 0;
    n = instruction;
    do {
        if (n->opcode == PC_RLWINM)
            nodes[i] = CodeGen_ReachingDefTable_00581af8[n->useStart + 1];
        else
            nodes[i] = CodeGen_ReachingDefTable_00581af8[n->useStart];
        n = nodes[i];
        if (nodes[0]->operandData.operands[1].value.reg != nodes[i]->operandData.operands[1].value.reg)
            return 0;
        if (i < 1) {
            if (nodes[i]->opcode != PC_RLWIMI)
                return 0;
        } else {
            if (nodes[i]->opcode != PC_RLWINM)
                return 0;
        }
        if (nodes[i]->operandData.operands[2].value.signed_value == 8) {
            if (nodes[i]->operandData.operands[3].value.signed_value == 0x10 &&
                nodes[i]->operandData.operands[4].value.signed_value == 0x17) {
                if (flags & 2)
                    return 0;
                flags |= 2;
            } else
                return 0;
        } else if (nodes[i]->operandData.operands[2].value.signed_value == 0x18) {
            if (nodes[i]->operandData.operands[3].value.signed_value == 0x18 &&
                nodes[i]->operandData.operands[4].value.signed_value == 0x1f) {
                if (flags & 1)
                    return 0;
                flags |= 1;
            } else
                return 0;
        } else
            return 0;
        i++;
    } while (i < 2);

    if (PCode_DefinedBetween(instruction, nodes[1], &nodes[0]->operandData.operands[1]))
        return 0;

    if (instruction->opcode == PC_STHX) {
        instruction->opcode = PC_STHBRX;
        instruction->operandData.operands[0] = nodes[0]->operandData.operands[1];
        CodeGen_ReachingDefTable_00581af8[instruction->useStart] =
            CodeGen_ReachingDefTable_00581af8[nodes[1]->useStart + 1];
        return 1;
    }

    if (instruction->opcode == PC_STH) {
        SInt32 k = instruction->operandData.operands[2].value.signed_value;
        if (k != 0 && ((1 << nodes[0]->operandData.operands[0].value.reg) & mask))
            return 0;
        instruction->opcode = PC_STHBRX;
        instruction->operandData.operands[0] = nodes[0]->operandData.operands[1];
        CodeGen_ReachingDefTable_00581af8[instruction->useStart] =
            CodeGen_ReachingDefTable_00581af8[nodes[1]->useStart + 1];
        if (k != 0) {
            nodes[0]->opcode = PC_ADDI;
            nodes[0]->operand_count = 3;
            nodes[0]->operandData.operands[1].kind = PCOp_GPR;
            nodes[0]->operandData.operands[1].kind = PCOp_GPR;
            nodes[0]->operandData.operands[1].value.reg = instruction->operandData.operands[1].value.reg;
            nodes[0]->operandData.operands[1].flags = (SInt8)instruction->operandData.operands[1].flags;
            nodes[0]->operandData.operands[2].kind = PCOp_IMMEDIATE;
            nodes[0]->operandData.operands[2].kind = PCOp_IMMEDIATE;
            nodes[0]->operandData.operands[2].value.signed_value = k;
            instruction->operandData.operands[1].kind = PCOp_GPR;
            instruction->operandData.operands[1].value.reg = 0;
            instruction->operandData.operands[1].flags = 0;
            instruction->operandData.operands[2].kind = PCOp_GPR;
            instruction->operandData.operands[2].value.reg = nodes[0]->operandData.operands[0].value.reg;
            instruction->operandData.operands[2].flags = (SInt8)nodes[0]->operandData.operands[0].flags;
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 2] = nodes[0];
            PCode_UnlinkInstruction(nodes[1]);
        } else {
            instruction->operandData.operands[2] = instruction->operandData.operands[1];
            CodeGen_ReachingDefTable_00581af8[instruction->useStart + 2] =
                CodeGen_ReachingDefTable_00581af8[instruction->useStart + 1];
            instruction->operandData.operands[1].kind = PCOp_GPR;
            instruction->operandData.operands[1].value.reg = 0;
            instruction->operandData.operands[1].flags = 0;
        }
        return 1;
    } else
        return 0;
}

void peephole_optimize_block(PCodeBlock *scope)
{
    UInt32 a, b, c, d;
    UInt32 i;
    Boolean flag;
    PCodeInstruction *block;

    flag = 0;
    a = register_block_liveness[scope->index].live_out;
    b = gRegisterBlockLiveness[scope->index].live_out;
    c = registerBlockLiveness[scope->index].live_out;
    d = data_005813ac[scope->index].live_out;

    for (block = scope->reverse_instructions; block != NULL; block = block->previous) {
        PeepHandler *handler;
        PCodeOperand *p;

        if (fn_004cc040(block, a, b, c, d, flag) != 0) {
            PCode_UnlinkInstruction(block);
            continue;
        }
        handler = CodeGen_PeepholeHandlers_005813b0[block->opcode];
        while (handler != NULL) {
            if (handler->func(block, a, b, c, d) != 0) {
                if (block->block == NULL)
                    break;
                handler = CodeGen_PeepholeHandlers_005813b0[block->opcode];
            } else {
                handler = handler->next;
            }
        }
        if (block->block != NULL) {
            p = block->operandData.operands;
            i = block->operand_count;
            while (i--) {
                if (p->kind == PCOp_GPR && (p->flags & 2))
                    a &= ~(1 << p->value.reg);
                else if (p->kind == PCOp_FPR && (p->flags & 2))
                    b &= ~(1 << p->value.reg);
                else if (p->kind == PCOp_CRFIELD && (p->flags & 2))
                    c &= ~(1 << p->value.reg);
                else if (p->kind == PCOp_VR && (p->flags & 2))
                    d &= ~(1 << p->value.reg);
                else if (p->kind == PCOp_SPR && p->value.reg == 0 && (p->flags & 2))
                    flag = 0;
                p++;
            }
            p = block->operandData.operands;
            i = block->operand_count;
            while (i--) {
                if (p->kind == PCOp_GPR && (p->flags & 1))
                    a |= 1 << p->value.reg;
                else if (p->kind == PCOp_FPR && (p->flags & 1))
                    b |= 1 << p->value.reg;
                else if (p->kind == PCOp_CRFIELD && (p->flags & 1))
                    c |= 1 << p->value.reg;
                else if (p->kind == PCOp_VR && (p->flags & 1))
                    d |= 1 << p->value.reg;
                else if (p->kind == PCOp_SPR && p->value.reg == 0 && (p->flags & 1))
                    flag = 1;
                p++;
            }
        }
    }
}

SInt32 compute_register_mask(PCodeInstruction *node, SInt16 registerNumber)
{
    PCodeOperand *entry;
    SInt32 result = -1;
    SInt32 entryCount;

    while (node != NULL) {
        entryCount = node->operand_count;
        entry = node->operandData.operands;
        while (entryCount--) {
            if (entry->kind == PCOp_GPR && entry->value.reg == registerNumber && (entry->flags & 2)) {
                switch (node->opcode) {
                    case PC_LBZ:
                    case PC_LBZU:
                    case PC_LBZX:
                    case PC_LBZUX:
                        result = 0xff;
                        break;
                    case PC_LHZ:
                    case PC_LHZU:
                    case PC_LHZX:
                    case PC_LHZUX:
                        result = 0xffff;
                        break;
                    case PC_LI:
                        result = node->operandData.operands[1].value.immediate_value;
                        break;
                    case PC_SRAWI:
                        result = compute_register_mask(node->previous, node->operandData.operands[1].value.reg) >>
                                 node->operandData.operands[2].value.immediate_value;
                        break;
                    case PC_RLWINM: {
                        UInt32 value = compute_register_mask(node->previous, node->operandData.operands[1].value.reg);
                        UInt32 afterEnd, fromStart, mask;
                        SInt32 rotateCount = node->operandData.operands[2].value.immediate_value;
                        result = value >> (32 - rotateCount);
                        result |= value << rotateCount;
                        if (node->operandData.operands[3].value.immediate_value <=
                            node->operandData.operands[4].value.immediate_value) {
                            if (node->operandData.operands[4].value.immediate_value + 1 > 31)
                                afterEnd = 0;
                            else
                                afterEnd = 0xffffffffu >> (node->operandData.operands[4].value.immediate_value + 1);
                            if (node->operandData.operands[3].value.immediate_value > 31)
                                fromStart = 0;
                            else
                                fromStart = 0xffffffffu >> node->operandData.operands[3].value.immediate_value;
                            mask = ~afterEnd & fromStart;
                        } else {
                            if (node->operandData.operands[4].value.immediate_value + 1 > 31)
                                afterEnd = 0;
                            else
                                afterEnd = 0xffffffffu >> (node->operandData.operands[4].value.immediate_value + 1);
                            if (node->operandData.operands[3].value.immediate_value > 31)
                                fromStart = 0;
                            else
                                fromStart = 0xffffffffu >> node->operandData.operands[3].value.immediate_value;
                            mask = ~afterEnd | fromStart;
                        }
                        result &= mask;
                        break;
                    }
                    case PC_RLWIMI: {
                        UInt32 value = compute_register_mask(node->previous, node->operandData.operands[1].value.reg);
                        UInt32 afterEnd, fromStart, mask;
                        SInt32 rotateCount = node->operandData.operands[2].value.immediate_value;
                        result = value >> (32 - rotateCount);
                        result |= value << rotateCount;
                        if (node->operandData.operands[3].value.immediate_value <=
                            node->operandData.operands[4].value.immediate_value) {
                            if (node->operandData.operands[4].value.immediate_value + 1 > 31)
                                afterEnd = 0;
                            else
                                afterEnd = 0xffffffffu >> (node->operandData.operands[4].value.immediate_value + 1);
                            if (node->operandData.operands[3].value.immediate_value > 31)
                                fromStart = 0;
                            else
                                fromStart = 0xffffffffu >> node->operandData.operands[3].value.immediate_value;
                            mask = ~afterEnd & fromStart;
                        } else {
                            if (node->operandData.operands[4].value.immediate_value + 1 > 31)
                                afterEnd = 0;
                            else
                                afterEnd = 0xffffffffu >> (node->operandData.operands[4].value.immediate_value + 1);
                            if (node->operandData.operands[3].value.immediate_value > 31)
                                fromStart = 0;
                            else
                                fromStart = 0xffffffffu >> node->operandData.operands[3].value.immediate_value;
                            mask = ~afterEnd | fromStart;
                        }
                        result &= mask;
                        result |= compute_register_mask(node->previous, node->operandData.operands[0].value.reg);
                        break;
                    }
                    case PC_OR:
                        result = compute_register_mask(node->previous, node->operandData.operands[2].value.reg);
                        result |= compute_register_mask(node->previous, node->operandData.operands[1].value.reg);
                        break;
                    case PC_ORI:
                        result = compute_register_mask(node->previous, node->operandData.operands[1].value.reg) |
                                 node->operandData.operands[2].value.immediate_value;
                        break;
                    case PC_AND:
                        result = compute_register_mask(node->previous, node->operandData.operands[2].value.reg);
                        result &= compute_register_mask(node->previous, node->operandData.operands[1].value.reg);
                        break;
                    case PC_ANDI:
                        result = compute_register_mask(node->previous, node->operandData.operands[1].value.reg) &
                                 node->operandData.operands[2].value.immediate_value;
                        break;
                    case PC_MR:
                        result = compute_register_mask(node->previous, node->operandData.operands[1].value.reg);
                        break;
                }
                return result;
            }
            entry++;
        }
        node = node->previous;
    }
    return -1;
}

unsigned int make_contiguous_mask(unsigned int value)
{
    unsigned int result;
    unsigned int bit;
    unsigned int remainingMask;

    bit = 1;
    remainingMask = 0xffffffff;
    result = 0;
    if (((value & 1) != 0) && ((value & 0x80000000) != 0)) {
        result = 0xffffffff;
        for (; (value & bit) == 1; bit = bit << 1) {
        }
        for (; (value & bit) == 0; bit = bit << 1) {
            result = result & ~bit;
        }
        return result;
    }
    for (; ((value & bit) == 0 && ((value & remainingMask) != 0)); remainingMask = remainingMask << 1) {
        bit = bit << 1;
    }
    for (; (value & remainingMask) != 0; remainingMask = remainingMask << 1) {
        result = result | bit;
        bit = bit << 1;
    }
    return result;
}

void optimize_rlwinm_and_addi(PCodeBlock *block)
{
    PCodeInstruction *instr;
    PCodeInstruction *scan;
    PCodeInstruction *previousInstruction;
    PCodeOperand *operand;
    PCodeOperand *scanOperands;
    int operandCount;
    SInt16 addRegister;
    SInt32 addend;
    char registerUsed;
    char passedNonAdd;

    instr = block->instructions;
    while (instr) {
        if (instr->opcode == PC_RLWINM) {
            SInt16 resultRegister;
            SInt32 sourceRegister;
            SInt32 maskBegin;
            UInt32 maskEnd;
            SInt16 mergedBegin;
            SInt16 mergedEnd;
            SInt16 conditionRegisterUsed = 0;
            SInt16 sourceRegisterChanged = 0;
            PCodeInstruction *clone;
            UInt32 scanMaskBegin;

            resultRegister = instr->operandData.operands[0].value.reg;
            sourceRegister = instr->operandData.operands[1].value.reg;
            maskBegin = instr->operandData.operands[3].value.signed_value;
            maskEnd = instr->operandData.operands[4].value.signed_value;

            for (scan = instr->next; scan; scan = scan->next) {
                scanOperands = scan->operandData.operands;
                if (scan->opcode == PC_RLWINM && scanOperands[1].value.reg == resultRegister) {
                    if ((scanMaskBegin = scanOperands[3].value.signed_value) == (maskBegin = maskBegin) &&
                        scanOperands[4].value.signed_value == maskEnd && scanOperands[2].value.signed_value == 0) {
                        if (scan->flags & fRecordBit) {
                            if (!conditionRegisterUsed) {
                                PCodeUtilities_MakeRecordForm(instr);
                                scan->flags &= ~fRecordBit;
                                scan->flags |= fIsMove;
                                scan->opcode = PC_MR;
                                scan->operand_count = 2;
                            } else {
                                scan->flags |= fIsMove;
                                scan->opcode = PC_MR;
                                scan->operand_count = 3;
                                scan->operandData.operands[2] = scan->operandData.operands[5];
                            }
                        } else {
                            scan->flags |= fIsMove;
                            scan->opcode = PC_MR;
                            scan->operand_count = 2;
                        }
                    } else if (resultRegister != sourceRegister && !sourceRegisterChanged &&
                               canmergemasks(maskBegin, maskEnd, scanOperands[2].value.signed_value, scanMaskBegin,
                                             scanOperands[4].value.signed_value, &mergedBegin, &mergedEnd)) {
                        scan->operandData.operands[1].value.reg = sourceRegister;
                        scan->operandData.operands[2].value.signed_value =
                            (scan->operandData.operands[2].value.signed_value +
                             instr->operandData.operands[2].value.signed_value) &
                            31;
                        scan->operandData.operands[3].value.signed_value = mergedBegin;
                        scan->operandData.operands[4].value.signed_value = mergedEnd;
                    }
                } else if (scan->opcode == PC_SRAWI && scanOperands[1].value.reg == resultRegister &&
                           resultRegister != sourceRegister && instr->operandData.operands[2].value.signed_value == 0 &&
                           !(compute_register_mask(instr, resultRegister) & 0x80000000) && !sourceRegisterChanged &&
                           canmergemasks(maskBegin, maskEnd, 32 - scanOperands[2].value.signed_value,
                                         scanOperands[2].value.signed_value, 31, &mergedBegin, &mergedEnd) &&
                           !has_reg_flag_one_before_flag_two(scan, 0)) {
                    PCode_InsertInstructionAfter(
                        scan, PCodeUtilities_CreateInstruction(0x67, scanOperands[0].value.reg, sourceRegister,
                                                               32 - scanOperands[2].value.signed_value, mergedBegin,
                                                               mergedEnd));
                    if (scan->flags & fRecordBit)
                        PCodeUtilities_MakeRecordForm(scan->next);
                    PCode_UnlinkInstruction(scan);
                } else if (scan->opcode == PC_OR && !sourceRegisterChanged && resultRegister != sourceRegister &&
                           !(scan->flags & fRecordBit) && !(instr->flags & fRecordBit) &&
                           scan->operandData.operands[0].value.reg != instr->operandData.operands[1].value.reg) {
                    if (scanOperands[1].value.reg == resultRegister &&
                        masks_disjoint(instr, scan, scanOperands[2].value.reg)) {
                        scan->opcode = PC_MR;
                        scan->flags |= fIsMove;
                        scan->operand_count = 2;
                        scan->operandData.operands[1] = scan->operandData.operands[2];
                        clone = PCode_CloneInstruction(instr);
                        clone->opcode = PC_RLWIMI;
                        clone->operandData.operands[0] = scan->operandData.operands[0];
                        clone->operandData.operands[0].flags |= 1;
                        if (InstrSelection_GetMaskRange(make_contiguous_mask(compute_register_mask(
                                                            instr, instr->operandData.operands[0].value.reg)),
                                                        &mergedBegin, &mergedEnd)) {
                            clone->operandData.operands[3].value.signed_value = mergedBegin;
                            clone->operandData.operands[4].value.signed_value = mergedEnd;
                        }
                        PCode_InsertInstructionAfter(scan, clone);
                        break;
                    }
                    if (scanOperands[2].value.reg == resultRegister &&
                        masks_disjoint(instr, scan, scanOperands[1].value.reg)) {
                        scan->opcode = PC_MR;
                        scan->flags |= fIsMove;
                        scan->operand_count = 2;
                        clone = PCode_CloneInstruction(instr);
                        clone->opcode = PC_RLWIMI;
                        clone->operandData.operands[0] = scan->operandData.operands[0];
                        clone->operandData.operands[0].flags |= 1;
                        if (InstrSelection_GetMaskRange(make_contiguous_mask(compute_register_mask(
                                                            instr, instr->operandData.operands[0].value.reg)),
                                                        &mergedBegin, &mergedEnd)) {
                            clone->operandData.operands[3].value.signed_value = mergedBegin;
                            clone->operandData.operands[4].value.signed_value = mergedEnd;
                        }
                        PCode_InsertInstructionAfter(scan, clone);
                        break;
                    }
                } else if (!sourceRegisterChanged && resultRegister != sourceRegister && maskEnd == 31 &&
                           instr->operandData.operands[2].value.signed_value == 0 &&
                           (((scan->opcode == PC_STB || scan->opcode == PC_STBX) && maskBegin <= 24) ||
                            ((scan->opcode == PC_STH || scan->opcode == PC_STHX) && maskBegin <= 16)) &&
                           scan->operandData.operands[0].value.reg == resultRegister) {
                    scan->operandData.operands[0].value.reg = sourceRegister;
                }

                operand = scan->operandData.operands;
                operandCount = scan->operand_count;
                while (operandCount--) {
                    if (operand->kind == PCOp_GPR && operand->value.reg == resultRegister && (operand->flags & 2)) {
                        scan = block->reverse_instructions;
                        break;
                    }
                    if (operand->kind == PCOp_GPR && operand->value.reg == sourceRegister && (operand->flags & 2))
                        sourceRegisterChanged = 1;
                    if (operand->kind == PCOp_CRFIELD && operand->value.reg == 0)
                        conditionRegisterUsed = 1;
                    operand++;
                }
            }
        } else if (instr->opcode == PC_ADDI &&
                   instr->operandData.operands[0].value.reg == instr->operandData.operands[1].value.reg &&
                   instr->operandData.operands[2].kind == PCOp_IMMEDIATE) {
            registerUsed = 0;
            addend = instr->operandData.operands[2].value.signed_value;
            passedNonAdd = 0;
            addRegister = instr->operandData.operands[0].value.reg;
            for (scan = instr->next; scan; scan = scan->next) {
                if ((scan->flags & fIsWrite) && scan->operandData.operands[0].value.reg == addRegister)
                    break;
                if ((scan->flags & (fIsRead | fIsWrite)) && scan->operandData.operands[1].value.reg == addRegister &&
                    scan->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                    addend + scan->operandData.operands[2].value.signed_value ==
                        (SInt16)(addend + scan->operandData.operands[2].value.signed_value)) {
                    scan->operandData.operands[2].value.signed_value += addend;
                    previousInstruction = instr->previous;
                    if ((scan->flags & fIsRead) && scan->operandData.operands[0].value.reg == addRegister &&
                        scan->operandData.operands[0].kind == PCOp_GPR) {
                        PCode_UnlinkInstruction(instr);
                    } else {
                        PCode_UnlinkInstruction(instr);
                        PCode_InsertInstructionAfter(scan, instr);
                    }
                    instr = previousInstruction;
                    break;
                }
                if (scan->opcode == PC_ADDI && scan->operandData.operands[1].value.reg == addRegister &&
                    scan->operandData.operands[2].kind == PCOp_IMMEDIATE &&
                    addend + scan->operandData.operands[2].value.signed_value ==
                        (SInt16)(addend + scan->operandData.operands[2].value.signed_value)) {
                    scan->operandData.operands[2].value.signed_value += addend;
                    previousInstruction = instr->previous;
                    if (scan->operandData.operands[0].value.reg == addRegister) {
                        PCode_UnlinkInstruction(instr);
                    } else {
                        PCode_UnlinkInstruction(instr);
                        PCode_InsertInstructionAfter(scan, instr);
                    }
                    instr = previousInstruction;
                    break;
                }
                if (scan->flags & 0x24) {
                    if (passedNonAdd && scan->previous != instr) {
                        previousInstruction = instr->previous;
                        PCode_UnlinkInstruction(instr);
                        PCode_InsertInstructionBefore(scan, instr);
                        instr = previousInstruction;
                    }
                    break;
                }
                operand = scan->operandData.operands;
                operandCount = scan->operand_count;
                while (operandCount--) {
                    if (operand->kind == PCOp_GPR && operand->value.reg == addRegister && (operand->flags & 3)) {
                        if (passedNonAdd && scan->previous != instr) {
                            previousInstruction = instr->previous;
                            PCode_UnlinkInstruction(instr);
                            PCode_InsertInstructionBefore(scan, instr);
                            instr = previousInstruction;
                        }
                        registerUsed = 1;
                        break;
                    }
                    operand++;
                }
                if (registerUsed)
                    break;
                if (scan->opcode != PC_ADDI)
                    passedNonAdd = 1;
                if (passedNonAdd && !scan->next) {
                    previousInstruction = instr->previous;
                    PCode_UnlinkInstruction(instr);
                    PCode_AppendInstruction(block, instr);
                    instr = previousInstruction;
                    break;
                }
            }
        }
        if (instr)
            instr = instr->next;
        else
            instr = block->instructions;
    }
}

static int masks_disjoint(PCodeInstruction *instr, PCodeInstruction *scan, SInt16 reg)
{
    UInt32 mask;

    mask = make_contiguous_mask(compute_register_mask(instr, instr->operandData.operands[0].value.reg));
    return (compute_register_mask(scan, reg) & mask) == 0;
}

void register_peephole_rules(void)
{
    int opcode;
    for (opcode = 0; opcode < 466; opcode++)
        CodeGen_PeepholeHandlers_005813b0[opcode] = NULL;

    AddPeepholeRule(92, (PeepholeRuleProc)fold_not_into_andc);
    AddPeepholeRule(139, (PeepholeRuleProc)replace_with_reaching_li);
    AddPeepholeRule(139, (PeepholeRuleProc)retarget_reaching_def_register);
    AddPeepholeRule(158, (PeepholeRuleProc)rewrite_reaching_def_reg);
    AddPeepholeRule(139, (PeepholeRuleProc)unlink_instruction_with_matching_mr_def);
    AddPeepholeRule(139, (PeepholeRuleProc)unlink_equal_reg_instruction);
    AddPeepholeRule(158, (PeepholeRuleProc)unlink_instruction_for_unmatched_fmr);
    AddPeepholeRule(158, (PeepholeRuleProc)unlink_same_reg_instruction);
    AddPeepholeRule(398, (PeepholeRuleProc)fn_004cba60);
    AddPeepholeRule(398, (PeepholeRuleProc)unlink_instruction_matching_vmr_source);
    AddPeepholeRule(398, (PeepholeRuleProc)unlink_same_reg_move);
    AddPeepholeRule(398, (PeepholeRuleProc)merge_reaching_def_instruction);
    AddPeepholeRule(82, (PeepholeRuleProc)make_record_form_and_unlink_instruction);
    AddPeepholeRule(105, (PeepholeRuleProc)fold_rlwinm_or_mr_reaching_def);
    AddPeepholeRule(103, (PeepholeRuleProc)fold_rlwinm_or_mr_reaching_def);
    AddPeepholeRule(103, (PeepholeRuleProc)bypass_extsb_for_low_byte_rotated_mask);
    AddPeepholeRule(103, bypass_extsh_for_rotated_mask);
    AddPeepholeRule(103, (PeepholeRuleProc)fold_lbz_lbzx_mask);
    AddPeepholeRule(103, (PeepholeRuleProc)fold_lhz_lhzx_mask);
    AddPeepholeRule(101, (PeepholeRuleProc)fold_reaching_lha_or_extsb);
    AddPeepholeRule(49, fold_rlwimi_store_to_stwbrx);
    AddPeepholeRule(51, fold_rlwimi_store_to_stwbrx);
    AddPeepholeRule(44, (PeepholeRuleProc)fold_rlwimi_rlwinm_to_sthbrx);
    AddPeepholeRule(46, (PeepholeRuleProc)fold_rlwimi_rlwinm_to_sthbrx);
    AddPeepholeRule(21, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(25, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(29, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(34, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(40, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(44, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(49, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(142, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(146, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(150, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(154, (PeepholeRuleProc)fold_li_operand);
    AddPeepholeRule(21, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(25, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(29, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(34, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(40, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(44, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(49, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(142, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(146, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(150, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(154, (PeepholeRuleProc)fold_addi_or_mr_reaching_def);
    AddPeepholeRule(22, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(26, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(30, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(35, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(41, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(45, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(50, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(143, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(147, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(151, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(155, (PeepholeRuleProc)fold_reaching_addi);
    AddPeepholeRule(21, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(25, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(29, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(34, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(40, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(44, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(49, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(142, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(146, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(150, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(154, (PeepholeRuleProc)eliminate_matching_addi);
    AddPeepholeRule(40, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(44, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(49, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(150, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(154, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(42, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(46, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(51, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(152, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(156, (PeepholeRuleProc)eliminate_redundant_store);
    AddPeepholeRule(5, remove_redundant_extsb);
    AddPeepholeRule(8, remove_redundant_extsb);
    AddPeepholeRule(5, (PeepholeRuleProc)bypass_cmpli_zero);
    AddPeepholeRule(8, (PeepholeRuleProc)bypass_cmpli_zero);
    AddPeepholeRule(5, fold_constant_compare_branch);
    AddPeepholeRule(8, fold_constant_compare_branch);
    AddPeepholeRule(103, (PeepholeRuleProc)fold_rlwinm_or_mr_reaching_def);
    AddPeepholeRule(73, (PeepholeRuleProc)combine_mulli);
    AddPeepholeRule(63, (PeepholeRuleProc)combine_addi);
    AddPeepholeRule(108, (PeepholeRuleProc)combine_srawi);
    AddPeepholeRule(139, (PeepholeRuleProc)rewrite_as_addi);
    AddPeepholeRule(93, (PeepholeRuleProc)fold_to_rlwnm);
}

void Peephole_VisitBlocksWithMultipleInstructions(void *context)
{
    PCodeBlock *block;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        if (block->instruction_count >= 2) {
            optimize_rlwinm_and_addi(block);
        }
    }
}

void Peephole_MergeAdjacentBlocks(Object *object, Boolean mergePrologue)
{
    PCodeBlock *block;
    PCodeBlock *nextBlock;
    PCodeInstruction *nextInsn;
    PCodeInstruction *savedNextInsn;
    PCodeInstruction *insn;
    PCodeBlockLink *successor;
    PCodeBlockLink *predecessor;
    Boolean nextIsPrologue;

    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        nextIsPrologue = 0;
        nextBlock = block->next;
        if (!mergePrologue) {
            nextIsPrologue = (nextBlock != NULL && (nextBlock->flags & 2) != 0);
            if (block->flags & 1)
                continue;
        }
        if (block->instruction_count <= 0)
            continue;
        for (insn = block->instructions; insn != NULL; insn = insn->next) {
            if (insn->flags & 0x420)
                break;
        }
        if (insn != NULL)
            continue;
        if (block->reverse_instructions != NULL && block->reverse_instructions->opcode == PC_B) {
            if (((PCodeBlockLink *)block->reverse_instructions->operandData.operands[0].value.unsigned_value)
                    ->payload.block != nextBlock)
                continue;
            PCode_UnlinkInstruction(block->reverse_instructions);
        }
        if (block->reverse_instructions != NULL && (block->reverse_instructions->flags & fIsBranch) != 0 &&
            block->reverse_instructions->opcode != PC_B)
            continue;
        while (block->successors->payload.block == nextBlock && block->successors->next == NULL &&
               nextBlock->predecessors->payload.block == block && nextBlock->predecessors->next == NULL &&
               !nextIsPrologue && (nextBlock->flags & 0x20) == 0 &&
               block->instruction_count + nextBlock->instruction_count <= 100) {
            if (nextBlock->instruction_count > 0) {
                nextInsn = nextBlock->instructions;
                while (nextInsn != NULL) {
                    savedNextInsn = nextInsn->next;
                    if (nextInsn->flags & 0x420)
                        break;
                    PCode_UnlinkInstruction(nextInsn);
                    if (nextInsn->opcode != PC_B ||
                        ((PCodeBlockLink *)nextInsn->operandData.operands[0].value.unsigned_value)->payload.block !=
                            nextBlock->next)
                        PCode_AppendInstruction(block, nextInsn);
                    nextInsn = savedNextInsn;
                }
            }
            if (nextBlock->instruction_count != 0 || nextBlock == gReturnBlock)
                break;
            block->successors = nextBlock->successors;
            for (successor = block->successors; successor != NULL; successor = successor->next)
                for (predecessor = successor->payload.block->predecessors; predecessor != NULL;
                     predecessor = predecessor->next)
                    if (predecessor->payload.block == nextBlock) {
                        predecessor->payload.block = block;
                        break;
                    }
            block->next = nextBlock->next;
            if (block->next != NULL)
                block->next->prev = block;
            nextBlock->flags |= 0x10;
            nextBlock = block->next;
            if (!mergePrologue)
                nextIsPrologue = (nextBlock != NULL && (nextBlock->flags & 2) != 0);
        }
    }
}

static void AddPeepholeRule(SInt16 opcode, void *handler)
{
    struct PeepHandler *rule = (struct PeepHandler *)lalloc(8);
    rule->func = handler;
    rule->next = CodeGen_PeepholeHandlers_005813b0[opcode];
    CodeGen_PeepholeHandlers_005813b0[opcode] = rule;
}

void Peephole_OptimizeBlocks(Object *object)
{
    PCodeBlock *block;

    register_peephole_rules();
    build_register_block_liveness(object);
    for (block = gPCodeBlocks; block != NULL; block = block->next) {
        if (1 <= block->instruction_count) {
            build_reaching_def_table(block);
            peephole_optimize_block(block);
            freeoheap();
        }
    }
}
