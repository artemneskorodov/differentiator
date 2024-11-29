#ifndef EXPRESSION_UTILS_H
#define EXPRESSION_UTILS_H

#include "expression_types.h"

expression_error_t expression_clean_buffer (const char         *expression,
                                            size_t             *index);

expression_error_t nodes_storage_remove    (nodes_storage_t    *storage,
                                            expression_node_t  *node);

expression_error_t nodes_storage_ctor      (nodes_storage_t    *storage);

expression_error_t nodes_storage_new_node  (nodes_storage_t    *storage,
                                            expression_node_t **output);

expression_error_t nodes_storage_dtor      (nodes_storage_t    *storage);

expression_node_t *copy_node               (expression_t       *derivative,
                                            expression_node_t  *node);

expression_node_t *new_node                (expression_t       *expression,
                                            node_type_t         type,
                                            node_value_t        value,
                                            expression_node_t  *left,
                                            expression_node_t  *right);

double             run_operation           (double              left,
                                            double              right,
                                            operation_t         operation);

bool               is_leaf                 (expression_node_t  *node);

bool               is_node_equal           (expression_node_t  *node,
                                            double              value);

size_t             find_tree_size          (expression_node_t  *node);

#endif