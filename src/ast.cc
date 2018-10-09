#include "ast.h"

#include <sstream>

namespace ast {

bool operator==(const Function& left, const Function& right) {
  if (!(left.return_type == right.return_type)) return false;
  if (left.parameters.size() != right.parameters.size()) return false;
  for (std::size_t i = 0, n = left.parameters.size(); i < n; i++) {
    if (!(left.parameters[i].type == right.parameters[i].type)) return false;
  }
  return true;
}

std::ostream& operator<<(std::ostream& output, const Type& type) {
  type.visit(
      [&](const auto& node) {
        using value_type = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<value_type, Void>) {
          output << "void";
        } else if constexpr (std::is_same_v<value_type, Primitive>) {
          switch (node) {
            case Primitive::BOOLEAN:
              output << "boolean";
              break;
            case Primitive::INTEGER:
              output << "integer";
              break;
          }
        } else if constexpr (std::is_same_v<value_type, Function>) {
          output << "function (";
          bool first = true;
          for (const auto& parameter : node.parameters) {
            if (first) {
              first = false;
            } else {
              output << ", ";
            }
            output << parameter.type.value();
          }
          output << ") -> " << node.return_type;
        }
      });
  return output;
}

const AnyExpression& GetMeta(const Expression& expression) {
  return expression.visit(
      [](const auto& x) -> const AnyExpression& { return x; });
}

}  // namespace ast

template class visitable::Node<ast::StatementVisitor>;
template class visitable::Node<ast::TopLevelVisitor>;
