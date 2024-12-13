#ifndef MATAN_KILLER_H
#define MATAN_KILLER_H

#include <stdio.h>
#include <stdint.h>

#include "diff_dump.h"
#include "expression_simplify.h"
#include "expression_types.h"
#include "diff_rules.h"

static const operation_prototype_t SupportedOperations[] = {
    {/*EMPTY SPACE HERE BECAUSE OPERATION NUMBERS START FROM 1*/},
    {"+"  ,    OPERATION_ADD   , "+"       , simplify_neutrals_add, latex_write_inorder          , diff_add   , 3},
    {"-"  ,    OPERATION_SUB   , "-"       , simplify_neutrals_sub, latex_write_inorder          , diff_sub   , 3},
    {"/"  ,    OPERATION_DIV   , "\\frac"  , simplify_neutrals_div, latex_write_preorder_two_args, diff_div   , 2},
    {"*"  ,    OPERATION_MUL   , "\\times" , simplify_neutrals_mul, latex_write_inorder          , diff_mul   , 2},
    {"sin",    OPERATION_SIN   , "\\sin"   , NULL                 , latex_write_preorder_one_arg , diff_sin   , 0},
    {"cos",    OPERATION_COS   , "\\cos"   , NULL                 , latex_write_preorder_one_arg , diff_cos   , 0},
    {"^"  ,    OPERATION_POW   , "^"       , simplify_neutrals_pow, latex_write_inorder          , diff_pow   , 1},
    {"ln" ,    OPERATION_LN    , "\\ln"    , simplify_neutrals_log, latex_write_preorder_one_arg , diff_ln    , 0},
    {"log",    OPERATION_LOG   , "\\log"   , simplify_neutrals_log, latex_write_func_log         , diff_log   , 0},
    {"tg" ,    OPERATION_TG    , "\\tg"    , NULL                 , latex_write_preorder_one_arg , diff_tg    , 0},
    {"ctg",    OPERATION_CTG   , "\\ctg"   , NULL                 , latex_write_preorder_one_arg , diff_ctg   , 0},
    {"arcsin", OPERATION_ARCSIN, "\\arcsin", NULL                 , latex_write_preorder_one_arg , diff_arcsin, 0},
    {"arccos", OPERATION_ARCCOS, "\\arccos", NULL                 , latex_write_preorder_one_arg , diff_arccos, 0},
    {"arctg" , OPERATION_ARCTG , "\\arctan", NULL                 , latex_write_preorder_one_arg , diff_arctg , 0},
    {"arcctg", OPERATION_ARCCTG, "\\arcctg", NULL                 , latex_write_preorder_one_arg , diff_arcctg, 0},
    {"sh" ,    OPERATION_SH    , "\\sinh"  , NULL                 , latex_write_preorder_one_arg , diff_sh    , 0},
    {"ch" ,    OPERATION_CH    , "\\cosh"  , NULL                 , latex_write_preorder_one_arg , diff_ch    , 0},
    {"th" ,    OPERATION_TH    , "\\tanh"  , NULL                 , latex_write_preorder_one_arg , diff_th    , 0},
    {"cth",    OPERATION_CTH   , "\\cth"   , NULL                 , latex_write_preorder_one_arg , diff_cth   , 0},
};

expression_error_t expression_ctor           (expression_t     *expression,
                                              const char       *technical_filename,
                                              variables_list_t *variables_list);

expression_error_t expression_evaluate       (expression_t     *expression,
                                              double           *result);

expression_error_t expression_differentiate  (expression_t     *expression,
                                              expression_t     *derivative,
                                              latex_log_info_t *log_info);

expression_error_t expression_dtor           (expression_t     *expression);

expression_error_t expression_read_from_user (expression_t     *expression,
                                              const char       *filename);

expression_error_t expression_tailor         (expression_t     *expression,
                                              expression_t     *tailor,
                                              size_t            members,
                                              latex_log_info_t *log_info);

#endif
