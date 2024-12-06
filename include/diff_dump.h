#ifndef DIFF_DUMP_H
#define DIFF_DUMP_H

#include <stdio.h>

#include "expression_types.h"

enum log_action_t {
    DIFFERENTIATION,
    SIMPLIFICATION_EVALUATE,
    SIMPLIFICATION_NEUTRALS,
    WRITING_RESULT,
    DIFF_START,
    DIFF_RESULT,
    TAILOR_START,
    TAILOR_NEW_DIFF,
    TAILOR_EVALUATE,
};

expression_error_t technical_dump_ctor           (expression_t      *expression,
                                                  const char        *filename);

expression_error_t technical_dump_dtor           (expression_t      *expression);

expression_error_t technical_dump                (expression_t      *expression,
                                                  expression_node_t *current_node,
                                                  const char        *format, ...);

expression_error_t latex_log_ctor                (latex_log_info_t  *log_info,
                                                  const char        *filename,
                                                  expression_t      *expression,
                                                  expression_t      *derivative);

expression_error_t latex_log_write               (latex_log_info_t  *log_info,
                                                  log_action_t       action,
                                                  expression_node_t *node, ...);

expression_error_t latex_log_dtor                (latex_log_info_t  *log_info);

expression_error_t latex_write_inorder           (latex_log_info_t  *log_info,
                                                  expression_node_t *node);

expression_error_t latex_write_preorder_one_arg  (latex_log_info_t  *log_info,
                                                  expression_node_t *node);

expression_error_t latex_write_preorder_two_args (latex_log_info_t  *log_info,
                                                  expression_node_t *node);

expression_error_t latex_write_func_log          (latex_log_info_t  *log_info,
                                                  expression_node_t *node);


#endif
