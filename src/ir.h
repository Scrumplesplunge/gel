#pragma once

#include "one_of.h"
#include "types.h"

#include <cstdint>
#include <map>

namespace ir {

enum class Variable : std::uint64_t {};
enum class Label : std::uint64_t {};

enum class Integer : std::int_least64_t {};
enum class Boolean : bool {};
struct Arithmetic;
struct Compare;
struct Call;
struct LetIn;
using Expression = one_of<Integer, Boolean, Arithmetic, Compare, Call, LetIn>;

struct Arithmetic {
  enum Operation {
    ADD,
    DIVIDE,
    MULTIPLY,
    SUBTRACT,
  };
  Operation operation;
  Variable left, right;
};

struct Compare {
  enum Operation {
    EQUAL,
    GREATER_OR_EQUAL,
    GREATER_THAN,
    LESS_OR_EQUAL,
    LESS_THAN,
    NOT_EQUAL,
  };
  Operation operation;
  Variable left, right;
};

struct Call {
  Operation;
  std::vector<Variable> inputs;
};

struct LetIn {
  Variable variable;
  Expression initializer;
  Expression consumer;
};

}  // namespace ir
