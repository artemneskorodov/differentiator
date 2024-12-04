#ifndef DIFF_RULES_H
#define DIFF_RULES_H

#include "expression_types.h"

#define DIFF_PROT(_func) expression_node_t *diff_ ## _func (expression_t      *derivative,      \
                                                            expression_node_t *node,            \
                                                            latex_log_info_t  *log_info,        \
                                                            size_t             diff_variable)

DIFF_PROT(add   );
DIFF_PROT(sub   );
DIFF_PROT(mul   );
DIFF_PROT(div   );
DIFF_PROT(sin   );
DIFF_PROT(cos   );
DIFF_PROT(pow   );
DIFF_PROT(ln    );
DIFF_PROT(log   );
DIFF_PROT(tg    );
DIFF_PROT(ctg   );
DIFF_PROT(arcsin);
DIFF_PROT(arccos);
DIFF_PROT(arctg );
DIFF_PROT(arcctg);
DIFF_PROT(sh    );
DIFF_PROT(ch    );
DIFF_PROT(th    );
DIFF_PROT(cth   );

expression_node_t *differentiate_node(expression_t      *derivative,
                                      expression_node_t *node,
                                      size_t             diff_variable,
                                      latex_log_info_t  *log_info);

#undef DIFF_PROT

#endif
