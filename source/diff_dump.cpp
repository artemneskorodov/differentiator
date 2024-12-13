#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "utils.h"
#include "colors.h"
#include "diff_dump.h"
#include "matan_killer.h"
#include "variable_list.h"
#include "expression_utils.h"
#include "custom_assert.h"

/*=========================================================================================================*/

static const char *const NodeTypeStringOperation    = "OPERATION";
static const char *const NodeTypeStringNumber       = "NUMBER";
static const char *const NodeTypeStringVariable     = "VARIABLE";

static const char *const NodeColorTypeOperation     = "#A6AEBF";
static const char *const NodeColorTypeNumber        = "#D0E8C5";
static const char *const NodeColorTypeVariable      = "#FFF8DE";
static const char *const NodeColorCurrentNode       = "#C5D3E8";

static const size_t MinSubstitutionSubtreeSize = 15;
static const size_t MaxSubstitutionSubtreeSize = 20;

/*=========================================================================================================*/

static const char *DifferentiationPhrases[] = {
    "Путём использования вашего недалёкого мозга можно заметить что:",
    "Ебал я этот матан, но всё же:",};
static const size_t DifferentiationPhrasesSize = sizeof(DifferentiationPhrases) / sizeof(DifferentiationPhrases[0]);

/*=========================================================================================================*/

static const char *SimplifyEvaluationPhrases[] = {
    "Ходил бы ты в пятый класс, знал бы что:",
    "Используя калькулятор, если ты такой тупой, можно понять, что:",
    "Нахуй этот ебучий блять матан, будем считать:"};
static const size_t SimplifyEvaluationPhrasesSize = sizeof(SimplifyEvaluationPhrases) / sizeof(SimplifyEvaluationPhrases[0]);

/*=========================================================================================================*/

static const char *SimplifyNeutralsPhrases[] = {
    "Ясен хуй:",
    "Невероятно блять тяжело заметить:",
    "Заметим, что данное действие нам нахуй не нужно:",
    "Если не быть слепым, то можно заметить, что:"};
static const size_t SimplifyNeutralsPhrasesSize = sizeof(SimplifyNeutralsPhrases) / sizeof(SimplifyNeutralsPhrases[0]);

/*=========================================================================================================*/

static const char *WritingResultPhrases[] = {
    "Подводя блядский итог:",
    "Несмотря на все сложности нам удалось найти ответ на эту ебучую задачу:"};
static const size_t WritingResultPhrasesSize = sizeof(WritingResultPhrases) / sizeof(WritingResultPhrases[0]);

/*=========================================================================================================*/

static const char *DiffStartPhrases[] = {
    "Сегодня мы будем дифференцировать вот такую ебаторию:",
    "Несмотря на то что меня не въебало, будем заниматься такой хуйнёй:"};
static const size_t DiffStartPhrasesSize = sizeof(DiffStartPhrases) / sizeof(DiffStartPhrases[0]);

/*=========================================================================================================*/

static const char *DiffResultPhrases[] = {
    "преобразуется в:"};
static const size_t DiffResultPhrasesSize = sizeof(DiffResultPhrases) / sizeof(DiffResultPhrases[0]);

/*=========================================================================================================*/

static const char *TailorStartPhrases[] = {
    "Ебанём ка такую хуйню в тейлора:",
    "СОСАЛ?"};
static const size_t TailorStartPhrasesSize = sizeof(TailorStartPhrases) / sizeof(TailorStartPhrases[0]);

/*=========================================================================================================*/

static const char *TailorNewDiffPhrases[] = {
    "А теперь хуйнём такую производную:",
    "Заебало уже блять, а ещё физос делать",};
static const size_t TailorNewDiffPhrasesSize = sizeof(TailorNewDiffPhrases) / sizeof(TailorNewDiffPhrases[0]);

/*=========================================================================================================*/

static const char *TailorEvaluatePhrases[] = {
    "Ебать наконец-то нашли",
    "Вот такую хуйню:"};
static const size_t TailorEvaluatePhrasesSize = sizeof(TailorEvaluatePhrases) / sizeof(TailorEvaluatePhrases[0]);

/*=========================================================================================================*/

static expression_error_t technical_dump_write_subtree          (expression_t      *expression,
                                                                 expression_node_t *node,
                                                                 FILE              *dot_file,
                                                                 size_t             level,
                                                                 expression_node_t *current_node);

static const char        *string_node_type                      (expression_node_t *node);

static const char        *string_node_value                     (expression_t      *expression,
                                                                 expression_node_t *node);

static const char        *node_color                            (expression_node_t *node,
                                                                 expression_node_t *current_node);

static const char        *get_latex_function                    (operation_t        operation);

static const char        *get_action_phrase                     (log_action_t       action);

static expression_error_t latex_write_subtree                   (latex_log_info_t  *log_info,
                                                                 expression_node_t *node);

static expression_error_t latex_log_write_substitutions         (latex_log_info_t  *log_info);

static expression_error_t latex_log_check_substitutions         (latex_log_info_t  *log_info,
                                                                 expression_node_t *node);

/*=========================================================================================================*/

expression_error_t technical_dump_ctor(expression_t *expression,
                                       const char   *filename) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER    );
    _C_ASSERT(filename   != NULL, return EXPRESSION_INVALID_FILENAME);

    char dump_filename[256] = {};
    sprintf(dump_filename, "logs/%s.html", filename);
    expression->dump_info.technical_file = fopen(dump_filename, "w");
    if(expression->dump_info.technical_file == NULL) {
        print_error("Error while creating general dump file.\n");
        return EXPRESSION_DUMP_OPENING_ERROR;
    }

    expression->dump_info.technical_number = 0;
    expression->dump_info.technical_filename = filename;

    fprintf(expression->dump_info.technical_file,
            "<div>\n"
            "Technical dump\n"
            "</div>\n");

    return EXPRESSION_SUCCESS;
}

expression_error_t technical_dump_dtor(expression_t *expression) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER);

    fclose(expression->dump_info.technical_file);
    expression->dump_info.technical_file = NULL;
    expression->dump_info.technical_filename = NULL;
    expression->dump_info.technical_number = 0;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t technical_dump(expression_t      *expression,
                                  expression_node_t *current_node,
                                  const char        *format, ...) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER       );
    _C_ASSERT(format     != NULL, return EXPRESSION_INVALID_DUMP_FORMAT);

    char dot_filename[256] = {};
    sprintf(dot_filename,
            "logs/dot/%s%04llx.dot",
            expression->dump_info.technical_filename,
            expression->dump_info.technical_number);

    FILE *dot_file = fopen(dot_filename, "w");

    fprintf(dot_file,
            "digraph {\n"
            "node[shape = Mrecord, style = filled];\n");
    if(expression->root != NULL) {
        _RETURN_IF_ERROR(technical_dump_write_subtree(expression, expression->root, dot_file, 0, current_node));
    }

    fprintf(dot_file, "}");

    fclose(dot_file);
    char command[256] = {};
    sprintf(command,
            "dot %s -Tsvg -o logs/img/%s%04llx.svg",
            dot_filename,
            expression->dump_info.technical_filename,
            expression->dump_info.technical_number);
    system(command);

    fprintf(expression->dump_info.technical_file,
            "<h1>Dump comment:</h1>\n"
            "<h1>------------------------------------------------------------------</h1>\n"
            "<h2>\n");

    va_list args;
    va_start(args, format);
    vfprintf(expression->dump_info.technical_file, format, args);
    va_end(args);

    fprintf(expression->dump_info.technical_file,
            "</h2>\n"
            "<h1>------------------------------------------------------------------</h1>\n"
            "<img src = \"img/%s%04llx.svg\">\n",
            expression->dump_info.technical_filename,
            expression->dump_info.technical_number);
    expression->dump_info.technical_number++;
    fflush(expression->dump_info.technical_file);

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t technical_dump_write_subtree(expression_t      *expression,
                                                expression_node_t *node,
                                                FILE              *dot_file,
                                                size_t             level,
                                                expression_node_t *current_node) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER      );
    _C_ASSERT(node       != NULL, return EXPRESSION_NODE_NULL_POINTER );
    _C_ASSERT(dot_file   != NULL, return EXPRESSION_WRITING_FILE_ERROR);

    fprintf(dot_file,
            "node%p[fillcolor = \"%s\", "
            "rank = %llu, "
            "label = \"{%p | type = %s | value = %s | { <l> %s | <r> %s }}\"];\n",
            node,
            node_color(node, current_node),
            level,
            node,
            string_node_type(node),
            string_node_value(expression, node),
            node->left == NULL ? "NULL" : "LEFT",
            node->right == NULL ? "NULL" : "RIGHT");

    if(node->left != NULL) {
        fprintf(dot_file, "node%p:<l> -> node%p;\n", node, node->left);
        _RETURN_IF_ERROR(technical_dump_write_subtree(expression,
                                                      node->left,
                                                      dot_file,
                                                      level + 1,
                                                      current_node));
    }
    if(node->right != NULL) {
        fprintf(dot_file, "node%p:<r> -> node%p;\n", node, node->right);
        _RETURN_IF_ERROR(technical_dump_write_subtree(expression,
                                                      node->right,
                                                      dot_file,
                                                      level + 1,
                                                      current_node));
    }
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

const char *string_node_type(expression_node_t *node) {
    if(node == NULL) {
        return "";
    }
    switch(node->type) {
        case NODE_TYPE_OP: {
            return NodeTypeStringOperation;
        }
        case NODE_TYPE_NUM: {
            return NodeTypeStringNumber;
        }
        case NODE_TYPE_VAR: {
            return NodeTypeStringVariable;
        }
        default: {
            return NULL;
        }
    }
}

/*=========================================================================================================*/

const char *string_node_value(expression_t      *expression,
                              expression_node_t *node) {
    _C_ASSERT(expression != NULL, return NULL);

    static char value_string[256] = {};
    if(node == NULL) {
        value_string[0] = '\0';
        return value_string;
    }
    switch(node->type) {
        case NODE_TYPE_OP: {
            for(size_t i = 0; i < sizeof(SupportedOperations) / sizeof(SupportedOperations[0]); i++) {
                if(node->value.operation == SupportedOperations[i].code) {
                    sprintf(value_string, "%s", SupportedOperations[i].name);
                    break;
                }
            }
            break;
        }
        case NODE_TYPE_NUM: {
            sprintf(value_string, "%lg", node->value.numeric_value);
            break;
        }
        case NODE_TYPE_VAR: {
            char varname = variables_list_get_varname(expression->variables_list, node->value.variable_index);
            sprintf(value_string, "%c", varname);
            break;
        }
        default: {
            return NULL;
        }
    }
    return value_string;
}

/*=========================================================================================================*/

const char *node_color(expression_node_t *node,
                       expression_node_t *current_node) {
    if(node == current_node) {
        return NodeColorCurrentNode;
    }
    if(node == NULL) {
        return "";
    }
    switch(node->type) {
        case NODE_TYPE_OP: {
            return NodeColorTypeOperation;
        }
        case NODE_TYPE_NUM: {
            return NodeColorTypeNumber;
        }
        case NODE_TYPE_VAR: {
            return NodeColorTypeVariable;
        }
        default: {
            return NULL;
        }
    }
}

/*=========================================================================================================*/

expression_error_t latex_log_ctor(latex_log_info_t *log_info,
                                  const char       *filename,
                                  expression_t     *expression,
                                  expression_t     *derivative) {
    _C_ASSERT(log_info   != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(filename   != NULL, return EXPRESSION_INVALID_DUMP_FILENAME);
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER         );
    _C_ASSERT(derivative != NULL, return EXPRESSION_NULL_POINTER         );

    char latex_filename[256] = {};
    srand((unsigned)time(NULL));

    log_info->variables_list = expression->variables_list;
    sprintf(latex_filename, "logs/%s.tex", filename);
    log_info->file = fopen(latex_filename, "w");
    if(log_info->file == NULL) {
        return EXPRESSION_LATEX_OPENING_ERROR;
    }
    log_info->filename   = filename;
    log_info->expression = expression;
    log_info->derivative = derivative;

    fprintf(log_info->file,
            "\\documentclass[12pt]{article}\n\n"
            "\\usepackage[utf8]{inputenc}\n"
            "\\usepackage[english, russian]{babel}\n\n"
            "\\title{Методическое пособие по ёбани}\n"
            "\\author{by хуйня corporated}\n"
            "\\begin{document}\n"
            "\\maketitle\n");
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_log_write(latex_log_info_t  *log_info,
                                   log_action_t       action,
                                   expression_node_t *node, ...) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(node     != NULL, return EXPRESSION_NODE_NULL_POINTER    );

    if(action == TAILOR_NEW_DIFF) {
        va_list args;
        va_start(args, node);
        size_t value = va_arg(args, size_t);
        fprintf(log_info->file, "%llu. ", value);
        va_end(args);
    }
    if(action == TAILOR_EVALUATE) {
        va_list args;
        va_start(args, node);
        size_t derivative_number = va_arg(args, size_t);
        double value = va_arg(args, double);
        fprintf(log_info->file, "\\[f^{(%llu)}=%lg\\]", derivative_number, value);
        va_end(args);
        return EXPRESSION_SUCCESS;
    }
    if(action == WRITING_RESULT) {
        fprintf(log_info->file, "Ебались мы с вот такой хернёй:\n");
        _RETURN_IF_ERROR(latex_log_check_substitutions(log_info, log_info->expression->root));
        fprintf(log_info->file, "\\[ y = ");
        _RETURN_IF_ERROR(latex_write_subtree(log_info, log_info->expression->root));
        fprintf(log_info->file, "\\]\n");
        _RETURN_IF_ERROR(latex_log_write_substitutions(log_info));
    }
    const char *phrase = get_action_phrase(action);
    fprintf(log_info->file, "%s\n", phrase);

    _RETURN_IF_ERROR(latex_log_check_substitutions(log_info, node));
    fprintf(log_info->file, "\\[ y = ");
    _RETURN_IF_ERROR(latex_write_subtree(log_info, node));
    fprintf(log_info->file, "\\]\n");
    _RETURN_IF_ERROR(latex_log_write_substitutions(log_info));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_log_dtor(latex_log_info_t *log_info) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);

    fprintf(log_info->file, "\\end{document}\n");
    fclose(log_info->file);

    char command[512] = {};
    sprintf(command, "pdflatex -output-directory=logs -quiet logs/%s.tex", log_info->filename);
    system(command);

    log_info->file = NULL;
    log_info->filename = NULL;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_write_subtree(latex_log_info_t  *log_info,
                                       expression_node_t *node) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(node     != NULL, return EXPRESSION_NODE_NULL_POINTER    );

    if(node->is_substitution) {
        log_info->substitution_to_write[log_info->to_write_number++] = node;
        fprintf(log_info->file, "{%s}", node->substitution_name);
        return EXPRESSION_SUCCESS;
    }
    switch(node->type) {
        case NODE_TYPE_NUM: {
            fprintf(log_info->file, "%lg", node->value.numeric_value);
            return EXPRESSION_SUCCESS;
        }
        case NODE_TYPE_VAR: {
            fprintf(log_info->file, "%c", variables_list_get_varname(log_info->variables_list,
                                                                     node->value.variable_index));
            return EXPRESSION_SUCCESS;
        }
        case NODE_TYPE_OP: {
            _RETURN_IF_ERROR(SupportedOperations[node->value.operation].latex_logger(log_info,
                                                                                     node));
            return EXPRESSION_SUCCESS;
        }
        default: {
            return EXPRESSION_UNKNOWN_NODE_TYPE;
        }
    }
}

/*=========================================================================================================*/

expression_error_t latex_write_inorder(latex_log_info_t  *log_info,
                                       expression_node_t *node) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(node     != NULL, return EXPRESSION_NODE_NULL_POINTER    );

    fprintf(log_info->file, "{%s",
            is_bigger_priority(node, node->left) ? "" : "(");

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->left));

    fprintf(log_info->file, "%s} %s {%s",
            is_bigger_priority(node, node->left) ? "" : ")",
            get_latex_function(node->value.operation),
            is_bigger_priority(node, node->right) ? "" : "(");

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->right));

    fprintf(log_info->file, "%s}",
            is_bigger_priority(node, node->right) ? "" : ")");
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_write_preorder_one_arg(latex_log_info_t  *log_info,
                                                expression_node_t *node) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(node     != NULL, return EXPRESSION_NODE_NULL_POINTER    );

    fprintf(log_info->file, "%s {%s",
            get_latex_function(node->value.operation),
            is_bigger_priority(node, node->right) ? "" : "(");

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->right));

    fprintf(log_info->file, "%s}",
            is_bigger_priority(node, node->right) ? "" : ")");
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_write_preorder_two_args(latex_log_info_t  *log_info,
                                                 expression_node_t *node) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(node     != NULL, return EXPRESSION_NODE_NULL_POINTER    );

    fprintf(log_info->file, "%s {",
            get_latex_function(node->value.operation));

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->left));

    fprintf(log_info->file, "}{");

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->right));

    fprintf(log_info->file, "}");
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_write_func_log(latex_log_info_t  *log_info,
                                   expression_node_t *node) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);
    _C_ASSERT(node     != NULL, return EXPRESSION_NODE_NULL_POINTER    );

    fprintf(log_info->file, "%s_{%s",
            get_latex_function(node->value.operation),
            is_bigger_priority(node, node->left) ? "" : "(");

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->left));

    fprintf(log_info->file, "%s}{%s",
            is_bigger_priority(node, node->left) ? "" : ")",
            is_bigger_priority(node, node->right) ? "" : "(");

    _RETURN_IF_ERROR(latex_write_subtree(log_info, node->right));

    fprintf(log_info->file, "%s}",
            is_bigger_priority(node, node->right) ? "" : ")");
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

const char *get_latex_function(operation_t operation) {
    return SupportedOperations[operation].latex_function;
}

/*=========================================================================================================*/

const char *get_action_phrase(log_action_t action) {
    const char **phrases_array = NULL;
    size_t phrases_array_size = 0;
    switch(action) {
        case DIFFERENTIATION: {
            phrases_array = DifferentiationPhrases;
            phrases_array_size = DifferentiationPhrasesSize;
            break;
        }
        case SIMPLIFICATION_EVALUATE: {
            phrases_array = SimplifyEvaluationPhrases;
            phrases_array_size = SimplifyEvaluationPhrasesSize;
            break;
        }
        case SIMPLIFICATION_NEUTRALS: {
            phrases_array = SimplifyNeutralsPhrases;
            phrases_array_size = SimplifyNeutralsPhrasesSize;
            break;
        }
        case WRITING_RESULT: {
            phrases_array = WritingResultPhrases;
            phrases_array_size = WritingResultPhrasesSize;
            break;
        }
        case DIFF_START: {
            phrases_array = DiffStartPhrases;
            phrases_array_size = DiffStartPhrasesSize;
            break;
        }
        case DIFF_RESULT: {
            phrases_array = DiffResultPhrases;
            phrases_array_size = DiffResultPhrasesSize;
            break;
        }
        case TAILOR_START: {
            phrases_array = TailorStartPhrases;
            phrases_array_size = TailorStartPhrasesSize;
            break;
        }
        case TAILOR_NEW_DIFF: {
            phrases_array = TailorNewDiffPhrases;
            phrases_array_size = TailorNewDiffPhrasesSize;
            break;
        }
        case TAILOR_EVALUATE: {
            phrases_array = TailorEvaluatePhrases;
            phrases_array_size = TailorEvaluatePhrasesSize;
            break;
        }
        default: {
            return NULL;
        }
    }

    return phrases_array[get_random_index(phrases_array_size)];
}

/*=========================================================================================================*/

expression_error_t latex_log_check_substitutions(latex_log_info_t  *log_info,
                                                 expression_node_t *node) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);

    if(log_info->derivative->root != NULL) {
    }
    size_t subtree_size = find_tree_size(node);
    if(subtree_size < MinSubstitutionSubtreeSize) {
        return EXPRESSION_SUCCESS;
    }
    if(subtree_size > MaxSubstitutionSubtreeSize ||
       node == log_info->expression->root ||
       node == log_info->derivative->root ||
       node->is_substitution == true) {
        _RETURN_IF_ERROR(latex_log_check_substitutions(log_info, node->left));
        _RETURN_IF_ERROR(latex_log_check_substitutions(log_info, node->right));
        return EXPRESSION_SUCCESS;
    }

    node->is_substitution = true;
    sprintf(node->substitution_name, "I_{%llu}", log_info->substitutions_number++);
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t latex_log_write_substitutions(latex_log_info_t *log_info) {
    _C_ASSERT(log_info != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);

    if(log_info->to_write_number != 0) {
        fprintf(log_info->file, "где\n");
    }
    for(size_t i = 0; i < log_info->to_write_number; i++) {
        fprintf(log_info->file, "\\[%s = ", log_info->substitution_to_write[i]->substitution_name);
        expression_node_t *node = log_info->substitution_to_write[i];
        log_info->substitution_to_write[i] = NULL;
        _RETURN_IF_ERROR(SupportedOperations[node->value.operation].latex_logger(log_info, node));
        fprintf(log_info->file, "\\]\n");
    }

    log_info->to_write_number = 0;
    return EXPRESSION_SUCCESS;
}

