#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "string_parser.h"
#include "expression_types.h"
#include "variable_list.h"
#include "expression_utils.h"

/*=========================================================================================================*/

static expression_error_t expression_get_expr (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

static expression_error_t expression_get_mul  (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

static expression_error_t expression_get_pow  (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

static expression_error_t expression_getP     (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

static expression_error_t expression_get_num  (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

static expression_error_t expression_get_var  (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

static expression_error_t expression_get_func (expression_t       *expression,
                                               expression_node_t **output,
                                               parser_info_t      *parser_info);

/*=========================================================================================================*/

expression_error_t read_expression(expression_t  *expression,
                                   parser_info_t *parser_info) {
    expression_node_t *root = NULL;
    _RETURN_IF_ERROR(expression_get_expr(expression, &root, parser_info));
    if(parser_info->input[parser_info->position] != '\0') {
        return EXPRESSION_READING_ERROR;
    }
    parser_info->position++;
    expression->root = root;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_get_expr(expression_t       *expression,
                                       expression_node_t **output,
                                       parser_info_t      *parser_info) {
    expression_node_t *res = NULL;
    _RETURN_IF_ERROR(expression_get_mul(expression, &res, parser_info));
    while(parser_info->input[parser_info->position] == '+' ||
          parser_info->input[parser_info->position] == '-') {
        char operation = parser_info->input[parser_info->position];
        parser_info->position++;

        expression_node_t *new_res = NULL;
        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &new_res));
        _RETURN_IF_ERROR(expression_get_mul(expression, &new_res->right, parser_info));
        new_res->left = res;
        new_res->type = NODE_TYPE_OP;
        if(operation == '+') {
            new_res->value.operation = OPERATION_ADD;
        }
        else {
            new_res->value.operation = OPERATION_SUB;
        }

        res = new_res;
    }
    *output = res;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_get_mul(expression_t       *expression,
                                      expression_node_t **output,
                                      parser_info_t      *parser_info) {
    expression_node_t *res = NULL;
    _RETURN_IF_ERROR(expression_get_pow(expression, &res, parser_info));
    while(parser_info->input[parser_info->position] == '*' ||
          parser_info->input[parser_info->position] == '/') {
        char operation = parser_info->input[parser_info->position];
        parser_info->position++;
        expression_node_t *new_res = NULL;
        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &new_res));
        _RETURN_IF_ERROR(expression_get_pow(expression, &new_res->right, parser_info));
        new_res->left = res;
        new_res->type = NODE_TYPE_OP;
        if(operation == '*') {
            new_res->value.operation = OPERATION_MUL;
        }
        else {
            new_res->value.operation = OPERATION_DIV;
        }

        res = new_res;
    }
    *output = res;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_get_pow(expression_t       *expression,
                                      expression_node_t **output,
                                      parser_info_t      *parser_info) {
    expression_node_t *res = NULL;
    _RETURN_IF_ERROR(expression_getP(expression, &res, parser_info));
    while(parser_info->input[parser_info->position] == '^') {
        parser_info->position++;
        expression_node_t *new_res = NULL;
        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &new_res));
        _RETURN_IF_ERROR(expression_getP(expression, &new_res->right, parser_info));
        new_res->left = res;
        new_res->type = NODE_TYPE_OP;
        new_res->value.operation = OPERATION_POW;
        res = new_res;
    }
    *output = res;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_getP(expression_t       *expression,
                                   expression_node_t **output,
                                   parser_info_t      *parser_info) {
    if(parser_info->input[parser_info->position] == '(') {
        parser_info->position++;
        _RETURN_IF_ERROR(expression_get_expr(expression, output, parser_info));
        if(parser_info->input[parser_info->position] != ')') {
            return EXPRESSION_READING_ERROR;
        }
        parser_info->position++;
        return EXPRESSION_SUCCESS;
    }
    _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, output));
    char *symbols = parser_info->input + parser_info->position;
    if(isdigit(symbols[0]) ||
       (symbols[0] == '-' && isdigit(symbols[1]))) {
        return expression_get_num(expression, output, parser_info);
    }
    if((isalpha(symbols[0]) && !isalpha(symbols[1])) ||
       (symbols[0] == '-' && isalpha(symbols[1]) && !isalpha(symbols[2]))) {
        return expression_get_var(expression, output, parser_info);
    }
    if(isalpha(symbols[0]) && isalpha(symbols[1])) {
        return expression_get_func(expression, output, parser_info);
    }

    return EXPRESSION_READING_ERROR;
}

/*=========================================================================================================*/

expression_error_t expression_get_num(expression_t       *expression,
                                      expression_node_t **output,
                                      parser_info_t      *parser_info) {
    (*output)->type = NODE_TYPE_NUM;

    double multiplier = 1;
    double result = 0;
    double pow = 0.1;
    if(parser_info->input[parser_info->position] == '-') {
        multiplier = -1;
        parser_info->position++;
    }
    while(isdigit(parser_info->input[parser_info->position])) {
        result = 10 * result + (parser_info->input[parser_info->position] - '0');
        parser_info->position++;
    }
    if(parser_info->input[parser_info->position] != '.') {
        (*output)->value.numeric_value = result;
        return EXPRESSION_SUCCESS;
    }
    parser_info->position++;
    while(isdigit(parser_info->input[parser_info->position])) {
        result += pow * (parser_info->input[parser_info->position] - '0');
        parser_info->position++;
        pow *= 0.1;
    }

    (*output)->value.numeric_value = result * multiplier;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_get_var(expression_t       *expression,
                                      expression_node_t **output,
                                      parser_info_t      *parser_info) {
    (*output)->type = NODE_TYPE_VAR;
    size_t *output_index = &(*output)->value.variable_index;
    if(parser_info->input[parser_info->position] == '-') {
        parser_info->position++;
        (*output)->type = NODE_TYPE_OP;
        (*output)->value.operation = OPERATION_MUL;

        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &(*output)->left));
        (*output)->left->type = NODE_TYPE_NUM;
        (*output)->left->value.numeric_value = -1;

        _RETURN_IF_ERROR(nodes_storage_new_node(&expression->nodes_storage, &(*output)->right));
        (*output)->right->type = NODE_TYPE_VAR;
        output_index = &(*output)->right->value.variable_index;
    }
    char varname = parser_info->input[parser_info->position++];
    _RETURN_IF_ERROR(variables_list_add(expression->variables_list, varname, output_index));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_get_func(expression_t       *expression,
                                       expression_node_t **output,
                                       parser_info_t      *parser_info) {
    (*output)->type = NODE_TYPE_OP;
    char function[256] = {};
    size_t function_index = 0;
    while(isalpha(parser_info->input[parser_info->position])) {
        function[function_index++] = parser_info->input[parser_info->position++];
    }

    operation_t operation = get_operation_code(function);
    if(operation == OPERATION_UNKNOWN) {
        return EXPRESSION_UNKNOWN_OPERATION;
    }
    (*output)->value.operation = operation;
    if(parser_info->input[parser_info->position] != '(') {
        return EXPRESSION_READING_ERROR;
    }
    parser_info->position++;
    _RETURN_IF_ERROR(expression_get_expr(expression, &(*output)->right, parser_info));
    if(parser_info->input[parser_info->position] != ')') {
        return EXPRESSION_READING_ERROR;
    }
    parser_info->position++;
    return EXPRESSION_SUCCESS;
}
