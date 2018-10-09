#include "ast.h"

#include <sstream>

namespace ast {
namespace {

template <typename F>
class LambdaExpressionVisitor : public ExpressionVisitor {
 public:
  LambdaExpressionVisitor(F* functor) : functor_(functor) {}
  using ExpressionVisitor::Visit;
  void Visit(const Identifier& i) override { (*functor_)(i); }
  void Visit(const Boolean& b) override { (*functor_)(b); }
  void Visit(const Integer& i) override { (*functor_)(i); }
  void Visit(const Binary& b) override { (*functor_)(b); }
  void Visit(const FunctionCall& f) override { (*functor_)(f); }
  void Visit(const LogicalNot& l) override { (*functor_)(l); }
 private:
  F* functor_;
};

}  // namespace

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
  const AnyExpression* meta = nullptr;
  auto get_meta = [&](const auto& node) { meta = &node; };
  LambdaExpressionVisitor visitor{&get_meta};
  visitor.Visit(expression);
  if (meta == nullptr) throw std::logic_error("Visit failed.");
  return *meta;
}

}  // namespace ast

template class visitable::Node<ast::ExpressionVisitor>;
template class visitable::Node<ast::StatementVisitor>;
template class visitable::Node<ast::TopLevelVisitor>;
