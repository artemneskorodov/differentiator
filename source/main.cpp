#define TAILOR
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "variable_list.h"
#include "matan_killer.h"
#include "diff_dump.h"
#include "diff_dump.h"

int main(void) {
#ifdef DIFFERENTIATE
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
#endif
#ifdef TAILOR
    variables_list_t varlist = {};
    printf("vars ctor | %d\n", variables_list_ctor(&varlist));

    expression_t expression = {};
    printf("expr ctor | %d\n", expression_ctor(&expression, "expr", &varlist));
    printf("expr read | %d\n", expression_read_from_user(&expression, NULL));
    printf("vars set  | %d\n", variables_list_set_from_console(&varlist));

    expression_t derivative = {};
    printf("diff ctor | %d\n", expression_ctor(&derivative, "derv", &varlist));

    latex_log_info_t log_info = {};
    printf("log ctor  | %d\n", latex_log_ctor(&log_info, "tailor", &expression, &derivative));
    latex_log_write(&log_info, TAILOR_START, expression.root);
    printf("tailor    | %d\n", expression_tailor(&expression, &derivative, 5, &log_info));
    latex_log_write(&log_info, WRITING_RESULT, derivative.root);
    expression_dtor(&expression);
    variables_list_dtor(&varlist);
    expression_dtor(&derivative);
    latex_log_dtor(&log_info);
#endif
    return EXIT_SUCCESS;

}
