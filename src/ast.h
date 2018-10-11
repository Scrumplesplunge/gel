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
struct Array;
struct Function;
using Type = one_of<Void, Primitive, Array, Function>;

struct Void {};

enum class Primitive {
  BOOLEAN,
  INTEGER,
};

struct Array {
  ast::Type element_type;
};

struct Identifier;
struct Function {
  Type return_type;
  std::vector<Identifier> parameters;
};

bool IsValueType(const ast::Type& type);

inline bool operator==(Void, Void) { return true; }
inline bool operator!=(Void, Void) { return false; }
inline bool operator>(Void, Void) { return false; }
inline bool operator>=(Void, Void) { return true; }
inline bool operator<(Void, Void) { return false; }
inline bool operator<=(Void, Void) { return true; }
inline bool operator==(const Array& l, const Array& r) {
  return l.element_type == r.element_type;
}
inline bool operator!=(const Array& l, const Array& r) {
  return l.element_type != r.element_type;
}
inline bool operator>(const Array& l, const Array& r) {
  return l.element_type > r.element_type;
}
inline bool operator>=(const Array& l, const Array& r) {
  return l.element_type >= r.element_type;
}
inline bool operator<(const Array& l, const Array& r) {
  return l.element_type < r.element_type;
}
inline bool operator<=(const Array& l, const Array& r) {
  return l.element_type <= r.element_type;
}
bool operator==(const Function& left, const Function& right);
inline bool operator!=(const Function& left, const Function& right) {
  return !(left == right);
}
bool operator<(const Function& left, const Function& right);
inline bool operator<=(const Function& left, const Function& right) {
  return !(right < left);
}
inline bool operator>=(const Function& left, const Function& right) {
  return !(left < right);
}
inline bool operator>(const Function& left, const Function& right) {
  return right < left;
}
std::ostream& operator<<(std::ostream& output, const Type& type);

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
