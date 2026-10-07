#ifndef COMPILER_CEXPR_H
#define COMPILER_CEXPR_H

#include <setjmp.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct BinaryOperatorResult {
    struct ENode *expression;
    struct ENode *left;
    struct ENode *right;
};
#pragma options align = reset
#pragma options align = mac68k
struct EMemberInfo {
    struct BClassList *path; /* 0x00: CExpr.c copies nameResult->basePath; getpointertomemberfunc reads bases */
    struct ENode *expr;      /* 0x04: CExpr.c stores the member access expression; getpointertomemberfunc checks it */
    struct NameSpaceObjectList *list; /* 0x08: make_member_function_esetconst supplies the candidate list */
    struct TemplArg *
        templargs; /* 0x0c: make_member_function_esetconst obtains CTemplateNew_ParseTemplateArguments; CExpr.c copies objlist.templargs */
    Boolean is_qualified; /* 0x10: CExpr.c copies nameResult->is_qualified */
    UInt8 addressTaken;   /* 0x11: make_memberpointer sets 1; getpointertomemberfunc checks explicit address taking */
    Boolean isambig;      /* 0x12: CExpr.c copies nameResult->isambig */
};
#pragma options align = reset
#pragma pack(push, 1)
struct MemberPointerInitializer {
    SInt32 adjustment;
    SInt32 index;
    SInt32 tableSize;
};
#pragma pack(pop)
extern TypeIntegral stbool;
extern void CExpr_CheckUnusedExpression(ENode *node);
extern ENode *conv_assignment_expression(void);
extern ENode *assignment_expression(void);
extern ENode *parse_arithmetic_binary_expression(ENode *left, char op, SInt16 token);
extern ENode *parse_arithmetic_compound_assignment(ENode *e, char operatorKind, SInt16 operation);
extern ENode *parse_assignment_operator(ENode *expr, UInt8 op, SInt16 arg3);
extern ENode *conditional_expression(void);
extern ENode *CExpr_New_ECOND_Node(ENode *a, ENode *b, ENode *c);
extern ENode *parse_binary_expression(ENode *left, unsigned char precedence, char conditional);
extern ENode *CExpr_NewDyadicNode(ENode *left, UInt8 op, ENode *right);
extern ENode *make_logical_or_node(ENode *left, ENode *right);
extern ENode *CExpr_New_ELAND_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_EOR_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_EXOR_Node(ENode *e1, ENode *e2);
extern ENode *CExpr_New_EAND_Node(ENode *left, ENode *right);
extern ENode *fold_or_make_comparison_node(ENode *left, ENode *right);
extern ENode *CExpr_MakeComparisonNode(ENode *left, ENode *right);
extern ENode *memberpointercompare(UInt8 op, ENode *e1, ENode *e2);
extern ENode *CExpr_New_EGREATEREQU_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_EGREATER_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_ELESSEQU_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_ELESS_Node(ENode *left, ENode *right);
extern ENode *simplify_unsigned_zero_comparison(ENode *node, Boolean flag1, Boolean flag2);
extern void make_pointer_comparison(UInt8 op, ENode *n1, ENode *n2);
extern ENode *CExpr_New_ESHR_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_ESHL_Node(ENode *left, ENode *right);
extern ENode *CExpr_New_EMODULO_Node(ENode *left, ENode *right, Boolean suppressWarning);
extern ENode *cast_expression(void);
extern ENode *CExpr_CastMemberPointer(ENode *value, TypeMemberPointer *sourceType, TypeMemberPointer *targetType);
extern ENode *unary_expression(void);
extern ENode *CExpr_New_EMONMIN_Node(ENode *ene);
extern ENode *CExpr_New_ELOGNOT_Node(ENode *expr);
extern ENode *parse_postfix_expression(Boolean allowSpecial);
extern ENode *scan_pseudo_destructor_call(ENode *node);
extern ENode *parse_primary_expression(Boolean expressionMode);
extern int CExpr_004f8a40(Type *p);
/* a type seen through its integral code or enumeration payload, as encode_type_bits reads it */
#pragma options align = mac68k
typedef struct TypeKind {
    UInt8 type;
    SInt32 size;
    union {
        UInt8 integral;
        struct {
            NameSpace *nspace;
            ObjEnumConst *enumlist;
            struct TypeKind *enumtype;
        } tenum;
    } u;
} TypeKind;
#pragma options align = reset
extern UInt32 encode_type_bits(TypeKind *e);
extern ENode *scan_vec_step(void);
extern ENode *CExpr_MakeNameLookupResultExpr(CScopeParseResult *p);
extern ENode *make_member_function_esetconst(CScopeParseResult *candidates);
extern ENode *scan_explicit_conversion(Type *type, SInt32 qualifiers);
extern ENode *CExpr_DoExplicitConversion(Type *classType, unsigned long qualifiers, ENodeList *arguments);
extern ENode *CExpr_AssignmentPromotion(ENode *expression, Type *type, unsigned short qualifiers, int mode);
extern ENode *oldassignmentpromotion(ENode *e, Type *t, SInt16 sz, SInt32 flag);
extern void check_implicit_pointer_qual_conversion(ENode *a, Type *b, SInt16 c);
extern ENode *CExpr_GeneratePointerAndRewriteConst(ENode *expr);
extern unsigned int fn_004f8ae0(const signed char *kind);
extern unsigned int encode_kind(unsigned char kind);
extern ENode *CExpr_IntegralConstOrDepExpr(void);
extern ENode *s_expression(void);
extern ENode *CExpr_ParseCommaExpression(void);
extern Boolean fn_004f0f40(ENode *node);
extern ENode *CExpr_New_EPRECOMP_Node(ENode *node, ENode *label);
extern ENode *CExpr_New_EBINNOT_Node(ENode *node);
extern void make_static_method_setconst(ObjectList *objects);
extern ENode *scan_member_function_pointer_call(ENode *expr);
extern Type *scan_type_or_expression_type(void);
extern ENodeList *CExpr_ScanExpressionList(char parenthesized);
extern ENode *CExpr_PointerGeneration(ENode *node);
extern ENode *pointer_generation(ENode *node);
extern ENode *CExpr_New_ESUB_Node(ENode *e1, ENode *e2);
extern ENode *CExpr_New_EADD_Node(ENode *left, ENode *right);
extern ENode *make_pointer_subtraction(ENode *n1, ENode *n2);
extern ENode *add_pointer_offset(ENode *node, ENode *arg);
extern ENode *convert_to_stunsignedlong_size(ENode *node);
extern SInt16 canadd(ENode *p, UInt32 n);
extern SInt16 add_to_expression_constant(ENode *node, CInt64 v);
extern ENode *CExpr_New_EDIV_Node(ENode *left, ENode *right, Boolean flag);
extern ENode *CExpr_New_EMUL_Node(ENode *lhs, ENode *rhs);
extern void unify_arithmetic_rtypes(ENode **leftp, ENode **rightp, SInt32 unused);
extern void CExpr_004fb400(ENode *e);
extern void optimizecomm(ENode *expression);
extern unsigned char get_binary_operator_info(short token, unsigned char *operatorInfo);
extern ENode *CExpr_MemberPointerConversion(ENode *enode, Type *type, Boolean flag);
extern ENode *classargument(ENode *node);
extern CInt64 CExpr_IntegralConstExprType(Type **ptype);
extern void convert_right_and_make_diadic_node(ENode *left, ENode *right, UInt8 type);
extern ENode *getnodeaddress(ENode *node, Boolean flag);
extern SInt32 scansizeof(void);
extern ENode *checkreference(ENode *e);
extern ENode *CExpr_RewriteConst(ENode *enode);
extern ENode *getpointertomemberfunc(ENode *node, Type *targetType, Boolean initialize);
extern void *make_memberpointer(ENode *node);
extern ENode *make_scope_parse_result_expr(CScopeParseResult *nameResult, ENode *expr, Boolean allowMemberReference,
                                           Boolean allowFunctionCall);
extern ENode *member_pointer_expression(void);
extern ENode *do_typecast(ENode *expr, Type *type, UInt32 qual);
extern void CExpr_CheckUnwantedAssignment(ENode *node);
extern CInt64 fn_004f0b30(void);
extern Type data_0055d5c0;
extern char non_type_template_argument_mode;
struct ENode;

#ifdef __cplusplus
}
#endif

#endif
