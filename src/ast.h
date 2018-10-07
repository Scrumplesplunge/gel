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

struct Void {};

enum class Primitive {
  BOOLEAN,
  INTEGER,
};

struct Identifier;
struct Function {
  Type return_type;
  std::vector<Identifier> parameters;
};

struct TypeVisitor {
  void Visit(const Type& type) { type.Visit(*this); }
  virtual void Visit(const Void&) = 0;
  virtual void Visit(const Primitive&) = 0;
  virtual void Visit(const Function&) = 0;
};

inline bool operator==(Void, Void) { return true; }
bool operator==(const Function& left, const Function& right);
bool operator==(const Type& left, const Type& right);
bool IsArithmeticType(const Type& type);
bool IsValueType(const Type& type);
const Function* GetFunctionType(const Type& type);
std::ostream& operator<<(std::ostream& output, const Type& type);

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

struct Binary : AnyExpression {
  enum Operation {
    ADD,
    COMPARE_EQ,
    COMPARE_GE,
    COMPARE_GT,
    COMPARE_LE,
    COMPARE_LT,
    COMPARE_NE,
    DIVIDE,
    LOGICAL_AND,
    LOGICAL_OR,
    MULTIPLY,
    SUBTRACT,
  };
  Operation operation;
  Expression left, right;
};

struct FunctionCall : AnyExpression {
  Identifier function;
  std::vector<Expression> arguments;
};

struct LogicalNot : AnyExpression {
  Expression argument;
};

struct ExpressionVisitor {
  void Visit(const Expression& expression) { expression.Visit(*this); }
  virtual void Visit(const Identifier&) = 0;
  virtual void Visit(const Integer&) = 0;
  virtual void Visit(const Binary&) = 0;
  virtual void Visit(const FunctionCall&) = 0;
  virtual void Visit(const LogicalNot&) = 0;
};

const AnyExpression& GetMeta(const Expression& expression);

struct StatementVisitor;
using Statement = visitable::Node<StatementVisitor>;

struct AnyStatement {
  Reader::Location location;
};

struct DefineVariable : AnyStatement {
  ast::Identifier variable;
  Expression value;
};

struct Assign : AnyStatement {
  ast::Identifier variable;
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
  ast::Identifier function;
  std::vector<ast::Identifier> parameters;
  std::vector<Statement> body;
};

struct TopLevelVisitor {
  virtual void Visit(const DefineFunction&) = 0;
  virtual void Visit(const std::vector<DefineFunction>&) = 0;
};

}  // namespace ast

extern template class visitable::Node<ast::ExpressionVisitor>;
extern template class visitable::Node<ast::StatementVisitor>;
extern template class visitable::Node<ast::TopLevelVisitor>;
