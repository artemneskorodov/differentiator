#ifndef EXPRESSION_TYPES_H
#define EXPRESSION_TYPES_H

#include <stdio.h>

static const size_t MaxVarsNumber = 10;
static const size_t MaxSubstitutionsNumber = 100;
static const size_t MaxSubstitutionNameSize = 16;

enum expression_error_t {
    EXPRESSION_SUCCESS                           = 0,
    EXPRESSION_VARS_DOUBLE_INIT                  = 1,
    EXPRESSION_VARIABLES_OVERFLOW                = 2,
    EXPRESSION_VARIABLE_UNSET                    = 3,
    EXPRESSION_UNKNOWN_VARIABLE                  = 4,
    EXPRESSION_SETTING_TO_ZERO_ERROR             = 5,
    EXPRESSION_READING_ERROR                     = 6,
    EXPRESSION_OPENING_FILE_ERROR                = 7,
    EXPRESSION_STRING_ALLOCATION_ERROR           = 8,
    EXPRESSION_UNKNOWN_NODE_TYPE                 = 9,
    EXPRESSION_CONTAINERS_ARRAY_ALLOCATION_ERROR = 10,
    EXPRESSION_CONTAINER_ALLOCATION_ERROR        = 11,
    EXPRESSION_READING_USER_INPUT_ERROR          = 12,
    EXPRESSION_DUMP_OPENING_ERROR                = 13,
    EXPRESSION_MEMSET_ERROR                      = 14,
    EXPRESSION_DIFFERENTIATING_ERROR             = 15,
    EXPRESSION_UNKNOWN_OPERATION                 = 16,
    EXPRESSION_UNKNOWN_ACTION                    = 17,
    EXPRESSION_LATEX_OPENING_ERROR               = 18,
    //TODO
};

#define _RETURN_IF_ERROR(...) {/*function call*/    \
    expression_error_t _error_code = (__VA_ARGS__); \
    if(_error_code != EXPRESSION_SUCCESS) {         \
        return _error_code;                         \
    }                                               \
}

enum node_type_t {
    NODE_TYPE_NUM,
    NODE_TYPE_OP,
    NODE_TYPE_VAR
};

enum operation_t {
    OPERATION_UNKNOWN = 0,
    OPERATION_ADD     = 1,
    OPERATION_SUB     = 2,
    OPERATION_DIV     = 3,
    OPERATION_MUL     = 4,
    OPERATION_SIN     = 5,
    OPERATION_COS     = 6,
    OPERATION_POW     = 7,
    OPERATION_LN      = 8,
    OPERATION_LOG     = 9,
    OPERATION_TG      = 10,
    OPERATION_CTG     = 11,
    OPERATION_ARCSIN  = 12,
    OPERATION_ARCCOS  = 13,
    OPERATION_ARCTG   = 14,
    OPERATION_ARCCTG  = 15,
    OPERATION_SH      = 16,
    OPERATION_CH      = 17,
    OPERATION_TH      = 18,
    OPERATION_CTH     = 19,
    //TODO
};

struct expression_dump_t {
    FILE                *technical_file;
    size_t               technical_number;
    const char          *technical_filename;
};

struct variable_t {
    char                 name;
    double               value;
};

struct parser_info_t {
    char *input;
    size_t position;
};

struct variables_list_t {
    variable_t           variables[MaxVarsNumber];
    size_t               size;
    size_t               capacity;
};

union node_value_t {
    double               numeric_value;
    size_t               variable_index;
    operation_t          operation;
};

struct expression_node_t {
    node_type_t          type;
    node_value_t         value;
    expression_node_t   *left;
    expression_node_t   *right;
    bool                 is_free;
    char                 substitution_name[MaxSubstitutionNameSize];
    bool                 is_substitution;
};

struct nodes_storage_t {
    expression_node_t  **containers;
    size_t               containers_number;
    size_t               container_capacity;
    size_t               capacity;
    size_t               size;
    expression_node_t   *free_head;
    expression_node_t   *free_tail;
};

struct expression_t {
    expression_node_t   *root;
    variables_list_t     variables_list;
    nodes_storage_t      nodes_storage;
    expression_dump_t    dump_info;
};

struct latex_log_info_t {
    FILE                *file;
    const char          *filename;
    expression_t        *expression;
    expression_t        *derivative;
    variables_list_t    *variables_list;
    expression_node_t   *substitution_to_write[MaxSubstitutionsNumber];
    size_t               to_write_number;
    size_t               substitutions_number;
};

struct operation_prototype_t {
    const char          *name;
    operation_t          code;
    const char          *latex_function;
    expression_error_t (*neutrals_simplifier)(expression_t *,
                                              expression_node_t *,
                                              expression_node_t **,
                                              latex_log_info_t *);
    expression_error_t (*latex_logger)       (latex_log_info_t *,
                                              expression_node_t *);
};

#endif
