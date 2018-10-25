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

enum class Arithmetic {
  ADD,
  DIVIDE,
  MULTIPLY,
  SUBTRACT,
};

enum class Compare {
  EQUAL,
  GREATER_OR_EQUAL,
  GREATER_THAN,
  LESS_OR_EQUAL,
  LESS_THAN,
  NOT_EQUAL,
};

enum class Logical {
  AND,
  OR,
};

template <typename Metadata>
struct Ast {
  using ExpressionMetadata = typename Metadata::Expression;
  using StatementMetadata = typename Metadata::Statement;
  using TopLevelMetadata = typename Metadata::TopLevel;

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

  struct Identifier : ExpressionMetadata {
    std::string name;
  };

  struct Boolean : ExpressionMetadata {
    bool value;
  };

  struct Integer : ExpressionMetadata {
    std::int64_t value;
  };

  struct ArrayLiteral : ExpressionMetadata {
    std::vector<Expression> parts;
  };

  struct Arithmetic : ExpressionMetadata {
    ::ast::Arithmetic operation;
    Expression left, right;
  };

  struct Compare : ExpressionMetadata {
    ::ast::Compare operation;
    Expression left, right;
  };

  struct Logical : ExpressionMetadata {
    ::ast::Logical operation;
    Expression left, right;
  };

  struct FunctionCall : ExpressionMetadata {
    std::string function;
    std::vector<Expression> arguments;
  };

  struct LogicalNot : ExpressionMetadata {
    Expression argument;
  };

  struct DefineVariable;
  struct Assign;
  struct DoFunction;
  struct If;
  struct While;
  struct ReturnVoid;
  struct Return;
  using Statement =
      one_of<DefineVariable, Assign, DoFunction, If, While, ReturnVoid, Return>;

  struct DefineVariable : StatementMetadata {
    Identifier variable;
    Expression value;
  };

  struct Assign : StatementMetadata {
    Identifier variable;
    Expression value;
  };

  struct DoFunction : StatementMetadata {
    FunctionCall function_call;
  };

  struct If : StatementMetadata {
    Expression condition;
    std::vector<Statement> if_true, if_false;
  };

  struct While : StatementMetadata {
    Expression condition;
    std::vector<Statement> body;
  };

  struct ReturnVoid : StatementMetadata {};

  struct Return : StatementMetadata {
    Expression value;
  };

  struct DefineFunction;
  using TopLevel = one_of<DefineFunction, std::vector<DefineFunction>>;

  struct DefineFunction : TopLevelMetadata {
    types::Function type;
    std::string name;
    std::vector<Identifier> parameters;
    std::vector<Statement> body;
  };

  static const ExpressionMetadata& GetMeta(const Expression& expression);
  static const StatementMetadata& GetMeta(const Statement& statement);
  static const TopLevelMetadata& GetMeta(const TopLevel& top_level);
};

}  // namespace ast

#include "ast.inl.h"
