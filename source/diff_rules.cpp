#include "diff_rules.h"
#include "expression_types.h"
#include "expression_utils.h"
#include "matan_killer.h"
#include "diff_dump.h"
#include "colors.h"
#include "custom_assert.h"

expression_node_t *pow_derivative (expression_t      *derivative,
                                   expression_node_t *node,
                                   latex_log_info_t  *log_info,
                                   size_t             diff_variable);

expression_node_t *log_derivative (expression_t      *derivative,
                                   expression_node_t *node,
                                   latex_log_info_t  *log_info,
                                   size_t             diff_variable);

#define DIFF_DEFINITION(_func, _res) expression_node_t * diff_ ## _func (expression_t      *derivative,     \
                                                                         expression_node_t *node,           \
                                                                         latex_log_info_t  *log_info,       \
                                                                         size_t             diff_variable)  \
                                                                        {return (_res);}

#define _CONST(_value)      new_node(derivative, NODE_TYPE_NUM, {.numeric_value = (_value) }, NULL   , NULL    )
#define _ADD(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_ADD}, (_left), (_right))
#define _MUL(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_MUL}, (_left), (_right))
#define _DIV(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_DIV}, (_left), (_right))
#define _SUB(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_SUB}, (_left), (_right))
#define _COS(_value)        new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_COS}, NULL   , (_value))
#define _SIN(_value)        new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_SIN}, NULL   , (_value))
#define _POW(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_POW}, (_left), (_right))
#define _LN(_value)         new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_LN }, NULL   , (_value))
#define _LOG(_left, _right) new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_LOG}, (_left), (_right))
#define _CH(_value)         new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_CH }, NULL   , (_value))
#define _SH(_value)         new_node(derivative, NODE_TYPE_OP,  {.operation = OPERATION_SH }, NULL   , (_value))

#define _COPY_LEFT          copy_node(derivative, node->left )
#define _COPY_RIGHT         copy_node(derivative, node->right)
#define _DIFF_LEFT          differentiate_node(derivative, node->left , diff_variable, log_info)
#define _DIFF_RIGHT         differentiate_node(derivative, node->right, diff_variable, log_info)

expression_node_t *differentiate_node(expression_t      *derivative,
                                      expression_node_t *node,
                                      size_t             diff_variable,
                                      latex_log_info_t  *log_info) {
    _C_ASSERT(derivative != NULL, return NULL);
    _C_ASSERT(node       != NULL, return NULL);
    _C_ASSERT(log_info   != NULL, return NULL);
    switch(node->type) {
        case NODE_TYPE_NUM: {
            return _CONST(0);
        }
        case NODE_TYPE_VAR: {
            if(node->value.variable_index == diff_variable) {
                return _CONST(1);
            }
            else {
                return _CONST(0);
            }
        }
        case NODE_TYPE_OP: {
            expression_node_t *differentiation_result = SupportedOperations[node->value.operation].diff_func(derivative,
                                                                                                             node,
                                                                                                             log_info,
                                                                                                             diff_variable);
            latex_log_write(log_info, DIFFERENTIATION, node);
            latex_log_write(log_info, DIFF_RESULT, differentiation_result);
            return differentiation_result;
        }
        default: {
            print_error("Unknown expression node type.\n");
            return NULL;
        }
    }
}

DIFF_DEFINITION(add, _ADD(_DIFF_LEFT, _DIFF_RIGHT))

DIFF_DEFINITION(sub, _SUB(_DIFF_LEFT, _DIFF_RIGHT))

DIFF_DEFINITION(mul, _ADD(_MUL(_DIFF_LEFT, _COPY_RIGHT), _MUL(_COPY_LEFT, _DIFF_RIGHT)))

DIFF_DEFINITION(div, _DIV(_SUB(_MUL(_DIFF_LEFT, _COPY_RIGHT), _MUL(_COPY_LEFT, _DIFF_RIGHT)), _POW(_COPY_RIGHT, _CONST(2))))

DIFF_DEFINITION(sin, _MUL(_COS(_COPY_RIGHT), _DIFF_RIGHT))

DIFF_DEFINITION(cos, _MUL(_CONST(-1), _MUL(_SIN(_COPY_RIGHT), _DIFF_RIGHT)))

DIFF_DEFINITION(pow, pow_derivative(derivative, node, log_info, diff_variable))

DIFF_DEFINITION(ln, _DIV(_DIFF_RIGHT, _COPY_RIGHT))

DIFF_DEFINITION(log, log_derivative(derivative, node, log_info, diff_variable))

DIFF_DEFINITION(tg, _DIV(_DIFF_RIGHT, _POW(_COS(_COPY_RIGHT), _CONST(2))))

DIFF_DEFINITION(ctg, _MUL(_CONST(-1), _DIV(_DIFF_RIGHT, _POW(_SIN(_COPY_RIGHT), _CONST(2)))))

DIFF_DEFINITION(arcsin, _DIV(_DIFF_RIGHT, _POW(_SUB(_CONST(1), _POW(_COPY_RIGHT, _CONST(2))), _CONST(0.5))))

DIFF_DEFINITION(arccos, _DIV(_MUL(_CONST(-1), _DIFF_RIGHT), _POW(_SUB(_CONST(1), _POW(_COPY_RIGHT, _CONST(2))), _CONST(0.5))))

DIFF_DEFINITION(arctg, _DIV(_DIFF_RIGHT, _ADD(_CONST(1), _POW(_COPY_RIGHT, _CONST(2)))))

DIFF_DEFINITION(arcctg, _DIV(_MUL(_CONST(-1), _DIFF_RIGHT), _ADD(_CONST(1), _POW(_COPY_RIGHT, _CONST(2)))))

DIFF_DEFINITION(sh, _MUL(_CH(_COPY_RIGHT), _DIFF_RIGHT))

DIFF_DEFINITION(ch, _MUL(_SH(_COPY_RIGHT), _DIFF_RIGHT))

DIFF_DEFINITION(th, _DIV(_DIFF_RIGHT, _POW(_CH(_COPY_RIGHT), _CONST(2))))

DIFF_DEFINITION(cth, _DIV(_MUL(_CONST(-1), _DIFF_RIGHT), _POW(_SH(_COPY_RIGHT), _CONST(2))))


expression_node_t *pow_derivative(expression_t      *derivative,
                                  expression_node_t *node,
                                  latex_log_info_t  *log_info,
                                  size_t             diff_variable) {
    _C_ASSERT(derivative != NULL, return NULL);
    _C_ASSERT(node       != NULL, return NULL);
    _C_ASSERT(log_info   != NULL, return NULL);

    size_t left = count_variables(node->left, diff_variable);
    size_t right = count_variables(node->right, diff_variable);
    if(left == 0 && right == 0) {
        return _CONST(0);
    }
    if(left == 0 && right != 0) {
        return _MUL(_MUL(_LN(_COPY_LEFT),
                         _POW(_COPY_LEFT,
                              _COPY_RIGHT)),
                    _DIFF_RIGHT);
    }
    if(left != 0 && right == 0) {
        return _MUL(_COPY_RIGHT,
                    _POW(_COPY_LEFT,
                         _SUB(_COPY_RIGHT,
                              _CONST(1))));
    }

    return _MUL(_ADD(_MUL(_DIFF_RIGHT,
                          _LN(_COPY_LEFT)),
                     _MUL(_COPY_RIGHT,
                          _DIV(_DIFF_LEFT,
                               _COPY_LEFT))),
                _POW(_COPY_LEFT,
                     _COPY_RIGHT));
}

expression_node_t *log_derivative(expression_t      *derivative,
                                  expression_node_t *node,
                                  latex_log_info_t  *log_info,
                                  size_t             diff_variable) {
    _C_ASSERT(derivative != NULL, return NULL);
    _C_ASSERT(node       != NULL, return NULL);
    _C_ASSERT(log_info   != NULL, return NULL);

    size_t left = count_variables(node->left, diff_variable);
    size_t right = count_variables(node->right, diff_variable);
    if(left == 0 && right == 0) {
        return _CONST(0);
    }
    if(left == 0 && right != 0) {
        return _DIV(_DIFF_RIGHT,
                    _MUL(_COPY_RIGHT,
                         _LN(_COPY_LEFT)));
    }
    if(left != 0 && right == 0) {
        return _MUL(_CONST(-1),
                    _DIV(_MUL(_DIFF_LEFT,
                              _LN(_COPY_RIGHT)),
                         _MUL(_COPY_LEFT,
                              _POW(_LN(_COPY_LEFT),
                                   _CONST(2)))));
    }
    return _DIV(_SUB(_MUL(_DIV(_DIFF_RIGHT,
                               _COPY_RIGHT),
                          _LN(_COPY_LEFT)),
                     _MUL(_DIV(_DIFF_LEFT,
                               _COPY_LEFT),
                          _LN(_COPY_RIGHT))),
                _POW(_LN(_COPY_LEFT),
                     _CONST(2)));
}

#undef _CONST
#undef _ADD
#undef _MUL
#undef _DIV
#undef _SUB
#undef _COS
#undef _SIN
#undef _POW
#undef _LN
#undef _LOG
#undef _CH
#undef _SH
#undef _COPY_LEFT
#undef _COPY_RIGHT
#undef _DIFF_LEFT
#undef _DIFF_RIGHT
