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
#include "custom_assert.h"

/*=========================================================================================================*/

static const size_t MaxFunctionNameLength = 32;

/*=========================================================================================================*/

static expression_error_t expression_read_from_file    (char             **expression_string,
                                                        const char        *filename);

static expression_error_t expression_read_from_console (char             **expression_string);

static expression_error_t expression_evaluate_node     (expression_t      *expression,
                                                        expression_node_t *node,
                                                        double            *output);

/*=========================================================================================================*/

expression_error_t expression_ctor(expression_t     *expression,
                                   const char       *technical_filename,
                                   variables_list_t *variables_list) {
    _C_ASSERT(expression         != NULL, return EXPRESSION_NULL_POINTER       );
    _C_ASSERT(variables_list     != NULL, return EXPRESSION_VARIABLES_LIST_NULL);
    _C_ASSERT(technical_filename != NULL, return EXPRESSION_INVALID_FILENAME   );

    expression->variables_list = variables_list;
    _RETURN_IF_ERROR(nodes_storage_ctor(&expression->nodes_storage));

    _RETURN_IF_ERROR(technical_dump_ctor(expression, technical_filename));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_read_from_user(expression_t *expression,
                                             const char   *filename) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER);

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
    _C_ASSERT(expression_string != NULL, return EXPRESSION_RESULT_NULL_POINTER);

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
    _C_ASSERT(expression_string != NULL, return EXPRESSION_RESULT_NULL_POINTER);
    _C_ASSERT(filename          != NULL, return EXPRESSION_INVALID_FILENAME   );

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
                                       double       *result) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER       );
    _C_ASSERT(result     != NULL, return EXPRESSION_RESULT_NULL_POINTER);

    _RETURN_IF_ERROR(expression_evaluate_node(expression, expression->root, result));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_dtor(expression_t *expression) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER);

    _RETURN_IF_ERROR(variables_list_dtor(expression->variables_list));
    _RETURN_IF_ERROR(nodes_storage_dtor(&expression->nodes_storage));
    _RETURN_IF_ERROR(technical_dump_dtor(expression));
    if(memset(expression, 0, sizeof(*expression)) != expression) {
        return EXPRESSION_SETTING_TO_ZERO_ERROR;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_evaluate_node(expression_t      *expression,
                                            expression_node_t *node,
                                            double            *output) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER       );
    _C_ASSERT(node       != NULL, return EXPRESSION_NODE_NULL_POINTER  );
    _C_ASSERT(output     != NULL, return EXPRESSION_RESULT_NULL_POINTER);

    if(node->type == NODE_TYPE_NUM) {
        *output = node->value.numeric_value;
        return EXPRESSION_SUCCESS;
    }
    if(node->type == NODE_TYPE_VAR) {
        _RETURN_IF_ERROR(variables_list_get_value(expression->variables_list, node->value.variable_index, output));
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

expression_error_t expression_differentiate(expression_t     *expression,
                                            expression_t     *derivative,
                                            latex_log_info_t *log_info) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER);
    _C_ASSERT(derivative != NULL, return EXPRESSION_NULL_POINTER);

    derivative->root = differentiate_node(derivative, expression->root, 0, log_info);
    if(derivative->root == NULL) {
        return EXPRESSION_DIFFERENTIATING_ERROR;
    }
    _RETURN_IF_ERROR(expression_simplify(derivative, log_info));


    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_tailor(expression_t     *expression,
                                     expression_t     *tailor,
                                     size_t            members,
                                     latex_log_info_t *log_info) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER         );
    _C_ASSERT(tailor     != NULL, return EXPRESSION_NULL_POINTER         );
    _C_ASSERT(log_info   != NULL, return EXPRESSION_LOG_INFO_NULL_POINTER);

    _RETURN_IF_ERROR(nodes_storage_new_node(&tailor->nodes_storage, &tailor->root));
    double point = 0;
    _RETURN_IF_ERROR(variables_list_get_value(expression->variables_list, 0, &point));
    tailor->root->type = NODE_TYPE_OP;
    tailor->root->value.operation = OPERATION_ADD;
    expression_node_t *prev_node = NULL;
    expression_node_t *current_node = tailor->root;
    double factorial = 1;
    for(size_t mem = 0; mem < members; mem++) {
        double value = 0;
        _RETURN_IF_ERROR(expression_evaluate(expression, &value));
        _RETURN_IF_ERROR(latex_log_write(log_info, TAILOR_EVALUATE, expression->root, mem, value));
        _RETURN_IF_ERROR(latex_log_write(log_info, TAILOR_NEW_DIFF, expression->root, mem));
        expression_node_t *new_member = new_node(tailor, NODE_TYPE_OP, {.operation = OPERATION_MUL},
                                                 new_node(tailor, NODE_TYPE_NUM, {.numeric_value = value / factorial}, NULL, NULL),
                                                 new_node(tailor, NODE_TYPE_OP , {.operation = OPERATION_POW},
                                                          new_node(tailor, NODE_TYPE_OP, {.operation = OPERATION_SUB},
                                                                   new_node(tailor, NODE_TYPE_VAR, {.variable_index = 0}, NULL, NULL),
                                                                   new_node(tailor, NODE_TYPE_NUM, {.numeric_value = point}, NULL, NULL)),
                                                          new_node(tailor, NODE_TYPE_NUM, {.numeric_value = (double)mem}, NULL, NULL)));
        factorial *= (double)(mem + 1);
        if(mem + 1 != members) {
            current_node->left = new_member;
            _RETURN_IF_ERROR(expression_differentiate(expression, expression, log_info));
            _RETURN_IF_ERROR(latex_log_write(log_info, WRITING_RESULT, expression->root));
            current_node->right = new_node(tailor, NODE_TYPE_OP, {.operation = OPERATION_ADD}, NULL, NULL);
            prev_node = current_node;
            current_node = current_node->right;
        }
        else {
            _RETURN_IF_ERROR(nodes_storage_remove(&tailor->nodes_storage, prev_node->right));
            prev_node->right = new_member;
        }
    }
    _RETURN_IF_ERROR(expression_simplify(tailor, log_info));
    return EXPRESSION_SUCCESS;
}
