#ifndef STRING_PARSER_H
#define STRING_PARSER_H

#include "expression_types.h"
#include "expression_utils.h"
#include "variable_list.h"

expression_error_t read_expression(expression_t *expression, parser_info_t *parser_info);

#endif
