#ifndef COMPILER_CODEGEN_H
#define COMPILER_CODEGEN_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct InterruptList {
    struct InterruptList *next;
    struct InterruptGenerationRecord *info; /* 0x04: CodeGen_ParsePragma saves interrupt options on the pragma stack */
};
#pragma options align = reset
#pragma pack(push, 1)
struct TemporaryObjectEntry {
    struct TemporaryObjectEntry *next;
    struct Object *object;
};
#pragma pack(pop)
extern SInt32 CodeGen_GetMethRecRtypeAndArgsSize(MethRec *p);
extern void CodeGen_SetIROptimizationEnabled(void);
extern void CodeGen_SetObjectSectionAndInterruptInfo(Object *obj);
extern void CodeGen_ParsePragma(HashNameNode *name);
extern void CodeGen_004332e0(void);
extern void parse_section_pragma(void);
extern void CodeGen_ParseDeclspecSection(HashNameNode *node, DeclInfo *value);
extern void CodeGen_EmitLoadAndBranchFunction(Object *a1, Object *a2, Object *a3, SInt32 a4);
extern char CodeGen_IsRegisteredObject(ObjBase *a0);
extern void CodeGen_InitializeLists(void);
extern void fn_00434660(Boolean initializationOptions);
extern void CodeGen_GenThunk(Object *stmt, Object *func, SInt32 a, SInt32 flag, SInt32 b);
extern void CodeGen_Generator(Statement *statements, Object *functionObject, Boolean context, Boolean zero);
extern void emit_name_string_address(const char *name);
extern void generate_return(ENode *enode, Boolean flag);
extern void generate_comparison_branch(ENode *enode, CLabel *context, SInt32 flag);
extern void set_block_line_and_execution_weight(SInt32 value, unsigned int initial_value, unsigned int set_flag);
extern InterruptGenerationRecord *CodeGen_FindInterruptGenerationRecord(SInt16 key);
extern void CodeGen_AssignMissingEntryValues(Statement *entry);
extern void emit_trailing_object_reg_moves(void);
extern void emit_dlocal_initialization(Object *object, SInt16 reg);
extern void allocate_registers_and_local_slots(void);
extern void allocate_saved_vrs(void);
extern void allocate_saved_fprs(void);
extern void allocate_saved_gprs(void);
extern void allocate_object_registers(void);
extern unsigned int CodeGen_GetObjCParameterOffset(MethRec *function, ObjCParameterNode *argument);
extern int fn_00432480(MethRec *record);
extern unsigned int CodeGen_GetMethRecRTypeSize(MethRec *record);
extern void bind_object_register(Object *a0, SInt16 a1);
extern void fn_00436390(ENode *expression);
extern void CodeGen_EnumerateArgumentRegisters(void (*cb)(Object *, SInt16));
extern ENode *CodeGen_MakeAltivecCall(Object *object, ENodeList *arguments);
extern void CodeGen_AllocateArgumentSlots(Object *arg1, Boolean arg2, Boolean arg3);
extern ENode *CodeGen_MakeAltivecStructCast(ENode *a, Type *type, UInt32 qual);
extern SInt32 data_00588274;
extern struct COpts copts;
extern struct Object *data_0058758c;
extern struct Object *data_005875c8;
extern struct Object *data_005875d4;
extern struct Object *data_005875d8;
extern struct Object *data_005875dc;
extern struct Object *data_005875e0;
extern struct Object *data_005875e4;
extern struct Object *data_005875e8;
extern struct Object *data_005875ec;
extern struct Object *data_005875f0;
extern struct Object *data_005875fc;
extern struct Object *data_00587604;
extern struct Object *data_0058760c;
extern struct Object *data_00587610;
extern struct Object *data_00587614;
extern struct Object *data_00587618;
extern struct Object *data_0058761c;
extern struct Object *data_00587628;
extern struct Object *data_0058762c;
extern struct Object *data_00587640;
extern struct Object *data_00587c8c;
extern struct Object *data_00587e34;
extern struct Object *data_00587e5c;
extern struct Object *data_00587e68;
extern struct Object *data_00587e6c;
extern struct Object *data_00587e7c;
extern struct Object *data_00587e80;
extern struct Object *data_00587e90;
extern struct Object *data_00587e94;
extern struct Object *data_00587e9c;
extern struct Object *data_00587ea4;
extern struct Object *data_00587eac;
extern struct Object *data_00587eb4;
extern struct Object *data_00587ec0;
extern struct Object *data_00587edc;
extern struct Object *data_00587ee4;
extern struct Object *data_00587f50;
extern struct Object *data_00587f9c;
extern struct CLabel *return_label;
extern struct Object *data_00587fec;
extern struct Object *data_00587ff0;
extern struct Object *data_00587ff4;
extern struct Object *data_00588020;
extern struct Object *data_00588038;
extern struct Object *data_0058803c;
extern struct Object *data_00588054;
extern struct Object *data_00588068;
extern struct Object *data_00588078;
extern struct Object *data_0058808c;
extern struct Object *data_00588210;
extern struct Object *data_00588214;
extern SInt32 data_00588224;
extern struct PCodeBlock *prologueBlock;
extern struct Object *data_0058823c;
extern struct Object *data_00588250;
extern void *interrupt_generation_records;
extern SInt32 has_dlocal_initialization;
extern HashNameNode *blank_name;
extern short stack_base_reg;
extern SInt16 data_00588434;
extern SInt16 data_00588476;
extern SInt16 data_00588478;
extern short gAvailableSavedGPRs;
extern int gRunLevel2Pipeline;
extern struct ObjectList *gTrailingObjectList_005876a0;
extern UInt8 gVectorArrayConversion;
extern Object *CodeGen_AllocateTemporaryObject(Type *type);
extern int CodeGen_CheckAltivecStypeMatch(ENode *a, Type *type, Boolean convert, Boolean checkAccess);
extern SInt16 next_varnumber;

#ifdef __cplusplus
}
#endif

#endif
