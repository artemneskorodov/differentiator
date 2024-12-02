#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "matan_killer.h"
#include "expression_types.h"
#include "expression_simplify.h"
#include "expression_utils.h"
#include "diff_dump.h"
#include "custom_assert.h"

/*=========================================================================================================*/

static expression_error_t expression_simplify_evaluate_subtree_operation (expression_t       *expression,
                                                                          expression_node_t  *node,
                                                                          double             *result,
                                                                          size_t             *changes_counter,
                                                                          latex_log_info_t   *log_info);

static expression_error_t expression_simplify_evaluate_subtree           (expression_t       *expression,
                                                                          expression_node_t  *node,
                                                                          double             *result,
                                                                          size_t             *changes_counter,
                                                                          latex_log_info_t   *log_info);

static expression_error_t expression_simplify_neutrals                   (expression_t       *expression,
                                                                          expression_node_t  *node,
                                                                          size_t             *changes_counter,
                                                                          expression_node_t **result,
                                                                          latex_log_info_t   *log_info);

/*=========================================================================================================*/

expression_error_t expression_simplify_evaluate_subtree(expression_t      *expression,
                                                        expression_node_t *node,
                                                        double            *result,
                                                        size_t            *changes_counter,
                                                        latex_log_info_t  *log_info) {
    // technical_dump(expression, node, "Trying to evaluate subtree");
    if(node == NULL) {
        *result = NAN;
        return EXPRESSION_SUCCESS;
    }
    switch(node->type) {
        case NODE_TYPE_VAR: {
            *result = NAN;
            return EXPRESSION_SUCCESS;
        }
        case NODE_TYPE_NUM: {
            *result = node->value.numeric_value;
            return EXPRESSION_SUCCESS;
        }
        case NODE_TYPE_OP: {
            _RETURN_IF_ERROR(expression_simplify_evaluate_subtree_operation(expression,
                                                                            node,
                                                                            result,
                                                                            changes_counter,
                                                                            log_info));
            return EXPRESSION_SUCCESS;
        }
        default: {
            *result = INFINITY;
            return EXPRESSION_UNKNOWN_NODE_TYPE;
        }
    }
}

/*=========================================================================================================*/

expression_error_t expression_simplify_evaluate_subtree_operation(expression_t      *expression,
                                                                  expression_node_t *node,
                                                                  double            *result,
                                                                  size_t            *changes_counter,
                                                                  latex_log_info_t  *log_info) {
    double result_left = NAN;
    double result_right = NAN;
    _RETURN_IF_ERROR(expression_simplify_evaluate_subtree(expression,
                                                          node->left,
                                                          &result_left,
                                                          changes_counter,
                                                          log_info));
    _RETURN_IF_ERROR(expression_simplify_evaluate_subtree(expression,
                                                          node->right,
                                                          &result_right,
                                                          changes_counter,
                                                          log_info));

    if(!isnan(result_left) && !isnan(result_right)) {
        *result = run_operation(result_left, result_right, *(operation_t *)&node->value);
        return EXPRESSION_SUCCESS;
    }

    if(!isnan(result_left) && isnan(result_right)) {
        if(node->left->left == NULL && node->left->right == NULL) {
            return EXPRESSION_SUCCESS;
        }
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_EVALUATE));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->left));
        node->left = new_node(expression, NODE_TYPE_NUM, {.numeric_value = result_left}, NULL, NULL);
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        (*changes_counter)++;
        return EXPRESSION_SUCCESS;
    }

    if(isnan(result_left) && !isnan(result_right)) {
        if(node->right->left == NULL && node->right->right == NULL) {
            return EXPRESSION_SUCCESS;
        }
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_EVALUATE));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
        node->right = new_node(expression, NODE_TYPE_NUM, {.numeric_value = result_right}, NULL, NULL);
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        (*changes_counter)++;
        return EXPRESSION_SUCCESS;
    }

    *result = NAN;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals(expression_t       *expression,
                                                expression_node_t  *node,
                                                size_t             *changes_counter,
                                                expression_node_t **result,
                                                latex_log_info_t   *log_info) {
    // technical_dump(expression, node, "Trying to simplify neutrals");
    if(node == NULL) {
        return EXPRESSION_SUCCESS;
    }
    if(node->type != NODE_TYPE_OP) {
        return EXPRESSION_SUCCESS;
    }
    if(SupportedOperations[node->value.operation].neutrals_simplifier != NULL) {
        _RETURN_IF_ERROR(SupportedOperations[node->value.operation].neutrals_simplifier(expression,
                                                                                        node,
                                                                                        result,
                                                                                        log_info));
    }
    if(*result != NULL) {
        (*changes_counter)++;
        return EXPRESSION_SUCCESS;
    }

    if(node->left != NULL) {
        expression_node_t *result_left = NULL;
        _RETURN_IF_ERROR(expression_simplify_neutrals(expression,
                                                      node->left,
                                                      changes_counter,
                                                      &result_left,
                                                      log_info));
        if(node->left->is_free) {
            node->left = NULL;
        }
        if(result_left != NULL) {
            node->left = result_left;
        }
    }
    if(node->right != NULL) {
        expression_node_t *result_right = NULL;
        _RETURN_IF_ERROR(expression_simplify_neutrals(expression,
                                                      node->right,
                                                      changes_counter,
                                                      &result_right,
                                                      log_info));
        if(node->right->is_free) {
            node->right = NULL;
        }
        if(result_right != NULL) {
            node->right = result_right;
        }
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals_add(expression_t       *expression,
                                                    expression_node_t  *node,
                                                    expression_node_t **result,
                                                    latex_log_info_t   *log_info) {
    if(is_node_equal(node->right, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
        *result =  node->left;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->left));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->left, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->left));
        *result = node->right;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->right));
        return EXPRESSION_SUCCESS;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals_sub(expression_t       *expression,
                                                    expression_node_t  *node,
                                                    expression_node_t **result,
                                                    latex_log_info_t   *log_info) {
    if(is_node_equal(node->right, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
        *result = node->left;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->left));
        return EXPRESSION_SUCCESS;
    }
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals_mul(expression_t       *expression,
                                                    expression_node_t  *node,
                                                    expression_node_t **result,
                                                    latex_log_info_t   *log_info) {
    if(is_node_equal(node->left, 0) || is_node_equal(node->right, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(set_node_to_const(expression, node, 0));
        *result = node;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->right, 1)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
        *result = node->left;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->left));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->left, 1)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->left));
        *result = node->right;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->right));
        return EXPRESSION_SUCCESS;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals_div(expression_t       *expression,
                                                    expression_node_t  *node,
                                                    expression_node_t **result,
                                                    latex_log_info_t   *log_info) {
    if(is_node_equal(node->right, 1)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
        *result = node->left;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->left));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->left, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(set_node_to_const(expression, node, 0));
        *result = node;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        return EXPRESSION_SUCCESS;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals_pow(expression_t       *expression,
                                                    expression_node_t  *node,
                                                    expression_node_t **result,
                                                    latex_log_info_t   *log_info) {
    if(is_node_equal(node->left, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(set_node_to_const(expression, node, 0));
        *result = node;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->left, 1)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(set_node_to_const(expression, node, 1));
        *result = node;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->right, 1)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
        *result = node->left;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node->left));
        return EXPRESSION_SUCCESS;
    }
    if(is_node_equal(node->right, 0)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(set_node_to_const(expression, node, 1));
        *result = node;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        return EXPRESSION_SUCCESS;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify_neutrals_log(expression_t       *expression,
                                                    expression_node_t  *node,
                                                    expression_node_t **result,
                                                    latex_log_info_t   *log_info) {
    if(is_node_equal(node->right, 1)) {
        _RETURN_IF_ERROR(latex_log_write_before(log_info, node, SIMPLIFICATION_NEUTRALS));
        _RETURN_IF_ERROR(set_node_to_const(expression, node, 0));
        *result = node;
        _RETURN_IF_ERROR(latex_log_write_after(log_info, node));
        return EXPRESSION_SUCCESS;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t expression_simplify(expression_t     *expression,
                                       latex_log_info_t *log_info) {
    while(true) {
        size_t changes_counter = 0;
        double evaluating_result = NAN;
        _RETURN_IF_ERROR(expression_simplify_evaluate_subtree(expression,
                                                              expression->root,
                                                              &evaluating_result,
                                                              &changes_counter,
                                                              log_info));
        if(!isnan(evaluating_result)) {
            _RETURN_IF_ERROR(expression_delete_subtree(expression, expression->root));
            expression->root = new_node(expression, NODE_TYPE_NUM, {.numeric_value = evaluating_result}, NULL, NULL);
            return EXPRESSION_SUCCESS;
        }

        expression_node_t *simplifying_neutrals_result = NULL;
        _RETURN_IF_ERROR(expression_simplify_neutrals(expression,
                                                      expression->root,
                                                      &changes_counter,
                                                      &simplifying_neutrals_result,
                                                      log_info));
        if(simplifying_neutrals_result != NULL) {
            _RETURN_IF_ERROR(nodes_storage_remove(&expression->nodes_storage, expression->root));
            expression->root = simplifying_neutrals_result;
        }

        if(changes_counter == 0) {
            break;
        }
    }
    return EXPRESSION_SUCCESS;
}
