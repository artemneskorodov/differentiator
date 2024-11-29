#ifndef EXPRESSION_SIMPLIFY_H
#define EXPRESSION_SIMPLIFY_H

#include "expression_types.h"

expression_error_t expression_simplify_neutrals_add     (expression_t       *expression,
                                                         expression_node_t  *node,
                                                         expression_node_t **result,
                                                         latex_log_info_t   *log_info);

expression_error_t expression_simplify_neutrals_sub     (expression_t       *expression,
                                                         expression_node_t  *node,
                                                         expression_node_t **result,
                                                         latex_log_info_t   *log_info);

expression_error_t expression_simplify_neutrals_mul     (expression_t       *expression,
                                                         expression_node_t  *node,
                                                         expression_node_t **result,
                                                         latex_log_info_t   *log_info);

expression_error_t expression_simplify_neutrals_div     (expression_t       *expression,
                                                         expression_node_t  *node,
                                                         expression_node_t **result,
                                                         latex_log_info_t   *log_info);

expression_error_t expression_simplify_neutrals_pow     (expression_t       *expression,
                                                         expression_node_t  *node,
                                                         expression_node_t **result,
                                                         latex_log_info_t   *log_info);

expression_error_t expression_simplify_neutrals_log     (expression_t       *expression,
                                                         expression_node_t  *node,
                                                         expression_node_t **result,
                                                         latex_log_info_t   *log_info);

expression_error_t expression_simplify                  (expression_t       *expression,
                                                         latex_log_info_t   *log_info);

#endif
