#define CERROR_FILE "LoopOptimization.c"
#include "compiler/common.h"
#include "compiler/LoopOptimization.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/BE_elf.h"
#include "compiler/BE_symbol.h"
#include "compiler/CError.h"
#include "compiler/CException.h"
#include "compiler/CExpr2.h"
#include "compiler/CFunc.h"
#include "compiler/CInline.h"
#include "compiler/CObjC.h"
#include "compiler/CObjCModern.h"
#include "compiler/CParser.h"
#include "compiler/CPrec.h"
#include "compiler/CPrep.h"
#include "compiler/CRTTI.h"
#include "compiler/CTemplateFunc.h"
#include "compiler/CTemplateNew.h"
#include "compiler/CTemplateTools.h"
#include "compiler/CodeGen.h"
#include "compiler/CodeMotion.h"
#include "compiler/CompilerTools.h"
#include "compiler/ConstantPropagation.h"
#include "compiler/DWARF.h"
#include "compiler/GlobalOptimizer.h"
#include "compiler/IROUseDef.h"
#include "compiler/InlineAsmPPC.h"
#include "compiler/InstrSelection.h"
#include "compiler/InterferenceGraph.h"
#include "compiler/Intrinsics.h"
#include "compiler/IroBitVect.h"
#include "compiler/IroCSE.h"
#include "compiler/IroJump.h"
#include "compiler/IroLoop.h"
#include "compiler/IroVars.h"
#include "compiler/LoopDetection.h"
#include "compiler/ObjGen_PPC_EABI.h"
#include "compiler/PCode.h"
#include "compiler/PCodeAssembly.h"
#include "compiler/PCodeUtilities.h"
#include "compiler/Registers.h"
#include "compiler/StrengthReduction.h"
#include "compiler/Switch.h"

static UInt32 *registers_used_outside_loop;
static UInt32 *self_addi_candidate_regs;
static SInt32 data_00582c70;

#define NULL 0

#define CE_ASSERT(c, s)                                                                                                \
    do {                                                                                                               \
        if (c)                                                                                                         \
            s;                                                                                                         \
    } while (0)

void fn_005289b0(void)
{
    UInt32 allocationSize;
    SInt32 registerCount;

    gLoopTransformChanged = 0;
    gArrayToRegisterEnabled = 0;
    gArrayToRegisterChanged = 0;
    if (data_0058763c != NULL) {
        COpt_SetLoopCodeMotionMode(0);
        registerCount = gUsedVirtualRegistersGPR;
        data_00582c70 = registerCount;
        allocationSize = ((registerCount + 31) >> 5) * sizeof(*registers_used_outside_loop);
        registers_used_outside_loop = CompilerTools_AllocatePoolMemory(allocationSize);
        registerCount = data_00582c70;
        allocationSize = ((registerCount + 31) >> 5) * sizeof(*self_addi_candidate_regs);
        self_addi_candidate_regs = CompilerTools_AllocatePoolMemory(allocationSize);
        walk_loop_children_postorder(data_0058763c);
        CompilerTools_ResetPool();
    }
}

static inline LoopVar *findarray(LoopVar *a, Object *o)
{
    for (; a; a = a->next)
        if (a->object == o)
            return a;
    return NULL;
}

void walk_loop_children_postorder(register Loop *loop)
{
    register Loop *childLoop, *grandchildLoop, *thirdLevelLoop, *fourthLevelLoop;
    register Loop *fifthLevelLoop, *sixthLevelLoop, *seventhLevelLoop;

    for (; loop != NULL; loop = loop->sibling) {
        if (loop->children != NULL) {
            for (childLoop = loop->children; childLoop != NULL; childLoop = childLoop->sibling) {
                if (childLoop->children != NULL) {
                    for (grandchildLoop = childLoop->children; grandchildLoop != NULL;
                         grandchildLoop = grandchildLoop->sibling) {
                        if (grandchildLoop->children != NULL) {
                            for (thirdLevelLoop = grandchildLoop->children; thirdLevelLoop != NULL;
                                 thirdLevelLoop = thirdLevelLoop->sibling) {
                                if (thirdLevelLoop->children != NULL) {
                                    for (fourthLevelLoop = thirdLevelLoop->children; fourthLevelLoop != NULL;
                                         fourthLevelLoop = fourthLevelLoop->sibling) {
                                        if (fourthLevelLoop->children != NULL) {
                                            for (fifthLevelLoop = fourthLevelLoop->children; fifthLevelLoop != NULL;
                                                 fifthLevelLoop = fifthLevelLoop->sibling) {
                                                if (fifthLevelLoop->children != NULL) {
                                                    for (sixthLevelLoop = fifthLevelLoop->children;
                                                         sixthLevelLoop != NULL;
                                                         sixthLevelLoop = sixthLevelLoop->sibling) {
                                                        if (sixthLevelLoop->children != NULL) {
                                                            for (seventhLevelLoop = sixthLevelLoop->children;
                                                                 seventhLevelLoop != NULL;
                                                                 seventhLevelLoop = seventhLevelLoop->sibling) {
                                                                if (seventhLevelLoop->children != NULL)
                                                                    walk_loop_children_postorder(
                                                                        seventhLevelLoop->children);
                                                                dispatch_counting_loop_transforms(seventhLevelLoop);
                                                            }
                                                        }
                                                        dispatch_counting_loop_transforms(sixthLevelLoop);
                                                    }
                                                }
                                                dispatch_counting_loop_transforms(fifthLevelLoop);
                                            }
                                        }
                                        dispatch_counting_loop_transforms(fourthLevelLoop);
                                    }
                                }
                                dispatch_counting_loop_transforms(thirdLevelLoop);
                            }
                        }
                        dispatch_counting_loop_transforms(grandchildLoop);
                    }
                }
                dispatch_counting_loop_transforms(childLoop);
            }
        }
        dispatch_counting_loop_transforms(loop);
    }
}

void COpt_ArrayToRegister(void)
{
    PCodeInstruction *newpc;
    LoopVar *head = NULL;
    PCodeBlock *block;
    PCodeInstruction *pc;
    long remaining;
    LoopVar *a, *best;
    PCodeOperand *op;
    LoopVar **link;
    int n, g, f, min, reg;
    int minf;
    int j;
    long src;
    head = build_array_loopvars();
    if (head) {
        for (block = gPCodeBlocks; block; block = block->next) {
            for (pc = block->instructions; pc; pc = pc->next) {
                if (!(pc->flags & fIsBranch) && pc->operand_count) {
                    for (op = pc->operandData.operands, remaining = pc->operand_count; remaining--; op++) {
                        if (op->kind == PCOp_MEMORY && op->flags == 1) {
                            a = findarray(head, op->object);
                            if (a && !a->isvalid) {
                                if ((pc->flags & (fIsRead | fIsWrite)) &&
                                    (long)op->value.signed_value % a->elemSize == 0 &&
                                    (long)op->value.signed_value < a->arraySize) {
                                    switch (pc->opcode) {
                                        case PC_LBZ:
                                        case PC_STB:
                                            if (a->elemSize != 1)
                                                a->isvalid = 1;
                                            break;
                                        case PC_LHZ:
                                        case PC_LHA:
                                        case PC_STH:
                                            if (a->elemSize != 2)
                                                a->isvalid = 1;
                                            break;
                                        case PC_LWZ:
                                        case PC_STW:
                                            if (a->elemSize != 4)
                                                a->isvalid = 1;
                                            break;
                                        case PC_LFD:
                                        case PC_STFD:
                                            if (a->elemSize != 8)
                                                a->isvalid = 1;
                                            break;
                                        default:
                                            a->isvalid = 1;
                                    }
                                    if (!a->isvalid)
                                        a->values[op->value.signed_value / a->elemSize]++;
                                } else
                                    a->isvalid = 1;
                            }
                        }
                    }
                }
            }
        }
        n = 0;
        g = 0;
        f = 0;
        link = &head;
        a = head;
        while (a) {
            if (a->isvalid) {
                *link = a->next;
                a = *link;
            } else {
                n++;
                if (a->isfloat)
                    f += a->count;
                else
                    g += a->count;
                for (j = 0; j < a->count; j++)
                    a->extra += a->values[j];
                a = a->next;
            }
        }
        if (head) {
            while (g > 8) {
                min = 0;
                best = NULL;
                for (a = head; a; a = a->next) {
                    if (!a->isfloat) {
                        if (best) {
                            if (a->extra < min) {
                                min = a->extra;
                                best = a;
                            }
                        } else {
                            best = a;
                            min = a->extra;
                        }
                    }
                }
                if (!best)
                    break;
                if (best == head)
                    head = best->next;
                else {
                    for (a = head; a; a = a->next) {
                        if (a->next == best) {
                            a->next = best->next;
                            break;
                        }
                    }
                }
                g -= best->count;
                n--;
            }
            while (f > 8) {
                minf = 0;
                best = NULL;
                for (a = head; a; a = a->next) {
                    if (a->isfloat) {
                        if (best) {
                            if (a->extra < minf) {
                                minf = a->extra;
                                best = a;
                            }
                        } else {
                            best = a;
                            minf = a->extra;
                        }
                    }
                }
                if (!best)
                    break;
                if (best == head)
                    head = best->next;
                else {
                    for (a = head; a; a = a->next) {
                        if (a->next == best) {
                            a->next = best->next;
                            break;
                        }
                    }
                }
                f -= best->count;
                n--;
            }
            if (g > 8 || f > 8)
                CError_FATAL(2315);
            a = head;
            if (!a)
                return;
            gArrayToRegisterChanged = 1;
            for (; a; a = a->next) {
                for (j = 0; j < a->count; j++) {
                    if (a->isfloat)
                        a->values[j] = gUsedVirtualRegistersFPR++;
                    else
                        a->values[j] = gUsedVirtualRegistersGPR++;
                }
            }
            for (block = gPCodeBlocks; block; block = block->next) {
                for (pc = block->instructions; pc; pc = pc->next) {
                    if (!(pc->flags & fIsBranch) && pc->operand_count && (pc->flags & (fIsRead | fIsWrite)) &&
                        pc->operandData.operands[2].kind == PCOp_MEMORY && pc->operandData.operands[2].flags == 1) {
                        a = findarray(head, pc->operandData.operands[2].object);
                        if (a && !a->isvalid) {
                            reg = a->values[pc->operandData.operands[2].value.signed_value / a->elemSize];
                            newpc = NULL;
                            src = pc->operandData.operands[0].value.reg;
                            switch (pc->opcode) {
                                case PC_LBZ:
                                    if (a->isgpr)
                                        newpc = PCodeUtilities_CreateInstruction(0x8b, src, reg);
                                    else
                                        newpc = PCodeUtilities_CreateInstruction(0x67, src, reg, 0, 24, 31);
                                    break;
                                case PC_STB:
                                    if (a->isgpr)
                                        newpc = PCodeUtilities_CreateInstruction(0x64, reg, src);
                                    else
                                        newpc = PCodeUtilities_CreateInstruction(0x67, reg, src, 0, 24, 31);
                                    break;
                                case PC_LHZ:
                                    newpc = PCodeUtilities_CreateInstruction(0x67, src, reg, 0, 16, 31);
                                    break;
                                case PC_LHA:
                                    newpc = PCodeUtilities_CreateInstruction(0x65, src, reg);
                                    break;
                                case PC_STH:
                                    if (a->isgpr)
                                        newpc = PCodeUtilities_CreateInstruction(0x65, reg, src);
                                    else
                                        newpc = PCodeUtilities_CreateInstruction(0x67, reg, src, 0, 16, 31);
                                    break;
                                case PC_LWZ:
                                    newpc = PCodeUtilities_CreateInstruction(0x8b, src, reg);
                                    break;
                                case PC_STW:
                                    newpc = PCodeUtilities_CreateInstruction(0x8b, reg, src);
                                    break;
                                case PC_LFD:
                                    newpc = PCodeUtilities_CreateInstruction(0x9e, src, reg);
                                    break;
                                case PC_STFD:
                                    newpc = PCodeUtilities_CreateInstruction(0x9e, reg, src);
                                    break;
                                default:
                                    CError_FATAL(2415);
                            }
                            if (newpc) {
                                PCode_InsertInstructionBefore(pc, newpc);
                                PCode_UnlinkInstruction(pc);
                                pc = newpc;
                            }
                        }
                    }
                }
            }
        }
    }
    CompilerTools_ResetPool();
}

LoopVar *build_array_loopvars(void)
{
    LoopVar *head = NULL;
    LoopVar *node;
    SInt32 arraySize;
    ObjectList *list;
    SInt32 count;
    SInt32 elemSize;
    SInt32 isFloat;
    int i;
    UInt32 qual;

    for (list = locals; list != NULL; list = list->next) {
        if (list->object == NULL)
            continue;
        if (list->object->type->type == TYPEPOINTER)
            qual = TYPE_POINTER(list->object->type)->qual;
        else
            qual = list->object->qual;
        qual = qual & Q_VOLATILE;
        if (qual != 0)
            continue;
        if (list->object->type == NULL)
            continue;
        if (list->object->type->type != TYPEARRAY)
            continue;
        if (TYPE_POINTER(list->object->type)->target == NULL)
            continue;
        if ((SInt8)TYPE_POINTER(list->object->type)->target->type >= TYPESTRUCT)
            continue;
        arraySize = list->object->type->size;
        elemSize = TYPE_POINTER(list->object->type)->target->size;
        count = arraySize / elemSize;
        if (count <= 0)
            continue;
        if (count > 8)
            continue;
        node = (LoopVar *)CompilerTools_AllocatePoolMemory(sizeof(LoopVar) + (count - 1) * sizeof(node->values[0]));
        node->next = head;
        head = node;
        node->object = list->object;
        node->elemSize = elemSize;
        node->arraySize = arraySize;
        node->count = arraySize / elemSize;
        node->extra = 0;
        node->isgpr = 1;
        isFloat = !copts.operandsDebug && TYPE_POINTER(list->object->type)->target->type == TYPEFLOAT;
        node->isfloat = isFloat;
        node->isvalid = 0;
        node->isgpr = 1;
        if (!node->isfloat && Type_IsUnsigned(TYPE_POINTER(list->object->type)->target))
            node->isgpr = 0;
        for (i = 0; i < count; i++)
            node->values[i] = 0;
    }
    return head;
}

void dispatch_counting_loop_transforms(Loop *loop)
{
    SInt32 n;

    mark_registers_used_outside_loop(loop);
    if (loop->has_indexed_load) {
    } else if (loop->isKnownCountingLoop) {
    }
    if (loop->isKnownCountingLoop) {
        if (loop->iterationCount > 0) {
            fn_0052b1a0(loop);
            if (!copts.uniformSpillBlockWeight && !loop->has_call && !loop->has_memory_barrier) {
                if (loop->skip_leaf_pass_4f)
                    unroll_counting_loop(loop);
                else if (!loop->uses_count_register)
                    unroll_loop_by_factor(loop);
            }
        }
        if ((n = loop->iterationCount)) {
            if (n == 1)
                unlink_loop_body_edge_and_instructions(loop);
            else if (!loop->uses_count_register && !loop->has_call)
                convert_to_count_register_loop(loop);
        }
        gLoopTransformChanged = 1;
    } else if (loop->isUnknownCountingLoop && !loop->uses_count_register && !loop->has_call) {
        convert_loop_to_count_register(loop);
        if (copts.ppcUnrollSpeculative && !copts.uniformSpillBlockWeight && loop->skip_leaf_pass_4f &&
            !loop->has_memory_barrier && !loop->has_block_flag_40)
            unroll_ctr_loop(loop);
        gLoopTransformChanged = 1;
    }
    remove_unused_self_addi(loop);
}
#define HIGH_PART(v) ((SInt16)(((v) >> 16) + (((v) >> 15) & 1)))
#define LOW_PART(v) ((SInt16)(v))
#define FITS_IN_SHORT(v) ((v) == LOW_PART(v))
#define FITS_IN_USHORT(v) ((v) == (UInt16)(v))

void convert_loop_to_count_register(struct Loop *loop)
{
    int unrollFactor;
    int value;
    PCodeInstruction *branch;
    PCodeInstruction *lis;
    int value1;
    int branchOpcode;
    int branchCondition;
    int counterReg;
    int reg1;
    int reg2;
    int tmpReg;
    UInt8 mode;
    UInt8 shift;
    PCodeInstruction *instr;
    PCodeInstruction *high;
    int lo;
    PCodeInstruction *pc;
    PCodeBlock *block;
    PCodeBlockLink *link;
    PCodeBlockLink **ptr;
    int n;
    int ok;
    PCodeInstruction *tail;
    PCodeBlock *labelBlock;

    unrollFactor = abs(loop->step);
    block = NULL;
    tail = (PCodeInstruction *)loop->body->reverse_instructions;
    instr = tail->previous;
    while (instr->opcode == PC_ADDI) {
        PCode_UnlinkInstruction(instr);
        tail = (PCodeInstruction *)loop->body->reverse_instructions;
        if (tail->operandData.operands[2].value.label->target.block->instructions) {
            tail = (PCodeInstruction *)loop->body->reverse_instructions;
            PCode_InsertInstructionBefore(tail->operandData.operands[2].value.label->target.block->instructions, instr);
        } else {
            tail = (PCodeInstruction *)loop->body->reverse_instructions;
            PCode_AppendInstruction(tail->operandData.operands[2].value.label->target.block, instr);
        }
        block = insert_block_after(loop->body, loop->execution_weight);
        PCode_AppendInstruction(block, PCode_CloneInstruction(instr));
        loop->footer = block;
        tail = (PCodeInstruction *)loop->body->reverse_instructions;
        instr = tail->previous;
    }

    if (loop->unknownCondition == ELESS) {
        branchOpcode = 8;
        if (loop->lowerType == 1) {
            branchCondition = 1;
            value1 = loop->lower;
            reg1 = instr->operandData.operands[2].value.reg;
            value = unrollFactor - 1 - value1;
            mode = 0;
        } else if (loop->upperType == 1) {
            branchCondition = 0;
            value1 = loop->upper;
            reg1 = instr->operandData.operands[1].value.reg;
            value = unrollFactor - 1 + value1;
            mode = 1;
        } else {
            branchCondition = 0;
            value = unrollFactor - 1;
            reg1 = instr->operandData.operands[1].value.reg;
            reg2 = instr->operandData.operands[2].value.reg;
            mode = 2;
        }
    } else if (loop->unknownCondition == ELESSEQU) {
        branchOpcode = 5;
        if (loop->lowerType == 1) {
            branchCondition = 0;
            value1 = loop->lower;
            reg1 = instr->operandData.operands[2].value.reg;
            value = unrollFactor - value1;
            mode = 0;
        } else if (loop->upperType == 1) {
            branchCondition = 1;
            value1 = loop->upper;
            reg1 = instr->operandData.operands[1].value.reg;
            value = value1 + unrollFactor;
            mode = 1;
        } else {
            branchCondition = 1;
            value1 = 0;
            reg1 = instr->operandData.operands[1].value.reg;
            value = unrollFactor;
            mode = 2;
            reg2 = instr->operandData.operands[2].value.reg;
        }
    } else if (loop->unknownCondition == EGREATER) {
        branchOpcode = 8;
        if (loop->lowerType == 1) {
            branchCondition = 0;
            value1 = loop->lower;
            reg1 = instr->operandData.operands[2].value.reg;
            value = unrollFactor - 1 + value1;
            mode = 1;
        } else if (loop->upperType == 1) {
            branchCondition = 1;
            value1 = loop->upper;
            reg1 = instr->operandData.operands[1].value.reg;
            value = unrollFactor - 1 - value1;
            mode = 0;
        } else {
            branchCondition = 1;
            value1 = 0;
            reg1 = instr->operandData.operands[1].value.reg;
            value = unrollFactor - 1;
            reg2 = instr->operandData.operands[2].value.reg;
            mode = 3;
        }
    } else if (loop->unknownCondition == EGREATEREQU) {
        branchOpcode = 5;
        if (loop->lowerType == 1) {
            branchCondition = 1;
            value1 = loop->lower;
            reg1 = instr->operandData.operands[2].value.reg;
            value = value1 + unrollFactor;
            mode = 1;
        } else if (loop->upperType == 1) {
            branchCondition = 0;
            value1 = loop->upper;
            reg1 = instr->operandData.operands[1].value.reg;
            value = unrollFactor - value1;
            mode = 0;
        } else {
            branchCondition = 0;
            reg1 = instr->operandData.operands[1].value.reg;
            reg2 = instr->operandData.operands[2].value.reg;
            value = unrollFactor;
            mode = 3;
        }
    } else if (loop->unknownCondition == ENOTEQU) {
        branchOpcode = 5;
        branchCondition = 2;
        if (loop->step > 0) {
            if (loop->lowerType == 1) {
                value1 = loop->lower;
                reg1 = instr->operandData.operands[2].value.reg;
                value = unrollFactor - 1 - value1;
                mode = 0;
            } else if (loop->upperType == 1) {
                value1 = loop->upper;
                reg1 = instr->operandData.operands[1].value.reg;
                value = unrollFactor - 1 + value1;
                mode = 1;
            } else {
                reg1 = instr->operandData.operands[1].value.reg;
                value = unrollFactor - 1;
                mode = 2;
                reg2 = instr->operandData.operands[2].value.reg;
            }
        } else {
            if (loop->lowerType == 1) {
                value1 = loop->lower;
                reg1 = instr->operandData.operands[2].value.reg;
                value = unrollFactor - 1 + value1;
                mode = 1;
            } else if (loop->upperType == 1) {
                value1 = loop->upper;
                reg1 = instr->operandData.operands[1].value.reg;
                value = unrollFactor - 1 - value1;
                mode = 0;
            } else {
                reg1 = instr->operandData.operands[1].value.reg;
                value = unrollFactor - 1;
                reg2 = instr->operandData.operands[2].value.reg;
                mode = 3;
            }
        }
    }

    for (;;) {
        CError_ASSERT(1778, pc = (PCodeInstruction *)loop->body->instructions);
        if (pc->opcode == PC_CMP || pc->opcode == PC_CMPI || pc->opcode == PC_CMPLI || pc->opcode == PC_CMPL)
            break;
        PCode_UnlinkInstruction(pc);
        PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, pc);
        loop->bodySize--;
    }

    counterReg = gUsedVirtualRegistersGPR++;
    if (mode == 1) {
        if (value == 0) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4b, counterReg, reg1));
        } else if (FITS_IN_SHORT(value)) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4f, counterReg, reg1, value));
        } else {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x8a, counterReg, 0, HIGH_PART(value)));
            if (LOW_PART(value))
                PCode_InsertInstructionBefore(
                    loop->preheader->reverse_instructions,
                    PCodeUtilities_CreateInstruction(0x3f, counterReg, counterReg, 0, LOW_PART(value)));
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4c, counterReg, reg1, counterReg));
        }
    } else if (mode == 0) {
        if (value == 0) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x8b, counterReg, reg1));
        } else if (FITS_IN_SHORT(value)) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x3f, counterReg, reg1, 0, value));
        } else {
            PCode_InsertInstructionBefore(
                loop->preheader->reverse_instructions,
                PCodeUtilities_CreateInstruction(0x42, counterReg, reg1, 0, HIGH_PART(value)));
            if (LOW_PART(value))
                PCode_InsertInstructionBefore(
                    loop->preheader->reverse_instructions,
                    PCodeUtilities_CreateInstruction(0x3f, counterReg, counterReg, 0, LOW_PART(value)));
        }
    } else if (mode == 2) {
        if (value == 0) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4c, counterReg, reg1, reg2));
        } else if (FITS_IN_SHORT(value)) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x3f, counterReg, reg2, 0, value));
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4c, counterReg, reg1, counterReg));
        } else {
            PCode_InsertInstructionBefore(
                loop->preheader->reverse_instructions,
                PCodeUtilities_CreateInstruction(0x42, counterReg, reg2, 0, HIGH_PART(value)));
            if (LOW_PART(value))
                PCode_InsertInstructionBefore(
                    loop->preheader->reverse_instructions,
                    PCodeUtilities_CreateInstruction(0x3f, counterReg, counterReg, 0, LOW_PART(value)));
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4c, counterReg, reg1, counterReg));
        }
    } else {
        if (value == 0) {
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4c, counterReg, reg2, reg1));
        } else {
            if (FITS_IN_SHORT(value)) {
                PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                              PCodeUtilities_CreateInstruction(0x3f, counterReg, reg1, 0, value));
            } else {
                PCode_InsertInstructionBefore(
                    loop->preheader->reverse_instructions,
                    PCodeUtilities_CreateInstruction(0x42, counterReg, reg1, 0, HIGH_PART(value)));
                if (LOW_PART(value))
                    PCode_InsertInstructionBefore(
                        loop->preheader->reverse_instructions,
                        PCodeUtilities_CreateInstruction(0x3f, counterReg, counterReg, 0, LOW_PART(value)));
            }
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x4c, counterReg, reg2, counterReg));
        }
    }

    if (unrollFactor > 1) {
        n = getbit(unrollFactor);
        ok = 0;
        if (n > 0 && n < 31)
            ok = 1;
        if ((shift = ok ? n : 0)) {
            PCode_InsertInstructionBefore(
                loop->preheader->reverse_instructions,
                PCodeUtilities_CreateInstruction(0x67, counterReg, counterReg, 32 - shift, shift, 31));
        } else {
            tmpReg = gUsedVirtualRegistersGPR++;
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x89, tmpReg, unrollFactor));
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x46, counterReg, counterReg, tmpReg));
        }
    }

    PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                  PCodeUtilities_CreateInstruction(0x78, counterReg));

    if (mode < 2) {
        if (instr->opcode == PC_CMPL || instr->opcode == PC_CMPLI) {
            if (FITS_IN_USHORT(value1)) {
                PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                              PCodeUtilities_CreateInstruction(0x54, 0, reg1, value1));
            } else {
                high = PCodeUtilities_CreateInstruction(0x8a, gUsedVirtualRegistersGPR++, 0, HIGH_PART(value1));
                PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, high);
                if ((SInt16)loop->iterationCount)
                    PCode_InsertInstructionAfter(
                        high,
                        PCodeUtilities_CreateInstruction(0x3f, high->operandData.operands[0].value.reg,
                                                         high->operandData.operands[0].value.reg, 0, LOW_PART(value1)));
                PCode_InsertInstructionBefore(
                    loop->preheader->reverse_instructions,
                    PCodeUtilities_CreateInstruction(0x55, 0, reg1, high->operandData.operands[0].value.reg));
            }
        } else {
            if (value1 == (lo = LOW_PART(value1))) {
                PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                              PCodeUtilities_CreateInstruction(0x52, 0, reg1, value1));
            } else {
                lis = PCodeUtilities_CreateInstruction(0x8a, gUsedVirtualRegistersGPR++, 0, HIGH_PART(value1));
                PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, lis);
                if ((SInt16)loop->iterationCount)
                    PCode_InsertInstructionAfter(
                        lis, PCodeUtilities_CreateInstruction(0x3f, lis->operandData.operands[0].value.reg,
                                                              lis->operandData.operands[0].value.reg, 0, lo));
                PCode_InsertInstructionBefore(
                    loop->preheader->reverse_instructions,
                    PCodeUtilities_CreateInstruction(0x53, 0, reg1, lis->operandData.operands[0].value.reg));
            }
        }
    } else {
        if (instr->opcode == PC_CMPL || instr->opcode == PC_CMPLI)
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x55, 0, reg1, reg2));
        else
            PCode_InsertInstructionBefore(loop->preheader->reverse_instructions,
                                          PCodeUtilities_CreateInstruction(0x53, 0, reg1, reg2));
    }

    if (!block)
        block = loop->body->next;
    labelBlock = (PCodeBlock *)block;
    pc = PCodeUtilities_CreateInstruction(branchOpcode, 0, branchCondition, labelBlock->labels);
    PCode_UnlinkInstruction(loop->preheader->reverse_instructions);
    PCode_AppendInstruction(loop->preheader, pc);
    tail = (PCodeInstruction *)loop->body->reverse_instructions;
    branch = PCodeUtilities_CreateInstruction(0xb, tail->operandData.operands[2].value.label);
    PCode_UnlinkInstruction(loop->body->instructions);
    PCode_UnlinkInstruction(loop->body->reverse_instructions);
    PCode_AppendInstruction(loop->body, branch);

    loop->preheader->successors = NULL;
    for (ptr = &loop->body->predecessors; (link = *ptr); ptr = &link->next) {
        if (link->payload.block == loop->preheader) {
            *ptr = link->next;
            break;
        }
    }

    link = CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    link->payload.block = loop->preheader->next;
    link->next = loop->preheader->successors;
    loop->preheader->successors = link;

    link = CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    link->payload.block = loop->preheader;
    link->next = loop->preheader->next->predecessors;
    loop->preheader->next->predecessors = link;

    link = CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    link->payload.block = block;
    link->next = loop->preheader->successors;
    loop->preheader->successors = link;

    link = CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    link->payload.block = loop->preheader;
    link->next = block->predecessors;
    block->predecessors = link;

    for (loop = loop->parent; loop; loop = loop->parent)
        loop->uses_count_register = 1;
}

void convert_to_count_register_loop(Loop *loop)
{
    PCodeInstruction *instruction;
    SInt16 lowPart = loop->iterationCount;

    if (loop->iterationCount != lowPart) {
        SInt16 highPart = (loop->iterationCount >> 16) + ((loop->iterationCount >> 15) & 1);
        instruction = PCodeUtilities_CreateInstruction(PC_LIS, gUsedVirtualRegistersGPR++, 0, highPart);
        PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, instruction);
        if ((SInt16)loop->iterationCount != 0)
            PCode_InsertInstructionAfter(
                instruction, PCodeUtilities_CreateInstruction(PC_ADDI, instruction->operandData.operands[0].value.reg,
                                                              instruction->operandData.operands[0].value.reg, 0,
                                                              (SInt16)loop->iterationCount));
    } else {
        instruction = PCodeUtilities_CreateInstruction(PC_LI, gUsedVirtualRegistersGPR++, loop->iterationCount);
        PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, instruction);
    }
    PCode_InsertInstructionBefore(
        loop->preheader->reverse_instructions,
        PCodeUtilities_CreateInstruction(PC_MTCTR, instruction->operandData.operands[0].value.reg));
    instruction = PCodeUtilities_CreateInstruction(
        PC_BDNZ, loop->body->reverse_instructions->operandData.operands[2].value.label);
    PCode_UnlinkInstruction(loop->body->instructions);
    PCode_UnlinkInstruction(loop->body->reverse_instructions);
    PCode_AppendInstruction(loop->body, instruction);
    for (loop = loop->parent; loop != NULL; loop = loop->parent)
        loop->uses_count_register = 1;
}

void unlink_loop_body_edge_and_instructions(Loop *p)
{
    PCodeBlockLink *node;
    PCodeBlockLink **pp;

    for (pp = &p->body->successors; (node = *pp) != NULL; pp = &node->next) {
        if (node->payload.block == p->body->reverse_instructions->operandData.operands[2].value.label->target.block) {
            *pp = node->next;
            break;
        }
    }

    for (pp = &p->body->reverse_instructions->operandData.operands[2].value.label->target.block->predecessors;
         (node = *pp) != NULL; pp = &node->next) {
        if (node->payload.block == p->body) {
            *pp = node->next;
            break;
        }
    }

    PCode_UnlinkInstruction(p->body->instructions);
    PCode_UnlinkInstruction(p->body->reverse_instructions);
    gArrayToRegisterEnabled = 1;
}

/* 0x420a10 */ /* 0x441af0 */ /* 0x441fa0 */ /* 0x49cfd0 */ /* 0x49d010 */
/* 0x49d140 */ /* 0x49d270 */                               /* 0x4a2620 */
/* 0x5233d0 */                                              /* 0x52ae70 */

void unroll_ctr_loop(Loop *loop)
{
    PCodeBlock *body;
    PCodeBlock *unrolledBody;
    PCodeInstruction *clonedUpdate;
    SInt32 copyIndex;
    PCodeInstruction *updateUse;
    SInt32 unrollFactor;
    PCodeBlock *start;
    SInt16 counterReg;
    PCodeBlock *exitBlock;
    PCodeInstruction *counterInstruction;
    SInt16 unrolledCounterReg;
    PCodeInstruction *scanInstruction;
    PCodeBlock *setupBlock;
    PCodeBlock *repeatBlock;
    PCodeBlock *remainderTest;
    PCodeBlock *remainderEntry;
    SInt16 scaledStepReg;
    PCodeOperand *operand;
    SInt32 operandCount;
    SInt32 shift;
    SInt32 rotateShift;
    SInt32 stepShift;
    PCodeBlock *block;
    PCodeInstruction *instruction;
    PCodeInstruction *currentInstruction;
    PCodeBlockLink *predecessor;

    if (loop->bodySize < 4)
        return;
    unrollFactor = 128;
    while (unrollFactor > copts.ppcUnrollFactorLimit)
        unrollFactor >>= 1;
    shift = copts.ppcUnrollInstructionsLimit;
    while (unrollFactor > 1 && (loop->bodySize - 2) * unrollFactor > shift)
        unrollFactor >>= 1;
    if (unrollFactor < 2)
        return;
    body = loop->preheader;
    exitBlock = loop->body->next;
    start = loop->preheader->next;
    setupBlock = insert_block_after(body, loop->execution_weight);
    unrolledBody = insert_block_after(setupBlock, loop->execution_weight);
    repeatBlock = insert_block_after(unrolledBody, loop->execution_weight);
    remainderTest = insert_block_after(repeatBlock, loop->execution_weight);
    remainderEntry = insert_block_after(remainderTest, loop->execution_weight);
    LoopDetection_AddBlock(loop, setupBlock);
    LoopDetection_AddBlock(loop, unrolledBody);
    LoopDetection_AddBlock(loop, repeatBlock);
    LoopDetection_AddBlock(loop, remainderTest);
    LoopDetection_AddBlock(loop, remainderEntry);
    for (scanInstruction = body->reverse_instructions; scanInstruction != NULL;
         scanInstruction = scanInstruction->previous) {
        if (scanInstruction->opcode == PC_MTCTR) {
            counterInstruction = scanInstruction;
            counterReg = scanInstruction->operandData.operands[0].value.reg;
        }
    }
    if (counterInstruction == NULL)
        return;
    clonedUpdate = NULL;
    for (block = start; block != loop->body; block = block->successors->payload.block) {
        for (currentInstruction = block->instructions; currentInstruction != NULL;
             currentInstruction = currentInstruction->next) {
            if (currentInstruction->opcode != PC_B) {
                instruction = PCode_CloneInstruction(currentInstruction);
                PCode_AppendInstruction(unrolledBody, instruction);
                if (currentInstruction == loop->inductionUpdate)
                    clonedUpdate = unrolledBody->reverse_instructions;
            }
        }
    }
    if (clonedUpdate == NULL) {
        instruction = PCode_CloneInstruction(loop->inductionUpdate);
        PCode_AppendInstruction(unrolledBody, instruction);
        clonedUpdate = unrolledBody->reverse_instructions;
    }
    updateUse = NULL;
    for (currentInstruction = unrolledBody->instructions; currentInstruction != NULL;
         currentInstruction = currentInstruction->next) {
        if (currentInstruction != clonedUpdate) {
            operandCount = currentInstruction->operand_count;
            operand = currentInstruction->operandData.operands;
            while (operandCount--) {
                if (operand->kind == PCOp_GPR &&
                    operand->value.reg == loop->inductionUpdate->operandData.operands[0].value.reg &&
                    (operand->flags & (PCodeOperand_Use | PCodeOperand_Definition)) != 0) {
                    updateUse = currentInstruction;
                    break;
                }
                operand++;
            }
        }
        if (updateUse != NULL)
            break;
    }
    if (updateUse == NULL) {
        PCode_UnlinkInstruction(clonedUpdate);
        PCode_UnlinkInstruction(loop->inductionUpdate);
        if ((exitBlock = loop->footer) == NULL)
            exitBlock = insert_block_after(loop->body, loop->execution_weight);
    } else {
        clonedUpdate = NULL;
    }
    for (copyIndex = 1; copyIndex < unrollFactor; copyIndex++) {
        for (block = start; block != loop->body; block = block->successors->payload.block) {
            for (currentInstruction = block->instructions; currentInstruction != NULL;
                 currentInstruction = currentInstruction->next) {
                if (currentInstruction->opcode != PC_B) {
                    instruction = PCode_CloneInstruction(currentInstruction);
                    PCode_AppendInstruction(unrolledBody, instruction);
                }
            }
        }
    }
    unrolledCounterReg = gUsedVirtualRegistersGPR++;
    shift = getbit(unrollFactor);
    shift = (shift > 0 && shift < 31) ? shift : 0;
    rotateShift = getbit(unrollFactor);
    rotateShift = (rotateShift > 0 && rotateShift < 31) ? rotateShift : 0;
    instruction = PCodeUtilities_CreateInstruction(0x67, unrolledCounterReg, counterReg, 32 - rotateShift, shift, 31);
    PCode_AppendInstruction(setupBlock, instruction);
    instruction = PCodeUtilities_CreateInstruction(0x54, 0, unrolledCounterReg, 0);
    PCode_AppendInstruction(setupBlock, instruction);
    if (clonedUpdate != NULL) {
        scaledStepReg = gUsedVirtualRegistersGPR++;
        if (loop->step == 1) {
            instruction = PCodeUtilities_CreateInstruction(0x8b, scaledStepReg, counterReg);
        } else if (loop->step == -1) {
            instruction = PCodeUtilities_CreateInstruction(0x4b, scaledStepReg, counterReg);
        } else {
            stepShift = getbit(abs(loop->step));
            if ((stepShift = (stepShift > 0 && stepShift < 31) ? stepShift : 0) > 0) {
                instruction =
                    PCodeUtilities_CreateInstruction(0x67, scaledStepReg, counterReg, stepShift, 0, 31 - stepShift);
                if (loop->step < 0) {
                    PCode_AppendInstruction(setupBlock, instruction);
                    instruction = PCodeUtilities_CreateInstruction(0x4b, scaledStepReg, scaledStepReg);
                }
            } else {
                instruction = PCodeUtilities_CreateInstruction(0x49, scaledStepReg, counterReg, loop->step);
            }
        }
        PCode_AppendInstruction(setupBlock, instruction);
    }
    instruction = PCodeUtilities_CreateInstruction(0x78, unrolledCounterReg);
    PCode_AppendInstruction(setupBlock, instruction);
    instruction = PCodeUtilities_CreateInstruction(5, 0, 2, remainderEntry->labels);
    PCode_AppendInstruction(setupBlock, instruction);
    PCode_AddSuccessor(setupBlock, remainderEntry->labels);
    predecessor = (PCodeBlockLink *)CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    predecessor->payload.block = setupBlock;
    predecessor->next = remainderEntry->predecessors;
    remainderEntry->predecessors = predecessor;
    instruction = PCodeUtilities_CreateInstruction(0xb, unrolledBody->labels);
    PCode_AppendInstruction(repeatBlock, instruction);
    PCode_AddSuccessor(repeatBlock, unrolledBody->labels);
    predecessor = (PCodeBlockLink *)CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    predecessor->payload.block = repeatBlock;
    predecessor->next = unrolledBody->predecessors;
    unrolledBody->predecessors = predecessor;
    unrollFactor--;
    instruction = PCodeUtilities_CreateInstruction(0x56, counterReg, counterReg, unrollFactor);
    PCode_AppendInstruction(remainderTest, instruction);
    instruction = PCodeUtilities_CreateInstruction(5, 0, 2, exitBlock->labels);
    PCode_AppendInstruction(remainderTest, instruction);
    PCode_AddSuccessor(remainderTest, exitBlock->labels);
    predecessor = (PCodeBlockLink *)CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    predecessor->payload.block = remainderTest;
    predecessor->next = exitBlock->predecessors;
    exitBlock->predecessors = predecessor;
    PCode_UnlinkInstruction(counterInstruction);
    PCode_AppendInstruction(remainderEntry, counterInstruction);
    if (clonedUpdate != NULL) {
        counterReg = clonedUpdate->operandData.operands[0].value.reg;
        if ((1 << counterReg) & registers_used_outside_loop[counterReg >> 5]) {
            instruction = PCodeUtilities_CreateInstruction(0x3c, counterReg, counterReg, scaledStepReg);
            if (exitBlock->instructions != NULL)
                PCode_InsertInstructionBefore(exitBlock->instructions, instruction);
            else
                PCode_AppendInstruction(exitBlock, instruction);
        }
    }
}
static inline int bit_set(UInt32 *bits, int bit)
{
    return bits[bit >> 5] & (1 << (bit & 31));
}

/* After unrolling: each original block whose branch led back to the loop body is unlinked from the body (its label
   operand is written through CLONE, the last branch cloned, as the original does). */
static inline void unroll_retarget_branches(Loop *loop, PCodeBlock **orig, SInt32 n, PCodeInstruction *clone,
                                            PCodeBlock **all)
{
    PCodeInstruction *pc;
    SInt32 i;
    PCodeBlockLink *prev;
    PCodeBlockLink *edge = NULL;
    SInt32 j;
    PCodeOperand *op;

    for (i = 0; i < n; i++) {
        for (pc = orig[i]->instructions; pc; pc = pc->next) {
            if (pc->flags & fIsBranch) {
                for (j = 0; j < pc->operand_count; j++) {
                    if (pc->operandData.operands[j].kind == PCOp_LABEL) {
                        op = &pc->operandData.operands[j];
                        break;
                    }
                }
                if (op && op->value.label->target.block == loop->body) {
                    clone->operandData.operands[j].value.label = (PCodeLabel *)all[0]->labels;
                    for (edge = orig[i]->successors, prev = NULL; edge; edge = edge->next) {
                        if (edge->payload.block == loop->body) {
                            if (prev)
                                prev->next = edge->next;
                            else
                                orig[i]->successors = edge->next;
                        } else
                            prev = edge;
                    }
                    prev = NULL;
                    for (edge = loop->body->predecessors; edge; edge = edge->next) {
                        if (edge->payload.block == orig[i]) {
                            if (prev)
                                prev->next = edge->next;
                            else
                                loop->body->predecessors = edge->next;
                        } else
                            prev = edge;
                    }
                }
            }
        }
    }
}

/* Unrolls a counting loop whose body is small enough: the body's blocks are copied FACTOR - 1 times (the largest factor
   up to the limit that divides the trip count), branches retargeted to the copies, and the trip count divided. */
void unroll_loop_by_factor(Loop *loop)
{
    int factor;
    SInt32 n, made, i = 0, k;
    int total;
    int round;
    PCodeBlock *b, *last, *target;
    PCodeInstruction *pc, *clone;
    PCodeBlock **copy, **orig, **all;
    SInt32 j;
    PCodeLabel *pending;
    PCodeBlockLink *edge, *prev;

    pending = NULL;
    made = 0;
    total = 0;
    factor = copts.ppcUnrollFactorLimit;
    while (factor > 1) {
        if (loop->iterationCount % factor == 0 && (loop->bodySize - 2) * factor <= copts.ppcUnrollInstructionsLimit)
            break;
        factor--;
    }
    if (factor == 1)
        return;
    for (n = 0, b = loop->preheader->successors->payload.block; b != loop->body; b = b->next) {
        n++;
        if (!bit_set(loop->memberblocks, b->index))
            total += b->instruction_count;
    }
    if (loop->bodySize - total - 2 < total || total > 8)
        return;
    k = n * 4;
    orig = (PCodeBlock **)CompilerTools_AllocatePoolMemory(k);
    copy = (PCodeBlock **)CompilerTools_AllocatePoolMemory(k);
    all = (PCodeBlock **)CompilerTools_AllocatePoolMemory(total = factor * k);
    memclrw(orig, k);
    memclrw(copy, k);
    memclrw(all, total);
    for (i = 0, b = loop->preheader->next; i < n; i++) {
        orig[i] = b;
        b = b->next;
    }
    last = orig[n - 1];
    for (round = 0; round < factor - 1; round++) {
        for (i = 0; i < n; i++) {
            copy[i] = insert_block_after(last, loop->execution_weight);
            all[made++] = copy[i];
            last = copy[i];
        }
        if (pending) {
            PCode_ResolveLabel(copy[0], pending);
            pending = NULL;
        }
        for (i = 0; i < n; i++) {
            for (pc = orig[i]->instructions; pc; pc = pc->next) {
                if (pc->flags & fIsBranch) {
                    PCodeOperand *op;

                    clone = PCode_CloneInstruction(pc);
                    for (j = 0; j < pc->operand_count; j++) {
                        if (pc->operandData.operands[j].kind == PCOp_LABEL) {
                            op = &pc->operandData.operands[j];
                            break;
                        }
                    }
                    if (op) {
                        if ((target = op->value.label->target.block) == loop->body) {
                            if (!pending)
                                pending = PCode_NewLabel();
                            clone->operandData.operands[j].value.label = pending;
                        } else {
                            for (k = 0; k < n; k++) {
                                if (target == orig[k]) {
                                    clone->operandData.operands[j].value.label = (PCodeLabel *)copy[k]->labels;
                                    break;
                                }
                            }
                        }
                    }
                    PCode_AppendInstruction(copy[i], clone);
                    if (op)
                        PCode_AddSuccessor(copy[i], clone->operandData.operands[j].value.label);
                } else {
                    PCode_AppendInstruction(copy[i], PCode_CloneInstruction(pc));
                }
            }
        }
        if (!(pc = loop->body->instructions))
            CError_FATAL(692);
        if (pc->opcode != PC_CMP && pc->opcode != PC_CMPL && pc->opcode != PC_CMPI && pc->opcode != PC_CMPLI)
            CError_FATAL(694);
        for (pc = pc->next; pc && !(pc->flags & fIsBranch); pc = pc->next)
            PCode_AppendInstruction(copy[n - 1], PCode_CloneInstruction(pc));
        for (i = 0; i < n; i++) {
            for (edge = orig[i]->successors; edge; edge = edge->next)
                if (edge->payload.block == orig[i]->next)
                    break;
            if (!edge) {
                for (edge = copy[i]->successors, prev = NULL; edge; edge = edge->next) {
                    if (edge->payload.block == copy[i]->next) {
                        if (prev)
                            prev->next = edge->next;
                        else
                            copy[i]->successors = edge->next;
                    } else
                        prev = edge;
                }
                edge = copy[i]->next->predecessors;
                prev = NULL;
                for (; edge; edge = edge->next) {
                    if (edge->payload.block == copy[i]) {
                        if (prev)
                            prev->next = edge->next;
                        else
                            copy[i]->next->predecessors = edge->next;
                    } else
                        prev = edge;
                }
            }
        }
    }
    if (pending)
        PCode_ResolveLabel(loop->body, pending);
    unroll_retarget_branches(loop, orig, n, clone, all);
    for (i = 0; i < made; i++)
        LoopOptimization_AddMissingSuccessorPredecessors(all[i]);
    loop->iterationCount /= factor;
}

PCodeBlock *insert_block_after(PCodeBlock *list, int execution_weight)
{
    PCodeLabel *label;
    PCodeBlock *block;
    PCodeBlock *successor;
    PCodeBlockLink *previous;
    PCodeBlockLink *edge;

    label = PCode_NewLabel();
    block = (PCodeBlock *)CompilerTools_AllocatePool(sizeof(PCodeBlock));
    successor = list->next;
    block->next = successor;
    list->next = block;
    block->prev = list;
    successor->prev = block;
    block->labels = NULL;
    block->predecessors = block->successors = NULL;
    block->instructions = block->reverse_instructions = NULL;
    block->line = -1;
    block->instruction_count = 0;
    block->execution_weight = execution_weight;
    block->flags = 0;
    block->index = gPCodeBlockCount++;
    PCode_ResolveLabel(block, label);

    previous = NULL;
    for (edge = list->successors; edge != NULL; edge = edge->next) {
        if (edge->payload.block == successor) {
            if (previous == NULL)
                list->successors = edge->next;
            else
                previous->next = edge->next;
            edge->next = NULL;
            block->successors = edge;
            break;
        }
        previous = edge;
    }
    previous = NULL;
    for (edge = successor->predecessors; edge != NULL; edge = edge->next) {
        if (edge->payload.block == list) {
            if (previous == NULL)
                successor->predecessors = edge->next;
            else
                previous->next = edge->next;
            edge->next = NULL;
            block->predecessors = edge;
            break;
        }
        previous = edge;
    }
    edge = (PCodeBlockLink *)CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    edge->payload.block = block;
    edge->next = list->successors;
    list->successors = edge;
    edge = (PCodeBlockLink *)CompilerTools_AllocatePool(sizeof(PCodeBlockLink));
    edge->payload.block = block;
    edge->next = successor->predecessors;
    successor->predecessors = edge;
    return block;
}

void LoopOptimization_AddMissingSuccessorPredecessors(PCodeBlock *block)
{
    PCodeBlockLink *successor;
    PCodeBlockLink *predecessor;
    successor = block->successors;
    while (successor != NULL) {
        if (successor->payload.block == NULL) {
            CError_FATAL(426);
        } else {
            predecessor = successor->payload.block->predecessors;
            while (predecessor != NULL) {
                if (predecessor->payload.block == block)
                    break;
                predecessor = predecessor->next;
            }
            if (predecessor == NULL) {
                predecessor = (PCodeBlockLink *)CompilerTools_AllocatePool(8U);
                predecessor->payload.block = block;
                predecessor->next = successor->payload.block->predecessors;
                successor->payload.block->predecessors = predecessor;
            }
        }
        successor = successor->next;
    }
}

static inline int rem(int x, int y)
{
    return x % y;
}

void unroll_counting_loop(Loop *loop)
{
    int limit;
    PCodeBlock *copies;
    PCodeInstruction *first;
    short opcode;
    PCodeInstruction *instruction;
    PCodeBlockLink *link;
    PCodeBlock *previous;
    PCodeBlock *block;
    PCodeInstruction *head;
    PCodeInstruction *copy;
    PCodeInstruction *next;
    PCodeInstruction *body;
    int factor;
    int iteration;

    factor = copts.ppcUnrollFactorLimit;
    while (factor > 1) {
        limit = copts.ppcUnrollInstructionsLimit;
        if (!rem(loop->iterationCount, factor) && (loop->bodySize - 2) * factor <= limit)
            break;
        --factor;
    }
    if (factor == 1)
        return;
    if (loop->iterationCount / factor != 1 && loop->bodySize < 4)
        return;

    copies = (PCodeBlock *)CompilerTools_AllocatePoolMemory(sizeof(PCodeBlock));
    copies->instructions = copies->reverse_instructions = NULL;
    for (iteration = 0; iteration < factor - 1; ++iteration) {
        CE_ASSERT((first = loop->body->instructions) == 0, CError_FATAL(378));
        CE_ASSERT((opcode = first->opcode) != 83 && opcode != 85 && opcode != 82 && opcode != 84, CError_FATAL(380));
        body = first->next;
        while (body && !(body->flags & PCodeInstruction_SkipCodeMotion)) {
            PCode_AppendInstruction(copies, PCode_CloneInstruction(body));
            body = body->next;
        }
        link = loop->preheader->successors;
        block = link->payload.block;
        while (block != loop->body) {
            for (instruction = block->instructions; instruction; instruction = instruction->next) {
                if (instruction->opcode != PC_B)
                    PCode_AppendInstruction(copies, PCode_CloneInstruction(instruction));
            }
            link = block->successors;
            block = link->payload.block;
        }
    }
    previous = loop->body->predecessors->payload.block;
    head = copies->instructions;
    copy = head;
    if (head) {
        do {
            next = copy->next;
            PCode_AppendInstruction(previous, copy);
            copy = next;
        } while (copy);
    }
    loop->iterationCount = loop->iterationCount / factor;
}

void fn_0052b1a0(Loop *loop)
{
    PCodeBlock *destination;
    PCodeLabel *entry;
    PCodeBlockLink **link, *node;
    PCodeInstruction *instruction;
    PCodeInstruction *nextInstruction;
    PCodeInstruction *copy;

    destination = loop->preheader;
    entry = loop->body->reverse_instructions->operandData.operands[2].value.label;
    destination->reverse_instructions->operandData.operands[0].value.label = entry;
    destination->successors->payload.block = entry->target.block;

    link = &loop->body->predecessors;
    while ((node = *link) != NULL) {
        if (node->payload.block == destination) {
            *link = node->next;
            break;
        }
        link = &node->next;
    }
    node->next = entry->target.block->predecessors;
    entry->target.block->predecessors = node;

    for (;;) {
        if ((instruction = loop->body->instructions) == NULL)
            CError_FATAL(299);
        if (instruction->opcode == PC_CMP || instruction->opcode == PC_CMPI || instruction->opcode == PC_CMPLI ||
            instruction->opcode == PC_CMPL)
            break;
        PCode_UnlinkInstruction(instruction);
        PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, instruction);
        loop->bodySize--;
    }
    nextInstruction = instruction->next;
    while (nextInstruction != NULL && (nextInstruction->flags & fIsBranch) == 0) {
        copy = PCode_CloneInstruction(nextInstruction);
        PCode_InsertInstructionBefore(loop->preheader->reverse_instructions, copy);
        nextInstruction = nextInstruction->next;
    }
}

void remove_unused_self_addi(Loop *loop)
{
    int mask;
    int wordIndex;
    PCodeOperand *operand;
    PCodeInstruction *instruction;
    PCodeInstruction *nextInstruction;
    int operandCount;
    int reg;
    int operandReg;
    PCodeBlockLink *link;
    PCodeInstruction *candidate;
    int candidateReg;
    PCodeBlockLink *scan;

    CRTTI_FillWords(self_addi_candidate_regs, data_00582c70, -1);
    scan = loop->blocks;
    while (scan != NULL) {
        instruction = scan->payload.block->instructions;
        if (instruction != NULL) {
            do {
                if (instruction->opcode == PC_ADDI) {
                    reg = instruction->operandData.operands[0].value.reg;
                    if (reg >= 32 && reg < data_00582c70 && instruction->operandData.operands[1].value.reg == reg) {
                        continue;
                    }
                }
                operand = instruction->operandData.operands;
                operandCount = instruction->operand_count;
                while (operandCount--) {
                    if (operand->kind == PCOp_GPR) {
                        operandReg = operand->value.reg;
                        if (operandReg >= 32 && operandReg < data_00582c70) {
                            self_addi_candidate_regs[operand->value.reg >> 5] &= ~(1 << (operand->value.reg & 31));
                        }
                    }
                    operand++;
                }
            } while ((instruction = instruction->next) != NULL);
        }
        scan = scan->next;
    }
    link = loop->blocks;
    if (link != NULL) {
        do {
            candidate = link->payload.block->instructions;
            if (candidate != NULL) {
                do {
                    nextInstruction = candidate->next;
                    if (candidate->opcode == PC_ADDI) {
                        candidateReg = candidate->operandData.operands[0].value.reg;
                        if (candidateReg >= 32 && candidateReg < data_00582c70 &&
                            candidate->operandData.operands[1].value.reg == candidateReg &&
                            ((mask = 1 << candidateReg) & self_addi_candidate_regs[wordIndex = candidateReg >> 5]) !=
                                0 &&
                            (mask & registers_used_outside_loop[wordIndex]) == 0) {
                            PCode_UnlinkInstruction(candidate);
                            gLoopTransformChanged = 1;
                        }
                    }
                    candidate = nextInstruction;
                } while (candidate != NULL);
            }
            link = link->next;
        } while (link != NULL);
    }
}

void mark_registers_used_outside_loop(Loop *state)
{
    UInt32 *useMap;
    PCodeBlockLink *objectUse;
    SInt32 registerNumber;

    CRTTI_FillWords(useMap = (UInt32 *)CompilerTools_AllocatePoolMemory(((data_00587e38 + 31) >> 5) << 2),
                    data_00587e38, 0);

    for (objectUse = state->blocks; objectUse != NULL; objectUse = objectUse->next) {
        SInt32 objectRegister = objectUse->payload.block->index;
        if (state->exitblocks[objectRegister >> 5] & (1 << objectRegister))
            CRTTI_OrBitVector(useMap, data_00587fe4[objectRegister].use_sets[3], data_00587e38);
    }

    CRTTI_FillWords(registers_used_outside_loop, data_00582c70, 0);

    for (registerNumber = 32; registerNumber < data_00582c70; registerNumber++) {
        CodeMotionEntryLink *use;
        for (use = code_motion_register_use_heads[registerNumber]; use != NULL; use = use->next) {
            SInt32 useIndex = use->entry_index;
            PCodeInstruction *value;
            if (!(useMap[useIndex >> 5] & (1 << useIndex)))
                continue;
            value = cm_entries[useIndex].instruction;
            if (value->block != NULL) {
                SInt32 objectRegister = value->block->index;
                if (state->memberblocks[objectRegister >> 5] & (1 << objectRegister))
                    continue;
            }
            registers_used_outside_loop[registerNumber >> 5] |= (1 << registerNumber);
        }
    }
}

static inline void clonefirst(PCodeBlock *v2, PCodeInstruction *v5)
{
    while (v5 != NULL && (v5->flags & fIsBranch) == 0) {
        PCode_AppendInstruction(v2, PCode_CloneInstruction(v5));
        v5 = v5->next;
    }
}
