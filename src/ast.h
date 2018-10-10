#pragma once

#include "one_of.h"
#include "reader.h"
#include "value.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace ast {

struct Void;
enum class Primitive;
struct Function;
using Type = one_of<Void, Primitive, Function>;

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

inline bool operator==(Void, Void) { return true; }
inline bool operator!=(Void, Void) { return false; }
bool operator==(const Function& left, const Function& right);
inline bool operator!=(const Function& left, const Function& right) {
  return !(left == right);
}
std::ostream& operator<<(std::ostream& output, const Type& type);

struct Identifier;
struct Boolean;
struct Integer;
struct Binary;
struct FunctionCall;
struct LogicalNot;
using Expression =
    one_of<Identifier, Boolean, Integer, Binary, FunctionCall, LogicalNot>;

struct AnyExpression {
  Reader::Location location;
  std::optional<Type> type = std::nullopt;
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
