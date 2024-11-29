#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "matan_killer.h"
#include "expression_types.h"
#include "variable_list.h"
#include "colors.h"
#include "utils.h"
#include "expression_utils.h"
#include "expression_simplify.h"
#include "diff_dump.h"
#include "string_parser.h"

/*=========================================================================================================*/

static const size_t MaxFunctionNameLength = 32;

/*=========================================================================================================*/

static expression_error_t expression_read_from_file    (char             **expression_string,
                                                        const char        *filename);

static expression_error_t expression_read_from_console (char             **expression_string);

static expression_error_t expression_read_node_value   (expression_t      *expression,
                                                        expression_node_t *node,
                                                        const char        *string,
                                                        size_t            *index);

static expression_error_t expression_create_tree       (expression_t      *expression,
                                                        const char        *expression_string);

static expression_error_t expression_read_node         (expression_t      *expression,
                                                        expression_node_t *node,
                                                        const char        *string,
                                                        size_t            *index);

static expression_error_t expression_evaluate_node     (expression_t      *expression,
                                                        expression_node_t *node,
                                                        double            *output);

static expression_node_t *differentiate_node           (expression_t      *derivative,
                                                        expression_node_t *node,
                                                        size_t             diff_variable,
                                                        latex_log_info_t  *log_info);

static expression_node_t *operation_derivative         (expression_t      *derivative,
                                                        expression_node_t *node,
                                                        size_t             diff_variable,
                                                        latex_log_info_t  *log_info);

/*=========================================================================================================*/

expression_error_t expression_ctor(expression_t *expression,
                                   const char   *technical_filename) {
    _RETURN_IF_ERROR(variables_list_ctor(&expression->variables_list));
    _RETURN_IF_ERROR(nodes_storage_ctor(&expression->nodes_storage));

    _RETURN_IF_ERROR(technical_dump_ctor(expression, technical_filename));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_read_from_user(expression_t *expression,
                                             const char   *filename) {
    parser_info_t parser_info = {};
    if(filename == NULL) {
        _RETURN_IF_ERROR(expression_read_from_console(&parser_info.input));
    }
    else {
        _RETURN_IF_ERROR(expression_read_from_file(&parser_info.input, filename));
    }

    expression_error_t error_code = read_expression(expression, &parser_info);
    // free(expression_string);
    return error_code;
}

/*=========================================================================================================*/

expression_error_t expression_read_from_console(char **expression_string) {
    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Enter expression:\n");
    if(scanf("\n%m[^\n]", expression_string) != 1) {
        print_error("Error while reading expression.\n");
        return EXPRESSION_READING_ERROR;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_read_from_file(char       **expression_string,
                                             const char  *filename) {
    FILE *expression_file = fopen(filename, "rb");
    if(expression_file == NULL) {
        print_error("Error while opening file %s.\n");
        return EXPRESSION_OPENING_FILE_ERROR;
    }
    size_t size = file_size(expression_file);
    *expression_string = (char *)calloc(size + 1, sizeof(char));
    if(*expression_string == NULL) {
        fclose(expression_file);
        print_error("Error while allocating memory to expression string.\n");
        return EXPRESSION_STRING_ALLOCATION_ERROR;
    }

    if(fread(*expression_string, sizeof(char), size, expression_file) != size) {
        fclose(expression_file);
        return EXPRESSION_READING_ERROR;
    }

    fclose(expression_file);
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_evaluate(expression_t *expression,
                                       const char   *filename,
                                       double       *result) {
    if(filename == NULL) {
        _RETURN_IF_ERROR(variables_list_set_from_console(&expression->variables_list));
    }
    else {
        _RETURN_IF_ERROR(variables_list_set_from_file(&expression->variables_list, filename));
    }
    _RETURN_IF_ERROR(expression_evaluate_node(expression, expression->root, result));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_dtor(expression_t *expression) {
    _RETURN_IF_ERROR(variables_list_dtor(&expression->variables_list));
    _RETURN_IF_ERROR(nodes_storage_dtor(&expression->nodes_storage));
    _RETURN_IF_ERROR(technical_dump_dtor(expression));
    if(memset(expression, 0, sizeof(*expression)) != expression) {
        return EXPRESSION_SETTING_TO_ZERO_ERROR;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_create_tree(expression_t *expression,
                                          const char   *expression_string) {
    size_t string_index = 0;
    _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &expression->root));

    _RETURN_IF_ERROR(expression_read_node(expression, expression->root, expression_string, &string_index));

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_read_node(expression_t      *expression,
                                        expression_node_t *node,
                                        const char        *string,
                                        size_t            *index) {
    _RETURN_IF_ERROR(expression_clean_buffer(string, index));
    if(string[(*index)] == '\0') {
        return EXPRESSION_SUCCESS;
    }
    if(string[(*index)++] != '(') {
        print_error("Unexpected character '%c' in expression on index %llu.\n", string[*index], *index - 1);
        return EXPRESSION_READING_ERROR;
    }

    _RETURN_IF_ERROR(expression_clean_buffer(string, index));

    if(string[(*index)] == '(') {
        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &node->left));
        _RETURN_IF_ERROR(expression_read_node(expression, node->left, string, index));
        _RETURN_IF_ERROR(expression_clean_buffer(string, index));
    }

    _RETURN_IF_ERROR(expression_read_node_value(expression, node, string, index));

    if(string[(*index)] == '(') {
        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &node->right));
        _RETURN_IF_ERROR(expression_read_node(expression, node->right, string, index));
        _RETURN_IF_ERROR(expression_clean_buffer(string, index));
    }

    if(string[(*index)++] != ')') {
        print_error("Unexpected character '%c' in expression on index %llu.\n", string[*index], *index - 1);
        return EXPRESSION_READING_ERROR;
    }
    _RETURN_IF_ERROR(expression_clean_buffer(string, index));

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_read_node_value(expression_t      *expression,
                                              expression_node_t *node,
                                              const char        *string,
                                              size_t            *index) {
    int read_symbols = 0;
    if(sscanf(string + (*index), "%lg%n", &node->value.numeric_value, &read_symbols) == 1) {
        *index += read_symbols;
        node->type = NODE_TYPE_NUM;
    }

    else if(isalpha(string[(*index)]) && !isalpha(string[(*index) + 1])){
        _RETURN_IF_ERROR(variables_list_add(&expression->variables_list, string[(*index)], &node->value.variable_index));
        node->type = NODE_TYPE_VAR;
        (*index)++;
    }

    else {
        char function[MaxFunctionNameLength + 1] = {};
        if(sscanf(string + (*index), "%[^\n (]%n", function, &read_symbols) != 1) {
            print_error("Unexpected expression on index %llu.\n", *index);
            return EXPRESSION_READING_ERROR;
        }
        node->type = NODE_TYPE_OP;
        operation_t operation = get_operation_code(function);
        if(operation == OPERATION_UNKNOWN) {
            print_error("Unknown operation on index %llu.\n", *index);
            return EXPRESSION_READING_ERROR;
        }
        node->value.operation = operation;
        *index += read_symbols;
    }
    _RETURN_IF_ERROR(expression_clean_buffer(string, index));
    technical_dump(expression, node, "");
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_evaluate_node(expression_t      *expression,
                                            expression_node_t *node,
                                            double            *output) {
    if(node->type == NODE_TYPE_NUM) {
        *output = node->value.numeric_value;
        return EXPRESSION_SUCCESS;
    }
    if(node->type == NODE_TYPE_VAR) {
        _RETURN_IF_ERROR(variables_list_get_value(&expression->variables_list, node->value.variable_index, output));
        return EXPRESSION_SUCCESS;
    }
    if(node->type == NODE_TYPE_OP) {
        double left = 0, right = 0;
        if(node->left != NULL) {
            _RETURN_IF_ERROR(expression_evaluate_node(expression, node->left, &left));
        }
        if(node->right != NULL) {
            _RETURN_IF_ERROR(expression_evaluate_node(expression, node->right, &right));
        }

        *output = run_operation(left, right, node->value.operation);
        return EXPRESSION_SUCCESS;
    }

    return EXPRESSION_UNKNOWN_NODE_TYPE;
}

/*=========================================================================================================*/

expression_error_t expression_differentiate(expression_t *expression,
                                            expression_t *derivative) {
    latex_log_info_t log_info = {};
    _RETURN_IF_ERROR(latex_log_ctor(&log_info, "diff_log", expression, derivative));
    derivative->root = differentiate_node(derivative, expression->root, 0, &log_info);
    if(derivative->root == NULL) {
        return EXPRESSION_DIFFERENTIATING_ERROR;
    }

    _RETURN_IF_ERROR(expression_simplify(derivative, &log_info));
    _RETURN_IF_ERROR(latex_log_write_before(&log_info, derivative->root, WRITING_RESULT));
    _RETURN_IF_ERROR(latex_log_dtor(&log_info));

    technical_dump(derivative, NULL, "answer");


    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

#define _CONST(_value)      new_node(derivative, NODE_TYPE_NUM, {.numeric_value = (_value) }, NULL   , NULL    )
#define _ADD(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_ADD}, (_left), (_right))
#define _MUL(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_MUL}, (_left), (_right))
#define _DIV(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_DIV}, (_left), (_right))
#define _SUB(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_SUB}, (_left), (_right))
#define _COS(_value)        new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_COS}, NULL   , (_value))
#define _SIN(_value)        new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_SIN}, NULL   , (_value))
#define _POW(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_POW}, (_left), (_right))
#define _LN(_value)         new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_LN }, NULL   , (_value))
#define _LOG(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_LOG}, (_left), (_right))
#define _CH(_value)         new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_CH }, NULL   , (_value))
#define _SH(_value)         new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_SH }, NULL   , (_value))

#define _COPY_LEFT          copy_node(derivative, node->left )
#define _COPY_RIGHT         copy_node(derivative, node->right)
#define _DIFF_LEFT          differentiate_node(derivative, node->left , diff_variable, log_info)
#define _DIFF_RIGHT         differentiate_node(derivative, node->right, diff_variable, log_info)

/*=========================================================================================================*/

expression_node_t *differentiate_node(expression_t      *derivative,
                                      expression_node_t *node,
                                      size_t             diff_variable,
                                      latex_log_info_t  *log_info) {
    switch(node->type) {
        case NODE_TYPE_NUM: {
            return _CONST(0);
        }
        case NODE_TYPE_VAR: {
            if(node->value.variable_index == diff_variable) {
                return _CONST(1);
            }
            else {
                return _CONST(0);
            }
        }
        case NODE_TYPE_OP: {
            expression_node_t *differentiation_result = operation_derivative(derivative, node, diff_variable, log_info);
            latex_log_write(log_info, node, differentiation_result, DIFFERENTIATION);
            return differentiation_result;
        }
        default: {
            print_error("Unknown expression node type.\n");
            return NULL;
        }
    }
}

/*=========================================================================================================*/

expression_node_t *operation_derivative(expression_t      *derivative,
                                        expression_node_t *node,
                                        size_t             diff_variable,
                                        latex_log_info_t  *log_info) {
    switch(node->value.operation) {
        case OPERATION_ADD: {
            return _ADD(_DIFF_LEFT,
                        _DIFF_RIGHT);
        }
        case OPERATION_SUB: {
            return _SUB(_DIFF_LEFT,
                        _DIFF_RIGHT);
        }
        case OPERATION_MUL: {
            return _ADD(_MUL(_DIFF_LEFT,
                             _COPY_RIGHT),
                        _MUL(_DIFF_RIGHT,
                             _COPY_LEFT));
        }
        case OPERATION_DIV: {
            return _DIV(_SUB(_MUL(_DIFF_LEFT,
                                  _COPY_RIGHT),
                             _MUL(_COPY_LEFT,
                                  _DIFF_RIGHT)),
                        _POW(_COPY_RIGHT,
                             _CONST(2)));
        }
        case OPERATION_SIN: {
            return _MUL(_COS(_COPY_RIGHT),
                        _DIFF_RIGHT);
        }
        case OPERATION_COS: {
            return _MUL(_CONST(-1),
                        _MUL(_SIN(_COPY_RIGHT),
                             _DIFF_RIGHT));
        }
        case OPERATION_POW: {
            size_t left = count_variables(node->left, diff_variable);
            size_t right = count_variables(node->right, diff_variable);
            if(left == 0 && right == 0) {
                return _CONST(0);
            }
            if(left == 0 && right != 0) {
                return _MUL(_MUL(_LN(_COPY_LEFT),
                                 _POW(_COPY_LEFT,
                                      _COPY_RIGHT)),
                            _DIFF_RIGHT);
            }
            if(left != 0 && right == 0) {
                return _MUL(_COPY_RIGHT,
                            _POW(_COPY_LEFT,
                                 _SUB(_COPY_RIGHT,
                                      _CONST(1))));
            }

            return _MUL(_ADD(_MUL(_DIFF_RIGHT,
                                  _LN(_COPY_LEFT)),
                             _MUL(_COPY_RIGHT,
                                  _DIV(_DIFF_LEFT,
                                       _COPY_LEFT))),
                        _POW(_COPY_LEFT,
                             _COPY_RIGHT));
        }
        case OPERATION_LN: {
            return _DIV(_DIFF_RIGHT,
                        _COPY_RIGHT);
        }
        case OPERATION_LOG: {
            size_t left = count_variables(node->left, diff_variable);
            size_t right = count_variables(node->right, diff_variable);
            if(left == 0 && right == 0) {
                return _CONST(0);
            }
            if(left == 0 && right != 0) {
                return _DIV(_DIFF_RIGHT,
                            _MUL(_COPY_RIGHT,
                                 _LN(_COPY_LEFT)));
            }
            if(left != 0 && right == 0) {
                return _MUL(_CONST(-1),
                            _DIV(_MUL(_DIFF_LEFT,
                                      _LN(_COPY_RIGHT)),
                                 _MUL(_COPY_LEFT,
                                      _POW(_LN(_COPY_LEFT),
                                           _CONST(2)))));
            }
            return _DIV(_SUB(_MUL(_DIV(_DIFF_RIGHT,
                                       _COPY_RIGHT),
                                  _LN(_COPY_LEFT)),
                             _MUL(_DIV(_DIFF_LEFT,
                                       _COPY_LEFT),
                                  _LN(_COPY_RIGHT))),
                        _POW(_LN(_COPY_LEFT),
                             _CONST(2)));
        }
        case OPERATION_TG: {
            return _DIV(_DIFF_RIGHT,
                        _POW(_COS(_COPY_RIGHT),
                             _CONST(2)));
        }
        case OPERATION_CTG: {
            return _MUL(_CONST(-1),
                        _DIV(_DIFF_RIGHT,
                             _POW(_SIN(_COPY_RIGHT),
                             _CONST(2))));
        }
        case OPERATION_ARCSIN: {
            return _DIV(_DIFF_RIGHT,
                        _POW(_SUB(_CONST(1),
                                  _POW(_COPY_RIGHT, _CONST(2))),
                             _CONST(0.5)));
        }
        case OPERATION_ARCCOS: {
            return _DIV(_MUL(_CONST(-1), _DIFF_RIGHT),
                        _POW(_SUB(_CONST(1),
                                  _POW(_COPY_RIGHT, _CONST(2))),
                             _CONST(0.5)));
        }
        case OPERATION_ARCTG: {
            return _DIV(_DIFF_RIGHT,
                        _ADD(_CONST(1),
                             _POW(_COPY_RIGHT, _CONST(2))));
        }
        case OPERATION_ARCCTG: {
            return _DIV(_MUL(_CONST(-1), _DIFF_RIGHT),
                        _ADD(_CONST(1), _POW(_COPY_RIGHT, _CONST(2))));
        }
        case OPERATION_SH: {
            return _MUL(_CH(_COPY_RIGHT), _DIFF_RIGHT);
        }
        case OPERATION_CH: {
            return _MUL(_SH(_COPY_RIGHT), _DIFF_RIGHT);
        }
        case OPERATION_TH: {
            return _DIV(_DIFF_RIGHT,
                        _POW(_CH(_COPY_RIGHT), _CONST(2)));
        }
        case OPERATION_CTH: {
            return _DIV(_MUL(_CONST(-1), _DIFF_RIGHT),
                        _POW(_SH(_COPY_RIGHT), _CONST(2)));
        }

        case OPERATION_UNKNOWN: {
            return NULL;
        }
        default: {
            return NULL;
        }
    }
    return NULL;
}

/*=========================================================================================================*/

#undef _CONST
#undef _ADD
#undef _MUL
#undef _DIV
#undef _SUB
#undef _COPY_LEFT
#undef _COPY_RIGHT
#undef _DIFF_LEFT
#undef _DIFF_RIGHT
