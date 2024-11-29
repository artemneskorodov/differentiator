#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "matan_killer.h"
#include "diff_dump.h"

int main(void) {
    expression_t expression = {};
    printf("expr ctor | %d\n", expression_ctor(&expression, "expr"));
    printf("expr read | %d\n", expression_read_from_user(&expression, NULL));

    expression_t derivative = {};
    printf("diff ctor | %d\n", expression_ctor(&derivative, "derv"));
    printf("diff      | %d\n", expression_differentiate(&expression, &derivative));

    printf("dtor expr | %d\n", expression_dtor(&expression));
    printf("dtor diff | %d\n", expression_dtor(&expression));
    return EXIT_SUCCESS;
}
