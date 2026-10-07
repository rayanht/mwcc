#ifndef COMPILER_CTEMPLATENEW_H
#define COMPILER_CTEMPLATENEW_H

#include <setjmp.h>
#include "compiler/common.h"
#include "compiler/templates.h"
#include "compiler/objects.h"
#include "compiler/types.h"
#include "compiler/CInline.h"
#include "compiler/CPrep.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma options align = mac68k
struct ParserPosition {
    long position;
};
#pragma options align = reset
#pragma options align = mac68k
struct TemplateScopeState {
    struct NameSpace *scope;
    struct NameSpace *linkedNamespace;
};
#pragma options align = reset
extern Boolean CTemplateNew_InstantiatePendingTemplates(void);
extern void CTemplateNew_CompileObject(struct TemplClass *templateClass, TemplClassInst *context,
                                       TemplateMember *source, Object *object, Boolean reset);
extern Boolean CTemplateNew_InstantiateFunction(TemplateFunction *definition, TemplFuncInstance *specialization,
                                                Boolean report);
extern void CTemplateNew_ParseTemplateDeclaration(TypeClass *templateClass);
extern void parse_explicit_template_specialization(void);
extern void parse_explicit_template_instantiation(void);
extern void parse_function_template_declaration(TemplateScopeState *stack, TemplParam *params, TypeClass *tclass,
                                                SInt32 *startOffset);
extern void parse_template_member_definition(void *context, TypeClass *template_info, DeclInfo *declaration,
                                             SInt32 *position);
extern unsigned char instantiate_members(struct TemplClass *templ, TemplClassInst *state, char force);
extern void CTemplateNew_ParseFuncDef(Object *object, TemplClassInst *context, TypeClass *tclass);
extern Boolean CTemplateNew_InstantiateInlineTemplateObject(Object *obj);
extern UInt8 CTemplateNew_LinkTemplateScope(DeclInfo *context, TypeTemplDep *request, NameSpace **destination);
extern TemplParam *parse_template_parameter_list(NameSpace *parserState, unsigned char mode);
extern TemplParam *parse_template_parameter(NameSpace *owner, TemplParam *value, short memberValue, char memberByte);
extern void fn_004f0000(void);
extern void CTemplateNew_Reset(void);
extern ENode *parse_non_type_template_argument(Type *targetType, unsigned int qualifiers);
extern Type *CTemplTool_GetSelfRefTemplate(struct TemplClass *record);
extern struct TemplArg *CTemplateNew_ParseTemplateArguments(struct TemplParam *arg, char flag);
extern void skip_balanced_angle_tokens(void);
extern SInt32 source_line;
extern char template_recordbrowseinfo;
extern TemplArg *parse_template_arguments(struct TemplClass **classType, TemplArg **result);

#ifdef __cplusplus
}
#endif

#endif
