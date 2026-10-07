#ifndef COMPILER_CEXPR2_H
#define COMPILER_CEXPR2_H

#include "compiler/common.h"
#include "compiler/CSOM.h"
#include "compiler/CScope.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct ArgMatch {
    struct Object *object;
    SInt16 score1Count;
    SInt16 score2Count;
    SInt16 score3Count;
    SInt16 score3Value;
    SInt16 qualificationPenalty;
    SInt16 score4Count;
    SInt16 score4Value0;
    SInt16 score4Value1;
    SInt16 score4Value2;
    SInt16 score4Value3;
    SInt16 score4Value4;
    struct ObjectList *list;
};
#pragma options align = reset
struct ComparisonValues {
    SInt16 kind1;
    SInt16 kind2;
    SInt16 kind3;
    SInt16 kind3Weight;
    SInt16 qualifierMatches;
};
#pragma options align = mac68k
struct ConIteratorList {
    struct ConIteratorList *next;
    struct ConIterator *iter;
};
#pragma options align = reset
#pragma pack(push, 2)
struct ConIterator {
    struct ConIterator *parent;
    struct ConIteratorList *children;
    struct TypeClass *tclass;
};
#pragma pack(pop)
struct ConIterator;
#pragma pack(push, 2)
struct ConversionIterator {
    CScopeObjectIterator objiter;
    struct ConIterator myconiter;
    struct ConIterator *coniter;
};
#pragma pack(pop)
#pragma pack(push, 2)
struct MemberCallArguments {
    struct ENodeList *arguments;
    struct FuncArg *parameters;
};
#pragma pack(pop)
extern ENode *intconstnode(Type *valueType, SInt32 value);
extern ENode *makemonadicnode(ENode *expr, UInt8 type);
extern ENode *create_objectnode(Object *object);
extern ENode *build_array_allocation_expression(Type *type, ENodeList *placement, char global);
extern ENode *make_class_member_or_global_call(Type *type, ENodeList *args, char global, Boolean flag);
extern void parse_pointer_and_array_declarator(Type **type, char allowNonconstant);
extern ENode *CExpr_ConstructObject(Type *type, ENode *ptr, ENodeList *arguments, Boolean keepResult,
                                    Boolean initializeVirtualBases, Boolean forceConstructor, SInt32 constructorFlags,
                                    Boolean suppressAccessCheck);
extern Boolean CExpr_CheckOperator(short token, ENode *left, ENode *right, BinaryOperatorResult *out);
extern Boolean select_operator_operand_types(Type *t1, Type *t2, SInt16 op);
extern Boolean CExpr_CheckOperatorConversion(short token, ENode *left, ENode *right, ENodeList *args,
                                             BinaryOperatorResult *out);
extern char try_class_conversion_to_kind(ENode *expr, short kind, BinaryOperatorResult *result);
extern unsigned char convert_class_binary_operands(ENode *left, ENode *right, BinaryOperatorResult *result);
extern Boolean CExpr2_0046e3e0(Type *type, SInt16 op);
extern ENode *CExpr_MakeFunctionCall(ENode *expr, ENodeList *args);
extern ENode *convert_memberfunc_to_setconst_or_objref(ENode *expr);
extern ENode *CExpr2_0046e9d0(Object *obj, Type *functype, ENodeList *args);
extern Boolean has_class_objref(ENode *node);
extern void make_funccall_with_dexprs(ENode *funcref, ENodeList *args, TypeFunc *ftype, FuncArg *arglist);
extern void CExpr_FuncArgMatch(NameSpaceObjectList *source, void *context, ENodeList *objects, ArgMatch *state,
                               ENode *mode, char exclude);
extern Boolean CExpr_GetFuncMatchArgs(Object *obj, ENodeList *arguments, ENode *instance, MemberCallArguments *result);
extern Boolean CExpr_MatchCompare(Object *obj, ArgMatch *dst, ArgMatch *src);
extern SInt16 assign_check(ENode *e1, Type *t2, SInt32 a3, Boolean a4, Boolean a5, Boolean a6);
extern ENode *get_address_of_temp_copy(ENode *expr, char materialize);
extern void CExpr_CheckArithmConversion(ENode *node, Type *type);
extern ENode *CExpr2_ConvertScalarOperand(ENode *result, Boolean integerOnly, Boolean preferBool);
extern Object *CExpr_ConversionIteratorNext(ConversionIterator *ctx);
extern void build_convertible_bases_tree(ConIterator *self);
extern SInt32 check_standard_conversion(ENode *node, Type *ty, Boolean convert, Boolean checkAccess);
extern ENode *CExpr_ConvertToBool(ENode *node, Boolean flag);
extern SInt32 match_overloaded_function_pointer(NameSpaceObjectList *list, void *arg2, Type *type, UInt8 flag);
extern ENode *CExpr2_004719c0(BClassList *scope, BClassList *baseList, ENode *node, UInt8 access, Boolean checkAccess);
extern ENode *CExpr_ClassPointerCast(BClassList *path, ENode *node, Boolean checkNull);
extern SInt32 check_member_pointer_conversion(Type *type, ENode *expr, Boolean convert);
extern SInt16 compare_short_arrays_lexicographically(SInt16 *left, SInt16 *right, Boolean compareFifth);
extern Boolean CExpr2_UpdateArgMatchScores(Type *target, UInt32 qualifiers, ENode *expression, ArgMatch *scores);
extern void CExpr_MatchCV(Type *ty1, UInt32 quals1, Type *ty2, UInt32 quals2, ArgMatch *ctx);
extern ENode *CExpr_FuncCallSix(Object *obj, ENode *a1, ENode *a2, ENode *a3, ENode *a4, ENode *a5, ENode *a6);
extern ENode *funccallexpr(Object *func, ENode *arg1, ENode *arg2, ENode *arg3, ENode *arg4);
extern ENode *CExpr_AdjustFunctionCall(ENode *p);
extern ENode *CExpr_IsTempConstruction(ENode *e, Type *type, ENode **out);
extern ENode *CExpr_New_EINDIRECT_Node(Object *obj);
extern ENode *create_objectrefnode(Object *obj);
extern ENode *CExpr_MakeObjRefNode(Object *obj, Boolean flag);
extern Boolean CExpr_IsLValue(ENode *expr);
extern ENode *CExpr_TempModifyExpr(ENode *expr);
extern ENode *get_indirect_operand(ENode *node);
extern ENode *CExpr2_00473720(ENode *expr, Type *type);
extern CInt64 CExpr_IntConstConvert(Type *type, Type *otherType, CInt64 value);
extern ENode *forceintegral(ENode *node);
extern ENode *CExpr2_RewriteExprToTemp(ENode *a);
extern UInt8 CExpr_IsOne(ENode *expr);
extern SInt16 isnotzero(ENode *node);
extern SInt16 CExpr2_IsZero(ENode *node);
extern ENode *makecommaexpression(ENode *a, ENode *b);
extern ENode *makediadicnode(ENode *left, ENode *right, UInt8 ty);
extern ENode *CExpr2_ReturnNode(ENode *node);
extern ENode *CExpr2_ReturnENode(ENode *ene);
extern ENode *CExpr_NewENode(UInt8 kind);
extern ENode *intconstnode(Type *valueType, SInt32 value);
extern ENode *nullnode(void);
extern ENode *CExpr2_NewESCOPEBEGINNode(Type *value, unsigned int withAuxiliary);
extern unsigned char has_indirect_class_objref(ENode *expr, TypeClass *arg2);
extern ENode *CExpr2_NewENEWEXCEPTIONARRAYNode(unsigned int value);
extern unsigned int fn_0046cea0(void);
extern void match_function_arguments(Object *signature, FuncArg *argument, ENodeList *objects, ArgMatch *value);
extern ENode *CExpr_ConvertToIntegral(ENode *expr);
extern void init_comparison_values(unsigned int kind, ComparisonValues *counts, Type *sourceType,
                                   unsigned int sourceQual, Type *targetType, unsigned int targetQual,
                                   unsigned int flag);
extern UInt8 CExpr_AllBitsSet(ENode *p);
extern ENode *replace_expr_tree_nodes(ENode *node);
extern void CExpr_SearchExprTree(ENode *expr, void (*value)(ENode *), SInt32 count, ...);
extern void CExpr2_004743d0(ENode *e);
extern ENode *CExpr_VarArgPromotion(ENode *expr, Boolean allowWarning);
extern ENode *CExpr_GenericFuncCall(BClassList *scope, ENode *instance, Boolean qualified, Object *function,
                                    void *candidates, void *kind, ENodeList *arguments, SInt32 extraArguments,
                                    Boolean flag9, Boolean checkAccess);
extern SInt16 user_assign_check(ENode *operand, Type *targetType, UInt32 targetQual, Boolean diagnose,
                                Boolean allowExplicit, Boolean conversionMode);
extern ENode *CExpr_LValue(ENode *expr, Boolean checkConst, Boolean reportError);
extern void build_destructor_aware_call(ENode *expression, Type *type, Boolean skipLookup);
extern ENode *make_call_with_optional_size_arg(Object *func, ENode *arg, Type *argtype);
extern ENode *scannew(char global);
extern struct Object *array_allocation_runtime_function;
extern Boolean (*DAT_00587fd8)(int value, struct Object *object);
extern FuncArg data_00583098;
extern struct ENode *converted_expr;
extern UInt8 data_0058850e;
extern UInt8 data_0058852b;
extern TypeIntegral stsignedlong;
extern struct ENode *scandelete(char mode);
struct Type;
struct Object;
struct Object;
struct Object;
struct Object;
struct Object;
struct Object;
struct Object;
struct Object;

#ifdef __cplusplus
}
#endif

#endif
