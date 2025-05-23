#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "expression_utils.h"
#include "utils.h"
#include "colors.h"
#include "matan_killer.h"
#include "custom_assert.h"

/*=========================================================================================================*/

static const size_t NodesStorageContainerCapacity = 64;
static const size_t InitNodesContainersNumber     = 64;

/*=========================================================================================================*/

static expression_error_t nodes_check_containers_array_size (nodes_storage_t *storage);
static expression_error_t nodes_storage_new_container       (nodes_storage_t *storage);

/*=========================================================================================================*/

bool is_node_equal(expression_node_t *node, double value) {
    _C_ASSERT(node != NULL, return false);

    if(node->type != NODE_TYPE_NUM) {
        return false;
    }
    return is_equal(node->value.numeric_value, value);
}

/*=========================================================================================================*/

expression_error_t nodes_storage_remove(nodes_storage_t *storage, expression_node_t *node) {
    _C_ASSERT(storage != NULL, return EXPRESSION_NODES_STORAGE_NULL);
    _C_ASSERT(node    != NULL, return EXPRESSION_NODE_NULL_POINTER );

    node->left                              = NULL;
    node->right                             = storage->free_head;
    storage->free_head                      = node;
    storage->free_head->type                = (node_type_t)0;
    storage->free_head->value.numeric_value = 0;
    storage->free_head->is_free             = true;
    storage->size--;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t nodes_check_containers_array_size(nodes_storage_t *storage) {
    _C_ASSERT(storage != NULL, return EXPRESSION_NODES_STORAGE_NULL);

    if(storage->containers_number > storage->size / storage->container_capacity) {
        return EXPRESSION_SUCCESS;
    }
    expression_node_t **new_containers_array = (expression_node_t **)realloc(storage->containers,
                                                                             storage->containers_number * 2 *
                                                                             sizeof(storage->containers[0]));
    if(new_containers_array == NULL) {
        print_error("Error while reallocating nodes containers array.\n");
        return EXPRESSION_CONTAINERS_ARRAY_ALLOCATION_ERROR;
    }

    expression_node_t **new_memory = new_containers_array + storage->containers_number;
    if(memset(new_memory, 0, storage->containers_number * sizeof(storage->containers[0])) != new_memory) {
        print_error("Error while setting reallocated memory to zeros.\n");
        return EXPRESSION_SETTING_TO_ZERO_ERROR;
    }

    storage->containers = new_containers_array;
    storage->containers_number *= 2;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t nodes_storage_ctor(nodes_storage_t *storage) {
    _C_ASSERT(storage != NULL, return EXPRESSION_NODES_STORAGE_NULL);

    storage->containers_number  = InitNodesContainersNumber;
    storage->capacity           = 0;
    storage->container_capacity = NodesStorageContainerCapacity;

    storage->containers = (expression_node_t **)calloc(InitNodesContainersNumber,
                                                       sizeof(storage->containers[0]));
    if(storage->containers == NULL) {
        print_error("Error while creating nodes containers array.\n");
        return EXPRESSION_CONTAINERS_ARRAY_ALLOCATION_ERROR;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t nodes_storage_new_node(nodes_storage_t *storage, expression_node_t **output) {
    _C_ASSERT(storage != NULL, return EXPRESSION_NODES_STORAGE_NULL );
    _C_ASSERT(output  != NULL, return EXPRESSION_RESULT_NULL_POINTER);

    if(storage->size == storage->capacity) {
        _RETURN_IF_ERROR(nodes_storage_new_container(storage));
    }

    expression_node_t *new_free_head       = storage->free_head->right;
    storage->free_head->right              = NULL;
    storage->free_head->is_substitution    = false;
    storage->free_head->is_free            = false;
    *output                                = storage->free_head;
    storage->free_head                     = new_free_head;
    storage->size++;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t nodes_storage_dtor(nodes_storage_t *storage) {
    _C_ASSERT(storage != NULL, return EXPRESSION_NODES_STORAGE_NULL);

    for(size_t container = 0; container < storage->containers_number; container++) {
        free(storage->containers[container]);
    }
    free(storage->containers);
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

double run_operation(double left, double right, operation_t operation) {
    switch(operation) {
        case OPERATION_ADD: {
            return left + right;
        }
        case OPERATION_SUB: {
            return left - right;
        }
        case OPERATION_DIV: {
            return left / right;
        }
        case OPERATION_MUL: {
            return left * right;
        }
        case OPERATION_SIN: {
            return sin(right);
        }
        case OPERATION_COS: {
            return cos(right);
        }
        case OPERATION_POW: {
            return pow(left, right);
        }
        case OPERATION_LOG: {
            return log(right) / log(left);
        }
        case OPERATION_LN: {
            return log(right);
        }
        case OPERATION_TG: {
            return tan(right);
        }
        case OPERATION_CTG: {
            return 1 / tan(right);
        }
        case OPERATION_ARCSIN: {
            return asin(right);
        }
        case OPERATION_ARCCOS: {
            return acos(right);
        }
        case OPERATION_ARCTG: {
            return atan(right);
        }
        case OPERATION_ARCCTG: {
            return M_PI * 0.5 - atan(right);
        }
        case OPERATION_SH: {
            return sinh(right);
        }
        case OPERATION_CH: {
            return cosh(right);
        }
        case OPERATION_TH: {
            return tanh(right);
        }
        case OPERATION_CTH: {
            return 1 / tanh(right);
        }
        case OPERATION_UNKNOWN: {
            return NAN;
        }
        default: {
            return NAN;
        }
    }
}

/*=========================================================================================================*/

expression_node_t *new_node(expression_t      *expression,
                            node_type_t        type,
                            node_value_t       value,
                            expression_node_t *left,
                            expression_node_t *right) {
    _C_ASSERT(expression != NULL, return NULL);

    expression_node_t *node = NULL;
    if(nodes_storage_new_node(&expression->nodes_storage, &node) != EXPRESSION_SUCCESS) {
        return NULL;
    }
    node->type = type;
    node->value = value;
    node->left = left;
    node->right = right;
    return node;
}

/*=========================================================================================================*/

expression_node_t *copy_node(expression_t *derivative, expression_node_t *node) {
    if(node == NULL) {
        return NULL;
    }

    return new_node(derivative,
                    node->type,
                    node->value,
                    copy_node(derivative, node->left),
                    copy_node(derivative, node->right));
}

/*=========================================================================================================*/

bool is_leaf(expression_node_t *node) {
    // _C_ASSERT(node != NULL, return false);

    if(node == NULL) {
        return false;
    }

    if((node->left == NULL && node->right == NULL) || node->is_substitution) {
        return true;
    }
    return false;
}

/*=========================================================================================================*/

size_t find_tree_size(expression_node_t *node) {
    if(node == NULL) {
        return 0;
    }
    if(node->is_substitution) {
        return 1;
    }
    size_t result = 1;
    if(node->left != NULL) {
        result += find_tree_size(node->left);
    }
    if(node->right != NULL) {
        result += find_tree_size(node->right);
    }
    return result;
}

/*=========================================================================================================*/

expression_error_t nodes_storage_new_container(nodes_storage_t *storage) {
    _C_ASSERT(storage != NULL, return EXPRESSION_NODES_STORAGE_NULL);

    _RETURN_IF_ERROR(nodes_check_containers_array_size(storage));
    expression_node_t *new_container = (expression_node_t *)calloc(storage->container_capacity,
                                                                       sizeof(storage->containers[0][0]));
    if(new_container == NULL) {
        print_error("Error while allocating nodes container.\n");
        return EXPRESSION_CONTAINER_ALLOCATION_ERROR;
    }

    new_container[storage->container_capacity - 1].is_free = true;
    for(size_t i = 0; i + 1 < storage->container_capacity; i++) {
        new_container[i].is_free = true;
        new_container[i].right   = new_container + i + 1;
    }

    storage->free_head = new_container;
    storage->free_tail = new_container + storage->container_capacity - 1;

    storage->containers[storage->capacity / storage->container_capacity] = new_container;
    storage->capacity += storage->container_capacity;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

operation_t get_operation_code(const char *operation) {
    _C_ASSERT(operation != NULL, return OPERATION_UNKNOWN);

    for(size_t i = 1; i < sizeof(SupportedOperations) / sizeof(SupportedOperations[0]); i++) {
        if(strcmp(operation, SupportedOperations[i].name) == 0) {
            return SupportedOperations[i].code;
        }
    }
    return OPERATION_UNKNOWN;
}

/*=========================================================================================================*/

expression_error_t expression_delete_subtree(expression_t      *expression,
                                             expression_node_t *node) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER);

    if(node == NULL) {
        return EXPRESSION_SUCCESS;
    }
    if(node->left != NULL) {
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->left));
    }
    if(node->right != NULL) {
        _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
    }
    _RETURN_IF_ERROR(nodes_storage_remove(&expression->nodes_storage, node));
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

size_t count_variables(expression_node_t *node, size_t diff_variable) {
    if(node == NULL) {
        return 0;
    }
    if(node->type == NODE_TYPE_NUM) {
        return 0;
    }
    if(node->type == NODE_TYPE_VAR) {
        if(node->value.variable_index == diff_variable) {
            return 1;
        }
        else {
            return 0;
        }
    }

    return count_variables(node->left, diff_variable) + count_variables(node->right, diff_variable);
}

/*=========================================================================================================*/

expression_error_t set_node_to_const(expression_t *expression, expression_node_t *node, double value) {
    _C_ASSERT(expression != NULL, return EXPRESSION_NULL_POINTER     );
    _C_ASSERT(node       != NULL, return EXPRESSION_NODE_NULL_POINTER);

    _RETURN_IF_ERROR(expression_delete_subtree(expression, node->left));
    _RETURN_IF_ERROR(expression_delete_subtree(expression, node->right));
    node->left = NULL;
    node->right = NULL;
    node->is_substitution = false;
    node->type = NODE_TYPE_NUM;
    node->value.numeric_value = value;

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

bool is_bigger_priority(expression_node_t *node, expression_node_t *child) {
    if(child->type == NODE_TYPE_NUM ||
       child->type == NODE_TYPE_VAR) {
        return true;
    }
    if(node->type == NODE_TYPE_OP && child->type == NODE_TYPE_OP) {
        if(SupportedOperations[node->value.operation].priority >= SupportedOperations[child->value.operation].priority) {
            return true;
        }
    }
    return false;
}
