#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/Help.h"
#include "driver/Memory.h"
#include "driver/Option.h"
#include "driver/Parameter.h"
#include "driver/ParserFace.h"
#include "driver/Projects.h"
#include "driver/StringUtils.h"
#include "driver/Targets.h"
#include "driver/Utils.h"
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "compiler/win32.h"

void append_formatted_text(HelpColumn *buf, char *format, ...)
{
    char text[1024];
    char *cursor;
    char ch;
    vsprintf(text, format, (va_list)&format + (((va_list)(&format + 1) - (va_list)&format + 3) / 4) * 4);

    cursor = text;
    while (*cursor != 0) {
        if (buf->count < 1024) {
            ch = *cursor++;
            if (ch == '~' && *cursor == '~') {
                cursor++;
                ch = *data_00587eec;
            }
            buf->text[(buf->base + buf->count) & (sizeof(buf->text) - 1)] = ch;
            buf->count++;
        } else if (buf == &firstHelpColumn) {
            format_help_columns(&firstHelpColumn, &third_help_column);
        } else {
            drain_column(&helpTextColumn);
        }
    }
}

int append_column_newline(HelpColumn *column)
{
    append_formatted_text(column, "\n");
}

void advance_column(HelpColumn *p, SInt16 n)
{
    if (p->width - p->column - n > help_width / 8) {
        p->column += n;
    } else if (p->width - p->column - 1 > help_width / 10) {
        p->column++;
    }
}

void fn_0042ae0a(HelpColumn *block, short amount)
{
    if (block->width - block->column < help_width / 8) {
        block->column++;
    } else if (block->column >= amount) {
        block->column -= amount;
    }
}

unsigned char fn_0042ae56(unsigned char c)
{
    return c == '\n' || c == '\r' || c == ' ' || c == '\t' || c == '\b' || c == '|';
}

void write_column_line(HelpColumn *source)
{
    SInt16 outputOffset;
    SInt16 newline = 0;
    SInt16 breakPosition;
    SInt16 backspace = 0;
    SInt16 copied;
    unsigned char character;
    unsigned char controlOffset;

    outputOffset = source->left;
    outputOffset += source->column;
    source->linepos = source->column;
    copied = 0;
    breakPosition = source->width - source->column;

    while (source->count > 0 && source->linepos < source->width && newline == 0 && backspace == 0) {
        character = source->text[source->base];
        help_line[outputOffset + copied] = character;
        copied++;
        source->linepos++;
        if (fn_0042ae56(character)) {
            breakPosition = copied;
            newline = 1;
            if (character != '\n' && character != '\r')
                newline = 0;
            newline += (character == '\r');
            backspace = 1;
            controlOffset = character - '\b';
            if (controlOffset > 1)
                backspace = 0;
            backspace += (character == '\b');
        }
        source->base = (source->base + 1) & 0x3ff;
        source->count--;
    }

    if (source->count != 0 || newline != 0 || backspace != 0) {
        source->count += copied - breakPosition;
        source->base = (source->base - (copied - breakPosition)) & 0x3ff;
        if (newline != 0 || backspace != 0) {
            copied++;
            breakPosition--;
        }
        while (breakPosition < copied) {
            copied--;
            help_line[outputOffset + copied] = ' ';
        }
    }

    source->linepos = 0;
    source->lines++;
    source->ended = (newline == 1) || (backspace != 0) || (source->count == 0);
    if ((source->ended != 0 || backspace != 0) && source->indented != 0) {
        fn_0042ae0a(source, help_width / 40);
        source->indented = 0;
    }
    if (backspace != 0) {
        if (backspace == 1)
            advance_column(source, help_width / 25);
        else
            fn_0042ae0a(source, help_width / 25);
    } else if (source->ended == 0 && source != &helpTextColumn && source->indented == 0) {
        advance_column(source, help_width / 40);
        source->indented = 1;
    }
}

void print_string_line(void)
{
    HPrintF(help_output, "%.*s\n", help_width - 1, help_line);
}

void format_help_columns(HelpColumn *a, HelpColumn *b)
{
    while (a->count != 0 || b->count != 0) {
        memset(help_line, ' ', help_width);
        help_line[a->left + a->width + 1] = '#';
        if (a->ended && b->ended)
            a->ended = b->ended = 0;
        if (!a->ended)
            write_column_line(a);
        if (!b->ended)
            write_column_line(b);
        print_string_line();
    }
}

void drain_column(HelpColumn *buf)
{
    while (buf->count != 0) {
        memset(help_line, ' ', help_width);
        if (buf->ended)
            buf->ended = 0;
        if (!buf->ended)
            write_column_line(buf);
        print_string_line();
    }
}

void format_first_and_third_help_columns(void)

{
    format_help_columns(&firstHelpColumn, &third_help_column);
    return;
}

void append_columns_newline(void)

{
    append_column_newline(&firstHelpColumn);
    append_column_newline(&third_help_column);
    return;
}

unsigned int Help_FormatOption(OptionList *scope, Option *pragma, unsigned int formatMode, char *nameFilter)
{
    char formatFlags;
    const char normalFormat = 1;
    const char alternateFormat = 2;
    char *separator;
    char noArguments;
    PARAM_T *lastArg;
    char showAllFlags;
    Boolean allArgsMatch;
    char nameBuffer[512];
    struct FlagTextBuffer flagBuffer;
    int argText;
    int argDetail;
    int argHelp;
    PARAM_T *arg;
    PARAM_T *firstArg;
    PARAM_T *currentArg;
    PARAM_T *checkArg;
    unsigned char flagsDiffer;
    unsigned int omitSeparator;
    unsigned int toolMask;

    if ((pragma->names[0] == 0 && (scope->flags & 4) == 0) ||
        (nameFilter != NULL && nameFilter[0] != 0 && strstr(pragma->names, nameFilter) == NULL &&
         (pragma->help == NULL || !strstr(pragma->help, nameFilter))))
        return 0;

    if (((pragma->avail & 0x1000) != 0 && (data_00587ce0 & 0x10) == 0) ||
        ((pragma->avail & 8) != 0 && (data_00587ce0 & 4) == 0) ||
        ((pragma->avail & 0x20) != 0 && (data_00587ce0 & 8) == 0) ||
        ((pragma->avail & 0x800) != 0 && (data_00587ce0 & 2) == 0) ||
        ((pragma->avail & 0x800000) != 0 && (data_00587ce0 & 0x20) == 0) ||
        ((data_00587ce0 & 0x80) == 0 && (pragma->avail & 0x801828) == 0))
        return 0;

    if (pragma->help != NULL || (pragma->avail & 0x8000) != 0) {
        noArguments = 1;
        lastArg = NULL;
        if ((data_00587ce0 & 0x100) != 0)
            append_columns_newline();
        if ((pragma->avail & 1) != 0 && formatMode == 0)
            append_formatted_text(&third_help_column, "global; ");
        if (data_0054aa78 != 1 && (pragma->avail & 4) != 0)
            append_formatted_text(&third_help_column, "cased; ");

        formatFlags = !formatMode ? normalFormat : alternateFormat;
        switch (pragma->avail & 0x700000) {
            case 0x100000:
                formatFlags |= 8;
                break;
            case 0x200000:
                formatFlags |= 0x10;
                break;
            case 0x400000:
                formatFlags |= 0x20;
                break;
        }
        if ((pragma->avail & 2) != 0)
            formatFlags |= 0x40;

        Utils_FormatOptions(pragma->names[0] != 0 ? pragma->names : "...", nameBuffer, formatFlags);
        append_formatted_text(&firstHelpColumn, nameBuffer);

        if ((pragma->avail & 8) != 0)
            append_formatted_text(&third_help_column, "obsolete;\r");
        if ((pragma->avail & 0x4000) != 0)
            append_formatted_text(&third_help_column, "compatibility;\r");
        if ((pragma->avail & 0x800) != 0)
            append_formatted_text(&third_help_column, "ignored;\r");

        toolMask = ((scope->flags & 0x100) ? 0x100 : 0) | ((scope->flags & 0x200) ? 0x40 : 0) |
                   ((scope->flags & 0x400) ? 0x80 : 0);

        if (fn_0041c8d5(pragma) == 0 || fn_0041c913(pragma) != 0 || fn_0041c8ba() != toolMask) {
            flagsDiffer = 0;
            showAllFlags = 1;
            if ((pragma->avail & 0x1c0) != toolMask)
                flagsDiffer = 1;
            if (fn_0041c8d5(pragma) != 0 && fn_0041c913(pragma) != 0)
                showAllFlags = 0;
            if (flagsDiffer != 0) {
                flagBuffer = data_0054dd04;
                if ((pragma->avail & 0x1c0) == 0x1c0) {
                    strcat(flagBuffer.text, "all tools");
                } else {
                    if (Option_IsAvailable(pragma, 0x100) != 0 && (fn_0041c8ba() != 0x100 || showAllFlags != 0))
                        strcat(flagBuffer.text, "this tool");
                    if (Option_IsAvailable(pragma, 0x40) != 0 && (fn_0041c8ba() != 0x40 || showAllFlags != 0)) {
                        if (flagBuffer.text[0] != 0)
                            strcat(flagBuffer.text, ", ");
                        strcat(flagBuffer.text, "linker");
                    }
                    if (Option_IsAvailable(pragma, 0x80) != 0 && (fn_0041c8ba() != 0x80 || showAllFlags != 0)) {
                        if (flagBuffer.text[0] != 0)
                            strcat(flagBuffer.text, ", ");
                        strcat(flagBuffer.text, "disassembler");
                    }
                    if (Option_IsAvailable(pragma, 0x1c0) == 0)
                        strcat(flagBuffer.text, "another tool");
                }
                if (showAllFlags != 0 || fn_0041c8d5(pragma) == 0) {
                    append_formatted_text(&third_help_column, "for %s;\r", flagBuffer.text);
                } else {
                    if (data_00587e23 != 0) {
                        append_formatted_text(&third_help_column, "passed to %s;\r", flagBuffer.text);
                    }
                }
            }
        }

        if ((pragma->avail & 0x80000) != 0)
            append_formatted_text(&third_help_column, "warning:\r");
        if ((pragma->avail & 0x20) != 0) {
            append_formatted_text(&third_help_column, "deprecated;\rinstead use ");
        } else {
            if ((pragma->avail & 0x10) != 0) {
                append_formatted_text(&third_help_column, "substituted with ");
            }
        }
        if (pragma->help != NULL)
            append_formatted_text(&third_help_column, "%s", pragma->help);

        if (pragma->args != NULL && (pragma->avail & 0x800) == 0) {
            for (arg = pragma->args, firstArg = NULL; arg != NULL; lastArg = arg, arg = arg->next) {
                if ((arg->flags & 3) == 1)
                    continue;
                currentArg = arg;
                if (firstArg == NULL)
                    firstArg = arg;
                noArguments = 0;
                Parameter_DispatchByWhich(arg, &argText, &argDetail, &argHelp);
                if (argText == 0)
                    continue;
                if ((arg->flags & 3) == 2 && arg->which != 0x0f && arg->which != 0x0e) {
                    if (*help_option_separator == ' ') {
                        if (arg != firstArg)
                            separator = "[,";
                        else if (formatMode != 0)
                            separator = "[=";
                        else
                            separator = " [";
                        append_formatted_text(&firstHelpColumn, separator);
                    } else {
                        if (arg != firstArg)
                            separator = ",";
                        else if (formatMode != 0)
                            separator = "=";
                        else
                            separator = help_option_separator;
                        append_formatted_text(&firstHelpColumn, "[%s", separator);
                    }
                } else {
                    if (arg != firstArg)
                        separator = ",";
                    else if (formatMode != 0)
                        separator = "=";
                    else {
                        omitSeparator = 0;
                        if ((pragma->avail & 2) != 0 && strchr(pragma->names, 0x7c) == NULL)
                            omitSeparator = 1;
                        if (omitSeparator != 0)
                            separator = "";
                        else
                            separator = help_option_separator;
                    }
                    append_formatted_text(&firstHelpColumn, separator);
                }
                append_formatted_text(&firstHelpColumn, "%s", argText);
                if ((arg->flags & 3) == 2 && arg->which != 0x0f && arg->which != 0x0e)
                    append_formatted_text(&firstHelpColumn, "]");
                if (argDetail != 0) {
                    if ((arg->flags & 3) != 2)
                        append_formatted_text(&third_help_column, "; for '%s', %s", argText, argDetail);
                    else
                        append_formatted_text(&third_help_column, "; if parameter specified, %s", argDetail);
                }
                if (argHelp != 0 && (pragma->avail & 0x2000) == 0) {
                    if (firstArg == arg) {
                        append_formatted_text(&third_help_column, "; default is %s", argHelp);
                    } else {
                        append_formatted_text(&third_help_column, ",%s", argHelp);
                    }
                }
            }
            if (noArguments != 0 && (pragma->avail & 0x2000) == 0) {
                checkArg = pragma->args;
                allArgsMatch = checkArg != NULL;
                for (; checkArg != NULL && allArgsMatch != 0; checkArg = checkArg->next)
                    allArgsMatch &= Parameter_DispatchParam(checkArg);
                if (allArgsMatch != 0)
                    append_formatted_text(&third_help_column, "; default");
            }
        }

        if ((pragma->avail & 0x800000) != 0)
            append_formatted_text(&third_help_column, "; meaningless for this target");

        if ((pragma->avail & 0x8000) != 0 && pragma->def != NULL) {
            if (noArguments == 0) {
                append_formatted_text(&firstHelpColumn, "%s",
                                      (pragma->avail & 0x10000) != 0 ? ((lastArg->flags & 8) ? "[=" : "[,")
                                                                     : ((lastArg->flags & 8) ? "=" : ","));
            } else {
                if ((pragma->avail & 2) == 0) {
                    if ((pragma->avail & 0x10000) != 0) {
                        if (*help_option_separator == ' ') {
                            append_formatted_text(&firstHelpColumn, formatMode != 0 ? "[=" : " [");
                        } else {
                            append_formatted_text(&firstHelpColumn, "[%s",
                                                  formatMode != 0 ? "=" : help_option_separator);
                        }
                    } else {
                        append_formatted_text(&firstHelpColumn, "%c", (formatMode != 0 ? '=' : *help_option_separator));
                    }
                } else {
                    if ((pragma->avail & 0x10000) != 0) {
                        append_formatted_text(&firstHelpColumn, formatMode != 0                 ? "["
                                                                : *help_option_separator == ' ' ? " ["
                                                                                                : "[");
                    }
                }
            }
            append_formatted_text(&firstHelpColumn, "%s%s%s", pragma->def->text ? pragma->def->text : "keyword",
                                  (pragma->def->flags & 1) ? "" : "[,...]", (pragma->avail & 0x10000) != 0 ? "]" : "");
            append_formatted_text(&firstHelpColumn, "\t");
            append_formatted_text(&third_help_column, "\t");
            Help_PrintOptionList(pragma->def, 1, "");
            append_formatted_text(&third_help_column, data_0054ded8);
            append_formatted_text(&firstHelpColumn, data_0054ded8);
        } else {
            append_formatted_text(&firstHelpColumn, "\n");
            append_formatted_text(&third_help_column, "\n");
        }
    }
    format_first_and_third_help_columns();
    return 1;
}

void Help_PrintOptionList(OptionList *list, int subprint, char *filter)
{
    struct Option **opts = list->options;
    int flag = 0;
    unsigned char show;
    if (fn_0041c8ba() == 0x100)
        flag |= 0x100;
    else
        flag |= 0x200;
    if (!subprint && (data_00587ce0 & 0x200)) {
        if ((data_00587ce0 & 0xc00) == 0x400 && (list->flags & 0x700) && !(list->flags & flag))
            return;
        if ((data_00587ce0 & 0xc00) == 0x800 && ((list->flags & 0x700) == flag || !(list->flags & 0x700)))
            return;
    }
    if (list->text && !subprint && *opts) {
        Help_PrintRepeatedCharLine('-');
        append_formatted_text(&helpTextColumn, "%s", list->text);
        drain_column(&helpTextColumn);
        Help_PrintRepeatedCharLine('-');
    }
    for (; *opts; opts++) {
        show = 0;
        if (!(data_00587ce0 & 0x200)) {
            if ((0xc00 & data_00587ce0) == 0xc00) {
                if (data_00587e23) {
                    flag = 1;
                    if (!Option_IsAvailable(*opts, 0x40) && !Option_IsAvailable(*opts, 0x80))
                        flag = 0;
                } else
                    flag = 1;
                if (flag && fn_0041c8d5(*opts))
                    show = 1;
            }
        } else if ((0xc00 & data_00587ce0) == 0xc00)
            show = 1;
        else if ((data_00587ce0 & 0x400) && fn_0041c8d5(*opts))
            show = 1;
        else if ((data_00587ce0 & 0x800) && !fn_0041c8d5(*opts))
            show = 1;
        else if ((data_00587ce0 & 0x800) && Option_IsAvailable(*opts, ~fn_0041c8ba() & 0x1c0))
            show = 1;
        if (show)
            Help_FormatOption(list, *opts, subprint, filter);
    }
    if (subprint && (data_00587ce0 & 0x100))
        append_columns_newline();
    format_first_and_third_help_columns();
    if (!subprint)
        HPrintF(help_output, "\n");
}

void Help_PrintOptionUsageNotes(void)
{
    const char *s;

    append_formatted_text(&helpTextColumn, option_usage_notes);
    drain_column(&helpTextColumn);
    if (data_0054aa78 != 1)
        s = "";
    else
        s = "colon or ";
    append_formatted_text(&helpTextColumn, option_usage_notes_format, s);
    append_formatted_text(&helpTextColumn, data_0054e0bc);
    drain_column(&helpTextColumn);
    if (data_0054aa78 != 1)
        s = "-- \"cased\" indicates that the option is case-sensitive.  By default, no options are case-sensitive.\r";
    else
        s = "";
    append_formatted_text(
        &helpTextColumn,
        "\t%s-- \"compatability\" indicates that the option is borrowed from another vendor's tool and may only approximate its counterpart.\r-- \"global\" indicates that the option has an effect over the entire command line and is parsed before any other options.  When several global options are specified, they are interpreted in order.\r-- \"deprecated\" indicates that the option will be eliminated in the future and should not be used any longer.  An alternative form is supplied.\r",
        s);
    drain_column(&helpTextColumn);
    append_formatted_text(&helpTextColumn, data_0054e38c);
    drain_column(&helpTextColumn);
    if (data_0054aa78 != 1)
        s = "and '='";
    else
        s = ", ':', and '='";
    append_formatted_text(&helpTextColumn, option_usage_notes_text, s);
    drain_column(&helpTextColumn);
    if (data_00587e23 && driverTool[0] == 0x436f6d70)
        append_formatted_text(&helpTextColumn, data_0054e66c);
    drain_column(&helpTextColumn);
}

void Help_InitColumns(void)
{
    short firstColumn;
    short secondColumn;
    short thirdColumn;
    short lastColumn;
    if (!(help_output = (struct StorageHandle *)Memory_NewHandle(0))) {
        Targets_ForwardVarArgsAndLongjmp("Out of memory");
        longjmp(plugin_request_jmp_buf, 7);
    }
    firstColumn = help_width / 40;
    secondColumn = help_width / 3 + firstColumn;
    thirdColumn = (help_width / 60 & ~1) + (secondColumn + 3);
    lastColumn = help_width - 1;
    Parameter_InitHelpColumn(&firstHelpColumn, firstColumn, secondColumn - firstColumn);
    Parameter_InitHelpColumn(&third_help_column, thirdColumn, lastColumn - thirdColumn);
    Parameter_InitHelpColumn(&helpTextColumn, 0, lastColumn);
}

void Help_PrintRepeatedCharLine(char c)
{
    char buf[256];
    memset(buf, c, 255);
    buf[255] = 0;
    HPrintF(help_output, "%.*s\n", help_width - 1, buf);
}

void fn_0042c0a0(void)
{
    ToolHelpers_cc_CallValuePairCallback(0U, help_output);
    Memory_FreeHandle(help_output);
    return;
}
