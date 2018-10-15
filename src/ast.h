#pragma once

#include "one_of.h"
#include "reader.h"
#include "types.h"
#include "value.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace ast {

struct Identifier;
struct Boolean;
struct Integer;
struct ArrayLiteral;
struct Arithmetic;
struct Compare;
struct Logical;
struct FunctionCall;
struct LogicalNot;
using Expression =
    one_of<Identifier, Boolean, Integer, ArrayLiteral, Arithmetic, Compare,
           Logical, FunctionCall, LogicalNot>;

struct AnyExpression {
  Reader::Location location;
  std::optional<types::Type> type = std::nullopt;
};

struct Identifier : AnyExpression {
  std::string name;
};

struct Boolean : AnyExpression {
  bool value;
};

struct Integer : AnyExpression {
  std::int64_t value;
};

struct ArrayLiteral : AnyExpression {
  std::vector<Expression> parts;
};

struct Arithmetic : AnyExpression {
  enum Operation {
    ADD,
    DIVIDE,
    MULTIPLY,
    SUBTRACT,
  };
  Operation operation;
  Expression left, right;
};

struct Compare : AnyExpression {
  enum Operation {
    EQUAL,
    GREATER_OR_EQUAL,
    GREATER_THAN,
    LESS_OR_EQUAL,
    LESS_THAN,
    NOT_EQUAL,
  };
  Operation operation;
  Expression left, right;
};

struct Logical : AnyExpression {
  enum Operation {
    AND,
    OR,
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

const AnyExpression& GetMeta(const Expression& expression);

struct DefineVariable;
struct Assign;
struct DoFunction;
struct If;
struct While;
struct ReturnVoid;
struct Return;
using Statement =
    one_of<DefineVariable, Assign, DoFunction, If, While, ReturnVoid, Return>;

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

struct DefineFunction;
using TopLevel = one_of<DefineFunction, std::vector<DefineFunction>>;

struct AnyTopLevel {
  Reader::Location location;
};

struct DefineFunction : AnyTopLevel {
  ast::Identifier function;
  std::vector<ast::Identifier> parameters;
  std::vector<Statement> body;
};

}  // namespace ast
