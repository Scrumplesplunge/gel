#pragma once

#include <memory>
#include <type_traits>
#include <vector>

namespace ast {

template <typename Visitor>
class AstNode {
 public:
  // requires visitor->visit(T) to be valid.
  template <typename T, typename = std::enable_if_t<
                            !std::is_same_v<AstNode, std::decay_t<T>>>>
  AstNode(T&& value) : model_(Adapt(std::forward<T>(value))) {}

  void Visit(Visitor& visitor) const;

 private:
  struct Model;

  template <typename T>
  static std::shared_ptr<const Model> Adapt(T&& value);

  std::shared_ptr<const Model> model_;
};

struct ExpressionVisitor;
using Expression = AstNode<ExpressionVisitor>;

struct Identifier { std::string_view name; };
struct Integer { std::int64_t value; };
struct Add { Expression left, right; };
struct Subtract { Expression left, right; };
struct Multiply { Expression left, right; };
struct Divide { Expression left, right; };
struct FunctionCall { Identifier function; std::vector<Expression> arguments; };

struct ExpressionVisitor {
  virtual void Visit(const Identifier&) = 0;
  virtual void Visit(const Integer&) = 0;
  virtual void Visit(const Add&) = 0;
  virtual void Visit(const Subtract&) = 0;
  virtual void Visit(const Multiply&) = 0;
  virtual void Visit(const Divide&) = 0;
  virtual void Visit(const FunctionCall&) = 0;
};

struct StatementVisitor;
using Statement = AstNode<StatementVisitor>;

struct DeclareVariable { Identifier identifier; Expression value; };
struct Assign { Identifier identifier; Expression value; };
struct DoFunction { FunctionCall function_call; };
struct If { Expression condition; std::vector<Statement> if_true, if_false; };

struct StatementVisitor {
  virtual void Visit(const DeclareVariable&) = 0;
  virtual void Visit(const Assign&) = 0;
  virtual void Visit(const DoFunction&) = 0;
  virtual void Visit(const If&) = 0;
};

}  // namespace ast

#include "ast.inl.h"
