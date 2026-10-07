#ifndef COMPILER_PCODE_H
#define COMPILER_PCODE_H

#include <stddef.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
struct BlockOrderEntry {
    struct PCodeBlock *block;
    struct PCodeBlockLink *cursor;
};
#pragma pack(pop)
#pragma pack(push, 2)
union PCodeOperandValue {
    struct PCodeLabel *label;
    short reg;
    int signed_value;
    signed long immediate_value;
    unsigned int unsigned_value;
};
#pragma pack(pop)
/* What a PCode operand is (PCodeOperand.kind): a register of a class (the call clobber list builds r0, r3-r12 as kind 0,
 * f0-f13 as 1, v0-v19 as 9, cr0/1/6/7 as 3; addic. writes XER as 2 and cr0 as 3), an immediate (MTSPR lists it as a
 * number), a symbol (the listing prints its object), a label (the listing prints its block). Numbers: kind is a byte. */
#define PCOp_GPR 0
#define PCOp_FPR 1
#define PCOp_SPR 2
#define PCOp_CRFIELD 3
#define PCOp_IMMEDIATE 4
#define PCOp_MEMORY 5
#define PCOp_LABEL 6
#define PCOp_VR 9

#pragma pack(push, 2)
struct PCodeOperand {
    unsigned char kind;
    signed char flags;
    PCodeOperandValue value;
    struct Object *object;
    unsigned char unknown_0a[2];
};
/* Operand kind 7: label difference, as format_operand tests before reading it. */
struct PCodeLabelDifference {
    UInt8 kind;                       /* 0x00: format_operand kind == 7 */
    UInt8 negate;                     /* 0x01: format_operand tests negate == 1 */
    SInt16 addend;                    /* 0x02: format_operand prints addend */
    struct PCodeLabel *subtractLabel; /* 0x04: format_operand prints subtracted block */
    struct PCodeLabel *addLabel;      /* 0x08: format_operand prints added block */
};
#pragma pack(pop)
/* The PCode opcodes, as the opcode table (gPCodeOpcodeDescriptors) names them; defined as numbers: a short compared with an
 * enumerator compiles differently from one compared with a literal. */
#define PC_B 0
#define PC_BL 1
#define PC_BC 2
#define PC_BCLR 3
#define PC_BCCTR 4
#define PC_BT 5
#define PC_BTLR 6
#define PC_BTCTR 7
#define PC_BF 8
#define PC_BFLR 9
#define PC_BFCTR 10
#define PC_BDNZ 11
#define PC_BDNZT 12
#define PC_BDNZF 13
#define PC_BDZ 14
#define PC_BDZT 15
#define PC_BDZF 16
#define PC_BLR 17
#define PC_BCTR 18
#define PC_BCTRL 19
#define PC_BLRL 20
#define PC_LBZ 21
#define PC_LBZU 22
#define PC_LBZX 23
#define PC_LBZUX 24
#define PC_LHZ 25
#define PC_LHZU 26
#define PC_LHZX 27
#define PC_LHZUX 28
#define PC_LHA 29
#define PC_LHAU 30
#define PC_LHAX 31
#define PC_LHAUX 32
#define PC_LHBRX 33
#define PC_LWZ 34
#define PC_LWZU 35
#define PC_LWZX 36
#define PC_LWZUX 37
#define PC_LWBRX 38
#define PC_LMW 39
#define PC_STB 40
#define PC_STBU 41
#define PC_STBX 42
#define PC_STBUX 43
#define PC_STH 44
#define PC_STHU 45
#define PC_STHX 46
#define PC_STHUX 47
#define PC_STHBRX 48
#define PC_STW 49
#define PC_STWU 50
#define PC_STWX 51
#define PC_STWUX 52
#define PC_STWBRX 53
#define PC_STMW 54
#define PC_DCBF 55
#define PC_DCBST 56
#define PC_DCBT 57
#define PC_DCBTST 58
#define PC_DCBZ 59
#define PC_ADD 60
#define PC_ADDC 61
#define PC_ADDE 62
#define PC_ADDI 63
#define PC_ADDIC 64
#define PC_ADDICR 65
#define PC_ADDIS 66
#define PC_ADDME 67
#define PC_ADDZE 68
#define PC_DIVW 69
#define PC_DIVWU 70
#define PC_MULHW 71
#define PC_MULHWU 72
#define PC_MULLI 73
#define PC_MULLW 74
#define PC_NEG 75
#define PC_SUBF 76
#define PC_SUBFC 77
#define PC_SUBFE 78
#define PC_SUBFIC 79
#define PC_SUBFME 80
#define PC_SUBFZE 81
#define PC_CMPI 82
#define PC_CMP 83
#define PC_CMPLI 84
#define PC_CMPL 85
#define PC_ANDI 86
#define PC_ANDIS 87
#define PC_ORI 88
#define PC_ORIS 89
#define PC_XORI 90
#define PC_XORIS 91
#define PC_AND 92
#define PC_OR 93
#define PC_XOR 94
#define PC_NAND 95
#define PC_NOR 96
#define PC_EQV 97
#define PC_ANDC 98
#define PC_ORC 99
#define PC_EXTSB 100
#define PC_EXTSH 101
#define PC_CNTLZW 102
#define PC_RLWINM 103
#define PC_RLWNM 104
#define PC_RLWIMI 105
#define PC_SLW 106
#define PC_SRW 107
#define PC_SRAWI 108
#define PC_SRAW 109
#define PC_CRAND 110
#define PC_CRANDC 111
#define PC_CREQV 112
#define PC_CRNAND 113
#define PC_CRNOR 114
#define PC_CROR 115
#define PC_CRORC 116
#define PC_CRXOR 117
#define PC_MCRF 118
#define PC_MTXER 119
#define PC_MTCTR 120
#define PC_MTLR 121
#define PC_MTCRF 122
#define PC_MTMSR 123
#define PC_MTSPR 124
#define PC_MFMSR 125
#define PC_MFSPR 126
#define PC_MFXER 127
#define PC_MFCTR 128
#define PC_MFLR 129
#define PC_MFCR 130
#define PC_MFFS 131
#define PC_MTFSF 132
#define PC_EIEIO 133
#define PC_ISYNC 134
#define PC_SYNC 135
#define PC_RFI 136
#define PC_LI 137
#define PC_LIS 138
#define PC_MR 139
#define PC_NOP 140
#define PC_NOT 141
#define PC_LFS 142
#define PC_LFSU 143
#define PC_LFSX 144
#define PC_LFSUX 145
#define PC_LFD 146
#define PC_LFDU 147
#define PC_LFDX 148
#define PC_LFDUX 149
#define PC_STFS 150
#define PC_STFSU 151
#define PC_STFSX 152
#define PC_STFSUX 153
#define PC_STFD 154
#define PC_STFDU 155
#define PC_STFDX 156
#define PC_STFDUX 157
#define PC_FMR 158
#define PC_FABS 159
#define PC_FNEG 160
#define PC_FNABS 161
#define PC_FADD 162
#define PC_FADDS 163
#define PC_FSUB 164
#define PC_FSUBS 165
#define PC_FMUL 166
#define PC_FMULS 167
#define PC_FDIV 168
#define PC_FDIVS 169
#define PC_FMADD 170
#define PC_FMADDS 171
#define PC_FMSUB 172
#define PC_FMSUBS 173
#define PC_FNMADD 174
#define PC_FNMADDS 175
#define PC_FNMSUB 176
#define PC_FNMSUBS 177
#define PC_FRES 178
#define PC_FRSQRTE 179
#define PC_FSEL 180
#define PC_FRSP 181
#define PC_FCTIW 182
#define PC_FCTIWZ 183
#define PC_FCMPU 184
#define PC_FCMPO 185
#define PC_LWARX 186
#define PC_LSWI 187
#define PC_LSWX 188
#define PC_STFIWX 189
#define PC_STSWI 190
#define PC_STSWX 191
#define PC_STWCX 192
#define PC_ECIWX 193
#define PC_ECOWX 194
#define PC_DCBI 195
#define PC_ICBI 196
#define PC_MCRFS 197
#define PC_MCRXR 198
#define PC_MFTB 199
#define PC_MFSR 200
#define PC_MTSR 201
#define PC_MFSRIN 202
#define PC_MTSRIN 203
#define PC_MTFSB0 204
#define PC_MTFSB1 205
#define PC_MTFSFI 206
#define PC_SC 207
#define PC_FSQRT 208
#define PC_FSQRTS 209
#define PC_TLBIA 210
#define PC_TLBIE 211
#define PC_TLBLD 212
#define PC_TLBLI 213
#define PC_TLBSYNC 214
#define PC_TW 215
#define PC_TRAP 216
#define PC_TWI 217
#define PC_OPWORD 218
#define PC_MFROM 219
#define PC_DSA 220
#define PC_ESA 221
#define PC_DCCCI 222
#define PC_DCREAD 223
#define PC_ICBT 224
#define PC_ICCCI 225
#define PC_ICREAD 226
#define PC_RFCI 227
#define PC_TLBRE 228
#define PC_TLBSX 229
#define PC_TLBWE 230
#define PC_WRTEE 231
#define PC_WRTEEI 232
#define PC_MFDCR 233
#define PC_MTDCR 234
#define PC_DCBA 235
#define PC_DSS 236
#define PC_DSSALL 237
#define PC_DST 238
#define PC_DSTT 239
#define PC_DSTST 240
#define PC_DSTSTT 241
#define PC_LVEBX 242
#define PC_LVEHX 243
#define PC_LVEWX 244
#define PC_LVSL 245
#define PC_LVSR 246
#define PC_LVX 247
#define PC_LVXL 248
#define PC_STVEBX 249
#define PC_STVEHX 250
#define PC_STVEWX 251
#define PC_STVX 252
#define PC_STVXL 253
#define PC_MFVSCR 254
#define PC_MTVSCR 255
#define PC_VADDCUW 256
#define PC_VADDFP 257
#define PC_VADDSBS 258
#define PC_VADDSHS 259
#define PC_VADDSWS 260
#define PC_VADDUBM 261
#define PC_VADDUBS 262
#define PC_VADDUHM 263
#define PC_VADDUHS 264
#define PC_VADDUWM 265
#define PC_VADDUWS 266
#define PC_VAND 267
#define PC_VANDC 268
#define PC_VAVGSB 269
#define PC_VAVGSH 270
#define PC_VAVGSW 271
#define PC_VAVGUB 272
#define PC_VAVGUH 273
#define PC_VAVGUW 274
#define PC_VCFSX 275
#define PC_VCFUX 276
#define PC_VCMPBFP 277
#define PC_VCMPEQFP 278
#define PC_VCMPEQUB 279
#define PC_VCMPEQUH 280
#define PC_VCMPEQUW 281
#define PC_VCMPGEFP 282
#define PC_VCMPGTFP 283
#define PC_VCMPGTSB 284
#define PC_VCMPGTSH 285
#define PC_VCMPGTSW 286
#define PC_VCMPGTUB 287
#define PC_VCMPGTUH 288
#define PC_VCMPGTUW 289
#define PC_VCTSXS 290
#define PC_VCTUXS 291
#define PC_VEXPTEFP 292
#define PC_VLOGEFP 293
#define PC_VMAXFP 294
#define PC_VMAXSB 295
#define PC_VMAXSH 296
#define PC_VMAXSW 297
#define PC_VMAXUB 298
#define PC_VMAXUH 299
#define PC_VMAXUW 300
#define PC_VMINFP 301
#define PC_VMINSB 302
#define PC_VMINSH 303
#define PC_VMINSW 304
#define PC_VMINUB 305
#define PC_VMINUH 306
#define PC_VMINUW 307
#define PC_VMRGHB 308
#define PC_VMRGHH 309
#define PC_VMRGHW 310
#define PC_VMRGLB 311
#define PC_VMRGLH 312
#define PC_VMRGLW 313
#define PC_VMULESB 314
#define PC_VMULESH 315
#define PC_VMULEUB 316
#define PC_VMULEUH 317
#define PC_VMULOSB 318
#define PC_VMULOSH 319
#define PC_VMULOUB 320
#define PC_VMULOUH 321
#define PC_VNOR 322
#define PC_VOR 323
#define PC_VPKPX 324
#define PC_VPKSHSS 325
#define PC_VPKSHUS 326
#define PC_VPKSWSS 327
#define PC_VPKSWUS 328
#define PC_VPKUHUM 329
#define PC_VPKUHUS 330
#define PC_VPKUWUM 331
#define PC_VPKUWUS 332
#define PC_VREFP 333
#define PC_VRFIM 334
#define PC_VRFIN 335
#define PC_VRFIP 336
#define PC_VRFIZ 337
#define PC_VRLB 338
#define PC_VRLH 339
#define PC_VRLW 340
#define PC_VRSQRTEFP 341
#define PC_VSL 342
#define PC_VSLB 343
#define PC_VSLH 344
#define PC_VSLO 345
#define PC_VSLW 346
#define PC_VSPLTB 347
#define PC_VSPLTH 348
#define PC_VSPLTW 349
#define PC_VSPLTISB 350
#define PC_VSPLTISH 351
#define PC_VSPLTISW 352
#define PC_VSR 353
#define PC_VSRAB 354
#define PC_VSRAH 355
#define PC_VSRAW 356
#define PC_VSRB 357
#define PC_VSRH 358
#define PC_VSRO 359
#define PC_VSRW 360
#define PC_VSUBCUW 361
#define PC_VSUBFP 362
#define PC_VSUBSBS 363
#define PC_VSUBSHS 364
#define PC_VSUBSWS 365
#define PC_VSUBUBM 366
#define PC_VSUBUBS 367
#define PC_VSUBUHM 368
#define PC_VSUBUHS 369
#define PC_VSUBUWM 370
#define PC_VSUBUWS 371
#define PC_VSUMSWS 372
#define PC_VSUM2SWS 373
#define PC_VSUM4SBS 374
#define PC_VSUM4SHS 375
#define PC_VSUM4UBS 376
#define PC_VUPKHPX 377
#define PC_VUPKHSB 378
#define PC_VUPKHSH 379
#define PC_VUPKLPX 380
#define PC_VUPKLSB 381
#define PC_VUPKLSH 382
#define PC_VXOR 383
#define PC_VMADDFP 384
#define PC_VMHADDSHS 385
#define PC_VMHRADDSHS 386
#define PC_VMLADDUHM 387
#define PC_VMSUMMBM 388
#define PC_VMSUMSHM 389
#define PC_VMSUMSHS 390
#define PC_VMSUMUBM 391
#define PC_VMSUMUHM 392
#define PC_VMSUMUHS 393
#define PC_VNMSUBFP 394
#define PC_VPERM 395
#define PC_VSEL 396
#define PC_VSLDOI 397
#define PC_VMR 398
#define PC_VMRP 399
#define PC_PSQ_L 400
#define PC_PSQ_LU 401
#define PC_PSQ_LX 402
#define PC_PSQ_LUX 403
#define PC_PSQ_ST 404
#define PC_PSQ_STU 405
#define PC_PSQ_STX 406
#define PC_PSQ_STUX 407
#define PC_DCBZ_L 408
#define PC_PS_ADD 409
#define PC_PS_SUB 410
#define PC_PS_MUL 411
#define PC_PS_DIV 412
#define PC_PS_MADD 413
#define PC_PS_MSUB 414
#define PC_PS_NMADD 415
#define PC_PS_NMSUB 416
#define PC_PS_RES 417
#define PC_PS_SEL 418
#define PC_PS_ABS 419
#define PC_PS_NABS 420
#define PC_PS_NEG 421
#define PC_PS_MR 422
#define PC_PS_RSQRTE 423
#define PC_PS_CMPU0 424
#define PC_PS_CMPO0 425
#define PC_PS_CMPU1 426
#define PC_PS_CMPO1 427
#define PC_PS_MERGE00 428
#define PC_PS_MERGE01 429
#define PC_PS_MERGE10 430
#define PC_PS_MERGE11 431
#define PC_PS_SUM0 432
#define PC_PS_SUM1 433
#define PC_PS_MULS0 434
#define PC_PS_MULS1 435
#define PC_PS_MADDS0 436
#define PC_PS_MADDS1 437
#define PC_SLE 438
#define PC_SLEQ 439
#define PC_SLIQ 440
#define PC_SLLIQ 441
#define PC_SLLQ 442
#define PC_SLQ 443
#define PC_SRAIQ 444
#define PC_SRAQ 445
#define PC_SRE 446
#define PC_SREA 447
#define PC_SREQ 448
#define PC_SRIQ 449
#define PC_SRLIQ 450
#define PC_SRLQ 451
#define PC_SRQ 452
#define PC_MASKG 453
#define PC_MASKIR 454
#define PC_LSCBX 455
#define PC_DIV 456
#define PC_DIVS 457
#define PC_DOZ 458
#define PC_MUL 459
#define PC_NABS 460
#define PC_ABS 461
#define PC_CLCS 462
#define PC_DOZI 463
#define PC_RLMI 464
#define PC_RRIB 465
#define PC_PENTRY 466
#define PC_PEXIT 467

/* A PCode instruction's flags (PCodeInstruction.flags), as the listing prints them (fIsConst, fIsVolatile, fSideEffects, ...); its
 * other bits are not named. */
#define fIsPtrOp 0x40
#define fSideEffects 0x400
#define fLink 0x4000
#define fIsConst 0x10000
#define fIsVolatile 0x20000
#define fAbsolute 0x40000
#define fOverflow 0x80000
#define fCallerSPRelative 0x400000
#define fIsLive 0x800000

/* More of an instruction's flags, as the opcode table sets them per opcode: direct branches, loads, stores, record forms
 * (cr0), carry setters, register moves. */
#define fIsBranch 0x4
#define fIsRead 0x8
#define fIsWrite 0x10
#define fRecordBit 0x80
#define fSetsCarry 0x100
#define fIsMove 0x800

#pragma pack(push, 2)
struct PCodeInstruction {
    struct PCodeInstruction *next;
    struct PCodeInstruction *previous;
    struct PCodeBlock *block;
    int definitionStart;
    int useStart;
    short opcode;
    unsigned int flags;
    short operand_count;
    union {
        PCodeOperand operands[1];
        struct PCodeAsmOperand {
            unsigned char kind;
            unsigned char arg;
            union {
                struct {
                    short reg;
                    int effect;
                } reg;
                struct {
                    int value;
                    struct Object *obj;
                } imm;
                struct {
                    int offset;
                    struct Object *obj;
                } mem;
                struct {
                    struct PCodeLabel *label;
                } label;
                struct {
                    short offset;
                    struct PCodeLabel *labelA;
                    struct PCodeLabel *labelB;
                } labeldiff;
            } data;
        } assemblyOperands[1];
    } operandData;
};
#pragma pack(pop)
#pragma pack(push, 2)
union PCodeBlockTarget {
    struct PCodeBlock *block; /* 0x00: PCode_AddSuccessor resolved != 0; PCode_ResolveLabel resolves to target */
    struct PCodeBlockLink *
        pendingLinks; /* 0x00: PCode_AddSuccessor resolved == 0; PCode_ResolveLabel walks unresolved successor fixups */
};
struct PCodeBlockLink {
    struct PCodeBlockLink *next; /* 0x00: PCode_AddSuccessor successor list */
    union PCodeBlockTarget
        payload; /* 0x04: PCode_AddSuccessor copies label state; PCode_ResolveLabel resolves pending links */
};
#pragma pack(pop)
#pragma pack(push, 2)
struct PCodeBlock {
    struct PCodeBlock *next;
    struct PCodeBlock *prev;
    struct PCodeLabel *labels; /* 0x08: PCode_ResolveLabel prepends entry to target's label list */
    PCodeBlockLink *predecessors;
    PCodeBlockLink *successors;
    PCodeInstruction *instructions;
    PCodeInstruction *reverse_instructions;
    int index; /* 0x1c: format_operand prints the label target's block index as B%ld */
    int line;
    int code_offset;
    int execution_weight;
    short instruction_count;
    short flags;
};
#pragma pack(pop)
#pragma options align = mac68k
struct PCodeLabel {
    struct PCodeLabel *next;
    union PCodeBlockTarget
        target; /* 0x04: PCode_AddSuccessor resolved selects block or pendingLinks; PCode_ResolveLabel resolves label */
    UInt16 resolved;
    UInt16 number;
};
#pragma options align = reset
extern unsigned int data_00587ffc;
extern unsigned int PCode_SetCodeOffsets(void);
extern void Operands_AllocateGPR(unsigned int flags);
extern void PCode_InsertInstructionAfter(PCodeInstruction *h, PCodeInstruction *n);
extern void PCode_UnlinkInstruction(PCodeInstruction *instruction);
extern PCodeInstruction *PCode_AppendInstruction(PCodeBlock *block, PCodeInstruction *instruction);
extern void PCode_ResolveLabel(PCodeBlock *target, PCodeLabel *entry);
extern void PCode_UnlinkBlocksWithoutFlag4(void);
extern void PCode_BuildPredecessors(void);
extern void PCode_AddSuccessor(PCodeBlock *block, PCodeLabel *name);
extern PCodeBlock *PCode_CreateBlock(void);
extern void PCode_InsertInstructionBefore(PCodeInstruction *h, PCodeInstruction *n);
extern PCodeInstruction *PCode_CloneInstruction(PCodeInstruction *instr);
extern PCodeLabel *PCode_NewLabel(void);
extern void PCode_ResetBlocks(void);
extern short next_label_number;
extern void SpillCode_BuildBlockOrder(void);
extern SInt32 pcodeBlockOrderIndex;
extern struct PCodeBlock **gPCodeBlockOrder;

#ifdef __cplusplus
}
#endif

#endif
