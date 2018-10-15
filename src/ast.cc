#include "ast.h"

#include <sstream>

namespace ast {

const AnyExpression& GetMeta(const Expression& expression) {
  return expression.visit(
      [](const auto& x) -> const AnyExpression& { return x; });
}

}  // namespace ast
