#include "ast.h"

#include <sstream>

namespace ast {
namespace {

template <typename F>
class LambdaTypeVisitor : public TypeVisitor {
 public:
  LambdaTypeVisitor(F* functor) : functor_(functor) {}
  using TypeVisitor::Visit;
  void Visit(const Void& v) override { (*functor_)(v); }
  void Visit(const Primitive& p) override { (*functor_)(p); }
  void Visit(const Function& f) override { (*functor_)(f); }
 private:
  F* functor_;
};

template <typename F>
class LambdaExpressionVisitor : public ExpressionVisitor {
 public:
  LambdaExpressionVisitor(F* functor) : functor_(functor) {}
  using ExpressionVisitor::Visit;
  void Visit(const Identifier& i) override { (*functor_)(i); }
  void Visit(const Integer& i) override { (*functor_)(i); }
  void Visit(const Binary& b) override { (*functor_)(b); }
  void Visit(const FunctionCall& f) override { (*functor_)(f); }
  void Visit(const LogicalNot& l) override { (*functor_)(l); }
 private:
  F* functor_;
};

}  // namespace

bool operator==(const Function& left, const Function& right) {
  return left.return_type == right.return_type &&
         left.parameters == right.parameters;
}

bool operator==(const Type& left, const Type& right) {
  bool result = false;
  auto left_visit = [&](const auto& left_type) {
    auto right_visit = [&](const auto& right_type) {
      result = std::is_same_v<decltype(left_type), decltype(right_type)> &&
               left_type == right_type;
    };
    LambdaTypeVisitor visitor{&right_visit};
    right.Visit(visitor);
  };
  LambdaTypeVisitor visitor{&left_visit};
  left.Visit(visitor);
  return result;
}

bool IsValueType(const Type& type) {
  bool result = false;
  class Visitor : public TypeVisitor {
   public:
    Visitor(bool* result) : result_(result) {}
    void Visit(const Void&) override { *result_ = false; }
    void Visit(const Primitive&) override { *result_ = true; }
    void Visit(const Function&) override { *result_ = false; }
   private:
    bool* result_;
  };
  Visitor visitor{&result};
  type.Visit(visitor);
  return result;
}

bool IsArithmeticType(const Type& type) {
  return type == Primitive::INTEGER;
}

const Function* GetFunctionType(const Type& type) {
  const Function* output = nullptr;
  auto get_function = [&](const auto& node) {
    if constexpr (std::is_same_v<std::decay_t<decltype(node)>, Function>) {
      output = &node;
    }
  };
  LambdaTypeVisitor visitor{&get_function};
  visitor.Visit(type);
  return output;
}

std::ostream& operator<<(std::ostream& output, const Type& type) {
  class TypeNameVisitor : public TypeVisitor {
   public:
    TypeNameVisitor(std::ostream* output) : output_(output) {}
    void Visit(const Void&) { *output_ << "void"; }
    void Visit(const Primitive& primitive) {
      switch (primitive) {
        case Primitive::BOOLEAN: *output_ << "boolean"; break;
        case Primitive::INTEGER: *output_ << "integer"; break;
      }
    }
    void Visit(const Function& function) {
      *output_ << "function (";
      bool first = true;
      for (const auto& parameter : function.parameters) {
        if (first) {
          first = false;
        } else {
          *output_ << ", ";
        }
        *output_ << parameter;
      }
      *output_ << ") -> " << function.return_type;
    }
   private:
    std::ostream* output_;
  };
  TypeNameVisitor visitor{&output};
  type.Visit(visitor);
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
