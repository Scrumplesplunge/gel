#pragma once

#include "visitable.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace ast {

struct ExpressionVisitor;
using Expression = visitable::Node<ExpressionVisitor>;

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
using Statement = visitable::Node<StatementVisitor>;

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

extern template class visitable::Node<ast::ExpressionVisitor>;
extern template class visitable::Node<ast::StatementVisitor>;
