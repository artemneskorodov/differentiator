#ifndef MATAN_KILLER_H
#define MATAN_KILLER_H

#include <stdio.h>
#include <stdint.h>

#include "diff_dump.h"
#include "expression_simplify.h"
#include "expression_types.h"

static const operation_prototype_t SupportedOperations[] = {
    {/*EMPTY SPACE HERE BECAUSE OPERATION NUMBERS START FROM 1*/},
    {"+"  ,    OPERATION_ADD   , "+"       , expression_simplify_neutrals_add, latex_write_inorder                },
    {"-"  ,    OPERATION_SUB   , "-"       , expression_simplify_neutrals_sub, latex_write_inorder                },
    {"/"  ,    OPERATION_DIV   , "\\frac"  , expression_simplify_neutrals_div, latex_write_preorder_two_args      },
    {"*"  ,    OPERATION_MUL   , "\\times" , expression_simplify_neutrals_mul, latex_write_inorder                },
    {"sin",    OPERATION_SIN   , "\\sin"   , NULL                            , latex_write_preorder_one_arg       },
    {"cos",    OPERATION_COS   , "\\cos"   , NULL                            , latex_write_preorder_one_arg       },
    {"^"  ,    OPERATION_POW   , "^"       , expression_simplify_neutrals_pow, latex_write_inorder                },
    {"ln" ,    OPERATION_LN    , "\\ln"    , expression_simplify_neutrals_log, latex_write_preorder_one_arg       },
    {"log",    OPERATION_LOG   , "\\log"   , expression_simplify_neutrals_log, latex_write_preorder_two_args_index},
    {"tg" ,    OPERATION_TG    , "\\tg"    , NULL                            , latex_write_preorder_one_arg       },
    {"ctg",    OPERATION_CTG   , "\\ctg"   , NULL                            , latex_write_preorder_one_arg       },
    {"arcsin", OPERATION_ARCSIN, "\\arcsin", NULL                            , latex_write_preorder_one_arg       },
    {"arccos", OPERATION_ARCCOS, "\\arccos", NULL                            , latex_write_preorder_one_arg       },
    {"arctg" , OPERATION_ARCTG , "\\arctan", NULL                            , latex_write_preorder_one_arg       },
    {"arcctg", OPERATION_ARCCTG, "\\arcctg", NULL                            , latex_write_preorder_one_arg       },
    {"sh" ,    OPERATION_SH    , "\\sinh"  , NULL                            , latex_write_preorder_one_arg       },
    {"ch" ,    OPERATION_CH    , "\\cosh"  , NULL                            , latex_write_preorder_one_arg       },
    {"th" ,    OPERATION_TH    , "\\tanh"  , NULL                            , latex_write_preorder_one_arg       },
    {"cth",    OPERATION_CTH   , "\\cth"   , NULL                            , latex_write_preorder_one_arg       },
};

expression_error_t expression_ctor           (expression_t *expression,
                                              const char   *technical_filename);

expression_error_t expression_evaluate       (expression_t *expression,
                                              const char   *filename,
                                              double       *result);

expression_error_t expression_differentiate  (expression_t *expression,
                                              expression_t *derivative);

expression_error_t expression_dtor           (expression_t *expression);

expression_error_t expression_read_from_user (expression_t *expression,
                                              const char   *filename);

#endif
