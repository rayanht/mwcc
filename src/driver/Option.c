#define CERROR_FILE "Option.c"
#include "compiler/common.h"
#include "driver/Option.h"
#include "driver/Arguments.h"
#include "driver/AssertionFailure.h"
#include "driver/CLIO.h"
#include "driver/ClientGlue.h"
#include "driver/Help.h"
#include "driver/Parameter.h"
#include "driver/ParserErrors.h"
#include "driver/ParserFace.h"
#include "driver/ParserGlue-eabi-ppc-cc.h"
#include "driver/Projects.h"
#include "driver/Targets.h"
#include "driver/Utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static struct OptionList *option_lists[32];
static OptionList optionList;

#define oStack data_00586d20
#define oStackPtr data_00587594

#define MAXSTACK 8
#define OPTION_ASSERT(cond, line) ((cond) ? (void)0 : CLIO_ReportAssertionFailure(#cond, "Option.c", line))
/* Declarations gathered from the merged files. */
OStack data_00586d20[MAXSTACK];

void push_option(void *a)
{
    Option_Push(1, a, NULL);
}

void push_option_arg(Option *opt, char *arg)
{
    int flags = 2;
    if (opt && (opt->avail & 2)) {
        if (Utils_MatchStringSegments(opt->names, arg, opt->avail & 4, 2))
            flags |= 0x80;
    }
    Option_Push(flags, opt, ClientGlue_DuplicateString(arg));
}

void fn_0041c1ae(char *destination)
{
    OStack *entry;

    entry = Option_PopStack(2U);
    if (destination != NULL) {
        strcpy(destination, entry->value);
    }
    free(entry->value);
}

void pop_option_stack(void)
{
    Option_PopStack(1);
}

void fn_0041c1e2(void)
{
    data_00587594 = 0;
    return;
}

int fn_0041c1eb(void)
{
    return data_00587594;
}

void Option_Push(short flags, void *a, char *b)
{
    OPTION_ASSERT(oStackPtr<MAXSTACK, 103);
    if (oStackPtr > 0) {
        short prev = (flags & 1) ? 2 : (flags & 2) ? 1 : (flags & 4) ? 2 : -1;
        OPTION_ASSERT((oStack[oStackPtr-1].flags & prev), 110);
    }
    oStack[oStackPtr].name = a;
    oStack[oStackPtr].value = b;
    oStack[oStackPtr].flags = flags;
    oStackPtr++;
}
#undef OPTION_ASSERT

#define OPTION_ASSERT(cond, line)                                                                                      \
    if (!(cond))                                                                                                       \
    CLIO_ReportAssertionFailure(#cond, "Option.c", line)
OStack *Option_PopStack(short flags)
{
    OPTION_ASSERT(oStackPtr>0, 121);
    --oStackPtr;
    OPTION_ASSERT((oStack[oStackPtr].flags & flags), 123);
    return &oStack[oStackPtr];
}

void format_ostack(char *str, short flags)
{
    char *buf = str;
    OStack *os;
    int i = 0;
    int level = 0;
    for (os = &oStack[i]; i < oStackPtr; i++, os++) {
        if (!flags || !(os->flags & 0x20)) {
            if (os->flags & 4) {
                OStack *prev = os - 1;
                if (!(os[-1].flags & 0x40)) {
                    if ((level == 1 || level == 2) && !(prev->flags & 0x80))
                        *buf++ = (flags & 0x20) ? '\n' : ' ';
                    os[-1].flags |= 0x40;
                } else if (level == 2)
                    *buf++ = ',';
                else if (level == 3)
                    *buf++ = '=';
                strcpy(buf, os->name);
                buf += strlen(buf);
            } else if (os->flags & 2) {
                if (level == 1)
                    *buf++ = *data_00587eec;
                else if (level == 2) {
                    OStack *pp = os - 2;
                    if (!(os[-1].flags & 0x40) && !(pp->flags & 0x80))
                        *buf++ = (flags & 0x20) ? '\n' : ' ';
                }
                if (os[-1].flags & 0x40) {
                    if (level == 2) {
                        if (flags & 0x20)
                            *buf++ = ',';
                        else
                            *buf++ = ' ';
                    }
                } else if (level == 3)
                    *buf++ = '=';
                os[-1].flags |= 0x40;
                strcpy(buf, os->value);
                buf += strlen(buf);
            }
            if (flags & 0x20)
                os->flags |= 0x20;
        }
        if (os->flags & 1)
            level++;
    }
}
void Option_FormatOStackToList(void *list)
{
    char buf[0x1000];
    char *p;
    format_ostack(buf, 0x20);
    if ((p = strchr(buf, '\n')) != NULL) {
        *p++ = 0;
        fn_0040fbe1(list, 2, buf);
        fn_0040fbe1(list, 1, NULL);
    } else
        p = buf;
    fn_0040fbe1(list, 2, p);
}

void Option_ResetOptionLists(void)
{
    option_list_count = 0;
    option_capacity = 0;
    option_count = 0;
    if (optionList.options)
        free(optionList.options);
    optionList.options = NULL;
    optionList.flags = ((fn_0041c8ba() & 0x100) ? 0x100 : 0) | ((fn_0041c8ba() & 0x40) ? 0x200 : 0) |
                       ((fn_0041c8ba() & 0x80) ? 0x400 : 0);
    fn_0041c1e2();
}

OptionList *Option_GetOptionList(void)
{
    return &optionList;
}

void add_option(Option *option)
{
    Option *opt = option;
    int i;
    Option *tmp;
    if (option_count >= option_capacity) {
        option_capacity += 32;
        optionList.options = ToolHelpers_ResizeBuffer("options", optionList.options, (option_capacity + 1) * 4);
    }
    optionList.options[option_count] = opt;
    if (opt->avail & 6) {
        for (i = 0; i < option_count && (optionList.options[i]->avail & 6); i++)
            ;
        if (i < option_count) {
            tmp = optionList.options[i];
            optionList.options[i] = optionList.options[option_count];
            optionList.options[option_count] = tmp;
        }
    }
    option_count++;
    optionList.options[option_count] = NULL;
}

int Option_RegisterOptionList(OptionList *list)
{
    Option **option;
    if (option_list_count >= 32)
        Targets_ForwardVarArgsAndLongjmp("Too many option lists defined!");
    option_lists[option_list_count] = list;
    option_list_count++;
    for (option = list->options; *option; option++)
        add_option(*option);
    return 1;
}

void clear_option_avail_high_bits(OptionList *options)
{
    Option **entry = options->options;
    if (entry != NULL) {
        while (*entry != NULL) {
            (*entry)->avail &= 1073741823U;
            entry++;
        }
    }
}

void format_option_list(char *buf, OptionList *list, int flags)
{
    Option **opts = list->options;
    unsigned char first = 1;
    char name[0x100];
    int count = 0;
    for (; *opts; opts++) {
        Option **scan = opts + 1;
        if (!((*opts)->avail & (((data_00587ce0 & 0x10) ? 0 : 0x1000) | ((data_00587ce0 & 8) ? 0 : 0x20) | 0x10)) &&
            *(*opts)->names) {
            int f;
            if (!first)
                buf += sprintf(buf, ", ");
            else
                first = 0;
            if (count > 1) {
                while (*scan && (((*scan)->avail &
                                  (((data_00587ce0 & 0x10) ? 0 : 0x1000) | ((data_00587ce0 & 8) ? 0 : 0x20) | 0x10)) ||
                                 !*(*scan)->names))
                    scan++;
                if (!*scan)
                    buf += sprintf(buf, "or ");
            }
            f = (flags & 2) ? (unsigned char)2 : (unsigned char)1;
            switch ((*opts)->avail & 0x700000) {
                case 0x100000:
                    f |= 8;
                    break;
                case 0x400000:
                    f |= 0x20;
                    break;
                case 0x200000:
                    f |= 0x10;
                    break;
            }
            Utils_FormatOptions((*opts)->names, name, f);
            count++;
            buf += sprintf(buf, "%s", name);
            if ((*opts)->avail & 0x8000)
                buf += sprintf(buf, " ...");
        }
    }
}

int Option_IsAvailable(Option *option, unsigned int mask)
{
    return (mask == 0) || ((option->avail & mask) != 0);
}

unsigned int fn_0041c8ba(void)
{
    unsigned int optionValue;

    if (pTool->tool == 'Comp') {
        optionValue = 0x100;
    } else {
        optionValue = 0x40;
    }
    return optionValue;
}

int fn_0041c8d5(Option *option)
{
    return !Option_IsAvailable(option, 0x1c0) || Option_IsAvailable(option, fn_0041c8ba());
}

unsigned int fn_0041c913(Option *option)
{
    unsigned int result = 0U;
    if (fn_0041c8d5(option) != 0U && Option_IsAvailable(option, (~fn_0041c8ba()) & 448U) != 0U)
        result = 1U;
    return result;
}

/* The token kinds that end an option at each nesting level. */
static Triple option_kind_triples[5] = {{0, 0, 0}, {1, 5, 3}, {5, 0, 0}, {4, 0, 0}, {0, 0, 0}};

Boolean token_matches_kind(int kind, TokenText *tok)
{
    if (kind == 1)
        return (tok->kind == 1 || tok->kind == 3) || tok->kind == 2;
    if (kind == 2) {
        TokenText *next = Targets_AdvanceArgument();
        Boolean ok = (tok->kind == 5 && next->kind != 3) || (tok->kind == 2 && next->kind != 1);
        Targets_DecrementCountAndGetTokenText();
        return ok;
    }
    if (kind == 3)
        return tok->kind == 4 || tok->kind == 2;
    return 0;
}

Boolean match_option_kind(int idx, TokenText *s)
{
    TokenText *previous;
    if (!s)
        return 1;
    if (s->kind == option_kind_triples[idx].a)
        return 1;
    if (option_kind_triples[idx].b && s->kind == option_kind_triples[idx].b) {
        TokenText *n = Targets_AdvanceArgument();
        if (n && n->kind == option_kind_triples[idx].c) {
            previous = Targets_DecrementCountAndGetTokenText();
            s = previous;
            return 1;
        }
        previous = Targets_DecrementCountAndGetTokenText();
        s = previous;
    }
    return 0;
}

Boolean fn_0041ca5d(int n, TokenText *x)
{
    if (!x)
        return 1;
    while (n > 0) {
        if (match_option_kind(n - 1, x))
            return 1;
        n--;
    }
    return 0;
}

/* Option descriptor and its associated option group. */
int parse_option(Option *option, int flags)
{
    SInt32 sameFlag;
    int result;
    SInt32 parseFlags;
    SInt32 called;
    SInt32 flag2;
    char buf[1024];

    result = 1;
    called = 0;
    sameFlag = (option->avail & 1) == (flags & 1);
    flag2 = (flags & 2) != 0;
    parseFlags = 0;
    if (flag2) {
        parseFlags |= 4;
    }
    if (flags & 8) {
        parseFlags |= 8;
    }
    if (flags & 0x40) {
        parseFlags |= 1;
    }
    if (option_name[0] != 0) {
        called = 1;
        push_option_arg(option, option_name);
    }
    if (sameFlag) {
        if (!(parseFlags & 1)) {
            if ((option->avail & 0x80000000) != 0 && (option->avail & 0x20000) != 0) {
                Option_ForwardVarArgs(0x1e);
            } else if (option->avail & 0x40000) {
                Option **entry = option->group->options;
                while (*entry != NULL && (*entry == option || ((*entry)->avail & 0x80000000) == 0)) {
                    entry++;
                }
                if (*entry != NULL && *entry != option) {
                    (*entry)->avail &= 0x7fffffff;
                    format_option_list(buf, option->group, flags);
                    if (option->group->text != NULL) {
                        Option_ForwardVarArgs(0x20, (*entry)->names, buf, option->group->text);
                    } else {
                        Option_ForwardVarArgs(0x1f, (*entry)->names, buf);
                    }
                }
            }
        }
        if (fn_0041c8ba() == 0x100) {
            if (Option_IsAvailable(option, 0xc0) != 0) {
                parseFlags |= 2;
                if (fn_0041c8d5(option) == 0) {
                    parseFlags |= 1;
                }
                if (!flag2) {
                    fn_0040fbe1(data_005876fc, 1, NULL);
                }
                Option_FormatOStackToList(data_005876fc);
            }
        }
        if (!(parseFlags & 1)) {
            if (option->avail & 8) {
                if (option->help != NULL) {
                    forward_varargs(0x16, option->help);
                } else {
                    forward_varargs(0x15);
                }
                parseFlags |= 1;
            }
            if (option->avail & 0x800) {
                if (!(option->avail & 0x880000)) {
                    if (option->help != NULL) {
                        Option_ForwardVarArgs(0x1b, option->help);
                    } else {
                        Option_ForwardVarArgs(0x1a);
                    }
                }
                parseFlags |= 1;
            } else {
                if (option->avail & 0x10) {
                    Option_ForwardVarArgs(0x17, option_name, option->help);
                } else if (option->avail & 0x20) {
                    if (option->help != NULL) {
                        Option_ForwardVarArgs(0x19, option->help);
                    } else {
                        Option_ForwardVarArgs(0x18);
                    }
                }
            }
            if (option->avail & 0x80000) {
                Option_ForwardVarArgs(0x1c, option->help);
            }
            if (option->avail & 0x800000) {
                Option_ForwardVarArgs(0x1d);
            }
        }
        option->avail |= 0x80000000;
        if (option->avail & 0x40000) {
            Option **entry = option->group->options;
            option->avail |= 0x40000000;
            while (*entry != NULL) {
                (*entry)->avail |= 0x40000000;
                entry++;
            }
        }
    } else {
        parseFlags |= 1;
    }
    if (result != 0) {
        SInt32 flag8000 = option->avail & 0x8000;
        if (option->args != NULL) {
            result = Parameter_CheckParameters(option->args, parseFlags | (flag8000 ? 0x20 : 0));
        } else {
            short *token = (short *)fn_0040f969();
            if (token != NULL && *token == 4) {
                TokenText *tokenResult = Targets_AdvanceArgument();
            }
        }
        if (result != 0 && flag8000 != 0) {
            SInt32 nextResult = 0;
            if (result != 0) {
                SInt32 flag4;
                SInt32 flag16 = flag2 ? 0x10 : 0;
                SInt32 skipFlag = (parseFlags & 1) ? 0x40 : 0;
                flag4 = (option->avail & 0x10000) ? 4 : 0;
                if (Option_ParseOptionList(option->def, (flags & 0xfffffffb) | 2 | skipFlag | flag16 | flag4) != 0) {
                    nextResult = 1;
                }
            }
            result = nextResult;
        }
    }
    if (called != 0) {
        fn_0041c1ae(option_name);
    }
    if (fn_0041c8ba() == 0x100 && !flag2) {
        fn_0040fbe1(data_005876fc, 1, NULL);
    }
    return result;
}

static inline int lowercase(char character)
{
    return ((int (*)(char))to_lowercase)(character);
}

int match_option_names(char *names, char *arg, int flags, int *result)
{
    char buf[64];
    if (result)
        *result = 0;
    if (!*names && !*arg)
        return 1;
    while (*names) {
        if (Utils_MatchStringSegments(names, arg, flags & 4, 0))
            return 1;
        switch (flags & 0x700000) {
            case 0x100000:
                if (lowercase(arg[0]) == 'n' && lowercase(arg[1]) == 'o' &&
                    Utils_MatchStringSegments(names, arg + 2, flags & 4, 0)) {
                    if (result)
                        *result |= 0x100000;
                    return 1;
                }
                break;
            case 0x400000:
                if (lowercase(arg[0]) == 'n' && lowercase(arg[1]) == 'o' && arg[2] == '-' &&
                    Utils_MatchStringSegments(names, arg + 3, flags & 4, 0)) {
                    if (result)
                        *result |= 0x400000;
                    return 1;
                }
                break;
            case 0x200000: {
                int len = strlen(arg);
                if (arg[len - 1] == '-') {
                    strcpy(buf, arg);
                    buf[len - 1] = 0;
                    if (Utils_MatchStringSegments(names, buf, flags & 4, 0)) {
                        if (result)
                            *result |= 0x200000;
                        return 1;
                    }
                }
                break;
            }
        }
        while (*names && *names != '|')
            names++;
        if (*names)
            names++;
        if (*names == '|')
            names++;
    }
    return 0;
}

Option *find_matching_option(OptionList *list, int x, int *result)
{
    Option **opts = list->options;
    Option *best = NULL;
    int flags = *result;
    if (opts) {
        for (; *opts; opts++) {
            Boolean dummy = 0;
            Boolean found = 0;
            char *name = (*opts)->names;
            Option *option = *opts;
            int avail = 2;
            avail &= option->avail;
            if (avail == 2) {
                while (*name && *name != '|')
                    name++;
                if (*name)
                    name++;
                if (*name == '|')
                    name++;
            }
            found = match_option_names(name, option_name, (*opts)->avail & 0x700004, result) &&
                    (*name || name == (*opts)->names);
            if (found) {
                if (!best || (*opts)->names[0] == option_name[0] || strlen(option_name) > 1)
                    return *opts;
            }
            if ((*opts)->avail & 2) {
                found = Utils_MatchStringSegments((*opts)->names, option_name, (*opts)->avail & 4, 2);
                if (found)
                    flags |= 2;
            } else
                found = 0;
            if (found)
                best = *opts;
        }
    }
    *result = flags;
    return best;
}

static inline void reportUnexpected(TokenText *optionNode, TokenText *node, TokenText *previousNode)
{
    if (optionNode != NULL) {
        push_option_arg(NULL, optionNode->text);
        if (node->kind == 2)
            Parameter_ForwardVarArgs(0x24, Targets_GetTokenTextDescription(node));
        else
            Parameter_ForwardVarArgs(0x23, Targets_GetTokenTextDescription(previousNode));
        fn_0041c1ae(optionNode->text);
    } else {
        if (node->kind == 2)
            fn_0040ecb1(0x24, Targets_GetTokenTextDescription(node));
        else
            fn_0040ecb1(0x23, Targets_GetTokenTextDescription(previousNode));
    }
}

int parse_option_list(OptionList *options, UInt32 flags)
{
    char *originalName;
    Option *definition;
    Option *fallbackDefinition;
    Triple *list;
    Boolean matches;
    Boolean unused;
    char *nameEnd;
    TokenText *previousNode;
    int primaryMatch;
    UInt32 errors = 0;
    int handled = 0;
    UInt32 hadError = 0;
    int commandLine = (flags & 2) != 0;
    int grouped = commandLine && (flags & 0x10);
    TokenText *firstNode;
    TokenText *node;
    TokenText *optionNode = NULL;
    int mode;
    int definitionFlags;
    char savedName[1024];
    char savedToken[64];
    char diagnosticName[64];

    if (!commandLine)
        mode = 1;
    else if (!grouped)
        mode = 2;
    else
        mode = 3;
    list = &option_kind_triples[mode];
    firstNode = (TokenText *)fn_0040f969();
    while ((node = (TokenText *)fn_0040f969()) != NULL) {
        definition = NULL;
        unused = 0;
        handled = 0;
        matches =
            (primaryMatch = (mode == 1 && node->kind == 3) ||
                            (mode == 2 && (node->kind == 2 && (node->text[0] != 0 || (options->flags & 4) != 0)))) ||
            (mode == 3 && node->kind == 4);
        if (mode == 3 && matches) {
            node = Targets_AdvanceArgument();
            matches = node != NULL && node->kind == 2;
            flags &= ~4;
        }
        if (matches) {
            definition = NULL;
            Targets_CopyTokenText(node, option_name, 0x400, 0);
            if (option_name[0] != 0) {
                definition = find_matching_option(options, 0x700000, &definitionFlags);
                if (definition != NULL) {
                    optionNode = node;
                    if ((definitionFlags & 2) == 0) {
                        if (node->kind == 3)
                            node = Targets_AdvanceArgument();
                        node = Targets_AdvanceArgument();
                    } else {
                        nameEnd = definition->names;
                        originalName = node->text;
                        while (*nameEnd != 0 && *nameEnd != '|')
                            nameEnd++;
                        node = Targets_AdvanceArgument();
                        if (!commandLine)
                            strcpy(node->text, originalName + (nameEnd - definition->names));
                        option_name[nameEnd - definition->names] = 0;
                        if (node->text == NULL || node->text[0] == 0) {
                            push_option_arg(NULL, option_name);
                            Option_FormatMessageWithOptionContext(0x22, option_name);
                            fn_0041c1ae(NULL);
                            return 0;
                        }
                    }
                }
            }
            if (definition == NULL) {
                if ((options->flags & 4) != 0) {
                    strcpy(savedName, option_name);
                    option_name[0] = 0;
                    definition = find_matching_option(options, 0, &definitionFlags);
                    strcpy(option_name, node->text);
                    if (definition != NULL) {
                        node = Targets_AdvanceArgument();
                        if (node->kind == 4)
                            node = Targets_AdvanceArgument();
                        errors = parse_option(definition, flags) == 0;
                        handled++;
                        if (errors != 0)
                            hadError = 1;
                        definition = NULL;
                    } else {
                        Targets_ForwardVarArgsAndLongjmp("Missing default for variable list");
                        return 0;
                    }
                    strcpy(option_name, savedName);
                } else if (definition == NULL) {
                    option_name[0] = 0;
                    do {
                        if (mode > 1) {
                            if ((flags & 4) != 0 && node == firstNode)
                                return errors == 0;
                            if ((options->flags & 2) != 0) {
                                if ((flags & 1) == 0) {
                                    format_option_list(option_parameter_text, options, flags);
                                    Option_ForwardVarArgs(0x14, node->text, &option_parameter_text);
                                }
                                node = Targets_AdvanceArgument();
                                handled++;
                                break;
                            } else {
                                format_option_list(option_parameter_text, options, flags);
                                forward_varargs(0x14, node->text, &option_parameter_text);
                            }
                        } else if ((options->flags & 2) != 0 || data_00587e2a != 0) {
                            if ((flags & 1) == 0)
                                Option_ForwardVarArgs(0x13, node->text);
                            node = Targets_AdvanceArgument();
                            node = (TokenText *)Targets_IncrementGlobalOnNonzeroResult();
                            handled++;
                            break;
                        } else {
                            forward_varargs(0x13, node->text);
                        }
                        errors++;
                        hadError = 1;
                    } while (0);
                }
            }
            if (errors == 0 && hadError == 0 && definition != NULL) {
                flags &= ~8;
                if ((definitionFlags & 0x700000) != 0)
                    flags |= 8;
                errors = parse_option(definition, flags) == 0;
                if (errors != 0)
                    hadError = 1;
                handled++;
            }
        } else if (mode == 1 && node->kind == 2) {
            optionNode = NULL;
            option_name[0] = 0;
            definition = find_matching_option(options, 0, &definitionFlags);
            strcpy(option_name, node->text);
            if (definition == NULL) {
                Option_ForwardVarArgs(0x21, option_name);
                hadError = 1;
            } else {
                if ((flags & 1) == 0)
                    errors = parse_option(definition, flags) == 0;
                else {
                    data_00587e10++;
                    errors = 0;
                }
                handled++;
                if (errors != 0)
                    hadError = 1;
                else
                    node = Targets_AdvanceArgument();
            }
        } else if (mode > 1 && fn_0041ca5d(mode, node) != 0) {
            strcpy(savedToken, option_name);
            if ((options->flags & 4) == 0) {
                option_name[0] = 0;
                fallbackDefinition = find_matching_option(options, 0, &definitionFlags);
            } else {
                fallbackDefinition = NULL;
            }
            do {
                if (fallbackDefinition == NULL) {
                    if ((flags & 4) == 0) {
                        forward_varargs(0x22, savedToken);
                        errors++;
                    } else {
                        strcpy(option_name, savedToken);
                        return errors == 0;
                    }
                } else {
                    errors = parse_option(fallbackDefinition, flags) == 0;
                    handled++;
                    if (errors == 0)
                        break;
                }
                hadError = 1;
            } while (0);
            strcpy(option_name, savedToken);
        }
        node = (TokenText *)fn_0040f969();
        if (hadError == 0) {
            if (node != NULL && node->kind == 2 && (definitionFlags & 2) != 0) {
                previousNode = Targets_DecrementCountAndGetTokenText();
                node = Targets_AdvanceArgument();
                if (node->text[0] != 0 && previousNode == optionNode) {
                    if (optionNode != NULL) {
                        strcpy(diagnosticName, optionNode->text);
                        diagnosticName[strlen(node->text)] = 0;
                        push_option_arg(NULL, diagnosticName);
                        Parameter_ForwardVarArgs(0x24, Targets_GetTokenTextDescription(node));
                        fn_0041c1ae(NULL);
                    } else {
                        fn_0040ecb1(0x24, Targets_GetTokenTextDescription(node));
                    }
                    errors++;
                } else {
                    if (node->text[0] == 0)
                        node = Targets_AdvanceArgument();
                    if ((flags & 1) != 0)
                        data_00587e10++;
                }
            } else {
                if (match_option_kind(mode - 1, node) != 0)
                    break;
                do {
                    if (match_option_kind(mode + 1, node) != 0) {
                        previousNode = node;
                        node = Targets_AdvanceArgument();
                        if (mode == 1 && node->kind == 3)
                            break;
                        reportUnexpected(optionNode, node, previousNode);
                        errors++;
                    } else if (mode < 2 && match_option_kind(mode + 2, node) != 0) {
                        reportUnexpected(optionNode, node, node);
                        errors++;
                    } else {
                        break;
                    }
                    hadError++;
                } while (0);
            }
        }
        if (errors != 0 || hadError != 0) {
            while (fn_0041ca5d(mode, node) == 0)
                node = (TokenText *)Targets_IncrementGlobalOnNonzeroResult();
            if (node == NULL)
                node = Targets_DecrementCountAndGetTokenText();
            if (node == NULL)
                CLIO_ReportAssertionFailure("tok", "Option.c", 0x532);
        }
        if (handled == 0)
            break;
        if (errors != 0)
            break;
        if (match_option_kind(mode, node) != 0)
            node = Targets_AdvanceArgument();
        else if (token_matches_kind(mode, node) == 0)
            break;
    }
    return errors == 0;
}

unsigned int Option_ParseOptionList(OptionList *holder, unsigned int flags)
{
    int result;
    int token;
    char savedState[64];

    clear_option_avail_high_bits(holder);
    push_option(holder);
    if ((flags & 2) == 0) {
        result = parse_option_list(holder, flags);
        token = fn_0041c8ba();
        if (token == 0x100) {
            fn_0040fbe1(data_005876fc, 1, NULL);
        }
    } else {
        strcpy(savedState, option_name);
        result = parse_option_list(holder, flags);
        strcpy(option_name, savedState);
    }
    pop_option_stack();
    return result;
}

int Option_ParseDefaultOption(OptionList *options)
{
    Option *option;
    int result;
    int flags;

    strcpy(option_name, "defaultoptions");
    flags = 0;
    option = find_matching_option(options, 0, &flags);
    if (option == NULL) {
        Targets_ForwardVarArgsAndLongjmp("Default options not defined");
        return 1;
    }
    push_option(option);
    result = parse_option(option, 0);
    pop_option_stack();
    return result;
}

void Option_FormatMessageWithOptionContext(SInt32 id, char *arg)
{
    char buf[0x1000];
    Targets_GetResourceCString(id, buf);
    sprintf(buf + strlen(buf), "\nwhile parsing option '");
    format_ostack(buf + strlen(buf), 0);
    sprintf(buf + strlen(buf), "'");
    Targets_FormatAndDispatchMessage(buf, arg);
}

void Option_ReportError(SInt32 id, char *arg)
{
    char buf[0x400];
    Targets_GetResourceCString(id, buf);
    sprintf(buf + strlen(buf), "\nwhile parsing option '");
    format_ostack(buf + strlen(buf), 0);
    sprintf(buf + strlen(buf), "'");
    Targets_ReportFormattedMessage(buf, arg);
}

void format_and_dispatch_option_message(SInt32 id, char *arg)
{
    char buf[0x400];
    Targets_GetResourceCString(id, buf);
    if (fn_0041c1eb() >= 2) {
        sprintf(buf + strlen(buf), "\nwhile parsing option '");
        format_ostack(buf + strlen(buf), 0);
        sprintf(buf + strlen(buf), "'");
    }
    Targets_FormatAndDispatchMessage(buf, arg);
}

void report_option_message(SInt32 id, char *arg)
{
    char buf[0x400];
    Targets_GetResourceCString(id, buf);
    if (fn_0041c1eb() >= 2) {
        sprintf(buf + strlen(buf), "\nwhile parsing option '");
        format_ostack(buf + strlen(buf), 0);
        sprintf(buf + strlen(buf), "'");
    }
    Targets_ReportFormattedMessage(buf, arg);
}

unsigned int forward_varargs(unsigned int messageId, ...)
{
    int base;
    int offset;
    va_list args;

    base = (int)&messageId;
    offset = (int)((char *)&messageId + sizeof(short)) - base;
    offset = (offset + 3) / 4 * 4;
    base = (int)&messageId;
    offset += base;
    args = (char *)offset;
    format_and_dispatch_option_message(messageId, args);
}

unsigned int Option_ForwardVarArgs(unsigned int messageId, ...)
{
    char *start = (char *)&messageId;
    char *end = (char *)&messageId;
    int size;
    va_list arguments;
    end += sizeof(short);
    size = end - start;
    size = (size + 3) / 4 * 4;
    size += (unsigned int)&messageId;
    arguments = (char *)size;
    report_option_message(messageId, arguments);
}

int print_option_help(char *filter)
{
    int i;
    Help_InitColumns();
    if (data_00587ce0 & 0x4000) {
        ToolHelpers_cc_PrintVersion(1);
        Help_PrintRepeatedCharLine('=');
        Help_PrintOptionUsageNotes();
    } else {
        ToolHelpers_cc_PrintVersion(1);
        for (i = 0; i < option_list_count; i++) {
            OptionList *list = option_lists[i];
            if (list) {
                if (data_00587ce0 & 0x8000) {
                    if (filter && *filter && list->text && strstr(list->text, filter))
                        Help_PrintOptionList(list, 0, "");
                } else
                    Help_PrintOptionList(list, 0, filter);
            }
        }
    }
    fn_0042c0a0();
    return 1;
}

int show_option_help(char *name)
{
    struct Option *list = NULL;
    int result = 0;
    int x;
    push_option(Option_GetOptionList());
    push_option_arg(NULL, "help");
    if (*name == *data_00587eec)
        strcpy(option_name, name + 1);
    else
        strcpy(option_name, name);
    if (!option_name[1])
        list = find_matching_option(Option_GetOptionList(), 0x700002, &x);
    if (!list)
        list = find_matching_option(Option_GetOptionList(), 0x700000, &x);
    if (list) {
        Help_InitColumns();
        if (!Help_FormatOption(Option_GetOptionList(), list, 0, ""))
            Targets_ReportMessage(0x26, name);
        fn_0042c0a0();
        result = 1;
    } else {
        forward_varargs(0x13, name);
        result = 0;
    }
    fn_0041c1ae(option_name);
    pop_option_stack();
    return result;
}

unsigned char Option_ShowHelp(void)
{
    if (data_00587ce0 & 1U) {
        show_option_help(data_00587ca0);
        return;
    }
    print_option_help(data_00587ca0);
}
