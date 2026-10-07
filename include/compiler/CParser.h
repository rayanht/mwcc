#ifndef COMPILER_CPARSER_H
#define COMPILER_CPARSER_H

#include <setjmp.h>
#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CParseCacheNode {
    struct CParseCacheNode *next;
    struct Object *object;
    struct ENode *expr;
};
#pragma options align = mac68k
struct CParseRec {
    struct CParseRec *next;
    struct TypeClass *listOwner;
};
#pragma options align = reset
/* A parser checkpoint (tentative parsing): the state to restore and where to longjmp when an error is raised while
   data_00588240 points at it. Each gNNN keeps the global at 0x58NNNN. */
struct CParseSave {
    struct CParseSave *g240;
    jmp_buf buf;
    NameSpace *g24c;
    struct TypeClass *g040;
    Object *g238;
    struct ObjectReferenceEntry *g134;
    SInt32 g7ecc;
    UInt8 g4f8;
};
struct ClassTypeLink {
    struct ClassTypeLink *next;
    struct TypeClass *type;
};
#pragma options align = mac68k
#pragma options align = reset
struct PendingObjectClass {
    struct PendingObjectClass *next;
    struct Object *object;
    struct TypeClass *theclass;
};
extern TypeIntegral stlongdouble;
extern Type stvoid;
extern TypeFunc data_0055d5e8;
/* The tokens the lexer returns in tk: a character is its own token, the rest are these (keywords as the lexer names them,
 * operators in the lexer's order); numbers, as tk is a short. */
#define TK_EOF 0
#define TK_EOL -7
#define TK_NEG6 -6
#define TK_STRING_WIDE -5
#define TK_STRING -4
#define TK_IDENTIFIER -3
#define TK_FLOATCONST -2
#define TK_INTCONST -1
#define TK_NOT 33
#define TK_BITAND 38
#define TK_BITOR 124
#define TK_COMPL 126
#define TK_AUTO 256
#define TK_REGISTER 257
#define TK_STATIC 258
#define TK_EXTERN 259
#define TK_TYPEDEF 260
#define TK_INLINE 261
#define TK_VOID 262
#define TK_CHAR 263
#define TK_SHORT 264
#define TK_INT 265
#define TK_FLOAT 267
#define TK_DOUBLE 268
#define TK_SIGNED 269
#define TK_UNSIGNED 270
#define TK_STRUCT 271
#define TK_UNION 272
#define TK_ENUM 273
#define TK_CLASS 274
#define TK_UU_VECTOR 283
#define TK_BOOL 284
#define TK_WCHAR_T 285
#define TK__COMPLEX 286
#define TK__IMAGINARY 287
#define TK_TYPENAME 288
#define TK_CONST 289
#define TK_VOLATILE 290
#define TK_PASCAL 291
#define TK_UU_DECLSPEC 292
#define TK_UU_STDCALL 293
#define TK_UU_CDECL 294
#define TK_UU_FASTCALL 295
#define TK_UU_FLOATCALL 296
#define TK_UU_FAR 297
#define TK_EXPLICIT 298
#define TK_MUTABLE 299
#define TK_ONEWAY 300
#define TK_IN 301
#define TK_INOUT 302
#define TK_OUT 303
#define TK_BYCOPY 304
#define TK_BYREF 305
#define TK_ASM 310
#define TK_CASE 311
#define TK_DEFAULT 312
#define TK_IF 313
#define TK_ELSE 314
#define TK_SWITCH 315
#define TK_WHILE 316
#define TK_FOR 318
#define TK_CONTINUE 320
#define TK_BREAK 321
#define TK_RETURN 322
#define TK_SIZEOF 323
#define TK_CATCH 324
#define TK_DELETE 325
#define TK_FRIEND 326
#define TK_NEW 327
#define TK_OPERATOR 328
#define TK_PRIVATE 329
#define TK_PROTECTED 330
#define TK_PUBLIC 331
#define TK_TEMPLATE 332
#define TK_THIS 333
#define TK_THROW 334
#define TK_TRY 335
#define TK_VIRTUAL 336
#define TK_CONST_CAST 338
#define TK_DYNAMIC_CAST 339
#define TK_NAMESPACE 340
#define TK_REINTERPRET_CAST 341
#define TK_STATIC_CAST 342
#define TK_USING 343
#define TK_TRUE 344
#define TK_FALSE 345
#define TK_TYPEID 346
#define TK_EXPORT 347
#define TK_MULT_ASSIGN 348
#define TK_DIV_ASSIGN 349
#define TK_MOD_ASSIGN 350
#define TK_ADD_ASSIGN 351
#define TK_SUB_ASSIGN 352
#define TK_SHL_ASSIGN 353
#define TK_SHR_ASSIGN 354
#define TK_AND_ASSIGN 355
#define TK_XOR_ASSIGN 356
#define TK_OR_ASSIGN 357
#define TK_LOGICAL_OR 358
#define TK_LOGICAL_AND 359
#define TK_LOGICAL_EQ 360
#define TK_LOGICAL_NE 361
#define TK_LESS_EQUAL 362
#define TK_GREATER_EQUAL 363
#define TK_SHL 364
#define TK_SHR 365
#define TK_INCREMENT 366
#define TK_DECREMENT 367
#define TK_ARROW 368
#define TK_ELLIPSIS 369
#define TK_DOT_STAR 370
#define TK_ARROW_STAR 371
#define TK_COLON_COLON 372
#define TK_AT_INTERFACE 373
#define TK_AT_IMPLEMENTATION 374
#define TK_AT_PROTOCOL 375
#define TK_AT_END 376
#define TK_AT_PRIVATE 377
#define TK_AT_PROTECTED 378
#define TK_AT_PUBLIC 379
#define TK_AT_CLASS 380
#define TK_AT_SELECTOR 381
#define TK_AT_ENCODE 382
#define TK_AT_DEFS 383
#define TK_RESTRICT 388
#define TK_UU_ATTRIBUTE 389
extern SInt16 tk;
extern NameSpaceName *runtime_operator_namespace_name;
extern NameSpaceName *data_00588008;
extern void CParser_CheckAnonymousUnion(DeclInfo *context, char flag);
extern Boolean CParser_IsAnonymousClass(Type **ptype, Boolean flag);
extern void CParser_Cleanup(void);
extern void CParser_GetDeclSpecs(DeclInfo *state, Boolean allowObject);
extern unsigned char CParser_CheckTemplateClassScope(Type *type);
extern int parse_dtype_specifiers(DeclInfo *state);
extern void CParser_ParseDeclSpec(DeclInfo *st, int arg2);
extern void CParser_ParseAttribute(Type *type, DeclInfo *function);
extern void TypedefDeclInfo(DeclInfo *slot, Type *type, UInt32 quals);
extern TypeIntegral *select_builtin_type(SInt16 token, SInt16 lengthModifier, SInt16 signModifier);
extern Boolean CParser_TryParamList(int parserOption);
extern UInt8 islookaheaddeclaration(void);
extern Boolean isdeclaration(Boolean option1, Boolean option2, Boolean option3, short option4);
extern unsigned char test_declaration(Boolean parseDeclaration, Boolean requireValue, Boolean declarationFlag,
                                      short terminator);
extern void appendmember(TypeStruct *s, StructMember *m);
extern Boolean Type_IsUnsigned(Type *type);
extern SInt16 iscpp_typeequal(Type *t1, Type *t2);
extern Boolean is_arglistsame(FuncArg *a, FuncArg *b);
extern SInt16 CParser_CompareArgLists(FuncArg *a, FuncArg *b);
extern SInt16 is_typeequal(Type *t1, Type *t2);
extern Boolean is_arglist_default_promoted(FuncArg *arg);
extern Object *CParser_FindClassMemberOrNamespaceFunctionObject(Type *type, Boolean useAlternate, Boolean skipLookup);
extern TypeIntegral *atomtype(void);
extern Object *CParser_NewFunctionObject(volatile DeclInfo *param);
extern Object *CParser_NewObject(DeclInfo *declaration);
extern Object *CParser_NewLocalDataObject(DeclInfo *declaration, unsigned int addToList);
extern HashNameNode *CParser_AppendUniqueNameFile(char *prefix);
extern HashNameNode *CParser_AppendUniqueName(char *name);
extern HashNameNode *CParser_GetUniqueName(void);
extern unsigned int fn_004905c0(unsigned int a0);
extern void CParser_PrintUniqueID(char *p);
extern SInt32 CParser_GetUniqueID(void);
extern Type *CParser_GetWCharType(void);
extern Type *CParser_GetBoolType(void);
extern FuncArg *CParser_NewFuncArg(void);
extern Boolean CParser_00490660(SInt16 *operatorToken, Boolean allowConversion);
extern void fn_004908d0(void);
extern unsigned int CParser_PrependClassTypeLink(TypeClass *type);
extern void CParser_NewCallBackAction(Object *object, TypeClass *theclass);
extern void CParser_RegisterSingleExprFunction(Object *object, ENode *expr);
extern void CParser_PrependClassParseRec(TypeClass *type);
extern Object *CParser_NewCompilerDefFunctionObject(void);
extern Object *CParser_NewCompilerDefDataObject(void);
extern void CParser_Setup(void);
extern Boolean CParser_IsNullOrAtOrDollarPrefixedName(HashNameNode *name);
extern void CParser_CallBackAction(Object *key);
extern char CParser_HasInternalLinkage(Object *obj);
extern UInt8 is_volatile_object(Object *object);
extern UInt8 CParser_IsVolatile(Type *type, unsigned int qualifiers);
extern StructMember *ismember(Type *type, HashNameNode *name);
extern Boolean CParser_IsVirtualFunction(Object *object, TypeClass **firstValue, UInt32 *secondValue);
extern UInt32 CParser_GetCVTypeQualifiers(Type *type, SInt32 qual);
extern void CParser_UpdateObject(Object *object, volatile DeclInfo *record);
extern SInt16 GetPrec(short token);
extern void fn_0048c220(char processInput);
extern Boolean is_pascal_object(Object *object);
extern UInt8 CParserIsVolatileExpr(ENode *node);
extern Boolean CParserIsConstExpr(ENode *expr);
extern Boolean is_const_object(Object *object);
extern UInt8 CParser_IsConst(Type *type, unsigned int qual);
extern unsigned int CParser_GetTypeQualifiers(Type *type, unsigned int qual);
extern Type *CParser_RemoveTopMostQualifiers(Type *type, UInt32 *qual);
extern SInt32 CParser_GetOperator(UInt8 kind);
extern unsigned short is_memberpointerequal(Type *type, Type *other);
extern Boolean is_funcarg_list_same(FuncArg *left, FuncArg *right);
extern HashNameNode *CParser_NameConcat(char *first, char *second);
extern void initialize_runtime_objects(void);
extern Object *CParser_CreateObject(struct DeclInfo *record);
extern Object *CParser_NewAliasObject(Object *a0, int a1);
extern void fn_00490210(Object *object, volatile DeclInfo *record);
extern SInt16 is_typesame(Type *e1, Type *e2);
extern void cparser(void);
extern void parse_declaration(DeclInfo *p);
extern void parse_namespace_declaration(void *declaration);
extern void parse_linkage_specification(DeclInfo *decl);
extern void CParser_ParseGlobalDeclaration(void);
extern Object *CParser_ParseObject(void);
extern Boolean CParser_ReInitRuntimeObjects(Boolean flag);
extern Object *CParser_NewRTFunc(Type *returnType, HashNameNode *name, char mangleName, int argumentCount, ...);
extern struct Object *DAT_005870d8;
extern struct Object *destructor_aware_call_rtfunc;
extern struct Object *DAT_005875a0;
extern struct Object *DAT_00587654;
extern struct Object *DAT_00587678;
extern struct Object *rt_memberpointercompare;
extern struct Object *DAT_0058769c;
extern struct Object *DAT_005876c0;
extern struct Object *memberpointercompare_func;
extern struct Object *class_array_initializer;
extern struct Object *DAT_00587ed0;
extern struct Object *typeid_func;
extern struct Object *DAT_00587f80;
extern struct Object *DAT_00588060;
extern struct Object *destructor_aware_call_func;
extern struct Object *DAT_00588260;
extern struct Object *som_ref_node_rtfunc;
extern struct Object *som_ref_node_runtime_object;
extern struct Object *DAT_00588278;
extern struct Object *DAT_005882a4;
extern unsigned char DAT_0058844a;
extern unsigned char DAT_0058848a;
extern unsigned char DAT_0058852e;
extern struct BufferedToken declaration_token;
extern FuncArg data_00584748;
extern struct ObjectReferenceEntry *object_reference_stack;
extern struct Object *data_0058717c;
extern struct Object *member_function_pointer_call_rtfunc;
extern struct Object *cast_member_pointer_func;
extern struct NameSpaceName *data_00587680;
extern struct NameSpaceName *data_00587e64;
extern struct Object *data_00587fd0;
extern struct PendingObjectClass *pending_object_classes;
extern struct Object *dynamic_cast_object;
extern SInt32 data_00588454;
extern Type data_0058847c;
extern unsigned short _DAT_0058844c;
extern unsigned short _DAT_0058848c;
extern unsigned int _DAT_0058843e;
extern unsigned int _DAT_0058847e;
extern TypeIntegral stwchar;
extern TypePointer void_ptr;
extern Boolean CParser_IsPublicRuntimeObject(ObjBase *arg);

#ifdef __cplusplus
}
#endif

#endif
