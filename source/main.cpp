#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "variable_list.h"
#include "matan_killer.h"
#include "diff_dump.h"

int main(void) {
    variables_list_t varlist = {};
    printf("vars ctor | %d\n", variables_list_ctor(&varlist));
    expression_t expression = {};
    printf("expr ctor | %d\n", expression_ctor(&expression, "expr", &varlist));
    printf("expr read | %d\n", expression_read_from_user(&expression, NULL));

    expression_t derivative = {};
    printf("diff ctor | %d\n", expression_ctor(&derivative, "derv", &varlist));
    printf("diff      | %d\n", expression_differentiate(&expression, &derivative));

    printf("dtor expr | %d\n", expression_dtor(&expression));
    printf("dtor diff | %d\n", expression_dtor(&derivative));
    return EXIT_SUCCESS;
}
