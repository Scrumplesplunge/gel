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

bool operator<(const Function& left, const Function& right) {
  if (left.return_type < right.return_type) return true;
  if (left.return_type != right.return_type) return false;
  return std::lexicographical_compare(
      left.parameters.begin(), left.parameters.end(),
      right.parameters.begin(), right.parameters.end(),
      [](const auto& l, const auto& r) { return l.type < r.type; });
}

std::ostream& operator<<(std::ostream& output, const Type& type) {
  type.visit([&](const auto& node) {
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
