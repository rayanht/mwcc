#define CERROR_FILE "CObjCModern.c"
#include "compiler/common.h"
#include "compiler/CObjCModern.h"
#include "compiler/enode.h"
#include "compiler/objects.h"
#include "compiler/scopes.h"
#include "compiler/types.h"
#include "compiler/CABI.h"
#include "compiler/CDecl.h"
#include "compiler/CError.h"
#include "compiler/CExpr.h"
#include "compiler/CExpr2.h"
#include "compiler/CObjC.h"
#include "compiler/CPrepTokenizer.h"
#include "compiler/CompilerTools.h"
#include <string.h>

ENode *fn_0050a000(TypeClass *context, ENode *receiver, HashNameNode *name, char option)
{
    MessageArgument *args;
    MessageArgument *arg;
    ENodeList *extra;
    ENode *result;
    char argumentFlag;

    CError_ASSERT(89, receiver->type == EINDIRECT);
    receiver = receiver->data.monadic;
    args = CABI_SplitNameIntoMessageArguments(name, &argumentFlag);
    tk = CPrepTokenizer_GetNextToken();
    if (tk == ')') {
        if (argumentFlag == 0) {
            CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
            return nullnode();
        }
        result = CObjC_MakeMessageSend(receiver, context, args, NULL, 1, option);
        tk = CPrepTokenizer_GetNextToken();
        return result;
    }
    if (argumentFlag != 0) {
        CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
        return nullnode();
    }
    extra = NULL;
    arg = args;
    for (;;) {
        arg->expression = assignment_expression();
        if (tk == ')') {
            if (arg->next != NULL) {
                CError_ReportError(ERR_FUNCTION_CALL_DOES_NOT_MATCH_PROTOTYPE);
                return nullnode();
            }
            break;
        }
        if (tk != ',') {
            CError_ReportError(ERR_EXPRESSION_SYNTAX_ERROR);
            return nullnode();
        }
        tk = CPrepTokenizer_GetNextToken();
        arg = arg->next;
        if (arg != NULL)
            continue;
        extra = CExpr_ScanExpressionList(0);
        break;
    }
    result = CObjC_MakeMessageSend(receiver, context, args, extra, 1, option);
    tk = CPrepTokenizer_GetNextToken();
    return result;
}

ENode *CObjCModern_TryParseMethodCall(TypeClass *a, ENode *b)
{
    SInt32 state;
    HashNameNode *name;

    CPrep_GetBufferedTokenPosition(&state);
    tk = CPrepTokenizer_GetNextToken();
    CObjC_ConvertKeywordToIdentifier();
    if (tk == TK_IDENTIFIER) {
        if (!strcmp(data_00587fa0->name, "super")) {
            if (CPrepTokenizer_GetNextToken() == 0x174) {
                tk = CPrepTokenizer_GetNextToken();
                CObjC_ConvertKeywordToIdentifier();
                name = data_00587fa0;
                if (tk == TK_IDENTIFIER && CPrepTokenizer_GetNextToken() == '(')
                    return fn_0050a000(a, b, name, 1);
            }
        } else {
            name = data_00587fa0;
            if (CPrepTokenizer_GetNextToken() == '(')
                return fn_0050a000(a, b, name, 0);
        }
    }
    CPrep_SetBufferedTokenPosition(&state);
    return NULL;
}

ENode *CObjCModern_CreateAllocMessage(TypeClass *object)
{
    ENode *result;
    struct MessageArgument *name;
    name = (struct MessageArgument *)lalloc(12U);
    memclrw(name, 12U);
    name->name = GetHashNameNodeExport("alloc");
    result = create_objectrefnode(object->objcinfo->classobject);
    return CObjC_MakeMessageSend(result, object, name, NULL, 1, 0);
}

ENode *CObjCModern_MakeDeallocMessage(TypeClass *type, ENode *object)
{
    MessageArgument *record;

    record = (MessageArgument *)lalloc(sizeof(MessageArgument));
    memclrw(record, sizeof(MessageArgument));
    record->name = GetHashNameNodeExport("dealloc");
    return CObjC_MakeMessageSend(object, type, record, NULL, 0, '\0');
}
