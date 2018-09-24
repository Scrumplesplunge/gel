#pragma once

#include "visitable.h"

#include <cstdint>
#include <vector>

namespace op {

struct Visitor;
using Node = visitable::Node<Visitor>;
using Sequence = std::vector<Node>;

// Push an integer.
struct Integer { std::int64_t value; };

// Push an address relative to the stack frame.
struct Frame { int offset; };

// Adjust the stack by the given amount. A negative number reserves space, while
// a positive number returns space.
struct Adjust { int size; };

// Pop an address, push the value stored at that address.
struct Load {};

// Pop a value, pop an address, store the value to the address.
struct Store {};

// Binary operations that consume two stack entries and produce one. The top of
// the stack is the right hand side.
struct Add {};
struct Subtract {};
struct Multiply {};
struct Divide {};

// Comparison operations. The top of the stack is the right hand side.
struct CompareEq {};
struct CompareNe {};
struct CompareLe {};
struct CompareLt {};
struct CompareGe {};
struct CompareGt {};

// Control flow operations.
struct Label { std::string name; };
struct Jump { std::string label; };

// Conditionally jump. If the branch is taken, the condition value remains on
// the stack. Otherwise, it is removed.
struct JumpIfZero { std::string label; };
struct JumpIfNonZero { std::string label; };

struct Visitor {
  void Visit(const Node& node) { node.Visit(*this); }
  virtual void Visit(const Sequence&) = 0;
  virtual void Visit(const Integer&) = 0;
  virtual void Visit(const Frame&) = 0;
  virtual void Visit(const Adjust&) = 0;
  virtual void Visit(const Load&) = 0;
  virtual void Visit(const Store&) = 0;
  virtual void Visit(const Add&) = 0;
  virtual void Visit(const Subtract&) = 0;
  virtual void Visit(const Multiply&) = 0;
  virtual void Visit(const Divide&) = 0;
  virtual void Visit(const CompareEq&) = 0;
  virtual void Visit(const CompareNe&) = 0;
  virtual void Visit(const CompareLe&) = 0;
  virtual void Visit(const CompareLt&) = 0;
  virtual void Visit(const CompareGe&) = 0;
  virtual void Visit(const CompareGt&) = 0;
  virtual void Visit(const Label&) = 0;
  virtual void Visit(const Jump&) = 0;
  virtual void Visit(const JumpIfZero&) = 0;
  virtual void Visit(const JumpIfNonZero&) = 0;
};

}  // namespace op

extern template class visitable::Node<op::Visitor>;
