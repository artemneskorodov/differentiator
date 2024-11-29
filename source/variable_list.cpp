#include <stdio.h>
#include <math.h>
#include <string.h>

#include "expression_types.h"
#include "variable_list.h"
#include "colors.h"

/*=========================================================================================================*/

static size_t get_variable_index(variables_list_t *variables, char varname);

/*=========================================================================================================*/

expression_error_t variables_list_ctor(variables_list_t *variables) {
    if(variables->size != 0 || variables->capacity != 0) {
        return EXPRESSION_VARS_DOUBLE_INIT;
    }

    variables->capacity = MaxVarsNumber;

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t variables_list_add(variables_list_t *variables, char varname, size_t *index) {
    *index = get_variable_index(variables, varname);
    if(*index < variables->capacity) {
        return EXPRESSION_SUCCESS;
    }

    if(variables->size >= variables->capacity) {
        return EXPRESSION_VARIABLES_OVERFLOW;
    }
    *index = variables->size;
    variable_t *new_variable = variables->variables + variables->size;
    new_variable->name       = varname;
    new_variable->value      = NAN;

    variables->size++;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t variables_list_dtor(variables_list_t *variables) {
    if(memset(variables, 0, sizeof(variables[0])) != variables) {
        return EXPRESSION_MEMSET_ERROR;
    }
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t variables_list_get_value(variables_list_t *variables, size_t index, double *value) {
    *value = variables->variables[index].value;
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t variables_list_set_from_console(variables_list_t *variables) {
    for(size_t var = 0; var < variables->size; var++) {
        color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "%c = ", variables->variables[var].name);
        if(scanf("%lg", &variables->variables[var].value) != 1) {
            print_error("Error while reading user input.\n");
            return EXPRESSION_READING_USER_INPUT_ERROR;
        }
    }
    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

expression_error_t variables_list_set_from_file(variables_list_t *variables, const char *filename) {
    FILE *variables_file = fopen(filename, "r");
    if(variables_file == NULL) {
        print_error("Error while opening variables file.\n");
        return EXPRESSION_OPENING_FILE_ERROR;
    }

    while(true) {
        char name = 0;
        double value = 0;

        if(fscanf(variables_file, "%c = %lg", &name, &value) != 2) {
            break;
        }

        size_t index = get_variable_index(variables, name);
        if(index >= variables->capacity) {
            return EXPRESSION_UNKNOWN_VARIABLE;
        }

        variables->variables[index].value = value;
    }

    return EXPRESSION_SUCCESS;
}

/*=========================================================================================================*/

size_t get_variable_index(variables_list_t *variables, char varname) {
    for(size_t var = 0; var < variables->size; var++) {
        if(varname == variables->variables[var].name) {
            return var;
        }
    }
    return variables->capacity;
}

/*=========================================================================================================*/

char variables_list_get_varname(variables_list_t *variables, size_t index) {
    return variables->variables[index].name;
}
