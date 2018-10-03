#pragma once

#include "reader.h"
#include "visitable.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace ast {

struct TypeVisitor;
using Type = visitable::Node<TypeVisitor>;

struct Boolean {};
struct Int {};

struct Function {
  Type return_type;
  std::vector<Type> parameters;
};

struct TypeVisitor {
  void Visit(const Type& type) { type.Visit(*this); }
  virtual void Visit(const Boolean&) = 0;
  virtual void Visit(const Int&) = 0;
  virtual void Visit(const Function&) = 0;
};

struct ExpressionVisitor;
using Expression = visitable::Node<ExpressionVisitor>;

struct AnyExpression {
  Reader::Location location;
  std::optional<Type> type = std::nullopt;
};

struct Identifier : AnyExpression {
  std::string name;
};

struct Integer : AnyExpression {
  std::int64_t value;
};

struct Add : AnyExpression {
  Expression left, right;
};

struct Subtract : AnyExpression {
  Expression left, right;
};

struct Multiply : AnyExpression {
  Expression left, right;
};

struct Divide : AnyExpression {
  Expression left, right;
};

struct FunctionCall : AnyExpression {
  Identifier function;
  std::vector<Expression> arguments;
};

struct CompareEq : AnyExpression {
  Expression left, right;
};

struct CompareNe : AnyExpression {
  Expression left, right;
};

struct CompareLe : AnyExpression {
  Expression left, right;
};

struct CompareLt : AnyExpression {
  Expression left, right;
};

struct CompareGe : AnyExpression {
  Expression left, right;
};

struct CompareGt : AnyExpression {
  Expression left, right;
};

struct LogicalNot : AnyExpression {
  Expression argument;
};

struct LogicalAnd : AnyExpression {
  Expression left, right;
};

struct LogicalOr : AnyExpression {
  Expression left, right;
};

struct ExpressionVisitor {
  void Visit(const Expression& expression) { expression.Visit(*this); }
  virtual void Visit(const Identifier&) = 0;
  virtual void Visit(const Integer&) = 0;
  virtual void Visit(const Add&) = 0;
  virtual void Visit(const Subtract&) = 0;
  virtual void Visit(const Multiply&) = 0;
  virtual void Visit(const Divide&) = 0;
  virtual void Visit(const FunctionCall&) = 0;
  virtual void Visit(const CompareEq&) = 0;
  virtual void Visit(const CompareNe&) = 0;
  virtual void Visit(const CompareLe&) = 0;
  virtual void Visit(const CompareLt&) = 0;
  virtual void Visit(const CompareGe&) = 0;
  virtual void Visit(const CompareGt&) = 0;
  virtual void Visit(const LogicalNot&) = 0;
  virtual void Visit(const LogicalAnd&) = 0;
  virtual void Visit(const LogicalOr&) = 0;
};

struct StatementVisitor;
using Statement = visitable::Node<StatementVisitor>;

struct AnyStatement {
  Reader::Location location;
};

struct DefineVariable : AnyStatement {
  std::string name;
  Expression value;
};

struct Assign : AnyStatement {
  std::string variable;
  Expression value;
};

struct DoFunction : AnyStatement {
  FunctionCall function_call;
};

struct If : AnyStatement {
  Expression condition;
  std::vector<Statement> if_true, if_false;
};

struct While : AnyStatement {
  Expression condition;
  std::vector<Statement> body;
};

struct ReturnVoid : AnyStatement {};

struct Return : AnyStatement {
  Expression value;
};

struct StatementVisitor {
  void Visit(const Statement& statement) { statement.Visit(*this); }
  virtual void Visit(const DefineVariable&) = 0;
  virtual void Visit(const Assign&) = 0;
  virtual void Visit(const DoFunction&) = 0;
  virtual void Visit(const If&) = 0;
  virtual void Visit(const While&) = 0;
  virtual void Visit(const ReturnVoid&) = 0;
  virtual void Visit(const Return&) = 0;
};

struct TopLevelVisitor;
using TopLevel = visitable::Node<TopLevelVisitor>;

struct AnyTopLevel {
  Reader::Location location;
};

struct DefineFunction : AnyTopLevel {
  std::string name;
  std::vector<std::string> parameters;
  std::vector<Statement> body;
};

struct TopLevelVisitor {
  virtual void Visit(const DefineFunction&) = 0;
  virtual void Visit(const std::vector<DefineFunction>&) = 0;
};

}  // namespace ast

extern template class visitable::Node<ast::ExpressionVisitor>;
extern template class visitable::Node<ast::StatementVisitor>;
