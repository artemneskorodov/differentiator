#ifndef VARIABLE_LIST_H
#define VARIABLE_LIST_H

#include <stdio.h>
#include <stdbool.h>

#include "expression_types.h"

expression_error_t variables_list_ctor             (variables_list_t *variables);

expression_error_t variables_list_add              (variables_list_t *variables,
                                                    char              varname,
                                                    size_t           *index);

expression_error_t variables_list_get_value        (variables_list_t *variables,
                                                    size_t            index,
                                                    double           *value);

expression_error_t variables_list_dtor             (variables_list_t *variables);

expression_error_t variables_list_set_from_console (variables_list_t *variables);

expression_error_t variables_list_set_from_file    (variables_list_t *variables,
                                                    const char       *filename);

char               variables_list_get_varname      (variables_list_t *variables,
                                                    size_t            index);

#endif
