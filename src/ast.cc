#include "ast.h"

template class visitable::Node<ast::ExpressionVisitor>;
template class visitable::Node<ast::StatementVisitor>;
