#define TAILOR
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "variable_list.h"
#include "matan_killer.h"
#include "diff_dump.h"
#include "diff_dump.h"

int main(int argc, const char *argv[]) {
    if(argc != 2) {
        printf("Unexpected amount of console parameters.\n");
        return EXIT_FAILURE;
    }
    if(strcmp(argv[1], "--diff") == 0) {
        variables_list_t varlist = {};
        printf("vars ctor | %d\n", variables_list_ctor(&varlist));
        expression_t expression = {};
        printf("expr ctor | %d\n", expression_ctor(&expression, "expr", &varlist));
        printf("expr read | %d\n", expression_read_from_user(&expression, NULL));

        expression_t derivative = {};
        printf("diff ctor | %d\n", expression_ctor(&derivative, "derv", &varlist));

        latex_log_info_t log_info = {};
        printf("log ctor  | %d\n", latex_log_ctor(&log_info, "tailor", &expression, &derivative));
        latex_log_write(&log_info, DIFF_START, expression.root);

        printf("diff      | %d\n", expression_differentiate(&expression, &derivative, &log_info));

        latex_log_write(&log_info, WRITING_RESULT, derivative.root);

        printf("dtor expr | %d\n", expression_dtor(&expression));
        printf("dtor diff | %d\n", expression_dtor(&derivative));
    }
    else if(strcmp(argv[1], "--tailor") == 0) {
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

        printf("vars dtor | %d\n", variables_list_dtor(&varlist));
        printf("expr dtor | %d\n", expression_dtor(&expression));
        printf("diff dtor | %d\n", expression_dtor(&derivative));
        printf("log dtor  | %d\n", latex_log_dtor(&log_info));
    }
    else {
        printf("Unknown flag '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
